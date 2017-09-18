///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2017 Ericsson Telecom AB
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////
//
//  File:               IPL4asp_PT.cc
//  Rev:                R25B
//  Prodnr:             CNL 113 531
//  Contact:            http://ttcn.ericsson.se
//  Reference:
//  DTLS references: http://www.net-snmp.org/wiki/index.php/DTLS_Implementation_Notes
//                   http://sctp.fh-muenster.de & http://sctp.fh-muenster.de/DTLS.pdf

#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <limits.h>
#include <time.h>
#include "IPL4asp_PT.hh"
#include "IPL4asp_PortType.hh"
#include "Socket_API_Definitions.hh"

#define  IPL4_SCTP_WHOLE_MESSAGE_RECEIVED 0
#define  IPL4_SCTP_EOF_RECEIVED 1
#define  IPL4_SCTP_ERROR_RECEIVED 2
#define  IPL4_SCTP_PARTIAL_RECEIVE 3
#define  IPL4_SCTP_SENDER_DRY_EVENT 4

#define  IPL4_IPV4_ANY_ADDR "0.0.0.0"
#define  IPL4_IPV6_ANY_ADDR "::"


//SSL
#ifdef IPL4_USE_SSL
#define AS_SSL_CHUNCK_SIZE 16384
// character buffer length to store temporary SSL informations, 256 is usually enough
#define SSL_CHARBUF_LENGTH 256
// number of bytes to read from the random devices
#define SSL_PRNG_LENGTH 1024
#if defined(BIO_CTRL_DGRAM_SCTP_GET_RCVINFO) && defined(BIO_CTRL_DGRAM_SCTP_SET_SNDINFO)
#define OPENSSL_SCTP_SUPPORT
#endif

// is DTLS_mehod available?
#ifndef SSL_OP_NO_DTLSv1_2
#define DTLS_client_method DTLSv1_client_method
#define DTLS_server_method DTLSv1_server_method
#endif

static int current_conn_id=-1;

#if OPENSSL_VERSION_NUMBER >= 0x1000200fL

static int ipl4_tls_alpn_cb (SSL *ssl, const unsigned char **out,
                                            unsigned char *outlen,
                                            const unsigned char *in,
                                            unsigned int inlen,
                                            void *arg){

  IPL4asp__PortType::IPL4asp__PT_PROVIDER* tp=(IPL4asp__PortType::IPL4asp__PT_PROVIDER*)arg;
  if(!tp->isConnIdValid(current_conn_id)){
    return SSL_TLSEXT_ERR_NOACK;
  }
  
  if(!tp->sockList[current_conn_id].alpn){
    return SSL_TLSEXT_ERR_NOACK;
  }

    if (SSL_select_next_proto
        ((unsigned char **)out, outlen, (const unsigned char*)(*tp->sockList[current_conn_id].alpn), 
         tp->sockList[current_conn_id].alpn->lengthof(), in,
         inlen) != OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_NOACK;
    }


  return SSL_TLSEXT_ERR_OK;

}
#endif

#endif

#ifdef USE_IPL4_EIN_SCTP

#include <pthread.h>

//const IPL4asp__Types::OptionList empty_IPL4asp__Types::OptionList(NULL_VALUE);

#endif

using namespace IPL4asp__Types;

namespace IPL4asp__PortType {

#define SET_OS_ERROR_CODE (result.os__error__code()() = errno)
#define RETURN_ERROR(code) {              \
    result.errorCode()() = PortError::code; \
    if (result.os__error__code().ispresent()) \
    result.os__error__text()() = strerror((int)result.os__error__code()()); \
    ASP__Event event; \
    event.result() = result; \
    if (portRef.globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) \
    portRef.incoming_message(event);  \
    return result;  \
}
#define RETURN_ERROR_STACK(code) {              \
    result.errorCode()() = PortError::code; \
    if (result.os__error__code().ispresent()) \
    result.os__error__text()() = strerror((int)result.os__error__code()()); \
    ASP__Event event; \
    event.result() = result; \
    if (globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) \
    incoming_message(event);  \
    return result;  \
}


#define IPL4_PORTREF_DEBUG(p,...) if(p.debugAllowed) {p.debug(__VA_ARGS__);}
#define IPL4_DEBUG(...) if(debugAllowed) {debug(__VA_ARGS__);}


#ifdef LKSCTP_MULTIHOMING_ENABLED
int my_sctp_connectx(int sd, struct sockaddr * addrs, int addrcnt){

#ifdef SCTP_SOCKOPT_CONNECTX_OLD
  return sctp_connectx(sd, addrs, addrcnt,NULL);
#else
  return sctp_connectx(sd, addrs, addrcnt);
#endif
}
#endif

inline void IPL4asp__PT_PROVIDER::testIfInitialized() const {
  if (!mapped) {
    TTCN_error("IPL4 Test Port not mapped");
  }
}


INTEGER simpleGetMsgLen(const OCTETSTRING& stream, ro__integer& /*args*/)
{
  return stream.lengthof();
} // simpleGetMsgLen


int SetLocalSockAddr(const char* debug_str, IPL4asp__PT_PROVIDER& portRef,
    int def_addr_family,
    const char *locName, int locPort,
    SockAddr& sockAddr, socklen_t& sockAddrLen){
  int hp=0;
  bool locName_empty=(strlen(locName)==0);
  const char* def_loc_host;
  if(strlen(portRef.defaultLocHost)==0){
    if(def_addr_family==AF_INET6){
      def_loc_host= IPL4_IPV6_ANY_ADDR;
    } else {
      def_loc_host= IPL4_IPV4_ANY_ADDR;
    }
  } else {
    def_loc_host=portRef.defaultLocHost;
  }
  IPL4_PORTREF_DEBUG(portRef, "SetLocalSockAddr: locName: %s loc_port %d def_loc_host %s, add_family %s", locName, locPort,def_loc_host,
      def_addr_family==AF_INET6?"AF_INET6":"AF_INET");

  if (locPort != -1 && !locName_empty)
    hp = SetSockAddr(locName, locPort, sockAddr, sockAddrLen);
  else if (locPort == -1 && locName_empty) { // use default host and port
    IPL4_PORTREF_DEBUG(portRef, "%s: use defaults: %s:%d", debug_str,
        def_loc_host, portRef.defaultLocPort);
    hp = SetSockAddr(def_loc_host,
        portRef.defaultLocPort, sockAddr, sockAddrLen);
  } else if (locPort == -1) { // use default port
    IPL4_PORTREF_DEBUG(portRef, "%s: use default port: %s:%d",  debug_str,
        locName, portRef.defaultLocPort);
    hp = SetSockAddr(locName, portRef.defaultLocPort,
        sockAddr, sockAddrLen);
  } else { // use default host
    IPL4_PORTREF_DEBUG(portRef, "%s: use default host: %s:%d", debug_str,
        def_loc_host, locPort);
    hp = SetSockAddr(def_loc_host,
        locPort, sockAddr, sockAddrLen);
  }
  return hp;
}

int SetSockAddr(const char *name, int port,
    SockAddr& sa, socklen_t& saLen) {

  //int err = 0;
  int addrtype = -1;
  struct sockaddr_in saddr;
#ifdef USE_IPV6
  struct sockaddr_in6 saddr6;
#endif
  if(inet_pton(AF_INET, name, &(saddr.sin_addr))) {
    memset(&sa.v4, 0, sizeof(sa.v4));
    saLen = sizeof(sa.v4);
    sa.v4.sin_family = AF_INET;
    sa.v4.sin_port = htons(port);
    memcpy(&sa.v4.sin_addr, &(saddr.sin_addr), sizeof(saddr.sin_addr));
    addrtype = AF_INET;
  }
#ifdef USE_IPV6
  else if(inet_pton(AF_INET6, name, &(saddr6.sin6_addr))) {
    memset(&sa.v6, 0, sizeof(sa.v6));
    saLen = sizeof(sa.v6);
    sa.v6.sin6_family = AF_INET6;
    sa.v6.sin6_port = htons(port);
    memcpy(&sa.v6.sin6_addr, &(saddr6.sin6_addr), sizeof(saddr6.sin6_addr));
    addrtype = AF_INET6;
  }
#endif
  else {

    struct addrinfo myaddr, *res;
    memset(&myaddr,0,sizeof(myaddr));
    myaddr.ai_flags = AI_ADDRCONFIG|AI_PASSIVE;
    myaddr.ai_socktype = SOCK_STREAM;
    myaddr.ai_protocol = 0;

//    if ((err = getaddrinfo(name, NULL, &myaddr, &res)) != 0) {
    if (getaddrinfo(name, NULL, &myaddr, &res) != 0) {
      //printf("SetSockAddr: getaddrinfo error: %i, %s", err, gai_strerror(err));
      return -1;
    }

    if (res->ai_addr->sa_family == AF_INET) { // IPv4
      struct sockaddr_in *saddr = (struct sockaddr_in *) res->ai_addr;
      memset(&sa.v4, 0, sizeof(sa.v4));
      saLen = sizeof(sa.v4);
      sa.v4.sin_family = AF_INET;
      sa.v4.sin_port = htons(port);
      memcpy(&sa.v4.sin_addr, &(saddr->sin_addr), sizeof(saddr->sin_addr));
      addrtype = AF_INET;
    }
#ifdef USE_IPV6
    else if (res->ai_addr->sa_family == AF_INET6){ // IPv6
      struct sockaddr_in6 *saddr = (struct sockaddr_in6 *) res->ai_addr;
      memset(&sa.v6, 0, sizeof(sa.v6));
      saLen = sizeof(sa.v6);
      memcpy(&sa.v6,saddr,saLen);
      sa.v6.sin6_port = htons(port);
      addrtype = AF_INET6;
    }
#endif
    else
    {
      //printf("sa_family not handled!");
    }
    freeaddrinfo(res);
  }
  return addrtype;
}



bool SetNameAndPort(SockAddr *sa, socklen_t saLen,
    HostName& name, PortNumber& port) {
  sa_family_t af;

  if (saLen == sizeof(sockaddr_in)) { // IPv4
    void *src;
    int vport;
    af = AF_INET;
    src = &sa->v4.sin_addr.s_addr;
    vport = sa->v4.sin_port;

    char dst[INET_ADDRSTRLEN];
    if (inet_ntop(af, src, dst, INET_ADDRSTRLEN) == NULL) {
      name = "?";
      port = -1;
      return false;
    } else
      name = CHARSTRING(dst);
    port = ntohs(vport);
    return true;
  }
#ifdef USE_IPV6
  else
  { // IPv6
    void *src;
    int vport;
    af = AF_INET6;
    src = &sa->v6.sin6_addr.s6_addr;
    vport = sa->v6.sin6_port;

    char dst[INET6_ADDRSTRLEN];
    if (inet_ntop(af, src, dst, INET6_ADDRSTRLEN) == NULL) {
      name = "?";
      port = -1;
      return false;
    } else
      name = CHARSTRING(dst);
    port = ntohs(vport);
    return true;
  }
#endif
  name = "?";
  port = -1;
  return false;
} // SetNameAndPort


#ifdef IPL4_USE_SSL
// cookie variables
unsigned char ssl_cookie_secret[IPL4_COOKIE_SECRET_LENGTH];
int ssl_cookie_initialized;          // do not initialize the cookie twice
#endif

IPL4asp__PT_PROVIDER::IPL4asp__PT_PROVIDER(const char *par_port_name)
: PORT(par_port_name) {
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::IPL4asp__PT_PROVIDER: enter");
  debugAllowed = false;
  mapped = false;
  sockListSize = SOCK_LIST_SIZE_MIN;
  defaultLocHost = memptystr();
  defaultLocPort = 9999;
  defaultRemHost = NULL;
  defaultRemPort = -1;
  default_mode = 0;
  default_proto = 0;
  backlog = SOMAXCONN;
  defaultGetMsgLen = simpleGetMsgLen;
  defaultGetMsgLen_forConnClosedEvent = simpleGetMsgLen; // by default we pass up to TTCN the remaining buffer content on connClosed
  defaultMsgLenArgs = new ro__integer(NULL_VALUE);
  defaultMsgLenArgs_forConnClosedEvent = new ro__integer(NULL_VALUE);
  pureNonBlocking = false;
  poll_timeout = -1;
  max_num_of_poll =-1;
  lonely_conn_id = -1;
  lazy_conn_id_level = 0;
  sctp_PMTU_size = 0;
  broadcast = false;
  ssl_cert_per_conn = false;
  send_extended_result = false;
#ifdef USE_SCTP
  (void) memset(&initmsg, 0, sizeof(struct sctp_initmsg));
  initmsg.sinit_num_ostreams = 64;
  initmsg.sinit_max_instreams = 64;
  initmsg.sinit_max_attempts = 0;
  initmsg.sinit_max_init_timeo = 0;
  (void) memset(&events, 0, sizeof (events));
  events.sctp_data_io_event = TRUE;
  events.sctp_association_event = TRUE;
  events.sctp_address_event = TRUE;
  events.sctp_send_failure_event = TRUE;
  events.sctp_peer_error_event = TRUE;
  events.sctp_shutdown_event = TRUE;
  events.sctp_partial_delivery_event = TRUE;
#ifdef SCTP_SENDER_DRY_EVENT
  events.sctp_sender_dry_event = FALSE;
#endif
#ifdef LKSCTP_1_0_7
  events.sctp_adaptation_layer_event = TRUE;
#elif defined LKSCTP_1_0_9
  events.sctp_adaptation_layer_event = TRUE;
  events.sctp_authentication_event = FALSE;
#else
  events.sctp_adaption_layer_event = TRUE;
#endif
#endif
#ifdef USE_IPL4_EIN_SCTP
  native_stack = TRUE;
  sctpInstanceId=0;
  userInstanceId=0;
  userInstanceIdSpecified=false;
  sctpInstanceIdSpecified=false;
  userId=USER01_ID;
  cpManagerIPA="";
#endif
  //SSL
#ifdef IPL4_USE_SSL
  ssl_initialized=false;
  ssl_key_file=NULL;
  ssl_certificate_file=NULL;
  ssl_trustedCAlist_file=NULL;
  ssl_cipher_list=NULL;
  ssl_verify_certificate=false;
  ssl_use_session_resumption=true;
  ssl_session=NULL;
  ssl_password=NULL;
  ssl_ctx = NULL;
  ssl_reconnect_attempts = 5;
  ssl_reconnect_delay = 10000; //in milisec, so by default 0.01sec
  memset(ssl_cookie_secret, 0, IPL4_COOKIE_SECRET_LENGTH);
  ssl_cookie_initialized = 0;
#endif
} // IPL4asp__PT_PROVIDER::IPL4asp__PT_PROVIDER



IPL4asp__PT_PROVIDER::~IPL4asp__PT_PROVIDER()
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::~IPL4asp__PT_PROVIDER: enter");
  delete defaultMsgLenArgs;
  delete defaultMsgLenArgs_forConnClosedEvent;
  
  Free(defaultLocHost);
  Free(defaultRemHost);
  // now SSL context can be removed
#ifdef IPL4_USE_SSL
  if (ssl_ctx!=NULL) {
    SSL_CTX_free(ssl_ctx);
  }
  if (ssl_dtls_server_ctx!=NULL) {
    SSL_CTX_free(ssl_dtls_server_ctx);
  }
  if (ssl_dtls_client_ctx!=NULL) {
    SSL_CTX_free(ssl_dtls_client_ctx);
  }
  delete [] ssl_key_file;
  delete [] ssl_certificate_file;
  delete [] ssl_trustedCAlist_file;
  delete [] ssl_cipher_list;
  delete [] ssl_password;
#endif
} // IPL4asp__PT_PROVIDER::~IPL4asp__PT_PROVIDER



void IPL4asp__PT_PROVIDER::debug(const char *fmt, ...) { // DO NOT CALL DIRECTLY, use macro IPL4_PORTREF_DEBUG or IPL4_DEBUG
  //  if (debugAllowed) {
  TTCN_Logger::begin_event(TTCN_DEBUG);
  TTCN_Logger::log_event("%s: ", get_name());
  va_list args;
  va_start(args, fmt);
  TTCN_Logger::log_event_va_list(fmt, args);
  va_end(args);
  TTCN_Logger::end_event();
  //  }
} // IPL4asp__PT_PROVIDER::debug



void IPL4asp__PT_PROVIDER::set_parameter(const char *parameter_name,
    const char *parameter_value)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_parameter: enter (name: %s, value: %s)",
      parameter_name, parameter_value);
  if (!strcmp(parameter_name, "debug")) {
    if (!strcasecmp(parameter_value,"YES")) {
      debugAllowed = true;
      ipDiscConfig.debugAllowed = true;
    }
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_parameter: enter (name: %s, value: %s)",
        parameter_name, parameter_value);
  } else if (!strcmp(parameter_name, "max_num_of_poll")) {
    max_num_of_poll = atoi(parameter_value);
  } else if (!strcmp(parameter_name, "poll_timeout")) {
    poll_timeout = atoi(parameter_value);
  } else if (!strcmp(parameter_name, "defaultListeningPort")) {
    defaultLocPort = atoi(parameter_value);
  } else if (!strcmp(parameter_name, "defaultListeningHost")) {
    if(defaultLocHost){
      Free(defaultLocHost);
    }
    defaultLocHost = mcopystr(parameter_value);
  } else if (!strcasecmp(parameter_name, "map_behavior")) {
    if(!strcasecmp(parameter_value, "connect")){
      default_mode = 1;
    } else if(!strcasecmp(parameter_value, "listen")) {
      default_mode = 2;
    } else {
      default_mode = 0;
    }
    
  } else if (!strcasecmp(parameter_name, "map_protocol")) {
    if(!strcasecmp(parameter_value, "tcp")){
      default_proto = 0;
    } else if(!strcasecmp(parameter_value, "tls")) {
      default_proto = 1;
    } else if(!strcasecmp(parameter_value, "sctp")) {
      default_proto = 2;
    } else if(!strcasecmp(parameter_value, "udp")) {
      default_proto = 3;
    }    
  } else if (!strcasecmp(parameter_name, "RemotePort")) {
    defaultRemPort = atoi(parameter_value);
  } else if (!strcasecmp(parameter_name, "RemoteHost")) {
    if(defaultRemHost){
      Free(defaultRemHost);
    }
    defaultRemHost = mcopystr(parameter_value);
  } else if (!strcmp(parameter_name, "backlog")) {
    backlog = atoi(parameter_value);
    if (backlog <= 0) {
      backlog = SOMAXCONN;
      TTCN_warning("IPL4asp__PT_PROVIDER::set_parameter: invalid "
          "backlog value set to %d", backlog);
    }
  } else if (!strcmp(parameter_name, "sockListSizeInit")) {
    sockListSize = atoi(parameter_value);
    if (sockListSize < SOCK_LIST_SIZE_MIN) {
      sockListSize = SOCK_LIST_SIZE_MIN;
      TTCN_warning("IPL4asp__PT_PROVIDER::set_parameter: invalid "
          "sockListSizeInit value set to %d", sockListSize);
    }
  } else if (!strcmp(parameter_name, "pureNonBlocking")) {
    if (!strcasecmp(parameter_value,"YES"))
      pureNonBlocking = true;
  } else if (!strcmp(parameter_name, "useExtendedResult")) {
    if (!strcasecmp(parameter_value,"YES"))
      send_extended_result = true;
  } else if (!strcmp(parameter_name, "lazy_conn_id_handling")) {
    if (!strcasecmp(parameter_value,"YES")){
      lazy_conn_id_level = 1;
    } else {
      lazy_conn_id_level = 0;
    }
  } else if (!strcasecmp(parameter_name, "sctp_path_mtu_size")) {
    sctp_PMTU_size = atoi(parameter_value);
  } else if (!strcmp(parameter_name, "ipAddressDiscoveryType")) {
    if (!strcasecmp(parameter_value,"DHCP_OR_ARP"))
      ipDiscConfig.type = IPDiscConfig::DHCP_OR_ARP;
    else if (!strcasecmp(parameter_value,"DHCP"))
      ipDiscConfig.type = IPDiscConfig::DHCP;
    else if (!strcasecmp(parameter_value,"ARP"))
      ipDiscConfig.type = IPDiscConfig::ARP;
  } else if (!strcmp(parameter_name, "freebind")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.freebind = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.freebind = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "interfaceName")) {
    ipDiscConfig.expIfName = parameter_value;
  } else if (!strcmp(parameter_name, "interfaceIpAddress")) {
    ipDiscConfig.expIfIpAddress = parameter_value;
  } else if (!strcmp(parameter_name, "excludedInterfaceIpAddress")) {
    ipDiscConfig.exclIfIpAddress = parameter_value;
  } else if (!strcmp(parameter_name, "ethernetAddressStart")) {
    ipDiscConfig.ethernetAddress = parameter_value;
  } else if (!strcmp(parameter_name, "leaseTime")) {
    ipDiscConfig.leaseTime = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "leaseFile")) {
    ipDiscConfig.leaseFile = parameter_value;
  } else if (!strcmp(parameter_name, "numberOfIpAddressesToFind")){
    ipDiscConfig.nOfAddresses = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "dhcpMsgRetransmitCount")){
    ipDiscConfig.dhcpMsgRetransmitCount = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "dhcpMsgRetransmitPeriodInms")){
    ipDiscConfig.dhcpMsgRetransmitPeriodInms = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "dhcpMaxParallelRequestCount")){
    ipDiscConfig.dhcpMaxParallelRequestCount = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "dhcpTimeout")){
    ipDiscConfig.dhcpTimeout = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "arpMsgRetransmitCount")){
    ipDiscConfig.arpMsgRetransmitCount = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "arpMsgRetransmitPeriodInms")){
    ipDiscConfig.arpMsgRetransmitPeriodInms = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "arpMaxParallelRequestCount")){
    ipDiscConfig.arpMaxParallelRequestCount = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "tcpReuseAddress")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.tcpReuseAddr = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.tcpReuseAddr = GlobalConnOpts::NO;
  }else if (!strcmp(parameter_name, "sslReuseAddress")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.tcpReuseAddr = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.tcpReuseAddr = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "udpReuseAddress")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.udpReuseAddr = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.udpReuseAddr = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctpReuseAddress")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctpReuseAddr = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctpReuseAddr = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "tcpKeepAlive")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.tcpKeepAlive = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.tcpKeepAlive = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "tcpKeepCount")){
    globalConnOpts.tcpKeepCnt = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "tcpKeepIdle")){
    globalConnOpts.tcpKeepIdle = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "tcpKeepInterval")){
    globalConnOpts.tcpKeepIntvl = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sslKeepAlive")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.tcpKeepAlive = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.tcpKeepAlive = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sslKeepCount")){
    globalConnOpts.tcpKeepCnt = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sslKeepIdle")){
    globalConnOpts.tcpKeepIdle = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sslKeepInterval")){
    globalConnOpts.tcpKeepIntvl = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "extendedPortEvents")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.extendedPortEvents = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.extendedPortEvents = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sinit_num_ostreams")){ //sctp specific params starts here
    globalConnOpts.sinit_num_ostreams = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sinit_max_instreams")){
    globalConnOpts.sinit_max_instreams = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sinit_max_attempts")){
    globalConnOpts.sinit_max_attempts = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sinit_max_init_timeo")){
    globalConnOpts.sinit_max_init_timeo = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "sctp_data_io_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_data_io_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_data_io_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_association_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_association_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_association_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_address_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_address_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_address_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_send_failure_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_send_failure_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_send_failure_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_peer_error_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_peer_error_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_peer_error_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_shutdown_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_shutdown_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_shutdown_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_partial_delivery_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_partial_delivery_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_partial_delivery_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_adaptation_layer_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_adaptation_layer_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_adaptation_layer_event = GlobalConnOpts::NO;
  } else if (!strcmp(parameter_name, "sctp_authentication_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_authentication_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_authentication_event = GlobalConnOpts::NO;
  } 
#ifdef SCTP_SENDER_DRY_EVENT
    else if (!strcmp(parameter_name, "sctp_sender_dry_event")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.sctp_sender_dry_event = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.sctp_sender_dry_event = GlobalConnOpts::NO;
  } 
#endif
    else if (!strcmp(parameter_name, "sctp_connection_method")){
    if (!strcasecmp(parameter_value,"METHOD_0"))
      globalConnOpts.connection_method = GlobalConnOpts::METHOD_ZERO;
    else if (!strcasecmp(parameter_value,"METHOD_1"))
      globalConnOpts.connection_method = GlobalConnOpts::METHOD_ONE;
    else if (!strcasecmp(parameter_value,"METHOD_2"))
      globalConnOpts.connection_method = GlobalConnOpts::METHOD_TWO;
  }
  else if (!strcmp(parameter_name, "sctp_stack")){
    if (!strcasecmp(parameter_value,"kernel"))
      native_stack=TRUE;
    else if (!strcasecmp(parameter_value,"EIN"))
      native_stack=FALSE;
  }
  else if(!strcmp(parameter_name,"broadcast")){
    if (!strcasecmp(parameter_value,"enabled"))
      broadcast = true;
    else if (!strcasecmp(parameter_value,"disabled"))
      broadcast = false;    
    else {
      broadcast = false;
      TTCN_warning("IPL4asp__PT::set_parameter(): Unsupported Test Port parameter value: %s", parameter_value);
    }
  }
  else if (!strcmp(parameter_name, "SSLv2")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.SSLv2 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.SSLv2 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "SSLv3")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.SSLv3 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.SSLv3 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "TLSv1")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.TLSv1 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.TLSv1 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "TLSv1.1")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.TLSv1_1 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.TLSv1_1 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "TLSv1.2")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.TLSv1_2 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.TLSv1_2 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "DTLSv1")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.DTLSv1 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.DTLSv1 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "DTLSv1.2")){
    if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.ssl_supp.DTLSv1_2 = GlobalConnOpts::YES;
    else if (!strcasecmp(parameter_value,"NO"))
      globalConnOpts.ssl_supp.DTLSv1_2 = GlobalConnOpts::NO;
  }
  else if (!strcmp(parameter_name, "TLS_CERT_PER_CONN")){
    if (!strcasecmp(parameter_value,"YES"))
       ssl_cert_per_conn= true;
    else
       ssl_cert_per_conn= false;
  }
#ifdef USE_IPL4_EIN_SCTP
  else if(!strcasecmp("cpManagerIPA", parameter_name))
  {
    cpManagerIPA = parameter_value;
  } 
  else if (!strcmp(parameter_name, "USERID")){
    if(!SS7Common::handle_parameter(parameter_name, parameter_value, &userId)) {
      TTCN_error("IPL4asp__PT::set_parameter(): Invalid user ID.");
    }
  }
  else if(strcasecmp("userInstanceId", parameter_name) == 0) 
  {
    int uinst = atoi(parameter_value);
    if(uinst < 0 || uinst > 255) 
    {
      log_warning("Wrong userInstanceId parameter value: %s, Must be an integer between 0 and 255 ", parameter_value);
    }
    else 
    {
      userInstanceId = uinst;
      userInstanceIdSpecified = true;
      if(!sctpInstanceIdSpecified)
        sctpInstanceId = userInstanceId;
    }
  }
  else if(strcasecmp("sctpInstanceId", parameter_name) == 0) 
  {
    int tinst = atoi(parameter_value);
    if(tinst < 0 || tinst > 255) 
    {
      log_warning("Wrong sctpInstanceId parameter value: %s, Must be an integer between 0 and 255 ", parameter_value);
    }
    else 
    {
      sctpInstanceId = tinst;
      sctpInstanceIdSpecified = true;
    }
  }

#endif


  //SSL params
#ifdef IPL4_USE_SSL
  else if(strcmp(parameter_name, ssl_use_session_resumption_name()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) ssl_use_session_resumption = true;
    else if(strcasecmp(parameter_value, "no") == 0) ssl_use_session_resumption = false;
    else log_warning("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_use_session_resumption_name());
  } else if(strcmp(parameter_name, ssl_private_key_file_name()) == 0) {
    delete [] ssl_key_file;
    ssl_key_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_key_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_trustedCAlist_file_name()) == 0) {
    delete [] ssl_trustedCAlist_file;
    ssl_trustedCAlist_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_trustedCAlist_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_certificate_file_name()) == 0) {
    delete [] ssl_certificate_file;
    ssl_certificate_file=new char[strlen(parameter_value)+1];
    strcpy(ssl_certificate_file, parameter_value);
  } else if(strcmp(parameter_name, ssl_cipher_list_name()) == 0) {
    delete [] ssl_cipher_list;
    ssl_cipher_list=new char[strlen(parameter_value)+1];
    strcpy(ssl_cipher_list, parameter_value);
  } else if(strcmp(parameter_name, ssl_password_name()) == 0) {
    ssl_password=new char[strlen(parameter_value)+1];
    strcpy(ssl_password, parameter_value);
  } else if(strcmp(parameter_name, ssl_verifycertificate_name()) == 0) {
    if(strcasecmp(parameter_value, "yes") == 0) ssl_verify_certificate = true;
    else if(strcasecmp(parameter_value, "no") == 0) ssl_verify_certificate = false;
    else log_warning("Parameter value '%s' not recognized for parameter '%s'", parameter_value, ssl_verifycertificate_name());
  } else if (!strcmp(parameter_name, "ssl_reconnect_attempts")){
    ssl_reconnect_attempts = atoi ( parameter_value );
  } else if (!strcmp(parameter_name, "ssl_reconnect_delay")){
    ssl_reconnect_delay = atoi ( parameter_value );
  }
#endif
 else if (!strcasecmp(parameter_name, "noDelay")) {
   if (!strcasecmp(parameter_value,"YES"))
      globalConnOpts.tcp_nodelay = GlobalConnOpts::YES;
      globalConnOpts.sctp_nodelay = GlobalConnOpts::YES;
  }
  // else if ( next param ) ...
} // IPL4asp__PT_PROVIDER::set_parameter


void IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable(int fd)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: fd: %i", fd);
  std::map<int,int>::iterator it = fd2IndexMap.find(fd);
  if (pureNonBlocking) {
    if (it != fd2IndexMap.end()) {
      //Add SSL layer
#ifdef IPL4_USE_SSL
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: client? %s sslState: %i", sockList[it->second].ssl_tls_type == CLIENT ? "yes" : "no", sockList[it->second].sslState);
      if(sockList[it->second].ssl_tls_type != NONE && sockList[it->second].type != IPL4asp_SCTP)
      {
        switch(sockList[it->second].sslState){
        case STATE_WAIT_FOR_RECEIVE_CALLBACK:
          sockList[it->second].sslState = STATE_NORMAL;
          Handler_Remove_Fd_Write(fd);
          IPL4_DEBUG("DONT WRITE ON %i", fd);
          return;
        case STATE_CONNECTING:
        case STATE_HANDSHAKING: {
          switch(perform_ssl_handshake(it->second))
          {
          case FAIL:
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: SSL mapping failed for client socket: %d", sockList[it->second].sock);
            if (ConnDel(it->second) == -1) IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: unable to close socket");
            sendError(PortError::ERROR__SOCKET, it->second);
            return;
          case WANT_READ:
            IPL4_DEBUG("DONT WRITE ON %i", fd);
            Handler_Remove_Fd_Write(fd);
            return;
          case WANT_WRITE:
            return;
          case SUCCESS: //if success, then the handshake is over, we are done
            break;
          default:
            IPL4_DEBUG( "IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: perform_ssl_handshake return value is not handled!");
          }

          sockList[it->second].sslState = STATE_NORMAL;
          IPL4_DEBUG("DONT WRITE ON %i", fd);
          Handler_Remove_Fd_Write(fd);

          if(sockList[it->second].server) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Client connection Accepted.");
            reportConnOpened(it->second);
          } else {
            sendError(PortError::ERROR__AVAILABLE, it->second);
          }

          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Leave.");
          return;
        }
        default:
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Unhandled SSL State %d . Client: %d", sockList[it->second].sslState, it->second);
          Handler_Remove_Fd_Write(fd);
          IPL4_DEBUG("DONT WRITE ON %i", fd);
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Leave.");
          return;
        }
      } else if(sockList[it->second].ssl_tls_type != NONE && sockList[it->second].type == IPL4asp_SCTP)
      {
        Handler_Remove_Fd_Write(fd);
        IPL4_DEBUG("DONT WRITE ON %i", fd);
        return;
      }
#endif
      Handler_Remove_Fd_Write(fd);
      IPL4_DEBUG("DONT WRITE ON %i", fd);
      sendError(PortError::ERROR__AVAILABLE, it->second);

    } else
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Error: fd not found");
  } else
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Error: pureNonBlocking is FALSE");

  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: Leave.");
}

void IPL4asp__PT_PROVIDER::Handle_Fd_Event_Error(int fd)
{
  // The error events are handled as readable event.
  Handle_Fd_Event_Readable(fd);
}

void IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable(int fd)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: enter, fd: %i", fd);

#ifdef USE_IPL4_EIN_SCTP
  if(!native_stack && (fd == pipe_to_TTCN_thread_fds[0] || fd == pipe_to_TTCN_thread_log_fds[0])){
    handle_message_from_ein(fd);
    return;
  }
#endif


  std::map<int,int>::iterator it = fd2IndexMap.find(fd);
  if (it == fd2IndexMap.end()) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Error: fd not found");
    return;
  }

  int connId = it->second;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: connId: %d READABLE   sock: %i, type: %i, sslState: %i",
      connId, sockList[connId].sock, sockList[connId].type, sockList[connId].sslState);

  ASP__RecvFrom asp;
  asp.connId() = connId;
  asp.userData() = sockList[connId].userData;
  asp.remName() = *(sockList[connId].remoteaddr);
  asp.remPort() = *(sockList[connId].remoteport);
  asp.locName() = *(sockList[connId].localaddr);
  asp.locPort() = *(sockList[connId].localport);
  asp.proto().tcp() = TcpTuple(null_type());

  // Identify local socket
  SockAddr sa;
  socklen_t saLen = sizeof(SockAddr);

  // Handle active socket
  int len = -3;
  unsigned char buf[RECV_MAX_LEN];
#ifdef IPL4_USE_SSL
  int ssl_err_msg = 0;
#endif
  if((sockList[connId].ssl_tls_type != NONE) and
      ((sockList[connId].sslState == STATE_CONNECTING) || (sockList[connId].sslState == STATE_HANDSHAKING))) {
    // 1st branch: handle SSL/TLS handshake

    if(sockList[connId].type == IPL4asp_UDP) {
      asp.proto().dtls().udp() = UdpTuple(null_type());
    } else {
      asp.proto().ssl() = SslTuple(null_type());
    }

    #ifdef IPL4_USE_SSL
    if(sockList[connId].server) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: incoming SSL connection...");
      if (ssl_verify_certificate && 
          ((sockList[connId].ssl_trustedCAlist_file==NULL && ssl_cert_per_conn) || (ssl_trustedCAlist_file==NULL && !ssl_cert_per_conn)))
      {
        IPL4_DEBUG("%s is not defined in the configuration file although %s=yes", ssl_trustedCAlist_file_name(), ssl_verifycertificate_name());
        sendError(PortError::ERROR__SOCKET, connId);
      }

      if(sockList[connId].type == IPL4asp_UDP) {
#ifdef SRTP_AES128_CM_SHA1_80
        // DTLS listen & accept the incoming connection
        SockAddr sa_client;
        int ret = DTLSv1_listen(sockList[connId].sslObj, 
#if OPENSSL_VERSION_NUMBER >= 0x10100000L 
         (BIO_ADDR*)
#endif        
        &sa_client);
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLSv1_listen exited with ret.value %d", ret);
        if(ret <= 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: not enough data for DTLSv1_listen...");
          ssl_getresult(ret);
          return;
        }

        // if UDP/DTLS server, then create a new socket for the incoming connection
        // get the listening socket parameters
        if(!getsockname(sockList[connId].sock, (struct sockaddr *)&sa, &saLen)) {
          int client_fd = socket(((struct sockaddr *)&sa)->sa_family, SOCK_DGRAM, 0);
          int reuse = 1;
          setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &reuse, (socklen_t) sizeof(reuse));
          if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {
            TTCN_warning("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: fcntl O_NONBLOCK on socket %d failed: %s",
                client_fd, strerror(errno));
            close(client_fd);
            sendError(PortError::ERROR__SOCKET, fd, errno);
            return;
          }
          if(bind(client_fd, (struct sockaddr *)&sa, saLen) < 0) {
            TTCN_warning("Binding to the incoming DTLS connection's local server socket failed!");
            close(client_fd);
            sendError(PortError::ERROR__SOCKET, fd, errno);
            return;
          }
          if(connect(client_fd, (struct sockaddr *) (struct sockaddr *)&sa_client, saLen) < 0) {
            TTCN_warning("Connecting from the incoming DTLS connection's local server socket failed!");
            close(client_fd);
            sendError(PortError::ERROR__SOCKET, fd, errno);
            return;
          }
          int clientConnId = ConnAdd(IPL4asp_UDP, client_fd, SERVER,NULL,connId);
          // client's bio is the read bio of the server
          sockList[clientConnId].bio = SSL_get_rbio(sockList[connId].sslObj);
          BIO_set_fd(sockList[clientConnId].bio, client_fd, BIO_NOCLOSE);
          BIO_ctrl(sockList[clientConnId].bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &sa_client);
          // set the server's SSL obj into the new client
          sockList[clientConnId].sslObj = sockList[connId].sslObj;
//          if(sockList[connId].dtlsSrtpProfiles) {  // Done by the ConnAdd
//            // set the server's dtls srtp profiles into the accepted client
//            // we could use strdup, but let's stick to Malloc if we want TITAN memory debug
//            int len = strlen(sockList[connId].dtlsSrtpProfiles);
//            sockList[clientConnId].dtlsSrtpProfiles = (char*)Malloc(len + 1);
//            memcpy(sockList[clientConnId].dtlsSrtpProfiles, sockList[connId].dtlsSrtpProfiles, len);
//            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: dtlsSrtpProfiles %s is set for the accepted client connId: %d", sockList[clientConnId].dtlsSrtpProfiles, clientConnId);
//          }
          // this connection will be client + SSL SERVER
          sockList[clientConnId].server = false;
          sockList[clientConnId].ssl_tls_type = SERVER;
          if (!SetNameAndPort(&sa_client, saLen, *(sockList[clientConnId].remoteaddr), *(sockList[clientConnId].remoteport))) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SetNameAndPort failed");
            sendError(PortError::ERROR__HOSTNAME, clientConnId);
          }
          // now restart the TLS on the server connId
          Socket__API__Definitions::Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
          starttls(connId, true, result);

          // continue the handshake with the client connId
          connId = clientConnId;

          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: client connection accepted on fd:%d/connId:%d; continuing with the handshake...", client_fd, clientConnId);
        } else {
          TTCN_warning("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: could not get current socket parameters");
          return;
        }
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
      }
    }

    switch(perform_ssl_handshake(connId)) {

    case SUCCESS: //if success continue
      sockList[it->second].sslState = STATE_NORMAL;
      break;
    case FAIL:
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SSL mapping failed for client socket: %d", connId);
      if (ConnDel(connId) == -1) IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: unable to close socket");
      sendError(PortError::ERROR__SOCKET, it->second);
      sendConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());
      return;
    case WANT_READ:
    case WANT_WRITE:
      return;
    default:
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: perform_ssl_handshake return value is not handled!");
    }

    if(sockList[connId].ssl_tls_type == SERVER) {
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: our server accepted an incoming %s connection.", sockList[connId].type == IPL4asp_UDP ? "DTLS" : "SSL");
      reportConnOpened(connId);
      if((sockList[connId].type == IPL4asp_SCTP) || (sockList[connId].type == IPL4asp_SCTP_LISTEN))
        sockList[connId].sctpHandshakeCompletedBeforeDtls = true;
    } else {
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: our client established a %s connection.", sockList[connId].type == IPL4asp_UDP ? "DTLS" : "SSL");
      sendError(PortError::ERROR__AVAILABLE, connId);
      if((sockList[connId].type == IPL4asp_SCTP) || (sockList[connId].type == IPL4asp_SCTP_LISTEN))
        sockList[connId].sctpHandshakeCompletedBeforeDtls = true;
    }
#endif //IPL4_USE_SSL
  } else if((sockList[connId].type == IPL4asp_TCP_LISTEN) || (sockList[connId].type == IPL4asp_SCTP_LISTEN)) {
    // 2nd branch: handle TCP/SCTP server accept

    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: incoming connection requested");
    int sock = accept(sockList[connId].sock, (struct sockaddr *)&sa, &saLen);
    if (sock == -1) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: tcp accept error: %s",
          strerror(errno));
      sendError(PortError::ERROR__SOCKET, connId, errno);
      return;
    } else {
      if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: fcntl O_NONBLOCK on socket %d failed: %s",
            sock, strerror(errno));
        close(sock);
        sendError(PortError::ERROR__SOCKET, sock, errno);
        return;
      }
    }

    int k;
    if(sockList[connId].type == IPL4asp_SCTP_LISTEN) {
#ifdef USE_SCTP
      k = ConnAdd(IPL4asp_SCTP, sock, sockList[connId].ssl_tls_type, NULL ,connId);
#else
      sendError(PortError::ERROR__UNSUPPORTED__PROTOCOL, sockList[connId].sock);
      return;
#endif
    } else { // TCP_LISTEN
      k = ConnAdd(IPL4asp_TCP, sock, sockList[connId].ssl_tls_type == SERVER ? CLIENT : NONE, NULL, connId);
    }

    if ( k == -1) {
      sendError(PortError::ERROR__INSUFFICIENT__MEMORY, connId);
      return;
    }

#ifdef IPL4_USE_SSL
    if((sockList[connId].ssl_tls_type == SERVER) && (sockList[connId].type == IPL4asp_TCP_LISTEN)) {
      Socket__API__Definitions::Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      starttls(k, true, result);
    }
#endif

    // if we are not doing SSL, then report the connection opened
    // if SSL is also triggered, we will report the connOpened at the end of the handshake
    if(sockList[connId].ssl_tls_type == NONE) {
      reportConnOpened(k);
    }

  } else {
    // 3rd branch: normal data receiving
    switch (sockList[connId].type) {

    case IPL4asp_UDP: {
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: udp message received");
      // normal UDP receiving first
      if((sockList[connId].ssl_tls_type == NONE) ||
          // if DTLS+SRTP, then no demultiplex yet, just pass the incoming packet as UDP to the testcase
          ((sockList[connId].dtlsSrtpProfiles != NULL) && (sockList[connId].sslState == STATE_NORMAL)))
      {
        asp.proto().udp() = UdpTuple(null_type());
        len = recvfrom(sockList[connId].sock, buf, RECV_MAX_LEN,
            0, (struct sockaddr *)&sa, &saLen);
        if ((len >= 0) && !SetNameAndPort(&sa, saLen, asp.remName(), asp.remPort()))
          sendError(PortError::ERROR__HOSTNAME, connId);
      }
#ifdef IPL4_USE_SSL
      // nice DTLS is coming
      else {
        asp.proto().dtls().udp() = UdpTuple(null_type());
        len = receive_ssl_message_on_fd(connId, &ssl_err_msg);
      }
#endif

      if (len == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: udp recvfrom error: %s",
            strerror(errno));
        sendError(PortError::ERROR__SOCKET, connId, errno);
        break;
      } else if((len == -2) and (sockList[connId].ssl_tls_type != NONE)) {
        // if SSL is reporting block for sending
        switch (sockList[connId].sslState) {
        case STATE_BLOCK_FOR_SENDING:
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: state is STATE_BLOCK_FOR_SENDING, don't close connection.");
          Handler_Remove_Fd_Read(connId);
          sockList[connId].sslState = STATE_DONT_CLOSE;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: setting socket state to STATE_DONT_CLOSE");
          break;
        case STATE_DONT_CLOSE:
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: state is STATE_DONT_CLOSE, don't close connection.");
          break;
        default:
          break;
        }
      } else if((len == 0 
#ifdef IPL4_USE_SSL
        || ssl_err_msg == SSL_ERROR_ZERO_RETURN
#endif
        ) and (sockList[connId].ssl_tls_type != NONE)) {
        reportRemainingData_beforeConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());
        sendConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());
          if (ConnDel(connId) == -1) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
            sendError(PortError::ERROR__SOCKET, connId);
          }
      }

      if(len > 0)
      {
        if(sockList[connId].ssl_tls_type == NONE) {
          asp.msg() = OCTETSTRING(len, buf);
          incoming_message(asp);
        } else {
          // throw warning if connId is SRTP & the incoming packet is DTLS
          if(sockList[connId].dtlsSrtpProfiles != NULL) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: first byte is %d.", (*buf));
            if((*buf > 19) && (*buf < 64)) { // this is the DTLS range according to http://tools.ietf.org/html/rfc5764#section-5.1.2
              // TODO FIXME: here we shall find a way how to decrypt the buffer (incoming DTLS) with the sockList[connId].ssl
              TTCN_warning("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS received on connId: %d, but demultiplexing is not implemented! Throwing the packet away...", connId);
              break;
            } else {
              // ordinary UDP (DTLS+SRTP or DTLS + STUN)
              asp.msg() = OCTETSTRING(len, buf);
            }
          } else {
            // no fragmentation in UDP, DTLS neither. no need to call the getMsgLen
            asp.msg() = OCTETSTRING(len, sockList[connId].buf[0]->get_data());
            sockList[connId].buf[0]->set_pos((size_t)len);
            sockList[connId].buf[0]->cut();
          }
          incoming_message(asp);
        }
      }
      break;
    }
    case IPL4asp_TCP_LISTEN: {break;} // IPL4asp_TCP_LISTEN
    case IPL4asp_TCP: {

      memset(&sa, 0, saLen);

      if(sockList[connId].ssl_tls_type == NONE) {

        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: TCP recvfrom enter:");

        asp.proto().tcp() = TcpTuple(null_type());
        len = recv(sockList[connId].sock, buf, RECV_MAX_LEN, 0);

      }
#ifdef IPL4_USE_SSL
      else {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SSL recvfrom enter:");

        asp.proto().ssl() = SslTuple(null_type());

        if (!isConnIdValid(connId)) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: invalid connId: %connId", connId);
          break;
        }

        len = receive_ssl_message_on_fd(connId, &ssl_err_msg);

      }
#endif

      if (len > 0) {
        // in normal case, this is where we put the incoming data in the buffer
        // in SSL case, the receive_ssl_message_on_fd() already placed it there
        if(sockList[connId].ssl_tls_type == NONE) {
          (*sockList[connId].buf)->put_s(len, buf);
        }

        bool msgFound = false;
        do {
          if (sockList[connId].getMsgLen != simpleGetMsgLen) {
            if (sockList[connId].msgLen == -1){
              OCTETSTRING oct;
              (*sockList[connId].buf)->get_string(oct);
              sockList[connId].msgLen = sockList[connId].getMsgLen.invoke(oct,*sockList[connId].msgLenArgs);
            }
          } else {
            sockList[connId].msgLen = (*sockList[connId].buf)->get_len();
          }
          msgFound = (sockList[connId].msgLen != -1) && (sockList[connId].msgLen <= (int)sockList[connId].buf[0]->get_len());
          if (msgFound) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: message length: (%d/%d bytes)\n",
                sockList[connId].msgLen, (int)sockList[connId].buf[0]->get_len());
            asp.msg() = OCTETSTRING(sockList[connId].msgLen, sockList[connId].buf[0]->get_data());
            sockList[connId].buf[0]->set_pos((size_t)sockList[connId].msgLen);
            sockList[connId].buf[0]->cut();
            if(lazy_conn_id_level && sockListCnt==1 && lonely_conn_id!=-1){
              asp.connId()=-1;
            }
            incoming_message(asp);
            sockList[connId].msgLen = -1;
          }
        } while (msgFound && sockList[connId].buf[0]->get_len() != 0);
        if (sockList[connId].buf[0]->get_len() != 0)
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: incomplete message (%d bytes)\n",
              (int)sockList[connId].buf[0]->get_len());
      }

      // NOTE: after this if, we will also enter the connClosed part with len == 0 or len == -1
      // and automatically perform connection closing in case of socket error
      if (len == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: %s recvfrom error: %s",
            sockList[connId].ssl_tls_type == NONE ? "tcp" : "ssl", strerror(errno));
        sendError(PortError::ERROR__SOCKET, connId, errno);
      } /* else if (len == -2) =>
          means that reading would block in SSL case.
          in this case I stop receiving message on the file descriptor, do nothing */

      if ((len == 0) || (len == -1)
#ifdef IPL4_USE_SSL
          || (ssl_err_msg == SSL_ERROR_ZERO_RETURN)
#endif
         ) { // peer disconnected
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: "
            "peer closed connection %d, fd: %d", connId, sockList[connId].sock);

        bool dontCloseConn = false;

        if(sockList[connId].ssl_tls_type == NONE) {
          closingPeer = sa;
          closingPeerLen = saLen;
          if (connId == dontCloseConnectionId) {
            //close(sockList[connId].sock);
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: closing connection %d"
                " postponed due to nonblocking send operation", connId);
            dontCloseConn = true;
            break;
          }
        }
#ifdef IPL4_USE_SSL
        else {
          // SSL case
          switch (sockList[connId].sslState) {
          case STATE_BLOCK_FOR_SENDING:
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: state is STATE_BLOCK_FOR_SENDING, don't close connection.");
            Handler_Remove_Fd_Read(connId);
            sockList[connId].sslState = STATE_DONT_CLOSE;
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: setting socket state to STATE_DONT_CLOSE");
            dontCloseConn = true;
            break;
          case STATE_DONT_CLOSE:
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: state is STATE_DONT_CLOSE, don't close connection.");
            dontCloseConn = true;
            break;
          default:
            closingPeer = sa;
            closingPeerLen = saLen;
            if (connId == dontCloseConnectionId) {
              //close(sockList[connId].sock);
              IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: closing connection %d"
                  " postponed due to nonblocking send operation", connId);
              dontCloseConn = true;
            }
          } // switch (client_data->reading_state)
        }
#endif

        reportRemainingData_beforeConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());
        sendConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());

        if(!dontCloseConn) {
          if (ConnDel(connId) == -1) {
            IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
            sendError(PortError::ERROR__SOCKET, connId);
          }
        }
        closingPeerLen = 0;

      }
      break;
    } // IPL4asp_TCP
    case IPL4asp_SCTP_LISTEN: {break;} // IPL4asp_SCTP_LISTEN
    case IPL4asp_SCTP: {
#ifdef USE_SCTP

      ASP__Event event;

      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: sctp message received");
      memset(&sa, 0, saLen);

      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: sctp peername and sockname obtained");

      int sock = sockList[connId].sock;
      ssize_t n = 0;
      size_t buflen = RECV_MAX_LEN;
      struct msghdr  msg[1];
      struct iovec  iov[1];
      struct cmsghdr  *cmsg;
      struct sctp_sndrcvinfo *sri;
      char   cbuf[sizeof (*cmsg) + sizeof (*sri)];
      size_t cmsglen = sizeof (*cmsg) + sizeof (*sri);
      struct sockaddr_storage peer_addr;
      socklen_t peer_addr_len=sizeof(struct sockaddr_storage);
      /* Initialize the message header for receiving */
      memset(msg, 0, sizeof (*msg));
      memset(&peer_addr, 0, peer_addr_len);
      msg->msg_name=(void *)&peer_addr;
      msg->msg_namelen=peer_addr_len;
      msg->msg_control = cbuf;
      msg->msg_controllen = sizeof (*cmsg) + sizeof (*sri);
      msg->msg_flags = 0;
      memset(cbuf, 0, sizeof (*cmsg) + sizeof (*sri));
      cmsg = (struct cmsghdr *)cbuf;
      sri = (struct sctp_sndrcvinfo *)(cmsg + 1);
      iov->iov_base = buf;
      iov->iov_len = RECV_MAX_LEN;
      msg->msg_iov = iov;
      msg->msg_iovlen = 1;
#ifdef OPENSSL_SCTP_SUPPORT
      bool sctpNotification = false;
#endif

      int getmsg_retv=getmsg(sock, connId, msg, buf, &buflen, &n, cmsglen);
      switch(getmsg_retv){
      case IPL4_SCTP_WHOLE_MESSAGE_RECEIVED:
      {
        if(msg->msg_namelen){
          char remaddr[46]; // INET6_ADDRSTRLEN
          memset(remaddr,0,46);
          if(peer_addr.ss_family==AF_INET){
            struct sockaddr_in* remipv4addr=(struct sockaddr_in*)&peer_addr;
            if(inet_ntop(AF_INET,&(remipv4addr->sin_addr),remaddr,46)){
              asp.remName()=remaddr;
            }
          }
#ifdef USE_IPV6
          else if(peer_addr.ss_family==AF_INET6){
            struct sockaddr_in6* remipv6addr=(struct sockaddr_in6*)&peer_addr;
            if(inet_ntop(AF_INET6,&(remipv6addr->sin6_addr),remaddr,46)){
              asp.remName()=remaddr;
            }
          }
#endif
        }
        (*sockList[connId].buf)->put_s(n, buf);
        const unsigned char* atm=(*sockList[connId].buf)->get_data();
        if (msg->msg_flags & MSG_NOTIFICATION) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Notification received");
          handle_event(sock, connId, atm);

#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
          sctpNotification = true;
#endif
          union sctp_notification *snp;
    	  snp = (sctp_notification *)buf;

    	  if(snp->sn_header.sn_type == SCTP_ASSOC_CHANGE)
    	  {
    	    struct sctp_assoc_change *sac;
    	    sac = &snp->sn_assoc_change;
    	    if (sac->sac_state == SCTP_COMM_UP and
    	        sockList[connId].ssl_tls_type == SERVER and sockList[connId].sslState == STATE_NORMAL) {
			    // now restart the TLS on the server connId
              Socket__API__Definitions::Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
              starttls(connId, true, result);
    	    }
    	  }
#ifdef SCTP_SENDER_DRY_EVENT
    	    // SCTP_SENDER_DRY_EVENT notifies that the SCTP stack has no more user data to send or retransmit (rfc6458).
    	    // This means that dtls DATA might be also received.
#ifdef OPENSSL_SCTP_SUPPORT

    	  if(snp->sn_header.sn_type == SCTP_SENDER_DRY_EVENT) sctpNotification = false;
#endif
#endif
    	  if(snp->sn_header.sn_type == SCTP_SHUTDOWN_EVENT)
    	  {
            // Somehow the recvmsg with MSG_PEEK flag in getmsg returns values > 0 even in case of the SCTP Shutdown event
            // The connection close should be then triggered at this point
    	    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Socket is closed.");

            if (connId == dontCloseConnectionId) {
              closingPeer = sa;
              closingPeerLen = saLen;
              //close(sockList[connId].sock);
              IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: closing connection %d"
                  " postponed due to nonblocking send operation", connId);
              return;
            }
            if(sockList[connId].ssl_tls_type == NONE || !sockList[connId].sctpHandshakeCompletedBeforeDtls){
              asp.proto().sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
            } else {
              // if the SCTP_SHUTDOWN_EVENT is received when DTLS is also used, the conn. closed will be evaluated at this point.
              // Thus, the DTLS-SCTP should be reported as protocol.
              asp.proto().dtls().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
            }
            sendConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());

            if (ConnDel(connId) == -1) {
              IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
              sendError(PortError::ERROR__SOCKET, connId);
            }
            closingPeerLen = 0;
    	  }
#endif
        }
        else if(sockList[connId].ssl_tls_type == NONE || !sockList[connId].sctpHandshakeCompletedBeforeDtls)
        {
          IPL4_DEBUG("PL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Incoming data (%ld bytes): stream = %hu, ssn = %hu, flags = %hx, ppid = %u \n", n,
          sri->sinfo_stream,(unsigned int)sri->sinfo_ssn, sri->sinfo_flags, sri->sinfo_ppid);
          INTEGER i_ppid;
          if (ntohl(sri->sinfo_ppid) <= (unsigned long)INT_MAX)
            i_ppid = ntohl(sri->sinfo_ppid);
          else {
            char sbuf[16];
            sprintf(sbuf, "%u", ntohl(sri->sinfo_ppid));
            i_ppid = INTEGER(sbuf);
          }

          asp.proto().sctp() = SctpTuple(sri->sinfo_stream, i_ppid, OMIT_VALUE, OMIT_VALUE);
          (*sockList[connId].buf)->get_string(asp.msg());
          if(lazy_conn_id_level && sockListCnt==1 && lonely_conn_id!=-1){
            asp.connId()=-1;
          }
          incoming_message(asp);
        }
        if(sockList[connId].buf) (*sockList[connId].buf)->clear();
      }
      break;
      case IPL4_SCTP_PARTIAL_RECEIVE:
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: partial receive: %ld bytes", n);
        (*sockList[connId].buf)->put_s(n, buf);
      break;
      case IPL4_SCTP_ERROR_RECEIVED:
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SCTP error, Socket is closed.");
      case IPL4_SCTP_EOF_RECEIVED:
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Socket is closed.");

        if (connId == dontCloseConnectionId) {
          closingPeer = sa;
          closingPeerLen = saLen;
          //close(sockList[connId].sock);
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: closing connection %d"
                  " postponed due to nonblocking send operation", connId);
          return;
        }
        if(sockList[connId].ssl_tls_type == NONE || !sockList[connId].sctpHandshakeCompletedBeforeDtls){
          asp.proto().sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
        } else {
          // if the SCTP_SHUTDOWN_EVENT is received when DTLS is also used, the conn. closed will be evaluated at this point.
          // Thus, the DTLS-SCTP should be reported as protocol.
          asp.proto().dtls().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
        }
        sendConnClosed(connId, asp.remName(), asp.remPort(), asp.locName(), asp.locPort(), asp.proto(), asp.userData());

#ifdef OPENSSL_SCTP_SUPPORT
        sctpNotification = true;
#endif

        if (ConnDel(connId) == -1) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
          sendError(PortError::ERROR__SOCKET, connId);
        }
        closingPeerLen = 0;
        break;
        default:

        break;
      }

#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
      if(sockList[connId].sctpHandshakeCompletedBeforeDtls && !sctpNotification) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: dtls/sctp message received");

        ssize_t n = 0;
    	struct bio_dgram_sctp_rcvinfo rinfo;

    	int getmsg_retv=getmsg(sockList[connId].sock, connId, &n, &ssl_err_msg);

    	switch(getmsg_retv){
    	case IPL4_SCTP_WHOLE_MESSAGE_RECEIVED: {
          // asp.remName()=remaddr; ??????

          INTEGER i_ppid;

    	  memset(&rinfo, 0, sizeof(struct bio_dgram_sctp_rcvinfo));
    	  BIO_ctrl(sockList[connId].bio, BIO_CTRL_DGRAM_SCTP_GET_RCVINFO, sizeof(struct bio_dgram_sctp_rcvinfo), &rinfo);
    	  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Incoming data ssl (%ld bytes): stream = %hu, ssn = %hu, flags = %hx, ppid = %u \n", n,
    	             rinfo.rcv_sid,(unsigned int)rinfo.rcv_ssn, rinfo.rcv_flags, rinfo.rcv_ppid);

    	  if (ntohl(rinfo.rcv_ppid) <= (unsigned long)INT_MAX)
    	    i_ppid = ntohl(rinfo.rcv_ppid);
    	  else {
    	    char sbuf[16];
    	  	sprintf(sbuf, "%u", ntohl(rinfo.rcv_ppid));
    	  	i_ppid = INTEGER(sbuf);
          }

    	  asp.proto().dtls().sctp() = SctpTuple(rinfo.rcv_sid, i_ppid, OMIT_VALUE, OMIT_VALUE);
    	  len = (*sockList[connId].buf)->get_len() - n;
    	  sockList[connId].buf[0]->set_pos((size_t)len);
    	  sockList[connId].buf[0]->cut();
    	  asp.msg() = OCTETSTRING(n, sockList[connId].buf[0]->get_data());

    	  if(lazy_conn_id_level && sockListCnt==1 && lonely_conn_id!=-1){
    	    asp.connId()=-1;
    	  }
    	  incoming_message(asp);

    	  if(sockList[connId].buf) (*sockList[connId].buf)->clear();

          if(ssl_err_msg == SSL_ERROR_ZERO_RETURN) {
    	    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SSL_ERROR_ZERO_RETURN received.");

    	    asp.proto().dtls().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);

    	    event.connClosed() = ConnectionClosedEvent(connId,asp.remName(), asp.remPort(),
    			                                     asp.locName(), asp.locPort(),
    	                                             asp.proto(), asp.userData());
    	    if (ConnDel(connId) == -1) {
    	      IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
    	      sendError(PortError::ERROR__SOCKET, connId);
    	    }
    	    incoming_message(event);
    	    closingPeerLen = 0;
          }
    	}
    	break;
        case IPL4_SCTP_PARTIAL_RECEIVE:
    	  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: partial receive: %ld bytes", n);
    	break;
    	case IPL4_SCTP_ERROR_RECEIVED:
    	  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS/SCTP error, Socket is closed.");
    	case IPL4_SCTP_EOF_RECEIVED:
    	  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Socket is closed.");
    	  if (connId == dontCloseConnectionId) {
    	    closingPeer = sa;
    	    closingPeerLen = saLen;
    	    //close(sockList[connId].sock);
    	    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: closing connection %d"
    	               " postponed due to nonblocking send operation", connId);
    	    return;
    	  }

    	  asp.proto().dtls().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);

    	  event.connClosed() = ConnectionClosedEvent(connId,asp.remName(), asp.remPort(),
    			                                     asp.locName(), asp.locPort(),
    	                                             asp.proto(), asp.userData());
    	  if (ConnDel(connId) == -1) {
    	    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: ConnDel failed");
    	    sendError(PortError::ERROR__SOCKET, connId);
    	  }
    	  incoming_message(event);
    	  closingPeerLen = 0;
    	  break;
        case IPL4_SCTP_SENDER_DRY_EVENT:
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SCTP_SENDER_DRY_EVENT did not contain any DTLS data");
          break;
    	default:
          break;
    	}
      }
#endif
#endif
#else
      sendError(PortError::ERROR__UNSUPPORTED__PROTOCOL,
          sockList[connId].sock);
#endif
      break;
    } // IPL4asp_SCTP
    default:
      sendError(PortError::ERROR__UNSUPPORTED__PROTOCOL,
          sockList[connId].sock);
      break;
    } // switch
  }
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: leave");
} // IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable


void IPL4asp__PT_PROVIDER::reportConnOpened(const int client_id) {
  ASP__Event event;

  event.connOpened().remName() = *(sockList[client_id].remoteaddr);
  event.connOpened().remPort() = *(sockList[client_id].remoteport);
  event.connOpened().locName() = *(sockList[client_id].localaddr);
  event.connOpened().locPort() = *(sockList[client_id].localport);

  if(sockList[client_id].type == IPL4asp_UDP) {
    if(sockList[client_id].ssl_tls_type != NONE) {
      event.connOpened().proto().dtls().udp() = UdpTuple(null_type());
    } else {
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::reportConnOpened: unhandled UDP case!");
    }
  } else if((sockList[client_id].type == IPL4asp_TCP) || (sockList[client_id].type == IPL4asp_TCP_LISTEN)) {
    if(sockList[client_id].ssl_tls_type != NONE) {
      event.connOpened().proto().ssl() = SslTuple(null_type());
    } else {
      event.connOpened().proto().tcp() = TcpTuple(null_type());
    }
  }  else if((sockList[client_id].type == IPL4asp_SCTP) || (sockList[client_id].type == IPL4asp_SCTP_LISTEN)) {
    if(sockList[client_id].ssl_tls_type != NONE) {
      event.connOpened().proto().dtls().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    } else {
      event.connOpened().proto().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    }
  } else {
    IPL4_DEBUG( "IPL4asp__PT_PROVIDER::reportConnOpened: unhandled protocol!");
  }

  event.connOpened().connId() = client_id;
  event.connOpened().userData() = sockList[client_id].userData;
  incoming_message(event);
}

#ifdef USE_SCTP
int IPL4asp__PT_PROVIDER::getmsg(int fd, int connId, struct msghdr *msg, void */*buf*/, size_t */*buflen*/,
    ssize_t *nrp, size_t /*cmsglen*/)
{
  if(!sockList[connId].sctpHandshakeCompletedBeforeDtls) {
    *nrp = recvmsg(fd, msg, 0);
  } else {
    // In case of DTLS/SCTP the socket will be accessed by the SSL_read as well.
    // With MSG_PEEK the data is copied into the buffer but is not removed from the input queue
    // so that SSL_read can access the data as well.
    *nrp = recvmsg(fd, msg, MSG_PEEK);
  }
  //IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: nr: %ld",*nrp);
  if (*nrp < 0) {
    /* EOF or error */
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: error: %d, %s",errno,strerror(errno));
    return IPL4_SCTP_ERROR_RECEIVED;
  } else if (*nrp==0){
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: connection closed");
    return IPL4_SCTP_EOF_RECEIVED;
  }
  /* Whole message is received, return it. */
  if (msg->msg_flags & MSG_EOR) {
    return IPL4_SCTP_WHOLE_MESSAGE_RECEIVED;
  }
#else

int IPL4asp__PT_PROVIDER::getmsg(int /*fd*/, int /*connId*/, struct msghdr */*msg*/, void */*buf*/, size_t */*buflen*/,
    ssize_t */*nrp*/, size_t /*cmsglen*/)
{
#endif
  return IPL4_SCTP_PARTIAL_RECEIVE;
}


int IPL4asp__PT_PROVIDER::getmsg(int fd, int connId, ssize_t *nrp, int *ssl_err_msg)
{
#ifdef USE_SCTP

#ifdef IPL4_USE_SSL
  *nrp = receive_ssl_message_on_fd(connId, ssl_err_msg);;
  if (*nrp == 0) {
    switch (sockList[connId].sslState) {
    case STATE_BLOCK_FOR_SENDING:
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: ssl state is STATE_BLOCK_FOR_SENDING, don't close connection.");
      Handler_Remove_Fd_Read(connId);
      sockList[connId].sslState = STATE_DONT_CLOSE;
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: setting socket ssl state to STATE_DONT_CLOSE");
      break;
    case STATE_DONT_CLOSE:
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getmsg: ssl state is STATE_DONT_CLOSE, don't close connection.");
      break;
    default:
      break;
    }
    return IPL4_SCTP_EOF_RECEIVED;
  } else if(*nrp > 0) {
    return IPL4_SCTP_WHOLE_MESSAGE_RECEIVED;
  } else if(*nrp < 0 && *ssl_err_msg != SSL_ERROR_WANT_READ){
    return IPL4_SCTP_ERROR_RECEIVED;
  } else if(*nrp < 0 && *ssl_err_msg == SSL_ERROR_WANT_READ){
    return IPL4_SCTP_SENDER_DRY_EVENT;
  }
#endif
#endif
  return IPL4_SCTP_PARTIAL_RECEIVE;
}

void IPL4asp__PT_PROVIDER::handle_event(int /*fd*/, int connId, const void *buf)
{
#ifdef USE_SCTP
  ASP__Event event;
  union sctp_notification  *snp;
  snp = (sctp_notification *)buf;

  switch (snp->sn_header.sn_type)
  {
  case SCTP_ASSOC_CHANGE:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_ASSOC_CHANGE event.");
    struct sctp_assoc_change *sac;
    sac = &snp->sn_assoc_change;
    if (events.sctp_association_event) {
      event.sctpEvent().sctpAssocChange().clientId() = connId;
      event.sctpEvent().sctpAssocChange().proto().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      switch(sac->sac_state){
      case SCTP_COMM_UP:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__COMM__UP;
        break;

      case SCTP_COMM_LOST:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__COMM__LOST;
        break;

      case SCTP_RESTART:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__RESTART;
        break;

      case SCTP_SHUTDOWN_COMP:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__SHUTDOWN__COMP;
        break;

      case SCTP_CANT_STR_ASSOC:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__CANT__STR__ASSOC;
        break;

      default:
        event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__UNKNOWN__SAC__STATE;
        TTCN_warning("IPL4asp__PT_PROVIDER::handle_event: Unexpected sac_state value received %d", sac->sac_state);
        break;
      }

      incoming_message(event);
      if(sac->sac_state == SCTP_COMM_LOST) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: SCTP_ASSOC_CHANGE event SCTP_COMM_LOST, closing Sock.");
        ProtoTuple proto_close;
        proto_close.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
        sendConnClosed(connId,
            *(sockList[connId].remoteaddr),
            *(sockList[connId].remoteport),
            *(sockList[connId].localaddr),
            *(sockList[connId].localport),
            proto_close, sockList[connId].userData);
        if (ConnDel(connId) == -1) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: ConnDel failed");
          sendError(PortError::ERROR__SOCKET, connId);
        }
        closingPeerLen = 0;

      }
    }
    break;
  case SCTP_PEER_ADDR_CHANGE:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_PEER_ADDR_CHANGE event.");
    struct sctp_paddr_change *spc;
    spc = &snp->sn_paddr_change;
    if (events.sctp_address_event)
    {
      event.sctpEvent().sctpPeerAddrChange().clientId() = connId;
      switch(spc->spc_state)
      {
      case SCTP_ADDR_AVAILABLE:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__AVAILABLE;
        break;

      case SCTP_ADDR_UNREACHABLE:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__UNREACHABLE;
        break;

      case SCTP_ADDR_REMOVED:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__REMOVED;
        break;

      case SCTP_ADDR_ADDED:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__ADDED;
        break;

      case SCTP_ADDR_MADE_PRIM:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__MADE__PRIM;
        break;

#if  defined(LKSCTP_1_0_7) || defined(LKSCTP_1_0_9)
      case SCTP_ADDR_CONFIRMED:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__ADDR__CONFIRMED;
        break;
#endif
      default:
        event.sctpEvent().sctpPeerAddrChange().spc__state() = IPL4asp__Types::SPC__STATE::SCTP__UNKNOWN__SPC__STATE;
        TTCN_warning("IPL4asp__PT_PROVIDER::handle_event: Unexpected spc_state value received %d", spc->spc_state);
        break;
      }
      incoming_message(event);
    }
    break;
  case SCTP_REMOTE_ERROR:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_REMOTE_ERROR event.");
    //      struct sctp_remote_error *sre;
    //      sre = &snp->sn_remote_error;
    if (events.sctp_peer_error_event) {
      event.sctpEvent().sctpRemoteError().clientId() = connId;
      incoming_message(event);
    }
    break;
  case SCTP_SEND_FAILED:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_SEND_FAILED event.");
    //      struct sctp_send_failed *ssf;
    //      ssf = &snp->sn_send_failed;
    if (events.sctp_send_failure_event) {
      event.sctpEvent().sctpSendFailed().clientId() = connId;
      incoming_message(event);
    }
    break;
  case SCTP_SHUTDOWN_EVENT:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_SHUTDOWN_EVENT event.");
    //      struct sctp_shutdown_event *sse;
    //      sse = &snp->sn_shutdown_event;
    if (events.sctp_shutdown_event) {
      event.sctpEvent().sctpShutDownEvent().clientId() = connId;
      incoming_message(event);
    }
    break;
#if  defined(LKSCTP_1_0_7) || defined(LKSCTP_1_0_9)
  case SCTP_ADAPTATION_INDICATION:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_ADAPTATION_INDICATION event.");
    //      struct sctp_adaptation_event *sai;
    //      sai = &snp->sn_adaptation_event;
    if (events.sctp_adaptation_layer_event) {
      event.sctpEvent().sctpAdaptationIndication().clientId() = connId;
      incoming_message(event);
    }
    break;
#else
  case SCTP_ADAPTION_INDICATION:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_ADAPTION_INDICATION event.");
    //      struct sctp_adaption_event *sai;
    //      sai = &snp->sn_adaption_event;
    if (events.sctp_adaption_layer_event) {
      event.sctpEvent().sctpAdaptationIndication().clientId() = connId;
      incoming_message(event);
    }
    break;
#endif
  case SCTP_PARTIAL_DELIVERY_EVENT:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_PARTIAL_DELIVERY_EVENT event.");
    //      struct sctp_pdapi_event *pdapi;
    //      pdapi = &snp->sn_pdapi_event;
    if (events.sctp_partial_delivery_event) {
      event.sctpEvent().sctpPartialDeliveryEvent().clientId() = connId;
      incoming_message(event);
    }
    break;
#ifdef SCTP_SENDER_DRY_EVENT
  case SCTP_SENDER_DRY_EVENT:
	    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: incoming SCTP_SENDER_DRY_EVENT event.");
	if (events.sctp_sender_dry_event) {
	  event.sctpEvent().sctpSenderDryEvent().clientId() = connId;
	  incoming_message(event);
	}
	break;
#endif
  default:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::handle_event: Unknown notification type!");
    break;
  }
#endif
}



void IPL4asp__PT_PROVIDER::user_map(const char *system_port)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::user_map(%s): enter",system_port);
  sockList = NULL;
  sockListCnt = 0;
  firstFreeSock = -1;
  lastFreeSock = -1;
  dontCloseConnectionId = -1;
  closingPeerLen = 0;
  lonely_conn_id = -1;
#ifdef USE_IPL4_EIN_SCTP
  if(!native_stack) do_bind();
#endif
  mapped = true;

  switch(default_mode){
    case 1:  // connect
      if(lazy_conn_id_level!=1){
        TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autoconnect: The lazy_conn_id_level should be \"Yes\" ",system_port);
      }
      if(defaultRemHost==NULL){
        TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autoconnect: The remote host should be specified.",system_port);
      }
      if(defaultRemPort==-1){
        TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autoconnect: The remote port should be specified.",system_port);
      }
      {
        ProtoTuple pt;
        pt.unspecified() = NULL_VALUE;
        switch(default_proto){
          case 1: // TLS
            pt.ssl() = NULL_VALUE;
            break;
          case 2: // SCTP
            pt.sctp() = SctpTuple(OMIT_VALUE,OMIT_VALUE,OMIT_VALUE,OMIT_VALUE);
            break;
          case 3: // UDP
            pt.udp() = NULL_VALUE;
            break;
          default:  // TCP
            pt.tcp() = NULL_VALUE;
            break;
        }
        OptionList op= NULL_VALUE;
        Result res= f__IPL4__PROVIDER__connect(*this,defaultRemHost,defaultRemPort,defaultLocHost,defaultLocPort,-1,pt,op);
        
        if(res.errorCode().ispresent()){ 
          TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autoconnect: Can not connect: %d %s ",system_port,res.os__error__code().ispresent()?(int)res.os__error__code()():-1,res.os__error__text().ispresent()?(const char*)res.os__error__text()():"");
        }
      }
      break;
    case 2: // listen
      if(lazy_conn_id_level!=0){
        TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autolisten: The lazy_conn_id_level should be \"No\" ",system_port);
      }
      {
        ProtoTuple pt;
        pt.unspecified() = NULL_VALUE;
        switch(default_proto){
          case 1: // TLS
            pt.ssl() = NULL_VALUE;
            break;
          case 2: // SCTP
            pt.sctp() = SctpTuple(OMIT_VALUE,OMIT_VALUE,OMIT_VALUE,OMIT_VALUE);
            break;
          case 3: // UDP
            pt.udp() = NULL_VALUE;
            break;
          default:  // TCP
            pt.tcp() = NULL_VALUE;
            break;
        }
        OptionList op= NULL_VALUE;
        Result res= f__IPL4__PROVIDER__listen(*this,defaultLocHost,defaultLocPort,pt,op);
        
        if(res.errorCode().ispresent()){ 
          TTCN_error("IPL4asp__PT_PROVIDER::user_map(%s): Autolisten: Can not listen: %d %s ",system_port,res.os__error__code().ispresent()?(int)res.os__error__code()():-1,res.os__error__text().ispresent()?(const char*)res.os__error__text()():"");
        }
      }
    
      break;
    default: // do nothing
     break;
  }

} // IPL4asp__PT_PROVIDER::user_map


void IPL4asp__PT_PROVIDER::user_unmap(const char *system_port)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::user_unmap(%s): enter",system_port);
  mapped = false;
  if (sockListCnt > 0) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::user_unmap: There are %i open connections",
        sockListCnt);
  }
  if (sockList != 0) {
    for (unsigned int i = 1; i < sockListSize; ++i) {
#ifdef USE_IPL4_EIN_SCTP
      if(!native_stack && sockList[i].sock != SockDesc::SOCK_NONEX && (sockList[i].type==IPL4asp_SCTP_LISTEN || sockList[i].type==IPL4asp_SCTP)){
        sockList[i].next_action=SockDesc::ACTION_DELETE;
        if(sockList[i].type == IPL4asp_SCTP){
          EINSS7_00SctpShutdownReq(sockList[i].sock);
        } else {
          if(sockList[i].ref_count==0){
            EINSS7_00SctpDestroyReq(sockList[i].endpoint_id);
            ConnDelEin(i);
          }
        }

      }
      else
#endif

        if (sockList[i].sock > 0)
          ConnDel(i);
    }
  }
  sockListCnt = 0;
  firstFreeSock = -1;
  lastFreeSock = -1;
#ifdef USE_IPL4_EIN_SCTP
  if(!native_stack) do_unbind();
#endif
  Free(sockList); sockList = 0;
  if(globalConnOpts.dtlsSrtpProfiles) {
    Free(globalConnOpts.dtlsSrtpProfiles);
    globalConnOpts.dtlsSrtpProfiles = NULL;
  }
  lonely_conn_id = -1;
  Uninstall_Handler();
} // IPL4asp__PT_PROVIDER::user_unmap



void IPL4asp__PT_PROVIDER::user_start()
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::user_start: enter");
} // IPL4asp__PT_PROVIDER::user_start



void IPL4asp__PT_PROVIDER::user_stop()
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::user_stop: enter");
} // IPL4asp__PT_PROVIDER::user_stop


bool IPL4asp__PT_PROVIDER::getAndCheckSockType(int connId,
    ProtoTuple::union_selection_type proto, SockType &type)
{
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getAndCheckSockType: invalid connId: %i", connId);
    return false;
  }
  type = sockList[connId].type;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::getAndCheckSockType: sock type is: %i", type);
  if (proto != ProtoTuple::UNBOUND_VALUE &&
      proto != ProtoTuple::ALT_unspecified) {
    /* Proto is specified. It is used for checking only. */
    if(((type == IPL4asp_UDP) && (proto != ProtoTuple::ALT_udp) && (proto != ProtoTuple::ALT_dtls)) ||
        (((type == IPL4asp_TCP_LISTEN) || (type == IPL4asp_TCP)) && ((proto != ProtoTuple::ALT_tcp) && (proto != ProtoTuple::ALT_ssl))) ||
        (((type == IPL4asp_SCTP_LISTEN) || (type == IPL4asp_SCTP)) && (proto != ProtoTuple::ALT_sctp))) {
      return false;
    }
  }
  return true;
}

void IPL4asp__PT_PROVIDER::outgoing_send(const ASP__Send& asp)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send: ASP Send: enter");
  testIfInitialized();
  Socket__API__Definitions::Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  outgoing_send_core(asp, result);
  if(result.errorCode().ispresent()){
    ASP__Event event;
    if(send_extended_result){
      event.extended__result().errorCode() =result.errorCode();
      event.extended__result().connId() =result.connId();
      event.extended__result().os__error__code() =result.os__error__code();
      event.extended__result().os__error__text() =result.os__error__text();
      event.extended__result().msg() = asp.msg();
    } else {
      event.result()=result;
    }
    incoming_message(event);
  }
} // IPL4asp__PT_PROVIDER::outgoing_send

int IPL4asp__PT_PROVIDER::outgoing_send_core(const ASP__Send& asp,  Socket__API__Definitions::Result& result)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP Send: enter");
  testIfInitialized();
  SockType type;
  int local_conn_id=asp.connId();
  if(lazy_conn_id_level && asp.connId()==-1){
    local_conn_id=lonely_conn_id;
  }
  ProtoTuple::union_selection_type proto = ProtoTuple::ALT_unspecified;
  if (asp.proto().ispresent())
    proto = asp.proto()().get_selection();
  if (getAndCheckSockType(local_conn_id, proto, type)) {
    if (asp.proto().ispresent()) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP Send: calling sendNonBlocking with proto");
      return sendNonBlocking(local_conn_id, (sockaddr *)NULL, (socklen_t)0, type, asp.msg(), result, asp.proto());
    } else {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP Send: calling sendNonBlocking without proto");
      return sendNonBlocking(local_conn_id, (sockaddr *)NULL, (socklen_t)0, type, asp.msg(), result);
    }
  }
  else {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP Send: INVALID INPUT PARAMETER");
    setResult(result, PortError::ERROR__INVALID__INPUT__PARAMETER, asp.connId());
  }
  return -1;
} // IPL4asp__PT_PROVIDER::outgoing_send_core

void IPL4asp__PT_PROVIDER::outgoing_send(const ASP__SendTo& asp)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send: ASP Send: enter");
  testIfInitialized();
  Socket__API__Definitions::Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  outgoing_send_core(asp, result);
  if(result.errorCode().ispresent()){
    ASP__Event event;
    if(send_extended_result){
      event.extended__result().errorCode() =result.errorCode();
      event.extended__result().connId() =result.connId();
      event.extended__result().os__error__code() =result.os__error__code();
      event.extended__result().os__error__text() =result.os__error__text();
      event.extended__result().msg() = asp.msg();
    } else {
      event.result()=result;
    }
    incoming_message(event);
  }
}
int IPL4asp__PT_PROVIDER::outgoing_send_core(const ASP__SendTo& asp,  Socket__API__Definitions::Result& result)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP SendTo: enter");
  testIfInitialized();
  SockType type;
  int local_conn_id=asp.connId();
  if(lazy_conn_id_level && local_conn_id==-1){
    local_conn_id=lonely_conn_id;
  }
  ProtoTuple::union_selection_type proto = ProtoTuple::ALT_unspecified;
  if (asp.proto().ispresent())
    proto = asp.proto()().get_selection();
  if (getAndCheckSockType(local_conn_id, proto, type)) {
    if (asp.remPort() < 0 || asp.remPort() > 65535){
      setResult(result, PortError::ERROR__INVALID__INPUT__PARAMETER, asp.connId());
      return -1;
    }
    SockAddr to;
    socklen_t toLen;
    switch (type) {
    case IPL4asp_UDP:
    case IPL4asp_TCP_LISTEN:
    case IPL4asp_TCP:
      //#ifdef IPL4_USE_SSL
      //    case IPL4asp_SSL_LISTEN:
      //    case IPL4asp_SSL:
      //#endif
      if (SetSockAddr(asp.remName(), asp.remPort(), to, toLen) == -1) {
        setResult(result,PortError::ERROR__HOSTNAME, asp.connId());
        return -1;
      }
      break;
#ifdef USE_SCTP
    case IPL4asp_SCTP_LISTEN:
    case IPL4asp_SCTP:
      if (SetSockAddr(asp.remName(), asp.remPort(), to, toLen) == -1) {
        setResult(result,PortError::ERROR__HOSTNAME, asp.connId());
        return -1;
      }
      break;
#endif
    default:
      setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL, asp.connId());
      return -1;
    }
    if (asp.proto().ispresent()) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::outgoing_send_core: ASP SendTo: calling sendNonBlocking with proto");
      return sendNonBlocking(local_conn_id, (sockaddr *)&to, toLen, type, asp.msg(), result, asp.proto());
    } else {
      return sendNonBlocking(local_conn_id, (sockaddr *)&to, toLen, type, asp.msg(), result);
    }
  } else {
    setResult(result, PortError::ERROR__INVALID__INPUT__PARAMETER, asp.connId());
  }
  return -1;
} // IPL4asp__PT_PROVIDER::outgoing_send_core



int IPL4asp__PT_PROVIDER::sendNonBlocking(const ConnectionId& connId, sockaddr *sa,
    socklen_t saLen, SockType type, const OCTETSTRING& msg, Socket__API__Definitions::Result& result, const Socket__API__Definitions::ProtoTuple& protoTuple)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: enter: connId: %d", (int)connId);
  int sock = sockList[(int)connId].sock;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: fd: %d", sock);
  if (sock < 0) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: Connection %s",
        (sock == SockDesc::SOCK_CLOSED) ? "closed" : "does not exist");
    setResult(result,PortError::ERROR__SOCKET, (int)connId);
    return -1;
  }
  int rem = msg.lengthof();
  int sent_octets=0;
  const unsigned char *ptr = (const unsigned char *)msg;

#ifdef IPL4_USE_SSL
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: ssl_tls_type: %d", sockList[(int)connId].ssl_tls_type);
  if((sockList[(int)connId].ssl_tls_type != NONE) && (protoTuple.get_selection() != ProtoTuple::ALT_udp))
  {
//    if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
//    ssl_current_client=(IPL4asp__PT_PROVIDER *)this;

    IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: sslState: %d", sockList[(int)connId].sslState);
    if (sockList[(int)connId].sslState!=STATE_NORMAL)
    {
      // The socket is not writeable, so we subscribe to the event that notifies us when it becomes writable again
      // and we pass up a TEMPORARILY_UNAVAILABLE ASP to inform the user.
      // TODO: This functionality can be improved by buffering the message that we couldn't send thus the user doesn't have to
      // buffer it himself. It would be more efficient to buffer it here anyway (it would mean less ASP traffic)
      Handler_Add_Fd_Write(sock);
      IPL4_DEBUG("DO WRITE ON %i", sock);
      setResult(result,PortError::ERROR__TEMPORARILY__UNAVAILABLE, (int)connId, EAGAIN);
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: leave (TEMPORARILY UNAVAILABLE)   "
          "connId: %i, fd: %i", (int)connId, sock);
//      ssl_current_client=NULL;
      return 0;
    }


    if(!getSslObj((int)connId, ssl_current_ssl))
    {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: current SSL invalid for client: %d", (int)connId);
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: Connection %s",
          (sock == SockDesc::SOCK_CLOSED) ? "closed" : "does not exist");
      setResult(result,PortError::ERROR__SOCKET, (int)connId);
//      ssl_current_client=NULL;
      return -1;
    }

    if (ssl_current_ssl==NULL)
    {
      IPL4_DEBUG("PL4asp__PT_PROVIDER::sendNonBlocking:No SSL data available for client %d",(int)connId);
      setResult(result,PortError::ERROR__SOCKET, (int)connId);
//      ssl_current_client=NULL;
      return -1;
    }
  }
#endif

  while (rem != 0) {
    int ret=-1;
    switch (type) {
    case IPL4asp_UDP:
    case IPL4asp_TCP_LISTEN:
    case IPL4asp_TCP:

      if((sockList[(int)connId].ssl_tls_type == NONE) ||
          (protoTuple.get_selection() == ProtoTuple::ALT_udp)) {  // if UDP is requested over the DTLS, then send UDP unencrypted
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking, sending unencrypted...");
        if (sa != NULL)
          ret = sendto(sock, ptr, rem, 0, sa, saLen);
        else
          ret = ::send(sock, ptr, rem, 0);
      } else {
#ifdef IPL4_USE_SSL
        write_ssl_message_on_fd(&ret, &rem, connId, ptr);
#endif
      } // else branch
      break;
    case IPL4asp_SCTP_LISTEN:
    case IPL4asp_SCTP:

#ifdef USE_IPL4_EIN_SCTP
      if(!native_stack){
        ULONG_T streamID=0;
        ULONG_T payloadProtId=0;

        if (protoTuple.get_selection()==ProtoTuple::ALT_sctp) {

          if(protoTuple.sctp().sinfo__stream().ispresent()) {
            streamID = (int)(protoTuple.sctp().sinfo__stream()());
          }
          if(protoTuple.sctp().sinfo__ppid().ispresent()) {
            payloadProtId=protoTuple.sctp().sinfo__ppid()().get_long_long_val();
          }
        }

        USHORT_T einretval=EINSS7_00SctpSendReq(sock,payloadProtId,rem,(unsigned char*)ptr,streamID,0,NULL,false);
        if(RETURN_OK != einretval){
          setResult(result,PortError::ERROR__SOCKET, (int)connId, einretval);
          return -1;
        } else {
          return rem;
        }
      }
#endif

#ifdef USE_SCTP

     if(sockList[connId].ssl_tls_type == NONE || !sockList[connId].sctpHandshakeCompletedBeforeDtls) {

        struct cmsghdr   *cmsg;
        struct sctp_sndrcvinfo  *sri;
        char cbuf[sizeof (*cmsg) + sizeof (*sri)];
        struct msghdr   msg;
        struct iovec   iov;

        iov.iov_len = rem;

        memset(&msg, 0, sizeof (msg));
        iov.iov_base = (char *)ptr;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cbuf;
        msg.msg_controllen = sizeof (*cmsg) + sizeof (*sri);

        memset(cbuf, 0, sizeof (*cmsg) + sizeof (*sri));
        cmsg = (struct cmsghdr *)cbuf;
        sri = (struct sctp_sndrcvinfo *)(cmsg + 1);

        cmsg->cmsg_len = sizeof (*cmsg) + sizeof (*sri);
        cmsg->cmsg_level = IPPROTO_SCTP;
        cmsg->cmsg_type  = SCTP_SNDRCV;

        sri->sinfo_stream = 0;
        sri->sinfo_ppid = 0;
        if (protoTuple.get_selection()==ProtoTuple::ALT_sctp) {

          if(protoTuple.sctp().sinfo__stream().ispresent()) {
            sri->sinfo_stream = (int)(protoTuple.sctp().sinfo__stream()());
          }
          if(protoTuple.sctp().sinfo__ppid().ispresent()) {
            unsigned int ui;
            INTEGER in = protoTuple.sctp().sinfo__ppid()();
            if (in.is_native() && in > 0)
              ui = (unsigned int)(int)in;
            else {
              ui = (unsigned int)in.get_long_long_val();
            }
            sri->sinfo_ppid = htonl(ui);
          }
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: sctp sinfo: %d, ppid: %d", sri->sinfo_stream,sri->sinfo_ppid);

        ret = ::sendmsg(sock, &msg, 0);
        break;
      }
#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
      else {
        struct bio_dgram_sctp_sndinfo sinfo;
        memset(&sinfo, 0, sizeof(struct bio_dgram_sctp_sndinfo));

        if (protoTuple.get_selection()==ProtoTuple::ALT_sctp) {
	  if(protoTuple.sctp().sinfo__stream().ispresent()) {
            sinfo.snd_sid = (int)(protoTuple.sctp().sinfo__stream()());
	  }
          if(protoTuple.sctp().sinfo__ppid().ispresent()) {
            unsigned int ui;
            INTEGER in = protoTuple.sctp().sinfo__ppid()();
            if (in.is_native() && in > 0) ui = (unsigned int)(int)in;
            else ui = (unsigned int)in.get_long_long_val();
            sinfo.snd_ppid = htonl(ui);
	  }
	}

        BIO_ctrl(sockList[connId].bio, BIO_CTRL_DGRAM_SCTP_SET_SNDINFO, sizeof(struct bio_dgram_sctp_sndinfo), &sinfo);
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: dtls over sctp sinfo: %d, ppid: %d", sinfo.snd_sid,sinfo.snd_ppid);
        write_ssl_message_on_fd(&ret, &rem, connId, ptr);

        break;
      }
#endif
#endif
#endif
    default:
      setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL, (int)connId);
      return -1;
    }

#ifdef IPL4_USE_SSL
    //In case of ssl it is used if the client id does not exist
    //if so, cannot do anything, send an error report and return;
    if(ret == -2)
    {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: Connection %s",
          (sock == SockDesc::SOCK_CLOSED) ? "closed" : "does not exist");
      setResult(result,PortError::ERROR__SOCKET, (int)connId);
//      ssl_current_client=NULL;
      return -1;
    }
#endif

    if (ret != -1) {
      rem -= ret;
      ptr += ret;
      sent_octets += ret;
      continue; // try sending the remaning octets
    }


    IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: error %s", strerror(errno));

    switch (errno) { // error handling
    case EINTR:
      // interrupted signal: try again
      break;
    case EPIPE: { //client closed connection
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: "
          "client closed connection, send event ASP and remove socket");

      int ii = connId;

      ASP__Event event;
      event.connClosed().connId() = connId;
      event.connClosed().remName() = *(sockList[ii].remoteaddr);
      event.connClosed().remPort() = *(sockList[ii].remoteport);
      event.connClosed().locName() = *(sockList[ii].localaddr);
      event.connClosed().locPort() = *(sockList[ii].localport);
      event.connClosed().proto().tcp() = TcpTuple(null_type());
      event.connClosed().userData() = 0;
      int l_ud=-1; getUserData((int)connId, l_ud); event.connClosed().userData() = l_ud;

      switch (type) {
      case IPL4asp_UDP:
        // no such operation in UDP
        break;
      case IPL4asp_TCP_LISTEN:
      case IPL4asp_TCP:
        if(sockList[ii].ssl_tls_type == NONE) {
          event.connClosed().proto().tcp() = TcpTuple(null_type());
        } else {
          event.connClosed().proto().ssl() = SslTuple(null_type());
        }
        break;
      case IPL4asp_SCTP_LISTEN:
      case IPL4asp_SCTP:
        event.connClosed().proto().sctp() =
            SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      default:
        setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL, (int)connId);
        break;
      }
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking");
      /*      if (getsockname(sock, (struct sockaddr *)&sa, &saLen) == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: getsockname error: %s",
              strerror(errno));
        setResult(result,PortError::ERROR__HOSTNAME, (int)connId, errno);
      } else if (!SetNameAndPort((SockAddr *)&sa, saLen,
                                 event.connClosed().locName(),
                                 event.connClosed().locPort())) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: SetNameAndPort failed");
        setResult(result,PortError::ERROR__HOSTNAME, (int)connId);
      }*/
      reportRemainingData_beforeConnClosed(connId, event.connClosed().remName(), event.connClosed().remPort(), event.connClosed().locName(), event.connClosed().locPort(), event.connClosed().proto(), event.connClosed().userData());
      if (ConnDel((int)connId) == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: ConnDel failed");
        setResult(result,PortError::ERROR__SOCKET, (int)connId);
      }
      /*      if (!SetNameAndPort(&closingPeer, closingPeerLen,
                          event.connClosed().remName(),
                          event.connClosed().remPort())) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: SetNameAndPort failed");
        setResult(result,PortError::ERROR__HOSTNAME, (int)connId);
      }*/
      incoming_message(event);
      return -1;
    }

    case EAGAIN: // same as case EWOULDBLOCK:
      if (pureNonBlocking)
      {
        // The socket is not writeable, so we subscribe to the event that notifies us when it becomes writable again
        // and we pass up a TEMPORARILY_UNAVAILABLE ASP to inform the user.
        // TODO: This functionality can be improved by buffering the message that we couldn't send thus the user doesn't have to
        // buffer it himself. It would be more efficient to buffer it here anyway (it would mean less ASP traffic)
        Handler_Add_Fd_Write(sock);
        IPL4_DEBUG("DO WRITE ON %i", sock);
        setResult(result,PortError::ERROR__TEMPORARILY__UNAVAILABLE, (int)connId, EAGAIN);
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: leave (TEMPORARILY UNAVAILABLE)   "
            "connId: %i, fd: %i", (int)connId, sock);
        return sent_octets;
      }
      // If we don't use purenonBlocking mode, we let TITAN work (and block) until the message can be sent:
      dontCloseConnectionId = (int)connId;
      closingPeerLen = 0;
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: waits in TTCN_Snapshot::block_for_sending");
      TTCN_Snapshot::block_for_sending((int)sock);
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: TITAN returned"
          " to send on connection %d, socket %d", (int)connId, sock);
      if (closingPeerLen > 0) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: "
            "postponed close of connection %d", (int)connId);
        ASP__Event event;
        event.connClosed().connId() = connId;
        int ii = connId;

        event.connClosed().remName() = *(sockList[ii].remoteaddr);
        event.connClosed().remPort() = *(sockList[ii].remoteport);
        event.connClosed().locName() = *(sockList[ii].localaddr);
        event.connClosed().locPort() = *(sockList[ii].localport);
        event.connClosed().proto().tcp() = TcpTuple(null_type());
        event.connClosed().userData() = 0;
        int l_ud=-1; getUserData((int)connId, l_ud); event.connClosed().userData() = l_ud;

        switch (type) {
        case IPL4asp_UDP:
          // no such operation in UDP
          break;
        case IPL4asp_TCP_LISTEN:
        case IPL4asp_TCP:
          if(sockList[ii].ssl_tls_type == NONE) {
            event.connClosed().proto().tcp() = TcpTuple(null_type());
          } else {
            event.connClosed().proto().ssl() = SslTuple(null_type());
          }
          break;
        case IPL4asp_SCTP_LISTEN:
        case IPL4asp_SCTP:
          event.connClosed().proto().sctp() =
              SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
        default:
          setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL, (int)connId);
          break;
        }
        /*        if (getsockname(sock, (struct sockaddr *)&sa, &saLen) == -1) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: getsockname error: %s",
                strerror(errno));
          setResult(result,PortError::ERROR__HOSTNAME, (int)connId, errno);
        } else if (!SetNameAndPort((SockAddr *)&sa, saLen,
                                   event.connClosed().locName(),
                                   event.connClosed().locPort())) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: SetNameAndPort failed");
          setResult(result,PortError::ERROR__HOSTNAME, (int)connId);
        }*/

        reportRemainingData_beforeConnClosed(connId, event.connClosed().remName(), event.connClosed().remPort(), event.connClosed().locName(), event.connClosed().locPort(), event.connClosed().proto(), event.connClosed().userData());

        if (ConnDel((int)connId) == -1) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: ConnDel failed");
          setResult(result,PortError::ERROR__SOCKET, (int)connId);
        }
        /*        if (!SetNameAndPort(&closingPeer, closingPeerLen,
                            event.connClosed().remName(),
                            event.connClosed().remPort())) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: SetNameAndPort failed");
          setResult(result,PortError::ERROR__HOSTNAME, (int)connId);
        }*/
        incoming_message(event);
        closingPeerLen = 0;
      }
      dontCloseConnectionId = -1;
      break;
    case ENOBUFS:
      // try again as in EINTR
      break;
    case EBADF: // invalid file descriptor
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: "
          "invalid fd: %d", (int)sock);
      setResult(result,PortError::ERROR__INVALID__CONNECTION, (int)connId, EBADF);
      return -1;
    default:
      setResult(result,PortError::ERROR__SOCKET, (int)connId, errno);
      return -1;
    } // switch (errno)
  } // while
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: leave");
  return sent_octets;
} // IPL4asp__PT::sendNonBlocking

#ifdef IPL4_USE_SSL

void IPL4asp__PT_PROVIDER::write_ssl_message_on_fd(int* ret, int* rem, const int connId, const unsigned char *msg_ptr){
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: Client ID = %d", (int)connId);
  int res;

  // check if client exists
  if (!isConnIdValid((int)connId)){
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd, Client ID %d does not exist.", (int)connId);
    *ret = -2;
    return;
  }

  IPL4_DEBUG("  one write cycle started");
  if (sockList[connId].sslState == STATE_DONT_CLOSE) {
    //goto client_closed_connection;
    //process postponed connection close
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: Process postponed connection close.");
    SSL_set_quiet_shutdown(ssl_current_ssl, 1);
    IPL4_DEBUG("Setting SSL SHUTDOWN mode to QUIET");
//          ssl_current_client=NULL;
    IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::write_ssl_message_on_fd()");
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_NORMAL");
    sockList[(int)connId].sslState = STATE_NORMAL;
    errno = EPIPE;
    *ret = -1;
    return;
  } else {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: sending message");
    res = ssl_getresult(SSL_write(ssl_current_ssl, msg_ptr, *rem));
  }

  switch (res) {
  case SSL_ERROR_NONE:
//          ssl_current_client=NULL;
    IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::write_ssl_message_on_fd() %d bytes is sent.", *rem);
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_NORMAL");
    sockList[(int)connId].sslState = STATE_NORMAL;
    *ret = *rem; //all bytes are sent without any problem
    break;
  case SSL_ERROR_WANT_WRITE:
    int old_bufsize, new_bufsize;
    if (increase_send_buffer((int)connId, old_bufsize, new_bufsize)) {
      IPL4_DEBUG("Sending data on on file descriptor %d",(int)connId);
      IPL4_DEBUG("The sending operation would block execution. The "
	             "size of the outgoing buffer was increased from %d to "
	             "%d bytes.",old_bufsize,
	             new_bufsize);
      //retry to send
      *ret = 0;
    } else {
      log_warning("Sending data on file descriptor %d", (int)connId);
      log_warning("The sending operation would block execution and it "
	              "is not possible to further increase the size of the "
	              "outgoing buffer. Trying to process incoming data to "
	              "avoid deadlock.");
//            ssl_current_client=NULL;
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_BLOCK_FOR_SENDING");
      sockList[(int)connId].sslState = STATE_BLOCK_FOR_SENDING;
      //continue with EAGAIN
      errno = EAGAIN;
      *ret = -1;
    }
    break;
  case SSL_ERROR_WANT_READ:
//receiving buffer is probably empty thus reading would block execution
    if(!pureNonBlocking)
    {
      IPL4_DEBUG("SSL_write cannot read data from socket %d. Trying to process data to avoid deadlock.", (int)connId);
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_DONT_RECEIVE");
      sockList[(int)connId].sslState = STATE_DONT_RECEIVE; //don't call receive_message_on_fd() to this socket
      for (;;) {
        TTCN_Snapshot::take_new(TRUE);
        pollfd pollClientFd = {sockList[(int)connId].sock, POLLIN, 0 };
        int nEvents = poll(&pollClientFd, 1, 0);
        if (nEvents == 1 && (pollClientFd.revents & (POLLIN | POLLHUP)) != 0)
	      break;
        if (nEvents < 0 && errno != EINTR)
        {
	      IPL4_DEBUG("System call poll() failed on file descriptor %d", sockList[(int)connId].sock);
	      SSL_set_quiet_shutdown(ssl_current_ssl, 1);
	      IPL4_DEBUG("Setting SSL SHUTDOWN mode to QUIET");
	      IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::write_ssl_message_on_fd()");
	      IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_NORMAL");
	      sockList[(int)connId].sslState = STATE_NORMAL;
	      *ret = -1;
	      break;
        }
      }
      IPL4_DEBUG("Deadlock resolved");
    }
    else
    {
      errno = EAGAIN;
      *ret = -1;
    }
    break;
  case SSL_ERROR_ZERO_RETURN:
    log_warning("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: SSL connection was interrupted by the other side");
    SSL_set_quiet_shutdown(ssl_current_ssl, 1);
    IPL4_DEBUG("Setting SSL SHUTDOWN mode to QUIET");
//          ssl_current_client=NULL;
    IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::write_ssl_message_on_fd()");
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::write_ssl_message_on_fd: setting socket state to STATE_NORMAL");
    sockList[(int)connId].sslState = STATE_NORMAL;
    errno = EPIPE;
    *ret = -1;
    break;
  default:
    IPL4_DEBUG("SSL error occured");
    SSL_set_quiet_shutdown(ssl_current_ssl, 1);
    IPL4_DEBUG("Setting SSL SHUTDOWN mode to QUIET");
    IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::write_ssl_message_on_fd()");
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: setting socket state to STATE_NORMAL");
    sockList[(int)connId].sslState = STATE_NORMAL;
    errno = EPIPE;
    *ret = -1;
  }//end switch res
}
#endif
void IPL4asp__PT_PROVIDER::set_ssl_supp_option(const int& conn_id, const IPL4asp__Types::OptionList& options){
//  sockList[conn_id].ssl_supp.SSLv2=globalConnOpts.ssl_supp.SSLv2;  // Moveed to connAdd
//  sockList[conn_id].ssl_supp.SSLv3=globalConnOpts.ssl_supp.SSLv3;
//  sockList[conn_id].ssl_supp.TLSv1=globalConnOpts.ssl_supp.TLSv1;
// sockList[conn_id].ssl_supp.TLSv1_1=globalConnOpts.ssl_supp.TLSv1_1;

  IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: set SSL options");

  for (int k = 0; k < options.size_of(); ++k) {
    switch (options[k].get_selection()) {
    case Option::ALT_ssl__support:{
        const SSL__proto__support &sp= options[k].ssl__support();
        for (int i=0; i<sp.size_of(); ++i){
          switch(sp[i].get_selection()){
            case SSL__protocols::ALT_SSLv2__supported: sockList[conn_id].ssl_supp.SSLv2=sp[i].SSLv2__supported(); break;
            case SSL__protocols::ALT_SSLv3__supported: sockList[conn_id].ssl_supp.SSLv3=sp[i].SSLv3__supported(); break;
            case SSL__protocols::ALT_TLSv1__supported: sockList[conn_id].ssl_supp.TLSv1=sp[i].TLSv1__supported(); break;
            case SSL__protocols::ALT_TLSv1__1__supported: sockList[conn_id].ssl_supp.TLSv1_1=sp[i].TLSv1__1__supported(); break;
            case SSL__protocols::ALT_TLSv1__2__supported: sockList[conn_id].ssl_supp.TLSv1_2=sp[i].TLSv1__2__supported(); break;
            case SSL__protocols::ALT_DTLSv1__supported: sockList[conn_id].ssl_supp.DTLSv1=sp[i].DTLSv1__supported(); break;
            case SSL__protocols::ALT_DTLSv1__2__supported: sockList[conn_id].ssl_supp.DTLSv1_2=sp[i].DTLSv1__2__supported(); break;
            default: break;
          }
        }
        return;
      }
      break;
    case Option::ALT_dtlsSrtpProfiles: {
        if(sockList[conn_id].dtlsSrtpProfiles) {
          Free(sockList[conn_id].dtlsSrtpProfiles);
        }
        sockList[conn_id].dtlsSrtpProfiles = mcopystr((const char*)options[k].dtlsSrtpProfiles());
      }
      break;
    case Option::ALT_cert__options: {
        if(options[k].cert__options().ssl__key__file().ispresent()){
          Free(sockList[conn_id].ssl_key_file);
          sockList[conn_id].ssl_key_file= mcopystr((const char*)options[k].cert__options().ssl__key__file()());
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: setting ssl key file");
        }
        if(options[k].cert__options().ssl__certificate__file().ispresent()){
          Free(sockList[conn_id].ssl_certificate_file);
          sockList[conn_id].ssl_certificate_file= mcopystr((const char*)options[k].cert__options().ssl__certificate__file()());
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: ssl ssl certificate file");
        }
        if(options[k].cert__options().ssl__trustedCAlist__file().ispresent()){
          Free(sockList[conn_id].ssl_trustedCAlist_file);
          sockList[conn_id].ssl_trustedCAlist_file= mcopystr((const char*)options[k].cert__options().ssl__trustedCAlist__file()());
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: setting ssl trusted CA list");
        }
        if(options[k].cert__options().ssl__cipher__list().ispresent()){
          Free(sockList[conn_id].ssl_cipher_list);
          sockList[conn_id].ssl_cipher_list= mcopystr((const char*)options[k].cert__options().ssl__cipher__list()());
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: setting sll cipher list");
        }
        if(options[k].cert__options().ssl__password().ispresent()){
          Free(sockList[conn_id].ssl_password);
          sockList[conn_id].ssl_password= mcopystr((const char*)options[k].cert__options().ssl__password()());
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::set_ssl_supp_option: setting ssl password");
        }
        
      }
      break;
    case Option::ALT_alpn__list:{
        delete sockList[conn_id].alpn;
        sockList[conn_id].alpn= new OCTETSTRING(0,NULL);
        for(int f=0;f<options[k].alpn__list().lengthof();f++){
          *sockList[conn_id].alpn= (*sockList[conn_id].alpn) + int2oct(options[k].alpn__list()[f].lengthof(),1) + char2oct(options[k].alpn__list()[f]);
        }
      }
#if OPENSSL_VERSION_NUMBER < 0x1000200fL
      TTCN_warning("TLS extension alpn is not supported by the used OpenSSL.");
#endif
      break;
    case Option::ALT_tls__hostname:{
        if(sockList[conn_id].tls_hostname){
          *sockList[conn_id].tls_hostname=options[k].tls__hostname();
        } else {
          sockList[conn_id].tls_hostname=new CHARSTRING(options[k].tls__hostname());
        }
      }
#ifndef SSL_CTRL_SET_TLSEXT_HOSTNAME
      TTCN_warning("TLS extension hostname is not supported by the used OpenSSL.");
#endif      
      break;
    default: break;
    }
  }
  return;
}

/*bool IPL4asp__PT_PROVIDER::setDtlsSrtpProfiles(const IPL4asp__Types::ConnectionId& connId, const IPL4asp__Types::OptionList& options)
{
#ifdef IPL4_USE_SSL
  // set the SRTP profiles for the connId
  for (int i = 0; i < options.size_of(); i++) {
    if(options[i].get_selection() == Option::ALT_dtlsSrtpProfiles) {
      char* profiles = mcopystr((const char*)options[i].dtlsSrtpProfiles());
      if(connId == -1) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setDtlsSrtpProfiles: storing global SRTP profiles %s...", profiles);
        if(globalConnOpts.dtlsSrtpProfiles) {
          Free(globalConnOpts.dtlsSrtpProfiles);
        }
        globalConnOpts.dtlsSrtpProfiles = profiles;
      } else {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setDtlsSrtpProfiles: storing SRTP profiles of connId %d to %s...", (int)connId, profiles);
        if(sockList[(int)connId].dtlsSrtpProfiles) {
          Free(sockList[(int)connId].dtlsSrtpProfiles);
        }
        sockList[(int)connId].dtlsSrtpProfiles = profiles;
      }
    }
  }
#endif
  return true;
}
*/
bool IPL4asp__PT_PROVIDER::setOptions(const OptionList& options,
    int sock, const Socket__API__Definitions::ProtoTuple& proto, bool beforeBind)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: enter, number of options: %i",
      options.size_of());
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: sock: %i", sock);
  bool allProto = proto.get_selection() == ProtoTuple::ALT_unspecified;
  bool udpProto = proto.get_selection() == ProtoTuple::ALT_udp || allProto;
  bool tcpProto = proto.get_selection() == ProtoTuple::ALT_tcp || allProto;
  bool sslProto = proto.get_selection() == ProtoTuple::ALT_ssl || allProto;
  bool sctpProto = proto.get_selection() == ProtoTuple::ALT_sctp || allProto;
  //  if (options.size_of() > 2 ||
  //      (options.size_of() == 2 && options[0].get_selection() == options[1].get_selection())) {
  //    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Invalid options");
  //    return false;
  //  }
  int iR = -1, iK = -1, iM = -1, iL = -1, iSSL=-1, iNoDelay=-1, iFreeBind=-1;
#ifdef IPL4_USE_SSL
  int iS =-1;
#endif
  for (int i = 0; i < options.size_of(); ++i) {
    switch (options[i].get_selection()) {
    case Option::ALT_reuseAddress: iR = i; break;
    case Option::ALT_tcpKeepAlive: iK = i; break;
    case Option::ALT_sctpEventHandle: iM = i; break;
#ifdef IPL4_USE_SSL
    case Option::ALT_sslKeepAlive: iS = i; break;
#endif
    case Option::ALT_solinger: iL = i; break;
    case Option::ALT_ssl__support: iSSL = i; break;
    case Option::ALT_no__delay: iNoDelay = i; break;
    case Option::ALT_freebind: iFreeBind = i; break;
    case Option::ALT_dtlsSrtpProfiles: {
        if(sock == -1){
          if(globalConnOpts.dtlsSrtpProfiles) {
            Free(globalConnOpts.dtlsSrtpProfiles);
          }
          globalConnOpts.dtlsSrtpProfiles = mcopystr((const char*)options[i].dtlsSrtpProfiles());
        }
      }
      break;
    default: break;
    }
  }

    int enable = GlobalConnOpts::NOT_SET;

    // Process FREEBIND
    if(iFreeBind!=-1 && sock==-1){  // Store the global option
      globalConnOpts.freebind = ((bool)options[iFreeBind].freebind())?GlobalConnOpts::YES:GlobalConnOpts::NO;
    }

    //Set the FREEBIND option
    if(sock != -1 && beforeBind && (iFreeBind!=-1 || globalConnOpts.freebind != GlobalConnOpts::NOT_SET )){
#ifdef   IP_FREEBIND    
      int flag=iFreeBind!=-1?(((bool)options[iFreeBind].freebind())?1:0):(globalConnOpts.freebind==GlobalConnOpts::YES?1:0);
      if (setsockopt(sock, IPPROTO_IP, IP_FREEBIND ,  &flag, sizeof(flag))<0){
        IPL4_DEBUG( "f__IPL4__PROVIDER__setOptions: setsockopt IP_FREEBIND on "
            "socket %d failed: %d %s", sock, errno,strerror(errno));
        return false;
      }
      IPL4_DEBUG( "IPL4asp__PT_PROVIDER::setOptions: IP option IP_FREEBIND on "
          "socket %d is set to: %d", sock,flag );
#else
     TTCN_warning("The IP option IP_FREEBIND is not supported by the OS. Use the ");
#endif      
    }

    // TCP/SSL/SCTP: set no delay option
    int no_delay_mode=GlobalConnOpts::NOT_SET;
    if (iNoDelay != -1){
      if (sock == -1) { // register global option
        if (!tcpProto && !sctpProto && !sslProto) {
          IPL4_DEBUG( "IPL4asp__PT_PROVIDER::setOptions: Unsupported protocol for NO_DELAY");
          return false;
        }
        if (options[iNoDelay].no__delay()){
          enable = GlobalConnOpts::YES;
        } else {
          enable = GlobalConnOpts::NO;
        }

        if (tcpProto) globalConnOpts.tcp_nodelay = enable;
        if (sctpProto) globalConnOpts.sctp_nodelay = enable;

      } else {
        if (options[iNoDelay].no__delay()){
          no_delay_mode = GlobalConnOpts::YES;
        } else {
          no_delay_mode = GlobalConnOpts::NO;
        }
      }
    }
    if(no_delay_mode==GlobalConnOpts::NOT_SET && sock != -1){
      if (tcpProto) no_delay_mode = globalConnOpts.tcp_nodelay;
      if (sctpProto) no_delay_mode = globalConnOpts.sctp_nodelay;
    }
    if(no_delay_mode!=GlobalConnOpts::NOT_SET && sock != -1) {
      int flag=no_delay_mode==GlobalConnOpts::YES?1:0;
#ifdef USE_SCTP
      if(sctpProto) {
        if (setsockopt(sock, IPPROTO_SCTP, SCTP_NODELAY, (char *) &flag, sizeof(flag))<0){
          IPL4_DEBUG( "f__IPL4__PROVIDER__setOptions: setsockopt SCTP_NODELAY on "
              "socket %d failed: %d %s", sock, errno,strerror(errno));
          return false;
        }
        IPL4_DEBUG( "IPL4asp__PT_PROVIDER::setOptions: SCTP option SCTP_NODELAY on "
            "socket %d is set to: %d", sock,flag );

      } else
#endif
      {  // TCP,SSL
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag))<0){
          IPL4_DEBUG( "f__IPL4__PROVIDER__setOptions: setsockopt TCP_NODELAY on "
              "socket %d failed: %d %s", sock, errno,strerror(errno));
          return false;
        }
        IPL4_DEBUG( "IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_NODELAY on "
            "socket %d is set to: %d", sock,flag );
      }
    }

  /* Supported SSL protocols*/
  if(sock == -1 && iSSL != -1) {
    const SSL__proto__support &sp= options[iSSL].ssl__support();
    for (int i=0; i<sp.size_of(); ++i){
      switch(sp[i].get_selection()){
      case SSL__protocols::ALT_SSLv2__supported: globalConnOpts.ssl_supp.SSLv2=sp[i].SSLv2__supported(); break;
      case SSL__protocols::ALT_SSLv3__supported: globalConnOpts.ssl_supp.SSLv3=sp[i].SSLv3__supported(); break;
      case SSL__protocols::ALT_TLSv1__supported: globalConnOpts.ssl_supp.TLSv1=sp[i].TLSv1__supported(); break;
      case SSL__protocols::ALT_TLSv1__1__supported: globalConnOpts.ssl_supp.TLSv1_1=sp[i].TLSv1__1__supported(); break;
      case SSL__protocols::ALT_TLSv1__2__supported: globalConnOpts.ssl_supp.TLSv1_2=sp[i].TLSv1__2__supported(); break;
      case SSL__protocols::ALT_DTLSv1__supported: globalConnOpts.ssl_supp.DTLSv1=sp[i].DTLSv1__supported(); break;
      case SSL__protocols::ALT_DTLSv1__2__supported: globalConnOpts.ssl_supp.DTLSv1_2=sp[i].DTLSv1__2__supported(); break;
      default: break;
      }
    }
  }

  /* set SO_LINGER */
  if(sock != -1 && iL != -1 && (tcpProto || sctpProto )){
    struct linger so_linger;


    so_linger.l_onoff = options[iL].solinger().l__onoff();
    so_linger.l_linger = options[iL].solinger().l__linger();
    if (setsockopt(sock, SOL_SOCKET,SO_LINGER , &so_linger, sizeof(so_linger)) < 0) {
      IPL4_DEBUG("f__IPL4__PROVIDER__setOptions: setsockopt SO_LINGER on "
          "socket %d failed: %d %s", sock, errno,strerror(errno));
      return false;
    }
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option SO_LINGER on "
        "socket %d is set to: enabled: %d, value %d", sock,so_linger.l_onoff,so_linger.l_linger );

  } else if (iL!=-1){
    IPL4_DEBUG("f__IPL4__PROVIDER__setOptions: SO_LINGER called for not connected TCP or SCTP socket ");
    return false;
  }

  /* Setting reuse address */
  enable = GlobalConnOpts::NOT_SET;
  if (iR != -1) {
    if (!tcpProto && !udpProto && !sctpProto && !sslProto) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Unsupported protocol for reuse address");
      return false;
    }
    enable = GlobalConnOpts::YES;
    if (options[iR].reuseAddress().enable().ispresent() &&
        options[iR].reuseAddress().enable()() == FALSE)
      enable = GlobalConnOpts::NO;
    if (sock == -1) {
      if (tcpProto) globalConnOpts.tcpReuseAddr = enable;
      if (udpProto) globalConnOpts.udpReuseAddr = enable;
      if (sctpProto) globalConnOpts.sctpReuseAddr = enable;
      if (sslProto) globalConnOpts.sctpReuseAddr = enable;
    }
  }
  if (sock != -1 && (iR != -1 || beforeBind)) {
    if (enable == GlobalConnOpts::NOT_SET) {
      if (allProto)
        return false;
      if (tcpProto) enable = globalConnOpts.tcpReuseAddr;
      else if (udpProto) enable = globalConnOpts.udpReuseAddr;
      else if (sctpProto) enable = globalConnOpts.sctpReuseAddr;
      else if (sslProto) enable = globalConnOpts.sslReuseAddr;
    }
    if (enable == GlobalConnOpts::YES) {
      int r = 1;
      if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
          (const char*)&r, sizeof(r)) < 0) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt REUSEADDR on "
            "socket %d failed: %s", sock, strerror(errno));
        return false;
      }
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Socket option REUSEADDR on "
          "socket %d is set to: %i", sock, r);
    }
  }

  // Set broadcast for UDP
  if(sock != -1 && udpProto )
  {
    if(broadcast){
      int on=1;
      if( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on) ) < 0 )
      {
        TTCN_error("Setsockopt error: SO_BROADCAST");
      } else {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::sendNonBlocking: Socket option SO_BROADCAST on ");
      }
    }
  }  

  /* Setting keep alive TCP*/
  if (iK != -1 && !tcpProto) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Unsupported protocol for tcp keep alive");
    return false;
  }
  if (tcpProto) {
    enable = globalConnOpts.tcpKeepAlive;
    int count = globalConnOpts.tcpKeepCnt;
    int idle = globalConnOpts.tcpKeepIdle;
    int interval = globalConnOpts.tcpKeepIntvl;
    if (iK != -1) {
      if (options[iK].tcpKeepAlive().enable().ispresent()) {
        enable = GlobalConnOpts::NO;
        if (options[iK].tcpKeepAlive().enable()() == TRUE)
          enable = GlobalConnOpts::YES;
      }
      if (options[iK].tcpKeepAlive().count().ispresent())
        count = options[iK].tcpKeepAlive().count()();
      if (options[iK].tcpKeepAlive().idle().ispresent())
        idle = options[iK].tcpKeepAlive().idle()();
      if (options[iK].tcpKeepAlive().interval().ispresent())
        interval = options[iK].tcpKeepAlive().interval()();
      if (count < 0 || idle < 0 || interval < 0) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Invalid tcp keep alive parameter");
        return false;
      }
      if (sock == -1) {
        globalConnOpts.tcpKeepAlive = enable;
        globalConnOpts.tcpKeepCnt = count;
        globalConnOpts.tcpKeepIdle = idle;
        globalConnOpts.tcpKeepIntvl = interval;
      }
    }
    if (sock != -1 && (iK != -1 || beforeBind)) {
#ifdef LINUX
      if (count != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
            (const char*)&count, sizeof(count)) < 0) {
          IPL4_DEBUG("f__IPL4__PROVIDER__connect: setsockopt TCP_KEEPCNT on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPCNT on "
            "socket %d is set to: %i", sock, count);
      }
      if (idle != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,
            (const char*)&idle, sizeof(idle)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt TCP_KEEPIDLE on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPIDLE on "
            "socket %d is set to: %i", sock, idle);
      }
      if (interval != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
            (const char*)&interval, sizeof(interval)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt TCP_KEEPINTVL on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPINTVL on "
            "socket %d is set to: %i", sock, interval);
      }
#endif
      if (enable != GlobalConnOpts::NOT_SET) {
        int r = (enable == GlobalConnOpts::YES) ? 1 : 0;
        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
            (const char*)&r, sizeof(r)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt SO_KEEPALIVE on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: socket option SO_KEEPALIVE on "
            "socket %d is set to: %i", sock, r);
      }
    }
  }

#ifdef IPL4_USE_SSL
  /* Setting keep alive SSL*/
  if (iS != -1 && !sslProto) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Unsupported protocol for ssl keep alive");
    return false;
  }
  if (sslProto) {
    enable = globalConnOpts.sslKeepAlive;
    int count = globalConnOpts.sslKeepCnt;
    int idle = globalConnOpts.sslKeepIdle;
    int interval = globalConnOpts.sslKeepIntvl;
    if (iS != -1) {
      if (options[iS].sslKeepAlive().enable().ispresent()) {
        enable = GlobalConnOpts::NO;
        if (options[iS].sslKeepAlive().enable()() == TRUE)
          enable = GlobalConnOpts::YES;
      }
      if (options[iS].sslKeepAlive().count().ispresent())
        count = options[iS].sslKeepAlive().count()();
      if (options[iS].sslKeepAlive().idle().ispresent())
        idle = options[iS].sslKeepAlive().idle()();
      if (options[iS].sslKeepAlive().interval().ispresent())
        interval = options[iS].sslKeepAlive().interval()();
      if (count < 0 || idle < 0 || interval < 0) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Invalid ssl keep alive parameter");
        return false;
      }
      if (sock == -1) {
        globalConnOpts.sslKeepAlive = enable;
        globalConnOpts.sslKeepCnt = count;
        globalConnOpts.sslKeepIdle = idle;
        globalConnOpts.sslKeepIntvl = interval;
      }
    }
    if (sock != -1 && (iS != -1 || beforeBind)) {
#ifdef LINUX
      if (count != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
            (const char*)&count, sizeof(count)) < 0) {
          IPL4_DEBUG("f__IPL4__PROVIDER__connect: setsockopt TCP_KEEPCNT on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPCNT on "
            "socket %d is set to: %i", sock, count);
      }
      if (idle != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,
            (const char*)&idle, sizeof(idle)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt TCP_KEEPIDLE on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPIDLE on "
            "socket %d is set to: %i", sock, idle);
      }
      if (interval != GlobalConnOpts::NOT_SET) {
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
            (const char*)&interval, sizeof(interval)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt TCP_KEEPINTVL on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: TCP option TCP_KEEPINTVL on "
            "socket %d is set to: %i", sock, interval);
      }
#endif
      if (enable != GlobalConnOpts::NOT_SET) {
        int r = (enable == GlobalConnOpts::YES) ? 1 : 0;
        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
            (const char*)&r, sizeof(r)) < 0) {
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt SO_KEEPALIVE on "
              "socket %d failed: %s", sock, strerror(errno));
          return false;
        }
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: socket option SO_KEEPALIVE on "
            "socket %d is set to: %i", sock, r);
      }
    }
  }
#endif

  /* Setting sctp events & inits */
  if (iM != -1 && !sctpProto) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Unsupported protocol for sctp events");
    return false;
  }
  if (sctpProto) {
#ifdef USE_SCTP
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: Setting sctp options sinit_num_ostreams:%d,"
        "sinit_max_instreams:%d, sinit_max_attempts:%d, sinit_max_init_timeo:%d",
        (int) globalConnOpts.sinit_num_ostreams, (int) globalConnOpts.sinit_max_instreams,
        (int) globalConnOpts.sinit_max_attempts, (int) globalConnOpts.sinit_max_init_timeo);
    struct sctp_initmsg initmsg;
    (void) memset(&initmsg, 0, sizeof(struct sctp_initmsg));
    initmsg.sinit_num_ostreams = (int) globalConnOpts.sinit_num_ostreams;
    initmsg.sinit_max_instreams = (int) globalConnOpts.sinit_max_instreams;
    initmsg.sinit_max_attempts = (int) globalConnOpts.sinit_max_attempts;
    initmsg.sinit_max_init_timeo = (int) globalConnOpts.sinit_max_init_timeo;

    if(sock!=-1){
      if (setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg,
          sizeof(struct sctp_initmsg)) < 0)
      {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: setsockopt: SCTP init on "
            "socket %d failed: %s", sock, strerror(errno));
      }
    }

    if(sctp_PMTU_size) {
#if defined LKSCTP_1_0_7 || defined LKSCTP_1_0_9
      struct sctp_paddrparams paddrparams;
      (void) memset(&paddrparams, 0, sizeof(struct sctp_paddrparams));
      paddrparams.spp_pathmtu = sctp_PMTU_size;
      paddrparams.spp_flags = SPP_PMTUD_DISABLE;

      if (setsockopt(sock, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &paddrparams,
          sizeof(struct sctp_paddrparams)) < 0)
      {
        TTCN_warning( "IPL4asp__PT_PROVIDER::setOptions: setsockopt: SCTP setting PMTU: %d "
            "socket %d failed: %s", sctp_PMTU_size, sock, strerror(errno));
      } else {
        IPL4_DEBUG( "IPL4asp__PT_PROVIDER::setOptions: setsockopt: SCTP setting PMTU: %d "
            "socket %d success!", sctp_PMTU_size, sock);
      }
#else
        TTCN_warning( "IPL4asp__PT_PROVIDER::setOptions: setsockopt: SCTP setting PMTU is not supported. The lksctp package is too old");
#endif
    }

    if (iM != -1) {
      if (options[iM].sctpEventHandle().sctp__data__io__event().ispresent()) {
        globalConnOpts.sctp_data_io_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__data__io__event()() == TRUE)
          globalConnOpts.sctp_data_io_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__association__event().ispresent()) {
        globalConnOpts.sctp_association_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__association__event()() == TRUE)
          globalConnOpts.sctp_association_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__address__event().ispresent()) {
        globalConnOpts.sctp_address_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__address__event()() == TRUE)
          globalConnOpts.sctp_address_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__send__failure__event().ispresent()) {
        globalConnOpts.sctp_send_failure_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__send__failure__event()() == TRUE)
          globalConnOpts.sctp_send_failure_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__peer__error__event().ispresent()) {
        globalConnOpts.sctp_peer_error_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__peer__error__event()() == TRUE)
          globalConnOpts.sctp_peer_error_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__shutdown__event().ispresent()) {
        globalConnOpts.sctp_shutdown_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__shutdown__event()() == TRUE)
          globalConnOpts.sctp_shutdown_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__partial__delivery__event().ispresent()) {
        globalConnOpts.sctp_partial_delivery_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__partial__delivery__event()() == TRUE)
          globalConnOpts.sctp_partial_delivery_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__adaptation__layer__event().ispresent()) {
        globalConnOpts.sctp_adaptation_layer_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__adaptation__layer__event()() == TRUE)
          globalConnOpts.sctp_adaptation_layer_event = GlobalConnOpts::YES;
      }
      if (options[iM].sctpEventHandle().sctp__authentication__event().ispresent()) {
        globalConnOpts.sctp_authentication_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__authentication__event()() == TRUE)
          globalConnOpts.sctp_authentication_event = GlobalConnOpts::YES;
      }
#ifdef SCTP_SENDER_DRY_EVENT
      if (options[iM].sctpEventHandle().sctp__sender__dry__event().ispresent()) {
        globalConnOpts.sctp_sender_dry_event = GlobalConnOpts::NO;
        if (options[iM].sctpEventHandle().sctp__sender__dry__event()() == TRUE)
          globalConnOpts.sctp_sender_dry_event = GlobalConnOpts::YES;
      }
#endif
    }

    (void) memset(&events, 0, sizeof(events));
    events.sctp_association_event = (boolean) globalConnOpts.sctp_association_event;
    events.sctp_address_event = (boolean) globalConnOpts.sctp_address_event;
    events.sctp_send_failure_event = (boolean) globalConnOpts.sctp_send_failure_event;
    events.sctp_peer_error_event = (boolean) globalConnOpts.sctp_peer_error_event;
    events.sctp_shutdown_event = (boolean) globalConnOpts.sctp_shutdown_event;
    events.sctp_partial_delivery_event = (boolean) globalConnOpts.sctp_partial_delivery_event;
#ifdef SCTP_SENDER_DRY_EVENT
    events.sctp_sender_dry_event = (boolean) globalConnOpts.sctp_sender_dry_event;
#endif
#ifdef LKSCTP_1_0_7
    events.sctp_adaptation_layer_event = (boolean) globalConnOpts.sctp_adaptation_layer_event;
#elif defined LKSCTP_1_0_9
    events.sctp_adaptation_layer_event = (boolean) globalConnOpts.sctp_adaptation_layer_event;
    events.sctp_authentication_event = (boolean) globalConnOpts.sctp_authentication_event;
#else
    events.sctp_adaption_layer_event = (boolean) globalConnOpts.sctp_adaptation_layer_event;
#endif
    if(sock!=-1){
      if (setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof (events)) < 0)
      {
        TTCN_warning("Setsockopt error!");
        errno = 0;
      }
    }
#endif
  }
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::setOptions: leave");
  return true;
}


int IPL4asp__PT_PROVIDER::ConnAdd(SockType type, int sock, SSL_TLS_Type ssl_tls_type, const IPL4asp__Types::OptionList  *options, int parentIdx)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER: ConnAdd enter: type: %d, ssl_tls_type: %d, sock: %d, parentIx: %d", type, ssl_tls_type, sock, parentIdx);
  testIfInitialized();
  if (sockListCnt + N_RECENTLY_CLOSED >= sockListSize - 1 || sockList == NULL) {
    unsigned int sz = sockListSize;
    if (sockList != NULL) sz *= 2;
    SockDesc *newSockList =
        (SockDesc *)Realloc(sockList, sizeof(SockDesc) * sz);
    int i0 = (sockList == 0) ? 1 : sockListSize;
    sockList = newSockList;
    sockListSize = sz;
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: new sockListSize: %d", sockListSize);
    int j = firstFreeSock;
    for ( int i = sockListSize - 1; i >= i0; --i ) {
      memset(sockList + i, 0, sizeof (sockList[i]));
      sockList[i].sock = SockDesc::SOCK_NONEX;
      sockList[i].nextFree = j;
      j = i;
    }
    firstFreeSock = j;
    if (lastFreeSock == -1) lastFreeSock = sockListSize - 1;
  }

  int i = firstFreeSock;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: connId: %d", i);

  if (parentIdx != -1) { // inherit the listener's properties
    sockList[i].userData = sockList[parentIdx].userData;
    sockList[i].getMsgLen = sockList[parentIdx].getMsgLen;
    sockList[i].getMsgLen_forConnClosedEvent = sockList[parentIdx].getMsgLen_forConnClosedEvent;
    sockList[i].parentIdx = parentIdx;
    sockList[i].msgLenArgs =
        new ro__integer(*sockList[parentIdx].msgLenArgs);
    sockList[i].msgLenArgs_forConnClosedEvent =
        new ro__integer(*sockList[parentIdx].msgLenArgs_forConnClosedEvent);
    sockList[i].ssl_supp.SSLv2=sockList[parentIdx].ssl_supp.SSLv2;
    sockList[i].ssl_supp.SSLv3=sockList[parentIdx].ssl_supp.SSLv3;
    sockList[i].ssl_supp.TLSv1=sockList[parentIdx].ssl_supp.TLSv1;
    sockList[i].ssl_supp.TLSv1_1=sockList[parentIdx].ssl_supp.TLSv1_1;
    sockList[i].ssl_supp.TLSv1_2=sockList[parentIdx].ssl_supp.TLSv1_2;
    sockList[i].ssl_supp.DTLSv1=sockList[parentIdx].ssl_supp.DTLSv1;
    sockList[i].ssl_supp.DTLSv1_2=sockList[parentIdx].ssl_supp.DTLSv1_2;
    if(sockList[parentIdx].dtlsSrtpProfiles){
      sockList[i].dtlsSrtpProfiles = mcopystr(sockList[parentIdx].dtlsSrtpProfiles);
    } else {
      sockList[i].dtlsSrtpProfiles = NULL;
    }
    if(sockList[parentIdx].ssl_key_file){
      sockList[i].ssl_key_file = mcopystr(sockList[parentIdx].ssl_key_file);
    } else {
      sockList[i].ssl_key_file = NULL;
    }
    if(sockList[parentIdx].ssl_certificate_file){
      sockList[i].ssl_certificate_file = mcopystr(sockList[parentIdx].ssl_certificate_file);
    } else {
      sockList[i].ssl_certificate_file = NULL;
    }
    if(sockList[parentIdx].ssl_cipher_list){
      sockList[i].ssl_cipher_list = mcopystr(sockList[parentIdx].ssl_cipher_list);
    } else {
      sockList[i].ssl_cipher_list = NULL;
    }
    if(sockList[parentIdx].ssl_trustedCAlist_file){
      sockList[i].ssl_trustedCAlist_file = mcopystr(sockList[parentIdx].ssl_trustedCAlist_file);
    } else {
      sockList[i].ssl_trustedCAlist_file = NULL;
    }
    if(sockList[parentIdx].ssl_password){
      sockList[i].ssl_password = mcopystr(sockList[parentIdx].ssl_password);
    } else {
      sockList[i].ssl_password = NULL;
    }

    if(sockList[parentIdx].tls_hostname){
     sockList[i].tls_hostname = new CHARSTRING(*sockList[parentIdx].tls_hostname);
    } else {
      sockList[i].tls_hostname = NULL;
    }
    if(sockList[parentIdx].alpn){
     sockList[i].alpn = new OCTETSTRING(*sockList[parentIdx].alpn);
    } else {
      sockList[i].alpn =NULL;
    }
    
    
  } else { // otherwise initialize to defaults
    sockList[i].userData = 0;
    sockList[i].getMsgLen = defaultGetMsgLen;
    sockList[i].getMsgLen_forConnClosedEvent = defaultGetMsgLen_forConnClosedEvent;
    sockList[i].parentIdx = -1;
    sockList[i].msgLenArgs = new ro__integer(*defaultMsgLenArgs);
    sockList[i].msgLenArgs_forConnClosedEvent = new ro__integer(*defaultMsgLenArgs_forConnClosedEvent);
    sockList[i].ssl_supp.SSLv2=globalConnOpts.ssl_supp.SSLv2;
    sockList[i].ssl_supp.SSLv3=globalConnOpts.ssl_supp.SSLv3;
    sockList[i].ssl_supp.TLSv1=globalConnOpts.ssl_supp.TLSv1;
    sockList[i].ssl_supp.TLSv1_1=globalConnOpts.ssl_supp.TLSv1_1;
    sockList[i].ssl_supp.TLSv1_2=globalConnOpts.ssl_supp.TLSv1_2;
    sockList[i].ssl_supp.DTLSv1=globalConnOpts.ssl_supp.DTLSv1;
    sockList[i].ssl_supp.DTLSv1_2=globalConnOpts.ssl_supp.DTLSv1_2;
    sockList[i].dtlsSrtpProfiles = NULL;
    sockList[i].ssl_key_file = NULL;
    sockList[i].ssl_certificate_file = NULL;
    sockList[i].ssl_trustedCAlist_file = NULL;
    sockList[i].ssl_cipher_list = NULL;
    sockList[i].ssl_password = NULL;
    sockList[i].tls_hostname = NULL;
    sockList[i].alpn =NULL;
    if(options){
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: connId: set ssl options for connId : %d", i);
      set_ssl_supp_option(i,*options);
    }
  }

  sockList[i].msgLen = -1;

  fd2IndexMap[sock] = i;
  sockList[i].type = type;
  sockList[i].ssl_tls_type = ssl_tls_type;
  sockList[i].localaddr=new CHARSTRING("");
  sockList[i].localport=new PortNumber(-1);
  sockList[i].remoteaddr=new CHARSTRING("");
  sockList[i].remoteport=new PortNumber(-1);

#ifdef IPL4_USE_SSL
  sockList[i].sslObj = NULL;
  sockList[i].bio = NULL;
  sockList[i].sslCTX = NULL;
#endif

  sockList[i].sctpHandshakeCompletedBeforeDtls = false;

  // Set local socket details
  SockAddr sa;
  socklen_t saLen = sizeof(SockAddr);
  if (getsockname(sock,(struct sockaddr *)&sa, &saLen) == -1) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: getsockname error: %s",
        strerror(errno));
    return -1;
  } else if (!SetNameAndPort(&sa, saLen,
      *(sockList[i].localaddr), *(sockList[i].localport))) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: SetNameAndPort failed");
    return -1;
  }

  sockList[i].buf = NULL;
  sockList[i].assocIdList = NULL;
  sockList[i].cnt = 0;

  switch (type) {
  case IPL4asp_TCP_LISTEN:
  case IPL4asp_SCTP_LISTEN:
    break;
  case IPL4asp_UDP:
  //case IPL4asp_UDP_LIGHT:
  case IPL4asp_TCP:
  case IPL4asp_SCTP:
    sockList[i].buf = (TTCN_Buffer **)Malloc(sizeof(TTCN_Buffer *));
    *sockList[i].buf = new TTCN_Buffer;
    if (*sockList[i].buf == NULL) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: failed to add socket %d", sock);
      Free(sockList[i].buf); sockList[i].buf = 0;
      return -1;
    }
    if(type == IPL4asp_TCP) {
      sockList[i].assocIdList = NULL;
    } else {
      // IPL4asp_SCTP
      sockList[i].assocIdList = (sctp_assoc_t *)Malloc(sizeof(sctp_assoc_t));
    }
    sockList[i].cnt = 1;
    if (getpeername(sock, (struct sockaddr *)&sa, &saLen) == -1) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: getpeername failed: %s", strerror(errno));
      //      sendError(PortError::ERROR__HOSTNAME, i, errno);
    } else if (!SetNameAndPort(&sa, saLen, *(sockList[i].remoteaddr), *(sockList[i].remoteport))) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: SetNameAndPort failed");
      sendError(PortError::ERROR__HOSTNAME, i);
    }
    break;
  }

  switch(ssl_tls_type) {
  case NONE:
    // nothing to be done
    break;
  case SERVER:
    sockList[i].sslState = STATE_NORMAL;
    break;
  case CLIENT:
    sockList[i].sslState = STATE_CONNECTING;
    sockList[i].server = false;
    break;
  default:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: unhandled SSL/TLS type");
    sendError(PortError::ERROR__GENERAL, i);
    break;
  }

  Handler_Add_Fd_Read(sock);

  sockList[i].sock = sock;
  firstFreeSock = sockList[i].nextFree;
  sockList[i].nextFree = -1;
  ++sockListCnt;
  if(sockListCnt==1) {lonely_conn_id=i;}
  else {lonely_conn_id=-1;}
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: leave: sockListCnt: %i", sockListCnt);

  return i;
} // IPL4asp__PT::ConnAdd

void SockDesc::clear()
{
  for (unsigned int i = 0; i < cnt; ++i) delete buf[i];
  cnt = 0;
  Free(buf); buf = 0;
  Free(assocIdList); assocIdList = 0;
  delete msgLenArgs; msgLenArgs = 0;
  delete msgLenArgs_forConnClosedEvent; msgLenArgs_forConnClosedEvent = 0;
  delete localaddr; localaddr = 0;
  delete localport; localport = 0;
  delete remoteaddr; remoteaddr = 0;
  delete remoteport; remoteport = 0;

  delete tls_hostname; tls_hostname = NULL;
  
  delete alpn; alpn=NULL;

  sock = SOCK_NONEX;
  msgLen = -1;
  nextFree = -1;
}

int IPL4asp__PT_PROVIDER::ConnDel(int connId)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnDel: enter: connId: %d", connId);
  int sock = sockList[connId].sock;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnDel: fd: %d", sock);
  if (sock <= 0)
    return -1;
  Handler_Remove_Fd(sock, EVENT_ALL);

#ifdef IPL4_USE_SSL
  if((sockList[connId].ssl_tls_type != NONE) &&
      (sockList[connId].type != IPL4asp_SCTP_LISTEN) &&
      (sockList[connId].type != IPL4asp_TCP_LISTEN) &&
      mapped) perform_ssl_shutdown(connId);

  sockList[connId].sctpHandshakeCompletedBeforeDtls = false;
#endif

  if (close(sock) == -1) {
    TTCN_warning("IPL4asp__PT_PROVIDER::ConnDel: failed to close socket"
        " %d: %s, connId: %d", sock, strerror(errno), connId);
  }

  fd2IndexMap.erase(sockList[connId].sock);

  sockList[connId].clear();
  sockList[lastFreeSock].nextFree = connId;
  lastFreeSock = connId;
  sockListCnt--;
  if(sockList[connId].dtlsSrtpProfiles){
    Free(sockList[connId].dtlsSrtpProfiles);
    sockList[connId].dtlsSrtpProfiles=NULL;
  }
  Free(sockList[connId].ssl_key_file);
  sockList[connId].ssl_key_file = NULL;
  Free(sockList[connId].ssl_certificate_file );
  sockList[connId].ssl_certificate_file = NULL;
  Free(sockList[connId].ssl_trustedCAlist_file);
  sockList[connId].ssl_trustedCAlist_file = NULL;
  Free(sockList[connId].ssl_cipher_list);
  sockList[connId].ssl_cipher_list = NULL;
  Free(sockList[connId].ssl_password);
  sockList[connId].ssl_password = NULL;

  return connId;
} // IPL4asp__PT_PROVIDER::ConnDel


int IPL4asp__PT_PROVIDER::setUserData(int connId, int userData)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::setUserData enter: connId %d userdata %d",
      connId, userData);
  testIfInitialized();
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setUserData: invalid connId: %i", connId);
    return -1;
  }
  sockList[connId].userData = userData;
  return connId;
} // IPL4asp__PT_PROVIDER::setUserData



int IPL4asp__PT_PROVIDER::getUserData(int connId, int& userData)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::getUserData enter: socket %d", connId);
  testIfInitialized();
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getUserData: invalid connId: %i", connId);
    return -1;
  }
  userData = sockList[connId].userData;
  return connId;
} // IPL4asp__PT_PROVIDER::getUserData



int IPL4asp__PT_PROVIDER::getConnectionDetails(int connId, IPL4__Param IPL4param, IPL4__ParamResult& IPL4paramResult)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails enter: socket %d", connId);
  testIfInitialized();
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails: invalid connId: %i", connId);
    return -1;
  }
  SockAddr sa;
  socklen_t saLen = sizeof(SockAddr);
  SockType type;
  switch (IPL4param) {
  case IPL4__Param::IPL4__LOCALADDRESS:
    if (getsockname(sockList[connId].sock,
        (struct sockaddr *)&sa, &saLen) == -1) {
      IPL4paramResult.local().hostName() = "?";
      IPL4paramResult.local().portNumber() = -1;
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails: getsockname error: %s",
          strerror(errno));
      return -1;
    }
    if (!SetNameAndPort(&sa, saLen,
        IPL4paramResult.local().hostName(),
        IPL4paramResult.local().portNumber())) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails: SetNameAndPort failed");
      return -1;
    }
    break;
  case IPL4__Param::IPL4__REMOTEADDRESS:
    if (getpeername(sockList[connId].sock,
        (struct sockaddr *)&sa, &saLen) == -1) {
      IPL4paramResult.remote().hostName() = "?";
      IPL4paramResult.remote().portNumber() = -1;
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails: getpeername error: %s",
          strerror(errno));
      return -1;
    }
    if (!SetNameAndPort(&sa, saLen,
        IPL4paramResult.remote().hostName(),
        IPL4paramResult.remote().portNumber())) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::getConnectionDetails: SetNameAndPort failed");
      return -1;
    }
    break;
  case IPL4__Param::IPL4__PROTO:
    type = sockList[connId].type;
    switch (type) {
    case IPL4asp_UDP:
      IPL4paramResult.proto().udp() = UdpTuple(null_type());
      break;
    case IPL4asp_TCP_LISTEN:
    case IPL4asp_TCP:
      if(sockList[connId].ssl_tls_type == NONE) {
        IPL4paramResult.proto().tcp() = TcpTuple(null_type());
      } else {
        IPL4paramResult.proto().ssl() = SslTuple(null_type());
      }
      break;
    case IPL4asp_SCTP_LISTEN:
    case IPL4asp_SCTP:
      IPL4paramResult.proto().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      break;
    default: break;
    }
    break;
    case IPL4__Param::IPL4__USERDATA:
      IPL4paramResult.userData() = sockList[connId].userData;
      break;
    case IPL4__Param::IPL4__PARENTIDX:
      IPL4paramResult.parentIdx() = sockList[connId].parentIdx;
      break;
    default: break;
  }

  return connId;
} // IPL4asp__PT_PROVIDER::getConnectionDetails


void IPL4asp__PT_PROVIDER::sendError(PortError code, const ConnectionId& id,
    int os_error_code)
{
  ASP__Event event;
  event.result().errorCode() = code;
  event.result().connId() = id;
  if (os_error_code != 0) {
    event.result().os__error__code() = os_error_code;
    event.result().os__error__text() = strerror(os_error_code);
  } else {
    event.result().os__error__code() = OMIT_VALUE;
    event.result().os__error__text() = OMIT_VALUE;
  }
  incoming_message(event);
} // IPL4asp__PT_PROVIDER::sendError

void IPL4asp__PT_PROVIDER::reportRemainingData_beforeConnClosed(const ConnectionId& id,
    const CHARSTRING& remoteaddr,
    const PortNumber& remoteport,
    const CHARSTRING& localaddr,
    const PortNumber& localport,
    const ProtoTuple& proto,
    const int& userData)
{
  // check if the remaining data is to be reported to the TTCN layer
  if((sockList[id].getMsgLen_forConnClosedEvent != NULL) &&
      (sockList[id].buf != NULL)) {
    bool msgFound = false;
    do {
      OCTETSTRING oct;
      (*sockList[id].buf)->get_string(oct);
      sockList[id].msgLen = sockList[id].getMsgLen_forConnClosedEvent.invoke(oct,*sockList[id].msgLenArgs_forConnClosedEvent);

      msgFound = (sockList[id].msgLen > 0) && (sockList[id].msgLen <= (int)sockList[id].buf[0]->get_len());
      if (msgFound) {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::reportRemainingData_beforeConnClosed: message length: (%d/%d bytes)\n",
            sockList[id].msgLen, (int)sockList[id].buf[0]->get_len());
        ASP__RecvFrom asp;
        asp.connId() = id;
        asp.userData() = userData;
        asp.remName() = remoteaddr;
        asp.remPort() = remoteport;
        asp.locName() = localaddr;
        asp.locPort() = localport;
        asp.proto() = proto;
        asp.msg() = OCTETSTRING(sockList[id].msgLen, sockList[id].buf[0]->get_data());
        sockList[id].buf[0]->set_pos((size_t)sockList[id].msgLen);
        sockList[id].buf[0]->cut();
        if(lazy_conn_id_level && sockListCnt==1 && lonely_conn_id!=-1){
          asp.connId()=-1;
        }
        incoming_message(asp);
        sockList[id].msgLen = -1;
      }
    } while (msgFound && sockList[id].buf[0]->get_len() != 0);
    if (sockList[id].buf[0]->get_len() != 0)
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::reportRemainingData_beforeConnClosed: incomplete message remained (%d bytes)\n",
          (int)sockList[id].buf[0]->get_len());
  }
}

void IPL4asp__PT_PROVIDER::sendConnClosed(const ConnectionId& id,
    const CHARSTRING& remoteaddr,
    const PortNumber& remoteport,
    const CHARSTRING& localaddr,
    const PortNumber& localport,
    const ProtoTuple& proto,
    const int& userData)
{
  ASP__Event event_close;
  event_close.connClosed() = ConnectionClosedEvent(id,
      remoteaddr,
      remoteport,
      localaddr,
      localport,
      proto, userData);
  incoming_message(event_close);
} // IPL4asp__PT_PROVIDER::sendConnClosed

void IPL4asp__PT_PROVIDER::setResult(Socket__API__Definitions::Result& result, PortError code, const ConnectionId& id,
    int os_error_code)
{
  result.errorCode() = code;
  result.connId() = id;
  if (os_error_code != 0) {
    result.os__error__code() = os_error_code;
    result.os__error__text() = strerror(os_error_code);
  } else {
    result.os__error__code() = OMIT_VALUE;
    result.os__error__text() = OMIT_VALUE;
  }
} // IPL4asp__PT_PROVIDER::setResult


void IPL4asp__PT_PROVIDER::starttls(const ConnectionId& connId, const BOOLEAN& server_side, Socket__API__Definitions::Result& result){

  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("starttls connId: %d: server_side: %s", (int)connId, server_side ? "yes" : "no");
    TTCN_Logger::end_event();
  }

#ifdef IPL4_USE_SSL
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: invalid connId: %i", (int)connId);
    setResult(result,PortError::ERROR__INVALID__CONNECTION,connId,0);
    return;
  }

  switch(sockList[connId].type) {
  case IPL4asp_UDP:
  //case IPL4asp_UDP_LIGHT:
  case IPL4asp_TCP:
  case IPL4asp_SCTP:
  case IPL4asp_TCP_LISTEN:
  case IPL4asp_SCTP_LISTEN:
    sockList[connId].ssl_tls_type = server_side ? SERVER : CLIENT;
    break;
  default:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: Unsupported protocol");
    setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL,connId,0);
  }

  sockList[connId].server = server_side;
  sockList[connId].sslState = STATE_CONNECTING;
  sockList[connId].sslObj = NULL;

  if((sockList[connId].type == IPL4asp_UDP || sockList[connId].type == IPL4asp_SCTP_LISTEN ||
      sockList[connId].type == IPL4asp_SCTP) and server_side) {

    if(!ssl_create_contexts_and_obj(connId)) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: SSL initialization failed. Client: %d", (int)connId);
      setResult(result,PortError::ERROR__SOCKET,connId,0);
      return;
    } else {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: set server side BIO, connId: %d", (int)connId);
      if(sockList[connId].type == IPL4asp_UDP) sockList[connId].bio = BIO_new_dgram(sockList[connId].sock, BIO_NOCLOSE);
#ifdef OPENSSL_SCTP_SUPPORT
      if(sockList[connId].type == IPL4asp_SCTP_LISTEN || sockList[connId].type == IPL4asp_SCTP) sockList[connId].bio = BIO_new_dgram_sctp(sockList[connId].sock, BIO_NOCLOSE);
#endif
      SSL_set_bio(sockList[connId].sslObj, sockList[connId].bio, sockList[connId].bio);
      /* Enable cookie exchange */
      if(!(sockList[connId].type == IPL4asp_SCTP_LISTEN || sockList[connId].type == IPL4asp_SCTP)) SSL_set_options(sockList[connId].sslObj, SSL_OP_COOKIE_EXCHANGE);
      // in DTLS server case, wait the client to ping us, and call DTLSv1_listen in the handleevent_readable
      return;
    }

  }

  // if we are DTLS client, then add BIO, then go further with the perform handshake
  if((sockList[connId].type == IPL4asp_UDP || sockList[connId].type == IPL4asp_SCTP_LISTEN  || 
      sockList[connId].type == IPL4asp_SCTP) and !server_side) {
    if(!ssl_create_contexts_and_obj(connId)) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: SSL initialization failed. Client: %d", (int)connId);
      setResult(result,PortError::ERROR__SOCKET,connId,0);
      return;
    } else {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: set client side BIO, connId: %d", (int)connId);
      if(sockList[connId].type == IPL4asp_UDP) sockList[connId].bio = BIO_new_dgram(sockList[connId].sock, BIO_NOCLOSE);

      SockAddr sa_server;
      socklen_t sa_len;
      SetSockAddr((const char*)*sockList[connId].remoteaddr, (int)*sockList[connId].remoteport, sa_server, sa_len);
      if(sockList[connId].type == IPL4asp_UDP) BIO_ctrl(sockList[connId].bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &sa_server);
#ifdef OPENSSL_SCTP_SUPPORT
      if(sockList[connId].type == IPL4asp_SCTP) sockList[connId].bio = BIO_new_dgram_sctp(sockList[connId].sock, BIO_NOCLOSE);
#endif
      SSL_set_bio(sockList[connId].sslObj, sockList[connId].bio, sockList[connId].bio);
    }
  }

  IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: before calling perform_ssl_handshake(%d)", (int)connId);
  int res=perform_ssl_handshake(connId);

  switch(res){
  case SUCCESS:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: handshaking is successful");
    sockList[connId].sslState = STATE_NORMAL;
    break;
  case WANT_WRITE:
  case WANT_READ:
    if(!server_side) {
      // wake me up when the client is receiving something
      Handler_Add_Fd_Read(sockList[connId].sock);
    } else {
      // wake me up when a client is ringing to make an incoming connection
      Handler_Add_Fd_Write(sockList[connId].sock);
    }

    setResult(result,PortError::ERROR__TEMPORARILY__UNAVAILABLE,connId,0);
    break;
  default:
    setResult(result,PortError::ERROR__SOCKET,connId,0);
  }

#else
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::starttls: Unsupported protocol");
  setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL,connId,0);

#endif
  return;
}

void IPL4asp__PT_PROVIDER::stoptls(const ConnectionId& connId, Socket__API__Definitions::Result& result){

  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("stoptls connId: %d", (int)connId);
    TTCN_Logger::end_event();
  }

#ifdef IPL4_USE_SSL
  if(!perform_ssl_shutdown(connId)) {
    setResult(result,PortError::ERROR__UNSUPPORTED__PROTOCOL,connId,0);
  }
#endif
}

OCTETSTRING IPL4asp__PT_PROVIDER::exportTlsKey(
  const IPL4asp__Types::ConnectionId& connId,
  const CHARSTRING& label,
  const OCTETSTRING& context,
  const INTEGER& keyLen) {

  if (!isConnIdValid(connId)) {
    TTCN_warning("Invalid connection id: %d", (int)connId);
    return OCTETSTRING(0, NULL);
  }

#ifdef IPL4_USE_SSL
#ifdef SRTP_AES128_CM_SHA1_80
  if(!sockList[(int)connId].sslObj) {
    TTCN_warning("Connection id: %d is not an SSL connection", (int)connId);
    return OCTETSTRING(0, NULL);
  }

  unsigned char* out = (unsigned char*)Malloc(keyLen + 1);

  int ret = SSL_export_keying_material(
      sockList[(int)connId].sslObj,
      out,
      (int)keyLen,
      label,
      label.lengthof(),
      context,
      context.lengthof(),
      context.lengthof() > 0);

  if(!ret) {
    TTCN_warning("Can not export TLS1 key of connection: %d", (int)connId);
    ssl_getresult(ret);
    Free(out);
    return OCTETSTRING(0, NULL);
  }

  OCTETSTRING final = OCTETSTRING(keyLen, out);
  Free(out);
  return final;
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1  in order to use the exportTlsKey function!");
  sendError(PortError::ERROR__SOCKET, connId);
  return OCTETSTRING(0, NULL);
#endif // IPL4_USE_SSL
}

IPL4__SrtpKeysAndSalts IPL4asp__PT_PROVIDER::exportSrtpKeysAndSalts(
  const IPL4asp__Types::ConnectionId& connId) {

  IPL4__SrtpKeysAndSalts ret(
      OCTETSTRING(0, NULL),
      OCTETSTRING(0, NULL),
      OCTETSTRING(0, NULL),
      OCTETSTRING(0, NULL));

  if (!isConnIdValid(connId)) {
    TTCN_warning("Invalid connection id: %d", (int)connId);
    return ret;
  }

#ifdef IPL4_USE_SSL
#ifdef SRTP_AES128_CM_SHA1_80
  if(!sockList[(int)connId].sslObj) {
    TTCN_warning("Connection id: %d is not an SSL connection", (int)connId);
    return ret;
  }

  // SRTP (rfc3711) defines Key length as 16, Salt length as 14
  // we need here space both for the client and server
  unsigned char* out = (unsigned char*)Malloc((16 + 14) * 2);

  int retv = SSL_export_keying_material(
      sockList[(int)connId].sslObj,
      out,
      (16 + 14) * 2,
      "EXTRACTOR-dtls_srtp",
      19,
      NULL,
      0,
      0);

  if(!retv) {
    TTCN_warning("Can not export DTLS keys of connection: %d", (int)connId);
    ssl_getresult(retv);
    Free(out);
    return ret;
  }

  if(sockList[(int)connId].ssl_tls_type == CLIENT) {
    ret.localKey() = OCTETSTRING(16, out);
    ret.remoteKey() = OCTETSTRING(16, out + 16);
    ret.localSalt() = OCTETSTRING(14, out + 32);
    ret.remoteSalt() = OCTETSTRING(14, out + 46);
  } else {
    ret.remoteKey() = OCTETSTRING(16, out);
    ret.localKey() = OCTETSTRING(16, out + 16);
    ret.remoteSalt() = OCTETSTRING(14, out + 32);
    ret.localSalt() = OCTETSTRING(14, out + 46);
  }

  return ret;
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1 in order to use the exportSrtpKeysAndSalts function!");
  sendError(PortError::ERROR__SOCKET, connId);
  return ret;
#endif // IPL4_USE_SSL
}

OCTETSTRING IPL4asp__PT_PROVIDER::exportSctpKey(
  const IPL4asp__Types::ConnectionId& connId) {

  if (!isConnIdValid(connId)) {
    TTCN_warning("Invalid connection id: %d", (int)connId);
    return OCTETSTRING(0, NULL);
  }

#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
  if(!sockList[(int)connId].sslObj) {
    TTCN_warning("Connection id: %d is not an SSL connection", (int)connId);
    return OCTETSTRING(0, NULL);
  }

  // RFC 6083: 64-byte shared secret key is derived from every master secret
  unsigned char sctpauthkey[64];

  // derive secret key from master key
  int ret = SSL_export_keying_material(
      sockList[(int)connId].sslObj,
      sctpauthkey,
      sizeof(sctpauthkey),
      DTLS1_SCTP_AUTH_LABEL,
      sizeof(DTLS1_SCTP_AUTH_LABEL),
      NULL,
      0,
      0);

  if(!ret) {
    TTCN_warning("Can not export SSL key of connection: %d", (int)connId);
    ssl_getresult(ret);
    return OCTETSTRING(0, NULL);
  }

  OCTETSTRING final = OCTETSTRING(sizeof(sctpauthkey), sctpauthkey);

  return final;
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: SCTP over DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1q is required!");
#endif
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1q  in order to use the exportSctpKey function!");
  sendError(PortError::ERROR__SOCKET, connId);
  return OCTETSTRING(0, NULL);
#endif // IPL4_USE_SSL
}

#ifdef IPL4_USE_SSL
CHARSTRING IPL4_get_certificate_fingerprint(const X509 *x509, const IPL4__DigestMethods& method) {
  CHARSTRING ret("");
  unsigned char fingerprint_buff[EVP_MAX_MD_SIZE];
  unsigned int fingerprint_size;

  const EVP_MD* fingerprint_type = EVP_sha1();
  switch(method) {
    case IPL4__DigestMethods::NULL__method:
      fingerprint_type = EVP_md_null();
      break;
    //MD2, MDC2 is disabled in OpenSSL
    /*case IPL4__DigestMethods::MD2:
      fingerprint_type = EVP_md2();
      break;
    case IPL4__DigestMethods::MDC2:
      fingerprint_type = EVP_mdc2();
      break;*/
    case IPL4__DigestMethods::MD4:
      fingerprint_type = EVP_md4();
      break;
    case IPL4__DigestMethods::MD5:
      fingerprint_type = EVP_md5();
      break;
#if OPENSSL_VERSION_NUMBER < 0x10100000L 
    case IPL4__DigestMethods::SHA:
      fingerprint_type = EVP_sha();
      break;
#endif
    case IPL4__DigestMethods::SHA1:
      fingerprint_type = EVP_sha1();
      break;
#if OPENSSL_VERSION_NUMBER < 0x10100000L 
    case IPL4__DigestMethods::DSS:
      fingerprint_type = EVP_dss();
      break;
    case IPL4__DigestMethods::DSS1:
      fingerprint_type = EVP_dss1();
      break;
    case IPL4__DigestMethods::ECDSA:
      fingerprint_type = EVP_ecdsa();
      break;
#endif
    case IPL4__DigestMethods::SHA224:
      fingerprint_type = EVP_sha224();
      break;
    case IPL4__DigestMethods::SHA256:
      fingerprint_type = EVP_sha256();
      break;
    case IPL4__DigestMethods::SHA384:
      fingerprint_type = EVP_sha384();
      break;
    case IPL4__DigestMethods::SHA512:
      fingerprint_type = EVP_sha512();
      break;
    case IPL4__DigestMethods::RIPEMD160:
      fingerprint_type = EVP_ripemd160();
      break;
#ifdef SRTP_AES128_CM_SHA1_80
    case IPL4__DigestMethods::WHIRLPOOL:
      fingerprint_type = EVP_whirlpool();
      break;
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
    default:
      TTCN_warning("IPL4_get_certificate_fingerprint: requested fingerprint type is not handled by IPL4 testport!");
      return ret;
  }

  if(!fingerprint_type) {
    TTCN_warning("IPL4_get_certificate_fingerprint: requested fingerprint type is not handled by OpenSSL!");
    return ret;
  }

  if (X509_digest(x509, fingerprint_type, fingerprint_buff, &fingerprint_size)) {
    //ssl_own_fingerprint = CHARSTRING(fingerprint_size, fingerprint_buff);
    CHARSTRING colon = CHARSTRING(":");
    char buf[3];
    //fprintf(stderr, "fingerprint: %s\n", fingerprint_buff);
    for (unsigned int i = 0; i < fingerprint_size; i++) {
      sprintf(buf, "%02X", fingerprint_buff[i]);
      ret = ret + CHARSTRING(2, buf);
      if(i < fingerprint_size - 1) { ret = ret + colon; }
    }
  } else {
    TTCN_warning("IPL4asp__PT_PROVIDER::IPL4_get_certificate_fingerprint: could not calculate fingerprint");
  }

  return ret;
}
#endif

CHARSTRING IPL4asp__PT_PROVIDER::getLocalCertificateFingerprint(const IPL4__DigestMethods& method,const ConnectionId& connId, const CHARSTRING& certificate__file) {

#ifdef IPL4_USE_SSL
  if(!ssl_init_SSL(connId)) {
    return "";
  }

  BIO* certificate_bio = BIO_new(BIO_s_file());

  const char* cf=NULL;
  if(certificate__file.lengthof()!=0){
    cf=(const char*)certificate__file;
  }else if(ssl_cert_per_conn && isConnIdValid(connId) && sockList[(int)connId].ssl_certificate_file){
    cf=sockList[(int)connId].ssl_certificate_file;
  } else {
    cf=ssl_certificate_file;
  }
  if(!BIO_read_filename(certificate_bio, cf)) {
    TTCN_warning("IPL4asp__PT_PROVIDER::getLocalCertificateFingerprint: could not load certificate file %s @1!", cf);
    return "";
  }
  X509 *x509 = PEM_read_bio_X509(certificate_bio, NULL, 0, NULL);
  if (!x509) {
    TTCN_warning("IPL4asp__PT_PROVIDER::getLocalCertificateFingerprint: could not load certificate file @2!");
    return "";
  }

  return IPL4_get_certificate_fingerprint(x509, method);
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1 in order to use the getLocalCertificateFingerprint function!");
  return "";
#endif // IPL4_USE_SSL
}

CHARSTRING IPL4asp__PT_PROVIDER::getPeerCertificateFingerprint(const ConnectionId& connId, const IPL4__DigestMethods& method){
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);

  if (!isConnIdValid(connId)) {
    TTCN_warning("Invalid connection id: %d", (int)connId);
    return "";
  }

#ifdef IPL4_USE_SSL
  if(!sockList[(int)connId].sslObj) {
    TTCN_warning("Connection id: %d is not an SSL connection", (int)connId);
    return "";
  }

  X509 *x509 = SSL_get_peer_certificate(sockList[(int)connId].sslObj);
  if(!x509) {
    TTCN_warning("Connection id: %d - peer has no certificate", (int)connId);
    return "";
  }

  return IPL4_get_certificate_fingerprint(x509, method);
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1 in order to use the getPeerCertificateFingerprint function!");
  sendError(PortError::ERROR__SOCKET, connId);
  return "";
#endif // IPL4_USE_SSL
}

CHARSTRING IPL4asp__PT_PROVIDER::getSelectedSrtpProfile(const ConnectionId& connId){
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);

  if (!isConnIdValid(connId)) {
    TTCN_warning("Invalid connection id: %d", (int)connId);
    return "";
  }

#ifdef IPL4_USE_SSL
#ifdef SRTP_AES128_CM_SHA1_80
  if(!sockList[(int)connId].sslObj) {
    TTCN_warning("Connection id: %d is not an SSL connection", (int)connId);
    return "";
  }

  SRTP_PROTECTION_PROFILE* profile = SSL_get_selected_srtp_profile(sockList[(int)connId].sslObj);

  return profile ? profile->name : "";
#else
      TTCN_error("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
#else //IPL4_USE_SSL
  log_warning("Compile the test with -DIPL4_USE_SSL & libopenssl >= 1.0.1 in order to use the getSelectedSrtpProfile function!");
  sendError(PortError::ERROR__SOCKET, connId);
  return "";
#endif // IPL4_USE_SSL
}

void f__IPL4__PROVIDER__setGetMsgLen(IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId, f__IPL4__getMsgLen& f,
    const ro__integer& msgLenArgs)
{
  portRef.testIfInitialized();
  if ((int)connId == -1) {
    portRef.defaultGetMsgLen = f;
    delete portRef.defaultMsgLenArgs;
    portRef.defaultMsgLenArgs = new Socket__API__Definitions::ro__integer(msgLenArgs);
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__setGetMsgLen: "
        "The default getMsgLen fn is modified");
  } else {
    if (!portRef.isConnIdValid(connId)) {
      IPL4_PORTREF_DEBUG(portRef, "IPL4asp__PT_PROVIDER::f__IPL4__PROVIDER__setGetMsgLen: "
          "invalid connId: %i", (int)connId);
      return;
    }
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__setGetMsgLen: "
        "getMsgLen fn for connection %d is modified", (int)connId);
    portRef.sockList[(int)connId].getMsgLen = f;
    delete portRef.sockList[(int)connId].msgLenArgs;
    portRef.sockList[(int)connId].msgLenArgs = new Socket__API__Definitions::ro__integer(msgLenArgs);
  }
} // f__IPL4__PROVIDER__setGetMsgLen


void f__IPL4__PROVIDER__setGetMsgLen__forConnClosedEvent(IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId, f__IPL4__getMsgLen& f,
    const ro__integer& msgLenArgs)
{
  portRef.testIfInitialized();
  if ((int)connId == -1) {
    portRef.defaultGetMsgLen_forConnClosedEvent = f;
    delete portRef.defaultMsgLenArgs_forConnClosedEvent;
    portRef.defaultMsgLenArgs_forConnClosedEvent = new Socket__API__Definitions::ro__integer(msgLenArgs);
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__setGetMsgLen_forConnClosedEvent: "
        "The default getMsgLen_forConnClosedEvent fn is modified");
  } else {
    if (!portRef.isConnIdValid(connId)) {
      IPL4_PORTREF_DEBUG(portRef, "IPL4asp__PT_PROVIDER::f__IPL4__PROVIDER__setGetMsgLen_forConnClosedEvent: "
          "invalid connId: %i", (int)connId);
      return;
    }
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__setGetMsgLen_forConnClosedEvent: "
        "getMsgLen_forConnClosedEvent fn for connection %d is modified", (int)connId);
    portRef.sockList[(int)connId].getMsgLen_forConnClosedEvent = f;
    delete portRef.sockList[(int)connId].msgLenArgs_forConnClosedEvent;
    portRef.sockList[(int)connId].msgLenArgs_forConnClosedEvent = new Socket__API__Definitions::ro__integer(msgLenArgs);
  }
} // f__IPL4__PROVIDER__setGetMsgLen_forConnClosedEvent


Result f__IPL4__PROVIDER__listen(IPL4asp__PT_PROVIDER& portRef, const HostName& locName,
    const PortNumber& locPort, const ProtoTuple& proto, const OptionList& options)
{
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  SockAddr sockAddr;
  socklen_t sockAddrLen;
  int hp = 0;
  SSL_TLS_Type ssl_tls_type = NONE;
  ProtoTuple my_proto = proto;

  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("entering f__IPL4__PROVIDER__listen: %s:%d / %s",
        (const char *)locName, (int)locPort,
        proto.get_selection() == ProtoTuple::ALT_udp ? "UDP" :
          proto.get_selection() == ProtoTuple::ALT_udpLight ? "UDP Light" :
            proto.get_selection() == ProtoTuple::ALT_tcp ? "TCP" :
              proto.get_selection() == ProtoTuple::ALT_sctp ? "SCTP" :
                proto.get_selection() == ProtoTuple::ALT_ssl ? "SSL" :
                  proto.get_selection() == ProtoTuple::ALT_udpLight ? "UDP Light" :
                    proto.get_selection() == ProtoTuple::ALT_unspecified ? "Unspecified" :
                      proto.get_selection() == ProtoTuple::ALT_dtls ?
                        proto.dtls().get_selection() == Socket__API__Definitions::DtlsTuple::ALT_udp ? "DTLS/UDP" :
                          proto.dtls().get_selection() == Socket__API__Definitions::DtlsTuple::ALT_sctp ? "DTLS/SCTP" : "DTLS/???" : 
                            "???");
    TTCN_Logger::end_event();
  }

  IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: enter %s %d", (const char *) locName, (int) locPort);
  portRef.testIfInitialized();

  //check if all mandatory SSL config params are present for a listening socket
#ifdef IPL4_USE_SSL

  if((proto.get_selection() == ProtoTuple::ALT_ssl) || (proto.get_selection() == ProtoTuple::ALT_dtls)) {
    ssl_tls_type = SERVER;
  }
  // set tls_used if tls is used, and copy the proto coming in proto.dtls into my_proto
  if(proto.get_selection() == ProtoTuple::ALT_dtls) {
    switch (proto.dtls().get_selection()) {
    case Socket__API__Definitions::DtlsTuple::ALT_udp:
      my_proto.udp() = proto.dtls().udp();
      break;
    case Socket__API__Definitions::DtlsTuple::ALT_sctp:
      my_proto.sctp() = proto.dtls().sctp();
      break;
    default: case ProtoTuple::UNBOUND_VALUE:
      RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
    }
  }
#endif

  if (locPort < -1 || locPort > 65535)
    RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);

  hp=SetLocalSockAddr("f__IPL4__PROVIDER__listen",portRef,AF_INET,locName, locPort, sockAddr, sockAddrLen);

  if (hp == -1) {
    SET_OS_ERROR_CODE;
    RETURN_ERROR(ERROR__HOSTNAME);
  }
#ifdef LKSCTP_MULTIHOMING_ENABLED
  int addr_index=-1;
  int num_of_addr=0;
  unsigned char* sarray=NULL;
#ifdef USE_IPL4_EIN_SCTP
  if(portRef.native_stack){
#endif
    for(int i=0; i<options.size_of();i++){
      if(options[i].get_selection()==Option::ALT_sctpAdditionalLocalAddresses){
        addr_index=i;
        num_of_addr=options[i].sctpAdditionalLocalAddresses().size_of();
        break;
      }
    }
    if(num_of_addr){
      sarray=(unsigned char*)Malloc(num_of_addr*
#ifdef USE_IPV6
          sizeof(struct sockaddr_in6)
#else
          sizeof(struct sockaddr_in)
#endif
      );
      //          SockAddr saLoc2;
      //          socklen_t saLoc2Len;
      int used_bytes=0;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family main: %s ",hp==-1?"Error":hp==AF_INET?"AF_INET":"AF_INET6");
      int final_hp=hp;
      for(int i=0; i<num_of_addr;i++){
        SockAddr saLoc2;
        socklen_t saLoc2Len;
        int hp3 = SetLocalSockAddr("f__IPL4__PROVIDER__connect",portRef,hp,options[addr_index].sctpAdditionalLocalAddresses()[i], locPort, saLoc2, saLoc2Len);
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr added Family: %s ",hp3==-1?"Error":hp3==AF_INET?"AF_INET":"AF_INET6");
        if (hp3 == -1) {
          SET_OS_ERROR_CODE;
          Free(sarray);
          RETURN_ERROR(ERROR__HOSTNAME);
        }
        if(hp3==AF_INET){
          memcpy (sarray + used_bytes, &saLoc2, sizeof (struct sockaddr_in));
          used_bytes += sizeof (struct sockaddr_in);
        }
#ifdef USE_IPV6
        else{
          final_hp=hp3;
          memcpy (sarray + used_bytes, &saLoc2, sizeof (struct sockaddr_in6));
          used_bytes += sizeof (struct sockaddr_in6);
        }
#endif

      }
      hp=final_hp;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family final: %s ",hp==-1?"Error":hp==AF_INET?"AF_INET":"AF_INET6");
    }
#ifdef USE_IPL4_EIN_SCTP
  }
#endif
#endif

  SockType sockType;
  // create socket based on the transport protocol
  int fd = -1;
  switch (my_proto.get_selection()) {
  case ProtoTuple::ALT_udp:
    sockType = IPL4asp_UDP; // go further to the next case
  case ProtoTuple::ALT_udpLight:
    fd = socket(hp, SOCK_DGRAM, 0);
    break;
#ifdef IPL4_USE_SSL
  case ProtoTuple::ALT_ssl: // go further to the next case
#endif
  case ProtoTuple::ALT_tcp:
    sockType = IPL4asp_TCP_LISTEN;
    fd = socket(hp, SOCK_STREAM, 0);
    break;
  case ProtoTuple::ALT_sctp:
    sockType = IPL4asp_SCTP_LISTEN;
#ifdef USE_IPL4_EIN_SCTP
    if(!portRef.native_stack){
      break;
    }
#endif
#ifdef USE_SCTP
    fd = socket(hp, SOCK_STREAM, IPPROTO_SCTP);
    break;
#endif
  default: case ProtoTuple::UNBOUND_VALUE:
    RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
  }
#ifdef USE_IPL4_EIN_SCTP
  if(portRef.native_stack || my_proto.get_selection() != ProtoTuple::ALT_sctp){
#endif
    if (fd == -1) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: failed to create new socket");
#ifdef LKSCTP_MULTIHOMING_ENABLED
      if(sarray) {Free(sarray);}
#endif
      SET_OS_ERROR_CODE;
      RETURN_ERROR(ERROR__SOCKET);
    }

    // set socket properties...
    if (!portRef.setOptions(options, fd, my_proto, true)) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: Setting options on "
          "socket %d failed: %s", fd, strerror(errno));
      SET_OS_ERROR_CODE;
      close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
      if(sarray) {Free(sarray);}
#endif
      RETURN_ERROR(ERROR__SOCKET);
    }
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: fcntl O_NONBLOCK on "
          "socket %d failed: %s", fd, strerror(errno));
      SET_OS_ERROR_CODE;
      close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
      if(sarray) {Free(sarray);}
#endif
      RETURN_ERROR(ERROR__SOCKET);
    }
#ifdef USE_IPL4_EIN_SCTP
  }
#endif

  // bind and listen
  int connId = -1;
  switch (my_proto.get_selection()) {

  case ProtoTuple::ALT_udp:
  case ProtoTuple::ALT_tcp:
#ifdef IPL4_USE_SSL
  case ProtoTuple::ALT_ssl:
#endif
    if (bind(fd, (struct sockaddr*)&sockAddr, sockAddrLen) == -1) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: bind on socket %d failed: %s",
          fd, strerror(errno));
      SET_OS_ERROR_CODE;
      close(fd);
      RETURN_ERROR(ERROR__SOCKET);
    }

    if((sockType == IPL4asp_TCP_LISTEN) || (sockType == IPL4asp_SCTP_LISTEN)) {
      if (listen(fd, portRef.backlog) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: "
            "listen on socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
        RETURN_ERROR(ERROR__SOCKET);
      }
    }

    connId = portRef.ConnAdd(sockType, fd, ssl_tls_type, &options);
    if (connId == -1)
      RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);


#ifdef IPL4_USE_SSL
    // if UDP/DTLS, then immediately trigger the handshake
    if((sockType == IPL4asp_UDP) && (ssl_tls_type == SERVER)) {

      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: starting TLS on connId: %d", connId);
      portRef.starttls(connId, true, result);

    }
#endif

    result.connId()() = connId;
    break;
  case ProtoTuple::ALT_sctp: {
#ifdef USE_IPL4_EIN_SCTP
    if(!portRef.native_stack){
      result=portRef.Listen_einsctp(locName, locPort,SockDesc::ACTION_NONE,"",-1,options,IPL4asp__Types::SocketList(NULL_VALUE));
      break;
    }
#endif

#ifdef USE_SCTP
    if (bind(fd, (struct sockaddr*)&sockAddr, sockAddrLen) == -1) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: bind on socket %d failed: %s",fd,
          strerror(errno));
      SET_OS_ERROR_CODE;
      close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
      if(sarray) {Free(sarray);}
#endif
      RETURN_ERROR(ERROR__SOCKET);
    }
#ifdef LKSCTP_MULTIHOMING_ENABLED
    if(num_of_addr){
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
          "sctp_bindx on socket %d num_addr: %d", fd, num_of_addr);

      if (sctp_bindx(fd, (struct sockaddr*)sarray, num_of_addr,  SCTP_BINDX_ADD_ADDR ) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
            "sctp_bindx on socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        Free(sarray);
        close(fd);
        RETURN_ERROR(ERROR__SOCKET);
      }
      Free(sarray);


    }
#endif
    if (listen(fd, portRef.backlog) == -1) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: "
          "listen on socket %d failed: %s", fd, strerror(errno));
      SET_OS_ERROR_CODE;
      close(fd);
      RETURN_ERROR(ERROR__SOCKET);
    }
    connId = portRef.ConnAdd(sockType, fd, ssl_tls_type, &options);
#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
    // Don't use this BIO. BIO will be reinitialized when starttls is used.
    BIO_new_dgram_sctp(fd, BIO_NOCLOSE);
#endif
#endif
    if (connId == -1)
      RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);
    result.connId()() = connId;
    break;
#endif
  }
  default: case ProtoTuple::UNBOUND_VALUE:
    RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
  } // switch(proto.get_selection())

  IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: leave: "
      "socket created, connection ID: %d, fd: %d", connId, fd);
  if(portRef.globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) {
    ASP__Event event;
    event.result() = result;
    portRef.incoming_message(event);
  }
  return result;
} // f__IPL4__PROVIDER__listen



Result f__IPL4__PROVIDER__connect(IPL4asp__PT_PROVIDER& portRef, const HostName& remName,
    const PortNumber& remPort, const HostName& locName,
    const PortNumber& locPort, const ConnectionId& connId,
    const ProtoTuple& proto, const OptionList& options)
{
  bool einprog = false;
  SockAddr saRem;
  socklen_t saRemLen;
  int hp = 0;
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);

  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("entering f__IPL4__PROVIDER__connect: %s:%d -> %s:%d / %s",
        (const char *)locName, (int)locPort,
        (const char *)remName, (int)remPort,
        proto.get_selection() == ProtoTuple::ALT_udp ? "UDP" :
            proto.get_selection() == ProtoTuple::ALT_udpLight ? "UDP Light" :
                proto.get_selection() == ProtoTuple::ALT_tcp ? "TCP" :
                    proto.get_selection() == ProtoTuple::ALT_sctp ? "SCTP" :
                        proto.get_selection() == ProtoTuple::ALT_ssl ? "SSL" :
                            proto.get_selection() == ProtoTuple::ALT_udpLight ? "UDP Light" :
                                proto.get_selection() == ProtoTuple::ALT_unspecified ? "Unspecified" :
                                    proto.get_selection() == ProtoTuple::ALT_dtls ?
                                        proto.dtls().get_selection() == Socket__API__Definitions::DtlsTuple::ALT_udp ? "DTLS/UDP" :
                                            proto.dtls().get_selection() == Socket__API__Definitions::DtlsTuple::ALT_sctp ? "DTLS/SCTP" : "DTLS/???" :
                                                "???");
    TTCN_Logger::end_event();
  }


  IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: enter");
  portRef.testIfInitialized();

  if (remName == "")
    RETURN_ERROR(ERROR__HOSTNAME);

  if (remPort < 0 || remPort > 65535 || locPort < -1 || locPort > 65535)
    RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);

  if ((hp = SetSockAddr(remName, remPort, saRem, saRemLen)) == -1) {
    SET_OS_ERROR_CODE;
    RETURN_ERROR(ERROR__HOSTNAME);
  }

  SSL_TLS_Type ssl_tls_type = NONE;
  ProtoTuple my_proto = proto;

  // set tls_used if tls is used, and copy the proto coming in proto.dtls into my_proto
  if(proto.get_selection() == ProtoTuple::ALT_dtls) {
    ssl_tls_type = CLIENT;
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: ssl_tls_type set to %d", ssl_tls_type);
    switch (proto.dtls().get_selection()) {
    case Socket__API__Definitions::DtlsTuple::ALT_udp:
      my_proto.udp() = proto.dtls().udp();
      break;
    case Socket__API__Definitions::DtlsTuple::ALT_sctp:
      my_proto.sctp() = proto.dtls().sctp();
      break;
    default: case ProtoTuple::UNBOUND_VALUE:
      RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
    }
  }

#ifdef LKSCTP_MULTIHOMING_ENABLED
  int num_of_addr_rem=0;
  unsigned char* sarray_rem=NULL;
#ifdef USE_IPL4_EIN_SCTP
  if(portRef.native_stack){
#endif // USE_IPL4_EIN_SCTP

    if(my_proto.ischosen(ProtoTuple::ALT_sctp) && my_proto.sctp().remSocks().ispresent()){
      num_of_addr_rem=my_proto.sctp().remSocks()().size_of();
    }
    if(num_of_addr_rem){
      sarray_rem=(unsigned char*)Malloc((num_of_addr_rem+1)*
#ifdef USE_IPV6
          sizeof(struct sockaddr_in6)
#else 
          sizeof(struct sockaddr_in)
#endif // USE_IPV6
      );
      int used_bytes=0;
      if(hp==AF_INET){
        memcpy (sarray_rem , &saRem, sizeof (struct sockaddr_in));
        used_bytes += sizeof (struct sockaddr_in);
      }
#ifdef USE_IPV6
      else{
        memcpy (sarray_rem , &saRem, sizeof (struct sockaddr_in6));
        used_bytes += sizeof (struct sockaddr_in6);
      }
#endif // USE_IPV6
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family main: %s ",hp==-1?"Error":hp==AF_INET?"AF_INET":"AF_INET6");
      int final_hp_rem=hp;
      for(int i=0; i<num_of_addr_rem;i++){
        SockAddr saLoc2;
        socklen_t saLoc2Len;
        int hp3 = SetLocalSockAddr("f__IPL4__PROVIDER__connect",portRef,hp,my_proto.sctp().remSocks()()[i].hostName(), remPort, saLoc2, saLoc2Len);
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr added Family: %s ",hp3==-1?"Error":hp3==AF_INET?"AF_INET":"AF_INET6");
        if (hp3 == -1) {
          SET_OS_ERROR_CODE;
          Free(sarray_rem);
          RETURN_ERROR(ERROR__HOSTNAME);
        }
        if(hp3==AF_INET){
          memcpy (sarray_rem + used_bytes, &saLoc2, sizeof (struct sockaddr_in));
          used_bytes += sizeof (struct sockaddr_in);
        }
#ifdef USE_IPV6
        else{
          final_hp_rem=hp3;
          memcpy (sarray_rem + used_bytes, &saLoc2, sizeof (struct sockaddr_in6));
          used_bytes += sizeof (struct sockaddr_in6);
        }
#endif // USE_IPV6

      }
      hp=final_hp_rem;
      num_of_addr_rem++;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family final: %s ",hp==-1?"Error":hp==AF_INET?"AF_INET":"AF_INET6");
    }
#ifdef USE_IPL4_EIN_SCTP
  }
#endif // USE_IPL4_EIN_SCTP
#endif // LKSCTP_MULTIHOMING_ENABLED


  int sock = -1;
  if (my_proto.get_selection() == ProtoTuple::ALT_udp && (int)connId > 0) {
    if (!portRef.isConnIdValid(connId)) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect:: invalid connId: %i", (int)connId);
      RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);
    }
    result.connId()() = connId;
    sock = portRef.sockList[(int)connId].sock;
  } else {
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
        "create new socket: %s:%d -> %s:%d",
        (const char *)locName, (int)locPort,
        (const char *)remName, (int)remPort);
    switch(my_proto.get_selection()) {

    case ProtoTuple::ALT_udp: {
      // use the original proto here; in case DTLS is used
      result = f__IPL4__PROVIDER__listen(portRef, locName, locPort, proto, options);
      if (result.errorCode().ispresent() && (result.errorCode() != PortError::ERROR__TEMPORARILY__UNAVAILABLE))
        return result;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: connId: %d", (int)result.connId()());
      sock = portRef.sockList[(int)result.connId()()].sock;
      break;
    }
#ifdef IPL4_USE_SSL
    case ProtoTuple::ALT_ssl:
#endif
    case ProtoTuple::ALT_tcp: {
      SockAddr saLoc;
      socklen_t saLocLen;
      //struct hostent *hp = NULL;
      int hp2 = SetLocalSockAddr("f__IPL4__PROVIDER__connect",portRef,hp,locName, locPort, saLoc, saLocLen);
      //      if (locPort != -1 && locName != "")
      //        hp = SetSockAddr(locName, locPort, saLoc, saLocLen);
      //      else if (locName == "" && locPort == -1) { // use default host and port
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use defaults: %s:%d",
      //          portRef.defaultLocHost, portRef.defaultLocPort);
      //        hp = SetSockAddr(HostName(portRef.defaultLocHost),
      //                portRef.defaultLocPort, saLoc, saLocLen);
      //      } else if (locPort == -1) { // use default port
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use default port: %s:%d",
      //          (const char *)locName, portRef.defaultLocPort);
      //        hp = SetSockAddr(locName, portRef.defaultLocPort, saLoc, saLocLen);
      //      } else { // use default host
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use default host: %s:%d",
      //          portRef.defaultLocHost, (int)locPort);
      //        hp = SetSockAddr(HostName(portRef.defaultLocHost),
      //                         locPort, saLoc, saLocLen);
      //      }
      if (hp2 == -1) {
        SET_OS_ERROR_CODE;
        RETURN_ERROR(ERROR__HOSTNAME);
      }

      int fd = socket(hp2, SOCK_STREAM, 0);
      if (fd == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
            "failed to create new socket");
        SET_OS_ERROR_CODE;
        RETURN_ERROR(ERROR__SOCKET);
      }

      // set socket properties
      if (!portRef.setOptions(options, fd, my_proto, true)) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: Setting options on "
            "socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
        RETURN_ERROR(ERROR__SOCKET);
      }

      if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: fcntl O_NONBLOCK on "
            "socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
        RETURN_ERROR(ERROR__SOCKET);
      }

      if (bind(fd, (struct sockaddr*)&saLoc, saLocLen) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
            "bind on socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
        RETURN_ERROR(ERROR__SOCKET);
      }

      sock = fd;
      break;
    } // ProtoTuple_alt_tcp
    case ProtoTuple::ALT_sctp: {
#ifdef USE_IPL4_EIN_SCTP
      if(!portRef.native_stack){
        result=portRef.Listen_einsctp(locName, locPort,SockDesc::ACTION_CONNECT,remName,remPort,options,
            my_proto.sctp().remSocks().ispresent()?my_proto.sctp().remSocks()():IPL4asp__Types::SocketList(NULL_VALUE));
        if(portRef.globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) {
          ASP__Event event;
          event.result() = result;
          portRef.incoming_message(event);
        }

        return result;
        break;
      }
#endif
#ifdef USE_SCTP
      SockAddr saLoc;
      socklen_t saLocLen;
      //struct hostent *hp = NULL;
      int hp2 = SetLocalSockAddr("f__IPL4__PROVIDER__connect",portRef,hp,locName, locPort, saLoc, saLocLen);

      //      if (locPort != -1 && locName != "")
      //        hp = SetSockAddr(locName, locPort, saLoc, saLocLen);
      //      else if (locName == "" && locPort == -1) { // use default host and port
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use defaults: %s:%d",
      //          portRef.defaultLocHost, portRef.defaultLocPort);
      //        hp = SetSockAddr(HostName(portRef.defaultLocHost),
      //                portRef.defaultLocPort, saLoc, saLocLen);
      //      } else if (locPort == -1) { // use default port
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use default port: %s:%d",
      //          (const char *)locName, portRef.defaultLocPort);
      //        hp = SetSockAddr(locName, portRef.defaultLocPort, saLoc, saLocLen);
      //      } else { // use default host
      //        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: use default host: %s:%d",
      //          portRef.defaultLocHost, (int)locPort);
      //        hp = SetSockAddr(HostName(portRef.defaultLocHost),
      //                         locPort, saLoc, saLocLen);
      //      }
      if (hp2 == -1) {
        SET_OS_ERROR_CODE;
#ifdef LKSCTP_MULTIHOMING_ENABLED
        if(sarray_rem) {Free(sarray_rem);}
#endif
        RETURN_ERROR(ERROR__HOSTNAME);
      }
#ifdef LKSCTP_MULTIHOMING_ENABLED
      int addr_index=-1;
      int num_of_addr=0;
      unsigned char* sarray=NULL;
      for(int i=0; i<options.size_of();i++){
        if(options[i].get_selection()==Option::ALT_sctpAdditionalLocalAddresses){
          addr_index=i;
          num_of_addr=options[i].sctpAdditionalLocalAddresses().size_of();
          break;
        }
      }
      if(num_of_addr){
        sarray=(unsigned char*)Malloc(num_of_addr*
#ifdef USE_IPV6
            sizeof(struct sockaddr_in6)
#else 
            sizeof(struct sockaddr_in)
#endif
        );
        //          SockAddr saLoc2;
        //          socklen_t saLoc2Len;
        int used_bytes=0;
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family main: %s ",hp2==-1?"Error":hp2==AF_INET?"AF_INET":"AF_INET6");
        int final_hp=hp2;
        for(int i=0; i<num_of_addr;i++){
          SockAddr saLoc2;
          socklen_t saLoc2Len;
          int hp3 = SetLocalSockAddr("f__IPL4__PROVIDER__connect",portRef,hp2,options[addr_index].sctpAdditionalLocalAddresses()[i], locPort, saLoc2, saLoc2Len);
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr added Family: %s ",hp3==-1?"Error":hp3==AF_INET?"AF_INET":"AF_INET6");
          if (hp3 == -1) {
            SET_OS_ERROR_CODE;
            Free(sarray);
            RETURN_ERROR(ERROR__HOSTNAME);
          }
          if(hp3==AF_INET){
            memcpy (sarray + used_bytes, &saLoc2, sizeof (struct sockaddr_in));
            used_bytes += sizeof (struct sockaddr_in);
          }
#ifdef USE_IPV6
          else{
            final_hp=hp3;
            memcpy (sarray + used_bytes, &saLoc2, sizeof (struct sockaddr_in6));
            used_bytes += sizeof (struct sockaddr_in6);          
          }
#endif

        }
        hp2=final_hp;
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__listen: addr family final: %s ",hp2==-1?"Error":hp2==AF_INET?"AF_INET":"AF_INET6");
      }
#endif

      int fd = socket(hp2, SOCK_STREAM, IPPROTO_SCTP);
      if (fd == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
            "failed to create new socket");
        SET_OS_ERROR_CODE;
#ifdef LKSCTP_MULTIHOMING_ENABLED
        if(sarray_rem) {Free(sarray_rem);}
        if(sarray) {Free(sarray);}
#endif
        RETURN_ERROR(ERROR__SOCKET);
      }

      // set socket properties
      if (!portRef.setOptions(options, fd, my_proto, true)) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: Setting options on "
            "socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
        if(sarray_rem) {Free(sarray_rem);}
        if(sarray) {Free(sarray);}
#endif
        RETURN_ERROR(ERROR__SOCKET);
      }

      if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: fcntl O_NONBLOCK on "
            "socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
        if(sarray_rem) {Free(sarray_rem);}
        if(sarray) {Free(sarray);}
#endif
        RETURN_ERROR(ERROR__SOCKET);
      }

      if (bind(fd, (struct sockaddr*)&saLoc, saLocLen) == -1) {
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
            "bind on socket %d failed: %s", fd, strerror(errno));
        SET_OS_ERROR_CODE;
        close(fd);
#ifdef LKSCTP_MULTIHOMING_ENABLED
        if(sarray_rem) {Free(sarray_rem);}
        if(sarray) {Free(sarray);}
#endif
        RETURN_ERROR(ERROR__SOCKET);
      }
#ifdef LKSCTP_MULTIHOMING_ENABLED
      if(num_of_addr){

        if (sctp_bindx(fd, (struct sockaddr*)sarray, num_of_addr,  SCTP_BINDX_ADD_ADDR ) == -1) {
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: "
              "sctp_bindx on socket %d failed: %s", fd, strerror(errno));
          SET_OS_ERROR_CODE;
          Free(sarray);
          if(sarray_rem) {Free(sarray_rem);}
          close(fd);
          RETURN_ERROR(ERROR__SOCKET);
        }
        Free(sarray);


      }
#endif
      sock = fd;
      break;
#endif
    }
    default: case ProtoTuple::UNBOUND_VALUE:
      RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
    } // switch(my_proto.get_selection())
  }

  switch(my_proto.get_selection()) {

  case ProtoTuple::ALT_udp:
#ifdef IPL4_USE_SSL
  case ProtoTuple::ALT_ssl:
#endif
  case ProtoTuple::ALT_tcp: {
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: sock: %d", sock);
    if (connect(sock, (struct sockaddr *)&saRem, saRemLen) == -1) {
      int l_errno = errno;
      SET_OS_ERROR_CODE;
      einprog = (l_errno == EINPROGRESS);
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: error: %s", strerror(errno));
      if (!einprog) {
        if ((my_proto.get_selection() == ProtoTuple::ALT_tcp) || (my_proto.get_selection() == ProtoTuple::ALT_ssl)) {
          close(sock);
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: socket %d is closed.", sock);
        } else {
          // The UDP socket has already been added to sockList and even if it
          // cannot be connected to some remote destination, it can still be used
          // as a listening socket or with the SendTo ASP. Therefore, it is not
          // removed from connList in case of error. But it is going to be removed,
          // if the socket has been created in this operation.
          if ((int)connId == -1) {
            if (portRef.ConnDel(result.connId()()) == -1)
              IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: unable to close socket %d (udp): %s",
                  sock, strerror(errno));
            result.connId() = OMIT_VALUE;
          }
        }
        RETURN_ERROR(ERROR__SOCKET);
      }
    }
    if (my_proto.get_selection() == ProtoTuple::ALT_tcp) {
      int l_connId = portRef.ConnAdd(IPL4asp_TCP, sock, ssl_tls_type,&options);
      if (l_connId == -1)
        RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);
      result.connId()() = l_connId;
      if((*portRef.sockList[l_connId].remoteport)==-1){
        *portRef.sockList[l_connId].remoteaddr=remName;
        *portRef.sockList[l_connId].remoteport=remPort;
      }

//      portRef.set_ssl_supp_option(l_connId,options);

#ifdef IPL4_USE_SSL
    }
    else  if (my_proto.get_selection() == ProtoTuple::ALT_ssl) {
      int l_connId = portRef.ConnAdd(IPL4asp_TCP, sock, CLIENT, &options);
      if (l_connId == -1)
        RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);
      result.connId()() = l_connId;
//      portRef.set_ssl_supp_option(l_connId,options);
      if((*portRef.sockList[l_connId].remoteport)==-1){
        *portRef.sockList[l_connId].remoteaddr=remName;
        *portRef.sockList[l_connId].remoteport=remPort;
      }
    }
    else  if (proto.get_selection() == ProtoTuple::ALT_dtls) { // if original proto was DTLS
      int l_connId = 0;
      switch(proto.dtls().get_selection()) {
      case Socket__API__Definitions::DtlsTuple::ALT_udp:
        // on UDP & UDP light, the connection is already booked when calling the listen()
        l_connId = result.connId()();
        break;
      default: case ProtoTuple::UNBOUND_VALUE:
        RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
      }
      if (l_connId == -1)
        RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);
      result.connId()() = l_connId;
      if((*portRef.sockList[l_connId].remoteport)==-1){
        *portRef.sockList[l_connId].remoteaddr=remName;
        *portRef.sockList[l_connId].remoteport=remPort;
      }
//      portRef.setDtlsSrtpProfiles(l_connId, options);
#endif
    } else {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: udp socket connected, connection ID: %d",
          (int)result.connId()());
      break;
    }

    if (einprog) {
      if (portRef.pureNonBlocking)
      {
        // The socket is not writeable yet
        portRef.Handler_Add_Fd_Write(sock);
        IPL4_PORTREF_DEBUG(portRef, "DO WRITE ON %i", sock);
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: leave (TEMPORARILY UNAVAILABLE)   fd: %i", sock);
        RETURN_ERROR(ERROR__TEMPORARILY__UNAVAILABLE);
      }
      result.os__error__code() = OMIT_VALUE;
      pollfd    pollFd;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: waiting in poll: fd %d   connId: %d",
          sock, (int)result.connId()());
      int nEvents = -1;
      for (int kk=1;;kk++) {
        memset(&pollFd, 0, sizeof(pollFd));
        pollFd.fd = sock;
        pollFd.events = POLLOUT | POLLIN;
        nEvents = poll(&pollFd, 1, portRef.poll_timeout); // infinite
        if (nEvents > 0) break;
        if (nEvents < 0 && errno != EINTR) break;
        if (portRef.max_num_of_poll>0 && portRef.max_num_of_poll<=kk) break;
      }
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: poll returned: %i", nEvents);
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: revents: 0x%04X", pollFd.revents);
      bool socketError = false;
      if (nEvents > 0) {
        if ((pollFd.revents & POLLOUT) != 0)
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: writable");
        if ((my_proto.get_selection() == ProtoTuple::ALT_tcp) || (my_proto.get_selection() == ProtoTuple::ALT_ssl)) {
          int conresult=0;
          socklen_t conresult_len = sizeof(conresult);
          int sendRes = getsockopt(sock, SOL_SOCKET, SO_ERROR, &conresult, &conresult_len);
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: Probing connection: getsockopt returned: %d, connection result code: %d", sendRes,conresult);
          if (sendRes < 0) {
            SET_OS_ERROR_CODE;
            IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: getsockopt error: %s", strerror(errno));
            socketError = true;
          } else if (conresult!=0) {
            result.os__error__code()() = conresult;
            IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: connect error: %s", strerror(conresult));
            socketError = true;
          }


        }
      } else {
        SET_OS_ERROR_CODE;
        socketError = true;
      }
      if (socketError) {
        if (my_proto.get_selection() != ProtoTuple::ALT_udp || (int)connId == -1) {
          if (portRef.ConnDel(result.connId()()) == -1)
            IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: unable to close socket");
          result.connId() = OMIT_VALUE;
        }
        RETURN_ERROR(ERROR__SOCKET);
      } // socketError
    } // einprog
#ifdef IPL4_USE_SSL
    //Add SSL layer
    if(my_proto.get_selection() == ProtoTuple::ALT_ssl)
    {
      switch(portRef.perform_ssl_handshake((int)result.connId()()))
      {
      case SUCCESS:
        break;
      case WANT_WRITE:
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: leave (TEMPORARILY UNAVAILABLE)   fd: %i", sock);
        RETURN_ERROR(ERROR__TEMPORARILY__UNAVAILABLE);
        break;
      case FAIL:
      default: //value
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: SSL mapping failed for client socket: %d", portRef.sockList[(int)result.connId()()].sock);
        SET_OS_ERROR_CODE;
        if (portRef.ConnDel(result.connId()()) == -1)IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: unable to close socket");
        result.connId() = OMIT_VALUE;
        RETURN_ERROR(ERROR__SOCKET);
        break;
      }
    } else if(ssl_tls_type != NONE) {
      // if the original proto was DTLS + UDP or DTLS + UDP Light, then start the TLS layer
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: starting TLS on connId: %d", (int)result.connId()());
      portRef.starttls((int)result.connId()(), false, result);
    }
#endif
    break;
  }
  case ProtoTuple::ALT_sctp: {
#ifdef USE_SCTP
    IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: sock: %d", sock);

#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
    // Initialize client BIO here
    // This will be initialized even in case of simple SCTP. When DTLS is utilized the BIO needs to be create before the connect.
    // This means that if a simple SCTP connect is called first and then later on the f_IPL4_StartTLS is used, 
    // the BIO should be initialized here, otherwise segmentation fault is triggered in OpenSSL.
    BIO *bio_ptr = BIO_new_dgram_sctp(sock, BIO_NOCLOSE);
#endif
#endif

#ifdef LKSCTP_MULTIHOMING_ENABLED
    if(sarray_rem) {
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connectx: sock: %d, num_of_addr %d", sock, num_of_addr_rem);
      if (my_sctp_connectx(sock, (struct sockaddr *)sarray_rem, num_of_addr_rem) == -1) {
        int l_errno = errno;
        SET_OS_ERROR_CODE;
        einprog = (l_errno == EINPROGRESS);
        if (!einprog) {
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: error: %s", strerror(errno));
          close(sock);
          Free(sarray_rem);
          RETURN_ERROR(ERROR__SOCKET);
        }
      }
      Free(sarray_rem);

    } else
#endif
      if (connect(sock, (struct sockaddr *)&saRem, saRemLen) == -1) {
        int l_errno = errno;
        SET_OS_ERROR_CODE;
        einprog = (l_errno == EINPROGRESS);
        if (!einprog) {
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: error: %s", strerror(errno));
          close(sock);
          RETURN_ERROR(ERROR__SOCKET);
        }
      }

    int l_connId = portRef.ConnAdd(IPL4asp_SCTP, sock, ssl_tls_type,&options);
    if (l_connId == -1) {
      RETURN_ERROR(ERROR__INSUFFICIENT__MEMORY);
    }
    result.connId()() = l_connId;
    if((*portRef.sockList[l_connId].remoteport)==-1){
      *portRef.sockList[l_connId].remoteaddr=remName;
      *portRef.sockList[l_connId].remoteport=remPort;
    }

#ifdef IPL4_USE_SSL
#ifdef OPENSSL_SCTP_SUPPORT
    // Init SSL object for client and copy BIO in sockList
    if((portRef.sockList[l_connId].type == IPL4asp_SCTP) && (portRef.sockList[l_connId].ssl_tls_type != NONE)) {
      portRef.sockList[l_connId].ssl_tls_type = CLIENT;
      portRef.sockList[l_connId].server = false;
      portRef.sockList[l_connId].sslState = STATE_CONNECTING;
      portRef.sockList[l_connId].sslObj = NULL;
      if(!portRef.ssl_create_contexts_and_obj(l_connId)) {
        IPL4_PORTREF_DEBUG(portRef, "IPL4asp__PT_PROVIDER::connect: SSL initialization failed. Client: %d", (int)l_connId);
        portRef.setResult(result,PortError::ERROR__SOCKET,l_connId,0);
      }
      else {
        IPL4_PORTREF_DEBUG(portRef, "IPL4asp__PT_PROVIDER::connect: set client side BIO, connId: %d", (int)l_connId);
        portRef.sockList[l_connId].bio = bio_ptr;
        SSL_set_bio(portRef.sockList[l_connId].sslObj, portRef.sockList[l_connId].bio, portRef.sockList[l_connId].bio);
      }
    }
#endif
#endif
    if (einprog) {
      if (portRef.pureNonBlocking)
      {
        // The socket is not writeable yet
        portRef.Handler_Add_Fd_Write(sock);
        IPL4_PORTREF_DEBUG(portRef, "DO WRITE ON %i", sock);
        IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: leave (TEMPORARILY UNAVAILABLE)   fd: %i", sock);
        RETURN_ERROR(ERROR__TEMPORARILY__UNAVAILABLE);
      }
      result.os__error__code() = OMIT_VALUE;
      pollfd    pollFd;
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: waiting in poll: fd %d   connId: %d",
          sock, (int)result.connId()());
      int nEvents = -1;
      for (int kk=1;;kk++) {
        memset(&pollFd, 0, sizeof(pollFd));
        pollFd.fd = sock;
        pollFd.events = POLLOUT | POLLIN;
        nEvents = poll(&pollFd, 1, portRef.poll_timeout); // infinite
        if (nEvents > 0) break;
        if (nEvents < 0 && errno != EINTR) break;
        if (portRef.max_num_of_poll>0 && portRef.max_num_of_poll<=kk) break;
      }
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: poll returned: %i", nEvents);
      IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: revents: 0x%04X", pollFd.revents);
      bool socketError = false;
      if (nEvents > 0) {
        if ((pollFd.revents & POLLOUT) != 0)
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: EAGAIN: writable");
        if (connect(sock, (struct sockaddr *)&saRem, saRemLen) == -1) {
          if(errno == EISCONN){
            IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: Probing connection, result: sucessfull");
          } else {
            IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: Probing connection, result: unsucessfull");
            SET_OS_ERROR_CODE;
            socketError = true;
          }
        }
      } else {
        SET_OS_ERROR_CODE;
        socketError = true;
      }
      if (socketError) {
        if (portRef.ConnDel(result.connId()()) == -1)
          IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: unable to close socket");
        result.connId() = OMIT_VALUE;
        RETURN_ERROR(ERROR__SOCKET);
      } // socketError
    } // einprog
#endif
    break;
  }
  default: case ProtoTuple::UNBOUND_VALUE:
    RETURN_ERROR(ERROR__UNSUPPORTED__PROTOCOL);
  } // switch(my_proto.get_selection())
  IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__connect: leave");

  if(portRef.globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) {
    ASP__Event event;
    event.result() = result;
    portRef.incoming_message(event);
  }

  return result;
} // f__IPL4__PROVIDER__connect



Result f__IPL4__PROVIDER__setOpt(IPL4asp__PT_PROVIDER& portRef, const OptionList& options,
    const ConnectionId& connId, const ProtoTuple& proto)
{
  portRef.testIfInitialized();
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);
  ProtoTuple protocol = proto;
  int sock = -1;
  if ((int)connId != -1) {
    if (!portRef.isConnIdValid(connId)) {
      RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);
    }
    sock = portRef.sockList[(int)connId].sock;
    SockType type = (SockType)-1;
    if (!portRef.getAndCheckSockType(connId, proto.get_selection(), type))
      RETURN_ERROR(ERROR__SOCKET);
    switch (type) {
    case IPL4asp_TCP_LISTEN:
    case IPL4asp_TCP:
      if(portRef.sockList[(int)connId].ssl_tls_type == NONE) {
        protocol.tcp() = TcpTuple(null_type());
      } else {
        protocol.ssl() = SslTuple(null_type());
      }
      break;
    case IPL4asp_UDP:
      protocol.udp() = UdpTuple(null_type()); break; //TCP<->UDP SWITCHED VALUES FIXED -- ETHNBA
    case IPL4asp_SCTP_LISTEN:
    case IPL4asp_SCTP:
      protocol.sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE); break;
    default: break;
    }
  }
  if (!portRef.setOptions(options, sock, protocol))
    RETURN_ERROR(ERROR__SOCKET);

//  if (!portRef.setDtlsSrtpProfiles(connId, options))
//      RETURN_ERROR(ERROR__SOCKET);

  if ((int)connId != -1) {
    portRef.set_ssl_supp_option((int)connId,options);
  }
  return result;
}



Result f__IPL4__PROVIDER__close(IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId, const ProtoTuple& proto)
{
  IPL4_PORTREF_DEBUG(portRef, "f__IPL4__PROVIDER__close: enter: connId: %d", (int)connId);
  portRef.testIfInitialized();
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);
  if (!portRef.isConnIdValid(connId)) {
    RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);
  }
  SockType type;
  if (!portRef.getAndCheckSockType(connId, proto.get_selection(), type)) {
    RETURN_ERROR(ERROR__INVALID__INPUT__PARAMETER);
  }

  if (type == IPL4asp_SCTP_LISTEN || type == IPL4asp_SCTP) {
    // Close SCTP associations if any, but the socket is not closed.
#ifdef USE_IPL4_EIN_SCTP
    if(!portRef.native_stack){
      portRef.sockList[connId].next_action=SockDesc::ACTION_DELETE;
      if(type == IPL4asp_SCTP){
        EINSS7_00SctpShutdownReq(portRef.sockList[connId].sock);
      } else {
        if(portRef.sockList[connId].ref_count==0){
          EINSS7_00SctpDestroyReq(portRef.sockList[connId].endpoint_id);
          portRef.ConnDelEin(connId);
        }
      }

    } else {
#endif

      if (portRef.ConnDel(connId) == -1)
        RETURN_ERROR(ERROR__SOCKET);
#ifdef USE_IPL4_EIN_SCTP

    }
#endif
  } else {
    if (portRef.ConnDel(connId) == -1)
      RETURN_ERROR(ERROR__SOCKET);
  }
  result.connId()() = connId;
  if(portRef.globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) {
    ASP__Event event;
    event.result() = result;
    portRef.incoming_message(event);
  }
  return result;
} // f__IPL4__PROVIDER__close



Result f__IPL4__PROVIDER__setUserData(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& id,
    const UserData& userData)
{
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);
  if (portRef.setUserData((int)id, (int)userData) == -1) {
    RETURN_ERROR(ERROR__GENERAL);
  }
  return result;
} // f__IPL4__PROVIDER__setUserData



Result f__IPL4__PROVIDER__getUserData(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId,
    UserData& userData)
{
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);
  int userDataTemp;
  if (portRef.getUserData((int)connId, userDataTemp) == -1) {
    RETURN_ERROR(ERROR__GENERAL);
  }
  userData = userDataTemp;
  return result;
} // f__IPL4__PROVIDER__getUserData



Result f__IPL4__PROVIDER__getConnectionDetails(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId,
    const IPL4__Param& IPL4param,
    IPL4__ParamResult& IPL4paramResult)
{
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE,OMIT_VALUE);
  IPL4__ParamResult paramResult;
  if (portRef.getConnectionDetails((int)connId, IPL4param, paramResult) == -1) {
    RETURN_ERROR(ERROR__GENERAL);
  }
  IPL4paramResult = paramResult;
  return result;
} // f__IPL4__PROVIDER__getConnectionDetails


Result f__IPL4__PROVIDER__StartTLS(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId,
    const BOOLEAN& server__side)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f_IPL4_StartTLS: connId: %d, %s side", portRef.get_name(),(int)connId,server__side?"server":"client");
    TTCN_Logger::end_event();
  }
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  portRef.starttls(connId,server__side,result);
  return result;
}

Result f__IPL4__PROVIDER__StopTLS(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f_IPL4_StopTLS: connId: %d", portRef.get_name(),(int)connId);
    TTCN_Logger::end_event();
  }
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  portRef.stoptls(connId,result);
  return result;
}

OCTETSTRING f__IPL4__PROVIDER__exportTlsKey(
    IPL4asp__PT_PROVIDER& portRef,
    const IPL4asp__Types::ConnectionId& connId,
    const CHARSTRING& label,
    const OCTETSTRING& context,
    const INTEGER& keyLen)
{
  return portRef.exportTlsKey(connId, label, context, keyLen);
}

IPL4__SrtpKeysAndSalts f__IPL4__PROVIDER__exportSrtpKeysAndSalts(
    IPL4asp__PT_PROVIDER& portRef,
    const IPL4asp__Types::ConnectionId& connId)
{
  return portRef.exportSrtpKeysAndSalts(connId);
}

OCTETSTRING f__IPL4__PROVIDER__exportSctpKey(
    IPL4asp__PT_PROVIDER& portRef,
    const IPL4asp__Types::ConnectionId& connId)
{
  return portRef.exportSctpKey(connId);
}

CHARSTRING f__IPL4__PROVIDER__getLocalCertificateFingerprint(
    IPL4asp__PT_PROVIDER& portRef,
    const IPL4__DigestMethods& method,
    const ConnectionId& connId,
    const CHARSTRING& certificate__file)
{
  return portRef.getLocalCertificateFingerprint(method,connId,certificate__file);
}

CHARSTRING f__IPL4__PROVIDER__getPeerCertificateFingerprint(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId,
    const IPL4__DigestMethods& method)
{
  return portRef.getPeerCertificateFingerprint(connId, method);
}

CHARSTRING f__IPL4__PROVIDER__getSelectedSrtpProfile(
    IPL4asp__PT_PROVIDER& portRef,
    const ConnectionId& connId)
{
  return portRef.getSelectedSrtpProfile(connId);
}

Result f__IPL4__listen(
    IPL4asp__PT& portRef,
    const HostName& locName,
    const PortNumber& locPort,
    const ProtoTuple& proto,
    const OptionList& options)
{
  return f__IPL4__PROVIDER__listen(portRef, locName, locPort, proto, options);
} // f__IPL4__listen



Result f__IPL4__connect(
    IPL4asp__PT& portRef,
    const HostName& remName,
    const PortNumber& remPort,
    const HostName& locName,
    const PortNumber& locPort,
    const ConnectionId& connId,
    const ProtoTuple& proto,
    const OptionList& options)
{
  return f__IPL4__PROVIDER__connect(portRef, remName, remPort,
      locName, locPort, connId, proto, options);
} // f__IPL4__connect



Result f__IPL4__setOpt(
    IPL4asp__PT& portRef,
    const OptionList& options,
    const ConnectionId& connId,
    const ProtoTuple& proto)
{
  return f__IPL4__PROVIDER__setOpt(portRef, options, connId, proto);
} // f__IPL4__setOpt



Result f__IPL4__close(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const ProtoTuple& proto)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f__IPL4__close: ", portRef.get_name());
    TTCN_Logger::log_event(" proto ");
    proto.log();
    TTCN_Logger::log_event(" connId ");
    connId.log();
    TTCN_Logger::end_event();
  }
  return f__IPL4__PROVIDER__close(portRef, connId, proto);
} // f__IPL4__close



Result f__IPL4__setUserData(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const UserData& userData)
{
  return f__IPL4__PROVIDER__setUserData(portRef, connId, userData);
} // f__IPL4__setUserData



Result f__IPL4__getUserData(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    UserData& userData)
{
  return f__IPL4__PROVIDER__getUserData(portRef, connId, userData);
} // f__IPL4__getUserData



Result f__IPL4__getConnectionDetails(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const IPL4__Param& IPL4param,
    IPL4__ParamResult& IPL4paramResult)
{
  return f__IPL4__PROVIDER__getConnectionDetails(portRef, connId, IPL4param, IPL4paramResult);
} // f__IPL4__getConnectionDetails


void f__IPL4__setGetMsgLen(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    f__IPL4__getMsgLen& f,
    const ro__integer& msgLenArgs)
{
  f__IPL4__PROVIDER__setGetMsgLen(portRef, connId, f, msgLenArgs);
} // f__IPL4__setGetMsgLen

void f__IPL4__setGetMsgLen__forConnClosedEvent(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    f__IPL4__getMsgLen& f,
    const ro__integer& msgLenArgs)
{
  f__IPL4__PROVIDER__setGetMsgLen__forConnClosedEvent(portRef, connId, f, msgLenArgs);
} // f__IPL4__setGetMsgLen_forConnClosedEvent

Result f__IPL4__send(
    IPL4asp__PT& portRef,
    const ASP__Send& asp,
    INTEGER& sent__octets)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f_IPL4_send: ", portRef.get_name());
    asp.log();
    TTCN_Logger::end_event();
  }
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  sent__octets=portRef.outgoing_send_core(asp,result);
  return result;
}

Result f__IPL4__sendto(
    IPL4asp__PT& portRef,
    const ASP__SendTo& asp,
    INTEGER& sent__octets)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f_IPL4_sendto: ", portRef.get_name());
    asp.log();
    TTCN_Logger::end_event();
  }
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  sent__octets=portRef.outgoing_send_core(asp,result);
  return result;
}

Result f__IPL4__StartTLS(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const BOOLEAN& server__side)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f__IPL4__StartTLS: ", portRef.get_name());
    connId.log();
    TTCN_Logger::log_event(" server_side: ");
    server__side.log();
    TTCN_Logger::end_event();
  }
  return f__IPL4__PROVIDER__StartTLS(portRef,connId,server__side);
}

Result f__IPL4__StopTLS(
    IPL4asp__PT& portRef,
    const ConnectionId& connId)
{
  if(TTCN_Logger::log_this_event(TTCN_PORTEVENT)){
    TTCN_Logger::begin_event(TTCN_PORTEVENT);
    TTCN_Logger::log_event("%s: f__IPL4__StopTLS: ", portRef.get_name());
    connId.log();
    TTCN_Logger::end_event();
  }
  return f__IPL4__PROVIDER__StopTLS(portRef,connId);
}

OCTETSTRING f__IPL4__exportTlsKey(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const CHARSTRING& label,
    const OCTETSTRING& context,
    const INTEGER& keyLen)
{
  return portRef.exportTlsKey(connId, label, context, keyLen);
}

IPL4__SrtpKeysAndSalts f__IPL4__exportSrtpKeysAndSalts(
    IPL4asp__PT& portRef,
    const ConnectionId& connId)
{
  return portRef.exportSrtpKeysAndSalts(connId);
}

OCTETSTRING f__IPL4__exportSctpKey(
    IPL4asp__PT& portRef,
    const IPL4asp__Types::ConnectionId& connId)
{
  return portRef.exportSctpKey(connId);
}

CHARSTRING f__IPL4__getLocalCertificateFingerprint(
    IPL4asp__PT& portRef,
    const IPL4__DigestMethods& method,
    const ConnectionId& connId,
    const CHARSTRING& certificate__file)
{
  return portRef.getLocalCertificateFingerprint(method,connId,certificate__file);
}

CHARSTRING f__IPL4__getPeerCertificateFingerprint(
    IPL4asp__PT& portRef,
    const ConnectionId& connId,
    const IPL4__DigestMethods& method)
{
  return portRef.getPeerCertificateFingerprint(connId, method);
}

CHARSTRING f__IPL4__getSelectedSrtpProfile(
    IPL4asp__PT& portRef,
    const ConnectionId& connId)
{
  return portRef.getSelectedSrtpProfile(connId);
}

INTEGER f__IPL4__fixedMsgLen(const OCTETSTRING& stream, ro__integer& msgLenArgs){

  int length_offset=(int)msgLenArgs[0];
  int nr_bytes_in_length=(int)msgLenArgs[1];

  int stream_length=stream.lengthof();

  if(stream_length<(length_offset+nr_bytes_in_length)){ 
    return -1;  // not enough bytes
  }

  int length_multiplier=(int)msgLenArgs[3];
  int value_offset=(int)msgLenArgs[2];

  int shift_diff;
  int shift_count;

  if(((int)msgLenArgs[4])==1){
    shift_count=0;  // Little endian
    shift_diff=1;
  } else {
    shift_count=nr_bytes_in_length - 1;  // Big endian
    shift_diff=-1;
  }

  unsigned long m_length = 0;

  const unsigned char* buff=(const unsigned char* )stream + length_offset;
  for (int i = 0; i < nr_bytes_in_length; i++) {
    m_length |= buff[i] << (8 * shift_count);
    shift_count+=shift_diff;
  }
  m_length *= length_multiplier;
  if (value_offset < 0 && (long)m_length < -value_offset) return stream_length;
  else return m_length + value_offset;

}

//************
//SSL
//************
#ifdef IPL4_USE_SSL
// ssl_session ID context of the server
static unsigned char ssl_server_context_name[] = "McHalls&EduardWasHere";
const unsigned char * IPL4asp__PT_PROVIDER::ssl_server_auth_session_id_context = ssl_server_context_name;
// Password pointer
//void  *IPL4asp__PT_PROVIDER::ssl_current_client = NULL;  // Not used currently. If the function ssl_verify_certificates_at_handshake will do more than return -1
// it should be used again.

// Data set/get functions
char       * IPL4asp__PT_PROVIDER::get_ssl_password() const {return ssl_password;}
/*void         IPL4asp__PT_PROVIDER::set_ssl_verifycertificate(bool par) {ssl_verify_certificate=par;}
void         IPL4asp__PT_PROVIDER::set_ssl_use_session_resumption(bool par) {ssl_use_session_resumption=par;}
void         IPL4asp__PT_PROVIDER::set_ssl_key_file(char * par) {
  delete [] ssl_key_file;
  ssl_key_file=par;
}
void         IPL4asp__PT_PROVIDER::set_ssl_certificate_file(char * par) {
  delete [] ssl_certificate_file;
  ssl_certificate_file=par;
}
void         IPL4asp__PT_PROVIDER::set_ssl_trustedCAlist_file(char * par) {
  delete [] ssl_trustedCAlist_file;
  ssl_trustedCAlist_file=par;
}
void         IPL4asp__PT_PROVIDER::set_ssl_cipher_list(char * par) {
  delete [] ssl_cipher_list;
  ssl_cipher_list=par;
}
void         IPL4asp__PT_PROVIDER::set_ssl_server_auth_session_id_context(const unsigned char * par) {
  ssl_server_auth_session_id_context=par;
}
*/
// Default parameter names
//const char* IPL4asp__PT_PROVIDER::ssl_use_ssl_name()                { return "ssl_use_ssl";}
const char* IPL4asp__PT_PROVIDER::ssl_use_session_resumption_name() { return "ssl_use_session_resumption";}
const char* IPL4asp__PT_PROVIDER::ssl_private_key_file_name()       { return "ssl_private_key_file";}
const char* IPL4asp__PT_PROVIDER::ssl_trustedCAlist_file_name()     { return "ssl_trustedCAlist_file";}
const char* IPL4asp__PT_PROVIDER::ssl_certificate_file_name()       { return "ssl_certificate_chain_file";}
const char* IPL4asp__PT_PROVIDER::ssl_password_name()               { return "ssl_private_key_password";}
//const char* IPL4asp__PT_PROVIDER::ssl_dtls_srtp_profiles_name()     { return "ssl_dtls_srtp_profiles";}
const char* IPL4asp__PT_PROVIDER::ssl_cipher_list_name()            { return "ssl_allowed_ciphers_list";}
const char* IPL4asp__PT_PROVIDER::ssl_verifycertificate_name()      { return "ssl_verify_certificate";}

SSL_CTX    * IPL4asp__PT_PROVIDER::get_selected_ssl_ctx(int client_id) const{
  if((sockList[client_id].type == IPL4asp_TCP) || (sockList[client_id].type == IPL4asp_TCP_LISTEN)) {
    return ssl_ctx;
  } else if(sockList[client_id].ssl_tls_type == CLIENT) {
    return ssl_dtls_client_ctx;
  } else {
    return ssl_dtls_server_ctx;
  }
  return NULL;
}

bool IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj(int client_id) {
  if(!ssl_init_SSL(client_id))
  {
    log_warning("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: SSL initialization failed during client: %d connection", client_id);
    return false;
  }

  SSL_CTX *selected_ctx=NULL;

  if(ssl_cert_per_conn){  // SSL cert, key etc can be set per connection
    if(sockList[client_id].ssl_certificate_file || sockList[client_id].ssl_key_file || sockList[client_id].ssl_trustedCAlist_file){  // We need a separate SSL_CTX if use separate cert file. There is no API to load cert chain for SSL obj.
      if((sockList[client_id].type == IPL4asp_TCP) || (sockList[client_id].type == IPL4asp_TCP_LISTEN)) {
        selected_ctx=SSL_CTX_new (SSLv23_method());
      } else if(sockList[client_id].ssl_tls_type == CLIENT) {
        selected_ctx=SSL_CTX_new (DTLS_client_method());
      } else {
        selected_ctx=SSL_CTX_new (DTLS_server_method());
      }
      sockList[client_id].sslCTX=selected_ctx;
      ssl_init_SSL_ctx(selected_ctx, client_id);
    } else {
      selected_ctx=get_selected_ssl_ctx(client_id);  // we can use the global one
    }
  } else {
    selected_ctx=get_selected_ssl_ctx(client_id);
  }

  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: Create a new SSL object for client %d", client_id);
  if (!selected_ctx)
  {
    log_warning("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: No SSL CTX found, SSL not initialized!");
    return false;
  }

  ssl_current_ssl=SSL_new(selected_ctx);

  if (ssl_current_ssl==NULL)
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: Creation of SSL object failed for client: %d", client_id);
    return false;
  }

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
  if(sockList[client_id].tls_hostname){
    IPL4_DEBUG("Setting TLS hostname");
    SSL_set_tlsext_host_name(ssl_current_ssl,(const char*)(*sockList[client_id].tls_hostname)) ; 
  }
#endif

#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
  if(sockList[client_id].alpn){
    IPL4_DEBUG("Setting ALPN");
    int ret=SSL_set_alpn_protos(ssl_current_ssl,(const unsigned char*)(*sockList[client_id].alpn),sockList[client_id].alpn->lengthof());
    if(ret!=0){
      ssl_getresult(ret);
        log_warning("Setting of ALPN failed.");
        return false;
    }
  }
#endif

  if(ssl_cert_per_conn && !sockList[client_id].sslCTX){
    if(sockList[client_id].ssl_cipher_list){
      IPL4_DEBUG("Setting ssl_cipher list restrictions");
      if (SSL_set_cipher_list(ssl_current_ssl, sockList[client_id].ssl_cipher_list)!=1)
      {
        log_warning("Cipher list restriction failed for %s", sockList[client_id].ssl_cipher_list);
        return false;
      }
    }
  }

  if(!setSslObj(client_id, ssl_current_ssl))
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: setSslObj failed for client: %d", client_id);
    return false;
  }
#ifdef SSL_OP_NO_SSLv2
  if(sockList[client_id].ssl_supp.SSLv2 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_SSLv2);
  }
#endif
#ifdef SSL_OP_NO_SSLv3
  if(sockList[client_id].ssl_supp.SSLv3 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_SSLv3);
  }
#endif
#ifdef SSL_OP_NO_TLSv1
  if(sockList[client_id].ssl_supp.TLSv1 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1);
  }
#endif
#ifdef SSL_OP_NO_TLSv1_1
  if(sockList[client_id].ssl_supp.TLSv1_1 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1_1);
  }
#endif
#ifdef SSL_OP_NO_TLSv1_2
  if(sockList[client_id].ssl_supp.TLSv1_2 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_TLSv1_2);
  }
#endif
#ifdef SSL_OP_NO_DTLSv1
  if(sockList[client_id].ssl_supp.DTLSv1 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_DTLSv1);
  }
#endif
#ifdef SSL_OP_NO_DTLSv1_2
  if(sockList[client_id].ssl_supp.DTLSv1_2 == GlobalConnOpts::NO){
    SSL_set_options(ssl_current_ssl,SSL_OP_NO_DTLSv1_2);
  }
#endif



  char* dtlsSrtpProfiles = sockList[client_id].dtlsSrtpProfiles;
  if(!dtlsSrtpProfiles) { dtlsSrtpProfiles = globalConnOpts.dtlsSrtpProfiles; }
  if(dtlsSrtpProfiles) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: applying SRTP profiles %s in connId %d...", dtlsSrtpProfiles, client_id);
#ifdef SRTP_AES128_CM_SHA1_80
    int ret = SSL_set_tlsext_use_srtp(ssl_current_ssl, dtlsSrtpProfiles);
    if(ret) {
      log_warning("IPL4asp__PT_PROVIDER::ssl_create_contexts_and_obj: could not set SRTP profiles!");
      ssl_getresult(ret);
      return false;
    } else {
      IPL4_DEBUG("profiles applied.");
    }
#else
      TTCN_error("DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
  }

  return true;
}

SSL_HANDSHAKE_RESULT IPL4asp__PT_PROVIDER::perform_ssl_handshake(int client_id) {

  IPL4_DEBUG("entering IPL4asp__PT_PROVIDER::perform_ssl_handshake() .");

  //in case of purenonblocking & client reconnect
  if(pureNonBlocking && getSslObj(client_id, ssl_current_ssl) && ssl_current_ssl)
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Reconnection for client: %d ", client_id);
  }
  else
  {
    if(!ssl_create_contexts_and_obj(client_id)) {
      // Warning should be used instead of debug log, It is a serious case
      log_warning("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL initialization failed on connection: %d", client_id);
      return FAIL;
    }
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: New client added with key '%d'", client_id);
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Binding SSL to the socket");
    if (SSL_set_fd(ssl_current_ssl, sockList[client_id].sock)!=1)
    {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Binding of SSL object to socket failed. Connection: %d", client_id);
      return FAIL;
    }
  }
  // Context change for SSL objects may come here in the
  // future.

  //server accepting
  if (sockList[client_id].ssl_tls_type == SERVER)
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Accept SSL connection request");
//    if (ssl_current_client!=NULL) log_warning("Warning: IPL4asp__PT_PROVIDER::perform_ssl_handshake: race condition while setting current client object pointer");
//    ssl_current_client=(IPL4asp__PT_PROVIDER *)this;

    int attempt = 0;
    while(true)
    {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL attempt to Accept Client Connection... %d", attempt + 1);
      current_conn_id=client_id;
      int res=ssl_getresult(SSL_accept(ssl_current_ssl));
      current_conn_id=-1;
      switch(res)
      {
      case SSL_ERROR_NONE:
        break;
      case SSL_ERROR_WANT_READ:
        if(pureNonBlocking)
        {
          sockList[client_id].sslState = STATE_HANDSHAKING;
          Handler_Add_Fd_Read(sockList[client_id].sock);
          Handler_Remove_Fd_Write(sockList[client_id].sock);
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Writable: willing to continue handshake by receiving data...");
//          ssl_current_client=NULL;
          // SSL says: SSL_WANT_READ -> do not listen to Writable events
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return WANT_READ;
        }
      case SSL_ERROR_WANT_WRITE:
        if(pureNonBlocking)
        {
          sockList[client_id].sslState = STATE_HANDSHAKING;
          Handler_Add_Fd_Write(sockList[client_id].sock);
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: willing to continue handshake by sending data...");
          IPL4_DEBUG("DO WRITE ON %i", sockList[client_id].sock);
//          ssl_current_client=NULL;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL_ERROR_WANT_WRITE");
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return WANT_WRITE;
        }

        if(++attempt == ssl_reconnect_attempts)
        {
//          ssl_current_client=NULL;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection accept failed");
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return FAIL;
        }
        //usleep(ssl_reconnect_delay);
        timespec tm_val;
        tm_val.tv_sec=ssl_reconnect_delay/1000000;
        tm_val.tv_nsec=(ssl_reconnect_delay%1000000)*1000;
        nanosleep(&tm_val,NULL);
        continue;
      case SSL_ERROR_SYSCALL:
        log_warning("Warning: IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL_ERROR_SYSCALL peer is disconnected");
      default:
        log_warning("Warning: IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection from client %d is refused", client_id);
//        ssl_current_client=NULL;
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
        return FAIL;
      }
      break;
    }

    sockList[client_id].sslState = STATE_NORMAL;
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection from client %d is accepted", client_id);
//    ssl_current_client=NULL;

  } else {//client connecting
    if (ssl_use_session_resumption && ssl_session!=NULL) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Try to use ssl_session resumption");
      if (ssl_getresult(SSL_set_session(ssl_current_ssl, ssl_session))!=SSL_ERROR_NONE)
      {
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL error occurred during set session. Client: %d", client_id);
        return FAIL;
      }
    }

    IPL4_DEBUG( "IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connecting...");
//    if (ssl_current_client!=NULL) log_warning("IPL4asp__PT_PROVIDER::perform_ssl_handshake: race condition while setting current client object pointer");
//    ssl_current_client=(IPL4asp__PT_PROVIDER *)this;

    int attempt = 0;
    while(true)
    {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL attempt to connect ... %d", attempt + 1);
      
      current_conn_id=client_id;
      int res=ssl_getresult(SSL_connect(ssl_current_ssl));
      current_conn_id=-1;
      
      switch(res)
      {
      case SSL_ERROR_NONE:
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection Success.");
        sockList[client_id].sslState = STATE_NORMAL;
        break;
      case SSL_ERROR_WANT_READ:
        if(pureNonBlocking)
        {
//          ssl_current_client=NULL;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL_ERROR_WANT_READ");
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return WANT_READ;
        }
      case SSL_ERROR_WANT_WRITE:
        if(pureNonBlocking)
        {
//          ssl_current_client=NULL;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: SSL_ERROR_WANT_WRITE");
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return WANT_WRITE;
        }

        if(++attempt == ssl_reconnect_attempts)
        {
//          ssl_current_client=NULL;
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection failed");
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
          return FAIL;
        }
        //usleep(ssl_reconnect_delay);
        timespec tm_val;
        tm_val.tv_sec=ssl_reconnect_delay/1000000;
        tm_val.tv_nsec=(ssl_reconnect_delay%1000000)*1000;
        nanosleep(&tm_val,NULL);
        continue;
      default:
//        ssl_current_client=NULL;
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connection failed");
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
        return FAIL;
      }
      break;
    }

//    ssl_current_client=NULL;
    if (ssl_use_session_resumption) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Connected, get new ssl_session");
      ssl_session=SSL_get1_session(ssl_current_ssl);
      if (ssl_session==NULL)
        log_warning("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Server did not send a session ID");
    }
  }

  if (ssl_use_session_resumption) {
    if (SSL_session_reused(ssl_current_ssl)) {IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Session was reused");}
    else { IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Session was not reused");}
  }

  if (!ssl_verify_certificates()) { // remove client
    log_warning("IPL4asp__PT_PROVIDER::perform_ssl_handshake: Verification failed");
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::perform_ssl_handshake: leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake()");
    return FAIL;

  }
  IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::perform_ssl_handshake() with SUCCESS");
  return SUCCESS;
}


bool IPL4asp__PT_PROVIDER::perform_ssl_shutdown(int client_id) {

  IPL4_DEBUG("entering IPL4asp__PT_PROVIDER::perform_ssl_shutdown()");

  bool ret = true;
  if (getSslObj(client_id, ssl_current_ssl) && ssl_current_ssl!=NULL) {
    if(sockList[client_id].sslState != STATE_CONNECTING) {
      IPL4_DEBUG("performing SSL_shutdown on connection: #%d", client_id);
      SSL_shutdown(ssl_current_ssl);
    }
    SSL_free(ssl_current_ssl);
    sockList[client_id].sslObj=NULL;
    sockList[client_id].bio = NULL;
    sockList[client_id].ssl_tls_type = NONE;
    sockList[client_id].sslState = STATE_CONNECTING;
    if(sockList[client_id].sslCTX!=NULL){
      SSL_CTX_free(sockList[client_id].sslCTX);
      sockList[client_id].sslCTX=NULL;
    }
  } else {
    log_warning("SSL object not found for client %d", client_id);
    ret = false;
  }
  IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::perform_ssl_shutdown()");
  return ret;
}


//Currently not used (separated to listen/connect directly
bool IPL4asp__PT_PROVIDER::user_all_mandatory_configparameters_present(int clientId) {
  if ((sockList[clientId].type == IPL4asp_TCP_LISTEN) && (sockList[clientId].ssl_tls_type == SERVER)) {
    if (ssl_certificate_file==NULL)
    {
      IPL4_DEBUG("%s is not defined in the configuration file", ssl_certificate_file_name());
      return false;
    }
    if (ssl_trustedCAlist_file==NULL)
    {
      IPL4_DEBUG("%s is not defined in the configuration file", ssl_trustedCAlist_file_name());
      return false;
    }
    if (ssl_key_file==NULL)
    {
      IPL4_DEBUG("%s is not defined in the configuration file", ssl_private_key_file_name());
      return false;
    }
  } else if((sockList[clientId].type == IPL4asp_TCP) && (sockList[clientId].ssl_tls_type == CLIENT)) {
    if (ssl_verify_certificate && ssl_trustedCAlist_file==NULL)
    {
      IPL4_DEBUG("%s is not defined in the configuration file altough %s=yes", ssl_trustedCAlist_file_name(), ssl_verifycertificate_name());
      return false;
    }
  }
  return true;
}



//STATE_WAIT_FOR_RECEIVE_CALLBACK: if the SSL_read operation would
//                                  block because the socket is not ready for writing,
//                                  I set the socket state to this state and add the file
//                                  descriptor to the Event_Handler. The Event_Handler will
//                                  wake up and call the receive_message_on_fd operation
//                                  if the socket is ready to write.
//If the SSL_read operation would block because the socket is not ready for
//reading, I do nothing
int IPL4asp__PT_PROVIDER::receive_ssl_message_on_fd(int client_id, int* error_msg)
{
  IPL4_DEBUG("entering IPL4asp__PT_PROVIDER::receive_message_on_fd(%d)", client_id);

//  if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
//  ssl_current_client=(IPL4asp__PT_PROVIDER *)this;

  TTCN_Buffer* recv_tb = *sockList[client_id].buf;

  if(sockList[client_id].sslState != STATE_NORMAL)
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: leave - Client is Connecting: %d", client_id);
//    ssl_current_client=NULL;
    return -2;
  }

  if(!getSslObj(client_id, ssl_current_ssl))
  {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: current SSL invalid for client: %d", client_id);
//    ssl_current_client=NULL;
    return 0;
  }

  int messageLength=0;
  size_t end_len=AS_SSL_CHUNCK_SIZE;
  unsigned char *end_ptr;
  int total_read=0;
  while (messageLength>=0) {
    end_len=AS_SSL_CHUNCK_SIZE;
    IPL4_DEBUG("  one read cycle started");
    recv_tb->get_end(end_ptr, end_len);
    IPL4_DEBUG("  try to read %d bytes",(int)end_len);
    messageLength = SSL_read(ssl_current_ssl, end_ptr, end_len);
    IPL4_DEBUG("  SSL_read returned %d",messageLength);
    if (messageLength <= 0) {
      *error_msg=ssl_getresult(messageLength);
      switch (*error_msg) {
      case SSL_ERROR_ZERO_RETURN:
        IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: SSL connection was interrupted by the other side");
        SSL_set_quiet_shutdown(ssl_current_ssl, 1);
        IPL4_DEBUG("SSL_ERROR_ZERO_RETURN is received, setting SSL SHUTDOWN mode to QUIET");
//        ssl_current_client=NULL;
        IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::receive_message_on_fd() with SSL_ERROR_ZERO_RETURN");
        return total_read;
      case SSL_ERROR_WANT_WRITE://writing would block
        if(!total_read){
         total_read=-2; 
        }
        if (pureNonBlocking){
          Handler_Add_Fd_Write(sockList[client_id].sock);
          IPL4_DEBUG("DO WRITE ON %i", sockList[client_id].sock);
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: setting socket state to STATE_WAIT_FOR_RECEIVE_CALLBACK");
          sockList[client_id].sslState = STATE_WAIT_FOR_RECEIVE_CALLBACK;

//          ssl_current_client=NULL;
          IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::receive_message_on_fd()");
        }
        return total_read;
      case SSL_ERROR_WANT_READ: //reading would block, continue processing data
        if(!total_read){
         total_read=-2; 
        }
        if (pureNonBlocking){
          IPL4_DEBUG("IPL4asp__PT_PROVIDER::receive_message_on_fd: reading would block, leaving IPL4asp__PT_PROVIDER::receive_message_on_fd()");
//          ssl_current_client = NULL;
          IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::receive_message_on_fd()");
        }
        return total_read;
        break;
      default:  // On error the connection should be closed, the caller function will close the connection if we return 0
        IPL4_DEBUG( "SSL error occured. Closing the connection.");
        SSL_set_quiet_shutdown(ssl_current_ssl, 1);
        IPL4_DEBUG( "SSL_ERROR is received, setting SSL SHUTDOWN mode to QUIET");
//        ssl_current_client=NULL;
        IPL4_DEBUG( "leaving IPL4asp__PT_PROVIDER::receive_message_on_fd() with SSL_ERROR_ZERO_RETURN");
        *error_msg = SSL_ERROR_ZERO_RETURN;
        return total_read;
      }
    } else {
      recv_tb->increase_length(messageLength);
      total_read+=messageLength;
    }
    if(sockList[client_id].type == IPL4asp_UDP) {
        // For UDP, we only read one packet at a time (and hope that the buffer was large enough)
        IPL4_DEBUG( "Returning early for UDP/DTLS");
        return total_read;
    }
  }
//  ssl_current_client=NULL;
  IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::receive_message_on_fd() with number of bytes read: %d",total_read );

  return total_read;
}

bool IPL4asp__PT_PROVIDER::increase_send_buffer(int fd,
    int &old_size, int& new_size)
{
  int set_size;
#if defined LINUX || defined FREEBSD || defined SOLARIS8
  socklen_t
#else /* SOLARIS or WIN32 */
  int
#endif
  optlen = sizeof(old_size);
  // obtaining the current buffer size first
  if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&old_size, &optlen))
    goto getsockopt_failure;
  if (old_size <= 0) {
    log_warning("System call getsockopt(SO_SNDBUF) "
        "returned invalid buffer size (%d) on file descriptor %d.",
        old_size, fd);
    return false;
  }
  // trying to double the buffer size
  set_size = 2 * old_size;
  if (set_size > old_size) {
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&set_size,
        sizeof(set_size))) {
      // the operation failed
      switch (errno) {
      case ENOMEM:
      case ENOBUFS:
        errno = 0;
        break;
      default:
        // other error codes indicate a fatal error
        goto setsockopt_failure;
      }
    } else {
      // the operation was successful
      goto success;
    }
  }
  // trying to perform a binary search to determine the maximum buffer size
  set_size = old_size;
  for (int size_step = old_size / 2; size_step > 0; size_step /= 2) {
    int tried_size = set_size + size_step;
    if (tried_size > set_size) {
      if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&tried_size,
          sizeof(tried_size))) {
        // the operation failed
        switch (errno) {
        case ENOMEM:
        case ENOBUFS:
          errno = 0;
          break;
        default:
          // other error codes indicate a fatal error
          goto setsockopt_failure;
        }
      } else {
        // the operation was successful
        set_size = tried_size;
      }
    }
  }
  if (set_size <= old_size) return false;
  success:
  // querying the new effective buffer size (it might be smaller
  // than set_size but should not be smaller than old_size)
  optlen = sizeof(new_size);
  if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&new_size,
      &optlen)) goto getsockopt_failure;
  if (new_size > old_size) return true;
  else {
    if (new_size < old_size)
      log_warning("System call getsockopt(SO_SNDBUF) returned unexpected buffer size "
          "(%d, after increasing it from %d to %d) on file descriptor %d.",
          new_size, old_size, set_size, fd);
    return false;
  }
  getsockopt_failure:
  log_warning("System call getsockopt(SO_SNDBUF) failed on file "
      "descriptor %d. (%s)", fd, strerror(errno));
  return false;
  setsockopt_failure:
  log_warning("System call setsockopt(SO_SNDBUF) failed on file "
      "descriptor %d. (%s)", fd, strerror(errno));
  return false;
}

bool IPL4asp__PT_PROVIDER::ssl_verify_certificates()
{
  char str[SSL_CHARBUF_LENGTH];

  IPL4_DEBUG("entering IPL4asp__PT_PROVIDER::ssl_verify_certificates()");

  ssl_log_SSL_info();

  // Get the other side's certificate
  IPL4_DEBUG("Check certificate of the other party");
  X509 *cert = SSL_get_peer_certificate (ssl_current_ssl);
  if (cert != NULL) {

    {
      IPL4_DEBUG("Certificate information:");
      X509_NAME_oneline (X509_get_subject_name (cert), str, SSL_CHARBUF_LENGTH);
      IPL4_DEBUG("  subject: %s", str);
    }

    // We could do all sorts of certificate verification stuff here before
    // deallocating the certificate.

    // Just a basic check that the certificate is valid
    // Other checks (e.g. Name in certificate vs. hostname) shall be
    // done on application level
    if (ssl_verify_certificate)
      IPL4_DEBUG("Verification state is: %s", X509_verify_cert_error_string(SSL_get_verify_result(ssl_current_ssl)));
    X509_free (cert);

  } else
    log_warning("Other side does not have certificate.");

  IPL4_DEBUG("leaving IPL4asp__PT_PROVIDER::ssl_verify_certificates()");
  return true;
}

bool IPL4asp__PT_PROVIDER::ssl_actions_to_seed_PRNG() {
  struct stat randstat;

  if(RAND_status()) {
    IPL4_DEBUG("PRNG already initialized, no action needed");
    return true;
  }
  IPL4_DEBUG("Seeding PRND");
  // OpenSSL tries to use random devives automatically
  // these would not be necessary
  if (!stat("/dev/urandom", &randstat)) {
    IPL4_DEBUG("Using installed random device /dev/urandom for seeding the PRNG with %d bytes.", SSL_PRNG_LENGTH);
    if (RAND_load_file("/dev/urandom", SSL_PRNG_LENGTH)!=SSL_PRNG_LENGTH)
    {
      IPL4_DEBUG("Could not read from /dev/urandom");
      return false;
    }
  } else if (!stat("/dev/random", &randstat)) {
    IPL4_DEBUG("Using installed random device /dev/random for seeding the PRNG with %d bytes.", SSL_PRNG_LENGTH);
    if (RAND_load_file("/dev/random", SSL_PRNG_LENGTH)!=SSL_PRNG_LENGTH)
    {
      IPL4_DEBUG("Could not read from /dev/random");
      return false;
    }
  } else {
    /* Neither /dev/random nor /dev/urandom are present, so add
       entropy to the SSL PRNG a hard way. */
    log_warning("Solaris patches to provide random generation devices are not installed.\nSee http://www.openssl.org/support/faq.html \"Why do I get a \"PRNG not seeded\" error message?\"\nA workaround will be used.");
    for (int i = 0; i < 10000  &&  !RAND_status(); ++i) {
      char buf[4];
      struct timeval tv;
      gettimeofday(&tv, 0);
      buf[0] = tv.tv_usec & 0xF;
      buf[2] = (tv.tv_usec & 0xF0) >> 4;
      buf[3] = (tv.tv_usec & 0xF00) >> 8;
      buf[1] = (tv.tv_usec & 0xF000) >> 12;
      RAND_add(buf, sizeof buf, 0.1);
    }
    return true;
  }

  if(!RAND_status()) {
    IPL4_DEBUG("Could not seed the Pseudo Random Number Generator with enough data.");
    return false;
  } else {
    IPL4_DEBUG("PRNG successfully initialized.");
  }

  return true;
}

// returns 1 on success, DTE on fail
int ssl_generate_cookie_callback(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {

#ifdef SRTP_AES128_CM_SHA1_80
  unsigned char *buffer, result[EVP_MAX_MD_SIZE];
  unsigned int length = 0, resultlength;

  /* Initialize a random secret */
  if (!ssl_cookie_initialized) {
    if (!RAND_bytes(ssl_cookie_secret, IPL4_COOKIE_SECRET_LENGTH)) {
      TTCN_error("generate_cookie: Could not generate secret cookie!");
    }
    ssl_cookie_initialized = 1;
  }

  SockAddr peer;
  /* Read peer information */
  (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

  /* Create buffer with peer's address and port */
  length = 0;
  switch (peer.ss.ss_family) {
  case AF_INET:
    length += sizeof(struct in_addr);
    break;
  case AF_INET6:
    length += sizeof(struct in6_addr);
    break;
  default:
    TTCN_error("generate_cookie: INET version not handled; please call Eduard Czimbalmos back from the pension!");
    break;
  }
  length += sizeof(in_port_t);

  buffer = (unsigned char*) OPENSSL_malloc(length);
  if (buffer == NULL) {
    TTCN_error("generate_cookie: out of memory!");
  }

  switch (peer.ss.ss_family) {
  case AF_INET:
    memcpy(buffer,
        &peer.v4.sin_port,
        sizeof(in_port_t));
    memcpy(buffer + sizeof(peer.v4.sin_port),
        &peer.v4.sin_addr,
        sizeof(struct in_addr));
    break;
  case AF_INET6:
    memcpy(buffer,
        &peer.v6.sin6_port,
        sizeof(in_port_t));
    memcpy(buffer + sizeof(in_port_t),
        &peer.v6.sin6_addr,
        sizeof(struct in6_addr));
    break;
  default:
    TTCN_error("generate_cookie: INET version not handled; please call Eduard Czimbalmos back from the pension!");
    break;
  }

  /* Calculate HMAC of buffer using the secret */
  HMAC(EVP_sha1(), (const void*) ssl_cookie_secret, IPL4_COOKIE_SECRET_LENGTH,
      (const unsigned char*) buffer, length, result, &resultlength);
  OPENSSL_free(buffer);

  memcpy(cookie, result, resultlength);
  *cookie_len = resultlength;

  //fprintf(stderr, "generate_cookie returning successfully\n");
  return 1;
#else
  TTCN_error("DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
  return 0;
#endif
}

// returns 1 on success, 0 on fail
int ssl_verify_cookie_callback(SSL *ssl, 
#if OPENSSL_VERSION_NUMBER >= 0x10100000L 
const 
#endif
    unsigned char *cookie,
    unsigned int cookie_len) {

#ifdef SRTP_AES128_CM_SHA1_80
  unsigned char *buffer, result[EVP_MAX_MD_SIZE];
  unsigned int length = 0, resultlength;

  /* If secret isn't initialized yet, the cookie can't be valid */
  if (!ssl_cookie_initialized) return 0;

  SockAddr peer;
  /* Read peer information */
  (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

  /* Create buffer with peer's address and port */
  length = 0;
  switch (peer.ss.ss_family) {
  case AF_INET:
    length += sizeof(struct in_addr);
    break;
  case AF_INET6:
    length += sizeof(struct in6_addr);
    break;
  default:
    TTCN_error("verify_cookie: INET version not handled; please call Eduard Czimbalmos back from the pension!");
    break;
  }
  length += sizeof(in_port_t);
  buffer = (unsigned char*) OPENSSL_malloc(length);

  if (buffer == NULL)
  {
    TTCN_error("verify_cookie: out of memory!");
  }

  switch (peer.ss.ss_family) {
  case AF_INET:
    memcpy(buffer,
        &peer.v4.sin_port,
        sizeof(in_port_t));
    memcpy(buffer + sizeof(in_port_t),
        &peer.v4.sin_addr,
        sizeof(struct in_addr));
    break;
  case AF_INET6:
    memcpy(buffer,
        &peer.v6.sin6_port,
        sizeof(in_port_t));
    memcpy(buffer + sizeof(in_port_t),
        &peer.v6.sin6_addr,
        sizeof(struct in6_addr));
    break;
  default:
    TTCN_error("verify_cookie: INET version not handled; please call Eduard Czimbalmos back from the pension!");
    break;
  }

  /* Calculate HMAC of buffer using the secret */
  HMAC(EVP_sha1(), (const void*) ssl_cookie_secret, IPL4_COOKIE_SECRET_LENGTH,
      (const unsigned char*) buffer, length, result, &resultlength);
  OPENSSL_free(buffer);

  if (cookie_len == resultlength && memcmp(result, cookie, resultlength) == 0) {
    return 1;
  }
#else
      TTCN_error("DTLS is not supported by the current OpenSSL API. libopenssl >=1.0.1 is required!");
#endif
  return 0;
}

bool IPL4asp__PT_PROVIDER::ssl_init_SSL_ctx(SSL_CTX* in_ssl_ctx, int conn_id) {
  if (in_ssl_ctx==NULL)
  {
    log_warning("SSL context creation failed.");
    return false;
  }

   IPL4_DEBUG("IPL4asp__PT_PROVIDER::ssl_init_SSL_ctx: Init CTX connId: %d", conn_id);
  // valid for all SSL objects created from this context afterwards
    char* cf=NULL;
    if(ssl_cert_per_conn && isConnIdValid(conn_id) && sockList[conn_id].ssl_certificate_file){
      cf=sockList[conn_id].ssl_certificate_file;
    } else {
      cf=ssl_certificate_file;
    }

    // valid for all SSL objects created from this context afterwards
    if(cf!=NULL) {
      IPL4_DEBUG("Loading certificate file");
      if(SSL_CTX_use_certificate_chain_file(in_ssl_ctx, cf)!=1)
      {
        log_warning("Can't read certificate file %s", cf);
        return false;
      }
    }
    if(ssl_cert_per_conn && isConnIdValid(conn_id) && sockList[conn_id].ssl_password){
      cf=sockList[conn_id].ssl_password;
    } else {
      cf=ssl_password;
    }
    if(cf!=NULL){
        SSL_CTX_set_default_passwd_cb(in_ssl_ctx, ssl_password_cb);
        SSL_CTX_set_default_passwd_cb_userdata(in_ssl_ctx, cf);
        }

    if(ssl_cert_per_conn && isConnIdValid(conn_id) && sockList[conn_id].ssl_key_file){
      cf=sockList[conn_id].ssl_key_file;
    } else {
      cf=ssl_key_file;
    }
    if(cf!=NULL) {
      IPL4_DEBUG("Loading key file");
//      if (ssl_current_client!=NULL) log_warning("Warning: race condition while setting current client object pointer");
//      ssl_current_client=(IPL4asp__PT_PROVIDER *)this;
      if(SSL_CTX_use_PrivateKey_file(in_ssl_ctx, cf, SSL_FILETYPE_PEM)!=1)
      {
        log_warning("Can't read key file %s", cf);
        return false;
      }

//      ssl_current_client=NULL;
    }

    if(ssl_cert_per_conn && isConnIdValid(conn_id) && sockList[conn_id].ssl_trustedCAlist_file){
      cf=sockList[conn_id].ssl_trustedCAlist_file;
    } else {
      cf=ssl_trustedCAlist_file;
    }
    if (cf!=NULL) {
      IPL4_DEBUG("Loading trusted CA list file");
      if (SSL_CTX_load_verify_locations(in_ssl_ctx, cf, NULL)!=1)
      {
        IPL4_DEBUG("Can't read trustedCAlist file %s", cf);
        return false;
      }
    }

    if (ssl_certificate_file!=NULL && ssl_key_file!=NULL) {
      IPL4_DEBUG("Check for consistency between private and public keys");
      if (SSL_CTX_check_private_key(in_ssl_ctx)!=1)
        log_warning("Private key does not match the certificate public key");
    }

    if(ssl_cert_per_conn && isConnIdValid(conn_id) && sockList[conn_id].ssl_cipher_list){
      cf=sockList[conn_id].ssl_cipher_list;
    } else {
      cf=ssl_cipher_list;
    }
    if (cf!=NULL) {
      IPL4_DEBUG("Setting ssl_cipher list restrictions");
      if (SSL_CTX_set_cipher_list(in_ssl_ctx, cf)!=1)
      {
        log_warning("Cipher list restriction failed for %s", cf);
        return false;
      }
    }
//  }
  // check the other side's certificates
  if (ssl_verify_certificate) {
    IPL4_DEBUG("Setting verification behaviour: verification required and do not allow to continue on failure");
    SSL_CTX_set_verify(in_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, ssl_verify_callback);
  } else {
    IPL4_DEBUG("Setting verification behaviour: verification not required and do allow to continue on failure");
    SSL_CTX_set_verify(in_ssl_ctx, SSL_VERIFY_NONE, ssl_verify_callback);
  }
  //Session id context can be set at any time but will be useful only for server(s)

  IPL4_DEBUG("Activate ssl_session resumption");
  IPL4_DEBUG("Context is: %s; length = %lu", ssl_server_auth_session_id_context, (unsigned long)strlen((const char*)ssl_server_auth_session_id_context));
  if (SSL_CTX_set_session_id_context(in_ssl_ctx, ssl_server_auth_session_id_context, strlen((const char*)ssl_server_auth_session_id_context))!=1)
  {
    log_warning("Activation of SSL ssl_session resumption failed");
    return false;
  }

  SSL_CTX_set_read_ahead(in_ssl_ctx, 1);

  //SSL_CTX_set_verify(in_ssl_ctx, SSL_VERIFY_NONE, ssl_verify_callback);
  if (conn_id != -1)
  { // with SCTP the cookies should not be used
    if (!((sockList[conn_id].type == IPL4asp_SCTP_LISTEN) || (sockList[conn_id].type == IPL4asp_SCTP)))
    {
      IPL4_DEBUG("cookie generation %d for connId %d", sockList[conn_id].type,conn_id);
      SSL_CTX_set_cookie_generate_cb(in_ssl_ctx, ssl_generate_cookie_callback);
      SSL_CTX_set_cookie_verify_cb(in_ssl_ctx, ssl_verify_cookie_callback);
    }
  } 
  else
  {
    SSL_CTX_set_cookie_generate_cb(in_ssl_ctx, ssl_generate_cookie_callback);
    SSL_CTX_set_cookie_verify_cb(in_ssl_ctx, ssl_verify_cookie_callback);
  }

#if OPENSSL_VERSION_NUMBER >= 0x1000200fL
  SSL_CTX_set_alpn_select_cb(in_ssl_ctx,ipl4_tls_alpn_cb,this);
  
#endif
  return true;
}


bool IPL4asp__PT_PROVIDER::ssl_init_SSL(int connId)
{
  if (ssl_initialized) {
    IPL4_DEBUG("SSL already initialized, no action needed");
    return true;
  }

  if(!ssl_actions_to_seed_PRNG())
  {
    IPL4_DEBUG("Can't seed PRNG.");
    return false;
  }

  IPL4_DEBUG("Init SSL started");
  IPL4_DEBUG("Using %s (%lx)", SSLeay_version(SSLEAY_VERSION), OPENSSL_VERSION_NUMBER);


  SSL_library_init();          // initialize library
  SSL_load_error_strings();    // readable error messages

  IPL4_DEBUG("Creating SSL/TLS context...");
  // Create context with SSLv23_method() method: both server and client understanding SSLv2, SSLv3, TLSv1
  ssl_ctx = SSL_CTX_new (SSLv23_method());
  if(!ssl_init_SSL_ctx(ssl_ctx, connId)) { return false; }

  IPL4_DEBUG("Creating DTLSv1 context...");
  // Create contexts with DTLSv1_server_method() and DTLSv1_client_method() method
  ssl_dtls_server_ctx = SSL_CTX_new (DTLS_server_method());
  if(!ssl_init_SSL_ctx(ssl_dtls_server_ctx, connId)) { return false; }
  ssl_dtls_client_ctx = SSL_CTX_new (DTLS_client_method());
  if(!ssl_init_SSL_ctx(ssl_dtls_client_ctx, connId)) { return false; }

  ssl_initialized=true;
  IPL4_DEBUG("Init SSL successfully finished");
  return true;
}


void IPL4asp__PT_PROVIDER::ssl_log_SSL_info()
{
  char str[SSL_CHARBUF_LENGTH];

  IPL4_DEBUG("Check SSL description");
  //  ssl_cipher=SSL_get_current_cipher(ssl_current_ssl);
  //  if (ssl_cipher!=NULL) {
  SSL_CIPHER_description(SSL_get_current_cipher(ssl_current_ssl), str, SSL_CHARBUF_LENGTH);
  {
    IPL4_DEBUG("SSL description:");
    IPL4_DEBUG("%s", str);
  }
  //  }
}



// Log the SSL error and flush the error queue
// Can be used after the followings:
// SSL_connect(), SSL_accept(), SSL_do_handshake(),
// SSL_read(), SSL_peek(), or SSL_write()
int IPL4asp__PT_PROVIDER::ssl_getresult(int res)
{
  IPL4_DEBUG("SSL operation result:");
  int err = SSL_get_error(ssl_current_ssl, res);

  switch(err) {
  case SSL_ERROR_NONE:
    IPL4_DEBUG("SSL_ERROR_NONE");
    break;
  case SSL_ERROR_ZERO_RETURN:
    IPL4_DEBUG("SSL_ERROR_ZERO_RETURN");
    break;
  case SSL_ERROR_WANT_READ:
    IPL4_DEBUG("SSL_ERROR_WANT_READ");
    break;
  case SSL_ERROR_WANT_WRITE:
    IPL4_DEBUG("SSL_ERROR_WANT_WRITE");
    break;
  case SSL_ERROR_WANT_CONNECT:
    IPL4_DEBUG("SSL_ERROR_WANT_CONNECT");
    break;
  case SSL_ERROR_WANT_ACCEPT:
    IPL4_DEBUG("SSL_ERROR_WANT_ACCEPT");
    break;
  case SSL_ERROR_WANT_X509_LOOKUP:
    IPL4_DEBUG("SSL_ERROR_WANT_X509_LOOKUP");
    break;
  case SSL_ERROR_SYSCALL:
    IPL4_DEBUG("SSL_ERROR_SYSCALL");
    IPL4_DEBUG("EOF was observed that violates the protocol, peer disconnected; treated as a normal disconnect");
    err = SSL_ERROR_ZERO_RETURN;
    break;
  case SSL_ERROR_SSL:
    IPL4_DEBUG("SSL_ERROR_SSL");
    break;
  default:
    IPL4_DEBUG("Unknown SSL error code: %d", err);
    //--ethnba -may need to return to skip reading the error string
  }
  // get the copy of the error string in readable format
  unsigned long e=ERR_get_error();
  if(!e) {
    IPL4_DEBUG("There is no SSL error at the moment.\n");
  }
  while (e) {
    IPL4_DEBUG("SSL error queue content:");
    IPL4_DEBUG("  Library:  %s", ERR_lib_error_string(e));
    IPL4_DEBUG("  Function: %s", ERR_func_error_string(e));
    IPL4_DEBUG("  Reason:   %s", ERR_reason_error_string(e));
    e=ERR_get_error();
  }
  //It does the same but more simple:
  // ERR_print_errors_fp(stderr);
  return err;
}

/*  Not used, Just a placeholder. 
int   IPL4asp__PT_PROVIDER::ssl_verify_certificates_at_handshake(int preverify_ok, X509_STORE_CTX *ssl_ctx) {
  // don't care by default
  return -1;
}
*/
// Callback function used by OpenSSL.
// Called when a password is needed to decrypt the private key file.
// NOTE: not thread safe
int IPL4asp__PT_PROVIDER::ssl_password_cb(char *buf, int num, int /*rwflag*/,void *userdata) {

    const char* pass = (const char*) userdata;
    if(userdata==NULL) return 0;
    int pass_len = strlen(pass) + 1;
    if (num < pass_len) return 0;

    strcpy(buf, pass);
    return(strlen(pass));
}

// Callback function used by OpenSSL.
// Called during SSL handshake with a pre-verification status.
int IPL4asp__PT_PROVIDER::ssl_verify_callback(int preverify_ok, X509_STORE_CTX *ssl_ctx)
{
  SSL     *ssl_pointer;
  SSL_CTX *ctx_pointer;
//  int user_result;

  ssl_pointer = (SSL *)X509_STORE_CTX_get_ex_data(ssl_ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
  ctx_pointer = SSL_get_SSL_CTX(ssl_pointer);

/* 
// the ssl_verify_certificates_at_handshake always return -1. No sense to call it. Later if more check is implemented
// the code can be reactivated.
  if (ssl_current_client!=NULL) {
    user_result=((IPL4asp__PT_PROVIDER *)ssl_current_client)->ssl_verify_certificates_at_handshake(preverify_ok, ssl_ctx);
    if (user_result>=0) return user_result;
  } else { // go on with default authentication
    fprintf(stderr, "Warning: no current SSL object found but ssl_verify_callback is called, programming error\n");
  }
*/
  // if ssl_verifiycertificate == "no", then always accept connections
  if (SSL_CTX_get_verify_mode(ctx_pointer) && SSL_VERIFY_NONE)
    return 1;
  // if ssl_verifiycertificate == "yes", then accept connections only if the
  // certificate is valid
  else if (SSL_CTX_get_verify_mode(ctx_pointer) && SSL_VERIFY_PEER)
    return preverify_ok;
  // something went wrong
  else
    return 0;
}

bool IPL4asp__PT_PROVIDER::setSslObj(int connId, SSL* sslObj)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::setSslObj enter: connId %d", connId);
  testIfInitialized();
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::setSslObj: invalid connId: %i", connId);
    return false;
  }
  sockList[connId].sslObj = sslObj;
  return true;
} // IPL4asp__PT_PROVIDER::setSslObj

bool IPL4asp__PT_PROVIDER::getSslObj(int connId, SSL*& sslObj)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::getSslObj enter: connId %d", connId);
  testIfInitialized();
  if (!isConnIdValid(connId)) {
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::getSslObj: invalid connId: %i", connId);
    return false;
  }
  sslObj = sockList[connId].sslObj;
  return true;
} // IPL4asp__PT_PROVIDER::getSslObj

#endif
////////////////////////////////////////////////////////////////////////
/////    Default SSL log functions
////////////////////////////////////////////////////////////////////////
void IPL4asp__PT_PROVIDER::log_debug(const char *fmt, ...) const
{
  if (debugAllowed) {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event_str("IPL4asp__PT_PROVIDER - SSL socket: ");
    va_list args;
    va_start(args, fmt);
    TTCN_Logger::log_event_va_list(fmt, args);
    va_end(args);
    TTCN_Logger::end_event();
  }
}

void IPL4asp__PT_PROVIDER::log_warning(const char *fmt, ...) const
{
  TTCN_Logger::begin_event(TTCN_WARNING);
  TTCN_Logger::log_event_str("IPL4asp__PT_PROVIDER - SSL socket Warning: ");
  va_list args;
  va_start(args, fmt);
  TTCN_Logger::log_event_va_list(fmt, args);
  va_end(args);
  TTCN_Logger::end_event();
}


void IPL4asp__PT_PROVIDER::log_hex(const char *prompt, const unsigned char *msg,
    size_t length) const
{
  if (debugAllowed) {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event_str("IPL4asp__PT_PROVIDER - SSL socket: ");
    if (prompt != NULL) TTCN_Logger::log_event_str(prompt);
    TTCN_Logger::log_event("Size: %lu, Msg:", (unsigned long)length);
    for (size_t i = 0; i < length; i++) TTCN_Logger::log_event(" %02x", msg[i]);
    TTCN_Logger::end_event();
  }
}

#ifdef USE_IPL4_EIN_SCTP

IPL4asp__PT_PROVIDER *IPL4asp__PT_PROVIDER::port_ptr=NULL;

void IPL4asp__PT_PROVIDER::do_bind()
{
  unsigned char bindRequestCounter = 0;
  exiting = FALSE;
  int otherId = SCTP_ID;

  IPL4_DEBUG("entering IPL4asp__PT_PROVIDER::do_bind() userId: %d, otherId: %d, userInstanceId: %d, sctpInstanceId: %d, cpManagerIPA %s", userId, otherId, userInstanceId, sctpInstanceId,(const char*)cpManagerIPA);

  SS7Common::setDebug(debugAllowed);

  SS7Common::connect_ein(userId, otherId, (const TEXT_T*)cpManagerIPA, userInstanceId, sctpInstanceId);

  if (userId == GENERATE_BY_CP_ID) {
    userId = SS7Common::getUsedMPOwner();  
    userInstanceId = SS7Common::getUsedMPInstance();
  }

#ifdef EIN_R3B

  EINSS7_00SCTPFUNCNEW_T newFunc;
  memset(&newFunc,0,sizeof(newFunc));
  newFunc.EINSS7_00SctpSetEpAliasConf= EINSS7_00SCTPSETEPALIASCONF;
  newFunc.EINSS7_00SctpSetUserCongestionLevelConf= EINSS7_00SCTPSETUSERCONGESTIONLEVELCONF;
  newFunc.EINSS7_00SctpGetUserCongestionLevelConf= EINSS7_00SCTPGETUSERCONGESTIONLEVELCONF;
  newFunc.EINSS7_00SctpRedirectInd= EINSS7_00SCTPREDIRECTIND;
  newFunc.EINSS7_00SctpInfoConf= EINSS7_00SCTPINFOCONF;
  newFunc.EINSS7_00SctpInfoInd= EINSS7_00SCTPINFOIND;
  newFunc.EINSS7_00SctpUpdateConf= EINSS7_00SCTPUPDATECONF;
  EINSS7_00SCTPRegFuncNew(&newFunc);

#endif
  USHORT_T ret_val;
  MSG_T msg;
  msg.receiver = userId;


  do
  {
    bindRequestCounter++;
    /* Bind to stack*/
    // hardcoded value: ANSI 1996
    IPL4_DEBUG("Sending:\n"
        "EINSS7_00SctpBindReq(\n"
        "userId=%d\n"
        "sctpInstanceId=%d\n"
        "api_type=%d"
        "bind_type=%d)\n",
        userId,
        sctpInstanceId,
        EINSS7_00SCTP_APITYPE_API,
        EINSS7_00SCTP_BIND_IF_NOT_BOUND);
    ret_val = EINSS7_00SctpBindReq( userId,
        sctpInstanceId,
        EINSS7_00SCTP_APITYPE_API,
        EINSS7_00SCTP_BIND_IF_NOT_BOUND
    );

    if (ret_val != RETURN_OK)
      TTCN_error("EINSS7_00SctpBindReq failed: %d (%s)",
          ret_val, get_ein_sctp_error_message(ret_val,API_RETURN_CODES));
    else
      IPL4_DEBUG("EINSS7_00SctpBindReq was successful");

    IPL4_DEBUG("IPL4asp__PT_PROVIDER::do_bind(): Waiting for BindConf for user %d...", userId);

    do
    {
      ret_val = SS7Common::CallMsgRecv(&msg);
    }
    while(ret_val == MSG_TIMEOUT);

    if (ret_val != MSG_RECEIVE_OK)
      TTCN_error("IPL4asp__PT_PROVIDER::do_bind(): CallMsgRecv failed: %d (%s)",
          ret_val, SS7Common::get_ein_error_message(ret_val));
    /* Print the message into the log */
    log_msg("IPL4asp__PT_PROVIDER::do_bind(): Received message", &msg);

    /*Init the global variable*/
    port_ptr = this;
    ret_val = EINSS7_00SctpHandleInd(&msg);
    if (ret_val != RETURN_OK)
      TTCN_error("IPL4 test port (%s): EINSS7_00SctpHandleInd failed: "
          "%d (%s) Message is ignored.", get_name(), ret_val,
          SS7Common::get_ein_error_message(ret_val));
    else
      IPL4_DEBUG("PL4asp__PT_PROVIDER::do_bind(): message processing "
          "was successful");

    /* Release the buffer */

    ret_val = SS7Common::CallReleaseMsgBuffer(&msg);
    if (ret_val != RETURN_OK)
      TTCN_warning("IPL4 test port (%s): CallReleaseMsgBuffer "
          "failed: %d (%s)", get_name(), ret_val,
          SS7Common::get_ein_error_message(ret_val));
    else
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::do_bind(): message was released "
          "successfully");
    /* Reset the global variable */
    port_ptr = NULL;

    if (bindResult != EINSS7_00SCTP_OK)
      TTCN_warning("Bind failed: %d, %s, tries: %d.", bindResult,
          get_ein_sctp_error_message(ret_val,API_RETURN_CODES), bindRequestCounter);
    else {
      IPL4_DEBUG("The bind was successful.");

    }
  } while (bindResult != EINSS7_00SCTP_NTF_OK
      && bindRequestCounter <= 3);


  create_pipes();

  IPL4_DEBUG("IPL4asp__PT_PROVIDER::do_bind() : starting the second thread.");
  start_thread();
}

void IPL4asp__PT_PROVIDER::create_pipes()
{
  if (pipe(pipe_to_TTCN_thread_fds))
    TTCN_error("IPL4asp__PT_PROVIDER::create_pipe() @place1: pipe system call failed");
  if (pipe(pipe_to_TTCN_thread_log_fds))
    TTCN_error("IPL4asp__PT_PROVIDER::create_pipe() @place1: pipe system call failed");


  Handler_Add_Fd_Read(pipe_to_TTCN_thread_fds[0]);
  Handler_Add_Fd_Read(pipe_to_TTCN_thread_log_fds[0]);

  if (pipe(pipe_to_EIN_thread_fds))
    TTCN_error("IPL4asp__PT_PROVIDER::create_pipe() @place2: pipe system call failed");
}

void IPL4asp__PT_PROVIDER::start_thread()
{
  if (thread_started)
    return;
  if (pthread_create(&thread, NULL, IPL4asp__PT_PROVIDER::thread_main, this))
    TTCN_error("TCAPasp_PT_EIN_Interface::launch_thread(): pthread_create failed.");
  thread_started = TRUE;
}

void *IPL4asp__PT_PROVIDER::thread_main(void *arg)
{
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  static_cast<IPL4asp__PT_PROVIDER*>(arg)->ein_receive_loop();
  return NULL;
}

void IPL4asp__PT_PROVIDER::ein_receive_loop()
{
  for (; !exiting; )
  {
    MSG_T msg;
    USHORT_T ret_val;
    msg.receiver = userId;

    do {
      ret_val = SS7Common::CallMsgRecv(&msg);
    } while(!exiting && ret_val == MSG_TIMEOUT);

    //IPL4_DEBUG("A message arrived from the stack.");

    if(exiting)
      break;

    if (ret_val != MSG_RECEIVE_OK)
    {
      log_thread(TTCN_DEBUG,"IPL4asp__PT_PROVIDER::ein_receive_loop(): EINSS7CpMsgRecv_r failed: %d (%s)",
          ret_val, SS7Common::get_ein_error_message(ret_val));
      return;
    }

    write_pipe(pipe_to_TTCN_thread_fds);
    if(exiting)
      break;

    read_pipe(pipe_to_EIN_thread_fds);

    if(exiting)
      break;

    log_msg("IPL4asp__PT_PROVIDER::ein_receive_loop(): Received message", &msg);

    /* Set the global variable so that the EIN callback functions reach
     * our member functions */
    port_ptr = this;
    /* Pass the message to the EIN stack TCAP layer */
    log_thread(TTCN_DEBUG,"IPL4asp__PT_PROVIDER::ein_receive_loop(): passing incoming message to EIN");


    ret_val = EINSS7_00SctpHandleInd(&msg);
    if (ret_val != RETURN_OK)
      log_thread(TTCN_WARNING,"IPL4 test port (%s): EINSS7_00SctpHandleInd failed: "
          "%d (%s) Message is ignored.", get_name(), ret_val,
          SS7Common::get_ein_error_message(ret_val));
    else log_thread(TTCN_DEBUG,"IPL4asp__PT_PROVIDER::ein_receive_loop(): message processing "
        "was successful");

    port_ptr = NULL;
    write_pipe(pipe_to_TTCN_thread_fds);

    /* Release the buffer */
    ret_val = SS7Common::CallReleaseMsgBuffer(&msg);
    if (ret_val != RETURN_OK)
      log_thread(TTCN_WARNING,"IPL4 test port (%s): CallReleaseMsgBuffer "
          "failed: %d (%s)", get_name(), ret_val,
          SS7Common::get_ein_error_message(ret_val));
    else log_thread(TTCN_DEBUG,"IPL4asp__PT_PROVIDER::ein_receive_loop(): message was released "
        "successfully");
    /* Reset the global variable */
  }

  log_thread(TTCN_DEBUG,"IPL4asp__PT_PROVIDER::ein_receive_loop(): exiting... ");
}

void IPL4asp__PT_PROVIDER::destroy_pipes()
{
  Handler_Remove_Fd_Read(pipe_to_TTCN_thread_fds[0]);
  Handler_Remove_Fd_Read(pipe_to_TTCN_thread_log_fds[0]);

  close(pipe_to_TTCN_thread_fds[0]);
  pipe_to_TTCN_thread_fds[0] = -1;
  close(pipe_to_TTCN_thread_fds[1]);
  pipe_to_TTCN_thread_fds[1] = -1;
  close(pipe_to_TTCN_thread_log_fds[0]);
  pipe_to_TTCN_thread_log_fds[0] = -1;
  close(pipe_to_TTCN_thread_log_fds[1]);
  pipe_to_TTCN_thread_log_fds[1] = -1;
  close(pipe_to_EIN_thread_fds[0]);
  pipe_to_EIN_thread_fds[0] = -1;
  close(pipe_to_EIN_thread_fds[1]);
  pipe_to_EIN_thread_fds[1] = -1;
}


void IPL4asp__PT_PROVIDER::read_pipe(int pipe_fds[])
{
  //IPL4_DEBUG("TCAPasp_PT_EIN_Interface: Waiting in read_pipe()...");
  unsigned char buf;
  if (read(pipe_fds[0], &buf, 1) != 1){
    exiting=TRUE;
    TTCN_warning("IPL4asp__PT_PROVIDER::read_pipe(): read system call failed");
  }
}


void IPL4asp__PT_PROVIDER::write_pipe(int pipe_fds[])
{
  //IPL4_DEBUG("TCAPasp_PT_EIN_Interface: Writing in the pipe...");
  unsigned char buf = '\0';
  if (write(pipe_fds[1], &buf, 1) != 1){
    exiting=TRUE;
    TTCN_warning("IPL4asp__PT_PROVIDER::write_pipe(): write system call failed");
  }
}

void IPL4asp__PT_PROVIDER::log_thread(TTCN_Logger::Severity severity, const char *fmt, ...){
  if(exiting) {return;}
  if (severity!=TTCN_DEBUG || debugAllowed)
  {
    thread_log *log_msg=(thread_log *)Malloc(sizeof(thread_log));
    va_list args;
    va_start(args, fmt);
    log_msg->severity=severity;
    log_msg->msg=mprintf_va_list(fmt, args);
    int len = write(pipe_to_TTCN_thread_log_fds[1], &log_msg, sizeof(thread_log*));
    if(exiting) {return;}
    if (len == 0) {
      TTCN_error("Internal queue shutdown");
    } else if (len < 0) {
      TTCN_error("Error while writing to internal queue (errno = %d)", errno);
    } else if (len != sizeof(thread_log*)){
      TTCN_error("Partial write to the queue: %d bytes written (errno = %d)",
          len, errno);
    }

  }
}

void IPL4asp__PT_PROVIDER::log_thread_msg(const char *header, const MSG_T *msg){
  if (debugAllowed)
  {
    char *msgstr=mprintf("TCAP test port (%s): ", get_name());
    if (header != NULL) msgstr=mputprintf(msgstr,"%s: ", header);
    msgstr=mputprintf(msgstr,"{");
    msgstr=mputprintf(msgstr," Sender: %d,", msg->sender);
    msgstr=mputprintf(msgstr," Receiver: %d,", msg->receiver);
    msgstr=mputprintf(msgstr," Primitive: %d,", msg->primitive);
    msgstr=mputprintf(msgstr," Size: %d,", msg->size);
    msgstr=mputprintf(msgstr," Message:");
    for (USHORT_T i = 0; i < msg->size; i++)
      msgstr=mputprintf(msgstr," %02X", msg->msg_p[i]);
    msgstr=mputprintf(msgstr," }");
    log_thread(TTCN_DEBUG,msgstr);
    Free(msgstr);
  }
}

void IPL4asp__PT_PROVIDER::do_unbind()
{
  USHORT_T ret_val;
  int otherId = SCTP_ID;

  /* UnBind */
  if (bindResult == EINSS7_00SCTP_NTF_OK)
  {
    exiting = TRUE;

    IPL4_DEBUG("Sending:\n"
        "EINSS7_00SctpBindReq(\n"
        "sctpInstanceId=%d)",
        sctpInstanceId);
    ret_val = EINSS7_00SctpUnbindReq(sctpInstanceId);
    switch (ret_val)
    {
    case RETURN_OK:
      IPL4_DEBUG("EINSS7_00SctpBindReq(%d) was successful", sctpInstanceId);
      break;
    case MSG_NOT_CONNECTED:
      TTCN_warning("IPL4 test port (%s): "
          "The EIN stack was not bound", get_name());
      break;
    default:
      TTCN_error("EINSS7_00SctpBindReq(%d) failed: %d (%s)",
          sctpInstanceId, ret_val,
          SS7Common::get_ein_error_message(ret_val));
    }
  }else
    TTCN_warning("IPL4 test port (%s): "
        "was not bound.", get_name());
  bindResult = EINSS7_00SCTP_NOT_BOUND;

  // wait 0.5 sec to unbind reach the stack
  // before closing connection
  // 0.5 sec was recommended by Ulf.Melin@tietoenator.com
  // TR ID: 6801
  usleep (500000);
  /* Disconnect from EIN stack */

  SS7Common::disconnect_ein(userId, otherId, userInstanceId, sctpInstanceId);

  /* Clean up resources */
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::do_unbind() : destroying the pipes.");
  destroy_pipes();

  ein_connected = false;
  thread_started = false;
}


Socket__API__Definitions::Result IPL4asp__PT_PROVIDER::Listen_einsctp(const HostName& locName,
    const PortNumber& locPort, int next_action , const HostName& remName,
    const PortNumber& remPort,  const IPL4asp__Types::OptionList& options, const IPL4asp__Types::SocketList &sock_list){

  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  SockAddr sockAddr;
  socklen_t sockAddrLen;
  //struct hostent *hp = NULL;
  int hp = 0;

  IPL4_DEBUG("f__IPL4__PROVIDER__listen: enter %s %d", (const char *) locName, (int) locPort);
  testIfInitialized();

  int addr_index=-1;
  int num_of_addr=1;
  int config_group=0;
  for(int i=0; i<options.size_of();i++){
    if(options[i].get_selection()==Option::ALT_sctpAdditionalLocalAddresses){
      addr_index=i;
      num_of_addr+=options[i].sctpAdditionalLocalAddresses().size_of();
      //      break;
    }
    if(options[i].get_selection()==Option::ALT_sctpEINConfigGroup){
      config_group=options[i].sctpEINConfigGroup();
    }    
  }



  if (locPort < -1 || locPort > 65535)
    RETURN_ERROR_STACK(PortError::ERROR__INVALID__INPUT__PARAMETER);

  IPADDRESS_T *ip_struct=(IPADDRESS_T *)Malloc(num_of_addr*sizeof(IPADDRESS_T));
  memset((void *)ip_struct,0,num_of_addr*sizeof(IPADDRESS_T));

  hp=SetLocalSockAddr("f__IPL4__PROVIDER__listen",*this,AF_INET,locName, locPort, sockAddr, sockAddrLen);

  if (hp == -1) {
    SET_OS_ERROR_CODE;
    Free(ip_struct);
    RETURN_ERROR_STACK(PortError::ERROR__HOSTNAME);
  }

  char ip_addr[46];
  memset((void *)ip_addr,0,46);
  if(hp == AF_INET){
    inet_ntop(AF_INET,&sockAddr.v4.sin_addr.s_addr,ip_addr,46);
    ip_struct[0].addrType=EINSS7_00SCTP_IPV4;
  }
#ifdef USE_IPV6
  else {
    inet_ntop(AF_INET6,&sockAddr.v6.sin6_addr.s6_addr,ip_addr,46);
    ip_struct[0].addrType=EINSS7_00SCTP_IPV6;
  }
#endif  
  ip_struct[0].addr=(unsigned char*)mcopystr(ip_addr);
  ip_struct[0].addrLength=strlen(ip_addr)+1;

  for(int i=1; i<num_of_addr;i++){
    hp=SetLocalSockAddr("f__IPL4__PROVIDER__listen",*this,AF_INET,options[addr_index].sctpAdditionalLocalAddresses()[i-1], locPort, sockAddr, sockAddrLen);

    if (hp == -1) {
      SET_OS_ERROR_CODE;
      for(int k=0;k<i;k++){
        Free(ip_struct[k].addr);
      }
      Free(ip_struct);
      RETURN_ERROR_STACK(PortError::ERROR__HOSTNAME);
    }

    memset((void *)ip_addr,0,46);
    if(hp == AF_INET){
      inet_ntop(AF_INET,&sockAddr.v4.sin_addr.s_addr,ip_addr,46);
      ip_struct[i].addrType=EINSS7_00SCTP_IPV4;
    }
#ifdef USE_IPV6
    else {
      inet_ntop(AF_INET6,sockAddr.v6.sin6_addr.s6_addr,ip_addr,46);
      ip_struct[i].addrType=EINSS7_00SCTP_IPV6;
    }
#endif  
    ip_struct[i].addr=(unsigned char*)mcopystr(ip_addr);
    ip_struct[i].addrLength=strlen(ip_addr)+1;
  }

  int conn_id=ConnAddEin(next_action==SockDesc::ACTION_CONNECT?IPL4asp_SCTP:IPL4asp_SCTP_LISTEN,SockDesc::SOCK_NOT_KNOWN,-1,ip_addr,locPort,remName,remPort,next_action);
  sockList[conn_id].remote_addr_list=sock_list;


  USHORT_T init_result=EINSS7_00SctpInitializeGroupIdReq(
      userId,
      sctpInstanceId,
      next_action==SockDesc::ACTION_CONNECT,
      (int) globalConnOpts.sinit_max_instreams,
      (int) globalConnOpts.sinit_num_ostreams,
      conn_id,
      locPort,
      num_of_addr,
      ip_struct,
      0,
      config_group
  );
  for(int k=0;k<num_of_addr;k++){
    Free(ip_struct[k].addr);
  }
  Free(ip_struct);

  if(EINSS7_00SCTP_OK!=init_result){
    ConnDelEin(conn_id);
    result.errorCode()=PortError::ERROR__GENERAL;
    result.os__error__code()=init_result;
    result.os__error__text()=get_ein_sctp_error_message(init_result,API_RETURN_CODES);
    return result;
  }

  result.connId()=conn_id;
  result.errorCode()=PortError::ERROR__TEMPORARILY__UNAVAILABLE;
  if(globalConnOpts.extendedPortEvents == GlobalConnOpts::YES) {
    ASP__Event event;
    event.result() = result;
    incoming_message(event);
  }
  return result;

}  

int IPL4asp__PT_PROVIDER::ConnAddEin(SockType type,
    int assoc_enpoint, int parentIdx, const HostName& locName,
    const PortNumber& locPort,const HostName& remName,
    const PortNumber& remPort, int next_action)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: enter: assoc_enpoint: %d, parentIx: %d",
      assoc_enpoint, parentIdx);
  testIfInitialized();
  if (sockListCnt + N_RECENTLY_CLOSED >= sockListSize - 1 || sockList == NULL) {
    unsigned int sz = sockListSize;
    if (sockList != NULL) sz *= 2;
    SockDesc *newSockList =
        (SockDesc *)Realloc(sockList, sizeof(SockDesc) * sz);
    int i0 = (sockList == 0) ? 1 : sockListSize;
    sockList = newSockList;
    sockListSize = sz;
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: new sockListSize: %d", sockListSize);
    int j = firstFreeSock;
    for ( int i = sockListSize - 1; i >= i0; --i ) {
      memset(sockList + i, 0, sizeof (sockList[i]));
      sockList[i].sock = SockDesc::SOCK_NONEX;
      sockList[i].nextFree = j;
      j = i;
    }
    firstFreeSock = j;
    if (lastFreeSock == -1) lastFreeSock = sockListSize - 1;
  }

  int i = firstFreeSock;
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: connId: %d", i);

  if (parentIdx != -1) { // inherit the listener's properties
    sockList[i].userData = sockList[parentIdx].userData;
    sockList[i].getMsgLen = sockList[parentIdx].getMsgLen;
    sockList[i].getMsgLen_forConnClosedEvent = sockList[parentIdx].getMsgLen_forConnClosedEvent;
    sockList[i].parentIdx = parentIdx;
    sockList[parentIdx].ref_count++;
    sockList[i].msgLenArgs =
        new ro__integer(*sockList[parentIdx].msgLenArgs);
  } else { // otherwise initialize to defaults
    sockList[i].userData = 0;
    sockList[i].getMsgLen = defaultGetMsgLen;
    sockList[i].getMsgLen_forConnClosedEvent = defaultGetMsgLen_forConnClosedEvent;
    sockList[i].parentIdx = -1;
    sockList[i].msgLenArgs = new ro__integer(*defaultMsgLenArgs);
  }
  if (sockList[i].msgLenArgs == NULL)
    return -1;
  sockList[i].msgLen = -1;

  //  ae2IndexMap[assoc_enpoint] = i;
  sockList[i].ref_count=0;
  sockList[i].type = type;
  sockList[i].localaddr=new CHARSTRING(locName);
  sockList[i].localport=new PortNumber(locPort);
  sockList[i].remoteaddr=new CHARSTRING(remName);
  sockList[i].remoteport=new PortNumber(remPort);
  sockList[i].next_action=next_action;
  sockList[i].remote_addr_index=0;
  sockList[i].remote_addr_list=IPL4asp__Types::SocketList(NULL_VALUE);



  switch (type) {
  case IPL4asp_SCTP_LISTEN:
    sockList[i].buf = NULL;
    sockList[i].assocIdList = NULL;
    sockList[i].cnt = 0;
    break;
  case IPL4asp_SCTP:
    sockList[i].buf = (TTCN_Buffer **)Malloc(sizeof(TTCN_Buffer *));
    *sockList[i].buf = new TTCN_Buffer;
    if (*sockList[i].buf == NULL) {
      IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: failed to add socket %d", assoc_enpoint);
      Free(sockList[i].buf); sockList[i].buf = 0;
      return -1;
    }
    sockList[i].assocIdList = (sctp_assoc_t *)Malloc(sizeof(sctp_assoc_t));
    sockList[i].cnt = 1;
    break;
  default:
    break;
  }

  sockList[i].sock = assoc_enpoint;
  firstFreeSock = sockList[i].nextFree;
  sockList[i].nextFree = -1;
  ++sockListCnt;
  if(sockListCnt==1) {lonely_conn_id=i;}
  else {lonely_conn_id=-1;}
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnAdd: leave: sockListCnt: %i", sockListCnt);

  return i;
} // IPL4asp__PT::ConnAdd


int IPL4asp__PT_PROVIDER::ConnDelEin(int connId)
{
  IPL4_DEBUG("IPL4asp__PT_PROVIDER::ConnDel: enter: connId: %d", connId);

  if(sockList[connId].parentIdx!=-1){
    int parentIdx=sockList[connId].parentIdx;
    sockList[parentIdx].ref_count--;
    if(sockList[parentIdx].ref_count==0 && sockList[parentIdx].next_action==SockDesc::ACTION_DELETE){
      EINSS7_00SctpDestroyReq(sockList[parentIdx].endpoint_id);
      ep2IndexMap.erase(sockList[parentIdx].endpoint_id);
      ConnDelEin(parentIdx);
    }
  } else {
    if(sockList[connId].ref_count!=0) {
      sockList[connId].next_action=SockDesc::ACTION_DELETE;
      return connId;
    }
    EINSS7_00SctpDestroyReq(sockList[connId].endpoint_id);
  }

  sockList[connId].clear();
  sockList[lastFreeSock].nextFree = connId;
  lastFreeSock = connId;
  sockListCnt--;
  if(sockListCnt==1) {
    unsigned int i=0;
    while(i<sockListSize && sockList[i].sock!=SockDesc::SOCK_NONEX){
      i++;
    }
    lonely_conn_id=i;
  }
  else {lonely_conn_id=-1;}

  return connId;
} // IPL4asp__PT_PROVIDER::ConnDel


USHORT_T  IPL4asp__PT_PROVIDER::SctpInitializeConf(
    UCHAR_T returnCode,
    ULONG_T sctpEndpointId,
    USHORT_T assignedMis,
    USHORT_T assignedOsServerMode,
    USHORT_T maxOs,
    ULONG_T pmtu,
    ULONG_T mappingKey,
    USHORT_T localPort
){
  IPL4_DEBUG("SctpInitializeConf sctpEndpointId %ul  mappingKey %ul returnCode %d ",sctpEndpointId,mappingKey,returnCode);

  if(EINSS7_00SCTP_NTF_DUPLICATE_INIT==returnCode && sockList[mappingKey].next_action==SockDesc::ACTION_CONNECT){
    std::map<int,int>::iterator it = ep2IndexMap.find(sctpEndpointId);
    int parent_id=it->second;
    sockList[parent_id].ref_count++;
    sockList[mappingKey].parentIdx=parent_id;

  } else if(EINSS7_00SCTP_NTF_OK!=returnCode){
    Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    EINSS7_00SctpDestroyReq(sctpEndpointId);
    result.errorCode()=PortError::ERROR__GENERAL;
    result.os__error__code()=returnCode;
    result.os__error__text()=get_ein_sctp_error_message(returnCode,CONF_RETURNCODE);
    result.connId()=mappingKey;
    ASP__Event event;
    event.result() = result;
    incoming_message(event);
    ProtoTuple proto;
    proto.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    sendConnClosed(mappingKey,
        *(sockList[mappingKey].remoteaddr),
        *(sockList[mappingKey].remoteport),
        *(sockList[mappingKey].localaddr),
        localPort,
        proto, sockList[mappingKey].userData);
    
    ConnDelEin(mappingKey);
    return RETURN_OK;
  }


  sockList[mappingKey].sock=sctpEndpointId;
  sockList[mappingKey].maxOs=maxOs;
  sockList[mappingKey].endpoint_id=sctpEndpointId;
  *(sockList[mappingKey].localport)=localPort;
  if(sockList[mappingKey].next_action==SockDesc::ACTION_CONNECT){
    if(EINSS7_00SCTP_NTF_OK==returnCode){
      sockList[mappingKey].ref_count=0;
      ep2IndexMap[sctpEndpointId] = mappingKey;
    }
    char rem_addr[46];
    memset((void *)rem_addr,0,46);
    IPADDRESS_T ip_struct;
    strcpy(rem_addr,(const char*)*(sockList[mappingKey].remoteaddr));
    if(!strchr(rem_addr,':')){
      ip_struct.addrType=EINSS7_00SCTP_IPV4;
    }
#ifdef USE_IPV6
    else {
      ip_struct.addrType=EINSS7_00SCTP_IPV6;
    }
#endif 
    ip_struct.addr=(unsigned char*)rem_addr;
    ip_struct.addrLength=strlen(rem_addr)+1;

    USHORT_T req_result=EINSS7_00SctpAssociateReq(
        sctpEndpointId,
        maxOs<(int) globalConnOpts.sinit_num_ostreams?maxOs:(int) globalConnOpts.sinit_num_ostreams,
            mappingKey,
            *(sockList[mappingKey].remoteport),
            ip_struct
    );

    if(EINSS7_00SCTP_OK!=req_result){
      Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      EINSS7_00SctpDestroyReq(sctpEndpointId);
      result.errorCode()=PortError::ERROR__GENERAL;
      result.os__error__code()=returnCode;
      result.os__error__text()=get_ein_sctp_error_message(returnCode,API_RETURN_CODES);
      result.connId()=mappingKey;
      ASP__Event event;
      event.result() = result;
      incoming_message(event);
      ProtoTuple proto;
      proto.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      sendConnClosed(mappingKey,
          *(sockList[mappingKey].remoteaddr),
          *(sockList[mappingKey].remoteport),
          *(sockList[mappingKey].localaddr),
          localPort,
          proto, sockList[mappingKey].userData);
      ConnDelEin(mappingKey);
    }

  } else {
    Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    sockList[mappingKey].next_action=SockDesc::ACTION_NONE;
    sockList[mappingKey].ref_count=0;
    ep2IndexMap[sctpEndpointId] = mappingKey;
    result.connId()=mappingKey;
    result.errorCode()=PortError::ERROR__AVAILABLE;
    ASP__Event event;
    event.result() = result;
    incoming_message(event);
  }
  return RETURN_OK;

}

USHORT_T  IPL4asp__PT_PROVIDER::SctpAssociateConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey
){
  IPL4_DEBUG(" SctpAssociateConf assocId %ul  ulpKey %ul returnCode %d ",assocId,ulpKey,returnCode);

  if(EINSS7_00SCTP_NTF_OK!=returnCode){
    Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    EINSS7_00SctpDestroyReq(sockList[ulpKey].endpoint_id);
    result.errorCode()=PortError::ERROR__GENERAL;
    result.os__error__code()=returnCode;
    result.os__error__text()=get_ein_sctp_error_message(returnCode,CONF_RETURNCODE);
    result.connId()=ulpKey;
    ASP__Event event;
    event.result() = result;
    incoming_message(event);
    ProtoTuple proto;
    proto.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    sendConnClosed(ulpKey,
        *(sockList[ulpKey].remoteaddr),
        *(sockList[ulpKey].remoteport),
        *(sockList[ulpKey].localaddr),
        *(sockList[ulpKey].localport),
        proto, sockList[ulpKey].userData);

    ConnDelEin(ulpKey);
    return RETURN_OK;
  }
  sockList[ulpKey].sock=assocId;
  Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
  //    sockList[ulpKey].next_action=SockDesc::ACTION_NONE;
  result.connId()=ulpKey;
  result.errorCode()=PortError::ERROR__AVAILABLE;
  ASP__Event event;
  event.result() = result;
  incoming_message(event);

  return RETURN_OK;

}

// Sometimes the stack is too fast and puts the indicatin into buffer before we can set the correct ulpkey
//   Stack                                                             IPL4 TP
//
//  SctpCommUpInd(ulpKey of listening socket)   --------|
//  SctpDataArriveInd(ulpKey of listening socket) --|   |
//                                                  |   |-------------> EINSS7_00SctpSetUlpKeyReq(new ulpKey)
//                                                  |                             |
//  EINSS7_00SctpSetUlpKeyReq processed <-----------------------------------------|
//                                                  |----------------->SctpDataArriveInd(ulpKey of listening socket)
//
//
// The verify_and_set_connid tries to detect the case and find the correct ulpKey, which is the connId of the IPL4 port
// the verification is based on the stored assocId
// The possible source of the ulpKey mixup is the parallel threads in the EIN stack

int IPL4asp__PT_PROVIDER::verify_and_set_connid(ULONG_T ulpKey, ULONG_T assocId){
  if(isConnIdValid(ulpKey) && sockList[ulpKey].sock==(int)assocId
      && sockList[ulpKey].type==IPL4asp_SCTP ){
      return ulpKey;             // correct ulpKey
  }
  // the key is not correct
  // Try to find in the database
  for(unsigned int a=0; a<sockListSize; a++){
    if(sockList[a].sock==(int)assocId
      && sockList[a].type==IPL4asp_SCTP){
      return a;    // the correct key has been found
    }
  }
  return -1; // WTF????? We're going to die.

}

USHORT_T  IPL4asp__PT_PROVIDER::SctpCommUpInd(
    ULONG_T sctpEndpointId,
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T origin,
    USHORT_T outboundStreams,
    USHORT_T inboundStreams,
    USHORT_T remotePort,
    UCHAR_T numOfRemoteIpAddrs,
    IPADDRESS_T * remoteIpAddrList_sp
){
  IPL4_DEBUG(" SctpCommUpInd assocId %ul  ulpKey %ul sctpEndpointId %d origin %d ",assocId,ulpKey,sctpEndpointId,origin);
  if(isConnIdValid(ulpKey) && sockList[ulpKey].sock==(int)assocId
      && sockList[ulpKey].endpoint_id==(int)sctpEndpointId ){

    sockList[ulpKey].next_action=SockDesc::ACTION_NONE;
    ASP__Event event;
    event.sctpEvent().sctpAssocChange().clientId() = ulpKey;
    event.sctpEvent().sctpAssocChange().proto().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__COMM__UP;
    incoming_message(event);

  } else {
    // new connection
    std::map<int,int>::iterator it = ep2IndexMap.find(sctpEndpointId);
    int parent_id=it->second;
    int conn_id;
    if(sockList[parent_id].next_action==SockDesc::ACTION_DELETE){
      conn_id=ConnAddEin(IPL4asp_SCTP,assocId,parent_id,
          *(sockList[parent_id].localaddr),
          *(sockList[parent_id].localport),
          CHARSTRING(remoteIpAddrList_sp[0].addrLength-1,(const char*)remoteIpAddrList_sp[0].addr)
          ,remotePort,
          SockDesc::ACTION_DELETE);
      sockList[conn_id].endpoint_id=sctpEndpointId;
      EINSS7_00SctpSetUlpKeyReq(assocId,conn_id);
      EINSS7_00SctpAbortReq(assocId);
      return RETURN_OK;
    }
    conn_id=ConnAddEin(IPL4asp_SCTP,assocId,parent_id,
        *(sockList[parent_id].localaddr),
        *(sockList[parent_id].localport),
        CHARSTRING(remoteIpAddrList_sp[0].addrLength-1,(const char*)remoteIpAddrList_sp[0].addr)
        ,remotePort,
        SockDesc::ACTION_NONE);

    sockList[conn_id].endpoint_id=sctpEndpointId;

    ASP__Event event;


    reportConnOpened(conn_id);

    EINSS7_00SctpSetUlpKeyReq(assocId,conn_id);

    event.sctpEvent().sctpAssocChange().clientId() = conn_id;
    event.sctpEvent().sctpAssocChange().proto().sctp() = SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    event.sctpEvent().sctpAssocChange().sac__state() = IPL4asp__Types::SAC__STATE::SCTP__COMM__UP;
    incoming_message(event);


  }
  return RETURN_OK;
}


void IPL4asp__PT_PROVIDER::handle_message_from_ein(int fd){
  if(pipe_to_TTCN_thread_log_fds[0]== fd){
    thread_log *r;
    int len = read(pipe_to_TTCN_thread_log_fds[0], &r, sizeof(thread_log *));
    if (len == sizeof(thread_log *)) {
      TTCN_Logger::begin_event(r->severity);
      TTCN_Logger::log_event("%s", r->msg);
      TTCN_Logger::end_event();

      Free(r->msg);
      Free(r);
    }
    else if (len == 0) {
      TTCN_warning(get_name(), "Internal queue shutdown");
    }
    else if (len < 0) {
      TTCN_warning(get_name(), "Error while reading from internal queue (errno = %d)", errno);
    }
    else {
      TTCN_warning(get_name(),
          "Partial read from the queue: %d bytes read (errno = %d)", len, errno);
    }
  } else if(pipe_to_TTCN_thread_fds[0] == fd){
    read_pipe(pipe_to_TTCN_thread_fds);
    write_pipe(pipe_to_EIN_thread_fds);
    read_pipe(pipe_to_TTCN_thread_fds);
  }

}
USHORT_T  IPL4asp__PT_PROVIDER::SctpDataArriveInd(
    ULONG_T assocId,
    USHORT_T streamId,
    ULONG_T ulpKey,
    ULONG_T payloadProtId,
    BOOLEAN_T unorderFlag,
    USHORT_T streamSequenceNumber,
    UCHAR_T partialDeliveryFlag,
    ULONG_T dataLength,
    UCHAR_T * data_p
){

  IPL4_DEBUG("SctpDataArriveInd  assocId %ul  ulpKey %ul  streamId %ud  payloadProtId %ul dataLength %ul",assocId,ulpKey,streamId,payloadProtId,dataLength);
  int connid=verify_and_set_connid(ulpKey,assocId);
  IPL4_DEBUG( "SctpDataArriveInd corretced connid %d",connid);

  switch(partialDeliveryFlag){
  case EINSS7_00SCTP_LAST_PARTIAL_DELIVERY:
  case EINSS7_00SCTP_NO_PARTIAL_DELIVERY:
  {
    ASP__RecvFrom asp;
    asp.connId() = connid;
    asp.userData() = sockList[connid].userData;
    asp.remName() = *(sockList[connid].remoteaddr);
    asp.remPort() = *(sockList[connid].remoteport);
    asp.locName() = *(sockList[connid].localaddr);
    asp.locPort() = *(sockList[connid].localport);
    sockList[connid].buf[0]->put_s(dataLength, data_p);
    //          IPL4_DEBUG("PL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: Incoming data (%ld bytes): stream = %hu, ssn = %hu, flags = %hx, ppid = %u \n", n,
    //          sri->sinfo_stream,(unsigned int)sri->sinfo_ssn, sri->sinfo_flags, sri->sinfo_ppid);

    INTEGER i_ppid;
    i_ppid.set_long_long_val(payloadProtId);

    asp.proto().sctp() = SctpTuple(streamId, i_ppid, OMIT_VALUE, OMIT_VALUE);

    sockList[connid].buf[0]->get_string(asp.msg());
    incoming_message(asp);
    if(sockList[connid].buf) sockList[connid].buf[0]->clear();
  }
  break;
  case EINSS7_00SCTP_FIRST_PARTIAL_DELIVERY:
  case EINSS7_00SCTP_MIDDLE_PARTIAL_DELIVERY:
    IPL4_DEBUG("IPL4asp__PT_PROVIDER::Handle_Fd_Event_Readable: partial receive: %ud bytes", dataLength);
    sockList[connid].buf[0]->put_s(dataLength, data_p);
    break;
  default:

    break;
  }

  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpCommLostInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T eventType,
    UCHAR_T origin
){

  IPL4_DEBUG(" SctpCommLostInd assocId %ul  ulpKey %ul eventType %d origin %d ",assocId,ulpKey,eventType,origin);
  if(sockList[ulpKey].next_action==SockDesc::ACTION_CONNECT &&
      globalConnOpts.connection_method==GlobalConnOpts::METHOD_ONE &&
      sockList[ulpKey].remote_addr_index<sockList[ulpKey].remote_addr_list.lengthof()){

    char rem_addr[46];
    memset((void *)rem_addr,0,46);
    IPADDRESS_T ip_struct;
    strcpy(rem_addr,(const char*)(sockList[ulpKey].remote_addr_list[sockList[ulpKey].remote_addr_index].hostName()));
    sockList[ulpKey].remote_addr_index++;
    if(!strchr(rem_addr,':')){
      ip_struct.addrType=EINSS7_00SCTP_IPV4;
    }
#ifdef USE_IPV6
    else {
      ip_struct.addrType=EINSS7_00SCTP_IPV6;
    }
#endif 
    ip_struct.addr=(unsigned char*)rem_addr;
    ip_struct.addrLength=strlen(rem_addr)+1;

    USHORT_T req_result=EINSS7_00SctpAssociateReq(
        sockList[ulpKey].endpoint_id,
        sockList[ulpKey].maxOs<(int) globalConnOpts.sinit_num_ostreams?sockList[ulpKey].maxOs:(int) globalConnOpts.sinit_num_ostreams,
            ulpKey,
            *(sockList[ulpKey].remoteport),
            ip_struct
    );

    if(EINSS7_00SCTP_OK!=req_result){
      Result result(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      EINSS7_00SctpDestroyReq(sockList[ulpKey].endpoint_id);
      result.errorCode()=PortError::ERROR__GENERAL;
      result.os__error__code()=req_result;
      result.os__error__text()=get_ein_sctp_error_message(req_result,API_RETURN_CODES);
      result.connId()=ulpKey;
      ASP__Event event;
      event.result() = result;
      incoming_message(event);
      ProtoTuple proto;
      proto.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
      sendConnClosed(ulpKey,
          *(sockList[ulpKey].remoteaddr),
          *(sockList[ulpKey].remoteport),
          *(sockList[ulpKey].localaddr),
          *(sockList[ulpKey].localport),
          proto, sockList[ulpKey].userData);
      ConnDelEin(ulpKey);
    }

    return RETURN_OK;
  } 
  else if(sockList[ulpKey].next_action!=SockDesc::ACTION_DELETE){
    ASP__Event event;
    ProtoTuple proto;
    proto.sctp()=SctpTuple(OMIT_VALUE, OMIT_VALUE, OMIT_VALUE, OMIT_VALUE);
    sendConnClosed(ulpKey,
        *(sockList[ulpKey].remoteaddr),
        *(sockList[ulpKey].remoteport),
        *(sockList[ulpKey].localaddr),
        *(sockList[ulpKey].localport),
        proto, sockList[ulpKey].userData);
  } else {
    sockList[ulpKey].next_action=SockDesc::ACTION_DELETE;
  }
  ConnDelEin(ulpKey);
  return RETURN_OK;

}

USHORT_T  IPL4asp__PT_PROVIDER::SctpAssocRestartInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    USHORT_T outboundStreams,
    USHORT_T inboundStreams,
    UCHAR_T numOfRemoteIpAddrs,
    IPADDRESS_T * remoteIpAddrList_sp
){
  IPL4_DEBUG("SctpAssocRestartInd  assocId %ul  ulpKey %ul ",assocId,ulpKey);

  return RETURN_OK;
}


USHORT_T  IPL4asp__PT_PROVIDER::SctpCommErrorInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T errorCode
){
  IPL4_DEBUG("SctpCommErrorInd  assocId %ul  ulpKey %ul  errorCode %d",assocId,ulpKey,errorCode);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpShutdownConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey
){
  IPL4_DEBUG(" SctpShutdownConf  assocId %ul  ulpKey %ul returnCode %d",assocId,ulpKey,returnCode);
  return RETURN_OK;
}


USHORT_T  IPL4asp__PT_PROVIDER::SctpCongestionCeaseInd(
    ULONG_T assocId,
    ULONG_T ulpKey
){
  IPL4_DEBUG(" SctpCongestionCeaseInd  assocId %ul  ulpKey %ul ",assocId,ulpKey);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpSendFailureInd(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey,
    ULONG_T dataLength,
    UCHAR_T * data_p,
    ULONG_T payloadProtId,
    USHORT_T streamId,
    ULONG_T userSequence,
    IPADDRESS_T * remoteIpAddr_s,
    BOOLEAN_T unorderFlag
){
  IPL4_DEBUG("SctpSendFailureInd   assocId %ul  ulpKey %ul returnCode %d",assocId,ulpKey,returnCode);
  return RETURN_OK;
}


USHORT_T  IPL4asp__PT_PROVIDER::SctpNetworkStatusChangeInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T newStatus,
    IPADDRESS_T remoteIpAddr_s
){
  IPL4_DEBUG(" SctpNetworkStatusChangeInd  assocId %ul  ulpKey %ul newStatus %d",assocId,ulpKey,newStatus);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpIndError(
    USHORT_T errorCode,
    MSG_T * msg_sp
){
  IPL4_DEBUG(" SctpIndError  errorCode %d ",errorCode);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpBindConf(
    EINSS7INSTANCE_T sctpInstanceId,
    UCHAR_T error
){
  IPL4_DEBUG(" SctpBindConf sctpInstanceId %ul, error %d ",sctpInstanceId, error);
  bindResult= error;
  return RETURN_OK;
}

USHORT_T IPL4asp__PT_PROVIDER::SctpStatusConf(
    UCHAR_T returnCode,
    ULONG_T mappingKey,
    ULONG_T sctpEndpointId,
    ULONG_T numOfAssociations,
    ASSOC_T * assocStatusList_sp
){
  IPL4_DEBUG(" SctpStatusConf returnCode %d mappingKey %ul, sctpEndpointId %ul",returnCode,mappingKey,sctpEndpointId);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpTakeoverConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T sctpEndpointId
){
  IPL4_DEBUG("SctpTakeoverConf returnCode %d assocId %ul, sctpEndpointId %ul",returnCode,assocId,sctpEndpointId);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpTakeoverInd(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T sctpEndpointId
){
  IPL4_DEBUG("SctpTakeoverInd returnCode %d assocId %ul, sctpEndpointId %ul",returnCode,assocId,sctpEndpointId);
  return RETURN_OK;
}

USHORT_T  IPL4asp__PT_PROVIDER::SctpCongestionInd(
    ULONG_T assocId,
    ULONG_T ulpKey
){
  IPL4_DEBUG("SctpCongestionInd assocId %ul, ulpKey %ul",assocId,ulpKey);
  return RETURN_OK;
}


#ifdef EIN_R3B

USHORT_T  EINSS7_00SCTPSETEPALIASCONF(
    UCHAR_T returnCode,
    ULONG_T sctpEndpointId,
    ULONG_T epAlias
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPSETEPALIASCONF");
  return RETURN_OK;
}

USHORT_T EINSS7_00SCTPSETUSERCONGESTIONLEVELCONF (
    UCHAR_T returnCode,
    ULONG_T epAlias,
    UCHAR_T congestionLevel,
    ULONG_T numberOfAffectedEndpoints
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPSETUSERCONGESTIONLEVELCONF");
  return RETURN_OK;
}

USHORT_T EINSS7_00SCTPGETUSERCONGESTIONLEVELCONF (
    UCHAR_T returnCode,
    ULONG_T epAlias,
    UCHAR_T congestionLevel
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPGETUSERCONGESTIONLEVELCONF");
  return RETURN_OK;
}

UINT16_T EINSS7_00SCTPREDIRECTIND (
    UINT8_T action,
    UINT16_T oldSctpFeInstance,
    UINT16_T newSctpFeInstance
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPREDIRECTIND");
  return RETURN_OK;
}

UINT16_T EINSS7_00SCTPINFOCONF (
    UINT8_T result,
    UINT8_T reservedByte,
    UINT8_T partialDeliveryFlag,
    UINT32_T mappingKey,
    EINSS7_00SCTP_INFO_TAG_T * topTag
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPINFOCONF");
  EINSS7_00SctpInfoTagRemove(topTag,true);
  return RETURN_OK;
}

UINT16_T EINSS7_00SCTPINFOIND (
    UINT8_T reservedByte1,
    UINT8_T reservedByte2,
    UINT8_T partialDeliveryFlag,
    UINT32_T mappingKey,
    EINSS7_00SCTP_INFO_TAG_T * topTag
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPINFOIND");
  EINSS7_00SctpInfoTagRemove(topTag,true);
  return RETURN_OK;
}

UINT16_T EINSS7_00SCTPUPDATECONF (
    UINT8_T result,
    UINT8_T reservedByte,
    UINT8_T partialDeliveryFlag,
    UINT32_T mappingKey,
    EINSS7_00SCTP_INFO_TAG_T * topTag
){
//  IPL4_PORTREF_DEBUG(port_ptr,"EINSS7_00SCTPUPDATECONF");
  EINSS7_00SctpInfoTagRemove(topTag,true);
  return RETURN_OK;
}


#endif


const char *IPL4asp__PT_PROVIDER::get_ein_sctp_error_message(const USHORT_T value, const code_set code_set_spec)
{

  switch (code_set_spec){
  case API_RETURN_CODES:
    switch (value)
    {
    case EINSS7_00SCTP_OK:
      return "EINSS7_00SCTP_OK";
      break;
    case EINSS7_00SCTP_INVALID_ASSOC_ID:
      return "EINSS7_00SCTP_INVALID_ASSOC_ID";
      break;
    case EINSS7_00SCTP_INVALID_OS:
      return "EINSS7_00SCTP_INVALID_OS";
      break;
    case EINSS7_00SCTP_INVALID_ADDR_TYPE:
      return "EINSS7_00SCTP_INVALID_ADDR_TYPE";
      break;
    case EINSS7_00SCTP_INVALID_INSTANCE_ID:
      return "EINSS7_00SCTP_INVALID_INSTANCE_ID";
      break;
    case EINSS7_00SCTP_INVALID_REQ_MIS:
      return "EINSS7_00SCTP_INVALID_REQ_MIS";
      break;
    case EINSS7_00SCTP_INVALID_OS_SM:
      return "EINSS7_00SCTP_INVALID_OS_SM";
      break;
    case EINSS7_00SCTP_NULL_IP_LIST:
      return "EINSS7_00SCTP_NULL_IP_LIST";
      break;
    case EINSS7_00SCTP_NO_DATA:
      return "EINSS7_00SCTP_NO_DATA";
      break;
    case EINSS7_00SCTP_CALLBACK_FUNC_ALREADY_SET:
      return "EINSS7_00SCTP_CALLBACK_FUNC_ALREADY_SET";
      break;
    case EINSS7_00SCTP_CALLBACK_FUNC_NOT_SET:
      return "EINSS7_00SCTP_CALLBACK_FUNC_NOT_SET";
      break;
    case EINSS7_00SCTP_MSG_ARG_VAL:
      return "EINSS7_00SCTP_MSG_ARG_VAL";
      break;
    case EINSS7_00SCTP_MSG_OUT_OF_MEMORY:
      return "EINSS7_00SCTP_MSG_OUT_OF_MEMORY";
      break;
    case EINSS7_00SCTP_MSG_GETBUF_FAIL:
      return "EINSS7_00SCTP_MSG_GETBUF_FAIL";
      break;
    case EINSS7_00SCTP_UNKNOWN_ERROR:
      return "EINSS7_00SCTP_UNKNOWN_ERROR";
      break;
    case EINSS7_00SCTP_WRONG_VERSION_ERROR:
      return "EINSS7_00SCTP_WRONG_VERSION_ERROR";
      break;
    case EINSS7_00SCTP_NO_SCTP_INST_ERROR:
      return "EINSS7_00SCTP_NO_SCTP_INST_ERROR";
      break;
    case EINSS7_00SCTP_NOT_BOUND:
      return "EINSS7_00SCTP_NOT_BOUND";
      break;
    case EINSS7_00SCTP_NOT_CONNECTED:
      return "EINSS7_00SCTP_NOT_CONNECTED";
      break;
#ifdef EIN_R3B
    case EINSS7_00SCTP_INVALID_TAG_POINTER:
      return "EINSS7_00SCTP_INVALID_TAG_POINTER";
      break;
    case EINSS7_00SCTP_INETPTON_FAILED:
      return "EINSS7_00SCTP_INETPTON_FAILED";
      break;
#endif
    case EINSS7_00SCTP_IND_MEMORY_ERROR:
      return "EINSS7_00SCTP_IND_MEMORY_ERROR";
      break;
    case EINSS7_00SCTP_MSG_WOULD_BLOCK:
      return "EINSS7_00SCTP_MSG_WOULD_BLOCK";
      break;
    default:
      return "Unknown error";
    }
    break;

    case CONF_RETURNCODE:
      switch (value)
      {
      case EINSS7_00SCTP_NTF_OK:
        return "EINSS7_00SCTP_OK";
        break;
      case EINSS7_00SCTP_NTF_MAX_INSTANCES_REACHED:
        return "EINSS7_00SCTP_NTF_MAX_INSTANCES_REACHED";
        break;
      case EINSS7_00SCTP_NTF_INVALID_TRANS_ADDR:
        return "EINSS7_00SCTP_NTF_INVALID_TRANS_ADDR";
        break;
      case EINSS7_00SCTP_NTF_INVALID_ENDPOINT_ID:
        return "EINSS7_00SCTP_NTF_INVALID_ENDPOINT_ID";
        break;
      case EINSS7_00SCTP_NTF_INCORRECT_IP_ADDR:
        return "EINSS7_00SCTP_NTF_INCORRECT_IP_ADDR";
        break;
      case EINSS7_00SCTP_NTF_OS_INCORRECT:
        return "EINSS7_00SCTP_NTF_OS_INCORRECT";
        break;
      case EINSS7_00SCTP_NTF_INVALID_ADDR_TYPE:
        return "EINSS7_00SCTP_NTF_INVALID_ADDR_TYPE";
        break;
      case EINSS7_00SCTP_NTF_NO_BUFFER_SPACE:
        return "EINSS7_00SCTP_NTF_NO_BUFFER_SPACE";
        break;
      case EINSS7_00SCTP_NTF_CONNECTION_CLOSING_DOWN:
        return "EINSS7_00SCTP_NTF_CONNECTION_CLOSING_DOWN";
        break;
      case EINSS7_00SCTP_NTF_INVALIDS_ASSOCIATION_ID:
        return "EINSS7_00SCTP_NTF_INVALIDS_ASSOCIATION_ID";
        break;
      case EINSS7_00SCTP_NTF_MAX_ASSOC_REACHED:
        return "EINSS7_00SCTP_NTF_MAX_ASSOC_REACHED";
        break;
      case EINSS7_00SCTP_NTF_INTERNAL_ERROR:
        return "EINSS7_00SCTP_NTF_INTERNAL_ERROR";
        break;
      case EINSS7_00SCTP_NTF_WRONG_PRIMITIVE_FORMAT:
        return "EINSS7_00SCTP_NTF_WRONG_PRIMITIVE_FORMAT";
        break;
      case EINSS7_00SCTP_NTF_NO_USER_DATA:
        return "EINSS7_00SCTP_NTF_NO_USER_DATA";
        break;
      case EINSS7_00SCTP_NTF_INV_STREAM_ID:
        return "EINSS7_00SCTP_NTF_INV_STREAM_ID";
        break;
      case EINSS7_00SCTP_NTF_WRONG_USER_MODULE_ID:
        return "EINSS7_00SCTP_NTF_WRONG_USER_MODULE_ID";
        break;
      case EINSS7_00SCTP_NTF_MAX_USER_INSTS_REACHED:
        return "EINSS7_00SCTP_NTF_MAX_USER_INSTS_REACHED";
        break;
      case EINSS7_00SCTP_NTF_WRONG_DSCP:
        return "EINSS7_00SCTP_NTF_WRONG_DSCP";
        break;
      case EINSS7_00SCTP_NTF_ALREADY_BOUND:
        return "EINSS7_00SCTP_NTF_ALREADY_BOUND";
        break;
      case EINSS7_00SCTP_NTF_INVALID_LOCAL_IP:
        return "EINSS7_00SCTP_NTF_INVALID_LOCAL_IP";
        break;
      case EINSS7_00SCTP_NTF_INVALID_IF_VERSION:
        return "EINSS7_00SCTP_NTF_INVALID_IF_VERSION";
        break;
      case EINSS7_00SCTP_NTF_USER_NOT_BOUND:
        return "EINSS7_00SCTP_NTF_USER_NOT_BOUND";
        break;
      case EINSS7_00SCTP_NTF_WRONG_STATE:
        return "EINSS7_00SCTP_NTF_WRONG_STATE";
        break;
      case EINSS7_00SCTP_NTF_DUPLICATE_INIT:
        return "EINSS7_00SCTP_NTF_DUPLICATE_INIT";
        break;
      case EINSS7_00SCTP_NTF_DUPLICATE_INIT_ANOTHER_INSTANCE:
        return "EINSS7_00SCTP_NTF_DUPLICATE_INIT_ANOTHER_INSTANCE";
        break;
      case EINSS7_00SCTP_NTF_INVALID_CONFIGURATION_GROUP_ID:
        return "EINSS7_00SCTP_NTF_INVALID_CONFIGURATION_GROUP_ID";
        break;
      case EINSS7_00SCTP_NTF_UNABLE_TO_ASSIGN_PORT_NUMBER:
        return "EINSS7_00SCTP_NTF_UNABLE_TO_ASSIGN_PORT_NUMBER";
        break;
#ifdef EIN_R3B
      case EINSS7_00SCTP_NTF_UNLOADING:
        return "EINSS7_00SCTP_NTF_UNLOADING";
        break;
      case EINSS7_00SCTP_NTF_BIG_IP_LIST:
        return "EINSS7_00SCTP_NTF_BIG_IP_LIST";
        break;
      case EINSS7_00SCTP_NTF_ENDPOINT_IS_BEING_DELETED:
        return "EINSS7_00SCTP_NTF_ENDPOINT_IS_BEING_DELETED";
        break;
#endif
      default:
        return "Unknown error";
      }
      break;


      default:
        return "Unknown error";
  }
}

void IPL4asp__PT_PROVIDER::log_msg(const char *header, const MSG_T *msg)
{
  if (debugAllowed)
  {
    TTCN_Logger::begin_event(TTCN_DEBUG);
    TTCN_Logger::log_event("IPL4 test port (%s): ", get_name());
    if (header != NULL) TTCN_Logger::log_event("%s: ", header);
    TTCN_Logger::log_event_str("{");
    TTCN_Logger::log_event(" Sender: %d,", msg->sender);
    TTCN_Logger::log_event(" Receiver: %d,", msg->receiver);
    TTCN_Logger::log_event(" Primitive: %d,", msg->primitive);
    TTCN_Logger::log_event(" Size: %d,", msg->size);
    TTCN_Logger::log_event_str(" Message:");
    for (USHORT_T i = 0; i < msg->size; i++)
      TTCN_Logger::log_event(" %02X", msg->msg_p[i]);
    TTCN_Logger::log_event_str(" }");
    TTCN_Logger::end_event();
  }
}
#endif


} // namespace IPL4asp__PortType

#ifdef USE_IPL4_EIN_SCTP

USHORT_T  EINSS7_00SctpAssocRestartInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    USHORT_T outboundStreams,
    USHORT_T inboundStreams,
    UCHAR_T numOfRemoteIpAddrs,
    IPADDRESS_T * remoteIpAddrList_sp
){

  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpAssocRestartInd(
      assocId,
      ulpKey,
      outboundStreams,
      inboundStreams,
      numOfRemoteIpAddrs,
      remoteIpAddrList_sp
  );
}

USHORT_T  EINSS7_00SctpCommUpInd(
    ULONG_T sctpEndpointId,
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T origin,
    USHORT_T outboundStreams,
    USHORT_T inboundStreams,
    USHORT_T remotePort,
    UCHAR_T numOfRemoteIpAddrs,
    IPADDRESS_T * remoteIpAddrList_sp
){

  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpCommUpInd(
      sctpEndpointId,
      assocId,
      ulpKey,
      origin,
      outboundStreams,
      inboundStreams,
      remotePort,
      numOfRemoteIpAddrs,
      remoteIpAddrList_sp
  );
}

USHORT_T  EINSS7_00SctpCommLostInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T eventType,
    UCHAR_T origin
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpCommLostInd(
      assocId,
      ulpKey,
      eventType,
      origin
  );
}

USHORT_T  EINSS7_00SctpCommErrorInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T errorCode
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpCommErrorInd(
      assocId,
      ulpKey,
      errorCode
  );
}

USHORT_T  EINSS7_00SctpDataArriveInd(
    ULONG_T assocId,
    USHORT_T streamId,
    ULONG_T ulpKey,
    ULONG_T payloadProtId,
    BOOLEAN_T unorderFlag,
    USHORT_T streamSequenceNumber,
    UCHAR_T partialDeliveryFlag,
    ULONG_T dataLength,
    UCHAR_T * data_p
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpDataArriveInd(
      assocId,
      streamId,
      ulpKey,
      payloadProtId,
      unorderFlag,
      streamSequenceNumber,
      partialDeliveryFlag,
      dataLength,
      data_p
  );
}

USHORT_T  EINSS7_00SctpShutdownConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpShutdownConf(
      returnCode,
      assocId,
      ulpKey
  );
}

USHORT_T  EINSS7_00SctpCongestionInd(
    ULONG_T assocId,
    ULONG_T ulpKey
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpCongestionInd(
      assocId,
      ulpKey
  );
}

USHORT_T  EINSS7_00SctpCongestionCeaseInd(
    ULONG_T assocId,
    ULONG_T ulpKey
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpCongestionCeaseInd(
      assocId,
      ulpKey
  );
}

USHORT_T  EINSS7_00SctpSendFailureInd(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey,
    ULONG_T dataLength,
    UCHAR_T * data_p,
    ULONG_T payloadProtId,
    USHORT_T streamId,
    ULONG_T userSequence,
    IPADDRESS_T * remoteIpAddr_s,
    BOOLEAN_T unorderFlag
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpSendFailureInd(
      returnCode,
      assocId,
      ulpKey,
      dataLength,
      data_p,
      payloadProtId,
      streamId,
      userSequence,
      remoteIpAddr_s,
      unorderFlag
  );
}


USHORT_T  EINSS7_00SctpNetworkStatusChangeInd(
    ULONG_T assocId,
    ULONG_T ulpKey,
    UCHAR_T newStatus,
    IPADDRESS_T remoteIpAddr_s
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpNetworkStatusChangeInd(
      assocId,
      ulpKey,
      newStatus,
      remoteIpAddr_s
  );
}

USHORT_T  EINSS7_00SctpIndError(
    USHORT_T errorCode,
    MSG_T * msg_sp
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpIndError(
      errorCode,
      msg_sp
  );
}

USHORT_T  EINSS7_00SctpBindConf(
    EINSS7INSTANCE_T sctpInstanceId,
    UCHAR_T error
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpBindConf(
      sctpInstanceId,
      error
  );
}

USHORT_T  EINSS7_00SctpStatusConf(
    UCHAR_T returnCode,
    ULONG_T mappingKey,
    ULONG_T sctpEndpointId,
    ULONG_T numOfAssociations,
    ASSOC_T * assocStatusList_sp
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpStatusConf(
      returnCode,
      mappingKey,
      sctpEndpointId,
      numOfAssociations,
      assocStatusList_sp
  );
}

USHORT_T  EINSS7_00SctpTakeoverConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T sctpEndpointId
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpTakeoverConf(
      returnCode,
      assocId,
      sctpEndpointId
  );
}

USHORT_T  EINSS7_00SctpTakeoverInd(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T sctpEndpointId
){
  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpTakeoverConf(
      returnCode,
      assocId,
      sctpEndpointId
  );
}

USHORT_T  EINSS7_00SctpInitializeConf(
    UCHAR_T returnCode,
    ULONG_T sctpEndpointId,
    USHORT_T assignedMis,
    USHORT_T assignedOsServerMode,
    USHORT_T maxOs,
    ULONG_T pmtu,
    ULONG_T mappingKey,
    USHORT_T localPort
){

  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpInitializeConf(
      returnCode,
      sctpEndpointId,
      assignedMis,
      assignedOsServerMode,
      maxOs,
      pmtu,
      mappingKey,
      localPort
  );
}

USHORT_T  EINSS7_00SctpAssociateConf(
    UCHAR_T returnCode,
    ULONG_T assocId,
    ULONG_T ulpKey
){

  return IPL4asp__PortType::IPL4asp__PT_PROVIDER::port_ptr->SctpAssociateConf(
      returnCode,
      assocId,
      ulpKey
  );
}
#endif

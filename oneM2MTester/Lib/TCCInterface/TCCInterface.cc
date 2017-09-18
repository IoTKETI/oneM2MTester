/******************************************************************************
* Copyright (c) 2004, 2015  Ericsson AB
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html

******************************************************************************/
///////////////////////////////////////////////////////////////////////////////
//
//  File:               TCCInterface.cc
//  Description:        TCC Useful Functions: Interface Functions
//  Rev:                R22B
//  Prodnr:             CNL 113 472
//  Updated:            2012-10-18
//  Contact:            http://ttcn.ericsson.se
//
///////////////////////////////////////////////////////////////////////////////

#include "TCCInterface_Functions.hh"

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <memory.h>
#include <netdb.h>              // gethostbyname
#include <errno.h>
#if defined LINUX
 #include <ifaddrs.h>
 #include <linux/types.h>
 #include "TCCInterface_ip.h"
#endif

#if defined LKSCTP_1_0_7 || defined LKSCTP_1_0_9 || USE_SCTP
#include <netinet/sctp.h>
#endif

#if defined SOLARIS || defined SOLARIS8
# include <sys/sockio.h>
#endif

namespace TCCInterface__Functions {

TCCInterface__PortStatus f__getPortAvailabilityStatus(const CHARSTRING& ipAddress, const INTEGER& portNumber,
                       const TCCInterface__ProtocolType& protocolType){

  int addrfamily = 0;
  int addrtype = 0;
  int addrproto = 0;

  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
// Legacy system support just for fun. Because we like Noah
#ifdef AF_INET6
  struct sockaddr_in6 saddr6;
  memset(&saddr6, 0, sizeof(saddr6));
#endif
  
  // Always prepare to worst
  TCCInterface__PortStatus ret_val = TCCInterface__PortStatus::PORT__STATUS__PARAMETER__ERROR;
  // and have a beer

  // Ah, new acquisitions! You are a protocol droid, are you not?
  // I am C-3PO, human/cyborg...
  // Just determine the protocol
  switch(protocolType){
    case TCCInterface__ProtocolType::PROTO__TYPE__UDP:
      addrtype=SOCK_DGRAM;
      break;
    case TCCInterface__ProtocolType::PROTO__TYPE__TCP:
      addrtype=SOCK_STREAM;
      break;
#if defined LKSCTP_1_0_7 || defined LKSCTP_1_0_9 || USE_SCTP
    case TCCInterface__ProtocolType::PROTO__TYPE__SCTP:
      addrtype=SOCK_STREAM;
      addrproto=IPPROTO_SCTP;
      break;
#endif
    default:
      //  Ouch! Pay attention to what you're doing!
      TTCN_warning("The f_getPortAvailabilityStatus was called with unsupported protocol. Try to enable the sctp support.");
      return ret_val;
      break;
  }

  // What port?
  if(portNumber<=0 || portNumber>65535){
    // I'm terribly sorry about all this.
      TTCN_warning("The f_getPortAvailabilityStatus was called with invalid port number. %s",(const char*)int2str(portNumber));
      return ret_val;
    
  }

  // Which address?
  if(inet_pton(AF_INET, ipAddress, &(saddr.sin_addr))) {
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons( portNumber );
    addrfamily = AF_INET;
  }
#ifdef AF_INET6
  else if(inet_pton(AF_INET6, ipAddress, &(saddr6.sin6_addr))) {
    saddr6.sin6_family = AF_INET6;
    saddr6.sin6_port = htons( portNumber );
    addrfamily = AF_INET6;
  }
#endif
  else {
    // This is all your fault. 
      TTCN_warning("The f_getPortAvailabilityStatus was called with invalid address: %s",(const char*)ipAddress);
      return ret_val;
  }

  int sock=socket(addrfamily,addrtype,addrproto);
  if(sock==-1){
    // We've stopped. Wake up! Wake up! 
    TTCN_warning("f_getPortAvailabilityStatus socket call failed: %d %s",errno,strerror(errno));
    return TCCInterface__PortStatus::PORT__STATUS__INTERNAL__ERROR;
  }

  int b_res=-1;
  if(addrfamily == AF_INET){
    b_res=bind(sock,(struct sockaddr *)&saddr, sizeof(saddr));
  }
#ifdef AF_INET6
  else {
    b_res=bind(sock,(struct sockaddr *)&saddr6, sizeof(saddr6));
  }
#endif

  if(b_res==0){ // success. The port was free to use, but not more
    ret_val = TCCInterface__PortStatus::PORT__STATUS__WAS__VACANT;
  } else {
    if(errno==EADDRINUSE){  // Somebody else use the port already
      ret_val = TCCInterface__PortStatus::PORT__STATUS__WAS__OCCUPIED;
    } else {  
      // I have a bad feeling about this.
      // Wrong address/port combination.
      TTCN_warning("The f_getPortAvailabilityStatus called with invalid arguments. The system returned: %d %s",errno,strerror(errno));
      TTCN_warning("f_getPortAvailabilityStatus parameters: %s %s",(const char*)ipAddress,(const char*)int2str(portNumber));
    }
  }
  close(sock);
  // the status information is not valid any more. We can return whatever we want. Doesn't matter.
  return ret_val;
}




///////////////////////////////////////////////////////////////////////////////
//  Function: f__setIP
// 
//  Purpose:
//    Set IP address, subnet mask and broadcast address in a network inteface
//    If number is set, a range of virtual interfaces are set up with
//    continuous IP address (no subnet mask, broadcast checking)
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    ipaddress - *in* *charstring* - starting IP address
//    subnetmask - *in* *charstring* - subnetmask
//    broadcast - *in* *charstring* - broadcast
//    number - *in* *integer* - number of interfaces to set up
// 
//  Return Value:
//    -
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
void f__setIP(const CHARSTRING& interface, const CHARSTRING& ipaddress,
              const CHARSTRING& subnetmask, const CHARSTRING& broadcast,
              const INTEGER& number)
{
#if defined USE_IPROUTE && defined LINUX
  if (! f__setIP__ip(interface, ipaddress, 32, 1)){
    TTCN_warning("f_setIP_ip error!");
  }
#else

#if defined LINUX || defined SOLARIS || defined SOLARIS8
  /* Check the name of the interface */
  if (!interface.is_bound())
    TTCN_error("f_setIP(): Unbound argument `interface'.");
  int interface_len = interface.lengthof();
  if (interface_len == 0) TTCN_error("f_setIP(): Argument `interface' "
      "is an empty string.");
  else if (interface_len > IFNAMSIZ) TTCN_error("f_setIP(): Argument "
      "`interface' is too long (expected: at most %d, given: %d characters).",
      IFNAMSIZ, interface_len);
  const char *interface_str = interface;

  /* Check the IP address */
  if (!ipaddress.is_bound())
    TTCN_error("f_setIP(): Unbound argument `ipaddress'.");
  int ip_len = ipaddress.lengthof();
  if (ip_len < 7 || ip_len > 15) TTCN_error("f_setIP(): Invalid length of argument `ipaddress' "
      "(expected 7-15, given: %d characters).", ip_len);

  /* Check the subnet mask */
  if (!subnetmask.is_bound())
    TTCN_error("f_setIP(): Unbound argument `subnetmask'.");
  int subnetmask_len = subnetmask.lengthof();
  if (subnetmask_len < 7 || subnetmask_len > 15) TTCN_error("f_setIP(): Invalid length of argument "
    "`subnetmask' (expected 7-15, given: %d characters).", subnetmask_len);

  /* Check the broadcast address */
  if (!broadcast.is_bound())
    TTCN_error("f_setIP(): Unbound argument `broadcast'.");
  int broadcast_len = broadcast.lengthof();
  if (broadcast_len < 7 || broadcast_len > 15) TTCN_error("f_setIP(): Invalid length of argument "
    "`broadcast' (expected 7-15, given: %d characters).", broadcast_len);
    
  /* Check number parameter */
  int loop = 1;
  if (!number.is_bound())
    TTCN_error("f_setIP(): Unbound argument `number'.");
  else
    loop = (int)number;

  /* Get start position (of virtual interface) */
  int start = 0;
  if (strchr(interface_str, ':') != NULL) {
    start = atoi(strchr(interface_str,':')+1);
  }
  
  /* Convert addresses */
  unsigned char ip[4];
  unsigned char subnet[4];
  unsigned char broad[4];
  inet_aton(ipaddress, (struct in_addr *)&(ip[0]));
  inet_aton(subnetmask, (struct in_addr *)&(subnet[0]));
  inet_aton(broadcast, (struct in_addr *)&(broad[0]));
  
  /* Set address of interfaces */
  if(loop != 1) {
    /* Set up number of virtual interfaces */
    for(int i=start; i<(loop+start); i++) {      
      char *ifname;
      ifname = (char *)Malloc(20*sizeof(char *));
      
      if(strchr(interface_str, ':') != NULL) {    // if virtual, change the end
        strncpy(ifname, interface_str, strlen(interface_str)-strlen(strchr(interface_str,':'))+2);
        ifname[strlen(interface_str)-strlen(strchr(interface_str,':'))+1] = '\0';
        strcat(ifname,(const char *)int2str(i));
      }
      else {    // if real, append
        strcpy(ifname,interface_str);
        strcat(ifname,":");
        strcat(ifname,(const char *)int2str(i));
      }

      /* Set the IP, Netmask, Broadcast for the actual virtual interface */

      /* Open a socket */
      int sock = socket(PF_INET, SOCK_DGRAM, 0);
      if (sock < 0) TTCN_error("f_setIP(): Could not create socket.");

      struct ifreq ifr;
      struct sockaddr_in *addr = (struct sockaddr_in*)&ifr.ifr_addr;

      /* Set the IP address */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, ifname, strlen(ifname));
      addr->sin_family = AF_INET;
      memcpy(&addr->sin_addr, ip, 4);

      if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
        TTCN_warning("f_setIP(): Could not set IP address of interface `%s'.", interface_str);
      }

      /* Set the subnet mask */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, ifname, strlen(ifname));
      addr->sin_family = AF_INET;
      memcpy(&addr->sin_addr, subnet, 4);

      if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
        TTCN_warning("f_setIP(): Could not set subnetmask of interface `%s'.",
         interface_str);
      }

      /* Set the broadcast address */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, ifname, strlen(ifname));
      addr->sin_family = AF_INET;
      memcpy(&addr->sin_addr, broad, 4);

      if (ioctl(sock, SIOCSIFBRDADDR, &ifr) < 0) {
        TTCN_warning("f_setIP(): Could not set broadcast address of interface `%s'.", interface_str);
      }

      close(sock);
      Free(ifname);
      
      /* Step the IPaddress */
      if(ip[3]+1 > 254) {  // 255 for broadcast
        ip[3] = 1;         //   0 reserved
        ip[2] += 1;
        if(ip[2] > 254) {
          ip[2] = 0;
          ip[1] += 1;
          if(ip[1] > 254) {
            ip[1] = 0;
            ip[0] += 1;
            if(ip[0] > 254) {
              TTCN_error("f_setIP(): IP address range limit.");
            }
          }
        }
      }
      else {
        ip[3] += 1;
      }

    }//for end
  }//if loop end
  else {
    /* Open a socket */
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) TTCN_error("f_setIP(): Could not create socket.");

    struct ifreq ifr;
    struct sockaddr_in *addr = (struct sockaddr_in*)&ifr.ifr_addr;

    /* Set the IP address */
    memset(&ifr, 0, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, interface_str, interface_len);
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr, ip, 4);

    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
      TTCN_warning("f_setIP(): Could not set IP address of interface `%s'.",
       interface_str);
    }

    /* Set the subnet mask */
    memset(&ifr, 0, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, interface_str, interface_len);
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr, subnet, 4);

    if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
      TTCN_warning("f_setIP(): Could not set subnet mask of interface `%s'.",
       interface_str);
    }

    /* Set the broadcast address */
    memset(&ifr, 0, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, interface_str, interface_len);
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr, broad, 4);

    if (ioctl(sock, SIOCSIFBRDADDR, &ifr) < 0) {
      TTCN_warning("f_setIP(): Could not set broadcast address of interface `%s'.",
       interface_str);
    }

    close(sock);
  }
#else
  TTCN_error("f_setIP(): Setting the IP address is supported on Linux and Solaris only.");
#endif
#endif // USE_IPROUTE
}





///////////////////////////////////////////////////////////////////////////////
//  Function: f__deleteIP
// 
//  Purpose:
//    Delete IP address from a network inteface
//
//  Parameters:
//    interface - *in* *charstring* - network interface
// 
//  Return Value:
//    -
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
void f__deleteIP(const CHARSTRING& interface)
{
#if defined USE_IPROUTE && defined LINUX
  if (! f__setIP__ip(interface, "", 32, 0)){
    TTCN_warning("f_delIP_ip error!");
  }
#else

#if defined LINUX || defined SOLARIS || defined SOLARIS8
  /* Check the name of the interface */
  if (!interface.is_bound())
    TTCN_error("f_deleteIP(): Unbound argument `interface'.");
  int interface_len = interface.lengthof();
  if (interface_len == 0) TTCN_error("f_deleteIP(): Argument `interface' "
      "is an empty string.");
  else if (interface_len > IFNAMSIZ) TTCN_error("f_deleteIP(): Argument "
      "`interface' is too long (expected: at most %d, given: %d characters).",
      IFNAMSIZ, interface_len);
  const char *interface_str = interface;

  /* Open a socket */
  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) TTCN_error("f_deleteIP(): Could not create socket.");

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(struct ifreq));
  memcpy(ifr.ifr_name, interface_str, interface_len);

  if (strchr(interface_str, ':') != NULL) {
    /* The interface is a virtual interface (e.g. eth1:0) */
    /* set the status to down */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
      TTCN_warning("f_deleteIP(): Could not get the flags of interface `%s'.",
        interface_str);
    }
    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
      TTCN_warning("f_deleteIP(): Could not set the flags of interface `%s'.",
        interface_str);
    }
  }
  else {
    /* The interface is a real interface (e.g. eth2) */
    /* assign IP address 0.0.0.0 = delete */
    struct sockaddr_in *addr = (struct sockaddr_in*)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
      TTCN_warning("f_deleteIP(): Could not delete IP address on interface `%s'.",interface_str);
    }
  }

  close(sock);
#else
  TTCN_error("f_deleteIP(): Deleting the IP address is supported on Linux and Solaris only.");
#endif
#endif
}


///////////////////////////////////////////////////////////////////////////////
//  Function: f__getIP
// 
//  Purpose:
//    Get IP address, subnet mask and broadcast address from a network inteface
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    ip - *out* *charstring* - IP address
//    netmask - *out* *charstring* - netmask
//    broadcast - *out* *charstring* - broadcast
//    addressType - *in* <TCCInterface_IPAddressType> - type of IP addresses (default is IPv4)
// 
//  Return Value:
//    -
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////  
void f__getIP(const CHARSTRING& interface, CHARSTRING& ip,
                    CHARSTRING& netmask, CHARSTRING& broadcast, const TCCInterface__IPAddressType& addressType)
{
  ip ="";
  netmask = "";
  broadcast ="";
  /* Check the name of the interface */
  if (!interface.is_bound())
    TTCN_error("f_getIP(): Unbound argument `interface'.");
  int interface_len = interface.lengthof();
  if (interface_len == 0) TTCN_error("f_getIP(): Argument `interface' "
      "is an empty string.");
  else if (interface_len > IFNAMSIZ) TTCN_error("f_getIP(): Argument "
      "`interface' is too long (expected: at most %d, given: %d characters).",
      IFNAMSIZ, interface_len);
  const char *interface_str = interface;

  switch (addressType) {
    case TCCInterface__IPAddressType::IPv4: { //IPv4

      /* Open a socket */
      int sock = socket(PF_INET, SOCK_DGRAM, 0);
      if (sock < 0) TTCN_error("f_getIP(): Could not create socket.");

      struct ifreq ifr;
      struct sockaddr_in *addr = (struct sockaddr_in*)&ifr.ifr_addr;   

      /* Get IP address */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, interface_str, interface_len);
  
      if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        TTCN_warning("f_getIP(): Could not get address of interface '%s', %s.\n",
            interface_str,strerror(errno));
      }
      else {
        ip = inet_ntoa(addr->sin_addr);
      }

      /* Get subnet mask */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, interface_str, interface_len);

      if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
        TTCN_warning("f_getIP(): Could not get subnet mask of interface `%s', %s.",
         interface_str,strerror(errno));
      }
      else {
        netmask = inet_ntoa(addr->sin_addr);
      }

      /* Get the broadcast address */
      memset(&ifr, 0, sizeof(struct ifreq));
      memcpy(ifr.ifr_name, interface_str, interface_len);

      if (ioctl(sock, SIOCGIFBRDADDR, &ifr) < 0) {
        TTCN_warning("f_getIP(): Could not get broadcast address of interface `%s', %s.",
         interface_str,strerror(errno));
      }
      else {      
        broadcast = inet_ntoa(addr->sin_addr);
      }

      close(sock);
      break;  
    }
    case TCCInterface__IPAddressType::IPv6: { //IPv6 
#if defined LINUX
/*    // works only with linux!
      char devname[IFNAMSIZ];
      char addr6p[8][5];
      char addr6[40];
      int plen, scope, dad_status, if_idx;
      FILE *f;
 
      if ((f = fopen("/proc/net/if_inet6", "r")) != NULL) 
        {
          while (fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %20s\n",
                        addr6p[0], addr6p[1], addr6p[2], addr6p[3],
                        addr6p[4], addr6p[5], addr6p[6], addr6p[7],
                        &if_idx, &plen, &scope, &dad_status, devname) != EOF) 
          {
            if (!strcmp(devname, interface)) 
              {
                sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
                        addr6p[0], addr6p[1], addr6p[2], addr6p[3],
                        addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
		ip = addr6;
              }
          }
        fclose(f);
      }
*/          
      char ipaddr[INET6_ADDRSTRLEN];
      char subnet[INET6_ADDRSTRLEN];	
      struct ifaddrs *ifap,*ifa;
      if((getifaddrs(&ifap)) <0){
        TTCN_warning("f_getIP(): Could not get address of interface '%s', %s.\n",
            interface_str, strerror(errno));		    
        break;
      }
      for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == interface && ifa->ifa_addr != NULL)
        {       
          if (ifa->ifa_addr->sa_family == AF_INET6)
          { 	    
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_addr;    
            inet_ntop(AF_INET6,&(sa->sin6_addr),ipaddr,INET6_ADDRSTRLEN);	    
	    ip = ipaddr;	    	    
            struct sockaddr_in6 *saMask = (struct sockaddr_in6 *)ifa->ifa_netmask;     
            inet_ntop(AF_INET6,&(saMask->sin6_addr),subnet,INET6_ADDRSTRLEN);	    
	    netmask = subnet;    	    	
	    broadcast = ""; //broadcast address not supported in IPv6
            freeifaddrs(ifap);
            break;
	  }
        }
      }

#else
      TTCN_error("f_getIP(): Getting the IPv6 address is supported on Linux only.");
#endif
      break;
    }    
    default: {
      TTCN_error("f_getIP(): Not supported address type.");      
      break;
    }
  } //switch       
}


///////////////////////////////////////////////////////////////////////////////
//  Function: f__setInterfaceUp
// 
//  Purpose:
//    Set up a network interface
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    addressType - *in* <TCCInterface_IPAddressType> - type of IP addresses (default is IPv4)
// 
//  Return Value:
//    -
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
void f__setInterfaceUp(const CHARSTRING& interface, const TCCInterface__IPAddressType& addressType)
{
#if defined LINUX || defined SOLARIS || defined SOLARIS8
  /* Check the name of the interface */
  if (!interface.is_bound())
    TTCN_error("f_setInterfaceUp(): Unbound argument `interface'.");
  int interface_len = interface.lengthof();
  if (interface_len == 0) TTCN_error("f_setInterfaceUp(): Argument `interface' "
      "is an empty string.");
  else if (interface_len > IFNAMSIZ) TTCN_error("f_setInterfaceUp(): Argument "
      "`interface' is too long (expected: at most %d, given: %d characters).",
      IFNAMSIZ, interface_len);
  const char *interface_str = interface;

  int sock;
  struct ifreq ifr;
  int sa_family;
  switch (addressType) {
    case TCCInterface__IPAddressType::IPv4: { //IPv4
      sa_family = AF_INET;
      break;  
    }
    case TCCInterface__IPAddressType::IPv6: { //IPv6
      sa_family = AF_INET6;
      break;
    }    
    default: {
      TTCN_error("f_setInterfaceDown(): Not supported address type.");      
      break;
    }
  } //switch 

  if ((sock = socket(sa_family, SOCK_DGRAM, IPPROTO_IP)) < 0) {
    TTCN_error("f_setInterfaceUp(): Cannot open socket.");
  }

  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);

  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
    TTCN_warning("f_setInterfaceUp(): Cannot get flags of interface '%s', %s.", interface_str, strerror(errno));
  }
  ifr.ifr_flags |= IFF_UP;
  if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
    TTCN_warning("f_setInterfaceUp(): Cannot set up interface '%s', %s.", interface_str, strerror(errno));
  }

  close(sock);
#else
  TTCN_error("f_setInterfaceUp(): Setting up the interface is supported on Linux and Solaris only.");
#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Function: f__setInterfaceDown
// 
//  Purpose:
//    Set down a network interface
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    addressType - *in* <TCCInterface_IPAddressType> - type of IP addresses (default is IPv4)
// 
//  Return Value:
//    -
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
void f__setInterfaceDown(const CHARSTRING& interface, const TCCInterface__IPAddressType& addressType)
{
#if defined LINUX || defined SOLARIS || defined SOLARIS8
  /* Check the name of the interface */
  if (!interface.is_bound())
    TTCN_error("f_setInterfaceDown(): Unbound argument `interface'.");
  int interface_len = interface.lengthof();
  if (interface_len == 0) TTCN_error("f_setInterfaceDown(): Argument `interface' "
      "is an empty string.");
  else if (interface_len > IFNAMSIZ) TTCN_error("f_setInterfaceDown(): Argument "
      "`interface' is too long (expected: at most %d, given: %d characters).",
      IFNAMSIZ, interface_len);
  const char *interface_str = interface;

  int sock;
  struct ifreq ifr;
 
  int sa_family;
  switch (addressType) {
    case TCCInterface__IPAddressType::IPv4: { //IPv4
      sa_family = AF_INET;
      break;  
    }
    case TCCInterface__IPAddressType::IPv6: { //IPv6
      sa_family = AF_INET6;
      break;
    }    
    default: {
      TTCN_error("f_setInterfaceDown(): Not supported address type.");      
      break;
    }
  } //switch 

  if ((sock = socket(sa_family, SOCK_DGRAM, IPPROTO_IP)) < 0) {
    TTCN_error("f_setInterfaceDown(): Cannot open socket.");
  }

  memset(&ifr, 0, sizeof(struct ifreq));
  strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name)-1);

  if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
    TTCN_warning("f_setInterfaceDown(): Cannot get flags of interface '%s', %s.", interface_str, strerror(errno));
  }
  ifr.ifr_flags &= ~IFF_UP;
  if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
    TTCN_warning("f_setInterfaceDown(): Cannot set down interface '%s', %s.", interface_str, strerror(errno));
  }

  close(sock);
#else
  TTCN_error("f_setInterfaceDown(): Setting down the interface is supported on Linux and Solaris only.");
#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Function: f__getHostName
// 
//  Purpose:
//    Get name of host
//
//  Parameters:
//    -
// 
//  Return Value:
//    charstring - name of the host
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
CHARSTRING f__getHostName()
{
  char *ret_val= new char[255];
  if (!gethostname(ret_val, 255))
  {return ret_val;}
  else
  {return "";};

}

///////////////////////////////////////////////////////////////////////////////
//  Function: f__getIpAddr
// 
//  Purpose:
//    Get IP address of host
//
//  Parameters:
//    hostname - *in* *charstring* - name of the host
//    addressType - *in* <TCCInterface_IPAddressType> - type of IP addresses (default is IPv4)
// 
//  Return Value:
//    charstring - IP address of the host
//
//  Errors:
//    - 
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
CHARSTRING f__getIpAddr(const CHARSTRING& hostname, const TCCInterface__IPAddressType& addressType)
{ 
#if defined LINUX || defined SOLARIS || defined SOLARIS8
  int err = 0;
  struct addrinfo myaddr, *res;
  memset(&myaddr,0,sizeof(myaddr));
  myaddr.ai_flags = AI_ADDRCONFIG|AI_PASSIVE|AI_CANONNAME;
  myaddr.ai_socktype = SOCK_STREAM;
  myaddr.ai_protocol = 0;   
    
  switch (addressType) {
    case TCCInterface__IPAddressType::IPv4: { //IPv4
      myaddr.ai_family = AF_INET;    
      char dst[INET_ADDRSTRLEN];  
      
      if ((err = getaddrinfo(hostname, NULL, &myaddr, &res)) != 0) {//&myaddr
        TTCN_Logger::log ( TTCN_DEBUG, "f_getIpAddr(): getaddrinfo: %i, %s", err, gai_strerror(err) );              
        return "";
      }
      else {
        struct sockaddr_in *saddr = (struct sockaddr_in *) res->ai_addr;      
	
        inet_ntop(AF_INET,&(saddr->sin_addr),dst,INET_ADDRSTRLEN);	
 	freeaddrinfo(res);
        return dst;
      }    
      break;  
    }
    case TCCInterface__IPAddressType::IPv6: { //IPv6
   
      char dst[INET6_ADDRSTRLEN];    
      myaddr.ai_family = AF_INET6;
          
      if ((err = getaddrinfo(hostname, NULL, &myaddr, &res)) != 0) {        
	TTCN_Logger::log ( TTCN_DEBUG, "f_getIpAddr(): getaddrinfo: %i, %s", err, gai_strerror(err) );        
        return "";
      }
      else { 
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *) res->ai_addr;     
        inet_ntop(AF_INET6,&(saddr->sin6_addr),dst,INET6_ADDRSTRLEN);	
	freeaddrinfo(res);	
      }
      return dst;                
      break;
    }      
    default: {
      TTCN_error("f_getIpAddr(): Not supported address type.");
      return "";        
      break;
    }
  } //switch    
#else  //Cygwin
  switch (addressType) {
    case TCCInterface__IPAddressType::IPv4: { //IPv4
      struct hostent *host;  
      struct in_addr address;
      host = gethostbyname (hostname);  //getaddrinfo not supported
      if (host == NULL) return "";
      else
      {
        memcpy (&address, host->h_addr_list[0], 4);
        return inet_ntoa(address);
      }    
      break;  
    }
    case TCCInterface__IPAddressType::IPv6: { //IPv6
      TTCN_error("f_getIpAddr(): Getting IPv6 addresses is supported on Linux and Solaris only.");
      return "";
      break;      
    }
    default: {
      TTCN_error("f_getIpAddr(): Not supported address type.");
      return "";        
      break;
    }
  } //switch
#endif  
  
}

///////////////////////////////////////////////////////////////////////////////
//  Function: f_setIP_ip
// 
//  Purpose:
//    Set IP address, subnet mask in a network inteface
//    Uses RTLN netlink interface on linux, which is faster than original POSIX.    
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    ipaddress - *in* *charstring* - starting IP address
//    prefix - *in* *integer* - subnetmask of the ipaddress
// 
//  Return Value:
//    True on success, false in other cases.
//
//  Errors:
//    Many possibilities, all generates a TTCN_warning
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
BOOLEAN f__setIP__ip(const CHARSTRING& interface, const CHARSTRING& ipaddress, const INTEGER& prefix = 32, const INTEGER& v_set = 1){

#ifdef LINUX
  
  if ((int)prefix > 32 or (int)prefix < 0) {
    TTCN_warning("Wrong prefix number");
    return false;
  }

  char buf[3];
  sprintf(buf, "%d", (int)prefix);
  char *ip = (char *)malloc((ipaddress.lengthof() + strlen(buf) + 1 + 1)*sizeof(char));
  strcpy(ip, (char*)(const char*)ipaddress);
  strcat(ip,"/");
  strcat(ip, buf);

  if (rtnl_open(&rth, 0) < 0 ){
    TTCN_warning("RTNL: Can not open RTLN link");
    return false;
  }

  int cmd = (v_set == 1 ? RTM_NEWADDR : RTM_DELADDR);
  int flags = NLM_F_CREATE|NLM_F_EXCL;

  struct {
    struct nlmsghdr 	n;
    struct ifaddrmsg 	ifa;
    char   			buf[256];
  } req;

  char  *d = NULL;
  char  *lcl_arg = NULL;
  inet_prefix lcl;
  //int local_len = 0;
  
  memset(&req, 0, sizeof(req));
  
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | flags;
  req.n.nlmsg_type = cmd;
  req.ifa.ifa_family = preferred_family;
  
  d = (char*)(const char*)interface;

  // IP_ADDRESS handling
  lcl_arg = ip;    
  if (get_prefix(&lcl, lcl_arg, req.ifa.ifa_family) == -1) return 0;
  if (req.ifa.ifa_family == AF_UNSPEC)
    req.ifa.ifa_family = lcl.family;
  addattr_l(&req.n, sizeof(req), IFA_LOCAL, &lcl.data, lcl.bytelen);
  //local_len = lcl.bytelen;

  // further processing
  if (cmd == RTM_DELADDR && lcl.family == AF_INET && !(lcl.flags & PREFIXLEN_SPECIFIED)) {
    TTCN_warning("You should never see this!");
  } else {
    //peer = lcl;
    addattr_l(&req.n, sizeof(req), IFA_ADDRESS, &lcl.data, lcl.bytelen);
  }


  if (req.ifa.ifa_prefixlen == 0)
    req.ifa.ifa_prefixlen = lcl.bitlen;
  if(cmd != RTM_DELADDR)
    req.ifa.ifa_scope = default_scope(&lcl);
  
  //  ll_init_map(&rth);
  
  if ((req.ifa.ifa_index = ll_name_to_index(d)) == 0) {
    TTCN_warning("RTNL: Cannot find device");
    goto error;
  }
  
  if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
    TTCN_warning("RTNL: talk error!");
    goto error;
  }
  
  free(ip);
  rtnl_close(&rth);
  return true;
  
  error:
    free(ip);
  return false;

#else

  TTCN_warning("f_setIP_ip is only supported on Linux!");
  return false;

#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Function: f_delIP_ip
// 
//  Purpose:
//    Set IP address, subnet mask in a network inteface
//    Uses RTLN netlink interface on linux, which is faster than original POSIX.    
//
//  Parameters:
//    interface - *in* *charstring* - network interface
//    ipaddress - *in* *charstring* - starting IP address
//    prefix - *in* *integer* - subnetmask of the ipaddress
// 
//  Return Value:
//    True on success, false in other cases.
//
//  Errors:
//    Many possibilities, all generates a TTCN_warning
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
BOOLEAN f__delIP__ip(const CHARSTRING& interface, const CHARSTRING& ipaddress, const INTEGER& prefix = 32){
#ifdef LINUX

  return f__setIP__ip(interface, ipaddress, prefix, 0);

#else

  TTCN_warning("f_deIP_ip is only supported on Linux!");
  return false;

#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Function: f_getIpAddresses
// 
//  Purpose:
//    Get both IPv4 and IPv6 addresses of the given host
//
//  Parameters:
//    hostname - *charstring* - the hostname
// 
//  Return Value:
//    Initialized IPAddress structure
//
//  Errors:
//    Many possibilities, all generates a TTCN_warning
// 
//  Detailed description:
//    -
// 
///////////////////////////////////////////////////////////////////////////////
IPAddresses f__getIpAddresses(const CHARSTRING& hostname){
#if defined LINUX || defined SOLARIS || defined SOLARIS8
  struct addrinfo hints, *res, *p;
  int status;
  char ipstr[INET6_ADDRSTRLEN];
  IPAddresses iplist;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version

  CharstringList ipv4list(NULL_VALUE);
  CharstringList ipv6list(NULL_VALUE);

  if ((status = getaddrinfo((const char*)hostname, NULL, &hints, &res)) != 0) {
    TTCN_warning("f_getIpAddresses: getaddrinfo: %s for \"%s\"\n", gai_strerror(status), (const char*)hostname);
    return IPAddresses(ipv4list, ipv6list);
  }

  for(p = res;p != NULL; p = p->ai_next) {
    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof ipstr);
      ipv4list[ipv4list.size_of()] = ipstr;
    } else if (p->ai_family == AF_INET6){ //IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      inet_ntop(p->ai_family, &(ipv6->sin6_addr), ipstr, sizeof ipstr);
      ipv6list[ipv6list.size_of()] = ipstr;
    }
  }
  freeaddrinfo(res); // free the linked list  
  return IPAddresses(ipv4list, ipv6list);
#else
  CharstringList ipv4list(NULL_VALUE);
  CharstringList ipv6list(NULL_VALUE);

  struct hostent *host;  
  struct in_addr address;
  host = gethostbyname(hostname);  //getaddrinfo not supported
  if (host)
    while (*host->h_addr_list){
      memcpy (&address, host->h_addr_list++, host->h_length);
      ipv4list[ipv4list.size_of()] = inet_ntoa(address);
    }
  return IPAddresses(ipv4list, ipv6list);
#endif
}

}//namespace

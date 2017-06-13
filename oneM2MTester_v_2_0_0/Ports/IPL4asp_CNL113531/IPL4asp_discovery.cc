///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Copyright Test Competence Center (TCC) ETH 2008                           //
//                                                                           //
// The copyright to the computer  program(s) herein  is the property of TCC. //
// The program(s) may be used and/or copied only with the written permission //
// of TCC or in accordance with  the terms and conditions  stipulated in the //
// agreement/contract under which the program(s) has been supplied.          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//
//  File:               IPL4asp_discovery.cc
//  Rev:                R25A
//  Prodnr:             CNL 113 531
//  Updated:            2010-10-04
//  Contact:            http://ttcn.ericsson.se
//  Reference:          
///////////////////////////////////////////////


#include <TTCN3.hh>
#include "IPL4asp_Functions.hh"
#include "IPL4asp_PT.hh"
#include "IPL4asp_PortType.hh"
#include "Socket_API_Definitions.hh"

#ifndef LINUX
#undef IP_AUTOCONFIG // Currently only Linux is supported
#endif
#ifdef IP_AUTOCONFIG

#include <sys/ioctl.h>
#include <net/if.h> 
#include <net/if_arp.h> 

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <iostream>
#include <fstream>

#include <pcap.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "IPL4asp_protocol_L234.hh"


namespace IPL4asp__PortType {

const int SNAPLEN = 2048;

static bool debugLogEnabled = false;

void debug ( const char * fmt, ... )
 __attribute__ ((__format__ (__printf__, 1, 2)));

void debug ( const char * fmt, ... )
{
  if ( ! debugLogEnabled ) return;
  TTCN_Logger::begin_event(TTCN_DEBUG);
  TTCN_Logger::log_event("IPL4: ");
  va_list args;
  va_start(args, fmt);
  TTCN_Logger::log_event_va_list(fmt, args);
  va_end(args);
  TTCN_Logger::end_event();
}

void debug_dump ( const unsigned char * data, unsigned int len )
{
  if ( debugLogEnabled ) {
    TTCN_Logger::begin_event( TTCN_DEBUG );
    TTCN_Logger::log_event( "Dump of %i bytes:\n", len );
    if ( data == 0 ) {
      TTCN_Logger::log_event( "Null pointer" );
    } else {
      TTCN_Logger::log_event(" %03X ", 0);
      for ( unsigned int i=0; i < len; i++ ) {
        TTCN_Logger::log_event( " %02X", data[i] );
        if ( i % 16 == 15 ) {
          TTCN_Logger::log_char( '\n' );
          TTCN_Logger::log_event( " %03X ", i + 1 );
        }
      }
    }
    TTCN_Logger::end_event();
  }
}


inline const timeval & operator -= ( timeval & t, const timeval & u ) {
  t.tv_usec -= u.tv_usec;
  t.tv_sec -= u.tv_sec;
  if ( (int) t.tv_usec < 0 ) {
    t.tv_usec += 1000000;
    --t.tv_sec;
  }
  if ( t.tv_sec < 0 ) {
    debug ( "timeval& -= timeval& (): Negative difference" );
  }
  return t;
}

inline timeval operator - ( const timeval & t, const timeval & u ) {
  timeval dt = t;
  dt -= u;
  return dt;
}

inline bool operator < ( const timeval & t, const timeval & u ) {
  return ( t.tv_sec != u.tv_sec ) ? ( t.tv_sec < u.tv_sec ) :
                                    ( t.tv_usec < u.tv_usec );
}

inline bool operator <= ( const timeval & t, const timeval & u ) {
  return ! ( u < t );
}

inline const timeval & operator += ( timeval & t, unsigned int tv_sec ) {
  t.tv_sec += tv_sec;
  return t;
}

inline timeval operator + ( const timeval & t, unsigned int tv_sec ) {
  timeval u = t;
  u += tv_sec;
  return u;
}


const char * IPAddr::asStr () const
{
  static char           buffer[8][64];// TODO: remove static
  static unsigned int   bI = 0;

  char *  cBuf = buffer[bI];
  bI = ( bI + 1 ) % 8;
  if ( len == 4 )
    sprintf ( cBuf, "%i.%i.%i.%i",
            bytes[0], bytes[1], bytes[2], bytes[3] );
  else
    sprintf ( cBuf,
            "%02X%02X:%02X%02X:%02X%02X:%02X%02X:"
            "%02X%02X:%02X%02X:%02X%02X:%02X%02X",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15] );
  return cBuf;
}

const char * ipv4ToStr ( unsigned int ipv4Addr )
{
  static char           buffer[8][32];// TODO: remove static
  static unsigned int   bI = 0;
 
  char *  cBuf = buffer[bI];
  bI = ( bI + 1 ) % 8;
  unsigned char * bytes = (unsigned char*) & ipv4Addr;
  sprintf ( cBuf, "%i.%i.%i.%i",
          bytes[0], bytes[1], bytes[2], bytes[3] );
  return cBuf;
}


const char * EtherAddr::asStr () const
{
  static char           buffer[8][32];// TODO: remove static
  static unsigned int   bI = 0;

  char *  cBuf = buffer[bI];
  bI = ( bI + 1 ) % 8;
  sprintf ( cBuf, "%02X-%02X-%02X-%02X-%02X-%02X",
           bytes[0], bytes[1], bytes[2],
           bytes[3], bytes[4], bytes[5] );
  return cBuf;
}

inline bool strToEtherAddr ( const char * src, EtherAddr & dst ) {
  unsigned int addr[6];
  if ( sscanf ( src, "%2X-%2X-%2X-%2X-%2X-%2X",
     addr, addr + 1, addr + 2, addr + 3, addr + 4, addr + 5 ) != 6 )
    return false;
  for ( int i = 0 ; i < 6; ++i ) {
    if ( addr[i] >= 256 )
      return false;
  }
  for ( int i = 0 ; i < 6; ++i )
    dst.bytes[i] = (unsigned char) addr[i];
  return true;
}


// Application level headers and messages

// DHCP base header message (without options)
struct DHCPHeader
{
  unsigned char   op;
  unsigned char   htype;
  unsigned char   hlen;
  unsigned char   hops;
  unsigned int    xid;
  unsigned short  secs;
  unsigned short  flags;
  unsigned int    ciaddr;
  unsigned int    yiaddr;
  unsigned int    siaddr;
  unsigned int    giaddr;
  unsigned char   chaddr[16];
  unsigned char   sname[64];
  unsigned char   file[128];
} __attribute__ ((__packed__));

struct DHCPBase
{
  EtherHdr        etherHdr;
  IPv4Hdr         ipv4Hdr;
  UDPHdr          udpHdr;
  DHCPHeader      dhcp;
  inline void setDef ( unsigned int trId, const EtherAddr & etherAddr ) {
    etherHdr.setIP ( etherAddr );
    ipv4Hdr.setUDP ( 0, 0xFFFFFFFF );
    udpHdr.set ( 68, 67 );
    memset ( &dhcp, 0, sizeof ( dhcp ) );
    dhcp.op = 1; dhcp.htype = 1; dhcp.hlen = 6; dhcp.hops = 0;
    dhcp.xid = trId;
    ( (unsigned char*) &dhcp.flags )[0] = 0x80; // BROADCAST flag set to 1
    etherAddr.copyTo ( dhcp.chaddr );
  }
  inline unsigned int setLast ( unsigned int optLength ) {
    ipv4Hdr.setLength ( 20 + 8 + sizeof ( DHCPHeader ) + optLength );
    ipv4Hdr.setCheckSum ( IPv4Hdr::calcCheckSum ( ipv4Hdr.raw () ) );
    udpHdr.setLength ( 8 + sizeof ( DHCPHeader ) + optLength );
    udpHdr.setCheckSum ( UDPHdr::calcCheckSum ( udpHdr.raw (), ipv4Hdr ) );
    return 14 + 20 + 8 + sizeof ( DHCPHeader ) + optLength;
  }
  inline bool check ( unsigned int len ) {
    if ( len < 278 ) return false;
    if ( etherHdr.getType () != EtherHdr::IP ) return false;
    if ( IPv4Hdr::check ( ipv4Hdr.raw (), len - 14 ) == 0 ) return false;
    if ( ipv4Hdr.protocol != IPv4Hdr::UDP || ipv4Hdr.hdrLen () > 20 ) return false;
    if ( ipv4Hdr.dstAddr != 0xFFFFFFFF ) return false;
    if ( ipv4Hdr.isMFSet () ) return false;
    if ( UDPHdr::check ( udpHdr.raw () , ipv4Hdr ) == 0 ) return false;
    if ( ntohs ( udpHdr.dstPort ) != 68 || ntohs ( udpHdr.srcPort ) != 67 )
      return false;
    return dhcp.op == 2;
  }
} __attribute__ ((__packed__));

const unsigned char DHCP_MAGIC_COOKIE[] = { 0x63, 0x82, 0x53, 0x63 };

struct DHCPRespOpt
{
  unsigned int    optFlags;
  enum { MSG_TYPE = 1, IPV4_MASK = 2, IPV4_ROUTER = 4, LEASE_TIME = 8,
         DHCP_SERVER = 16, VALID = 0x80000000 };
  inline bool isValid ( unsigned int addOptFlags ) {
    unsigned int f = VALID | MSG_TYPE | DHCP_SERVER | addOptFlags;
    return ( optFlags & f ) == f;
  }
  //
  unsigned int    messageType;// 53
  unsigned int    ipv4Mask;   //  1
  unsigned int    ipv4router; //  3  - only the first
  unsigned int    leaseTime;  // 51
  unsigned int    dhcpServer; // 54
  inline unsigned int storeOptPar
    ( const unsigned char * & data, unsigned int & length ) {
    switch ( * ( data++ ) ) {
      case 0:
        return 0;
      case 1:
        if ( length < 5 || data[0] != 4 )
          return 2; // invalid
        ipv4Mask = * (unsigned int*) ( data + 1 );
        optFlags |= IPV4_MASK;
        break;
       case 3:
        if ( length < 5 || data[0] < 4 || length < data[0] + 1U )
          return 2; // invalid
        ipv4router = * (unsigned int*) ( data + 1 );
        optFlags |= IPV4_ROUTER;
        break;
      case 51:
        if ( length < 5 || data[0] != 4 )
          return 2; // invalid
        leaseTime = ntohl ( * (unsigned int*) ( data + 1 ) );
        optFlags |= LEASE_TIME;
        break;
      case 53:
        if ( length < 2 || data[0] != 1 )
          return 2; // invalid
        messageType = data[1];
        optFlags |= MSG_TYPE;
        break;
      case 54:
        if ( length < 5 || data[0] < 4 || length < data[0] + 1U )
          return 2; // invalid
        dhcpServer = * (unsigned int*) ( data + 1 );
        optFlags |= DHCP_SERVER;
        break;
      case 255:
        return 1; // end of options
      }
      data += 1 + data[0];
      return 0;
    }
  DHCPRespOpt ( const unsigned char * data, unsigned int length ) {
    optFlags = 0;
    if ( length < 4 || memcmp ( data, DHCP_MAGIC_COOKIE, 4 ) != 0 ) {
      debug ( "DHCPRespOpt(): Magic cookie missing" );
      return; // Not valid
    }
    data += 4; length -= 4;
    while ( length > 0 ) {
      unsigned int res = storeOptPar ( data, length );
      if ( res == 1 ) {
        optFlags |= VALID;
        return;
      }
      if ( res == 2 ) {
        debug ( "DHCPRespOpt(): Bad option format" );
        return;
      }
    }
    debug ( "DHCPRespOpt(): Unexpected end" );
  }
private:
  DHCPRespOpt ();
};

/* Subset of the DHCP states */
class DHCPState {
public:
  enum {
    INIT = 0,
    REBOOTING,
    SELECTING,
    REQUESTING,
    BOUND,
    REBINDING
  };
};

unsigned int CreateDHCPDiscover ( unsigned char * packet,
  unsigned int trId, const EtherAddr & etherAddr )
{
  DHCPBase *  dhcp = (DHCPBase*) packet;
  dhcp->setDef ( trId, etherAddr );
  dhcp->ipv4Hdr.ttl = 16;
  static const unsigned char options_p1[] = {
    /* Message type: DHCP Discover */   0x35, 1, 1,
    /* Parameter request           */   0x37, 4, 1, 3, 51, 54,
    /* Client Id                   */   0x3D, 7, 1
  };
  unsigned char * p = (unsigned char*) ( dhcp + 1 );
  memcpy ( p, DHCP_MAGIC_COOKIE, 4 ); p += 4;
  memcpy ( p, options_p1, sizeof ( options_p1 ) );
  p += sizeof ( options_p1 );
  p += etherAddr.copyTo ( p );
  p[0] = 0xFF; p[1] = 0;
  return dhcp->setLast ( 4 + sizeof ( options_p1 ) + 6 + 2 );
}

unsigned int CreateDHCPRequest ( unsigned char * packet,
  unsigned int trId, const EtherAddr & etherAddr,
  unsigned int reqAddr, unsigned int srvAddr,
  unsigned int leaseTime, unsigned int state = DHCPState::SELECTING )
{
  DHCPBase *  dhcp = (DHCPBase*) packet;
  dhcp->setDef ( trId, etherAddr );
  dhcp->ipv4Hdr.ttl = 16;
  if ( state == DHCPState::REBINDING )
    dhcp->dhcp.ciaddr = reqAddr;
  static const unsigned char options_p1[] = {
    /* Message type: DHCP Request  */   0x35, 1, 3,
    /* Parameter request           */   0x37, 4, 1, 3, 51, 54,
    /* Client Id                   */   0x3D, 7, 1
  };
  unsigned char * p = (unsigned char*) ( dhcp + 1 );
  memcpy ( p, DHCP_MAGIC_COOKIE, 4 );
  unsigned int    i = 4;
  memcpy ( p + i, options_p1, sizeof ( options_p1 ) );
  i += sizeof ( options_p1 );
  i += etherAddr.copyTo ( p + i );
  if ( state == DHCPState::SELECTING || state == DHCPState::REBOOTING ) {
    /* 50: Requested Address */
    p[i++] = 0x32; p[i++] = 4;
    * ( (unsigned int*) ( p + i ) ) = reqAddr; i += 4;
  }
  if ( state == DHCPState::SELECTING ) {
    /* 54: DHCP Server */
    p[i++] = 0x36; p[i++] = 4;
    * ( (unsigned int*) ( p + i ) ) = srvAddr; i += 4;
  }
  /* 51: Lease time */
  p[i++] = 0x33; p[i++] = 4;
  * ( (unsigned int*) ( p + i ) ) = htonl ( leaseTime ); i += 4;
  p[i++] = 0xFF; p[i++] = 0;
  return dhcp->setLast ( i );
}

/*unsigned int CreateDHCPRelease ( unsigned char * packet,
  unsigned int trId, const EtherAddr & etherAddr,
  unsigned int ownAddr, unsigned int srvAddr )
{
  DHCPBase *  dhcp = (DHCPBase*) packet;
  dhcp->setDef ( trId, etherAddr );
  dhcp->dhcp.flags = 0;
  dhcp->dhcp.ciaddr = ownAddr;
  dhcp->ipv4Hdr.ttl = 16;
  dhcp->ipv4Hdr.srcAddr = ownAddr;
  dhcp->ipv4Hdr.dstAddr = srvAddr;
  static const unsigned char options_p1[] = {
    / * Message type: DHCP Release  * /   0x35, 1, 7,
    / * Client Id                   * /   0x3D, 7, 1
  };
  unsigned char * p = (unsigned char*) ( dhcp + 1 );
  memcpy ( p, DHCP_MAGIC_COOKIE, 4 ); p += 4;
  memcpy ( p, options_p1, sizeof ( options_p1 ) );
  p += sizeof ( options_p1 );
  p += etherAddr.copyTo ( p );
  / * 54: DHCP Server * /
  *(p++) = 0x36; *(p++) = 4;  * ( (unsigned int*) p ) = srvAddr; p += 4;
  p[0] = 0xFF; p[1] = 0;
  return dhcp->setLast ( 4 + sizeof ( options_p1 ) + 6 + 6 + 2 );
}*/


// Structure and functions to retreive Ethernet interface information
struct EtherIf
{
  char name[IF_NAMESIZE];
  unsigned int    index;
  EtherAddr       hwAddr;
  unsigned short  origFlags;
  IPAddr          realIpAddr;
  IPAddr          netMask;
  IPAddr          bCAddr;
  unsigned int    ipLength;
  unsigned int    netLength;
  bool            valid;
  bool            ipv6;
  char            pad[6];
  
  EtherIf() { memset ( this, 0, sizeof ( *this ) ); }
  bool find ( const char   * ifName,
              const char   * ifExpIpAddress,
              const char   * ifExclIpAddress );
  EtherIf ( const char   * ifName,
            const char   * ifExpIpAddress,
            const char   * ifExclIpAddress ) {
    memset ( this, 0, sizeof ( *this ) );
    find ( ifName, ifExpIpAddress, ifExclIpAddress );
  }
};

bool EtherIf::find ( const char   * ifName,
                     const char   * ifExpIpAddress,
                     const char   * ifExclIpAddress )
{
  IPAddr        ifExpIp;
  IPAddr        ifExclIp;
  
  if ( ifExpIpAddress != 0 ) {
    ifExpIp = IPAddr ( ifExpIpAddress ); // TODO: IPv6
    debug ( "EtherIf::find(): requested ifIp: %s", ifExpIp.asStr () );
  } else if ( ifExclIpAddress != 0 ) {
    ifExclIp = IPAddr ( ifExclIpAddress ); // TODO: IPv6
    debug ( "EtherIf::find(): requested ifExclIp: %s", ifExclIp.asStr () );
  }
  int soc = socket( PF_INET, SOCK_DGRAM,0 );
  if ( soc < 0 ) {
    TTCN_warning ( "EtherIf::find(): Cannot open socket" );
    return false;
  }
  for ( unsigned int i = 1; ; ++i ) {
    struct {
      char name[IF_NAMESIZE];
      unsigned char data[48];
    } ifr;
    // IF name
    char   ifn[IF_NAMESIZE];
    memset ( ifn, 0, sizeof ( ifn ) );
    if ( if_indextoname ( i, ifn ) == 0 )
      break;
    debug ( "EtherIf::find(): --- %i: \"%s\" ---", i, ifn );
    if ( ifName != 0 && strcmp ( ifn, ifName ) != 0 )
      continue;
    // IF flags
    memcpy ( ifr.name, ifn, IF_NAMESIZE);
    memset ( ifr.data, 0, sizeof ( ifr.data ) );
    int res = ioctl ( soc, SIOCGIFFLAGS, (struct ifreq *) &ifr );
    if ( res != 0 ) {
      TTCN_warning ( "EtherIf::find(): ioctl with SIOCGIFFLAGS failed" 
                     " on %s", ifn );
      continue;
    }
    unsigned short ifFlags = * (unsigned short *) ifr.data;
    debug ( "EtherIf::find(): %s flags: %04X", ifn, ifFlags );
    if ( ( ifFlags & IFF_LOOPBACK ) != 0 ) {
      debug ( "EtherIf::find(): %s is loop-back", ifn );
      continue;
    }
    if ( ( ifFlags & IFF_UP ) == 0 || ( ifFlags & IFF_RUNNING ) == 0 ) {
      debug ( "EtherIf::find(): %s is down or not running", ifn );
      continue;
    }
    // IF MAC address
    memcpy ( ifr.name, ifn, IF_NAMESIZE);
    memset ( ifr.data, 0, sizeof ( ifr.data ) );
    res = ioctl ( soc, SIOCGIFHWADDR, (struct ifreq *) &ifr );
    if ( res != 0 ) {
      TTCN_warning ( "EtherIf::find(): ioctl with SIOCGIFHWADDR failed" 
                     " on %s", ifn );
      continue;
    }
    unsigned short arpHwId = * (unsigned short *) ifr.data;
    debug ( "EtherIf::find(): %s HW: %04X", ifn, arpHwId );
    if ( arpHwId != ARPHRD_ETHER /* 1 */ ) {
      debug ( "EtherIf::find(): %s is not Ethernet", ifn );
      continue;
    }
    EtherAddr hwA ( ifr.data + 2, 6 );
    debug ( "EtherIf::find(): %s MAC A: %s", ifn, hwA.asStr () );
    // IF IP address
    memcpy ( ifr.name, ifn, IF_NAMESIZE);
    memset ( ifr.data, 0, sizeof ( ifr.data ) );
    res = ioctl ( soc, SIOCGIFADDR, (struct ifreq *) &ifr );
    if ( res != 0 ) {
      TTCN_warning ( "EtherIf::find(): ioctl with SIOCGIFADDR failed" 
                     " on %s", ifn );
      continue;
    }
    unsigned short ipPf = * (unsigned short *) ifr.data;
    debug ( "EtherIf::find(): %s PF: %04X", ifn, ipPf );
    if ( ipPf != PF_INET /* 2 */ && ipPf != PF_INET6 /* 10 */ ) {
      debug ( "EtherIf::find(): %s: PF is not IP ", ifn );
      continue;
    }
    unsigned int ipL = ( ipPf == PF_INET ) ? 4 : 16;
    IPAddr ipA ( ifr.data + 4, ipL );
    debug ( "EtherIf::find(): %s IP: %s", ifn, ipA.asStr () );
    if ( ifExpIpAddress != 0 ) {
      if ( ipA != ifExpIp ) {
        debug ( "EtherIf::find(): %s IP mismatch", ifn );
        continue;
      }
    } else {
      if ( ifExclIpAddress != 0 ) {
        if ( ipA == ifExclIp ) {
          debug ( "EtherIf::find(): %s IP excluded", ifn );
          continue;
        }
      }
    }    
    // Net Mask Length
    memcpy ( ifr.name, ifn, IF_NAMESIZE);
    memset ( ifr.data, 0, sizeof ( ifr.data ) );
    res = ioctl ( soc, SIOCGIFNETMASK, (struct ifreq *) &ifr );
    if ( res != 0 ) {
      TTCN_warning ( "EtherIf::find(): ioctl with SIOCGIFNETMASK" 
                     " failed on %s", ifn );
      continue;
    }
    IPAddr ipM ( ifr.data + 4, ipL );;
    debug ( "EtherIf::find(): %s NetMask: %s", ifn, ipM.asStr () );
    unsigned int ipML = 255;
    for ( unsigned int j = 0; j < ipL * 8; ++j ) {
      if ( ( ipM.bytes[j/8] & ( 1 << ( 7 - ( j & 7 ) ) ) ) == 0 ) {
        ipML = j;
        break;
      }
    }
    debug ( "EtherIf::find(): %s Net Mask Length: %i", ifn, ipML );
    if ( valid && netLength < ipML )
      continue;
    // Broadcast address
    memcpy ( ifr.name, ifn, IF_NAMESIZE);
    memset ( ifr.data, 0, sizeof ( ifr.data ) );
    res = ioctl ( soc, SIOCGIFBRDADDR, (struct ifreq *) &ifr );
    if ( res != 0 ) {
      TTCN_warning ( "EtherIf::find(): ioctl with SIOCGIFBRDADDR" 
                     " failed on %s", ifn );
      continue;
    }
    IPAddr bcA ( ifr.data + 4, ipL );
    debug ( "EtherIf::find(): %s Broadcast: %s", ifn, bcA.asStr () );
    // Interface found ( with network size test )
    debug ( "EtherIf::find(): Interface found: %s", ifn );
    strcpy ( name, ifn );
    index = i;
    hwAddr = hwA;
    origFlags = ifFlags;
    ipLength = ipL;
    realIpAddr = ipA;
    netMask = ipM;
    netLength = ipML;
    bCAddr = bcA;
    valid = true;
  }
  if ( valid ) {
    debug ( "EtherIf::find(): Interface selected: %i: %s",
            index, name );
  }
  close ( soc );
  return valid;
}


class Pcap
{
public:
  Pcap ( const char * ifNameStr, char * filterStr, bool nonBlockFlg = true ) :
    packetIn ( 0 ), pktLen ( 0 ), fnName ( "" ), pcap_hnd ( 0 ),
    nonBlock ( nonBlockFlg ), pcap_net ( 0 ), pcap_mask ( 0 ) {
      memset ( errBuf, 0, sizeof ( errBuf ) );
      memset ( filter, 0, sizeof ( filter ) );
      strncpy ( filter, filterStr, sizeof ( filter ) - 1 );
      memset ( ifName, 0, sizeof ( ifName ) );
      strncpy ( ifName, ifNameStr, sizeof ( ifName ) - 1 );
      nonBlock = nonBlockFlg;
  }
  bool start () {
    debug ( "Pcap::Pcap(): ifName: \"%s\"", ifName );
    memset ( errBuf, 0, sizeof ( errBuf ) );
    try {
      pcap_hnd = pcap_open_live( ifName, SNAPLEN, 1 /* promisc */,
                                 -1 /* to_ms */, errBuf );
      if ( pcap_hnd == 0 ) throw ( "pcap_open_live" );
      debug ( "Pcap::Pcap(): filter: \"%s\"", filter );
      if ( pcap_lookupnet ( ifName, &pcap_net, &pcap_mask, errBuf ) != 0 )
        throw ( "pcap_lookupnet" );
      debug ( "Pcap::Pcap(): net: %s", ipv4ToStr ( pcap_net ) );
      debug ( "Pcap::Pcap(): mask: %s", ipv4ToStr ( pcap_mask ) );
      int res = pcap_compile ( pcap_hnd, & fp, filter, 0 /* optimize no */,
                               pcap_net );
      if ( res != 0 )
        throw ( "pcap_compile" );
      if ( pcap_setfilter ( pcap_hnd, & fp ) != 0 )
        throw ( "pcap_setfilter" );
      if ( nonBlock ) {
        if ( pcap_setnonblock ( pcap_hnd, 1, errBuf ) != 0 )
          throw ( "pcap_setnonblock" );
      }
    } catch ( const char * pcapFName ) {
      fnName = pcapFName;
      if ( pcap_hnd != 0 ) {
        if ( errBuf[0] == 0 )
          strncpy ( errBuf, pcap_geterr ( pcap_hnd ), sizeof ( errBuf ) - 1 );
        pcap_close ( pcap_hnd ); pcap_hnd = 0;
      }
    }
    return pcap_hnd != 0;
  }
  ~Pcap () {
    if ( pcap_hnd != 0 ) {
      pcap_close ( pcap_hnd ); pcap_hnd = 0;
    }
  }
  inline const unsigned char * next ( unsigned int * length = 0 ) {
    packetIn = pcap_next ( pcap_hnd, &pkt_hdr );
    //if ( packetIn != 0 ) debug ( "Pcap::next(): packet received" );
    pktLen = pkt_hdr.len;
    if ( length != 0 )
      * length = pktLen;
    return packetIn;
  }
  inline bool sendPacket ( const unsigned char * packetOut,
                           unsigned int length ) {
    bool res = pcap_sendpacket ( pcap_hnd, packetOut, length ) == 0;
    if ( ! res )
      debug ( "Pcap::sendPacket(): pcap error: %s",
        pcap_geterr ( pcap_hnd ) );
    return res;
  }
  const unsigned char * packetIn;
  unsigned int        pktLen;
  char                errBuf[PCAP_ERRBUF_SIZE];
  const char          * fnName;
private:
  pcap_t        * pcap_hnd;
  char          ifName[IF_NAMESIZE];
  char          filter[1024];
  bpf_program   fp;
  bool          nonBlock;
  pcap_pkthdr   pkt_hdr;
  unsigned int  pcap_net;
  unsigned int  pcap_mask;
};

struct MsgWaitInfo
{
  unsigned int    index;
  unsigned int    nSent;
  timeval         sentTime;
  unsigned int    dhcpState;
  void set ( unsigned int i, unsigned char state ) {
    index = i;
    dhcpState = state;
    gettimeofday ( &sentTime, 0 );
    nSent = 1;
  }
  void update ( unsigned char state, bool resetCounter = true ) {
    dhcpState = state;
    gettimeofday ( &sentTime, 0 );
    if ( resetCounter )
      nSent = 1;
  }
  bool needRetransmit ( const timeval & now, unsigned int timeOutInms ) {
    timeval timeDiff = now - sentTime;
    //debug ( "MsgWaitInfo::needRetransmit(): %i.%06i",
    //  timeDiff.tv_sec, timeDiff.tv_usec );
    if ( (int) timeDiff.tv_sec < 0 )
      return false;
    if ( (unsigned int) timeDiff.tv_sec >= 1000u ) {
      debug ( "needRetransmit(): 1" );
      return true;
    }
    return (unsigned int) timeDiff.tv_sec * 1000000u +
           (unsigned int) timeDiff.tv_usec >= timeOutInms * 1000u;
  }
  MsgWaitInfo () {
    index = 0x8000DEAD;
    nSent = 0;
    sentTime.tv_sec = 0; sentTime.tv_usec = 0;
    dhcpState = DHCPState::INIT;
  }
};


struct DhcpCInfo {
  IPAddr          ipAddr;
  IPAddr          server;
  IPAddr          router;
  unsigned int    leaseExpire;
  EtherAddr       etherAddr;
  unsigned char   pad[2];
  inline DhcpCInfo () : leaseExpire ( 0 ), etherAddr ( ETHER_ADDR_ZERO ) {}
};

int cmpDhcpCInfoByAddrs ( const void * p1, const void * p2 ) // for qsort
{
  const IPAddr& aL = ( (DhcpCInfo*) p1 )->ipAddr;
  const IPAddr& aR = ( (DhcpCInfo*) p2 )->ipAddr;
  if ( aL.len != aR.len )
    return ( aL.len < aR.len ) ? -1 : +1;
  if ( aL.len != 0 )
    return memcmp ( aL.bytes, aR.bytes, 16 );
  return 0;
}

// Sorts a DhcpCInfo array based on state
// - First: bound addresses [0..iUnBnd-1],
// - then:  earlier bound, but currently not yet requested addresses [iUnReq..iInv-1],
// - last:  unrequested earlier unbound addresses and
//          addresses with error during requesting [iInv..nAddrs-1].
void sortDhcpInfoByState ( DhcpCInfo * clients, unsigned int nAddrs,
  unsigned int & iUnReq, unsigned int & iInv )
{
  iUnReq = 0;
  DhcpCInfo   tmp;
  for ( unsigned int i = 0; i < nAddrs; ++i ) {
    if ( clients[i].server.len != 0 &&
         clients[i].leaseExpire != 0 ) {
      if ( i != iUnReq ) {
        tmp = clients[iUnReq]; clients[iUnReq] = clients[i]; clients[i] = tmp;
      }
      ++iUnReq;
    }
  }
  iInv = iUnReq;
  for ( unsigned int i = iUnReq; i < nAddrs; ++i ) {
    if ( clients[i].leaseExpire != 0 ) {
      if ( i != iInv ) {
        tmp = clients[iInv]; clients[iInv] = clients[i]; clients[i] = tmp;
      }
      ++iInv;
    }
  }
}


static unsigned int dhcpMsgRetransmitCount = 0; // TODO: remove static
static unsigned int dhcpMsgRetransmitPeriodInms = 0; // TODO: remove static
static unsigned int dhcpMaxParallelRequestCount = 0; // TODO: remove static
static unsigned int dhcpTimeout = 0; // TODO: remove static

struct DhcpTimeOutError {};

class DhcpTimeoutInfo {
public:
  unsigned int  responseCnt;
  void start () {
    timeout1 = ( dhcpMsgRetransmitCount + 1 ) * ( ( dhcpMsgRetransmitPeriodInms + 999 ) / 1000 );
    if ( timeout1 > dhcpTimeout )
      timeout1 = dhcpTimeout;
    debug ( "DhcpTimeoutInfo::start(): timeouts: %i, %i", timeout1, dhcpTimeout );
    gettimeofday ( &startTime, 0 ); responseCnt = 0;
  }
  DhcpTimeoutInfo () { start (); }
  inline void msgArrived () { if ( responseCnt < 1000000000 ) ++responseCnt; }
  bool isTimeOut ( const timeval now ) {
    int timeDiff = ( now - startTime ).tv_sec;
    if ( timeDiff < 0 ) return false;
    if ( responseCnt == 0 && (unsigned int) timeDiff >= timeout1 ) {
      debug ( "DhcpTimeoutInfo::isTimeOut(): Timeout1: %i", timeDiff );
      return true;
    }
    if ( (unsigned int) timeDiff >= dhcpTimeout ) {
      debug ( "DhcpTimeoutInfo::isTimeOut(): DHCPTimeout: %i", timeDiff );
      return true;
    }
    return false;
  }
private:
  timeval       startTime;
  unsigned int  timeout1;
};

static DhcpTimeoutInfo dhcpTimeoutInfo;

// Performs DHCP operation
// The are 3 operations:
//   - Binding with discovery: Only etherAddr is filled, the rest is zeroed
//   - Reboot (tool restart): IP addresses must be filled, the server be cleared
//   - Rebind (for release): IP address and server must be set
// Return value:
//   - true in case of success false otherwise
// If determined Net Mask is given back in an otput parameter (otherwise it is
//   not changed)
// If given, then the number of bound addresses is returned in nBoundAddrs
//   (It is filled also in case of error.)
// TODO: works only for IPv4
bool dhcpOperation ( const char   * ifName,
                     unsigned int nAddrs,
                     DhcpCInfo    * clients,
                     unsigned int reqLeaseTime,
                     IPAddr       & netMask,
                     unsigned int * nBoundAddrs = 0 ) throw ( DhcpTimeOutError )
{
  if ( ifName == 0 || clients == 0 ) {
    debug ( "dhcpOperation(): Parameter error" );
    return false;
  }
  if ( nBoundAddrs != 0 )
    * nBoundAddrs = 0;

  timeval  now;
  gettimeofday ( &now, 0 );
  unsigned int rnd = now.tv_usec << 12; // random start value for transaction id

  Pcap  pcap ( ifName, "(udp and dst port 68 and src port 67)" );
  if ( ! pcap.start () ) {
    TTCN_warning ( "dhcpOperation(): Error in %s: %s",
      pcap.fnName, pcap.errBuf );
    return false;
  }
  unsigned char         packetOut[1514];
  const unsigned char * packetIn;

  unsigned int  nUnansweredMsg = 0;
  unsigned int  nUnboundAddr = nAddrs;
  MsgWaitInfo * msgInfo = new MsgWaitInfo[dhcpMaxParallelRequestCount];
  if ( msgInfo == 0 ) {
    TTCN_warning ( "dhcpOperation(): Memory allocation error" );
    return false;
  }
  unsigned int  ixNextIP = 0;
  timeval  lastChkTime;
  memset ( &lastChkTime, 0, sizeof ( lastChkTime ) );
  while ( nUnansweredMsg > 0 || ixNextIP < nAddrs ) {
    // Check and process responses
    int mct = 100;
    while ( ( packetIn = pcap.next () ) != 0 && mct-- ) {
      DHCPBase * dhcp = (DHCPBase*) packetIn;
      if ( ! dhcp->check ( pcap.pktLen ) )
        continue;
      //debug ( "dhcpOperation(): --> DHCP Response" );
      unsigned int  i = 0;
      DhcpCInfo     * client = 0;
      unsigned int  trId;
      for ( i = 0; i < nUnansweredMsg; ++i ) {
        trId = htonl ( msgInfo[i].index + rnd );
        if ( * (EtherAddr*) ( dhcp->dhcp.chaddr ) == clients[msgInfo[i].index].etherAddr &&
             trId == dhcp->dhcp.xid ) {
          client = & ( clients[msgInfo[i].index] );
          break;
        }
      }
      if ( client == 0 )
        continue;
      //debug ( "dhcpOperation(): etherAddr and trId matches" );
      //debug ( "dhcpOperation(): xid: %i", dhcp->dhcp.xid );
      DHCPRespOpt options ( (const unsigned char*) ( dhcp + 1 ),
        pcap.pktLen - sizeof ( dhcp ) );
      if ( msgInfo[i].dhcpState == DHCPState::SELECTING &&
           options.messageType == 2 /* DHCP OFFER */ ) {
        debug ( "dhcpOperation(): DHCPOFFER" );
        debug ( "dhcpOperation(): Offered address: %s",
            ipv4ToStr ( dhcp->dhcp.yiaddr ) );
        if ( ! options.isValid ( 0 ) ) {
          debug ( "dhcpOperation(): DHCPOFFER: bad options" );
          continue;
        }
        dhcpTimeoutInfo.msgArrived ();
        client->ipAddr.set ( dhcp->dhcp.yiaddr );
        client->server.set ( options.dhcpServer );
        unsigned int length = CreateDHCPRequest ( packetOut, trId,
          client->etherAddr, client->ipAddr.v4, client->server.v4, reqLeaseTime );
        msgInfo[i].update ( DHCPState::REQUESTING );
        debug ( "dhcpOperation(): <-- DHCP Request"
          " (DHCPREQUEST)   i: %i", msgInfo[i].index );
        //debug_dump ( packetOut, length );
        if ( ! pcap.sendPacket ( packetOut, length ) ) {
          // Rely on retransmission
        }
      } else if ( ( msgInfo[i].dhcpState == DHCPState::REQUESTING ||
                    msgInfo[i].dhcpState == DHCPState::REBOOTING ||
                    msgInfo[i].dhcpState == DHCPState::REBINDING ) &&
                  options.messageType == 5 /* DHCP ACK */ ) {
        debug ( "dhcpOperation(): DHCPACK" );
        if ( ! options.isValid ( DHCPRespOpt::IPV4_MASK |
          DHCPRespOpt::IPV4_ROUTER | DHCPRespOpt::LEASE_TIME ) ) {
          debug ( "dhcpOperation(): DHCPACK: bad options" );
          continue;
        }
        if ( msgInfo[i].dhcpState == DHCPState::REQUESTING &&
             options.dhcpServer != client->server )
          break;
        if ( dhcp->dhcp.yiaddr != client->ipAddr )
          break;
        dhcpTimeoutInfo.msgArrived ();
        if ( netMask.len == 0 ) {
          netMask.set ( options.ipv4Mask );
        } else if ( netMask != options.ipv4Mask ) {
          debug ( "dhcpOperation(): DHCPACK: bad netMask: %s",
                  ipv4ToStr ( options.ipv4Mask ) );
          debug ( "dhcpOperation(): DHCPACK: previous netMask: %s",
                  netMask.asStr () );
        }
        client->server.set ( options.dhcpServer );
        client->router.set ( options.ipv4router );
        client->leaseExpire = msgInfo[i].sentTime.tv_sec + options.leaseTime;
        --nUnboundAddr;
        if ( i != --nUnansweredMsg )
          msgInfo[i] = msgInfo[nUnansweredMsg];
        debug ( "dhcpOperation(): IP address %s is bound",
          ipv4ToStr ( dhcp->dhcp.yiaddr ) );
      } else if ( ( msgInfo[i].dhcpState == DHCPState::REBOOTING ) &&
                  options.messageType == 6 /* DHCP NACK */ ) {
        dhcpTimeoutInfo.msgArrived ();
        debug ( "dhcpOperation(): DHCPNAK in REBOOTING" );
        client->leaseExpire = 0;
        if ( i != --nUnansweredMsg )
            msgInfo[i] = msgInfo[nUnansweredMsg];
        client = 0;
      } else if ( ( msgInfo[i].dhcpState == DHCPState::REQUESTING ) &&
                  options.messageType == 6 /* DHCP NACK */ ) {
        dhcpTimeoutInfo.msgArrived ();
        debug ( "dhcpOperation(): DHCPNAK in REQUESTING" );
        if ( msgInfo[i].nSent <= dhcpMsgRetransmitCount ) {
          debug ( "dhcpOperation(): Stepping back to disover" );
          unsigned int length = CreateDHCPDiscover ( packetOut, trId,
            client->etherAddr );
          msgInfo[i].update ( DHCPState::SELECTING, false );
          if ( ! pcap.sendPacket ( packetOut, length ) ) {
            // Rely on retransmission
          }
        } else {
          debug ( "dhcpOperation(): Retransmit limit reached" );
          client->leaseExpire = 0;
          if ( i != --nUnansweredMsg )
              msgInfo[i] = msgInfo[nUnansweredMsg];
          client = 0;
        }
      } else if ( ( msgInfo[i].dhcpState == DHCPState::REBINDING ) &&
                  options.messageType == 6 /* DHCP NACK */ ) {
        dhcpTimeoutInfo.msgArrived ();
        debug ( "dhcpOperation(): DHCPNAK in REBINDING" );
        client->leaseExpire = 0;
        if ( i != --nUnansweredMsg )
            msgInfo[i] = msgInfo[nUnansweredMsg];
        client = 0;
      } else {
        debug ( "dhcpOperation(): Unexpected DHCP message" );
      }
    } // while ( packetIn != 0 ...
    // Check for DHCP timeout
    timeval  now;
    gettimeofday ( &now, 0 );
    if ( dhcpTimeoutInfo.isTimeOut ( now ) ) {
      delete[] msgInfo;
      if ( nBoundAddrs != 0 )
        * nBoundAddrs = 0;
      throw ( DhcpTimeOutError() );
    }
    // Check for message timeouts
    for ( unsigned int i = 0; i < nUnansweredMsg; ++i ) {
      if ( msgInfo[i].needRetransmit ( now, dhcpMsgRetransmitPeriodInms ) ) {
        debug ( "dhcpOperation(): Timeout i: %i   cnt: %i",
          msgInfo[i].index, msgInfo[i].nSent );
        if ( msgInfo[i].nSent > dhcpMsgRetransmitCount ) {
          if ( msgInfo[i].dhcpState == DHCPState::SELECTING )
            debug ( "dhcpOperation(): No answer i: %i",
              msgInfo[i].index );
          else
            debug ( "dhcpOperation(): No answer for address: %s",
              clients[msgInfo[i].index].ipAddr.asStr () );
          clients[msgInfo[i].index].leaseExpire = 0;
          if ( i != --nUnansweredMsg )
            msgInfo[i] = msgInfo[nUnansweredMsg];
          continue;
        }
        unsigned int length = 0;
        unsigned int trId = htonl ( msgInfo[i].index + rnd );
        if ( msgInfo[i].dhcpState == DHCPState::SELECTING ) {
          length = CreateDHCPDiscover ( packetOut, trId,
            clients[msgInfo[i].index].etherAddr );
        } else if ( msgInfo[i].dhcpState == DHCPState::REQUESTING ||
                    msgInfo[i].dhcpState == DHCPState::REBOOTING ||
                    msgInfo[i].dhcpState == DHCPState::REBINDING ) {
          length = CreateDHCPRequest ( packetOut, trId,
            clients[msgInfo[i].index].etherAddr,
            clients[msgInfo[i].index].ipAddr.v4,
            clients[msgInfo[i].index].server.v4,
            reqLeaseTime, msgInfo[i].dhcpState );
        } else
          continue;
        //debug_dump ( packetOut, length );
        if ( pcap.sendPacket ( packetOut, length ) ) {
          msgInfo[i].sentTime = now;
          ++msgInfo[i].nSent;
          debug ( "dhcpOperation(): <-- DHCP Request retransmit" );
        } else {
          // Rely on retransmission
        }
      }
    }
    lastChkTime = now;
    // Send new requests
    unsigned int  msct = 5;
    while ( nUnansweredMsg < dhcpMaxParallelRequestCount && ixNextIP < nAddrs &&
            msct-- > 0) {
      unsigned int state = 0;
      unsigned int length = 0;
      unsigned int trId = htonl ( ixNextIP + rnd );
      if ( clients[ixNextIP].leaseExpire == 0 ) {
        length = CreateDHCPDiscover ( packetOut, trId,
          clients[ixNextIP].etherAddr );
        state = DHCPState::SELECTING;
      } else {
        state = ( clients[ixNextIP].server.len == 0 )
          ? DHCPState::REBOOTING : DHCPState::REBINDING;
        length = CreateDHCPRequest ( packetOut, trId,
          clients[ixNextIP].etherAddr, clients[ixNextIP].ipAddr.v4,
          clients[ixNextIP].server.v4, reqLeaseTime, state );
      }
      //debug_dump ( packetOut, length );
      msgInfo[nUnansweredMsg++].set ( ixNextIP++, state );
      if ( pcap.sendPacket ( packetOut, length ) ) {
        debug ( "dhcpOperation(): <-- DHCP Request (DHCP%s)   i: %i",
          ( state == DHCPState::SELECTING ) ? "DISCOVER" : "REQUEST",
          ixNextIP - 1 );
      } else {
        // Rely on retransmission
      }
    }
    usleep ( 500 );
  }
  delete[] msgInfo;
  debug ( "dhcpOperation(): unsuccessful: %i / %i", nUnboundAddr, nAddrs );
  if ( nBoundAddrs != 0 )
    * nBoundAddrs = nAddrs - nUnboundAddr;
  return nUnboundAddr == 0;
}


class DhcpLeaseFile {
public:
  DhcpLeaseFile ( const char * fName ) : errorStr ( 0 ),
    fileName ( 0 ), fNlen ( 0 ), fileNameRenamed ( 0 ), file ( 0 ) {
    if ( fName != 0 && fName[0] != 0) {
      fNlen = strlen ( fName );
      fileName = new char[fNlen + 1];
      if ( fileName != 0 )
        memcpy ( fileName, fName, fNlen + 1 );
    }
  }
  ~DhcpLeaseFile () {
    if ( fileName != 0 ) delete[] fileName;
    if ( fileNameRenamed != 0 ) delete[] fileNameRenamed;
  }
  DhcpCInfo * readAll ( unsigned int & nAddrs );
  bool writeAll ( unsigned int nAddrs, const DhcpCInfo *clients );
  void rename () {
    fileNameRenamed = new char[fNlen + 2];
    if ( fileNameRenamed != 0 ) {
      memcpy ( fileNameRenamed, fileName, fNlen );
      fileNameRenamed[fNlen] = '~';
      fileNameRenamed[fNlen + 1] = 0;
      if ( std::rename ( fileName, fileNameRenamed ) != 0 ) {
        debug ( "DhcpLeaseFile::rename(): did not succeed" );
        delete[] fileNameRenamed;
        fileNameRenamed = 0;
      }
    }
  }
  void deleteRenamed () {
    if ( fileNameRenamed != 0 ) {
      std::remove ( fileNameRenamed ); fileNameRenamed = 0;
    }
  }
  void keepRenamed () {
    if ( fileNameRenamed != 0 ) {
      std::rename ( fileNameRenamed, fileName );
      delete[] fileNameRenamed; fileNameRenamed = 0;
      if ( fileName != 0 ) { delete[] fileName; fileName = 0; }
    }
  }
  const char  * errorStr;
  inline bool isError () { return memcmp ( errorStr, "Error", 5 ) == 0; }
private:
  char  * fileName;
  int   fNlen;
  char  * fileNameRenamed;
  enum { LINE_LEN = 128, MAX_N_ADDRS = 1024*1024 };
  char  line[LINE_LEN];
  FILE  * file;
  const char * readLine () throw ( const char * ) {
    if ( fgets ( line, LINE_LEN, file ) == 0 ) throw "Error while reading file";
    return line;
  }
private:
  DhcpLeaseFile();
};

DhcpCInfo * DhcpLeaseFile::readAll ( unsigned int & nAddrs )
{
  file = 0;
  DhcpCInfo *   clients = 0;
  unsigned int  ixAddr = 0;
  errorStr = 0;
  try {
    if ( fileName == 0 ) throw "No lease file to read";
    file = fopen ( fileName, "r" );
    if ( file == 0 ) throw "Could not open lease file";
    if ( strcmp ( readLine (), "DHCP IP leases\n" ) != 0 )
       throw "Error in line 1";
    unsigned int  nAddrsFile = 0;
    if ( sscanf ( readLine(), "Number of addresses: %u", & nAddrsFile ) != 1 )
       throw "Error in line 2";
    unsigned int  leaseExpire = 0, lEDeltaMax = 0;
    if ( sscanf ( readLine(), "Leases expire: %u   (+%u)",
                  & leaseExpire, & lEDeltaMax ) != 2 )
       throw "Error in line 3";
    timeval now;
    gettimeofday ( &now, 0 );
    if ( nAddrsFile == 0 )
       throw "No IP address in lease file";
    if ( leaseExpire + lEDeltaMax < (unsigned int) now.tv_sec )
       throw "Leases expired";
    if ( nAddrsFile > MAX_N_ADDRS ) throw "Error in line 2 parameter";
    clients = new DhcpCInfo[( nAddrsFile >= nAddrs ) ? nAddrsFile : nAddrs];
    if ( clients == 0 ) throw "Error while allocating memory";
    char          ipAddr[LINE_LEN], etherAddr[LINE_LEN];
    unsigned int  d = 0;
    for ( unsigned int i = 0; i < nAddrsFile; ++i ) {
      if ( sscanf ( readLine (), "%s %s %u", ipAddr, etherAddr, & d ) != 3 )
        throw "Error while reading file contents";
      if ( leaseExpire + d >= (unsigned int) now.tv_sec ) {
        clients[ixAddr].ipAddr = IPAddr ( ipAddr );
        if ( clients[ixAddr].ipAddr.len == 0 )
          throw "Error in file contents format";
        if ( ! strToEtherAddr ( etherAddr, clients[ixAddr].etherAddr ) )
          throw "Error in file contents format";
        clients[ixAddr].leaseExpire = leaseExpire + d;
        ++ixAddr;
      }
    }
    if ( ixAddr == 0 )
       throw "Leases expired";
    nAddrs = ixAddr;
  } catch ( const char * errStr ) {
    errorStr = errStr;
    if ( clients != 0 )
      delete[] clients;
    clients = 0;
    nAddrs = 0;
  }
  if ( file != 0 ) fclose ( file );
  file = 0;
  return clients;
}

bool DhcpLeaseFile::writeAll ( unsigned int nAddrs, const DhcpCInfo *clients )
{
  if ( fileName == 0 ) { errorStr = "No lease file to write"; return false; }
  unsigned int leaseExpire = 0xFFFFFFFF, lEMax = 0;
  if ( nAddrs > 0 ) {
    for ( unsigned int i = 0; i < nAddrs; ++i ) {
      if ( clients[i].leaseExpire < leaseExpire )
        leaseExpire = clients[i].leaseExpire;
      if ( clients[i].leaseExpire > lEMax )
        lEMax = clients[i].leaseExpire;
    }
  } else {
    timeval now;
    gettimeofday ( &now, 0 );
    leaseExpire = now.tv_sec;
    lEMax = leaseExpire;
  }
  static const char headerFormat[] =
    "DHCP IP leases\n"
    "Number of addresses: %10u\n"
    "Leases expire:       %10u   (+%u)   # %.24s\n";
  file = fopen ( fileName, "w" );
  if ( file == 0 ) { errorStr = "Error while writing to file"; return false; }
  errorStr = 0;
  try {
    time_t tm = leaseExpire;
    if ( fprintf ( file, headerFormat, nAddrs, leaseExpire, lEMax - leaseExpire,
                   ctime ( &tm ) ) <= 0 )
      throw "Error while writing to file";
    for ( unsigned int i = 0; i < nAddrs; ++i ) {
      if ( fprintf ( file, "%15s   %17s   %u\n",
                     clients[i].ipAddr.asStr (),
                     clients[i].etherAddr.asStr (),
                     clients[i].leaseExpire - leaseExpire ) <= 0 )
        throw "Error while writing to file contents";
    }
  } catch ( const char * errStr ) {
    errorStr = errStr;
  }
  if ( fclose ( file ) != 0 && errorStr == 0 )
    errorStr = "Error while closing written file";
  file = 0;
  return errorStr == 0;
}

EtherAddr * incrEtherAddr ( EtherAddr * dst, unsigned int cnt = 1,
                                const EtherAddr * src = 0 )
{
  if ( src == 0 )
    src = dst;
  for ( unsigned int i = 6; i != 0; ) {
    --i;
    unsigned int s = src->bytes[i] + cnt;
    dst->bytes[i] = s;
    cnt = s >> 8;
  }
  return dst;
}

int cmpEtherAddr ( const void * p1, const void * p2 ) // for qsort
{
  return memcmp ( * (void**) p1, * (void**) p2, 6 );
}

bool createUniqueEtherAddrs ( unsigned int nAddrs, DhcpCInfo * clients,
  unsigned int iNew, EtherAddr & etherAddr )
{
  EtherAddr * * exc = new EtherAddr* [iNew];
  if ( exc == 0 )
    return false;
  for ( unsigned int i = 0; i < iNew; ++i )
    exc[i] = & clients[i].etherAddr;
  qsort ( exc, iNew, sizeof ( *exc ), cmpEtherAddr );

  unsigned int j = 0;
  while ( j < iNew && memcmp ( etherAddr.bytes, exc[j]->bytes, 6 ) > 0 )
      ++j;
  for ( unsigned int i = iNew; i < nAddrs; ++i ) {
    while ( j < iNew && memcmp ( etherAddr.bytes, exc[j]->bytes, 6 ) == 0 ) {
      incrEtherAddr ( &etherAddr );
      ++j;
    }
    clients[i].etherAddr = etherAddr;
    incrEtherAddr ( & etherAddr );
  }
  delete[] exc;
  return true;
}

int cmpIPv4Addr ( const void * p1, const void * p2 ) // for qsort
{
  return memcmp ( p1, p2, 4 );
}
int cmpIPAddr ( const void * p1, const void * p2 ) // for qsort
{
  const IPAddr& aL = * (IPAddr*) p1;
  const IPAddr& aR = * (IPAddr*) p2;
  if ( aL.len != aR.len )
    return ( aL.len < aR.len ) ? -1 : +1;
  if ( aL.len != 0 )
    return memcmp ( aL.bytes, aR.bytes, 16 );
  return 0;
}

struct MemoryError {
  int   i;
  MemoryError ( int loc ) : i ( loc ) {}
private:
  MemoryError ();
};

bool requestIpAddressesWithDhcp ( const char        * ifName,
                                  const EtherAddr   & etherAddr,
                                  unsigned int      nAddrs,
                                  IPAddr            * ipAddrs,
                                  unsigned int      reqLeaseTime,
                                  IPAddr            & netMask,
                                  IPAddr            & router,
                                  const char        * leaseFileName )
{
  const unsigned int EXTRA_REQUEST_BURST = 20;
  if ( ifName == 0 || ipAddrs == 0 ) {
    debug ( "requestIpAddressesWithDhcp(): Parameter error" );
    return false;
  }
  netMask.clear ();
  router.clear ();
  DhcpLeaseFile   leaseFile ( leaseFileName );
  unsigned int    nReadAddrs = nAddrs + EXTRA_REQUEST_BURST;
                               // minimum size for array
  DhcpCInfo * clients = leaseFile.readAll ( nReadAddrs );
  if ( leaseFile.errorStr != 0 ) {
    if ( leaseFile.isError () )
      TTCN_warning ( "requestIpAddressesWithDhcp(): %s", leaseFile.errorStr );
    else
      debug ( "requestIpAddressesWithDhcp(): %s", leaseFile.errorStr );
  }
  if ( clients == 0 )
    clients = new DhcpCInfo[nAddrs + EXTRA_REQUEST_BURST];
  if ( clients == 0 ) {
    TTCN_warning ( "requestIpAddressesWithDhcp(): Memory allocation error" );
    return false;
  }
  leaseFile.rename ();
  unsigned int nBoundAddrs = 0;
  debug ( "requestIpAddressesWithDhcp(): N. of read addresses: %i", nReadAddrs );
  dhcpTimeoutInfo.start ();
  try {
    if ( nReadAddrs != 0 ) {
      unsigned int nReqAddrs = ( nAddrs <= nReadAddrs ) ? nAddrs : nReadAddrs;
      dhcpOperation ( ifName, nReqAddrs, clients, reqLeaseTime, netMask,
        & nBoundAddrs );
      unsigned int nLeft = nReadAddrs - nReqAddrs;
      while ( nBoundAddrs < nAddrs && nLeft > 0 ) {
        unsigned int nReqAS = nAddrs - nBoundAddrs;
        unsigned int nBoundAS = 0;
        if ( nReqAS < EXTRA_REQUEST_BURST ) nReqAS = EXTRA_REQUEST_BURST;
        if ( nReqAS > nLeft ) nReqAS = nLeft;
        dhcpOperation ( ifName, nReqAS, clients + nReqAddrs, reqLeaseTime,
          netMask, & nBoundAS );
        nBoundAddrs += nBoundAS;
        nReqAddrs += nReqAS;
        nLeft -= nReqAS;
      }
      debug ( "requestIpAddressesWithDhcp(): (file) requested: %i bound: %i",
        nReqAddrs, nBoundAddrs );
      unsigned int  iUnReq = 0, iInv = 0;
      sortDhcpInfoByState ( clients, nReadAddrs, iUnReq, iInv );
      debug ( "requestIpAddressesWithDhcp(): file: nRd: %i iUnR:%i iInv:%i",
             nReadAddrs, iUnReq, iInv );
      if ( iInv > nAddrs ) {
        // Release superfluous IP addresses
        debug ( "requestIpAddressesWithDhcp(): Releasing superfluous IP addresses" );
        dhcpOperation ( ifName, iInv - nAddrs, clients + nAddrs, 1, netMask );
      } 
    }
    if ( nBoundAddrs < nAddrs ) {
      // Request additional IP addresses
      debug ( "requestIpAddressesWithDhcp(): Requesting additional IP addresses" );
      for ( unsigned int i = nBoundAddrs; i < nAddrs; ++i )
        clients[i] = DhcpCInfo ();
      EtherAddr eAddr = etherAddr;
      if ( ! createUniqueEtherAddrs ( nAddrs, clients, nBoundAddrs, eAddr ) )
        throw MemoryError ( 1 );
      unsigned int nBoundAS = 0;
      dhcpOperation ( ifName, nAddrs - nBoundAddrs, clients + nBoundAddrs,
                      reqLeaseTime, netMask, & nBoundAS );
      nBoundAddrs += nBoundAS;
      debug ( "requestIpAddressesWithDhcp(): bound: +%i -> %i",
        nBoundAS, nBoundAddrs );
      if ( nBoundAddrs < nAddrs ) {
        unsigned int  iUnReq = 0, iInv = 0;
        sortDhcpInfoByState ( clients, nAddrs, iUnReq, iInv );
        for ( int nTry = 1; nBoundAddrs < nAddrs && nTry <= 5; ++nTry ) {
          debug ( "requestIpAddressesWithDhcp(): Try %u", nTry );
          unsigned int nReqAS = nAddrs - nBoundAddrs;
          if ( nReqAS < EXTRA_REQUEST_BURST ) nReqAS = EXTRA_REQUEST_BURST;
          if ( ! createUniqueEtherAddrs ( nBoundAddrs + nReqAS, clients,
                                          nBoundAddrs, eAddr ) )
            throw MemoryError ( 2 );
          nBoundAS = 0;
          dhcpOperation ( ifName, nReqAS, clients + nBoundAddrs, reqLeaseTime,
            netMask, & nBoundAS );
          sortDhcpInfoByState ( clients + nBoundAddrs, nReqAS, iUnReq, iInv );
          nBoundAddrs += nBoundAS;
          debug ( "requestIpAddressesWithDhcp(): bound: +%i -> %i",
            nBoundAS, nBoundAddrs );
        }
        if ( nBoundAddrs > nAddrs ) {
          // Release superfluous IP addresses
          dhcpOperation ( ifName, nBoundAddrs - nAddrs, clients + nAddrs, 1,
            netMask );
          nBoundAddrs = nAddrs;
        }
        if ( nBoundAddrs != nAddrs ) {
          // The requested number of IP addresses could not be requested.
          // Release all bound IP addresses
          sortDhcpInfoByState ( clients, nAddrs, iUnReq, iInv );
          dhcpOperation ( ifName, iInv, clients, 1, netMask );
          nBoundAddrs = 0;
          debug ( "requestIpAddressesWithDhcp():"
                  " Could not request enough IP addresses" );
        }
      }
    }
  } catch ( MemoryError err ) {
    nBoundAddrs = 0;
    TTCN_warning ( "requestIpAddressesWithDhcp(): Memory allocation error (%i)", err.i );
  } catch ( DhcpTimeOutError err ) {
    nBoundAddrs = 0;
    TTCN_warning ( "requestIpAddressesWithDhcp(): DHCP timeout" );
    if ( dhcpTimeoutInfo.responseCnt == 0 ) {
      leaseFile.keepRenamed ();
      delete[] clients;
      return false;
    }
  }
  if ( nBoundAddrs != 0 ) {
    router = clients[0].router;
    qsort ( clients, nBoundAddrs, sizeof ( *clients ), cmpDhcpCInfoByAddrs );
  }
  leaseFile.writeAll ( nBoundAddrs, clients );
  if ( leaseFile.errorStr != 0 ) {
    if ( leaseFile.isError () )
      TTCN_warning ( "requestIpAddressesWithDhcp(): %s", leaseFile.errorStr );
    else
      debug ( "requestIpAddressesWithDhcp(): %s", leaseFile.errorStr );
  }
  leaseFile.deleteRenamed ();
  if ( nBoundAddrs == nAddrs ) {
    for ( unsigned int i = 0; i < nAddrs; ++i )
      ipAddrs[i] = clients[i].ipAddr;
    qsort ( ipAddrs, nAddrs, sizeof ( *ipAddrs ) , cmpIPv4Addr );
  }
  delete[] clients;
  return nBoundAddrs == nAddrs;
}

bool releaseIpAddresses ( const char * ifName, const char * leaseFileName )
{
  if ( ifName == 0 ) {
    debug ( "releaseIpAddresses(): Parameter error" );
    return false;
  }
  DhcpLeaseFile   leaseFile ( leaseFileName );
  unsigned int    nAddrs = 0;
  DhcpCInfo * clients = leaseFile.readAll ( nAddrs );
  if ( leaseFile.errorStr != 0 ) {
    if ( leaseFile.isError () ) {
      TTCN_warning ( "releaseIpAddresses(): %s", leaseFile.errorStr );
      if ( clients == 0 )
        return false;
    }
    else
      debug ( "releaseIpAddresses(): %s", leaseFile.errorStr );
  }
  if ( clients == 0 )
    return true;
  leaseFile.rename ();
  bool res = false;
  dhcpTimeoutInfo.start ();
  try {
    IPAddr  netMask;
    res = dhcpOperation ( ifName, nAddrs, clients, 1, netMask );
  } catch ( DhcpTimeOutError err ) {
    TTCN_warning ( "releaseIpAddresses(): DHCP timeout" );
    if ( dhcpTimeoutInfo.responseCnt == 0 ) {
      leaseFile.keepRenamed ();
      return false;
    }
  }
  leaseFile.deleteRenamed ();
  leaseFile.writeAll ( 0, 0 );
  return res;
}


// Constructs an IP address
// from the real IP address offset with the index
IPAddr  makeIPAddr ( unsigned int index, const IPAddr & realIpAddr,
                                         const IPAddr & netMask ) {
  IPAddr  dst;
  unsigned int sum = index;
  for ( int i = realIpAddr.len - 1; i >= 0; --i ) {
    sum += (unsigned int) ( realIpAddr.bytes[i] );
    dst.bytes[i] = ( realIpAddr.bytes[i] & netMask.bytes[i] ) |
                   ( sum & ~netMask.bytes[i] );
    if ( sum >= (unsigned int) ( realIpAddr.bytes[i] ) )
      sum >>= 8;
    else
      sum = ( sum >> 8 ) + 1;
  }
  dst.len = realIpAddr.len;
  return dst;
}

// Tests a constructed IP address
// if it is valid (that is not network or broadcast address)
bool isIPAddrValid ( const IPAddr & ipAddr, const IPAddr & netMask ) {
  bool ah0 = true, ah1 = true;
  for ( int i = ipAddr.len - 1; i >= 0; --i ) {
    ah0 = ah0 && ( ipAddr.bytes[i] & ~netMask.bytes[i] ) == 0;
    ah1 = ah1 && ( ipAddr.bytes[i] & ~netMask.bytes[i] ) == ~netMask.bytes[i];
  }
  return ! ( ah0 || ah1 );
}

static unsigned int  arpMsgRetransmitCount = 0;
static unsigned int  arpMsgRetransmitPeriodInms = 0;
static unsigned int  arpMaxParallelRequestCount = 0;

bool createIpAddrsWithARP ( const char      * ifName,
                            const EtherAddr & etherAddr,
                            const IPAddr    & realIpAddr,
                            unsigned int    nAddrs,
                            IPAddr          * ipAddrs,
                            const IPAddr    & netMask )
{
  if ( ifName == 0 ) {
    debug ( "createIpAddrsWithARP(): Parameter error" );
    return false;
  }
  memset ( ipAddrs, 0, nAddrs * sizeof ( *ipAddrs ) );

  unsigned int netLength = 0;
  for ( unsigned int i = 0; i < netMask.len * 8; ++i ) {
    if ( ( netMask.bytes[i/8] & ( 1 << ( 7 - ( i & 7 ) ) ) ) == 0 ) {
      netLength = i;
      break;
    }
  }
  unsigned hostLen = netMask.len * 8 - netLength;
  unsigned int netSize = 1 << ( hostLen < 32 ? hostLen : 31 );
  debug ( "createIpAddrsWithARP(): hostLen: %i", hostLen );
  if ( nAddrs >= netSize ) {
    TTCN_warning ( "createIpAddrsWithARP(): network too small" );
    return false;
  }
  
  Pcap pcap ( ifName, "arp" );
  if ( ! pcap.start () ) {
    TTCN_warning ( "createIpAddrsWithARP(): Error in %s: %s",
      pcap.fnName, pcap.errBuf );
    return false;
  }

  unsigned char         packetOut[1514];
  const unsigned char * packetIn;

  unsigned int  nUnansweredMsg = 0;
  MsgWaitInfo * msgInfo = new MsgWaitInfo[arpMaxParallelRequestCount];
  if ( msgInfo == 0 ) {
    TTCN_warning ( "createIpAddrsWithARP(): Memory allocation error" );
    return false;
  }
  unsigned int  ixNextIP = 0;
  timeval  lastChkTime;
  memset ( &lastChkTime, 0, sizeof ( lastChkTime ) );
  unsigned int  offsIP = 1;
  while ( ixNextIP < nAddrs &&
          ( offsIP < netSize || nUnansweredMsg > 0 ) ) {
    // Check for responses
    int mct = 100;
    while ( ( packetIn = pcap.next () ) != 0 && mct-- ) {
      const EtherHdr * etherHdr = EtherHdr::check ( packetIn, pcap.pktLen );
      if ( etherHdr == 0 || etherHdr->getType () != EtherHdr::ARP ) continue;
      const ARPHdr * arpHdr = ARPHdr::check ( packetIn + EtherHdr::LENGTH,
                                              pcap.pktLen - EtherHdr::LENGTH );
      if ( arpHdr == 0 || arpHdr->operation[1] != ARPHdr::REPLY ) continue;
      unsigned int offs = EtherHdr::LENGTH + ARPHdr::LENGTH;
      IPAddr sender;
      if ( arpHdr->pLen == 4 ) {
        const ARPv4Payload * arpv4Payload = ARPv4Payload::check (
          packetIn + offs, pcap.pktLen - offs );
        if ( arpv4Payload != 0 )
          sender.set ( arpv4Payload->spa );
      } else {
        const ARPv6Payload * arpv6Payload = ARPv6Payload::check (
          packetIn + offs, pcap.pktLen - offs );
        if ( arpv6Payload != 0 )
          sender.set ( arpv6Payload->spa );
      } 
      debug ( "createIpAddrsWithARP(): --> ARP Reply" );
      //debug_dump ( packetIn, pcap.pktLen );
      for ( unsigned int i = 0; i < nUnansweredMsg; ++i ) {
        if ( sender ==
             makeIPAddr( msgInfo[i].index, realIpAddr, netMask ) ) {
          debug ( "createIpAddrsWithARP(): IP address %s cannot be used",
            sender.asStr () );
          if ( i != --nUnansweredMsg )
            msgInfo[i] = msgInfo[nUnansweredMsg];
          break;
        }
      } // for
    }
    // Check for timeouts
    timeval  now;
    gettimeofday ( &now, 0 );
    for ( unsigned int i = 0; i < nUnansweredMsg; ++i ) {
      if ( msgInfo[i].needRetransmit ( now, arpMsgRetransmitPeriodInms ) ) {
        //debug ( "createIpAddrsWithARP(): Timeout   IP offset: %i   cnt: %i",
        //  msgInfo[i].index, msgInfo[i].nSent );
        if ( msgInfo[i].nSent > arpMsgRetransmitCount ) {
          ipAddrs[ixNextIP++] = makeIPAddr( msgInfo[i].index, realIpAddr, netMask );
          //debug ( "createIpAddrsWithARP(): IP address %s is selected",
          //     ipAddrs[ixNextIP - 1].asStr () );
          if ( i != --nUnansweredMsg )
            msgInfo[i] = msgInfo[nUnansweredMsg];
        } else {
          unsigned int length = ARPMsgOut::create ( packetOut,
            etherAddr, realIpAddr,
            ETHER_BC_ADDR, makeIPAddr( msgInfo[i].index, realIpAddr, netMask ),
            ARPHdr::REQUEST );
          //debug_dump ( packetOut, sizeof ( ARPv4 ) );
          if ( pcap.sendPacket ( packetOut, length ) ) {
            msgInfo[i].sentTime = now;
            ++msgInfo[i].nSent;
          } else {
            // Rely on retransmission
          }
        }
      }
    }
    lastChkTime = now;
    // Send new request
    while ( nUnansweredMsg < arpMaxParallelRequestCount &&
            ixNextIP + nUnansweredMsg < nAddrs && offsIP < netSize ) {
      unsigned int length = ARPMsgOut::create ( packetOut,
        etherAddr, realIpAddr,
        ETHER_BC_ADDR, makeIPAddr( offsIP, realIpAddr, netMask ),
        ARPHdr::REQUEST );
      //debug_dump ( packetOut, length );
      if ( pcap.sendPacket ( packetOut, length ) ) {
        debug ( "createIpAddrsWithARP(): <-- ARP Request   IP offset: %i",
          offsIP );
        msgInfo[nUnansweredMsg++].set ( offsIP++, 0 );
        while ( !isIPAddrValid ( makeIPAddr( offsIP, realIpAddr, netMask ), netMask ) )
          ++offsIP;
      } else {
        // Rely on retransmission
      }
    }
    usleep ( 1000 );
  }
  delete[] msgInfo;
  debug ( "createIpAddrsWithARP(): Number of available addresses: %i / %i",
          ixNextIP, nAddrs );
  if ( ixNextIP == nAddrs )
    qsort ( ipAddrs, nAddrs, sizeof ( *ipAddrs ), cmpIPAddr );
  return ixNextIP == nAddrs;
}


const char * cCTS ( const CHARSTRING & s ) {
  return ( s.is_bound () && s != "" ) ? (const char*) s : (const char*) 0;
}

} /* namespace IPL4asp__PortType */

using namespace IPL4asp__PortType;

extern BOOLEAN IPL4asp__Functions::f__findIpAddressesWithDhcp (
  IPL4asp__PortType::IPL4asp__PT & portRef,
  const CHARSTRING & expIfName,
  const CHARSTRING & expIfIpAddress,
  const CHARSTRING & exclIfIpAddress,
  const CHARSTRING & ethernetAddress,
  const INTEGER & leaseTime,
  const CHARSTRING & leaseFile,
  const INTEGER& nOfAddresses,
  Socket__API__Definitions::ro__charstring & ipAddresses,
  CHARSTRING & netMask,
  CHARSTRING & broadcastAddr,
  CHARSTRING & ifName )
{
  portRef.debug ( "f__findIpAddressesWithDhcp() begin" );
  debugLogEnabled = portRef.ipDiscConfig.debugAllowed;
  dhcpMsgRetransmitCount = portRef.ipDiscConfig.dhcpMsgRetransmitCount;
  dhcpMsgRetransmitPeriodInms = portRef.ipDiscConfig.dhcpMsgRetransmitPeriodInms;
  dhcpMaxParallelRequestCount = portRef.ipDiscConfig.dhcpMaxParallelRequestCount;
  dhcpTimeout = portRef.ipDiscConfig.dhcpTimeout;
  if ( leaseTime <= 0 || nOfAddresses <= 0 ) {
    TTCN_warning ( "f__findIpAddressesWithDhcp(): Parameter Error" );
    return FALSE;
  }
  EtherAddr etherAddr ( 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 );
  if ( cCTS ( ethernetAddress ) ) {
    if ( ! strToEtherAddr ( ethernetAddress, etherAddr ) ) {
      TTCN_warning ( "f__findIpAddressesWithDhcp(): ethernetAddress is invalid" );
      return FALSE;
    }
  }
  ipAddresses.set_size ( nOfAddresses );
  IPAddr * ipAddrs = new IPAddr[nOfAddresses];
  if ( ipAddrs == 0 ) {
    TTCN_warning ( "f__findIpAddressesWithDhcp(): Memory allocation error" );
    return FALSE;
  }
  EtherIf   etherIf ( cCTS ( expIfName ),
    cCTS ( expIfIpAddress ), cCTS ( exclIfIpAddress ) );
  if ( ! etherIf.valid ) {
    TTCN_warning ( "f__findIpAddressesWithDhcp(): Interface not found" );
    return FALSE;
  }
  portRef.ipAddrLease.ifName = etherIf.name;
  if ( cCTS ( leaseFile ) )
    portRef.ipAddrLease.leaseFile = leaseFile;
  if ( ! cCTS ( ethernetAddress ) ) {
    unsigned int  macRnd = ( etherIf.hwAddr.bytes[3] << 16 ) +
                           ( etherIf.hwAddr.bytes[4] << 8 ) +
                           etherIf.hwAddr.bytes[5];
    unsigned int rPid = 0;
    unsigned int pid = getpid ();
    for ( unsigned int i = 0; i < 23; ++i ) {
      rPid <<= 1; rPid += ( pid & 1 ); pid >>= 1;
    }
    macRnd += rPid;
    macRnd &= 0x7FFFFF;
    etherAddr.bytes[3] = (unsigned char) ( macRnd >> 16 );
    etherAddr.bytes[4] = (unsigned char) ( macRnd >> 8 );
    etherAddr.bytes[5] = (unsigned char) macRnd;
  }
  IPAddr  nMask;
  IPAddr  router;
  bool res = requestIpAddressesWithDhcp ( etherIf.name, etherAddr,
    nOfAddresses, ipAddrs, leaseTime, nMask, router, cCTS ( leaseFile ) );
  if ( res ) {
    for ( int i = 0; i < nOfAddresses; ++i ) {
      ipAddresses[i] = ipAddrs[i].asStr ();
    }
    netMask = nMask.asStr ();
    IPAddr bCAddr = router;
    for ( unsigned i = 0; i < bCAddr.len; ++i ) 
      bCAddr.bytes[i] |= ~nMask.bytes[i];
    broadcastAddr = bCAddr.asStr ();
    ifName = etherIf.name;
  } else {
    ipAddresses.set_size ( 0 );
    TTCN_warning ( "IPL4: f__findIpAddressesWithDhcp(): Could not find enough IP addresses" );
  }
  delete[] ipAddrs;
  portRef.debug ( "f__findIpAddressesWithDhcp() end, result: %i", res );
  return res ? TRUE : FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__releaseIpAddressesFromDhcp (
  IPL4asp__PortType::IPL4asp__PT & portRef )
{
  portRef.debug ( "f__releaseIpAddressesFromDhcp() begin" );
  debugLogEnabled = portRef.ipDiscConfig.debugAllowed;
  dhcpMsgRetransmitCount = portRef.ipDiscConfig.dhcpMsgRetransmitCount;
  dhcpMsgRetransmitPeriodInms = portRef.ipDiscConfig.dhcpMsgRetransmitPeriodInms;
  dhcpMaxParallelRequestCount = portRef.ipDiscConfig.dhcpMaxParallelRequestCount;
  dhcpTimeout = portRef.ipDiscConfig.dhcpTimeout;
  bool res = true;
  if ( cCTS ( portRef.ipAddrLease.ifName ) != 0 )
    res = releaseIpAddresses ( portRef.ipAddrLease.ifName,
      portRef.ipAddrLease.leaseFile );
  if ( ! res )
    TTCN_warning ( "IPL4: f__releaseIpAddressesFromDhcp(): Error while releaseing IP addresses" );
  portRef.ipAddrLease.ifName = "";
  portRef.debug ( "f__releaseIpAddressesFromDhcp() end, result: %i", res );
  return res ? TRUE : FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__findIpAddressesWithARP (
  IPL4asp__PortType::IPL4asp__PT & portRef,
  const CHARSTRING& expIfName,
  const CHARSTRING& expIfIpAddress,
  const CHARSTRING& exclIfIpAddress,
  const INTEGER& nOfAddresses,
  Socket__API__Definitions::ro__charstring & ipAddresses,
  CHARSTRING & netMask,
  CHARSTRING & broadcastAddr,
  CHARSTRING & ifName )
{
  portRef.debug ( "f__findIpAddressesWithARP() begin" );
  debugLogEnabled = portRef.ipDiscConfig.debugAllowed;
  arpMsgRetransmitCount = portRef.ipDiscConfig.arpMsgRetransmitCount;
  arpMsgRetransmitPeriodInms = portRef.ipDiscConfig.arpMsgRetransmitPeriodInms;
  arpMaxParallelRequestCount = portRef.ipDiscConfig.arpMaxParallelRequestCount;
  if ( nOfAddresses <= 0 ) {
    TTCN_warning ( "f__findIpAddressesWithARP(): Parameter Error" );
    return FALSE;
  }
  ipAddresses.set_size ( nOfAddresses );
  IPAddr * ipAddrs = new IPAddr[nOfAddresses];
  if ( ipAddrs == 0 ) {
    TTCN_warning ( "f__findIpAddressesWithARP(): Memory allocation error" );
    return FALSE;
  }
  EtherIf   etherIf ( cCTS ( expIfName ),
    cCTS ( expIfIpAddress ), cCTS ( exclIfIpAddress ) );
  if ( ! etherIf.valid ) {
    TTCN_warning ( "f__findIpAddressesWithARP(): Interface not found" );
    return FALSE;
  }
  bool res = createIpAddrsWithARP ( etherIf.name, etherIf.hwAddr,
    etherIf.realIpAddr, nOfAddresses, ipAddrs, etherIf.netMask );
  if ( res ) {
    for ( int i = 0; i < nOfAddresses; ++i ) {
      ipAddresses[i] = ipAddrs[i].asStr ();
    }
    netMask = etherIf.netMask.asStr ();
    IPAddr bCAddr = ipAddrs[0];
    for ( unsigned i = 0; i < bCAddr.len; ++i ) 
      bCAddr.bytes[i] |= ~etherIf.netMask.bytes[i];
    broadcastAddr = bCAddr.asStr ();
    ifName = etherIf.name;
  } else {
    ipAddresses.set_size ( 0 );
    TTCN_warning ( "IPL4: f__findIpAddressesWithARP(): Could not find enough IP addresses" );
  }
  delete[] ipAddrs;
  portRef.debug ( "f__findIpAddressesWithARP() end, result: %i", res );
  return res ? TRUE : FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__findIpAddresses (
  IPL4asp__PortType::IPL4asp__PT & portRef,
  Socket__API__Definitions::ro__charstring & ipAddresses,
  CHARSTRING & netMask,
  CHARSTRING & broadcastAddr,
  CHARSTRING & ifName )
{
  portRef.debug ( "f__findIpAddresses() begin" );
  BOOLEAN res = FALSE;
  if ( portRef.ipDiscConfig.type == IPL4asp__PortType::IPDiscConfig::DHCP ||
       portRef.ipDiscConfig.type == IPL4asp__PortType::IPDiscConfig::DHCP_OR_ARP )
    res = IPL4asp__Functions::f__findIpAddressesWithDhcp (
      portRef,
      portRef.ipDiscConfig.expIfName,
      portRef.ipDiscConfig.expIfIpAddress,
      portRef.ipDiscConfig.exclIfIpAddress,
      portRef.ipDiscConfig.ethernetAddress,
      portRef.ipDiscConfig.leaseTime,
      portRef.ipDiscConfig.leaseFile,
      portRef.ipDiscConfig.nOfAddresses,
      ipAddresses, netMask, broadcastAddr, ifName );
  if ( portRef.ipDiscConfig.type == IPL4asp__PortType::IPDiscConfig::ARP ||
       ( portRef.ipDiscConfig.type == IPL4asp__PortType::IPDiscConfig::DHCP_OR_ARP
         && res != TRUE ) )
    res = IPL4asp__Functions::f__findIpAddressesWithARP (
      portRef,
      portRef.ipDiscConfig.expIfName,
      portRef.ipDiscConfig.expIfIpAddress,
      portRef.ipDiscConfig.exclIfIpAddress,
      portRef.ipDiscConfig.nOfAddresses,
      ipAddresses, netMask, broadcastAddr, ifName );
  if ( ! res )
    TTCN_warning ( "IPL4: f__findIpAddresses(): Could not find enough IP addresses" );
  portRef.debug ( "f__findIpAddresses() end, result: %i", (bool) res );
  return res;
}

#else //IP_AUTOCONFIG

extern BOOLEAN IPL4asp__Functions::f__findIpAddressesWithDhcp (
  IPL4asp__PortType::IPL4asp__PT & , const CHARSTRING & , const CHARSTRING & ,
  const CHARSTRING & , const CHARSTRING & , const INTEGER & ,
  const CHARSTRING & , const INTEGER& , Socket__API__Definitions::ro__charstring & ,
  CHARSTRING & , CHARSTRING & , CHARSTRING & )
{
  TTCN_warning ( "IPL4: f__findIpAddressesWithDhcp(): IP discovery not available" );
  return FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__releaseIpAddressesFromDhcp (
  IPL4asp__PortType::IPL4asp__PT &  )
{
  TTCN_warning ( "IPL4: f__releaseIpAddressesFromDhcp(): IP discovery not available" );
  return FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__findIpAddressesWithARP (
  IPL4asp__PortType::IPL4asp__PT & , const CHARSTRING& , const CHARSTRING& ,
  const CHARSTRING& , const INTEGER& , Socket__API__Definitions::ro__charstring & ,
  CHARSTRING & , CHARSTRING & , CHARSTRING & )
{
  TTCN_warning ( "IPL4: f__findIpAddressesWithARP(): IP discovery not available" );
  return FALSE;
}

extern BOOLEAN IPL4asp__Functions::f__findIpAddresses (
  IPL4asp__PortType::IPL4asp__PT & , Socket__API__Definitions::ro__charstring & ,
  CHARSTRING & , CHARSTRING & , CHARSTRING & )
{
  TTCN_warning ( "f__findIpAddresses(): IP discovery not available" );
  return FALSE;
}

#endif //IP_AUTOCONFIG

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
//  File:               IPL4asp_protocolL234.hh
//  Rev:                R25B
//  Prodnr:             CNL 113 531
//  Updated:            2008-01-30
//  Contact:            http://ttcn.ericsson.se
//  Reference:          RFC 791, RFC 792, RFC 768
///////////////////////////////////////////////


/*
  This file contains structures for protocol layer 2, 3 and 4 messages:
    Ethernet, ARP, IP, ICMP, UDP;
  basic filling, checking and related functions and methods as inlines.
*/


#include <string.h>     // memcpy, memcmp
#include <arpa/inet.h>  // ntohs, htons

namespace IPL4asp__PortType {

/*
#if __BYTE_ORDER == __LITTLE_ENDIAN
inline unsigned short ntoh2 ( unsigned short a) {
  return ( ( a << 8 ) & 0xFF00 ) | ( ( a >> 8 ) & 0x00FF );
}
inline unsigned int ntoh4 ( unsigned int a) {
  return ( ( a << 24 ) & 0xFF000000 ) |
         ( ( a <<  8 ) & 0x00FF0000 ) |
         ( ( a >>  8 ) & 0x0000FF00 ) |
         ( ( a >> 24 ) & 0x000000FF );
}
#else
inline unsigned short ntoh2 ( unsigned short a) { return a; }
inline unsigned int ntoh4 ( unsigned int a) { return a; }
#endif
inline unsigned short hton2 ( unsigned short a) { return ntoh2 ( a ); }
inline unsigned int hton4 ( unsigned int a) { return ntoh4 ( a ); }
*/

// Ethernet address structure
struct EtherAddr
{
  unsigned char   bytes[6];
  inline unsigned int copyTo ( unsigned char * buf ) const {
    * (unsigned int*) buf = * (unsigned int*) bytes;
    * (unsigned short*) ( buf + 4 ) = * (unsigned short*) ( bytes + 4 );
    return 6;
  }
  inline EtherAddr ( unsigned int b0, unsigned int b1, unsigned int b2,
    unsigned int b3, unsigned int b4, unsigned int b5 ) {
    bytes[0] = b0; bytes[1] = b1; bytes[2] = b2;
    bytes[3] = b3; bytes[4] = b4; bytes[5] = b5;
  }
  inline EtherAddr ( unsigned char * buf, unsigned int len ) {
    (void) len;
    * (unsigned int*) bytes = * (unsigned int*) buf;
    * (unsigned short*) ( bytes + 4 ) = * (unsigned short*) ( buf + 4 );
  }
  inline EtherAddr () {}
  const char * asStr() const;
} __attribute__ ((__packed__));

inline bool operator == ( const EtherAddr & aL, const EtherAddr & aR ) {
  return * (unsigned int*) aL.bytes == * (unsigned int*) aR.bytes &&
    * (unsigned short*) ( aL.bytes + 4 ) == * (unsigned short*) ( aR.bytes + 4 );
}

static const EtherAddr ETHER_BC_ADDR ( 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF );

static const EtherAddr ETHER_ADDR_ZERO ( 0, 0, 0, 0, 0, 0 );


// Ethernet header structure
struct EtherHdr
{
  EtherAddr       dstAddr;
  EtherAddr       srcAddr;
  unsigned char   type[2];
  enum { IP = 0x0800, ARP = 0x0806 };
  enum { LENGTH = 14 };
  inline void setIP ( const EtherAddr & srcA,
                      const EtherAddr & dstA = ETHER_BC_ADDR ) {
    dstAddr = dstA; srcAddr = srcA;
    type[0] = IP >> 8; type[1] = IP & 0xFF;
  }
  inline void setARP ( const EtherAddr & srcA,
                       const EtherAddr & dstA = ETHER_BC_ADDR ) {
    dstAddr = dstA; srcAddr = srcA;
    type[0] = ARP >> 8; type[1] = ARP & 0xFF;
  }
  inline unsigned short getType () const {
    unsigned short t = ( type[0] << 8 ) + type[1];
    return t;
  }
  static inline const EtherHdr * check ( const unsigned char * packet,
                                         unsigned int len ) {
    return ( len >= LENGTH ) ? (const EtherHdr*) packet : 0;
  }
} __attribute__ ((__packed__));



// IP address structures
struct IPv6Addr {
  unsigned char bytes[16];
};

struct IPAddr {
  union {
    unsigned int  v4;
    IPv6Addr      v6;
    unsigned char bytes[16];
  };
  unsigned int  len;
  inline unsigned int copyTo ( unsigned char * buf ) const {
    memcpy ( buf, this, len );
    return len;
  }
  inline IPAddr () : v4 ( 0 ), len ( 0 ) {}
  inline IPAddr ( unsigned char * buf, unsigned int length ) : len ( length ) {
    memcpy ( bytes, buf, length ); }
  inline IPAddr ( const char * str ) : len ( 0 ) {
    if ( str != 0 )
      len = inet_aton ( str, (struct in_addr*) this ) ? 4 : 0; // TODO: IPv6
  }
  inline void set ( unsigned int ipv4Addr ) { v4 = ipv4Addr; len = 4; }
  inline void set ( const IPv6Addr & ipv6Addr ) { v6 = ipv6Addr; len = 16; }
  inline void clear () { v4 = 0; len = 0; }
  const char * asStr() const;
};

inline bool operator == ( const IPAddr & aL, const IPAddr & aR ) {
  if ( aL.len != aR.len ) return false;
  if ( aL.len == 4 ) return aL.v4 == aR.v4;
  if ( aL.len == 16 ) return memcmp ( aL.v6.bytes, aR.v6.bytes, 16 ) == 0;
  return true;
}
inline bool operator != ( const IPAddr & aL, const IPAddr & aR ) {
  return ! ( aL == aR );
}
inline bool operator == ( const IPAddr & aL, unsigned int aR ) {
  return ( aL.len == 4 && aL.v4 == aR );
}
inline bool operator != ( const IPAddr & aL, unsigned int aR ) {
  return ! ( aL == aR );
}
inline bool operator == ( unsigned int aL, const IPAddr & aR ) {
  return ( aR.len == 4 && aL == aR.v4 );
}
inline bool operator != ( unsigned int aL, const IPAddr & aR ) {
  return ! ( aL == aR );
}

const char * ipv4ToStr ( unsigned int ipv4Addr );

inline IPAddr ipv4ToIPAddr ( unsigned int ipv4Addr ) {
  IPAddr ipAddr;
  ipAddr.set ( ipv4Addr );
  return ipAddr;
}

// Default contents of the ARP header (IPv4 and Request)
struct Raw4Bytes { unsigned char bytes[4]; };
static const Raw4Bytes ARPV4_HDR_CONST = { { 0, 1, 8, 0 } };

// ARP header structure
struct ARPHdr
{
  unsigned short  hType;
  unsigned short  pType;
  unsigned char   hLen;
  unsigned char   pLen;
  unsigned char   operation[2];
  enum { REQUEST = 1, REPLY = 2 };
  enum { LENGTH = 8 };
  inline void set ( unsigned char op = REQUEST, unsigned int ipLen = 4 ) {
    * (Raw4Bytes*) this = ARPV4_HDR_CONST;
    hLen = 6;
    pLen = ipLen;
    operation[0] = 0;
    operation[1] = op;
  }
  static inline const ARPHdr * check ( const unsigned char * arpMsg,
                                       unsigned int len ) {
    if ( len < LENGTH ) return 0;
    const ARPHdr * hdr = (const ARPHdr*) arpMsg;
    return ( len >= LENGTH &&
             * (unsigned int*) arpMsg == * (unsigned int*) &ARPV4_HDR_CONST &&
             hdr->hLen == 6 && hdr->operation[0] == 0 &&
             ( hdr->pLen == 4 || hdr->pLen == 16 ) &&
             ( hdr->operation[1] == REQUEST || hdr->operation[1] == REPLY ) )
            ? hdr : 0;
  }
} __attribute__ ((__packed__));

struct ARPv4Payload
{
  EtherAddr     sha;
  unsigned int  spa;
  EtherAddr     tha;
  unsigned int  tpa;
  enum { LENGTH = 20 };
  static inline const ARPv4Payload * check ( const unsigned char * msg,
                                             unsigned int len ) {
    return ( len >= LENGTH ) ? (const ARPv4Payload*) msg : 0;
  }
} __attribute__ ((__packed__));

struct ARPv6Payload
{
  EtherAddr     sha;
  IPv6Addr      spa;
  EtherAddr     tha;
  IPv6Addr      tpa;
  enum { LENGTH = 44 };
  static inline const ARPv6Payload * check ( const unsigned char * msg,
                                             unsigned int len ) {
    return ( len >= LENGTH ) ? (const ARPv6Payload*) msg : 0;
  }
} __attribute__ ((__packed__));

class ARPMsgOut
{
public:
  enum { BUFFER_SIZE = EtherHdr::LENGTH + ARPHdr::LENGTH + ARPv6Payload::LENGTH };
     /* BUFFER_SIZE = 66 */
  static inline unsigned int create ( unsigned char * msgOut,
    const EtherAddr & srcEtherAddr, const IPAddr & srcIPAddr,
    const EtherAddr & dstEtherAddr, const IPAddr & dstIPAddr,
    unsigned char op ) {
    unsigned int n;
    ( (EtherHdr*) msgOut )->setARP ( srcEtherAddr ); n = EtherHdr::LENGTH;
    ( (ARPHdr*) ( msgOut + n ) )->set ( op, dstIPAddr.len ); n += ARPHdr::LENGTH;
    n += srcEtherAddr.copyTo ( msgOut + n );
    n += srcIPAddr.copyTo ( msgOut + n );
    n += dstEtherAddr.copyTo ( msgOut + n );
    n += dstIPAddr.copyTo ( msgOut + n );
    return n;
  }
};


// 16 bit one's complement checksum calculation
// It is used in the IP header, in ICMP meessages and in UDP messages
inline unsigned short onesChkSum (
  const unsigned short * data, unsigned int len, unsigned int chkSum = 0 ) {
  for ( ; len >= 2; len -= 2, ++data )
    chkSum += ntohs ( *data );
  if ( len != 0 )
    chkSum += ( * (unsigned char*) data ) << 8;
  while ( ( chkSum & 0xFFFF0000 ) != 0 )
    chkSum = ( chkSum & 0xFFFF ) + ( chkSum >> 16 );
  return (unsigned short) chkSum;
}


// IPv4 header structure (fixed part)
struct IPv4Hdr
{
  unsigned char   verAndHdrLen;     // bits 0..3: version, 4..7: IHL
  unsigned char   tos;
  unsigned short  totalLength;
  unsigned short  id;
  unsigned short  flagsAndOffset;   // bit 1: DF, 2: MF, 3..15: fragment offset
  unsigned char   ttl;
  unsigned char   protocol;
  unsigned short  headerCheckSum;
  unsigned int    srcAddr;
  unsigned int    dstAddr;
  enum { ICMP = 1, TCP = 6, UDP = 17, SCTP = 132 };
  enum { LENGTH = 20 };
  inline void setDef ( unsigned char proto,
                       unsigned int srcA, unsigned int dstA ) {
    verAndHdrLen = 0x45; tos = 0; totalLength = 0 /* set later */;
    id = 0; flagsAndOffset = 0;
    ttl = 64; protocol = proto; headerCheckSum = 0 /* set later */;
    srcAddr = srcA;
    dstAddr = dstA;
  }
  inline void setUDP ( unsigned int srcA, unsigned int dstA ) {
    setDef ( UDP, srcA, dstA ); }
  inline void setTCP ( unsigned int srcA, unsigned int dstA ) {
    setDef ( TCP, srcA, dstA ); }
  inline void setLength ( unsigned int len ) { totalLength = htons ( len ); }
  inline void setCheckSum ( unsigned short chkSum ) {
    headerCheckSum = htons ( ~chkSum );
  }
  inline void setDF () { *(unsigned char*) &flagsAndOffset |= 0x40; }
  inline void setMF () { *(unsigned char*) &flagsAndOffset |= 0x20; }
  inline unsigned int hdrLen () const { return ( verAndHdrLen & 0x0F ) * 4; }
  inline unsigned int payloadLen () const {
    return ntohs ( totalLength ) - hdrLen (); }
  inline bool isDFSet () const {
    return ( *(unsigned char*) &flagsAndOffset & 0x40 ) != 0;
  }
  inline bool isMFSet () const {
    return ( *(unsigned char*) &flagsAndOffset & 0x20 ) != 0;
  }
  inline unsigned int getOffset () const {
    return ntohs ( flagsAndOffset ) & 0x1FFFF;
  }
  inline const unsigned char * raw () const { return (const unsigned char*) this; }
  static inline unsigned short calcCheckSum ( const unsigned char * ipMsg ) {
    const IPv4Hdr * hdr = (const IPv4Hdr *) ipMsg;
    return onesChkSum ( (const unsigned short*) ipMsg, hdr->hdrLen() );
  }
  static inline const IPv4Hdr * check ( const unsigned char * ipMsg,
                                        unsigned int len ) {
    if ( len < LENGTH ) return 0;
    const IPv4Hdr * hdr = (const IPv4Hdr *) ipMsg;
    if ( ( hdr->verAndHdrLen >> 4 ) != 4 ) return 0;
    unsigned int ihl = hdr->verAndHdrLen & 0x0F;
    if ( ihl < 5 || len < ihl * 4 ) return 0;
    if ( hdr->protocol != ICMP && hdr->protocol != UDP && hdr->protocol != TCP )
      return 0;
    if ( ntohs ( hdr->totalLength ) > len ) return 0;
    unsigned short chkSum = calcCheckSum ( ipMsg );
    if ( chkSum != 0 && chkSum != 0xFFFF ) return 0;
    return hdr;
  }
} __attribute__ ((__packed__));


// ICMPv4 header structure (together with used ICMP messages)
struct ICMPv4Hdr
{
  unsigned char   type;
  unsigned char   code;
  unsigned short  checkSum;
  enum { ECHO_REPLY = 0, ECHO_REQUEST = 8, ROUTER_ADVERTISEMENT = 9,
    ROUTER_SOLICITATION = 10, ADDRESS_MASK_REQUEST = 17, ADDRESS_MASK_REPLY = 18
  };
  enum { LENGTH = 4 };
  inline void setCheckSum ( unsigned short chkSum ) {
    checkSum = htons ( ~chkSum );
  }
  static inline unsigned short calcCheckSum ( const unsigned char * icmpMsg,
                                              const unsigned int len ) {
    return onesChkSum ( (const unsigned short*) icmpMsg, len );
  }
  static inline const ICMPv4Hdr * check ( const unsigned char * icmpMsg,
                                          unsigned int len ) {
    if ( len < LENGTH ) return 0;
    unsigned short chkSum =  calcCheckSum ( icmpMsg, len );
    if ( chkSum != 0 && chkSum != 0xFFFF ) return 0;
    return (const ICMPv4Hdr*) icmpMsg;
  }
} __attribute__ ((__packed__));

struct ICMPv4Echo
{
  unsigned short  id;
  unsigned short  seqNum;
  enum { LENGTH = 4 };
  static inline const ICMPv4Echo * check ( const unsigned char * msg,
                                           unsigned int len ) {
    return ( len >= LENGTH ) ? (const ICMPv4Echo*) msg : 0;
  }
} __attribute__ ((__packed__));

struct ICMPv4RouterAdvEntry
{
  unsigned int    ipAddr;
  unsigned int    prefLvl;
} __attribute__ ((__packed__));

struct ICMPv4RouterAdv
{
  unsigned char   nAddrs;
  unsigned char   entrySize;
  unsigned short  lifetime;
  ICMPv4RouterAdvEntry entry[1];
  static inline const ICMPv4RouterAdv * check ( const unsigned char * msg,
                                                unsigned int len ) {
    if ( len < 4 ) return 0;
    const ICMPv4RouterAdv * adv = (const ICMPv4RouterAdv*) msg;
    return ( adv->entrySize == 2U && len >= 4U + adv->nAddrs * 8U ) ? adv : 0;
  }
} __attribute__ ((__packed__));


// UDP header structure
struct UDPHdr
{
  unsigned short  srcPort;
  unsigned short  dstPort;
  unsigned short  length;
  unsigned short  checkSum;
  enum { LENGTH = 8 };
  inline void set ( unsigned int srcP, unsigned int dstP ) {
    srcPort = htons ( srcP ); dstPort = htons ( dstP );
    length = 0; checkSum = 0;
  }
  inline void setLength ( unsigned int len ) { length = htons ( len ); }
  inline void setCheckSum ( unsigned short chkSum ) {
    if ( chkSum != 0xFFFF ) chkSum = ~chkSum;
    checkSum = htons ( chkSum );
  }
  inline const unsigned char * raw () const { return (const unsigned char*) this; }
  static inline unsigned short calcCheckSum ( const unsigned char * udpMsg,
                                              const IPv4Hdr & ipv4Hdr ) {
    unsigned char * sA = (unsigned char*) &ipv4Hdr.srcAddr;
    unsigned char * dA = (unsigned char*) &ipv4Hdr.dstAddr;
    unsigned int udpLength = ( udpMsg[4] << 8 ) + udpMsg[5];
    unsigned int chkSum = ( sA[0] << 8 ) + sA[1] + ( sA[2] << 8 ) + sA[3] +
                          ( dA[0] << 8 ) + dA[1] + ( dA[2] << 8 ) + dA[3] +
                          IPv4Hdr::UDP + udpLength;
    return onesChkSum ( (const unsigned short*) udpMsg, udpLength, chkSum );
  }
  static inline const UDPHdr * check ( const unsigned char * udpMsg,
                                       const IPv4Hdr & ipv4Hdr ) {
    unsigned int len = ipv4Hdr.payloadLen ();
    if ( len < LENGTH ) return 0;
    const UDPHdr * hdr = (const UDPHdr*) udpMsg;
    if ( ntohs ( hdr->length ) != len ) return 0;
    if ( hdr->checkSum != 0 ) {
      unsigned short checkSum = calcCheckSum ( udpMsg, ipv4Hdr );
      if ( checkSum != 0 && checkSum != 0xFFFF ) {
        return 0;
      }
    }
    return hdr;
  }
} __attribute__ ((__packed__));

} /* namespace IPL4asp__PortType */

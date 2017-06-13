/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "NetworkHandler.hh"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "memory.h"
#include "../core/Textbuf.hh"

IPAddress::~IPAddress() { }

NetworkHandler::NetworkHandler() : m_family(ipv4) { }

void NetworkHandler::set_family(const char *p_addr)
{
  if (!p_addr || IPv4Address::is_valid(p_addr)) m_family = ipv4;
#if defined(LINUX) || defined(CYGWIN17)
  else if (IPv6Address::is_valid(p_addr)) m_family = ipv6;
#endif
  else m_family = ipv0;
}

int NetworkHandler::socket()
{
  return socket(m_family);
}

int NetworkHandler::socket(const NetworkFamily& p_family)
{
#if defined(LINUX) || defined(CYGWIN17)
  return ::socket(p_family == ipv4 ? AF_INET : AF_INET6, SOCK_STREAM, 0);
#else
  (void)p_family;
  return ::socket(AF_INET, SOCK_STREAM, 0);
#endif
}

HCNetworkHandler::HCNetworkHandler() : m_mc_addr(NULL), m_local_addr(NULL) { }

HCNetworkHandler::~HCNetworkHandler()
{
  delete m_mc_addr;
  delete m_local_addr;
}

bool HCNetworkHandler::set_local_addr(const char *p_addr, unsigned short p_port)
{
  if (!p_addr) return false;
  switch (m_family) {
  case ipv4: m_local_addr = new IPv4Address(p_addr, p_port); break;
#if defined(LINUX) || defined(CYGWIN17)
  case ipv6: m_local_addr = new IPv6Address(p_addr, p_port); break;
#endif
  default:
    break;
  }
  return m_local_addr != NULL;
}

bool HCNetworkHandler::set_mc_addr(const char *p_addr, unsigned short p_port)
{
  if (!p_addr) return false;
  switch (m_family) {
  case ipv4: m_mc_addr = new IPv4Address(p_addr, p_port); break;
#if defined(LINUX) || defined(CYGWIN17)
  case ipv6: m_mc_addr = new IPv6Address(p_addr, p_port); break;
#endif
  default:
    break;
  }
  return m_mc_addr != NULL;
}

int HCNetworkHandler::getsockname_local_addr(int p_sockfd)
{
  if (m_local_addr != NULL) delete m_local_addr;
  switch (m_family) {
  case ipv4: m_local_addr = new IPv4Address(); break;
#if defined(LINUX) || defined(CYGWIN17)
  case ipv6: m_local_addr = new IPv6Address(); break;
#endif
  default: return -1;
  }
  return m_local_addr->getsockname(p_sockfd);
}

int HCNetworkHandler::bind_local_addr(int p_sockfd) const
{
  m_local_addr->set_port(0);
  return ::bind(p_sockfd, m_local_addr->get_addr(), m_local_addr->get_addr_len());
}

#ifdef SOLARIS
#define CAST_AWAY_CONST_FOR_SOLARIS6 (sockaddr *)
#else
#define CAST_AWAY_CONST_FOR_SOLARIS6
#endif

int HCNetworkHandler::connect_to_mc(int p_sockfd) const
{
  return ::connect(p_sockfd, CAST_AWAY_CONST_FOR_SOLARIS6 m_mc_addr->get_addr(),
    m_mc_addr->get_addr_len());
}

IPAddress *IPAddress::create_addr(const NetworkFamily& p_family)
{
  switch (p_family) {
  case ipv4:
    return new IPv4Address();
#if defined(LINUX) || defined(CYGWIN17)
  case ipv6:
    return new IPv6Address();
#endif
  default:
    return NULL;
  }
}

IPAddress *IPAddress::create_addr(const char *p_addr)
{
  if (p_addr == NULL)
    return NULL;
  if (IPv4Address::is_valid(p_addr))
    return new IPv4Address(p_addr);
#if defined(LINUX) || defined(CYGWIN17)
  else if (IPv6Address::is_valid(p_addr))
    return new IPv6Address(p_addr);
#endif
  else
    return NULL;
}

IPv4Address::IPv4Address()
{
  clean_up();
  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  m_addr.sin_port = htons(0);
}

IPv4Address::IPv4Address(const char *p_addr, unsigned short p_port)
{
  set_addr(p_addr, p_port);
}

IPv4Address::~IPv4Address()
{
  clean_up();
}

bool IPv4Address::is_valid(const char *p_addr)
{
  if (p_addr == NULL) {
    return false;
  } else {
    IPv4Address addr;
    return addr.set_addr(p_addr);
  }
}

bool IPv4Address::is_local() const
{
  // Or: (ntohl(m_addr.sin_addr.s_addr) & 0xff000000u) == 0x7f000000u.
  return m_addr.sin_addr.s_addr == inet_addr("127.0.0.1");
}

bool IPv4Address::set_addr(const char *p_addr, unsigned short p_port)
{
  clean_up();
  if (p_addr != NULL) {
    struct hostent *hptr = gethostbyname(p_addr);
    if (hptr != NULL && static_cast<size_t>(hptr->h_length) == sizeof(struct in_addr)) {
      m_addr.sin_family = AF_INET;
      m_addr.sin_port = htons(p_port);
      memset(m_addr.sin_zero, 0, sizeof(m_addr.sin_zero));
      memcpy(&m_addr.sin_addr, hptr->h_addr_list[0], static_cast<size_t>(hptr->h_length));
      strncpy(m_addr_str, inet_ntoa(m_addr.sin_addr), sizeof(m_addr_str));
      strncpy(m_host_str, hptr->h_name, sizeof(m_host_str));
      return true;
    }
  }
  return false;
}

int IPv4Address::accept(int p_sockfd)
{
  clean_up();
  socklen_type addrlen = sizeof(m_addr);
  int fd = ::accept(p_sockfd, reinterpret_cast<struct sockaddr *>(&m_addr), &addrlen);
  if (fd >= 0) {
    strncpy(m_addr_str, inet_ntoa(m_addr.sin_addr), sizeof(m_addr_str));
    if (m_addr.sin_addr.s_addr != htonl(INADDR_ANY)) {
      struct hostent *hptr =
        gethostbyaddr(reinterpret_cast<const char *>(&m_addr.sin_addr),
                      sizeof(m_addr.sin_addr), m_addr.sin_family);
      if (hptr != NULL && static_cast<size_t>(hptr->h_length) == sizeof(struct in_addr)) {
        strncpy(m_host_str, hptr->h_name, sizeof(m_host_str));
      }
    }
  }
  return fd;
}

int IPv4Address::getsockname(int p_sockfd)
{
  clean_up();
  socklen_type addrlen = sizeof(m_addr);
  int s = ::getsockname(p_sockfd, reinterpret_cast<struct sockaddr *>(&m_addr), &addrlen);
  if (s >= 0) {
    strncpy(m_addr_str, inet_ntoa(m_addr.sin_addr), sizeof(m_addr_str));
    if (m_addr.sin_addr.s_addr != htonl(INADDR_ANY)) {
      struct hostent *hptr =
        gethostbyaddr(reinterpret_cast<const char *>(&m_addr.sin_addr),
                      sizeof(m_addr.sin_addr), m_addr.sin_family);
      if (hptr != NULL && static_cast<size_t>(hptr->h_length) == sizeof(struct in_addr)) {
        strncpy(m_host_str, hptr->h_name, sizeof(m_host_str));
      }
    }
  }
  return s;
}

bool IPv4Address::operator==(const IPAddress& p_addr) const
{
  return memcmp(&m_addr.sin_addr, &((static_cast<const IPv4Address&>(p_addr)).m_addr.sin_addr), sizeof(m_addr.sin_addr)) == 0;
}

bool IPv4Address::operator!=(const IPAddress& p_addr) const
{
  return !(*this == p_addr);
}

IPAddress& IPv4Address::operator=(const IPAddress& p_addr)
{
  clean_up();
  memcpy(&m_addr, &(static_cast<const IPv4Address&>(p_addr)).m_addr, sizeof(m_addr));
  strncpy(m_host_str, (static_cast<const IPv4Address&>(p_addr)).m_host_str, sizeof(m_host_str));
  strncpy(m_addr_str, (static_cast<const IPv4Address&>(p_addr)).m_addr_str, sizeof(m_addr_str));
  return *this;
}

void IPv4Address::push_raw(Text_Buf& p_buf) const
{
  p_buf.push_raw(sizeof(m_addr.sin_family), &m_addr.sin_family);
  p_buf.push_raw(sizeof(m_addr.sin_port), &m_addr.sin_port);
  p_buf.push_raw(sizeof(m_addr.sin_addr), &m_addr.sin_addr);
  p_buf.push_raw(sizeof(m_addr.sin_zero), &m_addr.sin_zero);
}

void IPv4Address::pull_raw(Text_Buf& p_buf)
{
  clean_up();
  p_buf.pull_raw(sizeof(m_addr.sin_family), &m_addr.sin_family);
  p_buf.pull_raw(sizeof(m_addr.sin_port), &m_addr.sin_port);
  p_buf.pull_raw(sizeof(m_addr.sin_addr), &m_addr.sin_addr);
  p_buf.pull_raw(sizeof(m_addr.sin_zero), &m_addr.sin_zero);
}

void IPv4Address::clean_up()
{
  memset(&m_addr, 0, sizeof(m_addr));
  memset(m_host_str, 0, sizeof(m_host_str));
  memset(m_addr_str, 0, sizeof(m_addr_str));
}

#if defined(LINUX) || defined(CYGWIN17)
IPv6Address::IPv6Address()
{
  clean_up();
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_addr = in6addr_any;
  m_addr.sin6_port = htons(0);
}

IPv6Address::IPv6Address(const char *p_addr, unsigned short p_port)
{
  set_addr(p_addr, p_port);
}

IPv6Address::~IPv6Address()
{
  clean_up();
}

const char *IPv6Address::get_addr_str() const
{
  if (strlen(m_addr_str) > 0) {
    // The host name should always contain the interface's name for link-local addresses.
    return (strlen(m_host_str) > 0 && strstr(m_host_str, "%") != NULL) ? m_host_str : m_addr_str;
  } else {
    return m_host_str;
  }
}

bool IPv6Address::set_addr(const char *p_addr, unsigned short p_port)
{
  clean_up();
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_CANONNAME | AI_PASSIVE;
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;  // Any.
  char p_port_str[6];
  memset(p_port_str, 0, sizeof(p_port_str));
  snprintf(p_port_str, sizeof(p_port_str), "%u", p_port);
  int s = getaddrinfo(p_addr, p_port_str, &hints, &res);
  if (s == 0) {
    struct sockaddr_in6 *addr = static_cast<struct sockaddr_in6 *>(static_cast<void*>(res->ai_addr));
    // The (void*) shuts up the "cast increases required alignment" warning.
    // Hopefully, the res->ai_addr points to a properly aligned sockaddr_in6
    // and we won't have problems like these if we decide to support Sparc:
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=340384
    memcpy(&m_addr, addr, sizeof(m_addr));
    m_addr.sin6_port = htons(p_port);
    inet_ntop(res->ai_family, &(addr->sin6_addr), m_addr_str, sizeof(m_addr_str));
    strncpy(m_host_str, res->ai_canonname, sizeof(m_host_str));
    freeaddrinfo(res);
    return true;
  }
  return false;
}

int IPv6Address::accept(int p_sockfd)
{
  clean_up();
  socklen_type addrlen = sizeof(m_addr);
  int fd = ::accept(p_sockfd, reinterpret_cast<struct sockaddr *>(&m_addr), &addrlen);
  if (fd >= 0) {
    if (!inet_ntop(AF_INET6, &m_addr.sin6_addr, m_addr_str, sizeof(m_addr_str))) {
      fprintf(stderr, "IPv6Address::accept(): Unable to convert IPv6 address "
              "from binary to text form: %s\n", strerror(errno));
    }
    int s = getnameinfo(reinterpret_cast<struct sockaddr *>(&m_addr), sizeof(m_addr),
                        m_host_str, sizeof(m_host_str), NULL, 0, 0);
    if (s != 0) {
      fprintf(stderr, "IPv6Address::accept(): Address to name translation "
              "failed: %s\n", gai_strerror(s));
    }
  }
  return fd;
}

int IPv6Address::getsockname(int p_sockfd)
{
  clean_up();
  socklen_type addrlen = sizeof(m_addr);
  int s1 = ::getsockname(p_sockfd, reinterpret_cast<struct sockaddr *>(&m_addr), &addrlen);
  if (s1 >= 0) {
    if (!inet_ntop(AF_INET6, &m_addr.sin6_addr, m_addr_str, sizeof(m_addr_str))) {
      fprintf(stderr, "IPv6Address::getsockname(): Unable to convert IPv6 "
              "address from binary to text form: %s\n", strerror(errno));
    }
    int s2 = getnameinfo(reinterpret_cast<struct sockaddr *>(&m_addr), sizeof(m_addr),
                         m_host_str, sizeof(m_host_str), NULL, 0, 0);
    if (s2 != 0) {
      fprintf(stderr, "IPv6Address::getsockname(): Address to name "
              "translation failed: %s\n", gai_strerror(s2));
    }
  }
  return s1;
}

bool IPv6Address::operator==(const IPAddress& p_addr) const
{
  return memcmp(&m_addr.sin6_addr, &((static_cast<const IPv6Address&>(p_addr)).m_addr.sin6_addr), sizeof(m_addr.sin6_addr)) == 0;
}

bool IPv6Address::operator!=(const IPAddress& p_addr) const
{
  return !(*this == p_addr);
}

IPAddress& IPv6Address::operator=(const IPAddress& p_addr)
{
  clean_up();
  memcpy(&m_addr, &(static_cast<const IPv6Address&>(p_addr)).m_addr, sizeof(m_addr));
  strncpy(m_host_str, (static_cast<const IPv6Address&>(p_addr)).m_host_str, sizeof(m_host_str));
  strncpy(m_addr_str, (static_cast<const IPv6Address&>(p_addr)).m_addr_str, sizeof(m_addr_str));
  return *this;
}

void IPv6Address::push_raw(Text_Buf& p_buf) const
{
  p_buf.push_raw(sizeof(m_addr.sin6_family), &m_addr.sin6_family);
  p_buf.push_raw(sizeof(m_addr.sin6_port), &m_addr.sin6_port);
  p_buf.push_raw(sizeof(m_addr.sin6_flowinfo), &m_addr.sin6_flowinfo);
  p_buf.push_raw(sizeof(m_addr.sin6_addr), &m_addr.sin6_addr);
  p_buf.push_raw(sizeof(m_addr.sin6_scope_id), &m_addr.sin6_scope_id);
}

void IPv6Address::pull_raw(Text_Buf& p_buf)
{
  clean_up();
  p_buf.pull_raw(sizeof(m_addr.sin6_family), &m_addr.sin6_family);
  p_buf.pull_raw(sizeof(m_addr.sin6_port), &m_addr.sin6_port);
  p_buf.pull_raw(sizeof(m_addr.sin6_flowinfo), &m_addr.sin6_flowinfo);
  p_buf.pull_raw(sizeof(m_addr.sin6_addr), &m_addr.sin6_addr);
  p_buf.pull_raw(sizeof(m_addr.sin6_scope_id), &m_addr.sin6_scope_id);
}

void IPv6Address::clean_up()
{
  memset(&m_addr, 0, sizeof(m_addr));
  memset(m_host_str, 0, sizeof(m_host_str));
  memset(m_addr_str, 0, sizeof(m_addr_str));
}

bool IPv6Address::is_local() const
{
  const unsigned char localhost_bytes[] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  const unsigned char mapped_ipv4_localhost[] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };

  return (memcmp(m_addr.sin6_addr.s6_addr, localhost_bytes, 16) == 0
          || memcmp(m_addr.sin6_addr.s6_addr, mapped_ipv4_localhost, 16) == 0);
}

bool IPv6Address::is_valid(const char *p_addr)
{
  if (p_addr == NULL) {
    return false;
  } else {
    IPv6Address addr;
    return addr.set_addr(p_addr);
  }
}
#endif // LINUX || CYGWIN17

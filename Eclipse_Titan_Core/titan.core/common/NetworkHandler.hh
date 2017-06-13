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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef NETWORKHANDLER_H_
#define NETWORKHANDLER_H_

#include "platform.h"
// platform.h includes sys/socket.h
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#ifdef WIN32
#include <cygwin/version.h>

#if CYGWIN_VERSION_DLL_MAJOR >= 1007
#define CYGWIN17
#else
#define CYGWIN15
#endif

#endif

// For legacy (e.g. Solaris 6) systems.
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

typedef enum { ipv0 = -1, ipv4 = 0, ipv6 } NetworkFamily;

class Text_Buf;

class IPAddress;
class IPv4Address;
#if defined(LINUX) || defined(CYGWIN17)
class IPV6Address;
#endif
class NetworkHandler;
class HCNetworkHandler;

class IPAddress {
public:
  virtual ~IPAddress() = 0;
  static IPAddress *create_addr(const NetworkFamily& p_family);
  static IPAddress *create_addr(const char *p_addr);
  // Always return something.
  virtual const char *get_host_str() const = 0;
  virtual const char *get_addr_str() const = 0;
  virtual bool operator==(const IPAddress& p_addr) const = 0;
  virtual bool operator!=(const IPAddress& p_addr) const = 0;
  virtual IPAddress& operator=(const IPAddress& p_addr) = 0;
  // Encode and decode the address and the corresponding port for internal
  // communication. Used by connected ports.
  virtual void push_raw(Text_Buf& p_buf) const = 0;
  virtual void pull_raw(Text_Buf& p_buf) = 0;
  virtual void clean_up() = 0;
  virtual int accept(int p_sockfd) = 0;
  // Return the current address and port to which the socket is bound to.
  virtual int getsockname(int p_sockfd) = 0;
  virtual unsigned short get_port() const = 0;
  virtual void set_port(unsigned short p_port) = 0;
  virtual bool set_addr(const char *p_addr, unsigned short p_port = 0) = 0;
  virtual const struct sockaddr *get_addr() const = 0;
  virtual socklen_type get_addr_len() const = 0;
  virtual bool is_local() const = 0;
};

class IPv4Address : public IPAddress {
public:
  IPv4Address();
  // Does DNS lookup.
  IPv4Address(const char *p_addr, unsigned short p_port = 0 /* Any port. */);
  //IPv4Address(const IPv4Address& p_addr) = default;
  //There are no pointers, so the compiler generated copy is OK.
  ~IPv4Address();

  bool operator==(const IPAddress& p_addr) const;
  bool operator!=(const IPAddress& p_addr) const;
  IPAddress& operator=(const IPAddress& p_addr);
  void push_raw(Text_Buf& p_buf) const;
  void pull_raw(Text_Buf& p_buf);
  void clean_up();
  int accept(int p_sockfd);
  int getsockname(int p_sockfd);
  inline unsigned short get_port() const { return ntohs(m_addr.sin_port); }
  inline void set_port(unsigned short p_port) { m_addr.sin_port = htons(p_port); }
  bool set_addr(const char *p_addr, unsigned short p_port = 0);
  inline const struct sockaddr *get_addr() const { return reinterpret_cast<const struct sockaddr *>(&m_addr); }
  inline socklen_type get_addr_len() const { return sizeof(m_addr); }
  inline const char *get_host_str() const { return strlen(m_host_str) > 0 ? m_host_str : m_addr_str; }
  inline const char *get_addr_str() const { return strlen(m_addr_str) > 0 ? m_addr_str : m_host_str; }
  static bool is_valid(const char *p_addr);
  bool is_local() const;
private:
  sockaddr_in m_addr;
  char m_host_str[NI_MAXHOST]; // DNS name.
  char m_addr_str[INET_ADDRSTRLEN]; // Address in numeric format.
};

#if defined(LINUX) || defined(CYGWIN17)
class IPv6Address : public IPAddress {
public:
  IPv6Address();
  // Does DNS lookup.
  IPv6Address(const char *p_addr, unsigned short p_port = 0 /* Any port. */);
  //IPv6Address(const IPv6Address& p_addr) = default;
  //There are no pointers, so the compiler generated copy is OK.
  ~IPv6Address();

  bool operator==(const IPAddress& p_addr) const;
  bool operator!=(const IPAddress& p_addr) const;
  IPAddress& operator=(const IPAddress& p_addr);
  void push_raw(Text_Buf& p_buf) const;
  void pull_raw(Text_Buf& p_buf);
  void clean_up();
  int accept(int p_sockfd);
  int getsockname(int p_sockfd);
  inline unsigned short get_port() const { return ntohs(m_addr.sin6_port); }
  inline void set_port(unsigned short p_port) { m_addr.sin6_port = htons(p_port); }
  bool set_addr(const char *p_addr, unsigned short p_port = 0);
  inline const struct sockaddr *get_addr() const { return reinterpret_cast<const struct sockaddr *>(&m_addr); }
  inline socklen_type get_addr_len() const { return sizeof(m_addr); }
  inline const char *get_host_str() const { return strlen(m_host_str) > 0 ? m_host_str : m_addr_str; }
  const char *get_addr_str() const;
  static bool is_valid(const char *p_addr);
  bool is_local() const;
private:
  sockaddr_in6 m_addr;
  char m_host_str[NI_MAXHOST]; // DNS name.
  char m_addr_str[INET6_ADDRSTRLEN]; // Address in numeric format.
};
#endif // LINUX || CYGWIN17

class NetworkHandler {
public:
  NetworkHandler();
  NetworkHandler(const NetworkFamily& p_family);
  NetworkHandler(const char *p_addr);

  inline void set_family(const NetworkFamily& p_family) { m_family = p_family; }
  void set_family(const char *p_addr);
  inline const NetworkFamily& get_family() const { return m_family; }
  int socket();
  static int socket(const NetworkFamily& p_family);
private:
  NetworkHandler(const NetworkHandler& p_handler);
  NetworkHandler& operator=(const NetworkHandler& p_handler);
protected:
  NetworkFamily m_family;
};

class HCNetworkHandler : public NetworkHandler {
public:
  HCNetworkHandler();
  ~HCNetworkHandler();

  bool set_local_addr(const char *p_addr, unsigned short p_port = 0 /* Any port. */);
  bool set_mc_addr(const char *p_addr, unsigned short p_port = 0 /* Any port. */);
  int getsockname_local_addr(int p_sockfd);
  int bind_local_addr(int p_sockfd) const;
  int connect_to_mc(int p_sockfd) const;
  inline const char *get_mc_host_str() const { return m_mc_addr->get_host_str(); }
  inline const char *get_mc_addr_str() const { return m_mc_addr->get_addr_str(); }
  inline const char *get_local_host_str() const { return m_local_addr->get_host_str(); }
  inline const char *get_local_addr_str() const { return m_local_addr->get_addr_str(); }
  inline IPAddress *get_mc_addr() const { return m_mc_addr; }
  inline IPAddress *get_local_addr() const { return m_local_addr; }
  inline unsigned short get_mc_port() const { return m_mc_addr->get_port(); }
  inline unsigned short get_local_port() const { return m_local_addr->get_port(); }
private:
  HCNetworkHandler(const HCNetworkHandler& p_handler);
  HCNetworkHandler& operator=(const HCNetworkHandler& p_handler);

  IPAddress *m_mc_addr;
  IPAddress *m_local_addr;
};

#endif // NETWORKHANDLER_H_

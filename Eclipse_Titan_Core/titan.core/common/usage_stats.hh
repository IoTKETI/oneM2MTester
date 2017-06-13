/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Ormandi, Matyas
 *
 ******************************************************************************/
#ifndef USAGE_STATS_H_
#define USAGE_STATS_H_

/*****************************************************
// Usage:
// include usage_stats.hh before dbgnew.hh!!
// XYSender sender; // XYSender is a subclass of Sender
// int timeout = 200; // timeout in msec for the data sender thread
// UsageData::getInstance().sendDataThreaded("info", timeout, &sender);

// Lib requirement:
// for Solaris -lnsl -lsocket -lresolv
// for Linux -lpthread -lrt

// iodine: use iodine as a library from /etc/dns/iodine/lib/libiodine.a
// or via pipe as an outside process
// uncomment this and the DNSSender::send function
//#include "../etc/dns/iodine/src/iodine.h"
*****************************************************/

#include <string>

#include "version_internal.h"

class Sender {
public:
  Sender() {};
  virtual ~Sender() {};
  virtual void send(const char*) = 0;
};

class DNSSender: public Sender {
public:
  DNSSender();
  ~DNSSender() {};
  void send(const char*);

private:
  const char* nameserver;
  const char* domain;
};

class HttpSender: public Sender {
public:
  HttpSender() {};
  ~HttpSender() {};
  void send(const char*);
};

struct thread_data {
  std::string msg;
  Sender* sndr;
};

class UsageData {
public:
  static UsageData& getInstance()
  {
    static UsageData instance; // Guaranteed to be destroyed.
    return instance; // Instantiated on first use.
  }
  static pthread_t sendDataThreaded(std::string msg, thread_data* data);

private:

  UsageData();
  ~UsageData();
  UsageData(UsageData const&); // Don't Implement
  void operator=(UsageData const&); // Don't implement

  static void* sendData(void*);

  static std::string id;
  static std::string host;
  static std::string platform;
};

#endif /* USAGE_STATS_H_ */

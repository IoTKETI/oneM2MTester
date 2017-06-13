/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Feher, Csaba
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef EVENT_HANDLER_HH
#define EVENT_HANDLER_HH

#include "Types.h"
#include <sys/select.h>

/**
* The definitions in this header file are needed for TITAN.
* These classes should not be used in user code.
* These classes are used as base classes for class PORT.
*/

class Fd_Event_Handler {
  virtual void Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writeable, boolean is_error) = 0;

public:
  Fd_Event_Handler() {}
  virtual ~Fd_Event_Handler() {}
  virtual void log() const;
private:
  Fd_Event_Handler(const Fd_Event_Handler&);
  const Fd_Event_Handler & operator= (const Fd_Event_Handler &);
  friend class Fd_And_Timeout_User;
  friend class Handler_List;
};


class Handler_List;
class FdSets;

class Fd_And_Timeout_Event_Handler : public Fd_Event_Handler {
  virtual void Handle_Fd_Event(int fd,
    boolean is_readable, boolean is_writeable, boolean is_error);
  virtual void Handle_Timeout(double time_since_last_call);
  /// Copy constructor disabled
  Fd_And_Timeout_Event_Handler(const Fd_And_Timeout_Event_Handler&);
  /// Assignment disabled
  Fd_And_Timeout_Event_Handler& operator=(const Fd_And_Timeout_Event_Handler&);
public:
  virtual void Event_Handler(const fd_set *read_fds, const fd_set *write_fds,
    const fd_set *error_fds, double time_since_last_call);

public:
  inline Fd_And_Timeout_Event_Handler() :
  callInterval(0.0), last_called(0.0), list(0), prev(0), next(0),
  fdSets(0), fdCount(0),
  isTimeout(TRUE), callAnyway(TRUE), isPeriodic(TRUE),
  hasEvent(FALSE)
  {}
  virtual ~Fd_And_Timeout_Event_Handler();
  virtual void log() const;

  friend class Fd_And_Timeout_User;
  friend class Handler_List;
private:
  double callInterval, last_called;
  Handler_List * list;
  Fd_And_Timeout_Event_Handler * prev, * next;
  FdSets * fdSets;
  int fdCount;
  boolean isTimeout, callAnyway, isPeriodic;
  boolean hasEvent;
protected:
  boolean getIsOldApi() { return fdSets != 0; }
};

#endif

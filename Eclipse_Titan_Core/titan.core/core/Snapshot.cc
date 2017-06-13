/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Czimbalmos, Eduard
 *   Feher, Csaba
 *   Kovacs, Ferenc
 *   Pilisi, Gergely
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/types.h>
#include <poll.h>
#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif

#include "Types.h"

#include "Snapshot.hh"
#include "Fd_And_Timeout_User.hh"
#include "Timer.hh"
#include "Logger.hh"
#include "Error.hh"
#include "Event_Handler.hh"


static const int MAX_INT_VAL = static_cast<int>(static_cast<unsigned int>( -2 ) >> 1u);

/******************************************************************************
 * class FdMap                                                                *
 ******************************************************************************/

int             FdMap::nItems;
int             FdMap::capacity;
FdMap::Item     FdMap::items1[ITEM1_CAPACITY];
FdMap::Data   * FdMap::items2;
#ifndef USE_EPOLL
pollfd          FdMap::pollFds1[ITEM1_CAPACITY];
pollfd          * FdMap::pollFds2;
boolean            FdMap::needUpdate;
int             FdMap::nPollFdsFrozen;
#endif
#ifdef USE_EPOLL
int             FdMap::epollFd;
epoll_event     FdMap::epollEvents[MAX_EPOLL_EVENTS];
#endif

fd_event_type_enum FdMap::add(int fd, Fd_Event_Handler * handler,
  fd_event_type_enum event)
{
  if (handler == 0)
    TTCN_error("FdMap::add: Internal error"); // debug
  if (fd < 0 || fd >= capacity) {
    TTCN_error_begin("Trying to add events of an invalid file descriptor "
      "(%d) to the set of events handled by \"", fd);
    handler->log();
    TTCN_Logger::log_event("\".");
    TTCN_error_end();
  }
  if ((event & ~(FD_EVENT_RD | FD_EVENT_WR | FD_EVENT_ERR)) != 0) {
    TTCN_error_begin("Trying to add invalid events (%d) of file descriptor "
      "(%d) to the set of events handled by \"", event, fd);
    handler->log();
    TTCN_Logger::log_event("\".");
    TTCN_error_end();
  }
#ifndef USE_EPOLL
  short pollEvent = eventToPollEvent(event);
#endif
  if (items2 == 0) {
    int i = findInsPointInItems1(fd);
    if (i < nItems && fd == items1[i].fd) {
      // Already exists...
      if (items1[i].d.hnd != 0 && items1[i].d.hnd != handler) {
        TTCN_error_begin("Trying to add file descriptor (%d) "
          "events (%d) to the set of events handled by \"",fd,event);
        handler->log();
        TTCN_Logger::log_event("\", but the events of the "
          "file descriptor already have a different handler: \"");
        if (items1[i].d.hnd != 0) items1[i].d.hnd->log();
        TTCN_Logger::log_event("\".");
        TTCN_error_end();
      }
      fd_event_type_enum oldEvent;
#ifdef USE_EPOLL
      oldEvent = static_cast<fd_event_type_enum>(items1[i].d.evt);
      items1[i].d.evt |= static_cast<short>(event);
#else
      items1[i].d.hnd = handler; // in case it is a frozen deleted item
      oldEvent = pollEventToEvent(pollFds1[items1[i].d.ixPoll].events);
      pollFds1[items1[i].d.ixPoll].events |= pollEvent;
#endif
      return oldEvent;
    }
    if (nItems < ITEM1_CAPACITY) {
      // Size of the static array is enough
      for (int j = nItems - 1; j >= i; --j) items1[j + 1] = items1[j];
      items1[i].fd = fd;
#ifdef USE_EPOLL
      items1[i].d.evt = static_cast<short>(event);
      items1[i].d.ixE = -1;
#else
      pollFds1[nItems].fd = fd;
      pollFds1[nItems].events = pollEvent;
      pollFds1[nItems].revents = 0;
      items1[i].d.ixPoll = nItems;
#endif
      items1[i].d.hnd = handler;
      ++nItems;
      return static_cast<fd_event_type_enum>(0);
    }
    // Copying items to the bigger dynamically allocated array
    items2 = new Data[capacity]; // items2 is initialized in the constructor
    for (i = 0; i < nItems; ++i) {
      items2[items1[i].fd] = items1[i].d;
      items1[i].init(); // not necessary - for debugging
    }
#ifndef USE_EPOLL
    pollFds2 = new pollfd[capacity];
    i = 0;
    while (i < nItems) { pollFds2[i] = pollFds1[i]; init(pollFds1[i++]); }
    while (i < ITEM1_CAPACITY) { init(pollFds2[i]); init(pollFds1[i++]); }
    while (i < capacity) init(pollFds2[i++]);
    //Note: From the above three loops: only the copying is needed
    //      The initializations are only for debugging
#endif
    // adding item...
  } else {
    if (findInItems2(fd)) {
      // Already exists...
      if (items2[fd].hnd != 0 && items2[fd].hnd != handler) {
        TTCN_error_begin("Trying to add file descriptor (%d) "
          "events (%d) to the set of events handled by \"",fd,event);
        handler->log();
        TTCN_Logger::log_event("\", but the events of the "
          "file descriptor already have a different handler: \"");
        if (items2[fd].hnd != 0) items2[fd].hnd->log();
        TTCN_Logger::log_event("\".");
        TTCN_error_end();
      }
      fd_event_type_enum oldEvent;
#ifdef USE_EPOLL
      oldEvent = static_cast<fd_event_type_enum>(items2[fd].evt);
      items2[fd].evt |= static_cast<short>(event);
#else
      items2[fd].hnd = handler; // in case it is a frozen deleted item
      oldEvent = pollEventToEvent(pollFds2[items2[fd].ixPoll].events);
      pollFds2[items2[fd].ixPoll].events |= pollEvent;
#endif
      return oldEvent;
    }
    // adding item...
  }
  // ...adding item
#ifdef USE_EPOLL
  items2[fd].evt = static_cast<short>(event);
  items2[fd].ixE = -1;
#else
  pollFds2[nItems].fd = fd;
  pollFds2[nItems].events = pollEvent;
  pollFds2[nItems].revents = 0;
  items2[fd].ixPoll = nItems;
#endif
  items2[fd].hnd = handler;
  ++nItems;
  return static_cast<fd_event_type_enum>(0);
}

fd_event_type_enum FdMap::remove(int fd, const Fd_Event_Handler * handler,
  fd_event_type_enum event)
{
  // Errors in Test Ports may be detected at this point
  // handler is used only for checking;
  // handler is 0 to remove a frozen deleted item
  if (fd < 0 || fd >= capacity) {
    TTCN_error_begin("Trying to remove events of an invalid file "
      "descriptor (%d) from the set of events handled by \"", fd);
    if (handler != 0) handler->log();
    TTCN_Logger::log_event("\".");
    TTCN_error_end();
  }
  if ((event & ~(FD_EVENT_RD | FD_EVENT_WR | FD_EVENT_ERR)) != 0) {
    TTCN_error_begin("Trying to remove invalid events (%d) of file "
      "descriptor (%d) from the set of events handled by \"", event, fd);
    if (handler != 0) handler->log();
    TTCN_Logger::log_event("\".");
    TTCN_error_end();
  }
  fd_event_type_enum oldEvent;
#ifndef USE_EPOLL
  short pollEvent = eventToPollEvent(event);
#endif
  if (items2 == 0) {
    int i = findInItems1(fd);
    if (i < 0) {
      TTCN_warning_begin("Trying to remove file descriptor (%d) "
        "events (%d) from the set of events handled by \"",
        fd, event);
      if (handler != 0) handler->log();
      TTCN_Logger::log_event("\", but events of the file descriptor "
        "do not have a handler.");
      TTCN_warning_end();
      // Ignore errors for HP53582.
      return FD_EVENT_ERR;
    }
    if (handler != items1[i].d.hnd) {
      TTCN_error_begin("Trying to remove file descriptor (%d) "
        "events (%d) from the set of events handled by \"", fd, event);
      if (handler != 0) handler->log();
      TTCN_Logger::log_event("\", but the events of the "
        "file descriptor have different handler: \"");
      if (items1[i].d.hnd != 0) items1[i].d.hnd->log();
      TTCN_Logger::log_event("\".");
      TTCN_error_end();
    }
#ifdef USE_EPOLL
    short ixE = items1[i].d.ixE;
    if (ixE >= 0) epollEvents[ixE].events &= ~eventToEpollEvent(event);
    oldEvent = static_cast<fd_event_type_enum>(items1[i].d.evt);
    if ((items1[i].d.evt &= ~static_cast<short>(event)) == 0) {
      --nItems;
      while (i < nItems) { items1[i] = items1[i + 1]; ++i; }
      items1[nItems].init(); // not necessary - for debugging
    }
#else
    int j = items1[i].d.ixPoll;
    oldEvent = pollEventToEvent(pollFds1[j].events);
    pollFds1[j].revents &= ~pollEvent;
    if ((pollFds1[j].events &= ~pollEvent) == 0) {
      if (j >= nPollFdsFrozen) {
        if (j < nItems - 1) {
          pollFds1[j] = pollFds1[nItems - 1];
          int k = findInItems1(pollFds1[j].fd); // reads nItems
          items1[k].d.ixPoll = j;
        }
        --nItems;
        while (i < nItems) { items1[i] = items1[i + 1]; ++i; }
        init(pollFds1[nItems]); // not necessary - for debugging
        items1[nItems].init(); // not necessary - for debugging
      } else { // The item is frozen; removal is postponed.
        items1[i].d.hnd = 0;
        needUpdate = TRUE;
      }
    }
#endif
  } else {
    if (!findInItems2(fd)) {
      TTCN_error_begin("Trying to remove file descriptor (%d) "
        "events (%d) from the set of events handled by \"",
        fd, event);
      if (handler != 0) handler->log();
      TTCN_Logger::log_event("\", but events of the file descriptor "
        "do not have a handler.");
      TTCN_error_end();
    }
    if (items2[fd].hnd != handler) {
      TTCN_error_begin("Trying to remove file descriptor (%d) "
        "events (%d) from the set of events handled by \"", fd, event);
      if (handler != 0) handler->log();
      TTCN_Logger::log_event("\", but the events of the "
        "file descriptor have different handler: \"");
      items2[fd].hnd->log(); TTCN_Logger::log_event("\".");
      TTCN_error_end();
    }
#ifdef USE_EPOLL
    short ixE = items2[fd].ixE;
    if (ixE >= 0) epollEvents[ixE].events &= ~eventToEpollEvent(event);
    oldEvent = static_cast<fd_event_type_enum>(items2[fd].evt);
    if ((items2[fd].evt &= ~static_cast<short>(event)) == 0) {
      --nItems;
      items2[fd].init(); // necessary to indicate an unused item
      if (nItems <= ITEM1_CAPACITY_LOW) {
        // could be improved with additional bookkeeping
        // items1 has to be kept ordered with fd as key
        for (int i = 0, n = 0; n < nItems && i < capacity; ++i) {
          if (findInItems2(i)) {
            items1[n].fd = i;
            items1[n++].d = items2[i];
          }
        }
        delete[] items2; items2 = 0;
        //if (n < nItems) => error
      }
    }
#else
    int i = items2[fd].ixPoll;
    oldEvent = pollEventToEvent(pollFds2[i].events);
    pollFds2[i].revents &= ~pollEvent;
    if ((pollFds2[i].events &= ~pollEvent) == 0) {
      if (i >= nPollFdsFrozen) {
        --nItems;
        if (i < nItems) {
          pollFds2[i] = pollFds2[nItems];
          items2[pollFds2[i].fd].ixPoll = i;
        }
        init(pollFds2[nItems]); // not necessary - for debugging
        items2[fd].init(); // necessary to indicate an unused item
        if (nItems <= ITEM1_CAPACITY_LOW) {
          // items1 has to be kept ordered with fd as key
          copyItems2ToItems1();
          delete[] items2; items2 = 0;
          delete[] pollFds2; pollFds2 = 0;
        }
      } else { // The item is frozen; removal is postponed.
        items2[fd].hnd = 0;
        needUpdate = TRUE;
      }
    }
#endif
  }
  return oldEvent;
}

fd_event_type_enum FdMap::find(int fd, Fd_Event_Handler * * handler)
{
  Data  * data;
  if (items2 == 0) {
    int i = findInItems1(fd);
    if (i < 0) { *handler = 0; return static_cast<fd_event_type_enum>(0); }
    data = &items1[i].d;
  } else {
    if (!findInItems2(fd)) {
      *handler = 0; return static_cast<fd_event_type_enum>(0);
    }
    data = &items2[fd];
  }
  *handler = data->hnd;
#ifdef USE_EPOLL
  return static_cast<fd_event_type_enum>(data->evt);
#else
  return pollEventToEvent(getPollFds()[data->ixPoll].events);
#endif
}

#ifndef USE_EPOLL
void FdMap::copyItems2ToItems1()
{
  if (nItems != ITEM1_CAPACITY_LOW)
    TTCN_error("FdMap::copyItems2ToItems1: Internal error");
  if (ITEM1_CAPACITY_LOW == 0) return;
  int n = nItems;
  int i = 0;
  for (int m = n - 1; m != 0; ++i) m >>= 1;
  Item    * d, itemsTmp[ITEM1_CAPACITY_LOW];
  boolean f = (i & 1) != 0;
  d = (i == 0 || f) ? items1 : itemsTmp;
  for (int j = 0, k = nItems - 1; j < k; j += 2) {
    pollFds1[j] = pollFds2[j]; pollFds1[j + 1] = pollFds2[j + 1];
    int fd1 = pollFds1[j].fd, fd2 = pollFds1[j + 1].fd;
    if (fd1 <= fd2) {
      d[j].fd = fd1;     d[j].d = items2[fd1];
      d[j + 1].fd = fd2; d[j + 1].d = items2[fd2];
    } else {
      d[j].fd = fd2;     d[j].d = items2[fd2];
      d[j + 1].fd = fd1; d[j + 1].d = items2[fd1];
    }
  }
  if ((nItems & 1) != 0) {
    pollFds1[n - 1] = pollFds2[n - 1];
    int fd1 = pollFds1[n - 1].fd;
    d[nItems - 1].fd = fd1;
    d[nItems - 1].d = items2[fd1];
  }
  for (int j = 1; j < i; ++j) {
    Item    * s = f ? items1 : itemsTmp;
    f = !f;
    d = f ? items1 : itemsTmp;
    int step = 1 << j;
    int step2 = step * 2;
    int w = 0;
    for (int k = 0; k < n; k += step2) {
      int u = k;
      int v = k + step;
      int uN = (n >= v) ? step : (n - u);
      int vN = (n >= v + step) ? step : (n - v);
      while (uN != 0 && vN > 0) {
        if (s[u].fd <= s[v].fd) {
          d[w++] = s[u++]; --uN;
        } else {
          d[w++] = s[v++]; --vN;
        }
      }
      while (uN != 0) { d[w++] = s[u++]; --uN; }
      while (vN > 0) { d[w++] = s[v++]; --vN; }
    }
  }
}
#endif

#ifdef USE_EPOLL
boolean FdMap::epollMarkFds(int nEvents)
{
  boolean all_valid = TRUE;
  for (int i = 0; i < nEvents; ++i) {
    int fd = epollEvents[i].data.fd;
    if (items2 == 0) {
      int j = findInItems1(fd);
      if (j >= 0) items1[j].d.ixE = i;
      else all_valid = FALSE;
    } else {
      if (findInItems2(fd)) items2[fd].ixE = i;
      else all_valid = FALSE;
    }
  }
  return all_valid;
}

void FdMap::epollUnmarkFds(int nEvents)
{
  for (int i = 0; i < nEvents; ++i) {
    int fd = epollEvents[i].data.fd;
    if (items2 == 0) {
      int j = findInItems1(fd);
      if (j >= 0) items1[j].d.ixE = -1;
    } else {
      if (findInItems2(fd)) items2[fd].ixE = -1;
    }
  }
}
#else
void FdMap::pollFreeze()
{
  nPollFdsFrozen = nItems;
}

void FdMap::pollUnfreeze()
{
  if (!needUpdate) { nPollFdsFrozen = 0; return; }
  int i = 0, j = nPollFdsFrozen;
  nPollFdsFrozen = 0;
  while (i < j) {
    pollfd  & pollFd = getPollFds()[i];
    if (pollFd.events == 0) {
      remove(pollFd.fd, 0, static_cast<fd_event_type_enum>(
        FD_EVENT_RD | FD_EVENT_WR | FD_EVENT_ERR));
      if (nItems < j) j = nItems;
      // item at index i is changed and has to be checked if exists
    } else ++i;
  }
  needUpdate = FALSE;
}

fd_event_type_enum FdMap::getPollREvent(int fd)
{
  if (items2 == 0) {
    int i = findInItems1(fd);
    if (i >= 0)
      return pollEventToEvent(pollFds1[items1[i].d.ixPoll].revents);
  } else {
    if (findInItems2(fd))
      return pollEventToEvent(pollFds2[items2[fd].ixPoll].revents);
  }
  return static_cast<fd_event_type_enum>(0);
}
#endif





/******************************************************************************
 * class Fd_Event_Handler                                                     *
 ******************************************************************************/

void Fd_Event_Handler::log() const
{
  TTCN_Logger::log_event("handler <invalid>");
}






/******************************************************************************
 * class Fd_And_Timeout_Event_Handler                                         *
 ******************************************************************************/

void Fd_And_Timeout_Event_Handler::Handle_Fd_Event(int,
  boolean, boolean, boolean)
{
  TTCN_error("Fd_And_Timeout_Event_Handler::Handle_Fd_Event: "
    "Erroneous usage of class Fd_And_Timeout_Event_Handler");
  // This method cannot be pure virtual because of necessary instantiation
  // inside TITAN
}

void Fd_And_Timeout_Event_Handler::Handle_Timeout(double)
{
  TTCN_error("Fd_And_Timeout_Event_Handler::Handle_Timeout: "
    "Erroneous usage of class Fd_And_Timeout_Event_Handler");
  // This method cannot be pure virtual because of necessary instantiation
  // inside TITAN
}

void Fd_And_Timeout_Event_Handler::Event_Handler(const fd_set *,
  const fd_set *, const fd_set *, double)
{
  TTCN_error("Fd_And_Timeout_Event_Handler::Event_Handler: "
    "Erroneous usage of class Fd_And_Timeout_Event_Handler");
  // This method cannot be pure virtual because of necessary instantiation
  // inside TITAN
}

Fd_And_Timeout_Event_Handler::~Fd_And_Timeout_Event_Handler()
{
  // In case the event handler forgot to stop its timer,
  // stop it at this point.
  Fd_And_Timeout_User::set_timer(this, 0.0);
  // In case the event handler forgot to remove all of its file descriptor
  // events, remove those at this point.
  Fd_And_Timeout_User::remove_all_fds(this);
}

void Fd_And_Timeout_Event_Handler::log() const
{
  TTCN_Logger::log_event("handler <unknown>");
}





/* The maximal blocking time to be used in poll, epoll and select (in seconds).
 * On some systems (e.g. Solaris) the select call is a libc wrapper for
 * poll(2). In the wrapper the overflows are not always handled thus select
 * may return EINVAL in some cases.
 * This value is derived from the maximal possible value for the last argument
 * of poll, which is measured in milliseconds. */
#define MAX_BLOCK_TIME (~(0x80 << ((sizeof(int) - 1) * 8)) / 1000)





/******************************************************************************
 * class Fd_And_Timeout_User                                                  *
 ******************************************************************************/

Handler_List Fd_And_Timeout_User::timedList,
    Fd_And_Timeout_User::oldApiCallList;
FdSets * Fd_And_Timeout_User::fdSetsReceived;
FdSets * Fd_And_Timeout_User::fdSetsToHnds;
int Fd_And_Timeout_User::nOldHandlers;
boolean Fd_And_Timeout_User::is_in_call_handlers;
int Fd_And_Timeout_User::curRcvdEvtIx;

inline void Fd_And_Timeout_User::checkFd(int fd)
{
  if (
#ifdef USE_EPOLL
    fd == FdMap::epollFd ||
#endif
    fcntl(fd, F_GETFD, FD_CLOEXEC) < 0)
    TTCN_error("Trying to add events of an invalid file descriptor (%d)", fd);
}

void Fd_And_Timeout_User::add_fd(int fd, Fd_Event_Handler * handler,
  fd_event_type_enum event)
{
  fd_event_type_enum oldEvent = FdMap::add(fd, handler, event);
  Fd_And_Timeout_Event_Handler * tmHnd =
    dynamic_cast<Fd_And_Timeout_Event_Handler *>(handler);
  if (tmHnd != 0) {
    if (tmHnd->fdSets != 0) {
      if (fd >= (int)FD_SETSIZE)
        TTCN_error("The file descriptor (%d) to be added is too big "
          "to be handled by Event_Handler. FD_SETSIZE is %d",
          fd, FD_SETSIZE);
      tmHnd->fdSets->add(fd, event);
    }
    if (oldEvent == 0) ++tmHnd->fdCount;
  }
#ifdef USE_EPOLL
  epoll_event epollEvent;
  memset(&epollEvent, 0, sizeof(epollEvent));
  epollEvent.events = FdMap::eventToEpollEvent(oldEvent | event);
  epollEvent.data.fd = fd;
  if ( ( (oldEvent == 0) ?
    epoll_ctl(FdMap::epollFd, EPOLL_CTL_ADD, fd, &epollEvent) :
    epoll_ctl(FdMap::epollFd, EPOLL_CTL_MOD, fd, &epollEvent) ) < 0 ) {
    checkFd(fd);
    TTCN_error("Fd_And_Timeout_User::add_fd: System call epoll_ctl failed "
      "when adding fd: %d, errno: %d", fd, errno);
  }
#else
  checkFd(fd);
#endif
}

void Fd_And_Timeout_User::remove_fd(int fd, Fd_Event_Handler * handler,
  fd_event_type_enum event)
{
  if (handler == 0)
    TTCN_error("Fd_And_Timeout_User::remove_fd: Internal error"); // debug
  fd_event_type_enum oldEvent = FdMap::remove(fd, handler, event);
  // Ignore errors for HP53582.
  if (oldEvent == FD_EVENT_ERR) return;
  fd_event_type_enum newEvent =
    static_cast<fd_event_type_enum>(oldEvent & ~event);
  Fd_And_Timeout_Event_Handler * tmHnd =
    dynamic_cast<Fd_And_Timeout_Event_Handler *>(handler);
  if (tmHnd != 0) {
    if (newEvent == 0) --tmHnd->fdCount;
    if (tmHnd->getIsOldApi()) {
      fdSetsReceived->remove(fd, event);
      tmHnd->fdSets->remove(fd, event);
    }
  }
#ifdef USE_EPOLL
  epoll_event epollEvent;
  memset(&epollEvent, 0, sizeof(epollEvent));
  epollEvent.data.fd = fd;
  if (newEvent == 0) {
    if (epoll_ctl(FdMap::epollFd, EPOLL_CTL_DEL, fd, &epollEvent) < 0) {
      // Check if fd exists. If not, assume, that it was closed earlier.
      int errno_tmp = errno;
      if (fcntl(fd, F_GETFD, FD_CLOEXEC) >= 0) {
        errno = errno_tmp;
        TTCN_error("System call epoll_ctl failed when deleting fd: %d, "
          "errno: %d", fd, errno);
      }
      // fd was closed before removing it from the database of TITAN.
      // This removes fd from the epoll set causing epoll_ctl to fail.
      errno = 0; // It is not an error if epoll_ctl fails here
    }
  } else {
    // There is still event type to wait for. - This is the unusual case.
    epollEvent.events = FdMap::eventToEpollEvent(newEvent);
    if (epoll_ctl(FdMap::epollFd, EPOLL_CTL_MOD, fd, &epollEvent) < 0) {
      TTCN_error("System call epoll_ctl failed when removing  fd: %d, "
        "errno: %d", fd, errno);
    }
  }
#endif
}

void Fd_And_Timeout_User::set_timer(Fd_And_Timeout_Event_Handler * handler,
  double call_interval,
  boolean is_timeout, boolean call_anyway, boolean is_periodic)
{
  if (call_interval != 0.0) {
    if (handler->list == 0) timedList.add(handler);
    handler->callInterval = call_interval;
    handler->last_called = TTCN_Snapshot::time_now();
    handler->isTimeout = is_timeout;
    handler->callAnyway = call_anyway;
    handler->isPeriodic = is_periodic;
  } else {
    if (handler->list == &timedList) timedList.remove(handler);
    // Note: Normally the port may be only in timedList or in no list.
    // - The port is put in oldApiCallList only temporarily while calling
    // event handlers.
    // The set_timer method may be called from outside snapshot evaluation
    // or in the event handler. In both cases the port is removed from
    // oldApiCallList beforehand.
    // - However when MC requests a port to be unmapped: the request is
    // processed in the event handler of MC_Connection. In this event
    // handler a port may be unmapped which has been put in oldApiCallList
    // temporarily.
    handler->callInterval = 0.0;
  }
}

void Fd_And_Timeout_User::set_fds_with_fd_sets(
  Fd_And_Timeout_Event_Handler * handler,
  const fd_set *read_fds, const fd_set *write_fds, const fd_set *error_fds)
{
  // Probaly in class PORT: Install_Handler should not be possible to be
  // called from new event handlers
  int fdLimit = FdMap::getFdLimit();
  if ((int)FD_SETSIZE < fdLimit) fdLimit = FD_SETSIZE;
  if (handler->fdSets == 0) {
    if (handler->fdCount != 0) {
      // Usage of the new and old API is mixed. - It should not happen.
      // It is handled, but not most efficiently.
      remove_all_fds(handler);
    }
    handler->fdSets = new FdSets;
    ++nOldHandlers;
    if (fdSetsReceived == 0) fdSetsReceived = new FdSets;
    if (fdSetsToHnds == 0) fdSetsToHnds = new FdSets;
  }
  FdSets * fdSets = handler->fdSets;
  fd_event_type_enum eventOld, eventNew;
#ifdef USE_EPOLL
  // Removing fds which refer to different descriptors than
  // in the previous call. (closed and other fd created with same id)
  epoll_event epollEvent;
  for (int fd = 0; ; ++fd) {
    fd = fdSets->getIxBothAnySet(read_fds, write_fds, error_fds,fd,fdLimit);
    if (fd >= fdLimit) break;
    memset(&epollEvent, 0, sizeof(epollEvent));
    epollEvent.data.fd = fd;
    // Check (inverse) if fd is still in the epoll set
    if (epoll_ctl(FdMap::epollFd, EPOLL_CTL_ADD, fd, &epollEvent) >= 0 ) {
      // fd was not in the epoll set as fd was closed and a new
      // descriptor was created with the same id.
      eventOld = fdSets->getEvent(fd);
      Fd_And_Timeout_User::remove_fd(fd, handler, eventOld);
    } else {
      errno = 0; // fd is already in the epoll set - it is not an error
    }
  }
#endif
  fd_event_type_enum event;
  for (int fd = 0; ; ++fd) {
    fd = fdSets->getIxDiff(read_fds, write_fds, error_fds, fd, fdLimit);
    if (fd >= fdLimit) break;
    eventOld = fdSets->getEvent(fd);
    eventNew = FdSets::getEvent(read_fds, write_fds, error_fds, fd);
    event = static_cast<fd_event_type_enum>(eventNew & ~eventOld);
    if (event != 0) Fd_And_Timeout_User::add_fd(fd, handler, event);
    event = static_cast<fd_event_type_enum>(eventOld & ~eventNew);
    if (event != 0) Fd_And_Timeout_User::remove_fd(fd, handler, event);
  }
}

void Fd_And_Timeout_User::remove_all_fds(Fd_And_Timeout_Event_Handler * handler)
{
  if (handler->fdSets != 0 &&
#ifdef USE_EPOLL
    !FdMap::isItems1Used()
#else
  (unsigned) FdMap::getSize() > FD_SETSIZE / sizeof(long)
#endif
  ) {
    // FdSets is used to enumerate and remove all file descriptor of
    // the specified handler
    FdSets * fdSets = handler->fdSets;
    for (int fd = 0; handler->fdCount != 0; ++fd) {
      fd = fdSets->getIxSet(fd, FD_SETSIZE);
      if (fd >= (int)FD_SETSIZE)
        TTCN_error("Fd_And_Timeout_User::remove_all_fds Internal "
          "error 1: fdCount: %i", handler->fdCount);//debug
      Fd_And_Timeout_User::remove_fd(fd, handler, fdSets->getEvent(fd));
    }
  } else {
    // FdMap is used to enumerate and remove all file descriptor of
    // the specified handler
    Fd_Event_Handler * hnd = 0;
    fd_event_type_enum  event = static_cast<fd_event_type_enum>(0);
    int i;
    int fd = -1;
#ifdef USE_EPOLL
    int fdLimit = FdMap::getFdLimit();
    while (handler->fdCount != 0 && !FdMap::isItems1Used()) {
      do {
        if (++fd >= fdLimit)
          TTCN_error("Fd_And_Timeout_User::remove_all_fds Internal "
            "error 2: fdCount: %i", handler->fdCount);//debug
        event = FdMap::item2atFd(fd, &hnd);
      } while (event == 0 || hnd != handler);
      Fd_And_Timeout_User::remove_fd(fd, handler, event);
    }
#else
    i = -1;
    while (handler->fdCount != 0 && !FdMap::isItems1Used()) {
      pollfd * pollFds = FdMap::getPollFds();
      do {
        ++i;
        if (i >= FdMap::getSize())
          TTCN_error("Fd_And_Timeout_User::remove_all_fds Internal "
            "error 3: fdCount: %i", handler->fdCount);//debug
        fd = pollFds[i].fd;
        event = FdMap::item2atFd(fd, &hnd);
      } while (event == 0 || hnd != handler);
      Fd_And_Timeout_User::remove_fd(fd, handler, event);
      --i; // recheck pollfd item at index i
    }
#endif
    i = -1;
    while (handler->fdCount != 0) {
      do {
        if (++i >= FdMap::getSize())
          TTCN_error("Fd_And_Timeout_User::remove_all_fds Internal "
            "error 4: fdCount: %i", handler->fdCount);//debug
        event = FdMap::item1atIndex(i, &fd, &hnd);
      } while (event == 0 || hnd != handler);
      Fd_And_Timeout_User::remove_fd(fd, handler, event);
      --i; // recheck item at index i
    }
  }
  if (handler->fdSets != 0) {
    delete handler->fdSets; handler->fdSets = 0;
    --nOldHandlers;
    if (nOldHandlers == 0) {
      delete fdSetsReceived; fdSetsReceived = 0;
      delete fdSetsToHnds; fdSetsToHnds = 0;
    }
  }
}

boolean Fd_And_Timeout_User::getTimeout(double * timeout)
{
  timedList.first();
  if (timedList.finished()) return FALSE;

  Fd_And_Timeout_Event_Handler * handler = timedList.current();
  double earliestTimeout = handler->last_called + handler->callInterval;
  timedList.next();

  while (!timedList.finished()) {
    handler = timedList.current();
    timedList.next();
    double nextCall = handler->last_called + handler->callInterval;
    if (nextCall < earliestTimeout) earliestTimeout = nextCall;
  }
  *timeout = earliestTimeout;
  return TRUE;
}

void Fd_And_Timeout_User::call_handlers(int nEvents)
{
  try { // To keep consistency in case of exceptions
    is_in_call_handlers = TRUE;
    if (nOldHandlers != 0) { fdSetsReceived->clear(); }
    if (nEvents > 0) {
      // Note: FdMap may be modified during event handler calls
#ifdef USE_EPOLL
      FdMap::epollMarkFds(nEvents);
      int ixLimit = nEvents;
#else
      FdMap::pollFreeze();
      int ixLimit = FdMap::getSize();
      // Below this index pollfd array items are not removed
      // If an item should have been removed, then the events field is 0
#endif
      try { // To keep consistency in case of exceptions
        for (int ix = 0; ix != ixLimit; ++ix) {
#ifdef USE_EPOLL
          int fd = FdMap::epollEvents[ix].data.fd;
          fd_event_type_enum  event =
            FdMap::epollEventToEvent(FdMap::epollEvents[ix].events);
#else
          pollfd * pollFd = &(FdMap::getPollFds()[ix]);
          if ((pollFd->revents & FdMap::pollEventMask) == 0) continue;
          int fd = pollFd->fd;
          fd_event_type_enum event = FdMap::pollEventToEvent(pollFd->revents);
          // The event handler may need pollFd.revents or epoll .events
#endif
          Fd_Event_Handler    * handler = 0;
          fd_event_type_enum wEvent = FdMap::find(fd, &handler);
          if (wEvent != 0) {
            event = static_cast<fd_event_type_enum>(
              event & (wEvent | FD_EVENT_ERR));
            if (event != 0) {
              curRcvdEvtIx = ix; // see getCurReceivedEvent()
              Fd_And_Timeout_Event_Handler * tmHnd =
                dynamic_cast<Fd_And_Timeout_Event_Handler*>(handler);
              if (tmHnd != 0 && tmHnd->getIsOldApi()) {
                fdSetsReceived->add(fd, event);
                if (tmHnd->list == 0) oldApiCallList.add(tmHnd);
              } else
                handler->Handle_Fd_Event(fd, (event & FD_EVENT_RD) != 0,
                  (event & FD_EVENT_WR) != 0,
                  (event & FD_EVENT_ERR) != 0);
              if (tmHnd != 0 && tmHnd->list == &timedList)
                tmHnd->hasEvent = TRUE;
            }
          }
#ifndef USE_EPOLL
          pollFd->revents = 0;
#endif
        }
      } catch(...) {
#ifdef USE_EPOLL
        FdMap::epollUnmarkFds(nEvents);
#else
        FdMap::pollUnfreeze();
#endif
        throw;
      }
#ifdef USE_EPOLL
      FdMap::epollUnmarkFds(nEvents);
#else
      FdMap::pollUnfreeze();
#endif
      // Call handlers with old API without timer
      for (oldApiCallList.first(); !oldApiCallList.finished(); ) {
        Fd_And_Timeout_Event_Handler * handler = oldApiCallList.current();
        oldApiCallList.next();
        oldApiCallList.remove(handler);
        // Check if the event handler was uninstalled in the meanwhile
        // (Currently the check is superfluous as an other event handler
        //  may not uninstall this event handler.)
        if (handler->fdSets == 0) continue;
        // Get the common set of received and waited events
        // Check if the set contains any element
        if ( fdSetsToHnds->setAnd(*fdSetsReceived, *handler->fdSets) ) {
          double current_time = TTCN_Snapshot::time_now();
          double time_since_last_call = current_time -
            handler->last_called;
          handler->last_called = current_time;
          handler->Event_Handler(fdSetsToHnds->getReadFds(),
            fdSetsToHnds->getWriteFds(), fdSetsToHnds->getErrorFds(),
            time_since_last_call);
        }
      }
    }
    // Call timeout handlers (also handlers with old API with timer)
    double current_time = TTCN_Snapshot::time_now();
    for  (timedList.first(); !timedList.finished(); ) {
      Fd_And_Timeout_Event_Handler * handler = timedList.current();
      timedList.next(); // The handler may be removed from the list
      if (handler->getIsOldApi())
        handler->hasEvent =
          fdSetsToHnds->setAnd(*fdSetsReceived, *handler->fdSets);
      boolean callHandler = (handler->hasEvent && handler->isTimeout) ?
        handler->callAnyway :
      current_time > (handler->last_called + handler->callInterval);
      if ( !handler->isPeriodic &&
        (callHandler || (handler->hasEvent && handler->isTimeout)) ) {
        handler->callInterval = 0.0;
        timedList.remove(handler);
      }
      handler->hasEvent = FALSE;
      if (callHandler) {
        double time_since_last_call = current_time - handler->last_called;
        handler->last_called = current_time;
        if (!handler->getIsOldApi())
          handler->Handle_Timeout(time_since_last_call);
        else if (handler->fdSets != 0)
          handler->Event_Handler(fdSetsToHnds->getReadFds(),
            fdSetsToHnds->getWriteFds(), fdSetsToHnds->getErrorFds(),
            time_since_last_call);
        current_time = TTCN_Snapshot::time_now();
      }
    }
    is_in_call_handlers = FALSE;
  } catch (...) { oldApiCallList.clear(); is_in_call_handlers = FALSE; throw; }
}

int Fd_And_Timeout_User::receiveEvents(int pollTimeout)
{
  int ret_val;
#ifdef USE_EPOLL
  ret_val = epoll_wait(FdMap::epollFd,
    FdMap::epollEvents, FdMap::MAX_EPOLL_EVENTS, pollTimeout);
  if (ret_val < 0 && errno != EINTR)
    TTCN_error("System call epoll_wait() failed when taking a new snapshot.");
#else
  ret_val = poll(FdMap::getPollFds(), FdMap::getSize(), pollTimeout);
  if (ret_val < 0 && errno != EINTR)
    TTCN_error("System call poll() failed when taking a new snapshot.");
#endif
  return ret_val;
}

#ifdef USE_EPOLL
void Fd_And_Timeout_User::reopenEpollFd()
{
  if (FdMap::epollFd != -1) { close (FdMap::epollFd); FdMap::epollFd = -1; }
  FdMap::epollFd = epoll_create(16 /* epoll size hint */);
  // FIXME: method to determine the optimal epoll size hint
  // epoll size hint is ignored in newer kernels
  // for older kernels: it should be big enough not to be too slow
  //                    and it should not be very big to limit memory usage
  if (FdMap::epollFd < 0)
    TTCN_error("System call epoll_create() failed in child process.");

  if (FdMap::getSize() != 1)
    TTCN_error("Fd_And_Timeout_User::reopenEpollFd: Internal error");
}
#endif





/******************************************************************************
 * class TTCN_Snapshot                                                        *
 ******************************************************************************/

boolean TTCN_Snapshot::else_branch_found;
double TTCN_Snapshot::alt_begin;

void TTCN_Snapshot::initialize()
{
  long openMax = sysconf(_SC_OPEN_MAX);
  int fdLimit = (openMax <= (long) MAX_INT_VAL) ? (int) openMax : MAX_INT_VAL;

  FdMap::initialize(fdLimit);
  Fd_And_Timeout_User::initialize();
#ifdef USE_EPOLL
  FdMap::epollFd = epoll_create(16 /* epoll size hint */);
  // FIXME: method to determine the optimal epoll size hint
  // epoll size hint is ignored in newer kernels
  // for older kernels: it should be big enough not to be too slow
  //                    and it should not be very big to limit memory usage
  if (FdMap::epollFd < 0)
    TTCN_error("TTCN_Snapshot::initialize: "
      "System call epoll_create() failed.");
#endif
  else_branch_found = FALSE;
  alt_begin = time_now();
}

void TTCN_Snapshot::check_fd_setsize()
{
  if ((long) FdMap::getFdLimit() > (long) FD_SETSIZE)
    TTCN_Logger::log_fd_limits(FdMap::getFdLimit(), (long) FD_SETSIZE);
}

void TTCN_Snapshot::terminate()
{
#ifdef USE_EPOLL
  if (FdMap::epollFd != -1) { close (FdMap::epollFd); FdMap::epollFd = -1; }
#endif
  Fd_And_Timeout_User::terminate();
  FdMap::terminate();
}

void TTCN_Snapshot::else_branch_reached()
{
  if (!else_branch_found) {
    else_branch_found = TRUE;
    TTCN_warning("An [else] branch of an alt construct has been reached. "
      "Re-configuring the snapshot manager to call the event handlers "
      "even when taking the first snapshot.");
  }
}

double TTCN_Snapshot::time_now()
{
  static time_t start_time;
  static boolean first_call = TRUE;
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1)
    TTCN_error("gettimeofday() system call failed.");
  if (first_call) {
    start_time = tv.tv_sec;
    first_call = FALSE;
  }
  return (double)(tv.tv_sec - start_time) + 1e-6 * (double)tv.tv_usec;
}


void TTCN_Snapshot::take_new(boolean block_execution)
{
  if (block_execution || else_branch_found) {

    // jump here if epoll()/poll()/select() was interrupted by a signal
    again:
    // determine the timeout value for epoll()/poll()/select()
    double timeout = 0.0;
    int pollTimeout = 0; // timeout for poll/epoll
    boolean handleTimer = FALSE;
    if (block_execution) {
      // find the earliest timeout
      double timer_timeout, handler_timeout = 0.0;
      boolean is_timer_timeout = TIMER::get_min_expiration(timer_timeout);
      boolean is_handler_timeout =
        Fd_And_Timeout_User::getTimeout(&handler_timeout);
      if (is_timer_timeout) {
        if (is_handler_timeout && handler_timeout < timer_timeout)
          timeout = handler_timeout;
        else timeout = timer_timeout;
      } else if (is_handler_timeout) timeout = handler_timeout;
      if (is_timer_timeout || is_handler_timeout) {
        // there are active TTCN-3 or test port timers
        double current_time = time_now();
        double block_time = timeout - current_time;
        if (block_time > 0.0) {
          // the first timeout is in the future: blocking is needed
          // filling up tv with appropriate values
          if (block_time < (double)MAX_BLOCK_TIME) {
            pollTimeout = static_cast<int>(floor(block_time*1000));
            handleTimer = TRUE;
          } else {
            // issue a warning: the user probably does not want such
            // long waiting
            TTCN_warning("The time needed for the first timer "
              "expiry is %g seconds. The operating system does "
              "not support such long waiting at once. The "
              "maximum time of blocking was set to %d seconds "
              "(ca. %d days).", block_time, MAX_BLOCK_TIME,
              MAX_BLOCK_TIME / 86400);
            // also modify the the timeout value to get out
            // immediately from the while() loop below
            timeout = current_time + (double)MAX_BLOCK_TIME;
            pollTimeout = MAX_BLOCK_TIME * 1000;
            handleTimer = TRUE;
          }
        } else {
          // first timer is already expired: do not block
          // pollTimeout is 0
          handleTimer = TRUE;
        }
      } else {
        // no active timers: infinite timeout
        pollTimeout = -1;
      }
    } else {
      // blocking was not requested (we are in a first snapshot after an
      // [else] branch): do not block - pollTimeout is 0
    }

    if (FdMap::getSize() == 0 && pollTimeout < 0)
      TTCN_error("There are no active timers and no installed event "
        "handlers. Execution would block forever.");

    int ret_val = 0;
    if (FdMap::getSize() != 0) {
      ret_val = Fd_And_Timeout_User::receiveEvents(pollTimeout);
      // call epoll_wait() / poll()
      if (ret_val < 0) { /* EINTR - signal */ errno = 0; goto again; }
    } else {
      // There isn't any file descriptor to check
      if (pollTimeout > 0) {
        // waiting only for a timeout in the future
        timeval tv;
        tv.tv_sec = pollTimeout / 1000;
        tv.tv_usec = (pollTimeout % 1000) * 1000;
        ret_val = select(0, NULL, NULL, NULL, &tv);
        if (ret_val < 0 && errno == EINTR) {
          /* signal */ errno = 0; goto again;
        }
        if ((ret_val < 0 && errno != EINTR) || ret_val > 0)
          TTCN_error("System call select() failed when taking a new "
            "snapshot.");
      } else if (pollTimeout < 0)
        TTCN_error("There are no active timers or installed event "
          "handlers. Execution would block forever.");
    }

    if (ret_val > 0) {
      Fd_And_Timeout_User::call_handlers(ret_val);
    } else if (ret_val == 0 && handleTimer) {
      // if select() returned because of the timeout, but too early
      // do an other round if it has to wait much,
      // or do a busy wait if only a few cycles are needed
      if (pollTimeout > 0){
        double difference = time_now() - timeout;
        if(difference < 0.0){
          if(difference < -0.001){
            errno = 0;
            goto again;
          } else {
            while (time_now() < timeout) ;
          }
        }
      }
      Fd_And_Timeout_User::call_handlers(0); // Call timeout handlers
    }
  }
  // just update the time and check the testcase guard timer if blocking was
  // not requested and there is no [else] branch in the test suite

  alt_begin = time_now();

  if (testcase_timer.timeout() == ALT_YES)
    TTCN_error("Guard timer has expired. Execution of current test case "
      "will be interrupted.");
}

void TTCN_Snapshot::block_for_sending(int send_fd, Fd_Event_Handler * handler)
{
  // To be backward compatible: handler is optional
  if (Fd_And_Timeout_User::get_is_in_call_handlers())
    TTCN_error("TTCN_Snapshot::block_for_sending: The function may not be "
      "called from event handler");
  Fd_Event_Handler * hnd = 0;
  if ((FdMap::find(send_fd, &hnd) & FD_EVENT_WR) != 0)
    TTCN_error("TTCN_Snapshot::block_for_sending: An event handler already "
      "waits for file descriptor %d to be writable", send_fd);
  if (handler != 0 && hnd != 0 && handler != hnd)
    TTCN_error("TTCN_Snapshot::block_for_sending: File descriptor %d "
      "already has a handler, which is different from the currently "
      "specified.", send_fd);
  static Fd_And_Timeout_Event_Handler dummyHandler;
  if (hnd == 0) { hnd = (handler != 0) ? handler : &dummyHandler; }
  Fd_And_Timeout_User::add_fd(send_fd, hnd, FD_EVENT_WR);
  for ( ; ; ) {
    int ret_val = Fd_And_Timeout_User::receiveEvents(-1); // epoll / poll
    if (ret_val >= 0) {
    boolean writable = FALSE;
    boolean readable  = FALSE;
#ifdef USE_EPOLL

      for (int i = 0; i < ret_val; ++i) {
        if (FdMap::epollEvents[i].data.fd == send_fd) {
          readable = TRUE;
          if ((FdMap::epollEvents[i].events & EPOLLOUT) != 0){
            writable = TRUE;
          }
          break;
        }
      }
#else
      if (FdMap::getPollREvent(send_fd) != 0) {readable = TRUE;}
      if ((FdMap::getPollREvent(send_fd) & FD_EVENT_WR) != 0) {writable = TRUE;}
#endif
      if (writable) break;
      Fd_And_Timeout_User::call_handlers(ret_val);
      if (readable) break;
    }
    // interrupted - EINTR
  }
  Fd_And_Timeout_User::remove_fd(send_fd, hnd, FD_EVENT_WR);
#ifndef USE_EPOLL
  // As Fd_And_Timeout_User::call_handlers is not called:
  // received events should be cleared at this point
  // (Most probably the behavior would be correct without clearing it.)
  int nPollFds = FdMap::getSize();
  pollfd * pollFds = FdMap::getPollFds();
  for (int i = 0; i < nPollFds; ++i) pollFds[i].revents = 0;
#endif
}

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
 *   Szabados, Kristof
 *
 ******************************************************************************/
#ifndef FD_AND_TIMEOUT_USER_HH
#define FD_AND_TIMEOUT_USER_HH

// These header files contain the definitions of classes:
//   FdMap, Handler_List and Fd_And_Timeout_User; and structure FdSets.
// These are used for socket and timeout event handling and
// provide services for classes TTCN_Communication, TTCN_Snapshot and PORT.
// Emulation of the backward compatible event handler interface is done
// in class Fd_And_Timeout_User.
// This file is not part of the binary distribution.
// The member functions are implemented in Snapshot.cc.


#include <sys/types.h>
#include "poll.h"
#ifdef USE_EPOLL

#if defined(__GNUC__) && (__GNUC__ < 3)
// __op is the name of one of the parameters of epoll_ctl()
// But for GCC 2, __op is a synonym of "operator"
#define __op p__op
#endif

#include "sys/epoll.h"

#ifdef p__op
#undef p__op
#endif

#endif

#include "Types.h"
#include "Error.hh"
#include "Event_Handler.hh"


class FdMap {
public:
  /** The return value contains the event value before the add operation */
  static fd_event_type_enum add(int fd, Fd_Event_Handler * handler,
    fd_event_type_enum event);
  /** The return value contains the event value before the remove operation */
  static fd_event_type_enum remove(int fd, const Fd_Event_Handler * handler,
    fd_event_type_enum event);
  static fd_event_type_enum find(int fd, Fd_Event_Handler * * handler);
  static inline int getSize() { return nItems; }
  static inline int getFdLimit() { return capacity; }
#ifdef USE_EPOLL
  static void initialize(int fdLimit) {
    nItems = 0; capacity = fdLimit; items2 = 0; epollFd = -1;
    for (int i = 0; i < ITEM1_CAPACITY; ++i) items1[i].init(); // debugging
  }
  static void terminate() { if (items2 != 0) { delete[] items2; items2=0; } }
#else
  static void initialize(int fdLimit) {
    nItems = 0; capacity = fdLimit; items2 = 0; pollFds2 = 0;
    needUpdate = FALSE; nPollFdsFrozen = 0;
    for (int i = 0; i < ITEM1_CAPACITY; ++i) {
      items1[i].init(); // for debugging
      init(pollFds1[i++]); // for debugging
    }
  }
  static void terminate() {
    if (items2 != 0) { delete[] items2; items2 = 0; }
    if (pollFds2 != 0) { delete[] pollFds2; pollFds2 = 0; }
  }
  static inline pollfd * getPollFds() {
    return (pollFds2 == 0) ? pollFds1 : pollFds2;
  }
#endif
  // implementation dependent public interface
  // only for better performance
  static inline boolean isItems1Used() { return items2 == 0; }
#ifdef USE_EPOLL
  static inline fd_event_type_enum item1atIndex(int i, int * fd,
    Fd_Event_Handler * * handler) {
    *fd = items1[i].fd;
    *handler = items1[i].d.hnd;
    return epollEventToEvent(items1[i].d.evt);
  }
  static inline fd_event_type_enum item2atFd(int fd,
    Fd_Event_Handler * * handler) {
    *handler = items2[fd].hnd;
    return epollEventToEvent(items2[fd].evt);
  }
#else
  static inline fd_event_type_enum item1atIndex(int i, int * fd,
    Fd_Event_Handler * * handler) {
    *fd = items1[i].fd;
    *handler = items1[i].d.hnd;
    return pollEventToEvent(getPollFds()[items1[i].d.ixPoll].events);
  }
  static inline fd_event_type_enum item2atFd(int fd,
    Fd_Event_Handler * * handler) {
    *handler = items2[fd].hnd;
    return pollEventToEvent(getPollFds()[items2[fd].ixPoll].events);
  }
#endif
private:
  struct Data {
#ifdef USE_EPOLL
    short               evt; // fd_event_type_enum with size of short
    short               ixE; // only for invalidating epoll_event entries
    Fd_Event_Handler    * hnd;
    inline Data() : evt(0), ixE(-1), hnd(0) {}
    inline void init() { evt = 0; ixE = -1; hnd = 0; }
#else
    int                 ixPoll; // index to poll array
    Fd_Event_Handler    * hnd;
    inline Data() : ixPoll(-1), hnd(0) {}
    inline void init() { ixPoll = -1; hnd = 0; }
#endif
  };
  struct Item {
    int     fd;
    Data    d;
    inline Item() : fd(-1), d() {}
    inline void init() { fd = -1; d.init(); }
  };
  static int  nItems;
  static int  capacity;
  static inline int findInItems1(int fd) {
    if (nItems < 2) return (nItems == 1 && fd == items1[0].fd) ? 0 : -1;
    int i = 0, j = nItems, k;
    do {
      k = (i + j) >> 1;
      if (fd < items1[k].fd) j = k; else i = k;
    } while (j - i > 1);
    return (fd == items1[i].fd) ? i : -1;
  }
  static inline int findInsPointInItems1(int fd) {
    if (nItems < 2) return (nItems == 1 && fd > items1[0].fd) ? 1 : 0;
    int i = 0, j = nItems, k;
    do {
      k = (i + j) >> 1;
      if (fd < items1[k].fd) j = k; else i = k;
    } while (j - i > 1);
    return (fd <= items1[i].fd) ? i : j;
  }
  static inline boolean findInItems2(int fd) {
#ifdef USE_EPOLL
return items2[fd].hnd != 0;
#else
  return items2[fd].ixPoll >= 0;
  //Note: Frozen deleted items has 0 hnd
#endif
  }
  // not implemented methods
  FdMap();
  FdMap(const FdMap &);
  const FdMap & operator=(const FdMap &);
  // implementation dependent member variables
  static const int    ITEM1_CAPACITY = 16;
  static const int    ITEM1_CAPACITY_LOW = 8;
  static Item     items1[ITEM1_CAPACITY];
  static Data     * items2;
#ifndef USE_EPOLL
  static pollfd   pollFds1[ITEM1_CAPACITY];
  static pollfd   * pollFds2;
  // pollFd2, items2 might be rewritten
  static inline void init(pollfd & p) { p.fd = -1; p.events = p.revents = 0; }
  static boolean         needUpdate;
  static int          nPollFdsFrozen;
  static void copyItems2ToItems1();
#endif
public:
#ifdef USE_EPOLL
  static const int    MAX_EPOLL_EVENTS = 64;
  static int          epollFd;
  static epoll_event  epollEvents[MAX_EPOLL_EVENTS];
  static boolean epollMarkFds(int nEvents);
  static void epollUnmarkFds(int nEvents);
  static inline __uint32_t eventToEpollEvent(fd_event_type_enum event) {
    __uint32_t epollEvent = 0;
    if ((event & FD_EVENT_RD) != 0) epollEvent |= EPOLLIN;
    if ((event & FD_EVENT_WR) != 0) epollEvent |= EPOLLOUT;
    if ((event & FD_EVENT_ERR) != 0) epollEvent |= EPOLLERR;
    return epollEvent;
  }
  static inline fd_event_type_enum epollEventToEvent(__uint32_t epollEvent) {
    int event = 0;
    if ((epollEvent & (EPOLLIN | EPOLLHUP)) != 0) event |= FD_EVENT_RD;
    if ((epollEvent & EPOLLOUT) != 0) event |= FD_EVENT_WR;
    if ((epollEvent & EPOLLERR) != 0) event |= FD_EVENT_ERR;
    return static_cast<fd_event_type_enum>(event);
  }
  static inline __uint32_t eventToEpollEvent(int event) {
    return eventToEpollEvent(static_cast<fd_event_type_enum>(event));
  }
#else
  static void pollFreeze();
static void pollUnfreeze();
static fd_event_type_enum getPollREvent(int fd);
static inline short eventToPollEvent(fd_event_type_enum event) {
  short pollEvent = 0;
  if ((event & FD_EVENT_RD) != 0) pollEvent |= POLLIN;
  if ((event & FD_EVENT_WR) != 0) pollEvent |= POLLOUT;
  if ((event & FD_EVENT_ERR) != 0) pollEvent |= POLLERR;
  return pollEvent;
}
static inline fd_event_type_enum pollEventToEvent(short pollEvent) {
  int event = 0;
  if ((pollEvent & (POLLIN | POLLHUP)) != 0) event |= FD_EVENT_RD;
  if ((pollEvent & POLLOUT) != 0) event |= FD_EVENT_WR;
  if ((pollEvent & POLLERR) != 0) event |= FD_EVENT_ERR;
  return static_cast<fd_event_type_enum>(event);
}
static inline short eventToPollEvent(int event) {
  return eventToPollEvent(static_cast<fd_event_type_enum>(event));
}
static const short pollEventMask = POLLIN | POLLOUT | POLLERR | POLLHUP;
#endif
};





class FdSets {
  /** FdSets is needed only for the old event handler API */
  fd_set read_fds, write_fds, error_fds;
public:
  inline FdSets() { clear(); }
  inline void clear() {
    FD_ZERO(&read_fds); FD_ZERO(&write_fds); FD_ZERO(&error_fds);
  }
  inline const fd_set * getReadFds() const { return &read_fds; }
  inline const fd_set * getWriteFds() const { return &write_fds; }
  inline const fd_set * getErrorFds() const { return &error_fds; }
  inline void add(int fd, fd_event_type_enum event) {
    if (fd >= (int)FD_SETSIZE)
      TTCN_error("FdSets::add: fd (%i) >= FD_SETSIZE (%i)",fd,FD_SETSIZE);
    if ((event & FD_EVENT_RD) != 0) { FD_SET(fd, &read_fds); }
    if ((event & FD_EVENT_WR) != 0) { FD_SET(fd, &write_fds); }
    if ((event & FD_EVENT_ERR) != 0) { FD_SET(fd, &error_fds); }
  }
  inline void remove(int fd, fd_event_type_enum event) {
    if (fd >= (int)FD_SETSIZE)
      TTCN_error("FdSets::remove: fd (%i) >= FD_SETSIZE (%i)",
        fd, FD_SETSIZE);
    if ((event & FD_EVENT_RD) != 0) { FD_CLR(fd, &read_fds); }
    if ((event & FD_EVENT_WR) != 0) { FD_CLR(fd, &write_fds); }
    if ((event & FD_EVENT_ERR) != 0) { FD_CLR(fd, &error_fds); }
  }
  inline fd_event_type_enum getEvent(int fd) const {
    return static_cast<fd_event_type_enum>(
      (FD_ISSET(fd, &read_fds) ? FD_EVENT_RD : 0) |
      (FD_ISSET(fd, &write_fds) ? FD_EVENT_WR : 0) |
      (FD_ISSET(fd, &error_fds) ? FD_EVENT_ERR : 0) );
  }
  static inline fd_event_type_enum getEvent(const fd_set * rs,
    const fd_set * ws, const fd_set * es, int fd) {
    return static_cast<fd_event_type_enum>(
      ((rs != 0 && FD_ISSET(fd, rs)) ? FD_EVENT_RD : 0) |
      ((ws != 0 && FD_ISSET(fd, ws)) ? FD_EVENT_WR : 0) |
      ((es != 0 && FD_ISSET(fd, es)) ? FD_EVENT_ERR : 0) );
  }
private:
  // Note: Platform dependent implementation
  // Operations done an all fds of the sets are done faster
  static const int NOBPI = sizeof(long int) * 8; // N.Of Bits Per Item
  static const int NOI = FD_SETSIZE / NOBPI; // N.Of Items
  //static const int NOI_MAX = (FD_SETSIZE + NOBPI - 1) / NOBPI;
  static const int NORB = FD_SETSIZE % NOBPI; // Remaining Bits
  static const int RBM = (1 << NORB) - 1; // Mask for Remaining Bits
  static inline boolean fdSetOr (fd_set & fdSet, const fd_set & fdSet2) {
    long orV = 0;
    for (int i = 0; i < NOI; ++i)
      orV |= ( ((long*)&fdSet)[i] |= ((const long*)&fdSet2)[i] );
    if (NORB > 0)
      orV |= RBM & ( ((long*)&fdSet)[NOI] |= ((const long*)&fdSet2)[NOI]);
    return orV != 0;
  }
  static inline boolean fdSetSubs (fd_set & fdSet, const fd_set & fdSet2) {
    long orV = 0;
    for (int i = 0; i < NOI; ++i)
      orV |= ( ((long*)&fdSet)[i] &= ~((const long*)&fdSet2)[i] );
    if (NORB > 0)
      orV |= RBM & (((long*)&fdSet)[NOI] &= ~((const long*)&fdSet2)[NOI]);
    return orV != 0;
  }
  static inline boolean fdSetAnd (fd_set & fdSet, const fd_set & fdSet2) {
    long orV = 0;
    for (int i = 0; i < NOI; ++i)
      orV |= ( ((long*)&fdSet)[i] &= ((const long*)&fdSet2)[i] );
    if (NORB > 0)
      orV |= RBM & (((long*)&fdSet)[NOI] &= ((const long*)&fdSet2)[NOI]);
    return orV != 0;
  }
  static inline boolean fdSetAnd (fd_set & fdSet,
    const fd_set & fdSet1, const fd_set & fdSet2) {
    long orV = 0;
    for (int i = 0; i < NOI; ++i)
      orV |= ( ((long*)&fdSet)[i] =
        (((const long*)&fdSet1)[i] & ((const long*)&fdSet2)[i]) );
    if (NORB > 0)
      orV |= RBM & ( ((long*)&fdSet)[NOI] =
        (((const long*)&fdSet1)[NOI] & ((const long*)&fdSet2)[NOI]) );
    return orV != 0;
  }
  inline long anyDiff(const FdSets & fdSet, int i) const {
    return (((const long*)&read_fds)[i] ^
      ((const long*)&fdSet.read_fds)[i]) |
      (((const long*)&write_fds)[i] ^
        ((const long*)&fdSet.write_fds)[i]) |
        (((const long*)&error_fds)[i] ^
          ((const long*)&fdSet.error_fds)[i]);
  }
  inline long anyDiff(const fd_set * rs, const fd_set * ws, const fd_set * es,
    int i) const {
    return (((const long*)&read_fds)[i] ^ ((rs!=0)?((const long*)rs)[i]:0))|
      (((const long*)&write_fds)[i] ^((ws!=0)?((const long*)ws)[i]:0))|
      (((const long*)&error_fds)[i] ^((es!=0)?((const long*)es)[i]:0));
  }
  inline long anySet(int i) const {
    return ((const long*)&read_fds)[i] | ((const long*)&write_fds)[i] |
      ((const long*)&error_fds)[i];
  }
  inline long bothAnySet(const fd_set * rs, const fd_set * ws, const fd_set * es,
    int i) const { // both fd_set triplet has something set
    return (((const long*)&read_fds)[i] |
      ((const long*)&write_fds)[i] |
      ((const long*)&error_fds)[i])      &
      (((rs!=0)?((const long*)rs)[i]:0) |
        ((ws!=0)?((const long*)ws)[i]:0) |
        ((es!=0)?((const long*)es)[i]:0));
  }
  inline long anyAnd(const FdSets & fdSet, int i) const {
    return (((const long*)&read_fds)[i] &
      ((const long*)&fdSet.read_fds)[i]) |
      (((const long*)&write_fds)[i] &
        ((const long*)&fdSet.write_fds)[i]) |
        (((const long*)&error_fds)[i] &
          ((const long*)&fdSet.error_fds)[i]);
  }
  static inline int lowestBit(long m) {
    int i = 0;
    while ((m & 0xFF) == 0) { m >>= 8l; i += 8; }
    while ((m & 1) == 0) { m >>= 1l; ++i; }
    return i;
  }
public:
  inline boolean setOr(const FdSets & fdSet) {
    return fdSetOr(read_fds, fdSet.read_fds) |
      fdSetOr(write_fds, fdSet.write_fds) |
      fdSetOr(error_fds, fdSet.error_fds);
  }
  inline boolean setSubstract(const FdSets & fdSet) {
    return fdSetSubs(read_fds, fdSet.read_fds) |
      fdSetSubs(write_fds, fdSet.write_fds) |
      fdSetSubs(error_fds, fdSet.error_fds);
  }
  inline boolean setAnd(const FdSets & fdSet) {
    return fdSetAnd(read_fds, fdSet.read_fds) |
      fdSetAnd(write_fds, fdSet.write_fds) |
      fdSetAnd(error_fds, fdSet.error_fds);
  }
  inline boolean setAnd(const FdSets & fdSet1, const FdSets & fdSet2) {
    return fdSetAnd(read_fds, fdSet1.read_fds, fdSet2.read_fds) |
      fdSetAnd(write_fds, fdSet1.write_fds, fdSet2.write_fds) |
      fdSetAnd(error_fds, fdSet1.error_fds, fdSet2.error_fds);
  }
  /** Gives back the first fd where any of the three sets differ
  * in interval fd, ... fdLimit - 1
  */
  inline int getIxDiff(const fd_set * rs, const fd_set * ws,
    const fd_set * es, int fd, int fdLimit) const {
    int iL = fdLimit / NOBPI;
    int i = fd / NOBPI;
    long m;
    if (i < iL) {
      m = (unsigned long) anyDiff(rs, ws, es, i) >>
        (unsigned long) (fd % NOBPI);
      if (m != 0) return fd + lowestBit(m);
      for (++i; i < iL; ++i)
        if ((m = anyDiff(rs, ws, es, i)) != 0)
          return i * NOBPI + lowestBit(m);
      long rbm = ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
      if (rbm != 0 && (m = anyDiff(rs, ws, es, i) & rbm) != 0 )
        return i * NOBPI + lowestBit(m);
    } else {
      if (fd >= fdLimit) return fdLimit;
      // i == iL, unlikely case: fdLimit % NOBPI != 0
        m = (unsigned long) anyDiff(rs, ws, es, i) &
          ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
        m >>= (unsigned long) (fd % NOBPI);
        if (m != 0) return fd + lowestBit(m);
    }
    return fdLimit;
  }
  inline int getIxSet(int fd, int fdLimit) const {
    int iL = fdLimit / NOBPI;
    int i = fd / NOBPI;
    long m;
    if (i < iL) {
      m = (unsigned long) anySet(i) >> (unsigned long) (fd % NOBPI);
      if (m != 0) return fd + lowestBit(m);
      for (++i; i < iL; ++i)
        if ((m = anySet(i)) != 0)
          return i * NOBPI + lowestBit(m);
      long rbm = ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
      if (rbm != 0 && (m = anySet(i) & rbm) != 0)
        return i * NOBPI + lowestBit(m);
    } else {
      if (fd >= fdLimit) return fdLimit;
      // i == iL
        m = (unsigned long) anySet(i) &
        ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
        m >>= (unsigned long) (fd % NOBPI);
        if (m != 0) return fd + lowestBit(m);
    }
    return fdLimit;
  }
  /** Gives back the first fd where both set triplets has something set
  * in interval fd, ... fdLimit - 1
  * (Used for special purpose: see Snapshot.cc: method set_fds_with_fd_sets)
  */
  inline int getIxBothAnySet(const fd_set * rs, const fd_set * ws,
    const fd_set * es, int fd, int fdLimit) const {
    int iL = fdLimit / NOBPI;
    int i = fd / NOBPI;
    long m;
    if (i < iL) {
      m = (unsigned long) bothAnySet(rs, ws, es, i) >>
        (unsigned long) (fd % NOBPI);
      if (m != 0) return fd + lowestBit(m);
      for (++i; i < iL; ++i)
        if ((m = bothAnySet(rs, ws, es, i)) != 0)
          return i * NOBPI + lowestBit(m);
      long rbm = ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
      if (rbm != 0 && (m = bothAnySet(rs, ws, es, i) & rbm) != 0 )
        return i * NOBPI + lowestBit(m);
    } else {
      if (fd >= fdLimit) return fdLimit;
      // i == iL, unlikely case: fdLimit % NOBPI != 0
        m = (unsigned long) bothAnySet(rs, ws, es, i) &
          ((1ul << (unsigned long) (fdLimit % NOBPI)) - 1);
        m >>= (unsigned long) (fd % NOBPI);
        if (m != 0) return fd + lowestBit(m);
    }
    return fdLimit;
  }
};





class Handler_List {
public:
  Handler_List() : head(), tail(), cur(0) {
    head.next = &tail; tail.prev = &head;
  }
  inline void first() { cur = head.next; }
  inline void next() { cur = cur->next; }
  inline boolean finished() const { return cur == &tail; }
  inline Fd_And_Timeout_Event_Handler * current() const { return cur; }
  inline void add(Fd_And_Timeout_Event_Handler * handler) {
    if (handler->list != 0 || handler->prev != 0 || handler->next != 0)
      TTCN_error("Handler_List::add: Error in parameter"); //debug
    tail.prev->next = handler; handler->prev = tail.prev;
    tail.prev = handler; handler->next = &tail;
    handler->list = this;
  }
  inline void remove(Fd_And_Timeout_Event_Handler * handler) {
    if (handler->list != this)
      TTCN_error("Handler_List::remove: Error in parameter"); //debug
    handler->prev->next = handler->next;
    handler->next->prev = handler->prev;
    handler->prev = 0; handler->next = 0; handler->list = 0;
  }
  inline void clear() {
    Fd_And_Timeout_Event_Handler * hnd = head.next, * h;
    while (hnd != &tail) {
      h = hnd->next; hnd->prev = 0; hnd->next = 0; hnd->list = 0; hnd = h;
    }
    head.next = &tail; tail.prev = &head; cur = 0;
  }
private:
  Fd_And_Timeout_Event_Handler head, tail, * cur;
};





class Fd_And_Timeout_User {
  static inline void checkFd(int fd);
public:
  static void initialize() {
    fdSetsReceived = 0; fdSetsToHnds = 0;
    nOldHandlers = 0; is_in_call_handlers = FALSE; curRcvdEvtIx = -1;
  }
  static void terminate() {
    if (fdSetsReceived != 0) { delete fdSetsReceived; fdSetsReceived = 0; }
    if (fdSetsToHnds != 0) { delete fdSetsToHnds; fdSetsToHnds = 0; }
  }
  static void add_fd(int fd, Fd_Event_Handler * handler,
    fd_event_type_enum event);
  static void remove_fd(int fd, Fd_Event_Handler * handler,
    fd_event_type_enum event);
  static void set_timer(Fd_And_Timeout_Event_Handler * handler,
    double call_interval, boolean is_timeout = TRUE,
    boolean call_anyway = TRUE, boolean is_periodic = TRUE);

  static void set_fds_with_fd_sets(Fd_And_Timeout_Event_Handler * handler,
    const fd_set *read_fds, const fd_set *write_fds,
    const fd_set *error_fds);
  static void remove_all_fds(Fd_And_Timeout_Event_Handler * handler);

  static boolean getTimeout(double * timeout);
  static void call_handlers(int nEvents);
  static int receiveEvents(int pollTimeout);
  static inline fd_event_type_enum getCurReceivedEvent() {
    // Checks are included because the test port might erroneously call
    // Handle_Fd_Event
#ifdef USE_EPOLL
    if (curRcvdEvtIx < 0 || curRcvdEvtIx >= FdMap::MAX_EPOLL_EVENTS)
      return static_cast<fd_event_type_enum>(0);
    return
    FdMap::epollEventToEvent(FdMap::epollEvents[curRcvdEvtIx].events);
#else
    if (curRcvdEvtIx < 0 || curRcvdEvtIx >= FdMap::getSize())
      return static_cast<fd_event_type_enum>(0);
    return
    FdMap::pollEventToEvent(FdMap::getPollFds()[curRcvdEvtIx].revents);
#endif
  }
  static inline boolean get_is_in_call_handlers() { return is_in_call_handlers; }
#ifdef USE_EPOLL
  static void reopenEpollFd();
#else
  static inline void reopenEpollFd() {}
#endif

private:
  static Handler_List timedList, oldApiCallList;
  static FdSets * fdSetsReceived; // active events for the old event handlers
  static FdSets * fdSetsToHnds; // temporary storage to pass it to the handler
  static int nOldHandlers;
  static boolean is_in_call_handlers;
  static int curRcvdEvtIx; // Current Received Event Index - see call_handlers
};

#endif

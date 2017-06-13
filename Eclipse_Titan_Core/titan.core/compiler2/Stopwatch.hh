/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef STOPWATCH_HH_
#define STOPWATCH_HH_

#include <sys/time.h>

class Stopwatch {
public:
  Stopwatch(const char *name);
  ~Stopwatch();
private:
  struct timeval tv_start;
  const char *my_name; // not owned
private:
  Stopwatch(const Stopwatch&); // no copy
  Stopwatch& operator=(const Stopwatch&); // no assignment
};

#ifdef NDEBUG
#define STOPWATCH(s) ((void)(s))
#else
#define STOPWATCH(s) Stopwatch chrono_meter(s)
#endif

#endif /* STOPWATCH_HH_ */

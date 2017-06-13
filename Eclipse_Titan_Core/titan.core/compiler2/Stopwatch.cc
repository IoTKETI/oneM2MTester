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
#include "Stopwatch.hh"
#include "error.h"
#include <stdio.h>

Stopwatch::Stopwatch(const char *name)
: tv_start()
, my_name(name)
{
  gettimeofday(&tv_start, 0);
}

Stopwatch::~Stopwatch()
{
  struct timeval tv_end;
  gettimeofday(&tv_end, 0);

  struct timeval tv_diff = {
    tv_end.tv_sec  - tv_start.tv_sec,
    tv_end.tv_usec - tv_start.tv_usec
  };
  if (tv_diff.tv_usec < 0) {
    --tv_diff.tv_sec;
    tv_diff.tv_usec += 1000000;
  }

  NOTIFY("%s took %ld.%06ld sec", my_name,
    (long)tv_diff.tv_sec, (long)tv_diff.tv_usec);
}

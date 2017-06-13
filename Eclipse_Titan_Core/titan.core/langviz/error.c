/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *
 ******************************************************************************/
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

unsigned verb_level=0x0007; /* default value */

const char *argv0; /* the programname :) */

void fatal_error(const char *filename, int lineno, const char *fmt, ...)
{
  va_list parameters;
  fprintf(stderr, "FATAL ERROR: %s: In line %d of %s: ",
          argv0, lineno, filename);
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
  abort();
}

void ERROR(const char *fmt, ...)
{
  fprintf(stderr, "%s: error: ", argv0);
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
}

void WARNING(const char *fmt, ...)
{
  if(!(verb_level & 2)) return;
  fprintf(stderr, "%s: warning: ", argv0);
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
}

void NOTSUPP(const char *fmt, ...)
{
  if(!(verb_level & 1)) return;
  fprintf(stderr, "%s: warning: not supported: ", argv0);
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
}

void NOTIFY(const char *fmt, ...)
{
  if(!(verb_level & 4)) return;
  fprintf(stderr, "Notify: ");
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
}

void DEBUG(unsigned level, const char *fmt, ...)
{
  if((level>7?7:level)>((verb_level>>3)&0x07)) return;
  fprintf(stderr, "%*sDebug: ", level, "");
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
  fflush(stderr);
}

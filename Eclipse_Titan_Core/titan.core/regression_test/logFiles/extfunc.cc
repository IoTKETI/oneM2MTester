/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include <stdio.h>
#include "LogFiles.hh"
#include "CommonStuff.hh"

#ifndef OLD_NAMES
namespace LogFiles {
#endif

#define MAXLINELENGTH (8 * 1024)

StrList readlogfile(const CHARSTRING& filename)
{
  StrList str_list(NULL_VALUE);
  FILE *fp;
  fp = fopen(filename, "r");
  if (!fp) return str_list;
  char line[MAXLINELENGTH];
  while (fgets(line, MAXLINELENGTH, fp))
  {
    size_t str_len = strlen(line);
    if (line[str_len - 1] == '\n') line[str_len - 1] = '\0';
    str_list[str_list.size_of()] = CHARSTRING(line);
  }
  fclose(fp);
  return str_list;
}

#ifndef OLD_NAMES
}
#endif

#ifndef OLD_NAMES
namespace CommonStuff {
#endif

EPTF__CharstringList v__EPTF__Common__errorMsgs(NULL_VALUE);

void f__EPTF__Common__initErrorMsgs(void) {
  v__EPTF__Common__errorMsgs = NULL_VALUE;
}

void f__EPTF__Common__addErrorMsg(const CHARSTRING& pl__newMsg)
{
  v__EPTF__Common__errorMsgs[v__EPTF__Common__errorMsgs.size_of()] = pl__newMsg;
}

INTEGER f__EPTF__Common__nofErrorMsgs()
{
  return v__EPTF__Common__errorMsgs.size_of();
}

CHARSTRING f__EPTF__Common__getErrorMsg(const INTEGER& pl__errorNum)
{
  if (v__EPTF__Common__errorMsgs.size_of() == 0) {
    return CHARSTRING("");
  }
  boolean tmp_16;
  tmp_16 = (v__EPTF__Common__errorMsgs.size_of() <= pl__errorNum);
  if (!tmp_16) tmp_16 = (pl__errorNum < 0);
  if (tmp_16) {
    return CHARSTRING("");
  }
  return v__EPTF__Common__errorMsgs[pl__errorNum];
}

void f__EPTF__Common__error(const CHARSTRING& pl__message)
{
  f__EPTF__Common__addErrorMsg(pl__message);
  TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED, pl__message);
}

void f__EPTF__Common__warning(const CHARSTRING& pl__message)
{
  TTCN_Logger::log_str(TTCN_Logger::WARNING_UNQUALIFIED, pl__message);
}

void f__EPTF__Common__user(const CHARSTRING& pl__message)
{
  TTCN_Logger::log_str(TTCN_Logger::USER_UNQUALIFIED, pl__message);
}

INTEGER f__EPTF__Base__upcast(const EPTF__Base__CT& pl__compRef) {
  return INTEGER((component)pl__compRef);
}

EPTF__Base__CT f__EPTF__Base__downcast(const INTEGER& pl__baseCompRef) {
  return EPTF__Base__CT(pl__baseCompRef);
}

void f__EPTF__Base__assert(const CHARSTRING& pl__assertMessage, const BOOLEAN& pl__predicate)
{
#ifdef EPTF_DEBUG
  if (!(pl__predicate)) {
    f__EPTF__Base__addAssertMsg(pl__assertMessage);
    TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED, CHARSTRING("f_EPTF_Base_assert: Assertion failed! ") + pl__assertMessage);
    if (EPTF__CommonStuff::EPTF__Base__CT__private_component_v__EPTF__Base__negativeTestMode == true) {
      f__EPTF__Base__stop(NONE);
    } else {
      f__EPTF__Base__stop(FAIL);
    }
  }
#endif
}

FLOAT f__EPTF__Base__getTimeOfDay()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

#ifndef OLD_NAMES
}
#endif

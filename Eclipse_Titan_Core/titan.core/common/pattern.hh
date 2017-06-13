/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _common_pattern_HH
#define _common_pattern_HH

#ifndef __GNUC__
/** If a C compiler other than GCC is used the macro below will substitute all
 * GCC-specific non-standard attributes with an empty string. */
#ifndef __attribute__
#define __attribute__(arg)
#endif
#endif

/** This is the interface for pattern. This function converts a TTCN-3
 *  charstring pattern to a POSIX Extended Regular Expression (ERE).
 *  If error occurs, returns a NULL-pointer. It uses the
 *  TTCN_pattern_error() and TTCN_pattern_warning() functions to
 *  report errors/warnings. 
 *
 *  The function is also used on universal charstring patterns (in UTF-8 format)
 *  during JSON schema generation. In this case the 2nd parameter must be set
 *  to true, so no errors are reported for the extended ASCII characters. */
extern char* TTCN_pattern_to_regexp(const char* p_pattern, bool utf8 = false);

extern char* TTCN_pattern_to_regexp_uni(const char* p_pattern, bool p_nocase,
  int** groups = 0);

/* defined elsewhere (can be different in compiler/runtime) */

extern void TTCN_pattern_error(const char *fmt_str, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
extern void TTCN_pattern_warning(const char *fmt_str, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

#endif /* _common_pattern_HH */

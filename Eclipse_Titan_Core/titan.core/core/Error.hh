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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef ERROR_HH
#define ERROR_HH

class TC_End { };

class TC_Error { };

class TTCN_Error : public TC_Error {
  char* error_msg;
public:
  TTCN_Error(char* p_error_msg): error_msg(p_error_msg) {}
  char* get_message() const { return error_msg; }
  ~TTCN_Error();
};

extern void TTCN_error(const char *err_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2), __noreturn__));
extern void TTCN_error_begin(const char *err_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
extern void TTCN_error_end()
  __attribute__ ((__noreturn__));

void TTCN_warning(const char *warning_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
extern void TTCN_warning_begin(const char *warning_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
extern void TTCN_warning_end();

extern void TTCN_pattern_error(const char *error_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2), __noreturn__));
extern void TTCN_pattern_warning(const char *warning_msg, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

#endif

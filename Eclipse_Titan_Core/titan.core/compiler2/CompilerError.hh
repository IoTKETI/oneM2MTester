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
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef _Common_Error_HH
#define _Common_Error_HH

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "error.h"

namespace Common {

  class Location;

  /**
   * Class Error_Context. :)
   */
  class Error_Context {
  private:
    /** Counts errors, warnings. */
    static unsigned int error_count, warning_count;
    /** Indicates whether the actual chain was printed. */
    static bool chain_printed;
    /** The head and tail of the actual chain. */
    static Error_Context *head, *tail;

    /** List-linking pointers. This contains the saved chain in a
     *  restorer element. */
    Error_Context *prev, *next;
    /** Pointer to the location info structure. */
    const Location *location;
    /** The message of the element. */
    char *str;
    /** Indicates whether this element was already printed. */
    bool is_printed;
    /** Indicates whether this element contains an outer chain. */
    bool is_restorer;
    /** Indicates whether the outer chain was printed. */
    bool outer_printed;
    /** Maximum number of errors to report */
    static unsigned int max_errors;

    /** Copy constructor not implemented. */
    Error_Context(const Error_Context& p);
    /** Assignment not implemented */
    Error_Context& operator=(const Error_Context& p);
  private:
    /** Prints the actual chain of error contexts to fp. */
    static void print_context(FILE *fp);
  public:
    /** Starts a new chain and preserves/restores the existing chain,
     *  but keeps the n_keep elements of the old chain. */
    Error_Context(size_t n_keep = 0);
    /** Adds a new element to the chain. The message shall be set
     *  later with a call to set_message (e.g. in a conditional
     *  construct). */
    Error_Context(const Location *p_location);
    /** Adds a new element to the chain with the given message. */
    Error_Context(const Location *p_location, const char *p_fmt, ...)
      __attribute__ ((__format__ (__printf__, 3, 4)));
    /** Destructor. Removes element from the chain. */
    ~Error_Context();

    /** Assigns the given message to the element */
    void set_message(const char *p_fmt, ...)
      __attribute__ ((__format__ (__printf__, 2, 3)));

    /** Generic error reporting function. */
    static void report_error(const Location *loc, const char *fmt,
      va_list args);

    /** Generic warning reporting function. */
    static void report_warning(const Location *loc, const char *fmt,
      va_list args);

    static void report_note(const Location *loc, const char *fmt,
      va_list args);

    /** Returns the number of errors encountered. */
    static unsigned int get_error_count() { return error_count; }
    /** Returns the number of warnings encountered. */
    static unsigned int get_warning_count() { return warning_count; }
    /** Increments the error counter. */
    static void increment_error_count();
    /** Increments the warning counter. */
    static void increment_warning_count();

    /** Prints statistics about the number of errors and warnings */
    static void print_error_statistics();

    /** Set the maximum number of errors */
    static void set_max_errors(unsigned int maxe) {
      max_errors = maxe;
    }
  };

} // namespace Common

#endif // _Common_Error_HH

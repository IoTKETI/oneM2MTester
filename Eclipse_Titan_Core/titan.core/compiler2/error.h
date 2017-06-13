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
 *   Gecse, Roland
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _ERROR_H
#define _ERROR_H

#ifndef __GNUC__
/** If a C compiler other than GCC is used the macro below will substitute all
 * GCC-specific non-standard attributes with an empty string. */
#ifndef __attribute__
#define __attribute__(arg)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup error Error reporting functions
 * \author Roland Gecse <ethrge@eth.ericsson.se>
 *
 * @{
 */

/**
 *
 * Verbosity level (bitmask).
 *
 * Meaning of bits:
 *  1: "not supported" messages
 *  2: warning
 *  4: notify
 *  8: debug 0
 *  16: debug 1
 *  32: debug 2
 *
 * Debug bits define the debug level in the interval 0..7; 0 means no
 * debug messages, 7 means very verbose.
 */
extern unsigned verb_level;

/**
 * The argv[0] of main, i.e. the program name.
 */
extern const char *argv0;

/**
 * FATAL_ERROR(const char *fmt, ...) macro prints a formatted error message
 * to stderr and aborts execution. It calls the fatal_error function with
 * appropriate file name and line number information
 */
#if defined(__GNUC__) && __GNUC__ < 3
/**
 * The preprocessors of GCC versions earlier than 3.0 do not support the
 * standard notation for variadic macros in C++ mode.
 * Therefore this proprietary notation is used with those old GCC versions.
 */
#define FATAL_ERROR(fmt, args...) \
	(fatal_error(__FILE__, __LINE__, fmt, ## args))
#else
/**
 * This is the standard notation.
 */
#define FATAL_ERROR(...) \
	(fatal_error(__FILE__, __LINE__, __VA_ARGS__))
#endif

/**
 * fatal_error function, which is not supposed to be called directly.
 */
extern void fatal_error(const char *filename, int lineno, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 3, 4), __noreturn__));

/**
 * Prints a formatted error message to stderr.
 * Used for reporting system-related errors, e.g. file not found etc.
 * Compiler errors are reported through Common::Location::error.
 */
extern void ERROR(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * Prints a formatted warning message to stderr.
 */
extern void WARNING(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * Prints a formatted warning message to stderr: "Warning: Not supported: %s".
 */
extern void NOTSUPP(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * Prints a formatted notify message to stderr.
 */
extern void NOTIFY(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

/**
 * Prints a formatted debug message to stderr.
 */
extern void DEBUG(unsigned level, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

/**
 * Returns the current value of the error counter.
 */
extern unsigned int get_error_count(void);

/**
 * Returns the current value of the warning counter.
 */
extern unsigned int get_warning_count(void);

/** @} end of error group */

#ifdef __cplusplus
}
#endif

#endif /* _ERROR_H */

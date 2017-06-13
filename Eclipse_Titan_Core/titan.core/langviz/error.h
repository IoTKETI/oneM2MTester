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
#ifndef _langviz_error_H
#define _langviz_error_H

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
 * The argv[0] of main, i.e. the programname.
 */
extern const char *argv0;

/**
 * FATAL_ERROR(const char *fmt, ...) macro prints a formatted error message
 * to stderr and aborts execution. It calls the fatal_error function with
 * appropriate file name and line number information
 */
#if defined (__GNUC__)
/**
 * The preprocessors of GCC versions 2.95.3 or earlier do not support the
 * standard notation for varadic macros in C++ mode.
 * Therefore this proprietary notation is used with GCC.
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


#ifdef __cplusplus
}
#endif

#endif // _langviz_error_H

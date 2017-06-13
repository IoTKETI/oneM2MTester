/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_memory_H
#define _Common_memory_H

#include <stddef.h>
#include <stdarg.h>

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
   * \defgroup mem Memory management and expstring functions
   * \brief Functions for memory management and string handling.
   * \author Janos Zoltan Szabo <Janos.Zoltan.Szabo@ericsson.com>
   *
   * Primarily used in the TTCN-3 Test Executor (TTCN-3, ASN.1 compilers and
   * runtime environment).
   * @{
   */

  /**
   * Same as the standard \c malloc(), but it never returns NULL.
   * It increases a malloc counter. If there is not enough memory,
   * it calls \c fatal_error(), which exits the application.
   *
   * @param size number of bytes to allocate
   * @return pointer to the beginning of the allocated memory, or
   *         NULL if and only if \c size is 0
   */
  extern void *Malloc(size_t size);
#ifdef MEMORY_DEBUG
  extern void *Malloc_dbg(const char *filename, int line, size_t size);
#define        Malloc(s) Malloc_dbg(__FILE__, __LINE__, s)
#endif

  /**
   * Same as the standard \c realloc(), but it never returns NULL if \a size is
   * positive. It updates the malloc or free counters if necessary.
   * Exits if there is not enough memory.
   *
   * @param ptr pointer to a memory block allocated by Malloc(). If \p ptr
   * is NULL, calls Malloc(size)
   * @param size new size for the memory block. If \p size is 0, it calls
   * Free(ptr) and returns NULL.
   * @return pointer to the beginning of the re-allocated memory.
   * Will only be NULL if size==0.
   */
  extern void *Realloc(void *ptr, size_t size);
#ifdef MEMORY_DEBUG
  extern void *Realloc_dbg(const char *filename, int line, void *ptr, size_t size);
#define        Realloc(p,s) Realloc_dbg(__FILE__, __LINE__, p, s)
#endif

  /**
   * Same as the standard \c free(). It increases the free counter if \a ptr
   * is not NULL.
   *
   * @param ptr pointer to a memory block allocated by Malloc(). If \p ptr
   * is NULL, this function does nothing.
   */
  extern void Free(void *ptr);
#ifdef MEMORY_DEBUG
  extern void Free_dbg(const char *filename, int line, void *ptr);
#define Free(p) Free_dbg(__FILE__, __LINE__, p)
#endif

  /**
   * Prints a warning message to stderr if the malloc and free counters
   * are not equals. It shall be called immediately before the end of
   * program run.
   */
  extern void check_mem_leak(const char *program_name);

#ifdef MEMORY_DEBUG
  /**
   * Checks all allocated blocks of the program. Prints an error message and
   * aborts if memory over-indexing is detected.
   */
  extern void check_mem_corrupt(const char *program_name);

#ifdef MEMORY_DEBUG_ADDRESS
  /** @brief Memory allocation checkpoint.

  If this variable is set to a nonzero value, allocations starting
  at that address will be logged to stderr.

  This can be used in conjunction with the output of check_mem_leak().
  */
  extern void * memory_debug_address;
#endif
#endif

  /**
   * Character string type with exponential buffer allocation. The size of
   * allocated buffer is always a power of 2. The terminating '\\0' character
   * is always present and the remaining bytes in the buffer are also set to
   * zero. This allows binary search for the end of string, which is
   * significantly faster than the linear one especially for long strings.
   * The spare bytes at the end make appending of small chunks very efficient.
   *
   * \warning If a function takes expstring_t as argument
   * you should not pass a regular string (or a static string literal)
   * to it. This may result in an unpredictable behaviour. You can
   * convert any regular string to expstring_t using mcopystr:
   * myexpstring = mcopystr(myregularstring);
   */
  typedef char *expstring_t;

  /**
   * mprintf() takes its arguments like \c printf() and prints according
   * to the format string \a fmt into a string buffer. It allocates
   * enough memory for the resulting string and returns the pointer
   * to the result string. The result string is an exponential string.
   * mprintf() never returns NULL.
   */
  extern expstring_t mprintf(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));
#ifdef MEMORY_DEBUG
  extern expstring_t mprintf_dbg(const char *filename, int line, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));
#if defined(__GNUC__) && __GNUC__ < 3
#  define mprintf(f, args...) mprintf_dbg(__FILE__, __LINE__, f, ## args)
#else
#  define mprintf(...) mprintf_dbg(__FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

  /**
   * The same as \a mprintf(), but it takes the arguments as va_list.
   * It is useful in wrapper functions with printf style argument strings.
   */
  extern expstring_t mprintf_va_list(const char *fmt, va_list pvar);
#ifdef MEMORY_DEBUG
  extern expstring_t mprintf_va_list_dbg(const char *filename, int line,
    const char *fmt, va_list pvar);
#define mprintf_va_list(f,v) mprintf_va_list_dbg(__FILE__, __LINE__, f, v)
#endif

  /**
   * mputprintf() prints its additional arguments according to the
   * format string \a fmt at the end of \a str. The buffer of \a str is
   * increased if the appended bytes do not fit in it. The result
   * string, which is also an expstring, is returned.
   * mputprintf() never returns NULL.
   * \note If str is NULL it is equivalent to \a mprintf().
   * \warning The first argument must be an exponential string,
   * otherwise its behaviour may be unpredictable.
   */
  extern expstring_t mputprintf(expstring_t str, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));

  /**
   * The same as \a mputprintf(), but it takes the arguments as va_list.
   * It is useful in wrapper functions with printf style argument strings.
   */
  extern expstring_t mputprintf_va_list(expstring_t str, const char *fmt,
    va_list pvar);

  /**
   * memptystr() creates and returns a new empty exponential string.
   * The returned value is never NULL,
   * it shall be deallocated using \a Free().
   */
  extern expstring_t memptystr(void);
#ifdef MEMORY_DEBUG
  extern expstring_t memptystr_dbg(const char *filename, int line);
#define              memptystr() memptystr_dbg(__FILE__, __LINE__)
#endif


  /**
   * mcopystr() creates a new exponential string and copies the contents of
   * \a str into it. The resulting expstring is returned.
   * The regular string \a str will not be deallocated and it may be
   * a static string literal.
   * If \a str is NULL an empty exponential string is returned.
   * mcopystr() never returns NULL.
   */
  extern expstring_t mcopystr(const char *str);
#ifdef MEMORY_DEBUG
  extern expstring_t mcopystr_dbg(const char *filename, int line, const char *str);
#define              mcopystr(s) mcopystr_dbg(__FILE__, __LINE__, s)
#endif

  /**
   * Create a new exponential string when the length is known.
   * Works exactly like mcopystr(), except the length is not measured;
   * the given length is used instead.
   *
   * @param str pointer to the original string; does not need to be 0-terminated
   * @param len number of characters to copy
   * @return the newly constructed string (it needs to be Free()-d)
   */
  extern expstring_t mcopystrn(const char *str, size_t len);
#ifdef MEMORY_DEBUG
  extern expstring_t mcopystrn_dbg(const char *filename, int line, const char *str,
    size_t len);
#define              mcopystrn(s, len) mcopystrn_dbg(__FILE__, __LINE__, s, len)
#endif

  /**
   * mputstr() appends the regular string \a str2 to the end of
   * expstring \a str. The resulting expstring is returned.
   * The buffer of \a str is increased if necessary.
   * If \a str is NULL then \a str2 is copied into a new exponential string
   * (i.e. mputstr(NULL, str) is identical to mcopystr(str)).
   * If \a str2 is NULL then \a str is returned and remains unchanged
   * (i.e. mputstr(str, NULL) is identical to str,
   * mputstr(NULL, NULL) always returns NULL).
   * \warning The first argument must be an exponential string,
   * otherwise its behaviour may be unpredictable.
   */
  extern expstring_t mputstr(expstring_t str, const char *str2);

  /** Appends \a len2 characters from the regular string \a str2 to the end of
   * expstring \a str. The resulting expstring is returned. @see mputstr()
   * @param str destination string
   * @param str2 pointer to characters; does not need to be 0-terminated
   * @param len2 number of characters to copy
   * @return the (possibly reallocated) str
   */
  extern expstring_t mputstrn(expstring_t str, const char *str2, size_t len2);

  /**
   * mputc() appends the single character \a c to the end of
   * expstring \a str. The buffer of \a str is increased if necessary.
   * The resulting expstring is returned.
   * If \a str is NULL then \a c is converted to a new exponential string.
   * If \a c is '\\0' then \a str is returned.
   * mputc() never returns NULL.
   * \warning The first argument must be an exponential string,
   * otherwise its behaviour may be unpredictable.
   */
  extern expstring_t mputc(expstring_t str, char c);

  /**
   * mtruncstr() truncates the expstring \a str by keeping only the first
   * \a newlen characters and returns the resulting string.
   * If the string is shorter than \a newlen it remains unchanged.
   * mtruncstr() may perform memory reallocation if necessary.
   * If \a str is NULL then a NULL pointer is returned.
   * If \a str is not an exponential string the behaviour of mtruncstr() may
   * be unpredictable.
   */
  extern expstring_t mtruncstr(expstring_t str, size_t newlen);

  /**
   * mstrlen() returns the length of expstring \a str or zero if \a str is
   * NULL. If \a str is not NULL the function has identical result as libc's
   * strlen(), but operates significantly faster. The behaviour may be
   * unpredictable if \a str is not an exponential string.
   */
  extern size_t mstrlen(const expstring_t str);

  /** @} end of mem group */

  /** Return the string for the build number.
   *
   * @param b build number.
   * @return a string which must be Free()-d by the caller
   * @pre b > 0 and b <= 99, or else NULL is returned
   */
  char * buildstr(unsigned int b);

#ifdef __cplusplus
  /** Convert a patch level to the "Ericsson letter" */
  inline char eri(unsigned int p) { /* p stands for patch level */
    char   i = static_cast<char>('A' + p); /* i stands for "if only it was that simple" */
    const int result = i + (i >= 'I') + 4 * (i >= 'N') + (i >= 'R');
    return static_cast<char>(result); /*check: does not overflow*/
  }

} /* extern "C" */

#endif

#endif /* _Common_memory_H */

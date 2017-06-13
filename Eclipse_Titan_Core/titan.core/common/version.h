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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef VERSION_H
#define VERSION_H

/* Version numbers */
#define TTCN3_MAJOR 6
#define TTCN3_MINOR 1
#define TTCN3_PATCHLEVEL 0   //0-> x=A, 1-> x=B, ... 
//#define TTCN3_BUILDNUMBER 0  //0=R5x, 1=R5x01, 2=R5x02, ...

/* The aggregated version number must be set manually since some stupid
 * 'makedepend' programs cannot calculate arithmetic expressions.
 * In official releases:
 * To display the correct version comment out TTCN3_BUILDNUMBER
 * TTCN3_VERSION = TTCN3_MAJOR * 10000 + TTCN3_MINOR * 100 + TTCN3_PATCHLEVEL
 * In pre-release builds:
 * To display the correct version uncomment TTCN3_BUILDNUMBER
 * TTCN3_VERSION = TTCN3_MAJOR * 1000000 + TTCN3_MINOR * 10000 +
 *                 TTCN3_PATCHLEVEL * 100 + TTCN3_BUILDNUMBER
 */
#define TTCN3_VERSION 60100

/* A monotonically increasing version number.
 * An official release is deemed to have the highest possible build number (99)
 * The revisions R1A01, R1A02 ... R1A, R1B01, R1B02 ... R1B
 */
#ifdef TTCN3_BUILDNUMBER
#define TTCN3_VERSION_MONOTONE TTCN3_VERSION
#else
#define TTCN3_VERSION_MONOTONE (TTCN3_VERSION * 100 + 99)
#endif


#if defined (SOLARIS)
  #define PLATFORM_STRING "SOLARIS"
#elif defined (SOLARIS8)
  #define PLATFORM_STRING "SOLARIS8"
#elif defined (LINUX)
  #define PLATFORM_STRING "LINUX"
#elif defined (WIN32)
  /* MINGW is defined only if CYGWIN is defined */
  #if defined (MINGW)
    #define PLATFORM_STRING "MINGW"
  #else
    #define PLATFORM_STRING "WIN32"
  #endif
#elif defined (FREEBSD)
  #define PLATFORM_STRING "FREEBSD"
#elif defined (INTERIX)
  #define PLATFORM_STRING "INTERIX"
/* TODO more */
#endif


#ifndef PLATFORM_STRING
/* Just to suppress error later */
  #define PLATFORM_STRING "UNKNOWN"
  #error "No supported platform has been defined in the Makefile. The supported ones: SOLARIS, SOLARIS8, LINUX, WIN32"
#endif

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

#if defined(__GNUC__)

  #ifdef __GNUC_PATCHLEVEL__
    #define GCC_PATCHLEVEL_STRING "." STR(__GNUC_PATCHLEVEL__)
  #else
    /* __GNUC_PATCHLEVEL__ appeared in gcc 3.0 */
    #define GCC_PATCHLEVEL_STRING
  #endif

#ifdef __clang__
  #define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100)
  #define COMPILER_VERSION_STRING " Clang: (GNU) " STR(__clang_major__) "." STR(__clang_minor__) "." STR(__clang_patchlevel__)
#else
  #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
  #define COMPILER_VERSION_STRING " GCC: (GNU) " STR(__GNUC__) "." STR(__GNUC_MINOR__) GCC_PATCHLEVEL_STRING
#endif


  static const char titan_[] __attribute__ ((section (".titan"))) = \
  "TITAN: " STR(TTCN3_VERSION) " PLATFORM: " PLATFORM_STRING COMPILER_VERSION_STRING;

#ifdef __cplusplus
  struct reffer {
    reffer(const char *);
  };

  /* Prevents the version string from being optimized away */
  static const reffer ref_ver(titan_);
#endif

#else /* not a GNU compiler */

#ifndef __attribute__
#define __attribute__(arg)
#endif

#if defined(__SUNPRO_C)

#define COMPILER_VERSION_STRING " SunPro: " STR(__SUNPRO_C)

#elif defined(__SUNPRO_CC)

#define COMPILER_VERSION_STRING " SunPro: " STR(__SUNPRO_CC)

#pragma ident "TITAN: " STR(TTCN3_VERSION) " PLATFORM: " PLATFORM_STRING COMPILER_VERSION_STRING;

#else
/* Luft! unknown compiler */
#endif /* not SunPro */

#endif
/* __SUNPRO_CC is an all-in-one value */


#define FN(f,x) requires_ ## f ## _ ## x
#define FIELD_NAME(f,x) FN(f,x)

#ifdef  TITAN_RUNTIME_2
#define TITAN_RUNTIME_NR 2
#else
#define TITAN_RUNTIME_NR 1
#endif

struct runtime_version {
  int FIELD_NAME(major_version, TTCN3_MAJOR);
  int FIELD_NAME(minor_version, TTCN3_MINOR);
  int FIELD_NAME(patch_level, TTCN3_PATCHLEVEL);
  int FIELD_NAME(runtime, TITAN_RUNTIME_NR);
};

extern const struct runtime_version current_runtime_version;

#ifdef __cplusplus
class RuntimeVersionChecker {
public:
  RuntimeVersionChecker(int ver_major, int ver_minor, int patch_level, int rt);
};
#endif

#undef FIELD_NAME
#undef FN

#endif

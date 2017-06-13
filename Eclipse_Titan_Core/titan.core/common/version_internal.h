/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef VERSION_INTERNAL_H
#define VERSION_INTERNAL_H

#include "version.h"

/* This file contains the macro settings that are not delivered to the user
 * in the binary package. */

/* Symbolic version string */
#ifdef TTCN3_BUILDNUMBER
/* pre-release, e.g. "1.6.pre4 build 1" */
# define GEN_VER2(major, minor, patchlevel, buildnumber) \
    #major "." #minor ".pre" #patchlevel " build " #buildnumber
# define GEN_VER(major, minor, patchlevel, buildnumber) \
    GEN_VER2(major, minor, patchlevel, buildnumber)
# define VERSION_STRING GEN_VER(TTCN3_MAJOR, TTCN3_MINOR, TTCN3_PATCHLEVEL, \
    TTCN3_BUILDNUMBER)
#else
/* stable release, e.g. "1.4.pl3" */
# define GEN_VER2(major, minor, patchlevel) #major "." #minor ".pl" #patchlevel
# define GEN_VER(major, minor, patchlevel) GEN_VER2(major, minor, patchlevel)
# define VERSION_STRING GEN_VER(TTCN3_MAJOR, TTCN3_MINOR, TTCN3_PATCHLEVEL)
#endif

/* Product number */
#define PRODNR_EXECUTOR  "CRL 113 200"

/* Ericsson revision: /m Rnx
 * m = TTCN3_MAJOR
 * n = TTCN3_MINOR
 * x = 'A' + TTCN3_PATCHLEVEL
 * Example: 1.4.pl3 = R4D */

#define GEN_ERICSSON_SUFFIX2(num) #num
#define GEN_ERICSSON_SUFFIX(num) GEN_ERICSSON_SUFFIX2(num)
#define ERICSSON_SUFFIX GEN_ERICSSON_SUFFIX(TTCN3_MAJOR)

#define GEN_ERICSSON_NUMBER2(num) #num
#define GEN_ERICSSON_NUMBER(num) GEN_ERICSSON_NUMBER2(num)
#define ERICSSON_NUMBER GEN_ERICSSON_NUMBER(TTCN3_MINOR)

#if TTCN3_PATCHLEVEL == 0
#define ERICSSON_LETTER "A"
#elif TTCN3_PATCHLEVEL == 1
#define ERICSSON_LETTER "B"
#elif TTCN3_PATCHLEVEL == 2
#define ERICSSON_LETTER "C"
#elif TTCN3_PATCHLEVEL == 3
#define ERICSSON_LETTER "D"
#elif TTCN3_PATCHLEVEL == 4
#define ERICSSON_LETTER "E"
#elif TTCN3_PATCHLEVEL == 5
#define ERICSSON_LETTER "F"
#elif TTCN3_PATCHLEVEL == 6
#define ERICSSON_LETTER "G"
#elif TTCN3_PATCHLEVEL == 7
#define ERICSSON_LETTER "H"
#elif TTCN3_PATCHLEVEL == 8
/* I: forbidden */
#define ERICSSON_LETTER "J"
#elif TTCN3_PATCHLEVEL == 9
#define ERICSSON_LETTER "K"
#elif TTCN3_PATCHLEVEL == 10
#define ERICSSON_LETTER "L"
#elif TTCN3_PATCHLEVEL == 11
#define ERICSSON_LETTER "M"
#elif TTCN3_PATCHLEVEL == 12
#define ERICSSON_LETTER "N"
/* O, P, Q, R: forbidden */
#elif TTCN3_PATCHLEVEL == 13
#define ERICSSON_LETTER "S"
#elif TTCN3_PATCHLEVEL == 14
#define ERICSSON_LETTER "T"
#elif TTCN3_PATCHLEVEL == 15
#define ERICSSON_LETTER "U"
#elif TTCN3_PATCHLEVEL == 16
#define ERICSSON_LETTER "V"
#elif TTCN3_PATCHLEVEL == 17
/* W: forbidden */
#define ERICSSON_LETTER "X"
#elif TTCN3_PATCHLEVEL == 18
#define ERICSSON_LETTER "Y"
#elif TTCN3_PATCHLEVEL == 19
#define ERICSSON_LETTER "Z"
#else
#error "Ericsson revision letter is not defined."
#endif

#ifdef TTCN3_BUILDNUMBER
/* The Ericsson version is suffixed with the build number (at least 2 digits)
 * in preliminary releases (like this: "R6E01") */
# if TTCN3_BUILDNUMBER < 10
#  define ERICSSON_BUILDNUMBER "0" GEN_ERICSSON_NUMBER(TTCN3_BUILDNUMBER)
# else
#  define ERICSSON_BUILDNUMBER GEN_ERICSSON_NUMBER(TTCN3_BUILDNUMBER)
# endif
#else
# define ERICSSON_BUILDNUMBER
#endif

#define ERICSSON_VERSION "/" ERICSSON_SUFFIX " R" ERICSSON_NUMBER ERICSSON_LETTER \
  ERICSSON_BUILDNUMBER

#define PRODUCT_NUMBER  PRODNR_EXECUTOR ERICSSON_VERSION

/* Version of the C/C++ compiler */

#if defined(__GNUC__)
# ifdef __clang__
#  ifdef __clang_patchlevel__
#    define GEN_COMP_VER2(major, minor, patchlevel) "clang " #major "." #minor "." #patchlevel
#    define GEN_COMP_VER(major, minor, patchlevel) GEN_COMP_VER2(major, minor, patchlevel)
#    define C_COMPILER_VERSION GEN_COMP_VER(__clang_major__, __clang_minor__, __clang_patchlevel__)
#  else
#    define GEN_COMP_VER2(major, minor) "clang " #major "." #minor ".?"
#    define GEN_COMP_VER(major, minor) GEN_COMP_VER2(major, minor)
#    define C_COMPILER_VERSION GEN_COMP_VER(__clang_major__, __clang_minor__)
#  endif
# else
    /* the code is compiled with GCC */
#  ifdef __GNUC_PATCHLEVEL__
     /* the patch number is known (version 3.0 or later) */
#    define GEN_COMP_VER2(major, minor, patchlevel) "GCC " #major "." #minor "." #patchlevel
#    define GEN_COMP_VER(major, minor, patchlevel) GEN_COMP_VER2(major, minor, patchlevel)
#    define C_COMPILER_VERSION GEN_COMP_VER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#  else
     /* the patch number is unknown (version 2.x.?) */
#    define GEN_COMP_VER2(major, minor) "GCC " #major "." #minor ".?"
#    define GEN_COMP_VER(major, minor) GEN_COMP_VER2(major, minor)
#    define C_COMPILER_VERSION GEN_COMP_VER(__GNUC__, __GNUC_MINOR__)
#  endif
# endif
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
  /* the code is compiled with Sun Workshop C/C++ compiler */
# ifdef __SUNPRO_C
#  define __SUNPRO_VERSION __SUNPRO_C
# else
#  define __SUNPRO_VERSION __SUNPRO_CC
# endif
# if __SUNPRO_VERSION <= 0xfff
   /* the version number is in format 0xVRP */
   /* version number (V) */
#  if (__SUNPRO_VERSION & 0xf00) == 0x000
#   define SUNPRO_V "0"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x100
#   define SUNPRO_V "1"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x200
#   define SUNPRO_V "2"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x300
#   define SUNPRO_V "3"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x400
#   define SUNPRO_V "4"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x500
#   define SUNPRO_V "5"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x600
#   define SUNPRO_V "6"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x700
#   define SUNPRO_V "7"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x800
#   define SUNPRO_V "8"
#  elif (__SUNPRO_VERSION & 0xf00) == 0x900
#   define SUNPRO_V "9"
#  elif (__SUNPRO_VERSION & 0xf00) == 0xa00
#   define SUNPRO_V "10"
#  elif (__SUNPRO_VERSION & 0xf00) == 0xb00
#   define SUNPRO_V "11"
#  elif (__SUNPRO_VERSION & 0xf00) == 0xc00
#   define SUNPRO_V "12"
#  elif (__SUNPRO_VERSION & 0xf00) == 0xd00
#   define SUNPRO_V "13"
#  elif (__SUNPRO_VERSION & 0xf00) == 0xe00
#   define SUNPRO_V "14"
#  else
#   define SUNPRO_V "15"
#  endif
   /* release number (R) */
#  if (__SUNPRO_VERSION & 0xf0) == 0x00
#   define SUNPRO_R "0"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x10
#   define SUNPRO_R "1"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x20
#   define SUNPRO_R "2"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x30
#   define SUNPRO_R "3"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x40
#   define SUNPRO_R "4"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x50
#   define SUNPRO_R "5"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x60
#   define SUNPRO_R "6"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x70
#   define SUNPRO_R "7"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x80
#   define SUNPRO_R "8"
#  elif (__SUNPRO_VERSION & 0xf0) == 0x90
#   define SUNPRO_R "9"
#  elif (__SUNPRO_VERSION & 0xf0) == 0xa0
#   define SUNPRO_R "10"
#  elif (__SUNPRO_VERSION & 0xf0) == 0xb0
#   define SUNPRO_R "11"
#  elif (__SUNPRO_VERSION & 0xf0) == 0xc0
#   define SUNPRO_R "12"
#  elif (__SUNPRO_VERSION & 0xf0) == 0xd0
#   define SUNPRO_R "13"
#  elif (__SUNPRO_VERSION & 0xf0) == 0xe0
#   define SUNPRO_R "14"
#  else
#   define SUNPRO_R "15"
#  endif
   /* patch number (P) */
#  if (__SUNPRO_VERSION & 0xf) == 0x0
#   define SUNPRO_P "0"
#  elif (__SUNPRO_VERSION & 0xf) == 0x1
#   define SUNPRO_P "1"
#  elif (__SUNPRO_VERSION & 0xf) == 0x2
#   define SUNPRO_P "2"
#  elif (__SUNPRO_VERSION & 0xf) == 0x3
#   define SUNPRO_P "3"
#  elif (__SUNPRO_VERSION & 0xf) == 0x4
#   define SUNPRO_P "4"
#  elif (__SUNPRO_VERSION & 0xf) == 0x5
#   define SUNPRO_P "5"
#  elif (__SUNPRO_VERSION & 0xf) == 0x6
#   define SUNPRO_P "6"
#  elif (__SUNPRO_VERSION & 0xf) == 0x7
#   define SUNPRO_P "7"
#  elif (__SUNPRO_VERSION & 0xf) == 0x8
#   define SUNPRO_P "8"
#  elif (__SUNPRO_VERSION & 0xf) == 0x9
#   define SUNPRO_P "9"
#  elif (__SUNPRO_VERSION & 0xf) == 0xa
#   define SUNPRO_P "10"
#  elif (__SUNPRO_VERSION & 0xf) == 0xb
#   define SUNPRO_P "11"
#  elif (__SUNPRO_VERSION & 0xf) == 0xc
#   define SUNPRO_P "12"
#  elif (__SUNPRO_VERSION & 0xf) == 0xd
#   define SUNPRO_P "13"
#  elif (__SUNPRO_VERSION & 0xf) == 0xe
#   define SUNPRO_P "14"
#  else
#   define SUNPRO_P "15"
#  endif
   /* the macro is defined in such an ugly way because the C preprocessor
    * cannot stringify the result of an arithmetic expression */
#  define C_COMPILER_VERSION "Sun Workshop Pro " SUNPRO_V "." SUNPRO_R "." SUNPRO_P
# else
   /* the version number is in unknown format */
#  define C_COMPILER_VERSION "Sun Workshop Pro unknown version"
# endif
#else
  /* the C/C++ compiler is unknown */
# define C_COMPILER_VERSION "unknown C/C++ compiler"
#endif

/* Copyright message */
#define COPYRIGHT_STRING COMMENT_PREFIX "Copyright (c) 2000-2017 Ericsson Telecom AB"

/* For prefixing the above messages. Default value: empty string. */
#define COMMENT_PREFIX

#endif

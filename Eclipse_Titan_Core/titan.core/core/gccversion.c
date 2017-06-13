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
 *   Raduly, Csaba
 *
 ******************************************************************************/
/* Write a header that checks the expected version of the compiler.
 * gccversion.c is a bit of a misnomer (also handles the Sun compiler),
 * but it's shorter than compiler_version.c */

#include <stdio.h>


#if defined(__GNUC__)

/* clang defines __GNUC__ but also has its own version numbers */
#ifdef __clang__

static unsigned int compiler_major = __clang_major__;
static unsigned int compiler_minor = __clang_minor__;
static unsigned int compiler_patchlevel = __clang_patchlevel__;
#define COMPILER_NAME_STRING "clang"

#else

#define COMPILER_NAME_STRING "GCC"
static unsigned int compiler_major = __GNUC__;
static unsigned int compiler_minor = __GNUC_MINOR__;
# ifdef __GNUC_PATCHLEVEL__
static unsigned int compiler_patchlevel = __GNUC_PATCHLEVEL__;
# else
static unsigned int compiler_patchlevel = 0; /* GCC below 3.0 */
# endif

#endif /* __clang__ */

#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
/* Just in case it's compiled with the C++ compiler */
# if !defined(__SUNPRO_C)
#  define __SUNPRO_C __SUNPRO_CC
# endif
static unsigned int compiler_major      = (__SUNPRO_C & 0xF00) >> 8;
static unsigned int compiler_minor      = (__SUNPRO_C & 0x0F0) >> 4;
static unsigned int compiler_patchlevel = (__SUNPRO_C & 0x00F);

#else
/* unknown compiler */
static unsigned int compiler_major = 0;
static unsigned int compiler_minor = 0;
static unsigned int compiler_patchlevel = 0;
#endif

int main(void)
{
  puts(
    "/* Check if the compiler matches the one used to build the runtime */\n"
    "#ifndef MAKEDEPEND_RUN\n\n");
  /* Do not check compiler version when makedepend is being run.
   * Old Solaris makedepend cannot compute GCC_VERSION. */
#if defined(__GNUC__)
  printf("\n"
#ifdef __clang__
    "#if CLANG_VERSION != %d\n"
#else
    "#if GCC_VERSION != %d\n"
#endif
    "#error The version of " COMPILER_NAME_STRING " does not match the expected version (" COMPILER_NAME_STRING " %d.%d.%d)\n"
    "#endif\n", compiler_major * 10000 + compiler_minor * 100,
    /* Note that we don't use compiler_patchlevel when checking.
     * This assumes that code is portable between GCC a.b.x and a.b.y */
    compiler_major, compiler_minor, compiler_patchlevel);
#elif defined(__SUNPRO_C)
  printf("\n"
    "#if __SUNPRO_CC != 0x%X\n"
    "#error The version of the Sun compiler does not match the expected version.\n"
    "#endif\n",
    compiler_major * 0x100 + compiler_minor * 0x10 + compiler_patchlevel);
#endif

  puts("\n#endif\n");

  return 0;
}

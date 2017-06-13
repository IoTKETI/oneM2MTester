/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_Code_HH
#define _Common_Code_HH

#include "ttcn3/compiler.h"

namespace Common {

  /**
   * \addtogroup AST
   *
   * @{
   */

  /**
   * Abstract class for storing Code.
   */
  class Code {
  public:

    static void init_output(output_struct *output, boolean no_alloc = FALSE);
    static void merge_output(output_struct *dest, output_struct *src);
    static void free_output(output_struct *output);

    static void init_cdef(const_def *cdef);
    static void merge_cdef(output_struct *dest, const_def *cdef);
    static void free_cdef(const_def *cdef);

    static void init_expr(expression_struct *expr);
    static void clean_expr(expression_struct *expr);
    static void free_expr(expression_struct *expr);
    static char* merge_free_expr(char* str, expression_struct *expr);

    /** Appends the C/C++ equivalent of character \a c to \a str. If flag
     * \a in_string is true (i.e. the \a c is a part of a string literal) then
     * the " character is escaped, otherwise (i.e. the \a c is a character
     * constant) ' is escaped. The function uses the standard C escape sequences
     * if possible or the octal notation for non-printable characters. */
    static char *translate_character(char *str, char c, bool in_string);
    /** Appends the C/C++ equivalent of literal string \a src to \a str. */
    static char *translate_string(char *str, const char *src);
  };

  /** @} end of AST group */

} // namespace Common

#endif // _Common_Code_HH

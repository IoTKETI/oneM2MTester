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
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Code.hh"
#include "../common/memory.h"
#include "error.h"
#include "CodeGenHelper.hh"

#include <ctype.h>

namespace Common {

  // =================================
  // ===== Code
  // =================================

  void Code::init_output(output_struct *output, boolean no_alloc)
  {
    output->header.includes = NULL;
    output->header.class_decls = NULL;
    output->header.typedefs = NULL;
    output->header.class_defs = NULL;
    output->header.function_prototypes = NULL;
    output->header.global_vars = NULL;
    output->header.testport_includes = NULL;
    output->source.includes = NULL;
    output->source.static_function_prototypes = NULL;
    output->source.static_conversion_function_prototypes = NULL;
    output->source.string_literals = NULL;
    output->source.class_defs = NULL;
    output->source.global_vars = NULL;
    output->source.methods = NULL;
    output->source.function_bodies = NULL;
    output->source.static_function_bodies = NULL;
    output->source.static_conversion_function_bodies = NULL;
    output->functions.pre_init = NULL;
    output->functions.post_init = NULL;
    output->functions.set_param = NULL;
    output->functions.get_param = NULL;
    output->functions.log_param = NULL;
    output->functions.init_comp = NULL;
    output->functions.start = NULL;
    output->functions.control = NULL;
    output->intervals.pre_things_size = 0;
    output->intervals.methods_size = 0;
    output->intervals.function_bodies_size = 0;
    output->intervals.static_conversion_function_bodies_size = 0;
    output->intervals.static_function_bodies_size = 0;
    output->intervals.methods_max_size = 1;
    output->intervals.function_bodies_max_size = 1;
    output->intervals.static_conversion_function_bodies_max_size = 1;
    output->intervals.static_function_bodies_max_size = 1;
    if (!no_alloc && CodeGenHelper::GetInstance().get_split_mode() == CodeGenHelper::SPLIT_TO_SLICES) {
      output->intervals.methods = (size_t*)Malloc(output->intervals.methods_max_size * sizeof(size_t));
      output->intervals.function_bodies = (size_t*)Malloc(output->intervals.function_bodies_max_size * sizeof(size_t));
      output->intervals.static_conversion_function_bodies = (size_t*)Malloc(output->intervals.static_conversion_function_bodies_max_size * sizeof(size_t));
      output->intervals.static_function_bodies = (size_t*)Malloc(output->intervals.static_function_bodies_max_size * sizeof(size_t));
      
      output->intervals.methods[0] = 0;
      output->intervals.function_bodies[0] = 0;
      output->intervals.static_conversion_function_bodies[0] = 0;
      output->intervals.static_function_bodies[0] = 0;
    } else {
      output->intervals.methods = NULL;
      output->intervals.function_bodies = NULL;
      output->intervals.static_conversion_function_bodies = NULL;
      output->intervals.static_function_bodies = NULL;
    }
  }

  void Code::merge_output(output_struct *dest, output_struct *src)
  {
    dest->header.includes =
      mputstr(dest->header.includes, src->header.includes);
    dest->header.class_decls =
      mputstr(dest->header.class_decls, src->header.class_decls);
    dest->header.typedefs =
      mputstr(dest->header.typedefs, src->header.typedefs);
    dest->header.class_defs =
      mputstr(dest->header.class_defs, src->header.class_defs);
    dest->header.function_prototypes =
      mputstr(dest->header.function_prototypes,
              src->header.function_prototypes);
    dest->header.global_vars =
      mputstr(dest->header.global_vars, src->header.global_vars);
    dest->header.testport_includes =
      mputstr(dest->header.testport_includes, src->header.testport_includes);
    dest->source.includes =
      mputstr(dest->source.includes, src->source.includes);
    dest->source.static_function_prototypes =
      mputstr(dest->source.static_function_prototypes,
              src->source.static_function_prototypes);
    dest->source.static_conversion_function_prototypes =
      mputstr(dest->source.static_conversion_function_prototypes,
              src->source.static_conversion_function_prototypes);
    dest->source.string_literals =
      mputstr(dest->source.string_literals, src->source.string_literals);
    dest->source.class_defs =
      mputstr(dest->source.class_defs, src->source.class_defs);
    dest->source.global_vars =
      mputstr(dest->source.global_vars, src->source.global_vars);
    dest->source.methods =
      mputstr(dest->source.methods, src->source.methods);
    dest->source.function_bodies =
      mputstr(dest->source.function_bodies, src->source.function_bodies);
    dest->source.static_function_bodies =
      mputstr(dest->source.static_function_bodies,
              src->source.static_function_bodies);
    dest->source.static_conversion_function_bodies =
      mputstr(dest->source.static_conversion_function_bodies,
              src->source.static_conversion_function_bodies);
    dest->functions.pre_init =
      mputstr(dest->functions.pre_init, src->functions.pre_init);
    dest->functions.post_init =
      mputstr(dest->functions.post_init, src->functions.post_init);
    dest->functions.set_param =
      mputstr(dest->functions.set_param, src->functions.set_param);
    dest->functions.get_param =
      mputstr(dest->functions.get_param, src->functions.get_param);
    dest->functions.log_param =
      mputstr(dest->functions.log_param, src->functions.log_param);
    dest->functions.init_comp =
      mputstr(dest->functions.init_comp, src->functions.init_comp);
    dest->functions.start =
      mputstr(dest->functions.start, src->functions.start);
    dest->functions.control =
      mputstr(dest->functions.control, src->functions.control);
  }

  void Code::free_output(output_struct *output)
  {
    Free(output->header.includes);
    Free(output->header.class_decls);
    Free(output->header.typedefs);
    Free(output->header.class_defs);
    Free(output->header.function_prototypes);
    Free(output->header.global_vars);
    Free(output->header.testport_includes);
    Free(output->source.includes);
    Free(output->source.static_function_prototypes);
    Free(output->source.static_conversion_function_prototypes);
    Free(output->source.string_literals);
    Free(output->source.class_defs);
    Free(output->source.global_vars);
    Free(output->source.methods);
    Free(output->source.function_bodies);
    Free(output->source.static_function_bodies);
    Free(output->source.static_conversion_function_bodies);
    Free(output->functions.pre_init);
    Free(output->functions.post_init);
    Free(output->functions.set_param);
    Free(output->functions.get_param);
    Free(output->functions.log_param);
    Free(output->functions.init_comp);
    Free(output->functions.start);
    Free(output->functions.control);
    Free(output->intervals.methods);
    Free(output->intervals.function_bodies);
    Free(output->intervals.static_conversion_function_bodies);
    Free(output->intervals.static_function_bodies);
    init_output(output, TRUE);
  }

  void Code::init_cdef(const_def *cdef)
  {
    cdef->decl = NULL;
    cdef->def = NULL;
    //cdef->cdef = NULL;
    cdef->init = NULL;
  }

  void Code::merge_cdef(output_struct *dest, const_def *cdef)
  {
    dest->header.global_vars = mputstr(dest->header.global_vars, cdef->decl);
    dest->source.global_vars = mputstr(dest->source.global_vars, cdef->def);
    dest->functions.pre_init = mputstr(dest->functions.pre_init, cdef->init);
  }

  void Code::free_cdef(const_def *cdef)
  {
    Free(cdef->decl);
    Free(cdef->def);
    //Free(cdef->cdef);
    Free(cdef->init);
  }

  void Code::init_expr(expression_struct *expr)
  {
    expr->preamble = NULL;
    expr->expr = NULL;
    expr->postamble = NULL;
  }

  void Code::clean_expr(expression_struct *expr)
  {
    Free(expr->expr);
    expr->expr = NULL;
  }

  void Code::free_expr(expression_struct *expr)
  {
    Free(expr->preamble);
    Free(expr->expr);
    Free(expr->postamble);
  }

  char* Code::merge_free_expr(char* str, expression_struct *expr)
  {
    if (expr->preamble || expr->postamble) {
      // open a statement block if the expression has a preamble or postamble
      str = mputstr(str, "{\n");
      // append the preamble if present
      if (expr->preamble) str = mputstr(str, expr->preamble);
    }
    // append the expression itself
    str = mputstr(str, expr->expr);
    // terminate it with a semi-colon
    str = mputstr(str, ";\n");
    if (expr->preamble || expr->postamble) {
      // append the postamble if present
      if (expr->postamble) str = mputstr(str, expr->postamble);
      // close the statement block
      str = mputstr(str, "}\n");
    }
    free_expr(expr);
    return str;
  }

  char *Code::translate_character(char *str, char c, bool in_string)
  {
    int i = (unsigned char)c;
    switch (i) {
    case '\a':
      return mputstr(str, "\\a");
    case '\b':
      return mputstr(str, "\\b");
    case '\f':
      return mputstr(str, "\\f");
    case '\n':
      return mputstr(str, "\\n");
    case '\r':
      return mputstr(str, "\\r");
    case '\t':
      return mputstr(str, "\\t");
    case '\v':
      return mputstr(str, "\\v");
    case '\\':
      return mputstr(str, "\\\\");
    case '\'':
      if (in_string) return mputc(str, '\'');
      else return mputstr(str, "\\'");
    case '"':
      if (in_string) return mputstr(str, "\\\"");
      else return mputc(str, '"');
    case '?':
      // to avoid recognition of trigraphs
      if (in_string) return mputstr(str, "\\?");
      else return mputc(str, '?');
    default:
      if (isascii(i) && isprint(i)) return mputc(str, c);
      return mputprintf(str, in_string ? "\\%03o" : "\\%o", i);
    }
  }

  char *Code::translate_string(char *str, const char *src)
  {
    for (size_t i = 0; src[i] != '\0'; i++)
      str = translate_character(str, src[i], true);
    return str;
  }

} // namespace Common

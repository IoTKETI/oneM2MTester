/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef TEXT_AST_HH
#define TEXT_AST_HH

#include "../Setting.hh"
#include "../Identifier.hh"
#include "../../common/memory.h"

struct textAST_matching_values {
  char* encode_token;
  char* decode_token;
  bool  case_sensitive;
  bool  generated_decode_token;
};

struct textAST_enum_def {
  Common::Identifier *name;
  textAST_matching_values value;
};

struct textAST_param_values {
  bool leading_zero;
  bool repeatable;
  int min_length;
  int max_length;
  int convert;
  int just;
};

class TextAST {
  private:
    void init_TextAST();
    TextAST(const TextAST&);
    TextAST& operator=(const TextAST&);
  public:
    textAST_matching_values *begin_val;
    textAST_matching_values *end_val;
    textAST_matching_values *separator_val;
    textAST_param_values coding_params;
    textAST_param_values decoding_params;
    size_t nof_field_params;
    textAST_enum_def **field_params;
    textAST_matching_values *true_params;
    textAST_matching_values *false_params;  
    char* decode_token;
    bool case_sensitive;
  
    TextAST() { init_TextAST(); }
    TextAST(const TextAST *other_val);
    ~TextAST();
    
    void print_TextAST() const;
    size_t get_field_param_index(const Common::Identifier *name);
};

void copy_textAST_matching_values(textAST_matching_values **to,
                                  const textAST_matching_values *from);

/** Checks for forbidden references within {} and substitutes the escape
 * sequences of apostrophes ('' and \') and quotation marks("" and \") in
 * \a decode_token. Returns the result of substitution. Argument \a loc is
 * used for error reporting. */
char *process_decode_token(const char *decode_token,
  const Common::Location& loc);

/**  "Encode" the TEXT matching pattern.
 *
 * @param str TTCN pattern from the TEXT attribute
 * @param cs true for case sensitive matching, false for case insensitive
 * @return a newly allocated string whose first character is
 * 'I' for case sensitive matching, 'N' for case insensitive matching,
 * or 'F' for fixed string matching.
 * The rest of the string is the POSIX regexp corresponding to the pattern
 * in \p str (or a copy of \p str for fixed string matching).
 *
 * The string must be deallocated by the caller using \b Free()
 */
char *make_posix_str_code(const char *str, bool cs);

/** Converts a TTCN-3 charstring value to an equivalent TTCN-3 pattern. The
 * special characters of TTCN-3 patterns are escaped. */
extern char *convert_charstring_to_pattern(const char *str);

void init_textAST_matching_values(textAST_matching_values *val);

#endif

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
#ifndef _TEXT_HH
#define _TEXT_HH

#include "Types.h"
#include "Encdec.hh"
#include <sys/types.h>
#include <regex.h>
/**
 * \defgroup TEXT TEXT-related stuff.
 *
 *  @{
 */
class CHARSTRING;
class Token_Match;
struct TTCN_TEXTdescriptor_values {
  boolean leading_zero;
  boolean repeatable;
  int min_length;
  int max_length;
  int convert;
  int just;
};

struct TTCN_TEXTdescriptor_bool {
  const CHARSTRING* true_encode_token;
  const Token_Match* true_decode_token;
  const CHARSTRING* false_encode_token;
  const Token_Match* false_decode_token;
};

struct TTCN_TEXTdescriptor_enum {
  const CHARSTRING* encode_token;
  const Token_Match* decode_token;
};


struct TTCN_TEXTdescriptor_param_values {
  TTCN_TEXTdescriptor_values coding_params;
  TTCN_TEXTdescriptor_values decoding_params;
};

union TTCN_TEXTdescriptor_union {
  const TTCN_TEXTdescriptor_param_values* parameters;
  const TTCN_TEXTdescriptor_bool* bool_values;
  const TTCN_TEXTdescriptor_enum* enum_values;
};

struct TTCN_TEXTdescriptor_t {
  const CHARSTRING* begin_encode;
  const Token_Match* begin_decode;
  const CHARSTRING* end_encode;
  const Token_Match* end_decode;
  const CHARSTRING* separator_encode;
  const Token_Match* separator_decode;
  const Token_Match* select_token;
  TTCN_TEXTdescriptor_union val;
};

class Token_Match {
  regex_t posix_regexp_begin; ///< regexp for the anchored match
  regex_t posix_regexp_first; ///< regexp for floating match
  const char *token_str; // not owned or freed by Token_Match
  size_t fixed_len; ///< length of fixed string, or zero
  boolean null_match; ///< true if the token_str was NULL or empty
public:
  Token_Match(const char *posix_str, boolean case_sensitive = TRUE,
    boolean fixed = FALSE);
  ~Token_Match();

  inline operator const char*() const { return token_str; }
  /** Match anchored at the beginning
   *
   * @param buf the buffer to search
   * @return length of the match or -1 if not matched
   *
   * It may call TTCN_error() in case of error, causing a DTE
   */
  int match_begin(TTCN_Buffer& buf) const;
  /** Floating match
   *
   * @param buf the buffer to search
   * @return the offset where the match begins, or -1 if not matched
   *
   * It may call TTCN_error() in case of error, causing a DTE
   */
  int match_first(TTCN_Buffer& buf) const;
};

class Limit_Token_List {
private:
  size_t num_of_tokens;
  size_t size_of_list;
  const Token_Match **list;
  int *last_match;
  int last_ret_val;
  const char* last_pos;
  /// Copy constructor disabled
  Limit_Token_List(const Limit_Token_List&);
  /// Assignment disabled
  Limit_Token_List& operator=(const Limit_Token_List&);
public:
  Limit_Token_List();
  ~Limit_Token_List();

  void add_token(const Token_Match *);
  void remove_tokens(size_t);
  int match(TTCN_Buffer&,size_t lim=0);
  inline boolean has_token(size_t ml=0) const { return num_of_tokens != ml; }
};

extern const TTCN_TEXTdescriptor_t INTEGER_text_;
extern const TTCN_TEXTdescriptor_t BOOLEAN_text_;
extern const TTCN_TEXTdescriptor_t CHARSTRING_text_;
extern const TTCN_TEXTdescriptor_t BITSTRING_text_;
extern const TTCN_TEXTdescriptor_t HEXSTRING_text_;
extern const TTCN_TEXTdescriptor_t OCTETSTRING_text_;
extern const TTCN_TEXTdescriptor_t UNIVERSAL_CHARSTRING_text_;

/** @} end of TEXT group */

#endif // _TEXT_HH

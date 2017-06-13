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
#include "TextAST.hh"
#include <stdio.h>
#include "common/pattern.hh"
#include "common/pattern_p.hh"

void TextAST::init_TextAST()
{
  begin_val=NULL;
  end_val=NULL;
  separator_val=NULL;
  nof_field_params=0;
  field_params=NULL;
  true_params=NULL;
  false_params=NULL;
  coding_params.leading_zero=false;
  coding_params.repeatable=false;
  coding_params.min_length=-1;
  coding_params.max_length=-1;
  coding_params.convert=0;
  coding_params.just=1;
  decoding_params.leading_zero=false;
  decoding_params.repeatable=false;
  decoding_params.min_length=-1;
  decoding_params.max_length=-1;
  decoding_params.convert=0;
  decoding_params.just=1;
  decode_token=NULL;
  case_sensitive=true;
}

TextAST::TextAST(const TextAST *other_val)
{
  init_TextAST();
  if(other_val){
//printf("HALI\n\r");
//other_val->print_TextAST();
    copy_textAST_matching_values(&begin_val,other_val->begin_val);
    copy_textAST_matching_values(&end_val,other_val->end_val);
    copy_textAST_matching_values(&separator_val,other_val->separator_val);

    nof_field_params=other_val->nof_field_params;
    if(nof_field_params) field_params=
      (textAST_enum_def**)Malloc(nof_field_params*sizeof(textAST_enum_def*));
    for(size_t a=0;a<nof_field_params;a++){
      if(other_val->field_params[a]){
        field_params[a]=(textAST_enum_def*)Malloc(sizeof(textAST_enum_def));
        field_params[a]->name=
          new Common::Identifier(*(other_val->field_params[a]->name));
        textAST_matching_values *b=&(field_params[a]->value);
        copy_textAST_matching_values(&b,&other_val->field_params[a]->value);
      } else field_params[a]=NULL;
    }
//print_TextAST();

    copy_textAST_matching_values(&true_params,other_val->true_params);
    copy_textAST_matching_values(&false_params,other_val->false_params);

    coding_params.leading_zero=other_val->coding_params.leading_zero;
    coding_params.repeatable=other_val->coding_params.repeatable;
    coding_params.min_length=other_val->coding_params.min_length;
    coding_params.max_length=other_val->coding_params.max_length;
    coding_params.convert=other_val->coding_params.convert;
    coding_params.just=other_val->coding_params.just;
    decoding_params.leading_zero=other_val->decoding_params.leading_zero;
    decoding_params.repeatable=other_val->decoding_params.repeatable;
    decoding_params.min_length=other_val->decoding_params.min_length;
    decoding_params.max_length=other_val->decoding_params.max_length;
    decoding_params.convert=other_val->decoding_params.convert;
    decoding_params.just=other_val->decoding_params.just;

    if(other_val->decode_token)
      decode_token=mcopystr(other_val->decode_token);
    case_sensitive=other_val->case_sensitive;
  }
}

TextAST::~TextAST()
{
  if(begin_val){
    Free(begin_val->encode_token);
    Free(begin_val->decode_token);
    Free(begin_val);
  }
  if(end_val){
    Free(end_val->encode_token);
    Free(end_val->decode_token);
    Free(end_val);
  }
  if(separator_val){
    Free(separator_val->encode_token);
    Free(separator_val->decode_token);
    Free(separator_val);
  }
  if(field_params){
    for(size_t a=0;a<nof_field_params;a++){
      //TODO: lets extract field_params[a] for speed
      if(field_params[a]){
        delete field_params[a]->name;
        Free(field_params[a]->value.encode_token);
        Free(field_params[a]->value.decode_token);
      }
      Free(field_params[a]);
    }
    Free(field_params);
  }
  if(true_params){
    Free(true_params->encode_token);
    Free(true_params->decode_token);
    Free(true_params);
  }
  if(false_params){
    Free(false_params->encode_token);
    Free(false_params->decode_token);
    Free(false_params);
  }
  Free(decode_token);
}

void TextAST::print_TextAST() const
{
  printf("\n\rBegin:");
  if(begin_val){
    printf("\n\r  Encode token:");
    if(begin_val->encode_token) printf(" %s\n\r",begin_val->encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(begin_val->decode_token) printf(" %s\n\r",begin_val->decode_token);
    else printf("  NULL\n\r");
    if(begin_val->case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  }
  else printf("NULL\n\r");
  printf("End:");
  if(end_val){
    printf("\n\r  Encode token:");
    if(end_val->encode_token) printf(" %s\n\r",end_val->encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(end_val->decode_token) printf(" %s\n\r",end_val->decode_token);
    else printf("  NULL\n\r");
    if(end_val->case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  }
  else printf("NULL\n\r");
  printf("Separator:");
  if(separator_val){
    printf("\n\r  Encode token:");
    if(separator_val->encode_token) printf(" %s\n\r",separator_val->encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(separator_val->decode_token) printf(" %s\n\r",separator_val->decode_token);
    else printf("  NULL\n\r");
    if(separator_val->case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  }
  else printf("NULL\n\r");
  printf("Select token:");
  if(decode_token) printf("%s",decode_token);
    if(case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  printf("True:");
  if(true_params){
    printf("\n\r  Encode token:");
    if(true_params->encode_token) printf(" %s\n\r",true_params->encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(true_params->decode_token) printf(" %s\n\r",true_params->decode_token);
    else printf("  NULL\n\r");
    if(true_params->case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  }
  else printf("NULL\n\r");
  printf("False:");
  if(true_params){
    printf("\n\r  Encode token:");
    if(false_params->encode_token) printf(" %s\n\r",false_params->encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(false_params->decode_token) printf(" %s\n\r",false_params->decode_token);
    else printf("  NULL\n\r");
    if(false_params->case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
  }
  else printf("NULL\n\r");
  printf("Number of fields:%lu\n\r",nof_field_params);
  for(size_t a=0;a<nof_field_params;a++){
    printf("Field %lu:\n\r",a);
    if(field_params[a]){
    printf("  Name: %s\n\r",field_params[a]->name->get_name().c_str());
    printf("  Encode token:");
    if(field_params[a]->value.encode_token) printf(" %s\n\r",field_params[a]->value.encode_token);
    else printf("  NULL\n\r");
    printf("  Decode token:");
    if(field_params[a]->value.decode_token) printf(" %s\n\r",field_params[a]->value.decode_token);
    else printf("  NULL\n\r");
    if(field_params[a]->value.case_sensitive) printf("  case_sensitive\n\r");
    else printf("  case_insensitive\n\r");
    } else printf("  NULL\n\r");
  }
  printf("Coding params:\n\r  Leading 0:");
    if(coding_params.leading_zero) printf(" true\n\r");
    else printf(" false\n\r");
    printf("  Length: %i - %i\n\r",coding_params.min_length,coding_params.max_length);
    printf("  Convert: %i\n\r  Just:%i\n\r",coding_params.convert,coding_params.just);

  printf("Decoding params:\n\r  Leading 0:");
    if(decoding_params.leading_zero) printf(" true\n\r");
    else printf(" false\n\r");
    printf("  Length: %i - %i\n\r",decoding_params.min_length,decoding_params.max_length);
    printf("  Convert: %i\n\r  Just:%i\n\r",decoding_params.convert,decoding_params.just);
}

size_t TextAST::get_field_param_index(const Common::Identifier *name)
{
  for(size_t a=0;a<nof_field_params;a++){
    if(*field_params[a]->name==*name) return a;
  }
  field_params=(textAST_enum_def **)Realloc(field_params,(nof_field_params+1)*sizeof(textAST_enum_def *));
  field_params[nof_field_params]=(textAST_enum_def*)Malloc(sizeof(textAST_enum_def));
  field_params[nof_field_params]->name= new Common::Identifier(*name);
  field_params[nof_field_params]->value.encode_token=NULL;
  field_params[nof_field_params]->value.decode_token=NULL;
  field_params[nof_field_params]->value.case_sensitive=true;
  nof_field_params++;
  return nof_field_params-1;
}

void copy_textAST_matching_values(textAST_matching_values **to,
                                  const textAST_matching_values *from)
{
  if(from==NULL) return;
  if(*to==NULL)
    *to=(textAST_matching_values*)Malloc(sizeof(textAST_matching_values));
  if(from->encode_token) (*to)->encode_token=mcopystr(from->encode_token);
  else (*to)->encode_token=NULL;
  if(from->decode_token && !from->generated_decode_token)
    (*to)->decode_token=mcopystr(from->decode_token);
  else (*to)->decode_token=NULL;
  (*to)->case_sensitive=from->case_sensitive;
  (*to)->generated_decode_token=false;
}

char *process_decode_token(const char *decode_token,
  const Common::Location& loc)
{
  enum { INITIAL, BS, BS_N, BS_Q } state = INITIAL;
  // points to the last backslash found
  size_t bs_idx = 0;
  char *ret_val = memptystr();
  for (size_t i = 0; decode_token[i] != '\0'; i++) {
    switch (state) {
    case INITIAL:
      switch (decode_token[i]) {
      case '\\':
        state = BS;
	bs_idx = i;
	break;
      case '{':
        // {reference}: find the matching bracket and skip the entire reference
        for (size_t j = i + 1; ; j++) {
	  if (decode_token[j] == '\0') {
	    loc.error("Invalid reference `%s' in matching pattern",
	      decode_token + i);
	    i = j - 1;
	    break;
	  } else if (decode_token[j] == '}') {
	    loc.error("Reference `%s' is not allowed in matching pattern",
	      string(j + 1 - i, decode_token + i).c_str());
	    i = j;
	    break;
	  }
	}
	break;
      case '\'':
      case '"':
        // replace '' -> ' and "" -> "
	ret_val = mputc(ret_val, decode_token[i]);
	if (decode_token[i + 1] == decode_token[i]) i++;
        break;
      default:
	ret_val = mputc(ret_val, decode_token[i]);
      }
      break;
    case BS:
      switch (decode_token[i]) {
      case 'N':
        state = BS_N;
	break;
      case 'q':
        state = BS_Q;
	break;
      case '\'':
      case '"':
        // replace \' -> ' and \" -> "
        ret_val = mputc(ret_val, decode_token[i]);
	state = INITIAL;
	break;
      default:
        // keep other escape sequences
        ret_val = mputc(ret_val, '\\');
        ret_val = mputc(ret_val, decode_token[i]);
	state = INITIAL;
      }
      break;
    case BS_N:
      switch (decode_token[i]) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\v':
      case '\f':
	// ignore whitespace following \N
	break;
      case '{':
        // \N{reference}: find the matching bracket and skip the entire
	// reference
        for (size_t j = i + 1; ; j++) {
	  if (decode_token[j] == '\0') {
	    loc.error("Invalid character set reference `%s' in matching "
	      "pattern", decode_token + bs_idx);
	    i = j - 1;
	    break;
	  } else if (decode_token[j] == '}') {
	    loc.error("Character set reference `%s' is not allowed in matching "
	      "pattern", string(j + 1 - bs_idx, decode_token + bs_idx).c_str());
	    i = j;
	    break;
	  }
	}
	state = INITIAL;
	break;
      default:
	loc.error("Invalid character set reference `%s' in matching pattern",
	  string(i + 1 - bs_idx, decode_token + bs_idx).c_str());
        state = INITIAL;
      }
      break;
    case BS_Q:
      switch (decode_token[i]) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case '\v':
      case '\f':
	// ignore whitespace following \q
	break;
      case '{':
	// copy the rest of the quadruple literally
        ret_val = mputstr(ret_val, "\\q{");
	state = INITIAL;
	break;
      default:
	loc.error("Invalid quadruple notation `%s' in matching pattern",
	  string(i + 1 - bs_idx, decode_token + bs_idx).c_str());
        state = INITIAL;
      }
    }
  }
  // final check of state
  switch (state) {
  case INITIAL:
    break;
  case BS:
    loc.error("Invalid trailing backslash in matching pattern");
    break;
  case BS_N:
    loc.error("Invalid character set reference `%s' in matching pattern",
      decode_token + bs_idx);
    break;
  case BS_Q:
    loc.error("Invalid quadruple notation `%s' in matching pattern",
      decode_token + bs_idx);
  }
  return ret_val;
}

extern bool has_meta(); // in pattern_la.l

extern int pattern_yylex();
extern void init_pattern_yylex(YYSTYPE *p);
struct yy_buffer_state;
extern yy_buffer_state* pattern_yy_scan_string(const char*);
extern int pattern_yylex_destroy();

char *make_posix_str_code(const char *pat, bool cs)
{
  if (pat == NULL || pat[0] == '\0') return mcopystr("I");
  char *ret_val;

  ret_val = mcopystr(cs ? "I" : "N"); // Case sensitive? _I_gen / _N_em

  // Run the pattern lexer over the pattern to check for meta-characters.
  // TTCN_pattern_to_regexp() would be overkill.
  yy_buffer_state *flex_buffer = pattern_yy_scan_string(pat);
  if (flex_buffer == NULL) {} //a fatal error was already generated
  else {
    YYSTYPE yylval;
    init_pattern_yylex(&yylval);
    while (pattern_yylex()) {} // 0 means end-of-string
    pattern_yylex_destroy();
  }

  // If the matching is case sensitive and there are no meta-characters,
  // matching for fixed strings can be done without regular expressions.
  const bool maybe_fixed = cs && !has_meta();

  char *mod_token = mprintf("(%s)*", pat);
  // disable all warnings and "not supported" messages while translating the
  // pattern to POSIX regexp again (these messages were already reported
  // during semantic analysis)
  unsigned orig_verb_level = verb_level;
  verb_level &= ~(1|2);
  char *posix_str = TTCN_pattern_to_regexp(mod_token);
  verb_level = orig_verb_level;
  Free(mod_token);
  if (posix_str == NULL) FATAL_ERROR("make_posix_str_code()");

  size_t len = mstrlen(posix_str);
  if (maybe_fixed) { // maybe we can do fixed strings
    *ret_val = 'F'; // "fixed"

    // posix_str contains: ^(pat).*$   but all we need is pat
    ret_val = mputstrn(ret_val, posix_str + 2, len - 6);
  }
  else { // no fixed-string optimization possible
    size_t skip = 0;
    // TEXT decoder optimization
    if (len > 3) {
      if (!memcmp(posix_str + (len - 3), ".*$", 3)) {
        // The POSIX RE is set to match a bunch of characters before the end.
        // This happens if the TTCN pattern wanted to match anywhere, not just
        // at the end (TTCN patterns are anchored at the beginning and end).
        // Instead of looking for (0 or more) characters and ignoring them,
        // just don't look for them at all. RE are not anchored by default.
        posix_str = mtruncstr(posix_str, len -= 3);
      }

      if (len > 3) { // len might have changed while truncating, above
        // Un-anchor the cheap way, at the beginning.
        skip = memcmp(posix_str, "^.*", 3) ? 0 : 3;
      }
    }

    ret_val = mputstr(ret_val, posix_str + skip);
  }

  Free(posix_str);

  return ret_val;
}

char *convert_charstring_to_pattern(const char *str)
{
  char *ret_val = memptystr();
  for (size_t i = 0; str[i]; i++) {
    switch (str[i]) {
    case '?':
    case '*':
    case '\\':
    case '[':
    case ']':
    case '{':
    case '}':
    case '|':
    case '(':
    case ')':
    case '#':
    case '+':
      // special characters of TTCN-3 patterns; escape with a backslash
      ret_val = mputc(ret_val, '\\');
      // no break
    default:
      ret_val = mputc(ret_val, str[i]);
    }
  }
  return ret_val;
}

void init_textAST_matching_values(textAST_matching_values *val){
  val->encode_token=NULL;
  val->decode_token=NULL;
  val->case_sensitive=true;
  val->generated_decode_token=false;
}

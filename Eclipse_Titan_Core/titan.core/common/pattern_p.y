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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
 
/**
 * Parser for TTCN-3 character patterns.
 *
 * \author Matyas Forstner (Matyas.Forstner@eth.ericsson.se)
 *
 * 20031121
 */

%{

/*********************************************************************
 * C(++) declarations
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(__CYGWIN__) && defined(__clang__)
/* Cygwin's clang 3.0 has its own limits.h, which does not bring in
   the system's limits.h unless we define this macro: */
#define __STDC_HOSTED__ 1
#define _GCC_NEXT_LIMITS_H
#endif
#include <limits.h>

#include <regex.h>
#if !defined(RE_DUP_MAX)
/* RE_DUP_MAX is defined in limits.h or regex.h, except on Cygwin 1.5 */
# include <sys/syslimits.h>
#endif

#include "memory.h"
#include "pattern.hh"

/* defined in lexer c-file: */

  union YYSTYPE;
  extern int pattern_yylex();
  extern void init_pattern_yylex(YYSTYPE *p);
  struct yy_buffer_state;
  extern yy_buffer_state* pattern_yy_scan_string(const char*);
  extern int pattern_yylex_destroy();
  extern unsigned int get_nof_parentheses();

/* defined in this file: */

  /** The converted regexp. */
  static char *ret_val;
  /** Turns error messages for extended ASCII characters on or off */
  static bool allow_ext_ascii = false;
  /** The parser error reporting function. */
  static void pattern_yyerror(const char *error_str);
  /** Creates the POSIX equivalent of literal character \a c using the
   * appropriate escape sequence when needed. */
  static char *translate_character(char c);
  /** Returns the printable equivalent of character \a c */
  static char *print_character(char c);
  /** Returns the printable equivalent of range \a lower .. \a upper */
  static char *print_range(char lower, char upper);
  /** structure for manipulating character sets */
  struct character_set;
  /** allocates, initializes and returns a new empty set */
  static character_set *set_init();
  /** allocates and returns a copy of \a set */
  static character_set *set_copy(const character_set *set);
  /** deallocates set \a set */
  static void set_free(character_set *set);
  /** returns whether set \a set is empty */
  static int set_is_empty(const character_set *set);
  /** returns whether set \a set contains all characters in range 1..127 */
  static int set_is_full(const character_set *set);
  /** returns whether set \a set contains the character \a c */
  static int set_has_char(const character_set *set, char c);
  /** adds character \a c to set \a set */
  static void set_add_char(character_set *set, char c);
  /** removes character \a c to set \a set */
  static void set_remove_char(character_set *set, char c);
  /** returns whether set \a set contains at least one character in the range
   * \a lower .. \a upper */
  static int set_has_range(const character_set *set, char lower, char upper);
  /** adds range \a lower .. \a upper to set \a set */
  static void set_add_range(character_set *set, char lower, char upper);
  /** returns whether set \a set1 and \a set2 has non-empty intersect */
  static int set_has_intersect(const character_set *set1,
    const character_set *set2);
  /** joins sets \a dst and \a src into \a dst */
  static void set_join(character_set *dst, const character_set *src);
  /** negates the set \a set */
  static void set_negate(character_set *set);
  /** reports the duplicate occurrences of characters and ranges in \a set1
   * and \a set2 */
  static void set_report_duplicates(const character_set *set1,
    const character_set *set2);
  /** generates the POSIX equivalent of \a set */
  static char *set_generate_posix(const character_set *set);

#define YYERROR_VERBOSE

static void yyprint(FILE *file, int type, const YYSTYPE& value);
#define YYPRINT(f,t,v) yyprint(f,t,v)

%}

/*********************************************************************
 * Bison declarations
 *********************************************************************/

%name-prefix="pattern_yy"
%output="pattern_p.cc"
%defines
%verbose
%expect 0
%start Pattern
%debug

/*********************************************************************
 * The union-type
 * Must be kept in sync with the one in pattern_uni.y !
 *********************************************************************/

%union {
  int b; /* boolean */
  char c; /* single character */
  char *s; /* character string */
  unsigned long int u; /* unsigned integer */
  struct character_set *set; // used by nonterminals in pattern_p.y

  union {
    unsigned int value;
#if defined(__sparc__) || defined(__sparc)
    struct {
      unsigned char group;
      unsigned char plane;
      unsigned char row;
      unsigned char cell;
    } comp;
#else
    struct {
      unsigned char cell;
      unsigned char row;
      unsigned char plane;
      unsigned char group;
    } comp;
#endif
  } q;  // single universal char, used by nonterminals in pattern_uni.y
  class QuadSet* qset;  // used by nonterminals in pattern_uni.y
}

/*********************************************************************
 * Tokens
 *********************************************************************/

%token <c> TOK_Char "<ordinary character>"
%token <u> TOK_Number "<number>"
%token <u> TOK_Digit "<digit>"

/*********************************************************************
 * Keywords
 *********************************************************************/

%token KW_BS_q "\\q"
%token KW_BS_d "\\d"
%token KW_BS_w "\\w"
%token KW_BS_t "\\t"
%token KW_BS_n "\\n"
%token KW_BS_r "\\r"
%token KW_BS_s "\\s"
%token KW_BS_b "\\b"

%token KW_Group_Begin "("
%token KW_Group_End ")"
%token KW_Set_Begin "["
%token KW_Set_Begin_Neg "[^"
%token KW_Set_Begin_Rsbrkt "[]"
%token KW_Set_Begin_Neg_Rsbrkt "[^]"
%token KW_Set_End "]"
%token KW_Set_Dash_End "-]"

/*********************************************************************
 * semantic types of nonterminals
 *********************************************************************/

%type <b> RE_Set_Begin RE_Set_Begin_Rsbrkt RE_Set_End
%type <c> RE_Set_Range_Char RE_Quadruple
%type <s> RE_Body RE_Elems RE_Alter_Elem RE_Concat_Elem
          RE_Multiply_Elem RE_Multiply_Statement RE_Group
          RE_OneCharPos
%type  <set> RE_Set RE_Set_Body RE_Set_Elem RE_Set_NoRange_Char

/*********************************************************************
 * Destructors
 *********************************************************************/

%destructor { Free($$); }
RE_Alter_Elem
RE_Body
RE_Concat_Elem
RE_Elems
RE_Group
RE_Multiply_Elem
RE_Multiply_Statement
RE_OneCharPos

%destructor { set_free($$); }
RE_Set
RE_Set_Body
RE_Set_Elem
RE_Set_NoRange_Char

%%

/*********************************************************************
 * Grammar
 *********************************************************************/

Pattern:
  RE_Body {ret_val=$1;}
;

RE_Body:
  /* empty */
  {
    $$ = mcopystr("^$");
  }
| RE_Elems
  {
    if ($1 != NULL) {
      $$ = mprintf("^%s$", $1);
      Free($1);
    } else $$ = mcopystr("^$");
  }
;

RE_Elems:
  RE_Alter_Elem { $$ = $1; }
| RE_Elems '|' RE_Alter_Elem
  {
    unsigned int nof_pars = get_nof_parentheses() + (yychar==KW_Group_End ? 1 : 0);
    if ($3 != NULL) {
      if ($1 != NULL) $$ = mputprintf($1, nof_pars ? "|%s" : "$|^%s", $3);
      else $$ = mprintf( nof_pars ? "()|%s" : "()$|^%s" , $3);
      Free($3);
    } else {
      if ($1 != NULL) $$ = mputstr($1, nof_pars ? "|()" : "$|^()");
      else $$ = NULL;
    }
  }
;

RE_Alter_Elem:
  RE_Concat_Elem { $$ = $1; }
| RE_Alter_Elem RE_Concat_Elem
  {
    $$ = mputstr($1, $2);
    Free($2);
  }
;

RE_Concat_Elem:
  RE_Multiply_Elem {$$=$1;}
| RE_Multiply_Elem RE_Multiply_Statement
  {
    if ($1 != NULL && $2 != NULL) {
      $$ = mputstr($1, $2);
      Free($2);
    } else {
      Free($1);
      Free($2);
      $$ = NULL;
    }
  }
| '*' {$$=mcopystr(".*");}
;

RE_Multiply_Elem:
  RE_Group {$$=$1;}
| RE_OneCharPos {$$=$1;}
;

RE_Group:
  KW_Group_Begin KW_Group_End
  {
    $$ = mcopystr("()");
  }
| KW_Group_Begin RE_Elems KW_Group_End
  {
    if ($2 != NULL) {
      $$ = mprintf("(%s)", $2);
      Free($2);
    } else {
      $$ = mcopystr("()");
    }
  }
;

RE_Multiply_Statement:
  '+'
  {
    $$ = mcopystr("+");
  }
| '#' '(' ',' ')'
  {
    $$ = mcopystr("*");
  }
| '#' TOK_Digit
  {
    if ($2 == 0) {
      TTCN_pattern_warning("The number of repetitions is zero: `#0'.");
      $$ = NULL;
    } else if ($2 == 1) $$ = memptystr();
    else {
      if ($2 > 9) TTCN_pattern_warning("Internal error: Invalid number of "
	"repetitions: `#%lu'.", $2);
      $$ = mprintf("{%lu}", $2);
    }
  }
| '#' '(' TOK_Number ')'
  {
    if ($3 == 0) {
      TTCN_pattern_warning("The number of repetitions is zero: `#(0)'.");
      $$ = NULL;
    } else if ($3 == 1) $$ = memptystr();
    else {
#ifdef RE_DUP_MAX
      if ($3 > RE_DUP_MAX) TTCN_pattern_warning("The number of repetitions in "
	"`#(%lu)' exceeds the limit allowed by this system (%d).", $3,
	RE_DUP_MAX);
#endif
      $$ = mprintf("{%lu}", $3);
    }
  }
| '#' '(' TOK_Number ',' TOK_Number ')'
  {
#ifdef RE_DUP_MAX
    if ($3 > RE_DUP_MAX) TTCN_pattern_warning("The minimum number of "
      "repetitions in `#(%lu,%lu)' exceeds the limit allowed by this system "
      "(%d).", $3, $5, RE_DUP_MAX);
    if ($5 > RE_DUP_MAX) TTCN_pattern_warning("The maximum number of "
      "repetitions in `#(%lu,%lu)' exceeds the limit allowed by this system "
      "(%d).", $3, $5, RE_DUP_MAX);
#endif
    if ($3 > $5) TTCN_pattern_error("The lower bound is higher than the upper "
      "bound in the number of repetitions: `#(%lu,%lu)'.", $3, $5);
    if ($3 == $5) {
      if ($3 == 0) {
	TTCN_pattern_warning("The number of repetitions is zero: `#(0,0)'.");
	$$ = NULL;
      } else if ($3 == 1) $$ = memptystr();
      else {
	$$ = mprintf("{%lu}", $3);
      }
    } else {
      if ($3 == 0 && $5 == 1) $$ = mcopystr("?");
      else $$ = mprintf("{%lu,%lu}", $3, $5);
    }
  }
| '#' '(' ',' TOK_Number ')'
  {
    if ($4 == 0) {
      TTCN_pattern_warning("The number of repetitions is zero: `#(,0)'.");
      $$ = NULL;
    } else {
#ifdef RE_DUP_MAX
      if ($4 > RE_DUP_MAX) TTCN_pattern_warning("The maximum number of "
	"repetitions in `#(,%lu)' exceeds the limit allowed by this system "
	"(%d).", $4, RE_DUP_MAX);
#endif
      if ($4 == 1) $$ = mcopystr("?");
      else $$ = mprintf("{0,%lu}", $4);
    }
  }
| '#' '(' TOK_Number ',' ')'
  {
    if ($3 == 0) $$ = mcopystr("*");
    else {
#ifdef RE_DUP_MAX
      if ($3 > RE_DUP_MAX) TTCN_pattern_warning("The minimum number of "
	"repetitions in `#(%lu,)' exceeds the limit allowed by this system "
	"(%d).", $3, RE_DUP_MAX);
#endif
      if ($3 == 1) $$ = mcopystr("+");
      else $$ = mprintf("{%lu,}", $3);
    }
  }
;

RE_OneCharPos:
  '?' {$$=mcopystr(".");}
| KW_BS_d {$$=mcopystr("[0-9]");}
| KW_BS_w {$$=mcopystr("[0-9A-Za-z]");}
| KW_BS_t {$$=mcopystr("\t");}
| KW_BS_n {$$=mcopystr("[\n-\r]");}
| KW_BS_r {$$=mcopystr("\r");}
| KW_BS_s {$$=mcopystr("[\t-\r ]");}
| KW_BS_b
  {
    TTCN_pattern_warning("Metacharacter `\\b' is not supported yet.");
    $$ = NULL;
  }
| TOK_Char
  {
    unsigned char c = $1;
    if (c == 0 || (c > 127 && !allow_ext_ascii)) TTCN_pattern_error("Character "
      "with code %u (0x%02x) cannot be used in a pattern for type charstring.", c, c);
    $$ = translate_character($1);
  }
| RE_Quadruple
  {
    $$ = translate_character($1);
  }
| RE_Set
  {
    if (set_is_empty($1)) {
      TTCN_pattern_error("Empty character set.");
      $$ = NULL;
    } else $$ = set_generate_posix($1);
    set_free($1);
  }
;

RE_Set:
  /* RE_Set_Begin        is 1 for "[^",  0 for "["
   * RE_Set_Begin_Rsbrkt is 1 for "[^]", 0 for "[]"
   * RE_Set_End          is 1 for "-]",  0 for "]"
   */
  RE_Set_Begin RE_Set_Body RE_Set_End
  {
    if ($2 != NULL) $$ = $2;
    else $$ = set_init();
    if ($3) {
      if (set_has_char($$, '-'))
	TTCN_pattern_warning("Duplicate character `-' in the character set.");
      else set_add_char($$, '-');
    }
    if ($1) set_negate($$);
  }
| RE_Set_Begin '-' RE_Set_Body RE_Set_End
  {
    if ($3 != NULL) $$ = $3;
    else $$ = set_init();
    if (set_has_char($$, '-'))
      TTCN_pattern_warning("Duplicate character `-' in the character set.");
    else set_add_char($$, '-');
    if ($4) {
      if (set_has_char($$, '-'))
	TTCN_pattern_warning("Duplicate character `-' in the character set.");
      else set_add_char($$, '-');
    }
    if ($1) set_negate($$);
  }
| RE_Set_Begin_Rsbrkt RE_Set_Body RE_Set_End
  {
    if ($2 != NULL) $$ = $2;
    else $$ = set_init();
    if (set_has_char($$, ']'))
      TTCN_pattern_warning("Duplicate character `]' in the character set.");
    else set_add_char($$, ']');
    if ($3) {
      if (set_has_char($$, '-'))
	TTCN_pattern_warning("Duplicate character `-' in the character set.");
      else set_add_char($$, '-');
    }
    if ($1) set_negate($$);
  }
| RE_Set_Begin_Rsbrkt '-' RE_Set_Range_Char RE_Set_Body RE_Set_End
  {
    if ($4 != NULL) $$ = $4;
    else $$ = set_init();
    char *range_str = print_range(']', $3);
    if (']' > $3) {
      TTCN_pattern_error("Invalid range `%s' in the character set: the "
	"character code of the lower bound (%u) is higher than that of the "
	"upper bound (%u).", range_str, ']', static_cast<unsigned char>($3));
    } else {
      if (set_has_range($$, ']', $3)) {
	character_set *tmpset = set_init();
	set_add_range(tmpset, ']', $3);
	set_report_duplicates($$, tmpset);
	set_free(tmpset);
      }
    }
    set_add_range($$, ']', $3);
    Free(range_str);
    if ($5) {
      if (set_has_char($$, '-'))
	TTCN_pattern_warning("Duplicate character `-' in the character set.");
      else set_add_char($$, '-');
    }
    if ($1) set_negate($$);
  }
;

RE_Set_Begin:
  KW_Set_Begin { $$ = 0; }
| KW_Set_Begin_Neg { $$ = 1; }
;

RE_Set_Begin_Rsbrkt:
  KW_Set_Begin_Rsbrkt { $$ = 0; }
| KW_Set_Begin_Neg_Rsbrkt { $$ = 1; }
;

RE_Set_End:
  KW_Set_End { $$ = 0; }
| KW_Set_Dash_End { $$ = 1; }
;

RE_Set_Body:
  /* empty */ { $$ = NULL; }
| RE_Set_Body RE_Set_Elem
  {
    if ($1 != NULL) {
      $$ = $1;
      if (set_has_intersect($$, $2)) set_report_duplicates($$, $2);
      set_join($$, $2);
      set_free($2);
    } else $$ = $2;
  }
;

RE_Set_Elem:
  RE_Set_Range_Char
  {
    $$ = set_init();
    set_add_char($$, $1);
  }
| RE_Set_NoRange_Char { $$ = $1; }
| RE_Set_Range_Char '-' RE_Set_Range_Char
  {
    if ($1 > $3) {
      char *range_str = print_range($1, $3);
      TTCN_pattern_error("Invalid range `%s' in the character set: the "
	"character code of the lower bound (%u) is higher than that of the "
	"upper bound (%u).", range_str, static_cast<unsigned char>($1), static_cast<unsigned char>($3));
      Free(range_str);
    }
    $$ = set_init();
    set_add_range($$, $1, $3);
  }
;

RE_Set_Range_Char:
  KW_BS_t { $$ = '\t'; }
| KW_BS_r { $$ = '\r'; }
| TOK_Char
  {
    unsigned char c = $1;
    if (c == 0 || (c > 127 && !allow_ext_ascii)) TTCN_pattern_error("Character "
      "with code %u (0x%02x) cannot be used in a pattern for type charstring.", c, c);
    $$ = $1;
  }
| RE_Quadruple { $$ = $1; }
;

RE_Set_NoRange_Char:
  KW_BS_d
  {
    $$ = set_init();
    set_add_range($$, '0', '9');
  }
| KW_BS_w
  {
    $$ = set_init();
    set_add_range($$, '0', '9');
    set_add_range($$, 'A', 'Z');
    set_add_range($$, 'a', 'z');
  }
| KW_BS_n
  {
    $$ = set_init();
    set_add_range($$, '\n', '\r');
  }
| KW_BS_s
  {
    $$ = set_init();
    set_add_range($$, '\t', '\r');
    set_add_char($$, ' ');
  }
| KW_BS_b
  {
    TTCN_pattern_error("Metacharacter `\\b' does not make any sense in a "
      "character set.");
    $$ = set_init();
  }
;

RE_Quadruple:
  KW_BS_q '{' TOK_Number ',' TOK_Number ',' TOK_Number ',' TOK_Number '}'
  {
    if ($3 > 127) TTCN_pattern_error("The first number (group) of quadruple "
      "`\\q{%lu,%lu,%lu,%lu}' is too large. It should be in the range 0..127 "
      "instead of %lu.", $3, $5, $7, $9, $3);
    if ($5 > 255) TTCN_pattern_error("The second number (plane) of quadruple "
      "`\\q{%lu,%lu,%lu,%lu}' is too large. It should be in the range 0..255 "
      "instead of %lu.", $3, $5, $7, $9, $5);
    if ($7 > 255) TTCN_pattern_error("The third number (row) of quadruple "
      "`\\q{%lu,%lu,%lu,%lu}' is too large. It should be in the range 0..255 "
      "instead of %lu.", $3, $5, $7, $9, $7);
    if ($9 > 255) TTCN_pattern_error("The fourth number (cell) of quadruple "
      "`\\q{%lu,%lu,%lu,%lu}' is too large. It should be in the range 0..255 "
      "instead of %lu.", $3, $5, $7, $9, $9);
    if ($3 > 0 || $5 > 0 || $7 > 0 || $9 > 127) TTCN_pattern_error("Quadruple "
      "`\\q{%lu,%lu,%lu,%lu}' is not valid in a pattern for type charstring.",
      $3, $5, $7, $9);
    if ($3 == 0 && $5 == 0 && $7 == 0 && $9 == 0) TTCN_pattern_error("Zero "
      "character (i.e. quadruple `\\q{0,0,0,0}') is not supported in a "
      "pattern for type charstring.");
    $$ = $9;
  }
;

%%

/*********************************************************************
 * Interface
 *********************************************************************/

char* TTCN_pattern_to_regexp(const char* p_pattern, bool utf8)
{
  /* if you want to debug */
  //pattern_yydebug=1;

  ret_val=NULL;

  /* allow extended ASCII characters if the pattern is in UTF-8 format */
  allow_ext_ascii = utf8;

  yy_buffer_state *flex_buffer = pattern_yy_scan_string(p_pattern);
  if(flex_buffer == NULL) {
    TTCN_pattern_error("Flex buffer creation failed.");
    return NULL;
  }
  init_pattern_yylex(&yylval);
  if(pattern_yyparse()) {
    Free(ret_val);
    ret_val=NULL;
  }
  pattern_yylex_destroy();
  return ret_val;
}

// Backwards compatibility shim
char* TTCN_pattern_to_regexp(const char* p_pattern, int ere)
{
  TTCN_pattern_warning(
    "TTCN_pattern_to_regexp(const char* p_pattern, int ere) is deprecated");
  if (ere != 1) TTCN_pattern_error(
    "BRE is not supported for TTCN_pattern_to_regexp");
  return TTCN_pattern_to_regexp(p_pattern);
}

/*********************************************************************
 * Static functions
 *********************************************************************/

/// Error reporting function
void pattern_yyerror(const char *error_str)
{
  TTCN_pattern_error("%s", error_str);
}

/** Escape plain characters which would be metacharacters in a regex.
 *
 * @param c plain character
 * @return a newly allocated string which must be Free() 'd
 */
char *translate_character(char c)
{
  int escape_needed = 0;
  switch (c) {
  case '|':
  case '+':
  case '?':
  case '{':
  case '}':
  case '(':
  case ')':
  case '.':
  case '^':
  case '$':
  case '[':
  case '*':
  case '\\':
    escape_needed = 1;
    break;
  }
  if (escape_needed) return mprintf("\\%c", c);
  else return mputc(NULL, c);
}

char *print_character(char c)
{
  switch (c) {
  case '\t':
    return mcopystr("\\t");
  case '\r':
    return mcopystr("\\r");
  default:
    if (isprint(static_cast<unsigned char>(c))) return mprintf("%c", c);
    else return mprintf("\\q{0,0,0,%u}", static_cast<unsigned char>(c));
  }
}

char *print_range(char lower, char upper)
{
  char *range_str = print_character(lower);
  range_str = mputc(range_str, '-');
  char *upper_str = print_character(upper);
  range_str = mputstr(range_str, upper_str);
  Free(upper_str);
  return range_str;
}

#define CS_BITS_PER_ELEM (8 * sizeof(unsigned long))
#define CS_NOF_ELEMS ((128 + CS_BITS_PER_ELEM - 1) / CS_BITS_PER_ELEM)

struct character_set {
  unsigned long set_members[CS_NOF_ELEMS];
};

character_set *set_init()
{
  character_set *set = static_cast<character_set*>(Malloc(sizeof(*set)));
  memset(set->set_members, 0, sizeof(set->set_members));
  return set;
}

character_set *set_copy(const character_set *set)
{
  character_set *set2 = static_cast<character_set*>(Malloc(sizeof(*set2)));
  memcpy(set2, set, sizeof(*set2));
  return set2;
}

void set_free(character_set *set)
{
  Free(set);
}

int set_is_empty(const character_set *set)
{
  if ((set->set_members[0] & ~1UL) != 0) return 0;
  for (size_t i = 1; i < CS_NOF_ELEMS; i++)
    if (set->set_members[i] != 0) return 0;
  return 1;
}

int set_is_full(const character_set *set)
{
  if (~(set->set_members[0] | 1UL) != 0) return 0;
  for (size_t i = 1; i < CS_NOF_ELEMS; i++)
    if (~set->set_members[i] != 0) return 0;
  return 1;
}

int set_has_char(const character_set *set, char c)
{
  if (set->set_members[c / CS_BITS_PER_ELEM] & 1UL << c % CS_BITS_PER_ELEM)
    return 1;
  else return 0;
}

void set_add_char(character_set *set, char c)
{
  set->set_members[c / CS_BITS_PER_ELEM] |= 1UL << c % CS_BITS_PER_ELEM;
}

void set_remove_char(character_set *set, char c)
{
  set->set_members[c / CS_BITS_PER_ELEM] &= ~(1UL << c % CS_BITS_PER_ELEM);
}

int set_has_range(const character_set *set, char lower, char upper)
{
  for (size_t i = lower; i <= static_cast<unsigned char>(upper); i++)
    if (set->set_members[i / CS_BITS_PER_ELEM] & 1UL << i % CS_BITS_PER_ELEM)
      return 1;
  return 0;
}

void set_add_range(character_set *set, char lower, char upper)
{
  for (size_t i = lower; i <= static_cast<unsigned char>(upper); i++)
    set->set_members[i / CS_BITS_PER_ELEM] |= 1UL << i % CS_BITS_PER_ELEM;
}

int set_has_intersect(const character_set *set1, const character_set *set2)
{
  for (size_t i = 0; i < CS_NOF_ELEMS; i++)
    if (set1->set_members[i] & set2->set_members[i]) return 1;
  return 0;
}

void set_join(character_set *dst, const character_set *src)
{
  for (size_t i = 0; i < CS_NOF_ELEMS; i++)
    dst->set_members[i] |= src->set_members[i];
}

void set_negate(character_set *set)
{
  for (size_t i = 0; i < CS_NOF_ELEMS; i++)
    set->set_members[i] = ~set->set_members[i];
}

void set_report_duplicates(const character_set *set1,
  const character_set *set2)
{
  for (unsigned char i = 0; i <= 127; ) {
    for (i++; i <= 127; i++)
      if (set_has_char(set2, i) && set_has_char(set1, i)) break;
    if (i > 127) break;
    char lower = i;
    for (i++; i <= 127; i++)
      if (!set_has_char(set2, i) || !set_has_char(set1, i)) break;
    char upper = i - 1;
    if (lower < upper) {
      char *range_str = print_range(lower, upper);
      TTCN_pattern_warning("Duplicate range `%s' in the character set.",
	range_str);
      Free(range_str);
    } else {
      char *char_str = print_character(lower);
      if(lower == '\r' ){
        TTCN_pattern_warning("Duplicate character `%s' in the character "
	        "set. Please note the \\n includes the \\r implicitly. "
          "Use \\q{0,0,0,10} if you would like to match the LF only.", char_str);
      } else {
        TTCN_pattern_warning("Duplicate character `%s' in the character "
	        "set.", char_str);
      }
      Free(char_str);
    }
  }
}

static char *append_posix_body(char *set_body, const character_set *set)
{
  for (unsigned char i = 0; i <= 127; ) {
    for (i++; i <= 127; i++) if (set_has_char(set, i)) break;
    if (i > 127) break;
    char lower = i;
    set_body = mputc(set_body, lower);
    for (i++; i <= 127; i++) if (!set_has_char(set, i)) break;
    char upper = i - 1;
    if (lower < upper) {
      if (lower + 1 < upper) set_body = mputc(set_body, '-');
      set_body = mputc(set_body, upper);
    }
  }
  return set_body;
}

static char *generate_posix_body(character_set *set)
{
  int has_caret;
  if (set_has_char(set, '^') && !(set_has_char(set, '^' - 1) &&
      set_has_char(set, '^' + 1))) {
    set_remove_char(set, '^');
    has_caret = 1;
  } else has_caret = 0;
  int has_dash;
  if (set_has_char(set, '-') && !(set_has_char(set, '-' - 1) &&
      set_has_char(set, '-' + 1))) {
    set_remove_char(set, '-');
    has_dash = 1;
  } else has_dash = 0;
  int has_rsbrkt;
  if (set_has_char(set, ']') && !(set_has_char(set, ']' - 1) &&
      set_has_char(set, ']' + 1))) {
    set_remove_char(set, ']');
    has_rsbrkt = 1;
  } else has_rsbrkt = 0;
  char *set_body = memptystr();
  if (set_is_empty(set) && !has_rsbrkt) {
    /* the `-' must precede the `^' */
    if (has_dash) set_body = mputc(set_body, '-');
    if (has_caret) set_body = mputc(set_body, '^');
  } else {
    /* order: ']', others, '^', '-' */
    if (has_rsbrkt) set_body = mputc(set_body, ']');
    set_body = append_posix_body(set_body, set);
    if (has_caret) set_body = mputc(set_body, '^');
    if (has_dash) set_body = mputc(set_body, '-');
  }
  return set_body;
}

static char *generate_posix_body_compl(character_set *set)
{
  set_negate(set);
  int has_dash;
  if (set_has_char(set, '-') && !(set_has_char(set, '-' - 1) &&
      set_has_char(set, '-' + 1))) {
    set_remove_char(set, '-');
    has_dash = 1;
  } else has_dash = 0;
  int has_rsbrkt;
  if (set_has_char(set, ']') && !(set_has_char(set, ']' - 1) &&
      set_has_char(set, ']' + 1))) {
    set_remove_char(set, ']');
    has_rsbrkt = 1;
  } else has_rsbrkt = 0;
  char *set_body = mcopystr("^");
  /* order: ']', others, '-' */
  if (has_rsbrkt) set_body = mputc(set_body, ']');
  set_body = append_posix_body(set_body, set);
  if (has_dash) set_body = mputc(set_body, '-');
  return set_body;
}

char *set_generate_posix(const character_set *set)
{
  /* a full set can only be represented in this way: */
  if (set_is_full(set)) return mcopystr(".");
  character_set *tempset = set_copy(set);
  char *set_body = generate_posix_body(tempset);
  set_free(tempset);
  char *posix_str;
  if (set_body[0] == '\0') {
    Free(set_body);
    TTCN_pattern_error("Internal error: empty POSIX set.");
    return NULL;
  }
  /* do not use the set notation in POSIX if the set contains only one
   * character */
  if (set_body[1] == '\0') posix_str = translate_character(set_body[0]);
  else {
    /* create the complemented version of the same set */
    tempset = set_copy(set);
    char *compl_body = generate_posix_body_compl(tempset);
    set_free(tempset);
    if (compl_body[0] == '\0') {
      Free(set_body);
      Free(compl_body);
      TTCN_pattern_error("Internal error: empty complemented POSIX set.");
      return NULL;
    }
    /* use the complemented form in the POSIX equivalent if it is the shorter
     * one */
    if (mstrlen(compl_body) < mstrlen(set_body))
      posix_str = mprintf("[%s]", compl_body);
    else posix_str = mprintf("[%s]", set_body);
    Free(compl_body);
  }
  Free(set_body);
  return posix_str;
}

void yyprint(FILE *file, int type, const YYSTYPE& value)
{
  switch (type) {
  case TOK_Char:
    fprintf(file, "'%c'", value.c);
    break;
  case TOK_Digit: case TOK_Number:
    fprintf(file, "'%lu'", value.u);
    break;
  default:
    break;
  }
}


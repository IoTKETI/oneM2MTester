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
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
 
/**
 *  Based on pattern_p.y
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

#include "Quadruple.hh"
#include "UnicharPattern.hh"

  union YYSTYPE;
/* defined in lexer c-file: */

  extern int pattern_yylex();
  inline int pattern_unilex() { return pattern_yylex(); }
  extern void init_pattern_yylex(YYSTYPE*);
  struct yy_buffer_state;
  extern yy_buffer_state* pattern_yy_scan_string(const char*);
  extern void pattern_yy_delete_buffer(yy_buffer_state*);
  extern unsigned int get_nof_parentheses();

/* defined in this file: */

  /** The converted regexp. */
  static char *ret_val;
  /** The parser error reporting function. */
  static void pattern_unierror(const char *error_str);

  static int user_groups;

  static bool nocase;

#define YYERROR_VERBOSE

static void yyprint(FILE *file, int type, const YYSTYPE& value);
#define YYPRINT(f,t,v) yyprint(f,t,v)

%}

/*********************************************************************
 * Bison declarations
 *********************************************************************/

%name-prefix="pattern_uni"
%output="pattern_uni.cc"
%defines
%verbose
%expect 0
%start Pattern
%debug

/*********************************************************************
 * The union-type
 * Must be kept in sync with the one in pattern_p.y !
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
%type <q> RE_Set_Range_Char RE_Quadruple
%type <s> RE_Body RE_Elems RE_Alter_Elem RE_Concat_Elem
          RE_Multiply_Elem RE_Multiply_Statement RE_Group
          RE_OneCharPos
%type <qset> RE_Set RE_Set_Body RE_Set_Elem RE_Set_NoRange_Char

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

%destructor { delete $$; }
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
| '*' {$$=mcopystr("(........)*");}
;

RE_Multiply_Elem:
  RE_Group {$$=$1;}
| RE_OneCharPos {$$=$1;}
;

RE_Group:
  KW_Group_Begin KW_Group_End
  {
    user_groups++;
    $$ = mcopystr("<)");
  }
| KW_Group_Begin RE_Elems KW_Group_End
  {
    user_groups++;
    if ($2 != NULL) {
      $$ = mprintf("<%s)", $2);
      Free($2);
    } else {
      $$ = mcopystr("<)");
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
  '?' {$$=mcopystr("(........)");}
| KW_BS_d {$$=mcopystr("(AAAAAAD[A-J])");}
| KW_BS_w {$$=mcopystr("(AAAAAAD[A-J]|AAAAAA(E[B-P]|F[A-K])|AAAAAA(G[B-P]|H[A-K]))");}
| KW_BS_t {$$=mcopystr("AAAAAAAJ");}
| KW_BS_n {$$=mcopystr("(AAAAAAA[K-N])");}
| KW_BS_r {$$=mcopystr("AAAAAAAN");}
| KW_BS_s {$$=mcopystr("(AAAAAAA[J-N]|AAAAAACA)");}
| KW_BS_b
  {
    TTCN_pattern_warning("Metacharacter `\\b' is not supported yet.");
    $$ = NULL;
  }
| TOK_Char
  {
    unsigned char c = $1;
    if ($1 <= 0) TTCN_pattern_error("Character with code %u "
      "(0x%02x) cannot be used in a pattern for type universal charstring.", $1, $1);
    $$ = Quad::get_hexrepr(nocase ? tolower(c) : c);
  }
| RE_Quadruple
  {
    $$ = Quad::get_hexrepr(nocase ?
      unichar_pattern.convert_quad_to_lowercase($1.value).get_value() : $1.value);
  }
| RE_Set
  {
    if ($1->is_empty()) {
      TTCN_pattern_error("Empty character set.");
      $$ = NULL;
    } else
      $$ = $1->generate_posix();
    delete $1;
  }
;

RE_Set:
  /* RE_Set_Begin        is 1 for "[^",  0 for "["
   * RE_Set_Begin_Rsbrkt is 1 for "[^]", 0 for "[]"
   * RE_Set_End          is 1 for "-]",  0 for "]"
   */
  RE_Set_Begin RE_Set_Body RE_Set_End
  {
    if ($2 != NULL)
      $$ = $2;
    else
      $$ = new QuadSet();
    if ($3 && !$$->add(new Quad('-')))
        TTCN_pattern_warning("Duplicate character `-' in the character set.");
    if ($1)
      $$->set_negate(true);
  }
| RE_Set_Begin '-' RE_Set_Body RE_Set_End
  {
    if ($3 != NULL)
      $$ = $3;
    else
      $$ = new QuadSet();
    if (!$$->add(new Quad('-')))
      TTCN_pattern_warning("Duplicate character `-' in the character set.");
    if ($1)
      $$->set_negate(true);
  }
| RE_Set_Begin_Rsbrkt RE_Set_Body RE_Set_End
  {
    if ($2 != NULL)
      $$ = $2;
    else
      $$ = new QuadSet();
    if (!$$->add(new Quad(']')))
      TTCN_pattern_warning("Duplicate character `]' in the character set.");
    if ($3 && !$$->add(new Quad('-')))
      TTCN_pattern_warning("Duplicate character `-' in the character set.");
    if ($1)
      $$->set_negate(true);
  }
| RE_Set_Begin_Rsbrkt '-' RE_Set_Range_Char RE_Set_Body RE_Set_End
  {
    if ($4 != NULL)
      $$ = $4;
    else
      $$ = new QuadSet();
    if (static_cast<unsigned int>(']') > $3.value) {
      TTCN_pattern_error("Invalid range in the character set: the "
        "character code of the lower bound (%u) is higher than that of the "
        "upper bound (%u).", ']', static_cast<unsigned int>($3.value));
    }
    $$->add(new QuadInterval(Quad(']'), Quad($3.value)));
    if ($5) {
      if (!$$->add(new Quad('-')))
        TTCN_pattern_warning("Duplicate character `-' in the character set.");
    }
    if ($1)
      $$->set_negate(true);
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
      $$->join($2);
      delete($2);
    } else
      $$ = $2;
  }
;

RE_Set_Elem:
  RE_Set_Range_Char
  {
    $$ = new QuadSet();
    $$->add(new Quad($1.value));
  }
| RE_Set_NoRange_Char { $$ = $1; }
| RE_Set_Range_Char '-' RE_Set_Range_Char
  {
    if ($1.value > $3.value) {
      TTCN_pattern_error("Invalid range in the character set: the "
        "character code of the lower bound (%u) is higher than that of the "
        "upper bound (%u).", static_cast<unsigned int>($1.value), static_cast<unsigned int>($3.value));
    }
    $$ = new QuadSet();
    $$->add(new QuadInterval(Quad($1.value), Quad($3.value)));
  }
;

RE_Set_Range_Char:
  KW_BS_t { $$.value = '\t'; }
| KW_BS_r { $$.value = '\r'; }
| TOK_Char
  {
    if ($1 <= 0) TTCN_pattern_error("Character with code %u "
      "(0x%02x) cannot be used in a pattern for type universal charstring.", $1, $1);
    $$.value = nocase ? tolower($1) : $1;
  }
| RE_Quadruple { $$.value = nocase ?
    unichar_pattern.convert_quad_to_lowercase($1.value).get_value() : $1.value; }
;

RE_Set_NoRange_Char:
  KW_BS_d
  {
    $$ = new QuadSet();
    $$->add(new QuadInterval(Quad('0'), Quad('9')));
  }
| KW_BS_w
  {
    $$ = new QuadSet();
    $$->add(new QuadInterval(Quad('0'), Quad('9')));
    $$->add(new QuadInterval(Quad('A'), Quad('Z')));
    $$->add(new QuadInterval(Quad('a'), Quad('z')));
  }
| KW_BS_n
  {
    $$ = new QuadSet();
    $$->add(new QuadInterval(Quad('\n'), Quad('\r')));
  }
| KW_BS_s
  {
    $$ = new QuadSet();
    $$->add(new QuadInterval(Quad('\t'), Quad('\r')));
    $$->add(new Quad(' '));
  }
| KW_BS_b
  {
    $$ = new QuadSet();
    TTCN_pattern_error("Metacharacter `\\b' does not make any sense in a "
      "character set.");
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
    if ($3 == 0 && $5 == 0 && $7 == 0 && $9 == 0) TTCN_pattern_error("Zero "
      "character (i.e. quadruple `\\q{0,0,0,0}') is not supported in a "
      "pattern for type universal charstring.");
    $$.comp.group = static_cast<unsigned char>($3);
    $$.comp.plane = static_cast<unsigned char>($5);
    $$.comp.row = static_cast<unsigned char>($7);
    $$.comp.cell = static_cast<unsigned char>($9);
  }
;

%%

/*********************************************************************
 * Interface
 *********************************************************************/

char* TTCN_pattern_to_regexp_uni(const char* p_pattern, bool p_nocase, int** groups)
{
  /* if you want to debug */
  //pattern_unidebug=1;

  ret_val=NULL;
  user_groups = 0;
  nocase = p_nocase;

  yy_buffer_state *flex_buffer = pattern_yy_scan_string(p_pattern);
  if(flex_buffer == NULL) {
    TTCN_pattern_error("Flex buffer creation failed.");
    return NULL;
  }
  init_pattern_yylex(&yylval);
  if(pattern_uniparse()) {
    Free(ret_val);
    ret_val=NULL;
  }
  pattern_yy_delete_buffer(flex_buffer);

  // needed by regexp to find user specified groups
  if (user_groups /*&& groups*/) {
    if (groups) {
      *groups = static_cast<int*>(Malloc(sizeof(int) * (user_groups + 1)));
      (*groups)[0] = user_groups;
    }

    int par = -1, index = 1;
    for (size_t i = 0; i < strlen(ret_val); i++) {
      if (ret_val[i] == '(') {
        par++;
      }
      if (ret_val[i] == '<') {
        ret_val[i] = '(';
        par++;
        if (groups) (*groups)[index++] = par;
      }
    }
  } else if (groups)
    *groups = static_cast<int*>(0);

  return ret_val;
}

// Backwards compatibility shim
char* TTCN_pattern_to_regexp_uni(const char* p_pattern, int ere, int** /*groups*/)
{
  TTCN_pattern_warning("TTCN_pattern_to_regexp_uni"
    "(const char* p_pattern, int ere, int** groups) is deprecated");
  if (ere != 1) TTCN_pattern_error(
    "BRE is not supported for TTCN_pattern_to_regexp_uni");
  return TTCN_pattern_to_regexp(p_pattern);
}


/*********************************************************************
 * Static functions
 *********************************************************************/

void pattern_unierror(const char *error_str)
{
  TTCN_pattern_error("%s", error_str);
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


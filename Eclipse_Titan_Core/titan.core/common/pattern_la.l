/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
 
/**
 * Lexical analyzer for TTCN-3 character patterns.
 *
 * \author Matyas Forstner (Matyas.Forstner@eth.ericsson.se)
 *
 * 20031121
 */

%option nostack
%option noyylineno
%option noyywrap
%option nounput
%option never-interactive
%option prefix="pattern_yy"

%{ /* ****************** C declarations ***************** */

#include <ctype.h>
#include <stddef.h>
#include "pattern.hh"
#include "pattern_p.hh"

  /* Access the semantic value of the bison parser. Usually this is done by
   * #defining yylval to the appropriate symbol whose name depends on the
   * %name-prefix of the parser, e.g. pattern_yylval or pattern_unilval.
   *
   * Because we need to be able to access either one or the other,
   * we keep a pointer which is set by the parser when it calls
   * init_pattern_yylex() */
static YYSTYPE  *yylval_ptr;
#define yylval (*yylval_ptr)

static unsigned int nof_parentheses = 0;
static bool meta = false;

%} /* ***************** definitions ***************** */

NUMBER 0|([1-9][0-9]*)

/* start conditions */
%x SC_Set SC_Hash SC_HashParen SC_Quadruple SC_Quadruple_Set

%% /* ***************** rules ************************* */

  /* drop whitespaces */
<SC_Hash,SC_HashParen,SC_Quadruple,SC_Quadruple_Set>[ \t\r\n\v\f]+

<SC_Set>
{

"]" {
  BEGIN(INITIAL);
  return KW_Set_End;
}

"-]" {
  BEGIN(INITIAL);
  return KW_Set_Dash_End;
}

"-" return '-';

} /* SC_Set */

<SC_Hash>
{

[0-9] {
  BEGIN(INITIAL);
  yylval.u = yytext[0] - '0';
  return TOK_Digit;
}

"(" {
  BEGIN(SC_HashParen);
  return '(';
}

} /* SC_Hash */

<SC_HashParen,SC_Quadruple,SC_Quadruple_Set>
{

{NUMBER} {
  errno = 0;
  yylval.u = strtoul(yytext, NULL, 10);
  if (errno != 0) TTCN_pattern_error("Number `%s' is too large to be "
    "represented in memory. (%s)", yytext, strerror(errno));
  return TOK_Number;
}

"," return ',';

} /* SC_HashParen,SC_Quadruple,SC_Quadruple_Set */

<SC_HashParen>")" {
  BEGIN(INITIAL);
  return ')';
}

<SC_Quadruple,SC_Quadruple_Set>
{

"{" return '{';

"}" {
  if (YY_START == SC_Quadruple) BEGIN(INITIAL);
  else BEGIN(SC_Set);
  return '}';
}

} /* SC_Quadruple,SC_Quadruple_Set */

"*" { meta = true; return '*'; }
"+" { meta = true; return '+'; }
"?" { meta = true; return '?'; }
"|" { meta = true; return '|'; }

"(" {
  nof_parentheses++;
  meta = true;
  return KW_Group_Begin;
}

")" {
  if (nof_parentheses > 0) {
    nof_parentheses--;
    return KW_Group_End;
  } else {
    TTCN_pattern_error("Unmatched `)'.");
    yylval.c = ')';
    return TOK_Char;
  }
}

"[" {
  BEGIN(SC_Set);
  meta = true;
  return KW_Set_Begin;
}

"[^" {
  BEGIN(SC_Set);
  meta = true;
  return KW_Set_Begin_Neg;
}

"[]" {
  BEGIN(SC_Set);
  meta = true;
  return KW_Set_Begin_Rsbrkt;
}

"[^]" {
  BEGIN(SC_Set);
  meta = true;
  return KW_Set_Begin_Neg_Rsbrkt;
}

"]" {
  TTCN_pattern_error("Unmatched `]'.");
  yylval.c = ']';
  return TOK_Char;
}

"#" {
  BEGIN(SC_Hash);
  meta = true;
  return '#';
}

<INITIAL,SC_Set>
{

  /* \metacharacters */
"\\d" { meta = true; return KW_BS_d; }
"\\w" { meta = true; return KW_BS_w; }
"\\t" { meta = true; return KW_BS_t; }
"\\n" { meta = true; return KW_BS_n; }
"\\r" { meta = true; return KW_BS_r; }
"\\s" { meta = true; return KW_BS_s; }
"\\b" { meta = true; return KW_BS_b; }

"\\q" {
  meta = true;
  if (YY_START == INITIAL) BEGIN(SC_Quadruple);
  else BEGIN(SC_Quadruple_Set);
  return KW_BS_q;
}

  /* escaped special characters: ? * \ [ ] - ^ | ( ) # + { } */
\\[][?*\\^|()#+{}-] {
  yylval.c = yytext[1];
  return TOK_Char; /* not meta */
}

 /* invalid escape sequences */
"\\"(.|"\n") {
  if (isprint(static_cast<unsigned char>(yytext[1])))
    TTCN_pattern_warning("Use of unrecognized escape sequence `\\%c' is "
      "deprecated.", yytext[1]);
  else TTCN_pattern_warning("Use of unrecognized escape sequence `\\' + "
    "character code %u (0x%02X) is deprecated.", static_cast<unsigned char>(yytext[1]),
    static_cast<unsigned char>(yytext[1]));
  yylval.c = yytext[1];
  return TOK_Char;
}

 /* single backslash (at the end) */
\\ {
  TTCN_pattern_error("Invalid single backslash (`\\') character at the end "
    "of the pattern.");
}

.|"\n" {
  yylval.c = yytext[0];
  return TOK_Char;
}

} /* INITIAL, SC_Set */

 /* erroneous characters */

<SC_Hash>.|\n {
  if (isprint(static_cast<unsigned char>(yytext[0])))
    TTCN_pattern_error("A digit or `(' was expected after `#' instead of "
      "character `%c'.", yytext[0]);
  else TTCN_pattern_error("A digit or `(' was expected after `#' instead of "
    "character with code %u (0x%02X).", static_cast<unsigned char>(yytext[0]),
    static_cast<unsigned char>(yytext[0]));
}

<SC_HashParen>. {
  if (isprint(static_cast<unsigned char>(yytext[0])))
    TTCN_pattern_error("A number, `,' or `)' was expected after `#(' instead "
      "of character `%c'.", yytext[0]);
  else TTCN_pattern_error("A number, `,' or `)' was expected after `#(' "
    "instead of character with code %u (0x%02X).", static_cast<unsigned char>(yytext[0]),
    static_cast<unsigned char>(yytext[0]));
}

<SC_Quadruple,SC_Quadruple_Set>. {
  if (isprint(static_cast<unsigned char>(yytext[0])))
    TTCN_pattern_error("A number, `,' or `}' was expected after `\\q{' "
      "instead of character `%c'.", yytext[0]);
  else TTCN_pattern_error("A number, `,' or `}' was expected after `\\q{' "
    "instead of character with code %u (0x%02X).", static_cast<unsigned char>(yytext[0]),
    static_cast<unsigned char>(yytext[0]));
}

%%

unsigned int get_nof_parentheses()
{
  return nof_parentheses;
}

bool has_meta()
{
  return meta;
}

void init_pattern_yylex(YYSTYPE *sema_val)
{
  BEGIN(INITIAL);
  yylval_ptr = sema_val;
  nof_parentheses = 0;
  meta = false;
}

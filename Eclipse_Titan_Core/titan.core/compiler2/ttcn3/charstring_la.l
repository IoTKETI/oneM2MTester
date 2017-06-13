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
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput
%option prefix="charstring_"

%{
// Scanner to transform string literals from external representation
// to internal representation.

#include "../../common/dbgnew.hh"
#include "../error.h"
#include "../string.hh"
#include "../Setting.hh"
#include "../CompilerError.hh"

using namespace Common;

#define INITIAL_BUFFER_SIZE 32

static void add_char(char*& string_ptr, size_t& string_len,
  size_t& string_size, char c)
{
  if (string_size <= string_len) {
    string_size *= 2;
    string_ptr = (char*)Realloc(string_ptr, string_size);
  }
  string_ptr[string_len++] = c;
}

#define ADD_CHAR(c) add_char(string_ptr, string_len, string_size, c)

#define YY_DECL static string *yylex(const char *current_file, \
  int current_line, int current_column)

%}

NEWLINE \r|\n|\r\n
HEXDIGIT [0-9A-Fa-f]

%%
  char *string_ptr = (char*)Malloc(INITIAL_BUFFER_SIZE);
  size_t string_len = 0, string_size = INITIAL_BUFFER_SIZE;

  /* Two consecutive doublequotes -> one doublequotequote */
["]["]    ADD_CHAR('"'); current_column += 2;

  /* Backslash-escaped singlequote, doublequote, question mark or backslash */
\\[''""?\\]    ADD_CHAR(yytext[1]); current_column += 2;

  /* C-style backslash-escapes */
\\a     ADD_CHAR('\a'); current_column += 2;
\\b     ADD_CHAR('\b'); current_column += 2;
\\f     ADD_CHAR('\f'); current_column += 2;
\\n     ADD_CHAR('\n'); current_column += 2;
\\r     ADD_CHAR('\r'); current_column += 2;
\\t     ADD_CHAR('\t'); current_column += 2;
\\v     ADD_CHAR('\v'); current_column += 2;

\\[0-7]{1,3}    {
  /* octal notation */
  if (yyleng == 4 && yytext[1] > '3') {
    Location loc(current_file, current_line, current_column, current_line,
      current_column + 4);
    loc.error("Invalid octal character code: `%s'", yytext);
    ADD_CHAR('\377');
  } else {
    char c = yytext[1] - '0';
    if (yyleng >= 3) c = c * 8 + yytext[2] - '0';
    if (yyleng == 4) c = c * 8 + yytext[3] - '0';
    ADD_CHAR(c);
  }
  current_column += yyleng;
}

\\x{HEXDIGIT}{1,2}  {
  /* hexadecimal notation */
  char c;
  if (yytext[2] >= 'A' && yytext[2] <= 'F') c = yytext[2] - 'A' + 10;
  else if (yytext[2] >= 'a' && yytext[2] <= 'f') c = yytext[2] - 'a' + 10;
  else c = yytext[2] - '0';
  if (yyleng == 4) {
    if (yytext[3] >= 'A' && yytext[3] <= 'F')
      c = c * 16 + yytext[3] - 'A' + 10;
    else if (yytext[3] >= 'a' && yytext[3] <= 'f')
      c = c * 16 + yytext[3] - 'a' + 10;
    else c = c * 16 + yytext[3] - '0';
  }
  ADD_CHAR(c);
  current_column += yyleng;
}

\\{NEWLINE}    {
  /* escaped newline character */
  current_line++;
  current_column = 0;
}

\\[][dsw()|^*#+-]  { /* '-' is non-special as last character, ']' as first */
  /* temporary hack: these escape sequences are handled transparently
     as they can appear in the second argument of regexp().
     In effect, they are re-escaped (both backslash and the next char survive),
     saving the user from having to double-escape them.
     Unfortunately, we don't know yet whether this string is being used
     as an argument to regexp(), so we can't suppress the warning. */
  Location loc(current_file, current_line, current_column, current_line,
    current_column + yyleng);
  loc.warning("Unknown escape sequence `%s' was treated literally", yytext);
  for (size_t i = 0; i < (size_t)yyleng; i++) ADD_CHAR(yytext[i]);
  current_column += 2;
}

\\(x[^\\""]|.)  {
  Location loc(current_file, current_line, current_column, current_line,
    current_column + yyleng);
  loc.error("Invalid escape sequence: `%s'", yytext);
  current_column += yyleng;
}

{NEWLINE}   {
  Location loc(current_file, current_line, current_column, current_line + 1, 0);
  loc.warning("Unescaped newline character");
  for (size_t i = 0; i < (size_t)yyleng; i++) ADD_CHAR(yytext[i]);
  current_line++;
  current_column = 0;
}

[""\\]  FATAL_ERROR("charstring_lex(): invalid single `%s'", yytext);

.       ADD_CHAR(yytext[0]); current_column++;

<<EOF>> {
  string *ret_val = new string(string_len, string_ptr);
  Free(string_ptr);
  return ret_val;
}

%%

string *parse_charstring_value(const char *str, const Location& loc)
{
  Error_Context cntxt(&loc, "In charstring value");
  struct yy_buffer_state *flex_buffer = charstring__scan_string(str);
  if (!flex_buffer) {
    FATAL_ERROR("parse_charstring_value(): flex buffer creation failed");
    return 0;
  }
  string *ret_val = yylex(loc.get_filename(), loc.get_first_line(),
    loc.get_first_column() + 1);
  charstring_lex_destroy();
  return ret_val;
}

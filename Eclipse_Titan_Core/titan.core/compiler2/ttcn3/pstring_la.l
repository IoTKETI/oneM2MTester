/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Forstner, Matyas
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option nostack
%option noyylineno
%option noyywrap
%option nounput
%option never-interactive
%option prefix="pstring_yy"

%{ /* ****************** C declarations ***************** */

#include <stdio.h>
#include <ctype.h>
#include "../error.h"
#include "../Identifier.hh"
#include "../Int.hh"
#include "PatternString.hh"

using namespace Ttcn;
using namespace Common;

/** Adjust location information.
 *
 * \c start_index and \c end_index index into \c yytext.
 * \c current_line and \c current_column index into the actual source line
 * (only a part of which is in \c yytext).
 *
 * UPDATE_LOCATION should be called to "cover" all of yytext.
 *
 * @param [in] start_index index into yytext (the first character to consider)
 * @param [in] end_index   index into yytext (one-past)
 * @param [in,out] current_line
 * @param [in,out] current_column
 * */
static void update_location(size_t start_index, size_t end_index,
  size_t& current_line, size_t& current_column)
{
  for (size_t i = start_index; i < (size_t)yyleng && i < end_index; i++) {
    // count CR, count LF, but count CR + LF as one
    switch (yytext[i]) {
    case '\r':
      current_line++;
      current_column = 0;
      break;
    case '\n':
      if (i == 0 || yytext[i - 1] != '\r') {
	current_line++;
	current_column = 0;
      }
      break;
    default:
      current_column++;
    }
  }
}

// This must be side-effect-free !
#define UPDATE_LOCATION(start_index, end_index) \
  update_location(start_index, end_index, current_line, current_column)

#define YY_DECL static PatternString *yylex(const char *current_file, \
  size_t current_line, size_t current_column)

%} /* ***************** definitions ***************** */

IDENTIFIER [A-Za-z][A-Za-z0-9_]*
NUMBER 0|([1-9][0-9]*)
WS [ \t\r\n\v\f]*
NEWLINE \r|\n|\r\n
UID [uU][+]?[0-9A-Fa-f]{1,8} 

%% /* ***************** rules ************************* */

  bool in_set = false; /* inside a [] */
  PatternString* ps = new PatternString();

"{"{WS}{IDENTIFIER}(({WS}\.{WS}{IDENTIFIER})|({WS}\[{WS}({IDENTIFIER}|{NUMBER}){WS}\]))*{WS}"}" {
if (in_set) {
  // in a set a reference (without the \N at start) is just simple text,
  // the matched token does not contain characters that are special in a set
  ps->addString(yytext);
  UPDATE_LOCATION(0, yyleng);
} else {
  vector<string> identifiers;
  vector<int>    beginnings;

  const char * beg = yytext;
  const char * end = 0;
  int id_begin = 0; // the first alphanumeric character of an identifier
  for (;;) {
    while (isspace(*++beg) || (*beg=='[') ) ; // skip whitespace and [
    int *current_begin = new int(beg - yytext);
    if (!id_begin) id_begin = *current_begin;
    end = beg;
    while (isalnum(*++end) || *end=='_') ; // scan for the end of identifier

    string* identifier = new string(end-beg, beg);
    identifiers.add(identifier);
    beginnings .add(current_begin);

    beg = end; // end remembers the position before the whitespace
    while (isspace(*beg) || (*beg==']') ) ++beg; // skip whitespace and ]
    if (*beg=='}') break;
  }

  size_t num_id = identifiers.size();
  bool error = false;
  Ttcn::Reference *ref = 0;
  size_t last = 0; // last "consumed" index into yytext
  int old_column = 0, old_line = 0; // the beginning of the entire reference

  for (size_t i = 0; i < num_id; ++i) {
    const string & id_str = *identifiers[i];
    const size_t    & id_beg = *beginnings [i];
    if (Identifier::is_reserved_word(id_str, Identifier::ID_TTCN)) {
      UPDATE_LOCATION(last, id_beg); // consume before identifier
      int first_line = current_line, first_column = current_column;
      UPDATE_LOCATION(id_beg, id_beg+id_str.size()); // consume identifier
      Location loc(current_file, first_line, first_column, current_line,
        current_column); // location covers the identifier
      UPDATE_LOCATION(id_beg+id_str.size(), yyleng); // consume to end
      loc.error("Invalid reference expression: `%s' is a reserved word in TTCN-3",
        id_str.c_str());
      error = true;
      break;
    } else if (in_set) {
      UPDATE_LOCATION(last, id_beg); // consume before identifier
      break;
    } else {
      if (i==0) {
	UPDATE_LOCATION(0, id_begin); // consume before identifier
	old_column = current_column; old_line = current_line;
	ref = new Ttcn::Reference(new Identifier(Identifier::ID_TTCN, id_str));
	UPDATE_LOCATION(id_begin, last = id_begin + id_str.size());
      } else {
	UPDATE_LOCATION(last, id_beg);
	Location loc(current_file, current_line, current_column, current_line,
	  current_column + id_str.size());
	UPDATE_LOCATION(id_beg, last = id_beg + id_str.size());

	int temp_i;
	FieldOrArrayRef *fieldref = 0;

	if(1 != sscanf( (id_str.c_str()), "%d", &temp_i)){
	  fieldref = new FieldOrArrayRef(new Identifier(Identifier::ID_TTCN, id_str));
	}
	else {
	  int_val_t* temp_intvalt = new int_val_t(temp_i);
	  Value* temp_value = new Value(Value::V_INT, temp_intvalt);
	  fieldref = new FieldOrArrayRef(temp_value);
	}
  if (ref) {
	ref->set_location(loc);
	ref->add(fieldref);
  } else {
    loc.error("Invalid reference expression");
  }
      }
    }
  } // next i

  if (error) {
    delete ref; // safe even if NULL
  } else {
    UPDATE_LOCATION(last, yyleng); // consume to the end
    Location loc(current_file, old_line, old_column, current_line, current_column);
    if (ref) {
      ref->set_location(loc);
      ps->addRef(ref, FALSE);
    } else {
      loc.error("Invalid reference expression");
    }
  }

  // cleanup (can't be done in the second loop because it may end early)
  for (size_t i = 0; i < num_id; ++i) {
    delete identifiers[i];
    delete beginnings [i];
  }
  identifiers.clear();
  beginnings .clear();
} //else
}

"{"[^}]*"}" {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  loc.error("Invalid reference expression: `%s'", yytext);
}

"\\N"{WS}"{"{WS}{IDENTIFIER}{WS}"}" {
  int id_begin = 3;
  while (!isalpha(yytext[id_begin])) id_begin++;
  int id_len = 1;
  while (isalnum(yytext[id_begin + id_len]) || yytext[id_begin + id_len] == '_')
    id_len++;
  string id_str(id_len, yytext + id_begin);
  
  Ttcn::Reference *ref = new Ttcn::Reference(new Identifier(
    Identifier::ID_TTCN, id_str));
  ps->addRef(ref, TRUE);
  
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  ref->set_location(loc);
  if (Identifier::is_reserved_word(id_str, Identifier::ID_TTCN)) {
    loc.error("Invalid character set reference: `%s' is a reserved word in "
      "TTCN-3", id_str.c_str());
  } else if (in_set) {
    loc.warning("Character set reference `\\N{%s}' is not supported, "
      "dropped out from the set", id_str.c_str());
  }
}


"\\N"{WS}"{"{WS}"universal"{WS}"charstring"{WS}"}" {
  /** The third {WS} is optional but if it's empty then the previous rule catches it**/
  string id_str("universal charstring");
  
  Ttcn::Reference *ref = new Ttcn::Reference(new Identifier(
    Identifier::ID_TTCN, id_str));
  ps->addRef(ref, TRUE);
  
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  ref->set_location(loc);
  if (in_set) {
    loc.warning("Character set reference `\\N{%s}' is not supported, "
      "dropped out from the set", id_str.c_str());
  }
}

"\\N"{WS}"{"[^}]*"}" {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  loc.error("Invalid character set reference: `%s'", yytext);
}

"\\q"{WS}"{"{WS}{NUMBER}{WS}","{WS}{NUMBER}{WS}","{WS}{NUMBER}{WS}","{WS}{NUMBER}{WS}"}" {
  size_t group_begin = 3;
  while (!isdigit(yytext[group_begin])) group_begin++;
  UPDATE_LOCATION(0, group_begin);
  size_t group_len = 1;
  while (isdigit(yytext[group_begin + group_len])) group_len++;
  string group_str(group_len, yytext + group_begin);
  Location group_loc(current_file, current_line, current_column, current_line,
    current_column + group_len);
  Int group = string2Int(group_str, group_loc);
  if (group < 0 || group > 127) {
    group_loc.error("The first number of quadruple (group) must be within "
      "the range 0 .. 127 instead of %s", Int2string(group).c_str());
    group = group < 0 ? 0 : 127;
  }
  size_t plane_begin = group_begin + group_len + 1;
  while (!isdigit(yytext[plane_begin])) plane_begin++;
  UPDATE_LOCATION(group_begin, plane_begin);
  int plane_len = 1;
  while (isdigit(yytext[plane_begin + plane_len])) plane_len++;
  string plane_str(plane_len, yytext + plane_begin);
  Location plane_loc(current_file, current_line, current_column, current_line,
    current_column + plane_len);
  Int plane = string2Int(plane_str, plane_loc);
  if (plane < 0 || plane > 255) {
    plane_loc.error("The second number of quadruple (plane) must be within "
      "the range 0 .. 255 instead of %s", Int2string(plane).c_str());
    plane = plane < 0 ? 0 : 255;
  }
  size_t row_begin = plane_begin + plane_len + 1;
  while (!isdigit(yytext[row_begin])) row_begin++;
  UPDATE_LOCATION(plane_begin, row_begin);
  size_t row_len = 1;
  while (isdigit(yytext[row_begin + row_len])) row_len++;
  string row_str(row_len, yytext + row_begin);
  Location row_loc(current_file, current_line, current_column, current_line,
    current_column + row_len);
  Int row = string2Int(row_str, row_loc);
  if (row < 0 || row > 255) {
    row_loc.error("The third number of quadruple (row) must be within "
      "the range 0 .. 255 instead of %s", Int2string(row).c_str());
    row = row < 0 ? 0 : 255;
  }
  size_t cell_begin = row_begin + row_len + 1;
  while (!isdigit(yytext[cell_begin])) cell_begin++;
  UPDATE_LOCATION(row_begin, cell_begin);
  size_t cell_len = 1;
  while (isdigit(yytext[cell_begin + cell_len])) cell_len++;
  string cell_str(cell_len, yytext + cell_begin);
  Location cell_loc(current_file, current_line, current_column, current_line,
    current_column + cell_len);
  Int cell = string2Int(cell_str, cell_loc);
  if (cell < 0 || cell > 255) {
    cell_loc.error("The fourth number of quadruple (cell) must be within "
      "the range 0 .. 255 instead of %s", Int2string(cell).c_str());
    cell = cell < 0 ? 0 : 255;
  }
  bool add_quadruple = true;
  if (group == 0 && plane == 0 && row == 0) {
    if (isprint(cell)) {
      switch (cell) {
      case '-':
      case '^':
	if (!in_set) break;
      case '?':
      case '*':
      case '\\':
      case '[':
      case ']':
      case '{':
      case '}':
      case '"':
      case '|':
      case '(':
      case ')':
      case '#':
      case '+':
	ps->addChar('\\');
      default:
	break;
      }
      ps->addChar(cell);
      add_quadruple = false;
    } else {
      switch (cell) {
      case '\t':
        ps->addString("\\t");
	add_quadruple = false;
	break;
      case '\r':
        ps->addString("\\r");
	add_quadruple = false;
      }
    }
  }
  if (add_quadruple) {
    ps->addString("\\q{" + Int2string(group) + "," + Int2string(plane) + "," +
      Int2string(row) + "," + Int2string(cell) + "}");
  }
  UPDATE_LOCATION(cell_begin, yyleng);
}

"\\q"({WS}"{"{WS}({UID}{WS}","{WS})*{UID}{WS}"}") {
  //Split UID-s. For example: \q{ U23423 , U+001 } -> [U23423, U+001]
  size_t begin = 3;
  size_t size = 0;
  char ** uids = (char **)Malloc(sizeof(char*));
  while(yytext[begin] != '}'){
    //Find first digit
    while(yytext[begin] != 'U' && yytext[begin] != 'u') begin++;
    size_t end = begin + 2;
    //Find last digit
    while(isxdigit(yytext[end])) end++;
    size++;
    uids = (char**)Realloc(uids, size * sizeof(char*));
    uids[size-1] = mcopystrn(yytext + begin, end-begin);
    //Skip whitespaces until the next UID or the end
    while(!isxdigit(yytext[end]) && yytext[end] != 'U' && yytext[end] != 'u' && yytext[end] != '}') end++;
    UPDATE_LOCATION(begin, end);
    begin = end;
  }
  ps->addStringUSI(uids, size);
  //Free
  for (size_t i = 0; i < size; ++i) {
    Free(uids[i]);
  }
  Free(uids);
}

"\\q"({WS}"{"[^}]*"}")? {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  loc.error("Invalid quadruple or UID-like notation: `%s'", yytext);
}

"[]" {
  if(in_set) {
    ps->addString("\\[]");
    in_set = false;
  } else {
    ps->addString("[\\]");
    in_set = true;
  }
  current_column += 2;
}

"[^]" {
  if(in_set) {
    ps->addString("\\[\\^]");
    in_set = false;
  } else {
    ps->addString("[^\\]");
    in_set = true;
  }
  current_column += 3;
}

"[" {
  if(in_set) {
    ps->addString("\\[");
  } else {
    ps->addChar('[');
    in_set = true;
  }
  current_column++;
}

"]" {
  if (in_set) {
    ps->addChar(']');
    in_set = false;
  } else {
    Location loc(current_file, current_line, current_column, current_line,
      current_column + 1);
    loc.error("Unmatched `]'. Did you mean `\\]'?");
    ps->addString("\\]");
  }
  current_column++;
}

"{"|"}" {
  Location loc(current_file, current_line, current_column, current_line,
    current_column + 1);
  loc.warning("Unmatched `%c' was treated literally", yytext[0]);
  ps->addChar('\\');
  ps->addChar(yytext[0]);
  current_column++;
}

"\\\""|"\"\"" {
  ps->addChar('"');
  current_column += 2;
}
  /* \metachars and escaped metachars */
\\[dwtnrsb?*\\\[\]\-\^|()#+] {
  ps->addString(yytext);
  current_column += 2;
}

"\\"(.|{NEWLINE}) {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  if (isprint((unsigned char)yytext[1]))
    loc.warning("Use of unrecognized escape sequence `\\%c' is deprecated",
      yytext[1]);
  else loc.warning("Use of unrecognized escape sequence is deprecated");
  ps->addString(yytext + 1);
}

"#"{WS}[0-9] {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  if (in_set) {
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#n' cannot be given inside a set "
      "expression");
  } else if (yytext[yyleng - 1] != '1') {
    ps->addChar('#');
    ps->addChar(yytext[yyleng - 1]);
  }
}

"#"{WS}"("{WS}{NUMBER}{WS}")" {
  if (in_set) {
    int first_line = current_line, first_column = current_column;
    UPDATE_LOCATION(0, yyleng);
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#(n)' cannot be given inside a set "
      "expression");
  } else {
    size_t number_begin = 2;
    while (!isdigit(yytext[number_begin])) number_begin++;
    UPDATE_LOCATION(0, number_begin);
    int number_len = 1;
    while (isdigit(yytext[number_begin + number_len])) number_len++;
    string number_str(number_len, yytext + number_begin);
    Location number_loc(current_file, current_line, current_column,
      current_line, current_column + number_len);
    UPDATE_LOCATION(number_begin, yyleng);
    Int number = string2Int(number_str, number_loc);
    if (number < 0) {
      number_loc.error("A non-negative integer value was expected as the "
        "number of repetitions instead of %s", Int2string(number).c_str());
    } else if (number != 1) ps->addString("#(" + Int2string(number) + ")");
  }
}

"#"{WS}"("{WS}{NUMBER}{WS}","{WS}{NUMBER}{WS}")" {
  int first_line = current_line, first_column = current_column;
  if (in_set) {
    UPDATE_LOCATION(0, yyleng);
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#(n,m)' cannot be given inside a set "
      "expression");
  } else {
    size_t lower_begin = 2;
    while (!isdigit(yytext[lower_begin])) lower_begin++;
    UPDATE_LOCATION(0, lower_begin);
    int lower_len = 1;
    while (isdigit(yytext[lower_begin + lower_len])) lower_len++;
    string lower_str(lower_len, yytext + lower_begin);
    Location lower_loc(current_file, current_line, current_column,
      current_line, current_column + lower_len);
    Int lower = string2Int(lower_str, lower_loc);
    if (lower < 0) {
      lower_loc.error("A non-negative integer value was expected as the "
        "minimum number of repetitions instead of %s",
	Int2string(lower).c_str());
      lower = 0;
    }
    size_t upper_begin = lower_begin + lower_len + 1;
    while (!isdigit(yytext[upper_begin])) upper_begin++;
    UPDATE_LOCATION(lower_begin, upper_begin);
    int upper_len = 1;
    while (isdigit(yytext[upper_begin + upper_len])) upper_len++;
    string upper_str(upper_len, yytext + upper_begin);
    Location upper_loc(current_file, current_line, current_column,
      current_line, current_column + upper_len);
    UPDATE_LOCATION(upper_begin, yyleng);
    Int upper = string2Int(upper_str, upper_loc);
    if (upper < 0) {
      upper_loc.error("A non-negative integer value was expected as the "
        "maximum number of repetitions instead of %s",
	Int2string(upper).c_str());
    } else if (lower > upper) {
      Location loc(current_file, first_line, first_column, current_line,
	current_column);
      loc.error("The lower bound is higher than the upper bound in the number "
        "of repetitions: `#(%s,%s)'", Int2string(lower).c_str(),
	Int2string(upper).c_str());
    } else if (lower == upper) {
      if (lower != 1) ps->addString("#(" + Int2string(lower) + ")");
    } else {
      if (lower == 0) ps->addString("#(," + Int2string(upper) + ")");
      else ps->addString("#(" + Int2string(lower) + "," + Int2string(upper) +
	")");
    }
  }
}

"#"{WS}"("{WS}{NUMBER}{WS}","{WS}")" {
  if (in_set) {
    int first_line = current_line, first_column = current_column;
    UPDATE_LOCATION(0, yyleng);
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#(n,)' cannot be given inside a set "
      "expression");
  } else {
    size_t lower_begin = 2;
    while (!isdigit(yytext[lower_begin])) lower_begin++;
    UPDATE_LOCATION(0, lower_begin);
    int lower_len = 1;
    while (isdigit(yytext[lower_begin + lower_len])) lower_len++;
    string lower_str(lower_len, yytext + lower_begin);
    Location lower_loc(current_file, current_line, current_column,
      current_line, current_column + lower_len);
    UPDATE_LOCATION(lower_begin, yyleng);
    Int lower = string2Int(lower_str, lower_loc);
    if (lower < 0) {
      lower_loc.error("A non-negative integer value was expected as the "
        "minimum number of repetitions instead of %s",
	Int2string(lower).c_str());
    } else if (lower == 1) ps->addChar('+');
    else ps->addString("#(" + Int2string(lower) + ",)");
  }
}

"#"{WS}"("{WS}","{WS}{NUMBER}{WS}")" {
  if (in_set) {
    int first_line = current_line, first_column = current_column;
    UPDATE_LOCATION(0, yyleng);
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#(,m)' cannot be given inside a set "
      "expression");
  } else {
    size_t upper_begin = 3;
    while (!isdigit(yytext[upper_begin])) upper_begin++;
    UPDATE_LOCATION(0, upper_begin);
    int upper_len = 1;
    while (isdigit(yytext[upper_begin + upper_len])) upper_len++;
    string upper_str(upper_len, yytext + upper_begin);
    Location upper_loc(current_file, current_line, current_column,
      current_line, current_column + upper_len);
    UPDATE_LOCATION(upper_begin, yyleng);
    Int upper = string2Int(upper_str, upper_loc);
    if (upper < 0) {
      upper_loc.error("A non-negative integer value was expected as the "
        "maximum number of repetitions instead of %s",
	Int2string(upper).c_str());
    } else ps->addString("#(," + Int2string(upper) + ")");
  }
}

"#"{WS}"("{WS}","{WS}")" {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  if (in_set) {
    Location loc(current_file, first_line, first_column, current_line,
      current_column);
    loc.error("Number of repetitions `#(,)' cannot be given inside a set "
      "expression");
  } else ps->addString("#(,)");
}

"#"{WS}"("[^)]*")" {
  int first_line = current_line, first_column = current_column;
  UPDATE_LOCATION(0, yyleng);
  Location loc(current_file, first_line, first_column, current_line,
    current_column);
  loc.error("Invalid notation for the number of repetitions: `%s'", yytext);
}

"#" {
  Location loc(current_file, current_line, current_column, current_line,
    current_column + 1);
  if (in_set) {
    loc.warning("Unescaped `#' inside character set was treated literally");
    ps->addChar('\\');
    ps->addChar('#');
  } else {
    loc.error("Syntax error in the number of repetitions `#...'");
  }
  current_column++;
}

"+" {
  if (in_set) {
    Location loc(current_file, current_line, current_column, current_line,
      current_column + 1);
    loc.warning("Unescaped `+' inside character set was treated literally");
    ps->addChar('\\');
  }
  ps->addChar('+');
  current_column++;
}

.|{NEWLINE} {
  ps->addString(yytext);
  UPDATE_LOCATION(0, yyleng);
}

<<EOF>> {
  if (in_set) {
    Location loc(current_file, current_line, current_column, current_line,
      current_column + 1);
    loc.error("Missing `]' at the end of the string");
    ps->addChar(']');
  }
  return ps;
}

%%

PatternString* parse_pattern(const char *str, const Location& p_loc)
{
  Error_Context cntxt(&p_loc, "In character string pattern");
  struct yy_buffer_state *flex_buffer=pstring_yy_scan_string(str);
  if (!flex_buffer) {
    FATAL_ERROR("parse_pattern(): flex buffer creation failed");
    return 0;
  }

  PatternString *ps = yylex(p_loc.get_filename(), p_loc.get_first_line(),
    p_loc.get_first_column() + 1);
  pstring_yylex_destroy();

  return ps;
}

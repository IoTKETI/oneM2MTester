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
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput
%option prefix="comptype_attrib_"

%{
#include <ctype.h>
#include "../Identifier.hh"
#include "../Setting.hh"
#include "../CompilerError.hh"
#include "Attributes.hh"
#include "AST_ttcn3.hh"
#include "../CompType.hh"

extern void parseExtendsCompTypeRefList(Ttcn::AttributeSpec *attrib,
  Common::CompTypeRefList*& attr_comp_refs);

/** states of the extension attribute parser */
enum extension_attr_parser_state_t {
  INITIAL_STATE, /**< state at the beginning, valid on return */
  EXTENDS_STATE, /**< INITIAL_STATE + "extends" or REFID[2]_STATE + "," ,
                      invalid on return */
  REFID_STATE,   /**< EXTENDS_STATE + IDENTIFIER , valid on return */
  IDDOT_STATE,   /**< REFID_STATE + "." , invalid on return */
  REFID2_STATE   /**< IDDOT_STATE + IDENTIFIER , valid on return */
};

#define YY_DECL static int yylex(Common::CompTypeRefList*& attr_comp_refs, \
int current_line, int current_column, const char *current_file)

#define FATAL_ERROR_INVALID_STATE \
  FATAL_ERROR("comptype_attr.l: invalid parser state");

#define ERROR_UNEXPECTED_EOF \
do { \
  Common::Location loc(current_file, current_line, current_column, \
                       current_line, current_column+1); \
  loc.error("Unexpected end of extension attribute"); \
} while (0)

#define ERROR_UNEXPECTED_TOKEN \
do { \
  Common::Location loc(current_file, current_line, current_column, \
    current_line, current_column+yyleng); \
  loc.error("Syntax error in extension attribute, unexpected token `%s'", \
    yytext); \
} while (0)

#define ADD_REFID \
do { \
  if (!id) FATAL_ERROR("ADD_REFID"); \
  Ttcn::Reference *ref = new Ttcn::Reference(id); \
  ref->set_location(current_file, id_loc); \
  if (!attr_comp_refs) attr_comp_refs = new Common::CompTypeRefList(); \
  attr_comp_refs->add_ref(ref); \
  id = 0; \
} while (0)

/** return from yylex with cleanup */
#define YYLEX_RETURN \
do { \
  if (id) ADD_REFID; \
  return EOF; \
} while (0)

%}

WHITESPACE [ \t\v\f]
NEWLINE \r|\n|\r\n
IDENTIFIER [A-Za-z][A-Za-z0-9_]*
LINECOMMENT "//"[^\r\n]*{NEWLINE}

%x SC_blockcomment

%%
  Common::Identifier *id=0;
  YYLTYPE id_loc;
  int start_line = 0;
  int start_column = 0;
  extension_attr_parser_state_t parser_state = INITIAL_STATE;
  BEGIN(INITIAL);

"/*" {
  start_line = current_line;
  start_column = current_column;
  current_column+=yyleng;
  BEGIN(SC_blockcomment);
}

<SC_blockcomment>
{
"*/" { current_column+=yyleng; BEGIN(INITIAL); }
{NEWLINE} { current_line++; current_column=0; }
. current_column++;
<<EOF>> {
  Common::Location loc(current_file, start_line, start_column,
                       current_line, current_column);
  loc.error("Unterminated block comment in extension attribute");
  YYLEX_RETURN;
}
}

{LINECOMMENT} { current_line++; current_column=0; }
{WHITESPACE} current_column+=yyleng;
{NEWLINE} { current_line++; current_column=0; }

"extends" {
  switch (parser_state) {
  case INITIAL_STATE:
    parser_state = EXTENDS_STATE;
    break;
  case EXTENDS_STATE:
  case REFID_STATE:
  case IDDOT_STATE:
  case REFID2_STATE:
    ERROR_UNEXPECTED_TOKEN;
    break;
  default:
    FATAL_ERROR_INVALID_STATE;
  }
  current_column+=yyleng;
}

{IDENTIFIER} {
  switch (parser_state) {
  case INITIAL_STATE:
    YYLEX_RETURN;
  case EXTENDS_STATE:
    id = new Common::Identifier(Common::Identifier::ID_TTCN, string(yytext));
    id_loc.first_line = id_loc.last_line = current_line;
    id_loc.first_column = current_column;
    id_loc.last_column = current_column + yyleng;
    parser_state = REFID_STATE;
    break;
  case IDDOT_STATE: {
    Common::Identifier *id2 =
      new Common::Identifier(Common::Identifier::ID_TTCN, string(yytext));
    if (!id || !id2) FATAL_ERROR("ADD_REFID2");
    Ttcn::Reference *ref = new Ttcn::Reference(id, id2);
    ref->set_location(current_file, id_loc.first_line, id_loc.first_column,
                      current_line, current_column+yyleng);
    if (!attr_comp_refs) attr_comp_refs = new Common::CompTypeRefList();
    attr_comp_refs->add_ref(ref);
    id = 0;
    parser_state = REFID2_STATE;
    }
    break;
  case REFID_STATE:
  case REFID2_STATE:
    ERROR_UNEXPECTED_TOKEN;
    break;
  default:
    FATAL_ERROR_INVALID_STATE;
  }
  current_column+=yyleng;
}

\. {
  switch (parser_state) {
  case INITIAL_STATE:
    YYLEX_RETURN;
    break;
  case REFID_STATE:
    parser_state = IDDOT_STATE;
    break;
  case EXTENDS_STATE:
  case IDDOT_STATE:
  case REFID2_STATE:
    ERROR_UNEXPECTED_TOKEN;
    break;
  default:
    FATAL_ERROR_INVALID_STATE;
  }
  current_column+=yyleng;
}

"," {
  switch (parser_state) {
  case INITIAL_STATE:
    YYLEX_RETURN;
    break;
  case REFID_STATE:
    ADD_REFID;
  case REFID2_STATE:
    parser_state = EXTENDS_STATE;
    break;
  case EXTENDS_STATE:
  case IDDOT_STATE:
    ERROR_UNEXPECTED_TOKEN;
    break;
  default:
    FATAL_ERROR_INVALID_STATE;
  }
  current_column+=yyleng;
}

. {
  int c = (unsigned char)yytext[0];
  Common::Location loc(current_file, current_line, current_column,
                       current_line, current_column+yyleng);
  loc.error("Invalid character `%c' (0x%02X) in extension attribute",
            isprint(c) ? c : '?', c);
  current_column+=yyleng;
}

<<EOF>> {
  switch (parser_state) {
  /* valid return states */
  case INITIAL_STATE:
  case REFID_STATE:
  case REFID2_STATE:
    break;
  /* invalid return states */
  case EXTENDS_STATE:
  case IDDOT_STATE:
    ERROR_UNEXPECTED_EOF;
    break;
  default:
    FATAL_ERROR_INVALID_STATE;
  }
  YYLEX_RETURN;
}

%%

void parseExtendsCompTypeRefList(Ttcn::AttributeSpec const& attrib,
  Common::CompTypeRefList*& attr_comp_refs)
{
  const string& s = attrib.get_spec();
  struct yy_buffer_state *flex_buffer = yy_scan_bytes(s.c_str(), s.size());
  if (!flex_buffer)
    FATAL_ERROR("parseExtendsCompTypeRefList(): flex buffer creation failed");
  yylex(attr_comp_refs, attrib.get_first_line(),
        attrib.get_first_column() + 1, attrib.get_filename() );
  yylex_destroy();
}

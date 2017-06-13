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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput
%option stack
%option noyy_top_state
%option prefix="coding_attrib_"

%{

/* Lexer for "extension" attributes of functions, external functions and
 * port types related to message encoding. */

#include <ctype.h>

#include "../string.hh"
#include "../Identifier.hh"
#include "Attributes.hh"
#include "Ttcnstuff.hh"

using namespace Ttcn;
using namespace Common;

#include "coding_attrib_p.hh"

#define yylval coding_attrib_lval
#define yylloc coding_attrib_lloc

/** global variable indicating the location of the returned token to bison */
extern YYLTYPE yylloc;

/** the name of the current input file */
const char *coding_attrib_infile;

/** always points to the first character of the regexp to be recognized */
static int current_line, current_column;

/** the actual size of state condition stack */
static size_t stack_size = 0;

static void fill_location()
{
  yylloc.first_line = current_line;
  yylloc.first_column = current_column;
  current_column += yyleng;
  yylloc.last_line = current_line;
  yylloc.last_column = current_column;
}

#define PUSH_STATE(new_state) yy_push_state(new_state); stack_size++

#define RETURN(ret_val) fill_location(); last_token = ret_val; return ret_val

%}

LINECOMMENT "//"[^\r\n]*
WHITESPACE [ \t\v\f]
NEWLINE \r|\n|\r\n
IDENTIFIER [A-Za-z][A-Za-z0-9_]*
NUMBER 0|[1-9][0-9]*

%s SC_start
%s SC_prototype SC_errorbehavior
%s SC_encodingtype SC_encodingoptions
%s SC_user SC_typemappinglist SC_mappingtype SC_functionmapping
%s SC_printing

%x SC_blockcomment

%%
  /* local variables of yylex() */
  static int last_token = EOF;
  int start_line = 0, start_column = 0; /**< used by block comments */

  /* eat up whitespaces and line comments in all states */

<*>{
  {WHITESPACE}+|{LINECOMMENT} current_column += yyleng;
  {WHITESPACE}*{NEWLINE} {
    current_line++;
    current_column = 0;
  }
}

"/*" {
  start_line = current_line;
  start_column = current_column;
  current_column += yyleng;
  PUSH_STATE(SC_blockcomment);
}

<SC_blockcomment>{
 "*/" {
   current_column += yyleng;
   yy_pop_state();
   stack_size--;
 }
 [^\r\n] current_column++;
}

<SC_errorbehavior>{IDENTIFIER} {
  yylval.str = new string(yyleng, yytext);
  RETURN(ErrorBehaviorString);
}

<SC_encodingoptions>[^ \t\r\n\(\)]+ {
  yylval.str = new string(yyleng, yytext);
  RETURN(EncodingOption);
}

<SC_printing>{
  compact     RETURN(CompactKeyword);
  pretty      RETURN(PrettyKeyword);
}

 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  * Initial start condition. Will switch to another start condition later,
  * when the opening paren '(' is encountered.
  */
<SC_start>{
  prototype     RETURN(PrototypeKeyword);

  address       RETURN(AddressKeyword);
  /* porttype */
  internal      RETURN(InternalKeyword);
  provider      RETURN(ProviderKeyword);
  user {
    BEGIN(SC_user);
    RETURN(UserKeyword);
  }
  anytype       RETURN(AnyTypeKeyword);

  version       RETURN(VersionKeyword);
  requires      RETURN(RequiresKeyword);
  requiresTITAN RETURN(ReqTitanKeyword);

  transparent   RETURN(TransparentKeyword);
}

  /* TTCN-3 keywords are valid in all (non-exclusive) states */

bitstring   RETURN(BitStringKeyword);
boolean     RETURN(BooleanKeyword);
charstring  RETURN(CharStringKeyword);
default     RETURN(DefaultKeyword);
float       RETURN(FloatKeyword);
function    RETURN(FunctionKeyword);
hexstring   RETURN(HexStringKeyword);
in          RETURN(InKeyword);
integer     RETURN(IntegerKeyword);
objid       RETURN(ObjectIdentifierKeyword);
octetstring RETURN(OctetStringKeyword);
out         RETURN(OutKeyword);
universal   RETURN(UniversalKeyword);
verdicttype RETURN(VerdictTypeKeyword);

  /* the rest of the keywords are context sensitive */

<SC_start,SC_mappingtype>{
  decode        RETURN(DecodeKeyword);
  encode        RETURN(EncodeKeyword);
  errorbehavior RETURN(ErrorBehaviorKeyword);
  printing      RETURN(PrintingKeyword);
}

<SC_mappingtype>{
  discard RETURN(DiscardKeyword);
  simple  RETURN(SimpleKeyword);
}

<SC_prototype>{
  backtrack {
    yylval.prototype = Def_Function_Base::PROTOTYPE_BACKTRACK;
    RETURN(PrototypeSetting);
  }
  convert {
    yylval.prototype = Def_Function_Base::PROTOTYPE_CONVERT;
    RETURN(PrototypeSetting);
  }
  fast {
    yylval.prototype = Def_Function_Base::PROTOTYPE_FAST;
    RETURN(PrototypeSetting);
  }
  sliding {
    yylval.prototype = Def_Function_Base::PROTOTYPE_SLIDING;
    RETURN(PrototypeSetting);
  }
}

<SC_encodingtype>{
  BER {
    yylval.encoding_type = Type::CT_BER;
    RETURN(EncodingType);
  }
  PER {
    yylval.encoding_type = Type::CT_PER;
    RETURN(EncodingType);
  }
  XER {
    yylval.encoding_type = Type::CT_XER;
    RETURN(EncodingType);
  }
  RAW {
    yylval.encoding_type = Type::CT_RAW;
    RETURN(EncodingType);
  }
  TEXT {
    yylval.encoding_type = Type::CT_TEXT;
    RETURN(EncodingType);
  }
  JSON {
    yylval.encoding_type = Type::CT_JSON;
    RETURN(EncodingType);
  }
  {IDENTIFIER} {
    yylval.str = new string(yyleng, yytext);
    RETURN(CustomEncoding);
  }
}

{IDENTIFIER} {
  yylval.id = new Identifier(Identifier::ID_TTCN, string(yyleng, yytext));
  RETURN(IDentifier);
}

{NUMBER} {
  yylval.number = atoi(yytext);
  RETURN(Number);
}


"->" RETURN(RedirectSymbol);

\( {
  switch (last_token) {
  case PrototypeKeyword:
    PUSH_STATE(SC_prototype);
    break;
  case EncodeKeyword:
  case DecodeKeyword:
    PUSH_STATE(SC_encodingtype);
    break;
  case ErrorBehaviorKeyword:
    PUSH_STATE(SC_errorbehavior);
    break;
  case InKeyword:
  case OutKeyword:
    PUSH_STATE(SC_typemappinglist);
    break;
  case FunctionKeyword:
    PUSH_STATE(SC_functionmapping);
    break;
  case PrintingKeyword:
    PUSH_STATE(SC_printing);
    break;
  default:
    PUSH_STATE(YY_START); /* Push the current state (duplicate) */
    break;
  }
  if (YY_START == SC_encodingoptions) {
    yylval.str = new string(yyleng, yytext);
    RETURN(EncodingOption);
  } else {
    RETURN('(');
  }
}

\) {
  if (stack_size > 0) {
    yy_pop_state();
    stack_size--;
  }
  if (YY_START == SC_encodingoptions) {
    yylval.str = new string(yyleng, yytext);
    RETURN(EncodingOption);
  } else {
    if (YY_START == SC_user) BEGIN(SC_start);
    RETURN(')');
  }
}

: {
  switch (YY_START) /* current start condition is... */ {
  case SC_encodingtype: /* BER | RAW | ... */
    BEGIN(SC_encodingoptions);
    break;
  case SC_typemappinglist:
    BEGIN(SC_mappingtype);
    break;
  }
  RETURN(':');
}

\,|\; {
  if (YY_START == SC_mappingtype) BEGIN(SC_typemappinglist);
  RETURN(*yytext);
}

[-.<>/] RETURN(*yytext);

<*>{
  . {
    fill_location();
    Location loc(coding_attrib_infile, yylloc);
    int c = (unsigned char)yytext[0];
    loc.error("Character `%c' (0x%02X) cannot be used in extension attribute",
      isprint(c) ? c : '?', c);
  }
  <<EOF>> {
    if (YY_START == SC_blockcomment) {
      Location loc(coding_attrib_infile, start_line, start_column,
        current_line, current_column);
      loc.error("Unterminated block comment");
    }
    /* we must set the fake length 1 in order to report the single column number
     * of the unexpected EOF in error messages */
    yyleng = 1;
    RETURN(EOF);
  }
}

%%

void init_coding_attrib_lex(/*char definition_type,*/ const AttributeSpec& attrib)
{
  if (stack_size > 0)
    FATAL_ERROR("init_coding_attrib_lex(): stack is not empty");
  coding_attrib_infile = attrib.get_filename();
  current_line = attrib.get_first_line();
  /* skip the leading " of the attribute value */
  current_column = attrib.get_first_column() + 1;
  const string& s = attrib.get_spec();
  yy_scan_bytes(s.c_str(), s.size());
  BEGIN(SC_start);
}

void cleanup_coding_attrib_lex()
{
  while (stack_size > 0) {
    yy_pop_state();
    stack_size--;
  }
  coding_attrib_lex_destroy();
}

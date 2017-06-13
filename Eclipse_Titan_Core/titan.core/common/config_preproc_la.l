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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
 
%option noyywrap
%option never-interactive
%option nounput
%option prefix="config_preproc_yy"

%{

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <deque>
#include <string>

#include "Path2.hh"
#include "cfg_process_utils.hh"

#include "config_preproc.h"
#include "path.h"
#include "config_preproc_p.tab.hh"

//#include "dbgnew.hh"

#define yylineno config_preproc_yylineno

extern int add_include_file(const std::string&);

/* If there are errors in the parsed define sections this value changes to 1.
   flex and bison for preprocessing can set it to 1, sometimes without printing
   the error message because strings and some other stuff is also parsed by the
   actual configuration parser which prints errors */
int preproc_error_flag;

/* used to track the curly brackets in the define section */
static int paren_stack = 0;

static std::deque<IncludeElem<YY_BUFFER_STATE> >* include_chain = NULL;

std::string get_cfg_preproc_current_file() {
  if (include_chain && !include_chain->empty()) {
    return include_chain->back().get_full_path();
  }
  return std::string();
}

%}

WHITESPACE [ \t]
WS {WHITESPACE}*
NEWLINE \r|\n|\r\n
LINECOMMENT ("//"|"#")[^\r\n]*{NEWLINE}
NUMBER 0|([1-9][0-9]*)

TTCN3IDENTIFIER [A-Za-z][A-Za-z0-9_]*

HEX [0-9A-Fa-f]

MACRORVALUE ([0-9A-Za-z._-]+)|{IPV6}
IPV6 [0-9A-Fa-f:.]+(%[0-9A-Za-z]+)?

MACRO_REFERENCE \${TTCN3IDENTIFIER}|\$"{"{WS}{TTCN3IDENTIFIER}{WS}(","{WS}charstring{WS})?"}"
MACRO_REFERENCE_INT \$"{"{WS}{TTCN3IDENTIFIER}{WS}(","{WS}integer{WS})?"}"


%x SC_blockcomment SC_cstring
%s SC_include SC_define SC_define_structured SC_ordered_include
%s SC_module_parameters SC_testport_parameters

%%
  /* valid in states SC_blockcomment, SC_cstring */
  int caller_state = INITIAL;
  /* valid in state SC_cstring */
  std::string cstring;

  preproc_error_flag = 0;

"/*" {
  caller_state = YY_START;
  BEGIN(SC_blockcomment);
}

<SC_blockcomment>
{
"*/" {
  BEGIN(caller_state);
  if (YY_START==SC_define || YY_START==SC_define_structured) {
    config_preproc_yylval.str_val = NULL;
    return FillerStuff;
  }
}

{NEWLINE} yylineno++;

. /* do nothing */

<<EOF>> {
  preproc_error_flag = 1; /* unterminated block comment, error msg. in parser */
  BEGIN(caller_state);
}
} /* SC_blockcomment */

{WHITESPACE} {
  if (YY_START==SC_define || YY_START==SC_define_structured) {
    config_preproc_yylval.str_val = mcopystrn(yytext, yyleng); return FillerStuff;
  }
}

{NEWLINE}|{LINECOMMENT} {
  yylineno++;
  if (YY_START==SC_define || YY_START==SC_define_structured) {
    config_preproc_yylval.str_val = NULL;
    return FillerStuff;
  }
}

  /* Section delimiters */

"["{WS}INCLUDE{WS}"]" BEGIN(SC_include);

"["{WS}ORDERED_INCLUDE{WS}"]" {
  caller_state = YY_START;
  BEGIN(SC_ordered_include);
}

"["{WS}DEFINE{WS}"]" BEGIN(SC_define);

"["{WS}MODULE_PARAMETERS{WS}"]"   BEGIN(SC_module_parameters);
"["{WS}TESTPORT_PARAMETERS{WS}"]" BEGIN(SC_testport_parameters);

<SC_testport_parameters>
{
  "["{NUMBER}"]" ;
}

<SC_module_parameters>
{
  "["[ \t0-9a-zA-Z+*/&-]*"]" ;
}

<SC_module_parameters,SC_testport_parameters>
{
  "["[^\r\n\[\]]*{MACRO_REFERENCE_INT}{WS}[^\r\n\[\]]*"]" ;
}

"["{WS}LOGGING{WS}"]"             BEGIN(INITIAL);
"["{WS}PROFILER{WS}"]"            BEGIN(INITIAL);
"["{WS}EXECUTE{WS}"]"             BEGIN(INITIAL);
"["{WS}EXTERNAL_COMMANDS{WS}"]"   BEGIN(INITIAL);
"["{WS}GROUPS{WS}"]"              BEGIN(INITIAL);
"["{WS}COMPONENTS{WS}"]"          BEGIN(INITIAL);
"["{WS}MAIN_CONTROLLER{WS}"]"     BEGIN(INITIAL);

"["[^\r\n\[\]]*"]" {
  preproc_error_flag = 1; 
  config_preproc_error("Invalid section name `%s'", yytext);
}

\" {
  caller_state = YY_START;
  BEGIN(SC_cstring);
  cstring.clear();

  if (caller_state == SC_define_structured) {
    cstring += '"';
  }
}

<SC_cstring>
{
\"\" cstring += '"';

\" {
  /* end of the string */
  BEGIN(caller_state);
  switch (YY_START) {
  case SC_define_structured:
    cstring += '"';
      /* No break */
  case SC_define:
    config_preproc_yylval.str_val = mcopystrn(cstring.c_str(), cstring.size());
    cstring.clear();
    return Cstring;
  case SC_include:
    if (add_include_file(cstring)) preproc_error_flag = 1;
    cstring.clear();
    break;
  case SC_ordered_include: 
    {
      std::string error_msg = switch_lexer(include_chain, cstring,
          YY_CURRENT_BUFFER, yy_create_buffer, yy_switch_to_buffer, yylineno,
          YY_BUF_SIZE);
      if (error_msg.empty()) {
        BEGIN(INITIAL);
      } else {
        preproc_error_flag = 1; 
        config_preproc_error("%s", error_msg.c_str());
      }
    }
    /* no break */
  default:
    cstring.clear();
  } /* switch */
} /* end of string */

\\[\\'"?] cstring += yytext[1]; /* backslash-quoted \ or " or ' or ? */
\\{NEWLINE} yylineno++;
\\a cstring += '\a';
\\b cstring += '\b';
\\f cstring += '\f';
\\n cstring += '\n';
\\r cstring += '\r';
\\t cstring += '\t';
\\v cstring += '\v';

\\[0-7]{1,3} {
  unsigned int c;
  sscanf(yytext + 1, "%o", &c);
  /* do not report error in case of invalid octal escape sequences */
  if (c <= 255) cstring += c;
  else preproc_error_flag = 1;
}

\\x{HEX}{1,2} {
  unsigned int c;
  sscanf(yytext + 2, "%x", &c);
  cstring += c;
}

\\(x[^\\\"]|.) preproc_error_flag = 1;

{NEWLINE} {
  cstring.append(yytext, yyleng);
  yylineno++;
}

. { cstring += yytext[0]; }

<<EOF>> {
  preproc_error_flag = 1; /* unterminated string literal, error msg. in parser */
  cstring.clear();
  BEGIN(caller_state);
  return EOF;
}
} /* SC_cstring */


<SC_define>
{

{TTCN3IDENTIFIER} {
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return Identifier;
}

":="|"=" { return AssignmentChar; }

{MACRORVALUE} {
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return MacroRValue;
}

{MACRO_REFERENCE} {
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return MacroReference;
}

"{" {
  ++paren_stack;
  BEGIN(SC_define_structured);
  return LCurly;
}
} /* SC_define */

<SC_define_structured>
{

{MACRO_REFERENCE} {
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return MacroReference;
}

"{" {
  ++paren_stack;
  return LCurly;
}

"}" {
  if (paren_stack == 0) { /* We don't have any opened curly brackets. */
    preproc_error_flag = 1;
    config_preproc_error("Invalid structured definition.");
    BEGIN(SC_define);
    return RCurly;
  }

  --paren_stack;
  if (paren_stack == 0) { /* The end of a structured definition. */
    BEGIN(SC_define);
  }
  return RCurly;
}

\\\" { /* \" is handled separately in the structured definitions */
  config_preproc_yylval.str_val = mcopystr("\""); 
  return FString;
}

\\. { /* Handle escaped characters */
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return FString;
}

[^{}"\\$\n\r\t #/]+  { /* Anything except {,},\,$,#,/ and whitespace */
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return FString;
}

"/" {
  config_preproc_yylval.str_val = mcopystrn(yytext, yyleng);
  return FString;
}

} /* SC_define_structured */

<<EOF>> { 
  if (include_chain->size() > 1) { // The last fp is owned by the parser
    yy_delete_buffer(YY_CURRENT_BUFFER);
    fclose(include_chain->back().fp);
    include_chain->pop_back();
    yy_switch_to_buffer(include_chain->back().buffer_state);
    BEGIN(SC_ordered_include);
  } else {
    return EOF; 
  }
}

. {
  switch (YY_START) {
  case SC_define:
  case SC_define_structured:
    preproc_error_flag = 1;
    config_preproc_error("Invalid character in [DEFINE] section: '%c'.", yytext[0]);
    break;
  case SC_include:
  case SC_ordered_include:
    preproc_error_flag = 1;
    config_preproc_error("Invalid character in [%s] section: '%c'.",
     (YY_START==SC_include || YY_START==SC_ordered_include) ? "INCLUDE" : "ORDERED_INCLUDE",
     yytext[0]);
  default:
    break;
  }
}


%%

void config_preproc_reset(const std::string& filename) {
  if (!include_chain) {
    include_chain = new std::deque<IncludeElem<YY_BUFFER_STATE> >();
  } else {
    include_chain->clear();
  }

  include_chain->push_back(IncludeElem<YY_BUFFER_STATE>(
        filename, config_preproc_yyin));
}

void config_preproc_close() {
  delete include_chain;
  include_chain = NULL;
}


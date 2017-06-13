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
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput

%{
 
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <deque>
#include <string>

#include "../common/cfg_process_utils.hh"
#include "../common/Path2.hh"

#include "../common/memory.h"
#include "Types.h"
#include "Parameters.h"
#include "Param_Types.hh"
#include "Port.hh"
#include "LoggingBits.hh"
#include "LoggingParam.hh"
#include "Error.hh"
#include "config_process.tab.hh"
#include "../common/config_preproc.h"
#include "../common/path.h"

#include "../common/dbgnew.hh"

#ifndef INFINITY
#include <float.h>
static const double INFINITY = (DBL_MAX*DBL_MAX);
#endif

#include "Profiler.hh"

extern string_map_t *config_defines;

#define yylval config_process_lval

static int current_line;

static YY_BUFFER_STATE main_buffer      = NULL;
/* This buffer is used for macro expansion */
static YY_BUFFER_STATE expansion_buffer = NULL;

static std::deque<IncludeElem<YY_BUFFER_STATE> >* include_chain = NULL;

static void set_ret_val_cstr(const std::string& cstring) {
  param_charstring_t& ch_val = yylval.charstring_val;
  ch_val.n_chars = cstring.size();
  ch_val.chars_ptr = (char*) Malloc(ch_val.n_chars + 1);
  memcpy(ch_val.chars_ptr, cstring.c_str(), cstring.size() + 1);
}

std::string get_cfg_process_current_file() {
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

FLOAT [+-]?({NUMBER}\.[0-9]+)|((({NUMBER}(\.[0-9]+)?)|(\.[0-9]+))[Ee][+-]?{NUMBER})|not_a_number

BIN 0|1
BITSTRING '{BIN}*'B
BINMATCH 0|1|\?|\*
BITSTRINGMATCH '{BINMATCH}*'B
BITSTRING_BAD '[^']*'B

HEX [0-9A-Fa-f]
HEXSTRING '{HEX}*'H
HEXMATCH [0-9A-Fa-f\?\*]
HEXSTRINGMATCH '{HEXMATCH}*'H
HEXSTRING_BAD '[^']*'H

OCT {HEX}{HEX}
OCTETSTRING '{OCT}*'O
OCTMATCH {HEX}{HEX}|\?|\*
OCTETSTRINGMATCH '{OCTMATCH}*'O
OCTETSTRING_BAD '[^']*'O

BINSTRING_BAD '[^']*'

TTCN3IDENTIFIER [A-Za-z][A-Za-z0-9_]*
ASN1LOWERIDENTIFIER [a-z](-?[A-Za-z0-9]+)*

COMPONENT .*\({NUMBER}\)

MACRO_CSTR \${TTCN3IDENTIFIER}|\$"{"{WS}{TTCN3IDENTIFIER}{WS}(","{WS}charstring{WS})?"}"
MACRO_BOOL \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}boolean{WS}"}"
MACRO_FLOAT \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}float{WS}"}"
MACRO_ID \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}identifier{WS}"}"
MACRO_INT \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}integer{WS}"}"
MACRO_BSTR \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}bitstring{WS}"}"
MACRO_HSTR \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}hexstring{WS}"}"
MACRO_OSTR \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}octetstring{WS}"}"
MACRO_BINARY \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}binaryoctet{WS}"}"
MACRO_HOSTNAME \$"{"{WS}{TTCN3IDENTIFIER}{WS}","{WS}hostname{WS}"}"

HOSTNAME [A-Za-z0-9]([A-Za-z0-9_\-]*[A-Za-z0-9])?
DNSNAME {HOSTNAME}(\.{HOSTNAME})*\.?

/* Example: fe80::c002:37ff:fe6c:0%fastethernet0/0 */
IPV6 [0-9A-Fa-f:.]+(%[0-9A-Za-z]+)?

UID [uU][+]?[0-9A-Fa-f]{1,8}

TTCNSTRINGPARSING "$#&&&(#TTCNSTRINGPARSING$#&&^#% "
TTCNSTRINGPARSING_COMPONENT "$#&&&(#TTCNSTRINGPARSING_COMPONENT$#&&^#% "

%x SC_commentblock SC_cstring SC_DEFINE
%s SC_MODULE_PARAMETERS SC_LOGGING SC_TESTPORT_PARAMETERS SC_EXECUTE SC_GROUPS
%s SC_COMPONENTS SC_EXTERNAL_COMMANDS SC_MAIN_CONTROLLER SC_INCLUDE SC_ORDERED_INCLUDE
%s SC_STRING2TTCN_COMPONENT SC_PROFILER SC_CHAR_KEYWORD

%%

	int caller_state = INITIAL;
  std::string cstring;
  int cstring_start = -1;

	/* Eat up comments and whitespaces */

"/*"	{
		caller_state = YY_START;
		BEGIN(SC_commentblock);
	}
<SC_commentblock>
{
"*/"		BEGIN(caller_state);
{NEWLINE}	current_line++;
.		/* do nothing */
<<EOF>>	{
	config_process_error("Unterminated block comment (missing */ at the "
	    "end of file).");
	BEGIN(caller_state);
	}
}

{WHITESPACE}
{NEWLINE}	|
{LINECOMMENT}	current_line++;

{TTCNSTRINGPARSING} {
  if (Ttcn_String_Parsing::happening() || Debugger_Value_Parsing::happening()) {
    BEGIN(SC_MODULE_PARAMETERS);
    return TtcnStringParsingKeyword;
  } else {
    config_process_error("Invalid character sequence encountered.");
  }
}

{TTCNSTRINGPARSING_COMPONENT} {
  if (Ttcn_String_Parsing::happening()) {
    BEGIN(SC_STRING2TTCN_COMPONENT);
    return TtcnStringParsingKeyword;
  } else {
    config_process_error("Invalid character sequence encountered.");
  }
}

	/* Section delimiters */

<*>"["{WS}MODULE_PARAMETERS{WS}"]"	{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_MODULE_PARAMETERS);
	return ModuleParametersKeyword;
	}
}

<*>"["{WS}LOGGING{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_LOGGING);
	return LoggingKeyword;
	}
}

<*>"["{WS}PROFILER{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_PROFILER);
	return ProfilerKeyword;
	}
}

<*>"["{WS}TESTPORT_PARAMETERS{WS}"]"	{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_TESTPORT_PARAMETERS);
	return TestportParametersKeyword;
	}
}

<*>"["{WS}EXECUTE{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_EXECUTE);
	return ExecuteKeyword;
	}
}

<*>"["{WS}EXTERNAL_COMMANDS{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_EXTERNAL_COMMANDS);
	return ExternalCommandsKeyword;
	}
}

<*>"["{WS}GROUPS{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_GROUPS);
	return GroupsKeyword;
	}
}

<*>"["{WS}COMPONENTS{WS}"]"	{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_COMPONENTS);
	return ComponentsKeyword;
	}
}

<*>"["{WS}MAIN_CONTROLLER{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_MAIN_CONTROLLER);
	return MainControllerKeyword;
	}
}

<*>"["{WS}INCLUDE{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_INCLUDE);
	return IncludeKeyword;
	}
}

<*>"["{WS}ORDERED_INCLUDE{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_ORDERED_INCLUDE);
	}
}

<*>"["{WS}DEFINE{WS}"]"		{
  if (YY_START!=SC_commentblock && YY_START!=SC_cstring) {
	BEGIN(SC_DEFINE);
	return DefineKeyword;
	}
}

	/* Rules for SC_DEFINE must precede everything else */

<SC_DEFINE>{

"/*"	{
		caller_state = SC_DEFINE;
		BEGIN(SC_commentblock);
}

\"  {
  caller_state = SC_DEFINE;
  BEGIN(SC_cstring);
  cstring.clear();
  cstring_start = current_line;
}

{LINECOMMENT}|{NEWLINE} current_line++;

\\\"

. /* eat unnecessary chars */

}

<SC_PROFILER>{HEX}+ {
  /* numeric statistics filter (check this before checking for NUMBERs) */
  yylval.uint_val = 0;
  while(0 != *yytext) {
    yylval.uint_val *= 16;
    if ('0' <= *yytext && '9' >= *yytext) {
      yylval.uint_val += *yytext - '0';
    }
    else if ('a' <= *yytext && 'f' >= *yytext) {
      yylval.uint_val += *yytext - 'a' + 10;
    }
    else {
      yylval.uint_val += *yytext - 'A' + 10;
    }
    ++yytext;
  }
  return ProfilerStatsFlag;
}

	/* Values */

{NUMBER}	{
	yylval.int_val = new int_val_t(yytext);
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    // return a different token for module parameters so it doesn't conflict with references
    return MPNumber;
  }
	return Number;
	}

{FLOAT}		{
  if (!strcmp(yytext, "not_a_number")) {
#ifdef NAN
    yylval.float_val = NAN;
#else
    yylval.float_val = INFINITY + (-INFINITY);
#endif
  } else {
    yylval.float_val = atof(yytext);
  }
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    // return a different token for module parameters so it doesn't conflict with references
    return MPFloat;
  }
	return Float;
	}

{BITSTRING}	{
	yylval.bitstring_val.n_bits = yyleng - 3;
	int n_bytes = (yylval.bitstring_val.n_bits + 7) / 8;
	yylval.bitstring_val.bits_ptr = (unsigned char *)Malloc(n_bytes);
	memset(yylval.bitstring_val.bits_ptr, 0, n_bytes);
	for (int i = 0; i < yylval.bitstring_val.n_bits; i++)
		if (yytext[i+1] == '1')
			yylval.bitstring_val.bits_ptr[i/8] |= 1 << (i % 8);
	return Bstring;
	}

{BITSTRINGMATCH} {
  yylval.str_val = mcopystrn(yytext+1, yyleng-3);
  return BstringMatch;
}

{BITSTRING_BAD}	{
	config_process_error("Invalid bitstring value.");
	yylval.bitstring_val.n_bits = 0;
	yylval.bitstring_val.bits_ptr = NULL;
	return Bstring;
}

{HEXSTRING}	{
	yylval.hexstring_val.n_nibbles = yyleng - 3;
	int n_bytes = (yylval.hexstring_val.n_nibbles + 1) / 2;
	yylval.hexstring_val.nibbles_ptr = (unsigned char *)Malloc(n_bytes);
	memset(yylval.hexstring_val.nibbles_ptr, 0, n_bytes);
	for (int i = 0; i < yylval.hexstring_val.n_nibbles; i++) {
		unsigned int hex_digit;
		sscanf(yytext+i+1, "%1x", &hex_digit);
		if(i % 2) yylval.hexstring_val.nibbles_ptr[i/2] |=
			(hex_digit << 4);
		else yylval.hexstring_val.nibbles_ptr[i/2] |= hex_digit;
	}
	return Hstring;
	}

{HEXSTRINGMATCH} {
  yylval.str_val = mcopystrn(yytext+1, yyleng-3);
  return HstringMatch;
}

{HEXSTRING_BAD}	{
	config_process_error("Invalid hexstring value.");
	yylval.hexstring_val.n_nibbles = 0;
	yylval.hexstring_val.nibbles_ptr = NULL;
	return Hstring;
}

{OCTETSTRING}	{
	yylval.octetstring_val.n_octets = (yyleng - 3) / 2;
	yylval.octetstring_val.octets_ptr = (unsigned char *)
		Malloc(yylval.octetstring_val.n_octets);

	for (int i = 0; i < yylval.octetstring_val.n_octets; i++) {
		unsigned int this_octet;
		sscanf(yytext+2*i+1, "%2x", &this_octet);
		yylval.octetstring_val.octets_ptr[i] = this_octet;
	}
	return Ostring;
	}

{OCTETSTRINGMATCH} {
  yylval.str_val = mcopystrn(yytext+1, yyleng-3);
  return OstringMatch;
}

{OCTETSTRING_BAD}	{
	config_process_error("Invalid octetstring value.");
	yylval.octetstring_val.n_octets = 0;
	yylval.octetstring_val.octets_ptr = NULL;
	return Ostring;
}

{BINSTRING_BAD}	config_process_error("Invalid string value.");

'	config_process_error("Unmatched ' character.");

\"	{
	caller_state = YY_START;
	BEGIN(SC_cstring);
  cstring.clear();
  cstring_start = current_line;
	}

<SC_cstring>
{
\"\"	cstring += '"';
\"	{
  /* end of the string */
	BEGIN(caller_state);
  switch (caller_state) {
  case SC_DEFINE:
    cstring.clear();
    break;
  case SC_ORDERED_INCLUDE:
    {
      std::string error_msg = switch_lexer(include_chain, cstring,
          YY_CURRENT_BUFFER, yy_create_buffer, yy_switch_to_buffer,
          current_line, YY_BUF_SIZE);
      if (error_msg.empty()) {
        BEGIN(INITIAL);
      } else {
        config_process_error(error_msg.c_str());
      }
    }
    break;
  default:
    set_ret_val_cstr(cstring);
    if (caller_state == SC_MODULE_PARAMETERS || caller_state == SC_CHAR_KEYWORD) {
      // return a different token for module parameters so it doesn't conflict with references
      return MPCstring;
    }
    return Cstring;
  }

  cstring.clear();
  cstring_start = -1;
	}
\\[\\'"?]	cstring += yytext[1]; /* backslash-quoted \ or " or ' or ? */
\\{NEWLINE}	current_line++;
\\a	cstring += '\a';
\\b	cstring += '\b';
\\f	cstring += '\f';
\\n	cstring += '\n';
\\r	cstring += '\r';
\\t	cstring += '\t';
\\v	cstring += '\v';
\\[0-7]{1,3}	{
	unsigned int c;
	sscanf(yytext + 1, "%o", &c);
	if (c > 255)
	    config_process_error("Invalid octal character code in string "
		"literal.");
	else if (c == 0 && caller_state != SC_MODULE_PARAMETERS)
	    config_process_error("NUL characters in string literals are "
		"allowed only in section [MODULE_PARAMETERS].");
	else cstring += c;
	}
\\x{HEX}{1,2}	{
	unsigned int c;
	sscanf(yytext + 2, "%x", &c);
	if (c == 0 && caller_state != SC_MODULE_PARAMETERS)
	    config_process_error("NUL characters in string literals are "
		"allowed only in section [MODULE_PARAMETERS].");
	else cstring += c;
	}
\\(x[^\\\"]|.)	{
	config_process_error("Invalid escape sequence in string literal.");
	}
{NEWLINE}	{
  cstring.append(yytext, yyleng);
	current_line++;
	}
.	cstring += yytext[0];
<<EOF>>	{
  if (cstring_start >=0) {
    config_process_error_f("Unterminated string literal starting at line %d "
        "(missing \" at the end of file).", cstring_start);
  } else {
    config_process_error("Unterminated string literal (missing \" at the "
          "end of file).");
  }
  BEGIN(caller_state);
  if (caller_state!=SC_DEFINE) {
    set_ret_val_cstr(cstring);
    if (caller_state == SC_MODULE_PARAMETERS || caller_state == SC_CHAR_KEYWORD) {
      // return a different token for module parameters so it doesn't conflict with references
      return MPCstring;
    }
	  return Cstring;
  } 

  cstring.clear();
	}
}

<SC_CHAR_KEYWORD>
{
  {UID} {
    yylval.str_val = mcopystrn(yytext, yyleng);
    return UIDval;
  }

  [,] { return *yytext; }

  [)] { BEGIN(SC_MODULE_PARAMETERS); return *yytext; }
}

	/* Section-wide keywords */

<SC_MODULE_PARAMETERS>
{
NULL		return NULLKeyword;
null		return nullKeyword;
char		{ BEGIN(SC_CHAR_KEYWORD); return CharKeyword; }
objid		return ObjIdKeyword;
omit		return OmitKeyword;
none		{
	yylval.verdict_val = NONE;
	return VerdictValue;
	}
pass		{
	yylval.verdict_val = PASS;
	return VerdictValue;
	}
inconc		{
	yylval.verdict_val = INCONC;
	return VerdictValue;
	}
fail		{
	yylval.verdict_val = FAIL;
	return VerdictValue;
	}
error		{
	yylval.verdict_val = ERROR;
	return VerdictValue;
	}
complement  return ComplementKeyword;
"\.\."      return DotDot;
superset    return SupersetKeyword;
subset      return SubsetKeyword;
pattern     return PatternKeyword;
permutation return PermutationKeyword;
length      return LengthKeyword;
ifpresent   return IfpresentKeyword;
infinity    return InfinityKeyword;
"@nocase"   return NocaseKeyword;
}

<SC_MODULE_PARAMETERS,SC_LOGGING,SC_PROFILER>
{
true		{
	yylval.bool_val = TRUE;
	return BooleanValue;
	}
false		{
	yylval.bool_val = FALSE;
	return BooleanValue;
	}
}

  /* We could add SC_LOGGING to make mtc and system special for the logging section too */
<SC_MODULE_PARAMETERS,SC_STRING2TTCN_COMPONENT,SC_TESTPORT_PARAMETERS,SC_LOGGING>
{
mtc		return MTCKeyword;
system		return SystemKeyword;
}

<SC_LOGGING>
{
[Ff]ile[Nn]ame			|
[Ll]og[Ff]ile			return LogFile;

[Ee]mergency[Ll]ogging     return EmergencyLogging;

[Ee]mergency[Ll]ogging[Bb]ehaviour     return EmergencyLoggingBehaviour;

[Ee]mergency[Ll]ogging[Mm]ask     return EmergencyLoggingMask;

[Ee]mergency[Ll]ogging[Ff]or[Ff]ail[Vv]erdict  return EmergencyLoggingForFailVerdict;

[Ff]ile[Mm]ask			return FileMask;

[Cc]onsole[Mm]ask		return ConsoleMask;

[Tt]ime[Ss]tamp[Ff]ormat	return TimestampFormat;
[Cc]onsole[Tt]ime[Ss]tamp[Ff]ormat	return ConsoleTimestampFormat;

[Ll]og[Ss]ource[Ii]nfo		|
[Ss]ource[Ii]nfo[Ff]ormat	return SourceInfoFormat;

[Aa]ppend[Ff]ile		return AppendFile;

[Ll]og[Ee]vent[Tt]ypes		return LogEventTypes;

[Ll]og[Ee]ntity[Nn]ame		return LogEntityName;

[Ll]og[Ff]ile[Ss]ize            return LogFileSize;

[Ll]og[Ff]ile[Nn]umber          return LogFileNumber;

[Dd]isk[Ff]ull[Aa]ction         return DiskFullAction;

[Mm]atching[Hh]ints             return MatchingHints;

[Ll]ogger[Pp]lugins return LoggerPlugins;

    /* Lexer's handling of logging keywords:
    *
    *  For sub-categories and categories with only one member,
    *  LoggingBit is returned and the value is the corresponding
    *  TTCN_Logger::Severity.
    *
    *  For the old categories which now have subcategories, the old keyword
    *  is returned as LoggingBitCollection with the corresponding
    *  _UNQUALIFIED as the semantic value. The parser will construct the
    *  multi-bit value.
    *
    *  The lexer never returns a token with a Logging_Bits type.
    */
LOG_NOTHING	{
	yylval.logseverity_val = TTCN_Logger::NOTHING_TO_LOG;
	return LoggingBit;
	}

ACTION_UNQUALIFIED	{
	yylval.logseverity_val = TTCN_Logger::ACTION_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?ACTION	{
	yylval.logseverity_val = TTCN_Logger::ACTION_UNQUALIFIED;
	return LoggingBitCollection;
	}

DEBUG_ENCDEC	{
	yylval.logseverity_val = TTCN_Logger::DEBUG_ENCDEC;
	return LoggingBit;
	}
DEBUG_TESTPORT	{
	yylval.logseverity_val = TTCN_Logger::DEBUG_TESTPORT;
	return LoggingBit;
	}
DEBUG_USER	{
	yylval.logseverity_val = TTCN_Logger::DEBUG_USER;
	return LoggingBit;
	}
DEBUG_FRAMEWORK	{
	yylval.logseverity_val = TTCN_Logger::DEBUG_FRAMEWORK;
	return LoggingBit;
	}
DEBUG_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::DEBUG_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?DEBUG		{
	yylval.logseverity_val = TTCN_Logger::DEBUG_UNQUALIFIED;
	return LoggingBitCollection;
	}

DEFAULTOP_ACTIVATE		{
	yylval.logseverity_val = TTCN_Logger::DEFAULTOP_ACTIVATE;
	return LoggingBit;
	}
DEFAULTOP_DEACTIVATE		{
	yylval.logseverity_val = TTCN_Logger::DEFAULTOP_DEACTIVATE;
	return LoggingBit;
	}
DEFAULTOP_EXIT		{
	yylval.logseverity_val = TTCN_Logger::DEFAULTOP_EXIT;
	return LoggingBit;
	}
DEFAULTOP_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::DEFAULTOP_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?DEFAULTOP		{
	yylval.logseverity_val = TTCN_Logger::DEFAULTOP_UNQUALIFIED;
	return LoggingBitCollection;
	}

ERROR_UNQUALIFIED	{
	yylval.logseverity_val = TTCN_Logger::ERROR_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?ERROR	{
	yylval.logseverity_val = TTCN_Logger::ERROR_UNQUALIFIED;
	return LoggingBitCollection;
	}

EXECUTOR_COMPONENT		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_COMPONENT;
	return LoggingBit;
	}
EXECUTOR_CONFIGDATA	{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_CONFIGDATA;
	return LoggingBit;
	}
EXECUTOR_EXTCOMMAND		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_EXTCOMMAND;
	return LoggingBit;
	}
EXECUTOR_LOGOPTIONS		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_LOGOPTIONS;
	return LoggingBit;
	}
EXECUTOR_RUNTIME		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_RUNTIME;
	return LoggingBit;
	}
EXECUTOR_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?EXECUTOR		{
	yylval.logseverity_val = TTCN_Logger::EXECUTOR_UNQUALIFIED;
	return LoggingBitCollection;
	}

FUNCTION_RND		{
	yylval.logseverity_val = TTCN_Logger::FUNCTION_RND;
	return LoggingBit;
	}
FUNCTION_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::FUNCTION_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?FUNCTION		{
	yylval.logseverity_val = TTCN_Logger::FUNCTION_UNQUALIFIED;
	return LoggingBitCollection;
	}

MATCHING_DONE		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_DONE;
	return LoggingBit;
	}
MATCHING_MCSUCCESS		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_MCSUCCESS;
	return LoggingBit;
	}
MATCHING_MCUNSUCC		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_MCUNSUCC;
	return LoggingBit;
	}
MATCHING_MMSUCCESS		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_MMSUCCESS;
	return LoggingBit;
	}
MATCHING_MMUNSUCC		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_MMUNSUCC;
	return LoggingBit;
	}
MATCHING_PCSUCCESS		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_PCSUCCESS;
	return LoggingBit;
	}
MATCHING_PCUNSUCC		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_PCUNSUCC;
	return LoggingBit;
	}
MATCHING_PMSUCCESS		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_PMSUCCESS;
	return LoggingBit;
	}
MATCHING_PMUNSUCC		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_PMUNSUCC;
	return LoggingBit;
	}
MATCHING_PROBLEM		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_PROBLEM;
	return LoggingBit;
	}
MATCHING_TIMEOUT		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_TIMEOUT;
	return LoggingBit;
	}
MATCHING_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?MATCHING		{
	yylval.logseverity_val = TTCN_Logger::MATCHING_UNQUALIFIED;
	return LoggingBitCollection;
	}

PARALLEL_PORTCONN		{
	yylval.logseverity_val = TTCN_Logger::PARALLEL_PORTCONN;
	return LoggingBit;
	}
PARALLEL_PORTMAP		{
	yylval.logseverity_val = TTCN_Logger::PARALLEL_PORTMAP;
	return LoggingBit;
	}
PARALLEL_PTC		{
	yylval.logseverity_val = TTCN_Logger::PARALLEL_PTC;
	return LoggingBit;
	}
PARALLEL_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::PARALLEL_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?PARALLEL		{
	yylval.logseverity_val = TTCN_Logger::PARALLEL_UNQUALIFIED;
	return LoggingBitCollection;
	}

PORTEVENT_DUALRECV		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_DUALRECV;
	return LoggingBit;
	}
PORTEVENT_DUALSEND		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_DUALSEND;
	return LoggingBit;
	}
PORTEVENT_MCRECV		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_MCRECV;
	return LoggingBit;
	}
PORTEVENT_MCSEND		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_MCSEND;
	return LoggingBit;
	}
PORTEVENT_MMRECV		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_MMRECV;
	return LoggingBit;
	}
PORTEVENT_MMSEND		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_MMSEND;
	return LoggingBit;
	}
PORTEVENT_MQUEUE		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_MQUEUE;
	return LoggingBit;
	}
PORTEVENT_PCIN		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_PCIN;
	return LoggingBit;
	}
PORTEVENT_PCOUT		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_PCOUT;
	return LoggingBit;
	}
PORTEVENT_PMIN		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_PMIN;
	return LoggingBit;
	}
PORTEVENT_PMOUT		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_PMOUT;
	return LoggingBit;
	}
PORTEVENT_PQUEUE		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_PQUEUE;
	return LoggingBit;
	}
PORTEVENT_STATE		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_STATE;
	return LoggingBit;
	}
PORTEVENT_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_UNQUALIFIED;
	return LoggingBit;
	}
PORTEVENT_SETSTATE		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_SETSTATE;
	return LoggingBit;
	}
(TTCN_)?PORTEVENT		{
	yylval.logseverity_val = TTCN_Logger::PORTEVENT_UNQUALIFIED;
	return LoggingBitCollection;
	}

STATISTICS_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::STATISTICS_UNQUALIFIED;
	return LoggingBit;
	}
STATISTICS_VERDICT		{
	yylval.logseverity_val = TTCN_Logger::STATISTICS_VERDICT;
	return LoggingBit;
	}
(TTCN_)?STATISTICS		{
	yylval.logseverity_val = TTCN_Logger::STATISTICS_UNQUALIFIED;
	return LoggingBitCollection;
	}

TESTCASE_FINISH		{
	yylval.logseverity_val = TTCN_Logger::TESTCASE_FINISH;
	return LoggingBit;
	}
TESTCASE_START		{
	yylval.logseverity_val = TTCN_Logger::TESTCASE_START;
	return LoggingBit;
	}
TESTCASE_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::TESTCASE_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?TESTCASE		{
	yylval.logseverity_val = TTCN_Logger::TESTCASE_UNQUALIFIED;
	return LoggingBitCollection;
	}

TIMEROP_GUARD		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_GUARD;
	return LoggingBit;
	}
TIMEROP_READ		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_READ;
	return LoggingBit;
	}
TIMEROP_START		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_START;
	return LoggingBit;
	}
TIMEROP_STOP		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_STOP;
	return LoggingBit;
	}
TIMEROP_TIMEOUT		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_TIMEOUT;
	return LoggingBit;
	}
TIMEROP_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?TIMEROP		{
	yylval.logseverity_val = TTCN_Logger::TIMEROP_UNQUALIFIED;
	return LoggingBitCollection;
	}

USER_UNQUALIFIED	{
	yylval.logseverity_val = TTCN_Logger::USER_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?USER	{
	yylval.logseverity_val = TTCN_Logger::USER_UNQUALIFIED;
	return LoggingBitCollection;
	}

VERDICTOP_FINAL		{
	yylval.logseverity_val = TTCN_Logger::VERDICTOP_FINAL;
	return LoggingBit;
	}
VERDICTOP_GETVERDICT		{
	yylval.logseverity_val = TTCN_Logger::VERDICTOP_GETVERDICT;
	return LoggingBit;
	}
VERDICTOP_SETVERDICT		{
	yylval.logseverity_val = TTCN_Logger::VERDICTOP_SETVERDICT;
	return LoggingBit;
	}
VERDICTOP_UNQUALIFIED		{
	yylval.logseverity_val = TTCN_Logger::VERDICTOP_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?VERDICTOP		{
	yylval.logseverity_val = TTCN_Logger::VERDICTOP_UNQUALIFIED;
	return LoggingBitCollection;
	}

WARNING_UNQUALIFIED	{
	yylval.logseverity_val = TTCN_Logger::WARNING_UNQUALIFIED;
	return LoggingBit;
	}
(TTCN_)?WARNING	{
	yylval.logseverity_val = TTCN_Logger::WARNING_UNQUALIFIED;
	return LoggingBitCollection;
	}

LOG_ALL	{
	yylval.logseverity_val = TTCN_Logger::LOG_ALL_IMPORTANT;
	return LoggingBitCollection;
	}

[Tt][Ii][Mm][Ee]	{
	yylval.timestamp_value = TTCN_Logger::TIMESTAMP_TIME;
	return TimestampValue;
	}
[Dd][Aa][Tt][Ee][Tt][Ii][Mm][Ee]	{
	yylval.timestamp_value = TTCN_Logger::TIMESTAMP_DATETIME;
	return TimestampValue;
	}
[Ss][Ee][Cc][Oo][Nn][Dd][Ss]	{
	yylval.timestamp_value = TTCN_Logger::TIMESTAMP_SECONDS;
	return TimestampValue;
	}
[Nn][Oo][Nn][Ee]	{
	yylval.source_info_value = TTCN_Logger::SINFO_NONE;
	return SourceInfoValue;
	}
[Ss][Ii][Nn][Gg][Ll][Ee]	{
	yylval.source_info_value = TTCN_Logger::SINFO_SINGLE;
	return SourceInfoValue;
	}
[Ss][Tt][Aa][Cc][Kk]	{
	yylval.source_info_value = TTCN_Logger::SINFO_STACK;
	return SourceInfoValue;
	}
[Yy][Ee][Ss]		{
	yylval.bool_val = TRUE;
	return YesNo;
	}
[Nn][Oo]		{
	yylval.bool_val = FALSE;
	return YesNo;
	}
[Bb]uffer[Aa]ll  {
  yylval.emergency_logging_behaviour_value = TTCN_Logger::BUFFER_ALL;
  return EmergencyLoggingBehaviourValue;
  }
 [Bb]uffer[Mm]asked  {
  yylval.emergency_logging_behaviour_value = TTCN_Logger::BUFFER_MASKED;
  return EmergencyLoggingBehaviourValue;
  }

[Cc]ompact            return Compact;
[Dd]etailed           return Detailed;
[Ss]ub[Cc]ategories   return SubCategories;

[Ee]rror   return Error;

[Ss]top    return Stop;

[Rr]etry   return Retry;

[Dd]elete  return Delete;
}

<SC_PROFILER>
{
  [Dd]isable[Pp]rofiler     return DisableProfilerKeyword;
  [Dd]isable[Cc]overage     return DisableCoverageKeyword;
  [Dd]ata[Bb]ase[Ff]ile     return DatabaseFileKeyword;
  [Aa]ggregate[Dd]ata       return AggregateDataKeyword;
  [Ss]tatistics[Ff]ile      return StatisticsFileKeyword;
  [Dd]isable[Ss]tatistics   return DisableStatisticsKeyword;
  [Ss]tatistics[Ff]ilter    return StatisticsFilterKeyword;
  [Ss]tart[Aa]utomatically  return StartAutomaticallyKeyword;
  [Nn]et[Ll]ine[Tt]imes     return NetLineTimesKeyword;
  [Nn]et[Ff]unction[Tt]imes return NetFunctionTimesKeyword;
  
  /* statistics filters */
  [Nn]umber[Oo]f[Ll]ines {
    yylval.uint_val = Profiler_Tools::STATS_NUMBER_OF_LINES;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Dd]ata[Rr]aw {
    yylval.uint_val = Profiler_Tools::STATS_LINE_DATA_RAW;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Dd]ata[Rr]aw {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_DATA_RAW;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Aa]vg[Rr]aw {
    yylval.uint_val = Profiler_Tools::STATS_LINE_AVG_RAW;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Aa]vg[Rr]aw {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_AVG_RAW;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Tt]imes[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_LINE_TIMES_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Tt]imes[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_TIMES_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Tt]imes[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_LINE_TIMES_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Tt]imes[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_TIMES_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Cc]ount[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_LINE_COUNT_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Cc]ount[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_COUNT_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Cc]ount[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_LINE_COUNT_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Cc]ount[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_COUNT_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Aa]vg[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_LINE_AVG_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Aa]vg[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_AVG_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Aa]vg[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_LINE_AVG_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Aa]vg[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_AVG_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ll]ine[Tt]imes {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_LINE_TIMES;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ff]unc[Tt]imes {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_FUNC_TIMES;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ll]ine[Cc]ount {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_LINE_COUNT;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ff]unc[Cc]ount {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_FUNC_COUNT;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ll]ine[Aa]vg {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_LINE_AVG;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ff]unc[Aa]vg {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_FUNC_AVG;
    return ProfilerStatsFlag;
  }
  [Uu]nused[Ll]ines {
    yylval.uint_val = Profiler_Tools::STATS_UNUSED_LINES;
    return ProfilerStatsFlag;
  }
  [Uu]nused[Ff]unc {
    yylval.uint_val = Profiler_Tools::STATS_UNUSED_FUNC;
    return ProfilerStatsFlag;
  }
  [Aa]ll[Rr]aw[Dd]ata {
    yylval.uint_val = Profiler_Tools::STATS_ALL_RAW_DATA;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Dd]ata[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_LINE_DATA_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Dd]ata[Ss]orted[Bb]y[Mm]od {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_DATA_SORTED_BY_MOD;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Dd]ata[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_LINE_DATA_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Dd]ata[Ss]orted[Tt]otal {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_DATA_SORTED_TOTAL;
    return ProfilerStatsFlag;
  }
  [Ll]ine[Dd]ata[Ss]orted {
    yylval.uint_val = Profiler_Tools::STATS_LINE_DATA_SORTED;
    return ProfilerStatsFlag;
  }
  [Ff]unc[Dd]ata[Ss]orted {
    yylval.uint_val = Profiler_Tools::STATS_FUNC_DATA_SORTED;
    return ProfilerStatsFlag;
  }
  [Aa]ll[Dd]ata[Ss]orted {
    yylval.uint_val = Profiler_Tools::STATS_ALL_DATA_SORTED;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ll]ine[Dd]ata {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_LINE_DATA;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Ff]unc[Dd]ata {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_FUNC_DATA;
    return ProfilerStatsFlag;
  }
  [Tt]op10[Aa]ll[Dd]ata {
    yylval.uint_val = Profiler_Tools::STATS_TOP10_ALL_DATA;
    return ProfilerStatsFlag;
  }
  [Uu]nused[Dd]ata {
    yylval.uint_val = Profiler_Tools::STATS_UNUSED_DATA;
    return ProfilerStatsFlag;
  }
  [Aa]ll {
    yylval.uint_val = Profiler_Tools::STATS_ALL;
    return ProfilerStatsFlag;
  }
}

<SC_EXECUTE>control		return ControlKeyword;

<SC_EXTERNAL_COMMANDS>
{
[Bb]egin[Cc]ontrol[Pp]art	return BeginControlPart;
[Ee]nd[Cc]ontrol[Pp]art		return EndControlPart;
[Bb]egin[Tt]est[Cc]ase		return BeginTestCase;
[Ee]nd[Tt]est[Cc]ase		return EndTestCase;
}

<SC_MAIN_CONTROLLER>
{
[Ll]ocal[Aa]ddress	return LocalAddress;
[Tt][Cc][Pp][Pp]ort	return TCPPort;
[Kk]ill[Tt]imer		return KillTimer;
[Nn]um[Hh][Cc]s		return NumHCs;
[Uu]nix[Ss]ockets[Ee]nabled return UnixSocketEnabled;
[Yy][Ee][Ss]            return YesToken;
[Nn][Oo]                return NoToken;
}

{TTCN3IDENTIFIER}	{
    yylval.str_val = mcopystr(yytext);
    return Identifier;
}

<SC_MODULE_PARAMETERS>{ASN1LOWERIDENTIFIER}	{
    yylval.str_val = mcopystr(yytext);
    for (size_t i = 0; i < yyleng; i++) {
	if (yylval.str_val[i] == '-') yylval.str_val[i] = '_';
    }
    TTCN_warning("In line %d of configuration file: `%s' is not a valid TTCN-3 "
	"identifier. Did you mean `%s'?", current_line, yytext, yylval.str_val);
    return ASN1LowerIdentifier;
}

<SC_GROUPS,SC_COMPONENTS,SC_MAIN_CONTROLLER>{DNSNAME}|{IPV6} return DNSName;
  /* Information is discarded ! */

<SC_STRING2TTCN_COMPONENT>{COMPONENT} {
  /* Ignore the component name, just return its number */
  size_t len = strlen(yytext);
  size_t pos = len - 1;
  while(yytext[pos] != '(' && pos != 0) {
    --pos;
  }
  char* comp_num_str = mcopystrn(yytext + pos + 1, len - pos - 1);
  yylval.int_val = new int_val_t(comp_num_str);
  Free(comp_num_str);
  return MPNumber;
}

{MACRO_BOOL} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (!strcmp(macrovalue, "true")) yylval.bool_val = TRUE;
      else if (!strcmp(macrovalue, "false")) yylval.bool_val = FALSE;
      else {
	config_process_error_f("Macro `%s' cannot be interpreted as boolean "
	  "value: `%s'", macroname, macrovalue);
	yylval.bool_val = FALSE;
      }
    } else {
      config_process_error_f("No macro or environmental variable defined"
	" with name `%s'", macroname);
      yylval.bool_val = FALSE;
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.bool_val = FALSE;
  }
  return BooleanValue;
}

{MACRO_INT} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (string_is_int(macrovalue, macrolen))
	yylval.int_val = new int_val_t(macrovalue);
      else {
	config_process_error_f("Macro `%s' cannot be interpreted as integer "
	  "value: `%s'", macroname, macrovalue);
	yylval.int_val = new int_val_t((RInt)0);
      }
    } else {
      config_process_error_f("No macro or environmental variable defined"
	" with name `%s'", macroname);
      yylval.int_val = new int_val_t((RInt)0);
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.int_val = new int_val_t((RInt)0);
  }
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    // return a different token for module parameters so it doesn't conflict with references
    return MPNumber;
  }
  return Number;
}

{MACRO_FLOAT} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (string_is_float(macrovalue, macrolen))
	yylval.float_val = atof(macrovalue);
      else {
	config_process_error_f("Macro `%s' cannot be interpreted as float value: "
	  "`%s'", macroname, macrovalue);
	yylval.float_val = 0.0;
      }
    } else {
      config_process_error_f("No macro or environmental variable defined"
	" with name `%s'", macroname);
      yylval.float_val = 0.0;
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.float_val = 0.0;
  }
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    // return a different token for module parameters so it doesn't conflict with references
    return MPFloat;
  }
  return Float;
}

{MACRO_ID} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    boolean is_asn = FALSE;
    if (macrovalue != NULL) {
      if (string_is_id(macrovalue, macrolen)) {
	yylval.str_val = mcopystr(macrovalue);
	for (size_t i = 0; i < macrolen; i++) {
	  if (yylval.str_val[i]=='-') {
	    yylval.str_val[i] = '_';
	    is_asn = TRUE;
	  }
	}
	if (is_asn)
          TTCN_warning("In line %d of configuration file: `%s' is not a valid"
                       " TTCN-3 identifier. Did you mean `%s'?",
                       current_line, macrovalue, yylval.str_val);
      } else {
	config_process_error_f("Macro `%s' cannot be interpreted as identifier: "
	  "`%s'", macroname, macrovalue);
	yylval.str_val = memptystr();
      }
    } else {
      config_process_error_f("No macro or environmental variable defined with "
	"name `%s'", macroname);
      yylval.str_val = memptystr();
    }
    Free(macroname);
    return is_asn ? ASN1LowerIdentifier : Identifier;
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.str_val = memptystr();
    return Identifier;
  }
}

{MACRO_CSTR} {
  if (config_defines == NULL) {
    config_process_error("Internal error: Macro reference cannot be used in "
        "this context.");
    yylval.charstring_val.n_chars = 0;
    yylval.charstring_val.chars_ptr = memptystr();
    if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
      // return a different token for module parameters so it doesn't conflict with references
      return MPCstring;
    }
    return Cstring;
  }

  char *macroname;
  if (yytext[1] == '{') macroname = get_macro_id_from_ref(yytext);
  else macroname = mcopystr(yytext + 1);
  size_t macrolen;
  const char *macrovalue = string_map_get_bykey
    (config_defines, macroname, &macrolen);

  if (macrovalue == NULL) {
    config_process_error_f("No macro or environmental variable defined with name "
        "`%s'", macroname);
    yylval.charstring_val.n_chars=0;
    yylval.charstring_val.chars_ptr=memptystr();
    Free(macroname);
    if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
      // return a different token for module parameters so it doesn't conflict with references
      return MPCstring;
    }
    return Cstring;
  }

  if (macrolen > 0 && macrovalue[0] == '{') { // structured
    main_buffer = YY_CURRENT_BUFFER;
    expansion_buffer = yy_scan_string(macrovalue);
    yy_switch_to_buffer(expansion_buffer);
    Free(macroname);
  } else {
    yylval.charstring_val.n_chars=macrolen;
    yylval.charstring_val.chars_ptr=(char*)Malloc(macrolen+1);
    memcpy(yylval.charstring_val.chars_ptr, macrovalue, macrolen+1);
    Free(macroname);
    if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
      // return a different token for module parameters so it doesn't conflict with references
      return MPCstring;
    }
    return Cstring;
  }
}

{MACRO_BSTR} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (string_is_bstr(macrovalue, macrolen)) {
	yylval.bitstring_val.n_bits = macrolen;
	int n_bytes = (yylval.bitstring_val.n_bits + 7) / 8;
	yylval.bitstring_val.bits_ptr = (unsigned char *)Malloc(n_bytes);
	memset(yylval.bitstring_val.bits_ptr, 0, n_bytes);
	for (int i = 0; i < yylval.bitstring_val.n_bits; i++)
          if (macrovalue[i] == '1')
            yylval.bitstring_val.bits_ptr[i/8] |= 1 << (i % 8);
      } else {
	config_process_error_f("Macro `%s' cannot be interpreted as bitstring "
	  "value: `%s'", macroname, macrovalue);
	yylval.bitstring_val.n_bits = 0;
	yylval.bitstring_val.bits_ptr = NULL;
      }
    } else {
      config_process_error_f("No macro or environmental variable defined with "
        "name `%s'", macroname);
      yylval.bitstring_val.n_bits = 0;
      yylval.bitstring_val.bits_ptr = NULL;
    }
    Free(macroname);
  } else {
    config_process_error_f("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.bitstring_val.n_bits = 0;
    yylval.bitstring_val.bits_ptr = NULL;
  }
  return Bstring;
}

{MACRO_HSTR} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if(string_is_hstr(macrovalue, macrolen)) {
	yylval.hexstring_val.n_nibbles = macrolen;
	int n_bytes = (yylval.hexstring_val.n_nibbles + 1) / 2;
	yylval.hexstring_val.nibbles_ptr = (unsigned char *)Malloc(n_bytes);
	memset(yylval.hexstring_val.nibbles_ptr, 0, n_bytes);
	for (int i = 0; i < yylval.hexstring_val.n_nibbles; i++) {
          unsigned int hex_digit;
          sscanf(macrovalue+i, "%1x", &hex_digit);
          if(i % 2) yylval.hexstring_val.nibbles_ptr[i/2] |=
            (hex_digit << 4);
          else yylval.hexstring_val.nibbles_ptr[i/2] |= hex_digit;
	}
      } else {
	config_process_error_f("Macro `%s' cannot be interpreted as hexstring "
	  "value: `%s'", macroname, macrovalue);
	yylval.hexstring_val.n_nibbles = 0;
	yylval.hexstring_val.nibbles_ptr = NULL;
      }
    } else {
      config_process_error_f("No macro or environmental variable defined with "
	"name `%s'", macroname);
      yylval.hexstring_val.n_nibbles = 0;
      yylval.hexstring_val.nibbles_ptr = NULL;
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.hexstring_val.n_nibbles = 0;
    yylval.hexstring_val.nibbles_ptr = NULL;
  }
  return Hstring;
}

{MACRO_OSTR} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (string_is_ostr(macrovalue, macrolen)) {
	yylval.octetstring_val.n_octets = macrolen / 2;
	yylval.octetstring_val.octets_ptr = (unsigned char *)
          Malloc(yylval.octetstring_val.n_octets);
	for (int i = 0; i < yylval.octetstring_val.n_octets; i++) {
          unsigned int this_octet;
          sscanf(macrovalue+2*i, "%2x", &this_octet);
          yylval.octetstring_val.octets_ptr[i] = this_octet;
	}
      } else {
	config_process_error_f("Macro `%s' cannot be interpreted as octetstring "
	  "value: `%s'", macroname, macrovalue);
	yylval.octetstring_val.n_octets = 0;
	yylval.octetstring_val.octets_ptr = NULL;
      }
    } else {
      config_process_error_f("No macro or environmental variable defined with "
	"name `%s'", macroname);
      yylval.octetstring_val.n_octets = 0;
      yylval.octetstring_val.octets_ptr = NULL;
    }
    Free(macroname);
  } else {
    config_process_error_f("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.octetstring_val.n_octets = 0;
    yylval.octetstring_val.octets_ptr = NULL;
  }
  return Ostring;
}

{MACRO_BINARY} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      yylval.octetstring_val.n_octets=macrolen;
      yylval.octetstring_val.octets_ptr = (unsigned char*)Malloc(macrolen);
      memcpy(yylval.octetstring_val.octets_ptr, macrovalue, macrolen);
    }
    else {
      config_process_error_f("No macro or environmental variable defined with "
	"name `%s'", macroname);
      yylval.octetstring_val.n_octets = 0;
      yylval.octetstring_val.octets_ptr = NULL;
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
    yylval.octetstring_val.n_octets = 0;
    yylval.octetstring_val.octets_ptr = NULL;
  }
  return Ostring;
}

{MACRO_HOSTNAME} {
  if (config_defines != NULL) {
    char *macroname = get_macro_id_from_ref(yytext);
    size_t macrolen;
    const char *macrovalue =
      string_map_get_bykey(config_defines, macroname, &macrolen);
    if (macrovalue != NULL) {
      if (!string_is_hostname(macrovalue, macrolen)) {
	config_process_error_f("Macro `%s' cannot be interpreted as host name: "
	  "`%s'", macroname, macrovalue);
      }
    } else {
      config_process_error_f("No macro or environmental variable defined with "
	"name `%s'", macroname);
    }
    Free(macroname);
  } else {
    config_process_error("Internal error: Macro reference cannot be used in "
      "this context.");
  }
  return DNSName;
}

":="|"="    return AssignmentChar;
"&="        return ConcatChar;

<<EOF>> {
   if (expansion_buffer) {
     yy_switch_to_buffer(main_buffer);
     yy_delete_buffer(expansion_buffer);
     expansion_buffer = NULL;
   } else {
     if (include_chain->size() > 1) {
       yy_delete_buffer(YY_CURRENT_BUFFER);
       fclose(include_chain->back().fp);
       include_chain->pop_back();
       yy_switch_to_buffer(include_chain->back().buffer_state);
       current_line = include_chain->back().line_number;
       BEGIN(SC_ORDERED_INCLUDE);
     } else {
       yyterminate();
       return EOF;
     }
   }
 }



.       return yytext[0];


%%

void reset_config_process_lex(const char* fname)
{
  if (!include_chain) {
    include_chain = new std::deque<IncludeElem<YY_BUFFER_STATE> >();
  }
  BEGIN(INITIAL);
  current_line = 1;
  if (fname) {
    include_chain->push_back(IncludeElem<YY_BUFFER_STATE>(std::string(fname), config_process_in));
  } 
}

void config_process_close() {
  delete include_chain;
  include_chain = NULL;
}

int config_process_get_current_line()
{
  return current_line;
}



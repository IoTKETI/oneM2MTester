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
 *   Gecse, Roland
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
%option yylineno
%option noyywrap
%option never-interactive
%option nounput

%{

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <deque>
#include <string>

#include <openssl/crypto.h>
#include <openssl/bn.h>

#include "../../common/cfg_process_utils.hh"
#include "../../common/Path2.hh"
#include "../../common/config_preproc.h"

#include "../../common/memory.h"
#include "../mctr/config_data.h"
#include "../../core/Types.h"
#include "config_read.tab.hh"

//#include "../../common/dbgnew.hh"

#ifndef INFINITY
#include <float.h>
static const double INFINITY = (DBL_MAX*DBL_MAX);
#endif

extern string_map_t *config_defines;

#define yylval config_read_lval

/* This buffer stores the state of the main buffer
   while the macro expansion is happening. */
static YY_BUFFER_STATE main_buffer      = NULL;
/* This buffer is active while the expansion of 
   a reference to a structured macro definition is happening.
   Otherwise its value is NULL. */
static YY_BUFFER_STATE expansion_buffer = NULL;

static void update_buffer();
static boolean whether_update_buffer();

#define RETURN(x) do {update_buffer(); return (x);} while(0)

void config_read_warning(const char *warning_str, ...);
extern void config_read_error(const char *error_str, ...);
extern config_data *cfg;

static std::deque<IncludeElem<YY_BUFFER_STATE> >* include_chain = NULL;

std::string get_cfg_read_current_file() {
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

MACRORVALUE ([0-9A-Za-z._-]+)|{IPV6}
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

CHAR [^\\\"]|\\([\\\'\"\?abfnrtv]|{NEWLINE}|[0-7]{1,3}|x{HEX}{1,2})|\"\"
CHARSTRING \"{CHAR}*\"

HOSTNAME [A-Za-z0-9]([A-Za-z0-9_\-]*[A-Za-z0-9])?
DNSNAME {HOSTNAME}(\.{HOSTNAME})*\.?

/* Example: fe80::c002:37ff:fe6c:0%fastethernet0/0 */
IPV6 [0-9A-Fa-f:.]+(%[0-9A-Za-z]+)?

UID [uU][+]?[0-9A-Fa-f]{1,8}

%x SC_blockcomment SC_DEFINE SC_CSTRING SC_ORDERED_INCLUDE
%s SC_MODULE_PARAMETERS SC_LOGGING SC_TESTPORT_PARAMETERS SC_EXECUTE SC_GROUPS
%s SC_COMPONENTS SC_EXTERNAL_COMMANDS SC_MAIN_CONTROLLER SC_INCLUDE SC_PROFILER
%s SC_CHAR_KEYWORD

%%
	int comment_caller = INITIAL;

  std::string cstring;

	/* Eat up comments and whitespaces */

"/*"	{
		comment_caller = YY_START;
		BEGIN(SC_blockcomment);
	}
<SC_blockcomment>
{
"*/"		BEGIN(comment_caller);
.|\n
}

{LINECOMMENT}|{WHITESPACE}|{NEWLINE}

	/* Section delimiters */

<*>"["{WS}MODULE_PARAMETERS{WS}"]"	{
  if (YY_START!=SC_blockcomment) {
    BEGIN(SC_MODULE_PARAMETERS);
    RETURN(ModuleParametersKeyword);
  }
}

<*>"["{WS}LOGGING{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
    BEGIN(SC_LOGGING);
    RETURN(LoggingKeyword);
	}
}

<*>"["{WS}PROFILER{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
    BEGIN(SC_PROFILER);
    RETURN(ProfilerKeyword);
	}
}

<*>"["{WS}TESTPORT_PARAMETERS{WS}"]"	{
  if (YY_START!=SC_blockcomment) {
    BEGIN(SC_TESTPORT_PARAMETERS);
    RETURN(TestportParametersKeyword);
	}
}

<*>"["{WS}EXECUTE{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_EXECUTE);
	  RETURN(ExecuteKeyword);
	}
}

<*>"["{WS}EXTERNAL_COMMANDS{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_EXTERNAL_COMMANDS);
	  RETURN(ExternalCommandsKeyword);
	}
}

<*>"["{WS}GROUPS{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_GROUPS);
	  RETURN(GroupsKeyword);
	}
}

<*>"["{WS}COMPONENTS{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_COMPONENTS);
	  RETURN(ComponentsKeyword);
	}
}

<*>"["{WS}MAIN_CONTROLLER{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_MAIN_CONTROLLER);
	  RETURN(MainControllerKeyword);
	}
}

<*>"["{WS}INCLUDE{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_INCLUDE);
	  RETURN(IncludeKeyword);
	}
}

<*>"["{WS}ORDERED_INCLUDE{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_ORDERED_INCLUDE);
	}
}


<*>"["{WS}DEFINE{WS}"]"		{
  if (YY_START!=SC_blockcomment) {
  	BEGIN(SC_DEFINE);
	  RETURN(DefineKeyword);
	}
}

	/* Rules for SC_DEFINE must precede everything else */

<SC_DEFINE>{

"/*"	{
		comment_caller = SC_DEFINE;
		BEGIN(SC_blockcomment);
}

{CHARSTRING} /* eats string */

{LINECOMMENT}

{NEWLINE} /* this is not in . */

\\\"

. /* eats unnecessary things */

}

<SC_ORDERED_INCLUDE>{

"/*"	{
		comment_caller = YY_START;
		BEGIN(SC_blockcomment);
	}

\" { BEGIN(SC_CSTRING); }

{LINECOMMENT}

{NEWLINE} /* this is not in . */

\\\"

. /* eat unnecessary chars */

}


<SC_CSTRING>{

\"\" cstring += '"';

\" {
  std::string error_msg = switch_lexer(include_chain, cstring,
      YY_CURRENT_BUFFER, yy_create_buffer, yy_switch_to_buffer, yylineno,
      YY_BUF_SIZE);
  if (error_msg.empty()) {
    BEGIN(INITIAL);
  } else {
    config_read_error(error_msg.c_str());
  }

  cstring.clear();
}  /* end of string */

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
  if (c <= 255) cstring += c;
  else config_read_error("Invalid octal character code in string literal");
}

\\x{HEX}{1,2} {
  unsigned int c;
  sscanf(yytext + 2, "%x", &c);
  cstring += c;
}

\\(x[^\\\"]|.) config_read_error("Invalid excape sequence in string literal.");

{NEWLINE} {
  cstring.append(yytext, yyleng);
  yylineno++;
}

. { 
  cstring += yytext[0];
}
}
  /* SC_cstring */


<SC_PROFILER>{HEX}+ {
  /* numeric statistics filter (check this before checking NUMBERs) */
  RETURN(ProfilerStatsFlag);
}


	/* Values */

{NUMBER} {
	yylval.int_val = NULL;
  BN_dec2bn(&yylval.int_val, *yytext == '+' ? yytext + 1 : yytext);
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    RETURN(MPNumber);
  }
	RETURN(Number);
}

{FLOAT}	{
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
    RETURN(MPFloat);
  }
	RETURN(Float);
}

{BITSTRING}	RETURN(Bstring);

{BITSTRINGMATCH} RETURN(BstringMatch);

{BITSTRING_BAD}	{
	config_read_error("Invalid bitstring value.");
	RETURN(Bstring);
}

{HEXSTRING}	RETURN(Hstring);

{HEXSTRINGMATCH} RETURN(HstringMatch);

{HEXSTRING_BAD}	{
	config_read_error("Invalid hexstring value.");
	RETURN(Hstring);
}

{OCTETSTRING}	RETURN(Ostring);

{OCTETSTRINGMATCH} RETURN(OstringMatch);

{OCTETSTRING_BAD}	{
	config_read_error("Invalid octetstring value.");
	RETURN(Ostring);
}

{BINSTRING_BAD}	config_read_error("Invalid string value.");

'	config_read_error("Unmatched ' character.");

{CHARSTRING}	{
  yylval.str_val = mcopystrn(yytext, yyleng);
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    RETURN(MPCstring);
  }
  RETURN(Cstring);
}

<SC_CHAR_KEYWORD>
{
  {UID} {
    yylval.str_val = mcopystrn(yytext, yyleng);
    RETURN(Cstring);
  }

  [,] { RETURN(*yytext); }

  [)] { BEGIN(SC_MODULE_PARAMETERS); RETURN(*yytext); }
}

	/* Section-wide keywords */

<SC_MODULE_PARAMETERS>
{
NULL	RETURN(NULLKeyword);
null	RETURN(nullKeyword);
char	{ BEGIN(SC_CHAR_KEYWORD); RETURN(CharKeyword); }
objid	RETURN(ObjIdKeyword);
omit	RETURN(OmitKeyword);
none	|
pass	|
inconc	|
fail	|
error	RETURN(VerdictValue);
complement  RETURN(ComplementKeyword);
"\.\."      RETURN(DotDot);
superset    RETURN(SupersetKeyword);
subset      RETURN(SubsetKeyword);
pattern     RETURN(PatternKeyword);
permutation RETURN(PermutationKeyword);
length      RETURN(LengthKeyword);
ifpresent   RETURN(IfpresentKeyword);
infinity    RETURN(InfinityKeyword);
"@nocase"   RETURN(NocaseKeyword);
}

<SC_MODULE_PARAMETERS,SC_LOGGING,SC_PROFILER>
{
true	|
false	RETURN(BooleanValue);
}

<SC_MODULE_PARAMETERS,SC_LOGGING,SC_TESTPORT_PARAMETERS>
{
mtc	RETURN(MTCKeyword);
system	RETURN(SystemKeyword);
}

<SC_LOGGING>
{
[Ff]ile[Nn]ame			|
[Ll]og[Ff]ile			RETURN(LogFile);

[Ee]mergency[Ll]ogging     RETURN(EmergencyLogging);

[Ee]mergency[Ll]ogging[Bb]ehaviour     RETURN(EmergencyLoggingBehaviour);

[Bb]uffer[Aa]ll      |
[Bb]uffer[Ma]asked         RETURN(EmergencyLoggingBehaviourValue);

[Ee]mergency[Ll]ogging[Mm]ask     RETURN(EmergencyLoggingMask);

[Ee]mergency[Ll]ogging[Ff]or[Ff]ail[Vv]erdict  RETURN(EmergencyLoggingForFailVerdict);

[Ff]ile[Mm]ask			RETURN(FileMask);

[Cc]onsole[Mm]ask		RETURN(ConsoleMask);

[Tt]ime[Ss]tamp[Ff]ormat	RETURN(TimestampFormat);
[Cc]onsole[Tt]ime[Ss]tamp[Ff]ormat	RETURN(ConsoleTimestampFormat);

[Ll]og[Ss]ource[Ii]nfo		|
[Ss]ource[Ii]nfo[Ff]ormat	RETURN(SourceInfoFormat);

[Aa]ppend[Ff]ile		RETURN(AppendFile);

[Ll]og[Ee]vent[Tt]ypes		RETURN(LogEventTypes);

[Ll]og[Ee]ntity[Nn]ame		RETURN(LogEntityName);

[Ll]og[Ff]ile[Ss]ize            RETURN(LogFileSize);

[Ll]og[Ff]ile[Nn]umber          RETURN(LogFileNumber);

[Dd]isk[Ff]ull[Aa]ction         RETURN(DiskFullAction);

[Mm]atching[Hh]ints             RETURN(MatchingHints);

[Ll]ogger[Pp]lugins RETURN(LoggerPlugins);

LOG_NOTHING RETURN(LoggingBit);

(TTCN_)?ACTION |
(TTCN_)?DEBUG |
(TTCN_)?DEFAULTOP |
(TTCN_)?ERROR |
(TTCN_)?EXECUTOR |
(TTCN_)?FUNCTION |
(TTCN_)?MATCHING |
(TTCN_)?PARALLEL |
(TTCN_)?PORTEVENT |
(TTCN_)?STATISTICS |
(TTCN_)?TESTCASE |
(TTCN_)?TIMEROP |
(TTCN_)?USER |
(TTCN_)?VERDICTOP |
(TTCN_)?WARNING |
LOG_ALL RETURN(LoggingBitCollection);

ACTION_UNQUALIFIED |
DEBUG_ENCDEC |
DEBUG_FRAMEWORK |
DEBUG_TESTPORT |
DEBUG_UNQUALIFIED |
DEBUG_USER |
DEFAULTOP_ACTIVATE |
DEFAULTOP_DEACTIVATE |
DEFAULTOP_EXIT |
DEFAULTOP_UNQUALIFIED |
ERROR_UNQUALIFIED |
EXECUTOR_COMPONENT |
EXECUTOR_CONFIGDATA |
EXECUTOR_EXTCOMMAND |
EXECUTOR_LOGOPTIONS |
EXECUTOR_RUNTIME |
EXECUTOR_UNQUALIFIED |
FUNCTION_RND |
FUNCTION_UNQUALIFIED |
MATCHING_DONE |
MATCHING_MCSUCCESS |
MATCHING_MCUNSUCC |
MATCHING_MMSUCCESS |
MATCHING_MMUNSUCC |
MATCHING_PCSUCCESS |
MATCHING_PCUNSUCC |
MATCHING_PMSUCCESS |
MATCHING_PMUNSUCC |
MATCHING_PROBLEM |
MATCHING_TIMEOUT |
MATCHING_UNQUALIFIED |
PARALLEL_PORTCONN |
PARALLEL_PORTMAP |
PARALLEL_PTC |
PARALLEL_UNQUALIFIED |
PORTEVENT_DUALRECV |
PORTEVENT_DUALSEND |
PORTEVENT_MCRECV |
PORTEVENT_MCSEND |
PORTEVENT_MMRECV |
PORTEVENT_MMSEND |
PORTEVENT_MQUEUE |
PORTEVENT_PCIN |
PORTEVENT_PCOUT |
PORTEVENT_PMIN |
PORTEVENT_PMOUT |
PORTEVENT_PQUEUE |
PORTEVENT_STATE |
PORTEVENT_UNQUALIFIED |
PORTEVENT_SETSTATE |
STATISTICS_UNQUALIFIED |
STATISTICS_VERDICT |
TESTCASE_FINISH |
TESTCASE_START |
TESTCASE_UNQUALIFIED |
TIMEROP_GUARD |
TIMEROP_READ |
TIMEROP_START |
TIMEROP_STOP |
TIMEROP_TIMEOUT |
TIMEROP_UNQUALIFIED |
USER_UNQUALIFIED |
VERDICTOP_FINAL |
VERDICTOP_GETVERDICT |
VERDICTOP_SETVERDICT |
VERDICTOP_UNQUALIFIED |
WARNING_UNQUALIFIED RETURN(LoggingBit);

[Tt][Ii][Mm][Ee]	                {yylval.ts_val=TSF_TIME; RETURN(TimestampValue);}
[Dd][Aa][Tt][Ee][Tt][Ii][Mm][Ee]  {yylval.ts_val=TSF_DATE_TIME; RETURN(TimestampValue);}
[Ss][Ee][Cc][Oo][Nn][Dd][Ss]		  {yylval.ts_val=TSF_SEC; RETURN(TimestampValue);}

[Nn][Oo][Nn][Ee]		|
[Ss][Ii][Nn][Gg][Ll][Ee]	|
[Ss][Tt][Aa][Cc][Kk]		RETURN(SourceInfoValue);

[Yy][Ee][Ss]	|
[Nn][Oo]	RETURN(YesNo);

[Dd]etailed		RETURN(Detailed);
[Cc]ompact		RETURN(Compact);
[Ss]ub[Cc]ategories	RETURN(SubCategories);

[Ee]rror  RETURN(Error);

[Ss]top   RETURN(Stop);

[Rr]etry  RETURN(Re_try);

[Dd]elete RETURN(Delete);
}

<SC_PROFILER>
{
  [Dd]isable[Pp]rofiler     RETURN(DisableProfilerKeyword);
  [Dd]isable[Cc]overage     RETURN(DisableCoverageKeyword);
  [Dd]ata[Bb]ase[Ff]ile     RETURN(DatabaseFileKeyword);
  [Aa]ggregate[Dd]ata       RETURN(AggregateDataKeyword);
  [Ss]tatistics[Ff]ile      RETURN(StatisticsFileKeyword);
  [Dd]isable[Ss]tatistics   RETURN(DisableStatisticsKeyword);
  [Ss]tatistics[Ff]ilter    RETURN(StatisticsFilterKeyword);
  [Ss]tart[Aa]utomatically  RETURN(StartAutomaticallyKeyword);
  [Nn]et[Ll]ine[Tt]imes     RETURN(NetLineTimesKeyword);
  [Nn]et[Ff]unction[Tt]imes RETURN(NetFunctionTimesKeyword);
  
  /* statistics filters */
  [Nn]umber[Oo]f[Ll]ines |
  [Ll]ine[Dd]ata[Rr]aw |
  [Ff]unc[Dd]ata[Rr]aw |
  [Ll]ine[Aa]vg[Rr]aw |
  [Ff]unc[Aa]vg[Rr]aw |
  [Ll]ine[Tt]imes[Ss]orted[Bb]y[Mm]od |
  [Ff]unc[Tt]imes[Ss]orted[Bb]y[Mm]od |
  [Ll]ine[Tt]imes[Ss]orted[Tt]otal |
  [Ff]unc[Tt]imes[Ss]orted[Tt]otal |
  [Ll]ine[Cc]ount[Ss]orted[Bb]y[Mm]od |
  [Ff]unc[Cc]ount[Ss]orted[Bb]y[Mm]od |
  [Ll]ine[Cc]ount[Ss]orted[Tt]otal |
  [Ff]unc[Cc]ount[Ss]orted[Tt]otal |
  [Ll]ine[Aa]vg[Ss]orted[Bb]y[Mm]od |
  [Ff]unc[Aa]vg[Ss]orted[Bb]y[Mm]od |
  [Ll]ine[Aa]vg[Ss]orted[Tt]otal |
  [Ff]unc[Aa]vg[Ss]orted[Tt]otal |
  [Tt]op10[Ll]ine[Tt]imes |
  [Tt]op10[Ff]unc[Tt]imes |
  [Tt]op10[Ll]ine[Cc]ount |
  [Tt]op10[Ff]unc[Cc]ount |
  [Tt]op10[Ll]ine[Aa]vg |
  [Tt]op10[Ff]unc[Aa]vg |
  [Uu]nused[Ll]ines |
  [Uu]nused[Ff]unc |
  [Aa]ll[Rr]aw[Dd]ata |
  [Ll]ine[Dd]ata[Ss]orted[Bb]y[Mm]od |
  [Ff]unc[Dd]ata[Ss]orted[Bb]y[Mm]od |
  [Ll]ine[Dd]ata[Ss]orted[Tt]otal |
  [Ff]unc[Dd]ata[Ss]orted[Tt]otal |
  [Ll]ine[Dd]ata[Ss]orted |
  [Ff]unc[Dd]ata[Ss]orted |
  [Aa]ll[Dd]ata[Ss]orted |
  [Tt]op10[Ll]ine[Dd]ata |
  [Tt]op10[Ff]unc[Dd]ata |
  [Tt]op10[Aa]ll[Dd]ata |
  [Uu]nused[Dd]ata |
  [Aa]ll RETURN(ProfilerStatsFlag);
}

<SC_EXECUTE>control		RETURN(ControlKeyword);

<SC_EXTERNAL_COMMANDS>
{
[Bb]egin[Cc]ontrol[Pp]art	RETURN(BeginControlPart);
[Ee]nd[Cc]ontrol[Pp]art		RETURN(EndControlPart);
[Bb]egin[Tt]est[Cc]ase		RETURN(BeginTestCase);
[Ee]nd[Tt]est[Cc]ase		RETURN(EndTestCase);
}

<SC_MAIN_CONTROLLER>
{
[Ll]ocal[Aa]ddress	RETURN(LocalAddress);
[Tt][Cc][Pp][Pp]ort	RETURN(TCPPort);
[Kk]ill[Tt]imer		RETURN(KillTimer);
[Nn]um[Hh][Cc]s		RETURN(NumHCs);
[Uu]nix[Ss]ockets[Ee]nabled RETURN(UnixSocketEnabled);
[Yy][Ee][Ss] RETURN(YesToken);
[Nn][Oo] RETURN(NoToken);
}

{TTCN3IDENTIFIER}	{
    switch (YY_START) {
    case SC_GROUPS:
    case SC_COMPONENTS:
    case SC_EXECUTE:
    case SC_MAIN_CONTROLLER:
	yylval.str_val = (char*)Malloc(yyleng + 1);
	memcpy(yylval.str_val, yytext, yyleng + 1);
	break;
    default:
	yylval.str_val = NULL;
    }
    RETURN(Identifier);
}

<SC_MODULE_PARAMETERS>{ASN1LOWERIDENTIFIER} {
  char *ttcn3_id = (char*)Malloc(yyleng + 1);
  for (size_t i = 0; i < yyleng; i++) {
    if (yytext[i] == '-') ttcn3_id[i] = '_';
    else ttcn3_id[i] = yytext[i];
  }
  ttcn3_id[yyleng] = '\0';
  config_read_warning("`%s' is not a valid TTCN-3 identifier. Did you mean "
    "`%s'?", yytext, ttcn3_id);
  cfg->config_read_buffer = mputprintf(cfg->config_read_buffer, "%s ", ttcn3_id);
  Free(ttcn3_id);
  return ASN1LowerIdentifier;
}

<SC_GROUPS,SC_COMPONENTS,SC_MAIN_CONTROLLER>{DNSNAME}|{IPV6} {
  yylval.str_val = (char*)Malloc(yyleng + 1);
  for (size_t i = 0; i < yyleng; i++) yylval.str_val[i] = tolower(yytext[i]);
  yylval.str_val[yyleng] = '\0';
  RETURN(DNSName);
}

{MACRO_BOOL} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  boolean bool_val;
  if (macrovalue != NULL) {
    if (!strcmp(macrovalue, "true")) bool_val = TRUE;
    else if (!strcmp(macrovalue, "false")) bool_val = FALSE;
    else {
      config_read_error("Macro `%s' cannot be interpreted as boolean value: "
	"`%s'", macroname, macrovalue);
      bool_val = FALSE;
    }
  } else {
    config_read_error("No macro or environmental variable defined with name "
      "`%s'", macroname);
    bool_val = FALSE;
  }
  Free(macroname);
  if (whether_update_buffer()) cfg->config_read_buffer =
    mputprintf(cfg->config_read_buffer, "%s ", bool_val ? "true" : "false");
  return BooleanValue;
}

{MACRO_INT} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen = 0;
  BIGNUM *BN_0 = BN_new();
  BN_set_word(BN_0, 0);
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_int(macrovalue, macrolen)) {
      yylval.int_val = NULL;
      BN_dec2bn(&yylval.int_val,
                *macrovalue == '+' ? macrovalue + 1 : macrovalue);
    } else {
      config_read_error("Macro `%s' cannot be interpreted as integer value: "
                        "`%s'", macroname, macrovalue);
      yylval.int_val = BN_dup(BN_0);
    }
  } else {
    config_read_error("No macro or environmental variable defined with name "
                      "`%s'", macroname);
    yylval.int_val = BN_dup(BN_0);
  }
  BN_free(BN_0);
  Free(macroname);
  char *int_val_str = BN_bn2dec(yylval.int_val);
  if (whether_update_buffer())
    cfg->config_read_buffer = mputprintf(cfg->config_read_buffer, "%s ", int_val_str);
  OPENSSL_free(int_val_str);
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    return MPNumber;
  }
  return Number;
}

{MACRO_FLOAT} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_float(macrovalue, macrolen))
      yylval.float_val = atof(macrovalue);
    else {
      config_read_error("Macro `%s' cannot be interpreted as float value: "
	"`%s'", macroname, macrovalue);
      yylval.float_val = 0.0;
    }
  } else {
    config_read_error("No macro or environmental variable defined"
                            " with name `%s'", macroname);
    yylval.float_val = 0.0;
  }
  Free(macroname);
  if (whether_update_buffer()) cfg->config_read_buffer =
    mputprintf(cfg->config_read_buffer, "%f ", yylval.float_val);
  if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
    return MPFloat;
  }
  return Float;
}

{MACRO_ID} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  boolean is_asn = FALSE;
  if (macrovalue != NULL) {
    if (string_is_id(macrovalue, macrolen)) {
      yylval.str_val = mcopystr(macrovalue);
      for (size_t i = 0; i < macrolen; i++) {
	if (yylval.str_val[i] == '-') {
	  is_asn = TRUE;
	  yylval.str_val[i] = '_';
	}
      }
      if (is_asn) config_read_warning("`%s' is not a valid TTCN-3 identifier. "
	"Did you mean `%s'?", macrovalue, yylval.str_val);
      if (whether_update_buffer()) cfg->config_read_buffer =
	mputprintf(cfg->config_read_buffer, "%s ", yylval.str_val);
    } else {
      config_read_error("Macro `%s' cannot be interpreted as identifier: `%s'",
	macroname, macrovalue);
      yylval.str_val = NULL;
    }
  } else {
    config_read_error("No macro or environmental variable defined with name "
      "`%s'", macroname);
    yylval.str_val = NULL;
  }
  Free(macroname);
  return Identifier;
}

{MACRO_CSTR} {
  char *macroname;
  if (yytext[1] == '{') macroname = get_macro_id_from_ref(yytext);
  else macroname = mcopystr(yytext + 1);
  size_t macrolen;
  const char *macrovalue = string_map_get_bykey
    (config_defines, macroname, &macrolen);
  if (macrovalue == NULL) {
    config_read_error("No macro or environmental variable defined with "
        "name `%s'", macroname);
    yylval.str_val = memptystr();
    Free(macroname);
    if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
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
    if (whether_update_buffer()) {
      cfg->config_read_buffer=mputc(cfg->config_read_buffer, '"');
      for (size_t i = 0; i < macrolen; i++) {
        switch (macrovalue[i]) {
        case '"':
          cfg->config_read_buffer = mputstr(cfg->config_read_buffer, "\\\"");
          break;
        case '\\':
          cfg->config_read_buffer = mputstr(cfg->config_read_buffer, "\\\\");
          break;
        case '\0':
          cfg->config_read_buffer = mputstr(cfg->config_read_buffer, "\\000");
          break;
        default:
          cfg->config_read_buffer = mputc(cfg->config_read_buffer, macrovalue[i]);
        } /* switch */
      } /* for */
      cfg->config_read_buffer=mputstr(cfg->config_read_buffer, "\" ");
    }
    Free(macroname);
    yylval.str_val = mcopystr(macrovalue);
    if (YY_START == SC_MODULE_PARAMETERS || YY_START == SC_CHAR_KEYWORD) {
      return MPCstring;
    }
    return Cstring;
  }
}

{MACRO_BSTR} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_bstr(macrovalue, macrolen)) {
      if (whether_update_buffer()) cfg->config_read_buffer =
	mputprintf(cfg->config_read_buffer, "'%s'B ", macrovalue);
    } else config_read_error("Macro `%s' cannot be interpreted as bitstring "
      "value: `%s'", macroname, macrovalue);
  } else config_read_error("No macro or environmental variable defined with "
    "name `%s'", macroname);
  Free(macroname);
  return Bstring;
}

{MACRO_HSTR} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_hstr(macrovalue, macrolen)) {
      if (whether_update_buffer()) cfg->config_read_buffer =
	mputprintf(cfg->config_read_buffer, "'%s'H ", macrovalue);
    } else config_read_error("Macro `%s' cannot be interpreted as hexstring "
      "value: `%s'", macroname, macrovalue);
  } else config_read_error("No macro or environmental variable defined with "
    "name `%s'", macroname);
  Free(macroname);
  return Hstring;
}

{MACRO_OSTR} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_ostr(macrovalue, macrolen)) {
      if (whether_update_buffer()) cfg->config_read_buffer =
	mputprintf(cfg->config_read_buffer, "'%s'O ", macrovalue);
    } else config_read_error("Macro `%s' cannot be interpreted as octetstring "
      "value: `%s'", macroname, macrovalue);
  } else config_read_error("No macro or environmental variable defined with "
    "name `%s'", macroname);
  Free(macroname);
  return Ostring;
}

{MACRO_BINARY} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (whether_update_buffer()) {
      cfg->config_read_buffer = mputc(cfg->config_read_buffer, '\'');
      for (size_t i = 0; i < macrolen; i++) {
	cfg->config_read_buffer = mputprintf(cfg->config_read_buffer, "%02X",
	  macrovalue[i]);
      }
      cfg->config_read_buffer = mputstr(cfg->config_read_buffer, "'O ");
    }
  } else config_read_error("No macro or environmental variable defined with "
    "name `%s'", macroname);
  Free(macroname);
  return Ostring;
}

{MACRO_HOSTNAME} {
  char *macroname = get_macro_id_from_ref(yytext);
  size_t macrolen;
  const char *macrovalue =
    string_map_get_bykey(config_defines, macroname, &macrolen);
  if (macrovalue != NULL) {
    if (string_is_hostname(macrovalue, macrolen)) {
      if (whether_update_buffer()) {
	for (size_t i = 0; i < macrolen; i++) {
	  cfg->config_read_buffer =
	    mputc(cfg->config_read_buffer, tolower(macrovalue[i]));
	}
	yylval.str_val = NULL;
      } else {
	yylval.str_val = (char*)Malloc(macrolen + 1);
	for (size_t i = 0; i < macrolen; i++)
	  yylval.str_val[i] = tolower(macrovalue[i]);
	yylval.str_val[macrolen] = '\0';
      }
    } else {
      config_read_error("Macro `%s' cannot be interpreted as host name: "
	"`%s'", macroname, macrovalue);
      yylval.str_val = NULL;
    }
  } else {
    config_read_error("No macro or environmental variable defined with "
      "name `%s'", macroname);
    yylval.str_val = NULL;
  }
  Free(macroname);
  return DNSName;
}

":="|"="    RETURN(AssignmentChar);
"&="        RETURN(ConcatChar);

.	          RETURN(yytext[0]);

<*><<EOF>> {
  if (expansion_buffer) {
    yy_switch_to_buffer(main_buffer);
    yy_delete_buffer(expansion_buffer);
    expansion_buffer = NULL;
  } else {
    if (YY_START == SC_blockcomment) {
      config_read_error("Unterminated block "
        "comment (missing */ at the end of file).");
      return EOF;
    } else {
      if (include_chain->size() > 1) {
        yy_delete_buffer(YY_CURRENT_BUFFER);
        fclose(include_chain->back().fp);
        include_chain->pop_back();
        yy_switch_to_buffer(include_chain->back().buffer_state);
        yylineno = include_chain->back().line_number;
        BEGIN(SC_ORDERED_INCLUDE);
      } else {
        return EOF;
      }
    }
  }
}

%%

static void update_buffer()
{
  if (whether_update_buffer()) {
    cfg->config_read_buffer = mputstr(cfg->config_read_buffer, yytext);
    cfg->config_read_buffer = mputc  (cfg->config_read_buffer, ' ');
  }
}

static boolean whether_update_buffer()
{
  switch (YY_START) {
  case SC_MODULE_PARAMETERS:
  case SC_CHAR_KEYWORD:
  case SC_LOGGING:
  case SC_TESTPORT_PARAMETERS:
  case SC_EXTERNAL_COMMANDS:
  case SC_PROFILER:
    return TRUE;
  default:
    return FALSE;
  } /* switch */
}

void config_read_reset(const char* current_filename_) {
  if (!include_chain) {
    include_chain = new std::deque<IncludeElem<YY_BUFFER_STATE> >();
  }

  include_chain->clear();
  include_chain->push_back(
      IncludeElem<YY_BUFFER_STATE>(std::string(current_filename_), config_read_in));
}

void config_read_close() {
  delete include_chain;
  include_chain = NULL;
}


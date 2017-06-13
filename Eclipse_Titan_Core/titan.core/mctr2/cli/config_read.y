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
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <openssl/crypto.h>
#include <openssl/bn.h>

#include "../../common/memory.h"
#include "../../common/config_preproc.h"
#include "../mctr/config_data.h"
#include "../../core/Types.h"
#include "../../core/RInt.hh"

#define YYERROR_VERBOSE

extern FILE *config_read_in;
extern int config_read_lineno;
extern char *config_read_text;
extern int config_read_lex();
extern void config_read_restart(FILE *new_file);
extern void config_read_reset(const char* fname);
extern void config_read_close();
extern std::string get_cfg_read_current_file();

config_data *cfg;

boolean error_flag = FALSE;
static boolean local_addr_set = FALSE,
  tcp_listen_port_set = FALSE,
  kill_timer_set = FALSE,
  num_hcs_set = FALSE;

static int config_read_parse();
int process_config_read_file(const char *file_name);
static void check_duplicate_option(const char *option_name,
  boolean *option_flag);
void config_read_warning(const char *warning_str, ...);
void config_read_error(const char *error_str, ...);

/* For parameters */

static char *group_name = NULL;

string_map_t *config_defines;

#ifndef NDEBUG

union YYSTYPE;
static void yyprint(FILE *file, int type, const YYSTYPE& value);
#define YYPRINT(f,t,v) yyprint(f,t,v)

#endif

%}

%union {
	char *str_val;
	BIGNUM *int_val;
	double float_val;
  boolean bool_val;
  cf_timestamp_format ts_val;
	execute_list_item	execute_item_val;
}

%token ModuleParametersKeyword
%token LoggingKeyword
%token ProfilerKeyword
%token TestportParametersKeyword
%token ExecuteKeyword
%token ExternalCommandsKeyword
%token GroupsKeyword
%token ComponentsKeyword
%token MainControllerKeyword
%token IncludeKeyword
%token DefineKeyword

%token ObjIdKeyword "objid"
%token CharKeyword "char"
%token ControlKeyword "control"
%token MTCKeyword "mtc"
%token SystemKeyword "system"
%token NULLKeyword "NULL"
%token nullKeyword "null"
%token OmitKeyword "omit"
%token AssignmentChar ":= or ="
%token ConcatChar "&="
%token ComplementKeyword "complement"
%token DotDot ".."
%token SupersetKeyword "superset"
%token SubsetKeyword "subset"
%token PatternKeyword "pattern"
%token PermutationKeyword "permutation"
%token LengthKeyword "length"
%token IfpresentKeyword "ifpresent"
%token InfinityKeyword "infinity"
%token NocaseKeyword "@nocase"

%token LogFile "LogFile of FileName"
%token EmergencyLogging
%token EmergencyLoggingBehaviour
%token EmergencyLoggingBehaviourValue "BufferAll or BufferMasked"
%token EmergencyLoggingMask
%token EmergencyLoggingForFailVerdict
%token FileMask
%token ConsoleMask
%token TimestampFormat
%token ConsoleTimestampFormat
%token SourceInfoFormat "LogSourceInfo or SourceInfoFormat"
%token AppendFile
%token LogEventTypes
%token LogEntityName
%token MatchingHints
%token LoggerPlugins

%token BeginControlPart
%token EndControlPart
%token BeginTestCase
%token EndTestCase


%token <str_val> Identifier
%token ASN1LowerIdentifier "ASN.1 identifier beginning with a lowercase letter"
%token MacroRValue
%token <int_val> Number MPNumber "integer value"
%token <float_val> Float MPFloat "float value"
%token BooleanValue "true or false"
%token VerdictValue
%token Bstring "bit string value"
%token Hstring "hex string value"
%token Ostring "octet string value"
%token BstringMatch "bit string template"
%token HstringMatch "hex string template"
%token OstringMatch "octet string template"
%token <str_val> Cstring "character string value"
%token <str_val> MPCstring "charstring value"
%token <str_val> DNSName "a host name"
%token LoggingBit
%token LoggingBitCollection
%token <ts_val> TimestampValue  "Time, Datetime or Seconds"
%token SourceInfoValue "None, Single or Stack"
%token YesNo "Yes or No" /* from LOGGING section */
%token LocalAddress
%token TCPPort
%token KillTimer
%token NumHCs
%token UnixSocketEnabled
%token YesToken "yes" /* from MAIN_CONTROLLER section */
%token NoToken  "no"
%token Detailed
%token Compact
%token SubCategories
%token LogFileSize
%token LogFileNumber
%token DiskFullAction
%token Error
%token Stop
%token Re_try /* Retry clashes with an enum in Qt */
%token Delete

%token DisableProfilerKeyword    "DisableProfiler"
%token DisableCoverageKeyword    "DisableCoverage"
%token DatabaseFileKeyword       "DatabaseFile"
%token AggregateDataKeyword      "AggregateData"
%token StatisticsFileKeyword     "StatisticsFile"
%token DisableStatisticsKeyword  "DisableStatistics"
%token StatisticsFilterKeyword   "StatisticsFilter"
%token StartAutomaticallyKeyword "StartAutomatically"
%token NetLineTimesKeyword       "NetLineTimes"
%token NetFunctionTimesKeyword   "NetFunctionTimes"
%token ProfilerStatsFlag        "profiler statistics flag"

%type <int_val> IntegerValue
%type <float_val> FloatValue KillTimerValue
%type <str_val> HostName StringValue LogFileName
%type <str_val> ComponentName
%type <str_val> ComponentLocation
%type <execute_item_val> ExecuteItem

%destructor { Free($$); }
Identifier
DNSName
HostName
ComponentName
ComponentLocation
Cstring
MPCstring
StringValue
LogFileName

%destructor {
  Free($$.module_name);
  Free($$.testcase_name);
}
ExecuteItem

%destructor { BN_free($$); }
IntegerValue
Number
MPNumber

%left '&' /* to avoid shift/reduce conflicts */
%left '+' '-'
%left '*' '/'
%left UnarySign

%expect 2

/*
Source of conflicts (1 S/R):

1.) 1 conflict
When seeing a '*' token after a module parameter expression the parser cannot
decide whether the token is a multiplication operator (shift) or it refers to
all modules in the next module parameter (reduce).

The built-in Bison behavior always chooses the shift over the reduce
(the * is interpreted as multiplication).
1 conflict:
infinity can be a float value in LengthMatch's upper bound (SimpleParameterValue)
or an infinityKeyword.
*/

%%

ConfigFile:
	/* empty */
	| ConfigFile Section
;

Section:
	ModuleParametersSection
	| LoggingSection
  | ProfilerSection
	| TestportParametersSection
	| ExecuteSection
	| ExternalCommandsSection
	| GroupsSection
	| ComponentsSection
	| MainControllerSection
	| IncludeSection
	| DefineSection
;

/******************* [MODULE_PARAMETERS] section *******************/

ModuleParametersSection:
	ModuleParametersKeyword ModuleParameters
;

ModuleParameters:
	/* empty */
	| ModuleParameters ModuleParameter optSemiColon
;

ModuleParameter:
    ParameterName ParamOpType ParameterValue
;

ParameterName:
  ParameterNameSegment
| '*' '.' ParameterNameSegment
;

ParameterNameSegment:
  ParameterNameSegment '.' Identifier { Free($3); }
| ParameterNameSegment IndexItemIndex
| Identifier                          { Free($1); }


ParameterValue:
  ParameterExpression
| ParameterExpression LengthMatch
| ParameterExpression IfpresentKeyword
| ParameterExpression LengthMatch IfpresentKeyword
;

LengthMatch:
  LengthKeyword '(' LengthBound ')'
| LengthKeyword '(' LengthBound DotDot LengthBound ')'
| LengthKeyword '(' LengthBound DotDot InfinityKeyword ')'
;

LengthBound:
  ParameterExpression
;

ParameterExpression:
  SimpleParameterValue
| ParameterReference
| '(' ParameterExpression ')'
| '+' ParameterExpression %prec UnarySign
| '-' ParameterExpression %prec UnarySign
| ParameterExpression '+' ParameterExpression
| ParameterExpression '-' ParameterExpression
| ParameterExpression '*' ParameterExpression
| ParameterExpression '/' ParameterExpression
| ParameterExpression '&' ParameterExpression
;

ParameterReference:
  ParameterNameSegment
;

SimpleParameterValue:
	MPNumber { BN_free($1); }
| MPFloat
| InfinityKeyword
| BooleanValue
| ObjIdValue
| VerdictValue
| BitstringValue
| HexstringValue
| OctetstringValue
| MPCstring { Free($1); }
| UniversalCharstringValue
| OmitKeyword
| NULLKeyword
| nullKeyword
| '?'
| '*'
| IntegerRange
| FloatRange
| StringRange
| PatternKeyword PatternChunk
| PatternKeyword NocaseKeyword PatternChunk
| BstringMatch
| HstringMatch
| OstringMatch
| CompoundValue
| MTCKeyword
| SystemKeyword
;

PatternChunk:
  MPCstring { Free($1); }
| Quadruple
;

IntegerRange:
  '(' '-' InfinityKeyword DotDot MPNumber ')' { BN_free($5); }
| '(' '-' InfinityKeyword DotDot '!' MPNumber ')' { BN_free($6); }
| '(' MPNumber DotDot MPNumber ')' { BN_free($2); BN_free($4); }
| '(' '!' MPNumber DotDot MPNumber ')' { BN_free($3); BN_free($5); }
| '(' MPNumber DotDot '!' MPNumber ')' { BN_free($2); BN_free($5); }
| '(' '!' MPNumber DotDot '!' MPNumber ')' { BN_free($3); BN_free($6); }
| '(' MPNumber DotDot InfinityKeyword ')' { BN_free($2); }
| '(' '!' MPNumber DotDot InfinityKeyword ')' { BN_free($3); }
;

FloatRange:
  '(' '-' InfinityKeyword DotDot MPFloat ')'
| '(' '!' '-' InfinityKeyword DotDot MPFloat ')'
| '(' '-' InfinityKeyword DotDot '!' MPFloat ')'
| '(' '!' '-' InfinityKeyword DotDot '!' MPFloat ')'
| '(' MPFloat DotDot MPFloat ')'
| '(' '!' MPFloat DotDot MPFloat ')'
| '(' MPFloat DotDot '!' MPFloat ')'
| '(' '!' MPFloat DotDot '!' MPFloat ')'
| '(' MPFloat DotDot InfinityKeyword ')'
| '(' '!' MPFloat DotDot InfinityKeyword ')'
| '(' MPFloat DotDot '!' InfinityKeyword ')'
| '(' '!' MPFloat DotDot '!' InfinityKeyword ')'
;
  
StringRange:
  '(' UniversalCharstringFragment DotDot UniversalCharstringFragment ')'
| '(' '!' UniversalCharstringFragment DotDot UniversalCharstringFragment ')'
| '(' UniversalCharstringFragment DotDot '!' UniversalCharstringFragment ')'
| '(' '!' UniversalCharstringFragment DotDot '!' UniversalCharstringFragment ')'

IntegerValue:
	Number { $$ = $1; }
	| '(' IntegerValue ')' { $$ = $2; }
	| '+' IntegerValue %prec UnarySign { $$ = $2; }
	| '-' IntegerValue %prec UnarySign
	{
	  BN_set_negative($2, !BN_is_negative($2));
    $$ = $2;
	}
	| IntegerValue '+' IntegerValue
	{
	  $$ = BN_new();
    BN_add($$, $1, $3);
    BN_free($1);
    BN_free($3);
	}
	| IntegerValue '-' IntegerValue
	{
	  $$ = BN_new();
    BN_sub($$, $1, $3);
    BN_free($1);
    BN_free($3);
	}
	| IntegerValue '*' IntegerValue
	{
	  $$ = BN_new();
    BN_CTX *ctx = BN_CTX_new();
    //BN_CTX_init(ctx);
    BN_mul($$, $1, $3, ctx);
    BN_CTX_free(ctx);
    BN_free($1);
    BN_free($3);
	}
	| IntegerValue '/' IntegerValue
  {
    $$ = BN_new();
    BIGNUM *BN_0 = BN_new();
    BN_set_word(BN_0, 0);
    if (BN_cmp($3, BN_0) == 0) {
      config_read_error("Integer division by zero.");
      $$ = BN_0;
    } else {
      BN_CTX *ctx = BN_CTX_new();
      //BN_CTX_init(ctx);
      BN_div($$, NULL, $1, $3, ctx);
      BN_CTX_free(ctx);
      BN_free(BN_0);
    }
    BN_free($1);
    BN_free($3);
  }
;

FloatValue:
	Float { $$ = $1; }
	| '(' FloatValue ')' { $$ = $2; }
	| '+' FloatValue %prec UnarySign { $$ = $2; }
	| '-' FloatValue %prec UnarySign { $$ = -$2; }
	| FloatValue '+' FloatValue { $$ = $1 + $3; }
	| FloatValue '-' FloatValue { $$ = $1 - $3; }
	| FloatValue '*' FloatValue { $$ = $1 * $3; }
	| FloatValue '/' FloatValue
{
	if ($3 == 0.0) {
		config_read_error("Floating point division by zero.");
		$$ = 0.0;
	} else $$ = $1 / $3;
}
;

ObjIdValue:
	ObjIdKeyword '{' ObjIdComponentList '}'
;

ObjIdComponentList:
	ObjIdComponent
	| ObjIdComponentList ObjIdComponent
;

ObjIdComponent:
	NumberForm
	| NameAndNumberForm
;

NumberForm:
	MPNumber { BN_free($1); }
;

NameAndNumberForm:
	Identifier '(' MPNumber ')' { Free($1); BN_free($3); }
;

BitstringValue:
	Bstring
;

HexstringValue:
	Hstring
;

OctetstringValue:
	Ostring
;

UniversalCharstringValue:
	Quadruple
	| USI
;

UniversalCharstringFragment:
	MPCstring { Free($1); }
	| Quadruple
;

Quadruple:
	CharKeyword '(' ParameterExpression ',' ParameterExpression ','
  ParameterExpression ',' ParameterExpression ')'
;

USI:
    CharKeyword '(' UIDlike ')'     
;

UIDlike:
    Cstring { Free($1); }
    | UIDlike ',' Cstring { Free($3); }
;

StringValue:
	Cstring { $$ = $1; }
	| StringValue '&' Cstring {
	  $$ = mputstr($1, $3);
	  Free($3);
	}
;

CompoundValue:
  '{' '}'
|	'{' FieldValueList '}'
| '{' ArrayItemList '}'
| '{' IndexItemList '}'
| '(' ParameterValue ',' TemplateItemList ')' /* at least 2 elements to avoid shift/reduce conflicts with the ParameterExpression rule */
| ComplementKeyword '(' TemplateItemList ')'
| SupersetKeyword '(' TemplateItemList ')'
| SubsetKeyword '(' TemplateItemList ')'
;

ParameterValueOrNotUsedSymbol:
  ParameterValue
| '-'
;

TemplateItemList:
  ParameterValue
| TemplateItemList ',' ParameterValue
;

FieldValueList:
	FieldValue
	| FieldValueList ',' FieldValue
;

FieldValue:
	FieldName AssignmentChar ParameterValueOrNotUsedSymbol
;

FieldName:
	Identifier { Free($1); }
	| ASN1LowerIdentifier
;

ArrayItemList:
	ArrayItem
	| ArrayItemList ',' ArrayItem
;

ArrayItem:
  ParameterValueOrNotUsedSymbol
| PermutationKeyword '(' TemplateItemList ')'
;

IndexItemList:
	IndexItem
	| IndexItemList ',' IndexItem
;

IndexItem:
	IndexItemIndex AssignmentChar ParameterValue
;

IndexItemIndex:
	'[' ParameterExpression ']'
;

/******************* [LOGGING] section *******************/

LoggingSection:
	LoggingKeyword LoggingParamList
;

LoggingParamList:
	/* empty */
	| LoggingParamList LoggingParamLines optSemiColon
;

LoggingParamLines:
	LoggingParam
	| ComponentId '.' LoggingParam
	| ComponentId '.' LoggerPluginId '.' LoggingParam
	| LoggerPlugins AssignmentChar '{' LoggerPluginList '}'
	| ComponentId '.' LoggerPlugins AssignmentChar '{' LoggerPluginList '}'
;

LoggerPluginId:
  '*'
  | Identifier { Free($1); }
;

LoggingParam:
	FileMask AssignmentChar LoggingBitMask
	| ConsoleMask AssignmentChar LoggingBitMask
	| LogFileSize AssignmentChar Number { BN_free($3); }
	| EmergencyLogging AssignmentChar Number { BN_free($3); }
	| EmergencyLoggingBehaviour AssignmentChar EmergencyLoggingBehaviourValue
	| EmergencyLoggingMask AssignmentChar LoggingBitMask
	| EmergencyLoggingForFailVerdict AssignmentChar YesNoOrBoolean
	| LogFileNumber AssignmentChar Number { BN_free($3); }
	| DiskFullAction AssignmentChar DiskFullActionValue
	| LogFile AssignmentChar LogFileName { cfg->set_log_file($3); }
	| TimestampFormat AssignmentChar TimestampValue
	| ConsoleTimestampFormat AssignmentChar TimestampValue {cfg->tsformat=$3;}
	| SourceInfoFormat AssignmentChar SourceInfoSetting
	| AppendFile AssignmentChar YesNoOrBoolean
	| LogEventTypes AssignmentChar LogEventTypesValue
	| LogEntityName AssignmentChar YesNoOrBoolean
	| MatchingHints AssignmentChar MatchVerbosityValue
	| Identifier AssignmentChar StringValue { Free($1); Free($3); }
;

LoggerPluginList:
	LoggerPlugin
	| LoggerPluginList ',' LoggerPlugin
;

LoggerPlugin:
  Identifier { Free($1); }
  | Identifier AssignmentChar StringValue { Free($1); Free($3); }
;

DiskFullActionValue:
	Error
	| Stop
	| Re_try
	| Re_try '(' Number ')' { BN_free($3); }
	| Delete
;

LogFileName:
	StringValue { $$ = $1; }
;

//optTestComponentIdentifier:
	/* empty */
/*	| Identifier '.' { Free($1); }
	| Number '.'
	| MTCKeyword '.'
	| '*' '.'
;*/

LoggingBitMask:
	LoggingBitorCollection
	| LoggingBitMask ListOp LoggingBitorCollection
;

ListOp:
	/* empty */
	/*|*/ '|'
;

LoggingBitorCollection:
	LoggingBit
	| LoggingBitCollection
;

SourceInfoSetting:
	SourceInfoValue
	| YesNoOrBoolean
;

YesNoOrBoolean:
	YesNo
	| BooleanValue
;

LogEventTypesValue:
	YesNoOrBoolean
	| Detailed
	| SubCategories
;

MatchVerbosityValue:
	Compact
	| Detailed
;

/*********************** [PROFILER] ********************************/

ProfilerSection:
  ProfilerKeyword ProfilerSettings
;

ProfilerSettings:
  /* empty */
| ProfilerSettings ProfilerSetting optSemiColon
;

ProfilerSetting:
  DisableProfilerSetting
| DisableCoverageSetting
| DatabaseFileSetting
| AggregateDataSetting
| StatisticsFileSetting
| DisableStatisticsSetting
| StatisticsFilterSetting
| StartAutomaticallySetting
| NetLineTimesSetting
| NetFunctionTimesSetting
;

DisableProfilerSetting:
  DisableProfilerKeyword AssignmentChar BooleanValue
;

DisableCoverageSetting:
  DisableCoverageKeyword AssignmentChar BooleanValue
;

DatabaseFileSetting:
  DatabaseFileKeyword AssignmentChar StringValue { Free($3); }
;

AggregateDataSetting:
  AggregateDataKeyword AssignmentChar BooleanValue
;

StatisticsFileSetting:
  StatisticsFileKeyword AssignmentChar StringValue { Free($3); }
;

DisableStatisticsSetting:
  DisableStatisticsKeyword AssignmentChar BooleanValue
;

StatisticsFilterSetting:
  StatisticsFilterKeyword AssignmentChar ProfilerStatsFlags
| StatisticsFilterKeyword ConcatChar ProfilerStatsFlags
;

ProfilerStatsFlags:
  ProfilerStatsFlag
| ProfilerStatsFlag '&' ProfilerStatsFlags
| ProfilerStatsFlag '|' ProfilerStatsFlags
;

StartAutomaticallySetting:
  StartAutomaticallyKeyword AssignmentChar BooleanValue
;

NetLineTimesSetting:
  NetLineTimesKeyword AssignmentChar BooleanValue
;

NetFunctionTimesSetting:
  NetFunctionTimesKeyword AssignmentChar BooleanValue
;

/******************* [TESTPORT_PARAMETERS] section *******************/

TestportParametersSection:
	TestportParametersKeyword TestportParameterList
;

TestportParameterList:
	/* empty */
	| TestportParameterList TestportParameter optSemiColon
;

TestportParameter:
	ComponentId '.' TestportName '.' TestportParameterName AssignmentChar
	TestportParameterValue
;

ComponentId:
	Identifier { Free($1); }
	| Number { BN_free($1); }
	| MTCKeyword
	| '*'
	| SystemKeyword
	| Cstring { Free($1); }
;

TestportName:
	Identifier { Free($1); }
	| Identifier ArrayRef { Free($1); }
	| '*'
;

ArrayRef:
	'[' IntegerValue ']' { BN_free($2); }
	| ArrayRef '[' IntegerValue ']' { BN_free($3); }
;

TestportParameterName:
	Identifier { Free($1); }
;

TestportParameterValue:
	StringValue { Free($1); }
;

/******************* [EXECUTE] section *******************/

ExecuteSection:
	ExecuteKeyword ExecuteList
;

ExecuteList:
	/* empty */
	| ExecuteList ExecuteItem optSemiColon
{
	cfg->add_exec($2);
}
;

ExecuteItem:
	Identifier
{
	$$.module_name = $1;
	$$.testcase_name = NULL;
}
	| Identifier '.' ControlKeyword
{
	$$.module_name = $1;
	$$.testcase_name = NULL;
}
	| Identifier '.' Identifier
{
	$$.module_name = $1;
	$$.testcase_name = $3;
}
	| Identifier '.' '*'
{
	$$.module_name = $1;
	$$.testcase_name = mcopystr("*");
}
;

/******************* [EXTERNAL_COMMANDS] section *******************/

ExternalCommandsSection:
	ExternalCommandsKeyword ExternalCommandList
;

ExternalCommandList:
	/* empty */
	| ExternalCommandList ExternalCommand optSemiColon
;

ExternalCommand:
	BeginControlPart AssignmentChar Command
	| EndControlPart AssignmentChar Command
	| BeginTestCase AssignmentChar Command
	| EndTestCase AssignmentChar Command
;

Command:
	StringValue { Free($1); }
;

/******************* [GROUPS] section *******************/

GroupsSection:
	GroupsKeyword GroupList
;

GroupList:
	/* empty */
	| GroupList Group optSemiColon
;

Group:
	GroupName AssignmentChar GroupMembers
{
	Free(group_name);
	group_name = NULL;
}
;

GroupName:
	Identifier { group_name = $1; }
;

GroupMembers:
	'*'
{
	if (group_name != NULL) cfg->add_host(group_name, NULL);
}
	| seqGroupMember
;

seqGroupMember:
	HostName
{
	if (group_name != NULL && $1 != NULL)
		cfg->add_host(group_name, $1);
	Free($1);
}
	| seqGroupMember ',' HostName
{
	if (group_name != NULL && $3 != NULL)
		cfg->add_host(group_name, $3);
	Free($3);
}
;

HostName:
	DNSName { $$ = $1; }
	| Identifier
{
    $$ = $1;
    if ($$ != NULL) {
	    size_t string_len = strlen($$);
	    for (size_t i = 0; i < string_len; i++) $$[i] = tolower($$[i]);
    }
}
;

/******************* [COMPONENTS] section *******************/

ComponentsSection:
	ComponentsKeyword ComponentList
;

ComponentList:
	/* empty */
	| ComponentList ComponentItem optSemiColon
;

ComponentItem:
	ComponentName AssignmentChar ComponentLocation
{
	if ($3 != NULL) cfg->add_component($3, $1);
	//Free($1);
	//Free($3);
}
;

ComponentName:
	Identifier { $$ = $1; }
	| '*' { $$ = NULL; }
;

ComponentLocation:
	Identifier { $$ = $1; }
	| DNSName { $$ = $1; }
;

/******************* [MAIN_CONTROLLER] section *******************/

MainControllerSection:
	MainControllerKeyword MCParameterList
;

MCParameterList:
	/* empty */
	| MCParameterList MCParameter optSemiColon
;

MCParameter:
	LocalAddress AssignmentChar HostName
{
	check_duplicate_option("LocalAddress", &local_addr_set);
	Free(cfg->local_addr);
	cfg->local_addr = $3;
}
	| TCPPort AssignmentChar IntegerValue
{
	check_duplicate_option("TCPPort", &tcp_listen_port_set);
	BIGNUM *BN_0 = BN_new();
	BN_set_word(BN_0, 0);
  BIGNUM *BN_65535 = BN_new();
  BN_set_word(BN_65535, 65535);
  char *int_val_str = BN_bn2dec($3);
	if (BN_cmp($3, BN_0) < 0 || BN_cmp($3, BN_65535) > 0)
	  config_read_error("An integer value within range 0 .. 65535 was "
		                  "expected for parameter TCPPort instead of %s.",
		                  int_val_str);
  else cfg->tcp_listen_port = (unsigned short)BN_get_word($3);
  BN_free(BN_0);
  BN_free(BN_65535);
  BN_free($3);
  OPENSSL_free(int_val_str);
}
	| KillTimer AssignmentChar KillTimerValue
{
	check_duplicate_option("KillTimer", &kill_timer_set);
	if ($3 >= 0.0) cfg->kill_timer = $3;
	else config_read_error("A non-negative numeric value was expected for "
		"parameter KillTimer instead of %g.", $3);
}
	| NumHCs AssignmentChar IntegerValue
{
	check_duplicate_option("NumHCs", &num_hcs_set);
	BIGNUM *BN_0 = BN_new();
	BN_set_word(BN_0, 0);
	char *int_val_str = BN_bn2dec($3);
	if (BN_cmp($3, BN_0) <= 0)
	  config_read_error("A positive integer value was expected for "
		                  "parameter NumHCs instead of %s.", int_val_str);
  else cfg->num_hcs = (int)BN_get_word($3);
  BN_free(BN_0);
  // Check if we really need to free this!
  BN_free($3);
  OPENSSL_free(int_val_str);
}
  | UnixSocketEnabled AssignmentChar YesToken
{
  cfg->unix_sockets_enabled = true;
}
  | UnixSocketEnabled AssignmentChar NoToken
{
  cfg->unix_sockets_enabled = false;
}
  | UnixSocketEnabled AssignmentChar HostName
{
  config_read_error("Only 'yes' or 'no' is accepted instead of '%s'", $3);
}
;

KillTimerValue:
	FloatValue { $$ = $1; }
	| IntegerValue
	{
    double tmp = (double)BN_get_word($1);
    if (BN_is_negative($1)) tmp *= -1;
    $$ = tmp;
    BN_free($1);
  }
;

/******************* [INCLUDE] section *******************/

IncludeSection:
  IncludeKeyword IncludeFiles
;

IncludeFiles:
  /* empty */
| IncludeFiles IncludeFile
;

IncludeFile:
  Cstring { Free($1); }
;

/******************* [DEFINE] section *******************/

DefineSection:
  DefineKeyword
;

/********************************************************/

ParamOpType:
  AssignmentChar
| ConcatChar
;

optSemiColon:
	/* empty */
	| ';'
;

%%


int process_config_read_file(const char *file_name, config_data *pcfg)
{
  // reset "locals"
  local_addr_set = FALSE;
  tcp_listen_port_set = FALSE;
  kill_timer_set = FALSE;
  num_hcs_set = FALSE;

  error_flag = FALSE;
  string_chain_t *filenames=NULL;
  cfg = pcfg;

  /* Initializing parameters to default values */
  cfg->clear();

  if(preproc_parse_file(file_name, &filenames, &config_defines))
    error_flag=TRUE;

  while(filenames) {
    char *fn=string_chain_cut(&filenames);
    config_read_lineno=1;
    /* The lexer can modify config_process_in
     * when it's input buffer is changed */
    config_read_in = fopen(fn, "r");
    if (config_read_in == NULL) {
      fprintf(stderr, "Cannot open configuration file: %s (%s)\n",
              fn, strerror(errno));
      error_flag=TRUE;
    } else {
      FILE* tmp_cfg = config_read_in;
      config_read_restart(config_read_in);
      config_read_reset(fn);
      if(config_read_parse()) error_flag=TRUE;
      fclose(tmp_cfg);
      /* During parsing flex or libc may use some system calls (e.g. ioctl)
       * that fail with an error status. Such error codes shall be ignored in
       * future error messages. */
      errno = 0;
    }
    Free(fn);
  }

  config_read_close();

  string_map_free(config_defines);
  config_defines=NULL;

  return error_flag ? -1 : 0;
}

static void check_duplicate_option(const char *option_name,
  boolean *option_flag)
{
  if (*option_flag) {
    config_read_warning("Option `%s' was given more than once in section "
      "[MAIN_CONTROLLER].", option_name);
  } else *option_flag = TRUE;
}


#ifndef NDEBUG
void yyprint(FILE *file, int type, const YYSTYPE& value)
{
  switch (type) {
  case DNSName:
  case Identifier:
  case Cstring:
    fprintf(file, "'%s'", value.str_val);
    break;

  case Number: {
    char *string_repr = BN_bn2dec(value.int_val);
    fprintf(file, "%s", string_repr);
    OPENSSL_free(string_repr);
    break; }
  default:
    break;
  }
}
#endif

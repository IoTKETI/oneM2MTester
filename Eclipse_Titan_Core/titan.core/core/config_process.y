/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
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
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <openssl/bn.h>

#include "../common/config_preproc.h"

#include "Param_Types.hh"
#include "Integer.hh"
#include "Float.hh"
#include "Boolean.hh"
#include "Objid.hh"
#include "Verdicttype.hh"
#include "Bitstring.hh"
#include "Hexstring.hh"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"

#include "Module_list.hh"
#include "Port.hh"
#include "Runtime.hh"

#include "LoggingBits.hh"
#include "LoggingParam.hh"

#include "Profiler.hh"
#include "Debugger.hh"
#include "DebugCommands.hh"

#define YYERROR_VERBOSE

#include "config_process.lex.hh"

#ifndef INFINITY
#include <float.h>
static const double INFINITY = (DBL_MAX*DBL_MAX);
#endif


extern void reset_config_process_lex(const char* fname);
extern void config_process_close();
extern int config_process_get_current_line();
extern std::string get_cfg_process_current_file();

static int config_process_parse();
static void check_duplicate_option(const char *section_name,
    const char *option_name, boolean& option_flag);
static void check_ignored_section(const char *section_name);
static void set_param(Module_Param& module_param);
static unsigned char char_to_hexdigit_(char c);

static boolean error_flag = FALSE;

static Module_Param* parsed_module_param = NULL;
static char * parsing_error_messages = NULL;

/*
For detecting duplicate entries in the config file. Start out as FALSE,
set to TRUE bycheck_duplicate_option().
Exception: duplication of parameters that can be component specific is checked
by set_xxx(), these start out as TRUE.
*/
static boolean file_name_set = FALSE,
	file_mask_set = TRUE,
	console_mask_set = TRUE,
	timestamp_format_set = FALSE,
	source_info_format_set = FALSE,
	append_file_set = FALSE,
	log_event_types_set = FALSE,
	log_entity_name_set = FALSE,
	begin_controlpart_command_set = FALSE,
	end_controlpart_command_set = FALSE,
	begin_testcase_command_set = FALSE,
	end_testcase_command_set = FALSE,
	log_file_size_set = TRUE,
	log_file_number_set = TRUE,
	log_disk_full_action_set = TRUE,
	matching_verbosity_set = FALSE,
	logger_plugins_set = FALSE,
	plugin_specific_set = FALSE;

int execute_list_len = 0;
execute_list_item *execute_list = NULL;

string_map_t *config_defines;

%}

%union{
	char 					*str_val;
	int_val_t				*int_val;
	int						int_native;
	unsigned int			uint_val;
	double					float_val;
	boolean					bool_val;
	param_objid_t			objid_val;
	verdicttype				verdict_val;
	param_bitstring_t		bitstring_val;
	param_hexstring_t		hexstring_val;
	param_charstring_t		charstring_val;
	param_octetstring_t		octetstring_val;
	universal_char			universal_char_val;
	param_universal_charstring_t universal_charstring_val;
	Module_Param::operation_type_t param_optype_val;
  Vector<Module_Param*>* module_param_list;
	Module_Param*			module_param_val;
  Module_Param_Length_Restriction* module_param_length_restriction;
	Vector<char*>*		name_vector;
	component_id_t		comp_id;
	execute_list_item	execute_item_val;
	TTCN_Logger::emergency_logging_behaviour_t emergency_logging_behaviour_value;
	TTCN_Logger::timestamp_format_t timestamp_value;
	TTCN_Logger::source_info_format_t source_info_value;
  TTCN_Logger::log_event_types_t log_event_types_value;
  TTCN_Logger::disk_full_action_t disk_full_action_value;
  TTCN_Logger::matching_verbosity_t matching_verbosity_value;
	TTCN_Logger::Severity	logseverity_val;
	Logging_Bits logoptions_val;

  logging_plugin_t *logging_plugins;
	logging_param_t logging_params;
	logging_setting_t logging_param_line;
  struct {
    size_t nElements;
    char **elements;
  } uid_list;
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

%token Detailed
%token Compact
%token ObjIdKeyword "objid"
%token CharKeyword "char"
%token ControlKeyword "control"
%token MTCKeyword "mtc"
%token SystemKeyword "system"
%token NULLKeyword "NULL"
%token nullKeyword "null"
%token OmitKeyword "omit"
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
%token AssignmentChar ":= or ="
%token ConcatChar "&="
%token LogFile "LogFile or FileName"
%token EmergencyLogging
%token EmergencyLoggingBehaviour
%token EmergencyLoggingMask
%token EmergencyLoggingForFailVerdict
%token BufferAll
%token BufferMasked
%token FileMask
%token ConsoleMask
%token TimestampFormat
%token ConsoleTimestampFormat
%token SourceInfoFormat "LogSourceInfo or SourceInfoFormat"
%token AppendFile
%token LogEventTypes
%token LogEntityName
%token BeginControlPart
%token EndControlPart
%token BeginTestCase
%token EndTestCase
%token <str_val> Identifier
%token <str_val> ASN1LowerIdentifier "ASN.1 identifier beginning with a lowercase letter"
%token <int_val> Number MPNumber "integer value"
%token <float_val> Float MPFloat "float value"
%token <bool_val> BooleanValue "true or false"
%token <verdict_val> VerdictValue
%token <bitstring_val> Bstring "bit string value"
%token <hexstring_val> Hstring "hex string value"
%token <octetstring_val> Ostring "octet string value"
%token <str_val> BstringMatch "bit string template"
%token <str_val> HstringMatch "hex string template"
%token <str_val> OstringMatch "octet string template"
%token <charstring_val> Cstring MPCstring "charstring value"
%token <str_val> UIDval
%token DNSName "hostname"
/* a single bit */
%token <logseverity_val> LoggingBit
/* a collection of bits */
%token <logseverity_val>  LoggingBitCollection
%token SubCategories
%token <emergency_logging_behaviour_value> EmergencyLoggingBehaviourValue "BufferAll or BufferMasked"
%token <timestamp_value> TimestampValue "Time, Datetime or Seconds"
%token <source_info_value> SourceInfoValue "None, Single or Stack"
%token <bool_val> YesNo "yes or no"
%token LocalAddress
%token TCPPort
%token KillTimer
%token NumHCs
%token UnixSocketEnabled
%token YesToken "yes"
%token NoToken  "no"
%token LogFileSize
%token LogFileNumber
%token DiskFullAction
%token MatchingHints
%token LoggerPlugins
%token Error
%token Stop
%token Retry
%token Delete
%token TtcnStringParsingKeyword
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
%token <uint_val> ProfilerStatsFlag "profiler statistics filter"

%type <int_val> IntegerValue
%type <float_val> FloatValue
%type <objid_val> ObjIdValue ObjIdComponentList
%type <int_val> ObjIdComponent NumberForm NameAndNumberForm

%type <universal_charstring_val> UniversalCharstringValue UniversalCharstringFragment
%type <universal_char_val> Quadruple
%type <uid_list> USI UIDlike

%type <str_val> LoggerPluginId
%type <logging_plugins> LoggerPlugin LoggerPluginList
%type <logging_params> LoggingParam
%type <logging_param_line> LoggingParamLines
%type <str_val> LogFileName StringValue
/* a collection of bits */
%type <logoptions_val> LoggingBitmask
%type <logoptions_val> LoggingBitOrCollection
%type <source_info_value> SourceInfoSetting
%type <bool_val> YesNoOrBoolean
%type <log_event_types_value> LogEventTypesValue
%type <disk_full_action_value> DiskFullActionValue
%type <str_val> Command
%type <matching_verbosity_value> VerbosityValue
%type <comp_id> ComponentId
%type <str_val> TestportName ArrayRef TestportParameterName
	TestportParameterValue

%type <execute_item_val> ExecuteItem

%type <bitstring_val> BitstringValue
%type <hexstring_val> HexstringValue
%type <octetstring_val> OctetstringValue

%type <name_vector> ParameterName ParameterNameSegment
%type <param_optype_val> ParamOpType
%type <str_val> FieldName
%type <module_param_val> ParameterValue SimpleParameterValue ParameterValueOrNotUsedSymbol
  FieldValue ArrayItem IndexItem IndexItemList FieldValueList ArrayItemList CompoundValue IntegerRange FloatRange StringRange
  ParameterExpression ParameterReference
%type <module_param_list> TemplateItemList
%type <module_param_length_restriction> LengthMatch
%type <str_val> PatternChunk
%type <int_native> IndexItemIndex LengthBound
%type <uint_val> ProfilerStatsFlags

%destructor { Free($$); }
ArrayRef
ASN1LowerIdentifier
Command
FieldName
Identifier
LogFileName
StringValue
TestportName
TestportParameterName
TestportParameterValue
PatternChunk
BstringMatch
HstringMatch
OstringMatch

%destructor { Free($$.components_ptr); }
ObjIdComponentList
ObjIdValue

%destructor { Free($$.bits_ptr); }
Bstring
BitstringValue

%destructor { Free($$.nibbles_ptr); }
Hstring
HexstringValue

%destructor { Free($$.octets_ptr); }
Ostring
OctetstringValue

%destructor { Free($$.chars_ptr); }
Cstring
MPCstring

%destructor { Free($$.uchars_ptr); }
UniversalCharstringFragment
UniversalCharstringValue

%destructor { if ($$.id_selector == COMPONENT_ID_NAME) Free($$.id_name); }
ComponentId

%destructor { Free($$.module_name); Free($$.testcase_name); }
ExecuteItem

%destructor { delete $$; }
IntegerValue
NameAndNumberForm
Number
MPNumber
NumberForm
ObjIdComponent
ParameterValue
SimpleParameterValue
ParameterValueOrNotUsedSymbol
ArrayItemList
FieldValueList
IndexItemList
CompoundValue
ArrayItem
FieldValue
IndexItem
IntegerRange
FloatRange
StringRange
ParameterExpression
ParameterReference

%destructor { delete $$; }
LengthMatch

%destructor { for(size_t i=0; i<$$->size(); i++) { delete $$->at(i); } delete $$; }
TemplateItemList

%destructor { for(size_t i=0; i<$$->size(); i++) { Free($$->at(i)); } delete $$; }
ParameterName
ParameterNameSegment

%left '&' /* to avoid shift/reduce conflicts */
%left '+' '-'
%left '*' '/'
%left UnarySign

%expect 2

/*
1 conflict:
When seeing a '*' token after a module parameter expression the parser cannot
decide whether the token is a multiplication operator (shift) or it refers to 
all modules in the next module parameter (reduce).
1 conflict:
infinity can be a float value in LengthMatch's upper bound (SimpleParameterValue)
or an infinityKeyword.
*/
%%

GrammarRoot:
  ConfigFile
  {
    if (Ttcn_String_Parsing::happening() || Debugger_Value_Parsing::happening()) {
      config_process_error("Config file cannot be parsed as ttcn string");
    }
  }
| TtcnStringParsingKeyword ParameterValue
  {
    parsed_module_param = $2;
  }
;

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

ModuleParametersSection:
	ModuleParametersKeyword ModuleParameters
;

ModuleParameters:
	/* empty */
	| ModuleParameters ModuleParameter optSemiColon
;

ModuleParameter:
	ParameterName ParamOpType ParameterValue
{
  Module_Param* mp = $3;
  mp->set_id(new Module_Param_Name(*$1));
  mp->set_operation_type($2);
  set_param(*mp);
  delete mp;
  delete $1;
}
;

ParameterName:
  ParameterNameSegment            { $$ = $1; }
| '*' '.' ParameterNameSegment    { $$ = $3; }
;

ParameterNameSegment:
	ParameterNameSegment '.' Identifier
{
  $$ = $1;
  $$->push_back($3);
}
| ParameterNameSegment IndexItemIndex
{
  $$ = $1;
  $$->push_back(mprintf("%d", $2));
}
| Identifier
{
  $$ = new Vector<char*>();
  $$->push_back($1); 
}
;

ParameterValue:
  ParameterExpression
  {
    $$ = $1;
  }
| ParameterExpression LengthMatch
  {
    $$ = $1;
    $$->set_length_restriction($2);
  }
| ParameterExpression IfpresentKeyword
  {
    $$ = $1;
    $$->set_ifpresent();
  }
| ParameterExpression LengthMatch IfpresentKeyword
  {
    $$ = $1;
    $$->set_length_restriction($2);
    $$->set_ifpresent();
  }
;

LengthMatch:
  LengthKeyword '(' LengthBound ')'
  {
    $$ = new Module_Param_Length_Restriction();
    $$->set_single((size_t)$3);
  }
| LengthKeyword '(' LengthBound DotDot LengthBound ')'
  {
    if ($3>$5) {
      config_process_error("invalid length restriction: lower bound > upper bound");
    }
    $$ = new Module_Param_Length_Restriction();
    $$->set_min((size_t)$3);
    $$->set_max((size_t)$5);
  }
| LengthKeyword '(' LengthBound DotDot InfinityKeyword ')'
  {
    $$ = new Module_Param_Length_Restriction();
    $$->set_min((size_t)$3);
  }
;

LengthBound:
  ParameterExpression
{
  $1->set_id(new Module_Param_CustomName(mcopystr("length bound")));
  INTEGER tmp;
  tmp.set_param(*$1);
  if (!tmp.get_val().is_native()) {
    config_process_error("bignum length restriction bound.");
    $$ = 0;
  } else if (tmp.get_val().is_negative()) {
    config_process_error("negative length restriction bound.");
    $$ = 0;
  } else {
    $$ = tmp;
  }
  delete $1;
}
;

// one global rule for expressions in module parameters
// the expression's result will be calculated by set_param()
ParameterExpression:
  SimpleParameterValue { $$ = $1; }
| ParameterReference { $$ = $1; }
| '(' ParameterExpression ')' { $$ = $2; }
| '+' ParameterExpression %prec UnarySign { $$ = $2; }
| '-' ParameterExpression %prec UnarySign { $$ = new Module_Param_Expression($2); }
| ParameterExpression '+' ParameterExpression
  {
    $$ = new Module_Param_Expression(Module_Param::EXPR_ADD, $1, $3);
  }
| ParameterExpression '-' ParameterExpression
  {
    $$ = new Module_Param_Expression(Module_Param::EXPR_SUBTRACT, $1, $3);
  }
| ParameterExpression '*' ParameterExpression
  {
    $$ = new Module_Param_Expression(Module_Param::EXPR_MULTIPLY, $1, $3);
  }
| ParameterExpression '/' ParameterExpression
  {
    $$ = new Module_Param_Expression(Module_Param::EXPR_DIVIDE, $1, $3);
  }
| ParameterExpression '&' ParameterExpression
  {
    $$ = new Module_Param_Expression(Module_Param::EXPR_CONCATENATE, $1, $3);
  }
;

ParameterReference:
  // enumerated values are also treated as references by the parser,
  // these will be sorted out later during set_param()
  ParameterNameSegment
  {
#ifdef TITAN_RUNTIME_2
    $$ = new Module_Param_Reference(new Module_Param_Name(*$1));
#else
    // no references allowed in RT1, so the name segment must be an enumerated value
    // (which means it can only contain 1 name)
    if ($1->size() != 1) {
      config_process_error("Module parameter references are not allowed in the "
        "Load Test Runtime.");
    }
    $$ = new Module_Param_Enumerated($1->front());
#endif
    delete $1;
  }
;

SimpleParameterValue:
	MPNumber
  {
    $$ = new Module_Param_Integer($1);
  }
| MPFloat
  {
    $$ = new Module_Param_Float($1);
  }
| InfinityKeyword
  {
    $$ = new Module_Param_Float(INFINITY);
  }
| BooleanValue
  {
    $$ = new Module_Param_Boolean($1);
  }
| ObjIdValue
  {
    $$ = new Module_Param_Objid($1.n_components, $1.components_ptr);
  }
| VerdictValue
  {
    $$ = new Module_Param_Verdict($1);
  }
| BitstringValue
  {
    $$ = new Module_Param_Bitstring($1.n_bits, $1.bits_ptr);
  }
| HexstringValue
  {
    $$ = new Module_Param_Hexstring($1.n_nibbles, $1.nibbles_ptr);
  }
| OctetstringValue
  {
    $$ = new Module_Param_Octetstring($1.n_octets, $1.octets_ptr);
  }
| MPCstring
  {
    $$ = new Module_Param_Charstring($1.n_chars, $1.chars_ptr);
  }
| UniversalCharstringValue
  {
    $$ = new Module_Param_Universal_Charstring($1.n_uchars, $1.uchars_ptr);
  }
| OmitKeyword
  {
    $$ = new Module_Param_Omit();
  }
| NULLKeyword
  {
    $$ = new Module_Param_Asn_Null();
  }
| nullKeyword
  {
    $$ = new Module_Param_Ttcn_Null();
  }
| MTCKeyword
  {
    $$ = new Module_Param_Ttcn_mtc();
  }
| SystemKeyword
  {
    $$ = new Module_Param_Ttcn_system();
  }
| '?'
  {
    $$ = new Module_Param_Any();
  }
| '*'
  {
    $$ = new Module_Param_AnyOrNone();
  }
| IntegerRange
  {
    $$ = $1;
  }
| FloatRange
  {
    $$ = $1;
  }
| StringRange
  {
    $$ = $1;
  }
| PatternKeyword PatternChunk
  {
    $$ = new Module_Param_Pattern($2, FALSE);
  }
| PatternKeyword NocaseKeyword PatternChunk
  {
    $$ = new Module_Param_Pattern($3, TRUE);
  }
| BstringMatch
  {
    // conversion
    int n_chars = (int)mstrlen($1);
    unsigned char* chars_ptr = (unsigned char*)Malloc(n_chars*sizeof(unsigned char));
    for (int i=0; i<n_chars; i++) {
      switch ($1[i]) {
      case '0':
        chars_ptr[i] = 0;
        break;
      case '1':
        chars_ptr[i] = 1;
        break;
      case '?':
        chars_ptr[i] = 2;
        break;
      case '*':
        chars_ptr[i] = 3;
        break;
      default:
        chars_ptr[i] = 0;
        config_process_error_f("Invalid char (%c) in bitstring template", $1[i]);
      }
    }
    Free($1);
    $$ = new Module_Param_Bitstring_Template(n_chars, chars_ptr);
  }
| HstringMatch
  {
    int n_chars = (int)mstrlen($1);
    unsigned char* chars_ptr = (unsigned char*)Malloc(n_chars*sizeof(unsigned char));
    for (int i=0; i<n_chars; i++) {
      if ($1[i]=='?') chars_ptr[i] = 16;
      else if ($1[i]=='*') chars_ptr[i] = 17;
      else chars_ptr[i] = char_to_hexdigit_($1[i]);
    }
    Free($1);
    $$ = new Module_Param_Hexstring_Template(n_chars, chars_ptr);
  }
| OstringMatch
  {
    Vector<unsigned short> octet_vec;
    int str_len = (int)mstrlen($1);
    for (int i=0; i<str_len; i++) {
      unsigned short num;
      if ($1[i]=='?') num = 256;
      else if ($1[i]=='*') num = 257;
      else {
        // first digit
        num = 16 * char_to_hexdigit_($1[i]);
        i++;
        if (i>=str_len) config_process_error("Unexpected end of octetstring pattern");
        // second digit
        num += char_to_hexdigit_($1[i]);
      }
      octet_vec.push_back(num);
    }
    Free($1);
    int n_chars = (int)octet_vec.size();
    unsigned short* chars_ptr = (unsigned short*)Malloc(n_chars*sizeof(unsigned short));
    for (int i=0; i<n_chars; i++) chars_ptr[i] = octet_vec[i];
    $$ = new Module_Param_Octetstring_Template(n_chars, chars_ptr);
  }
| CompoundValue
  {
    $$ = $1;
  }
;

PatternChunk:
  MPCstring
  {
    $$ = mcopystr($1.chars_ptr);
    Free($1.chars_ptr);
  }
| Quadruple
  {
    $$ = mprintf("\\q{%d,%d,%d,%d}", $1.uc_group, $1.uc_plane, $1.uc_row, $1.uc_cell);
  }
;

IntegerRange:
  '(' '-' InfinityKeyword DotDot MPNumber ')'
  {
    $$ = new Module_Param_IntRange(NULL, $5, false, false);
  }
|  '(' '-' InfinityKeyword DotDot '!' MPNumber ')'
  {
    $$ = new Module_Param_IntRange(NULL, $6, false, true);
  }
| '(' MPNumber DotDot MPNumber ')'
  {
    $$ = new Module_Param_IntRange($2, $4, false, false);
  }
| '(' '!' MPNumber DotDot MPNumber ')'
  {
    $$ = new Module_Param_IntRange($3, $5, true, false);
  }
| '(' MPNumber DotDot '!' MPNumber ')'
  {
    $$ = new Module_Param_IntRange($2, $5, false, true);
  }
| '(' '!' MPNumber DotDot '!' MPNumber ')'
  {
    $$ = new Module_Param_IntRange($3, $6, true, true);
  }
| '(' MPNumber DotDot InfinityKeyword ')'
  {
    $$ = new Module_Param_IntRange($2, NULL, false, false);
  }
| '(' '!' MPNumber DotDot InfinityKeyword ')'
  {
    $$ = new Module_Param_IntRange($3, NULL, true, false);
  }
;

FloatRange:
  '(' '-' InfinityKeyword DotDot MPFloat ')'
  {
    $$ = new Module_Param_FloatRange(0.0, false, $5, true, false, false);
  }
| '(' '!' '-' InfinityKeyword DotDot MPFloat ')'
  {
    $$ = new Module_Param_FloatRange(0.0, false, $6, true, true, false);
  }
| '(' '-' InfinityKeyword DotDot '!' MPFloat ')'
  {
    $$ = new Module_Param_FloatRange(0.0, false, $6, true, false, true);
  }
| '(' '!' '-' InfinityKeyword DotDot '!' MPFloat ')'
  {
    $$ = new Module_Param_FloatRange(0.0, false, $7, true, true, true);
  }
| '(' MPFloat DotDot MPFloat ')'
  {
    $$ = new Module_Param_FloatRange($2, true, $4, true, false, false);
  }
| '(' '!' MPFloat DotDot MPFloat ')'
  {
    $$ = new Module_Param_FloatRange($3, true, $5, true, true, false);
  }
| '(' MPFloat DotDot '!' MPFloat ')'
  {
    $$ = new Module_Param_FloatRange($2, true, $5, true, false, true);
  }
| '(' '!' MPFloat DotDot '!' MPFloat ')'
  {
    $$ = new Module_Param_FloatRange($3, true, $6, true, true, true);
  }
| '(' MPFloat DotDot InfinityKeyword ')'
  {
    $$ = new Module_Param_FloatRange($2, true, 0.0, false, false, false);
  }
| '(' '!' MPFloat DotDot InfinityKeyword ')'
  {
    $$ = new Module_Param_FloatRange($3, true, 0.0, false, true, false);
  }
| '(' MPFloat DotDot '!' InfinityKeyword ')'
  {
    $$ = new Module_Param_FloatRange($2, true, 0.0, false, false, true);
  }
| '(' '!' MPFloat DotDot '!' InfinityKeyword ')'
  {
    $$ = new Module_Param_FloatRange($3, true, 0.0, false, true, true);
  }
;

StringRange:
  '(' UniversalCharstringFragment DotDot UniversalCharstringFragment ')'
  {
    universal_char lower; lower.uc_group=lower.uc_plane=lower.uc_row=lower.uc_cell=0;
    universal_char upper; upper.uc_group=upper.uc_plane=upper.uc_row=upper.uc_cell=0;
    if ($2.n_uchars!=1) {
      config_process_error("Lower bound of char range must be 1 character only");
    } else if ($4.n_uchars!=1) {
      config_process_error("Upper bound of char range must be 1 character only");
    } else {
      lower = *($2.uchars_ptr);
      upper = *($4.uchars_ptr);
      if (upper<lower) {
        config_process_error("Lower bound is larger than upper bound in the char range");
        lower = upper;
      }
    }
    Free($2.uchars_ptr);
    Free($4.uchars_ptr);
    $$ = new Module_Param_StringRange(lower, upper, false, false);
  }
| '(' '!' UniversalCharstringFragment DotDot UniversalCharstringFragment ')'
  {
    universal_char lower; lower.uc_group=lower.uc_plane=lower.uc_row=lower.uc_cell=0;
    universal_char upper; upper.uc_group=upper.uc_plane=upper.uc_row=upper.uc_cell=0;
    if ($3.n_uchars!=1) {
      config_process_error("Lower bound of char range must be 1 character only");
    } else if ($5.n_uchars!=1) {
      config_process_error("Upper bound of char range must be 1 character only");
    } else {
      lower = *($3.uchars_ptr);
      upper = *($5.uchars_ptr);
      if (upper<lower) {
        config_process_error("Lower bound is larger than upper bound in the char range");
        lower = upper;
      }
    }
    Free($3.uchars_ptr);
    Free($5.uchars_ptr);
    $$ = new Module_Param_StringRange(lower, upper, true, false);
  }
| '(' UniversalCharstringFragment DotDot '!' UniversalCharstringFragment ')'
  {
    universal_char lower; lower.uc_group=lower.uc_plane=lower.uc_row=lower.uc_cell=0;
    universal_char upper; upper.uc_group=upper.uc_plane=upper.uc_row=upper.uc_cell=0;
    if ($2.n_uchars!=1) {
      config_process_error("Lower bound of char range must be 1 character only");
    } else if ($5.n_uchars!=1) {
      config_process_error("Upper bound of char range must be 1 character only");
    } else {
      lower = *($2.uchars_ptr);
      upper = *($5.uchars_ptr);
      if (upper<lower) {
        config_process_error("Lower bound is larger than upper bound in the char range");
        lower = upper;
      }
    }
    Free($2.uchars_ptr);
    Free($5.uchars_ptr);
    $$ = new Module_Param_StringRange(lower, upper, false, true);
  }
| '(' '!' UniversalCharstringFragment DotDot '!' UniversalCharstringFragment ')'
  {
    universal_char lower; lower.uc_group=lower.uc_plane=lower.uc_row=lower.uc_cell=0;
    universal_char upper; upper.uc_group=upper.uc_plane=upper.uc_row=upper.uc_cell=0;
    if ($3.n_uchars!=1) {
      config_process_error("Lower bound of char range must be 1 character only");
    } else if ($6.n_uchars!=1) {
      config_process_error("Upper bound of char range must be 1 character only");
    } else {
      lower = *($3.uchars_ptr);
      upper = *($6.uchars_ptr);
      if (upper<lower) {
        config_process_error("Lower bound is larger than upper bound in the char range");
        lower = upper;
      }
    }
    Free($3.uchars_ptr);
    Free($6.uchars_ptr);
    $$ = new Module_Param_StringRange(lower, upper, true, true);
  }
;

// integers outside of the [MODULE_PARAMETERS] section
IntegerValue:
	Number { $$ = $1; }
	| '(' IntegerValue ')' { $$ = $2; }
	| '+' IntegerValue %prec UnarySign { $$ = $2; }
  | '-' IntegerValue %prec UnarySign
{
  INTEGER op1;
  op1.set_val(*$2);
  $$ = new int_val_t((-op1).get_val());
  delete $2;
}
  | IntegerValue '+' IntegerValue
{
  INTEGER op1, op2;
  op1.set_val(*$1);
  op2.set_val(*$3);
  $$ = new int_val_t((op1 + op2).get_val());
  delete $1;
  delete $3;
}
  | IntegerValue '-' IntegerValue
{
  INTEGER op1, op2;
  op1.set_val(*$1);
  op2.set_val(*$3);
  $$ = new int_val_t((op1 - op2).get_val());
  delete $1;
  delete $3;
}
  | IntegerValue '*' IntegerValue
{
  INTEGER op1, op2;
  op1.set_val(*$1);
  op2.set_val(*$3);
  $$ = new int_val_t((op1 * op2).get_val());
  delete $1;
  delete $3;
}
  | IntegerValue '/' IntegerValue
{
  if (*$3 == 0) {
    config_process_error("Integer division by zero.");
    $$ = new int_val_t((RInt)0);
    delete $1;
    delete $3;
  } else {
    INTEGER op1, op2;
    op1.set_val(*$1);
    op2.set_val(*$3);
    $$ = new int_val_t((op1 / op2).get_val());
    delete $1;
    delete $3;
  }
}
;

// floats outside of the [MODULE_PARAMETERS] section
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
		config_process_error("Floating point division by zero.");
		$$ = 0.0;
	} else $$ = $1 / $3;
}
;

ObjIdValue:
	ObjIdKeyword '{' ObjIdComponentList '}' { $$ = $3; }
;

ObjIdComponentList:
	ObjIdComponent
{
	$$.n_components = 1;
	$$.components_ptr = (int *)Malloc(sizeof(int));
	$$.components_ptr[0] = $1->get_val();
	delete $1;
}
	| ObjIdComponentList ObjIdComponent
{
	$$.n_components = $1.n_components + 1;
	$$.components_ptr = (int *)Realloc($1.components_ptr,
		$$.n_components * sizeof(int));
	$$.components_ptr[$$.n_components - 1] = $2->get_val();
	delete $2;
}
;

ObjIdComponent:
	NumberForm { $$ = $1; }
	| NameAndNumberForm { $$ = $1; }
;

NumberForm:
	MPNumber { $$ = $1; }
;

NameAndNumberForm:
	Identifier '(' MPNumber ')'
{
	Free($1);
	$$ = $3;
}
;

BitstringValue:
	Bstring { $$ = $1; }
;

HexstringValue:
	Hstring { $$ = $1; }
;

OctetstringValue:
	Ostring { $$ = $1; }
;

UniversalCharstringValue:
  Quadruple
{
	$$.n_uchars = 1;
	$$.uchars_ptr = (universal_char*)Malloc(sizeof(universal_char));
	$$.uchars_ptr[0] = $1;
}
  | USI
{
  $$.n_uchars = $1.nElements;
  $$.uchars_ptr = (universal_char*)Malloc($$.n_uchars * sizeof(universal_char));
  for (int i = 0; i < $$.n_uchars; ++i) {
    size_t offset = 1; //Always starts with u or U
    offset = $1.elements[i][1] == '+' ? offset + 1 : offset; //Optional '+'

    char* p;
    unsigned long int_val = strtoul($1.elements[i] + offset, &p, 16);
    if(*p != 0) {
      //Error, should not happen
      config_process_error_f("Invalid hexadecimal string %s.", $1.elements[i] + offset);
    }
    
    //Fill in the quadruple
    $$.uchars_ptr[i].uc_group = (int_val >> 24) & 0xFF;
    $$.uchars_ptr[i].uc_plane = (int_val >> 16) & 0xFF;
    $$.uchars_ptr[i].uc_row   = (int_val >> 8) & 0xFF;
    $$.uchars_ptr[i].uc_cell  = int_val & 0xFF;

    Free($1.elements[i]);
  }
  Free($1.elements);
}
;

UniversalCharstringFragment:
	MPCstring
{
	$$.n_uchars = $1.n_chars;
	$$.uchars_ptr = (universal_char*)
		Malloc($$.n_uchars * sizeof(universal_char));
	for (int i = 0; i < $1.n_chars; i++) {
		$$.uchars_ptr[i].uc_group = 0;
		$$.uchars_ptr[i].uc_plane = 0;
		$$.uchars_ptr[i].uc_row = 0;
		$$.uchars_ptr[i].uc_cell = $1.chars_ptr[i];
	}
	Free($1.chars_ptr);
}
	| Quadruple
{
	$$.n_uchars = 1;
	$$.uchars_ptr = (universal_char*)Malloc(sizeof(universal_char));
	$$.uchars_ptr[0] = $1;
}
;

Quadruple:
	CharKeyword '(' ParameterExpression ',' ParameterExpression ','
  ParameterExpression ','	ParameterExpression ')'
{
  $3->set_id(new Module_Param_CustomName(mcopystr("quadruple group")));
  $5->set_id(new Module_Param_CustomName(mcopystr("quadruple plane")));
  $7->set_id(new Module_Param_CustomName(mcopystr("quadruple row")));
  $9->set_id(new Module_Param_CustomName(mcopystr("quadruple cell")));
  INTEGER g, p, r, c;
  g.set_param(*$3);
  p.set_param(*$5);
  r.set_param(*$7);
  c.set_param(*$9);
  if (g < 0 || g > 127) {
    char *s = g.get_val().as_string();
    config_process_error_f("The first number of quadruple (group) must be "
      "within the range 0 .. 127 instead of %s.", s);
    Free(s);
    $$.uc_group = g < 0 ? 0 : 127;
  } else {
    $$.uc_group = g;
  }
  if (p < 0 || p > 255) {
    char *s = p.get_val().as_string();
    config_process_error_f("The second number of quadruple (plane) must be "
      "within the range 0 .. 255 instead of %s.", s);
    Free(s);
    $$.uc_plane = p < 0 ? 0 : 255;
  } else {
    $$.uc_plane = p;
  }
  if (r < 0 || r > 255) {
    char *s = r.get_val().as_string();
    config_process_error_f("The third number of quadruple (row) must be "
      "within the range 0 .. 255 instead of %s.", s);
    Free(s);
    $$.uc_row = r < 0 ? 0 : 255;
  } else {
    $$.uc_row = r;
  }
  if (c < 0 || c > 255) {
    char *s = c.get_val().as_string();
    config_process_error_f("The fourth number of quadruple (cell) must be "
      "within the range 0 .. 255 instead of %s.", s);
    Free(s);
    $$.uc_cell = c < 0 ? 0 : 255;
  } else {
    $$.uc_cell = c;
  }
  delete $3;
  delete $5;
  delete $7;
  delete $9;
}
;

USI:
  CharKeyword '(' UIDlike ')'
  {
    $$ = $3;
  }
;

UIDlike:
  UIDval
  {
    $$.nElements = 1;
    $$.elements = (char**)
      Malloc($$.nElements * sizeof(*$$.elements));
    $$.elements[$$.nElements-1] = $1;
  }
| UIDlike ',' UIDval {
    $$.nElements = $1.nElements + 1;
    $$.elements = (char**)
      Realloc($1.elements, ($$.nElements) * sizeof(*$1.elements));
    $$.elements[$$.nElements-1] = $3;
  }
;

// character strings outside of the [MODULE_PARAMETERS] section
StringValue:
	Cstring
{
    $$ = mcopystr($1.chars_ptr);
    Free($1.chars_ptr);
}
	| StringValue '&' Cstring
{
    $$ = mputstr($1, $3.chars_ptr);
    Free($3.chars_ptr);
}
;

CompoundValue:
'{' '}'
  {
    $$ = new Module_Param_Value_List();
  }
|	'{' FieldValueList '}'
  {
    $$ = $2;
  }
| '{' ArrayItemList '}'
  {
    $$ = $2;
  }
| '{' IndexItemList '}'
  {
    $$ = $2;
  }
| '(' ParameterValue ',' TemplateItemList ')' /* at least 2 elements to avoid shift/reduce conflicts with the ParameterExpression rule */
  {
    $$ = new Module_Param_List_Template();
    $2->set_id(new Module_Param_Index($$->get_size(),false));
    $$->add_elem($2);
    $$->add_list_with_implicit_ids($4);
    delete $4;
  }
| ComplementKeyword '(' TemplateItemList ')'
  {
    $$ = new Module_Param_ComplementList_Template();
    $$->add_list_with_implicit_ids($3);
    delete $3;
  }
| SupersetKeyword '(' TemplateItemList ')'
  {
    $$ = new Module_Param_Superset_Template();
    $$->add_list_with_implicit_ids($3);
    delete $3;
  }
| SubsetKeyword '(' TemplateItemList ')'
  {
    $$ = new Module_Param_Subset_Template();
    $$->add_list_with_implicit_ids($3);
    delete $3;
  }
;

ParameterValueOrNotUsedSymbol:
  ParameterValue
  {
    $$ = $1;
  }
| '-'
  {
    $$ = new Module_Param_NotUsed();
  }
;

TemplateItemList:
  ParameterValue
  {
    $$ = new Vector<Module_Param*>();
    $$->push_back($1);
  }
| TemplateItemList ',' ParameterValue
  {
    $$ = $1;
    $$->push_back($3);
  }
;

FieldValueList:
  FieldValue
  {
    $$ = new Module_Param_Assignment_List();
    $$->add_elem($1);
  }
| FieldValueList ',' FieldValue
  {
    $$ = $1;
    $$->add_elem($3);
  }
;

FieldValue:
  FieldName AssignmentChar ParameterValueOrNotUsedSymbol
  {
    $$ = $3;
    $$->set_id(new Module_Param_FieldName($1));
  }
;

FieldName:
  Identifier
  {
    $$ = $1;
  }
| ASN1LowerIdentifier
  {
    $$ = $1;
  }
;

ArrayItemList:
  ArrayItem
  {
    $$ = new Module_Param_Value_List();
    $1->set_id(new Module_Param_Index($$->get_size(),false));
    $$->add_elem($1);
  }
| ArrayItemList ',' ArrayItem
  {
    $$ = $1;
    $3->set_id(new Module_Param_Index($$->get_size(),false));
    $$->add_elem($3);
  }
;

ArrayItem:
  ParameterValueOrNotUsedSymbol
  {
    $$ = $1;
  }
| PermutationKeyword '(' TemplateItemList ')'
  {
    $$ = new Module_Param_Permutation_Template();
    $$->add_list_with_implicit_ids($3);
    delete $3;
  }
;

IndexItemList:
  IndexItem
  {
    $$ = new Module_Param_Indexed_List();
    $$->add_elem($1);
  }
| IndexItemList ',' IndexItem
  {
    $$ = $1;
    $$->add_elem($3);
  }
;

IndexItem:
  IndexItemIndex AssignmentChar ParameterValue
  {
    $$ = $3;
    $$->set_id(new Module_Param_Index((size_t)$1,true));
  }
;

IndexItemIndex:
	'[' ParameterExpression ']'
{
  $2->set_id(new Module_Param_CustomName(mcopystr("array index")));
  INTEGER tmp;
  tmp.set_param(*$2);
  if (!tmp.get_val().is_native()) {
    config_process_error("bignum index."); // todo
  }
  if (tmp.get_val().is_negative()) {
    config_process_error("negative index."); // todo
  }
  $$ = tmp;
  delete $2;
}
;

/*************** [LOGGING] section *********************************/

LoggingSection:
	LoggingKeyword LoggingParamList
;

LoggingParamList:
  /* empty */
  | LoggingParamList LoggingParamLines optSemiColon
  {
	  // Centralized duplication handling for `[LOGGING]'.
    if ($2.logparam.log_param_selection != LP_UNKNOWN && TTCN_Logger::add_parameter($2)) {
      switch ($2.logparam.log_param_selection) {
      case LP_FILEMASK:
        check_duplicate_option("LOGGING", "FileMask", file_mask_set);
        break;
      case LP_CONSOLEMASK:
        check_duplicate_option("LOGGING", "ConsoleMask", console_mask_set);
        break;
      case LP_LOGFILESIZE:
        check_duplicate_option("LOGGING", "LogFileSize", log_file_size_set);
        break;
      case LP_LOGFILENUMBER:
        check_duplicate_option("LOGGING", "LogFileNumber", log_file_number_set);
        break;
      case LP_DISKFULLACTION:
        check_duplicate_option("LOGGING", "DiskFullAction", log_disk_full_action_set);
        break;
      case LP_LOGFILE:
        check_duplicate_option("LOGGING", "LogFile", file_name_set);
        break;
      case LP_TIMESTAMPFORMAT:
        check_duplicate_option("LOGGING", "TimeStampFormat", timestamp_format_set);
        break;
      case LP_SOURCEINFOFORMAT:
        check_duplicate_option("LOGGING", "SourceInfoFormat", source_info_format_set);
        break;
      case LP_APPENDFILE:
        check_duplicate_option("LOGGING", "AppendFile", append_file_set);
        break;
      case LP_LOGEVENTTYPES:
        check_duplicate_option("LOGGING", "LogEventTypes", log_event_types_set);
        break;
      case LP_LOGENTITYNAME:
        check_duplicate_option("LOGGING", "LogEntityName", log_entity_name_set);
        break;
      case LP_MATCHINGHINTS:
        check_duplicate_option("LOGGING", "MatchingVerbosity", matching_verbosity_set);
        break;
      case LP_PLUGIN_SPECIFIC:
        // It would be an overkill to check for the infinite number of custom parameters...
        check_duplicate_option("LOGGING", "PluginSpecific", plugin_specific_set);
        break;
      default:
        break;
      }
    }
  }
;

LoggingParamLines:
	LoggingParam
	{
		$$.component.id_selector = COMPONENT_ID_ALL;
    $$.component.id_name = NULL;
		$$.plugin_id = NULL;
		$$.logparam = $1;
	}
	| ComponentId '.' LoggingParam
	{
		$$.component = $1;
		$$.plugin_id = NULL;
		$$.logparam = $3;
	}
	| ComponentId '.' LoggerPluginId '.' LoggingParam
	{
		$$.component = $1;
		$$.plugin_id = $3;
		$$.logparam = $5;
	}
	| LoggerPlugins AssignmentChar '{' LoggerPluginList '}'
	{
	  check_duplicate_option("LOGGING", "LoggerPlugins", logger_plugins_set);
	  component_id_t comp;
	  comp.id_selector = COMPONENT_ID_ALL;
	  comp.id_name = NULL;
	  logging_plugin_t *plugin = $4;
	  while (plugin != NULL) {
	    // `identifier' and `filename' are reused.  Various checks and
	    // validations must be done in the logger itself (e.g. looking for
	    // duplicate options).
      TTCN_Logger::register_plugin(comp, plugin->identifier, plugin->filename);
      logging_plugin_t *tmp = plugin;
      plugin = tmp->next;
      Free(tmp);
	  }
	  $$.logparam.log_param_selection = LP_UNKNOWN;
	}
	| ComponentId '.' LoggerPlugins AssignmentChar '{' LoggerPluginList '}'
	{
    check_duplicate_option("LOGGING", "LoggerPlugins", logger_plugins_set);
    logging_plugin_t *plugin = $6;
    while (plugin != NULL) {
      TTCN_Logger::register_plugin($1, plugin->identifier, plugin->filename);
      logging_plugin_t *tmp = plugin;
      plugin = tmp->next;
      Free(tmp);
    }
    // Component names shall be duplicated in `register_plugin()'.
    if ($1.id_selector == COMPONENT_ID_NAME)
      Free($1.id_name);
    $$.logparam.log_param_selection = LP_UNKNOWN;
	}
;

LoggerPluginList:
  LoggerPlugin
  {
    $$ = $1;
  }
  | LoggerPluginList ',' LoggerPlugin
  {
    $$ = $3;
    $$->next = $1;
  }
;

LoggerPlugin:
  Identifier
  {
    $$ = (logging_plugin_t *)Malloc(sizeof(logging_plugin_t));
    $$->identifier = $1;
    $$->filename = NULL;
    $$->next = NULL;
  }
  | Identifier AssignmentChar StringValue
  {
    $$ = (logging_plugin_t *)Malloc(sizeof(logging_plugin_t));
    $$->identifier = $1;
    $$->filename = $3;
    $$->next = NULL;
  }
;

LoggerPluginId:
  '*' { $$ = mcopystr("*"); }
  | Identifier { $$ = $1; }
;

LoggingParam:
  FileMask AssignmentChar LoggingBitmask
  {
    $$.log_param_selection = LP_FILEMASK;
    $$.logoptions_val = $3;
  }
  | ConsoleMask AssignmentChar LoggingBitmask
  {
    $$.log_param_selection = LP_CONSOLEMASK;
    $$.logoptions_val = $3;
  }
  | LogFileSize AssignmentChar Number
  {
    $$.log_param_selection = LP_LOGFILESIZE;
    $$.int_val = (int)$3->get_val();
    delete $3;
  }
  | EmergencyLogging AssignmentChar Number
  {
    $$.log_param_selection = LP_EMERGENCY;
    $$.int_val = (int)$3->get_val();
    delete $3;
  }
  | EmergencyLoggingBehaviour AssignmentChar EmergencyLoggingBehaviourValue
  {
    $$.log_param_selection = LP_EMERGENCYBEHAVIOR;
    $$.emergency_logging_behaviour_value = $3;
  }
  | EmergencyLoggingMask AssignmentChar LoggingBitmask
  {
    $$.log_param_selection = LP_EMERGENCYMASK;
    $$.logoptions_val = $3;
  }
  | EmergencyLoggingForFailVerdict AssignmentChar YesNoOrBoolean
  {
    $$.log_param_selection = LP_EMERGENCYFORFAIL;
    $$.bool_val = $3;
  }
  | LogFileNumber AssignmentChar Number
  {
    $$.log_param_selection = LP_LOGFILENUMBER;
    $$.int_val = (int)$3->get_val();
    delete $3;
  }
  | DiskFullAction AssignmentChar DiskFullActionValue
  {
    $$.log_param_selection = LP_DISKFULLACTION;
    $$.disk_full_action_value = $3;
  }
  | LogFile AssignmentChar LogFileName
  {
    $$.log_param_selection = LP_LOGFILE;
    $$.str_val = $3;
  }
  | TimestampFormat AssignmentChar TimestampValue
  {
    $$.log_param_selection = LP_TIMESTAMPFORMAT;
    $$.timestamp_value = $3;
  }
  | ConsoleTimestampFormat AssignmentChar TimestampValue
  {
    $$.log_param_selection = LP_UNKNOWN;
  }
  | SourceInfoFormat AssignmentChar SourceInfoSetting
  {
    $$.log_param_selection = LP_SOURCEINFOFORMAT;
    $$.source_info_value = $3;
  }
  | AppendFile AssignmentChar YesNoOrBoolean
  {
    $$.log_param_selection = LP_APPENDFILE;
    $$.bool_val = $3;
  }
  | LogEventTypes AssignmentChar LogEventTypesValue
  {
    $$.log_param_selection = LP_LOGEVENTTYPES;
    $$.log_event_types_value = $3;
  }
  | LogEntityName AssignmentChar YesNoOrBoolean
  {
    $$.log_param_selection = LP_LOGENTITYNAME;
    $$.bool_val = $3;
  }
  | MatchingHints AssignmentChar VerbosityValue
  {
    $$.log_param_selection = LP_MATCHINGHINTS;
    $$.matching_verbosity_value = $3;
  }
  | Identifier AssignmentChar StringValue
  {
    $$.log_param_selection = LP_PLUGIN_SPECIFIC;
    $$.param_name = $1;
    $$.str_val = $3;
  }
;

VerbosityValue:
	Compact { $$ = TTCN_Logger::VERBOSITY_COMPACT; }
	| Detailed { $$ = TTCN_Logger::VERBOSITY_FULL; }
	;

DiskFullActionValue:
	Error { $$.type = TTCN_Logger::DISKFULL_ERROR; }
	| Stop { $$.type = TTCN_Logger::DISKFULL_STOP; }
	| Retry
{
    $$.type = TTCN_Logger::DISKFULL_RETRY;
    $$.retry_interval = 30; /* default retry interval */
}
	| Retry '(' Number ')'
{
    $$.type = TTCN_Logger::DISKFULL_RETRY;
    $$.retry_interval = (size_t)$3->get_val();
    delete $3;
}
	| Delete { $$.type = TTCN_Logger::DISKFULL_DELETE; }
;

LogFileName:
	StringValue { $$ = $1; }
;

LoggingBitOrCollection:
	LoggingBit
{
    $$.clear();
    $$.add_sev($1);
}
	| LoggingBitCollection
{
    $$.clear();
    switch($1)
    {
    case TTCN_Logger::ACTION_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::ACTION_UNQUALIFIED);
	break;
    case TTCN_Logger::DEFAULTOP_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::DEFAULTOP_ACTIVATE);
	$$.add_sev(TTCN_Logger::DEFAULTOP_DEACTIVATE);
	$$.add_sev(TTCN_Logger::DEFAULTOP_EXIT);
	$$.add_sev(TTCN_Logger::DEFAULTOP_UNQUALIFIED);
	break;
    case TTCN_Logger::ERROR_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::ERROR_UNQUALIFIED);
	break;
    case TTCN_Logger::EXECUTOR_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::EXECUTOR_RUNTIME);
	$$.add_sev(TTCN_Logger::EXECUTOR_CONFIGDATA);
	$$.add_sev(TTCN_Logger::EXECUTOR_EXTCOMMAND);
	$$.add_sev(TTCN_Logger::EXECUTOR_COMPONENT);
	$$.add_sev(TTCN_Logger::EXECUTOR_LOGOPTIONS);
	$$.add_sev(TTCN_Logger::EXECUTOR_UNQUALIFIED);
	break;
    case TTCN_Logger::FUNCTION_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::FUNCTION_RND);
	$$.add_sev(TTCN_Logger::FUNCTION_UNQUALIFIED);
	break;
    case TTCN_Logger::PARALLEL_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::PARALLEL_PTC);
	$$.add_sev(TTCN_Logger::PARALLEL_PORTCONN);
	$$.add_sev(TTCN_Logger::PARALLEL_PORTMAP);
	$$.add_sev(TTCN_Logger::PARALLEL_UNQUALIFIED);
	break;
    case TTCN_Logger::PORTEVENT_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::PORTEVENT_PQUEUE);
	$$.add_sev(TTCN_Logger::PORTEVENT_MQUEUE);
	$$.add_sev(TTCN_Logger::PORTEVENT_STATE);
	$$.add_sev(TTCN_Logger::PORTEVENT_PMIN);
	$$.add_sev(TTCN_Logger::PORTEVENT_PMOUT);
	$$.add_sev(TTCN_Logger::PORTEVENT_PCIN);
	$$.add_sev(TTCN_Logger::PORTEVENT_PCOUT);
	$$.add_sev(TTCN_Logger::PORTEVENT_MMRECV);
	$$.add_sev(TTCN_Logger::PORTEVENT_MMSEND);
	$$.add_sev(TTCN_Logger::PORTEVENT_MCRECV);
	$$.add_sev(TTCN_Logger::PORTEVENT_MCSEND);
	$$.add_sev(TTCN_Logger::PORTEVENT_DUALRECV);
	$$.add_sev(TTCN_Logger::PORTEVENT_DUALSEND);
	$$.add_sev(TTCN_Logger::PORTEVENT_UNQUALIFIED);
  $$.add_sev(TTCN_Logger::PORTEVENT_SETSTATE);
	break;
    case TTCN_Logger::TESTCASE_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::TESTCASE_START);
	$$.add_sev(TTCN_Logger::TESTCASE_FINISH);
	$$.add_sev(TTCN_Logger::TESTCASE_UNQUALIFIED);
	break;
    case TTCN_Logger::TIMEROP_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::TIMEROP_READ);
	$$.add_sev(TTCN_Logger::TIMEROP_START);
	$$.add_sev(TTCN_Logger::TIMEROP_GUARD);
	$$.add_sev(TTCN_Logger::TIMEROP_STOP);
	$$.add_sev(TTCN_Logger::TIMEROP_TIMEOUT);
	$$.add_sev(TTCN_Logger::TIMEROP_UNQUALIFIED);
	break;
    case TTCN_Logger::USER_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::USER_UNQUALIFIED);
	break;
    case TTCN_Logger::STATISTICS_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::STATISTICS_VERDICT);
	$$.add_sev(TTCN_Logger::STATISTICS_UNQUALIFIED);
	break;
    case TTCN_Logger::VERDICTOP_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::VERDICTOP_GETVERDICT);
	$$.add_sev(TTCN_Logger::VERDICTOP_SETVERDICT);
	$$.add_sev(TTCN_Logger::VERDICTOP_FINAL);
	$$.add_sev(TTCN_Logger::VERDICTOP_UNQUALIFIED);
	break;
    case TTCN_Logger::WARNING_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::WARNING_UNQUALIFIED);
	break;
    case TTCN_Logger::MATCHING_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::MATCHING_DONE);
	$$.add_sev(TTCN_Logger::MATCHING_TIMEOUT);
	$$.add_sev(TTCN_Logger::MATCHING_PCSUCCESS);
	$$.add_sev(TTCN_Logger::MATCHING_PCUNSUCC);
	$$.add_sev(TTCN_Logger::MATCHING_PMSUCCESS);
	$$.add_sev(TTCN_Logger::MATCHING_PMUNSUCC);
	$$.add_sev(TTCN_Logger::MATCHING_MCSUCCESS);
	$$.add_sev(TTCN_Logger::MATCHING_MCUNSUCC);
	$$.add_sev(TTCN_Logger::MATCHING_MMSUCCESS);
	$$.add_sev(TTCN_Logger::MATCHING_MMUNSUCC);
	$$.add_sev(TTCN_Logger::MATCHING_PROBLEM);
	$$.add_sev(TTCN_Logger::MATCHING_UNQUALIFIED);
	break;
    case TTCN_Logger::DEBUG_UNQUALIFIED:
	$$.add_sev(TTCN_Logger::DEBUG_ENCDEC);
	$$.add_sev(TTCN_Logger::DEBUG_TESTPORT);
	$$.add_sev(TTCN_Logger::DEBUG_USER);
	$$.add_sev(TTCN_Logger::DEBUG_FRAMEWORK);
	$$.add_sev(TTCN_Logger::DEBUG_UNQUALIFIED);
	break;
    case TTCN_Logger::LOG_ALL_IMPORTANT:
	$$ = Logging_Bits::log_all;
	break;
    default:
	/*  The lexer sent something the parser doesn't understand!
	    Parser needs to be updated.
	*/
        TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED,
          "Internal error: unknown logbit from lexer.");
	break;
    } // switch
}
;

LoggingBitmask:
	LoggingBitOrCollection
{
	$$ = $1;
}
	| LoggingBitmask '|' LoggingBitOrCollection
{
	$$ = $1;
	$$.merge($3);
}
;

SourceInfoSetting:
	SourceInfoValue { $$ = $1; }
	| YesNoOrBoolean
{
	$$ = $1 ? TTCN_Logger::SINFO_SINGLE : TTCN_Logger::SINFO_NONE;
}
;

YesNoOrBoolean:
	YesNo { $$ = $1; }
	| BooleanValue { $$ = $1; }
;

LogEventTypesValue:
	YesNoOrBoolean { $$ = $1 ? TTCN_Logger::LOGEVENTTYPES_YES :
			TTCN_Logger::LOGEVENTTYPES_NO; }
	| SubCategories { $$ = TTCN_Logger::LOGEVENTTYPES_SUBCATEGORIES; }
	| Detailed { $$ = TTCN_Logger::LOGEVENTTYPES_SUBCATEGORIES; }
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
  DisableProfilerKeyword AssignmentChar BooleanValue {
    ttcn3_prof.set_disable_profiler($3);
  }
;

DisableCoverageSetting:
  DisableCoverageKeyword AssignmentChar BooleanValue {
    ttcn3_prof.set_disable_coverage($3);
  }
;

DatabaseFileSetting:
  DatabaseFileKeyword AssignmentChar StringValue {
    ttcn3_prof.set_database_filename($3);
  }
;

AggregateDataSetting:
  AggregateDataKeyword AssignmentChar BooleanValue {
    ttcn3_prof.set_aggregate_data($3);
  }
;

StatisticsFileSetting:
  StatisticsFileKeyword AssignmentChar StringValue {
    ttcn3_prof.set_stats_filename($3);
  }
;

DisableStatisticsSetting:
  DisableStatisticsKeyword AssignmentChar BooleanValue {
    ttcn3_prof.set_disable_stats($3);
  }
;

StatisticsFilterSetting:
  StatisticsFilterKeyword AssignmentChar ProfilerStatsFlags {
    ttcn3_prof.reset_stats_flags();
    ttcn3_prof.add_stats_flags($3);
  }
| StatisticsFilterKeyword ConcatChar ProfilerStatsFlags {
    ttcn3_prof.add_stats_flags($3);
  }
;

ProfilerStatsFlags:
  ProfilerStatsFlag                        { $$ = $1; }
| ProfilerStatsFlag '&' ProfilerStatsFlags { $$ = $1 | $3; }
| ProfilerStatsFlag '|' ProfilerStatsFlags { $$ = $1 | $3; }
;

StartAutomaticallySetting:
  StartAutomaticallyKeyword AssignmentChar BooleanValue {
    if ($3) {
      ttcn3_prof.start();
    }
    else {
      ttcn3_prof.stop();
    }
  }
;

NetLineTimesSetting:
  NetLineTimesKeyword AssignmentChar BooleanValue {
    TTCN3_Stack_Depth::set_net_line_times($3);
  }
;

NetFunctionTimesSetting:
  NetFunctionTimesKeyword AssignmentChar BooleanValue {
    TTCN3_Stack_Depth::set_net_func_times($3);
  }
;


/**************** [TESTPORT_PARAMETERS] ****************************/

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
{
	PORT::add_parameter($1, $3, $5, $7);
	if ($1.id_selector == COMPONENT_ID_NAME) Free($1.id_name);
	Free($3);
	Free($5);
	Free($7);
}
;

ComponentId:
	Identifier
{
	$$.id_selector = COMPONENT_ID_NAME;
	$$.id_name = $1;
}
|	Cstring
{
	$$.id_selector = COMPONENT_ID_NAME;
	$$.id_name = $1.chars_ptr;
}
	| Number
{
	$$.id_selector = COMPONENT_ID_COMPREF;
	$$.id_compref = (component)$1->get_val();
	delete $1;
}
	| MTCKeyword
{
	$$.id_selector = COMPONENT_ID_COMPREF;
	$$.id_compref = MTC_COMPREF;
}
	| '*'
{
	$$.id_selector = COMPONENT_ID_ALL;
	$$.id_name = NULL;
}
	| SystemKeyword
{
	$$.id_selector = COMPONENT_ID_SYSTEM;
	$$.id_name = NULL;
}
;

TestportName:
	Identifier { $$ = $1; }
	| Identifier ArrayRef
{
	$$ = mputstr($1, $2);
	Free($2);
}
	| '*' { $$ = NULL; }
;

ArrayRef:
	'[' IntegerValue ']'
{
	char *s = $2->as_string();
	$$ = memptystr();
	$$ = mputc  ($$, '[');
	$$ = mputstr($$, s);
	$$ = mputc  ($$, ']');
	Free(s);
	delete $2;
}
	| ArrayRef '[' IntegerValue ']'
{
	char *s = $3->as_string();
	$$ = mputc  ($1, '[');
	$$ = mputstr($$, s);
	$$ = mputc  ($$, ']');
	Free(s);
	delete $3;
}
;

TestportParameterName:
	Identifier { $$ = $1; }
;

TestportParameterValue:
	StringValue { $$ = $1; }
;

/****************** [EXECUTE] section *************/

ExecuteSection:
	ExecuteKeyword ExecuteList
{
    if (!TTCN_Runtime::is_single()) config_process_error("Internal error: "
	"the Main Controller must not send section [EXECUTE] of the "
	"configuration file.");
}
;

ExecuteList:
	/* empty */
	| ExecuteList ExecuteItem optSemiColon
{
	if (TTCN_Runtime::is_single()) {
		execute_list = (execute_list_item *)Realloc(
			execute_list, (execute_list_len + 1) *
			sizeof(*execute_list));
		execute_list[execute_list_len++] = $2;
	} else {
		Free($2.module_name);
		Free($2.testcase_name);
	}
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

/****************** [EXTERNAL_COMMANDS] section **********************/

ExternalCommandsSection:
	ExternalCommandsKeyword ExternalCommandList
;

ExternalCommandList:
	/* empty */
	| ExternalCommandList ExternalCommand optSemiColon
;

ExternalCommand:
	BeginControlPart AssignmentChar Command
{
    check_duplicate_option("EXTERNAL_COMMANDS", "BeginControlPart",
	begin_controlpart_command_set);

    TTCN_Runtime::set_begin_controlpart_command($3);

    Free($3);
}
	| EndControlPart AssignmentChar Command
{
    check_duplicate_option("EXTERNAL_COMMANDS", "EndControlPart",
	end_controlpart_command_set);

    TTCN_Runtime::set_end_controlpart_command($3);

    Free($3);
}
	| BeginTestCase AssignmentChar Command
{
    check_duplicate_option("EXTERNAL_COMMANDS", "BeginTestCase",
	begin_testcase_command_set);

    TTCN_Runtime::set_begin_testcase_command($3);

    Free($3);
}
	| EndTestCase AssignmentChar Command
{
    check_duplicate_option("EXTERNAL_COMMANDS", "EndTestCase",
	end_testcase_command_set);

    TTCN_Runtime::set_end_testcase_command($3);

    Free($3);
}
;

Command:
	StringValue { $$ = $1; }
;

/***************** [GROUPS] section *******************/

GroupsSection:
	GroupsKeyword GroupList
{
    check_ignored_section("GROUPS");
}
;

GroupList:
	/* empty */
	| GroupList Group optSemiColon
;

Group:
	GroupName AssignmentChar GroupMembers
;

GroupName:
	Identifier { Free($1); }
;

GroupMembers:
	'*'
	| seqGroupMember
;

seqGroupMember:
	HostName
	| seqGroupMember ',' HostName
;

HostName:
	DNSName
	| Identifier { Free($1); }
;

/******************** [COMPONENTS] section *******************/
ComponentsSection:
	ComponentsKeyword ComponentList
{
    check_ignored_section("COMPONENTS");
}
;

ComponentList:
	/* empty */
	| ComponentList ComponentItem optSemiColon
;

ComponentItem:
	ComponentName AssignmentChar ComponentLocation
;

ComponentName:
	Identifier { Free($1); }
	| '*'
;

ComponentLocation:
	Identifier { Free($1); }
	| DNSName
;

/****************** [MAIN_CONTROLLER] section *********************/
MainControllerSection:
	MainControllerKeyword MCParameterList
{
    check_ignored_section("MAIN_CONTROLLER");
}
;

MCParameterList:
	/* empty */
	| MCParameterList MCParameter optSemiColon
;

MCParameter:
	LocalAddress AssignmentChar HostName { }
	| TCPPort AssignmentChar IntegerValue { delete $3; }
	| KillTimer AssignmentChar KillTimerValue { }
	| NumHCs AssignmentChar IntegerValue { delete $3; }
        | UnixSocketEnabled AssignmentChar YesToken {}
        | UnixSocketEnabled AssignmentChar NoToken {}
        | UnixSocketEnabled AssignmentChar HostName {}
;

KillTimerValue:
	FloatValue { }
	| IntegerValue { delete $1; }
;

/****************** [INCLUDE] section *********************/
IncludeSection:
  IncludeKeyword IncludeFiles
  {
    if(!TTCN_Runtime::is_single())
      config_process_error
        ("Internal error: the Main Controller must not send section [INCLUDE]"
         " of the configuration file.");
  }
;

IncludeFiles:
  /* empty */
| IncludeFiles IncludeFile
;

IncludeFile:
  Cstring { Free($1.chars_ptr); }
;

/****************** [DEFINE] section *********************/
DefineSection:
  DefineKeyword
  {
    if(!TTCN_Runtime::is_single())
      config_process_error
        ("Internal error: the Main Controller must not send section [DEFINE]"
         " of the configuration file.");
  }
;

/*********************************************************/

ParamOpType:
  AssignmentChar
  {
    $$ = Module_Param::OT_ASSIGN;
  }
|
  ConcatChar
  {
    $$ = Module_Param::OT_CONCAT;
  }
;

optSemiColon:
	/* empty */
	| ';'
;

%%

static void reset_configuration_options()
{
    /* Section [MODULE_PARAMETERS */
    /** \todo reset module parameters to their default values */
    /* Section [LOGGING] */
    TTCN_Logger::close_file();
    TTCN_Logger::reset_configuration();
    file_name_set = FALSE;
    file_mask_set = TRUE;
    console_mask_set = TRUE;
    timestamp_format_set = FALSE;
    source_info_format_set = FALSE;
    append_file_set = FALSE;
    log_event_types_set = FALSE;
    log_entity_name_set = FALSE;
    /* Section [TESTPORT_PARAMETERS] */
    PORT::clear_parameters();
    /* Section [EXTERNAL_COMMANDS] */

    TTCN_Runtime::clear_external_commands();

    begin_controlpart_command_set = FALSE;
    end_controlpart_command_set = FALSE;
    begin_testcase_command_set = FALSE;
    end_testcase_command_set = FALSE;
}

Module_Param* process_config_string2ttcn(const char* mp_str, boolean is_component)
{
  if (parsed_module_param!=NULL || parsing_error_messages!=NULL) TTCN_error("Internal error: previously parsed ttcn string was not cleared.");
  // add the hidden keyword
  std::string mp_string = (is_component) ? std::string("$#&&&(#TTCNSTRINGPARSING_COMPONENT$#&&^#% ") + mp_str
    : std::string("$#&&&(#TTCNSTRINGPARSING$#&&^#% ") + mp_str;
  struct yy_buffer_state *flex_buffer = config_process__scan_bytes(mp_string.c_str(), (int)mp_string.size());
  if (flex_buffer == NULL) TTCN_error("Internal error: flex buffer creation failed.");
  reset_config_process_lex(NULL);
  error_flag = FALSE;
  try {
    Ttcn_String_Parsing ttcn_string_parsing;
    if (config_process_parse()) error_flag = TRUE;
  } catch (const TC_Error& TC_error) {
    if (parsed_module_param!=NULL) { delete parsed_module_param; parsed_module_param = NULL; }
    error_flag = TRUE;
  }
  config_process_close();
  config_process_lex_destroy();

  if (error_flag || parsing_error_messages!=NULL) {
    delete parsed_module_param;
    parsed_module_param = NULL;
    char* pem = parsing_error_messages!=NULL ? parsing_error_messages : mcopystr("Unknown parsing error");
    parsing_error_messages = NULL;
    TTCN_error_begin("%s", pem);
    Free(pem);
    TTCN_error_end();
    return NULL;
  } else {
    if (parsed_module_param==NULL) TTCN_error("Internal error: could not parse ttcn string.");
    Module_Param* ret_val = parsed_module_param;
    parsed_module_param = NULL;
    return ret_val;
  }
}

Module_Param* process_config_debugger_value(const char* mp_str)
{
  if (parsed_module_param != NULL || parsing_error_messages != NULL) {
    ttcn3_debugger.print(DRET_NOTIFICATION,
      "Internal error: previously parsed TTCN string was not cleared.");
    return NULL;
  }
  // add the hidden keyword
  std::string mp_string = std::string("$#&&&(#TTCNSTRINGPARSING$#&&^#% ") + mp_str;
  struct yy_buffer_state *flex_buffer = config_process__scan_bytes(mp_string.c_str(), (int)mp_string.size());
  if (flex_buffer == NULL) {
    ttcn3_debugger.print(DRET_NOTIFICATION, "Internal error: flex buffer creation failed.");
    return NULL;
  }
  reset_config_process_lex(NULL);
  error_flag = FALSE;
  try {
    Debugger_Value_Parsing debugger_value_parsing;
    if (config_process_parse()) {
      error_flag = TRUE;
    }
  }
  catch (const TC_Error& TC_error) {
    if (parsed_module_param != NULL) {
      delete parsed_module_param;
      parsed_module_param = NULL;
    }
    error_flag = TRUE;
  }
  config_process_close();
  config_process_lex_destroy();

  if (error_flag || parsing_error_messages != NULL) {
    delete parsed_module_param;
    parsed_module_param = NULL;
    char* pem = parsing_error_messages != NULL ? parsing_error_messages :
      mcopystr("Unknown parsing error");
    parsing_error_messages = NULL;
    ttcn3_debugger.print(DRET_NOTIFICATION, "%s", pem);
    Free(pem);
    return NULL;
  }
  else {
    if (parsed_module_param == NULL) {
      ttcn3_debugger.print(DRET_NOTIFICATION, "Internal error: could not parse TTCN string.");
      return NULL;
    }
    Module_Param* ret_val = parsed_module_param;
    parsed_module_param = NULL;
    return ret_val;
  }
}

boolean process_config_string(const char *config_string, int string_len)
{
  error_flag = FALSE;

  struct yy_buffer_state *flex_buffer =
    config_process__scan_bytes(config_string, string_len);
  if (flex_buffer == NULL) {
    TTCN_Logger::log_str(TTCN_Logger::ERROR_UNQUALIFIED,
      "Internal error: flex buffer creation failed.");
    return FALSE;
  }

  try {
    reset_configuration_options();
    reset_config_process_lex(NULL); 

    if (config_process_parse()) error_flag = TRUE;

  } catch (const TC_Error& TC_error) {
    error_flag = TRUE;
  }

  config_process_close();
  config_process_lex_destroy();

  return !error_flag;
}


boolean process_config_file(const char *file_name)
{
  error_flag = FALSE;
  string_chain_t *filenames=NULL;

  reset_configuration_options();

  if(preproc_parse_file(file_name, &filenames, &config_defines))
    error_flag=TRUE;

  while(filenames) {
    char *fn=string_chain_cut(&filenames);
    reset_config_process_lex(fn);
    /* The lexer can modify config_process_in
     * when it's input buffer is changed */
    config_process_in = fopen(fn, "r");
    FILE* tmp_cfg = config_process_in;
    if (config_process_in != NULL) {
      try {
	if(config_process_parse()) error_flag=TRUE;
      } catch (const TC_Error& TC_error) {
	error_flag=TRUE;
      }
      fclose(tmp_cfg);
      config_process_close();
      config_process_lex_destroy();
    } else {
      TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
      TTCN_Logger::log_event("Cannot open configuration file: %s", fn);
      TTCN_Logger::OS_error();
      TTCN_Logger::end_event();
      error_flag=TRUE;
    }
    /* During parsing flex or libc may use some system calls (e.g. ioctl)
     * that fail with an error status. Such error codes shall be ignored in
     * future error messages. */
    errno = 0;

    Free(fn);
  }

  string_map_free(config_defines);
  config_defines = NULL;

  return !error_flag;
}

void config_process_error_f(const char *error_str, ...)
{
  if (Ttcn_String_Parsing::happening() || Debugger_Value_Parsing::happening()) {
    va_list p_var;
    va_start(p_var, error_str);
    char* error_msg_str = mprintf_va_list(error_str, p_var);
    va_end(p_var);
    if (parsing_error_messages!=NULL) parsing_error_messages = mputc(parsing_error_messages, '\n');
    if (Debugger_Value_Parsing::happening()) {
      parsing_error_messages = mputprintf(parsing_error_messages,
        "Parse error at or before token `%s': %s", config_process_text, error_msg_str);
    }
    else { // Ttcn_String_Parsing::happening()
      parsing_error_messages = mputprintf(parsing_error_messages,
        "Parse error in line %d, at or before token `%s': %s", config_process_get_current_line(), config_process_text, error_msg_str);
    }
    Free(error_msg_str);
    error_flag = TRUE;
    return;
  }
  TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
  if (!get_cfg_process_current_file().empty()) {
    TTCN_Logger::log_event("Parse error in configuration file `%s': in line %d, "
                          "at or before token `%s': ",
      get_cfg_process_current_file().c_str(), config_process_get_current_line(),
      config_process_text
      );
  } else {
    TTCN_Logger::log_event("Parse error while reading configuration "
      "information: in line %d, at or before token `%s': ",
      config_process_get_current_line(), config_process_text);
  }
  va_list pvar;
  va_start(pvar, error_str);
  TTCN_Logger::log_event_va_list(error_str, pvar);
  va_end(pvar);
  TTCN_Logger::end_event();
  error_flag = TRUE;
}

void config_process_error(const char *error_str)
{
  config_process_error_f("%s", error_str);
}

void config_preproc_error(const char *error_str, ...)
{
    TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
    TTCN_Logger::log_event("Parse error while pre-processing configuration"
                           " file `%s': in line %d: ",
                           get_cfg_preproc_current_file().c_str(),
                           config_preproc_yylineno);
    va_list pvar;
    va_start(pvar, error_str);
    TTCN_Logger::log_event_va_list(error_str, pvar);
    va_end(pvar);
    TTCN_Logger::end_event();
    error_flag = TRUE;
}

void path_error(const char *fmt, ...)
{
  va_list parameters;
  fprintf(stderr, "File error: ");
  va_start(parameters, fmt);
  vfprintf(stderr, fmt, parameters);
  va_end(parameters);
  fprintf(stderr, "\n");
}

static void check_duplicate_option(const char *section_name,
    const char *option_name, boolean& option_flag)
{
  if (option_flag) {
    TTCN_warning("Option `%s' was given more than once in section [%s] of the "
      "configuration file.", option_name, section_name);
  } else option_flag = TRUE;
}

static void check_ignored_section(const char *section_name)
{
  if (TTCN_Runtime::is_single()) TTCN_warning("Section [%s] of "
      "configuration file is ignored in single mode.", section_name);
  else config_process_error_f("Internal error: the Main Controller must not "
      "send section [%s] of the configuration file.", section_name);
}

static void set_param(Module_Param& param)
{
  try {
    Module_List::set_param(param);
  }
  catch (const TC_Error& TC_error) {
    error_flag = TRUE;
  }
}

unsigned char char_to_hexdigit_(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  else {
    config_process_error_f("char_to_hexdigit_(): invalid argument: %c", c);
    return 0; // to avoid warning
  }
}

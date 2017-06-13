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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Dimitrov, Peter
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
/* Syntax check parser and compiler for TTCN-3 */

/* BNF compliance: v3.2.1 with extensions */

%{

/* C declarations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../../common/dbgnew.hh"
#include "../../common/memory.h"

#include "../datatypes.h"
#include "Attributes.hh"
#include "../main.hh"
#include "compiler.h"

#include "../Identifier.hh"
#include "Templatestuff.hh"
#include "AST_ttcn3.hh"
#include "../Type.hh"
#include "../CompType.hh"
#include "../CompField.hh"
#include "../EnumItem.hh"
#include "../SigParam.hh"

#include "../subtype.hh"
#include "../Value.hh"
#include "../Valuestuff.hh"
#include "../ustring.hh"
#include "Ttcnstuff.hh"
#include "TtcnTemplate.hh"
#include "Templatestuff.hh"
#include "ArrayDimensions.hh"
#include "PatternString.hh"
#include "Statement.hh"

using namespace Ttcn;
using namespace Common;

const char *infile = NULL;

static Ttcn::Module *act_ttcn3_module = NULL;
static Ttcn::ErroneousAttributeSpec *act_ttcn3_erroneous_attr_spec = NULL;
bool is_erroneous_parsed = false;
static void ttcn3_error(const char *str);
static Group* act_group = NULL;
extern string anytype_field(const string& type_name);
static bool anytype_access = false;

#ifndef NDEBUG

union YYSTYPE;
static void yyprint(FILE *file, int type, const YYSTYPE& value);
#define YYPRINT(f,t,v) yyprint(f,t,v)

#endif

extern Modules *modules;

extern FILE *ttcn3_in;
extern char *ttcn3_text;
extern int ttcn3_lex();
extern void init_ttcn3_lex();
extern void free_ttcn3_lex();
extern void set_md5_checksum(Ttcn::Module *m);

extern void init_erroneous_lex(const char* p_infile, int p_line, int p_column);
struct yy_buffer_state;
extern int ttcn3_lex_destroy(void);
extern yy_buffer_state *ttcn3__scan_string(const char *yy_str);
extern void free_dot_flag_stuff();

extern string *parse_charstring_value(const char *str, const Location& loc);
extern PatternString* parse_pattern(const char *str, const Location& loc);

static const string anyname("anytype");

/* Various C macros */

#define YYERROR_VERBOSE


%}

/* Bison declarations */

/*********************************************************************
 * The union-type
 *********************************************************************/

%union {
  /* NOTE: the union is written to compiler.tab.hh, which is #included
   * into compiler.l; therefore all types used here must be declared
   * in compiler.l (forward declared or #included) */
  bool bool_val; /* boolean value */
  char *str; /* simple string value */
  unsigned char uchar_val;
  int_val_t *int_val; /* integer value */
  Real float_val; /* float value */
  Identifier *id;
  string *string_val;
  ustring *ustring_val;

  Type::typetype_t typetype;
  PortTypeBody::PortOperationMode_t portoperationmode;
  Value::operationtype_t operationtype;
  Value::macrotype_t macrotype;
  SingleWithAttrib::attribtype_t attribtype;
  ImpMod::imptype_t imptype;

  AltGuard *altguard;
  AltGuards *altguards;
  ArrayDimension *arraydimension;
  AttributeSpec *attributespec;
  CompField *compfield;
  CompFieldMap *compfieldmap;
  Def_Type *deftype;
  Def_Timer *deftimer;
  Definition *definition;
  Definitions *defs;
  EnumItem *enumitem;
  EnumItems *enumitems;
  FieldOrArrayRef *fieldorarrayref;
  FormalPar *formalpar;
  FormalParList *formalparlist;
  Group *group;
  FriendMod *friendMod;
  IfClause *ifclause;
  IfClauses *ifclauses;
  ImpMod *impmod;
  LengthRestriction *lenrestr;
  LogArgument *logarg;
  LogArguments *logargs;
  NamedTemplate *namedtempl;
  NamedTemplates *namedtempls;
  NamedValue *namedvalue;
  NamedValues *namedvalues;
  IndexedTemplate *indexedtempl;
  IndexedTemplates *indexedtempls;
  IndexedValue *indexedvalue;
  MultiWithAttrib *multiwithattrib;
  OID_comp *oidcomp;
  ParamAssignment *parass;
  ParamAssignments *parasss;
  ParamRedirect *parredir;
  ParsedActualParameters *parsedpar;
  PatternString *patstr;
  Qualifier *qualifier;
  Qualifiers *qualifiers;
  SelectCase *selectcase;
  SelectCases *selectcases;
  SelectUnion *selectunion;
  SelectUnions *selectunions;
  SignatureExceptions *signexc;
  SignatureParam *signparam;
  SignatureParamList *signparamlist;
  SingleWithAttrib *singlewithattrib;
  Statement *stmt;
  StatementBlock *statementblock;
  SubTypeParse *subtypeparse;
  Template *templ;
  TemplateInstance *templinst;
  TemplateInstances *templinsts;
  Templates *templs;
  Ttcn::Assignment *ass;
  Ttcn::Ref_base *refbase;
  Ttcn::Ref_pard *refpard;
  Ttcn::Reference *reference;
  ValueRange *valuerange;
  Type *type;
  Types *types;
  Value *value;
  Values *values;
  VariableEntries *variableentries;
  VariableEntry *variableentry;
  vector<SubTypeParse> *subtypeparses;
  CompTypeRefList *comprefs;
  ComponentTypeBody *compbody;
  template_restriction_t template_restriction;
  ValueRedirect* value_redirect;
  SingleValueRedirect* single_value_redirect;
  param_eval_t eval;
  TypeMappingTargets *typemappingtargets;

  struct {
    bool is_raw;
    ErroneousAttributeSpec::indicator_t indicator;
  } erroneous_indicator;

  struct arraydimension_list_t {
    size_t nElements;
    ArrayDimension **elements;
  } arraydimension_list;

  struct {
    size_t nElements;
    FieldOrArrayRef **elements;
  } fieldorarrayref_list;

  struct {
    size_t nElements;
    Ttcn::Definition **elements;
  } def_list;

  struct {
    size_t nElements;
    Ttcn::FriendMod **elements;
  } friend_list;

  struct {
    size_t nElements;
    Statement **elements;
  } stmt_list;

  struct {
    size_t nElements;
    const char **elements;
  } uid_list;

  struct {
    Value *lower;
    bool lower_exclusive;
    Value *upper;
    bool upper_exclusive;
  } rangedef;

  struct {
    Type *type;
    bool returns_template;
    template_restriction_t template_restriction;
  } returntype;

  struct {
    Type *type;
    bool no_block_kw;
  } returntypeornoblock;

  struct {
    Identifier *id;
    CompFieldMap* cfm;
  } structdefbody;

  struct {
    Type *type;
    Identifier *id;
  } structofdefbody;

  struct {
    Ttcn::Types *in_list, *out_list, *inout_list;
    bool in_all, out_all, inout_all;
    TypeMappings *in_mappings, *out_mappings;
    size_t varnElements;
    Ttcn::Definition **varElements;
  } portdefbody;
  
  struct {
    Types * types;
    TypeMappings *mappings;
  } types_with_mapping;

  struct {
    Ttcn::Reference *ref;
    Identifier *id;
  } ischosenarg;

  struct {
    bool is_ifpresent;
    LengthRestriction *len_restr;
  } extramatchingattrs;

  struct {
    Identifier *name;
    Type *type;
    FormalParList *fp_list;
  } basetemplate;

  struct {
    Identifier *modid;
    Identifier *id;
  } templateref;

  struct {
    Ttcn::Ref_pard *ref_pard;
    Value *derefered_value;
    ParsedActualParameters *ap_list;
    Value *value;
  } testcaseinst;

  struct {
    Ttcn::Ref_pard *ref_pard;
    Value *derefered_value;
    TemplateInstances *ap_list;
  } activateop;

  struct {
    TemplateInstance *templ_inst;
    Value *val;
  } portsendop;

  struct {
    Value *calltimerval; // if NULL: see nowait
    bool nowait;
  } calltimerval;

  struct {
    TemplateInstance *templ_inst;
    Value *calltimerval; // if NULL: see nowait
    bool nowait;
    Value *val; // not used in callparams
  } portcallop;

  struct {
    TemplateInstance *templ_inst;
    Value *replyval;
    Value *toclause;
  } portreplyop;

  struct {
    Ttcn::Reference *signature;
    TemplateInstance *templ_inst;
    Value *toclause;
  } portraiseop;

  struct {
    ValueRedirect *redirectval;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portredirect;

  struct {
    ParamRedirect *redirectparam;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portredirectwithparam;

  struct {
    ValueRedirect *redirectval;
    ParamRedirect *redirectparam;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portredirectwithvalueandparam;

  struct {
    TemplateInstance *templ_inst;
    TemplateInstance *valuematch;
  } getreplypar;

  struct {
    TemplateInstance *templ_inst;
    TemplateInstance *fromclause;
    ValueRedirect *redirectval;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portreceiveop;

  struct {
    TemplateInstance *templ_inst;
    TemplateInstance *fromclause;
    ParamRedirect *redirectparam;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portgetcallop;

  struct {
    TemplateInstance *templ_inst;
    TemplateInstance *valuematch;
    TemplateInstance *fromclause;
    ValueRedirect *redirectval;
    ParamRedirect *redirectparam;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portgetreplyop;

  struct {
    Ttcn::Reference *signature;
    TemplateInstance *templ_inst;
    bool timeout;
  } catchoppar;

  struct {
    Ttcn::Reference *signature;
    TemplateInstance *templ_inst;
    bool timeout;
    TemplateInstance *fromclause;
    ValueRedirect *redirectval;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portcatchop;

  struct {
    Statement::statementtype_t statementtype;
    Ttcn::Reference *signature;
    TemplateInstance *templ_inst;
    TemplateInstance *valuematch;
    bool timeout;
    TemplateInstance *fromclause;
    ValueRedirect *redirectval;
    ParamRedirect *redirectparam;
    Ttcn::Reference *redirectsender;
    Ttcn::Reference* redirectindex;
  } portcheckop;

  struct {
    Value *compref;
    Ttcn::Reference *portref;
  } portref;

  struct {
    Value *compref1;
    Ttcn::Reference *portref1;
    Value *compref2;
    Ttcn::Reference *portref2;
  } connspec;

  struct {
    TemplateInstance *donematch;
    ValueRedirect *valueredirect;
    Ttcn::Reference* indexredirect;
  } donepar;

  struct {
    bool is_ref;
    union {
      Ttcn::Reference *ref;
      Identifier *id;
    };
  } reforid;

  struct {
    Definitions *defs;
    Ttcn::Assignment *ass;
  } initial;

  struct {
    Ttcn::Reference *runsonref;
    Ttcn::Reference *systemref;
  } configspec;

  struct {
    Value *name;
    Value *loc;
  } createpar;

  struct {
    Value *value;
    ParsedActualParameters *ap_list;
  } applyop;


  struct extconstidentifier_t {
    Identifier *id;
    YYLTYPE yyloc;
  } extconstidentifier;

  struct {
    size_t nElements;
    extconstidentifier_t *elements;
  } identifier_list;

  struct singlevarinst_t {
    Identifier *id;
    arraydimension_list_t arrays;
    Value *initial_value;
    YYLTYPE yyloc;
  } singlevarinst;

  struct singlevarinst_list_t {
    size_t nElements;
    struct singlevarinst_t *elements;
  } singlevarinst_list;

  struct singletempvarinst_t {
    Identifier *id;
    arraydimension_list_t arrays;
    Template *initial_value;
    YYLTYPE yyloc;
  } singletempvarinst;

  struct singletempvarinst_list_t {
    size_t nElements;
    singletempvarinst_t *elements;
  } singletempvarinst_list;

  struct singlemodulepar_t {
    Identifier *id;
    Value *defval;
    YYLTYPE yyloc;
  } singlemodulepar;

  struct singletemplatemodulepar_t {
    Identifier *id;
    Template *deftempl;
    YYLTYPE yyloc;
  } singletemplatemodulepar;

  struct singlemodulepar_list_t {
    size_t nElements;
    singlemodulepar_t *elements;
  } singlemodulepar_list;

  struct singletemplatemodulepar_list_t {
    size_t nElements;
    singletemplatemodulepar_t *elements;
  } singletemplatemodulepar_list;

  struct portelement_t {
    Identifier *id;
    ArrayDimensions *dims;
    YYLTYPE yyloc;
  } portelement;

  struct portelement_list_t {
    size_t nElements;
    portelement_t *elements;
  } portelement_list;

  struct runs_on_compref_or_self_t {
    bool self;
    Ttcn::Reference *reference;
  } runs_on_compref_or_self;

  struct {
    visibility_t visibility;
  } visbilitytype;

  struct {
    Value* string_encoding;
    TemplateInstance* target_template;
  } decode_match;

  struct {
    bool is_decoded;
    Value* string_encoding;
  } decoded_modifier;

  struct {
    size_t nElements;
    SingleValueRedirect** elements;
  } single_value_redirect_list;

  struct {
    Ttcn::Reference* reference;
    bool any_from;
  } reference_or_any;
  
  struct {
    size_t nElements;
    Ttcn::Reference** elements;
  } reference_list;
}

/* Tokens of TTCN-3 */

/*********************************************************************
 * Tokens with semantic value
 *********************************************************************/

/* Terminals with semantic value  */

%token <int_val> Number
%token <float_val> FloatValue
%token <id> IDentifier "Identifier"

%token <string_val> Bstring
  Hstring
  Ostring
  BitStringMatch
  HexStringMatch
  OctetStringMatch
%token <str> Cstring
%token NullValue "ASN.1_NULL_value"
%token <macrotype> MacroValue

/*********************************************************************
 * Tokens without semantic value
 *********************************************************************/

/* Terminals without semantic value - keywords, operators, etc. */

%token TOK_errval "erroneous_value"

/* A number of terminals (including ApplyKeyword, CallOpKeyword, etc)
 * are listed as unused by Bison. They do not appear in any rule,
 * because the lexer does some magic to combine them with the preceding dot
 * and returns a (DotApplyKeyword, DotCallOpKeyword, etc) instead.
 * This magic requires the presence of the unused keywords.
 * (It can return an ApplyKeyword if not preceded by a dot) */
%token TitanErroneousHackKeyword
%token ActionKeyword
%token ActivateKeyword
%token AddressKeyword
%token AliveKeyword
%token AllKeyword
%token AltKeyword
%token AltstepKeyword
%token AndKeyword
%token And4bKeyword
%token AnyKeyword
%token AnyTypeKeyword
%token ApplyKeyword
%token BitStringKeyword
%token BooleanKeyword
%token BreakKeyword
%token CallOpKeyword
%token CaseKeyword
%token CatchOpKeyword
%token CharKeyword
%token CharStringKeyword
%token CheckOpKeyword
%token CheckStateKeyword
%token ClearOpKeyword
%token ComplementKeyword
%token ComponentKeyword
%token ConnectKeyword
%token ConstKeyword
%token ContinueKeyword
%token ControlKeyword
%token CreateKeyword
%token DecodedMatchKeyword
%token DeactivateKeyword
%token DefaultKeyword
%token DerefersKeyword
%token DisconnectKeyword
%token DisplayKeyword
%token DoKeyword
%token DoneKeyword
%token ElseKeyword
%token EncodeKeyword
%token EnumKeyword
%token ErrorKeyword
%token ExceptKeyword
%token ExceptionKeyword
%token ExecuteKeyword
%token ExtendsKeyword
%token ExtensionKeyword
%token ExtKeyword
%token FailKeyword
%token FalseKeyword
%token FloatKeyword
%token ForKeyword
%token FriendKeyword
%token FromKeyword
%token FunctionKeyword
%token GetCallOpKeyword
%token GetReplyOpKeyword
%token GetVerdictKeyword
%token GotoKeyword
%token GroupKeyword
%token HaltKeyword
%token HexStringKeyword
%token IfKeyword
%token IfPresentKeyword
%token ImportKeyword
%token InconcKeyword
%token InfinityKeyword
%token InOutParKeyword
%token InParKeyword
%token IntegerKeyword
%token InterleavedKeyword
%token KillKeyword
%token KilledKeyword
%token LabelKeyword
%token LanguageKeyword
%token LengthKeyword
%token LogKeyword
%token MapKeyword
%token MatchKeyword
%token MessageKeyword
%token MixedKeyword
%token ModKeyword
%token ModifiesKeyword
%token ModuleParKeyword
%token MTCKeyword
%token NaNKeyword
%token NoBlockKeyword
%token NoneKeyword
%token NotKeyword
%token Not4bKeyword
%token NowaitKeyword
%token NullKeyword
%token ObjectIdentifierKeyword
%token OctetStringKeyword
%token OfKeyword
%token OmitKeyword
%token OnKeyword
%token OptionalKeyword
%token OrKeyword
%token Or4bKeyword
%token OutParKeyword
%token OverrideKeyword
%token PassKeyword
%token ParamKeyword
%token PatternKeyword
%token PermutationKeyword
%token PresentKeyword
%token PortKeyword
%token PrivateKeyword
%token ProcedureKeyword
%token PublicKeyword
%token RaiseKeyword
%token ReadKeyword
%token ReceiveOpKeyword
%token RecordKeyword
%token RecursiveKeyword
%token RefersKeyword
%token RemKeyword
%token RepeatKeyword
%token ReplyKeyword
%token ReturnKeyword
%token RunningKeyword
%token RunsKeyword
%token SelectKeyword
%token SelfKeyword
%token SenderKeyword
%token SendOpKeyword
%token SetKeyword
%token SetstateKeyword
%token SetVerdictKeyword
%token SignatureKeyword
%token StartKeyword
%token StopKeyword
%token SubsetKeyword
%token SupersetKeyword
%token SystemKeyword
%token TemplateKeyword
%token TestcaseKeyword
%token TimeoutKeyword
%token TimerKeyword
%token ToKeyword
%token TriggerOpKeyword
%token TrueKeyword
%token TTCN3ModuleKeyword
%token TypeDefKeyword
%token UnionKeyword
%token UniversalKeyword
%token UnmapKeyword
%token ValueKeyword
%token ValueofKeyword
%token VarKeyword
%token VariantKeyword
%token VerdictTypeKeyword
%token WhileKeyword
%token WithKeyword
%token XorKeyword
%token Xor4bKeyword

/* modifier keywords */
%token NocaseKeyword
%token LazyKeyword
%token DecodedKeyword
%token DeterministicKeyword
%token FuzzyKeyword
%token IndexKeyword

/* TITAN specific keywords */
%token TitanSpecificTryKeyword
%token TitanSpecificCatchKeyword
%token TitanSpecificProfilerKeyword
%token TitanSpecificUpdateKeyword

/* Keywords combined with a leading dot */

/* If a '.' (dot) character is followed by one of the keywords below the
 * lexical analyzer shall return one combined token instead of two distinct
 * tokens. This eliminates the ambiguity that causes additional shift/reduce
 * conflicts because the dot can be either the part of a field reference or a
 * built-in operation denoted by a keyword. */

%token DotAliveKeyword
%token DotApplyKeyword
%token DotCallOpKeyword
%token DotCatchOpKeyword
%token DotCheckOpKeyword
%token DotCheckStateKeyword
%token DotClearOpKeyword
%token DotCreateKeyword
%token DotDoneKeyword
%token DotGetCallOpKeyword
%token DotGetReplyOpKeyword
%token DotHaltKeyword
%token DotKillKeyword
%token DotKilledKeyword
%token DotRaiseKeyword
%token DotReadKeyword
%token DotReceiveOpKeyword
%token DotReplyKeyword
%token DotRunningKeyword
%token DotSendOpKeyword
%token DotStartKeyword
%token DotStopKeyword
%token DotTimeoutKeyword
%token DotTriggerOpKeyword

/* Predefined function identifiers */

%token bit2hexKeyword
%token bit2intKeyword
%token bit2octKeyword
%token bit2strKeyword
%token char2intKeyword
%token char2octKeyword
%token decompKeyword
%token float2intKeyword
%token hex2bitKeyword
%token hex2intKeyword
%token hex2octKeyword
%token hex2strKeyword
%token int2bitKeyword
%token int2charKeyword
%token int2enumKeyword
%token int2floatKeyword
%token int2hexKeyword
%token int2octKeyword
%token int2strKeyword
%token int2unicharKeyword
%token isvalueKeyword
%token isboundKeyword
%token ischosenKeyword
%token ispresentKeyword
%token istemplatekindKeyword
%token lengthofKeyword
%token oct2bitKeyword
%token oct2charKeyword
%token oct2hexKeyword
%token oct2intKeyword
%token oct2strKeyword
%token oct2unicharKeyword
%token regexpKeyword
%token replaceKeyword
%token rndKeyword
%token testcasenameKeyword
%token sizeofKeyword
%token str2floatKeyword
%token str2intKeyword
%token str2octKeyword
%token substrKeyword
%token unichar2intKeyword
%token unichar2charKeyword
%token unichar2octKeyword

%token float2strKeyword
%token str2bitKeyword
%token str2hexKeyword

%token log2strKeyword
%token enum2intKeyword

%token encvalueKeyword
%token decvalueKeyword

%token ttcn2stringKeyword
%token string2ttcnKeyword
%token remove_bomKeyWord
%token get_stringencodingKeyWord
%token encode_base64KeyWord
%token decode_base64KeyWord
%token encvalue_unicharKeyWord
%token decvalue_unicharKeyWord
%token any2unistrKeyWord
%token hostidKeyWord

/* Multi-character operators */

%token AssignmentChar ":="
%token DotDot ".."
%token PortRedirectSymbol "->"
%token EQ "=="
%token NE "!="
%token GE ">="
%token LE "<="
%token SL "<<"
%token SR ">>"
%token RL "<@"
%token _RR "@>" /* Name clash with bn.h:292 */

/*********************************************************************
 * Semantic types of nonterminals
 *********************************************************************/

%type <bool_val> optAliveKeyword optOptionalKeyword optOverrideKeyword
  optErrValueRaw optAllKeyword
%type <str> FreeText optLanguageSpec PatternChunk PatternChunkList
%type <uchar_val> Group Plane Row Cell
%type <id> FieldIdentifier FieldReference GlobalModuleId
  IdentifierOrAddressKeyword StructFieldRef PredefOrIdentifier
%type <string_val> CstringList
%type <ustring_val> Quadruple
%type <uid_list> USI UIDlike

%type <typetype> PredefinedType
%type <portoperationmode> PortOperationMode
%type <operationtype> PredefinedOpKeyword1 PredefinedOpKeyword2 PredefinedOpKeyword3

%type <activateop> ActivateOp
%type <attribtype> AttribKeyword

%type <altguard> CallBodyStatement AltGuard ElseStatement GuardStatement
  InterleavedGuardElement
%type <altguards> AltGuardList CallBodyStatementList InterleavedGuardList
  optPortCallBody
%type <arraydimension> ArrayIndex
%type <attributespec> AttribSpec
%type <compbody> optComponentDefList ComponentElementDefList
%type <compfield> StructFieldDef UnionFieldDef
%type <compfieldmap> StructFieldDefList optStructFieldDefList UnionFieldDefList
%type <definition> AltstepDef ExtFunctionDef FunctionDef TemplateDef TestcaseDef
%type <deftype> TypeDefBody StructuredTypeDef SubTypeDef RecordDef UnionDef
  SetDef RecordOfDef SetOfDef EnumDef PortDef PortDefBody ComponentDef
  TypeDef SignatureDef FunctionTypeDef AltstepTypeDef TestcaseTypeDef
%type <deftimer> SingleTimerInstance
%type <enumitem> Enumeration
%type <enumitems> EnumerationList
%type <fieldorarrayref> ArrayOrBitRef ArrayOrBitRefOrDash FieldOrArrayReference
%type <formalpar> FormalValuePar FormalTemplatePar FormalTimerPar
  TemplateFormalPar FunctionFormalPar TestcaseFormalPar
%type <formalparlist> optTemplateFormalParList TemplateFormalParList
  optFunctionFormalParList FunctionFormalParList optTestcaseFormalParList
  TestcaseFormalParList optAltstepFormalParList
%type <group> GroupDef GroupIdentifier
%type <friend_list> FriendModuleDef
%type <ifclause> ElseIfClause
%type <ifclauses> seqElseIfClause
%type <impmod> ImportFromSpec ModuleId ImportDef
%type <lenrestr> optStringLength LengthMatch StringLength
%type <logarg> LogItem
%type <logargs> LogItemList
%type <multiwithattrib> MultiWithAttrib WithAttribList WithStatement
  optWithStatement optWithStatementAndSemiColon
%type <namedtempl> FieldSpec
%type <namedtempls> seqFieldSpec
%type <namedvalue> FieldExpressionSpec
%type <namedvalues> seqFieldExpressionSpec
%type <indexedtempl> ArraySpec
%type <indexedtempls> seqArraySpec
%type <indexedvalue> ArrayExpressionSpec
%type <oidcomp> NumberForm NameAndNumberForm ObjIdComponent
%type <parass> VariableAssignment
%type <parasss> AssignmentList
%type <parredir> ParamAssignmentList ParamSpec
%type <patstr> CharStringMatch
%type <qualifier> DefOrFieldRef FullGroupIdentifier
%type <qualifiers> DefOrFieldRefList optAttribQualifier
%type <selectcase> SelectCase
%type <selectcases> seqSelectCase SelectCaseBody
%type <selectunion> SelectUnion SelectunionElse
%type <selectunions> seqSelectUnion SelectUnionBody
%type <signexc> ExceptionTypeList optExceptionSpec
%type <signparam> SignatureFormalPar
%type <signparamlist> SignatureFormalParList optSignatureFormalParList
%type <singlewithattrib> SingleWithAttrib
%type <stmt> AltConstruct BasicStatements BreakStatement BehaviourStatements
  CallBodyOps CallStatement CatchStatement CheckStatement ClearStatement
  CommunicationStatements ConditionalConstruct ConfigurationStatements
  ConnectStatement ContinueStatement ControlStatement DeactivateStatement
  DisconnectStatement DoWhileStatement DoneStatement ForStatement
  FunctionStatement GetCallStatement GetReplyStatement GotoStatement GuardOp
  HaltStatement InterleavedConstruct KillTCStatement KilledStatement
  LabelStatement LogStatement LoopConstruct MapStatement RaiseStatement
  ReceiveStatement RepeatStatement ReplyStatement ReturnStatement SUTStatements
  SendStatement SetLocalVerdict StartStatement StartTCStatement
  StartTimerStatement StopExecutionStatement StopStatement StopTCStatement
  StopTimerStatement TimeoutStatement TimerStatements TriggerStatement
  UnmapStatement VerdictStatements WhileStatement SelectCaseConstruct
  SelectUnionConstruct UpdateStatement SetstateStatement
  StopTestcaseStatement String2TtcnStatement ProfilerStatement int2enumStatement
%type <statementblock> StatementBlock optElseClause FunctionStatementOrDefList
  ControlStatementOrDefList ModuleControlBody
%type <subtypeparse> ValueOrRange
%type <templ> MatchingSymbol SingleValueOrAttrib SimpleSpec TemplateBody
  ArrayElementSpec ArrayValueOrAttrib FieldSpecList ArraySpecList
  AllElementsFrom TemplateListElem
%type <templinst> AddressRef FromClause FunctionActualPar InLineTemplate
  ReceiveParameter SendParameter TemplateActualPar TemplateInstance
  /* TestcaseActualPar */ ValueMatchSpec optFromClause optParDefaultValue
  optReceiveParameter
%type <parsedpar> FunctionActualParList TestcaseActualParList
  optFunctionActualParList optTestcaseActualParList
  NamedPart UnnamedPart
%type <templinsts>  optTemplateActualParList
  seqTemplateActualPar seqTemplateInstance
%type <templs> ValueOrAttribList seqValueOrAttrib ValueList Complement
  ArrayElementSpecList SubsetMatch SupersetMatch PermutationMatch
%type <ass> Assignment Step
%type <refbase> DerivedRefWithParList TemplateRefWithParList DecValueArg
%type <refpard> FunctionInstance AltstepInstance
%type <reference> PortType optDerivedDef DerivedDef IndexSpec Signature
  VariableRef TimerRef Port PortOrAll ValueStoreSpec
  SenderSpec ComponentType optRunsOnSpec RunsOnSpec optSystemSpec optPortSpec
%type <reference_or_any> PortOrAny TimerRefOrAny
%type <valuerange> Range
%type <type> NestedEnumDef NestedRecordDef NestedRecordOfDef NestedSetDef
  NestedSetOfDef NestedTypeDef NestedUnionDef PortDefAttribs ReferencedType
  Type TypeOrNestedTypeDef NestedFunctionTypeDef NestedAltstepTypeDef
  NestedTestcaseTypeDef
%type <types_with_mapping> TypeList AllOrTypeList AllOrTypeListWithFrom
AllOrTypeListWithTo TypeListWithFrom TypeListWithTo
%type <value> AddressValue AliveOp AllPortsSpec AltGuardChar ArrayBounds
  ArrayExpression ArrayExpressionList BitStringValue BooleanExpression
  BooleanValue CharStringValue ComponentRef ComponentReferenceOrLiteral
  ComponentOrDefaultReference CompoundExpression ConfigurationOps CreateOp
  DereferOp Expression FieldExpressionList Final GetLocalVerdict HexStringValue
  IntegerValue LowerBound MTCOp MatchOp NotUsedOrExpression ObjIdComponentList
  ObjectIdentifierValue OctetStringValue OmitValue OpCall PredefinedOps
  PredefinedValue ReadTimerOp ReferOp ReferencedValue RunningOp RunningTimerOp
  SelfOp SingleExpression SingleLowerBound SystemOp TemplateOps TimerOps
  TimerValue UpperBound Value ValueofOp VerdictOps VerdictValue optReplyValue
  optTestcaseTimerValue optToClause ProfilerRunningOp
%type <values> ArrayElementExpressionList seqArrayExpressionSpec
%type <variableentries> VariableList
%type <variableentry> VariableEntry
%type <subtypeparses> seqValueOrRange AllowedValues optSubTypeSpec
%type <eval> optLazyOrFuzzyModifier

%type <arraydimension_list> optArrayDef
%type <fieldorarrayref_list> optExtendedFieldReference
%type <def_list> AltstepLocalDef AltstepLocalDefList ComponentElementDef
  ConstDef ExtConstDef FunctionLocalDef FunctionLocalInst ModuleDef ModulePar
  ModuleParDef MultiTypedModuleParList PortInstance TimerInstance TimerList
  VarInstance PortElementVarDef
%type <stmt_list> FunctionStatementOrDef ControlStatementOrDef

%type <rangedef> RangeDef
%type <returntype> optReturnType
%type <returntypeornoblock> optReturnTypeOrNoBlockKeyword
%type <structdefbody> StructDefBody UnionDefBody
%type <structofdefbody> StructOfDefBody
%type <portdefbody> PortDefList seqPortDefList PortDefLists
%type <ischosenarg> IschosenArg
%type <extramatchingattrs> optExtraMatchingAttributes
%type <basetemplate> BaseTemplate
%type <templateref> TemplateRef TestcaseRef FunctionRef
%type <testcaseinst> TestcaseInstance
%type <portsendop> PortSendOp
%type <calltimerval> CallTimerValue
%type <portcallop> PortCallOp CallParameters
%type <portreplyop> PortReplyOp
%type <portraiseop> PortRaiseOp
%type <portredirect> optPortRedirect
%type <portredirectwithparam> optPortRedirectWithParam
%type <portredirectwithvalueandparam> optPortRedirectWithValueAndParam
%type <getreplypar> optGetReplyParameter
%type <portreceiveop> PortReceiveOp PortTriggerOp
%type <portgetcallop> PortGetCallOp
%type <portgetreplyop> PortGetReplyOp
%type <catchoppar> optCatchOpParameter CatchOpParameter
%type <portcatchop> PortCatchOp
%type <portcheckop> optCheckParameter CheckParameter CheckPortOpsPresent
  FromClausePresent RedirectPresent
%type <portref> PortRef AllConnectionsSpec
%type <connspec> SingleConnectionSpec SingleOrMultiConnectionSpec
%type <donepar> optDoneParameter
%type <reforid> Reference
%type <initial> Initial
%type <configspec> ConfigSpec
%type <createpar> optCreateParameter
%type <applyop> ApplyOp
%type <identifier_list> IdentifierList IdentifierListOrPredefType
%type <singlevarinst> SingleConstDef SingleVarInstance
%type <singlevarinst_list> ConstList VarList
%type <singletempvarinst> SingleTempVarInstance
%type <singletempvarinst_list> TempVarList
%type <singlemodulepar> SingleModulePar
%type <singletemplatemodulepar> SingleTemplateModulePar
%type <singlemodulepar_list> ModuleParList
%type <singletemplatemodulepar_list> TemplateModuleParList
%type <portelement> PortElement
%type <portelement_list> PortElementList
%type <comprefs> optExtendsDef ComponentTypeList
%type <runs_on_compref_or_self> optRunsOnComprefOrSelf
%type <template_restriction> TemplateRestriction optTemplateRestriction
  TemplateOptRestricted
%type <visbilitytype> optVisibility ComponentElementVisibility
%type <float_val> FloatOrSpecialFloatValue
%type <erroneous_indicator> ErroneousIndicator
%type <imptype> ImportSpec ImportElement
%type <decode_match> DecodedContentMatch
%type <decoded_modifier> optDecodedModifier
%type <value_redirect> ValueSpec
%type <single_value_redirect> SingleValueSpec
%type <single_value_redirect_list> SingleValueSpecList
%type <typemappingtargets> WithList
%type <reference_list> PortTypeList

/*********************************************************************
 * Destructors
 *********************************************************************/

%destructor {
  act_group = $$->get_parent_group();
}
GroupDef
GroupIdentifier

%destructor {Free($$);}
Cstring
FreeText
optLanguageSpec
PatternChunk
PatternChunkList

%destructor {
  delete $$.ref_pard;
  delete $$.derefered_value;
  delete $$.ap_list;
}
ActivateOp

%destructor {
  delete $$.types;
  delete $$.mappings;
}
AllOrTypeList
AllOrTypeListWithFrom
AllOrTypeListWithTo
TypeList
TypeListWithFrom
TypeListWithTo

%destructor {delete $$;}
AddressRef
AddressValue
AliveOp
AllPortsSpec
AltConstruct
AltGuard
AltGuardChar
AltGuardList
AltstepDef
AltstepInstance
AltstepTypeDef
ArrayBounds
ArrayElementExpressionList
ArrayElementSpec
ArrayElementSpecList
ArrayExpression
ArrayIndex
ArrayOrBitRef
ArrayOrBitRefOrDash
ArrayValueOrAttrib
Assignment
AssignmentList
AttribSpec
BasicStatements
BehaviourStatements
BitStringMatch
BitStringValue
BooleanExpression
BooleanValue
BreakStatement
Bstring
CallBodyOps
CallBodyStatement
CallBodyStatementList
CallStatement
CatchStatement
CharStringMatch
CharStringValue
CheckStatement
ClearStatement
CommunicationStatements
Complement
ComponentDef
ComponentElementDefList
ComponentOrDefaultReference
ComponentRef
ComponentReferenceOrLiteral
ComponentType
ComponentTypeList
CompoundExpression
ConditionalConstruct
ConfigurationOps
ConfigurationStatements
ConnectStatement
ContinueStatement
ControlStatement
ControlStatementOrDefList
CreateOp
DeactivateStatement
DefOrFieldRef
DefOrFieldRefList
DereferOp
DerivedDef
DerivedRefWithParList
DisconnectStatement
DoWhileStatement
DoneStatement
ElseIfClause
ElseStatement
EnumDef
Enumeration
EnumerationList
ExceptionTypeList
Expression
FieldExpressionList
FieldExpressionSpec
FieldIdentifier
FieldOrArrayReference
FieldReference
FieldSpec
FieldSpecList
Final
ForStatement
FormalTemplatePar
FormalTimerPar
FormalValuePar
FromClause
FullGroupIdentifier
FunctionActualPar
FunctionActualParList
FunctionDef
FunctionFormalPar
FunctionFormalParList
FunctionInstance
FunctionStatement
FunctionStatementOrDefList
FunctionTypeDef
GetCallStatement
GetLocalVerdict
GetReplyStatement
GlobalModuleId
GotoStatement
GuardOp
GuardStatement
HaltStatement
HexStringMatch
HexStringValue
Hstring
IDentifier
IdentifierOrAddressKeyword
ImportFromSpec
IndexSpec
InLineTemplate
int2enumStatement
IntegerValue
InterleavedConstruct
InterleavedGuardElement
InterleavedGuardList
KillTCStatement
KilledStatement
LabelStatement
LengthMatch
LogItem
LogItemList
LogStatement
String2TtcnStatement
LoopConstruct
LowerBound
MTCOp
MapStatement
MatchOp
MatchingSymbol
ModuleControlBody
ModuleId
MultiWithAttrib
NamedPart
NameAndNumberForm
NestedAltstepTypeDef
NestedEnumDef
NestedFunctionTypeDef
NestedRecordDef
NestedRecordOfDef
NestedSetDef
NestedSetOfDef
NestedTestcaseTypeDef
NestedTypeDef
NestedUnionDef
NotUsedOrExpression
Number
NumberForm
ObjIdComponent
ObjIdComponentList
ObjectIdentifierValue
OctetStringMatch
OctetStringValue
OmitValue
OpCall
Ostring
ParamAssignmentList
ParamSpec
PermutationMatch
Port
PortDef
PortDefAttribs
PortDefBody
PortOrAll
PortType
PredefOrIdentifier
PredefinedOps
PredefinedValue
ProfilerRunningOp
ProfilerStatement
RaiseStatement
Range
ReadTimerOp
ReceiveParameter
ReceiveStatement
RecordDef
RecordOfDef
ReferOp
ReferencedType
ReferencedValue
RepeatStatement
ReplyStatement
ReturnStatement
RunningOp
RunningTimerOp
RunsOnSpec
SUTStatements
SelfOp
SendParameter
SendStatement
SenderSpec
SetDef
SetLocalVerdict
SetOfDef
SetstateStatement
Signature
SignatureDef
SignatureFormalPar
SignatureFormalParList
SimpleSpec
SingleExpression
SingleLowerBound
SingleTimerInstance
SingleValueOrAttrib
SingleValueSpec
SingleWithAttrib
DecValueArg
StartStatement
StartTCStatement
StartTimerStatement
StatementBlock
Step
StopExecutionStatement
StopTestcaseStatement
StopStatement
StopTCStatement
StopTimerStatement
StringLength
StructFieldDef
StructFieldDefList
StructFieldRef
StructuredTypeDef
SubTypeDef
SubsetMatch
SupersetMatch
SystemOp
TemplateActualPar
TemplateBody
TemplateDef
TemplateFormalPar
TemplateFormalParList
TemplateInstance
TemplateOps
TemplateRefWithParList
/* TestcaseActualPar */
TestcaseActualParList
TestcaseDef
TestcaseFormalPar
TestcaseFormalParList
TestcaseTypeDef
TimeoutStatement
TimerOps
TimerRef
TimerStatements
TimerValue
TriggerStatement
Type
TypeDef
TypeDefBody
TypeOrNestedTypeDef
UnionDef
UnionFieldDef
UnionFieldDefList
UnmapStatement
UnnamedPart
UpdateStatement
UpperBound
Value
ValueList
ValueMatchSpec
ValueOrAttribList
ValueOrRange
ValueSpec
ValueStoreSpec
ValueofOp
VariableAssignment
VariableEntry
VariableList
VariableRef
VerdictOps
VerdictStatements
VerdictValue
WhileStatement
WithAttribList
WithList
WithStatement
optAltstepFormalParList
optAttribQualifier
optComponentDefList
optDerivedDef
optElseClause
optExceptionSpec
optExtendsDef
optFromClause
optFunctionActualParList
optFunctionFormalParList
optParDefaultValue
optPortCallBody
optReceiveParameter
optReplyValue
optRunsOnSpec
optSignatureFormalParList
optStringLength
optStructFieldDefList
optSystemSpec
optTemplateActualParList
optTemplateFormalParList
optTestcaseActualParList
optTestcaseFormalParList
optTestcaseTimerValue
optToClause
optWithStatement
optWithStatementAndSemiColon
seqElseIfClause
seqFieldExpressionSpec
seqFieldSpec
seqTemplateActualPar
seqTemplateInstance
seqValueOrAttrib
Quadruple

%destructor {
  for (size_t i = 0; i < $$->size(); i++) delete (*$$)[i];
  $$->clear();
  delete $$;
}
AllowedValues
optSubTypeSpec
seqValueOrRange

%destructor {
  for(size_t i=0; i<$$.nElements; i++) delete $$.elements[i];
  Free($$.elements);
}
AltstepLocalDef
AltstepLocalDefList
ComponentElementDef
ConstDef
ControlStatementOrDef
ExtConstDef
FunctionLocalDef
FunctionLocalInst
FunctionStatementOrDef
ModuleDef
ModulePar
ModuleParDef
MultiTypedModuleParList
PortInstance
SingleValueSpecList
TimerInstance
TimerList
VarInstance
optArrayDef
optExtendedFieldReference
FriendModuleDef
USI
UIDlike
PortTypeList
PortElementVarDef


%destructor {
  delete $$.lower;
  delete $$.upper;
}
RangeDef

%destructor {
  delete $$.type;
}
optReturnType
optReturnTypeOrNoBlockKeyword

%destructor {
  delete $$.id;
  delete $$.cfm;
}
StructDefBody
UnionDefBody

%destructor {
  delete $$.type;
  delete $$.id;
}
StructOfDefBody

%destructor {
  delete $$.in_list;
  delete $$.out_list;
  delete $$.inout_list;
  delete $$.in_mappings;
  delete $$.out_mappings;
  for (size_t i = 0; i < $$.varnElements; i++) {
    delete $$.varElements[i];
  }
  delete $$.varElements;
}
PortDefList
PortDefLists
seqPortDefList

%destructor {
  delete $$.ref;
  delete $$.id;
}
IschosenArg

%destructor {
  delete $$.len_restr;
}
optExtraMatchingAttributes

%destructor {
  delete $$.name;
  delete $$.type;
  delete $$.fp_list;
}
BaseTemplate

%destructor {
  delete $$.modid;
  delete $$.id;
}
FunctionRef
TemplateRef
TestcaseRef

%destructor {
  delete $$.ref_pard;
  delete $$.derefered_value;
  delete $$.ap_list;
  delete $$.value;
}
TestcaseInstance

%destructor {
  delete $$.templ_inst;
  delete $$.val;
}
PortSendOp

%destructor {
  delete $$.calltimerval;
}
CallTimerValue

%destructor {
  delete $$.templ_inst;
  delete $$.calltimerval;
  delete $$.val;
}
PortCallOp CallParameters

%destructor {
  delete $$.templ_inst;
  delete $$.replyval;
  delete $$.toclause;
}
PortReplyOp

%destructor {
  delete $$.signature;
  delete $$.templ_inst;
  delete $$.toclause;
}
PortRaiseOp

%destructor {
  delete $$.redirectval;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
optPortRedirect

%destructor {
  delete $$.redirectparam;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
optPortRedirectWithParam

%destructor {
  delete $$.redirectval;
  delete $$.redirectparam;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
optPortRedirectWithValueAndParam

%destructor {
  delete $$.templ_inst;
  delete $$.valuematch;
}
optGetReplyParameter

%destructor {
  delete $$.templ_inst;
  delete $$.fromclause;
  delete $$.redirectval;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
PortReceiveOp
PortTriggerOp

%destructor {
  delete $$.templ_inst;
  delete $$.fromclause;
  delete $$.redirectparam;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
PortGetCallOp

%destructor {
  delete $$.templ_inst;
  delete $$.valuematch;
  delete $$.fromclause;
  delete $$.redirectval;
  delete $$.redirectparam;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
PortGetReplyOp

%destructor {
  delete $$.signature;
  delete $$.templ_inst;
}
optCatchOpParameter
CatchOpParameter

%destructor {
  delete $$.signature;
  delete $$.templ_inst;
  delete $$.fromclause;
  delete $$.redirectval;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
PortCatchOp

%destructor {
  delete $$.signature;
  delete $$.templ_inst;
  delete $$.valuematch;
  delete $$.fromclause;
  delete $$.redirectval;
  delete $$.redirectparam;
  delete $$.redirectsender;
  delete $$.redirectindex;
}
optCheckParameter
CheckParameter
CheckPortOpsPresent
FromClausePresent
RedirectPresent

%destructor {
  delete $$.compref;
  delete $$.portref;
}
PortRef
AllConnectionsSpec

%destructor {
  delete $$.compref1;
  delete $$.portref1;
  delete $$.compref2;
  delete $$.portref2;
}
SingleConnectionSpec
SingleOrMultiConnectionSpec

%destructor {
  delete $$.donematch;
  delete $$.valueredirect;
  delete $$.indexredirect;
}
optDoneParameter

%destructor {
  if ($$.is_ref) delete $$.ref;
  else delete $$.id;
}
Reference

%destructor {
  delete $$.defs;
  delete $$.ass;
}
Initial

%destructor {
  delete $$.runsonref;
  delete $$.systemref;
}
ConfigSpec

%destructor {
  delete $$.name;
  delete $$.loc;
}
optCreateParameter

%destructor {
  delete $$.value;
  delete $$.ap_list;
}
ApplyOp

%destructor {
  for (size_t i = 0; i < $$.nElements; i++) delete $$.elements[i].id;
  Free($$.elements);
}
IdentifierList
IdentifierListOrPredefType

%destructor {
  delete $$.id;
  for (size_t i = 0; i < $$.arrays.nElements; i++)
    delete $$.arrays.elements[i];
  Free($$.arrays.elements);
  delete $$.initial_value;
}
SingleConstDef
SingleVarInstance
SingleTempVarInstance

%destructor {
  for (size_t i = 0; i < $$.nElements; i++) {
    delete $$.elements[i].id;
    for (size_t j = 0; i < $$.elements[i].arrays.nElements; j++)
      delete $$.elements[i].arrays.elements[j];
    Free($$.elements[i].arrays.elements);
    delete $$.elements[i].initial_value;
  }
  Free($$.elements);
}
ConstList
TempVarList
VarList

%destructor {
  delete $$.id;
  delete $$.defval;
}
SingleModulePar

%destructor {
  delete $$.id;
  delete $$.deftempl;
}
SingleTemplateModulePar

%destructor {
  for (size_t i = 0; i < $$.nElements; i++) {
    delete $$.elements[i].id;
    delete $$.elements[i].defval;
  }
  Free($$.elements);
}
ModuleParList

%destructor {
  for (size_t i = 0; i < $$.nElements; i++) {
    delete $$.elements[i].id;
    delete $$.elements[i].deftempl;
  }
  Free($$.elements);
}
TemplateModuleParList

%destructor {
  delete $$.id;
  delete $$.dims;
}
PortElement

%destructor {
  for (size_t i = 0; i < $$.nElements; i++) {
    delete $$.elements[i].id;
    delete $$.elements[i].dims;
  }
  Free($$.elements);
}
PortElementList

%destructor {
  delete $$.reference;
}
optRunsOnComprefOrSelf
PortOrAny
TimerRefOrAny

%destructor {
  if ($$.string_encoding != NULL) {
    delete $$.string_encoding;
  }
  delete $$.target_template;
}
DecodedContentMatch

%destructor {
  if ($$.string_encoding != NULL) {
    delete $$.string_encoding;
  }
}
optDecodedModifier

/*********************************************************************
 * Operator precedences (lowest first)
 *********************************************************************/

%left OrKeyword
%left XorKeyword
%left AndKeyword
%left NotKeyword
%left EQ NE
%nonassoc '<' '>' GE LE
%left SL SR RL _RR
%left Or4bKeyword
%left Xor4bKeyword
%left And4bKeyword
%left Not4bKeyword
%left '+' '-' '&'
%left '*' '/' ModKeyword RemKeyword
%left UnarySign

%expect 66

%start GrammarRoot

/*
XXX Source of conflicts (66 S/R):

1.) 9 conflicts in one state
The Expression after 'return' keyword is optional in ReturnStatement.
For 9 tokens the parser cannot decide whether the token is a part of
the return expression (shift) or it is the beginning of the next statement
(reduce).

2.) 10 distinct states, each with one conflict caused by token '['
The local definitions in altsteps can be followed immediately by the guard
expression. When the parser sees the '[' token it cannot decide whether it
belongs to the local definition as array dimension or array subreference
(shift) or it is the beginning of the first guard expression (reduce).
The situations are the following:
- var t v := ref <here> [
- var t v := ref[subref] <here> [
- var t v := ref.integer <here> [
- var t v := ref.subref <here> [
- timer t <here> [
- var t v <here> [
- var t v := ref.objid{...}.subref <here> [
- var template t v <here> [
- var t v := function(...)<subrefs> <here> [
- var template t v := decmatch (...) ref <here> [

3.) 1 conflict
The sequence identifier.objid can be either the beginning of a module name
qualified with a module object identifier (shift) or a reference to an objid
value within an entity of type anytype (reduce).

4.) 1 conflict
The '{' token after a call statement can be either the part of the response and
exception handling part (shift) or the beginning of the next statement, which
is a StatementBlock (reduce). Note that StatementBlock as a statement is a
non-standard language extension.

5.) 5 conflicts in in three states, related to named parameters

6.) 1 Conflict due to pattern concatenation

7.) 27 conflicts in one state
In the DecodedContentMatch rule a SingleExpression encased in round brackets is
followed by an in-line template. For 26 tokens (after the ')' ) the parser cannot
decide whether the token is the beginning of the in-line template (shift) or
the brackets are only part of the SingleExpression itself and the conflicting
token is the next segment in the expression (reduce).

8.) 1 conflict
In the current version when the compiler finds '(' SingleExpression . ')'
it can not decide if it should resolve to a SingleValueOrAttrib or to a SingleExpression.
Shift is fine as single element list can be resolved via SingleValueOrAttrib too.

9.) 4 conflicts in 4 states
In the rules for 'running' and 'alive' operations with the 'any from' clause,
the redirect operator ("->") after the 'running' or 'alive' keyword can be the
start of the operation's index redirect (shift) or another expression that starts
with "->" (reduce).
TODO: Find out what the index redirect conflicts with. It's probably something
that would cause a semantic error anyway, but it would be good to know.

10.) 2 conflicts in the rule TypeListWithTo.

11.) 4 conflicts in 4 states
In the Expression and SingleExpression rules when an AnyValue or AnyOrOmit is
followed by a LengthMatch, the parser cannot decide whether the LengthMatch token
belongs to the AnyValue/AnyOrOmit (shift) or the resulting template (reduce).
This is only relevant for template concatenation:
ex.: template octetstring t := 'AB'O & ? length (3);
Because the parser shifts, the length restriction here is applied to the '?'
instead of the concatenation result, which is the expected behavior.

12.) 1 conflict in PortDefLists
PortElementVarDef contains optSemicolon which causes 1 conflict

Note that the parser implemented by bison always chooses to shift instead of
reduce in case of conflicts.
*/

%%

/*********************************************************************
 * Grammar
 *********************************************************************/

/* The grammar of TTCN-3 */
/* The numbers correspond to ES 201 873-1 V4.1.2 (2009-07) */

GrammarRoot:
  TTCN3Module
  {
    if (is_erroneous_parsed) {
      delete act_ttcn3_module;
      act_ttcn3_module = NULL;
      Location loc(infile, @1);
      loc.error("The erroneous attribute cannot be a TTCN-3 module.");
    }
  }
| TitanErroneousHackKeyword ErroneousAttributeSpec
  {
    if (!is_erroneous_parsed) {
      delete act_ttcn3_erroneous_attr_spec;
      act_ttcn3_erroneous_attr_spec = NULL;
      Location loc(infile, @$);
      loc.error("File `%s' does not contain a TTCN-3 module.", infile);
    }
  }
| error
;

ErroneousAttributeSpec:
  ErroneousIndicator AssignmentChar TemplateInstance optAllKeyword
  {
    act_ttcn3_erroneous_attr_spec = new ErroneousAttributeSpec($1.is_raw, $1.indicator, $3, $4);
  }
;

ErroneousIndicator:
  ValueKeyword optErrValueRaw
  {
    $$.indicator = ErroneousAttributeSpec::I_VALUE;
    $$.is_raw = $2;
  }
| IDentifier optErrValueRaw
  {
    if ($1->get_ttcnname()=="before") $$.indicator = ErroneousAttributeSpec::I_BEFORE;
    else if ($1->get_ttcnname()=="after") $$.indicator = ErroneousAttributeSpec::I_AFTER;
    else {
      Location loc(infile, @1);
      loc.error("Invalid indicator. Valid indicators are: "
                "`before', `value' and `after'");
      $$.indicator = ErroneousAttributeSpec::I_INVALID;
    }
    delete $1;
    $$.is_raw = $2;
  }
;

optAllKeyword:
  /* empty */ { $$ = false; }
| AllKeyword { $$ = true; }
;

optErrValueRaw:
  /* empty */ { $$ = false; }
| '(' IDentifier ')'
  {
    if ($2->get_ttcnname()=="raw") $$ = true;
    else {
      Location loc(infile, @2);
      loc.error("Invalid keyword, only the optional `raw' keyword can be used here.");
      $$ = false;
    }
    delete $2;
  }
;


/* A.1.6.0 TTCN Module */

TTCN3Module: // 1
  TTCN3ModuleId ModuleBody optWithStatementAndSemiColon optError
  {
    act_ttcn3_module->set_with_attr($3);
    if (anytype_access) {
      // If there was an attempted access to an anytype field, artificially
      // create a type definition as if the following appeared in TTCN-3:
      // type union anytype { /* empty, members added later */ }
      // NOTE: anything which looks like usage of an anytype field will bring
      // the local anytype to life, which trumps any imported anytype
      // (when resolving, the local anytype will be found first).
      // TODO: just to be safe, anytype should be removed from the exports.
      Type *t = new Type(Type::T_ANYTYPE);
      Identifier *anytype_id = new Identifier(Identifier::ID_TTCN, anyname);
      Def_Type *anytypedef = new Def_Type(anytype_id, t);
      anytypedef->set_parent_path(act_ttcn3_module->get_attrib_path());
      act_ttcn3_module->add_ass(anytypedef);
      // Def_Type is-a Definition is-a Assignment
    }
  }
;

TTCN3ModuleId: // 3
  optError TTCN3ModuleKeyword IDentifier optDefinitiveIdentifier
  optLanguageSpec optError
  {
    act_ttcn3_module = new Ttcn::Module($3);
    act_ttcn3_module->set_scope_name($3->get_dispname());
    act_ttcn3_module->set_language_spec($5);
    Free($5);
  }
;

ModuleId: // 4
  GlobalModuleId optLanguageSpec
  {
    $$ = new ImpMod($1);
    $$->set_language_spec($2);
    Free($2);
  }
;

GlobalModuleId: // 5
  IDentifier { $$ = $1; }
| IDentifier '.' ObjectIdentifierValue
  { $$ = $1; delete $3; }
;

optLanguageSpec:
  /* empty */ { $$ = NULL; }
| LanguageKeyword FreeText optPackageNameList { $$ = $2; } // sort-of 7 LanguageSpec
;

optPackageNameList:
  /* empty */
| PackageNameList
;

PackageNameList:
  ',' FreeText
|  PackageNameList ',' FreeText
;

ModuleBody:
  '{' optErrorBlock '}'
| '{' ModuleDefinitionsList optErrorBlock '}'
| '{' ModuleDefinitionsList ModuleControlPart optErrorBlock '}'
| '{' ModuleControlPart optErrorBlock '}'
;

/* A.1.6.1 Module definitions part */

/* A.1.6.1.0 General */

ModuleDefinitionsList: // 10
  ModuleDefinition optSemiColon
| error ModuleDefinition optSemiColon
| ModuleDefinitionsList optErrorBlock ModuleDefinition optSemiColon
;

optVisibility: // 12
//  /* empty */ { $$.visibility = PUBLIC;}
/* empty */     { $$.visibility = NOCHANGE;}
| PublicKeyword { $$.visibility = PUBLIC;}
| PrivateKeyword { $$.visibility = PRIVATE;}
| FriendKeyword { $$.visibility = FRIEND;}
;

/* A definition _in_ the module, not a definition _of_ a module */
ModuleDefinition: // 11
  optVisibility ModuleDef optWithStatement
  {
    for (size_t i = 0; i < $2.nElements; i++) {
      if ($3) {
        if (i == 0) $2.elements[i]->set_with_attr($3);
        else $2.elements[i]->set_with_attr($3->clone());
      }
      if ($1.visibility != NOCHANGE) {
        $2.elements[i]->set_visibility($1.visibility);
      }
      act_ttcn3_module->add_ass($2.elements[i]);
      if (act_group) {
        $2.elements[i]->set_parent_path(act_group->get_attrib_path());
        act_group->add_ass($2.elements[i]);
      } else {
        $2.elements[i]->set_parent_path(act_ttcn3_module->get_attrib_path());
      }
    }
    Free($2.elements);
  }
| optVisibility ImportDef optWithStatement
  {
    $2->set_with_attr($3);
    if ($1.visibility != NOCHANGE) {
      $2->set_visibility($1.visibility);
    }
    if (act_group) {
      $2->set_parent_path(act_group->get_attrib_path());
      act_group->add_impmod($2);
    } else {
      $2->set_parent_path(act_ttcn3_module->get_attrib_path());
    }
    act_ttcn3_module->add_impmod($2);
  }
| PublicKeyword GroupDef optWithStatement
  { // only public allowed for group, and it's redundant
    $2->set_with_attr($3);
    act_group = $2->get_parent_group();
  }
| GroupDef optWithStatement
  { // same code as above
    $1->set_with_attr($2);
    act_group = $1->get_parent_group();
  }
| PrivateKeyword FriendModuleDef optWithStatement
  { // only private allowed for "friend module", and it's redundant
    for (size_t i = 0; i < $2.nElements; i++) {
      if ($3) {
        if (i == 0) $2.elements[i]->set_with_attr($3);
        else $2.elements[i]->set_with_attr($3->clone());
      }
      act_ttcn3_module->add_friendmod($2.elements[i]);
      if (act_group) {
        $2.elements[i]->set_parent_path(act_group->get_attrib_path());
        act_group->add_friendmod($2.elements[i]);
      } else {
        $2.elements[i]->set_parent_path(act_ttcn3_module->get_attrib_path());
      }
    }
    Free($2.elements);
  }
| FriendModuleDef optWithStatement
  { // same code as above
    for (size_t i = 0; i < $1.nElements; i++) {
      if ($2) {
        if (i == 0) $1.elements[i]->set_with_attr($2);
        else $1.elements[i]->set_with_attr($2->clone());
      }
      act_ttcn3_module->add_friendmod($1.elements[i]);
      if (act_group) {
        $1.elements[i]->set_parent_path(act_group->get_attrib_path());
        act_group->add_friendmod($1.elements[i]);
      } else {
        $1.elements[i]->set_parent_path(act_ttcn3_module->get_attrib_path());
      }
    }
    Free($1.elements);
  }
;

ModuleDef:
  ConstDef { $$ = $1; }
| ModuleParDef { $$ = $1; }
| TypeDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| TemplateDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| FunctionDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| SignatureDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| TestcaseDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| AltstepDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| ExtFunctionDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| ExtConstDef { $$ = $1; }
;

/* A.1.6.1.1 Typedef definitions */

TypeDef: // 13
  TypeDefKeyword TypeDefBody
  {
    $$ = $2;
    $$->set_location(infile, @$);
  }
;

TypeDefBody: // 14
  StructuredTypeDef { $$ = $1; }
| SubTypeDef { $$ = $1; }
;

StructuredTypeDef: // 16
  RecordDef { $$ = $1; }
| UnionDef { $$ = $1; }
| SetDef { $$ = $1; }
| RecordOfDef { $$ = $1; }
| SetOfDef { $$ = $1; }
| EnumDef { $$ = $1; }
| PortDef { $$ = $1; }
| ComponentDef { $$ = $1; }
| FunctionTypeDef { $$ = $1; }
| AltstepTypeDef { $$ = $1; }
| TestcaseTypeDef { $$ = $1; }
;

RecordDef: // 17
  RecordKeyword StructDefBody
  {
    Type *type = new Type(Type::T_SEQ_T, $2.cfm);
    type->set_location(infile, @$);
    $$ = new Def_Type($2.id, type);
  }
;

StructDefBody: // 19
  IDentifier optStructDefFormalParList
  '{' optStructFieldDefList '}'
  {
    $$.id = $1;
    $$.cfm = $4;
  }
| AddressKeyword '{' optStructFieldDefList '}'
  {
    $$.id = new Identifier(Identifier::ID_TTCN, string("address"));
    $$.cfm = $3;
  }
;

optStructDefFormalParList:
  /* empty */ optError
| '(' StructDefFormalParList optError ')'
  {
    Location loc(infile, @$);
    loc.error("Type parameterization is not currently supported");
  }
| '(' error ')'
  {
    Location loc(infile, @$);
    loc.error("Type parameterization is not currently supported");
  }
;

StructDefFormalParList: // -> 202 784 "Advanced Parameterization", 9
  optError StructDefFormalPar
| StructDefFormalParList optError ',' optError StructDefFormalPar
| StructDefFormalParList optError ',' error
;

StructDefFormalPar: // -> 202 784 "Advanced Parameterization", 10
  FormalValuePar { delete $1; }
;

optStructFieldDefList:
  /* empty */ optError { $$ = new CompFieldMap; }
| StructFieldDefList optError { $$ = $1; }
;

StructFieldDefList:
  optError StructFieldDef
  {
    $$ = new CompFieldMap;
    $$->add_comp($2);
  }
| StructFieldDefList optError ',' optError StructFieldDef
  {
    $$ = $1;
    $$->add_comp($5);
  }
| StructFieldDefList optError ',' error { $$ = $1; }
;

StructFieldDef: // 21
  TypeOrNestedTypeDef IDentifier optArrayDef optSubTypeSpec optOptionalKeyword
  {
    if ($4) {
      /* The subtype constraint belongs to the innermost embedded type of
       * possible nested 'record of' or 'set of' constructs. */
      Type *t = $1;
      while (t->is_seof()) t = t->get_ofType();
      t->set_parsed_restrictions($4);
    }
    Type *type = $1;
    /* creation of array type(s) if necessary (from right to left) */
    for (size_t i = $3.nElements; i > 0; i--) {
      type = new Type(Type::T_ARRAY, type, $3.elements[i - 1], true);
      type->set_location(*$1);
    }
    Free($3.elements);
    $$ = new CompField($2, type, $5);
    $$->set_location(infile, @$);
  }
;

optOptionalKeyword:
  /* empty */ { $$ = false; }
| OptionalKeyword { $$ = true; }
;

TypeOrNestedTypeDef:
  Type { $$ = $1; }
| NestedTypeDef { $$ = $1; }
;

NestedTypeDef: // 22
  NestedRecordDef { $$ = $1; }
| NestedUnionDef { $$ = $1; }
| NestedSetDef { $$ = $1; }
| NestedRecordOfDef { $$ = $1; }
| NestedSetOfDef { $$ = $1; }
| NestedEnumDef { $$ = $1; }
| NestedFunctionTypeDef { $$ = $1; }
| NestedAltstepTypeDef { $$ = $1; }
| NestedTestcaseTypeDef { $$ = $1; }
;

NestedRecordDef: // 23
  RecordKeyword '{' optStructFieldDefList '}'
  {
    $$ = new Type(Type::T_SEQ_T, $3);
    $$->set_location(infile, @$);
  }
;

NestedUnionDef: // 24
  UnionKeyword '{' UnionFieldDefList optError '}'
  {
    $$ = new Type(Type::T_CHOICE_T, $3);
    $$->set_location(infile, @$);
  }
;

NestedSetDef:  // 25
  SetKeyword '{' optStructFieldDefList '}'
  {
    $$ = new Type(Type::T_SET_T, $3);
    $$->set_location(infile, @$);
  }
;

NestedRecordOfDef: // 26
  RecordKeyword optStringLength OfKeyword TypeOrNestedTypeDef
  {
    $$ = new Type(Type::T_SEQOF, $4);
    if ($2) {
      vector<SubTypeParse> *vstp = new vector<SubTypeParse>;
      SubTypeParse *stp = new SubTypeParse($2);
      vstp->add(stp);
      $$->set_parsed_restrictions(vstp);
    }
    $$->set_location(infile, @$);
  }
;

NestedSetOfDef: // 27
  SetKeyword optStringLength OfKeyword TypeOrNestedTypeDef
  {
    $$ = new Type(Type::T_SETOF, $4);
    if ($2) {
      vector<SubTypeParse> *vstp = new vector<SubTypeParse>;
      SubTypeParse *stp = new SubTypeParse($2);
      vstp->add(stp);
      $$->set_parsed_restrictions(vstp);
    }
    $$->set_location(infile, @$);
  }
;

NestedEnumDef: // 28
  EnumKeyword '{' EnumerationList optError '}'
  {
    $$ = new Type(Type::T_ENUM_T, $3);
    $$->set_location(infile, @$);
  }
;

NestedFunctionTypeDef:
  FunctionKeyword '(' optFunctionFormalParList ')'
  optRunsOnComprefOrSelf optReturnType
  {
    $3->set_location(infile, @2, @4);
    $$ = new Type(Type::T_FUNCTION, $3, $5.reference, $5.self, $6.type,
                  $6.returns_template, $6.template_restriction);
    $$->set_location(infile, @$);
  }
;

NestedAltstepTypeDef:
  AltstepKeyword '(' optAltstepFormalParList ')'
  optRunsOnComprefOrSelf
  {
    $3->set_location(infile, @2, @4);
    $$ = new Type(Type::T_ALTSTEP, $3, $5.reference, $5.self);
    $$->set_location(infile, @$);
  }
;

NestedTestcaseTypeDef:
  TestcaseKeyword '(' optTestcaseFormalParList ')'
  ConfigSpec
  {
    $3->set_location(infile, @2, @4);
    $$ = new Type(Type::T_TESTCASE, $3, $5.runsonref,
                          $5.systemref);
    $$->set_location(infile, @$);
  }
;

UnionDef: // 31
  UnionKeyword UnionDefBody
  {
    Type *type = new Type(Type::T_CHOICE_T, $2.cfm);
    type->set_location(infile, @$);
    $$ = new Def_Type($2.id, type);
  }
;

UnionDefBody: // 33
  IDentifier optStructDefFormalParList
  '{' UnionFieldDefList optError '}'
  {
    $$.id = $1;
    $$.cfm = $4;
  }
| AddressKeyword '{' UnionFieldDefList optError '}'
  {
    $$.id = new Identifier(Identifier::ID_TTCN, string("address"));
    $$.cfm = $3;
  }
;

UnionFieldDefList:
  optError UnionFieldDef
  {
    $$ = new CompFieldMap;
    $$->add_comp($2);
  }
| UnionFieldDefList optError ',' optError UnionFieldDef
  {
    $$ = $1;
    $$->add_comp($5);
  }
| UnionFieldDefList optError ',' error { $$ = $1; }
| error { $$ = new CompFieldMap; }
;

UnionFieldDef: // 34
  TypeOrNestedTypeDef IDentifier optArrayDef optSubTypeSpec
  {
    if ($4) {
      /* The subtype constraint belongs to the innermost embedded type of
       * possible nested 'record of' or 'set of' constructs. */
      Type *t = $1;
      while (t->is_seof()) t = t->get_ofType();
      t->set_parsed_restrictions($4);
    }
    Type *type = $1;
    /* creation of array type(s) if necessary (from right to left) */
    for (size_t i = $3.nElements; i > 0; i--) {
      type = new Type(Type::T_ARRAY, type, $3.elements[i - 1], true);
      type->set_location(*$1);
    }
    Free($3.elements);
    $$ = new CompField($2, type, false);
    $$->set_location(infile, @$);
  }
;

SetDef: // 35
  SetKeyword StructDefBody
  {
    Type *type = new Type(Type::T_SET_T, $2.cfm);
    type->set_location(infile, @$);
    $$ = new Def_Type($2.id, type);
  }
;

RecordOfDef: // 37
  RecordKeyword optStringLength OfKeyword StructOfDefBody
  {
    Type *type = new Type(Type::T_SEQOF, $4.type);
    if ($2) {
      vector<SubTypeParse> *vstp = new vector<SubTypeParse>;
      vstp->add(new SubTypeParse($2));
      type->set_parsed_restrictions(vstp);
    }
    type->set_location(infile, @$);
    $$ = new Def_Type($4.id, type);
  }
;

StructOfDefBody: // 39
  TypeOrNestedTypeDef IdentifierOrAddressKeyword optSubTypeSpec
  {
    if ($3) {
      /* The subtype constraint belongs to the innermost embedded type of
       * possible nested 'record of' or 'set of' constructs. */
      Type *t = $1;
      while (t->is_seof()) t = t->get_ofType();
      t->set_parsed_restrictions($3);
    }
    $$.type = $1;
    $$.id = $2;
  }
;

IdentifierOrAddressKeyword:
  IDentifier { $$ = $1; }
| AddressKeyword
  {
    $$ = new Identifier(Identifier::ID_TTCN, string("address"));
  }
;

SetOfDef: // 40
  SetKeyword optStringLength OfKeyword StructOfDefBody
  {
    Type *type = new Type(Type::T_SETOF, $4.type);
    if ($2) {
      vector<SubTypeParse> *vstp = new vector<SubTypeParse>;
      vstp->add(new SubTypeParse($2));
      type->set_parsed_restrictions(vstp);
    }
    type->set_location(infile, @$);
    $$ = new Def_Type($4.id, type);
  }
;

EnumDef: // 41
  EnumKeyword IdentifierOrAddressKeyword
  '{' EnumerationList optError '}'
  {
    Type *type = new Type(Type::T_ENUM_T, $4);
    type->set_location(infile, @$);
    $$ = new Def_Type($2, type);
  }
;

EnumerationList: // 44
  optError Enumeration
  {
    $$ = new EnumItems;
    $$->add_ei($2);
  }
| EnumerationList optError ',' optError Enumeration
  {
    $$ = $1;
    $$->add_ei($5);
  }
| EnumerationList optError ',' error { $$ = $1; }
| error { $$ = new EnumItems; }
;

Enumeration: // 45
  IDentifier
  {
    $$ = new EnumItem($1, NULL);
    $$->set_location(infile, @$);
  }
| IDentifier '(' SingleExpression optError ')'
  {
    $$ = new EnumItem($1, $3);
    $$->set_location(infile, @$);
  }
| IDentifier '(' error ')'
  {
    Value *value = new Value(Value::V_ERROR);
    value->set_location(infile, @3);
    $$ = new EnumItem($1, value);
    $$->set_location(infile, @$);
  }
;

SubTypeDef: // 47
  Type IdentifierOrAddressKeyword optArrayDef optSubTypeSpec
  {
    /* the subtype constraint belongs to the innermost type */
    if ($4) $1->set_parsed_restrictions($4);
    Type *type = $1;
    /* creation of array type(s) if necessary (from right to left) */
    for (size_t i = $3.nElements; i > 0; i--) {
      type = new Type(Type::T_ARRAY, type, $3.elements[i - 1], true);
      type->set_location(*$1);
    }
    Free($3.elements);
    $$ = new Def_Type($2, type);
  }
;

optSubTypeSpec: // [49]
  /* empty */ { $$ = 0; }
| AllowedValues { $$ = $1; }
| AllowedValues StringLength
  {
    $$ = $1;
    $$->add(new SubTypeParse($2));
  }
| StringLength
  {
    $$ = new vector<SubTypeParse>;
    $$->add(new SubTypeParse($1));
  }
;

AllowedValues: // 50
  '(' seqValueOrRange optError ')' { $$ = $2; }
  | '(' CharStringMatch optError ')'
  {
    $$ = new vector<SubTypeParse>;
    $$->add(new SubTypeParse($2));
  }
  | '(' error ')' { $$ = new vector<SubTypeParse>; }
;

seqValueOrRange:
  optError ValueOrRange
  {
    $$ = new vector<SubTypeParse>;
    $$->add($2);
  }
| seqValueOrRange optError ',' optError ValueOrRange
  {
    $$ = $1;
    $$->add($5);
  }
| seqValueOrRange optError ',' error { $$ = $1; }
;

ValueOrRange: // 51
  RangeDef { $$ = new SubTypeParse($1.lower, $1.lower_exclusive, $1.upper, $1.upper_exclusive); }
| Expression { $$ = new SubTypeParse($1); }
;

RangeDef: // 52
  LowerBound DotDot UpperBound
  {
    $$.lower_exclusive = false;
    $$.lower = $1;
    $$.upper_exclusive = false;
    $$.upper = $3;
  }
| '!' LowerBound DotDot UpperBound
  {
    $$.lower_exclusive = true;
    $$.lower = $2;
    $$.upper_exclusive = false;
    $$.upper = $4;
  }
| LowerBound DotDot '!' UpperBound
  {
    $$.lower_exclusive = false;
    $$.lower = $1;
    $$.upper_exclusive = true;
    $$.upper = $4;
  }
| '!' LowerBound DotDot '!' UpperBound
  {
    $$.lower_exclusive = true;
    $$.lower = $2;
    $$.upper_exclusive = true;
    $$.upper = $5;
  }
;

optStringLength:
  /* empty */ optError { $$ = 0; }
| StringLength { $$ = $1; }
;

StringLength: // 53
  LengthKeyword '(' Expression optError ')'
  {
    $$ = new LengthRestriction($3);
    $$->set_location(infile, @$);
  }
| LengthKeyword '(' Expression DotDot UpperBound optError ')'
  {
    $$ = new LengthRestriction($3, $5);
    $$->set_location(infile, @$);
  }
| LengthKeyword '(' error ')'
  {
    Value *value = new Value(Value::V_ERROR);
    value->set_location(infile, @3);
    $$ = new LengthRestriction(value);
    $$->set_location(infile, @$);
  }
;

PortType: // 55
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(infile, @$);
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $5);
    $$->set_location(infile, @$);
    delete $3;
  }
;

PortTypeList:
  PortType
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Reference**>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
| PortTypeList ',' optError PortType
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<Ttcn::Reference**>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

PortDef: // 56
  PortKeyword PortDefBody { $$ = $2; }
;

PortDefBody: // 57
  IDentifier PortDefAttribs
  {
    $$ = new Def_Type($1, $2);
  }
;

PortDefAttribs: // 60
  PortOperationMode PortDefLists
  {
    Definitions * defs = new Definitions();
    for (size_t i = 0; i < $2.varnElements; i++) {
      defs->add_ass($2.varElements[i]);
    }
    Free($2.varElements);
    PortTypeBody *body = new PortTypeBody($1,
      $2.in_list, $2.out_list, $2.inout_list,
      $2.in_all, $2.out_all, $2.inout_all, defs);
    body->set_location(infile, @$);
    $$ = new Type(Type::T_PORT, body);
    $$->set_location(infile, @$);
    delete $2.in_mappings;
    delete $2.out_mappings;
  }
| 
  PortOperationMode MapKeyword ToKeyword PortTypeList PortDefLists
  {
    Definitions * defs = new Definitions();
    for (size_t i = 0; i < $5.varnElements; i++) {
      defs->add_ass($5.varElements[i]);
    }
    Free($5.varElements);
    PortTypeBody *body = new PortTypeBody($1,
      $5.in_list, $5.out_list, $5.inout_list,
      $5.in_all, $5.out_all, $5.inout_all, defs);
    body->set_location(infile, @$);
    $$ = new Type(Type::T_PORT, body);
    body->add_user_attribute($4.elements, $4.nElements, $5.in_mappings, $5.out_mappings, false);
    delete $4.elements;
    $$->set_location(infile, @$);
  }
;

PortOperationMode:
  MessageKeyword { $$ = PortTypeBody::PO_MESSAGE; } // 61
| ProcedureKeyword { $$ = PortTypeBody::PO_PROCEDURE; } // 68
| MixedKeyword { $$ = PortTypeBody::PO_MIXED; } // 73
| error { $$ = PortTypeBody::PO_MIXED; }
;

PortDefLists:
  '{' seqPortDefList '}' { $$ = $2; }
| '{' error '}'
  {
    $$.in_list = 0;
    $$.out_list = 0;
    $$.inout_list = 0;
    $$.in_all = false;
    $$.out_all = false;
    $$.inout_all = false;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
;

seqPortDefList:
  optError PortDefList optSemiColon { $$ = $2; }
| seqPortDefList PortDefList optSemiColon
  {
    $$ = $1;
    if ($2.in_list) {
      if ($$.in_list) {
        $$.in_list->steal_types($2.in_list);
        delete $2.in_list;
      } else $$.in_list = $2.in_list;
    }
    if ($2.in_mappings) {
      if ($$.in_mappings) {
        $$.in_mappings->steal_mappings($2.in_mappings);
        delete $2.in_mappings;
      } else {
        $$.in_mappings = $2.in_mappings;
      }
    }
    if ($2.out_list) {
      if ($$.out_list) {
        $$.out_list->steal_types($2.out_list);
        delete $2.out_list;
      } else $$.out_list = $2.out_list;
    }
    if ($2.out_mappings) {
      if ($$.out_mappings) {
        $$.out_mappings->steal_mappings($2.out_mappings);
        delete $2.out_mappings;
      } else { 
        $$.out_mappings = $2.out_mappings;
      }
    }
    if ($2.inout_list) {
      if ($$.inout_list) {
        $$.inout_list->steal_types($2.inout_list);
        delete $2.inout_list;
      } else $$.inout_list = $2.inout_list;
    }
    if ($2.in_all) {
      if ($$.in_all) {
        Location loc(infile, @2);
        loc.warning("Duplicate directive `in all' in port type definition");
      } else $$.in_all = true;
    }
    if ($2.out_all) {
      if ($$.out_all) {
        Location loc(infile, @2);
        loc.warning("Duplicate directive `out all' in port type definition");
      } else $$.out_all = true;
    }
    if ($2.inout_all) {
      if ($$.inout_all) {
        Location loc(infile, @2);
        loc.warning("Duplicate directive `inout all' in port type definition");
      } else $$.inout_all = true;
    }
    if ($2.varnElements > 0) {
      size_t i = $$.varnElements;
      $$.varnElements += $2.varnElements;
      $$.varElements = static_cast<Ttcn::Definition**>(Realloc($$.varElements, $$.varnElements * sizeof(Ttcn::Definition*)));
      // Intentional j start
      for (size_t j = 0; i < $$.varnElements; i++, j++) {
        $$.varElements[i] = $2.varElements[j];
      }
      Free($2.varElements);
    }
  }
;

PortDefList:
  InParKeyword AllOrTypeListWithFrom
  {
    if ($2.types) {
      $$.in_list = $2.types;
      $$.in_list->set_location(infile, @$);
      $$.in_all = false;
      if ($2.mappings) {
        $$.in_mappings = $2.mappings;
      } else {
        $$.in_mappings = 0;
      }
    } else {
      $$.in_list = 0;
      $$.in_all = true;
      $$.in_mappings = 0;
    }
    $$.out_list = 0;
    $$.out_all = false;
    $$.inout_list = 0;
    $$.out_mappings = 0;
    $$.inout_all = false;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| OutParKeyword AllOrTypeListWithTo
  {
    if ($2.types) {
      $$.out_list = $2.types;
      $$.out_list->set_location(infile, @$);
      $$.out_all = false;
      if ($2.mappings) {
        $$.out_mappings = $2.mappings;
      } else {
        $$.out_mappings = 0;
      }
    } else {
      $$.out_list = 0;
      $$.out_all = true;
      $$.out_mappings = 0;
    }
    $$.in_list = 0;
    $$.in_all = false;
    $$.in_mappings = 0;
    $$.inout_list = 0;
    $$.inout_all = false;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| InOutParKeyword AllOrTypeList
  {
    $$.in_list = 0;
    $$.in_all = false;
    $$.out_list = 0;
    $$.out_all = false;
    if ($2.types) {
      $$.inout_list = $2.types;
      $$.inout_list->set_location(infile, @$);
      $$.inout_all = false;
    } else {
      $$.inout_list = 0;
      $$.inout_all = true;
    }
    delete $2.mappings;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| InParKeyword error
  {
    $$.in_list = 0;
    $$.out_list = 0;
    $$.inout_list = 0;
    $$.in_all = false;
    $$.out_all = false;
    $$.inout_all = false;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| OutParKeyword error
  {
    $$.in_list = 0;
    $$.out_list = 0;
    $$.inout_list = 0;
    $$.in_all = false;
    $$.out_all = false;
    $$.inout_all = false;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| InOutParKeyword error
  {
    $$.in_list = 0;
    $$.out_list = 0;
    $$.inout_list = 0;
    $$.in_all = false;
    $$.out_all = false;
    $$.inout_all = false;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = 0;
    $$.varElements = 0;
  }
| PortElementVarDef optSemiColon {
    $$.in_list = 0;
    $$.out_list = 0;
    $$.inout_list = 0;
    $$.in_all = false;
    $$.out_all = false;
    $$.inout_all = false;
    $$.in_mappings = 0;
    $$.out_mappings = 0;
    $$.varnElements = $1.nElements;
    $$.varElements = static_cast<Ttcn::Definition**>(Malloc($$.varnElements * sizeof(Ttcn::Definition*)));
    for (size_t i = 0; i < $$.varnElements; i++) {
      $$.varElements[i] = $1.elements[i];
    }
    delete $1.elements;
}
;

WithList:
  Type WithKeyword FunctionRef '(' optError ')'
  {
    $$ = new TypeMappingTargets();
    Ttcn::Reference *func_ref = new Ttcn::Reference($3.modid, $3.id);
    TypeMappingTarget * tm = new TypeMappingTarget($1, TypeMappingTarget::TM_FUNCTION, func_ref);
    $$->add_target(tm);
  }
| WithList ':' optError Type WithKeyword FunctionRef '(' optError ')'
  {
    Ttcn::Reference *func_ref = new Ttcn::Reference($6.modid, $6.id);
    TypeMappingTarget * tm = new TypeMappingTarget($4, TypeMappingTarget::TM_FUNCTION, func_ref);
    $$ = $1;
    $$->add_target(tm);
  }
;

AllOrTypeList: // 65
  AllKeyword { $$.types = 0; $$.mappings = 0; }
| TypeList { $$ = $1; }
;

TypeList: // 67
  optError Type
  {
    $$.types = new Types;
    $$.types->add_type($2);
    $$.mappings = new TypeMappings();
  }
| TypeList optError ',' optError Type
  {
    $$ = $1;
    $$.types->add_type($5);
  }
| TypeList optError ',' error { $$ = $1; }
;

AllOrTypeListWithFrom:
  AllKeyword { $$.types = 0; $$.mappings = 0; }
| TypeListWithFrom { $$ = $1; }
;

TypeListWithFrom:
  optError Type
  {
    $$.types = new Types;
    $$.types->add_type($2);
    $$.mappings = new TypeMappings();
  }
| optError Type optError FromKeyword WithList
  {
    $$.types = new Types;
    $$.types->add_type($2);
    $$.mappings = new TypeMappings();
    $$.mappings->add_mapping(new TypeMapping($2->clone(), $5));
  }
| TypeListWithFrom optError ',' optError Type
  {
    $$ = $1;
    $$.types->add_type($5);
  }
| TypeListWithFrom optError ',' optError Type FromKeyword WithList
  {
    $$ = $1;
    $$.types->add_type($5);
    $$.mappings->add_mapping(new TypeMapping($5->clone(), $7));
  }
| TypeListWithFrom optError ',' error { $$ = $1; }
;

AllOrTypeListWithTo:
  AllKeyword { $$.types = 0; $$.mappings = 0; }
| TypeListWithTo { $$ = $1; }
;

TypeListWithTo:
  optError Type
  {
    $$.types = new Types;
    $$.types->add_type($2);
    $$.mappings = new TypeMappings();
  }
| optError Type optError ToKeyword WithList
  {
    $$.types = new Types;
    $$.types->add_type($2);
    $$.mappings = new TypeMappings();
    $$.mappings->add_mapping(new TypeMapping($2->clone(), $5));
  }
| TypeListWithTo optError ',' optError Type
  {
    $$ = $1;
    $$.types->add_type($5);
  }
| TypeListWithTo optError ',' optError Type ToKeyword WithList
  {
    $$ = $1;
    $$.types->add_type($5);
    $$.mappings->add_mapping(new TypeMapping($5->clone(), $7));
  }
| TypeListWithTo optError ',' error { $$ = $1; }
;

PortElementVarDef:
  VarInstance { $$ = $1; }
| ConstDef { $$ = $1; }

ComponentDef: // 78
  ComponentKeyword IDentifier
  optExtendsDef
  '{' optComponentDefList '}'
  {
    $5->set_id($2);
    if ($3) $5->add_extends($3);
    $5->set_location(infile, @$);
    Type *type = new Type(Type::T_COMPONENT, $5);
    type->set_location(infile, @$);
    $$ = new Def_Type($2, type);
  }
;

optExtendsDef:
  /* empty */ optError { $$ = 0; }
| ExtendsKeyword ComponentTypeList optError
  {
    $$ = $2;
    $$->set_location(infile, @1, @2);
  }
| ExtendsKeyword error { $$ = 0; }
;

ComponentTypeList:
  optError ComponentType
  {
    $$ = new CompTypeRefList();
    $$->add_ref($2);
  }
| ComponentTypeList optError ',' optError ComponentType
  {
    $$ = $1;
    $$->add_ref($5);
  }
| ComponentTypeList optError ',' error { $$ = $1; }
;

ComponentType: // 81
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(infile, @$);
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $5);
    $$->set_location(infile, @$);
    delete $3;
  }
;

optComponentDefList:
  optError /* empty */ { $$ = new ComponentTypeBody(); }
| ComponentElementDefList optError { $$ = $1; }
;

ComponentElementDefList:
  optError ComponentElementDef optSemiColon
  {
    $$ = new ComponentTypeBody();
    for (size_t i = 0; i < $2.nElements; i++)
      $$->add_ass($2.elements[i]);
    Free($2.elements);
  }
| ComponentElementDefList optError ComponentElementDef optSemiColon
  {
    $$ = $1;
    for (size_t i = 0; i < $3.nElements; i++)
      $$->add_ass($3.elements[i]);
    Free($3.elements);
  }
;

ComponentElementVisibility:
  PublicKeyword { $$.visibility = PUBLIC;}
| PrivateKeyword { $$.visibility = PRIVATE;}
;

ComponentElementDef: // 84
  PortInstance { $$ = $1; }
| VarInstance { $$ = $1; }
| TimerInstance { $$ = $1; }
| ConstDef { $$ = $1; }
| ComponentElementVisibility PortInstance
  {
    $$ = $2;
    for (size_t i = 0; i < $2.nElements; i++) {
      $2.elements[i]->set_visibility($1.visibility);
    }
  }
| ComponentElementVisibility VarInstance
  {
    $$ = $2;
    for (size_t i = 0; i < $2.nElements; i++) {
      $2.elements[i]->set_visibility($1.visibility);
    }
  }
| ComponentElementVisibility TimerInstance
  {
    $$ = $2;
    for (size_t i = 0; i < $2.nElements; i++) {
      $2.elements[i]->set_visibility($1.visibility);
    }
  }
| ComponentElementVisibility ConstDef
  {
    $$ = $2;
    for (size_t i = 0; i < $2.nElements; i++) {
      $2.elements[i]->set_visibility($1.visibility);
    }
  }
;

PortInstance: // 85
  PortKeyword PortType PortElementList
  {
    $$.nElements = $3.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements*sizeof(*$$.elements)));
    for (size_t i = 0; i < $3.nElements; i++) {
      Ttcn::Reference *ref = i > 0 ? $2->clone() : $2;
      $$.elements[i] = new Ttcn::Def_Port($3.elements[i].id, ref,
                                          $3.elements[i].dims);
      $$.elements[i]->set_location(infile, $3.elements[i].yyloc);
    }
    Free($3.elements);
  }
;

PortElementList:
  optError PortElement
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::portelement_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| PortElementList ',' optError PortElement
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::portelement_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
| PortElementList ',' error { $$ = $1; }
;

PortElement: // 86
  IDentifier optArrayDef
  {
    $$.id = $1;
    if ($2.nElements > 0) {
      $$.dims = new ArrayDimensions;
      for (size_t i = 0; i < $2.nElements; i++) $$.dims->add($2.elements[i]);
      Free($2.elements);
    } else $$.dims = 0;
    $$.yyloc = @$;
  }
;

/* A.1.6.1.2 Constant definitions */

ConstDef: // 88
  ConstKeyword Type ConstList
  {
    $$.nElements = $3.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements*sizeof(*$$.elements)));
    for (size_t i = 0; i < $$.nElements; i++) {
      Type *type;
      if (i > 0) {
	type = new Type(Type::T_REFDSPEC, $2);
	type->set_location(*$2);
      } else type = $2;
      /* creation of array type(s) if necessary (from right to left) */
      for (size_t j = $3.elements[i].arrays.nElements; j > 0; j--) {
	type = new Type(Type::T_ARRAY, type,
	  $3.elements[i].arrays.elements[j - 1], false);
	type->set_location(*$2);
      }
      Free($3.elements[i].arrays.elements);

      /* Create the definition */
      $$.elements[i] = new Def_Const($3.elements[i].id,
                                     type, $3.elements[i].initial_value);
      $$.elements[i]->set_location(infile, $3.elements[i].yyloc);
    }
    Free($3.elements);
  }
;

ConstList: // 98
  optError SingleConstDef
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::singlevarinst_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| ConstList ',' optError SingleConstDef
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::singlevarinst_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

SingleConstDef: // 90
  IDentifier optArrayDef AssignmentChar Expression
  {
    $$.id = $1;
    $$.arrays = $2;
    $$.initial_value = $4;
    $$.yyloc = @$;
  }
;

FunctionTypeDef:
  FunctionKeyword IDentifier '(' optFunctionFormalParList ')'
  optRunsOnComprefOrSelf optReturnType
  {
    $4->set_location(infile, @3, @5);
    Type *type = new Type(Type::T_FUNCTION, $4, $6.reference, $6.self, $7.type,
                          $7.returns_template, $7.template_restriction);
    type->set_location(infile, @$);
    $$ = new Ttcn::Def_Type($2, type);
    $$->set_location(infile, @$);
  }
;

AltstepTypeDef:
  AltstepKeyword IDentifier '(' optAltstepFormalParList ')'
  optRunsOnComprefOrSelf
  {
    $4->set_location(infile, @3, @5);
    Type *type = new Type(Type::T_ALTSTEP, $4, $6.reference, $6.self);
    type->set_location(infile, @$);
    $$ = new Ttcn::Def_Type($2, type);
    $$->set_location(infile, @$);
  }
;

TestcaseTypeDef:
  TestcaseKeyword IDentifier '(' optTestcaseFormalParList ')'
  ConfigSpec
  {
    $4->set_location(infile, @3, @5);
    Type *type = new Type(Type::T_TESTCASE, $4, $6.runsonref,
                          $6.systemref);
    type->set_location(infile, @$);
    $$ = new Ttcn::Def_Type($2, type);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.1.3 Template definitions */

TemplateDef: // 93
  TemplateKeyword optTemplateRestriction optLazyOrFuzzyModifier BaseTemplate
  optDerivedDef AssignmentChar TemplateBody
  {
    $$ = new Def_Template($2, $4.name, $4.type, $4.fp_list, $5, $7);
    $$->set_location(infile, @$);
  }
;

BaseTemplate: // 94
  Type IDentifier optTemplateFormalParList
  {
    $$.name = $2;
    $$.type = $1;
    $$.fp_list = $3;
  }
/* | Signature IDentifier optTemplateFormalParList -- covered by the previous
   rule */
;

optDerivedDef:
  /* empty */ { $$ = 0; }
| DerivedDef { $$ = $1; }
;

DerivedDef: // 97
  ModifiesKeyword TemplateRef
  {
    $$ = new Ttcn::Reference($2.modid, $2.id);
    $$->set_location(infile, @$);
  }
| ModifiesKeyword error { $$ = 0; }
;

optTemplateFormalParList:
  /* empty */ optError { $$ = 0; }
| '(' TemplateFormalParList optError ')'
  {
    $$ = $2;
    $$->set_location(infile, @$);
  }
| '(' error ')'
  {
    $$ = new FormalParList;
    $$->set_location(infile, @$);
  }
;

TemplateFormalParList: // 99 is a FormalParList*
  optError TemplateFormalPar
  {
    $$ = new FormalParList;
    $$->add_fp($2);
  }
| TemplateFormalParList optError ',' optError TemplateFormalPar
  {
    $$ = $1;
    $$->add_fp($5);
  }
| TemplateFormalParList optError ',' error { $$ = $1; }
;

TemplateFormalPar: // 100
  FormalValuePar { $$ = $1; }
| FormalTemplatePar { $$ = $1; }
;

TemplateBody: // 101 is a Template*
  SimpleSpec optExtraMatchingAttributes
  {
    $$ = $1;
    $$->set_length_restriction($2.len_restr);
    $$->set_ifpresent($2.is_ifpresent);
  }
| FieldSpecList optExtraMatchingAttributes
  {
    $$ = $1;
    $$->set_length_restriction($2.len_restr);
    $$->set_ifpresent($2.is_ifpresent);
  }
| ArraySpecList optExtraMatchingAttributes
  {
    $$ = $1;
    $$->set_length_restriction($2.len_restr);
    $$->set_ifpresent($2.is_ifpresent);
  }
| ArrayValueOrAttrib optExtraMatchingAttributes
  {
    $$ = $1;
    $$->set_length_restriction($2.len_restr);
    $$->set_ifpresent($2.is_ifpresent);
  }
| DecodedContentMatch
  {
    $$ = new Template($1.string_encoding, $1.target_template);
    $$->set_location(infile, @$);
  }
;

SimpleSpec: // 102
  SingleValueOrAttrib { $$ = $1; }
;

FieldSpecList: // 103
  '{' '}'
  {
    $$ = new Template(Template::TEMPLATE_LIST, new Templates());
    $$->set_location(infile, @$);

  }
| '{' seqFieldSpec optError '}'
  {
    $$ = new Template($2); // NAMED_TEMLATE_LIST
    $$->set_location(infile, @$);
  }
| '{' error '}'
  {
    $$ = new Template(Template::TEMPLATE_ERROR);
    $$->set_location(infile, @$);
  }
;

/* Sequence of FieldSpec. \note Cannot be empty */
seqFieldSpec: // a NamedTemplates*
  FieldSpec
  {
    $$ = new NamedTemplates();
    $$->add_nt($1);
  }
| error FieldSpec
  {
    $$ = new NamedTemplates();
    $$->add_nt($2);
  }
| seqFieldSpec optError ',' optError FieldSpec
  {
    $$=$1;
    $$->add_nt($5);
  }
| seqFieldSpec optError ',' error { $$ = $1; }
;

FieldSpec: // 104 a NamedTemplate*
  FieldReference AssignmentChar TemplateBody
  {
    $$ = new NamedTemplate($1, $3);
    $$->set_location(infile, @$);
  }
| FieldReference AssignmentChar NotUsedSymbol
  {
    Template* temp = new Template(Template::TEMPLATE_NOTUSED);
    temp->set_location(infile, @3);
    $$ = new NamedTemplate($1, temp);
    $$->set_location(infile, @$);
  }
;

FieldReference: // 105
  StructFieldRef { $$ = $1; }
/*  | ArrayOrBitRef -- covered by ArraySpecList */
/*  | ParRef -- covered by StructFieldRef */
;

StructFieldRef: // 106 (and 107. ParRef)
  PredefOrIdentifier { $$ = $1; }
/*  | TypeReference
Note: Non-parameterized type references are covered by (StructField)Identifier.
      Parameterized type references are covered by FunctionInstance */
| FunctionInstance
  {
    Location loc(infile, @$);
    loc.error("Reference to a parameterized field of type `anytype' is "
      "not currently supported");
    delete $1;
    $$ = new Identifier(Identifier::ID_TTCN, string("<error>"));
  }
;

ArraySpecList:
  '{' seqArraySpec optError '}'
  {
    $$ = new Template($2);
    $$->set_location(infile, @$);
  }
;

seqArraySpec:
  ArraySpec
  {
    $$ = new IndexedTemplates();
    $$->add_it($1);
  }
  /* It was optError before.  */
| error ArraySpec
  {
    $$ = new IndexedTemplates();
    $$->add_it($2);
  }
| seqArraySpec optError ',' optError ArraySpec
  {
    $$ = $1;
    $$->add_it($5);
  }
| seqArraySpec optError ',' error { $$ = $1; }
;

ArraySpec:
  ArrayOrBitRef AssignmentChar TemplateBody
  {
    $$ = new IndexedTemplate($1, $3);
    $$->set_location(infile, @$);
  }
;

ArrayOrBitRef: // 109
  '[' Expression ']'
  {
    $$ = new FieldOrArrayRef($2);
    $$->set_location(infile, @$);
  }
| '[' error ']'
  {
    $$ = new FieldOrArrayRef(new Value(Value::V_ERROR));
    $$->set_location(infile, @$);
  }
;

SingleValueOrAttrib: // 111
  MatchingSymbol { $$ = $1; }
| SingleExpression
  {
    // SingleExpr is a Template*
    // this constructor determines the template type based on the value
    $$ = new Template($1);
    $$->set_location(infile, @$);
  }
/* | TemplateRefWithParList -- covered by SingleExpression */
;

ArrayValueOrAttrib: // 112
  '{' ArrayElementSpecList optError '}'
  {
    $$ = new Template(Template::TEMPLATE_LIST, $2);
    $$->set_location(infile, @$);
  }
;

ArrayElementSpecList: // 113
  ArrayElementSpec
  {
    $$ = new Templates;
    $$->add_t($1);
  }
| error ArrayElementSpec
  {
    $$ = new Templates;
    $$->add_t($2);
  }
| ArrayElementSpecList optError ',' optError ArrayElementSpec
  {
    $$=$1;
    $$->add_t($5);
  }
| ArrayElementSpecList optError ',' error { $$ = $1; }
;

ArrayElementSpec: // 114 is a Template*
  NotUsedSymbol
  {
    $$ = new Template(Template::TEMPLATE_NOTUSED);
    $$->set_location(infile, @$);
  }
| PermutationMatch
  {
    $$ = new Template(Template::PERMUTATION_MATCH, $1);
    $$->set_location(infile, @$);
  }
| TemplateListElem { $$ = $1; }
;

NotUsedSymbol: // 115
  '-'
;

MatchingSymbol: // 116 is a Template*
  Complement
  {
    $$ = new Template(Template::COMPLEMENTED_LIST, $1);
    $$->set_location(infile, @$);
  }
/*| AnyValue // these are in the SingleExpression and Expression rules now
  {
    $$ = new Template(Template::ANY_VALUE);
    $$->set_location(infile, @$);
  }
| AnyOrOmit
  {
    $$ = new Template(Template::ANY_OR_OMIT);
    $$->set_location(infile, @$);
  }*/
| ValueOrAttribList
  {
    $$ = new Template(Template::VALUE_LIST, $1);
    $$->set_location(infile, @$);
  }
| Range
  {
    $$ = new Template($1);
    $$->set_location(infile, @$);
  }
| BitStringMatch
  {
    $$ = new Template(Template::BSTR_PATTERN, $1);
    $$->set_location(infile, @$);
  }
| HexStringMatch
  {
    $$ = new Template(Template::HSTR_PATTERN, $1);
    $$->set_location(infile, @$);
  }
| OctetStringMatch
  {
    $$ = new Template(Template::OSTR_PATTERN, $1);
    $$->set_location(infile, @$);
  }
| CharStringMatch
  {
    $$ = new Template($1);
    $$->set_location(infile, @$);
  }
| SubsetMatch
  {
    $$ = new Template(Template::SUBSET_MATCH, $1);
    $$->set_location(infile, @$);
  }
| SupersetMatch
  {
    $$ = new Template(Template::SUPERSET_MATCH, $1);
    $$->set_location(infile, @$);
  }
;

optExtraMatchingAttributes: // [117]
  /* empty */
  {
    $$.is_ifpresent = false;
    $$.len_restr = NULL;
  }
| LengthMatch
  {
    $$.len_restr = $1;
    $$.is_ifpresent = false;
  }
| IfPresentMatch
  {
    $$.len_restr = NULL;
    $$.is_ifpresent = true;
  }
| LengthMatch IfPresentMatch
  {
    $$.len_restr = $1;
    $$.is_ifpresent = true;
  }
;

CharStringMatch: // 124
  PatternKeyword PatternChunkList
  {
    Location loc(infile, @2);
    $$ = parse_pattern($2, loc);
    Free($2);
  }
| PatternKeyword NocaseKeyword PatternChunkList
  {
    Location loc(infile, @3);
    $$ = parse_pattern($3, loc);
    $$->set_nocase(true);
    Free($3);
  }
;

PatternChunkList:
  PatternChunk
  {
    $$ = $1;
  }
| PatternChunkList '&' PatternChunk
  {
    $$ = $1;
    $$ = mputstr($$, $3);
    Free($3);
  }
;

PatternChunk:
  Cstring
  {
    $$ = $1;
  }
| ReferencedValue
  {
    switch ($1->get_valuetype()) {
    case Value::V_REFD: {
      /* Pretend to be a reference.  Let pstring_la.l discover the
         references.  */
      Common::Reference *ref = $1->get_reference();
      $$ = mprintf("{%s}", (ref->get_dispname()).c_str());
      break; }
    case Value::V_UNDEF_LOWERID: {
      const Common::Identifier *id = $1->get_val_id();
      $$ = mprintf("{%s}", (id->get_dispname()).c_str());
      break; }
    default:
      FATAL_ERROR("Internal error.");
    }
    /* Forget the Node.  */
    delete $1;
  }
| Quadruple
  {
    ustring::universal_char& uc = $1->operator[](0);
    $$ = mprintf("\\q{%d,%d,%d,%d}", uc.group, uc.plane, uc.row, uc.cell);
    delete $1;
  }
;

Complement: // 130 is a Templates*
  ComplementKeyword ValueList { $$ = $2; }
;

ValueList: // 132 is a Templates*
  '(' seqValueOrAttrib optError ')' { $$ = $2; }
| '(' error ')' { $$ = new Templates; }
;

seqValueOrAttrib: // is a Templates*
  optError TemplateListElem
  {
    $$ = new Templates;
    $$->add_t($2);
  }
| seqValueOrAttrib optError ',' optError TemplateListElem
  {
    $$ = $1;
    $$->add_t($5);
  }
| seqValueOrAttrib optError ',' error { $$ = $1; }
;

SubsetMatch: // 133 is a Templates*
  SubsetKeyword ValueList { $$ = $2; }
;

SupersetMatch: // 135 is a Templates*
  SupersetKeyword ValueList { $$ = $2; }
;

PermutationMatch: // 137 is a Templates*
  PermutationKeyword ValueList { $$ = $2; }
;

DecodedContentMatch:
  DecodedMatchKeyword '(' SingleExpression ')' InLineTemplate
  {
    $$.string_encoding = $3;
    $$.target_template = $5;
  }
| DecodedMatchKeyword InLineTemplate
  {
    $$.string_encoding = NULL;
    $$.target_template = $2;
  }
;

AnyValue: // 140
  '?'
;

AnyOrOmit: // 141
  '*'
;

TemplateListElem: // is a Template*
  TemplateBody
| AllElementsFrom
;

AllElementsFrom: // is a Template*
  AllKeyword FromKeyword TemplateBody
  { // $3 is a Template*
    $$ = new Template($3); // Constructs ALL_FROM
    $$->set_location(infile, @$);
  }
;

ValueOrAttribList: // 142 is a Templates*
  /* ValueOrAttribList always has two or more elements
   * (there's always a comma) and is reduced through
   * ValueOrAttribList -> MatchingSymbol -> SingleValueOrAttrib
   *
   * The one-element list is reduced through
   * '(' SingleExpression ')' -> SingleExpression -> SingleValueOrAttrib */
  '(' TemplateListElem optError ',' seqValueOrAttrib optError ')'
  {
    $$ = $5;
    $$->add_front_t($2);
  }
| '(' TemplateListElem optError ')'
  {
    $$ = new Templates;
    $$->add_front_t($2);
  }
| '(' error TemplateListElem optError ',' seqValueOrAttrib optError ')'
  {
    $$ = $6;
    $$->add_front_t($3);
  }
;

LengthMatch: // 143
  StringLength { $$ = $1; }
;

IfPresentMatch: // 144
  IfPresentKeyword
;

Range: // 147
  '(' SingleLowerBound DotDot UpperBound optError ')'
  { $$ = new ValueRange($2, false, $4, false); }
| '(' '!' SingleLowerBound DotDot UpperBound optError ')'
  { $$ = new ValueRange($3, true, $5, false); }
| '(' SingleLowerBound DotDot '!' UpperBound optError ')'
  { $$ = new ValueRange($2, false, $5, true); }
| '(' '!' SingleLowerBound DotDot '!' UpperBound optError ')'
  { $$ = new ValueRange($3, true, $6, true); }
;

SingleLowerBound:
  SingleExpression
  {
    if ($1->is_parsed_infinity()==-1) {
      /* the conflicting rule alternative faked here: '-' InfinityKeyword */
      delete $1;
      $$ = 0;
    } else {
      $$ = $1;
    }
  }
;

LowerBound: // 148
  Expression
  {
    if ($1->is_parsed_infinity()==-1) {
      /* the conflicting rule alternative faked here: '-' InfinityKeyword */
      delete $1;
      $$ = 0;
    } else {
      $$ = $1;
    }
  }
;

UpperBound: // 149
  Expression
  {
    if ($1->is_parsed_infinity()==1) {
      /* the conflicting rule alternative faked here: InfinityKeyword */
      delete $1;
      $$ = 0;
    } else {
      $$ = $1;
    }
  }
;

TemplateInstance: // 151
  InLineTemplate { $$ = $1; }
;

TemplateRefWithParList: /* refbase */ // 153 ?
  TemplateRef optTemplateActualParList
  {
    if ($2) {
      $$ = new Ttcn::Ref_pard($1.modid, $1.id, new ParsedActualParameters($2));
      $$->set_location(infile, @$);
    } else {
      $$ = new Ttcn::Reference($1.modid, $1.id);
      $$->set_location(infile, @$);
    }
  }
;

TemplateRef: // 154
  IDentifier
  {
    $$.modid = 0;
    $$.id = $1;
  }
| IDentifier '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $3;
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $5;
    delete $3;
  }
;

InLineTemplate: // 155
  TemplateBody
  {
    $$ = new TemplateInstance(0, 0, $1);
    $$->set_location(infile, @$);
  }
| Type ':' TemplateBody
  {
    $$ = new TemplateInstance($1, 0, $3);
    $$->set_location(infile, @$);
  }
/* | Signature ':' TemplateBody -- covered by the previous rule */
| DerivedRefWithParList AssignmentChar TemplateBody
  {
    $$ = new TemplateInstance(0, $1, $3);
    $$->set_location(infile, @$);
  }
| Type ':' DerivedRefWithParList AssignmentChar TemplateBody
  {
    $$ = new TemplateInstance($1, $3, $5);
    $$->set_location(infile, @$);
  }
/* | Signature ':' DerivedRefWithParList AssignmentChar TemplateBody
 -- covered by the previous rule */
;

DerivedRefWithParList: // 156
  ModifiesKeyword TemplateRefWithParList { $$ = $2; }
;

optTemplateActualParList: // [157]
  /* empty */ optError { $$ = 0; }
| '(' seqTemplateActualPar optError ')'
  {
    $$ = $2;
    $$->set_location(infile, @$);
  }
| '(' error ')'
  {
    $$ = new TemplateInstances;
    $$->set_location(infile, @$);
  }
;

seqTemplateActualPar:
  optError TemplateActualPar
  {
    $$ = new TemplateInstances;
    $$->add_ti($2);
  }
| seqTemplateActualPar optError ',' optError TemplateActualPar
  {
    $$ = $1;
    $$->add_ti($5);
  }
| seqTemplateActualPar optError ',' error { $$ = $1; }
;

TemplateActualPar: // 158
  TemplateInstance { $$ = $1; }
| NotUsedSymbol
  {
    Template *t = new Template(Template::TEMPLATE_NOTUSED);
    t->set_location(infile, @$);
    $$ = new TemplateInstance(0, 0, t);
    $$->set_location(infile, @$);
  }
;

TemplateOps: // 159
  MatchOp { $$ = $1; }
| ValueofOp { $$ = $1; }
;

MatchOp: // 160
  MatchKeyword '(' optError Expression optError ',' optError TemplateInstance
  optError ')'
  {
    $$ = new Value(Value::OPTYPE_MATCH, $4, $8);
    $$->set_location(infile, @$);
  }
| MatchKeyword '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @3);
    TemplateInstance *ti = new TemplateInstance(0, 0, t);
    ti->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_MATCH, v, ti);
    $$->set_location(infile, @$);
  }
;

ValueofOp: // 162
  ValueofKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_VALUEOF, $4);
    $$->set_location(infile, @$);
  }
| ValueofKeyword '(' error ')'
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @3);
    TemplateInstance *ti = new TemplateInstance(0, 0, t);
    ti->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_VALUEOF, ti);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.1.4 Function definitions */

FunctionDef: // 164
  FunctionKeyword optDeterministicModifier IDentifier '(' optFunctionFormalParList ')'
  optRunsOnSpec optPortSpec optReturnType optError StatementBlock
  {
    $5->set_location(infile, @4, @6);
    $$ = new Def_Function($3, $5, $7, $8, $9.type, $9.returns_template,
                          $9.template_restriction, $11);
    $$->set_location(infile, @$);
  }
;

optDeterministicModifier:
  /* empty */
| DeterministicKeyword /* just ignore it for now */
;

optFunctionFormalParList: // [167]
  /* empty */ { $$ = new FormalParList; }
| FunctionFormalParList { $$ = $1; }
| error { $$ = new FormalParList; }
;

FunctionFormalParList: // 167
  optError FunctionFormalPar
  {
    $$ = new FormalParList;
    $$->add_fp($2);
  }
| FunctionFormalParList optError ',' optError FunctionFormalPar
  {
    $$ = $1;
    $$->add_fp($5);
  }
| FunctionFormalParList optError ',' error { $$ = $1; }
;

FunctionFormalPar: // 168
  FormalValuePar { $$ = $1; }
| FormalTimerPar { $$ = $1; }
| FormalTemplatePar { $$ = $1; }
/*| FormalPortPar -- covered by FormalValuePar */
;

optReturnType: // [169]
  /* empty */
  {
    $$.type = 0;
    $$.returns_template = false;
    $$.template_restriction = TR_NONE;
  }
| ReturnKeyword Type
  {
    $$.type = $2;
    $$.returns_template = false;
    $$.template_restriction = TR_NONE;
  }
| ReturnKeyword TemplateOptRestricted Type
  {
    $$.type = $3;
    $$.returns_template = true;
    $$.template_restriction = $2;
  }
| ReturnKeyword error
  {
    $$.type = 0;
    $$.returns_template = false;
    $$.template_restriction = TR_NONE;
  }
;

optRunsOnComprefOrSelf:
  optRunsOnSpec
  {
    $$.self = false;
    $$.reference = $1;
  }
| RunsKeyword OnKeyword SelfKeyword
  {
    $$.self = true;
    $$.reference = 0;
  }
;

optRunsOnSpec:
  /* empty */ { $$ = 0; }
| RunsOnSpec { $$ = $1; }
| RunsKeyword error { $$ = 0; }
;

RunsOnSpec: // 171
  RunsKeyword OnKeyword ComponentType { $$ = $3; }
;

optPortSpec:
  /* empty */ { $$ = 0; }
| PortKeyword Port { $$ = $2; }
;


  /* StatementBlock changed in 4.1.2 to explicitly prevent statements
   * followed by definitions. TITAN still allows them to be mixed. */
StatementBlock: /* StatementBlock *statementblock */ // 175
  '{' optError '}'
  {
    $$ = new StatementBlock;
    $$->set_location(infile, @$);
  }
| '{' FunctionStatementOrDefList optError '}'
  {
    $$ = $2;
    $$->set_location(infile, @$);
  }
;

FunctionStatementOrDefList: // (171 in 3.2.1)
  optError FunctionStatementOrDef optSemiColon
  {
    $$ = new StatementBlock;
    for(size_t i=0; i<$2.nElements; i++) $$->add_stmt($2.elements[i]);
    Free($2.elements);
  }
| FunctionStatementOrDefList optError FunctionStatementOrDef optSemiColon
  {
    $$=$1;
    for(size_t i=0; i<$3.nElements; i++) $$->add_stmt($3.elements[i]);
    Free($3.elements);
  }
;

FunctionStatementOrDef: // (172 in 3.2.1)
  FunctionLocalDef // constant or template definition
  {
    $$.nElements=$1.nElements;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    for(size_t i=0; i<$1.nElements; i++) {
      $$.elements[i]=new Statement(Statement::S_DEF, $1.elements[i]);
      $$.elements[i]->set_location(*$1.elements[i]);
    }
    Free($1.elements);
  }
| FunctionLocalInst // variable or timer instance
  {
    $$.nElements=$1.nElements;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    for(size_t i=0; i<$1.nElements; i++) {
      $$.elements[i]=new Statement(Statement::S_DEF, $1.elements[i]);
      $$.elements[i]->set_location(*$1.elements[i]);
    }
    Free($1.elements);
  }
| FunctionStatement
  {
    $$.nElements=1;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    $$.elements[0]=$1;
  }
;

FunctionLocalInst: // 178
  VarInstance { $$=$1; }
| TimerInstance { $$=$1; }
;

FunctionLocalDef: // 179
  ConstDef { $$=$1; }
| TemplateDef
  {
    $1->set_local();
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
;

FunctionStatement: // 180
  ConfigurationStatements {$$=$1;}
| TimerStatements {$$=$1;}
| CommunicationStatements {$$=$1;}
| BasicStatements {$$=$1;}
| BehaviourStatements {$$=$1;}
| VerdictStatements {$$=$1;}
| SUTStatements {$$=$1;}
| StopExecutionStatement { $$ = $1; }
| StopTestcaseStatement { $$ = $1; }
| ProfilerStatement { $$ = $1; }
| int2enumStatement { $$ = $1; }
| UpdateStatement { $$ = $1; }
| SetstateStatement { $$ = $1; }
;

FunctionInstance: /* refpard */ // 181
  FunctionRef '(' optFunctionActualParList ')'
  /* templateref  templinsts */
  {
    $3->set_location(infile, @2, @4);
    $$ = new Ttcn::Ref_pard($1.modid, $1.id, $3);
    $$->set_location(infile, @$);
  }
;

FunctionRef: // 182
  IDentifier
  {
    $$.modid = 0;
    $$.id = $1;
  }
  | IDentifier '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $3;
  }
  | IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $5;
    delete $3;
  }
;

optFunctionActualParList: /* parsedpar */ // [185]
  /* empty */ optError { $$ = new ParsedActualParameters; }
| FunctionActualParList optError { $$ = $1; }
;

/* ***** this * can * not * be * empty ****************** */
FunctionActualParList: /* parsedpar */ // 184
  UnnamedPart
| NamedPart
| UnnamedPart ',' NamedPart
/* Splitting the NamedPart and UnnamedPart ensures that a named parameter
 * followed by an unnamed one causes a syntax error */
{
    /* UnnamedPart becomes the value */
    $$ = $1;
    /* append the elements from NamedPart */
    const size_t n3 = $3->get_nof_nps();
    for (size_t i = 0; i < n3; ++i) {
        $$->add_np( $3->extract_np_byIndex(i) );
    }
    delete $3;
}
;

UnnamedPart: /* parsedpar */
/*optError*/ FunctionActualPar
{
  $$ = new ParsedActualParameters;
  $$->add_ti($1);
}
| UnnamedPart /*optError*/ ',' /*optError*/ FunctionActualPar
/* These optErrors mess up the parsing of actual parameters.
 * After only one FunctionActualPar, it is reduced to UnnamedPart
 * and the rest is expected to be the NamedPart */
{
  $$ = $1;
  $$->add_ti($3);
}
| UnnamedPart optError ',' error { $$ = $1; }
;

NamedPart: /* parsedpar */
  seqFieldSpec /* namedtempls */
  {
    $$ = new ParsedActualParameters(0, new NamedParams);
    const size_t n1 = $1->get_nof_nts();
    for (size_t i = 0; i < n1; ++i) {
	NamedTemplate *pnt = $1->get_nt_byIndex(i);
	TemplateInstance *pti = new TemplateInstance(0,0,pnt->extract_template());
	NamedParam *pnp = new NamedParam(pnt->get_name().clone(), pti);
	pnp->set_location(*pnt);
	$$->add_np(pnp);
    }
    delete $1;
    $$->set_location(infile, @$);
    // This call to ParsedActualParameters::set_location copies the same
    // location info to the named and unnamed part. We cannot copy
    // the location info from the NamedTemplates to the named part,
    // because NamedTemplates is not a Location.
  }
;

FunctionActualPar: /* templinst */ // 185
/*  TimerRef */
  TemplateInstance { $$ = $1; }
| NotUsedSymbol
  {
    Template *t = new Template(Template::TEMPLATE_NOTUSED);
    t->set_location(infile, @$);
    $$ = new TemplateInstance(0, 0, t);
    $$->set_location(infile, @$);
  }
/* | Port
   | ComponentRef -- TemplateInstance covers all the others */
;

ApplyOp:
  Reference DotApplyKeyword '(' optFunctionActualParList ')'
  {
    if($1.is_ref) $$.value = new Value(Value::V_REFD, $1.ref);
    else {
      Ttcn::Reference *t_ref = new Ttcn::Reference($1.id);
      t_ref->set_location(infile, @1);
      $$.value = new Value(Value::V_REFD, t_ref);
    }
    $$.value->set_location(infile, @1);
    $$.ap_list = $4;
    $$.ap_list->set_location(infile, @3 , @5);
  }
  | FunctionInstance DotApplyKeyword '(' optFunctionActualParList ')'
  {
    $$.value = new Value(Value::V_REFD, $1);
    $$.value->set_location(infile, @1);
    $$.ap_list = $4;
    $$.ap_list->set_location(infile, @3 , @5);
  }
  | ApplyOp DotApplyKeyword '(' optFunctionActualParList ')'
  {
    $$.value = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    $$.value->set_location(infile, @1);
    $$.ap_list = $4;
    $$.ap_list->set_location(infile, @3 , @5);
  }
;

DereferOp:
  DerefersKeyword '(' Expression ')' { $$ = $3; }
;

/* A.1.6.1.5 Signature definitions */

SignatureDef: // 187
  SignatureKeyword IDentifier
  '(' optSignatureFormalParList ')' optReturnTypeOrNoBlockKeyword
  optExceptionSpec
  {
    Type *type = new Type(Type::T_SIGNATURE, $4, $6.type, $6.no_block_kw, $7);
    type->set_location(infile, @3, @7);
    $$ = new Ttcn::Def_Type($2, type);
    $$->set_location(infile, @$);
  }
;

optSignatureFormalParList: // [190]
  /* empty */ { $$ = 0; }
| SignatureFormalParList { $$ = $1; }
| error { $$ = 0; }
;

SignatureFormalParList: // 190
  optError SignatureFormalPar
  {
    $$ = new SignatureParamList;
    $$->add_param($2);
  }
| SignatureFormalParList optError ',' optError SignatureFormalPar
  {
    $$ = $1;
    $$->add_param($5);
  }
| SignatureFormalParList optError ',' error { $$ = $1; }
;

SignatureFormalPar: // 191
  Type IDentifier
  {
    $$ = new SignatureParam(SignatureParam::PARAM_IN, $1, $2);
    $$->set_location(infile, @$);
  }
| InParKeyword Type IDentifier
  {
    $$ = new SignatureParam(SignatureParam::PARAM_IN, $2, $3);
    $$->set_location(infile, @$);
  }
| InOutParKeyword Type IDentifier
  {
    $$ = new SignatureParam(SignatureParam::PARAM_INOUT, $2, $3);
    $$->set_location(infile, @$);
  }
| OutParKeyword Type IDentifier
  {
    $$ = new SignatureParam(SignatureParam::PARAM_OUT, $2, $3);
    $$->set_location(infile, @$);
  }
;

optReturnTypeOrNoBlockKeyword:
  /* empty */
  {
    $$.type = NULL;
    $$.no_block_kw = false;
  }
| ReturnKeyword Type
  {
    $$.type = $2;
    $$.no_block_kw = false;
  }
| NoBlockKeyword
  {
    $$.type = NULL;
    $$.no_block_kw = true;
  }
;

optExceptionSpec: // [192]
  /* empty */ { $$ = NULL; }
| ExceptionKeyword '(' error ')' { $$ = NULL; }
| ExceptionKeyword '(' ExceptionTypeList optError ')'
  {
    $$ = $3;
    $$->set_location(infile, @$);
  }
;

ExceptionTypeList: // 194
  optError Type
  {
    $$ = new SignatureExceptions;
    $$->add_type($2);
  }
| ExceptionTypeList optError ',' optError Type
  {
    $$ = $1;
    $$->add_type($5);
  }
| ExceptionTypeList optError ',' error { $$ = $1; }
;

Signature: // 196
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(infile, @$);
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $5);
    $$->set_location(infile, @$);
    delete $3;
  }
;

/* A.1.6.1.6 Testcase definitions */

TestcaseDef: // 197
  TestcaseKeyword IDentifier '(' optTestcaseFormalParList ')'
  ConfigSpec optError StatementBlock
  {
    $4->set_location(infile, @3, @5);
    $$ = new Def_Testcase($2, $4, $6.runsonref, $6.systemref, $8);
    $$->set_location(infile, @$);
  }
;

optTestcaseFormalParList: // [200]
  /* empty */ { $$ = new FormalParList; }
| TestcaseFormalParList { $$ = $1; }
| error { $$ = new FormalParList; }
;

TestcaseFormalParList: // 200
  optError TestcaseFormalPar
  {
    $$ = new FormalParList;
    $$->add_fp($2);
  }
| TestcaseFormalParList optError ',' optError TestcaseFormalPar
  {
    $$ = $1;
    $$->add_fp($5);
  }
| TestcaseFormalParList optError ',' error { $$ = $1; }
;

TestcaseFormalPar: // 201
  FormalValuePar { $$ = $1; }
| FormalTemplatePar { $$ = $1; }
;

ConfigSpec: // 202
  RunsOnSpec optSystemSpec
  {
    $$.runsonref=$1;
    $$.systemref=$2;
  }
;

optSystemSpec: // [203]
  /* empty */ { $$ = 0; }
| SystemKeyword ComponentType { $$ = $2; }
| SystemKeyword error { $$ = 0; }
;

TestcaseInstance: // 205
  ExecuteKeyword '(' TestcaseRef '(' optTestcaseActualParList ')'
  optTestcaseTimerValue optError ')'
  {
    $5->set_location(infile, @4, @6);
    $$.ref_pard = new Ttcn::Ref_pard($3.modid, $3.id, $5);
    $$.ref_pard->set_location(infile, @3, @6);
    $$.derefered_value = 0;
    $$.ap_list = $5;
    $$.value = $7;
  }
| ExecuteKeyword '(' DereferOp '(' optTestcaseActualParList ')'
  optTestcaseTimerValue optError ')'
  {
    $5->set_location(infile, @4, @6);
    $$.ref_pard = 0;
    $$.derefered_value = $3;
    $$.ap_list = $5;
    $$.value = $7;
  }
| ExecuteKeyword '(' error ')'
  {
    $$.ref_pard = 0;
    $$.derefered_value = 0;
    $$.ap_list = 0;
    $$.value = 0;
  }
;

TestcaseRef: // 207
  IDentifier
  {
    $$.modid = NULL;
    $$.id = $1;
  }
| IDentifier '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $3;
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  {
    $$.modid = $1;
    $$.id = $5;
    delete $3;
  }
;

optTestcaseTimerValue:
  /* empty */ { $$ = 0; }
| ',' optError Expression { $$ = $3; }
| ',' error
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
;

optTestcaseActualParList: // [202]
  /* empty */ optError { $$ = new ParsedActualParameters; }
| TestcaseActualParList optError { $$ = $1; }
;

TestcaseActualParList: // 208
UnnamedPart
| NamedPart
| UnnamedPart ',' NamedPart
/* Splitting the NamedPart and UnnamedPart ensures that a named parameter
* followed by an unnamed one causes a syntax error */
{
  /* UnnamedPart becomes the value */
  $$ = $1;
  /* append the elements from NamedPart */
  const size_t n3 = $3->get_nof_nps();
  for (size_t i = 0; i < n3; ++i) {
  	$$->add_np( $3->extract_np_byIndex(i) );
  }
  delete $3;
}

/*
  optError TestcaseActualPar
  {
    $$ = new TemplateInstances;
    $$->add_ti($2);
  }
| TestcaseActualParList optError ',' optError TestcaseActualPar
  {
    $$ = $1;
    $$->add_ti($5);
  }
| TestcaseActualParList optError ',' error { $$ = $1; }
*/
;

/*
TestcaseActualPar:
  TemplateInstance { $$ = $1; }
| NotUsedSymbol
  {
    Template *t = new Template(Template::TEMPLATE_NOTUSED);
    t->set_location(infile, @$);
    $$ = new TemplateInstance(0, 0, t);
    $$->set_location(infile, @$);
  }
;
*/

/* A.1.6.1.7 Altstep definitions */

AltstepDef: // 211
  AltstepKeyword IDentifier '(' optAltstepFormalParList ')' optRunsOnSpec
  optError '{' AltstepLocalDefList AltGuardList optError '}'
  {
    StatementBlock *sb = new StatementBlock;
    for (size_t i = 0; i < $9.nElements; i++) {
      Statement *stmt = new Statement(Statement::S_DEF, $9.elements[i]);
      stmt->set_location(*$9.elements[i]);
      sb->add_stmt(stmt);
    }
    Free($9.elements);
    $4->set_location(infile, @4);
    $$ = new Def_Altstep($2, $4, $6, sb, $10);
    $$->set_location(infile, @$);
  }
;

optAltstepFormalParList: // [214]
  /* empty */ { $$ = new FormalParList; }
| FunctionFormalParList { $$ = $1; }
| error { $$ = new FormalParList; }
;

AltstepLocalDefList: // 215
  /* empty */
  {
    $$.nElements = 0;
    $$.elements = 0;
  }
| AltstepLocalDefList optError AltstepLocalDef optSemiColon
  {
    $$.nElements = $1.nElements + $3.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)));
    memcpy($$.elements + $1.nElements, $3.elements,
           $3.nElements * sizeof(*$$.elements));
    Free($3.elements);
  }
;

AltstepLocalDef: // 216
  VarInstance { $$ = $1; }
| TimerInstance { $$ = $1; }
| ConstDef { $$ = $1; }
| TemplateDef
  {
    $1->set_local();
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $1;
  }
;

AltstepInstance: /* refpard */ // 217
  FunctionRef '(' optFunctionActualParList ')'
  {
    $3->set_location(infile, @2, @4);
    $$ = new Ttcn::Ref_pard($1.modid, $1.id, $3);
    $$->set_location(infile, @$);
  }
;

/* Taken over by FunctionRef
AltstepRef: // 211
  IDentifier
| IDentifier '.' IDentifier
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
;
*/

/* A.1.6.1.8 Import definitions */

ImportDef: // 219
  ImportKeyword ImportFromSpec AllWithExcepts
  {
    $2->set_imptype(ImpMod::I_ALL);
    $2->set_location(infile, @$);

    $$ = $2;
  }
| ImportKeyword ImportFromSpec '{' ImportSpec '}'
  {
    Location loc(infile, @$);
    if ( $4 == ImpMod::I_IMPORTSPEC) {
      loc.warning("Unsupported selective import statement was treated as "
                "`import all'");
    }
    $2->set_imptype($4);
    $2->set_location(infile, @$);

    $$ = $2;
  }
;

AllWithExcepts: // 221
  AllKeyword
| AllKeyword ExceptsDef
  {
    Location loc(infile, @$);
    loc.warning("Unsupported selective import statement was treated as "
                "`import all'");
  }
;

ExceptsDef: // 222
  ExceptKeyword '{' ExceptSpec '}'
;

ExceptSpec: // 224
  /* empty */ optError
| ExceptSpec ExceptElement optSemiColon
;

ExceptElement: // 225
  ExceptGroupSpec
| ExceptTypeDefSpec
| ExceptTemplateSpec
| ExceptConstSpec
| ExceptTestcaseSpec
| ExceptAltstepSpec
| ExceptFunctionSpec
| ExceptSignatureSpec
| ExceptModuleParSpec
;

ExceptGroupSpec: // 226
  GroupKeyword ExceptGroupRefList
| GroupKeyword AllKeyword
;

ExceptTypeDefSpec: // 227
  TypeDefKeyword TypeRefList
| TypeDefKeyword AllKeyword
;

ExceptTemplateSpec: // 228
  TemplateKeyword TemplateRefList
| TemplateKeyword AllKeyword
;

ExceptConstSpec: // 229
  ConstKeyword ConstRefList
| ConstKeyword AllKeyword
;

ExceptTestcaseSpec: // 230
  TestcaseKeyword TestcaseRefList
| TestcaseKeyword AllKeyword
;

ExceptAltstepSpec: // 231
  AltstepKeyword AltstepRefList
| AltstepKeyword AllKeyword
;

ExceptFunctionSpec: // 232
  FunctionKeyword FunctionRefList
| FunctionKeyword AllKeyword
;

ExceptSignatureSpec: // 233
  SignatureKeyword SignatureRefList
| SignatureKeyword AllKeyword
;

ExceptModuleParSpec: // 234
  ModuleParKeyword ModuleParRefList
| ModuleParKeyword AllKeyword
;

ImportSpec: // 235
  /* empty */ optError
  { $$ = ImpMod::I_ALL; }
  | ImportSpec ImportElement optSemiColon
  {
    switch ($$) {
    case ImpMod::I_ALL: // it was empty before
      $$ = $2;
      break;

    case ImpMod::I_IMPORTSPEC:
      switch ($2) {
      case ImpMod::I_IMPORTSPEC:
        // selective import followed by another selective import: NOP
        break;
      case ImpMod::I_IMPORTIMPORT:
        $$ = $2; // import of import wins over selective import
        break;
      default: // including I_ALL
        FATAL_ERROR("Selective import cannot be followed by import all");
      }
      break;

    case ImpMod::I_IMPORTIMPORT:
      switch ($2) {
      case ImpMod::I_IMPORTSPEC:
        // import of import followed by selective import: NOP (import of import wins)
        break;
      case ImpMod::I_IMPORTIMPORT:
        // import of import following another import of import: error
        Location(infile, @2).error("Import of imports can only be used once");
        break;
      default: // including I_ALL
        FATAL_ERROR("Import of imports cannot be followed by import all");
      }
      break;

    default:
      FATAL_ERROR("Invalid import type");
    }
  }
;

ImportElement: // 236
  ImportGroupSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportTypeDefSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportTemplateSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportConstSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportTestcaseSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportAltstepSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportFunctionSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportSignatureSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportModuleParSpec
  { $$ = ImpMod::I_IMPORTSPEC; }
| ImportImportSpec
  { $$ = ImpMod::I_IMPORTIMPORT; }
;

ImportImportSpec:
  ImportKeyword AllKeyword

ImportFromSpec: // 237
  FromKeyword ModuleId { $$ = $2; }
| FromKeyword ModuleId RecursiveKeyword // already deprecated in v3.2.1
  {
    $$ = $2;
    $$->set_recursive();
  }
;

ImportGroupSpec: // 239
  GroupKeyword GroupRefListWithExcept
| GroupKeyword AllGroupsWithExcept
| GroupKeyword error
;

GroupRefList: // 240
  optError FullGroupIdentifier { delete $2; }
| GroupRefList optError ',' optError FullGroupIdentifier { delete $5; }
| GroupRefList optError ',' error
;

GroupRefListWithExcept: // 241
  optError FullGroupIdentifierWithExcept
| GroupRefListWithExcept optError ',' optError FullGroupIdentifierWithExcept
| GroupRefListWithExcept optError ',' error
;

AllGroupsWithExcept: // 242
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword GroupRefList
| AllKeyword ExceptKeyword error
;

FullGroupIdentifier: // 243
  IDentifier
  {
    $$ = new Qualifier();
    $$->add(new FieldOrArrayRef($1));
    $$->set_location(infile, @$);
  }
| FullGroupIdentifier '.' IDentifier
  {
    $$ = $1;
    $$->add(new FieldOrArrayRef($3));
    $$->set_location(infile, @$);
  }
;

FullGroupIdentifierWithExcept: // 244
  FullGroupIdentifier { delete $1; }
| FullGroupIdentifier ExceptsDef { delete $1; }
;

ExceptGroupRefList: // 245
  optError ExceptFullGroupIdentifier
| ExceptGroupRefList optError ',' optError ExceptFullGroupIdentifier
| ExceptGroupRefList optError ',' error
;

ExceptFullGroupIdentifier: // 246
  FullGroupIdentifier { delete $1;}
;

ImportTypeDefSpec: // 247
  TypeDefKeyword TypeRefList
| TypeDefKeyword AllTypesWithExcept
| TypeDefKeyword error
;

TypeRefList: // 248
  optError IDentifier { delete $2; }
| TypeRefList optError ',' optError IDentifier { delete $5; }
| TypeRefList optError ',' error
;

AllTypesWithExcept: // 249
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword TypeRefList
| AllKeyword ExceptKeyword error
;

/* 250. TypeDefIdentifier is replaced by IDentifier */

ImportTemplateSpec: // 251
  TemplateKeyword TemplateRefList
| TemplateKeyword AllTemplsWithExcept
| TemplateKeyword error
;

TemplateRefList: // 252
  optError IDentifier { delete $2; }
| TemplateRefList optError ',' optError IDentifier { delete $5; }
| TemplateRefList optError ',' error
;

AllTemplsWithExcept: // 253
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword TemplateRefList
| AllKeyword ExceptKeyword error
;

ImportConstSpec: // 254
  ConstKeyword ConstRefList
| ConstKeyword AllConstsWithExcept
| ConstKeyword error
;

ConstRefList: // 255
  optError IDentifier { delete $2; }
| ConstRefList optError ',' optError IDentifier { delete $5; }
| ConstRefList optError ',' error
;

AllConstsWithExcept: // 256
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword ConstRefList
| AllKeyword ExceptKeyword error
;

ImportAltstepSpec: // 257
  AltstepKeyword AltstepRefList
| AltstepKeyword AllAltstepsWithExcept
| AltstepKeyword error
;

AltstepRefList: // 258
  optError IDentifier { delete $2; }
| AltstepRefList optError ',' optError IDentifier { delete $5; }
| AltstepRefList optError ',' error
;

AllAltstepsWithExcept: // 259
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword AltstepRefList
| AllKeyword ExceptKeyword error
;

ImportTestcaseSpec: // 260
  TestcaseKeyword TestcaseRefList
| TestcaseKeyword AllTestcasesWithExcept
| TestcaseKeyword error
;

TestcaseRefList: // 261
  optError IDentifier { delete $2; }
| TestcaseRefList optError ',' optError IDentifier { delete $5; }
| TestcaseRefList optError ',' error
;

AllTestcasesWithExcept: // 262
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword TestcaseRefList
| AllKeyword ExceptKeyword error
;

ImportFunctionSpec: // 263
  FunctionKeyword FunctionRefList
| FunctionKeyword AllFunctionsWithExcept
| FunctionKeyword error
;

FunctionRefList: // 264
  optError IDentifier { delete $2; }
| FunctionRefList optError ',' optError IDentifier { delete $5; }
| FunctionRefList optError ',' error
;

AllFunctionsWithExcept: // 265
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword FunctionRefList
| AllKeyword ExceptKeyword error
;

ImportSignatureSpec: // 266
  SignatureKeyword SignatureRefList
| SignatureKeyword AllSignaturesWithExcept
| SignatureKeyword error
;

SignatureRefList: // 267
  optError IDentifier { delete $2; }
| SignatureRefList optError ',' optError IDentifier { delete $5; }
| SignatureRefList optError ',' error
;

AllSignaturesWithExcept: // 268
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword SignatureRefList
| AllKeyword ExceptKeyword error
;

ImportModuleParSpec: // 269
  ModuleParKeyword ModuleParRefList
| ModuleParKeyword AllModuleParWithExcept
| ModuleParKeyword error
;

ModuleParRefList: // 270
  optError IDentifier { delete $2; }
| ModuleParRefList optError ',' optError IDentifier { delete $5; }
| ModuleParRefList optError ',' error
;

AllModuleParWithExcept: // 271
  AllKeyword
| AllKeyword error
| AllKeyword ExceptKeyword ModuleParRefList
| AllKeyword ExceptKeyword error
;

// 272 ImportImportSpec: ImportKeyword AllKeyword

/* A.1.6.1.9 Group definitions */

GroupDef: // 273
  GroupIdentifier '{' optErrorBlock '}'
  {
    $$ = $1;
    $$->set_location(infile, @$);
  }
| GroupIdentifier '{' ModuleDefinitionsList optErrorBlock '}'
  {
    $$ = $1;
    $$->set_location(infile, @$);
  }
;

GroupIdentifier: // 274 (followed by) 275.
  GroupKeyword IDentifier
  {
    $$ = new Group($2);
    $$->set_parent_group(act_group);
    $$->set_location(infile, @$);
    if (act_group) {
      act_group->add_group($$);
      $$->set_parent_path(act_group->get_attrib_path());
    } else {
      act_ttcn3_module->add_group($$);
      $$->set_parent_path(act_ttcn3_module->get_attrib_path());
    }
    act_group = $$;
  }
;

/* A.1.6.1.10 External function definitions */

ExtFunctionDef: // 276
  ExtKeyword FunctionKeyword optDeterministicModifier IDentifier
  '(' optFunctionFormalParList ')' optReturnType
  {
    $6->set_location(infile, @5, @7);
    $$ = new Def_ExtFunction($4, $6, $8.type, $8.returns_template,
                             $8.template_restriction);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.1.11 External constant definitions */

ExtConstDef: // 279
  ExtKeyword ConstKeyword Type IdentifierList
  {
    $$.nElements = $4.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements * sizeof(*$$.elements)));
    for (size_t i = 0; i < $$.nElements; i++) {
      Type *type;
      if (i > 0) {
	type = new Type(Type::T_REFDSPEC, $3);
	type->set_location(*$3);
      } else type = $3;
      $$.elements[i] = new Ttcn::Def_ExtConst($4.elements[i].id, type);
      $$.elements[i]->set_location(infile, $4.elements[i].yyloc);
    }
    Free($4.elements);
  }
;

IdentifierList: // 280 ExtConstIdentifierList
  optError IDentifier
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0].id = $2;
    $$.elements[0].yyloc = @2;
  }
| IdentifierList ',' optError IDentifier
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$1.nElements].id = $4;
    $$.elements[$1.nElements].yyloc = @4;
  }
| IdentifierList ',' error { $$ = $1; }
;

/* A.1.6.1.12 Module parameter definitions */

ModuleParDef: // 282
  ModuleParKeyword ModulePar { $$ = $2; }
| ModuleParKeyword '{' MultiTypedModuleParList optError '}' { $$ = $3; }
| ModuleParKeyword '{' error '}' { $$.nElements = 0; $$.elements = NULL; }
;

MultiTypedModuleParList: // 284
  optError ModulePar optSemiColon { $$ = $2; }
| MultiTypedModuleParList optError ModulePar optSemiColon
  {
    $$.nElements = $1.nElements + $3.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
          Realloc($1.elements, $$.nElements * sizeof(*$$.elements)));
    memcpy($$.elements + $1.nElements, $3.elements,
      $3.nElements * sizeof(*$$.elements));
    Free($3.elements);
  }
;

ModulePar: // 285
  Type ModuleParList
  {
    $$.nElements = $2.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements * sizeof(*$$.elements)));
    for(size_t i = 0; i < $2.nElements; i++) {
      Type *type;
      if (i > 0) {
	type = new Type(Type::T_REFDSPEC, $1);
	type->set_location(*$1);
      } else type = $1;
      $$.elements[i] = new Def_Modulepar($2.elements[i].id, type,
	$2.elements[i].defval);
      $$.elements[i]->set_location(infile, $2.elements[i].yyloc);
    }
    Free($2.elements);
  }
| TemplateKeyword Type TemplateModuleParList
  {
    $$.nElements = $3.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(Malloc($$.nElements * sizeof(*$$.elements)));
    for(size_t i = 0; i < $3.nElements; i++) {
      Type *type;
      if (i > 0) {
        type = new Type(Type::T_REFDSPEC, $2);
        type->set_location(*$2);
      } else type = $2;
      $$.elements[i] = new Def_Modulepar_Template($3.elements[i].id, type, $3.elements[i].deftempl);
      $$.elements[i]->set_location(infile, $3.elements[i].yyloc);
    }
    Free($3.elements);
  }
;

ModuleParList: // 287
  optError SingleModulePar
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::singlemodulepar_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| ModuleParList ',' optError SingleModulePar
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::singlemodulepar_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

TemplateModuleParList: // 287
  optError SingleTemplateModulePar
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::singletemplatemodulepar_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| TemplateModuleParList ',' optError SingleTemplateModulePar
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::singletemplatemodulepar_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

SingleModulePar:
  IDentifier
  {
    $$.id = $1;
    $$.defval = NULL;
    $$.yyloc = @$;
  }
| IDentifier AssignmentChar Expression
  {
    $$.id = $1;
    $$.defval = $3;
    $$.yyloc = @$;
  }
;

SingleTemplateModulePar:
  IDentifier
  {
    $$.id = $1;
    $$.deftempl = NULL;
    $$.yyloc = @$;
  }
| IDentifier AssignmentChar TemplateBody
  {
    $$.id = $1;
    $$.deftempl = $3;
    $$.yyloc = @$;
  }
;

/* A.1.6.1.13 */
FriendModuleDef: // 289
  FriendKeyword TTCN3ModuleKeyword IdentifierList optSemiColon
  {
    $$.nElements = $3.nElements;
    $$.elements = static_cast<Ttcn::FriendMod**>(
      Malloc($$.nElements*sizeof(*$$.elements)) );
    for (size_t i = 0; i < $$.nElements; i++) {
      $$.elements[i] = new FriendMod($3.elements[i].id);
      $$.elements[i]->set_location(infile, $3.elements[i].yyloc);
    }
    Free($3.elements);
  }
;

/* A.1.6.2 Control part */

/* A.1.6.2.0 General */

ModuleControlPart: // 290
  optError ControlKeyword
  '{' ModuleControlBody '}'
  optWithStatementAndSemiColon
  {
    ControlPart* controlpart = new ControlPart($4);
    controlpart->set_location(infile, @2, @6);
    controlpart->set_with_attr($6);
    controlpart->set_parent_path(act_ttcn3_module->get_attrib_path());
    act_ttcn3_module->add_controlpart(controlpart);
  }
;

ModuleControlBody: // 292
  /* empty */ optError { $$=new StatementBlock(); }
| ControlStatementOrDefList { $$ = $1; }
;

ControlStatementOrDefList: // 293
  optError ControlStatementOrDef optSemiColon
  {
    $$=new StatementBlock();
    for(size_t i=0; i<$2.nElements; i++) $$->add_stmt($2.elements[i]);
    Free($2.elements);
  }
| ControlStatementOrDefList optError ControlStatementOrDef optSemiColon
  {
    $$=$1;
    for(size_t i=0; i<$3.nElements; i++) $$->add_stmt($3.elements[i]);
    Free($3.elements);
  }
;

ControlStatementOrDef: // 294
  FunctionLocalDef
  {
    $$.nElements=$1.nElements;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    for(size_t i=0; i<$1.nElements; i++) {
      $$.elements[i]=new Statement(Statement::S_DEF, $1.elements[i]);
      $$.elements[i]->set_location(*$1.elements[i]);
    }
    Free($1.elements);
  }
|  FunctionLocalInst
  {
    $$.nElements=$1.nElements;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    for(size_t i=0; i<$1.nElements; i++) {
      $$.elements[i]=new Statement(Statement::S_DEF, $1.elements[i]);
      $$.elements[i]->set_location(*$1.elements[i]);
    }
    Free($1.elements);
  }
| ControlStatement
  {
    $$.nElements=1;
    $$.elements=static_cast<Statement**>(Malloc($$.nElements*sizeof(*$$.elements)));
    $$.elements[0]=$1;
  }
;

ControlStatement: /* Statement *stmt */ // 295
  TimerStatements { $$ = $1; }
| BasicStatements { $$ = $1; }
| BehaviourStatements { $$ = $1; }
| SUTStatements { $$ = $1; }
| StopExecutionStatement { $$ = $1; }
| ProfilerStatement { $$ = $1; }
| int2enumStatement { $$ = $1; }
| UpdateStatement { $$ = $1; }
;

/* A.1.6.2.1 Variable instantiation */

VarInstance: // 296
  VarKeyword optLazyOrFuzzyModifier Type VarList
  {
    $$.nElements = $4.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements*sizeof(*$$.elements)));
    for (size_t i = 0; i < $$.nElements; i++) {
      Type *type;
      if (i > 0) {
	type = new Type(Type::T_REFDSPEC, $3);
	type->set_location(*$3);
      } else type = $3;
      /* creation of array type(s) if necessary (from right to left) */
      for (size_t j = $4.elements[i].arrays.nElements; j > 0; j--) {
        type = new Type(Type::T_ARRAY, type,
	  $4.elements[i].arrays.elements[j - 1], false);
        type->set_location(*$3);
      }
      Free($4.elements[i].arrays.elements);

      /* Create the definition */
      $$.elements[i] = new Def_Var($4.elements[i].id,
                                   type, $4.elements[i].initial_value);
      $$.elements[i]->set_location(infile, $4.elements[i].yyloc);
    }
    Free($4.elements);
  }
| VarKeyword TemplateOptRestricted optLazyOrFuzzyModifier Type TempVarList
  {
    $$.nElements = $5.nElements;
    $$.elements = static_cast<Ttcn::Definition**>(
      Malloc($$.nElements * sizeof(*$$.elements)));
    for (size_t i = 0; i < $$.nElements; i++) {
      Type *type;
      if (i > 0) {
	type = new Type(Type::T_REFDSPEC, $4);
	type->set_location(*$4);
      } else type = $4;
      /* creation of array type(s) if necessary (from right to left) */
      for (size_t j = $5.elements[i].arrays.nElements; j > 0; j--) {
        type = new Type(Type::T_ARRAY, type,
	  $5.elements[i].arrays.elements[j - 1], false);
        type->set_location(*$4);
      }
      Free($5.elements[i].arrays.elements);

      /* Create the definition */
      $$.elements[i] = new Def_Var_Template($5.elements[i].id, type,
                                            $5.elements[i].initial_value, $2);
      $$.elements[i]->set_location(infile, $5.elements[i].yyloc);
    }

    Free($5.elements);
}
;

VarList: // 297
  optError SingleVarInstance
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::singlevarinst_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| VarList ',' optError SingleVarInstance
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::singlevarinst_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

SingleVarInstance: // 298
  IDentifier optArrayDef
  {
    $$.id = $1;
    $$.arrays = $2;
    $$.initial_value = 0;
    $$.yyloc = @$;
  }
| IDentifier optArrayDef AssignmentChar Expression
  {
    $$.id = $1;
    $$.arrays = $2;
    $$.initial_value = $4;
    $$.yyloc = @$;
  }
;

TempVarList: // 302
  optError SingleTempVarInstance
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::singletempvarinst_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| TempVarList ',' optError SingleTempVarInstance
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::singletempvarinst_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$$.nElements - 1] = $4;
  }
;

SingleTempVarInstance: // 303
  IDentifier optArrayDef
  {
    $$.id = $1;
    $$.arrays = $2;
    $$.initial_value = NULL;
    $$.yyloc = @$;
  }
| IDentifier optArrayDef AssignmentChar TemplateBody
  {
    $$.id = $1;
    $$.arrays = $2;
    $$.initial_value = $4;
    $$.yyloc = @$;
  }
;

VariableRef: // 305
  Reference
  {
    if ($1.is_ref) $$ = $1.ref;
    else {
      $$ = new Ttcn::Reference($1.id);
      $$->set_location(infile, @$);
    }
  }
;

/* A.1.6.2.2 Timer instantiation */

TimerInstance: // 306
  TimerKeyword TimerList { $$ = $2; }
;

TimerList: // 307
  optError SingleTimerInstance
  {
    $$.nElements = 1;
    $$.elements = static_cast<Ttcn::Definition**>(Malloc(sizeof(*$$.elements)));
    $$.elements[0] = $2;
  }
| TimerList ',' optError SingleTimerInstance
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<Ttcn::Definition**>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)));
    $$.elements[$$.nElements - 1] = $4;
  }
;

SingleTimerInstance: // 308
  IDentifier optArrayDef
  {
    ArrayDimensions *dims;
    if ($2.nElements > 0) {
      dims = new ArrayDimensions;
      for (size_t i = 0; i < $2.nElements; i++) dims->add($2.elements[i]);
      Free($2.elements);
    } else dims = 0;
    $$ = new Ttcn::Def_Timer($1, dims, 0);
    $$->set_location(infile, @$);
  }
| IDentifier optArrayDef AssignmentChar TimerValue
  {
    ArrayDimensions *dims;
    if ($2.nElements > 0) {
      dims = new ArrayDimensions;
      for (size_t i = 0; i < $2.nElements; i++) dims->add($2.elements[i]);
      Free($2.elements);
    } else dims = 0;
    $$ = new Ttcn::Def_Timer($1, dims, $4);
    $$->set_location(infile, @$);
  }
;

TimerValue: // 311
  Expression { $$ = $1; }
;

TimerRef: // 312
  VariableRef { $$ = $1; }
;

/* A.1.6.2.3 Component operations */

ConfigurationStatements: // 313
  ConnectStatement { $$ = $1; }
| MapStatement { $$ = $1; }
| DisconnectStatement { $$ = $1; }
| UnmapStatement { $$ = $1; }
| DoneStatement { $$ = $1; }
| KilledStatement { $$ = $1; }
| StartTCStatement { $$ = $1; }
| StopTCStatement { $$ = $1; }
| KillTCStatement { $$ = $1; }
;

ConfigurationOps: // 314
  CreateOp {$$=$1;}
| SelfOp {$$=$1;}
| SystemOp {$$=$1;}
| MTCOp {$$=$1;}
| RunningOp {$$=$1;}
| AliveOp { $$ = $1; }
;

CreateOp: // 315
  VariableRef DotCreateKeyword optCreateParameter optAliveKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_CREATE, $1, $3.name, $3.loc, $4);
    $$->set_location(infile, @$);
  }
;

optCreateParameter:
  /* empty */
  {
    $$.name=0;
    $$.loc=0;
  }
| '(' optError Expression optError ')'
  {
    $$.name = $3;
    $$.loc = 0;
  }
| '(' optError Expression optError ',' optError Expression optError ')'
  {
    $$.name = $3;
    $$.loc = $7;
  }
| '(' optError NotUsedSymbol optError ',' optError Expression optError ')'
  {
    $$.name = 0;
    $$.loc = $7;
  }
| '(' error ')'
  {
    $$.name = 0;
    $$.loc = 0;
  }
;

optAliveKeyword: // [328]
  /* empty */ { $$ = false; }
| AliveKeyword { $$ = true; }
;

SystemOp: // 316
  SystemKeyword
  {
    $$=new Value(Value::OPTYPE_COMP_SYSTEM);
    $$->set_location(infile, @$);
  }
;

SelfOp: // 317
  SelfKeyword
  {
    $$=new Value(Value::OPTYPE_COMP_SELF);
    $$->set_location(infile, @$);
  }
;

MTCOp: // 318
  MTCKeyword
  {
    $$=new Value(Value::OPTYPE_COMP_MTC);
    $$->set_location(infile, @$);
  }
;

DoneStatement: // 319
  ComponentOrDefaultReference DotDoneKeyword optDoneParameter
  {
    $$ = new Statement(Statement::S_DONE, $1, $3.donematch, $3.valueredirect,
      $3.indexredirect, false);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ComponentOrDefaultReference DotDoneKeyword optDoneParameter
  {
    $$ = new Statement(Statement::S_DONE, $3, $5.donematch, $5.valueredirect,
      $5.indexredirect, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword ComponentKeyword DotDoneKeyword
  {
    $$ = new Statement(Statement::S_DONE, Statement::C_ANY);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotDoneKeyword
  {
    $$ = new Statement(Statement::S_DONE, Statement::C_ALL);
    $$->set_location(infile, @$);
  }
;

optDoneParameter:
  optReceiveParameter
  {
    $$.donematch = $1;
    $$.valueredirect = 0;
    $$.indexredirect = 0;
  }
| optReceiveParameter PortRedirectSymbol IndexSpec
  {
    $$.donematch = $1;
    $$.valueredirect = 0;
    $$.indexredirect = $3;
  }
| optReceiveParameter PortRedirectSymbol ValueSpec
  {
    $$.donematch = $1;
    $$.valueredirect = $3;
    $$.indexredirect = 0;
  }
| optReceiveParameter PortRedirectSymbol ValueSpec IndexSpec
  {
    $$.donematch = $1;
    $$.valueredirect = $3;
    $$.indexredirect = $4;
  }
;

KilledStatement: // 320
  ComponentOrDefaultReference DotKilledKeyword
  {
    $$ = new Statement(Statement::S_KILLED, $1, false, NULL);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ComponentOrDefaultReference DotKilledKeyword
  {
    $$ = new Statement(Statement::S_KILLED, $3, true, NULL);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ComponentOrDefaultReference DotKilledKeyword
  PortRedirectSymbol IndexSpec
  {
    $$ = new Statement(Statement::S_KILLED, $3, true, $6);
    $$->set_location(infile, @$);
  }
| AnyKeyword ComponentKeyword DotKilledKeyword
  {
    $$ = new Statement(Statement::S_KILLED, Statement::C_ANY);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotKilledKeyword
  {
    $$ = new Statement(Statement::S_KILLED, Statement::C_ALL);
    $$->set_location(infile, @$);
  }
;

/*
ComponentId: // 321
  ComponentOrDefaultReference
| AnyKeyword ComponentKeyword
| AllKeyword ComponentKeyword
;
*/

RunningOp: // 324
/*  VariableRef DotRunningKeyword -- covered by RunningTimerOp */
  FunctionInstance DotRunningKeyword
  {
    Value *t_val = new Value(Value::V_REFD, $1);
    t_val->set_location(infile, @1);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, NULL, false);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword FunctionInstance DotRunningKeyword
  {
    Value *t_val = new Value(Value::V_REFD, $3);
    t_val->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, NULL, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword FunctionInstance DotRunningKeyword
  PortRedirectSymbol IndexSpec
  {
    Value *t_val = new Value(Value::V_REFD, $3);
    t_val->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, $6, true);
    $$->set_location(infile, @$);
  }
| ApplyOp DotRunningKeyword
  {
    Value *t_val = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    t_val->set_location(infile, @1);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, NULL, false);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ApplyOp DotRunningKeyword
  {
    Value *t_val = new Value(Value::V_INVOKE, $3.value, $3.ap_list);
    t_val->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, NULL, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ApplyOp DotRunningKeyword PortRedirectSymbol IndexSpec
  {
    Value *t_val = new Value(Value::V_INVOKE, $3.value, $3.ap_list);
    t_val->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_COMP_RUNNING, t_val, $6, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword ComponentKeyword DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_RUNNING_ANY);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_RUNNING_ALL);
    $$->set_location(infile, @$);
  }
;

AliveOp: // 326
  ComponentOrDefaultReference DotAliveKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_ALIVE, $1, NULL, false);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ComponentOrDefaultReference DotAliveKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_ALIVE, $3, NULL, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword ComponentOrDefaultReference DotAliveKeyword
  PortRedirectSymbol IndexSpec
  {
    $$ = new Value(Value::OPTYPE_COMP_ALIVE, $3, $6, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword ComponentKeyword DotAliveKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_ALIVE_ANY);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotAliveKeyword
  {
    $$ = new Value(Value::OPTYPE_COMP_ALIVE_ALL);
    $$->set_location(infile, @$);
  }
;

ConnectStatement: // 329
  ConnectKeyword SingleConnectionSpec
  {
    $$=new Statement(Statement::S_CONNECT,
                     $2.compref1, $2.portref1, $2.compref2, $2.portref2);
    $$->set_location(infile, @$);
  }
;

SingleConnectionSpec: // 331
  '(' PortRef optError ',' optError PortRef optError ')'
  {
    $$.compref1 = $2.compref;
    $$.portref1 = $2.portref;
    $$.compref2 = $6.compref;
    $$.portref2 = $6.portref;
  }
;

PortRef: // 332
  ComponentRef ':' Port
  {
    $$.compref = $1;
    $$.portref = $3;
  }
;

ComponentRef: // 333
  ComponentOrDefaultReference { $$ = $1; }
| SystemOp { $$ = $1; }
| SelfOp { $$ = $1; }
| MTCOp { $$ = $1; }
;

DisconnectStatement: // 335
  DisconnectKeyword
  {
    Location loc(infile, @$);
    loc.error("Disconnect operation on multiple connections is "
      "not currently supported");
    $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
  }
| DisconnectKeyword SingleOrMultiConnectionSpec
  {
    if ($2.portref1 && $2.portref2 && $2.compref1 && $2.compref2) {
      $$ = new Statement(Statement::S_DISCONNECT,
	$2.compref1, $2.portref1, $2.compref2, $2.portref2);
    } else {
      Location loc(infile, @$);
      loc.error("Disconnect operation on multiple connections is "
	"not currently supported");
      delete $2.compref1;
      delete $2.portref1;
      delete $2.compref2;
      delete $2.portref2;
      $$ = new Statement(Statement::S_ERROR);
    }
    $$->set_location(infile, @$);
  }
;

SingleOrMultiConnectionSpec: // 336
  SingleConnectionSpec { $$ = $1; }
| AllConnectionsSpec
  {
    $$.compref1 = $1.compref;
    $$.portref1 = $1.portref;
    $$.compref2 = 0;
    $$.portref2 = 0;
  }
| AllPortsSpec
  {
    $$.compref1 = $1;
    $$.portref1 = 0;
    $$.compref2 = 0;
    $$.portref2 = 0;
  }
| AllCompsAllPortsSpec
  {
    $$.compref1 = 0;
    $$.portref1 = 0;
    $$.compref2 = 0;
    $$.portref2 = 0;
  }
;

AllConnectionsSpec: // 337
  '(' PortRef optError ')' { $$ = $2; }
;

AllPortsSpec: // 338
  '(' ComponentRef ':' AllKeyword PortKeyword optError ')' { $$ = $2; }
;

AllCompsAllPortsSpec: // 339
  '(' AllKeyword ComponentKeyword ':' AllKeyword PortKeyword optError ')'
;

MapStatement: // 341
  MapKeyword SingleConnectionSpec
  {
    $$=new Statement(Statement::S_MAP,
                     $2.compref1, $2.portref1,
                     $2.compref2, $2.portref2);
    $$->set_location(infile, @$);
  }
;

UnmapStatement: // 343
  UnmapKeyword
  {
    $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
    Location loc(infile, @$);
    loc.error("Unmap operation on multiple mappings is "
      "not currently supported");
  }
| UnmapKeyword SingleOrMultiConnectionSpec
  {
    if ($2.compref1 && $2.portref1 && $2.compref1 && $2.compref2) {
      $$ = new Statement(Statement::S_UNMAP,
	$2.compref1, $2.portref1, $2.compref2, $2.portref2);
    } else {
      Location loc(infile, @$);
      loc.error("Unmap operation on multiple mappings is "
	"not currently supported");
      delete $2.compref1;
      delete $2.portref1;
      delete $2.compref2;
      delete $2.portref2;
      $$ = new Statement(Statement::S_ERROR);
    }
    $$->set_location(infile, @$);
  }
;

StartTCStatement: // 345
/* VariableRef DotStartKeyword '(' FunctionInstance ')'
  -- covered by StartTimerStatement */
  VariableRef DotStartKeyword '(' DereferOp '(' optFunctionActualParList ')'
  optError ')'
  {
    Value *t_val = new Value(Value::V_REFD, $1);
    t_val->set_location(infile, @1);
    $6->set_location(infile, @5, @7);
    //ParsedActualParameters *pap = new ParsedActualParameters($6);
    $$ = new Statement(Statement::S_START_COMP_REFD, t_val, $4, $6);
    $$->set_location(infile, @$);
  }
| FunctionInstance DotStartKeyword '(' FunctionInstance optError ')'
  {
    Value *t_val = new Value(Value::V_REFD, $1);
    t_val->set_location(infile, @1);
    $$ = new Statement(Statement::S_START_COMP, t_val, $4);
    $$->set_location(infile, @$);
  }
| FunctionInstance DotStartKeyword '(' DereferOp '('
    optFunctionActualParList ')' optError ')'
  {
    Value *t_val = new Value(Value::V_REFD, $1);
    t_val->set_location(infile, @1);
    $6->set_location(infile, @5 , @7);
    $$ = new Statement(Statement::S_START_COMP_REFD, t_val, $4, $6);
    $$->set_location(infile, @$);
  }
| FunctionInstance DotStartKeyword '(' error ')'
  {
    delete $1;
    $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
  }
| ApplyOp DotStartKeyword '(' FunctionInstance ')'
  {
    Value *t_val = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    t_val->set_location(infile, @1);
    $$ = new Statement(Statement::S_START_COMP, t_val, $4);
    $$->set_location(infile, @$);
  }
| ApplyOp DotStartKeyword '(' DereferOp '(' optFunctionActualParList ')'
  optError ')'
  {
    Value *t_val = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    t_val->set_location(infile, @1);
    $6->set_location(infile, @5 , @7);
    $$ = new Statement(Statement::S_START_COMP_REFD, t_val, $4, $6);
    $$->set_location(infile, @$);
  }
| ApplyOp DotStartKeyword '(' error ')'
  {
    delete $1.value;
    delete $1.ap_list;
    $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
  }
;

StopTCStatement: // 337
/* VariableRef DotStopKeyword -- covered by StopTimerStatement */
  FunctionInstance DotStopKeyword
  {
    Value *t_val = new Value(Value::V_REFD, $1);
    t_val->set_location(infile, @1);
    $$ = new Statement(Statement::S_STOP_COMP, t_val);
    $$->set_location(infile, @$);
  }
| ApplyOp DotStopKeyword
  {
    Value *t_val = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    t_val->set_location(infile, @1);
    $$ = new Statement(Statement::S_STOP_COMP, t_val);
    $$->set_location(infile, @$);
  }
| MTCOp DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_COMP, $1);
    $$->set_location(infile, @$);
  }
| SelfOp DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_COMP, $1);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_COMP, static_cast<Value*>(0));
    $$->set_location(infile, @$);
  }
;

ComponentReferenceOrLiteral: // 348
  ComponentOrDefaultReference { $$ = $1; }
| MTCOp { $$ = $1; }
| SelfOp { $$ = $1; }
;

KillTCStatement: // 349
  KillKeyword
  {
    Value *self = new Value(Value::OPTYPE_COMP_SELF);
    self->set_location(infile, @1);
    $$ = new Statement(Statement::S_KILL, self);
    $$->set_location(infile, @$);
  }
| ComponentReferenceOrLiteral DotKillKeyword
  {
    $$ = new Statement(Statement::S_KILL, $1);
    $$->set_location(infile, @$);
  }
| AllKeyword ComponentKeyword DotKillKeyword
  {
    $$ = new Statement(Statement::S_KILL, static_cast<Value*>(0));
    $$->set_location(infile, @$);
  }
;

ComponentOrDefaultReference: // 350
  VariableRef
  {
    $$ = new Value(Value::V_REFD, $1);
    $$->set_location(infile, @$);
  }
| FunctionInstance
  {
    $$ = new Value(Value::V_REFD, $1);
    $$->set_location(infile, @$);
  }
| ApplyOp
  {
    $$ = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.2.4 Port operations */

Port: // 325
  VariableRef { $$ = $1; }
;

CommunicationStatements: // 353
  SendStatement {$$ = $1;}
| CallStatement {$$ = $1;}
| ReplyStatement {$$ = $1;}
| RaiseStatement {$$ = $1;}
| ReceiveStatement {$$ = $1;}
| TriggerStatement {$$ = $1;}
| GetCallStatement {$$ = $1;}
| GetReplyStatement {$$ = $1;}
| CatchStatement {$$ = $1;}
| CheckStatement {$$ = $1;}
| ClearStatement {$$ = $1;}
| StartStatement {$$ = $1;}
| StopStatement {$$ = $1;}
| HaltStatement {$$ = $1;}
;

SendStatement: // 354
  Port DotSendOpKeyword PortSendOp
  {
    $$ = new Statement(Statement::S_SEND, $1, $3.templ_inst, $3.val);
    $$->set_location(infile, @$);
  }
;

PortSendOp: // 355
  '(' SendParameter optError ')' optToClause
  {
    $$.templ_inst = $2;
    $$.val = $5;
  }
| '(' error ')' optToClause
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @2);
    $$.templ_inst = new TemplateInstance(0, 0, t);
    $$.templ_inst->set_location(infile, @2);
    $$.val = $4;
  }
;

SendParameter: // 357
  TemplateInstance { $$=$1; }
;

optToClause: // [3583]
  /* empty */ { $$ = 0; }
| ToKeyword AddressRef
  {
    Template *templ = $2->get_Template();
    if (!$2->get_Type() && !$2->get_DerivedRef() && templ->is_Value()) {
      $$ = templ->get_Value();
    } else {
      Location loc(infile, @$);
      loc.error("Multicast communication is not currently supported");
      $$ = 0;
    }
    delete $2;
  }
/* | ToKeyword AddressRefList -- covered by the previous rule
   (as ValueOrAttribList) */
| ToKeyword AllKeyword ComponentKeyword
  {
    Location loc(infile, @$);
    loc.error("Broadcast communication is not currently supported");
    $$ = 0;
  }
| ToKeyword error { $$ = 0; }
;

/*
AddressRefList: // 359
  '(' seqAddressRef ')'
;

seqAddressRef:
  AddressRef
| seqAddressRef ',' AddressRef
;
*/

AddressRef: // 361
  TemplateInstance { $$ = $1; }
;

CallStatement: // 362
  Port DotCallOpKeyword PortCallOp optPortCallBody
  {
    $$ = new Statement(Statement::S_CALL, $1, $3.templ_inst,
                       $3.calltimerval, $3.nowait, $3.val, $4);
    $$->set_location(infile, @$);
  }
;

PortCallOp: // 363
  '(' CallParameters optError ')' optToClause
  {
    $$.templ_inst = $2.templ_inst;
    $$.calltimerval = $2.calltimerval;
    $$.nowait = $2.nowait;
    $$.val = $5;
  }
| '(' error ')' optToClause
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @2);
    $$.templ_inst = new TemplateInstance(0, 0, t);
    $$.templ_inst->set_location(infile, @2);
    $$.calltimerval = 0;
    $$.nowait = false;
    $$.val = $4;
  }
;

CallParameters: // 365
  TemplateInstance
  {
    $$.templ_inst=$1;
    $$.calltimerval=0;
    $$.nowait=false;
  }
| TemplateInstance ',' optError CallTimerValue
  {
    $$.templ_inst=$1;
    $$.calltimerval=$4.calltimerval;
    $$.nowait=$4.nowait;
  }
;

CallTimerValue: // 366
  TimerValue
  {
    $$.calltimerval=$1;
    $$.nowait=false;
  }
| NowaitKeyword
  {
    $$.calltimerval=0;
    $$.nowait=true;
  }
;

optPortCallBody: // [368]
  /* empty */ { $$=0; }
| '{' CallBodyStatementList optError '}' { $$=$2; }
| '{' error '}' { $$ = new AltGuards; }
;

CallBodyStatementList: // 369
  optError CallBodyStatement
  {
    $$=new AltGuards();
    $$->add_ag($2);
  }
| CallBodyStatementList optError CallBodyStatement
  {
    $$=$1;
    $$->add_ag($3);
  }
;

CallBodyStatement: // 370 and 371. rolled into one.
  AltGuardChar CallBodyOps ';' // This alternative is a TITAN extension
  {
    $$=new AltGuard($1, $2, new StatementBlock());
    $$->set_location(infile, @$);
  }
| AltGuardChar CallBodyOps optSemiColon StatementBlock optSemiColon
  {
    $$=new AltGuard($1, $2, $4);
    $$->set_location(infile, @$);
  }
;

CallBodyOps: // 372
  GetReplyStatement {$$=$1;}
| CatchStatement {$$=$1;}
;

ReplyStatement: // 373
  Port DotReplyKeyword PortReplyOp
  {
    $$ = new Statement(Statement::S_REPLY, $1, $3.templ_inst,
                       $3.replyval, $3.toclause);
    $$->set_location(infile, @$);
  }
;

PortReplyOp: // 374
  '(' TemplateInstance optReplyValue optError ')' optToClause
  {
    $$.templ_inst = $2;
    $$.replyval = $3;
    $$.toclause = $6;
  }
| '(' error ')' optToClause
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @2);
    $$.templ_inst = new TemplateInstance(0, 0, t);
    $$.templ_inst->set_location(infile, @2);
    $$.replyval = 0;
    $$.toclause = $4;
  }
;

optReplyValue: // [376]
  /* empty */ { $$=0; }
| ValueKeyword Expression { $$=$2; }
| ValueKeyword error { $$ = 0; }
;

RaiseStatement: // 377
  Port DotRaiseKeyword PortRaiseOp
  {
    if ($3.signature) $$ = new Statement(Statement::S_RAISE, $1,
      $3.signature, $3.templ_inst, $3.toclause);
    else {
      $$ = new Statement(Statement::S_ERROR);
      delete $1;
      delete $3.signature;
      delete $3.templ_inst;
      delete $3.toclause;
    }
    $$->set_location(infile, @$);
  }
;

PortRaiseOp: // 378
  '(' Signature optError ',' optError TemplateInstance optError ')' optToClause
  {
    $$.signature = $2;
    $$.templ_inst = $6;
    $$.toclause = $9;
  }
| '(' error ')' optToClause
  {
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.toclause = 0;
    delete $4;
  }
;

ReceiveStatement: // 380
  PortOrAny DotReceiveOpKeyword PortReceiveOp
  {
    $$ = new Statement(Statement::S_RECEIVE, $1.reference, $1.any_from,
                       $3.templ_inst, $3.fromclause, $3.redirectval,
                       $3.redirectsender, $3.redirectindex);
    $$->set_location(infile, @$);
  }
;

PortOrAny: // 381
  Port { $$.reference = $1; $$.any_from = false; }
| AnyKeyword PortKeyword { $$.reference = 0; $$.any_from = false; }
| AnyKeyword FromKeyword Port { $$.reference = $3; $$.any_from = true; }
;

PortReceiveOp: // 382
  optReceiveParameter optFromClause optPortRedirect
  {
    $$.templ_inst = $1;
    $$.fromclause = $2;
    $$.redirectval = $3.redirectval;
    $$.redirectsender = $3.redirectsender;
    $$.redirectindex = $3.redirectindex;
  }
;

optReceiveParameter: // [384]
  /* empty */ { $$ = 0; }
| '(' ReceiveParameter optError ')' { $$ = $2; }
| '(' error ')'
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @2);
    $$ = new TemplateInstance(0, 0, t);
    $$->set_location(infile, @$);
  }
;

ReceiveParameter: // 384
  TemplateInstance { $$ = $1; }
;

optFromClause: // [385]
  /* empty */ { $$=0; }
| FromClause { $$=$1; }
;

FromClause: // 385
  FromKeyword AddressRef { $$=$2; }
| FromKeyword error { $$ = 0; }
;

optPortRedirect: // [387]
  /* empty */
  {
    $$.redirectval=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec
  {
    $$.redirectval=$2;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol SenderSpec
  {
    $$.redirectval=0;
    $$.redirectsender=$2;
    $$.redirectindex=0;
  }
| PortRedirectSymbol IndexSpec
  {
    $$.redirectval=0;
    $$.redirectsender=0;
    $$.redirectindex=$2;
  }
| PortRedirectSymbol ValueSpec SenderSpec
  {
    $$.redirectval=$2;
    $$.redirectsender=$3;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectsender=0;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol SenderSpec IndexSpec
  {
    $$.redirectval=0;
    $$.redirectsender=$2;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol ValueSpec SenderSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectsender=$3;
    $$.redirectindex=$4;
  }
| PortRedirectSymbol error
  {
    $$.redirectval=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
;

ValueSpec: // 389
  ValueStoreSpec 
  {
    $$ = new ValueRedirect();
    SingleValueRedirect* p = new SingleValueRedirect($1);
    p->set_location(infile, @$);
    $$->add(p);
    $$->set_location(infile, @$);
  }
| ValueKeyword '(' SingleValueSpecList ')'
  {
    $$ = new ValueRedirect();
    for (size_t i = 0; i < $3.nElements; ++i) {
      $$->add($3.elements[i]);
    }
    Free($3.elements);
    $$->set_location(infile, @$);
  }
;

ValueStoreSpec:
  ValueKeyword VariableRef { $$ = $2; }
| ValueKeyword error { $$ = 0; }
;

SingleValueSpecList:
  SingleValueSpec
  {
    $$.nElements = 1;
    $$.elements = static_cast<SingleValueRedirect**>(Malloc(sizeof(SingleValueRedirect*)) );
    $$.elements[0] = $1;
  }
| SingleValueSpecList ',' SingleValueSpec
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<SingleValueRedirect**>(Realloc($1.elements,
      $$.nElements * sizeof(SingleValueRedirect*)) );
    $$.elements[$$.nElements - 1] = $3;
  }
;

SingleValueSpec:
  VariableRef
  {
    $$ = new SingleValueRedirect($1);
    $$->set_location(infile, @$);
  }
| VariableRef AssignmentChar optDecodedModifier PredefOrIdentifier
  optExtendedFieldReference
  {
    FieldOrArrayRef* field_ref = new FieldOrArrayRef($4);
    field_ref->set_location(infile, @4);
    FieldOrArrayRefs* subrefs = new FieldOrArrayRefs;
    subrefs->add(field_ref);
    for (size_t i = 0; i < $5.nElements; ++i) {
      subrefs->add($5.elements[i]);
    }
    Free($5.elements);
    $$ = new SingleValueRedirect($1, subrefs, $3.is_decoded, $3.string_encoding);
    $$->set_location(infile, @$);
  }
;

SenderSpec: // 391
  SenderKeyword VariableRef { $$ = $2; }
| SenderKeyword error { $$ = 0; }
;

IndexSpec:
  IndexKeyword ValueStoreSpec { $$ = $2; }
;

TriggerStatement: // 393
  PortOrAny DotTriggerOpKeyword PortTriggerOp
  {
    $$ = new Statement(Statement::S_TRIGGER, $1.reference, $1.any_from,
                       $3.templ_inst, $3.fromclause, $3.redirectval,
                       $3.redirectsender, $3.redirectindex);
    $$->set_location(infile, @$);
  }
;

PortTriggerOp: // 394
  optReceiveParameter optFromClause optPortRedirect
  {
    $$.templ_inst = $1;
    $$.fromclause = $2;
    $$.redirectval = $3.redirectval;
    $$.redirectsender = $3.redirectsender;
    $$.redirectindex = $3.redirectindex;
  }
;

GetCallStatement: // 396
  PortOrAny DotGetCallOpKeyword PortGetCallOp
  {
    $$ = new Statement(Statement::S_GETCALL, $1.reference, $1.any_from,
                       $3.templ_inst, $3.fromclause, $3.redirectparam,
                       $3.redirectsender, $3.redirectindex);
    $$->set_location(infile, @$);
  }
;

PortGetCallOp: // 397
  optReceiveParameter optFromClause optPortRedirectWithParam
  {
    $$.templ_inst = $1;
    $$.fromclause = $2;
    $$.redirectparam = $3.redirectparam;
    $$.redirectsender = $3.redirectsender;
    $$.redirectindex = $3.redirectindex;
  }
;

optPortRedirectWithParam: // [399]
  /* empty */
  {
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ParamSpec
  {
    $$.redirectparam=$2;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol SenderSpec
  {
    $$.redirectparam=0;
    $$.redirectsender=$2;
    $$.redirectindex=0;
  }
| PortRedirectSymbol IndexSpec
  {
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=$2;
  }
| PortRedirectSymbol ParamSpec SenderSpec
  {
    $$.redirectparam=$2;
    $$.redirectsender=$3;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ParamSpec IndexSpec
  {
    $$.redirectparam=$2;
    $$.redirectsender=0;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol SenderSpec IndexSpec
  {
    $$.redirectparam=0;
    $$.redirectsender=$2;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol ParamSpec SenderSpec IndexSpec
  {
    $$.redirectparam=$2;
    $$.redirectsender=$3;
    $$.redirectindex=$4;
  }
| PortRedirectSymbol error
  {
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
;

ParamSpec: // 401
  ParamKeyword ParamAssignmentList { $$ = $2; }
| ParamKeyword error { $$ = 0; }
;

ParamAssignmentList: // 403
  '(' AssignmentList optError ')'
  {
    $$=new ParamRedirect($2);
    $$->set_location(infile, @$);
  }
| '(' VariableList optError ')'
  {
    $$=new ParamRedirect($2);
    $$->set_location(infile, @$);
  }
| '(' error ')'
  {
    $$=new ParamRedirect(new ParamAssignments());
    $$->set_location(infile, @$);
  }
;

AssignmentList: // 404
  VariableAssignment
  {
    $$ = new ParamAssignments();
    $$->add_parass($1);
  }
| error VariableAssignment
  {
    $$ = new ParamAssignments();
    $$->add_parass($2);
  }
| AssignmentList optError ',' optError VariableAssignment
  {
    $$ = $1;
    $$->add_parass($5);
  }
| AssignmentList optError ',' error { $$ = $1; }
;

VariableAssignment: // 405
  VariableRef AssignmentChar optDecodedModifier IDentifier
  {
    $$ = new ParamAssignment($4, $1, $3.is_decoded, $3.string_encoding);
    $$->set_location(infile, @$);
  }
;

optDecodedModifier:
  /* empty */
  {
    $$.is_decoded = false;
    $$.string_encoding = NULL;
  }
| DecodedKeyword
  {
    $$.is_decoded = true;
    $$.string_encoding = NULL;
  }
| DecodedKeyword '(' SingleExpression ')'
  {
    $$.is_decoded = true;
    $$.string_encoding = $3;
  }
;

VariableList: // 407
  VariableEntry
  {
    $$ = new VariableEntries();
    $$->add_ve($1);
  }
| error VariableEntry
  {
    $$ = new VariableEntries();
    $$->add_ve($2);
  }
| VariableList optError ',' optError VariableEntry
  {
    $$ = $1;
    $$->add_ve($5);
  }
| VariableList optError ',' error { $$ = $1; }
;

VariableEntry: // 408
  VariableRef
  {
    $$ = new VariableEntry($1);
    $$->set_location(infile, @$);
  }
| NotUsedSymbol
  {
    $$ = new VariableEntry;
    $$->set_location(infile, @$);
  }
;

GetReplyStatement: // 409
  PortOrAny DotGetReplyOpKeyword PortGetReplyOp
  {
    $$ = new Statement(Statement::S_GETREPLY, $1.reference, $1.any_from,
                       $3.templ_inst, $3.valuematch, $3.fromclause,
                       $3.redirectval, $3.redirectparam, $3.redirectsender,
                       $3.redirectindex);
    $$->set_location(infile, @$);
  }
;

PortGetReplyOp: // 410
  optGetReplyParameter optFromClause optPortRedirectWithValueAndParam
  {
    $$.templ_inst = $1.templ_inst;
    $$.valuematch = $1.valuematch;
    $$.fromclause = $2;
    $$.redirectval = $3.redirectval;
    $$.redirectparam = $3.redirectparam;
    $$.redirectsender = $3.redirectsender;
    $$.redirectindex = $3.redirectindex;
  }
;

optPortRedirectWithValueAndParam: // [411]
  /* empty */
  {
    $$.redirectval=0;
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ParamSpec
  {
    $$.redirectval=0;
    $$.redirectparam=$2;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol SenderSpec
  {
    $$.redirectval=0;
    $$.redirectparam=0;
    $$.redirectsender=$2;
    $$.redirectindex=0;
  }
| PortRedirectSymbol IndexSpec
  {
    $$.redirectval=0;
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=$2;
  }
| PortRedirectSymbol ValueSpec ParamSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=$3;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec SenderSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=0;
    $$.redirectsender=$3;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol ParamSpec SenderSpec
  {
    $$.redirectval=0;
    $$.redirectparam=$2;
    $$.redirectsender=$3;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ParamSpec IndexSpec
  {
    $$.redirectval=0;
    $$.redirectparam=$2;
    $$.redirectsender=0;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol SenderSpec IndexSpec
  {
    $$.redirectval=0;
    $$.redirectparam=0;
    $$.redirectsender=$2;
    $$.redirectindex=$3;
  }
| PortRedirectSymbol ValueSpec ParamSpec SenderSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=$3;
    $$.redirectsender=$4;
    $$.redirectindex=0;
  }
| PortRedirectSymbol ValueSpec ParamSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=$3;
    $$.redirectsender=0;
    $$.redirectindex=$4;
  }
| PortRedirectSymbol ValueSpec SenderSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=0;
    $$.redirectsender=$3;
    $$.redirectindex=$4;
  }
| PortRedirectSymbol ParamSpec SenderSpec IndexSpec
  {
    $$.redirectval=0;
    $$.redirectparam=$2;
    $$.redirectsender=$3;
    $$.redirectindex=$4;
  }
| PortRedirectSymbol ValueSpec ParamSpec SenderSpec IndexSpec
  {
    $$.redirectval=$2;
    $$.redirectparam=$3;
    $$.redirectsender=$4;
    $$.redirectindex=$5;
  }
| PortRedirectSymbol error
  {
    $$.redirectval=0;
    $$.redirectparam=0;
    $$.redirectsender=0;
    $$.redirectindex=0;
  }
;

optGetReplyParameter:
  /* empty */
  {
    $$.templ_inst=0;
    $$.valuematch=0;
  }
| '(' ReceiveParameter optError ')'
  {
    $$.templ_inst=$2;
    $$.valuematch=0;
  }
| '(' ReceiveParameter ValueMatchSpec optError ')'
  {
    $$.templ_inst=$2;
    $$.valuematch=$3;
  }
| '(' error ')'
  {
    Template *t = new Template(Template::TEMPLATE_ERROR);
    t->set_location(infile, @2);
    $$.templ_inst = new TemplateInstance(0, 0, t);
    $$.templ_inst->set_location(infile, @2);
    $$.valuematch = 0;
  }
;

ValueMatchSpec: // 414
  ValueKeyword TemplateInstance { $$=$2; }
| ValueKeyword error { $$ = 0; }
;

CheckStatement: // 415
  PortOrAny DotCheckOpKeyword optCheckParameter
  {
    switch ($3.statementtype) {
    case Statement::S_CHECK:
      $$ = new Statement(Statement::S_CHECK, $1.reference, $1.any_from,
                         $3.templ_inst, $3.redirectsender, $3.redirectindex);
      break;
    case Statement::S_CHECK_RECEIVE:
      $$ = new Statement(Statement::S_CHECK_RECEIVE, $1.reference, $1.any_from,
                         $3.templ_inst, $3.fromclause, $3.redirectval,
                         $3.redirectsender, $3.redirectindex);
      break;
    case Statement::S_CHECK_GETCALL:
      $$ = new Statement(Statement::S_CHECK_GETCALL, $1.reference, $1.any_from,
                         $3.templ_inst, $3.fromclause, $3.redirectparam,
                         $3.redirectsender, $3.redirectindex);
      break;
    case Statement::S_CHECK_GETREPLY:
      $$ = new Statement(Statement::S_CHECK_GETREPLY, $1.reference, $1.any_from,
                         $3.templ_inst, $3.valuematch, $3.fromclause,
                         $3.redirectval, $3.redirectparam, $3.redirectsender,
                         $3.redirectindex);
      break;
    case Statement::S_CHECK_CATCH:
      $$ = new Statement(Statement::S_CHECK_CATCH, $1.reference, $1.any_from,
                         $3.signature, $3.templ_inst, $3.timeout, $3.fromclause,
                         $3.redirectval, $3.redirectsender, $3.redirectindex);
      break;
    default:
      FATAL_ERROR("Internal error.");
    } // switch
    $$->set_location(infile, @$);
  }
;

optCheckParameter: // [418]
  /* empty */
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = 0;
    $$.redirectindex = 0;
  }
| '(' CheckParameter optError ')' { $$ = $2; }
| '(' error ')'
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = 0;
    $$.redirectindex = 0;
  }
;

CheckParameter: // 418
  CheckPortOpsPresent { $$ = $1; }
| FromClausePresent { $$ = $1; }
| RedirectPresent { $$ = $1; }
;

FromClausePresent: // 419
  FromClause
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = $1;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = 0;
    $$.redirectindex = 0;
  }
| FromClause PortRedirectSymbol IndexSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = $1;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = 0;
    $$.redirectindex = $3;
  }
| FromClause PortRedirectSymbol SenderSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = $1;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = $3;
    $$.redirectindex = 0;
  }
| FromClause PortRedirectSymbol SenderSpec IndexSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = $1;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = $3;
    $$.redirectindex = $4;
  }
;

RedirectPresent: // 420
  PortRedirectSymbol SenderSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = $2;
    $$.redirectindex = 0;
  }
| PortRedirectSymbol IndexSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = 0;
    $$.redirectindex = $2;
  }
| PortRedirectSymbol SenderSpec IndexSpec
  {
    $$.statementtype = Statement::S_CHECK;
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = 0;
    $$.redirectval = 0;
    $$.redirectparam = 0;
    $$.redirectsender = $2;
    $$.redirectindex = $3;
  }
;

CheckPortOpsPresent: // 421
  ReceiveOpKeyword PortReceiveOp
  {
    $$.statementtype = Statement::S_CHECK_RECEIVE;
    $$.signature = 0;
    $$.templ_inst = $2.templ_inst;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = $2.fromclause;
    $$.redirectval = $2.redirectval;
    $$.redirectparam = 0;
    $$.redirectsender = $2.redirectsender;
    $$.redirectindex = $2.redirectindex;
  }
| GetCallOpKeyword PortGetCallOp
  {
    $$.statementtype = Statement::S_CHECK_GETCALL;
    $$.signature = 0;
    $$.templ_inst = $2.templ_inst;
    $$.valuematch = 0;
    $$.timeout = false;
    $$.fromclause = $2.fromclause;
    $$.redirectval = 0;
    $$.redirectparam = $2.redirectparam;
    $$.redirectsender = $2.redirectsender;
    $$.redirectindex = $2.redirectindex;
}
| GetReplyOpKeyword PortGetReplyOp
  {
    $$.statementtype = Statement::S_CHECK_GETREPLY;
    $$.signature = 0;
    $$.templ_inst = $2.templ_inst;
    $$.valuematch = $2.valuematch;
    $$.timeout = false;
    $$.fromclause = $2.fromclause;
    $$.redirectval = $2.redirectval;
    $$.redirectparam = $2.redirectparam;
    $$.redirectsender = $2.redirectsender;
    $$.redirectindex = $2.redirectindex;
}
| CatchOpKeyword PortCatchOp
  {
    $$.statementtype = Statement::S_CHECK_CATCH;
    $$.signature = $2.signature;
    $$.templ_inst = $2.templ_inst;
    $$.valuematch = 0;
    $$.timeout = $2.timeout;
    $$.fromclause = $2.fromclause;
    $$.redirectval = $2.redirectval;
    $$.redirectparam = 0;
    $$.redirectsender = $2.redirectsender;
    $$.redirectindex = $2.redirectindex;
  }
;

CatchStatement: // 422
  PortOrAny DotCatchOpKeyword PortCatchOp
  {
    $$ = new Statement(Statement::S_CATCH, $1.reference, $1.any_from,
                       $3.signature, $3.templ_inst, $3.timeout, $3.fromclause,
                       $3.redirectval, $3.redirectsender, $3.redirectindex);
    $$->set_location(infile, @$);
  }
;

PortCatchOp: // 423
  optCatchOpParameter optFromClause optPortRedirect
  {
    $$.signature = $1.signature;
    $$.templ_inst = $1.templ_inst;
    $$.timeout = $1.timeout;
    $$.fromclause = $2;
    $$.redirectval = $3.redirectval;
    $$.redirectsender = $3.redirectsender;
    $$.redirectindex = $3.redirectindex;
  }
;

optCatchOpParameter: // [425]
  /* empty */
  {
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.timeout = false;
  }
| '(' CatchOpParameter optError ')' { $$ = $2; }
| '(' error ')'
  {
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.timeout = false;
  }
;

CatchOpParameter: // 425
  Signature optError ',' optError TemplateInstance
  {
    $$.signature = $1;
    $$.templ_inst = $5;
    $$.timeout = false;
  }
| TimeoutKeyword
  {
    $$.signature = 0;
    $$.templ_inst = 0;
    $$.timeout = true;
  }
;

ClearStatement: // 426
  PortOrAll DotClearOpKeyword
  {
    $$ = new Statement(Statement::S_CLEAR, $1);
    $$->set_location(infile, @$);
  }
;

PortOrAll: // 427
  Port { $$ = $1; }
| AllKeyword PortKeyword { $$ = 0; }
;

StartStatement: // 430
/*  Port DotPortStartKeyword -- covered by StartTimerStatement */
  AllKeyword PortKeyword DotStartKeyword
  {
    $$=new Statement(Statement::S_START_PORT, static_cast<Ttcn::Reference*>(0));
    $$->set_location(infile, @$);
  }
;

StopStatement: // 432
/*  Port DotPortStopKeyword -- covered by StopTimerStatement */
  AllKeyword PortKeyword DotStopKeyword
  {
    $$=new Statement(Statement::S_STOP_PORT, static_cast<Ttcn::Reference*>(0));
    $$->set_location(infile, @$);
  }
;

HaltStatement: // 435
  PortOrAll DotHaltKeyword
  {
    $$ = new Statement(Statement::S_HALT, $1);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.2.5 Timer operations */

TimerStatements: // 439
  StartTimerStatement { $$ = $1; }
| StopTimerStatement { $$ = $1; }
| TimeoutStatement { $$ = $1; }
;

TimerOps: // 440
  ReadTimerOp { $$ = $1; }
| RunningTimerOp { $$ = $1; }
;

StartTimerStatement: // 441
  VariableRef DotStartKeyword
  {
    $$ = new Statement(Statement::S_START_UNDEF, $1, static_cast<Value*>(0));
    $$->set_location(infile, @$);
  }
| VariableRef DotStartKeyword '(' Expression optError ')'
  {
    $$ = new Statement(Statement::S_START_UNDEF, $1, $4);
    $$->set_location(infile, @$);
  }
| VariableRef DotStartKeyword '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @4);
    $$ = new Statement(Statement::S_START_UNDEF, $1, v);
    $$->set_location(infile, @$);
  }
;

StopTimerStatement: // 442
  TimerRef DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_UNDEF, $1, static_cast<Value*>(0));
    $$->set_location(infile, @$);
  }
| AllKeyword TimerKeyword DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_TIMER, static_cast<Ttcn::Reference*>(0));
    $$->set_location(infile, @$);
  }
;

/* no separate rule, folded into StopTimerStatement
TimerRefOrAll: // 443
  TimerRef
| AllKeyword TimerKeyword
;
*/

ReadTimerOp: // 444
  TimerRef DotReadKeyword
  {
    $$ = new Value(Value::OPTYPE_TMR_READ, $1);
    $$->set_location(infile, @$);
  }
;

RunningTimerOp: // 446
  TimerRef DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_UNDEF_RUNNING, $1, NULL, false);
    $$->set_location(infile, @$);
  }
| AnyKeyword TimerKeyword DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_TMR_RUNNING_ANY);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword TimerRef DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_UNDEF_RUNNING, $3, NULL, true);
    $$->set_location(infile, @$);
  }
| AnyKeyword FromKeyword TimerRef DotRunningKeyword PortRedirectSymbol IndexSpec
  {
    $$ = new Value(Value::OPTYPE_UNDEF_RUNNING, $3, $6, true);
    $$->set_location(infile, @$);
  }
;

TimeoutStatement: // 447
  TimerRefOrAny DotTimeoutKeyword
  {
    $$ = new Statement(Statement::S_TIMEOUT, $1.reference, $1.any_from, NULL);
    $$->set_location(infile, @$);
  }
| TimerRefOrAny DotTimeoutKeyword PortRedirectSymbol IndexSpec
  {
    $$ = new Statement(Statement::S_TIMEOUT, $1.reference, $1.any_from, $4);
    $$->set_location(infile, @$);
  }
;

TimerRefOrAny: // 448
  TimerRef { $$.reference = $1; $$.any_from = false; }
| AnyKeyword TimerKeyword { $$.reference = 0; $$.any_from = false; }
| AnyKeyword FromKeyword TimerRef { $$.reference = $3; $$.any_from = true; }
;

/* A.1.6.3 Type */

Type: // 450
  PredefinedType
  {
    $$ = new Type($1);
    $$->set_location(infile, @$);
  }
| AnyTypeKeyword /* a predefined type with special treatment */
  {
    anytype_access = true;
    Identifier *id = new Identifier(Identifier::ID_TTCN, string("anytype"));
    Ttcn::Reference *ref = new Ttcn::Reference(id);
    ref->set_location(infile, @1);
    $$ = new Type(Type::T_REFD, ref);
  }
| ReferencedType { $$ = $1; }

;

PredefinedType: // 451, but see below
  BitStringKeyword { $$ = Type::T_BSTR; }
| BooleanKeyword { $$ = Type::T_BOOL; }
| CharStringKeyword { $$ = Type::T_CSTR; }
| UniversalCharString { $$ = Type::T_USTR; }
| CharKeyword // not in the standard anymore
  {
    Location loc(infile, @$);
    loc.warning("Obsolete type `char' was substituted with `charstring'");
    $$ = Type::T_CSTR;
  }
| UniversalChar // not in the standard anymore
  {
    Location loc(infile, @$);
    loc.warning("Obsolete type `universal char' was substituted with "
                "`universal charstring'");
    $$ = Type::T_USTR;
  }
| IntegerKeyword { $$ = Type::T_INT; }
| OctetStringKeyword { $$ = Type::T_OSTR; }
| HexStringKeyword { $$ = Type::T_HSTR; }
| VerdictTypeKeyword { $$ = Type::T_VERDICT; }
| FloatKeyword { $$ = Type::T_REAL; }
| AddressKeyword { $$ = Type::T_ADDRESS; }
| DefaultKeyword { $$ = Type::T_DEFAULT; }
| ObjectIdentifierKeyword { $$ = Type::T_OID; }
/*
 * AnyTypeKeyword is not part of PredefinedType (this differs from the BNF
 * in the TTCN-3 standard).
 * PredefinedType is used in two contexts:
 * - as a RHS for Type, above (where AnyTypeKeyword needs special treatment,
 *   and it's easier to appear as an alternative to PredefinedType)
 * - as field name for the anytype (where anytype is not permitted)
 */
;

UniversalCharString: // 463
  UniversalKeyword CharStringKeyword
;

UniversalChar:
  UniversalKeyword CharKeyword
;

ReferencedType: // 465
  Reference
  {
    if ($1.is_ref) $$ = new Type(Type::T_REFD, $1.ref);
    else {
      Ttcn::Reference *ref = new Ttcn::Reference($1.id);
      ref->set_location(infile, @1);
      $$ = new Type(Type::T_REFD, ref);
    }
    $$->set_location(infile, @$);
  }
| FunctionInstance optExtendedFieldReference
  /* covers all parameterized type references */
  {
    Location loc(infile, @1);
    loc.error("Reference to parameterized type is not currently supported");
    delete $1;
    for (size_t i = 0; i < $2.nElements; i++) delete $2.elements[i];
    Free($2.elements);
    $$ = new Type(Type::T_ERROR);
    $$->set_location(infile, @$);
  }
;

/*
TypeReference: // 466
  IDentifier
| IDentifier TypeActualParList
;
*/

TypeActualParList: // -> 202 784 "Advanced Parameterization"
  '(' seqTypeActualPar optError ')'
| '(' error ')'
;

seqTypeActualPar: // -> 202 784 "Advanced Parameterization"
  optError TypeActualPar
| seqTypeActualPar optError ',' optError TypeActualPar
| seqTypeActualPar optError ',' error
;

TypeActualPar: // -> 202 784 "Advanced Parameterization"
  Expression { delete $1; }
;

optArrayDef: // [467]
  /* empty */
  {
    $$.nElements = 0;
    $$.elements = 0;
  }
| optArrayDef ArrayIndex
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<ArrayDimension**>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)) );
    $$.elements[$1.nElements] = $2;
  }
;

ArrayIndex:
  '[' ArrayBounds ']'
  {
    $$ = new ArrayDimension($2);
    $$->set_location(infile, @$);
  }
| '[' ArrayBounds DotDot ArrayBounds ']'
  {
    $$ = new ArrayDimension($2, $4);
    $$->set_location(infile, @$);
  }
| '[' error ']'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @2);
    $$ = new ArrayDimension(v);
    $$->set_location(infile, @$);
  }
;

ArrayBounds: // 468
  Expression { $$ = $1; }
;

/* A.1.6.4 Value */

Value: // 469
  PredefinedValue { $$ = $1; }
| ReferencedValue { $$ = $1; }
;

PredefinedValue: // 470
  BitStringValue { $$ = $1; }
| BooleanValue { $$ = $1; }
| CharStringValue { $$ = $1; }
| IntegerValue { $$ = $1; }
| OctetStringValue { $$ = $1; }
| HexStringValue { $$ = $1; }
| VerdictValue { $$ = $1; }
/*  | EnumeratedValue -- covered by ReferencedValue */
| FloatOrSpecialFloatValue
  {
    $$ = new Value(Value::V_REAL, $1);
    $$->set_location(infile, @$);
  }
| AddressValue { $$ = $1; }
| OmitValue { $$ = $1; }
| NullValue
  {
    $$ = new Value(Value::V_NULL);
    $$->set_location(infile, @$);
  }
| MacroValue
  {
    $$ = new Value(Value::V_MACRO, $1);
    $$->set_location(infile, @$);
  }
| ObjectIdentifierValue { $$ = $1; }
| TOK_errval
  {
    $$=new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
;

BitStringValue: // 471
  Bstring
  {
    $$ = new Value(Value::V_BSTR, $1);
    $$->set_location(infile, @$);
  }
;

BooleanValue: // 472
  TrueKeyword
  {
    $$ = new Value(Value::V_BOOL, true);
    $$->set_location(infile, @$);
  }
| FalseKeyword
  {
    $$ = new Value(Value::V_BOOL, false);
    $$->set_location(infile, @$);
  }
;

/* TTCN-3 core language V4.2.1 */
FloatOrSpecialFloatValue:
  FloatValue
  {
    $$ = $1;
  }
| InfinityKeyword
  {
    $$ = REAL_INFINITY;
  }
| NaNKeyword
  {
    $$ = REAL_NAN;
  }
;

IntegerValue: // 473
  Number
  {
    $$ = new Value(Value::V_INT, $1);
    $$->set_location(infile, @$);
  }
;

OctetStringValue: // 474
  Ostring
  {
    $$ = new Value(Value::V_OSTR, $1);
    $$->set_location(infile, @$);
  }
;

HexStringValue: // 475
  Hstring
  {
    $$ = new Value(Value::V_HSTR, $1);
    $$->set_location(infile, @$);
  }
;

VerdictValue: // 476 VerdictTypeValue
  NoneKeyword
  {
    $$ = new Value(Value::V_VERDICT, Value::Verdict_NONE);
    $$->set_location(infile, @$);
  }
| PassKeyword
  {
    $$ = new Value(Value::V_VERDICT, Value::Verdict_PASS);
    $$->set_location(infile, @$);
  }
| InconcKeyword
  {
    $$ = new Value(Value::V_VERDICT, Value::Verdict_INCONC);
    $$->set_location(infile, @$);
  }
| FailKeyword
  {
    $$ = new Value(Value::V_VERDICT, Value::Verdict_FAIL);
    $$->set_location(infile, @$);
  }
| ErrorKeyword
  {
    $$ = new Value(Value::V_VERDICT, Value::Verdict_ERROR);
    $$->set_location(infile, @$);
  }
;

CharStringValue: // 478
  CstringList
  {
    if ($1->is_cstr()) $$ = new Value(Value::V_CSTR, $1);
    else {
      $$ = new Value(Value::V_USTR, new ustring(*$1));
      delete $1;
    }
    $$->set_location(infile, @$);
  }
| Quadruple
  {
    $$ = new Value(Value::V_USTR, new ustring(*$1));
    delete $1;
    $$->set_location(infile, @$);
  }
| USI
  {
    $$ = new Value(Value::V_USTR, new ustring($1.elements, $1.nElements));
    for(size_t i = 0; i < $1.nElements; ++i) {
      Free(const_cast<char*>($1.elements[i]));
    }
    Free($1.elements);
    $$->set_location(infile, @$);
  }
;

CstringList:
  Cstring
  {
    Location loc(infile, @1);
    $$ = parse_charstring_value($1, loc);
    Free($1);
  }
;

USI:
  CharKeyword '(' optError UIDlike optError ')'
  {
    $$ = $4;
  }
;

UIDlike:
  Cstring
  {
    $$.nElements = 1;
    $$.elements = static_cast<const char**>(
      Realloc($$.elements, ($$.nElements) * sizeof(*$$.elements)) );
    $$.elements[$$.nElements-1] = $1;
  }
| UIDlike optError ',' optError Cstring {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<const char**>(
      Realloc($1.elements, ($$.nElements) * sizeof(*$$.elements)) );
    $$.elements[$$.nElements-1] = $5;
  }
;

Quadruple: // 479
  CharKeyword '(' optError Group optError ',' optError Plane optError ','
  optError Row optError ',' optError Cell optError ')'
  { $$ = new ustring($4, $8, $12, $16); }
| CharKeyword '(' error ')' { $$ = new ustring; }
;

Group: // 481
  Number
  {
    if (*$1 < 0 || *$1 > 127) {
      Location loc(infile, @1);
      loc.error("The first number of quadruple (group) must be within the "
        "range 0 .. 127 instead of %s", $1->t_str().c_str());
      $$ = *$1 < 0 ? 0 : 127;
    } else {
    	$$ = $1->get_val();
    }
    delete $1;
  }
;

Plane: // 482
  Number
  {
    if (*$1 < 0 || *$1 > 255) {
      Location loc(infile, @1);
      loc.error("The second number of quadruple (plane) must be within the "
        "range 0 .. 255 instead of %s", $1->t_str().c_str());
      $$ = *$1 < 0 ? 0 : 255;
    } else {
    	$$ = $1->get_val();
   	}
    delete $1;
  }
;

Row: // 483
  Number
  {
    if (*$1 < 0 || *$1 > 255) {
      Location loc(infile, @1);
      loc.error("The third number of quadruple (row) must be within the "
        "range 0 .. 255 instead of %s", $1->t_str().c_str());
      $$ = *$1 < 0 ? 0 : 255;
    } else {
    	$$ = $1->get_val();
   	}
    delete $1;
  }
;

Cell: // 484
  Number
  {
    if (*$1 < 0 || *$1 > 255) {
      Location loc(infile, @1);
      loc.error("The fourth number of quadruple (cell) must be within the "
        "range 0 .. 255 instead of %s", $1->t_str().c_str());
      $$ = *$1 < 0 ? 0 : 255;
    } else {
    	$$ = $1->get_val();
    }
    delete $1;
  }
;

FreeText: // 509
  Cstring { $$ = $1; }
| FreeText Cstring
  {
    $$ = mputstr($1, $2);
    Free($2);
  }
;

AddressValue: // 510
  NullKeyword
  {
    $$ = new Value(Value::V_TTCN3_NULL);
    $$->set_location(infile, @$);
  }
;

OmitValue: // 511
  OmitKeyword
  {
    $$ = new Value(Value::V_OMIT);
    $$->set_location(infile, @$);
  }
;

ReferencedValue: // 489
  Reference
  {
    if ($1.is_ref) $$ = new Value(Value::V_REFD, $1.ref);
    else $$ = new Value(Value::V_UNDEF_LOWERID, $1.id);
    $$->set_location(infile, @$);
  }

Reference: // 490 ValueReference
  IDentifier
  {
    $$.is_ref = false;
    $$.id = $1;
  }
| IDentifier '.' PredefOrIdentifier optExtendedFieldReference
  {
    $$.is_ref = true;
    $$.ref = new Ttcn::Reference($1);
    FieldOrArrayRef *fieldref = new FieldOrArrayRef($3);
    fieldref->set_location(infile, @3);
    $$.ref->add(fieldref);
    for (size_t i = 0; i < $4.nElements; i++) $$.ref->add($4.elements[i]);
    Free($4.elements);
    $$.ref->set_location(infile, @$);
  }
| IDentifier ArrayOrBitRef optExtendedFieldReference
  {
    $$.is_ref = true;
    $$.ref = new Ttcn::Reference($1);
    $$.ref->add($2);
    for (size_t i = 0; i < $3.nElements; i++) $$.ref->add($3.elements[i]);
    Free($3.elements);
    $$.ref->set_location(infile, @$);
  }
| IDentifier '[' NotUsedSymbol ']'
{
  $$.is_ref = true;
  $$.ref = new Ttcn::Reference($1);
  Value* novalue = new Value(Value::V_NOTUSED);
  novalue->set_location(infile, @3);
  $$.ref->add(new FieldOrArrayRef(novalue));
  $$.ref->set_location(infile, @$);
}
| IDentifier '.' ObjectIdentifierValue '.' IDentifier
  optExtendedFieldReference
  {
    $$.is_ref = true;
    $$.ref = new Ttcn::Reference($1, $5);
    delete $3;
    for (size_t i = 0; i < $6.nElements; i++) $$.ref->add($6.elements[i]);
    Free($6.elements);
    $$.ref->set_location(infile, @$);
  }
;

/* A.1.6.5 Parameterization */

optLazyOrFuzzyModifier:
  /* empty */ { $$ = NORMAL_EVAL; }
| LazyKeyword { $$ = LAZY_EVAL; }
| FuzzyKeyword { $$ = FUZZY_EVAL; }
;

FormalValuePar: // 516
  optLazyOrFuzzyModifier Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_VAL, $2, $3, $4, $1);
    $$->set_location(infile, @$);
  }
| InParKeyword optLazyOrFuzzyModifier Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_VAL_IN, $3, $4, $5, $2);
    $$->set_location(infile, @$);
  }
| InOutParKeyword Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_VAL_INOUT, $2, $3, $4);
    $$->set_location(infile, @$);
  }
| OutParKeyword Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_VAL_OUT, $2, $3, $4);
    $$->set_location(infile, @$);
  }
;

/*
FormalPortPar: // 518
  IDentifier IDentifier
| InOutParKeyword IDentifier IDentifier
;
*/

FormalTimerPar: // 520
  TimerKeyword IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TIMER, $2, $3);
    $$->set_location(infile, @$);
  }
| InOutParKeyword TimerKeyword IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TIMER, $3, $4);
    $$->set_location(infile, @$);
  }
;

FormalTemplatePar: // 522
  TemplateOptRestricted optLazyOrFuzzyModifier Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TEMPL_IN, $1, $3, $4, $5, $2);
    $$->set_location(infile, @$);
  }
| InParKeyword TemplateOptRestricted optLazyOrFuzzyModifier Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TEMPL_IN, $2, $4, $5, $6, $3);
    $$->set_location(infile, @$);
  }
| InOutParKeyword TemplateOptRestricted Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TEMPL_INOUT, $2, $3, $4, $5);
    $$->set_location(infile, @$);
  }
| OutParKeyword TemplateOptRestricted Type IDentifier optParDefaultValue
  {
    $$ = new FormalPar(Common::Assignment::A_PAR_TEMPL_OUT, $2, $3, $4, $5);
    $$->set_location(infile, @$);
  }
;

/* template with optional restriction */
TemplateOptRestricted:
  TemplateKeyword optTemplateRestriction
  {
    $$ = $2;
  }
| OmitKeyword
  {
    $$ = TR_OMIT;
  }
;

optTemplateRestriction:
  /* none */ { $$ = TR_NONE; }
| TemplateRestriction { $$ = $1; }

TemplateRestriction:
  '(' OmitKeyword ')'    { $$ = TR_OMIT; }
| '(' ValueKeyword ')'   { $$ = TR_VALUE; }
| '(' PresentKeyword ')'   { $$ = TR_PRESENT; }
;

optParDefaultValue:
  /* empty */ { $$ = NULL; }
| AssignmentChar TemplateInstance { $$ = $2; }
| AssignmentChar NotUsedSymbol
  {
    Template *t = new Template(Template::TEMPLATE_NOTUSED);
    t->set_location(infile, @$);
    $$ = new TemplateInstance(0, 0, t);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.6 With statement */

optWithStatement: // [526]
  /* empty */ { $$ = NULL; }
| WithStatement { $$ = $1; }
;

optWithStatementAndSemiColon:
  /* empty */ { $$ = NULL; }
| WithStatement { $$ = $1; }
| ';' { $$ = NULL; }
| WithStatement ';' { $$ = $1; }
;

WithStatement: // 526
  WithKeyword WithAttribList { $$ = $2; }
;

WithAttribList: // 528
  '{' MultiWithAttrib optError '}' { $$ = $2; }
| '{' error '}' { $$ = NULL; }
;

MultiWithAttrib: // 529
  optError SingleWithAttrib optSemiColon
  {
    $$ = new MultiWithAttrib;
    $$->set_location(infile, @$);
    $$->add_element($2);
  }
| MultiWithAttrib optError SingleWithAttrib optSemiColon
  {
    $$ = $1;
    $$->add_element($3);
  }
;

SingleWithAttrib: // 530
  AttribKeyword optOverrideKeyword optAttribQualifier AttribSpec
  {
    $$ = new SingleWithAttrib($1,$2,$3,$4);
    $$->set_location(infile, @$);
  }
;

AttribKeyword: // 531
  EncodeKeyword { $$ = SingleWithAttrib::AT_ENCODE; }
| VariantKeyword { $$ = SingleWithAttrib::AT_VARIANT; }
| DisplayKeyword { $$ = SingleWithAttrib::AT_DISPLAY; }
| ExtensionKeyword { $$ = SingleWithAttrib::AT_EXTENSION; }
| OptionalKeyword { $$ = SingleWithAttrib::AT_OPTIONAL; }
| IDentifier
  {
    /* workaround to get rid of ErroneousKeyword which would clash with
     * existing TTCN-3 source code */
    if ($1->get_ttcnname()=="erroneous") $$ = SingleWithAttrib::AT_ERRONEOUS;
    else {
      Location loc(infile, @1);
      loc.error("Invalid attribute. Valid attributes are: "
                "`encode', `variant' , `display' , `extension', `optional' and `erroneous'");
      if ($1->get_ttcnname()=="titan")
        loc.note("\n"
          " ________    _____   ________     ____        __      _  \n"
          "(___  ___)  (_   _) (___  ___)   (    )      /  \\    / ) \n"
          "    ) )       | |       ) )      / /\\ \\     / /\\ \\  / /  \n"
          "   ( (        | |      ( (      ( (__) )    ) ) ) ) ) )  \n"
          "    ) )       | |       ) )      )    (    ( ( ( ( ( (   \n"
          "   ( (       _| |__    ( (      /  /\\  \\   / /  \\ \\/ /   \n"
          "   /__\\     /_____(    /__\\    /__(  )__\\ (_/    \\__/    \n");
      $$ = SingleWithAttrib::AT_INVALID;
    }
    delete $1;
  }
;

optOverrideKeyword: // [536]
  /* empty */ { $$ = false; }
| OverrideKeyword { $$ = true; }
;

optAttribQualifier: // [537]
  /* empty */ { $$ = NULL; }
| '(' DefOrFieldRefList optError ')' { $$ = $2; }
| '(' error ')' { $$ = NULL; }
;

DefOrFieldRefList: // 538
  optError DefOrFieldRef
  {
    $$ = new Qualifiers();
    if ($2) $$->add_qualifier($2);
  }
| DefOrFieldRefList optError ',' optError DefOrFieldRef
  {
    $$ = $1;
    if ($5) $$->add_qualifier($5);
  }
| DefOrFieldRefList optError ',' error { $$ = $1; }
;

ArrayOrBitRefOrDash:
ArrayOrBitRef  { $$ = $1; }
| '[' NotUsedSymbol ']'
{
  Value* novalue = new Value(Value::V_NOTUSED);
  novalue->set_location(infile, @2);
  $$ = new FieldOrArrayRef(novalue);
  $$->set_location(infile, @$);
}

DefOrFieldRef: // 539
  IDentifier
  {
    $$ = new Qualifier();
    $$->add(new FieldOrArrayRef($1));
    $$->set_location(infile, @1);
  }
| IDentifier '.' IDentifier optExtendedFieldReference
{
  $$ = new Qualifier();
  $$->add(new FieldOrArrayRef($1));
  $$->add(new FieldOrArrayRef($3));
  for(size_t i=0; i<$4.nElements; i++) {
    $$->add($4.elements[i]);
  }
  Free($4.elements);
  $$->set_location(infile, @$);
}
| IDentifier ArrayOrBitRefOrDash optExtendedFieldReference
{
  $$ = new Qualifier();
  $$->add(new FieldOrArrayRef($1));
  $$->add($2);
  for(size_t i=0; i<$3.nElements; i++) {
    $$->add($3.elements[i]);
  }
  Free($3.elements);
  $$->set_location(infile, @$);
}
| ArrayOrBitRefOrDash optExtendedFieldReference
{
  $$ = new Qualifier();
  $$->add($1);
  for(size_t i=0; i<$2.nElements; i++) {
    $$->add($2.elements[i]);
  }
  Free($2.elements);
  $$->set_location(infile, @$);
}
| AllRef
  {
    Location loc(infile, @$);
    loc.error("Reference to multiple definitions in attribute qualifiers is "
      "not currently supported");
    $$ = 0;
  }
;

AllRef: // 541
  GroupKeyword AllKeyword
| GroupKeyword AllKeyword ExceptKeyword '{' GroupRefList optError '}'
| TypeDefKeyword AllKeyword
| TypeDefKeyword AllKeyword ExceptKeyword '{' TypeRefList optError '}'
| TemplateKeyword AllKeyword
| TemplateKeyword AllKeyword ExceptKeyword '{' TemplateRefList optError '}'
| ConstKeyword AllKeyword
| ConstKeyword AllKeyword ExceptKeyword '{' ConstRefList optError '}'
| AltstepKeyword AllKeyword
| AltstepKeyword AllKeyword ExceptKeyword '{' AltstepRefList optError '}'
| TestcaseKeyword AllKeyword
| TestcaseKeyword AllKeyword ExceptKeyword '{' TestcaseRefList optError '}'
| FunctionKeyword AllKeyword
| FunctionKeyword AllKeyword ExceptKeyword '{' FunctionRefList optError '}'
| SignatureKeyword AllKeyword
| SignatureKeyword AllKeyword ExceptKeyword '{' SignatureRefList optError '}'
| ModuleParKeyword AllKeyword
| ModuleParKeyword AllKeyword ExceptKeyword '{' ModuleParRefList optError '}'
;

AttribSpec: // 542
  FreeText
  {
    $$ = new AttributeSpec(string($1));
    $$->set_location(infile, @$);
    Free($1);
  }
;

/* A.1.6.7 Behaviour statements */

BehaviourStatements: // 543
  TestcaseInstance
  {
    if ($1.ref_pard) $$ = new Statement(Statement::S_TESTCASE_INSTANCE,
      $1.ref_pard, $1.value);
    else if($1.derefered_value) {
      $$ = new Statement(Statement::S_TESTCASE_INSTANCE_REFD,
        $1.derefered_value, $1.ap_list->steal_tis(), $1. value);
      delete $1.ap_list;
    }
    else $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
  }
| FunctionInstance
  {
    $$ = new Statement(Statement::S_UNKNOWN_INSTANCE, $1);
    $$->set_location(infile, @$);
  }
| ApplyOp
  {
    $$ = new Statement(Statement::S_UNKNOWN_INVOKED, $1.value, $1.ap_list);
    $$->set_location(infile, @$);
  }
| ReturnStatement { $$ = $1; }
| AltConstruct { $$ = $1; }
| InterleavedConstruct { $$ = $1; }
| LabelStatement { $$ = $1; }
| GotoStatement { $$=$1; }
| RepeatStatement { $$ = $1; }
| BreakStatement { $$ = $1; }
| ContinueStatement { $$ = $1; }
| DeactivateStatement { $$ = $1; }
/* | AltstepInstance -- covered by FunctionInstance */
| ActivateOp
  {
    if ($1.ref_pard) $$ = new Statement(Statement::S_ACTIVATE, $1.ref_pard);
    else if($1.derefered_value) $$ = new Statement(Statement::S_ACTIVATE_REFD,
      $1.derefered_value, new ParsedActualParameters($1.ap_list));
    else $$ = new Statement(Statement::S_ERROR);
    $$->set_location(infile, @$);
  }
;

VerdictStatements: // 544
  SetLocalVerdict { $$ = $1; }
;

VerdictOps: // 545
  GetLocalVerdict { $$ = $1; }
;

SetLocalVerdict: // 546
  SetVerdictKeyword '(' Expression optError ',' LogItemList optError ')'
  {
    $$=new Statement(Statement::S_SETVERDICT, $3, $6);
    $$->set_location(infile, @$);
  }
| SetVerdictKeyword '(' Expression optError ')'
  {
    $$=new Statement(Statement::S_SETVERDICT, $3, static_cast<LogArguments*>(0));
    $$->set_location(infile, @$);
  }
| SetVerdictKeyword '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$ = new Statement(Statement::S_SETVERDICT, v);
    $$->set_location(infile, @$);
  }
;

GetLocalVerdict: // 548
  GetVerdictKeyword
  {
    $$=new Value(Value::OPTYPE_GETVERDICT);
    $$->set_location(infile, @$);
  }
;

SUTStatements: // 549
  ActionKeyword '(' ')'
  {
    $$=new Statement(Statement::S_ACTION, static_cast<LogArguments*>(0));
    $$->set_location(infile, @$);
  }
| ActionKeyword '(' LogItemList optError ')'
  {
    $$=new Statement(Statement::S_ACTION, $3);
    $$->set_location(infile, @$);
  }
| ActionKeyword '(' error ')'
  {
    $$=new Statement(Statement::S_ACTION, new LogArguments());
    $$->set_location(infile, @$);
  }
;

StopExecutionStatement:
  StopKeyword
  {
    $$=new Statement(Statement::S_STOP_EXEC);
    $$->set_location(infile, @$);
  }
;

StopTestcaseStatement:
  TestcaseKeyword DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_TESTCASE, static_cast<LogArguments*>(0));
    $$->set_location(infile, @$);
  }
| TestcaseKeyword DotStopKeyword '(' LogItemList optError ')'
  {
    $$ = new Statement(Statement::S_STOP_TESTCASE, $4);
    $$->set_location(infile, @$);
  }
;

  /* these deliberately don't have their locations set */
ProfilerStatement:
  TitanSpecificProfilerKeyword DotStartKeyword
  {
    $$ = new Statement(Statement::S_START_PROFILER);
  }
| TitanSpecificProfilerKeyword DotStopKeyword
  {
    $$ = new Statement(Statement::S_STOP_PROFILER);
  }
;

int2enumStatement:
  int2enumKeyword '(' optError Expression optError ',' optError Reference optError ')'
  {
    Ttcn::Reference* out_ref;
    if ($8.is_ref) out_ref = $8.ref;
    else {
      out_ref = new Ttcn::Reference($8.id);
      out_ref->set_location(infile, @8);
    }
    $$ = new Statement(Statement::S_INT2ENUM, $4, out_ref);
    $$->set_location(infile, @$);
  }
;

UpdateStatement:
  TitanSpecificUpdateKeyword '(' Reference ')' optWithStatement
  {
    Ttcn::Reference* ref;
    if ($3.is_ref) {
      ref = $3.ref;
    }
    else {
      ref = new Ttcn::Reference($3.id);
      ref->set_location(infile, @3);
    }
    $$ = new Statement(Statement::S_UPDATE, ref, $5);
    $$->set_location(infile, @$);
  }
;

SetstateStatement:
  PortKeyword '.' SetstateKeyword '(' SingleExpression ')'
  {
    $$ = new Statement(Statement::S_SETSTATE, $5);
    $$->set_location(infile, @$);
  }
| PortKeyword '.' SetstateKeyword '(' SingleExpression ',' TemplateInstance ')'
  {
    $$ = new Statement(Statement::S_SETSTATE, $5, $7);
    $$->set_location(infile, @$);
  }

ProfilerRunningOp:
  TitanSpecificProfilerKeyword DotRunningKeyword
  {
    $$ = new Value(Value::OPTYPE_PROF_RUNNING);
    $$->set_location(infile, @$);
  }
;

ReturnStatement: // 552
  ReturnKeyword
  {
    $$=new Statement(Statement::S_RETURN, static_cast<Template*>(0));
    $$->set_location(infile, @$);
  }
| ReturnKeyword TemplateBody
  {
    $$=new Statement(Statement::S_RETURN, $2);
    $$->set_location(infile, @$);
  }
;

AltConstruct: // 553
  AltKeyword '{' AltGuardList optError '}'
  {
    $$=new Statement(Statement::S_ALT, $3);
    $$->set_location(infile, @$);
  }
| AltKeyword '{' error '}'
  {
    $$=new Statement(Statement::S_ALT, new AltGuards());
    $$->set_location(infile, @$);
  }
;

AltGuardList: // 555
  optError AltGuard
  {
    $$ = new AltGuards;
    $$->add_ag($2);
  }
| AltGuardList optError AltGuard
  {
    $$ = $1;
    $$->add_ag($3);
  }
;

AltGuard:
  GuardStatement { $$ = $1; }
| ElseStatement { $$ = $1; }
;

GuardStatement: // 556
  AltGuardChar AltstepInstance optSemiColon
  {
    $$=new AltGuard($1, $2, 0);
    $$->set_location(infile, @$);
  }
| AltGuardChar ApplyOp optSemiColon
  {
    $$=new AltGuard($1, $2.value, $2.ap_list->steal_tis(), 0);
    $$->set_location(infile, @$);
    delete $2.ap_list;
  }
| AltGuardChar AltstepInstance optSemiColon StatementBlock optSemiColon
  {
    $$=new AltGuard($1, $2, $4);
    $$->set_location(infile, @$);
  }
| AltGuardChar ApplyOp optSemiColon StatementBlock optSemiColon
  {
    $$= new AltGuard($1, $2.value, $2.ap_list->steal_tis(), $4);
    $$->set_location(infile, @$);
    delete $2.ap_list;
  }
| AltGuardChar GuardOp ';'
  {
    $$=new AltGuard($1, $2, new StatementBlock());
    $$->set_location(infile, @$);
  }
| AltGuardChar GuardOp optSemiColon StatementBlock optSemiColon
  {
    $$=new AltGuard($1, $2, $4);
    $$->set_location(infile, @$);
  }
;

ElseStatement: // 557
  '[' ElseKeyword ']' StatementBlock optSemiColon
  {
    $$=new AltGuard($4);
    $$->set_location(infile, @$);
  }
;

AltGuardChar: // 558
  '[' ']' { $$=0; }
| '[' BooleanExpression ']' { $$ = $2; }
;

GuardOp: // 559
  TimeoutStatement { $$=$1; }
| ReceiveStatement { $$=$1; }
| TriggerStatement { $$=$1; }
| GetCallStatement { $$=$1; }
| CatchStatement { $$=$1; }
| CheckStatement { $$=$1; }
| GetReplyStatement { $$=$1; }
| DoneStatement { $$=$1; }
| KilledStatement { $$ = $1; }
;

InterleavedConstruct: // 560
  InterleavedKeyword '{' InterleavedGuardList optError '}'
  {
    $$ = new Statement(Statement::S_INTERLEAVE, $3);
    $$->set_location(infile, @$);
  }
| InterleavedKeyword '{' error '}'
  {
    $$ = new Statement(Statement::S_INTERLEAVE, new AltGuards());
    $$->set_location(infile, @$);
  }
;

InterleavedGuardList: // 562
  optError InterleavedGuardElement
  {
    $$ = new AltGuards();
    $$->add_ag($2);
  }
| InterleavedGuardList optError InterleavedGuardElement
  { $$ = $1; $$->add_ag($3); }
;

InterleavedGuardElement: // 563
  '[' optError ']' GuardOp ';'
  {
    $$ = new AltGuard(0, $4, new StatementBlock());
    $$->set_location(infile, @$);
  }
| '[' optError ']' GuardOp optSemiColon StatementBlock optSemiColon
  {
    $$ = new AltGuard(0, $4, $6);
    $$->set_location(infile, @$);
  }
;

/* The following were folded into the above rule:

InterleavedGuardElement: // 563
  InterleavedGuard InterleavedAction
;

InterleavedGuard: // 564
  '[' ']' GuardOp
;

InterleavedAction: // 565
  StatementBlock
;
*/

LabelStatement: // 566
  LabelKeyword IDentifier
  {
    $$=new Statement(Statement::S_LABEL, $2);
    $$->set_location(infile, @$);
  }
;

GotoStatement: // 569
  GotoKeyword IDentifier
  {
    $$=new Statement(Statement::S_GOTO, $2);
    $$->set_location(infile, @$);
  }
| GotoKeyword AltKeyword
  {
    Location loc(infile, @$);
    loc.warning("Obsolete statement `goto alt' was substituted with `repeat'");
    $$=new Statement(Statement::S_REPEAT);
    $$->set_location(infile, @$);
  }
;

RepeatStatement: // 571
  RepeatKeyword
  {
    $$=new Statement(Statement::S_REPEAT);
    $$->set_location(infile, @$);
  }
;

ActivateOp: // 572
  ActivateKeyword '(' AltstepInstance optError ')'
  {
    $$.ref_pard = $3;
    $$.derefered_value = 0;
    $$.ap_list = 0;
  }
| ActivateKeyword '(' DereferOp '(' optFunctionActualParList ')' optError ')'
  {
    $5->set_location(infile, @4, @6);
    $$.ref_pard = 0;
    $$.derefered_value = $3;
    $$.ap_list = $5->steal_tis(); /* XXX perhaps propagate the datatype instead ? */
    delete $5;
  }
| ActivateKeyword '(' error ')'
  {
    $$.ref_pard = 0;
    $$.derefered_value = 0;
    $$.ap_list = 0;
  }
;

ReferOp:
  RefersKeyword '(' FunctionRef ')'
  {
    Ttcn::Reference* t_ref = new Ttcn::Reference($3.modid, $3.id);
    t_ref->set_location(infile, @3);
    $$ = new Value(Value::V_REFER, t_ref);
    $$->set_location(infile, @$);
  }
;

DeactivateStatement: // 574
  DeactivateKeyword
  {
    $$=new Statement(Statement::S_DEACTIVATE, static_cast<Value*>(0));
    $$->set_location(infile, @$);
  }
| DeactivateKeyword '(' Expression optError ')'
  {
    $$=new Statement(Statement::S_DEACTIVATE, $3);
    $$->set_location(infile, @$);
  }
| DeactivateKeyword '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$ = new Statement(Statement::S_DEACTIVATE, v);
    $$->set_location(infile, @$);
  }
;

BreakStatement: // 576
  BreakKeyword
  {
    $$=new Statement(Statement::S_BREAK);
    $$->set_location(infile, @$);
  }
;

ContinueStatement: // 577
  ContinueKeyword
  {
    $$=new Statement(Statement::S_CONTINUE);
    $$->set_location(infile, @$);
  }
;

/* A.1.6.8 Basic statements */

BasicStatements: // 578
  Assignment
  {
    $$=new Statement(Statement::S_ASSIGNMENT, $1);
    $$->set_location(infile, @$);
  }
| LogStatement { $$ = $1; }
| String2TtcnStatement { $$ = $1; }
| StatementBlock
  {
    $$ = new Statement(Statement::S_BLOCK, $1);
    $$->set_location(infile, @$);
  }
| TitanSpecificTryKeyword StatementBlock
  {
    $$ = new Statement(Statement::S_BLOCK, $2);
    $2->set_exception_handling(StatementBlock::EH_TRY);
    $$->set_location(infile, @$);
  }
| TitanSpecificCatchKeyword '(' IDentifier ')' StatementBlock
  {
    $$ = new Statement(Statement::S_BLOCK, $5);
    $5->set_exception_handling(StatementBlock::EH_CATCH);
    /* add a newly constructed first statement which will contain the error message,
       same as: 'var charstring IDentifier;' */
    Type* str_type = new Type(Type::T_CSTR);
    str_type->set_location(infile, @3);
    Def_Var* str_def = new Def_Var($3, str_type, 0);
    str_def->set_location(infile, @3);
    Statement* str_stmt = new Statement(Statement::S_DEF, str_def);
    str_stmt->set_location(infile, @3);
    $5->add_stmt(str_stmt, true);
    $$->set_location(infile, @$);
  }
| LoopConstruct { $$ = $1; }
| ConditionalConstruct { $$ = $1; }
| SelectCaseConstruct { $$ = $1; }
| SelectUnionConstruct { $$ = $1; }
;

Expression: // 579
  '(' optError Expression optError ')' { $$ = $3; }
| '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| '+' Expression %prec UnarySign
  {
    $$ = new Value(Value::OPTYPE_UNARYPLUS, $2);
    $$->set_location(infile, @$);
  }
| '-' Expression %prec UnarySign
  {
    $$ = new Value(Value::OPTYPE_UNARYMINUS, $2);
    $$->set_location(infile, @$);
  }
| Expression '*' Expression
  {
    $$ = new Value(Value::OPTYPE_MULTIPLY, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '/' Expression
  {
    $$ = new Value(Value::OPTYPE_DIVIDE, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression ModKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_MOD, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression RemKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_REM, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '+' Expression
  {
    $$ = new Value(Value::OPTYPE_ADD, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '-' Expression
  {
    $$ = new Value(Value::OPTYPE_SUBTRACT, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '&' Expression
  {
    $$ = new Value(Value::OPTYPE_CONCAT, $1, $3);
    $$->set_location(infile, @$);
  }
| Not4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_NOT4B, $2);
    $$->set_location(infile, @$);
  }
| Expression And4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_AND4B, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression Xor4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_XOR4B, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression Or4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_OR4B, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression SL Expression
  {
    $$ = new Value(Value::OPTYPE_SHL, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression SR Expression
  {
    $$ = new Value(Value::OPTYPE_SHR, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression RL Expression
  {
    $$ = new Value(Value::OPTYPE_ROTL, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression _RR Expression
  {
    $$ = new Value(Value::OPTYPE_ROTR, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '<' Expression
  {
    $$ = new Value(Value::OPTYPE_LT, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression '>' Expression
  {
    $$ = new Value(Value::OPTYPE_GT, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression LE Expression
  {
    $$ = new Value(Value::OPTYPE_LE, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression GE Expression
  {
    $$ = new Value(Value::OPTYPE_GE, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression EQ Expression
  {
    $$ = new Value(Value::OPTYPE_EQ, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression NE Expression
  {
    $$ = new Value(Value::OPTYPE_NE, $1, $3);
    $$->set_location(infile, @$);
  }
| NotKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_NOT, $2);
    $$->set_location(infile, @$);
  }
| Expression AndKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_AND, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression XorKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_XOR, $1, $3);
    $$->set_location(infile, @$);
  }
| Expression OrKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_OR, $1, $3);
    $$->set_location(infile, @$);
  }
| OpCall { $$ = $1; }
| Value { $$ = $1; }
| CompoundExpression { $$ = $1; }
/* These are needed for template concatenation */
| AnyValue
  {
    $$ = new Value(Value::V_ANY_VALUE, static_cast<LengthRestriction*>(NULL));
    $$->set_location(infile, @$);
  }
| AnyValue LengthMatch
  {
    $$ = new Value(Value::V_ANY_VALUE, $2);
    $$->set_location(infile, @$);
  }
| AnyOrOmit
  {
    $$ = new Value(Value::V_ANY_OR_OMIT, static_cast<LengthRestriction*>(NULL));
    $$->set_location(infile, @$);
  }
| AnyOrOmit LengthMatch
  {
    $$ = new Value(Value::V_ANY_OR_OMIT, $2);
    $$->set_location(infile, @$);
  }
;

CompoundExpression: // 565
  FieldExpressionList { $$ = $1; }
| ArrayExpressionList { $$ = $1; }
| ArrayExpression { $$ = $1; }
;

FieldExpressionList: // 581
  '{' seqFieldExpressionSpec optError '}'
  {
    $$ = new Value(Value::V_SEQ, $2);
    $$->set_location(infile, @$);
  }
;

seqFieldExpressionSpec:
  FieldExpressionSpec
  {
    $$ = new NamedValues();
    $$->add_nv($1);
  }
| error FieldExpressionSpec
  {
    $$ = new NamedValues();
    $$->add_nv($2);
  }
| seqFieldExpressionSpec optError ',' optError FieldExpressionSpec
  {
    $$ = $1;
    $$->add_nv($5);
  }
| seqFieldExpressionSpec optError ',' error { $$ = $1; }
;

FieldExpressionSpec: // 582
  FieldReference AssignmentChar NotUsedOrExpression
  {
    $$ = new NamedValue($1, $3);
    $$->set_location(infile, @$);
  }
;

ArrayExpressionList:
  '{' seqArrayExpressionSpec optError '}'
  {
    $$ = new Value(Value::V_SEQOF, $2);
    $$->set_location(infile, @$);
  }
;

seqArrayExpressionSpec:
  optError ArrayExpressionSpec
  {
    // The only place for indexed-list notation.
    $$ = new Values(true);
    $$->add_iv($2);
  }
| seqArrayExpressionSpec optError ',' optError ArrayExpressionSpec
  {
    $$ = $1;
    $$->add_iv($5);
  }
| seqArrayExpressionSpec optError ',' error { $$ = $1; }
;

ArrayExpressionSpec:
  ArrayOrBitRef AssignmentChar Expression
  {
    $$ = new IndexedValue($1, $3);
    $$->set_location(infile, @$);
  }
;

ArrayExpression: // 583
  '{' '}'
  {
    $$ = new Value(Value::V_SEQOF, new Values);
    $$->set_location(infile, @$);
  }
| '{' ArrayElementExpressionList optError '}'
  {
    $$ = new Value(Value::V_SEQOF, $2);
    $$->set_location(infile, @$);
  }
| '{' error '}'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
;

ArrayElementExpressionList: // 584
  NotUsedOrExpression
  {
    $$ = new Values;
    $$->add_v($1);
  }
| error NotUsedOrExpression
  {
    $$ = new Values;
    $$->add_v($2);
  }
| ArrayElementExpressionList optError ',' optError NotUsedOrExpression
  {
    $$ = $1;
    $$->add_v($5);
  }
| ArrayElementExpressionList optError ',' error { $$ = $1; }
;

NotUsedOrExpression: // 585
  Expression { $$ = $1; }
| NotUsedSymbol
  {
    $$ = new Value(Value::V_NOTUSED);
    $$->set_location(infile, @$);
  }
;

BooleanExpression: // 588
  Expression { $$ = $1; }
| error
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
;

Assignment: // 594
  VariableRef AssignmentChar TemplateBody
  { 
    $$ = new Ttcn::Assignment($1, $3); 
    $$->set_location(infile, @$);
  }
;

/* This can not be a single CompoundExpression (as opposed to Expression) */
SingleExpression: // 595
  '(' SingleExpression ')' { $$ = $2; }
| '(' error SingleExpression ')' { $$ = $3; }
| '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| '+' Expression %prec UnarySign
  {
    $$ = new Value(Value::OPTYPE_UNARYPLUS, $2);
    $$->set_location(infile, @$);
  }
| '-' Expression %prec UnarySign
  {
    $$ = new Value(Value::OPTYPE_UNARYMINUS, $2);
    $$->set_location(infile, @$);
  }
| SingleExpression '*' Expression
  {
    $$ = new Value(Value::OPTYPE_MULTIPLY, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '/' Expression
  {
    $$ = new Value(Value::OPTYPE_DIVIDE, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression ModKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_MOD, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression RemKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_REM, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '+' Expression
  {
    $$ = new Value(Value::OPTYPE_ADD, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '-' Expression
  {
    $$ = new Value(Value::OPTYPE_SUBTRACT, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '&' Expression
  {
    $$ = new Value(Value::OPTYPE_CONCAT, $1, $3);
    $$->set_location(infile, @$);
  }
| Not4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_NOT4B, $2);
    $$->set_location(infile, @$);
  }
| SingleExpression And4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_AND4B, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression Xor4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_XOR4B, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression Or4bKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_OR4B, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression SL Expression
  {
    $$ = new Value(Value::OPTYPE_SHL, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression SR Expression
  {
    $$ = new Value(Value::OPTYPE_SHR, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression RL Expression
  {
    $$ = new Value(Value::OPTYPE_ROTL, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression _RR Expression
  {
    $$ = new Value(Value::OPTYPE_ROTR, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '<' Expression
  {
    $$ = new Value(Value::OPTYPE_LT, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression '>' Expression
  {
    $$ = new Value(Value::OPTYPE_GT, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression LE Expression
  {
    $$ = new Value(Value::OPTYPE_LE, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression GE Expression
  {
    $$ = new Value(Value::OPTYPE_GE, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression EQ Expression
  {
    $$ = new Value(Value::OPTYPE_EQ, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression NE Expression
  {
    $$ = new Value(Value::OPTYPE_NE, $1, $3);
    $$->set_location(infile, @$);
  }
| NotKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_NOT, $2);
    $$->set_location(infile, @$);
  }
| SingleExpression AndKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_AND, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression XorKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_XOR, $1, $3);
    $$->set_location(infile, @$);
  }
| SingleExpression OrKeyword Expression
  {
    $$ = new Value(Value::OPTYPE_OR, $1, $3);
    $$->set_location(infile, @$);
  }
| OpCall { $$ = $1; }
| Value { $$ = $1; }
/* These are needed for template concatenation */
| AnyValue
  {
    $$ = new Value(Value::V_ANY_VALUE, static_cast<LengthRestriction*>(NULL));
    $$->set_location(infile, @$);
  }
| AnyValue LengthMatch
  {
    $$ = new Value(Value::V_ANY_VALUE, $2);
    $$->set_location(infile, @$);
  }
| AnyOrOmit
  {
    $$ = new Value(Value::V_ANY_OR_OMIT, static_cast<LengthRestriction*>(NULL));
    $$->set_location(infile, @$);
  }
| AnyOrOmit LengthMatch
  {
    $$ = new Value(Value::V_ANY_OR_OMIT, $2);
    $$->set_location(infile, @$);
  }
;

optExtendedFieldReference:
// perhaps this should be called seqExtendedFieldReference,
// but the convention appears to be that seq... can not be empty
  /* empty */
  {
    $$.nElements = 0;
    $$.elements = 0;
  }
| optExtendedFieldReference FieldOrArrayReference
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<FieldOrArrayRef**>(
      Realloc($1.elements, ($$.nElements) * sizeof(*$$.elements)) );
    $$.elements[$1.nElements] = $2;
  }
;

FieldOrArrayReference:
  '.' FieldIdentifier
  {
    $$ = new FieldOrArrayRef($2);
    $$->set_location(infile, @$);
  }
  | ArrayOrBitRefOrDash { $$ = $1; }
;

FieldIdentifier:
  PredefOrIdentifier { $$ = $1; }
| IDentifier /* maybe PredefOrIdentifier here too */ TypeActualParList
  {
    Location loc(infile, @$);
    loc.error("Reference to a parameterized field of type `anytype' is "
      "not currently supported");
    $$ = $1;
  }
;

OpCall: // 611
  ConfigurationOps { $$ = $1; }
| VerdictOps { $$ = $1; }
| TimerOps { $$ = $1; }
| TestcaseInstance
  {
    if ($1.ref_pard) $$ = new Value(Value::OPTYPE_EXECUTE, $1.ref_pard,
      $1.value);
    else if($1.derefered_value)
      $$ = new Value(Value::OPTYPE_EXECUTE_REFD, $1.derefered_value, $1.ap_list,
      $1.value);
    else $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| FunctionInstance optExtendedFieldReference
  {
    for (size_t i = 0; i < $2.nElements; i++) $1->add($2.elements[i]);
    Free($2.elements);
    $$ = new Value(Value::V_REFD, $1);
    $$->set_location(infile, @$);
  }
| ApplyOp
  {
    $$ = new Value(Value::V_INVOKE, $1.value, $1.ap_list);
    $$->set_location(infile, @$);
  }
| TemplateOps { $$ = $1; }
| PredefinedOps { $$ = $1; }
| ReferOp { $$ = $1; }
| ActivateOp
  {
    if ($1.ref_pard) $$ = new Value(Value::OPTYPE_ACTIVATE, $1.ref_pard);
    else if($1.derefered_value) $$ = new Value(Value::OPTYPE_ACTIVATE_REFD,
      $1.derefered_value, new ParsedActualParameters($1.ap_list));
    else $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| ProfilerRunningOp { $$ = $1; }
| PortOrAny DotCheckStateKeyword '(' SingleExpression ')'
  {
    if ($1.any_from) {
      Location loc(infile, @1);
      loc.error("The 'any from' clause cannot be used in a checkstate operation");
    }
    $$ = new Value(Value::OPTYPE_CHECKSTATE_ANY, $1.reference, $4);
    $$->set_location(infile, @$);
  }
// PortOrAll would cause a conflict 
| AllKeyword PortKeyword DotCheckStateKeyword '(' SingleExpression ')'
  {
    Ttcn::Reference *r = NULL;
    $$ = new Value(Value::OPTYPE_CHECKSTATE_ALL, r, $5);
    $$->set_location(infile, @$);
  }
;

PredefinedOps:
  PredefinedOpKeyword1 '(' optError Expression optError ')'
  {
    $$ = new Value($1, $4);
    $$->set_location(infile, @$);
  }
|  PredefinedOpKeyword1 '(' error ')'
  {
    Value *v1 = new Value(Value::V_ERROR);
    v1->set_location(infile, @3);
    $$ = new Value($1, v1);
    $$->set_location(infile, @$);
  }
| PredefinedOpKeyword2 '(' optError Expression optError ',' optError
  Expression optError ')'
  {
    $$ = new Value($1, $4, $8);
    $$->set_location(infile, @$);
  }
| PredefinedOpKeyword2 '(' error ')'
  {
    Value *v1 = new Value(Value::V_ERROR);
    v1->set_location(infile, @3);
    Value *v2 = new Value(Value::V_ERROR);
    v2->set_location(infile, @3);
    $$ = new Value($1, v1, v2);
    $$->set_location(infile, @$);
  }
| PredefinedOpKeyword3 '(' optError Expression optError ',' optError
  Expression optError ')'
  {
    $$ = new Value($1, $4, $8);
    $$->set_location(infile, @$);
  }
| PredefinedOpKeyword3 '(' optError Expression optError ')'
  {
    $$ = new Value($1, $4, static_cast<Common::Value*>(NULL));
    $$->set_location(infile, @$);
  }
| PredefinedOpKeyword3 '(' error ')'
  {
    Value *v1 = new Value(Value::V_ERROR);
    v1->set_location(infile, @3);
    Value *v2 = new Value(Value::V_ERROR);
    v2->set_location(infile, @3);
    $$ = new Value($1, v1, v2);
    $$->set_location(infile, @$);
  }
| decompKeyword '(' optError Expression optError ',' optError
  Expression optError ',' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_DECOMP, $4, $8, $12);
    $$->set_location(infile, @$);
  }
| decompKeyword '(' error ')'
  {
    Value *v1 = new Value(Value::V_ERROR);
    v1->set_location(infile, @3);
    Value *v2 = new Value(Value::V_ERROR);
    v2->set_location(infile, @3);
    Value *v3 = new Value(Value::V_ERROR);
    v3->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_DECOMP, v1, v2, v3);
    $$->set_location(infile, @$);
  }
| regexpKeyword '(' optError TemplateInstance optError ',' optError
  TemplateInstance optError ',' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_REGEXP, $4, $8, $12, false);
    $$->set_location(infile, @$);
  }
| regexpKeyword NocaseKeyword '(' optError TemplateInstance optError ',' optError
  TemplateInstance optError ',' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_REGEXP, $5, $9, $13, true);
    $$->set_location(infile, @$);
  }
| regexpKeyword '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @3);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @3);
    Template *t2 = new Template(Template::TEMPLATE_ERROR);
    t2->set_location(infile, @3);
    TemplateInstance *ti2 = new TemplateInstance(0, 0, t2);
    ti2->set_location(infile, @3);
    Value *v3 = new Value(Value::V_ERROR);
    v3->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_REGEXP, ti1, ti2, v3, false);
    $$->set_location(infile, @$);
  }
| regexpKeyword NocaseKeyword '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @4);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @4);
    Template *t2 = new Template(Template::TEMPLATE_ERROR);
    t2->set_location(infile, @4);
    TemplateInstance *ti2 = new TemplateInstance(0, 0, t2);
    ti2->set_location(infile, @4);
    Value *v3 = new Value(Value::V_ERROR);
    v3->set_location(infile, @4);
    $$ = new Value(Value::OPTYPE_REGEXP, ti1, ti2, v3, true);
    $$->set_location(infile, @$);
  }
| encvalueKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_ENCODE, $4);
    $$->set_location(infile, @$);
  }
| encvalueKeyword '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @3);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_ENCODE, ti1);
    $$->set_location(infile, @$);
  }
| substrKeyword '(' optError TemplateInstance optError ',' optError
  Expression optError ',' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_SUBSTR, $4, $8, $12);
    $$->set_location(infile, @$);
  }
| substrKeyword '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @3);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @3);
    Value *v2 = new Value(Value::V_ERROR);
    v2->set_location(infile, @3);
    Value *v3 = new Value(Value::V_ERROR);
    v3->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_SUBSTR, ti1, v2, v3);
    $$->set_location(infile, @$);
  }
| replaceKeyword '(' optError TemplateInstance optError ',' optError
  Expression optError ',' optError Expression optError ',' optError
  TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_REPLACE, $4, $8, $12, $16);
    $$->set_location(infile, @$);
  }
| replaceKeyword '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @3);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @3);
    Value *v2 = new Value(Value::V_ERROR);
    v2->set_location(infile, @3);
    Value *v3 = new Value(Value::V_ERROR);
    v3->set_location(infile, @3);
    Template *t4 = new Template(Template::TEMPLATE_ERROR);
    t4->set_location(infile, @3);
    TemplateInstance *ti4 = new TemplateInstance(0, 0, t4);
    ti4->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_REPLACE, ti1, v2, v3, ti4);
    $$->set_location(infile, @$);
  }
| decvalueKeyword '(' optError DecValueArg optError ',' optError DecValueArg optError ')'
  {
    $$ = new Value(Value::OPTYPE_DECODE, $4, $8);
    $$->set_location(infile, @$);
  }
| decvalueKeyword '(' error ')'
  {
    /*Value *v1 = new Value(Value::V_ERROR);
    v1->set_location(infile, @3);
    TemplateInstance *t2 = new TemplateInstance(Type::T_ERROR);*/
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| isvalueKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_ISVALUE, $4);
    $$->set_location(infile, @$);
  }
| isvalueKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| isboundKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_ISBOUND, $4);
    $$->set_location(infile, @$);
  }
| isboundKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| ischosenKeyword '(' optError IschosenArg optError ')'
  {
    $$ = new Value(Value::OPTYPE_ISCHOSEN, $4.ref, $4.id);
    $$->set_location(infile, @$);
  }
| ischosenKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| ispresentKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_ISPRESENT, $4);
    $$->set_location(infile, @$);
  }
| ispresentKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| istemplatekindKeyword '(' optError TemplateInstance optError ','
  optError SingleExpression optError ')'
  {
    $$ = new Value(Value::OPTYPE_ISTEMPLATEKIND, $4, $8);
    $$->set_location(infile, @$);
  }
| istemplatekindKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| lengthofKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_LENGTHOF, $4);
    $$->set_location(infile, @$);
  }
| lengthofKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| rndKeyword '(' ')'
  {
    $$ = new Value(Value::OPTYPE_RND);
    $$->set_location(infile, @$);
  }
| rndKeyword '(' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_RNDWITHVAL, $4);
    $$->set_location(infile, @$);
  }
| rndKeyword '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_RNDWITHVAL, v);
    $$->set_location(infile, @$);
  }
| sizeofKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_SIZEOF, $4);
    $$->set_location(infile, @$);
  }
| sizeofKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| log2strKeyword '(' ')'
  {
    $$ = new Value(Value::OPTYPE_LOG2STR, new LogArguments());
    $$->set_location(infile, @$);
  }
| log2strKeyword '(' LogItemList optError ')'
  {
    $$ = new Value(Value::OPTYPE_LOG2STR, $3);
    $$->set_location(infile, @$);
  }
| log2strKeyword '(' error ')'
  {
    $$ = new Value(Value::OPTYPE_LOG2STR, new LogArguments());
    $$->set_location(infile, @$);
  }
| any2unistrKeyWord '(' LogItemList optError ')'
  {
    if ($3->get_nof_logargs() != 1) {
      Location loc(infile, @1);
      loc.error("The any2unistr function takes exactly one argument, not %lu.",
        static_cast<unsigned long>($3->get_nof_logargs()));
        delete $3;
        $$ = new Value(Value::OPTYPE_ANY2UNISTR, new LogArguments());
        $$->set_location(infile, @$);
    } else {
      $$ = new Value(Value::OPTYPE_ANY2UNISTR, $3);
      $$->set_location(infile, @$);
    }
  }
| testcasenameKeyword '(' ')'
  {
    $$ = new Value(Value::OPTYPE_TESTCASENAME);
    $$->set_location(infile, @$);
  }
| ttcn2stringKeyword '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_TTCN2STRING, $4);
    $$->set_location(infile, @$);
  }
| ttcn2stringKeyword '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| encvalue_unicharKeyWord '(' optError TemplateInstance optError ',' optError
  Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_ENCVALUE_UNICHAR, $4, $8);
    $$->set_location(infile, @$);
  }
| encvalue_unicharKeyWord '(' optError TemplateInstance optError ')'
  {
    $$ = new Value(Value::OPTYPE_ENCVALUE_UNICHAR, $4);
    $$->set_location(infile, @$);
  }
| encvalue_unicharKeyWord '(' error ')'
  {
    Template *t1 = new Template(Template::TEMPLATE_ERROR);
    t1->set_location(infile, @3);
    TemplateInstance *ti1 = new TemplateInstance(0, 0, t1);
    ti1->set_location(infile, @3);
    $$ = new Value(Value::OPTYPE_ENCVALUE_UNICHAR, ti1);
    $$->set_location(infile, @$);
  }
| decvalue_unicharKeyWord '(' optError DecValueArg optError ',' optError
  DecValueArg optError ')'
  {
    $$ = new Value(Value::OPTYPE_DECVALUE_UNICHAR, $4, $8);
    $$->set_location(infile, @$);
  }
| decvalue_unicharKeyWord '(' optError DecValueArg optError ',' optError
  DecValueArg optError ',' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_DECVALUE_UNICHAR, $4, $8, $12);
    $$->set_location(infile, @$);
  }
| decvalue_unicharKeyWord '(' error ')'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
| hostidKeyWord '(' ')'
  {
    Value *null_value = NULL;
    $$ = new Value(Value::OPTYPE_HOSTID, null_value);
    $$->set_location(infile, @$);
  }
| hostidKeyWord '(' optError Expression optError ')'
  {
    $$ = new Value(Value::OPTYPE_HOSTID, $4);
    $$->set_location(infile, @$);
  }
;

DecValueArg:
  Reference
  {
    if ($1.is_ref) $$ = $1.ref;
    else {
      $$ = new Ttcn::Reference($1.id);
      $$->set_location(infile, @$);
    }
  }
| FunctionInstance { $$ = $1; }
;

PredefOrIdentifier:
  IDentifier { $$ = $1; }
| PredefinedType /* shall not be "anytype" */
  {
    // Construct an identifier "on the fly" ($1 here is just a typetype_t)
    const char* builtin_typename = Type::get_typename_builtin($1);
    if (0 == builtin_typename) FATAL_ERROR("Unexpected type %d", $1);
    const string& at_field = anytype_field(string(builtin_typename));

    $$ = new Identifier(Identifier::ID_TTCN, at_field);
  }
| NullValue
  {
    $$ = new Identifier(Identifier::ID_NAME, string("NULL"));
  }

IschosenArg: /* see also Reference... */
  IDentifier '.' PredefOrIdentifier
  {
    $$.ref = new Ttcn::Reference($1);
    $$.ref->set_location(infile, @1);
    $$.id = $3;
  }
| IDentifier '.' PredefOrIdentifier optExtendedFieldReference '.' PredefOrIdentifier
  {
    $$.ref = new Ttcn::Reference($1);
    FieldOrArrayRef *fieldref = new FieldOrArrayRef($3);
    fieldref->set_location(infile, @3);
    $$.ref->add(fieldref);
    for(size_t i=0; i<$4.nElements; i++) $$.ref->add($4.elements[i]);
    Free($4.elements);
    $$.ref->set_location(infile, @1, @4);
    $$.id = $6;
  }
| IDentifier ArrayOrBitRef optExtendedFieldReference '.' PredefOrIdentifier
  {
    $$.ref = new Ttcn::Reference($1);
    $$.ref->add($2);
    for(size_t i=0; i<$3.nElements; i++) $$.ref->add($3.elements[i]);
    Free($3.elements);
    $$.ref->set_location(infile, @1, @3);
    $$.id = $5;
  }
| IDentifier '.' ObjectIdentifierValue '.' IDentifier optExtendedFieldReference
  '.' PredefOrIdentifier
  {
    $$.ref = new Ttcn::Reference($1, $5);
    delete $3;
    for(size_t i=0; i<$6.nElements; i++) $$.ref->add($6.elements[i]);
    Free($6.elements);
    $$.ref->set_location(infile, @1, @6);
    $$.id = $8;
  }
;

PredefinedOpKeyword1:
  bit2hexKeyword { $$ = Value::OPTYPE_BIT2HEX; }
| bit2intKeyword { $$ = Value::OPTYPE_BIT2INT; }
| bit2octKeyword { $$ = Value::OPTYPE_BIT2OCT; }
| bit2strKeyword { $$ = Value::OPTYPE_BIT2STR; }
| char2intKeyword { $$ = Value::OPTYPE_CHAR2INT; }
| char2octKeyword { $$ = Value::OPTYPE_CHAR2OCT; }
| float2intKeyword { $$ = Value::OPTYPE_FLOAT2INT; }
| float2strKeyword { $$ = Value::OPTYPE_FLOAT2STR; }
| hex2bitKeyword { $$ = Value::OPTYPE_HEX2BIT; }
| hex2intKeyword { $$ = Value::OPTYPE_HEX2INT; }
| hex2octKeyword { $$ = Value::OPTYPE_HEX2OCT; }
| hex2strKeyword { $$ = Value::OPTYPE_HEX2STR; }
| int2charKeyword { $$ = Value::OPTYPE_INT2CHAR; }
| int2floatKeyword { $$ = Value::OPTYPE_INT2FLOAT; }
| int2strKeyword { $$ = Value::OPTYPE_INT2STR; }
| int2unicharKeyword { $$ = Value::OPTYPE_INT2UNICHAR; }
| oct2bitKeyword { $$ = Value::OPTYPE_OCT2BIT; }
| oct2charKeyword { $$ = Value::OPTYPE_OCT2CHAR; }
| oct2hexKeyword { $$ = Value::OPTYPE_OCT2HEX; }
| oct2intKeyword { $$ = Value::OPTYPE_OCT2INT; }
| oct2strKeyword { $$ = Value::OPTYPE_OCT2STR; }
| str2bitKeyword { $$ = Value::OPTYPE_STR2BIT; }
| str2floatKeyword { $$ = Value::OPTYPE_STR2FLOAT; }
| str2hexKeyword { $$ = Value::OPTYPE_STR2HEX; }
| str2intKeyword { $$ = Value::OPTYPE_STR2INT; }
| str2octKeyword { $$ = Value::OPTYPE_STR2OCT; }
| unichar2intKeyword { $$ = Value::OPTYPE_UNICHAR2INT; }
| unichar2charKeyword { $$ = Value::OPTYPE_UNICHAR2CHAR; }
| enum2intKeyword { $$ = Value::OPTYPE_ENUM2INT; }
| remove_bomKeyWord { $$ = Value::OPTYPE_REMOVE_BOM; }
| get_stringencodingKeyWord { $$ = Value::OPTYPE_GET_STRINGENCODING; }
| decode_base64KeyWord { $$ = Value::OPTYPE_DECODE_BASE64; }
;

PredefinedOpKeyword2:
  int2bitKeyword { $$ = Value::OPTYPE_INT2BIT; }
| int2hexKeyword { $$ = Value::OPTYPE_INT2HEX; }
| int2octKeyword { $$ = Value::OPTYPE_INT2OCT; }
;

PredefinedOpKeyword3:
  unichar2octKeyword { $$ = Value::OPTYPE_UNICHAR2OCT; }
| oct2unicharKeyword { $$ = Value::OPTYPE_OCT2UNICHAR; }
| encode_base64KeyWord { $$ = Value::OPTYPE_ENCODE_BASE64; }
;

String2TtcnStatement:
  string2ttcnKeyword '(' optError Expression optError ',' optError Reference optError ')'
  {
    Ttcn::Reference* out_ref;
    if ($8.is_ref) out_ref = $8.ref;
    else {
      out_ref = new Ttcn::Reference($8.id);
      out_ref->set_location(infile, @8);
    }
    $$ = new Statement(Statement::S_STRING2TTCN, $4, out_ref);
    $$->set_location(infile, @$);
  }
;

LogStatement: // 619
  LogKeyword '(' ')'
  {
    $$=new Statement(Statement::S_LOG, static_cast<LogArguments*>(0));
    $$->set_location(infile, @$);
  }
| LogKeyword '(' LogItemList optError ')'
  {
    $$=new Statement(Statement::S_LOG, $3);
    $$->set_location(infile, @$);
  }
| LogKeyword '(' error ')'
  {
    $$=new Statement(Statement::S_LOG, new LogArguments());
    $$->set_location(infile, @$);
  }
;

LogItemList:
  optError LogItem
  {
    $$ = new LogArguments();
    $$->add_logarg($2);
  }
| LogItemList optError ',' optError LogItem
  {
    $$ = $1;
    $$->add_logarg($5);
  }
| LogItemList optError ',' error { $$ = $1; }
;

LogItem: // 621
  TemplateInstance
  {
    $$ = new LogArgument($1);
    $$->set_location(infile, @$);
  }
;

LoopConstruct: // 622
  ForStatement { $$ = $1; }
| WhileStatement { $$ = $1; }
| DoWhileStatement { $$ = $1; }
;

ForStatement: // 623
  ForKeyword '(' Initial ';' Final ';' Step optError ')'
  StatementBlock
  {
    $$ = new Statement(Statement::S_FOR, $3.defs, $3.ass, $5, $7, $10);
    $$->set_location(infile, @$);
  }
;

Initial: // 625
  VarInstance
  {
    $$.defs = new Definitions;
    for (size_t i = 0; i < $1.nElements; i++) $$.defs->add_ass($1.elements[i]);
    Free($1.elements);
    $$.ass = 0;
  }
| Assignment
  {
    $$.defs = 0;
    $$.ass = $1;
  }
| error
  {
    $$.defs = new Definitions;
    $$.ass = 0;
  }
;

Final: // 626
  BooleanExpression { $$ = $1; }
;

Step: // 627
  Assignment { $$ = $1; }
/** \todo for-ban nem lehet null a step
| error { $$=NULL; }
*/
;

WhileStatement: // 628
  WhileKeyword '(' BooleanExpression ')' StatementBlock
{
  $$=new Statement(Statement::S_WHILE, $3, $5);
  $$->set_location(infile, @$);
}
;

DoWhileStatement: // 630
  DoKeyword StatementBlock
  WhileKeyword '(' BooleanExpression ')'
  {
    $$=new Statement(Statement::S_DOWHILE, $5, $2);
    $$->set_location(infile, @$);
  }
;

ConditionalConstruct: // 632
  IfKeyword '(' BooleanExpression ')'
  StatementBlock
  seqElseIfClause optElseClause
  {
    IfClause *ic=new IfClause($3, $5);
    ic->set_location(infile, @1, @5);
    $6->add_front_ic(ic);
    $$=new Statement(Statement::S_IF, $6, $7,
                     $7 ? new Location(infile, @7) : 0);
    $$->set_location(infile, @$);
  }
;

seqElseIfClause:
  /* empty */ { $$=new IfClauses(); }
| seqElseIfClause ElseIfClause
  {
    $$=$1;
    $$->add_ic($2);
  }
;

ElseIfClause: // 634
  ElseKeyword IfKeyword '(' BooleanExpression ')' StatementBlock
  {
    $$=new IfClause($4, $6);
    $$->set_location(infile, @$);
  }
;

optElseClause: // [636]
  /* empty */ { $$=0; }
| ElseKeyword StatementBlock { $$=$2; }
;

SelectCaseConstruct: // 637
  SelectKeyword '(' Expression optError ')' SelectCaseBody
  {
    $$=new Statement(Statement::S_SELECT, $3, $6);
    $$->set_location(infile, @$);
  }
| SelectKeyword '(' error ')' SelectCaseBody
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$=new Statement(Statement::S_SELECT, v, $5);
    $$->set_location(infile, @$);
  }
;

SelectCaseBody: // 639
  '{' seqSelectCase optError '}' {$$=$2;}
| '{' error '}' {$$=new SelectCases;}
;

seqSelectCase:
  optError SelectCase
  {
    $$=new SelectCases;
    $$->add_sc($2);
  }
| seqSelectCase optError SelectCase
  {
    $$=$1;
    $$->add_sc($3);
  }
;

SelectCase: // 640
  CaseKeyword '(' seqTemplateInstance optError ')' StatementBlock optSemiColon
  {
    $3->set_location(infile, @2, @5);
    $$=new SelectCase($3, $6);
    $$->set_location(infile, @$);
  }
| CaseKeyword '(' error ')' StatementBlock optSemiColon
  {
    TemplateInstances *tis = new TemplateInstances;
    tis->set_location(infile, @2, @4);
    $$ = new SelectCase(tis, $5);
    $$->set_location(infile, @$);
  }
| CaseKeyword ElseKeyword StatementBlock optSemiColon
  {
    $$=new SelectCase(0, $3);
    $$->set_location(infile, @$);
  }
;

seqTemplateInstance:
  optError TemplateInstance
  {
    $$ = new TemplateInstances;
    $$->add_ti($2);
  }
| seqTemplateInstance optError ',' optError TemplateInstance
  {
    $$ = $1;
    $$->add_ti($5);
  }
| seqTemplateInstance optError ',' error { $$ = $1; }
;


IdentifierListOrPredefType:
  optError IDentifier
  {
    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0].id = $2;
    $$.elements[0].yyloc = @2;
  }
| IdentifierListOrPredefType ',' optError IDentifier
  {
    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)));
    $$.elements[$1.nElements].id = $4;
    $$.elements[$1.nElements].yyloc = @4;
  }
| optError PredefinedType /* shall not be "anytype" */
  {
    // Construct an identifier "on the fly" ($2 here is just a typetype_t)
    const char* builtin_typename = Type::get_typename_builtin($2);
    if (0 == builtin_typename) FATAL_ERROR("Unexpected type %d", $2);
    const string& at_field = anytype_field(string(builtin_typename));
    Identifier* type_id = new Identifier(Identifier::ID_TTCN, at_field);

    $$.nElements = 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(Malloc(sizeof(*$$.elements)));
    $$.elements[0].id = type_id;
    $$.elements[0].yyloc = @2;
  }
| IdentifierListOrPredefType ',' optError PredefinedType
  {
     // Construct an identifier "on the fly" ($2 here is just a typetype_t)
    const char* builtin_typename = Type::get_typename_builtin($4);
    if (0 == builtin_typename) FATAL_ERROR("Unexpected type %d", $4);
    const string& at_field = anytype_field(string(builtin_typename));
    Identifier* type_id = new Identifier(Identifier::ID_TTCN, at_field);

    $$.nElements = $1.nElements + 1;
    $$.elements = static_cast<YYSTYPE::extconstidentifier_t*>(
      Realloc($1.elements, $$.nElements * sizeof(*$$.elements)));
    $$.elements[$1.nElements].id = type_id;
    $$.elements[$1.nElements].yyloc = @4;
  }
| IdentifierListOrPredefType ',' error { $$ = $1; }
;


SelectUnionConstruct:
  SelectKeyword UnionKeyword '(' SingleExpression optError ')' SelectUnionBody
  {
    $$=new Statement(Statement::S_SELECTUNION, $4, $7);
    $$->set_location(infile, @$);
  }
| SelectKeyword UnionKeyword '(' error ')' SelectUnionBody
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$=new Statement(Statement::S_SELECTUNION, v, $6);
    $$->set_location(infile, @$);
  }
;

SelectUnionBody:
  '{' seqSelectUnion optError '}' {$$=$2;}
| '{' seqSelectUnion optError SelectunionElse optError '}'
  {
    $$=$2;
    $$->add_su($4);
  }
| '{' error '}' {$$=new SelectUnions();}
;

seqSelectUnion:
  optError SelectUnion
  {
    $$=new SelectUnions();
    $$->add_su($2);
  }
| seqSelectUnion optError SelectUnion
  {
    $$=$1;
    $$->add_su($3);
  }
;

SelectUnion:
  CaseKeyword '(' IdentifierListOrPredefType optError ')' StatementBlock optSemiColon
  {
    $$=new SelectUnion($6);
    for (size_t i = 0; i < $3.nElements; i++) {
      $$->add_id($3.elements[i].id);
    }
    $$->set_location(infile, @3);
    Free($3.elements);
  }
| CaseKeyword '(' error ')' StatementBlock optSemiColon
  {
    $$ = new SelectUnion($5);
    $$->set_location(infile, @$);
  }
;

SelectunionElse:
  CaseKeyword ElseKeyword StatementBlock optSemiColon
  {
    $$=new SelectUnion($3);  // The else branch, ids is empty
    $$->set_location(infile, @2);
  }



/* A.1.6.9 Miscellaneous productions */

optSemiColon: // [645]
  /* empty */
| ';'
;

/* A.1 ASN.1 support, from ES 201 873-7 V3.1.1 (2005-06) */

optDefinitiveIdentifier:
  /* empty */
| DefinitiveIdentifier
;

DefinitiveIdentifier:
  '.' ObjectIdentifierKeyword '{' DefinitiveObjIdComponentList optError '}'
| '.' ObjectIdentifierKeyword '{' error '}'
;

DefinitiveObjIdComponentList:
  optError DefinitiveObjIdComponent
| DefinitiveObjIdComponentList optError DefinitiveObjIdComponent
;

DefinitiveObjIdComponent:
  NameForm
| DefinitiveNumberForm
| DefinitiveNameAndNumberForm
;

DefinitiveNumberForm:
  Number { delete $1; }
;

DefinitiveNameAndNumberForm:
  IDentifier '(' Number optError ')' { delete $1; delete $3; }
| IDentifier '(' error ')' { delete $1; }
;

ObjectIdentifierValue:
  ObjectIdentifierKeyword '{' ObjIdComponentList optError '}'
  {
    $$ = $3;
    $$->set_location(infile, @$);
  }
| ObjectIdentifierKeyword '{' error '}'
  {
    $$ = new Value(Value::V_ERROR);
    $$->set_location(infile, @$);
  }
;

ObjIdComponentList:
  optError ObjIdComponent
  {
    $$ = new Value(Value::V_OID);
    $$->add_oid_comp($2);
  }
| ObjIdComponentList optError ObjIdComponent
  {
    $$ = $1;
    $$->add_oid_comp($3);
  }
;

ObjIdComponent:
/* NameForm -- covered by NumberForm (as ReferencedValue) */
  NumberForm { $$ = $1; }
| NameAndNumberForm { $$ = $1; }
;

NumberForm:
  Number
  {
    Value *v = new Value(Value::V_INT, $1);
    v->set_location(infile, @1);
    $$ = new OID_comp(0, v);
    $$->set_location(infile, @$);
  }
| Reference
  {
    if ($1.is_ref) {
      /* it can be only a referenced value */
      Value *v = new Value(Value::V_REFD, $1.ref);
      v->set_location(infile, @1);
      $$ = new OID_comp(v);
    } else {
      /* it can be either a name form or a referenced value */
      $$ = new OID_comp($1.id, 0);
    }
    $$->set_location(infile, @$);
  }
;

NameAndNumberForm:
  IDentifier '(' Number optError ')'
  {
    Value *v = new Value(Value::V_INT, $3);
    v->set_location(infile, @3);
    $$ = new OID_comp($1, v);
    $$->set_location(infile, @$);
  }
| IDentifier '(' ReferencedValue optError ')'
  {
    $$ = new OID_comp($1, $3);
    $$->set_location(infile, @$);
  }
| IDentifier '(' error ')'
  {
    Value *v = new Value(Value::V_ERROR);
    v->set_location(infile, @3);
    $$ = new OID_comp($1, v);
    $$->set_location(infile, @$);
  }
;

NameForm:
  IDentifier { delete $1; }
;

/* Rules for error recovery */

optError:
  /* empty */
| error
;

optErrorBlock:
  optError
| optErrorBlock ErrorBlock optError
;

ErrorBlock:
  '{' error '}'
| '{' optError ErrorBlock optError '}'
;

%%

static void ttcn3_error(const char *str)
{
  Location loc(infile, ttcn3_lloc);
  if (*ttcn3_text) {
    // the most recently parsed token is known
    loc.error("at or before token `%s': %s", ttcn3_text, str);
  } else {
    // the most recently parsed token is unknown
    loc.error("%s", str);
  }
}

int ttcn3_parse_file(const char* filename, boolean generate_code)
{
  anytype_access = false;
  ttcn3_in = fopen(filename, "r");
  if (ttcn3_in == NULL) {
    ERROR("Cannot open input file `%s': %s", filename, strerror(errno));
    return -1;
  }
  infile = filename;
  init_ttcn3_lex();

  is_erroneous_parsed = false;
  NOTIFY("Parsing TTCN-3 module `%s'...", filename);

  int retval = ttcn3_parse();

  free_ttcn3_lex(); // does fclose(ttcn3_in);

  if (act_ttcn3_module) {
    act_ttcn3_module->set_location(filename);
    set_md5_checksum(act_ttcn3_module);
    if (generate_code) act_ttcn3_module->set_gen_code();
    modules->add_mod(act_ttcn3_module);
    act_ttcn3_module = 0;
  }

  act_group = 0;

  return retval;
}

Ttcn::ErroneousAttributeSpec* ttcn3_parse_erroneous_attr_spec_string(
  const char* p_str, const Common::Location& str_loc)
{
  is_erroneous_parsed = true;
  act_ttcn3_erroneous_attr_spec = NULL;
  string titan_err_str("$#&&&(#TITANERRONEOUS$#&&^#% ");
  int hack_str_len = static_cast<int>(titan_err_str.size());
  string *parsed_string = parse_charstring_value(p_str, str_loc);
  titan_err_str += *parsed_string;
  delete parsed_string;
  init_erroneous_lex(str_loc.get_filename(), str_loc.get_first_line(), str_loc.get_first_column()-hack_str_len+1);
  yy_buffer_state *flex_buffer = ttcn3__scan_string(titan_err_str.c_str());
  if (flex_buffer == NULL) {
    ERROR("Flex buffer creation failed.");
    return NULL;
  }
  yyparse();
  ttcn3_lex_destroy();
  free_dot_flag_stuff();

  return act_ttcn3_erroneous_attr_spec;
}

#ifndef NDEBUG
static void yyprint(FILE *file, int type, const YYSTYPE& value)
{
  switch (type) {
    case IDentifier:
      fprintf(file, "``%s''", value.id->get_name().c_str());
      break;
    case Number:
      fprintf(file, "%s", value.int_val->t_str().c_str());
      break;
    case FloatValue:
      fprintf(file, "%f", value.float_val);
      break;
    case Bstring:
    case Hstring:
    case Ostring:
    case BitStringMatch:
    case HexStringMatch:
    case OctetStringMatch:
      fprintf(file, "``%s''", value.string_val->c_str());
      break;
    case Cstring:
      fprintf(file, "``%s''", value.str);
      break;
    default:
      break;
  }
}
#endif

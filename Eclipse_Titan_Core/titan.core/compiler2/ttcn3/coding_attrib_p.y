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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
/* Parser for "extension" attributes of functions, external functions and
 * port types related to message encoding. */

%{

#include "../../common/dbgnew.hh"
#include "../string.hh"
#include "../Identifier.hh"
#include "../Setting.hh"
#include "../Type.hh"
#include "AST_ttcn3.hh"
#include "Attributes.hh"
#include "Ttcnstuff.hh"
#include "../stack.hh"

using namespace Ttcn;
using namespace Common;

/* Various C macros */

#define YYERROR_VERBOSE
#define yytext coding_attrib_text

/* C/C++ declarations */

extern char *yytext;
extern int yylex();
extern const char *coding_attrib_infile;

/* Init the lexer. Located in coding_attrib_la.l */
extern void init_coding_attrib_lex(const AttributeSpec& attrib);
extern void cleanup_coding_attrib_lex();

ExtensionAttributes *extatrs = 0;

static void yyerror(const char *str);

%}

/*********************************************************************
 * Bison declarations
 *********************************************************************/

%name-prefix="coding_attrib_"
%output="coding_attrib_p.cc"
%expect 0

/*********************************************************************
 * The union-type
 *********************************************************************/

%union {
  ErrorBehaviorList *errorbehaviorlist;
  ErrorBehaviorSetting *errorbehaviorsetting;
  PrintingType *printing;
  Identifier *id;
  Ttcn::Reference *reference;
  string *str;
  Type *type;
  Types *types;
  TypeMapping *typemapping;
  TypeMappings *typemappings;
  TypeMappingTarget *typemappingtarget;
  TypeMappingTargets *typemappingtargets;

  struct {
    Type::MessageEncodingType_t encoding_type;
    string *encoding_options;
  } encdec_attribute;

  struct {
    Type::MessageEncodingType_t encoding_type;
    string *encoding_options;
    ErrorBehaviorList *eb_list;
  } encdec_mapping;

  struct {
    TypeMappings *in_mappings, *out_mappings;
  } in_out_mappings;

  struct {
    Ttcn::Reference *port_type_ref;
    TypeMappings *in_mappings, *out_mappings;
  } user_attribute;

  Def_Function_Base::prototype_t prototype;
  Type::typetype_t typetype;
  Type::MessageEncodingType_t encoding_type;
  ExtensionAttributes *extattrs;
  ExtensionAttribute  *extattr;
  int number;
}

/*********************************************************************
 * Tokens with semantic value
 *********************************************************************/

%token <prototype> PrototypeSetting
%token <encoding_type> EncodingType
%token <str> EncodingOption
%token <str> ErrorBehaviorString
%token <id> IDentifier "Identifier"
%token <number> Number
%token <str> CustomEncoding

/*********************************************************************
 * Tokens without semantic value
 *********************************************************************/

/* TTCN-3 keywords */

%token AddressKeyword          "address"
%token AnyTypeKeyword          "anytype"
%token BitStringKeyword        "bitstring"
%token BooleanKeyword          "boolean"
%token CharStringKeyword       "charstring"
%token DefaultKeyword          "default"
%token EncodeKeyword           "encode"
%token FloatKeyword            "float"
%token FunctionKeyword         "function"
%token HexStringKeyword        "hexstring"
%token InKeyword               "in"
%token IntegerKeyword          "integer"
%token ObjectIdentifierKeyword "objid"
%token OctetStringKeyword      "octetstring"
%token OutKeyword              "out"
%token UniversalKeyword        "universal"
%token VerdictTypeKeyword      "verdicttype"
%token VersionKeyword          "version"
%token RequiresKeyword         "requires"
%token ReqTitanKeyword         "requiresTitan"

/* Non-standard keywords used by the attributes with context-sensitive
 * recognition */

%token DecodeKeyword        "decode"
%token DiscardKeyword       "discard"
%token ErrorBehaviorKeyword "errorbehavior"
%token InternalKeyword      "internal"
%token PrototypeKeyword     "prototype"
%token ProviderKeyword      "provider"
%token SimpleKeyword        "simple"
%token UserKeyword          "user"
%token TransparentKeyword   "transparent"
%token PrintingKeyword      "printing"

%token CompactKeyword       "compact"
%token PrettyKeyword        "pretty"

%token RedirectSymbol "->"

%token '.'
%token '<'
%token '>'
%token '/'

/*********************************************************************
 * Semantic types of nonterminals
 *********************************************************************/

%type <errorbehaviorsetting> ErrorBehaviorSetting
%type <errorbehaviorlist> ErrorBehaviorSettingList ErrorBehaviorAttribute
%type <printing> PrintingAttribute PrintingType
%type <reference> FunctionMapping FunctionReference PortTypeReference
  ReferencedType
%type <str> EncodingOptions
%type <type> Type
%type <types> TypeList
%type <typemapping> TypeMapping
%type <typemappings> TypeMappingList
%type <typemappingtarget> TypeMappingTarget
%type <typemappingtargets> TypeMappingTargetList

%type <encdec_attribute> DecodeAttribute EncDecAttributeBody EncodeAttribute
%type <encdec_mapping> DecodeMapping EncodeMapping
%type <in_out_mappings> InOutTypeMapping InOutTypeMappingList
%type <user_attribute> UserAttribute

%type <prototype> PrototypeAttribute
%type <typetype> PredefinedType

%type <extattr> ExtensionAttribute ExternalFunctionAttribute
 PortTypeAttribute AnyTypeAttribute EncDecValueAttribute
 VersionAttribute RequiresAttribute TransparentAttribute

%type <extattrs> ExtensionAttributes

/*********************************************************************
 * Destructors
 *********************************************************************/

%destructor { delete $$; }
EncodingOption
EncodingOptions
ErrorBehaviorAttribute
ErrorBehaviorSetting
ErrorBehaviorSettingList
ErrorBehaviorString
FunctionMapping
FunctionReference
IDentifier
PortTypeReference
ReferencedType
Type
TypeList
TypeMapping
TypeMappingList
TypeMappingTarget
TypeMappingTargetList
ExtensionAttribute
ExtensionAttributes
ExternalFunctionAttribute
PortTypeAttribute
AnyTypeAttribute
EncDecValueAttribute
VersionAttribute
RequiresAttribute
PrintingAttribute
PrintingType
CustomEncoding

%destructor { delete $$.encoding_options; }
DecodeAttribute
EncDecAttributeBody
EncodeAttribute

%destructor {
  delete $$.encoding_options;
  delete $$.eb_list;
}
DecodeMapping
EncodeMapping

%destructor {
  delete $$.in_mappings;
  delete $$.out_mappings;
}
InOutTypeMapping
InOutTypeMappingList

%destructor {
  delete $$.port_type_ref;
  delete $$.in_mappings;
  delete $$.out_mappings;
}
UserAttribute

%expect 4

%%

/*********************************************************************
 * Grammar
 *********************************************************************/

GrammarRoot:
  ExtensionAttributes
  {
    if (extatrs == 0) extatrs = $1;
    else {
      extatrs->import($1);
      delete $1;
    }
  }
;

ExtensionAttributes:
  /* empty */
  {
    $$ = new ExtensionAttributes;
  }
| ExtensionAttributes ExtensionAttribute
  {
    $$=$1;
    if ($2 != NULL) $$->add($2);
  }
;

ExtensionAttribute:
  ExternalFunctionAttribute
| PortTypeAttribute
| AnyTypeAttribute
| EncDecValueAttribute
| VersionAttribute
| RequiresAttribute
| TransparentAttribute
;

VersionAttribute:
VersionKeyword IDentifier
{ // version   R2D2
  $$ = new ExtensionAttribute(NULL, 0, 0, 0, $2); // VERSION
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s'", $2->get_name().c_str());
    delete $2;
    delete $$;
    $$ = NULL;
  }
}
| VersionKeyword '<' IDentifier '>'
{ // version      <RnXnn>
  $$ = new ExtensionAttribute(NULL, 0, 0, 0, $3, ExtensionAttribute::VERSION_TEMPLATE);
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version template '<%s>'", $3->get_name().c_str());
    delete $3;
    delete $$;
    $$ = NULL;
  }
}
| VersionKeyword IDentifier Number Number IDentifier
{ // version     CNL        113    200    R2D2
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, 0, $5); // VERSION
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d %s'",
      $2->get_dispname().c_str(), $3, $4, $5->get_dispname().c_str());
    delete $5;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
| VersionKeyword IDentifier Number Number '<' IDentifier '>'
{ // version     CNL        113    200    <RnXnn>
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, 0, $6, ExtensionAttribute::VERSION_TEMPLATE); // VERSION
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d <%s>'",
      $2->get_dispname().c_str(), $3, $4, $6->get_dispname().c_str());
    delete $6;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
| VersionKeyword IDentifier Number Number '/' Number IDentifier
{ // version     CNL        113    200     /  1      R2D2
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, $6, $7); // VERSION
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d / %d %s'",
      $2->get_dispname().c_str(), $3, $4, $6, $7->get_dispname().c_str());
    delete $7;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
| VersionKeyword IDentifier Number Number '/' Number '<' IDentifier '>'
{ // version     CNL        113    200     /  1      <RnXnn>
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, $6, $8, ExtensionAttribute::VERSION_TEMPLATE); // VERSION
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d / %d <%s>'",
      $2->get_dispname().c_str(), $3, $4, $6, $8->get_dispname().c_str());
    delete $8;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
;

RequiresAttribute:
RequiresKeyword IDentifier IDentifier
{ //            module     R1B
  $$ = new ExtensionAttribute($2, NULL, 0, 0, 0, $3); // REQUIRES
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    /* parsing the version has failed */
    $$->error("Incorrect version data '%s'", $3->get_name().c_str());
    delete $2;
    delete $3;
    delete $$;
    $$ = NULL;
  }
}
| RequiresKeyword IDentifier IDentifier Number Number IDentifier
{ //              module     CNL        xxx    xxx    R1A
  $$ = new ExtensionAttribute($2, $3->get_dispname().c_str(), $4, $5, 0, $6); // REQUIRES
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d %s'",
      $3->get_dispname().c_str(), $4, $5, $6->get_dispname().c_str());
    delete $2;
    delete $6;
    delete $$;
    $$ = NULL;
  }
  delete $3;
}
| RequiresKeyword IDentifier IDentifier Number Number '/' Number IDentifier
{ //              module     CNL        xxx    xxx     /  1      R9A
  $$ = new ExtensionAttribute($2, $3->get_dispname().c_str(), $4, $5, $7, $8); // REQUIRES
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d / %d %s'",
      $3->get_dispname().c_str(), $4, $5, $7, $8->get_dispname().c_str());
    delete $2;
    delete $8;
    delete $$;
    $$ = NULL;
  }
  delete $3;
}
| ReqTitanKeyword IDentifier
{ //              R1A
  $$ = new ExtensionAttribute(NULL, 0, 0, 0, $2, ExtensionAttribute::REQ_TITAN);
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    /* parsing the version has failed */
    $$->error("Incorrect version data '%s'", $2->get_name().c_str());
    delete $2;
    delete $$;
    $$ = NULL;
  }
}
| ReqTitanKeyword IDentifier Number Number IDentifier
{ //              CRL        113    200    R1A
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, 0, $5, ExtensionAttribute::REQ_TITAN);
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d %s'",
      $2->get_dispname().c_str(), $3, $4, $5->get_dispname().c_str());
    delete $5;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
| ReqTitanKeyword IDentifier Number Number '/' Number IDentifier
{ //              CRL        113    200     /  2      R1A
  $$ = new ExtensionAttribute($2->get_dispname().c_str(), $3, $4, $6, $7, ExtensionAttribute::REQ_TITAN);
  $$->set_location(coding_attrib_infile, @$);
  if ($$->get_type() == ExtensionAttribute::NONE) {
    $$->error("Incorrect version data '%s %d %d / %d %s'",
      $2->get_dispname().c_str(), $3, $4, $6, $7->get_dispname().c_str());
    delete $7;
    delete $$;
    $$ = NULL;
  }
  delete $2;
}
;

TransparentAttribute: TransparentKeyword
{
  $$ = new ExtensionAttribute(ExtensionAttribute::TRANSPARENT);
  $$->set_location(coding_attrib_infile, @$);
}
;

EncDecValueAttribute:
  InOutTypeMappingList
  {
    $$ = new ExtensionAttribute($1.in_mappings, $1.out_mappings); // ENCDECVALUE
    $$->set_location(coding_attrib_infile, @$);
  }
;

/* FunctionAttribute is covered by the PrototypeAttribute branch */
ExternalFunctionAttribute:
  PrototypeAttribute
  {
    $$ = new ExtensionAttribute($1); // PROTOTYPE
    $$->set_location(coding_attrib_infile, @$);
  }
| EncodeAttribute
  {
    $$ = new ExtensionAttribute(ExtensionAttribute::ENCODE,
      $1.encoding_type, $1.encoding_options);
    $$->set_location(coding_attrib_infile, @$);
  }
| DecodeAttribute
  {
    $$ = new ExtensionAttribute(ExtensionAttribute::DECODE,
      $1.encoding_type, $1.encoding_options);
    $$->set_location(coding_attrib_infile, @$);
  }
| ErrorBehaviorAttribute
  {
    $$ = new ExtensionAttribute($1); // ERRORBEHAVIOR
    $$->set_location(coding_attrib_infile, @$);
  }
| PrintingAttribute
  {
    $$ = new ExtensionAttribute($1); // PRINTING
    $$->set_location(coding_attrib_infile, @$);
  }
;

PortTypeAttribute:
  InternalKeyword
  {
    $$ = new ExtensionAttribute(PortTypeBody::TP_INTERNAL); // PORT_API
    $$->set_location(coding_attrib_infile, @$);
  }
| AddressKeyword
  {
    $$ = new ExtensionAttribute(PortTypeBody::TP_ADDRESS); // PORT_API
    $$->set_location(coding_attrib_infile, @$);
  }
| ProviderKeyword
  {
    $$ = new ExtensionAttribute(ExtensionAttribute::PORT_TYPE_PROVIDER);
    $$->set_location(coding_attrib_infile, @$);
  }
| UserAttribute
  {
    $$ = new ExtensionAttribute($1.port_type_ref,
      $1.in_mappings, $1.out_mappings); // PORT_TYPE_USER
    $$->set_location(coding_attrib_infile, @$);
  }
;

PrototypeAttribute:
  PrototypeKeyword '(' PrototypeSetting ')' { $$ = $3; }
;

EncodeAttribute:
  EncodeKeyword EncDecAttributeBody { $$ = $2; }
;

DecodeAttribute:
  DecodeKeyword EncDecAttributeBody { $$ = $2; }
;

EncDecAttributeBody:
  '(' EncodingType ')'
  {
    $$.encoding_type = $2;
    $$.encoding_options = 0;
  }
| '(' EncodingType ':' EncodingOptions ')'
  {
    $$.encoding_type = $2;
    $$.encoding_options = $4;
  }
| '(' CustomEncoding ')'
  {
    $$.encoding_type = Type::CT_CUSTOM;
    $$.encoding_options = $2;
  }
;

EncodingOptions:
  EncodingOption { $$ = $1; }
| EncodingOptions EncodingOption
  {
    $$ = $1;
    *$$ += ' ';
    *$$ += *$2;
    delete $2;
  }
;

ErrorBehaviorAttribute:
  ErrorBehaviorKeyword '(' ErrorBehaviorSettingList ')'
  {
    $$ = $3;
    $$->set_location(coding_attrib_infile, @$);
  }
;

ErrorBehaviorSettingList:
  ErrorBehaviorSetting
  {
    $$ = new ErrorBehaviorList();
    $$->add_ebs($1);
  }
| ErrorBehaviorSettingList ',' ErrorBehaviorSetting
  {
    $$ = $1;
    $$->add_ebs($3);
  }
;

ErrorBehaviorSetting:
  ErrorBehaviorString ':' ErrorBehaviorString
  {
    $$ = new ErrorBehaviorSetting(*$1, *$3);
    $$->set_location(coding_attrib_infile, @$);
    delete $1;
    delete $3;
  }
;

PrintingAttribute:
  PrintingKeyword '(' PrintingType ')' 
  {
    $$ = $3;
    $$->set_location(coding_attrib_infile, @$);
  }
;

PrintingType:
  CompactKeyword 
  { 
    $$ = new PrintingType();
    $$->set_printing(PrintingType::PT_COMPACT); 
  }
| PrettyKeyword 
  { 
    $$ = new PrintingType();
    $$->set_printing(PrintingType::PT_PRETTY); 
  }
;

UserAttribute:
  UserKeyword PortTypeReference InOutTypeMappingList
  {
    $$.port_type_ref = $2;
    $$.in_mappings = $3.in_mappings;
    $$.out_mappings = $3.out_mappings;
  }
;

PortTypeReference:
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(coding_attrib_infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(coding_attrib_infile, @$);
  }
;

InOutTypeMappingList:
  InOutTypeMapping { $$ = $1; }
| InOutTypeMappingList InOutTypeMapping
  {
    if ($1.in_mappings) {
      $$.in_mappings = $1.in_mappings;
      if ($2.in_mappings) {
	$$.in_mappings->steal_mappings($2.in_mappings);
	delete $2.in_mappings;
      }
    } else $$.in_mappings = $2.in_mappings;
    if ($1.out_mappings) {
      $$.out_mappings = $1.out_mappings;
      if ($2.out_mappings) {
	$$.out_mappings->steal_mappings($2.out_mappings);
	delete $2.out_mappings;
      }
    } else $$.out_mappings = $2.out_mappings;
  }
;

InOutTypeMapping:
  InKeyword '(' TypeMappingList ')'
  {
    $$.in_mappings = $3;
    $$.in_mappings->set_location(coding_attrib_infile, @$);
    $$.out_mappings = 0;
  }
| OutKeyword '(' TypeMappingList ')'
  {
    $$.in_mappings = 0;
    $$.out_mappings = $3;
    $$.out_mappings->set_location(coding_attrib_infile, @$);
  }
;

TypeMappingList:
  TypeMapping
  {
    $$ = new TypeMappings();
    $$->add_mapping($1);
  }
| TypeMappingList ';' TypeMapping
  {
    $$ = $1;
    $$->add_mapping($3);
  }
;

TypeMapping:
  Type RedirectSymbol TypeMappingTargetList
  {
    $$ = new TypeMapping($1, $3);
    $$->set_location(coding_attrib_infile, @$);
  }
;

TypeMappingTargetList:
  TypeMappingTarget
  {
    $$ = new TypeMappingTargets();
    $$->add_target($1);
  }
| TypeMappingTargetList ',' TypeMappingTarget
  {
    $$ = $1;
    $$->add_target($3);
  }
;

TypeMappingTarget:
  Type ':' SimpleKeyword
  {
    $$ = new TypeMappingTarget($1, TypeMappingTarget::TM_SIMPLE);
    $$->set_location(coding_attrib_infile, @$);
  }
| '-' ':' DiscardKeyword
  {
    $$ = new TypeMappingTarget(0, TypeMappingTarget::TM_DISCARD);
    $$->set_location(coding_attrib_infile, @$);
  }
| Type ':' FunctionMapping
  {
    $$ = new TypeMappingTarget($1, TypeMappingTarget::TM_FUNCTION, $3);
    $$->set_location(coding_attrib_infile, @$);
  }
| Type ':' EncodeMapping
  {
    $$ = new TypeMappingTarget($1, TypeMappingTarget::TM_ENCODE,
      $3.encoding_type, $3.encoding_options, $3.eb_list);
    $$->set_location(coding_attrib_infile, @$);
  }
| Type ':' DecodeMapping
  {
    $$ = new TypeMappingTarget($1, TypeMappingTarget::TM_DECODE,
      $3.encoding_type, $3.encoding_options, $3.eb_list);
    $$->set_location(coding_attrib_infile, @$);
  }
;

FunctionMapping:
  FunctionKeyword '(' FunctionReference ')' { $$ = $3; }
;

FunctionReference:
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(coding_attrib_infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(coding_attrib_infile, @$);
  }
;

EncodeMapping:
  EncodeAttribute
  {
    $$.encoding_type = $1.encoding_type;
    $$.encoding_options = $1.encoding_options;
    $$.eb_list = 0;
  }
| EncodeAttribute ErrorBehaviorAttribute
  {
    $$.encoding_type = $1.encoding_type;
    $$.encoding_options = $1.encoding_options;
    $$.eb_list = $2;
  }
;

DecodeMapping:
  DecodeAttribute
  {
    $$.encoding_type = $1.encoding_type;
    $$.encoding_options = $1.encoding_options;
    $$.eb_list = 0;
  }
| DecodeAttribute ErrorBehaviorAttribute
  {
    $$.encoding_type = $1.encoding_type;
    $$.encoding_options = $1.encoding_options;
    $$.eb_list = $2;
  }
;

Type:
  PredefinedType
  {
    $$ = new Type($1);
    $$->set_location(coding_attrib_infile, @$);
  }
| ReferencedType
  {
    $$ = new Type(Type::T_REFD, $1);
    $$->set_location(coding_attrib_infile, @$);
  }
;

PredefinedType: /* typetype_t */
  BitStringKeyword { $$ = Type::T_BSTR; }
| BooleanKeyword { $$ = Type::T_BOOL; }
| CharStringKeyword { $$ = Type::T_CSTR; }
| UniversalKeyword CharStringKeyword { $$ = Type::T_USTR; }
| IntegerKeyword { $$ = Type::T_INT; }
| OctetStringKeyword { $$ = Type::T_OSTR; }
| HexStringKeyword { $$ = Type::T_HSTR; }
| VerdictTypeKeyword { $$ = Type::T_VERDICT; }
| FloatKeyword { $$ = Type::T_REAL; }
| AddressKeyword { $$ = Type::T_ADDRESS; }
| DefaultKeyword { $$ = Type::T_DEFAULT; }
| AnyTypeKeyword
  {
    Location loc(coding_attrib_infile, @$);
    loc.error("Type `anytype' as a field of `anytype' is not supported");
    $$ = Type::T_ERROR;
  }
| ObjectIdentifierKeyword { $$ = Type::T_OID; }
;

ReferencedType: /* a Reference* */
  IDentifier
  {
    $$ = new Ttcn::Reference($1);
    $$->set_location(coding_attrib_infile, @$);
  }
| IDentifier '.' IDentifier
  {
    $$ = new Ttcn::Reference($1, $3);
    $$->set_location(coding_attrib_infile, @$);
  }
;


AnyTypeAttribute:
  AnyTypeKeyword TypeList
  {
    $$ = new ExtensionAttribute($2); // ANYTYPELIST
    $$->set_location(coding_attrib_infile, @$);
  }


TypeList:
  Type
  {
    $$ = new Types;
    $$->add_type($1);
    $$->set_location(coding_attrib_infile, @$);
  }
| TypeList ',' Type
  {
    $$ = $1;
    $$->add_type($3);
    $$->set_location(coding_attrib_infile, @$);
  }
;

%%

static void yyerror(const char *str)
{
  Location loc(coding_attrib_infile, yylloc);
  if (*yytext) {
    // the most recently parsed token is known
    loc.error("at or before token `%s': %s", yytext, str);
  } else {
    // the most recently parsed token is unknown
    loc.error("%s", str);
  }
}

/** Parse all extension attributes in a "with" statement */
ExtensionAttributes * parse_extattributes(WithAttribPath *w_attrib_path)
{
  extatrs = 0;
  if (!w_attrib_path) FATAL_ERROR("parse_extattributes(): NULL pointer");
  // Collect attributes from outer scopes
  vector<SingleWithAttrib> const& real_attribs =
    w_attrib_path->get_real_attrib();
  const size_t nof_attribs = real_attribs.size();

  for (size_t i = 0; i < nof_attribs; i++) {
    SingleWithAttrib *attrib = real_attribs[i];
    if (attrib->get_attribKeyword() == SingleWithAttrib::AT_EXTENSION) {
      Qualifiers *qualifiers = attrib->get_attribQualifiers();
      if (!qualifiers || qualifiers->get_nof_qualifiers() == 0) {
        // Only care about extension attributes without qualifiers
        Error_Context cntxt(attrib, "In `extension' attribute");
        init_coding_attrib_lex(attrib->get_attribSpec());
        int result = yyparse(); /* 0=ok, 1=error, 2=out of memory */
        (void)result;
        cleanup_coding_attrib_lex();
      } // if no qualifiers
    } // if AT_EXTENSION
  } // for

  return extatrs;
}

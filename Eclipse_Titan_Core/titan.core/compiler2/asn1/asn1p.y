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
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
/*
 * Asn1:1997 parser
 *
 * Written by Matyas Forstner
 * 20021119
 */

%{

/*********************************************************************
 * C(++) declarations
 *********************************************************************/

/* These declarations, intended for asn1p.tab.cc, are written by Bison
 * into asn1p.tab.hh, which is included from multiple C++ files.
 * YYBISON is defined in asn1p.tab.cc */
#ifdef YYBISON

#include <errno.h>
#include <limits.h>

#include "../Identifier.hh"
#include "AST_asn1.hh"
#include "Ref.hh"
#include "../Value.hh"
#include "../Valuestuff.hh"
#include "../Type.hh"
#include "../Typestuff.hh" // FIXME CTs
#include "../CompField.hh"
#include "../EnumItem.hh"
#include "Tag.hh"
#include "TableConstraint.hh"
#include "Object.hh"
#include "Block.hh"
#include "../main.hh"

using namespace Asn;
using namespace Common;

extern Modules *modules;

extern const char *asn1_infile;
extern bool asn1_yy_parse_internal;
extern void asn1la_newfile(const char *filename);

/* defined in lexer c-file */
extern FILE *asn1_yyin;
#define yyin asn1_yyin
extern int asn1_yylineno;
#define yylineno asn1_yylineno
int asn1_plineno;
#define plineno asn1_plineno
/* copy-paste from xxxxx.yy.cc */
#define YY_BUF_SIZE 16384
extern int asn1_yylex_my();
#define asn1_yylex asn1_yylex_my
struct yy_buffer_state;
extern yy_buffer_state *asn1_yy_create_buffer(FILE *file, int size);
extern void asn1_yy_switch_to_buffer(yy_buffer_state *new_buffer);
extern int asn1_yylex_destroy  (void);
extern yy_buffer_state *asn1_yy_scan_string(const char *yy_str);

static Asn::Module *act_asn1_module = NULL;

#define YYERROR_VERBOSE

void yyerror(const char *s)
{
  Location loc(asn1_infile, plineno);
  loc.error("%s", s);
}

void yywarning(const char *s)
{
  Location loc(asn1_infile, plineno);
  loc.warning("%s", s);
}

void yynote(const char *s)
{
  Location loc(asn1_infile, plineno);
  loc.note("%s", s);
}

#endif /* YYBISON */
%}

/*********************************************************************
 * Bison declarations
 *********************************************************************/

%name-prefix="asn1_yy"
%output="asn1p.tab.cc"
%defines
%verbose
%glr-parser
/**
   shift-reduce conflicts:
   2: in IMPORTS clause
     SymbolsFromModule:
       SymbolList KW_FROM TOK_UpperIdentifier . AssignedIdentifier

   1: in upper references (Upper_FromObjs and ExtUpperRef)
     "A" . "." "zzz" <--> "A" . "." "&zzz"

   1: in constraints: "(" lowerid "<" . ...
      selectiontype or (minvalue < .. maxvalue)
*/
%expect 4
%start GrammarRoot

/*********************************************************************
 * The union-type
 *********************************************************************/

%union {
  /** The Asn namespace must be explicitly used here because this block
   * is also visible from the common header files. */

  bool b;
  Common::Int _i;
  Common::int_val_t *i;
  string *str;

  Common::Node *node;
  Common::Identifier *id;
  Asn::Ref_defd_simple *ref_ds;
  Asn::Ref_defd *ref_d;
  Asn::FieldName *fieldname;
  Asn::FromObj *fromobj;
  Asn::Reference *ref;

  Asn::Module *module;
  Asn::TagDefault::tagdef_t tagging;
  struct {
    Asn::Exports *exp;
    Asn::Imports *imp;
    Asn::Assignments *asss;
  } modbody;
  Asn::Symbols *syms;
  Asn::Exports *exports;
  Asn::ImpMod *impmod;
  Asn::Imports *impmods;
  Asn::Assignments *asss;
  Asn::Assignment *ass;
  Asn::Ass_pard *ass_pard;

  Common::Block *block;

  /* type stuff */
  Common::Type *type;
  struct {
    Common::Identifier *name;
    Common::Type *type;
  } namedType;
  Common::Tag *tag;
  Common::Tag::tagclass_t tagclass;

  Common::EnumItem *enumitem;
  Common::EnumItems *enumitems;
  Common::ExcSpec *excSpec;
  Common::CTs_EE_CTs *ctss;
  Common::CTs *cts;
  Common::ExtAndExc *extAndExc;
  Common::ExtAdds *extAdds;
  Common::ExtAdd *extAdd;
  Common::CT *ct;
  Common::CompField *compField;
  Common::Constraint *constraint;
  Common::RangeEndpoint *rangeendpoint;
  Common::Constraints *constraints;

  /* sub-type constraints */
  Common::ContainedSubtypeConstraint* contsubt__constraint;
  Common::ValueRangeConstraint* valuerange_constraint;
  Common::SizeConstraint* size_constraint;
  Common::PermittedAlphabetConstraint* permalpha_constraint;
  Common::SetOperationConstraint* setoperation_constraint;
  Common::PatternConstraint* pattern_constraint;
  Common::SingleInnerTypeConstraint* single_inner_constraint;
  Common::MultipleInnerTypeConstraint* multiple_inner_constraint;
  Common::NamedConstraint* named_constraint;
  Common::NamedConstraint::presence_constraint_t presence_constraint;

  /* value stuff */
  Common::Value *value;
  Common::Values *values;
  Common::NamedValue *namedValue;
  Common::NamedValues *namedValues;
  Common::OID_comp *oic;
  Common::CharsDefn *charsdefn;
  Common::CharSyms *charsyms;

  /* object/class/set stuff */
  Asn::ObjectClass *objclass;
  Asn::Object *obj;
  Asn::ObjectSet *objset;
  Asn::FieldSpec *fieldspec;
  Asn::FieldSpecs *fieldspecs;
  Asn::OS_Element *os_element;
  Asn::OS_defn *os_defn;
  Asn::AtNotation *an;
  Asn::AtNotations *ans;
  Asn::TableConstraint *tbl_cnstr;
}

/*********************************************************************
 * Tokens with semantic value
 *********************************************************************/

%token <id> TOK_UpperIdentifier "<upperidentifier>"
  TOK_LowerIdentifier "<loweridentifier>"
  TOK_ampUpperIdentifier "<&upperidentifier>"
  TOK_ampLowerIdentifier  "<&loweridentifier>"
%token <i> TOK_Number "<number>"
%token <str> TOK_CString "<cstring>"
%token <value>
  TOK_RealNumber "<realnumber>"
  TOK_BString "<bstring>"
  TOK_HString "<hstring>"
%token <block> TOK_Block "{block}"

/* special keyword-like tokens */

%token
  KW_Block_NamedNumberList
  KW_Block_Enumerations
  KW_Block_Assignment
  KW_Block_NamedBitList
  KW_Block_IdentifierList
  KW_Block_FieldSpecList
  KW_Block_ComponentTypeLists
  KW_Block_AlternativeTypeLists
  KW_Block_Type
  KW_Block_Value
  KW_Block_Object
  KW_Block_ObjectSet
  KW_Block_SeqOfValue
  KW_Block_SetOfValue
  KW_Block_SequenceValue
  KW_Block_SetValue
  KW_Block_ObjectSetSpec
  KW_Block_DefinedObjectSetBlock
  KW_Block_AtNotationList
  KW_Block_OIDValue
  KW_Block_ROIDValue
  KW_Block_CharStringValue
  KW_Block_QuadrupleOrTuple
  KW_TITAN_Assignments
  KW_Block_DefinitiveOIDValue
  KW_Block_ValueSet
  KW_Block_MultipleTypeConstraints

/*********************************************************************
 * Tokens without semantic value (keywords, operators)
 *********************************************************************/

%token TOK_Assignment "::="
%token TOK_RangeSeparator ".."
%token TOK_Ellipsis "..."
%token TOK_LeftVersionBrackets "[["
%token TOK_RightVersionBrackets "]]"
%token '{' "{"
%token '}' "}"
%token '(' "("
%token ')' ")"
%token '[' "["
%token ']' "]"
%token ',' ","
%token '.' "."
%token '-' "-"
%token ':' ":"
%token ';' ";"
%token '@' "@"
%token '|' "|"
%token '!' "!"
%token '^' "^"
%token '\'' "'"
%token '"' "\""
%token '<' "<"
%token '>' ">"

%token
  KW_ABSENT "ABSENT"
/*
  KW_ABSTRACT_SYNTAX "ABSTRACT-SYNTAX"
*/
  KW_ALL "ALL"
  KW_ANY "ANY"
  KW_APPLICATION "APPLICATION"
  KW_AUTOMATIC "AUTOMATIC"
  KW_BEGIN "BEGIN"
  KW_BIT "BIT"
  KW_BMPString "BMPString"
  KW_BOOLEAN "BOOLEAN"
  KW_BY "BY"
  KW_CHARACTER "CHARACTER"
  KW_CHOICE "CHOICE"
  KW_CLASS "CLASS"
  KW_COMPONENT "COMPONENT"
  KW_COMPONENTS "COMPONENTS"
  KW_CONSTRAINED "CONSTRAINED"
  KW_CONTAINING "CONTAINING"
  KW_DEFAULT "DEFAULT"
  KW_DEFINED "DEFINED"
  KW_DEFINITIONS "DEFINITIONS"
  KW_EMBEDDED "EMBEDDED"
  KW_ENCODED "ENCODED"
  KW_END "END"
  KW_ENUMERATED "ENUMERATED"
  KW_EXCEPT "EXCEPT"
  KW_EXPLICIT "EXPLICIT"
  KW_EXPORTS "EXPORTS"
  KW_EXTENSIBILITY "EXTENSIBILITY"
  KW_EXTERNAL "EXTERNAL"
  KW_FALSE "FALSE"
  KW_FROM "FROM"
  KW_GeneralizedTime "GeneralizedTime"
  KW_GeneralString "GeneralString"
  KW_GraphicString "GraphicString"
  KW_IA5String "IA5String"
  KW_IDENTIFIER "IDENTIFIER"
  KW_IMPLICIT "IMPLICIT"
  KW_IMPLIED "IMPLIED"
  KW_IMPORTS "IMPORTS"
  KW_INCLUDES "INCLUDES"
  KW_INSTANCE "INSTANCE"
  KW_INTEGER "INTEGER"
  KW_INTERSECTION "INTERSECTION"
  KW_ISO646String "ISO646String"
  KW_MAX "MAX"
  KW_MIN "MIN"
  KW_MINUS_INFINITY "MINUS-INFINITY"
  KW_NOT_A_NUMBER "NOT-A-NUMBER"
  KW_NULL "NULL"
  KW_NumericString "NumericString"
  KW_OBJECT "OBJECT"
  KW_ObjectDescriptor "ObjectDescriptor"
  KW_OCTET "OCTET"
  KW_OF "OF"
  KW_OPTIONAL "OPTIONAL"
  KW_PATTERN "PATTERN"
  KW_PDV "PDV"
  KW_PLUS_INFINITY "PLUS-INFINITY"
  KW_PRESENT "PRESENT"
  KW_PrintableString "PrintableString"
  KW_PRIVATE "PRIVATE"
  KW_REAL "REAL"
  KW_RELATIVE_OID "RELATIVE-OID"
  KW_SEQUENCE "SEQUENCE"
  KW_SET "SET"
  KW_SIZE "SIZE"
  KW_STRING "STRING"
  KW_SYNTAX "SYNTAX"
  KW_T61String "T61String"
  KW_TAGS "TAGS"
  KW_TeletexString "TeletexString"
  KW_TRUE "TRUE"
/*
  KW_TYPE_IDENTIFIER "TYPE-IDENTIFIER"
*/
  KW_UNION "UNION"
  KW_UNIQUE "UNIQUE"
  KW_UNIVERSAL "UNIVERSAL"
  KW_UniversalString "UniversalString"
  KW_UTCTime "UTCTime"
  KW_UTF8String "UTF8String"
  KW_VideotexString "VideotexString"
  KW_VisibleString "VisibleString"
  KW_WITH "WITH"

/*********************************************************************
 * Semantic types of nonterminals
 *********************************************************************/

%type <id> ModuleIdentifier
%type <module> ModuleDefinition
%type <tagging> TagDefault
%type <b> ExtensionDefault
%type <modbody> ModuleBody
%type <id> Symbol
%type <syms> SymbolList
%type <exports> Exports SymbolsExported
%type <impmod> SymbolsFromModule
%type <impmods> SymbolsFromModuleList SymbolsImported Imports
%type <asss> AssignmentList
%type <ass_pard> OptParList
%type <ass> Assignment TypeAssignment ValueAssignment
  ValueSetTypeAssignment /*ObjectSetAssignment*/
  ObjectClassAssignment /*ObjectAssignment*/
  UndefAssignment
%type <objclass> /*ObjectClass DefinedObjectClass*/ ObjectClassDefn
  /*UsefulObjectClassReference*/
%type <obj> Object DefinedObject ObjectDefn
%type <objset> ObjectSet DefinedObjectSetBlock
%type <os_element> ObjectSetElements
%type <os_defn> ObjectSetSpec ObjectSetSpec0
%type <_i> Levels Level
%type <an> AtNotation
%type <ans> AtNotationList
%type <tbl_cnstr> TableConstraint ComponentRelationConstraint
%type <type>
  Type Type_reg BuiltinType BuiltinType_reg
  NakedType NakedType_reg
  ConstrainedType SeqOfSetOfWithConstraint
  ReferencedType /*DefinedType*/
  UsefulType
  EmbeddedPDVType ExternalType
  CharacterStringType RestrictedCharacterStringType
  UnrestrictedCharacterStringType
  Dflt_Type
%type <ctss> ComponentTypeLists AlternativeTypeLists
%type <extAdds> ExtensionAdditionList ExtensionAdditionAlternativesList
%type <extAdd> ExtensionAddition  ExtensionAdditionGroup
  ExtensionAdditionAlternative
%type <cts> ComponentTypeList AlternativeTypeList
%type <ct> ComponentType
%type <compField> ComponentType_reg
%type <namedType> NamedType
%type <excSpec> ExceptionIdentification ExceptionSpec
%type <extAndExc> ExtensionAndException
%type <type> ChoiceType SelectionType SelectionTypeType
%type <type> SequenceType SetType
%type <type> SequenceOfType SetOfType
%type <enumitem> EnumerationItem
%type <enumitems> Enumeration
%type <type> Enumerations EnumeratedType
%type <type> AnyType
%type <type> IntegerType
%type <type> BooleanType
%type <type> NullType
%type <type> ObjectIdentifierType RelativeOIDType
%type <type> OctetStringType
%type <type> BitStringType
%type <type> GeneralString
  BMPString GraphicString IA5String NumericString PrintableString
  TeletexString UniversalString UTF8String VideotexString
  VisibleString
%type <type> RealType
%type <tagclass> Class
%type <tag> Tag
%type <rangeendpoint> LowerEndpoint UpperEndpoint LowerEndValue UpperEndValue
%type <constraint> Constraint ConstraintSpec SubtypeConstraint GeneralConstraint
  ElementSetSpecs RootElementSetSpec ElementSetSpec Unions Intersections
  IntersectionElements Elements SubtypeElements SingleValue
  InnerTypeConstraints Elems IElems UElems AdditionalElementSetSpec
  ValueConstraint
%type <contsubt__constraint> ContainedSubtype
%type <valuerange_constraint> ValueRange
%type <size_constraint> SizeConstraint
%type <permalpha_constraint> PermittedAlphabet
%type <setoperation_constraint> Exclusions
%type <pattern_constraint> PatternConstraint
%type <single_inner_constraint> SingleTypeConstraint
%type <multiple_inner_constraint> MultipleTypeConstraints TypeConstraints
%type <named_constraint> NamedConstraint
%type <presence_constraint> PresenceConstraint
%type <constraints> Constraints
%type <value> Val_Number Val_CString Value Value_reg
  ObjectClassFieldValue ObjectClassFieldValue_reg
/*
  OpenTypeFieldValue OpenTypeFieldValue_reg
*/
  FixedTypeFieldValue FixedTypeFieldValue_reg
  BuiltinValue BuiltinValue_reg
  ReferencedValue ReferencedValue_reg
  DefinitiveIdentifier AssignedIdentifier
  ClassNumber
  Dflt_Value Dflt_Value_reg Dflt_NullValue
  DefinedValue DefinedValue_reg
  BooleanValue
  ChoiceValue
  SignedNumber
  LowerIdValue
  RealValue SpecialRealValue NumericRealValue
  NullValue
  DefinitiveOIDValue DefinitiveObjIdComponentList
  ObjectIdentifierValue ObjIdComponentList
  RelativeOIDValue RelativeOIDComponentList
%type <oic> DefinitiveObjIdComponent
  ObjIdComponent ObjIdComponentNumber
  DefinitiveNameAndNumberForm NameAndNumberForm
  RelativeOIDComponent
%type <value> SequenceValue SetValue
  SeqOfValue SetOfValue
%type <values> ValueList ValueList0
%type <namedValue> NamedValue NamedNumber NamedBit
%type <namedValues> NamedNumberList
                    ComponentValueList NamedBitList
%type <charsdefn> CharsDefn QuadrupleOrTuple QuadrupleOrTuple0
%type <charsyms> CharStringValue CharSyms
%type <value> IdentifierList IdentifierList1
%type <ref_ds> UpperRef ExtUpperRef SimplDefdUpper
  LowerRef ExtLowerRef SimplDefdLower SimplDefdLower_reg
%type <ref_d> PardUpper DefdUpper
  PardLower DefdLower DefdLower_reg
%type <fieldname> FieldNameUpper FieldNameUppers FieldNameLower
  ComponentIdList
%type <fromobj> UpperFromObj FromObjs Lower_FromObjs
  LowerFromObj Upper_FromObjs SimplUpper_FromObjs PardUpper_FromObjs
%type <ref> RefdUpper RefdLower RefdLower_reg
  Dflt_RefdLower Dflt_RefdLower_reg
%type <fieldspec> FieldSpec TypeFieldSpec FixedTypeValueFieldSpec
  UndefFieldSpec
%type <fieldspecs> FieldSpecList
%type <block> Dflt_Block
%type <id> Dflt_LowerId
%type <block> WithSyntaxSpec_opt
%type <b> UNIQ_opt
%type <node> BlockDefinition

/*********************************************************************
 * Destructors
 *********************************************************************/

%destructor {delete $$;}
TOK_BString
TOK_Block
TOK_CString
TOK_HString
TOK_LowerIdentifier
TOK_Number
TOK_RealNumber
TOK_UpperIdentifier
TOK_ampLowerIdentifier
TOK_ampUpperIdentifier
AlternativeTypeList
AlternativeTypeLists
AnyType
AssignedIdentifier
Assignment
AtNotation
AtNotationList
BMPString
BitStringType
BlockDefinition
BooleanType
BooleanValue
BuiltinType
BuiltinType_reg
BuiltinValue
BuiltinValue_reg
CharStringValue
CharSyms
CharacterStringType
CharsDefn
ChoiceType
ChoiceValue
ClassNumber
ComponentIdList
ComponentRelationConstraint
ComponentType
ComponentTypeList
ComponentTypeLists
ComponentType_reg
ComponentValueList
ConstrainedType
Constraint
ConstraintSpec
Constraints
DefdLower
DefdLower_reg
DefdUpper
DefinedObject
//DefinedObjectClass
DefinedObjectSetBlock
//DefinedType
DefinedValue
DefinedValue_reg
DefinitiveIdentifier
DefinitiveNameAndNumberForm
DefinitiveOIDValue
DefinitiveObjIdComponent
DefinitiveObjIdComponentList
Dflt_Block
Dflt_LowerId
Dflt_NullValue
Dflt_RefdLower
Dflt_RefdLower_reg
Dflt_Type
Dflt_Value
Dflt_Value_reg
ElementSetSpec
ElementSetSpecs
Elements
EmbeddedPDVType
EnumeratedType
Enumeration
EnumerationItem
Enumerations
ExceptionIdentification
ExceptionSpec
Exports
ExtLowerRef
ExtUpperRef
ExtensionAddition
ExtensionAdditionAlternative
ExtensionAdditionAlternativesList
ExtensionAdditionGroup
ExtensionAdditionList
ExtensionAndException
ExternalType
FieldNameLower
FieldNameUpper
FieldNameUppers
FieldSpec
FieldSpecList
FixedTypeFieldValue
FixedTypeFieldValue_reg
FixedTypeValueFieldSpec
FromObjs
GeneralConstraint
GeneralString
GraphicString
IA5String
IdentifierList
IdentifierList1
Imports
IntegerType
IntersectionElements
Intersections
LowerFromObj
LowerIdValue
LowerRef
Lower_FromObjs
ModuleDefinition
ModuleIdentifier
NakedType
NakedType_reg
NameAndNumberForm
NamedBit
NamedBitList
NamedNumber
NamedNumberList
NamedValue
NullType
NullValue
NumericRealValue
NumericString
ObjIdComponent
ObjIdComponentList
ObjIdComponentNumber
Object
//ObjectClass
ObjectClassAssignment
ObjectClassDefn
ObjectClassFieldValue
ObjectClassFieldValue_reg
ObjectDefn
ObjectIdentifierType
ObjectIdentifierValue
ObjectSet
ObjectSetElements
ObjectSetSpec
ObjectSetSpec0
OctetStringType
OptParList
PardLower
PardUpper
PrintableString
QuadrupleOrTuple
QuadrupleOrTuple0
RealType
RealValue
RefdLower
RefdLower_reg
RefdUpper
ReferencedType
ReferencedValue
ReferencedValue_reg
RelativeOIDComponent
RelativeOIDComponentList
RelativeOIDType
RelativeOIDValue
RestrictedCharacterStringType
RootElementSetSpec
AdditionalElementSetSpec
SelectionType
SelectionTypeType
SeqOfSetOfWithConstraint
SeqOfValue
SequenceOfType
SequenceType
SequenceValue
SetOfType
SetOfValue
SetType
SetValue
SignedNumber
SimplDefdLower
SimplDefdLower_reg
SimplDefdUpper
SingleValue
SpecialRealValue
SubtypeConstraint
SubtypeElements
Symbol
SymbolList
SymbolsExported
SymbolsFromModule
SymbolsFromModuleList
SymbolsImported
TableConstraint
Tag
TeletexString
Type
TypeAssignment
TypeFieldSpec
Type_reg
UTF8String
UndefAssignment
UndefFieldSpec
Unions
UniversalString
UnrestrictedCharacterStringType
UpperFromObj
UpperRef
Upper_FromObjs
UsefulType
Val_CString
Val_Number
Value
ValueAssignment
ValueList ValueList0
ValueSetTypeAssignment
Value_reg
VideotexString
VisibleString
WithSyntaxSpec_opt
ContainedSubtype
ValueRange
PermittedAlphabet
SizeConstraint
InnerTypeConstraints
PatternConstraint
LowerEndpoint
UpperEndpoint
LowerEndValue
UpperEndValue
Exclusions
Elems
IElems
UElems
SingleTypeConstraint
MultipleTypeConstraints
TypeConstraints
NamedConstraint
ValueConstraint

%destructor {
  delete $$.exp;
  delete $$.imp;
  delete $$.asss;
}
ModuleBody

%destructor {
  delete $$.name;
  delete $$.type;
}
NamedType

%printer { fprintf(yyoutput, "'%s'", $$->c_str()); } TOK_CString

%printer { fprintf(yyoutput, "%s", $$->get_asnname().c_str()); } TOK_UpperIdentifier
  TOK_LowerIdentifier
  TOK_ampUpperIdentifier
  TOK_ampLowerIdentifier
/* With 2.4 (or even 2.3a) we could use
 * %printer { ... } <id>
 * Alas, not with Bison 2.3 */

%printer { fprintf(yyoutput, "%s", $$->t_str().c_str()); } TOK_Number;

/*%printer {
  int old_v = verb_level;
  verb_level = 255;
  $$->dump(0);
  verb_level = old_v;
} TOK_Block;*/


%%

/*********************************************************************
 * Grammar
 *********************************************************************/

GrammarRoot:
  ModuleDefinition {act_asn1_module=$1;}
| BlockDefinition {parsed_node=$1;}
| KW_TITAN_Assignments AssignmentList {parsed_assignments=$2;}
| error {act_asn1_module=0; parsed_node=0;}
;

/* 12.1 page 22 */
ModuleDefinition:
  ModuleIdentifier DefinitiveIdentifier
  KW_DEFINITIONS
  TagDefault
  ExtensionDefault
  TOK_Assignment
  KW_BEGIN
  ModuleBody
  ModuleEnd
  {
    DEBUG(6, "Module `%s' parsed successfully.",
          $1->get_dispname().c_str());
    $$=new Asn::Module($1, $4, $5, $8.exp, $8.imp, $8.asss);
    $$->set_location(asn1_infile, @1.first_line);
    delete $2;
  }
;

/*********************************************************************
 * Module header
 *********************************************************************/

ModuleIdentifier:
  TOK_UpperIdentifier
  {
    $$=$1;
  }
;

DefinitiveIdentifier:
/*
  '{' DefinitiveObjIdComponentList '}' {$$=new Asn_Value_ObjId($2);}
*/
  TOK_Block
  {
    $$=new Value(Value::V_UNDEF_BLOCK, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| /* empty */ {$$=0;}
;

DefinitiveOIDValue:
  DefinitiveObjIdComponentList {$$=$1;}
;

DefinitiveObjIdComponentList:
  DefinitiveObjIdComponent
  {
    $$=new Value(Value::V_OID);
    $$->set_location(asn1_infile, @1.first_line);
    $$->add_oid_comp($1);
  }
| DefinitiveObjIdComponentList DefinitiveObjIdComponent
  {$$=$1; $$->add_oid_comp($2);}
;

DefinitiveObjIdComponent:
  TOK_LowerIdentifier
  {
    $$=new OID_comp($1, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| Val_Number
  {
    $1->set_location(asn1_infile, @1.first_line);
    $$=new OID_comp(0, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| DefinitiveNameAndNumberForm {$$=$1;}
;

/* 12.1 page 23 */
TagDefault:
  KW_EXPLICIT KW_TAGS {$$=TagDefault::EXPLICIT;}
| KW_IMPLICIT KW_TAGS {$$=TagDefault::IMPLICIT;}
| KW_AUTOMATIC KW_TAGS {$$=TagDefault::AUTOMATIC;}
| /* empty */ {$$=TagDefault::EXPLICIT;}
;

/* 12.1 page 23 */
ExtensionDefault:
  KW_EXTENSIBILITY KW_IMPLIED {$$=true;}
| /* empty */ {$$=false;}
;

/*********************************************************************
 * Module body
 *********************************************************************/

/* 12.1 page 23 */
ModuleBody:
  Exports Imports AssignmentList
  {
    $$.exp = $1;
    $$.imp = $2;
    $$.asss = $3;
  }
| /* empty */
  {
    $$.exp = new Exports(false);
    $$.imp = new Imports();
    $$.asss = new Asn::Assignments();
  }
;

ModuleEnd:
  KW_END
| /* empty - error */
  {yyerror("Missing `END'");}
| KW_END error
  {yyerror("Something after `END'");}
;

/* 12.1 page 23 */
Exports:
  /* empty */
  {
    $$=new Exports(true); /* exports all */
    $$->set_location(asn1_infile);
  }
| KW_EXPORTS SymbolsExported ";" {$$=$2;}
;

/* 12.1 page 23 */
SymbolsExported:
  SymbolList
  {
    $$=new Exports($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| /* empty */
  {
    $$=new Exports(false); /* exports nothing */
    $$->set_location(asn1_infile);
  }
| KW_ALL
  {
    $$=new Exports(true); /* exports all */
    $$->set_location(asn1_infile);
  }
;

/* 12.1 page 23 */
Imports:
  /* empty */
  {
    $$=new Imports();
    $$->set_location(asn1_infile);
    yywarning("Missing IMPORTS clause is interpreted as `IMPORTS ;'"
              " (import nothing) instead of importing all symbols"
              " from all modules.");
  }
| KW_IMPORTS SymbolsImported ";" {$$=$2;}
;

/* 12.1 page 23 */
SymbolsImported:
  SymbolsFromModuleList {$$=$1;}
| /* empty */
  {
    $$=new Imports();
    $$->set_location(asn1_infile);
  }
;

/* 12.1 page 23 */
SymbolsFromModuleList:
  SymbolsFromModule
  {
    $$=new Imports();
    $$->set_location(asn1_infile, @1.first_line);
    $$->add_impmod($1);
  }
| SymbolsFromModuleList SymbolsFromModule {$$=$1; $$->add_impmod($2);}
;

/* 12.1 page 23 */
SymbolsFromModule:
  SymbolList KW_FROM TOK_UpperIdentifier AssignedIdentifier
  {
    $$=new ImpMod($3, $1);
    $$->set_location(asn1_infile, @1.first_line);
    delete $4;
  }
;

/* 12.1 page 23 */
AssignedIdentifier:
/*
  ObjectIdentifierValue {$$=$1;}
*/
  TOK_Block
  {
    $$=new Value(Value::V_UNDEF_BLOCK, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| DefinedValue {$$=$1;}
| /* empty */ {$$=0;}
;

/* 12.1 page 23 */
SymbolList:
  Symbol {$$=new Symbols(); $$->add_sym($1);}
| SymbolList "," Symbol {$$=$1; $$->add_sym($3);}
;

/* 12.1 page 23 */
Symbol:
  TOK_UpperIdentifier {$$=$1;}
| TOK_LowerIdentifier {$$=$1;}
| TOK_UpperIdentifier TOK_Block {$$=$1; delete $2;}
| TOK_LowerIdentifier TOK_Block {$$=$1; delete $2;}
/*
| ParameterizedReference
| objectclassreference
| objectreference
| objectsetreference
*/
;

/*********************************************************************
 * BlockDefinition
 *********************************************************************/

BlockDefinition:
  KW_Block_NamedNumberList NamedNumberList {$$=$2;}
| KW_Block_Enumerations Enumerations {$$=$2;}
| KW_Block_Assignment Assignment {$$=$2;}
| KW_Block_Assignment Assignment error {$$=$2;}
| KW_Block_NamedBitList NamedBitList {$$=$2;}
| KW_Block_IdentifierList IdentifierList {$$=$2;}
| KW_Block_FieldSpecList FieldSpecList {$$=$2;}
| KW_Block_ComponentTypeLists ComponentTypeLists {$$=$2;}
| KW_Block_AlternativeTypeLists AlternativeTypeLists {$$=$2;}
| KW_Block_Type Type {$$=$2;}
| KW_Block_Type Type error {$$=$2;}
| KW_Block_Value Value {$$=$2;}
| KW_Block_Value Value error {$$=$2;}
| KW_Block_Object Object {$$=$2;}
| KW_Block_Object Object error {$$=$2;}
| KW_Block_ObjectSet ObjectSet {$$=$2;}
| KW_Block_ObjectSet ObjectSet error {$$=$2;}
| KW_Block_SeqOfValue SeqOfValue {$$=$2;}
| KW_Block_SetOfValue SetOfValue {$$=$2;}
| KW_Block_SequenceValue SequenceValue {$$=$2;}
| KW_Block_SetValue SetValue {$$=$2;}
| KW_Block_ObjectSetSpec ObjectSetSpec {$$=$2;}
| KW_Block_DefinedObjectSetBlock DefinedObjectSetBlock {$$=$2;}
| KW_Block_AtNotationList AtNotationList {$$=$2;}
| KW_Block_OIDValue ObjectIdentifierValue {$$=$2;}
| KW_Block_ROIDValue RelativeOIDValue {$$=$2;}
| KW_Block_CharStringValue CharStringValue {$$=$2;}
| KW_Block_QuadrupleOrTuple QuadrupleOrTuple {$$=$2;}
| KW_Block_DefinitiveOIDValue DefinitiveOIDValue {$$=$2;}
| KW_Block_ValueSet ElementSetSpecs {$$=$2;}
| KW_Block_MultipleTypeConstraints MultipleTypeConstraints {$$=$2;}
;

/*********************************************************************
 * Assignments
 *********************************************************************/

/* 12.1 page 24 */
AssignmentList:
  Assignment {$$=new Asn::Assignments(); $$->add_ass($1);}
| AssignmentList Assignment {$$=$1; $$->add_ass($2);}
| error Assignment {$$=new Asn::Assignments(); $$->add_ass($2);}
| AssignmentList error {$$=$1;}
;

/* 12.1 page 24 */
Assignment:
  TypeAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
| ValueAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
| ValueSetTypeAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
| ObjectClassAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
/*
| ObjectAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
| ObjectSetAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
| ParameterizedAssignment
*/
| UndefAssignment {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
;

/*
AbsoluteReference:
  '@' GlobalModuleReference '.' ItemSpec
;

ItemSpec:
  TOK_UpperIdentifier
| ItemId '.' ComponentId
;

ItemId:
  ItemSpec
;

ComponentId:
  TOK_LowerIdentifier
| TOK_Number
| '*'
;
*/

OptParList:
  /* empty */ {$$=0;}
| TOK_Block {$$=new Ass_pard($1);}

ObjectClassAssignment:
  TOK_UpperIdentifier OptParList TOK_Assignment ObjectClassDefn
  {$$=new Ass_OC($1, $2, $4);}
;

/*
ObjectAssignment:
  TOK_LowerIdentifier OptParList DefinedObjectClass_reg TOK_Assignment
    TOK_Block
  {$$=new Ass_O($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList DefinedObjectClass_reg TOK_Assignment
    RefdLower
  {$$=new Ass_O($1, $2, $3, $5);}
;
*/

/*
ObjectSetAssignment:
  TOK_UpperIdentifier OptParList DefinedObjectClass_reg
    TOK_Assignment TOK_Block
  {$$=new Ass_OS($1, $2, $3, $5);}
;
*/

/* 15.1 page 28 */
TypeAssignment:
  TOK_UpperIdentifier OptParList TOK_Assignment Type_reg
  {$$=new Ass_T($1, $2, $4);}
| TOK_UpperIdentifier OptParList TOK_Assignment UpperFromObj
  {
    Type *t = new Type(Type::T_REFD, $4);
    t->set_location(asn1_infile, @4.first_line);
    $$ = new Ass_T($1, $2, t);
  }
| TOK_UpperIdentifier OptParList TOK_Assignment FromObjs
  {
    Type *t = new Type(Type::T_REFD, $4);
    t->set_location(asn1_infile, @4.first_line);
    $$ = new Ass_T($1, $2, t);
  }
;

/* 15.2 page 28 */
ValueAssignment:
  TOK_LowerIdentifier OptParList Type_reg TOK_Assignment Value_reg
  {$$=new Ass_V($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList Type_reg TOK_Assignment NullValue
  {$$=new Ass_V($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList Type_reg TOK_Assignment TOK_Block
  {
    Value *v = new Value(Value::V_UNDEF_BLOCK, $5);
    v->set_location(asn1_infile, @5.first_line);
    $$=new Ass_V($1, $2, $3, v);
  }
| TOK_LowerIdentifier OptParList Type_reg TOK_Assignment ReferencedValue_reg
  {$$=new Ass_V($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList Type_reg TOK_Assignment LowerIdValue
  {$$=new Ass_V($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList DefdUpper TOK_Assignment Value_reg
  {
    Type *t = new Type(Type::T_REFD, $3);
    t->set_location(asn1_infile, @3.first_line);
    $$ = new Ass_V($1, $2, t, $5);
  }
| TOK_LowerIdentifier OptParList DefdUpper TOK_Assignment NullValue
  {
    Type *t = new Type(Type::T_REFD, $3);
    t->set_location(asn1_infile, @3.first_line);
    $$ = new Ass_V($1, $2, t, $5);
  }
| TOK_LowerIdentifier OptParList UpperFromObj TOK_Assignment Value
  {
    Type *t = new Type(Type::T_REFD, $3);
    t->set_location(asn1_infile, @3.first_line);
    $$ = new Ass_V($1, $2, t, $5);
  }
| TOK_LowerIdentifier OptParList FromObjs TOK_Assignment Value
  {
    Type *t = new Type(Type::T_REFD, $3);
    t->set_location(asn1_infile, @3.first_line);
    $$ = new Ass_V($1, $2, t, $5);
  }
;

/* 15.6 page 29 */
ValueSetTypeAssignment:
  TOK_UpperIdentifier OptParList Type_reg TOK_Assignment TOK_Block
  {$$=new Ass_VS($1, $2, $3, $5);}
;

UndefAssignment:
  TOK_UpperIdentifier OptParList DefdUpper TOK_Assignment TOK_Block
  {$$=new Ass_Undef($1, $2, $3, $5);}
| TOK_UpperIdentifier OptParList TOK_Assignment DefdUpper
  {$$=new Ass_Undef($1, $2, 0, $4);}
| TOK_LowerIdentifier OptParList DefdUpper TOK_Assignment TOK_Block
  {$$=new Ass_Undef($1, $2, $3, $5);}
| TOK_LowerIdentifier OptParList DefdUpper TOK_Assignment RefdLower
  {$$=new Ass_Undef($1, $2, $3, $5);}
;

/* 15.7 page 29 */
//ValueSet:
///*
//  "{" ElementSetSpecs "}"
//*/
//  TOK_Block {delete $1;}
//;

/*********************************************************************
 * Tag stuff
 *********************************************************************/

/*
TaggedType:
  Tag Type
| Tag KW_IMPLICIT Type
| Tag KW_EXPLICIT Type
;
*/

Tag:
  "[" Class ClassNumber "]"
  {
    $$=new Tag(Tag::TAG_DEFPLICIT, $2, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| "[" Class ClassNumber "]" KW_IMPLICIT
  {
    $$=new Tag(Tag::TAG_IMPLICIT, $2, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| "[" Class ClassNumber "]" KW_EXPLICIT
  {
    $$=new Tag(Tag::TAG_EXPLICIT, $2, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ClassNumber:
  Val_Number
  {
    $$=$1;
    $1->set_location(asn1_infile, @1.first_line);
  }
| DefinedValue {$$=$1;}
;

/* 30.1 page 48 */
Class:
  KW_UNIVERSAL
  {
    $$=Tag::TAG_UNIVERSAL;
    if(!asn1_yy_parse_internal)
      yynote("Usage of UNIVERSAL tagclass is not recommended.");
  }
| KW_APPLICATION {$$=Tag::TAG_APPLICATION;}
| KW_PRIVATE {$$=Tag::TAG_PRIVATE;}
| /* empty */ {$$=Tag::TAG_CONTEXT;}
;

/*********************************************************************
 * Type stuff
 *********************************************************************/

/* 16.1 page 30 */
Type_reg:
  ConstrainedType
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| BuiltinType
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| Tag Type
  {
    $$=$2;
    $$->set_location(asn1_infile, @2.first_line);
    $$->add_tag($1);
  }
;

Type:
  ConstrainedType
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| NakedType
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| Tag Type
  {
    $$=$2;
    $$->set_location(asn1_infile, @2.first_line);
    $$->add_tag($1);
  }
;

/* 45.1 page 66 */
ConstrainedType:
  NakedType_reg Constraints {$$=$1; $$->add_constraints($2);}
| SeqOfSetOfWithConstraint {$$=$1;}
;

NakedType:
  NakedType_reg {$$=$1;}
| SequenceOfType {$$=$1;}
| SetOfType {$$=$1;}
;

NakedType_reg:
  ReferencedType {$$=$1;}
| BuiltinType_reg {$$=$1;}
;

BuiltinType:
  BuiltinType_reg {$$=$1;}
| SequenceOfType {$$=$1;}
| SetOfType {$$=$1;}
;

BuiltinType_reg:
  NullType {$$=$1;}
| BitStringType {$$=$1;}
| BooleanType {$$=$1;}
| CharacterStringType {$$=$1;}
| ChoiceType {$$=$1;}
| SelectionType {$$=$1;}
| EnumeratedType {$$=$1;}
| IntegerType {$$=$1;}
| ObjectIdentifierType {$$=$1;}
| RelativeOIDType {$$=$1;}
| OctetStringType {$$=$1;}
| RealType {$$=$1;}
| SequenceType {$$=$1;}
| SetType {$$=$1;}
| EmbeddedPDVType {$$=$1;}
| ExternalType {$$=$1;}
| AnyType {$$=$1;}
| UsefulType {$$=$1;}
  /*
| InstanceOfType
| ObjectClassFieldType
  */
;


/* to eliminate the following reduce/reduce conflict:
   Constraint ... ->
   SubtypeElements->ContainedSubtype->Type->KW_NULL
   SubtypeElements->TypeConstraint->Type->KW_NULL
   SubtypeElements->SingleValue->Value->KW_NULL
*/

/*********************************************************************
 * Value stuff
 *********************************************************************/

BuiltinValue:
  BuiltinValue_reg {$$=$1;}
| NullValue {$$=$1;}
| LowerIdValue {$$=$1;}
| TOK_Block
  {
    $$=new Value(Value::V_UNDEF_BLOCK, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/*
 e.g., the EnumeratedValue is LowerIdValue
 */
LowerIdValue:
  TOK_LowerIdentifier
  {
    $$=new Value(Value::V_UNDEF_LOWERID, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

BuiltinValue_reg:
  TOK_BString
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_HString
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| Val_CString
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| BooleanValue
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| ChoiceValue
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
| SignedNumber {$$=$1;}
| RealValue
  {
    $$=$1;
    $$->set_location(asn1_infile, @1.first_line);
  }
/*
| EmbeddedPDVValue
| InstanceOfValue
| ExternalValue
  --> SequenceOrSetValue
| EnumeratedValue
  --> TOK_LowerIdentifier
*/
;

/*********************************************************************
 * The individual types and its value notation
 *********************************************************************/

BooleanType:
  KW_BOOLEAN {$$=new Type(Type::T_BOOL);}
;

BooleanValue:
  KW_TRUE {$$=new Value(Value::V_BOOL, true);}
| KW_FALSE {$$=new Value(Value::V_BOOL, false);}
;

IntegerType:
  KW_INTEGER
  {$$=new Type(Type::T_INT_A);}
| KW_INTEGER TOK_Block {$$=new Type(Type::T_INT_A, $2);}
;

/*********************************************************************
 * NamedNumberList
 *********************************************************************/

NamedNumberList:
  NamedNumber
  {
    $$=new NamedValues();
    $$->add_nv($1);
  }
| NamedNumberList "," NamedNumber
  {
    $$=$1;
    $$->add_nv($3);
  }
| error NamedNumber
  {
    $$=new NamedValues();
    $$->add_nv($2);
  }
| NamedNumberList error
  {$$=$1;}
;

NamedNumber:
  TOK_LowerIdentifier "(" SignedNumber ")"
  {
    $$=new NamedValue($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_LowerIdentifier "(" DefinedValue ")"
  {
    $$=new NamedValue($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

SignedNumber:
  Val_Number
  {
    $1->set_location(asn1_infile, @1.first_line);
    $$=$1;
  }
| "-" Val_Number
  {
    $2->set_location(asn1_infile, @2.first_line);
    if(*$2->get_val_Int()==0)
      $2->error("-0 is not a valid signed number");
    $$=$2;
    $$->change_sign();
  }
;

EnumeratedType:
  KW_ENUMERATED TOK_Block {$$=new Type(Type::T_ENUM_A, $2);}
;

Enumerations:
  Enumeration
  {$$=new Type(Type::T_ENUM_A, $1, false, 0);}
| Enumeration "," TOK_Ellipsis ExceptionSpec
  {$$=new Type(Type::T_ENUM_A, $1, true, 0); delete $4;}
| Enumeration "," TOK_Ellipsis ExceptionSpec "," Enumeration
  {$$=new Type(Type::T_ENUM_A, $1, true, $6); delete $4;}
;

Enumeration:
  EnumerationItem
  {
    $$=new EnumItems();
    $$->add_ei($1);
  }
| Enumeration "," EnumerationItem
  {
    $$=$1;
    $$->add_ei($3);
  }
| error EnumerationItem
  {
    $$=new EnumItems();
    $$->add_ei($2);
  }
| Enumeration error
  {$$=$1;}
;

EnumerationItem:
  TOK_LowerIdentifier
  {
    $$=new EnumItem($1, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| NamedNumber
  {
    $$=new EnumItem($1->get_name().clone(), $1->get_value()->clone());
    $$->set_location(asn1_infile, @1.first_line);
    delete $1;
  }
;

RealType:
  KW_REAL {$$=new Type(Type::T_REAL);}
;

RealValue:
  SpecialRealValue {$$=$1;}
| NumericRealValue {$$=$1;}
;

NumericRealValue:
  TOK_RealNumber {$$=$1;}
| "-" TOK_RealNumber
  {
    $$=$2;
    $$->change_sign();
  }
/*
| SequenceValue
*/
;

SpecialRealValue:
  KW_PLUS_INFINITY
  {
    Location loc(asn1_infile, @1.first_line);
    $$=new Value(Value::V_REAL, string2Real("INF", loc));
  }
| KW_MINUS_INFINITY
  {
    Location loc(asn1_infile, @1.first_line);
    $$=new Value(Value::V_REAL, string2Real("-INF", loc));
  }
| KW_NOT_A_NUMBER
  {
    Location loc(asn1_infile, @1.first_line);
    $$=new Value(Value::V_REAL, string2Real("NaN", loc));
  }
;

BitStringType:
  KW_BIT KW_STRING {$$=new Type(Type::T_BSTR_A);}
| KW_BIT KW_STRING TOK_Block {$$=new Type(Type::T_BSTR_A, $3);}
;

NamedBitList:
  NamedBit
  {
    $$=new NamedValues();
    $$->add_nv($1);
  }
| NamedBitList "," NamedBit
  {
    $$=$1;
    $$->add_nv($3);
  }
| error NamedBit
  {
    $$=new NamedValues();
    $$->add_nv($2);
  }
| NamedBitList error
  {$$=$1;}
;

NamedBit:
  TOK_LowerIdentifier "(" Val_Number ")"
  {
    $$=new NamedValue($1, $3);
    $3->set_location(asn1_infile, @3.first_line);
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_LowerIdentifier "(" DefinedValue ")"
  {
    $$=new NamedValue($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* used in bitstring values */
IdentifierList:
  /* empty */
  {
    $$=new Value(Value::V_NAMEDBITS);
    $$->set_location(asn1_infile);
  }
| IdentifierList1 {$$=$1;}
;

IdentifierList1:
  TOK_LowerIdentifier
  {
    $$=new Value(Value::V_NAMEDBITS);
    $$->add_id($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| IdentifierList1 "," TOK_LowerIdentifier
  {
    $$=$1;
    $$->add_id($3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| error TOK_LowerIdentifier
  {
    $$=new Value(Value::V_NAMEDBITS);
    $$->add_id($2);
    $$->set_location(asn1_infile, @2.first_line);
  }
| IdentifierList1 error
  {$$=$1;}
;

OctetStringType:
  KW_OCTET KW_STRING {$$=new Type(Type::T_OSTR);}
;

AnyType:
  KW_ANY {$$=new Type(Type::T_ANY);}
| KW_ANY KW_DEFINED KW_BY TOK_LowerIdentifier
  {delete $4; $$=new Type(Type::T_ANY);}
;

NullType:
  KW_NULL {$$=new Type(Type::T_NULL);}
;

NullValue:
  KW_NULL
  {
    $$=new Value(Value::V_NULL);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/*
SequenceType:
  KW_SEQUENCE '{' '}'
| KW_SEQUENCE '{' ExtensionAndException OptionalExtensionMarker '}'
| KW_SEQUENCE '{' ComponentTypeLists '}'
;
*/

SequenceType:
  KW_SEQUENCE TOK_Block
  {$$=new Type(Type::T_SEQ_A, $2);}
;

ExtensionAndException:
  TOK_Ellipsis ExceptionSpec
  {$$=new ExtAndExc($2);}
  /*
| TOK_Ellipsis
  --> ExceptionSpec can be empty, so this is surplus
  */
;

ComponentTypeLists:
  /* empty */
  {$$=new CTs_EE_CTs(0, 0, 0);}
| ComponentTypeList
  {$$=new CTs_EE_CTs($1, 0, 0);}
| ComponentTypeList "," ExtensionAndException OptionalExtensionMarker
  {$$=new CTs_EE_CTs($1, $3, 0);}
| ComponentTypeList "," ExtensionAndException "," ExtensionAdditionList
  OptionalExtensionMarker
  {
    $3->set_eas($5);
    $$=new CTs_EE_CTs($1, $3, 0);
  }
| ComponentTypeList "," ExtensionAndException "," ExtensionAdditionList
  ExtensionEndMarker "," ComponentTypeList
  {
    $3->set_eas($5);
    $$=new CTs_EE_CTs($1, $3, $8);
  }
| ComponentTypeList "," ExtensionAndException ExtensionEndMarker ","
  ComponentTypeList
  {
    $$=new CTs_EE_CTs($1, $3, $6);
  }
| ExtensionAndException "," ExtensionAdditionList ExtensionEndMarker ","
  ComponentTypeList
  {
    $1->set_eas($3);
    $$=new CTs_EE_CTs(0, $1, $6);
  }
| ExtensionAndException "," ExtensionEndMarker "," ComponentTypeList
  {$$=new CTs_EE_CTs(0, $1, $5);}
| ExtensionAndException "," ExtensionAdditionList OptionalExtensionMarker
  {
    $1->set_eas($3);
    $$ = new CTs_EE_CTs(0, $1, 0);
  }
| ExtensionAndException OptionalExtensionMarker
  {$$=new CTs_EE_CTs(0, $1, 0);}
| error
  {$$=0;}
;

/*
ExtensionAdditions:
  ',' ExtensionAdditionList
| * empty *
;
*/

ExtensionAdditionList:
  ExtensionAddition
  {
    $$=new ExtAdds();
    $$->add_ea($1);
  }
| ExtensionAdditionList "," ExtensionAddition
  {
    $$=$1;
    $$->add_ea($3);
  }
| error ExtensionAddition
  {
    $$=new ExtAdds();
    $$->add_ea($2);
  }
| ExtensionAdditionList error
  {$$=$1;}
;

ExtensionEndMarker:
  "," TOK_Ellipsis
;

OptionalExtensionMarker:
  ExtensionEndMarker
| /* empty */
;

ExtensionAddition:
  ComponentType {$$=$1;}
| ExtensionAdditionGroup {$$=$1;}
;

ExtensionAdditionGroup:
  TOK_LeftVersionBrackets ComponentTypeList TOK_RightVersionBrackets
  {$$=new ExtAddGrp(0, $2);}
| TOK_LeftVersionBrackets error TOK_RightVersionBrackets
  {$$=new ExtAddGrp(0, new CTs());}
;

ComponentTypeList:
  ComponentType
  {
    $$=new CTs();
    $$->add_ct($1);
  }
| ComponentTypeList "," ComponentType
  {
    $$=$1;
    $$->add_ct($3);
  }
| error ComponentType
  {
    $$=new CTs();
    $$->add_ct($2);
  }
| ComponentTypeList error
  {$$=$1;}
;

ComponentType:
  ComponentType_reg
  {
    $$=new CT_reg($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| KW_COMPONENTS KW_OF Type
  {
    $$=new CT_CompsOf($3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ComponentType_reg:
  NamedType
  {
    $$=new CompField($1.name, $1.type, false, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| NamedType KW_OPTIONAL
  {
    $$=new CompField($1.name, $1.type, true, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| NamedType KW_DEFAULT Value
  {
    if (default_as_optional) { /* optional should not have a default value */
      delete $3;
    }
    $$=new CompField($1.name, $1.type, default_as_optional,
      default_as_optional ? NULL : $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

SequenceValue:
  ComponentValueList
  {
    $$=new Value(Value::V_SEQ, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| /* empty */
  {
    $$=new Value(Value::V_SEQ, new NamedValues());
    $$->set_location(asn1_infile, @0.first_line);
  }
;

SetValue:
  ComponentValueList
  {
    $$=new Value(Value::V_SET, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| /* empty */
  {
    $$=new Value(Value::V_SET, new NamedValues());
    $$->set_location(asn1_infile, @0.first_line);
  }
;

ComponentValueList:
  NamedValue
  {
    $$=new NamedValues();
    $$->add_nv($1);
  }
| ComponentValueList "," NamedValue
  {
    $$=$1;
    $$->add_nv($3);
  }
| error NamedValue
  {
    $$=new NamedValues();
    $$->add_nv($2);
  }
| ComponentValueList error
  {$$=$1;}
;

SequenceOfType:
  KW_SEQUENCE KW_OF Type {$$=new Type(Type::T_SEQOF, $3);}
| KW_SEQUENCE KW_OF NamedType {delete $3.name; $$=new Type(Type::T_SEQOF, $3.type);}
;

SetOfType:
  KW_SET KW_OF Type {$$=new Type(Type::T_SETOF, $3);}
| KW_SET KW_OF NamedType {delete $3.name; $$=new Type(Type::T_SETOF, $3.type);}
;

SeqOfValue:
  ValueList0
  {
    $$=new Value(Value::V_SEQOF, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

SetOfValue:
  ValueList0
  {
    $$=new Value(Value::V_SETOF, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ValueList0:
  /* empty */ {$$ = new Values();}
| ValueList {$$ = $1;}
;

ValueList:
  Value {$$=new Values(); $$->add_v($1);}
| ValueList "," Value {$$=$1; $$->add_v($3);}
| error Value {$$=new Values(); $$->add_v($2);}
| ValueList error {$$=$1;}
;

/*
SetType:
  KW_SET '{' '}'
| KW_SET '{' ExtensionAndException OptionalExtensionMarker '}'
| KW_SET '{' ComponentTypeLists '}'
;
*/

SetType:
  KW_SET TOK_Block
  {$$=new Type(Type::T_SET_A, $2);}
;

ChoiceType:
  KW_CHOICE TOK_Block {$$=new Type(Type::T_CHOICE_A, $2);}
;

AlternativeTypeLists:
  AlternativeTypeList
  {$$=new CTs_EE_CTs($1, 0, 0);}
| AlternativeTypeList "," ExtensionAndException OptionalExtensionMarker
  {$$=new CTs_EE_CTs($1, $3, 0);}
| AlternativeTypeList "," ExtensionAndException ","
  ExtensionAdditionAlternativesList OptionalExtensionMarker
  {
    $3->set_eas($5);
    $$=new CTs_EE_CTs($1, $3, 0);
  }
| error
  {$$=0;}
;

ExtensionAdditionAlternativesList:
  ExtensionAdditionAlternative
  {
    $$=new ExtAdds();
    $$->add_ea($1);
  }
| ExtensionAdditionAlternativesList "," ExtensionAdditionAlternative
  {
    $$=$1;
    $$->add_ea($3);
  }
| error ExtensionAdditionAlternative
  {
    $$=new ExtAdds();
    $$->add_ea($2);
  }
| ExtensionAdditionAlternativesList error
  {$$=$1;}
;

ExtensionAdditionAlternative:
  TOK_LeftVersionBrackets AlternativeTypeList TOK_RightVersionBrackets
  {$$=new ExtAddGrp(0, $2);}
| NamedType
  {
    CompField *cf = new CompField($1.name, $1.type, false, 0);
    cf->set_location(asn1_infile, @1.first_line);
    $$=new CT_reg(cf);
  }
| TOK_LeftVersionBrackets error TOK_RightVersionBrackets
  {$$=new ExtAddGrp(0, new CTs());}
;

AlternativeTypeList:
  NamedType
  {
    $$=new CTs();
    CompField *cf = new CompField($1.name, $1.type, false, 0);
    cf->set_location(asn1_infile, @1.first_line);
    $$->add_ct(new CT_reg(cf));
  }
| AlternativeTypeList "," NamedType
  {
    $$=$1;
    CompField *cf = new CompField($3.name, $3.type, false, 0);
    cf->set_location(asn1_infile, @3.first_line);
    $$->add_ct(new CT_reg(cf));
  }
| error NamedType
  {
    $$=new CTs();
    CompField *cf = new CompField($2.name, $2.type, false, 0);
    cf->set_location(asn1_infile, @2.first_line);
    $$->add_ct(new CT_reg(cf));
  }
| AlternativeTypeList error
  {$$=$1;}
;

ChoiceValue:
  TOK_LowerIdentifier ":" Value
  {$$=new Value(Value::V_CHOICE, $1, $3);}
;

/*
SelectionType:
  TOK_LowerIdentifier "<" Type
;
*/

SelectionTypeType:
  ReferencedType {$$=$1;}
| ChoiceType {$$=$1;}
| SelectionType {$$=$1;}
| Tag SelectionTypeType
  {
    $$=$2;
    $$->set_location(asn1_infile, @2.first_line);
    $$->add_tag($1);
  }
;

SelectionType:
  TOK_LowerIdentifier "<" SelectionTypeType
  {
    $$=new Type(Type::T_SELTYPE, $1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ObjectIdentifierType:
  KW_OBJECT KW_IDENTIFIER {$$=new Type(Type::T_OID);}
;

RelativeOIDType:
  KW_RELATIVE_OID {$$=new Type(Type::T_ROID);}
;

ObjectIdentifierValue:
  ObjIdComponentList {$$=$1;}
/*
| '{' DefinedValue ObjIdComponentList '}'
  --> it must be checked if the ObjIdComponentList's first component
      is a DefinedValue to a type OBJECT IDENTIFIER
*/
;

RelativeOIDValue:
  RelativeOIDComponentList {$$=$1;}
;

ObjIdComponentList:
  ObjIdComponent
  {
    $$=new Value(Value::V_OID);
    $$->set_location(asn1_infile, @1.first_line);
    $$->add_oid_comp($1);
  }
| ObjIdComponentList ObjIdComponent
  {
    $$=$1;
    $$->add_oid_comp($2);
  }
| ObjIdComponentList error
  {$$=$1;}
;

RelativeOIDComponentList:
  RelativeOIDComponent
  {
    $$=new Value(Value::V_ROID);
    $$->set_location(asn1_infile, @1.first_line);
    $$->add_oid_comp($1);
  }
| RelativeOIDComponentList RelativeOIDComponent
  {
    $$=$1;
    $$->add_oid_comp($2);
  }
| RelativeOIDComponentList error
  {$$=$1;}
;

/*
ObjIdComponent:
  NameForm
| NumberForm
| NameAndNumberForm
;
*/

ObjIdComponent:
  ObjIdComponentNumber {$$=$1;}
| TOK_LowerIdentifier /* NameForm or reference */
  {
    $$=new OID_comp($1, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| DefinedValue_reg
  {
    $$=new OID_comp($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

RelativeOIDComponent:
  ObjIdComponentNumber {$$=$1;}
| DefinedValue
  {
    $$=new OID_comp($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ObjIdComponentNumber:
  NameAndNumberForm {$$=$1;}
| Val_Number
  {
    $1->set_location(asn1_infile, @1.first_line);
    $$=new OID_comp(0, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

NameAndNumberForm:
  DefinitiveNameAndNumberForm {$$=$1;}
| TOK_LowerIdentifier "(" DefinedValue ")"
  {
    $$=new OID_comp($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

DefinitiveNameAndNumberForm:
  TOK_LowerIdentifier "(" Val_Number ")"
  {
    $3->set_location(asn1_infile, @3.first_line);
    $$=new OID_comp($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

EmbeddedPDVType:
  KW_EMBEDDED KW_PDV {$$=new Type(Type::T_EMBEDDED_PDV);}
;

/*
EmbeddedPDVValue:
  SequenceValue
;
*/

ExternalType:
  KW_EXTERNAL {$$=new Type(Type::T_EXTERNAL);}
;

/*
ExternalValue:
  SequenceValue
;
*/

/*********************************************************************
 * Character string stuff
 *********************************************************************/

/* 36.1 page 55 */
CharacterStringType:
  RestrictedCharacterStringType {$$=$1;}
| UnrestrictedCharacterStringType {$$=$1;}
;

/*
CharacterStringValue:
  RestrictedCharacterStringValue
| UnrestrictedCharacterStringValue
  --> SequenceValue
;
*/

/* 37 page 55 */
RestrictedCharacterStringType:
  GeneralString {$$=$1;}
| BMPString {$$=$1;}
| GraphicString {$$=$1;}
| IA5String {$$=$1;}
| NumericString {$$=$1;}
| PrintableString {$$=$1;}
| TeletexString {$$=$1;}
| UniversalString {$$=$1;}
| UTF8String {$$=$1;}
| VideotexString {$$=$1;}
| VisibleString {$$=$1;}
;

GeneralString:
  KW_GeneralString
  {$$=new Type(Type::T_GENERALSTRING);}
;

BMPString:
  KW_BMPString
  {$$=new Type(Type::T_BMPSTRING);}
;

GraphicString:
  KW_GraphicString
  {$$=new Type(Type::T_GRAPHICSTRING);}
;

IA5String:
  KW_IA5String
  {$$=new Type(Type::T_IA5STRING);}
;

NumericString:
  KW_NumericString
  {$$=new Type(Type::T_NUMERICSTRING);}
;

PrintableString:
  KW_PrintableString
  {$$=new Type(Type::T_PRINTABLESTRING);}
;

TeletexString:
  KW_TeletexString
  {$$=new Type(Type::T_TELETEXSTRING);}
| KW_T61String
  {$$=new Type(Type::T_TELETEXSTRING);}
;

UniversalString:
  KW_UniversalString
  {$$=new Type(Type::T_UNIVERSALSTRING);}
;

UTF8String:
  KW_UTF8String
  {$$=new Type(Type::T_UTF8STRING);}
;

VideotexString:
  KW_VideotexString
  {$$=new Type(Type::T_VIDEOTEXSTRING);}
;

VisibleString:
  KW_VisibleString
  {$$=new Type(Type::T_VISIBLESTRING);}
| KW_ISO646String
  {$$=new Type(Type::T_VISIBLESTRING);}
;

/*
RestrictedCharacterStringValue:
  TOK_CString {$$=$1;}
| CharacterStringList
| Quadruple
| Tuple
;

CharacterStringList:
  '{' CharSyms '}'
;
*/

CharStringValue:
  CharSyms {$$=$1;}
| QuadrupleOrTuple
  {
    $$=new CharSyms();
    $$->add_cd($1);
  }
;

CharSyms:
  CharsDefn
  {
    $$=new CharSyms();
    $$->add_cd($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| CharSyms "," CharsDefn
  {
    $$=$1;
    $$->add_cd($3);
  }
| error CharsDefn
  {
    $$=new CharSyms();
    $$->add_cd($2);
    $$->set_location(asn1_infile, @1.first_line);
  }
| CharSyms error
  {
    $$=$1;
  }
;

/*
CharsDefn:
  TOK_CString
| Quadruple
| Tuple
| DefinedValue
;
*/

CharsDefn:
  TOK_CString
  {
    $$=new CharsDefn($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_Block
  {
    $$=new CharsDefn($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| ReferencedValue
  {
    $$=new CharsDefn($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

QuadrupleOrTuple:
  QuadrupleOrTuple0 {$$=$1;}
| QuadrupleOrTuple0 error {$$=$1;}
| error
  {
    $$=new CharsDefn(0, 0, 0, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

QuadrupleOrTuple0:
  TOK_Number "," TOK_Number "," TOK_Number "," TOK_Number
  {
  	Int group = 0, plane = 0, row = 0, cell = 0;
		if (*$1 > INT_MAX) {
		  Location loc(asn1_infile, @1);
			loc.error("The first number of quadruple (group) should be less than "
			  "%d instead of %s", INT_MAX, ($1->t_str()).c_str());
		} else {
		  group = $1->get_val();
		}
		if (*$3 > INT_MAX) {
			Location loc(asn1_infile, @3);
			loc.error("The second number of quadruple (plane) should be less than "
			  "%d instead of %s", INT_MAX, ($3->t_str()).c_str());
		} else {
		  plane = $3->get_val();
		}
		if (*$5 > INT_MAX) {
			Location loc(asn1_infile, @5);
			loc.error("The third number of quadruple (row) should be less than %d "
			  "instead of %s", INT_MAX, ($5->t_str()).c_str());
		} else {
		  row = $5->get_val();
		}
		if (*$7 > INT_MAX) {
			Location loc(asn1_infile, @7);
			loc.error("The fourth number of quadruple (cell) should be less than "
			  "%d instead of %s", INT_MAX, ($7->t_str()).c_str());
		} else {
		  cell = $7->get_val();
		}
   	$$=new CharsDefn(group, plane, row, cell);
    $$->set_location(asn1_infile, @1.first_line);
    delete $1;
    delete $3;
    delete $5;
    delete $7;
  }
| TOK_Number "," TOK_Number
  {
  	Int table_column = 0, table_row = 0;
  	if (*$1 > INT_MAX) {
  		Location loc(asn1_infile, @1);
  		loc.error("The first number of tuple (table column) should be less "
  		  "than %d instead of %s", INT_MAX, ($1->t_str()).c_str());
  	} else {
  	  table_column = $1->get_val();
  	}
  	if (*$3 > INT_MAX) {
  		Location loc(asn1_infile, @3);
  		loc.error("The second number of tuple (table row) should be less than "
  		  "%d instead of %s", INT_MAX, ($3->t_str()).c_str());
  	} else {
  	  table_row = $3->get_val();
  	}
    $$=new CharsDefn(table_column, table_row);
    $$->set_location(asn1_infile, @1.first_line);
    delete $1;
    delete $3;
  }
;

/*
Quadruple:
  '{' Group ',' Plane ',' Row ',' Cell '}'
;

Group:
  TOK_Number
;

Plane:
  TOK_Number
;

Row:
  TOK_Number
;

Cell:
  TOK_Number
;

Tuple:
  '{' TableColumn ',' TableRow '}'
;

TableColumn:
  TOK_Number
;

TableRow:
  TOK_Number
;
*/

UnrestrictedCharacterStringType:
  KW_CHARACTER KW_STRING {$$=new Type(Type::T_UNRESTRICTEDSTRING);}
;

/*
UnrestrictedCharacterStringValue:
  SequenceValue
;
*/

UsefulType:
  KW_GeneralizedTime
  {$$=new Type(Type::T_GENERALIZEDTIME);}
| KW_UTCTime
  {$$=new Type(Type::T_UTCTIME);}
| KW_ObjectDescriptor
  {$$=new Type(Type::T_OBJECTDESCRIPTOR);}
;

/*********************************************************************
 * Constraint stuff
 *********************************************************************/

/*
ConstrainedType:
  Type Constraint
| TypeWithConstraint
;
*/

SeqOfSetOfWithConstraint:
  KW_SET Constraint KW_OF Type
  {
    $$=new Type(Type::T_SETOF, $4);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SET Constraint KW_OF NamedType
  {
    delete $4.name;
    $$=new Type(Type::T_SETOF, $4.type);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SET SizeConstraint KW_OF Type
  {
    $$=new Type(Type::T_SETOF, $4);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SET SizeConstraint KW_OF NamedType
  {
    delete $4.name;
    $$=new Type(Type::T_SETOF, $4.type);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SEQUENCE Constraint KW_OF Type
  {
    $$=new Type(Type::T_SEQOF, $4);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SEQUENCE Constraint KW_OF NamedType
  {
    delete $4.name;
    $$=new Type(Type::T_SEQOF, $4.type);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SEQUENCE SizeConstraint KW_OF Type
  {
    $$=new Type(Type::T_SEQOF, $4);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
| KW_SEQUENCE SizeConstraint KW_OF NamedType
  {
    delete $4.name;
    $$=new Type(Type::T_SEQOF, $4.type);
    Constraints *cons = new Constraints();
    if ($2) cons->add_con($2);
    $$->add_constraints(cons);
  }
;

Constraints:
  Constraint
  {
    $$ = new Constraints();
    if ($1) $$->add_con($1);
  }
| Constraints Constraint
  {
    $$ = $1;
    if ($2) $$->add_con($2);
  }
;

Constraint:
  "(" ConstraintSpec ExceptionSpec ")"
  {
    $$ = $2;
    delete $3;
  }
| "(" error ")"
  {
    $$ = new IgnoredConstraint("invalid constraint");
  }
;

/* 45.6 page 67 */
ConstraintSpec:
  SubtypeConstraint { $$ = $1; }
| GeneralConstraint { $$ = $1; }
;

/* 45.7 page 67 */
SubtypeConstraint:
  ElementSetSpecs { $$ = $1; }
;

/* 49.4 page 76 */
ExceptionSpec:
  "!" ExceptionIdentification {$$=$2;}
| /* empty */ {$$=0;}
| "!" error {$$=0;}
;

/* 49.4 page 76 */
ExceptionIdentification:
  SignedNumber {$$=new ExcSpec(0, $1);}
| DefinedValue {$$=new ExcSpec(0, $1);}
| Type ":" Value {$$=new ExcSpec($1, $3);}
;

/* 46.1 page 67 */
ElementSetSpecs:
  RootElementSetSpec
  {
    $$ = $1;
  }
| RootElementSetSpec "," TOK_Ellipsis
  {
    $$ = new ElementSetSpecsConstraint($1, NULL);
    $$->set_location(asn1_infile, @1.first_line);
  }
| RootElementSetSpec "," TOK_Ellipsis "," AdditionalElementSetSpec
  {
    $$ = new ElementSetSpecsConstraint($1, $5);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
RootElementSetSpec:
  ElementSetSpec
  {
    $$ = $1;
  }
;

/* 46.1 page 67 */
AdditionalElementSetSpec:
  ElementSetSpec
  {
    $$ = $1;
  }
;

/* 46.1 page 67 */
ElementSetSpec:
  Unions { $$ = $1; }
| KW_ALL Exclusions
  {
    $2->set_first_operand(new FullSetConstraint());
    $$ = $2;
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
Unions:
  Intersections { $$ = $1; }
| UElems UnionMark Intersections
  {
    $$ = new SetOperationConstraint($1, SetOperationConstraint::UNION, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
UElems:
  Unions
  {
    $$ = $1;
  }
;

/* 46.1 page 67 */
Intersections:
  IntersectionElements { $$ = $1; }
| IElems IntersectionMark IntersectionElements
  {
    $$ = new SetOperationConstraint($1, SetOperationConstraint::INTERSECTION, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
IElems:
  Intersections
  {
    $$ = $1;
  }
;

/* 46.1 page 67 */
IntersectionElements:
  Elements { $$ = $1; }
| Elems Exclusions
  {
    $2->set_first_operand($1);
    $$ = $2;
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
Elems:
  Elements
  {
    $$ = $1;
  }
;

/* 46.1 page 67 */
Exclusions:
  KW_EXCEPT Elements
  {
    $$ = new SetOperationConstraint(0, SetOperationConstraint::EXCEPT, $2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 46.1 page 67 */
UnionMark:
  "|"
| KW_UNION
;

/* 46.1 page 67 */
IntersectionMark:
  "^"
| KW_INTERSECTION
;

/* 46.5 page 68 */
Elements:
  SubtypeElements { $$ = $1; }
  /*
| ObjectSetElements
  */
| "(" ElementSetSpec ")"
  {
    $$ = $2;
  }
;

/* 47.1 page 69 */
SubtypeElements:
  SingleValue { $$ = $1; }
| ContainedSubtype { $$ = $1; }
| ValueRange { $$ = $1; }
| PermittedAlphabet { $$ = $1; }
| SizeConstraint { $$ = $1; }
| InnerTypeConstraints { $$ = $1; }
| PatternConstraint { $$ = $1; }
;

/* 47.2.1 page 70 */
SingleValue:
  Value_reg
  {
    $$ = new SingleValueConstraint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| LowerIdValue
  {
    $$ = new SingleValueConstraint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_Block
  {
    /** \todo: It would be more straightforward to create an undefined
     * constraint here and classify it later on during parsing or semantic
     * analysis to SimpleTableConstraint or SingleValue. */
    $$ = new TableConstraint($1, 0);
    $$->set_location(asn1_infile, @1.first_line);
  }
| ReferencedValue_reg
  {
    $$ = new SingleValueConstraint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.3.1 page 70
   TypeConstraint (47.6.1 page 72) has been removed because it's conflicting
   with second option of ContainedSubtype rule */
ContainedSubtype:
  KW_INCLUDES Type
  {
    $$ = new ContainedSubtypeConstraint($2, true);
    $$->set_location(asn1_infile, @1.first_line);
  }
| Type
  {
    $$ = new ContainedSubtypeConstraint($1, false);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.4.1 page 71 */
ValueRange:
  LowerEndpoint TOK_RangeSeparator UpperEndpoint
  {
    $$ = new ValueRangeConstraint($1,$3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

LowerEndpoint:
  LowerEndValue
  {
    $$ = $1;
  }
| LowerEndValue "<"
  {
    $$ = $1;
    $$->set_exclusive();
  }
;

UpperEndpoint:
  UpperEndValue
  {
    $$ = $1;
  }
| "<" UpperEndValue
  {
    $$ = $2;
    $$->set_exclusive();
  }
;

LowerEndValue:
  Value
  {
    $$ = new RangeEndpoint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| KW_MIN
  {
    $$ = new RangeEndpoint(RangeEndpoint::MIN);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

UpperEndValue:
  Value
  {
    $$ = new RangeEndpoint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
| KW_MAX
  {
    $$ = new RangeEndpoint(RangeEndpoint::MAX);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.5.1 page 71 */
SizeConstraint:
  KW_SIZE Constraint
  {
    $$ = new SizeConstraint($2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.7.1 page 72 */
PermittedAlphabet:
  KW_FROM Constraint
  {
    $$ = new PermittedAlphabetConstraint($2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.8.1 page 72 */
InnerTypeConstraints:
  KW_WITH KW_COMPONENT SingleTypeConstraint
  {
    $$ = $3;
    $$->set_location(asn1_infile, @1.first_line);
  }
| KW_WITH KW_COMPONENTS TOK_Block
  {
    $$ = new UnparsedMultipleInnerTypeConstraint($3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.8.3 page 72 */
SingleTypeConstraint:
  Constraint
  {
    $$ = new SingleInnerTypeConstraint($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* 47.8.4 page 72 */
MultipleTypeConstraints:
  TypeConstraints
  {
    $$ = $1;
    $$->set_partial(false);
    $$->set_location(asn1_infile, @1.first_line);
  }
| TOK_Ellipsis "," TypeConstraints
  {
    $$ = $3;
    $$->set_partial(true);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

TypeConstraints:
  NamedConstraint
  {
    $$ = new MultipleInnerTypeConstraint();
    $$->addNamedConstraint($1);
  }
| TypeConstraints "," NamedConstraint
  {
    $$ = $1;
    $$->addNamedConstraint($3);
  }
| error NamedConstraint
  {
    $$ = new MultipleInnerTypeConstraint();
    $$->addNamedConstraint($2);
  }
| TypeConstraints error
  {
    $$ = $1;
  }
;

NamedConstraint:
  TOK_LowerIdentifier ValueConstraint PresenceConstraint
  {
    $$ = new NamedConstraint($1, $2, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ValueConstraint:
  Constraint
  {
    $$ = $1;
  }
| /* empty */
  {
    $$ = NULL;
  }
;

PresenceConstraint:
  KW_PRESENT  { $$ = NamedConstraint::PC_PRESENT; }
| KW_ABSENT   { $$ = NamedConstraint::PC_ABSENT; }
| KW_OPTIONAL { $$ = NamedConstraint::PC_OPTIONAL; }
| /* empty */ { $$ = NamedConstraint::PC_NONE; }
;

/* 47.9.1 page 73 */
PatternConstraint:
  KW_PATTERN Value
  {
    $$ = new PatternConstraint($2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* X.682 clause 8.1 */
GeneralConstraint:
  UserDefinedConstraint
  {
    $$ = new IgnoredConstraint("user defined constraint");
    $$->set_location(asn1_infile, @1.first_line);
  }
| TableConstraint
  {
    $$=$1;
  }
| ContentsConstraint /* TODO? */
  {
    $$ = new IgnoredConstraint("content constraint");
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/* X.682 clause 9.1 */
UserDefinedConstraint:
  KW_CONSTRAINED KW_BY TOK_Block {delete $3;}
;

/* X.682 clause 11.1 */
ContentsConstraint:
  KW_CONTAINING Type {delete $2;}
| KW_ENCODED KW_BY Value {delete $3;}
| KW_CONTAINING Type KW_ENCODED KW_BY Value {delete $2; delete $5;}
;

/* X.682 clause 10.3 */
TableConstraint:
/*  SimpleTableConstraint
| */
  ComponentRelationConstraint { $$ = $1; }
;

/* commented out to eliminate reduce/reduce conflicts:
SimpleTableConstraint containing TOK_Block is reachable by path
Constraint->SubtypeConstraint->...->SingleValue->Value->TOK_Block

SimpleTableConstraint:
  ObjectSetSpec
;
*/

/* X.682 clause 10.7 */
/* the first block contains DefinedObjectSet, the second AtNotationList */
ComponentRelationConstraint:
  TOK_Block TOK_Block {$$=new TableConstraint($1, $2);}
;

DefinedObjectSetBlock:
  RefdUpper {$$=new OS_refd($1);}
;

AtNotationList:
  AtNotation {$$=new AtNotations(); $$->add_an($1);}
| AtNotationList "," AtNotation {$$=$1; $$->add_an($3);}
| error AtNotation {$$=new AtNotations(); $$->add_an($2);}
| AtNotationList error {$$=$1;}
;

AtNotation:
  // No `BIGNUM' for Levels.
  "@" Levels ComponentIdList {$$=new AtNotation($2, $3);}
;

Levels:
  /* empty */ {$$=0;}
| Level {$$=$1;}
;

Level:
  "." {$$=1;}
| TOK_RangeSeparator {$$=2;} /* ".." */
| TOK_Ellipsis {$$=3;} /* "..." */
| Level "." {$$=$1+1;}
| Level TOK_RangeSeparator {$$=$1+2;}
| Level TOK_Ellipsis {$$=$1+3;}
;

ComponentIdList:
  TOK_LowerIdentifier {$$=new FieldName(); $$->add_field($1);}
| ComponentIdList "." TOK_LowerIdentifier {$$=$1; $$->add_field($3);}
;


/*********************************************************************
 * X.681 - stuff
 *********************************************************************/

//ObjectClass:
//  DefinedObjectClass {$$=$1;}
//| ObjectClassDefn {$$=$1;}
///*
//| ParameterizedObjectClass
// */
//;

//DefinedObjectClass:
//  DefdUpper
//  {
//    $$=new OC_refd($1);
//    $$->set_location(asn1_infile, @1.first_line);
//  }
//;

/*
UsefulObjectClassReference:
  KW_TYPE_IDENTIFIER
| KW_ABSTRACT_SYNTAX
;
*/

/* X.681 clause 9.3 */
ObjectClassDefn:
  KW_CLASS TOK_Block WithSyntaxSpec_opt
  {
    $$=new OC_defn($2, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

FieldSpecList:
  FieldSpec
  {
    $$=new FieldSpecs(); $$->add_fs($1);
    $1->set_location(asn1_infile, @1.first_line);
  }
| FieldSpecList "," FieldSpec
  {
    $$=$1; $$->add_fs($3);
    $3->set_location(asn1_infile, @3.first_line);
  }
| error FieldSpec
  {
    $$=new FieldSpecs(); $$->add_fs($2);
    $2->set_location(asn1_infile, @2.first_line);
  }
| FieldSpecList error
  {$$=$1;}
;

/* X.681 clause 9.4 */
/** \todo add support for ValueSet and VariableType FieldSpecs
 * (new AST nodes are required) */
FieldSpec:
  TypeFieldSpec {$$=$1;}
| FixedTypeValueFieldSpec {$$=$1;}
/*
| VariableTypeValueFieldSpec {$$=$1;}
| FixedTypeValueSetFieldSpec {$$=$1;}
| VariableTypeValueSetFieldSpec {$$=$1;}
| ObjectFieldSpec {$$=$1;}
| ObjectSetFieldSpec {$$=$1;}
*/
| UndefFieldSpec {$$=$1;}
;

UNIQ_opt:
  KW_UNIQUE {$$=true;}
|  /* empty */ {$$=false;}
;

Dflt_Type:
  KW_DEFAULT Type {$$=$2;}
;

Dflt_Value:
  KW_DEFAULT Value {$$=$2;}
;

Dflt_Value_reg:
  KW_DEFAULT Value_reg {$$=$2;}
;

Dflt_NullValue:
  KW_DEFAULT NullValue {$$=$2;}
;

Dflt_Block:
  KW_DEFAULT TOK_Block {$$=$2;}
;

Dflt_RefdLower:
  KW_DEFAULT RefdLower {$$=$2;}
;

Dflt_RefdLower_reg:
  KW_DEFAULT RefdLower_reg {$$=$2;}
;

Dflt_LowerId:
  KW_DEFAULT TOK_LowerIdentifier {$$=$2;}
;

TypeFieldSpec:
  TOK_ampUpperIdentifier
  {$$=new FieldSpec_T($1, false, 0);}
| TOK_ampUpperIdentifier KW_OPTIONAL
  {$$=new FieldSpec_T($1, true, 0);}
| TOK_ampUpperIdentifier Dflt_Type
  {$$=new FieldSpec_T($1, false, $2);}
;

FixedTypeValueFieldSpec:
/* 1 */
  TOK_ampLowerIdentifier Type_reg UNIQ_opt
  {$$=new FieldSpec_V_FT($1, $2, $3, false, 0);}
| TOK_ampLowerIdentifier Type_reg UNIQ_opt KW_OPTIONAL
  {$$=new FieldSpec_V_FT($1, $2, $3, true, 0);}
| TOK_ampLowerIdentifier Type_reg UNIQ_opt Dflt_Value
  {$$=new FieldSpec_V_FT($1, $2, $3, false, $4);}

/* 2 */
| TOK_ampLowerIdentifier UpperFromObj UNIQ_opt
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, false, 0);
  }
| TOK_ampLowerIdentifier UpperFromObj UNIQ_opt KW_OPTIONAL
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, true, 0);
  }
| TOK_ampLowerIdentifier UpperFromObj UNIQ_opt Dflt_Value
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, false, $4);
  }

/* 3 */
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, 0);
  }
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE KW_OPTIONAL
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, true, true, 0);
  }

| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE Dflt_Value_reg
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, $4);
  }
| TOK_ampLowerIdentifier SimplDefdUpper Dflt_Value_reg
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, $3);
  }
| TOK_ampLowerIdentifier PardUpper Dflt_Value_reg
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, $3);
  }

/* 4 */
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE Dflt_NullValue
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, $4);
  }
| TOK_ampLowerIdentifier SimplDefdUpper Dflt_NullValue
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, $3);
  }
| TOK_ampLowerIdentifier PardUpper Dflt_NullValue
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, $3);
  }

/* 5 */
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE Dflt_Block
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_UNDEF_BLOCK, $4);
    v->set_location(asn1_infile, @4.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, v);
  }

/* 6 */
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE Dflt_LowerId
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_UNDEF_LOWERID, $4);
    v->set_location(asn1_infile, @4.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, v);
  }

/* 7 */
| TOK_ampLowerIdentifier DefdUpper KW_UNIQUE Dflt_RefdLower_reg
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_REFD, $4);
    v->set_location(asn1_infile, @4.first_line);
    $$=new FieldSpec_V_FT($1, t, true, false, v);
  }

/* 8-9 */
| TOK_ampLowerIdentifier PardUpper
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, 0);
  }
| TOK_ampLowerIdentifier PardUpper KW_OPTIONAL
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, false, true, 0);
  }
| TOK_ampLowerIdentifier PardUpper Dflt_Block
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_UNDEF_BLOCK, $3);
    v->set_location(asn1_infile, @3.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, v);
  }
| TOK_ampLowerIdentifier PardUpper Dflt_LowerId
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_UNDEF_LOWERID, $3);
    v->set_location(asn1_infile, @3.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, v);
  }
| TOK_ampLowerIdentifier PardUpper Dflt_RefdLower_reg
  {
    Type *t = new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    Value *v = new Value(Value::V_REFD, $3);
    v->set_location(asn1_infile, @3.first_line);
    $$=new FieldSpec_V_FT($1, t, false, false, v);
  }

/* ObjectClassFieldType */
/* UpperFromObjs szetbombazva */
| TOK_ampLowerIdentifier PardUpper_FromObjs UNIQ_opt
  {
    Type *t=new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, false, 0);
  }
/* SimplUpper_FromObjs szetbombazva */
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameLower UNIQ_opt
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, 0);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUpper UNIQ_opt
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, 0);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUppers UNIQ_opt
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, 0);
  }
/* UpperFromObjs szetbombazva */
| TOK_ampLowerIdentifier PardUpper_FromObjs UNIQ_opt KW_OPTIONAL
  {
    Type *t=new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, true, 0);
  }
/* SimplUpper_FromObjs szetbombazva */
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameLower UNIQ_opt KW_OPTIONAL
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, true, 0);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUpper UNIQ_opt KW_OPTIONAL
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, true, 0);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUppers UNIQ_opt
  KW_OPTIONAL
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, true, 0);
  }
/* UpperFromObjs szetbombazva */
| TOK_ampLowerIdentifier PardUpper_FromObjs UNIQ_opt Dflt_Value
  {
    Type *t=new Type(Type::T_REFD, $2);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $3, false, $4);
  }
/* SimplUpper_FromObjs szetbombazva */
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameLower UNIQ_opt Dflt_Value
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, $6);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUpper UNIQ_opt Dflt_Value
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, $6);
  }
| TOK_ampLowerIdentifier SimplDefdUpper "." FieldNameUppers UNIQ_opt Dflt_Value
  {
    FromObj *fo=new FromObj($2, $4);
    fo->set_location(asn1_infile, @2.first_line);
    Type *t=new Type(Type::T_REFD, fo);
    t->set_location(asn1_infile, @2.first_line);
    $$=new FieldSpec_V_FT($1, t, $5, false, $6);
  }
;

/** \todo PardUpper as governor can be either Type or OC reference
 * the respective rules of FixedTypeValueFieldSpec should be moved here */
UndefFieldSpec:
/* &lower */
  TOK_ampLowerIdentifier SimplDefdUpper
  {$$=new FieldSpec_Undef($1, $2, false, 0);}
| TOK_ampLowerIdentifier SimplDefdUpper KW_OPTIONAL
  {$$=new FieldSpec_Undef($1, $2, true, 0);}
| TOK_ampLowerIdentifier SimplDefdUpper Dflt_Block
  {$$=new FieldSpec_Undef($1, $2, false, $3);}
| TOK_ampLowerIdentifier SimplDefdUpper Dflt_RefdLower
  {$$=new FieldSpec_Undef($1, $2, false, $3);}
/* &upper */
| TOK_ampUpperIdentifier SimplDefdUpper
  {$$=new FieldSpec_Undef($1, $2, false, 0);}
| TOK_ampUpperIdentifier SimplDefdUpper KW_OPTIONAL
  {$$=new FieldSpec_Undef($1, $2, true, 0);}
| TOK_ampUpperIdentifier SimplDefdUpper Dflt_Block
  {$$=new FieldSpec_Undef($1, $2, false, $3);}
;

/*
VariableTypeValueFieldSpec:
  TOK_ampLowerIdentifier FieldName {}
;

FixedTypeValueSetFieldSpec:
  TOK_ampUpperIdentifier Type ValueSetOptionalitySpec {}
;

ValueSetOptionalitySpec:
  KW_OPTIONAL {}
| KW_DEFAULT ValueSet {}
| / * empty * / {}
;

VariableTypeValueSetFieldSpec:
  TOK_ampUpperIdentifier FieldName ValueSetOptionalitySpec {}
;

ObjectFieldSpec:
  TOK_ampLowerIdentifier DefinedObjectClass ObjectOptionalitySpec {}
;

ObjectOptionalitySpec:
  KW_OPTIONAL {}
| KW_DEFAULT Object {}
| / * empty * / {}
;

ObjectSetFieldSpec:
  TOK_ampUpperIdentifier DefinedObjectClass ObjectSetOptionalitySpec {}
;

ObjectSetOptionalitySpec:
  KW_OPTIONAL {}
| KW_DEFAULT ObjectSet {}
| / * empty * / {}
;
*/

WithSyntaxSpec_opt:
  KW_WITH KW_SYNTAX TOK_Block {$$=$3;}
| /* empty */ {$$=0;}
;

Object:
  DefinedObject {$$=$1;}
| ObjectDefn {$$=$1;}
/*
| ObjectFromObject
| ParameterizedObject
*/
;

DefinedObject:
  RefdLower
  {
    $$=new Obj_refd($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ObjectDefn:
  TOK_Block
  {
    $$=new Obj_defn($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ObjectSet:
  TOK_Block
  {
    $$=new OS_defn($1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

ObjectSetElements:
  Object {$$=$1;}
| RefdUpper
  {
    OS_refd *os_refd=new OS_refd($1);
    os_refd->set_location(asn1_infile, @1.first_line);
    $$=os_refd;
  }
;

ObjectSetSpec:
  ObjectSetSpec0 {$$=$1;}
| ObjectSetSpec0 "," TOK_Ellipsis {$$=$1;}
| TOK_Ellipsis
  {$$=new OS_defn(); $$->set_location(asn1_infile, @1.first_line);}
| TOK_Ellipsis "," ObjectSetSpec0 {$$=$3;}
| ObjectSetSpec0 "," TOK_Ellipsis "," ObjectSetSpec0
  {$$=$1; $$->steal_oses($5);}
;

/*
  this is a temporary solution. only unions are supported
  (intersections, and except are not...)
 */
ObjectSetSpec0:
  ObjectSetElements
  {
    $$=new OS_defn();
    $$->set_location(asn1_infile, @1.first_line);
    $$->add_ose($1);
  }
| ObjectSetSpec0 UnionMark ObjectSetElements {$$=$1; $$->add_ose($3);}
| error ObjectSetElements
  {
    $$=new OS_defn();
    $$->set_location(asn1_infile, @2.first_line);
    $$->add_ose($2);
  }
| ObjectSetSpec0 error {$$=$1;}
;

/*********************************************************************
 * Type - stuff
 *********************************************************************/

/* 16.3 page 30 */
ReferencedType:
  RefdUpper {$$=new Type(Type::T_REFD, $1);}
/*
  includes TypeFromObject, ValueSetFromObjects and DefinedType
*/
/*
| SelectionType {$$=$1;}
*/
;

//DefinedType:
//  DefdUpper {$$=new Type_refd($1);}
///*
//| ParameterizedType
//| ParameterizedValueSetType
//*/
//;

/*********************************************************************
 * Value - stuff
 *********************************************************************/

/* 16.7 page 31 */
Value:
  ObjectClassFieldValue
  {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
;

Value_reg:
  ObjectClassFieldValue_reg
  {$$=$1; $$->set_location(asn1_infile, @1.first_line);}
;

ObjectClassFieldValue:
  FixedTypeFieldValue {$$=$1;}
/*
| OpenTypeFieldValue
*/
;

ObjectClassFieldValue_reg:
  FixedTypeFieldValue_reg {$$=$1;}
/*
| OpenTypeFieldValue
*/
;

FixedTypeFieldValue:
  BuiltinValue {$$=$1;}
| ReferencedValue_reg {$$=$1;}
;

FixedTypeFieldValue_reg:
  BuiltinValue_reg {$$=$1;}
;

ReferencedValue:
  RefdLower
  {
    $$=new Value(Value::V_REFD, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
/*
| ValueFromObject
*/
;

ReferencedValue_reg:
  RefdLower_reg
  {
    $$=new Value(Value::V_REFD, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
/*
| ValueFromObject
*/
;

DefinedValue:
  DefdLower
  {
    $$=new Value(Value::V_REFD, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

DefinedValue_reg:
  DefdLower_reg
  {
    $$=new Value(Value::V_REFD, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
/*
| ParameterizedValue
*/
;

Val_Number:
  TOK_Number {$$=new Value(Value::V_INT, $1);}
;

Val_CString:
  TOK_CString {$$=new Value(Value::V_CSTR, $1);}
;

/*********************************************************************
 * Named Type/Value
 *********************************************************************/

NamedType:
  TOK_LowerIdentifier Type
  {
    $$.name = $1;
    $$.type = $2;
  }
;

NamedValue:
  TOK_LowerIdentifier Value
  {
    $$=new NamedValue($1, $2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/*********************************************************************
 * FieldName stuff
 *********************************************************************/

/*
FieldName:
  PrimitiveFieldNameList {}
;

PrimitiveFieldNameList:
  PrimitiveFieldName {}
| PrimitiveFieldNameList "." PrimitiveFieldName {}
;

PrimitiveFieldName:
  TOK_ampUpperIdentifier {}
| TOK_ampLowerIdentifier {}
;
*/

FieldNameLower:
  TOK_ampLowerIdentifier {$$=new FieldName(); $$->add_field($1);}
| FieldNameLower "." TOK_ampLowerIdentifier {$$=$1; $$->add_field($3);}
;

FieldNameUpper:
  TOK_ampUpperIdentifier {$$=new FieldName(); $$->add_field($1);}
| FieldNameLower "." TOK_ampUpperIdentifier {$$=$1; $$->add_field($3);}
;

FieldNameUppers:
  FieldNameUpper "." TOK_ampLowerIdentifier {$$=$1; $$->add_field($3);}
| FieldNameUpper "." TOK_ampUpperIdentifier {$$=$1; $$->add_field($3);}
| FieldNameUppers "." TOK_ampLowerIdentifier {$$=$1; $$->add_field($3);}
| FieldNameUppers "." TOK_ampUpperIdentifier {$$=$1; $$->add_field($3);}
;

/*********************************************************************
 * Referenced upper stuff
 *********************************************************************/

RefdUpper:
  DefdUpper {$$=$1;}
| UpperFromObj {$$=$1;}
| FromObjs {$$=$1;}
;

UpperFromObj:
  DefdLower "." FieldNameUpper
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

FromObjs:
  Upper_FromObjs {$$=$1;}
| Lower_FromObjs {$$=$1;}
;

/*
Upper_FromObjs:
  DefdUpper "." FieldNameLower
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| DefdUpper "." FieldNameUpper
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| DefdUpper "." FieldNameUppers
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;
*/

Upper_FromObjs:
  SimplUpper_FromObjs {$$=$1;}
| PardUpper_FromObjs {$$=$1;}
;

SimplUpper_FromObjs:
  SimplDefdUpper "." FieldNameLower
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| SimplDefdUpper "." FieldNameUpper
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| SimplDefdUpper "." FieldNameUppers
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

PardUpper_FromObjs:
  PardUpper "." FieldNameLower
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| PardUpper "." FieldNameUpper
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
| PardUpper "." FieldNameUppers
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

Lower_FromObjs:
  DefdLower "." FieldNameUppers
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

DefdUpper:
  SimplDefdUpper {$$=$1;}
| PardUpper {$$=$1;}
;

PardUpper:
  SimplDefdUpper TOK_Block
  {
    $$=new Ref_pard($1, $2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

SimplDefdUpper:
  ExtUpperRef {$$=$1;}
| UpperRef {$$=$1;}
;

ExtUpperRef:
  TOK_UpperIdentifier "." TOK_UpperIdentifier
  {
    $$=new Ref_defd_simple($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

UpperRef:
  TOK_UpperIdentifier
  {
    $$=new Ref_defd_simple(0, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

/*********************************************************************
 * Referenced lower stuff
 *********************************************************************/

RefdLower_reg:
  DefdLower_reg {$$=$1;}
| LowerFromObj {$$=$1;}
;

RefdLower:
  DefdLower {$$=$1;}
| LowerFromObj {$$=$1;}
;

LowerFromObj:
  DefdLower "." FieldNameLower
  {
    $$=new FromObj($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

DefdLower_reg:
  SimplDefdLower_reg {$$=$1;}
| PardLower {$$=$1;}
;

DefdLower:
  SimplDefdLower {$$=$1;}
| PardLower {$$=$1;}
;

PardLower:
  SimplDefdLower TOK_Block
  {
    $$=new Ref_pard($1, $2);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

SimplDefdLower_reg:
  ExtLowerRef {$$=$1;}
;

SimplDefdLower:
  ExtLowerRef {$$=$1;}
| LowerRef {$$=$1;}
;

ExtLowerRef:
  TOK_UpperIdentifier "." TOK_LowerIdentifier
  {
    $$=new Ref_defd_simple($1, $3);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

LowerRef:
  TOK_LowerIdentifier
  {
    $$=new Ref_defd_simple(0, $1);
    $$->set_location(asn1_infile, @1.first_line);
  }
;

%%

/*********************************************************************
 * Interface
 *********************************************************************/

int asn1_parse_file(const char* filename, boolean generate_code)
{
  asn1_yy_parse_internal=false;
  int retval=0;
  asn1la_newfile(filename);
  yyin=fopen(filename, "r");
  if(yyin==NULL) {
    ERROR("Cannot open input file `%s': %s",
          filename, strerror(errno));
    return -1;
  }
  yy_buffer_state *flex_buffer = asn1_yy_create_buffer(yyin, YY_BUF_SIZE);
  asn1_yy_switch_to_buffer(flex_buffer);
  act_asn1_module=0;
  NOTIFY("Parsing ASN.1 module `%s'...", filename);
  retval=yyparse();
  fclose(yyin);
  asn1_yylex_destroy();
  if(retval==0 && act_asn1_module!=0) {
    act_asn1_module->set_location(filename);
    if (generate_code) act_asn1_module->set_gen_code();
    modules->add_mod(act_asn1_module);
  }
  act_asn1_module=0;
  return retval;
}

/**
 * This functions is used to parse internal stuff. :)
 */
int asn1_parse_string(const char* p_str)
{
  unsigned verb_level_backup=verb_level;
  verb_level=0; // be vewy, vewy quiet
  asn1la_newfile("<internal>");

  asn1_yy_parse_internal=true;
  yy_buffer_state *flex_buffer = asn1_yy_scan_string(p_str);
  if(flex_buffer == NULL) {
    ERROR("Flex buffer creation failed.");
    verb_level=verb_level_backup;
    return -1;
  }
  int retval=yyparse();
  asn1_yylex_destroy();
  verb_level=verb_level_backup;
  return retval;
}



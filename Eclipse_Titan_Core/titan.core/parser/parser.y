/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/

/*

Parser source code for TTCN-3 (input of bison)
Rev: PA10
Date: April 15 2005
Author: Janos Zoltan Szabo (ejnosza)

Revision history:

Rev.	Date		Author		Comments
PA1	Nov 10 2000	tmpjsz		Generated from the prototype compiler.
					(Actions were removed.)
PA2	Dec  5 2000	tmpjsz		Many minor changes.
					Updated according to BNF v1.0.9.
PA3	Dec 11 2000	tmpjsz		Added support for module definitive
					identifiers.
PA5	Sep 19 - Oct 1 2001	tmpjsz	Upgrade to BNF v1.1.2.
PA6	Apr 16-18 2002	tmpjsz		Upgrade to BNF v2.2.0 (Rev. 12.5)
PA7	Nov 26-27 2002	tmpjsz		Upgrade to BNF v2.2.1 (Rev. 12.7)
PA8	May 10-13 2004	ejnosza		Upgrade to BNF v3.0.0Mockupv1
PA9	March 2005	ejnosza		Added support for multi-dimension
					sub-references in port/timer/component
					operations.
PA10	Apr 13-15 2005	ejnosza		Upgrade to final BNF v3.0.0

*/

%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void yyerror(char *s);
int yylex();

#define YYERROR_VERBOSE

%}

/* Bison declarations */

/* Terminal symbols of TTCN-3 */

/* Inline constants */

%token Number
%token FloatValue
%token Identifier

%token BooleanConst
%token VerdictConst
%token Bstring
%token Hstring
%token Ostring
%token Cstring
%token NullKeyword
%token BitStringMatch
%token HexStringMatch
%token OctetStringMatch

/* Keywords */

%token ActionKeyword
%token ActivateKeyword
%token AddressKeyword
%token AliveKeyword
%token AllKeyword
%token AltKeyword
%token AltstepKeyword
%token AnyKeyword
%token AnyTypeKeyword
%token BitStringKeyword
%token BooleanKeyword
%token CallOpKeyword
%token CaseKeyword
%token CatchOpKeyword
%token CharKeyword
%token CharStringKeyword
%token CheckOpKeyword
%token ClearOpKeyword
%token ComplementKeyword
%token ComponentKeyword
%token ConnectKeyword
%token ConstKeyword
%token ControlKeyword
%token CreateKeyword
%token DeactivateKeyword
%token DefaultKeyword
%token DisconnectKeyword
%token DisplayKeyword
%token DoKeyword
%token DoneKeyword
%token ElseKeyword
%token EncodeKeyword
%token EnumKeyword
%token ExceptKeyword
%token ExceptionKeyword
%token ExecuteKeyword
%token ExtendsKeyword
%token ExtensionKeyword
%token ExtKeyword
%token FloatKeyword
%token ForKeyword
%token FromKeyword
%token FunctionKeyword
%token GetCallOpKeyword
%token GetReplyOpKeyword
%token GetVerdictKeyword
%token GotoKeyword
%token GroupKeyword
%token HexStringKeyword
%token IfKeyword
%token IfPresentKeyword
%token ImportKeyword
%token InParKeyword
%token InfinityKeyword
%token InOutParKeyword
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
%token ModifiesKeyword
%token TTCN3ModuleKeyword
%token ModuleParKeyword
%token MTCKeyword
%token NoBlockKeyword
%token NowaitKeyword
%token ObjectIdentifierKeyword
%token OctetStringKeyword
%token OfKeyword
%token OmitKeyword
%token OnKeyword
%token OptionalKeyword
%token OutParKeyword
%token OverrideKeyword
%token ParamKeyword
%token PatternKeyword
%token PermutationKeyword
%token PortKeyword
%token ProcedureKeyword
%token RaiseKeyword
%token ReadKeyword
%token ReceiveOpKeyword
%token RecordKeyword
%token RecursiveKeyword
%token RepeatKeyword
%token ReplyKeyword
%token ReturnKeyword
%token RunningKeyword
%token RunsKeyword
%token SelectKeyword
%token SelfOp
%token SendOpKeyword
%token SenderKeyword
%token SetKeyword
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

/* Keywords combined with a leading dot */

%token DotAliveKeyword
%token DotCallOpKeyword
%token DotCatchOpKeyword
%token DotCheckOpKeyword
%token DotClearOpKeyword
%token DotCreateKeyword
%token DotDoneKeyword
%token DotGetCallOpKeyword
%token DotGetReplyOpKeyword
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

/* Multi-character operators */

%token AssignmentChar ":="
%token DotDot ".."
%token PortRedirectSymbol "->"
%token EQ "=="
%token NE "!="
%token GE ">="
%token LE "<="
%token Or "or"
%token Or4b "or4b"
%token Xor "xor"
%token Xor4b "xor4b"
%token Mod "mod"
%token Rem "rem"
%token And "and"
%token And4b "and4b"
%token Not "not"
%token Not4b "not4b"
%token SL "<<"
%token SR ">>"
%token RL "<@"
%token RR "@>"

/* Operator precedences */

/*
%left Or
%left Xor
%left And
%left Not
%left EQ NE
%left '<' '>' GE LE
%left SL SR RL RR
%left Or4b
%left Xor4b
%left And4b
%left Not4b
%left '+' '-' '&'
%left '*' '/' Mod Rem
%left UnarySign
*/

%expect 16

/*
Source of conflicts (16 S/R):

1.) 7 conflicts in one state
The Expression after 'return' keyword is optional in ReturnStatement.
For 7 tokens the parser cannot decide whether the token is a part of
the return expression (shift) or it is the beginning of the next statement
(reduce).

2.) 8 distinct states, each with one conflict caused by token '['
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

3.) 1 conflict
The sequence identifier.objid can be either the beginning of a module name
qualified with a module object identifier (shift) or a reference to an objid
value within an entity of type anytype (reduce).

Note that the parser implemented by bison always chooses to shift instead of
reduce in case of conflicts. This is the desired behaviour in situations 1.)
and 2.) since the opposite alternative can be forced by the correct usage
of semi-colons.
*/

%%

/* The grammar of TTCN-3 */

/* A.1.6.0 TTCN-3 module */

TTCN3Module:
	TTCN3ModuleKeyword TTCN3ModuleId
	'{'
	optModuleDefinitionsPart
	optModuleControlPart
	'}'
	optWithStatementAndSemiColon
;

TTCN3ModuleId:
	Identifier optDefinitiveIdentifier optLanguageSpec
;

ModuleId:
	GlobalModuleId optLanguageSpec
;

GlobalModuleId:
	Identifier
	| Identifier ObjectIdentifierValue
;

optLanguageSpec:
	/* empty */
	| LanguageKeyword FreeText
;

/* A.1.6.1 Module definitions part */

/* A.1.6.1.0 General */

optModuleDefinitionsPart:
	/* empty */
	| ModuleDefinitionsList
;

ModuleDefinitionsList:
	ModuleDefinition optSemiColon
	| ModuleDefinitionsList ModuleDefinition optSemiColon
;

ModuleDefinition:
	ModuleDef optWithStatement
;

ModuleDef:
	TypeDef
	| ConstDef
	| TemplateDef
	| ModuleParDef
	| FunctionDef
	| SignatureDef
	| TestcaseDef
	| AltstepDef
	| ImportDef
	| GroupDef
	| ExtFunctionDef
	| ExtConstDef
;

/* A.1.6.1.1 Typedef definitions */

TypeDef:
	TypeDefKeyword TypeDefBody
;

TypeDefBody:
	StructuredTypeDef
	| SubTypeDef
;

StructuredTypeDef:
	RecordDef
	| UnionDef
	| SetDef
	| RecordOfDef
	| SetOfDef
	| EnumDef
	| PortDef
	| ComponentDef
;

RecordDef:
	RecordKeyword StructDefBody
;

StructDefBody:
	Identifier optStructDefFormalParList
	'{' optStructFieldDefList '}'
	| AddressKeyword '{' optStructFieldDefList '}'
;

optStructDefFormalParList:
	/* empty */
	|  '(' StructDefFormalParList ')'
;

StructDefFormalParList:
	StructDefFormalPar
	| StructDefFormalParList ',' StructDefFormalPar
;

StructDefFormalPar:
	FormalValuePar
;

optStructFieldDefList:
	/* empty */
	| StructFieldDefList
;

StructFieldDefList:
	StructFieldDef
	| StructFieldDefList ',' StructFieldDef
;

StructFieldDef:
	TypeOrNestedTypeDef Identifier optArrayDef optSubTypeSpec
	| TypeOrNestedTypeDef Identifier optArrayDef optSubTypeSpec
	OptionalKeyword
;

TypeOrNestedTypeDef:
	Type
	| NestedTypeDef
;

NestedTypeDef:
	NestedRecordDef
	| NestedUnionDef
	| NestedSetDef
	| NestedRecordOfDef
	| NestedSetOfDef
	| NestedEnumDef
;

NestedRecordDef:
	RecordKeyword '{' optStructFieldDefList '}'
;

NestedUnionDef:
	UnionKeyword '{' UnionFieldDefList '}'
;

NestedSetDef:
	SetKeyword '{' optStructFieldDefList '}'
;

NestedRecordOfDef:
	RecordKeyword optStringLength OfKeyword TypeOrNestedTypeDef
;

NestedSetOfDef:
	SetKeyword optStringLength OfKeyword TypeOrNestedTypeDef
;

NestedEnumDef:
	EnumKeyword '{' EnumerationList '}'
;

UnionDef:
	UnionKeyword UnionDefBody
;

UnionDefBody:
	Identifier optStructDefFormalParList '{' UnionFieldDefList '}'
	| AddressKeyword '{' UnionFieldDefList '}'

;

UnionFieldDefList:
	UnionFieldDef
	| UnionFieldDefList ',' UnionFieldDef
;

UnionFieldDef:
	TypeOrNestedTypeDef Identifier optArrayDef optSubTypeSpec
;

SetDef:
	SetKeyword StructDefBody
;

RecordOfDef:
	RecordKeyword optStringLength OfKeyword StructOfDefBody
;

StructOfDefBody:
	TypeOrNestedTypeDef Identifier optSubTypeSpec
	| TypeOrNestedTypeDef AddressKeyword optSubTypeSpec
;

SetOfDef:
	SetKeyword optStringLength OfKeyword StructOfDefBody
;

EnumDef:
	EnumKeyword Identifier '{' EnumerationList '}'
	| EnumKeyword AddressKeyword '{' EnumerationList '}'
;

EnumerationList:
	Enumeration
	| EnumerationList ',' Enumeration
;

Enumeration:
	Identifier
	| Identifier '(' Number ')'
	| Identifier '(' Minus Number ')'
;

SubTypeDef:
	Type Identifier optArrayDef optSubTypeSpec
	| Type AddressKeyword optArrayDef optSubTypeSpec
;

optSubTypeSpec:
	/* empty */
	| AllowedValues
	| AllowedValues StringLength
	| StringLength
;

AllowedValues:
	'(' seqValueOrRange ')'
	| CharStringMatch
;

seqValueOrRange:
	ValueOrRange
	| seqValueOrRange ',' ValueOrRange
;

ValueOrRange:
	RangeDef
	| Expression
;

RangeDef:
	LowerBound DotDot UpperBound
;

optStringLength:
	/* empty */
	| StringLength
;

StringLength:
	LengthKeyword '(' SingleExpression ')'
	| LengthKeyword '(' SingleExpression DotDot UpperBound ')'
;

PortType:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

PortDef:
	PortKeyword PortDefBody
;

PortDefBody:
	Identifier PortDefAttribs
;

PortDefAttribs:
	MessageAttribs
	| ProcedureAttribs
	| MixedAttribs
;

MessageAttribs:
	MessageKeyword
	'{' MessageListList '}'
;

MessageListList:
	MessageList optSemiColon
	| MessageListList MessageList optSemiColon
;

MessageList:
	Direction AllOrTypeList
;

Direction:
	InParKeyword
	| OutParKeyword
	| InOutParKeyword
;

AllOrTypeList:
	AllKeyword
	| TypeList
;

TypeList:
	Type
	| TypeList ',' Type
;

ProcedureAttribs:
	ProcedureKeyword
	'{' ProcedureListList '}'
;

ProcedureListList:
	ProcedureList optSemiColon
	| ProcedureListList ProcedureList optSemiColon
;

ProcedureList:
	Direction AllOrSignatureList
;

AllOrSignatureList:
	AllKeyword
	| SignatureList
;

SignatureList:
	Signature
	| SignatureList ',' Signature
;

MixedAttribs:
	MixedKeyword
	'{' MixedListList '}'
;

MixedListList:
	MixedList optSemiColon
	| MixedListList MixedList optSemiColon
;

MixedList:
	Direction ProcOrTypeList
;

ProcOrTypeList:
	seqProcOrType
	| AllKeyword
;

seqProcOrType:
	ProcOrType
	| seqProcOrType ',' ProcOrType
;

ProcOrType:
/*	Signature -- covered by Type */
	Type
;

ComponentDef:
	ComponentKeyword Identifier
	optExtendsDef
	'{' ComponentDefList '}'
;

optExtendsDef:
	/* empty */
	| ExtendsKeyword ComponentTypeList
;

ComponentTypeList:
	ComponentType
	| ComponentTypeList ',' ComponentType
;

ComponentType:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

ComponentDefList:
	/* empty */
	| ComponentDefList ComponentElementDef optSemiColon
;

ComponentElementDef:
	PortInstance
	| VarInstance
	| TimerInstance
	| ConstDef
;

PortInstance:
	PortKeyword PortType PortElementList
;

PortElementList:
	PortElement
	| PortElementList ',' PortElement
;

PortElement:
	Identifier optArrayDef
;

/* A.1.6.1.2 Constant definitions */

ConstDef:
	ConstKeyword Type ConstList
;

ConstList:
	SingleConstDef
	| ConstList ',' SingleConstDef
;

SingleConstDef:
	Identifier optArrayDef AssignmentChar Expression
;

/* A.1.6.1.3 Template definitions */

TemplateDef:
	TemplateKeyword BaseTemplate optDerivedDef AssignmentChar TemplateBody
;

BaseTemplate:
	Type Identifier optTemplateFormalParList
/*	| Signature Identifier optTemplateFormalParList -- covered by
	the previous rule */
;

optDerivedDef:
	/* empty */
	| ModifiesKeyword TemplateRef
;

optTemplateFormalParList:
	/* empty */
	| '(' TemplateFormalParList ')'
;

TemplateFormalParList:
	TemplateFormalPar
	| TemplateFormalParList ',' TemplateFormalPar
;

TemplateFormalPar:
	FormalValuePar
	| FormalTemplatePar
;

TemplateBody:
	SimpleSpec optExtraMatchingAttributes
	| FieldSpecList optExtraMatchingAttributes
	| ArrayValueOrAttrib optExtraMatchingAttributes
;

SimpleSpec:
	SingleValueOrAttrib
;

FieldSpecList:
	'{' '}'
	| '{' seqFieldSpec '}'
;

seqFieldSpec:
	FieldSpec
	| seqFieldSpec ',' FieldSpec
;

FieldSpec:
	FieldReference AssignmentChar TemplateBody
;

FieldReference:
	StructFieldRef
	| ArrayOrBitRef
/*	| ParRef -- covered by StructFieldRef */
;

StructFieldRef:
	Identifier
	| PredefinedType
/*	| TypeReference
Note: Non-parameterized type references are covered by (StructField)Identifier.
      Parameterized type references are covered by FunctionInstance */
	| FunctionInstance
;

ParRef:
	Identifier
;

ArrayOrBitRef:
	'[' FieldOrBitNumber ']'
;

FieldOrBitNumber:
	SingleExpression
;

SingleValueOrAttrib:
	MatchingSymbol
	| SingleExpression
/*	| TemplateRefWithParList -- covered by SingleExpression */
;

ArrayValueOrAttrib:
	'{' ArrayElementSpecList '}'
;

ArrayElementSpecList:
	ArrayElementSpec
	| ArrayElementSpecList ',' ArrayElementSpec
;

ArrayElementSpec:
	NotUsedSymbol
	| PermutationMatch
	| TemplateBody
;

NotUsedSymbol:
	Dash
;

MatchingSymbol:
	Complement
	| AnyValue
	| AnyOrOmit
	| ValueOrAttribList
	| Range
	| BitStringMatch
	| HexStringMatch
	| OctetStringMatch
	| CharStringMatch
	| SubsetMatch
	| SupersetMatch
;

optExtraMatchingAttributes:
	/* empty */
	| LengthMatch
	| IfPresentMatch
	| LengthMatch IfPresentMatch
;

CharStringMatch:
	PatternKeyword Cstring
;

Complement:
	ComplementKeyword ValueList
;

ValueList:
	'(' seqValue ')'
;

seqValue:
	Expression
	| seqValue ',' Expression
;

SubsetMatch:
	SubsetKeyword ValueList
;

SupersetMatch:
	SupersetKeyword ValueList
;

PermutationMatch:
	PermutationKeyword PermutationList
;

PermutationList:
	'(' seqValueOrAttrib ')'
;

AnyValue:
	'?'
;

AnyOrOmit:
	'*'
;

ValueOrAttribList:
	'(' TemplateBody ',' seqValueOrAttrib ')'
;

seqValueOrAttrib:
	TemplateBody
	| seqValueOrAttrib ',' TemplateBody
;

LengthMatch:
	StringLength
;

IfPresentMatch:
	IfPresentKeyword
;

Range:
	'(' LowerBound DotDot UpperBound ')'
;

LowerBound:
	SingleExpression
	| Minus InfinityKeyword
;

UpperBound:
	SingleExpression
	| InfinityKeyword
;

TemplateInstance:
	InLineTemplate
;

TemplateRefWithParList:
	Identifier optTemplateActualParList
	| Identifier Dot Identifier optTemplateActualParList
	| Identifier Dot ObjectIdentifierValue Dot Identifier
	optTemplateActualParList
;

TemplateRef:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

InLineTemplate:
	TemplateBody
	| Type Colon TemplateBody
/* 	| Signature Colon TemplateBody -- covered by the previous rule */
	| DerivedRefWithParList AssignmentChar TemplateBody
	| Type Colon DerivedRefWithParList AssignmentChar TemplateBody
/*	| Signature Colon DerivedRefWithParList AssignmentChar TemplateBody
 -- covered by the previous rule */
;

DerivedRefWithParList:
	ModifiesKeyword TemplateRefWithParList
;

optTemplateActualParList:
	/* empty */
	| '(' seqTemplateActualPar ')'
;

seqTemplateActualPar:
	TemplateActualPar
	| seqTemplateActualPar ',' TemplateActualPar
;

TemplateActualPar:
	TemplateInstance
;

TemplateOps:
	MatchOp
	| ValueofOp
;

MatchOp:
	MatchKeyword '(' Expression ',' TemplateInstance ')'
;

ValueofOp:
	ValueofKeyword '(' TemplateInstance ')'
;

/* A.1.6.1.4 Function definitions */

FunctionDef:
	FunctionKeyword Identifier
	'(' optFunctionFormalParList ')' optRunsOnSpec optReturnType
	StatementBlock
;

optFunctionFormalParList:
	/* empty */
	| FunctionFormalParList
;

FunctionFormalParList:
	FunctionFormalPar
	| FunctionFormalParList ',' FunctionFormalPar
;

FunctionFormalPar:
	FormalValuePar
	| FormalTimerPar
	| FormalTemplatePar
/*	| FormalPortPar -- covered by FormalValuePar */
;

optReturnType:
	/* empty */
	| ReturnType
;

ReturnType:
	ReturnKeyword Type
	| ReturnKeyword TemplateKeyword Type
;

optRunsOnSpec:
	/* empty */
	| RunsOnSpec
;

RunsOnSpec:
	RunsKeyword OnKeyword ComponentType
;

StatementBlock:
	'{' optFunctionStatementOrDefList '}'
;

optFunctionStatementOrDefList:
	/* empty */
	| optFunctionStatementOrDefList FunctionStatementOrDef optSemiColon
;

FunctionStatementOrDef:
	FunctionLocalDef
	| FunctionLocalInst
	| FunctionStatement
;

FunctionLocalInst:
	VarInstance
	| TimerInstance
;

FunctionLocalDef:
	ConstDef
	| TemplateDef
;

FunctionStatement:
	ConfigurationStatements
	| TimerStatements
	| CommunicationStatements
	| BasicStatements
	| BehaviourStatements
	| VerdictStatements
	| SUTStatements
;

FunctionInstance:
	Identifier '(' optFunctionActualParList ')'
	| Identifier Dot Identifier '(' optFunctionActualParList ')'
	| Identifier Dot ObjectIdentifierValue Dot Identifier
	'(' optFunctionActualParList ')'
;

optFunctionActualParList:
	/* empty */
	| FunctionActualParList
;

FunctionActualParList:
	FunctionActualPar
	| FunctionActualParList ',' FunctionActualPar
;

FunctionActualPar:
/*	TimerRef */
	TemplateInstance
/*	| Port
	| ComponentRef -- TemplateInstance covers all the others */
;

/* A.1.6.1.5 Signature definitions */

SignatureDef:
	SignatureKeyword Identifier
	'(' optSignatureFormalParList ')' optReturnTypeOrNoBlockKeyword
	optExceptionSpec
;

optSignatureFormalParList:
	/* empty */
	| SignatureFormalParList
;

SignatureFormalParList:
	SignatureFormalPar
	| SignatureFormalParList ',' SignatureFormalPar
;

SignatureFormalPar:
	FormalValuePar
;

optReturnTypeOrNoBlockKeyword:
	/* empty */
	| ReturnType
	| NoBlockKeyword
;

optExceptionSpec:
	/* empty */
	| ExceptionKeyword '(' ExceptionTypeList ')'
;

ExceptionTypeList:
	Type
	| ExceptionTypeList ',' Type
;

Signature:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

/* A.1.6.1.6 Testcase definitions */

TestcaseDef:
	TestcaseKeyword Identifier
	'(' optTestcaseFormalParList ')' ConfigSpec
	StatementBlock
;

optTestcaseFormalParList:
	/* empty */
	| TestcaseFormalParList
;

TestcaseFormalParList:
	TestcaseFormalPar
	| TestcaseFormalParList ',' TestcaseFormalPar
;

TestcaseFormalPar:
	FormalValuePar
	| FormalTemplatePar
;

ConfigSpec:
	RunsOnSpec optSystemSpec
;

optSystemSpec:
	/* empty */
	| SystemKeyword ComponentType
;

TestcaseInstance:
	ExecuteKeyword '(' TestcaseRef '(' optTestcaseActualParList ')'
	optTestcaseTimerValue ')'
;

TestcaseRef:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

optTestcaseTimerValue:
	/* empty */
	| ',' TimerValue
;

optTestcaseActualParList:
	/* empty */
	| TestcaseActualParList
;

TestcaseActualParList:
	TestcaseActualPar
	| TestcaseActualParList ',' TestcaseActualPar
;

TestcaseActualPar:
	TemplateInstance
;

/* A.1.6.1.7 Altstep definitions */

AltstepDef:
	AltstepKeyword Identifier
	'(' optAltstepFormalParList ')' optRunsOnSpec
	'{' AltstepLocalDefList	AltGuardList '}'
;

optAltstepFormalParList:
	/* empty */
	| FunctionFormalParList
;

AltstepLocalDefList:
	/* empty */
	| AltstepLocalDefList AltstepLocalDef optSemiColon
;

AltstepLocalDef:
	VarInstance
	| TimerInstance
	| ConstDef
	| TemplateDef
;

AltstepInstance:
	Identifier '(' optFunctionActualParList ')'
	| Identifier Dot Identifier '(' optFunctionActualParList ')'
	| Identifier Dot ObjectIdentifierValue Dot Identifier
	'(' optFunctionActualParList ')'
;

AltstepRef:
	Identifier
	| Identifier Dot Identifier
	| Identifier Dot ObjectIdentifierValue Dot Identifier
;

/* A.1.6.1.8 Import definitions */

ImportDef:
	ImportKeyword ImportFromSpec AllWithExcepts
	| ImportKeyword ImportFromSpec '{' ImportSpec '}'
;

AllWithExcepts:
	AllKeyword
	| AllKeyword ExceptsDef
;

ExceptsDef:
	ExceptKeyword '{' ExceptSpec '}'
;

ExceptSpec:
	/* empty */
	| ExceptSpec ExceptElement optSemiColon
;

ExceptElement:
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

ExceptGroupSpec:
	GroupKeyword ExceptGroupRefList
	| GroupKeyword AllKeyword
;

ExceptTypeDefSpec:
	TypeDefKeyword TypeRefList
	| TypeDefKeyword AllKeyword
;

ExceptTemplateSpec:
	TemplateKeyword TemplateRefList
	| TemplateKeyword AllKeyword
;

ExceptConstSpec:
	ConstKeyword ConstRefList
	| ConstKeyword AllKeyword
;

ExceptTestcaseSpec:
	TestcaseKeyword TestcaseRefList
	| TestcaseKeyword AllKeyword
;

ExceptAltstepSpec:
	AltstepKeyword AltstepRefList
	| AltstepKeyword AllKeyword
;

ExceptFunctionSpec:
	FunctionKeyword FunctionRefList
	| FunctionKeyword AllKeyword
;

ExceptSignatureSpec:
	SignatureKeyword SignatureRefList
	| SignatureKeyword AllKeyword
;

ExceptModuleParSpec:
	ModuleParKeyword ModuleParRefList
	| ModuleParKeyword AllKeyword
;

ImportSpec:
	/* empty */
	| ImportSpec ImportElement optSemiColon
;

ImportElement:
	ImportGroupSpec
	| ImportTypeDefSpec
	| ImportTemplateSpec
	| ImportConstSpec
	| ImportTestcaseSpec
	| ImportAltstepSpec
	| ImportFunctionSpec
	| ImportSignatureSpec
	| ImportModuleParSpec
;

ImportFromSpec:
	FromKeyword ModuleId
	| FromKeyword ModuleId RecursiveKeyword
;

ImportGroupSpec:
	GroupKeyword GroupRefListWithExcept
	| GroupKeyword AllGroupsWithExcept
;

GroupRefList:
	FullGroupIdentifier
	| GroupRefList ',' FullGroupIdentifier
;

GroupRefListWithExcept:
	FullGroupIdentifierWithExcept
	| GroupRefListWithExcept ',' FullGroupIdentifierWithExcept
;

AllGroupsWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword GroupRefList
;

FullGroupIdentifier:
	Identifier
	| FullGroupIdentifier Dot Identifier
;

FullGroupIdentifierWithExcept:
	FullGroupIdentifier
	| FullGroupIdentifier ExceptsDef
;

ExceptGroupRefList:
	ExceptFullGroupIdentifier
	| ExceptGroupRefList ',' ExceptFullGroupIdentifier
;

ExceptFullGroupIdentifier:
	FullGroupIdentifier
;

ImportTypeDefSpec:
	TypeDefKeyword TypeRefList
	| TypeDefKeyword AllTypesWithExcept
;

TypeRefList:
	Identifier
	| TypeRefList ',' Identifier
;

AllTypesWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword TypeRefList
;

ImportTemplateSpec:
	TemplateKeyword TemplateRefList
	| TemplateKeyword AllTemplsWithExcept
;

TemplateRefList:
	Identifier
	| TemplateRefList ',' Identifier
;

AllTemplsWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword TemplateRefList
;

ImportConstSpec:
	ConstKeyword ConstRefList
	| ConstKeyword AllConstsWithExcept
;

ConstRefList:
	Identifier
	| ConstRefList ',' Identifier
;

AllConstsWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword ConstRefList
;

ImportAltstepSpec:
	AltstepKeyword AltstepRefList
	| AltstepKeyword AllAltstepsWithExcept
;

AltstepRefList:
	Identifier
	| AltstepRefList ',' Identifier
;

AllAltstepsWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword AltstepRefList
;

ImportTestcaseSpec:
	TestcaseKeyword TestcaseRefList
	| TestcaseKeyword AllTestcasesWithExcept
;

TestcaseRefList:
	Identifier
	| TestcaseRefList ',' Identifier
;

AllTestcasesWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword TestcaseRefList
;

ImportFunctionSpec:
	FunctionKeyword FunctionRefList
	| FunctionKeyword AllFunctionsWithExcept
;

FunctionRefList:
	Identifier
	| FunctionRefList ',' Identifier
;

AllFunctionsWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword FunctionRefList
;

ImportSignatureSpec:
	SignatureKeyword SignatureRefList
	| SignatureKeyword AllSignaturesWithExcept
;

SignatureRefList:
	Identifier
	| SignatureRefList ',' Identifier
;

AllSignaturesWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword SignatureRefList
;

ImportModuleParSpec:
	ModuleParKeyword ModuleParRefList
	| ModuleParKeyword AllModuleParWithExcept
;

ModuleParRefList:
	Identifier
	| ModuleParRefList ',' Identifier
;

AllModuleParWithExcept:
	AllKeyword
	| AllKeyword ExceptKeyword ModuleParRefList
;

/* A.1.6.1.9 Group definitions */

GroupDef:
	GroupKeyword Identifier
	'{' optModuleDefinitionsPart '}'
;

/* A.1.6.1.10 External function definitions */

ExtFunctionDef:
	ExtKeyword FunctionKeyword Identifier
	'(' optFunctionFormalParList ')' optReturnType
;

/* A.1.6.1.11 External constant definitions */

ExtConstDef:
	ExtKeyword ConstKeyword Type Identifier
;

/* A.1.6.1.12 Module parameter definitions */

ModuleParDef:
	ModuleParKeyword ModulePar
	| ModuleParKeyword '{' MultitypedModuleParList '}'
;

MultitypedModuleParList:
	ModulePar SemiColon
	| MultitypedModuleParList ModulePar SemiColon
;

ModulePar:
	Type ModuleParList
;

ModuleParList:
	SingleModulePar
	| ModuleParList ',' SingleModulePar
;

SingleModulePar:
	Identifier
	| Identifier AssignmentChar Expression
;

/* A.1.6.2 Control part */

/* A.1.6.2.0 General */

optModuleControlPart:
	/* empty */
	| ModuleControlPart
;

ModuleControlPart:
	ControlKeyword
	'{' ModuleControlBody '}'
	optWithStatementAndSemiColon
;

ModuleControlBody:
	/* empty */
	| ControlStatementOrDefList
;

ControlStatementOrDefList:
	ControlStatementOrDef optSemiColon
	| ControlStatementOrDefList ControlStatementOrDef optSemiColon
;

ControlStatementOrDef:
	FunctionLocalDef
	| FunctionLocalInst
	| ControlStatement
;

ControlStatement:
	TimerStatements
	| BasicStatements
	| BehaviourStatements
	| SUTStatements
	| StopKeyword
;

/* A.1.6.2.1 Variable instantiation */

VarInstance:
	VarKeyword Type VarList
	| VarKeyword TemplateKeyword Type TempVarList
;

VarList:
	SingleVarInstance
	| VarList ',' SingleVarInstance
;

SingleVarInstance:
	Identifier optArrayDef
	| Identifier optArrayDef AssignmentChar VarInitialValue
;

VarInitialValue:
	Expression
;

TempVarList:
	SingleTempVarInstance
	| TempVarList ',' SingleTempVarInstance
;

SingleTempVarInstance:
	Identifier optArrayDef
	| Identifier optArrayDef AssignmentChar TempVarInitialValue
;

TempVarInitialValue:
	TemplateBody
;

VariableRef:
	Reference
;

/* A.1.6.2.2 Timer instantiation */

TimerInstance:
	TimerKeyword TimerList
;

TimerList:
	SingleTimerInstance
	| TimerList ',' SingleTimerInstance
;

SingleTimerInstance:
	Identifier optArrayDef
	| Identifier optArrayDef AssignmentChar TimerValue
;

TimerValue:
	Expression
;

TimerRef:
	VariableRef
;

/* A.1.6.2.3 Component operations */

ConfigurationStatements:
	ConnectStatement
	| MapStatement
	| DisconnectStatement
	| UnmapStatement
	| DoneStatement
	| KilledStatement
	| StartTCStatement
	| StopTCStatement
	| KillTCStatement
;

ConfigurationOps:
	CreateOp
	| SelfOp
	| SystemOp
	| MTCOp
	| RunningOp
	| AliveOp
;

CreateOp:
	VariableRef DotCreateKeyword optAliveKeyword
	| VariableRef DotCreateKeyword '(' SingleExpression ')' optAliveKeyword
;

optAliveKeyword:
	/* empty */
	| AliveKeyword
;

SystemOp:
	SystemKeyword
;

MTCOp:
	MTCKeyword
;

DoneStatement:
	ComponentId DotDoneKeyword
;

KilledStatement:
	ComponentId DotKilledKeyword
;

ComponentId:
	ComponentOrDefaultReference
	| AnyKeyword ComponentKeyword
	| AllKeyword ComponentKeyword
;

RunningOp:
/*	VariableRef DotRunningKeyword -- covered by RunningTimerOp */
	FunctionInstance DotRunningKeyword
	| AnyKeyword ComponentKeyword DotRunningKeyword
	| AllKeyword ComponentKeyword DotRunningKeyword
;

AliveOp:
	ComponentId DotAliveKeyword
;

ConnectStatement:
	ConnectKeyword SingleConnectionSpec
;

SingleConnectionSpec:
	'(' PortRef ',' PortRef ')'
;

PortRef:
	ComponentRef Colon Port
;

ComponentRef:
	ComponentOrDefaultReference
	| SystemOp
	| SelfOp
	| MTCOp
;

DisconnectStatement:
	DisconnectKeyword optSingleOrMultiConnectionSpec
;

optSingleOrMultiConnectionSpec:
	/* empty */
	| SingleConnectionSpec
	| AllConnectionsSpec
	| AllPortsSpec
	| AllCompsAllPortsSpec
;

AllConnectionsSpec:
	'(' PortRef ')'
;

AllPortsSpec:
	'(' ComponentRef ':' AllKeyword PortKeyword ')'
;

AllCompsAllPortsSpec:
	'(' AllKeyword ComponentKeyword ':' AllKeyword PortKeyword ')'
;

MapStatement:
	MapKeyword SingleConnectionSpec
;

UnmapStatement:
	UnmapKeyword optSingleOrMultiConnectionSpec
;

StartTCStatement:
/*	VariableRef DotStartKeyword '(' FunctionInstance ')'
	-- covered by StartTimerStatement */
	FunctionInstance DotStartKeyword '(' FunctionInstance ')'
;

StopTCStatement:
	StopKeyword
/*	| VariableRef DotStopKeyword -- covered by StopTimerStatement */
	| FunctionInstance DotStopKeyword
	| MTCOp DotStopKeyword
	| SelfOp DotStopKeyword
	| AllKeyword ComponentKeyword DotStopKeyword
;

ComponentReferenceOrLiteral:
	ComponentOrDefaultReference
	| MTCOp
	| SelfOp
;

KillTCStatement:
	KillKeyword
	| ComponentReferenceOrLiteral DotKillKeyword
	| AllKeyword ComponentKeyword DotKillKeyword
;

ComponentOrDefaultReference:
	VariableRef
	| FunctionInstance
;

/* A.1.6.2.4 Port operations */

Port:
	VariableRef
;

CommunicationStatements:
	SendStatement
	| CallStatement
	| ReplyStatement
	| RaiseStatement
	| ReceiveStatement
	| TriggerStatement
	| GetCallStatement
	| GetReplyStatement
	| CatchStatement
	| CheckStatement
	| ClearStatement
	| StartStatement
	| StopStatement
;

SendStatement:
	Port DotSendOpKeyword PortSendOp
;

PortSendOp:
	'(' SendParameter ')' optToClause
;

SendParameter:
	TemplateInstance
;

optToClause:
	/* empty */
	| ToKeyword AddressRef
/*	| ToKeyword AddressRefList -- covered by the previous rule
	(as ValueOrAttribList) */
	| ToKeyword AllKeyword ComponentKeyword
;

AddressRefList:
	'(' seqAddressRef ')'
;

seqAddressRef:
	AddressRef
	| seqAddressRef ',' AddressRef
;

AddressRef:
	TemplateInstance
;

CallStatement:
	Port DotCallOpKeyword PortCallOp optPortCallBody
;

PortCallOp:
	'(' CallParameters ')' optToClause
;

CallParameters:
	TemplateInstance
	| TemplateInstance ',' CallTimerValue
;

CallTimerValue:
	TimerValue
	| NowaitKeyword
;

optPortCallBody:
	/* empty */
	| '{' CallBodyStatementList '}'
;

CallBodyStatementList:
	CallBodyStatement optSemiColon
	| CallBodyStatementList CallBodyStatement optSemiColon
;

CallBodyStatement:
	CallBodyGuard StatementBlock
;

CallBodyGuard:
	AltGuardChar CallBodyOps
;

CallBodyOps:
	GetReplyStatement
	| CatchStatement
;

ReplyStatement:
	Port DotReplyKeyword PortReplyOp
;

PortReplyOp:
	'(' TemplateInstance optReplyValue ')' optToClause
;

optReplyValue:
	/* empty */
	| ValueKeyword Expression
;

RaiseStatement:
	Port DotRaiseKeyword PortRaiseOp
;

PortRaiseOp:
	'(' Signature ',' TemplateInstance ')' optToClause
;

ReceiveStatement:
	PortOrAny DotReceiveOpKeyword PortReceiveOp
;

PortOrAny:
	Port
	| AnyKeyword PortKeyword
;

PortReceiveOp:
	optReceiveParameter optFromClause optPortRedirect
;

optReceiveParameter:
	/* empty */
	| '(' ReceiveParameter ')'
;

ReceiveParameter:
	TemplateInstance
;

optFromClause:
	/* empty */
	| FromClause
;

FromClause:
	FromKeyword AddressRef
;

optPortRedirect:
	/* empty */
	| PortRedirectSymbol ValueSpec
	| PortRedirectSymbol SenderSpec
	| PortRedirectSymbol ValueSpec SenderSpec
;

ValueSpec:
	ValueKeyword VariableRef
;

SenderSpec:
	SenderKeyword VariableRef
;

TriggerStatement:
	PortOrAny DotTriggerOpKeyword PortTriggerOp
;

PortTriggerOp:
	optReceiveParameter optFromClause optPortRedirect
;

GetCallStatement:
	PortOrAny DotGetCallOpKeyword PortGetCallOp
;

PortGetCallOp:
	optReceiveParameter optFromClause optPortRedirectWithParam
;

optPortRedirectWithParam:
	/* empty */
	| PortRedirectSymbol RedirectWithParamSpec
;

RedirectWithParamSpec:
	ParamSpec
	| ParamSpec SenderSpec
	| SenderSpec
;

ParamSpec:
	ParamKeyword ParamAssignmentList
;

ParamAssignmentList:
	'(' AssignmentList ')'
	| '(' VariableList ')'
;

AssignmentList:
	VariableAssignment
	| AssignmentList ',' VariableAssignment
;

VariableAssignment:
	VariableRef AssignmentChar Identifier
;

VariableList:
	VariableEntry
	| VariableList ',' VariableEntry
;

VariableEntry:
	VariableRef
	| NotUsedSymbol
;

GetReplyStatement:
	PortOrAny DotGetReplyOpKeyword PortGetReplyOp
;

PortGetReplyOp:
	optGetReplyParameter optFromClause
	optPortRedirectWithValueAndParam
;

optPortRedirectWithValueAndParam:
	/* empty */
	| PortRedirectSymbol RedirectWithValueAndParamSpec
;

RedirectWithValueAndParamSpec:
	ValueSpec
	| ValueSpec ParamSpec
	| ValueSpec SenderSpec
	| ValueSpec ParamSpec SenderSpec
	| RedirectWithParamSpec
;

optGetReplyParameter:
	/* empty */
	| '(' ReceiveParameter ')'
	| '(' ReceiveParameter ValueMatchSpec ')'
;

ValueMatchSpec:
	ValueKeyword TemplateInstance
;

CheckStatement:
	PortOrAny DotCheckOpKeyword optCheckParameter
;

optCheckParameter:
	/* empty */
	| '(' CheckParameter ')'
;

CheckParameter:
	CheckPortOpsPresent
	| FromClausePresent
	| RedirectPresent
;

FromClausePresent:
	FromClause
	| FromClause PortRedirectSymbol SenderSpec
;

RedirectPresent:
	PortRedirectSymbol SenderSpec
;

CheckPortOpsPresent:
	ReceiveOpKeyword PortReceiveOp
	| GetCallOpKeyword PortGetCallOp
	| GetReplyOpKeyword PortGetReplyOp
	| CatchOpKeyword PortCatchOp
;

CatchStatement:
	PortOrAny DotCatchOpKeyword PortCatchOp
;

PortCatchOp:
	optCatchOpParameter optFromClause optPortRedirect
;

optCatchOpParameter:
	/* empty */
	| '(' CatchOpParameter ')'
;

CatchOpParameter:
	Signature ',' TemplateInstance
	| TimeoutKeyword
;

ClearStatement:
	PortOrAll DotClearOpKeyword
;

PortOrAll:
	Port
	| AllKeyword PortKeyword
;

StartStatement:
/*	Port DotPortStartKeyword -- covered by StartTimerStatement */
	AllKeyword PortKeyword DotStartKeyword
;

StopStatement:
/*	Port DotPortStopKeyword -- covered by StopTimerStatement */
	AllKeyword PortKeyword DotStopKeyword
;

/* A.1.6.2.5 Timer operations */

TimerStatements:
	StartTimerStatement
	| StopTimerStatement
	| TimeoutStatement
;

TimerOps:
	ReadTimerOp
	| RunningTimerOp
;

StartTimerStatement:
	TimerRef DotStartKeyword optTimerValue
;

optTimerValue:
	/* empty */
	| '(' TimerValue ')'
;

StopTimerStatement:
	TimerRefOrAll DotStopKeyword
;

TimerRefOrAll:
	TimerRef
	| AllKeyword TimerKeyword
;

ReadTimerOp:
	TimerRef DotReadKeyword
;

RunningTimerOp:
	TimerRefOrAny DotRunningKeyword
;

TimeoutStatement:
	TimerRefOrAny DotTimeoutKeyword
;

TimerRefOrAny:
	TimerRef
	| AnyKeyword TimerKeyword
;

/* A.1.6.3 Type */

Type:
	PredefinedType
	| ReferencedType
;

PredefinedType:
	BitStringKeyword
	| BooleanKeyword
	| CharStringKeyword
	| UniversalCharString
	| IntegerKeyword
	| OctetStringKeyword
	| HexStringKeyword
	| VerdictTypeKeyword
	| FloatKeyword
	| AddressKeyword
	| DefaultKeyword
	| AnyTypeKeyword
	| ObjectIdentifierKeyword
;

UniversalCharString:
	UniversalKeyword CharStringKeyword
;

ReferencedType:
	Reference
	| FunctionInstance optExtendedFieldReference
	/* covers all parameterized type references */
;

TypeReference:
	Identifier
	| Identifier TypeActualParList
;

TypeActualParList:
	'(' seqTypeActualPar ')'
;

seqTypeActualPar:
	TypeActualPar
	| seqTypeActualPar ',' TypeActualPar
;

TypeActualPar:
	Expression
;

optArrayDef:
	/* empty */
	| optArrayDef ArrayIndex
;

ArrayIndex:
	'[' ArrayBounds ']'
	| '[' ArrayBounds DotDot ArrayBounds ']'
;

ArrayBounds:
	SingleExpression
;

/* A.1.6.4 Value */

Value:
	PredefinedValue
	| ReferencedValue
;

PredefinedValue:
	BitStringValue
	| BooleanValue
	| CharStringValue
	| IntegerValue
	| OctetStringValue
	| HexStringValue
	| VerdictTypeValue
/*	| EnumeratedValue -- covered by ReferencedValue */
	| FloatValue
	| AddressValue
	| OmitValue
	| ObjectIdentifierValue
;

BitStringValue:
	Bstring
;

BooleanValue:
	BooleanConst
;

IntegerValue:
	Number
;

OctetStringValue:
	Ostring
;

HexStringValue:
	Hstring
;

VerdictTypeValue:
	VerdictConst
;

CharStringValue:
	Cstring
	| Quadruple
;

Quadruple:
	CharKeyword '(' Group ',' Plane ',' Row ',' Cell ')'
;

Group:
	Number
;

Plane:
	Number
;

Row:
	Number
;

Cell:
	Number
;

ReferencedValue:
	Reference
;

Reference:
	Identifier
	| Identifier Dot Identifier optExtendedFieldReference
	| Identifier Dot PredefinedType optExtendedFieldReference
	| Identifier ArrayOrBitRef optExtendedFieldReference
	| Identifier Dot ObjectIdentifierValue Dot Identifier
	optExtendedFieldReference
;

FreeText:
	Cstring
;

AddressValue:
	NullKeyword
;

OmitValue:
	OmitKeyword
;

/* A.1.6.5 Parameterization */

FormalValuePar:
	Type Identifier
	| InParKeyword Type Identifier
	| InOutParKeyword Type Identifier
	| OutParKeyword Type Identifier
;

FormalPortPar:
	Identifier Identifier
	| InOutParKeyword Identifier Identifier
;

FormalTimerPar:
	TimerKeyword Identifier
	| InOutParKeyword TimerKeyword Identifier
;

FormalTemplatePar:
	TemplateKeyword Type Identifier
	| InParKeyword TemplateKeyword Type Identifier
	| InOutParKeyword TemplateKeyword Type Identifier
	| OutParKeyword TemplateKeyword Type Identifier
;

/* A.1.6.6 With statement */

optWithStatement:
	/* empty */
	| WithStatement
;

optWithStatementAndSemiColon:
	/* empty */
	| WithStatement
	| SemiColon
	| WithStatement SemiColon
;

WithStatement:
	WithKeyword WithAttribList
;

WithAttribList:
	'{' MultiWithAttrib '}'
;

MultiWithAttrib:
	SingleWithAttrib optSemiColon
	| MultiWithAttrib SingleWithAttrib optSemiColon
;

SingleWithAttrib:
	AttribKeyword optOverrideKeyword optAttribQualifier AttribSpec
;

AttribKeyword:
	EncodeKeyword
	| VariantKeyword
	| DisplayKeyword
	| ExtensionKeyword
;

optOverrideKeyword:
	/* empty */
	| OverrideKeyword
;

optAttribQualifier:
	/* empty */
	| '(' DefOrFieldRefList ')'
;

DefOrFieldRefList:
	DefOrFieldRef
	| DefOrFieldRefList ',' DefOrFieldRef
;

DefOrFieldRef:
	DefinitionRef
	| FieldReference
	| AllRef
;

DefinitionRef:
	/* other alternatives are covered by FieldReference */
	Identifier Dot FullGroupIdentifier
;

AllRef:
	GroupKeyword AllKeyword
	| GroupKeyword AllKeyword ExceptKeyword '{' GroupRefList '}'
	| TypeDefKeyword AllKeyword
	| TypeDefKeyword AllKeyword ExceptKeyword '{' TypeRefList '}'
	| TemplateKeyword AllKeyword
	| TemplateKeyword AllKeyword ExceptKeyword '{' TemplateRefList '}'
	| ConstKeyword AllKeyword
	| ConstKeyword AllKeyword ExceptKeyword '{' ConstRefList '}'
	| AltstepKeyword AllKeyword
	| AltstepKeyword AllKeyword ExceptKeyword '{' AltstepRefList '}'
	| TestcaseKeyword AllKeyword
	| TestcaseKeyword AllKeyword ExceptKeyword '{' TestcaseRefList '}'
	| FunctionKeyword AllKeyword
	| FunctionKeyword AllKeyword ExceptKeyword '{' FunctionRefList '}'
	| SignatureKeyword AllKeyword
	| SignatureKeyword AllKeyword ExceptKeyword '{' SignatureRefList '}'
	| ModuleParKeyword AllKeyword
	| ModuleParKeyword AllKeyword ExceptKeyword '{' ModuleParRefList '}'
;

AttribSpec:
	FreeText
;

/* A.1.6.7 Behaviour statements */

BehaviourStatements:
	TestcaseInstance
	| FunctionInstance
	| ReturnStatement
	| AltConstruct
	| InterleavedConstruct
	| LabelStatement
	| GotoStatement
	| RepeatStatement
	| DeactivateStatement
/*	| AltstepInstance -- covered by FunctionInstance */
	| ActivateOp
;

VerdictStatements:
	SetLocalVerdict
;

VerdictOps:
	GetLocalVerdict
;

SetLocalVerdict:
	 SetVerdictKeyword '(' SingleExpression ')'
;

GetLocalVerdict:
	GetVerdictKeyword
;

SUTStatements:
	ActionKeyword '(' ')'
	| ActionKeyword '(' seqActionText ')'
;

seqActionText:
	ActionText
/*	| seqActionText StringOp ActionText -- covered by the previous rule */
;

ActionText:
/*	FreeText -- covered by the next rule */
	Expression
;

ReturnStatement:
	ReturnKeyword
	| ReturnKeyword Expression
;

AltConstruct:
	AltKeyword '{' AltGuardList '}'
;

AltGuardList:
	AltGuard
	| AltGuardList AltGuard
;

AltGuard:
	GuardStatement
	| ElseStatement optSemiColon
;

GuardStatement:
	AltGuardChar AltstepInstance
	| AltGuardChar AltstepInstance StatementBlock
	| AltGuardChar GuardOp StatementBlock
;

ElseStatement:
	'[' ElseKeyword ']' StatementBlock
;

AltGuardChar:
	'[' ']'
	| '[' BooleanExpression ']'
;

GuardOp:
	TimeoutStatement
	| ReceiveStatement
	| TriggerStatement
	| GetCallStatement
	| CatchStatement
	| CheckStatement
	| GetReplyStatement
	| DoneStatement
	| KilledStatement
;

InterleavedConstruct:
	InterleavedKeyword '{' InterleavedGuardList '}'
;

InterleavedGuardList:
	InterleavedGuardElement optSemiColon
	| InterleavedGuardList InterleavedGuardElement optSemiColon
;

InterleavedGuardElement:
	InterleavedGuard InterleavedAction
;

InterleavedGuard:
	'[' ']' GuardOp
;

InterleavedAction:
	StatementBlock
;

LabelStatement:
	LabelKeyword Identifier
;

GotoStatement:
	GotoKeyword Identifier
;

RepeatStatement:
	RepeatKeyword
;

ActivateOp:
	ActivateKeyword '(' AltstepInstance ')'
;

DeactivateStatement:
	DeactivateKeyword
	| DeactivateKeyword '(' ComponentOrDefaultReference ')'
;

/* A.1.6.8 Basic statements */

BasicStatements:
	Assignment
	| LogStatement
	| LoopConstruct
	| ConditionalConstruct
	| SelectCaseConstruct
;

Expression:
	SingleExpression
	| CompoundExpression
;

CompoundExpression:
	FieldExpressionList
	| ArrayExpression
;

FieldExpressionList:
	'{' seqFieldExpressionSpec '}'
;

seqFieldExpressionSpec:
	FieldExpressionSpec
	| seqFieldExpressionSpec ',' FieldExpressionSpec
;

FieldExpressionSpec:
	FieldReference AssignmentChar NotUsedOrExpression
;

ArrayExpression:
	'{' '}'
	| '{' ArrayElementExpressionList '}'
;

ArrayElementExpressionList:
	NotUsedOrExpression
	| ArrayElementExpressionList ',' NotUsedOrExpression
;

NotUsedOrExpression:
	Expression
	| NotUsedSymbol
;

BooleanExpression:
	SingleExpression
;

Assignment:
/*	VariableRef AssignmentChar Expression -- covered by the next rule */
	VariableRef AssignmentChar TemplateBody
;

SingleExpression:
	XorExpression
	| SingleExpression Or XorExpression
;

XorExpression:
	AndExpression
	| XorExpression Xor AndExpression
;

AndExpression:
	NotExpression
	| AndExpression And NotExpression
;

NotExpression:
	EqualExpression
	| Not EqualExpression
;

EqualExpression:
	RelExpression
	| EqualExpression EqualOp RelExpression
;

RelExpression:
	ShiftExpression
	| ShiftExpression RelOp ShiftExpression
;

ShiftExpression:
	BitOrExpression
	| ShiftExpression ShiftOp BitOrExpression
;

BitOrExpression:
	BitXorExpression
	| BitOrExpression Or4b BitXorExpression
;

BitXorExpression:
	BitAndExpression
	| BitXorExpression Xor4b BitAndExpression
;

BitAndExpression:
	BitNotExpression
	| BitAndExpression And4b BitNotExpression
;

BitNotExpression:
	AddExpression
	| Not4b AddExpression
;

AddExpression:
	MulExpression
	| AddExpression AddOp MulExpression
;

MulExpression:
	UnaryExpression
	| MulExpression MultiplyOp UnaryExpression
;

UnaryExpression:
	Primary
	| UnaryOp Primary
;

Primary:
	OpCall
	| Value
	| '(' SingleExpression ')'
;

optExtendedFieldReference:
	/* empty */
	| optExtendedFieldReference Dot FieldIdentifier
	| optExtendedFieldReference ArrayOrBitRef
;

FieldIdentifier:
	Identifier
	| PredefinedType
	| Identifier TypeActualParList
;

OpCall:
	ConfigurationOps
	| VerdictOps
	| TimerOps
	| TestcaseInstance
	| FunctionInstance
	| TemplateOps
	| ActivateOp
;

AddOp:
	'+'
	| '-'
	| StringOp
;

MultiplyOp:
	'*'
	| '/'
	| Mod
	| Rem
;

UnaryOp:
	'+'
	| '-'
;

RelOp:
	'<'
	| '>'
	| GE
	| LE
;

EqualOp:
	EQ
	| NE
;

StringOp:
	'&'
;

ShiftOp:
	SL
	| SR
	| RL
	| RR
;

LogStatement:
	LogKeyword '(' seqLogItem ')'
;

seqLogItem:
	LogItem
	| seqLogItem ',' LogItem
;

LogItem:
/*	FreeText -- covered by the next rule */
	TemplateInstance
;

LoopConstruct:
	ForStatement
	| WhileStatement
	| DoWhileStatement
;

ForStatement:
	ForKeyword '(' Initial SemiColon Final SemiColon Step ')'
	StatementBlock
;

Initial:
	VarInstance
	| Assignment
;

Final:
	BooleanExpression
;

Step:
	Assignment
;

WhileStatement:
	WhileKeyword '(' BooleanExpression ')'
	StatementBlock
;

DoWhileStatement:
	DoKeyword StatementBlock
	WhileKeyword '(' BooleanExpression ')'
;

ConditionalConstruct:
	IfKeyword '(' BooleanExpression ')'
	StatementBlock
	seqElseIfClause optElseClause
;

seqElseIfClause:
	/* empty */
	| seqElseIfClause ElseIfClause
;

ElseIfClause:
	ElseKeyword IfKeyword '(' BooleanExpression ')' StatementBlock
;

optElseClause:
	/* empty */
	| ElseKeyword StatementBlock
;

SelectCaseConstruct:
	SelectKeyword '(' SingleExpression ')' SelectCaseBody
;

SelectCaseBody:
	'{' seqSelectCase '}'
;

seqSelectCase:
	SelectCase
	| seqSelectCase SelectCase
;

SelectCase:
	CaseKeyword '(' seqTemplateInstance ')' StatementBlock
	| CaseKeyword ElseKeyword StatementBlock
;

seqTemplateInstance:
	TemplateInstance
	| seqTemplateInstance ',' TemplateInstance
;

/* A.1.6.9 Miscellanous productions */

Dot:
	'.'
;

Dash:
	'-'
;

Minus:
	'-'
;

SemiColon:
	';'
;

optSemiColon:
	/* empty */
	| ';'
;

Colon:
	':'
;

/* A.1 ASN.1 support */

optDefinitiveIdentifier:
	/* empty */
	| DefinitiveIdentifier
;

DefinitiveIdentifier:
	Dot ObjectIdentifierKeyword '{' DefinitiveObjIdComponentList '}'
;

DefinitiveObjIdComponentList:
	DefinitiveObjIdComponent
	| DefinitiveObjIdComponentList DefinitiveObjIdComponent
;

DefinitiveObjIdComponent:
	NameForm
	| DefinitiveNumberForm
	| DefinitiveNameAndNumberForm
;

DefinitiveNumberForm:
	Number
;

DefinitiveNameAndNumberForm:
	Identifier '(' DefinitiveNumberForm ')'
;

ObjectIdentifierValue:
	ObjectIdentifierKeyword '{' ObjIdComponentList '}'
;

ObjIdComponentList:
	ObjIdComponent
	| ObjIdComponentList ObjIdComponent
;

ObjIdComponent:
/*	NameForm -- covered by NumberForm (as ReferencedValue) */
	NumberForm
	| NameAndNumberForm
;

NumberForm:
	Number
	| ReferencedValue
;

NameAndNumberForm:
	Identifier '(' NumberForm ')'
;

NameForm:
	Identifier
;

%%

/* Additional C code */

extern int yylineno;
extern char *yytext;
extern FILE *yyin;

extern int dot_flag;

void yyerror(char *s)
{
	fprintf(stderr, "line %d: syntax error at or near token '%s': "
		"%s\n", yylineno, yytext, s);
}

void parse_input()
{
	dot_flag = 0;
	if (!yyparse()) fprintf(stderr, "The input was parsed successfully!\n");
	else fprintf(stderr, "Parse error!\n");
}

int main(int argc, char *argv[])
{
	puts("TTCN-3 parser. Complies with BNF v3.0.0\n"
	"Copyright (c) 2000-2017 Ericsson Telecom AB");
	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++) {
			yyin = fopen(argv[i], "r");
			if (yyin == NULL) {
				fprintf(stderr, "%s: Cannot open file"
				" %s for reading: %s\n", argv[0], argv[i],
				strerror(errno));
				return 1;
			}
			printf("Parsing file %s ...\n", argv[i]);
			parse_input();
			fclose(yyin);
		}
	} else {
		puts("Parsing standard input ...");
		parse_input();
	}
	return 0;
}

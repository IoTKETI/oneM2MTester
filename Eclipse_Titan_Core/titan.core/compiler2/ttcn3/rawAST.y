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
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
/* RAW encoding/decoding specification parser & compiler */

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../common/memory.h"
#include "../Int.hh"
#include "../Real.hh"
#include "../Value.hh"
#include "AST_ttcn3.hh"
#include "rawASTspec.h"
#include "RawAST.hh"
#include "../XerAttributes.hh"
#include "BerAST.hh"
#include "JsonAST.hh"

extern void rawAST_error(const char *str);
extern int rawAST_lex();

extern RawAST* rawstruct;
extern Common::Module *mymod;
extern int length_multiplier;
extern TextAST *textstruct;
extern bool raw_f;
extern bool text_f;
extern bool xer_f;
extern bool ber_f;
extern bool json_f;
extern XerAttributes* xerstruct;
extern BerAST* berstruct;
extern JsonAST* jsonstruct;

extern string *parse_charstring_value(const char *str,
  const Common::Location& loc);

#define YYERROR_VERBOSE

#ifndef NDEBUG
union YYSTYPE;
static void yyprint(FILE *file, int type, const YYSTYPE& value);
#define YYPRINT(f,t,v) yyprint(f,t,v)
#endif

%}

%union {
    bool boolval;
    int enumval;
    char* str;
    const char *cstr;
    Common::Int intval;
    Common::Real floatval;
    Common::Value::verdict_t verdictval;
    Common::Identifier *identifier;
    rawAST_field_list *fieldlist;
    rawAST_tag_list taglist;
    rawAST_tag_field_value keyid;
    rawAST_single_tag singletag;
    rawAST_toplevel toplevel;
    rawAST_values values;
    struct {
      char *token;
      bool case_sensitive;
    } decodetoken;
    NamespaceSpecification nsspec;
    NamespaceRestriction   nsrestr;
    struct {
      char* str;
      Ttcn::Reference* reference;
      bool ref;
    } dfestruct;
}

%token <intval> XNumber
%token <floatval> XFloatValue
%token <identifier> XIdentifier
%token <str> XBstring
%token <str> XHstring
%token <str> XOstring
%token <str> XCstring
%token <str> Xstring
%token <boolval> XBooleanConst
%token <verdictval> XVerdictConst
%token XNullKeyword
%token XNULLKeyword
%token XOmitKeyword

%token <str> Xtoken
%token XBeginKeyword
%token XEndKeyword
%token XSeparatorKeyword
%token XCodingKeyword
%token XLengthToken
%token XLowerToken
%token XUpperToken
%token XJustToken
%token XLeftToken
%token XRightToken
%token XCenterToken
%token XLeadingToken
%token XTrueToken
%token XSensitivToken
%token XInSensitivToken
%token XConvertToken
%token XFalseToken
%token XRepeatToken

%token XPaddingKeyword
%token XPaddAllKeyword
%token XPrePaddingKeyword
%token XPaddingPatternKeyword
%token XOtherwise
%token <enumval> XYes
%token <enumval> XNo
%token <enumval> XReverse
%token XFieldOrderKeyword
%token <enumval> XMsb
%token <enumval> XLsb
%token XExtensionBitKeyword
%token XExtensionBitGroupKeyword
%token XLengthToKeyword
%token XPointerToKeyword
%token XUnitKeyword
%token XPtrUnitKeyword
%token XPtrOffsetKeyword
%token <enumval> XBits
%token <enumval> XOctets
%token XLengthIndexKeyword
%token XTagKeyword
%token XCrossTagKeyword
%token XPresenceKeyword
%token XFieldLengthKeyword
%token XAlignKeyword
%token <enumval> XLeft
%token <enumval> XRight
%token XCompKeyword
%token <enumval> XUnsigned
%token <enumval> XCompl
%token <enumval> XSignbit
%token XByteOrderKeyword
%token <enumval> XFirst
%token <enumval> XLast
%token XBitOrderKeyword
%token XBitOrderInFieldKeyword
%token XBitOrderInOctetKeyword
%token XHexOrderKeyword
%token <enumval> XLow
%token <enumval> XHigh
%token XToplevelKeyword
%token XRepeatableKeyword
%token XIntXKeyword
%token XBitKeyword
%token XUnsignedKeyword
%token XUTF8Keyword
%token XUTF16Keyword
%token XIEEE754FloatKeyword
%token XIEEE754DoubleKeyword

       /* XER attributes */
%token XKWall           "all"
%token XKWas            "as"
%token XKWin            "in"
%token XKWcapitalized   "capitalized"
%token XKWuncapitalized "uncapitalized"
%token XKWlowercased    "lowercased"
%token XKWuppercased    "uppercased"
%token XKWpreserve      "preserve"
%token XKWreplace       "replace"
%token XKWcollapse      "collapse"
%token XKWfrom          "from"
%token XKWexcept        "except"
%token XKWunqualified   "unqualified"
%token XKWqualified     "qualified"
%token XKWprefix        "prefix"


%token XKWabstract         "abstact"
%token XKWanyAttributes    "anyAttributes"
%token XKWanyElement       "anyElement"
%token XKWattribute        "attribute"
%token XKWattributeFormQualified   "attributeFormQualified"
%token XKWblock            "block"
%token XKWcontrolNamespace "controlNamespace"
%token XKWdefaultForEmpty  "defaultForEmpty"
%token XKWelement          "element"
%token XKWelementFormQualified          "elementFormQualified"
%token XKWembedValues      "embedValues"
%token XKWform             "form"
%token XKWfractionDigits   "fractionDigits"
%token XKWlist             "list"
%token XKWname             "name"
%token XKWnamespace        "namespace"
%token XKWtext             "text"
%token XKWuntagged         "untagged"
%token XKWuseNil           "useNil"
%token XKWuseNumber        "useNumber"
%token XKWuseOrder         "useOrder"
%token XKWuseUnion         "useUnion"
%token XKWuseType          "useType"
%token XKWwhiteSpace       "whiteSpace"
%token XSD                 "XSD"

       /* XSD:something */
%token XSDstring "string"
%token XSDnormalizedString "normalizedString"
%token XSDtoken "token"
%token XSDName "Name"
%token XSDNMTOKEN "NMTOKEN"
%token XSDNCName "NCName"
%token XSDID "ID"
%token XSDIDREF "IDREF"
%token XSDENTITY "ENTITY"
%token XSDhexBinary "hexBinary"
%token XSDbase64Binary "base64Binary"
%token XSDanyURI "anyURI"
%token XSDlanguage "language"
%token XSDinteger "integer"
%token XSDpositiveInteger "positiveInteger"
%token XSDnonPositiveInteger "nonPositiveInteger"
%token XSDnegativeInteger "negativeInteger"
%token XSDnonNegativeInteger "nonNegativeInteger"
  /* XKWlong instead of %token XSDlong */
%token XSDunsignedLong "unsignedLong"
%token XSDint "int"
%token XSDunsignedInt "unsignedInt"
  /* XKWshort instead of %token XSDshort */
%token XSDunsignedShort "unsignedShort"
%token XSDbyte "byte"
%token XSDunsignedByte "unsignedByte"
%token XSDdecimal "decimal"
%token XSDfloat "float"
%token XSDdouble "double"
%token XSDduration "duration"
%token XSDdateTime "dateTime"
%token XSDtime "time"
%token XSDdate "date"
%token XSDgYearMonth "gYearMonth"
%token XSDgYear "gYear"
%token XSDgMonthDay "gMonthDay"
%token XSDgDay "gDay"
%token XSDgMonth "gMonth"
%token XSDNMTOKENS "NMTOKENS"
%token XSDIDREFS "IDREFS"
%token XSDENTITIES "ENTITIES"
%token XSDQName "QName"
%token XSDboolean "boolean"
%token XSDanySimpleType "anySimpleType"
%token XSDanyType "anyType"



/* BER attributes */
%token XKWlength     "length"
%token XKWaccept     "accept"
%token XKWlong       "long"
%token XKWshort      "short"
%token XKWindefinite "indefinite"
%token XKWdefinite   "definite"


// JSON attributes
%token XKWjson            "JSON"
%token XKWomit            "omit"
%token XKWnull            "null"
%token XAliasToken        "JSON alias"
%token XKWvalue           "value"
%token XKWdefault         "default"
%token XKWextend          "extend"
%token XKWmetainfo        "metainfo"
%token XKWfor             "for"
%token XKWunbound         "unbound"
%token XJsonValueStart    "("
%token XJsonValueEnd      ")"
%token XJsonValueSegment  "JSON value"


%type <enumval>
    XYesOrNo XMsbOrLsb XFieldOrderDef XPaddingDef XExtensionBitDef XUnitDef
    XBitsOctets XAlignDef XLeftOrRight XCompDef XCompValues XByteOrderDef
    XFirstOrLast XBitOrderDef XBitOrderInFieldDef XBitOrderInOctetDef
    XHexOrderDef XLowOrHigh XYesOrNoOrReverse XFieldLengthDef
    XPtrOffsetDef XPrePaddingDef XRepeatableDef form

%type <fieldlist>
    XLengthIndexDef XStructFieldRefOrEmpty XStructFieldRef

%type <str>
    XEncodeToken XmatchDef XAliasToken XJsonValueSegment XJsonValueCore XJsonValue
    XJsonAlias

%type <decodetoken>
    XDecodeToken

%type <cstr>
    XTextReservedWord

%type <identifier>
    XPointerToDef XRecordFieldRef XIdentifierOrReserved

%type <values>
    XRvalue

%type <taglist>
    XAssocList

/* note: multiple "XSingleEncodingDefs have to be merged into one 'fullspec' */
%type <fullspec>
    XEncodingDefList XSingleEncodingDef

%type <keyid>
    XKeyId
%type <boolval>
    XmodifierDef

%type <singletag>
    XAssocElement XKeyIdOrIdList XKeyIdList XMultiKeyId

%type <toplevel>
    XToplevelDef XTopDefList XTopDef

    /* XER */
%type <nsrestr>
    anyAttributes anyElement optNamespaceRestriction urilist


%type <str>  name newnameOrKeyword  keyword optPrefix quotedURIorAbsent

%type <dfestruct> defaultForEmpty

%type <nsspec> namespace namespacespecification controlNamespace text

%type <intval> whiteSpace fractionDigits

    /* destructors */
%destructor { Free($$); }
XBstring
XCstring
XEncodeToken
XHstring
XmatchDef
XOstring
Xtoken
Xstring
XAliasToken
XJsonAlias
XJsonValueSegment
XJsonValueCore
XJsonValue

%destructor { delete $$; }
XIdentifier
XIdentifierOrReserved
XPointerToDef
XRecordFieldRef

%destructor { free_rawAST_field_list($$); }
XLengthIndexDef
XStructFieldRef
XStructFieldRefOrEmpty

%destructor { free_rawAST_tag_list(&$$); }
XAssocList

%destructor { free_rawAST_tag_field_value(&$$); }
XKeyId

%destructor { free_rawAST_single_tag(&$$); }
XAssocElement
XKeyIdList
XKeyIdOrIdList
XMultiKeyId

%destructor {
  Free($$.value);
  delete $$.v_value;
}
XRvalue

%destructor { Free($$.token); }
XDecodeToken

%destructor {
  FreeNamespaceRestriction($$);
}
anyAttributes anyElement optNamespaceRestriction urilist

%destructor {
  Free($$.uri);
  Free($$.prefix);
}
namespace
namespacespecification
text
controlNamespace



%start XAttribSpec

%%

/* BNF, actions */

XAttribSpec : /* Empty */
    | XEncodingDefList
        { }
    | XERattributes
    	{ }
    | XBERattributes
        { }
    ;

XBERattributes :
      XKWlength XKWaccept XKWshort { berstruct->decode_param = BerAST::LENGTH_ACCEPT_SHORT; ber_f=true; }
    | XKWlength XKWaccept XKWlong { berstruct->decode_param = BerAST::LENGTH_ACCEPT_LONG; ber_f=true; }
    | XKWlength XKWaccept XKWindefinite { berstruct->decode_param = BerAST::LENGTH_ACCEPT_INDEFINITE; ber_f=true; }
    | XKWlength XKWaccept XKWdefinite { berstruct->decode_param = BerAST::LENGTH_ACCEPT_DEFINITE; ber_f=true; }
;

XEncodingDefList : XSingleEncodingDef
        { }
    | XEncodingDefList ','  XSingleEncodingDef
        { };

XSingleEncodingDef : XPaddingDef
        { rawstruct->padding=$1;raw_f=true;}
    | XPrePaddingDef
        { rawstruct->prepadding=$1;raw_f=true;}
    | XPaddingPattern { raw_f=true;}
    | XPaddAll { rawstruct->paddall=XDEFYES;raw_f=true;}
    | XFieldOrderDef
        { rawstruct->fieldorder=$1;raw_f=true;}
    | XExtensionBitDef
        { rawstruct->extension_bit=$1;raw_f=true;}
    | XExtensionBitGroupDef
        { raw_f=true;}
    | XLengthToDef
        { raw_f=true;}
    | XPointerToDef
        { delete rawstruct->pointerto;
          rawstruct->pointerto = $1;
          raw_f=true;
        }
    | XUnitDef
        { rawstruct->unit=$1; raw_f=true;}
    | XLengthIndexDef
        { free_rawAST_field_list(rawstruct->lengthindex);
          rawstruct->lengthindex = $1;raw_f=true; }
    | XTagDef
        {raw_f=true; }
    | XCrossTagDef
        {raw_f=true; }
    | XPresenceDef
        { raw_f=true;}
    | XFieldLengthDef
        { rawstruct->fieldlength = $1*length_multiplier; raw_f=true;}
    | XPtrOffsetDef
        { raw_f=true; }
    | XAlignDef
        { rawstruct->align = $1; raw_f=true;}
    | XCompDef
        { rawstruct->comp = $1;raw_f=true; }
    | XByteOrderDef
        { rawstruct->byteorder = $1; raw_f=true;}
    | XBitOrderInFieldDef
        { rawstruct->bitorderinfield = $1; raw_f=true;}
    | XBitOrderInOctetDef
        { rawstruct->bitorderinoctet = $1; raw_f=true;}
    | XHexOrderDef
        { rawstruct->hexorder = $1;raw_f=true; }
    | XRepeatableDef
        { rawstruct->repeatable = $1;raw_f=true; }
    | XToplevelDef
        { rawstruct->topleveleind=1; raw_f=true;}
    | XIntXKeyword
        { rawstruct->intx = true; raw_f = true; }
    | XBitDef
        { raw_f = true; }
    | XUTFDef
        { raw_f = true; }
    | XIEEE754Def
        { raw_f = true; }
    /* TEXT encoder keywords */
    | XBeginDef
        { text_f=true; }
    | XEndDef
        { text_f=true; }
    | XSeparatorDef
        { text_f=true; }
    | XCodingDef
        { text_f=true; }
    | XJsonDef
        { json_f = true; }
    ;

XRepeatableDef : XRepeatableKeyword '(' XYesOrNo ')'  { $$ = $3; }

/* Padding*/
XPaddingDef : XPaddingKeyword '(' XYesOrNo ')'  { $$ = $3==XDEFYES?8:0; }
           | XPaddingKeyword '(' XBitsOctets ')'  { $$ = $3;};
XPrePaddingDef : XPrePaddingKeyword '(' XYesOrNo ')'  { $$ = $3==XDEFYES?8:0; }
           | XPrePaddingKeyword '(' XBitsOctets ')'  { $$ = $3;};
XYesOrNo : XYes | XNo;

XPaddingPattern:
  XPaddingPatternKeyword '(' XBstring ')'
  {
    Free(rawstruct->padding_pattern);
    size_t len = strlen($3);
    if (len > 0) {
      rawstruct->padding_pattern = mcopystr($3);
      rawstruct->padding_pattern_length = len;
      while (rawstruct->padding_pattern_length % 8 != 0) {
	rawstruct->padding_pattern = mputstr(rawstruct->padding_pattern, $3);
	rawstruct->padding_pattern_length += len;
      }
    } else rawstruct->padding_pattern = NULL;
    Free($3);
  }
;

XPaddAll:  XPaddAllKeyword
/* FieldOrder */
XFieldOrderDef : XFieldOrderKeyword '(' XMsbOrLsb ')'
        { $$ = $3; };
XMsbOrLsb : XMsb | XLsb;

/* Extension bit */
XExtensionBitDef : XExtensionBitKeyword '(' XYesOrNoOrReverse ')'
        { $$ = $3; };
XYesOrNoOrReverse : XYes | XNo | XReverse;

XExtensionBitGroupDef: XExtensionBitGroupKeyword '(' XYesOrNoOrReverse ','
                       XIdentifier ',' XIdentifier ')'
        { if(rawstruct->ext_bit_goup_num==0){
            rawstruct->ext_bit_groups=
                    (rawAST_ext_bit_group*)Malloc(sizeof(rawAST_ext_bit_group));
            rawstruct->ext_bit_goup_num=1;
          } else{
            rawstruct->ext_bit_goup_num++;
            rawstruct->ext_bit_groups=(rawAST_ext_bit_group*)
                 Realloc(rawstruct->ext_bit_groups,
                      rawstruct->ext_bit_goup_num*sizeof(rawAST_ext_bit_group));
          }
          rawstruct->ext_bit_groups[rawstruct->ext_bit_goup_num-1].ext_bit=$3;
          rawstruct->ext_bit_groups[rawstruct->ext_bit_goup_num-1].from=$5;
          rawstruct->ext_bit_groups[rawstruct->ext_bit_goup_num-1].to=$7;
        }

/* LengthTo */
XLengthToDef : XLengthToKeyword '(' XRecordFieldRefList ')'
        {  };

/* PointerTo */
XPointerToDef : XPointerToKeyword '(' XRecordFieldRef ')'
        { $$ = $3; };

/* Unit */
XUnitDef : XUnitKeyword '(' XBitsOctets ')' { $$ = $3; }
         | XPtrUnitKeyword '(' XBitsOctets ')' { $$ = $3; };
XBitsOctets : XBits {$$=$1;}
          | XOctets {$$=$1;}
          | XNumber {$$=$1;};

/* LengthIndex */
XLengthIndexDef : XLengthIndexKeyword '(' XStructFieldRefOrEmpty ')'
        { $$ = $3; };


/* Tag, Crosstag */
XTagDef : XTagKeyword '(' XAssocList XoptSemiColon ')'
        { free_rawAST_tag_list(&(rawstruct->taglist));
          link_rawAST_tag_list(&(rawstruct->taglist),&$3);};
XCrossTagDef : XCrossTagKeyword '(' XAssocList XoptSemiColon ')'
        { free_rawAST_tag_list(&(rawstruct->crosstaglist));
          link_rawAST_tag_list(&(rawstruct->crosstaglist),&$3);};

XAssocList:
  XAssocElement
  {
    $$.nElements = 1;
    $$.tag = (rawAST_single_tag*)Malloc(sizeof(*$$.tag));
    link_rawAST_single_tag($$.tag, &$1);
  }
| XAssocList XSemiColon XAssocElement
  {
    int dupl_id_index = -1;
    if ($3.nElements > 0) {
      /* the otherwise element is never merged */
      for (int i = 0; i < $1.nElements; i++)
	if (*$1.tag[i].fieldName == *$3.fieldName) {
	  dupl_id_index = i;
	  break;
	}
    }
    if (dupl_id_index >= 0) {
      /* this identifier is already specified in XAssocList
	 merge the new parameters to the existing entry */
      $$ = $1;
      rawAST_single_tag *tag = $$.tag + dupl_id_index;
      tag->keyList = (rawAST_tag_field_value*)
	Realloc(tag->keyList, (tag->nElements + $3.nElements)
	  * sizeof(*tag->keyList));
      memcpy(tag->keyList + tag->nElements, $3.keyList,
	$3.nElements * sizeof(*$3.keyList));
      tag->nElements += $3.nElements;
      delete $3.fieldName;
      Free($3.keyList);
    } else {
      $$.nElements = $1.nElements + 1;
      $$.tag = (rawAST_single_tag*)
	Realloc($1.tag, $$.nElements * sizeof(*$$.tag));
      $$.tag[$1.nElements] = $3;
    }
  }
;

XSemiColon:
  ';'
;

XoptSemiColon:
  /* Empty */
| ';'
;

XAssocElement:
  XIdentifier ',' XKeyIdOrIdList
  {
    $$.fieldName = $1;
    $$.nElements = $3.nElements;
    $$.keyList = $3.keyList;
  }
| XIdentifier ',' XOtherwise
  {
    $$.fieldName = $1;
    $$.nElements = 0;
    $$.keyList = NULL;
  }
;

XKeyIdOrIdList:
  XKeyId
  {
    $$.fieldName = NULL;
    $$.nElements = 1;
    $$.keyList = (rawAST_tag_field_value*)Malloc(sizeof(*$$.keyList));
    $$.keyList[0] = $1;
  }
| XKeyIdList { $$ = $1; }
;

XKeyIdList:
  '{' XMultiKeyId '}' { $$ = $2; }
;

XMultiKeyId:
  XKeyId
  {
    $$.fieldName = NULL;
    $$.nElements = 1;
    $$.keyList = (rawAST_tag_field_value*)Malloc(sizeof(*$$.keyList));
    $$.keyList[0] = $1;
  }
| XMultiKeyId ',' XKeyId
  {
    $$.fieldName = NULL;
    $$.nElements = $1.nElements + 1;
    $$.keyList = (rawAST_tag_field_value*)
      Realloc($1.keyList, $$.nElements * sizeof(*$$.keyList));
    $$.keyList[$1.nElements] = $3;
  }
;

XKeyId:
  XStructFieldRef '=' XRvalue
  {
    $$.keyField = $1;
    $$.value = $3.value;
    $$.v_value = $3.v_value;
  }
;

/* Presence */
XPresenceDef : XPresenceKeyword '(' XKeyIdList XoptSemiColon ')'
        { free_rawAST_single_tag(&(rawstruct->presence));
        link_rawAST_single_tag(&(rawstruct->presence), &$3);}
        | XPresenceKeyword '(' XMultiKeyId XoptSemiColon ')'
        { free_rawAST_single_tag(&(rawstruct->presence));
        link_rawAST_single_tag(&(rawstruct->presence), &$3);};


/* FieldLength */
XFieldLengthDef : XFieldLengthKeyword '(' XNumber ')'
        { $$ = $3;};

/* PtrOffset */
XPtrOffsetDef : XPtrOffsetKeyword '(' XNumber ')'
        { rawstruct->ptroffset = $3; }
        |XPtrOffsetKeyword '(' XIdentifier ')'
        {
	  delete rawstruct->ptrbase;
	  rawstruct->ptrbase = $3;
        }
/* Align */
XAlignDef : XAlignKeyword '(' XLeftOrRight ')'
        { $$ = $3; };
XLeftOrRight : XLeft | XRight;

/* Comp */
XCompDef : XCompKeyword '(' XCompValues ')'
        { $$ = $3; };
XCompValues : XUnsigned | XCompl | XSignbit;

/* ByteOrder */
XByteOrderDef : XByteOrderKeyword '(' XFirstOrLast ')'
        { $$ = $3; };
XFirstOrLast : XFirst | XLast;

/* BitOrder */
XBitOrderInFieldDef : XBitOrderInFieldKeyword '(' XMsbOrLsb ')'
        { $$ = $3; };
XBitOrderInOctetDef : XBitOrderInOctetKeyword '(' XMsbOrLsb ')'
        { $$ = $3; }
        | XBitOrderKeyword '(' XMsbOrLsb ')' { $$ = $3; };
XToplevelDef : XToplevelKeyword '(' XTopDefList ')'
        { };
XTopDefList : XTopDef
        { }
    | XTopDefList ',' XTopDef
        {  };
XTopDef : XBitOrderDef
        { rawstruct->toplevel.bitorder = $1;};
XBitOrderDef : XBitOrderKeyword '(' XMsbOrLsb ')'
        { $$ = $3; };

/* HexOrder */
XHexOrderDef : XHexOrderKeyword '(' XLowOrHigh ')'
        { $$ = $3; };
XLowOrHigh : XLow | XHigh;

/* General types */
/* FieldRefList */
XRecordFieldRefList : XRecordFieldRef
        { rawstruct->lengthto_num = 1;
          rawstruct->lengthto =
                  (Common::Identifier**)Malloc(sizeof(*(rawstruct->lengthto)));
          rawstruct->lengthto[0] = $1;
        }
    | XRecordFieldRefList ',' XRecordFieldRef
        { rawstruct->lengthto_num++;
          rawstruct->lengthto =
                 (Common::Identifier**)Realloc(rawstruct->lengthto,
                       rawstruct->lengthto_num*sizeof(*(rawstruct->lengthto)));
          rawstruct->lengthto[rawstruct->lengthto_num - 1] = $3;
          }
;

XRecordFieldRef : XIdentifier
        { $$ = $1; };

XStructFieldRefOrEmpty : /* Empty */
        { $$ = NULL;}
    | XStructFieldRef
        { $$ = $1; };

XStructFieldRef : XIdentifier
        { $$ = (rawAST_field_list*)Malloc(sizeof(*$$));
          $$->nElements=1;
          $$->names = (Common::Identifier**)Malloc(sizeof(*($$->names)));
          $$->names[0] = $1;
        }
    | XStructFieldRef '.' XIdentifier
        { $$ = $1;
	  $$->nElements++;
          $$->names = (Common::Identifier**)
                      Realloc($$->names, $$->nElements*sizeof(*($$->names)));
          $$->names[$$->nElements - 1] = $3;
	}
;

XRvalue: XIdentifier{
        $$.value = mcopystr($1->get_name().c_str());
        $$.v_value = new Common::Value(Common::Value::V_UNDEF_LOWERID, $1);
        $$.v_value->set_location(infile, @$);
      }
    | XBstring{
        string *str= new string($1);
        $$.value=mprintf(" %s", mymod->add_bitstring_literal(*str).c_str());
        $$.v_value=new Common::Value(Common::Value::V_BSTR,str);
        $$.v_value->set_location(infile, @$);
        Free($1);
      }
    | XHstring{
        string *str= new string($1);
        $$.value=mprintf(" %s", mymod->add_hexstring_literal(*str).c_str());
        $$.v_value=new Common::Value(Common::Value::V_HSTR,str);
        $$.v_value->set_location(infile, @$);
        Free($1);
      }

    | XOstring{
        string *str= new string($1);
        $$.value=mprintf(" %s", mymod->add_octetstring_literal(*str).c_str());
        $$.v_value=new Common::Value(Common::Value::V_OSTR,str);
        $$.v_value->set_location(infile, @$);
        Free($1);
      }
    | XCstring{
	Common::Location loc(infile, @$);
	string *str = parse_charstring_value($1, loc);
	Free($1);
        $$.value=mprintf(" %s", mymod->add_charstring_literal(*str).c_str());
        $$.v_value=new Common::Value(Common::Value::V_CSTR,str);
        $$.v_value->set_location(loc);
      }
    | XFloatValue{
        $$.value=mcopystr(Common::Real2code($1).c_str());
        $$.v_value=new Common::Value(Common::Value::V_REAL, $1);
        $$.v_value->set_location(infile, @$);
      }
    | XNumber{
        $$.value = mcopystr(Common::Int2string($1).c_str());
        $$.v_value=new Common::Value(Common::Value::V_INT, $1);
        $$.v_value->set_location(infile, @$);
      }
    | XBooleanConst
      {
        $$.value = mcopystr($1 ? "TRUE" : "FALSE");
        $$.v_value=new Common::Value(Common::Value::V_BOOL, $1);
        $$.v_value->set_location(infile, @$);
      }
    | XVerdictConst
      {
	$$.v_value = new Common::Value(Common::Value::V_VERDICT, $1);
        $$.v_value->set_location(infile, @$);
	$$.value = mcopystr($$.v_value->get_single_expr().c_str());
      }
    | XNullKeyword
      {
	$$.value = mcopystr("NULL_COMPREF");
	$$.v_value = new Common::Value(Common::Value::V_TTCN3_NULL);
        $$.v_value->set_location(infile, @$);
      }
    | XNULLKeyword
      {
	$$.value = mcopystr("ASN_NULL_VALUE");
	$$.v_value = new Common::Value(Common::Value::V_NULL);
        $$.v_value->set_location(infile, @$);
      }
    | XOmitKeyword
      {
	$$.value = mcopystr("OMIT_VALUE");
	$$.v_value = new Common::Value(Common::Value::V_OMIT);
	$$.v_value->set_location(infile, @$);
      }
;

/* Alternative RAW attributes for types defined in annex E of the TTCN-3 standard */
XBitDef:
  XNumber XBitKeyword
  {
    rawstruct->fieldlength = $1;
    rawstruct->comp = XDEFSIGNBIT;
    rawstruct->byteorder = XDEFLAST;
  }
| XUnsignedKeyword XNumber XBitKeyword
  {
    rawstruct->fieldlength = $2;
    rawstruct->comp = XDEFUNSIGNED;
    rawstruct->byteorder = XDEFLAST;
  }
;

XUTFDef:
  XUTF8Keyword { rawstruct->stringformat = CharCoding::UTF_8; }
| XUTF16Keyword { rawstruct->stringformat = CharCoding::UTF16; }

XIEEE754Def:
  XIEEE754FloatKeyword { rawstruct->fieldlength = 32; }
| XIEEE754DoubleKeyword { rawstruct->fieldlength = 64; }

/* Text encoder */
XBeginDef:
    XBeginKeyword '(' XEncodeToken ')' {
       if(textstruct->begin_val==NULL){
          textstruct->begin_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->begin_val);
        }
        Free(textstruct->begin_val->encode_token);
        textstruct->begin_val->encode_token=$3;
      }
    | XBeginKeyword '(' XEncodeToken ',' XmatchDef ')' {
       if(textstruct->begin_val==NULL){
          textstruct->begin_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->begin_val);
        }
        Free(textstruct->begin_val->encode_token);
        textstruct->begin_val->encode_token=$3;
        if($5){
          Free(textstruct->begin_val->decode_token);
          textstruct->begin_val->decode_token=$5;
        }
      }
    | XBeginKeyword '(' XEncodeToken ',' XmatchDef ',' XmodifierDef ')'{
       if(textstruct->begin_val==NULL){
          textstruct->begin_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->begin_val);
        }
        Free(textstruct->begin_val->encode_token);
        textstruct->begin_val->encode_token=$3;
        if($5){
          Free(textstruct->begin_val->decode_token);
          textstruct->begin_val->decode_token=$5;
        }
        textstruct->begin_val->case_sensitive=$7;
      }
;

XEndDef:
    XEndKeyword '(' XEncodeToken ')' {
       if(textstruct->end_val==NULL){
          textstruct->end_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->end_val);
        }
        Free(textstruct->end_val->encode_token);
        textstruct->end_val->encode_token=$3;
      }
    | XEndKeyword '(' XEncodeToken ',' XmatchDef ')'{
       if(textstruct->end_val==NULL){
          textstruct->end_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->end_val);
        }
        Free(textstruct->end_val->encode_token);
        textstruct->end_val->encode_token=$3;
        if($5){
          Free(textstruct->end_val->decode_token);
          textstruct->end_val->decode_token=$5;
        }
      }
    | XEndKeyword '(' XEncodeToken ',' XmatchDef ',' XmodifierDef ')'{
       if(textstruct->end_val==NULL){
          textstruct->end_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->end_val);
        }
        Free(textstruct->end_val->encode_token);
        textstruct->end_val->encode_token=$3;
        if($5){
          Free(textstruct->end_val->decode_token);
          textstruct->end_val->decode_token=$5;
        }
        textstruct->end_val->case_sensitive=$7;
      }
;

XSeparatorDef:
    XSeparatorKeyword '(' XEncodeToken ')'{
       if(textstruct->separator_val==NULL){
          textstruct->separator_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->separator_val);
        }
        Free(textstruct->separator_val->encode_token);
        textstruct->separator_val->encode_token=$3;
      }
    | XSeparatorKeyword '(' XEncodeToken ',' XmatchDef ')'{
       if(textstruct->separator_val==NULL){
          textstruct->separator_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->separator_val);
        }
        Free(textstruct->separator_val->encode_token);
        textstruct->separator_val->encode_token=$3;
        if($5){
          Free(textstruct->separator_val->decode_token);
          textstruct->separator_val->decode_token=$5;
        }
      }
    | XSeparatorKeyword '(' XEncodeToken ',' XmatchDef ',' XmodifierDef ')'{
       if(textstruct->separator_val==NULL){
          textstruct->separator_val=(textAST_matching_values*)
                                        Malloc(sizeof(textAST_matching_values));
          init_textAST_matching_values(textstruct->separator_val);
        }
        Free(textstruct->separator_val->encode_token);
        textstruct->separator_val->encode_token=$3;
        if($5){
          Free(textstruct->separator_val->decode_token);
          textstruct->separator_val->decode_token=$5;
        }
        textstruct->separator_val->case_sensitive=$7;
      }
;

XCodingDef:
    XCodingKeyword '(' XCodingRule ')'
        { }
    | XCodingKeyword '(' XCodingRule ',' XDecodingRule ')'
        { }
    | XCodingKeyword '(' XCodingRule ',' XDecodingRule ',' XmatchDef ')' {
          Free(textstruct->decode_token);
          textstruct->decode_token=$7;
        }
    | XCodingKeyword '(' XCodingRule ',' XDecodingRule ',' XmatchDef ','
                                                           XmodifierDef ')' {
          if($7){
            Free(textstruct->decode_token);
            textstruct->decode_token=$7;
          }
          textstruct->case_sensitive=$9;
        }
;

XCodingRule:
    /* empty */ {}
    | Xattrlistenc {}
    | XtokendefList {}
;

XDecodingRule:
    /* empty */ {}
    | XattrList {}
    | XDecodingtokendefList {}
;

XattrList:
   Xattr {}
   | XattrList ';' Xattr {}
;

Xattr:
   XLengthToken '=' XNumber {
     textstruct->decoding_params.min_length=$3;
   }
   | XLengthToken '=' XNumber '-' XNumber {
     textstruct->decoding_params.min_length=$3;
     textstruct->decoding_params.max_length=$5;
   }
   | XConvertToken '=' XLowerToken {
     textstruct->decoding_params.convert=-1;
   }
   | XConvertToken '=' XUpperToken {
     textstruct->decoding_params.convert=1;
   }
   | XJustToken '=' XLeftToken {
     textstruct->decoding_params.just=-1;
   }
   | XJustToken '=' XRightToken {
     textstruct->decoding_params.just=1;
   }
   | XJustToken '=' XCenterToken {
     textstruct->decoding_params.just=0;
   }
   | XLeadingToken '=' XTrueToken {
     textstruct->decoding_params.leading_zero=true;
   }
   | XLeadingToken '=' XFalseToken {
     textstruct->decoding_params.leading_zero=false;
   }
   | XRepeatToken '=' XTrueToken {
     textstruct->decoding_params.repeatable=true;
   }
   | XRepeatToken '=' XFalseToken {
     textstruct->decoding_params.repeatable=false;
   };

Xattrlistenc:
   Xattrenc {}
   | Xattrlistenc ';' Xattrenc {}
;

Xattrenc:
   XLengthToken '=' XNumber {
     textstruct->coding_params.min_length=$3;
     textstruct->decoding_params.min_length=$3;
   }
   | XLengthToken '=' XNumber '-' XNumber {
     textstruct->coding_params.min_length=$3;
     textstruct->coding_params.max_length=$5;
     textstruct->decoding_params.min_length=$3;
     textstruct->decoding_params.max_length=$5;
   }
   | XConvertToken '=' XLowerToken {
     textstruct->coding_params.convert=-1;
     textstruct->decoding_params.convert=-1;
   }
   | XConvertToken '=' XUpperToken {
     textstruct->coding_params.convert=1;
     textstruct->decoding_params.convert=1;
   }
   | XJustToken '=' XLeftToken {
     textstruct->coding_params.just=-1;
     textstruct->decoding_params.just=-1;
   }
   | XJustToken '=' XRightToken {
     textstruct->coding_params.just=1;
     textstruct->decoding_params.just=1;
   }
   | XJustToken '=' XCenterToken {
     textstruct->coding_params.just=0;
     textstruct->decoding_params.just=0;
   }
   | XLeadingToken '=' XTrueToken {
     textstruct->coding_params.leading_zero=true;
     textstruct->decoding_params.leading_zero=true;
   }
   | XLeadingToken '=' XFalseToken {
     textstruct->coding_params.leading_zero=false;
     textstruct->decoding_params.leading_zero=false;
   }
   | XRepeatToken '=' XTrueToken {
     textstruct->coding_params.repeatable=true;
     textstruct->decoding_params.repeatable=true;
   }
   | XRepeatToken '=' XFalseToken {
     textstruct->coding_params.repeatable=false;
     textstruct->decoding_params.repeatable=false;
   };


XtokendefList:
   Xtokendef {}
   | XtokendefList ';' Xtokendef {}
;

Xtokendef:
  XIdentifierOrReserved ':' XEncodeToken
  {
    size_t idx = textstruct->get_field_param_index($1);
    if (textstruct->field_params[idx]->value.encode_token) {
      Free(textstruct->field_params[idx]->value.encode_token);
      Common::Location loc(infile, @3);
      loc.error("Duplicate encode token for value `%s'",
	$1->get_dispname().c_str());
    }
    textstruct->field_params[idx]->value.encode_token = $3;
    delete $1;
  }
| XTrueToken ':' XEncodeToken
  {
    if (textstruct->true_params == NULL) {
      textstruct->true_params = (textAST_matching_values*)
	Malloc(sizeof(textAST_matching_values));
      textstruct->true_params->encode_token = $3;
      textstruct->true_params->decode_token = NULL;
      textstruct->true_params->case_sensitive = true;
      textstruct->true_params->generated_decode_token = false;
    } else {
      if (textstruct->true_params->encode_token) {
	Free(textstruct->true_params->encode_token);
	Common::Location loc(infile, @3);
	loc.error("Duplicate encode token for true value");
      }
      textstruct->true_params->encode_token = $3;
    }
  }
| XFalseToken ':' XEncodeToken
  {
    if (textstruct->false_params == NULL) {
      textstruct->false_params = (textAST_matching_values*)
	Malloc(sizeof(textAST_matching_values));
      textstruct->false_params->encode_token = $3;
      textstruct->false_params->decode_token = NULL;
      textstruct->false_params->case_sensitive = true;
      textstruct->false_params->generated_decode_token = false;
    } else {
      if (textstruct->false_params->encode_token) {
	Free(textstruct->false_params->encode_token);
	Common::Location loc(infile, @3);
	loc.error("Duplicate encode token for false value");
      }
      textstruct->false_params->encode_token = $3;
    }
  }
;

XIdentifierOrReserved:
  XIdentifier { $$ = $1; }
| XTextReservedWord
  { $$ = new Common::Identifier(Common::Identifier::ID_TTCN, string($1)); }
;

XTextReservedWord:
  XLengthToken { $$ = "length"; }
| XRepeatToken { $$ = "repeatable"; }
| XConvertToken { $$ = "convert"; }
| XLowerToken { $$ = "lower_case"; }
| XUpperToken { $$ = "upper_case"; }
| XJustToken { $$ = "just"; }
| XLeftToken { $$ = "left"; }
| XRightToken { $$ = "right"; }
| XCenterToken { $$ = "center"; }
| XLeadingToken { $$ = "leading0"; }
| XSensitivToken { $$ = "case_sensitive"; }
| XInSensitivToken { $$ = "case_insensitive"; }
;

XDecodingtokendefList:
    Xdecodingtokendef  {}
   |  XDecodingtokendefList ';' Xdecodingtokendef {}
;

Xdecodingtokendef:
  XIdentifierOrReserved ':' XDecodeToken
  {
    size_t idx = textstruct->get_field_param_index($1);
    if (textstruct->field_params[idx]->value.decode_token) {
      Free(textstruct->field_params[idx]->value.decode_token);
      Common::Location loc(infile, @3);
      loc.error("Duplicate decode token for value `%s'",
	$1->get_dispname().c_str());
    }
    textstruct->field_params[idx]->value.decode_token = $3.token;
    textstruct->field_params[idx]->value.case_sensitive = $3.case_sensitive;
    delete $1;
  }
| XTrueToken ':' XDecodeToken
  {
    if (textstruct->true_params == NULL) {
      textstruct->true_params = (textAST_matching_values*)
	Malloc(sizeof(textAST_matching_values));
      textstruct->true_params->encode_token = NULL;
      textstruct->true_params->decode_token = $3.token;
      textstruct->true_params->case_sensitive = $3.case_sensitive;
      textstruct->true_params->generated_decode_token = false;
    } else {
      if (textstruct->true_params->decode_token) {
	Free(textstruct->true_params->decode_token);
	Common::Location loc(infile, @3);
	loc.error("Duplicate decode token for true value");
      }
      textstruct->true_params->decode_token = $3.token;
      textstruct->true_params->case_sensitive = $3.case_sensitive;
    }
  }
| XFalseToken ':' XDecodeToken
  {
    if (textstruct->false_params == NULL) {
      textstruct->false_params = (textAST_matching_values*)
	Malloc(sizeof(textAST_matching_values));
      textstruct->false_params->encode_token = NULL;
      textstruct->false_params->decode_token= $3.token;
      textstruct->false_params->case_sensitive = $3.case_sensitive;
      textstruct->false_params->generated_decode_token = false;
    } else {
      if (textstruct->false_params->decode_token) {
	Free(textstruct->false_params->decode_token);
	Common::Location loc(infile, @3);
	loc.error("Duplicate decode token for false value");
      }
      textstruct->false_params->decode_token = $3.token;
      textstruct->false_params->case_sensitive = $3.case_sensitive;
    }
  }
;

XEncodeToken:
  Xtoken
  {
    Common::Location loc(infile, @$);
    string *str = parse_charstring_value($1, loc);
    Free($1);
    $$ = mcopystr(str->c_str());
    delete str;
  }
;

XDecodeToken:
  Xtoken
  {
    $$.token = $1;
    $$.case_sensitive = true;
  }
| '{' Xtoken '}'
  {
    $$.token = $2;
    $$.case_sensitive = true;
  }
| '{' XmatchDef ',' XmodifierDef '}'
  {
    $$.token = $2;
    $$.case_sensitive = $4;
  }
;

XmatchDef:
  /* empty */ { $$ = NULL; }
| Xtoken { $$ = $1; }
;

XmodifierDef:
  /* empty */ { $$ = true; }
| XSensitivToken { $$ = true; }
| XInSensitivToken { $$ = false; }
;

	/********** XERSTUFF "raw" attributes */

XERattributes: /* a non-empty list */
    XERattribute { xer_f = true; }
    ;

XERattribute:
    XKWabstract { xerstruct->abstract_ = true; }
    | anyAttributes { FreeNamespaceRestriction(xerstruct->anyAttributes_); xerstruct->anyAttributes_ = $1; }
    | anyElement  { FreeNamespaceRestriction(xerstruct->anyElement_); xerstruct->anyElement_ = $1; }
    | XKWattribute  { xerstruct->attribute_ = true; }
    | XKWattributeFormQualified { xerstruct->form_ |= XerAttributes::ATTRIBUTE_DEFAULT_QUALIFIED; }
    | XKWblock { xerstruct->block_ = true; }
    | controlNamespace /* directly on the module */
      {
        mymod->set_controlns($1.uri, $1.prefix);
      }
    | defaultForEmpty
      {
        xerstruct->defaultForEmpty_ = $1.str;
        xerstruct->defaultForEmptyIsRef_ = $1.ref;
        xerstruct->defaultForEmptyRef_ = $1.reference;
      }
    | XKWelement { xerstruct->element_ = true; }
    | XKWelementFormQualified { xerstruct->form_ |= XerAttributes::ELEMENT_DEFAULT_QUALIFIED; }
    | XKWembedValues { xerstruct->embedValues_ = true; }
    | form { xerstruct->form_ |= $1; }
    | fractionDigits { xerstruct->fractionDigits_ = $1; xerstruct->has_fractionDigits_ = true; }
    | XKWlist { xerstruct->list_ = true; }
    | name
      { /* overwrites any previous name */
        switch (xerstruct->name_.kw_) {
        case NamespaceSpecification::NO_MANGLING:
        case NamespaceSpecification::CAPITALIZED:
        case NamespaceSpecification::UNCAPITALIZED:
        case NamespaceSpecification::UPPERCASED:
        case NamespaceSpecification::LOWERCASED:
          break; // nothing to do
        default: // real string, must be freed
          Free(xerstruct->name_.nn_);
        }
        xerstruct->name_.nn_ = $1;
      }
    | namespace
      { /* overwrites any previous namespace */
        Free(xerstruct->namespace_.uri);
        Free(xerstruct->namespace_.prefix);
        xerstruct->namespace_ = $1;
      }
    | text
      {
        xerstruct->text_ = (NamespaceSpecification *)Realloc(xerstruct->text_,
          ++xerstruct->num_text_ * sizeof(NamespaceSpecification));
        xerstruct->text_[xerstruct->num_text_-1] = $1;
      }
    | XKWuntagged  { xerstruct->untagged_  = true; }
    | XKWuseNil    { xerstruct->useNil_    = true; }
    | XKWuseNumber { xerstruct->useNumber_ = true; }
    | XKWuseOrder  { xerstruct->useOrder_  = true; }
    | XKWuseUnion  { xerstruct->useUnion_  = true; }
    | XKWuseType   { xerstruct->useType_   = true; }
    | whiteSpace
      {
        xerstruct->whitespace_ = static_cast<XerAttributes::WhitespaceAction>($1);
      }
    |   XSD ':' xsddata {}
    ;

anyAttributes:
    XKWanyAttributes optNamespaceRestriction
    { $$ = $2; }
    ;

anyElement:
    XKWanyElement optNamespaceRestriction
    { $$ = $2; }
    ;

optNamespaceRestriction:
    /* empty */ {
      $$.nElements_ = 0; $$.uris_ = 0; $$.type_ = NamespaceRestriction::NOTHING;
    }
    | XKWfrom   urilist { $$ = $2; $$.type_ = NamespaceRestriction::FROM; }
    | XKWexcept urilist { $$ = $2; $$.type_ = NamespaceRestriction::EXCEPT; }
    ;

urilist: /* a non-empty list */
    quotedURIorAbsent {
      $$.nElements_ = 1;
      $$.uris_ = (char**)Malloc(sizeof($$.uris_[0]));
      $$.uris_[0] = $1;
    }
    | urilist ',' quotedURIorAbsent
    {
      $$ = $1;
      $$.uris_ = (char**)Realloc($$.uris_, ++$$.nElements_ * sizeof($$.uris_[0]));
      $$.uris_[$$.nElements_-1] = $3;
    }
    ;

quotedURIorAbsent:
    Xstring /* as quotedURI */
    | XKWunqualified { $$ = NULL; }
    ;

controlNamespace: /* nsspec */
    XKWcontrolNamespace Xstring /* <-- as the QuotedURI */ XKWprefix Xstring
    /* Prefix is required by TTCN-3; it is optional in the ASN.1 standard
     * but the OSS ASN.1 compiler seems to require it anyway */
    {
      $$.uri = $2;
      $$.prefix = $4;
    }
    ;


form:
    XKWform XKWas XKWunqualified { $$ = XerAttributes::UNQUALIFIED; }
    | XKWform XKWas XKWqualified { $$ = XerAttributes::QUALIFIED; }


name:
    XKWname XKWas newnameOrKeyword { $$ = $3; }
    ;

newnameOrKeyword: keyword
| Xstring { $$ = $1; }
;

keyword:
XKWcapitalized     { $$ = (char*)NamespaceSpecification::CAPITALIZED; }
| XKWuncapitalized { $$ = (char*)NamespaceSpecification::UNCAPITALIZED; }
| XKWlowercased    { $$ = (char*)NamespaceSpecification::LOWERCASED; }
| XKWuppercased    { $$ = (char*)NamespaceSpecification::UPPERCASED; }
;

namespace:
    XKWnamespace namespacespecification
    { $$ = $2; }
;

namespacespecification:
    XKWas
    Xstring /* as QuotedURI, required */
    optPrefix
    {
      if (*$2) {
        $$.uri = $2; $$.prefix = $3;
      }
      else { /* URI is empty */
        if (*$3) { /* prefix not empty, error */
          Common::Location loc(infile, @2);
          loc.error("Empty string is not a valid URI");
        }
        else {
          /* Both are empty strings, use one of the non-string values
           * to signal "override and remove any namespace" */
          Free($2);
          Free($3);
          $$.keyword = NamespaceSpecification::UNCAPITALIZED;
          $$.prefix = NULL;
        }
      }
    }
;

optPrefix: /* empty */ { $$ = memptystr(); }
|    XKWprefix Xstring
    { $$ = $2; }
;

text:
    XKWtext Xstring XKWas newnameOrKeyword
    { $$.uri = $4; $$.prefix = $2; }
    | XKWtext XKWall  XKWas newnameOrKeyword
    { $$.uri = $4; $$.prefix = (char*)NamespaceSpecification::ALL; }
    | XKWtext
    { $$.uri = 0; $$.prefix = 0; }
    /* "text as <something>" is not allowed */
    ;

defaultForEmpty:
    XKWdefaultForEmpty XKWas Xstring
    { $$.str = $3; $$.ref = false; $$.reference = NULL; }
    | XKWdefaultForEmpty XKWas XIdentifier
    { $$.reference = new Ttcn::Reference($3);
      $$.reference->set_location(infile, @$);
      $$.ref = true;
      $$.str = NULL;
    }
    | XKWdefaultForEmpty XKWas XIdentifier '.' XIdentifier
    { $$.reference = new Ttcn::Reference($3, $5);
      $$.reference->set_location(infile, @$);
      $$.ref = true;
      $$.str = NULL;
    }
;

fractionDigits:
   XKWfractionDigits XNumber
   { $$ = $2; } 



whiteSpace:
    XKWwhiteSpace XKWpreserve { $$ = XerAttributes::PRESERVE; }
|   XKWwhiteSpace XKWreplace  { $$ = XerAttributes::REPLACE; }
|   XKWwhiteSpace XKWcollapse { $$ = XerAttributes::COLLAPSE; }
;

xsddata: /* XSD:something */
    XSDbase64Binary {
      xerstruct->base64_ = true;
      xerstruct->xsd_type = XSD_BASE64BINARY;
    }
    | XSDdecimal {
      xerstruct->decimal_ = true;
      xerstruct->xsd_type = XSD_DECIMAL;
    }
    | XSDhexBinary {
      xerstruct->hex_ = true;
      xerstruct->xsd_type = XSD_HEXBINARY;
    }
    | XSDQName {
      xerstruct->useQName_ = true;
      xerstruct->xsd_type = XSD_QNAME;
    }
    /* everything below is recognized */
    | XKWshort { xerstruct->xsd_type = XSD_SHORT; }
    | XKWlong { xerstruct->xsd_type = XSD_LONG; }
    | XSDstring           { xerstruct->xsd_type = XSD_STRING; }
    | XSDnormalizedString { xerstruct->xsd_type = XSD_NORMALIZEDSTRING; }
    | XSDtoken            { xerstruct->xsd_type = XSD_TOKEN; }
    | XSDName             { xerstruct->xsd_type = XSD_NAME; }
    | XSDNMTOKEN          { xerstruct->xsd_type = XSD_NMTOKEN; }
    | XSDNCName           { xerstruct->xsd_type = XSD_NCName; }
    | XSDID               { xerstruct->xsd_type = XSD_ID; }
    | XSDIDREF            { xerstruct->xsd_type = XSD_IDREF; }
    | XSDENTITY           { xerstruct->xsd_type = XSD_ENTITY; }
    | XSDanyURI           { xerstruct->xsd_type = XSD_ANYURI; }
    | XSDlanguage         { xerstruct->xsd_type = XSD_LANGUAGE; }
    /* TODO apply subtype to the types below */
    | XSDinteger          { xerstruct->xsd_type = XSD_INTEGER; }
    | XSDpositiveInteger  { xerstruct->xsd_type = XSD_POSITIVEINTEGER; }
    | XSDnonPositiveInteger { xerstruct->xsd_type = XSD_NONPOSITIVEINTEGER; }
    | XSDnegativeInteger { xerstruct->xsd_type = XSD_NEGATIVEINTEGER; }
    | XSDnonNegativeInteger { xerstruct->xsd_type = XSD_NONNEGATIVEINTEGER; }
    | XSDunsignedLong     { xerstruct->xsd_type = XSD_UNSIGNEDLONG; }
    | XSDint              { xerstruct->xsd_type = XSD_INT; }
    | XSDunsignedInt      { xerstruct->xsd_type = XSD_UNSIGNEDINT; }
    | XSDunsignedShort    { xerstruct->xsd_type = XSD_UNSIGNEDSHORT; }
    | XSDbyte             { xerstruct->xsd_type = XSD_BYTE; }
    | XSDunsignedByte     { xerstruct->xsd_type = XSD_UNSIGNEDBYTE; }
    | XSDfloat            { xerstruct->xsd_type = XSD_FLOAT; }
    | XSDdouble           { xerstruct->xsd_type = XSD_DOUBLE; }
    | XSDduration         { xerstruct->xsd_type = XSD_DURATION; }
    | XSDdateTime         { xerstruct->xsd_type = XSD_DATETIME; }
    | XSDtime             { xerstruct->xsd_type = XSD_TIME; }
    | XSDdate             { xerstruct->xsd_type = XSD_DATE; }
    | XSDgYearMonth       { xerstruct->xsd_type = XSD_GYEARMONTH; }
    | XSDgYear            { xerstruct->xsd_type = XSD_GYEAR; }
    | XSDgMonthDay        { xerstruct->xsd_type = XSD_GMONTHDAY; }
    | XSDgDay             { xerstruct->xsd_type = XSD_GDAY; }
    | XSDgMonth           { xerstruct->xsd_type = XSD_GMONTH; }
    | XSDNMTOKENS         { xerstruct->xsd_type = XSD_NMTOKENS; }
    | XSDIDREFS           { xerstruct->xsd_type = XSD_IDREFS; }
    | XSDENTITIES         { xerstruct->xsd_type = XSD_ENTITIES; }
    | XSDboolean          { xerstruct->xsd_type = XSD_BOOLEAN; }

    | XSDanySimpleType    { xerstruct->xsd_type = XSD_ANYSIMPLETYPE; }
    | XSDanyType          { xerstruct->xsd_type = XSD_ANYTYPE; }

;

// JSON encoder
XJsonDef:
  XOptSpaces XKWjson XOptSpaces ':' XOptSpaces XJsonAttribute XOptSpaces
;

XJsonAttribute:
  XOmitAsNull 
| XNameAs
| XAsValue
| XDefault
| XExtend
| XMetainfoForUnbound
;

XOmitAsNull:
  XKWomit XSpaces XKWas XSpaces XKWnull { jsonstruct->omit_as_null = true; }
;

XNameAs:
  XKWname XSpaces XKWas XSpaces XJsonAlias { jsonstruct->alias = $5; }
;

XJsonAlias: // include all keywords, so they can be used as aliases for fields, too
  XAliasToken { $$ = $1; }
| XKWomit     { $$ = mcopystr("omit"); }
| XKWas       { $$ = mcopystr("as"); }
| XKWnull     { $$ = mcopystr("null"); }
| XKWname     { $$ = mcopystr("name"); }
| XKWvalue    { $$ = mcopystr("value"); }
| XKWdefault  { $$ = mcopystr("default"); }
| XKWextend   { $$ = mcopystr("extend"); }
| XKWmetainfo { $$ = mcopystr("metainfo"); }
| XKWfor      { $$ = mcopystr("for"); }
| XKWunbound  { $$ = mcopystr("unbound"); }

XAsValue:
  XKWas XSpaces XKWvalue { jsonstruct->as_value = true; }
;

XDefault:
  XKWdefault XOptSpaces XJsonValue { jsonstruct->default_value = $3; }
;

XExtend:
  XKWextend XOptSpaces XJsonValue XOptSpaces ':' XOptSpaces XJsonValue
  { jsonstruct->schema_extensions.add(new JsonSchemaExtension($3, $7)); }
;

XJsonValue:
  XJsonValueStart XJsonValueCore XJsonValueEnd { $$ = $2; }
| XJsonValueStart XJsonValueEnd                { $$ = mcopystr(""); }
;

XJsonValueCore:
  XJsonValueCore XJsonValueSegment { $$ = mputstr($1, $2); Free($2); }
| XJsonValueSegment                { $$ = $1; }
;

XMetainfoForUnbound:
  XKWmetainfo XOptSpaces XKWfor XOptSpaces XKWunbound { jsonstruct->metainfo_unbound = true; }
;

XOptSpaces:
  /* Empty */
| XSpaces
;

XSpaces:
  XSpaces XSpace
| XSpace
;

XSpace:
  ' '
| '\t'
;

%%

/* parse_rawAST(), which calls our rawAST_parse, is over in rawASST.l */

#ifndef NDEBUG
static void yyprint(FILE *file, int type, const YYSTYPE& value)
{
  switch (type) {
  case XIdentifier:
    fprintf(file, "``%s''", value.identifier->get_name().c_str());
    break;
  case Xstring:
    fprintf(file, "``%s''", value.str);
    break;
  default:
    fprintf(file, "# %d", type);
    break;
  }
}
#endif

/*
 Local Variables:
 mode: C
 indent-tabs-mode: nil
 c-basic-offset: 4
 End:
*/

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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput

%{

#include "../../common/memory.h"
#include "../Int.hh"
#include "../Real.hh"
#include "../Setting.hh"
#include "../Value.hh"
#include "../main.hh"
#include "RawAST.hh"
#include "../XerAttributes.hh"
#include "rawASTspec.h"
// the next one is the header generated from the .y
#include "rawAST.tab.hh"
#include "AST_ttcn3.hh"
#include "Attributes.hh"
#include "BerAST.hh"

using namespace Common;

#define ALLOCATE_IF_NULL(retv, oldptr, ptype) \
    if((oldptr)==NULL){\
        retv = (ptype*)Malloc(sizeof(ptype));  \
    } else \
        retv = oldptr;

const Common::Module *mymod;
RawAST *rawstruct;
TextAST *textstruct;
XerAttributes *xerstruct;
BerAST *berstruct;
JsonAST *jsonstruct;
bool raw_f;
bool text_f;
bool xer_f;
bool ber_f;
bool json_f;
int length_multiplier;

extern void rawAST_error(const char *str); /* defined in this file */
extern int rawAST_parse(); /* in rawAST.tab.cc, generated from rawAST.y */

//rawAST_full_spec rawAST_result;
//rawAST_encoding_spec rawAST_partial_result;

extern int rawAST_debug;

#define yylval rawAST_lval
#define yylloc rawAST_lloc

extern YYLTYPE yylloc;

/* always points to the first character of the regexp to be recognized */
static int current_line, current_column;

static void update_lloc()
{
  yylloc.first_line = current_line;
  yylloc.first_column = current_column;
  current_column += yyleng;
  yylloc.last_line = current_line;
  yylloc.last_column = current_column;
}

#define RETURN(retv) update_lloc(); return retv

%}

/* definitions */

LINECOMMENT "//"[^\r\n]*
WHITESPACE [ \t\v\f]+
NEWLINE \r|\n|\r\n

BIN 0|1
HEX [0-9A-Fa-f]
OCT {HEX}{HEX}
FLOAT ({NUMBER}\.[0-9]+)|((({NUMBER}(\.[0-9]+)?)|(\.[0-9]+))[Ee][+-]?{NUMBER})
NUMBER 0|([1-9][0-9]*)
IDENTIFIER [A-Za-z][A-Za-z0-9_]*
DQUOTE (\"|\\)\"
CHAR [^\"\\]|\\[^\"]|{DQUOTE}{DQUOTE}
MATCH_CHAR [^'\\]|"''"|\\(.|\n)
RPAREN [)]

/* start conditions (x: exclusive) */

%x ccomment
%x textcodec
%x rawcodec
%x jsoncodec
%x jsonvalue

%%
  int comment_caller = INITIAL;

<INITIAL,textcodec,rawcodec>{           /* all but ccomment */
"/*"	{
  yylloc.first_line = current_line;
  yylloc.first_column = current_column;
  current_column += yyleng;
  comment_caller = YY_START;
  BEGIN(ccomment);
}

{WHITESPACE}|{LINECOMMENT} current_column += yyleng;

}       /* end <all but ccomment> */

<*>{NEWLINE} current_line++; current_column = 0;

<ccomment>{
"*/" current_column += yyleng; BEGIN(comment_caller);
.    current_column++;
<<EOF>>	{
  yylloc.last_line = current_line;
  yylloc.last_column = current_column;
  Location loc(infile, yylloc);
  loc.error("Unterminated block comment in variant attribute");
  RETURN(EOF);
}
}       /* ccomment end */

<INITIAL,textcodec,rawcodec>{NUMBER}  {
  update_lloc();
  Location loc(infile, yylloc);
  yylval.intval = string2Int(yytext, loc);
  return XNumber;
}

<rawcodec,INITIAL>{

{FLOAT}	  {
  update_lloc();
  Location loc(infile, yylloc);
  yylval.floatval = string2Real(yytext, loc);
  return XFloatValue;
}
true	{ yylval.boolval = true;
          RETURN(XBooleanConst);}
false	{ yylval.boolval = false;
          RETURN(XBooleanConst);}
none 	{ yylval.verdictval = Value::Verdict_NONE;
	  RETURN(XVerdictConst);}
pass	{ yylval.verdictval = Value::Verdict_PASS;
	  RETURN(XVerdictConst);}
inconc	{ yylval.verdictval = Value::Verdict_INCONC;
	  RETURN(XVerdictConst);}
fail	{ yylval.verdictval = Value::Verdict_FAIL;
	  RETURN(XVerdictConst);}
error	{ yylval.verdictval = Value::Verdict_ERROR;
	  RETURN(XVerdictConst);}
null	RETURN(XNullKeyword);
NULL	RETURN(XNULLKeyword);
omit	RETURN(XOmitKeyword);

{DQUOTE}{CHAR}*{DQUOTE}	{
  yylloc.first_line = current_line;
  /* we have to cheat because parse_charstring_value() expects one quotation
   * mark character as delimiter, but {DQUOTE} matches two characters */
  yylloc.first_column = current_column + 1;
  current_column += 2;
  bool backslash_flag = false;
  yylval.str = memptystr();
  size_t len = (size_t)yyleng - 2;
  for (size_t i = 2; i < len; ) {
    if (!backslash_flag && (yytext[i] == '\\' || yytext[i] == '"') &&
	yytext[i + 1] == '"' &&
	(yytext[i + 2] == '\\' || yytext[i + 2] == '"') &&
	yytext[i + 3] == '"') {
      /* transform embedded escaped quotes: \"\" or """" -> "" */
      /* we must use the octal notation to keep the column numbers in synch */
      yylval.str = mputstr(yylval.str, "\\042");
      current_column += 4;
      i += 4;
    } else if (yytext[i] == '\r' && yytext[i + 1] == '\n') {
      /* handle the CR-LF sequence as a single newline */
      yylval.str = mputstr(yylval.str, "\r\n");
      current_line++;
      current_column = 0;
      backslash_flag = false;
      i += 2;
    } else {
      yylval.str = mputc(yylval.str, yytext[i]);
      if (yytext[i] == '\r' || yytext[i] == '\n') {
	  current_line++;
	  current_column = 0;
      } else current_column++;
      if (backslash_flag) backslash_flag = false;
      else if (yytext[i] == '\\') backslash_flag = true;
      i++;
    }
  }
  current_column += 2;
  yylloc.last_line = current_line;
  yylloc.last_column = current_column - 1;
  return XCstring;
}

'{BIN}*'B {
  yytext[yyleng - 2] = '\0';
  yylval.str = mcopystr(yytext + 1);
  RETURN(XBstring);
}

'[^\']*'B { /* a properly delimited bit string with incorrect content */
  /* the backslash in the character class is redundant, but helps editors
   * get the syntax highlighting right */
  update_lloc();
  rawAST_error("invalid bitstring value");
  yylval.str = memptystr();
  return XBstring;
}

'{HEX}*'H {
  yytext[yyleng - 2] = '\0';
  yylval.str = mcopystr(yytext + 1);
  RETURN(XHstring);
}

'[^\']*'H { /* a properly delimited hex string with incorrect content */
  update_lloc();
  rawAST_error("invalid hexstring value");
  yylval.str = memptystr();
  return XHstring;
}

'{OCT}*'O {
  yytext[yyleng - 2] = '\0';
  yylval.str = mcopystr(yytext + 1);
  RETURN(XOstring);
}

'[^\']*'O { /* a properly delimited octet string with incorrect content */
  update_lloc();
  rawAST_error("invalid octetstring value");
  yylval.str = memptystr();
  return XOstring;
}


'[^\']*' { /* A string delimited by a pair of ' */
  yytext[yyleng - 1] = '\0';
  yylval.str = mcopystr(yytext + 1);
  RETURN(Xstring);
}

}       /* end <rawcodec,INITIAL> */

    /* RAW encoder keywords */
PADDING     { BEGIN(rawcodec); RETURN(XPaddingKeyword); }
PREPADDING  { BEGIN(rawcodec); RETURN(XPrePaddingKeyword); }
PADDING_PATTERN  { BEGIN(rawcodec); RETURN(XPaddingPatternKeyword); }
PADDALL     RETURN(XPaddAllKeyword);
FIELDORDER  { BEGIN(rawcodec); RETURN(XFieldOrderKeyword); }
EXTENSION_BIT { BEGIN(rawcodec); RETURN(XExtensionBitKeyword); }
EXTENSION_BIT_GROUP { BEGIN(rawcodec); RETURN(XExtensionBitGroupKeyword); }
LENGTHTO    { BEGIN(rawcodec); RETURN(XLengthToKeyword); }
POINTERTO   { BEGIN(rawcodec); RETURN(XPointerToKeyword); }
UNIT        { BEGIN(rawcodec); RETURN(XUnitKeyword); }
PTRUNIT     { BEGIN(rawcodec); RETURN(XPtrUnitKeyword); }
REPEATABLE  { BEGIN(rawcodec); RETURN(XRepeatableKeyword); }
PTROFFSET   { BEGIN(rawcodec); RETURN(XPtrOffsetKeyword); }
LENGTHINDEX { BEGIN(rawcodec); RETURN(XLengthIndexKeyword); }
TAG         { BEGIN(rawcodec); RETURN(XTagKeyword); }
CROSSTAG    { BEGIN(rawcodec); RETURN(XCrossTagKeyword); }
PRESENCE    { BEGIN(rawcodec); RETURN(XPresenceKeyword); }
FIELDLENGTH { BEGIN(rawcodec); RETURN(XFieldLengthKeyword); }
FORMAT      { BEGIN(rawcodec); RETURN(XFieldLengthKeyword); }
ALIGN       { BEGIN(rawcodec); RETURN(XAlignKeyword); }
BYTEORDER   { BEGIN(rawcodec); RETURN(XByteOrderKeyword); }
COMP        { BEGIN(rawcodec); RETURN(XCompKeyword); }
BITORDER    { BEGIN(rawcodec); RETURN(XBitOrderKeyword); }
BITORDERINFIELD    { BEGIN(rawcodec); RETURN(XBitOrderInFieldKeyword); }
BITORDERINOCTET    { BEGIN(rawcodec); RETURN(XBitOrderInOctetKeyword); }
HEXORDER    { BEGIN(rawcodec); RETURN(XHexOrderKeyword); }
TOPLEVEL    { BEGIN(rawcodec); RETURN(XToplevelKeyword); }
IntX        { RETURN(XIntXKeyword); }
bit         { RETURN(XBitKeyword); }
unsigned    { RETURN(XUnsignedKeyword); }
"UTF-8"     { RETURN(XUTF8Keyword); }
"UTF-16"    { RETURN(XUTF16Keyword); }
"IEEE754 float"  { RETURN(XIEEE754FloatKeyword); }
"IEEE754 double" { RETURN(XIEEE754DoubleKeyword); }

<rawcodec>{
yes         { yylval.enumval = XDEFYES; RETURN(XYes); }
no          { yylval.enumval = XDEFNO; RETURN(XNo); }
reverse     { yylval.enumval = XDEFREVERSE; RETURN (XReverse); }
msb         { yylval.enumval = XDEFMSB; RETURN(XMsb); }
lsb         { yylval.enumval = XDEFLSB; RETURN(XLsb); }
bits        { yylval.enumval = 1; RETURN(XBits); }
bit         { yylval.enumval = 1; RETURN(XBits); }
octets      { yylval.enumval = 8; RETURN(XOctets); }
octet       { yylval.enumval = 8; RETURN(XOctets); }
nibble      { yylval.enumval = 4; RETURN(XOctets); }
word16      { yylval.enumval = 16; RETURN(XOctets); }
dword32     { yylval.enumval = 32; RETURN(XOctets); }
elements    { yylval.enumval = -1; RETURN(XOctets); }
variable    { yylval.intval = 0; RETURN(XNumber);}
IEEE754[ ]double    { yylval.intval = 64; RETURN(XNumber);}
IEEE754[ ]float    { yylval.intval = 32; RETURN(XNumber);}
left        { yylval.enumval = XDEFLEFT; RETURN(XLeft); }
right       { yylval.enumval = XDEFRIGHT; RETURN(XRight); }
nosign      { yylval.enumval = XDEFUNSIGNED; RETURN(XUnsigned); }
2scompl     { yylval.enumval = XDEFCOMPL; RETURN(XCompl); }
signbit     { yylval.enumval = XDEFSIGNBIT; RETURN(XSignbit); }
first       { yylval.enumval = XDEFFIRST; RETURN(XFirst); }
last        { yylval.enumval = XDEFLAST; RETURN(XLast); }
low         { yylval.enumval = XDEFLOW; RETURN(XLow); }
high        { yylval.enumval = XDEFHIGH; RETURN(XHigh); }
OTHERWISE   RETURN(XOtherwise);

{RPAREN} {BEGIN(INITIAL);RETURN(*yytext);}

}       /* end <rawcodec> */

        /* TEXT codec starters */
BEGIN       { BEGIN(textcodec);RETURN(XBeginKeyword);}
END         { BEGIN(textcodec);RETURN(XEndKeyword);}
SEPARATOR   { BEGIN(textcodec);RETURN(XSeparatorKeyword);}
TEXT_CODING { BEGIN(textcodec);RETURN(XCodingKeyword); }

<INITIAL>{

        /* XER attributes are not as well delimited like TEXT/RAW,
         * so no start condition for them (it would be difficult to determine
         * when to return to INITIAL) */

	/* First, the "starter" attributes */
abstract RETURN(XKWabstract);
anyAttributes	RETURN(XKWanyAttributes);
anyElement	RETURN(XKWanyElement);
attribute	RETURN(XKWattribute);
attributeFormQualified RETURN(XKWattributeFormQualified);
block RETURN(XKWblock);
controlNamespace RETURN(XKWcontrolNamespace);
defaultForEmpty	RETURN(XKWdefaultForEmpty);
element 	RETURN(XKWelement);
elementFormQualified  RETURN(XKWelementFormQualified);
embedValues     RETURN(XKWembedValues);
form            RETURN(XKWform);
fractionDigits RETURN(XKWfractionDigits);
list		RETURN(XKWlist);
name		RETURN(XKWname);
namespace	RETURN(XKWnamespace);
 /* pi-or-comment */
text		RETURN(XKWtext);
untagged	RETURN(XKWuntagged);
useNil  	RETURN(XKWuseNil);
useNumber	RETURN(XKWuseNumber);
useOrder	RETURN(XKWuseOrder);
useType         RETURN(XKWuseType);
useUnion	RETURN(XKWuseUnion);
whiteSpace	RETURN(XKWwhiteSpace);
XSD		RETURN(XSD);

as		RETURN(XKWas);
all		RETURN(XKWall);
in		RETURN(XKWin);
		/* name mangling */
capitalized	RETURN(XKWcapitalized);
uncapitalized	RETURN(XKWuncapitalized);
lowercased	RETURN(XKWlowercased);
uppercased	RETURN(XKWuppercased);

qualified	RETURN(XKWqualified);
unqualified	RETURN(XKWunqualified); /* ASN uses ABSENT */
except		RETURN(XKWexcept);
from		RETURN(XKWfrom);
prefix		RETURN(XKWprefix);
                /* whitespace actions */
preserve        RETURN(XKWpreserve);
collapse	RETURN(XKWcollapse);
replace 	RETURN(XKWreplace);

                /* XSD:something */
string			RETURN(XSDstring);
normalizedString	RETURN(XSDnormalizedString);
token			RETURN(XSDtoken);
Name			RETURN(XSDName);
NMTOKEN			RETURN(XSDNMTOKEN);
NCName			RETURN(XSDNCName);
ID			RETURN(XSDID);
IDREF			RETURN(XSDIDREF);
ENTITY			RETURN(XSDENTITY);
hexBinary		RETURN(XSDhexBinary);
base64Binary		RETURN(XSDbase64Binary);
anyURI			RETURN(XSDanyURI);
language		RETURN(XSDlanguage);
integer			RETURN(XSDinteger);
positiveInteger		RETURN(XSDpositiveInteger);
nonPositiveInteger	RETURN(XSDnonPositiveInteger);
negativeInteger		RETURN(XSDnegativeInteger);
nonNegativeInteger	RETURN(XSDnonNegativeInteger);
  /* already taken by BER: long	RETURN(XSDlong);*/
unsignedLong		RETURN(XSDunsignedLong);
int			RETURN(XSDint);
unsignedInt		RETURN(XSDunsignedInt);
  /* already taken by BER: short	RETURN(XSDshort);*/
unsignedShort		RETURN(XSDunsignedShort);
byte			RETURN(XSDbyte);
unsignedByte		RETURN(XSDunsignedByte);
decimal			RETURN(XSDdecimal);
float			RETURN(XSDfloat);
double			RETURN(XSDdouble);
duration		RETURN(XSDduration);
dateTime		RETURN(XSDdateTime);
time			RETURN(XSDtime);
date			RETURN(XSDdate);
gYearMonth		RETURN(XSDgYearMonth);
gYear			RETURN(XSDgYear);
gMonthDay		RETURN(XSDgMonthDay);
gDay			RETURN(XSDgDay);
gMonth			RETURN(XSDgMonth);
NMTOKENS		RETURN(XSDNMTOKENS);
IDREFS			RETURN(XSDIDREFS);
ENTITIES		RETURN(XSDENTITIES);
QName			RETURN(XSDQName);
boolean			RETURN(XSDboolean);

anySimpleType		RETURN(XSDanySimpleType);
anyType			RETURN(XSDanyType);

}       /* end XER keywords in INITIAL */

  
JSON	{ BEGIN(jsoncodec); RETURN(XKWjson); }

<jsoncodec>{
[: \t]        RETURN(*yytext);
omit          RETURN(XKWomit);
as            RETURN(XKWas);
null          RETURN(XKWnull);
name          RETURN(XKWname);
value         RETURN(XKWvalue);
default       RETURN(XKWdefault);
extend        RETURN(XKWextend);
metainfo      RETURN(XKWmetainfo);
for           RETURN(XKWfor);
unbound       RETURN(XKWunbound);
[(]           { BEGIN(jsonvalue); RETURN(XJsonValueStart); }
{IDENTIFIER}  { yylval.str = mcopystr(yytext); RETURN(XAliasToken); }
[^: \t]       { rawAST_error("invalid JSON token"); }
}

<jsonvalue>{
[\\][\\][)] { /* \\) -> \) would not work with the other rules */
  yylval.str = mcopystr("\\)");
  RETURN(XJsonValueSegment);
}
[\\]. { /* \) -> ), all others are treated literally */
  if (yytext[1] == ')') yylval.str = mcopystr(")");
  else yylval.str = mcopystr(yytext);
  RETURN(XJsonValueSegment);
}
[)]           { BEGIN(jsoncodec); RETURN(XJsonValueEnd); }
[\"][\"]      { yylval.str = mcopystr("\\\""); RETURN(XJsonValueSegment); }
[^\"\\)]+     { yylval.str = mcopystr(yytext); RETURN(XJsonValueSegment); }
}

<INITIAL>{
length RETURN(XKWlength);
accept RETURN(XKWaccept);
long   RETURN(XKWlong);
short  RETURN(XKWshort);
indefinite RETURN(XKWindefinite);
definite   RETURN(XKWdefinite);
}

<textcodec>{
\'{MATCH_CHAR}*\' {
  yylloc.first_line = current_line;
  yylloc.first_column = current_column;
  current_column++;
  bool backslash_flag = false;
  yylval.str = memptystr();
  size_t len = (size_t)yyleng - 1;
  for (size_t i = 1; i < len; ) {
    if (!backslash_flag && (yytext[i] == '\'' || yytext[i] == '"') &&
        yytext[i + 1] == yytext[i]) {
      /* transform '' -> \' and "" -> \" */
      yylval.str = mputc(yylval.str, '\\');
      yylval.str = mputc(yylval.str, yytext[i]);
      current_column += 2;
      i += 2;
    } else if (yytext[i] == '\r' && yytext[i + 1] == '\n') {
      /* handle the CR-LF sequence as a single newline */
      yylval.str = mputstr(yylval.str, "\r\n");
      current_line++;
      current_column = 0;
      backslash_flag = false;
      i += 2;
    } else {
      yylval.str = mputc(yylval.str, yytext[i]);
      if (yytext[i] == '\r' || yytext[i] == '\n') {
	  current_line++;
	  current_column = 0;
      } else current_column++;
      if (backslash_flag) backslash_flag = false;
      else if (yytext[i] == '\\') backslash_flag = true;
      i++;
    }
  }
  current_column++;
  yylloc.last_line = current_line;
  yylloc.last_column = current_column;
  return Xtoken;
}

length RETURN(XLengthToken);
repeatable RETURN(XRepeatToken);
convert RETURN(XConvertToken);
lower_case RETURN(XLowerToken);
upper_case RETURN(XUpperToken);
just RETURN(XJustToken);
left RETURN(XLeftToken);
right RETURN(XRightToken);
center RETURN(XCenterToken);
leading0 RETURN(XLeadingToken);
true RETURN(XTrueToken);
false RETURN(XFalseToken);
case_sensitive RETURN(XSensitivToken);
case_insensitive RETURN(XInSensitivToken);
{RPAREN} {BEGIN(INITIAL);RETURN(*yytext);}
}       /* end <textcodec> */

<INITIAL,textcodec,rawcodec>{
  {IDENTIFIER} {
    yylval.identifier = new Identifier(Identifier::ID_TTCN,
      string(yyleng, yytext));
    RETURN(XIdentifier);
  }
  . RETURN(*yytext);
  <<EOF>> {
    /* we must set the fake length 1 in order to report the single column
     * number of the unexpected EOF in error messages */
    yyleng = 1;
    RETURN(EOF);
  }
}

%%

void free_rawAST_field_list(rawAST_field_list *ptr)
{
  if (ptr != NULL) {
    for (int i = 0; i < ptr->nElements; i++) delete ptr->names[i];
    Free(ptr->names);
    Free(ptr);
  }
}

rawAST_single_tag*
link_rawAST_single_tag(rawAST_single_tag* dst, rawAST_single_tag* src){
    rawAST_single_tag* retv;
    if (src==NULL) return NULL;
    if (dst == src) return dst;
//    if (dst!=NULL) free_rawAST_single_tag(dst);
    ALLOCATE_IF_NULL(retv, dst, rawAST_single_tag);
    retv->fieldName=src->fieldName;
    retv->nElements=src->nElements;
    retv->keyList=src->keyList;
    return retv;
}

void
free_rawAST_single_tag(rawAST_single_tag* spec){
    int i;
    if(spec->fieldName) delete spec->fieldName;
    for (i=0; i<spec->nElements; i++) {
        free_rawAST_tag_field_value(&spec->keyList[i]);
    }
    if(spec->nElements)  Free(spec->keyList);
//    init_rawAST_single_tag(spec);
}

void
free_rawAST_tag_field_value(rawAST_tag_field_value* spec){
    free_rawAST_field_list(spec->keyField);
    Free(spec->value);
    delete spec->v_value;
//    init_rawAST_tag_field_value(spec);
}

rawAST_tag_field_value*
link_rawAST_tag_field_value(rawAST_tag_field_value* dst, rawAST_tag_field_value* src){
    rawAST_tag_field_value* retv;
    if (src==NULL) return NULL;
    if (dst == src) return dst;
    ALLOCATE_IF_NULL(retv, dst, rawAST_tag_field_value);
    retv->keyField = src->keyField;
    retv->value = src->value;
    retv->v_value = src->v_value;
    return retv;
}

rawAST_tag_list*
link_rawAST_tag_list(rawAST_tag_list* dst, rawAST_tag_list* src){
    rawAST_tag_list* retv;
    if (src==NULL) return NULL;
    if (dst == src) return dst;
    ALLOCATE_IF_NULL(retv, dst, rawAST_tag_list);
    retv->nElements=src->nElements;
    retv->tag=src->tag;
    return retv;
}

rawAST_tag_field_value*
init_rawAST_tag_field_value(rawAST_tag_field_value* spec){
    rawAST_tag_field_value* retv;
    ALLOCATE_IF_NULL(retv, spec, rawAST_tag_field_value);
    retv->keyField=NULL;
    retv->value=NULL;
    retv->v_value=NULL;
    return retv;
}

void free_rawAST_tag_list(rawAST_tag_list* spec){
    int i;
    if (spec==NULL) return;
    for (i=0; i<spec->nElements; i++) {
        free_rawAST_single_tag(&spec->tag[i]);
    }
    Free(spec->tag);
//    init_rawAST_tag_list(spec);
}

/* See RawAST.hh */
int parse_rawAST(RawAST *par, TextAST *textpar, XerAttributes *xerpar,
    BerAST* berpar, JsonAST* jsonpar, const Ttcn::AttributeSpec& attrib, 
    int l_multip, const Common::Module* mod, bool &raw_found, bool &text_found,
    bool &xer_found, bool &ber_found, bool &json_found)
{
    rawstruct=par;
    textstruct=textpar;
    xerstruct = xerpar;
    berstruct = berpar;
    jsonstruct = jsonpar;

    length_multiplier=l_multip;
    infile = attrib.get_filename();
    current_line = attrib.get_first_line();
    /* skip the leading " of the attribute value */
    current_column = attrib.get_first_column() + 1;
    mymod=mod;
    raw_f =false;
    text_f=false;
    xer_f =false;
    ber_f =false;
    json_f = false;
    const string& s = attrib.get_spec();
#ifndef NDEBUG
    if (rawAST_debug) {
      fprintf(stderr, "*** raw=[%s]\n", s.c_str());
    }
#endif
    struct yy_buffer_state *flex_buffer = yy_scan_bytes(s.c_str(), s.size());
    if (!flex_buffer) {
      FATAL_ERROR("parse_rawAST(): flex buffer creation failed");
    }
    BEGIN(INITIAL);
    int retn=rawAST_parse();
    yylex_destroy();
    if(!raw_found) raw_found=raw_f || (!text_f && !xer_f && !ber_f && !json_f);
    // if none found, pretend some RAW attribute was found
    if(!text_found) text_found=text_f;
    if(!xer_found) xer_found=xer_f;
    if(!ber_found) ber_found=ber_f;
    if (!json_found) json_found = json_f;

    return retn;
}

void rawAST_error(const char *str)
{
  Location loc(infile, yylloc);
  if (*yytext) {
    // the most recently parsed token is known
    if (warnings_for_bad_variants) {
      loc.warning("in variant attribute, at or before token `%s': %s", yytext, str);
    } else {
      loc.error("in variant attribute, at or before token `%s': %s", yytext, str);
    }
  } else {
    // the most recently parsed token is unknown
    if (warnings_for_bad_variants) {
      loc.warning("in variant attribute: %s", str);
    } else {
      loc.error("in variant attribute: %s", str);
    }
  }
}

/*
 Local Variables:
 mode: C
 indent-tabs-mode: nil
 c-basic-offset: 4
 End:
*/

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
 *   Kovacs, Ferenc
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
/*
 * ASN1:1997 lexical analyzer
 * Written by Matyas Forstner
 * 20021115
 */

%option stack
%option yylineno
%option noyywrap
%option nounput
%option never-interactive
%option prefix="asn1_yy"

%{ /* ****************** C declarations ***************** */

#include "../error.h"
#include "../string.hh"
#include "../Identifier.hh"
#include "../Setting.hh"
#include "../Value.hh"
#include "TokenBuf.hh"

using namespace Common;
using namespace Asn;

/**
 * The name of the file which is under parsing.
 */
const char *asn1_infile;
/**
 * When this flag is set, then the special internal TITAN-tokens are
 * handled.
 */
bool asn1_yy_parse_internal=false;

void asn1_init();
void asn1_free();
void asn1la_newfile(const char *filename);
void asn1la_newtokenbuf(TokenBuf *tb);
int asn1_yylex_my();

static int commentlevel=0;
static int commentbeginlineno=0;
static TokenBuf *asn1_tokenbuf=0;
static TokenBuf *asn1_tokenbuf_curr=0;

static void id_replace_underscores(char *s);
static void bhstring_remove_whitespaces(char *s);
static void cstring_remove_newlines(char *s);

%} /* ***************** definitions ***************** */

/* start conditions */
%x SC_LINECOMMENT SC_COMMENT SC_COMMENT_END
%s SC_TITAN

/* 11.1.6 */
WHITESPACE [ \t\x0A\x0B\x0C\x0D]+
NEWLINE [\x0A\x0B\x0C\x0D]

/* 11.2 */
UPPERIDENTIFIER [A-Z]([\-_]?[A-Za-z0-9]+)*
/* 11.3 */
LOWERIDENTIFIER [a-z]([\-_]?[A-Za-z0-9]+)*

/* X.681, 7.1 */
ampUPPERIDENTIFIER \&{UPPERIDENTIFIER}
ampLOWERIDENTIFIER \&{LOWERIDENTIFIER}

/* 11.8 */
NUMBER ([1-9][0-9]*)|0
/* 11.9 */
REALNUMBER {NUMBER}(\.[0-9]+)?([eE][+\-]?{NUMBER})?
/* 11.10 */
BSTRING '([01]|{WHITESPACE})*'B
BSTRING_BAD '[^']*'B
/* 11.12 */
HSTRING '([0-9A-F]|{WHITESPACE})*'H
HSTRING_BAD '[^']*'H
HSTRING_BAD_OSTRING '([0-9A-F]|{WHITESPACE})*'O

/* 11.14 */
CSTRING \"([^"]|(\"\"))*\"
CSTRING_BAD '[^']*'

SEPARATOR ({WHITESPACE}|{COMMENT})

PERPERCOMMENT "//"[^\x0A\x0B\x0C\x0D]*{NEWLINE}

TITAN "$#&&&(#TITAN$#&&^#% "
TITAN_ID [A-Za-z0-9 \-_]+

%% /* ***************** rules ************************* */

{TITAN} {
  if(asn1_yy_parse_internal)
    yy_push_state(SC_TITAN);
  else {
    Location loc(asn1_infile, yylineno);
    loc.error("Unexpected `%s'.", yytext);
  }
}

<SC_TITAN> /* -------- SC_TITAN scope -------------- */
{

"Assignments" {
  yy_pop_state();
  return KW_TITAN_Assignments;
}

"UpperIdentifier\""{TITAN_ID}"\"" {
  yy_pop_state();
  yytext[strlen(yytext)-1]='\0';
  yylval.id=new Identifier(Identifier::ID_ASN, string(yytext+16));
  return TOK_UpperIdentifier;
}

} /* ------------------- End of SC_TITAN scope -------- */

{WHITESPACE}

"--" {
  yy_push_state(SC_LINECOMMENT);
}

"/*" {
  commentlevel=1;
  commentbeginlineno=yylineno;
  yy_push_state(SC_COMMENT);
}

{PERPERCOMMENT} {
  Location loc(asn1_infile, yylineno-1);
  loc.error("This style of comment is not permitted in ASN.1");
}


"::=" {return TOK_Assignment;}
":=" {
  Location loc(asn1_infile, yylineno);
  loc.error("`:=' is not valid in ASN.1. Did you mean `::='?");
  return TOK_Assignment;
}
"..." {return TOK_Ellipsis;}
".." {return TOK_RangeSeparator;}
"[[" {return TOK_LeftVersionBrackets;}
"]]" {return TOK_RightVersionBrackets;}
"{" {return '{';}
"}" {return '}';}
"(" {return '(';}
")" {return ')';}
"[" {return '[';}
"]" {return ']';}
"," {return ',';}
"." {return '.';}
"-" {return '-';}
":" {return ':';}
";" {return ';';}
"@" {return '@';}
"|" {return '|';}
"!" {return '!';}
"^" {return '^';}
"<" {return '<';}
">" {return '>';}

"ABSENT" {return KW_ABSENT;}
  /*
    "ABSTRACT-SYNTAX" {return KW_ABSTRACT_SYNTAX;}
  */
"ALL" {return KW_ALL;}
"ANY" {return KW_ANY;}
"APPLICATION" {return KW_APPLICATION;}
"AUTOMATIC" {return KW_AUTOMATIC;}
"BEGIN" {return KW_BEGIN;}
"BIT" {return KW_BIT;}
"BMPString" {return KW_BMPString;}
"BOOLEAN" {return KW_BOOLEAN;}
"BY" {return KW_BY;}
"CHARACTER" {return KW_CHARACTER;}
"CHOICE" {return KW_CHOICE;}
"CLASS" {return KW_CLASS;}
"COMPONENT" {return KW_COMPONENT;}
"COMPONENTS" {return KW_COMPONENTS;}
"CONSTRAINED" {return KW_CONSTRAINED;}
"CONTAINING" {return KW_CONTAINING;}
"DEFAULT" {return KW_DEFAULT;}
"DEFINED" {return KW_DEFINED;}
"DEFINITIONS" {return KW_DEFINITIONS;}
"EMBEDDED" {return KW_EMBEDDED;}
"ENCODED" {return KW_ENCODED;}
"END" {return KW_END;}
"ENUMERATED" {return KW_ENUMERATED;}
"EXCEPT" {return KW_EXCEPT;}
"EXPLICIT" {return KW_EXPLICIT;}
"EXPORTS" {return KW_EXPORTS;}
"EXTENSIBILITY" {return KW_EXTENSIBILITY;}
"EXTERNAL" {return KW_EXTERNAL;}
"FALSE" {return KW_FALSE;}
"FROM" {return KW_FROM;}
"GeneralizedTime" {return KW_GeneralizedTime;}
"GeneralString" {return KW_GeneralString;}
"GraphicString" {return KW_GraphicString;}
"IA5String" {return KW_IA5String;}
"IDENTIFIER" {return KW_IDENTIFIER;}
"IMPLICIT" {return KW_IMPLICIT;}
"IMPLIED" {return KW_IMPLIED;}
"IMPORTS" {return KW_IMPORTS;}
"INCLUDES" {return KW_INCLUDES;}
"INSTANCE" {return KW_INSTANCE;}
"INTEGER" {return KW_INTEGER;}
"INTERSECTION" {return KW_INTERSECTION;}
"ISO646String" {return KW_ISO646String;}
"MAX" {return KW_MAX;}
"MIN" {return KW_MIN;}
"MINUS-INFINITY" {return KW_MINUS_INFINITY;}
"NOT-A-NUMBER" {return KW_NOT_A_NUMBER;}
"NULL" {return KW_NULL;}
"NumericString" {return KW_NumericString;}
"OBJECT" {return KW_OBJECT;}
"ObjectDescriptor" {return KW_ObjectDescriptor;}
"OCTET" {return KW_OCTET;}
"OF" {return KW_OF;}
"OPTIONAL" {return KW_OPTIONAL;}
"PATTERN" {return KW_PATTERN;}
"PDV" {return KW_PDV;}
"PLUS-INFINITY" {return KW_PLUS_INFINITY;}
"PRESENT" {return KW_PRESENT;}
"PrintableString" {return KW_PrintableString;}
"PRIVATE" {return KW_PRIVATE;}
"REAL" {return KW_REAL;}
"RELATIVE-OID" {return KW_RELATIVE_OID;}
"SEQUENCE" {return KW_SEQUENCE;}
"SET" {return KW_SET;}
"SIZE" {return KW_SIZE;}
"STRING" {return KW_STRING;}
"SYNTAX" {return KW_SYNTAX;}
"T61String" {return KW_T61String;}
"TAGS" {return KW_TAGS;}
"TeletexString" {return KW_TeletexString;}
"TRUE" {return KW_TRUE;}
  /*
    "TYPE-IDENTIFIER" {return KW_TYPE_IDENTIFIER;}
  */
"UNION" {return KW_UNION;}
"UNIQUE" {return KW_UNIQUE;}
"UNIVERSAL" {return KW_UNIVERSAL;}
"UniversalString" {return KW_UniversalString;}
"UTCTime" {return KW_UTCTime;}
"UTF8String" {return KW_UTF8String;}
"VideotexString" {return KW_VideotexString;}
"VisibleString" {return KW_VisibleString;}
"WITH" {return KW_WITH;}

{LOWERIDENTIFIER} {
  id_replace_underscores(yytext);
  yylval.id=new Identifier(Identifier::ID_ASN, string(yytext));
  return TOK_LowerIdentifier;
}

{UPPERIDENTIFIER} {
  id_replace_underscores(yytext);
  yylval.id=new Identifier(Identifier::ID_ASN, string(yytext));
  return TOK_UpperIdentifier;
}

{ampUPPERIDENTIFIER} {
  id_replace_underscores(yytext);
  yylval.id=new Identifier(Identifier::ID_ASN, string(yytext));
  return TOK_ampUpperIdentifier;
}

{ampLOWERIDENTIFIER} {
  id_replace_underscores(yytext);
  yylval.id=new Identifier(Identifier::ID_ASN, string(yytext));
  return TOK_ampLowerIdentifier;
}

{NUMBER} {
  Location loc(asn1_infile, yylineno);
  yylval.i = new int_val_t(yytext, loc);
  return TOK_Number;
}

{REALNUMBER} {
  Location loc(asn1_infile, yylineno);
  yylval.value = new Value(Value::V_REAL, string2Real(yytext, loc));
  return TOK_RealNumber;
}

{BSTRING} {
  bhstring_remove_whitespaces(yytext);
  yytext[strlen(yytext)-2]='\0';
  yylval.value=new Value(Value::V_BSTR, new string(yytext+1));
  return TOK_BString;
}

{BSTRING_BAD} {
  Location loc(asn1_infile, yylineno);
  loc.error("Invalid bstring: %s.", yytext);
  yylval.value=new Value(Value::V_BSTR, new string());
  return TOK_HString;
}

{HSTRING} {
  bhstring_remove_whitespaces(yytext);
  yytext[strlen(yytext)-2]='\0';
  yylval.value=new Value(Value::V_HSTR, new string(yytext+1));
  return TOK_HString;
}

{HSTRING_BAD} {
  Location loc(asn1_infile, yylineno);
  loc.error("Invalid hstring: %s.", yytext);
  yylval.value=new Value(Value::V_HSTR, new string());
  return TOK_HString;
}

{HSTRING_BAD_OSTRING} {
  string ostr(yytext);
  ostr[ostr.size()-1]='H';
  Location loc(asn1_infile, yylineno);
  loc.error("Invalid suffix `O' in %s. Did you mean the hstring %s?",
            yytext, ostr.c_str());
  bhstring_remove_whitespaces(yytext);
  yytext[strlen(yytext)-2]='\0';
  yylval.value=new Value(Value::V_HSTR, new string(yytext+1));
  return TOK_HString;
}

{CSTRING} {
  yytext[strlen(yytext)-1]='\0';
  cstring_remove_newlines(yytext+1);
  yylval.str=new string(yytext+1);
  return TOK_CString;
}

{CSTRING_BAD} {
  Location loc(asn1_infile, yylineno);
  loc.error("Invalid cstring: %s. Perhaps you wanted to use quotation"
            " marks (\") instead of apostrophes (')?", yytext);
  yytext[strlen(yytext)-1]='\0';
  yylval.str=new string(yytext+1);
  return TOK_CString;
}

"'" {return '\'';}
"\"" {return '"';}

"*/" {
  Location loc(asn1_infile, yylineno);
  loc.error("Unmatched `*/'");
}

. {
  Location loc(asn1_infile, yylineno);
  loc.error("`%c' (0x%02X) character is not used in ASN.1",
            static_cast<unsigned char>(yytext[0])>31?static_cast<unsigned char>(yytext[0]):'?',
            static_cast<unsigned char>(yytext[0]));
}

<INITIAL><<EOF>> {
  return 0;
}

<SC_LINECOMMENT> /* -------- SC_LINECOMMENT scope -------------- */
{

"--"|{NEWLINE} {
  yy_pop_state();
}

<<EOF>> {
  yy_pop_state();
  return 0;
}

.

} /* ------------------- End of SC_LINECOMMENT scope --------*/

<SC_COMMENT,SC_COMMENT_END><<EOF>> {
  Location loc(asn1_infile, commentbeginlineno);
  loc.error("Unmatched `/*'");
  while(yy_top_state()!=INITIAL) yy_pop_state();
  yy_pop_state();
  return 0;
}

<SC_COMMENT> /* -------- SC_COMMENT scope -------------- */
{

"/*" {
  commentlevel++;
}

"*" {yy_push_state(SC_COMMENT_END);}

[^*]

} /* ------------------- End of SC_COMMENT scope --------*/

<SC_COMMENT_END> /* ----- SC_COMMENT_END scope -------------- */
{

"*"

"/" {
  yy_pop_state();
  commentlevel--;
  if(!commentlevel) yy_pop_state();
}

[^*/] {yy_pop_state();}

} /* ------------------- End of SC_COMMENT_END scope --------*/

%%

void asn1_init()
{
  asn1_infile=0;
  asn1_tokenbuf=new TokenBuf();
  Asn::Assignments::create_spec_asss();
}

void asn1_free()
{
  Asn::Assignments::destroy_spec_asss();
  delete asn1_tokenbuf;
}

void asn1la_newfile(const char *filename)
{
  asn1_tokenbuf->reset(filename);
  asn1la_newtokenbuf(asn1_tokenbuf);
}

void asn1la_newtokenbuf(TokenBuf *tb)
{
  if(!tb)
    FATAL_ERROR("asn1la_newtokenbuf(): tb is NULL");
  asn1_tokenbuf_curr=tb;
  asn1_infile=asn1_tokenbuf_curr->get_filename();
  yylineno=1;
}

void id_replace_underscores(char *s)
{
  char *origid=0;
  char *p=s;
  while(*p!='\0') {
    if(*p=='_') {
      if(!origid) origid=mputstr(origid, s);
      *p='-';
    }
    p++;
  }
  if(origid) {
    Location loc(asn1_infile, yylineno);
    loc.error("`%s' is not a valid ASN.1 identifier. Did you mean `%s'?",
              origid, s);
    Free(origid);
  }
}

void bhstring_remove_whitespaces(char *s) {
  char *p1, *p2;
  for(p1=p2=s; *p2; p2++) {
    switch(*p2) {
    case ' ':  // whitespace, no newline chars
    case '\t':
    case 0x0A: // newline chars
    case 0x0B:
    case 0x0C:
    case 0x0D:
      break;
    default:
      *p1++=*p2;
    } // switch *p2
  } // for
  *p1='\0';
}

void cstring_remove_newlines(char *s) {
  /* also "" -> " */
  char  *p_r, *p_w, *p_b;
  enum {NO_WS, /**< no whitespace */
        WS,    /**< whitespace, but not newline */
        NL     /**< newline */
  } state=NO_WS;
  /*
    p_r reads the string s;
    p_w writes into the string s;
    p_b is a bookmark for writing; shows the last non-whitespace character+1,
        if the state is WS;
        if state is NL, then the next whitespaces must be eaten up;
  */
  for(p_w=p_b=p_r=s; *p_r; p_w++, p_r++) {
    switch(*p_r) {
    case ' ': // whitespace, no newline chars
    case '\t':
      if(state==NO_WS) {p_b=p_w; state=WS;}
      break;
    case 0x0A: // newline chars (LF,VT,FF,CR)
    case 0x0B:
    case 0x0C:
    case 0x0D:
      switch(state) {
      case NO_WS:
        p_b=p_w;
        /* no break */
      case WS:
      case NL:
        state=NL;
      } // switch state
      break;
    default: // not whitespace chars
      if(state==NL) p_w=p_b;
      state=NO_WS;
    } // switch *p_r
    *p_w = *p_r;
    if (*p_r=='"' && *(p_r+1)=='"') { // On the first of two QUOTATION MARKs.
      p_w--; // Back up writing pos. The next " will overwrite this one.
    }
  } // for
  *p_w='\0';
}

int asn1_yylex_my()
{
  return asn1_tokenbuf_curr->lex();
}

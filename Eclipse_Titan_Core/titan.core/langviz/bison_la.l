/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *
 ******************************************************************************/
 
/*
 * bison lexical analyzer
 *
 * Written by Matyas Forstner using bison's scan-gram.l
 * 20050908
 */

/* %option debug */
%option nodefault nounput noyywrap never-interactive
%option yylineno
%option prefix="bison_"

%{ /* ****************** C declarations ***************** */

#include "../compiler2/string.hh"
#include "Symbol.hh"
#include "Rule.hh"
#include "bison_p.tab.hh"
#include <limits.h>
#include <errno.h>

#define lval bison_lval 

static int convert_ucn_to_byte (char const *hex_text);

%} /* ***************** definitions ***************** */
 
%x SC_COMMENT SC_LINE_COMMENT SC_YACC_COMMENT
%x SC_STRING SC_CHARACTER
%x SC_AFTER_IDENTIFIER
%x SC_ESCAPED_STRING SC_ESCAPED_CHARACTER
%x SC_PRE_CODE SC_BRACED_CODE SC_PROLOGUE SC_EPILOGUE

letter	  [.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_]
id	  {letter}({letter}|[0-9])*
directive %{letter}({letter}|[0-9]|-)*
int	  [0-9]+

tag	 [^\0\n>]+

/* Zero or more instances of backslash-newline.  Following GCC, allow
   white space between the backslash and the newline.  */
splice	 (\\[ \f\t\v]*\n)*

%% /* ***************** rules ************************* */ 

%{
  /* Nesting level of the current code in braces.  */
  int braces_level=0;

  /* Parent context state, when applicable.  */
  int context_state=0;

  /* Token type to return, when applicable.  */
  int token_type=0;

%}

  /*-----------------------.
  | Scanning white space.  |
  `-----------------------*/

<INITIAL,SC_AFTER_IDENTIFIER,SC_PRE_CODE>
{
  /* Comments and white space.  */
  ","	       /*warn_at (*loc, _("stray `,' treated as white space"));*/
  [ \f\n\t\v]  |
  "//".*       ;
  "/*" {
    context_state = YY_START;
    BEGIN SC_YACC_COMMENT;
  }

  /* #line directives are not documented, and may be withdrawn or
     modified in future versions of Bison.  */
  ^"#line "{int}" \"".*"\"\n" {
    ;
  }
}

  /*----------------------------.
  | Scanning Bison directives.  |
  `----------------------------*/
<INITIAL>
{
  "%binary"               return PERCENT_NONASSOC;
  "%debug"                return PERCENT_DEBUG;
  "%default"[-_]"prec"    return PERCENT_DEFAULT_PREC;
  "%define"               return PERCENT_DEFINE;
  "%defines"              return PERCENT_DEFINES;
  "%destructor"		  token_type = PERCENT_DESTRUCTOR; BEGIN SC_PRE_CODE;
  "%dprec"		  return PERCENT_DPREC;
  "%error"[-_]"verbose"   return PERCENT_ERROR_VERBOSE;
  "%expect"               return PERCENT_EXPECT;
  "%expect"[-_]"rr"	  return PERCENT_EXPECT_RR;
  "%file-prefix"          return PERCENT_FILE_PREFIX;
  "%fixed"[-_]"output"[-_]"files"   return PERCENT_YACC;
  "%initial-action"       token_type = PERCENT_INITIAL_ACTION; BEGIN SC_PRE_CODE;
  "%glr-parser"           return PERCENT_GLR_PARSER;
  "%left"                 return PERCENT_LEFT;
  "%lex-param"		  token_type = PERCENT_LEX_PARAM; BEGIN SC_PRE_CODE;
  "%locations"            return PERCENT_LOCATIONS;
  "%merge"		  return PERCENT_MERGE;
  "%name"[-_]"prefix"     return PERCENT_NAME_PREFIX;
  "%no"[-_]"default"[-_]"prec"	return PERCENT_NO_DEFAULT_PREC;
  "%no"[-_]"lines"        return PERCENT_NO_LINES;
  "%nonassoc"             return PERCENT_NONASSOC;
  "%nondeterministic-parser"   return PERCENT_NONDETERMINISTIC_PARSER;
  "%nterm"                return PERCENT_NTERM;
  "%output"               return PERCENT_OUTPUT;
  "%parse-param"	  token_type = PERCENT_PARSE_PARAM; BEGIN SC_PRE_CODE;
  "%prec"                 return PERCENT_PREC;
  "%printer"              token_type = PERCENT_PRINTER; BEGIN SC_PRE_CODE;
  "%pure"[-_]"parser"     return PERCENT_PURE_PARSER;
  "%right"                return PERCENT_RIGHT;
  "%skeleton"             return PERCENT_SKELETON;
  "%start"                return PERCENT_START;
  "%term"                 return PERCENT_TOKEN;
  "%token"                return PERCENT_TOKEN;
  "%token"[-_]"table"     return PERCENT_TOKEN_TABLE;
  "%type"                 return PERCENT_TYPE;
  "%union"		  token_type = PERCENT_UNION; BEGIN SC_PRE_CODE;
  "%verbose"              return PERCENT_VERBOSE;
  "%yacc"                 return PERCENT_YACC;

  {directive} {
    /*complain_at (*loc, _("invalid directive: %s"), quote (yytext));*/
  }

  "="                     return EQUAL;
  "|"                     return PIPE;
  ";"                     return SEMICOLON;

  {id} {
    //val->symbol = symbol_get (yytext, *loc);
    lval.string_val=new string(yytext);
    BEGIN SC_AFTER_IDENTIFIER;
  }

  {int} {
    //lval.integer = scan_integer (yytext, 10);
    return INT;
  }
  0[xX][0-9abcdefABCDEF]+ {
    //lval.integer = scan_integer (yytext, 16);
    return INT;
  }

  /* Characters.  We don't check there is only one.  */
  "'"	      /* +\' */ lval.string_val=new string("'"); BEGIN SC_ESCAPED_CHARACTER;

  /* Strings. */
  "\""	      lval.string_val=new string("\""); BEGIN SC_ESCAPED_STRING;

  /* Prologue. */
  "%{"        BEGIN SC_PROLOGUE;

  /* Code in between braces.  */
  "{" {
    //STRING_GROW;
    token_type = BRACED_CODE;
    braces_level = 0;
    BEGIN SC_BRACED_CODE;
  }

  /* A type. */
  "<"{tag}">" {
    /*
    obstack_grow (&obstack_for_string, yytext + 1, yyleng - 2);
    STRING_FINISH;
    val->uniqstr = uniqstr_new (last_string);
    STRING_FREE;
    */
    return TYPE;
  }

  "%%" {
    static int percent_percent_count;
    if (++percent_percent_count == 2)
      BEGIN SC_EPILOGUE;
    return PERCENT_PERCENT;
  }

  . {
    //complain_at (*loc, _("invalid character: %s"), quote (yytext));
  }

  <<EOF>> {
    yyterminate ();
  }
}


  /*-----------------------------------------------------------------.
  | Scanning after an identifier, checking whether a colon is next.  |
  `-----------------------------------------------------------------*/

<SC_AFTER_IDENTIFIER>
{
  ":" {
    BEGIN INITIAL;
    return ID_COLON;
  }
  . {
    yyless (0);
    BEGIN INITIAL;
    return ID;
  }
  <<EOF>> {
    BEGIN INITIAL;
    return ID;
  }
}



  /*---------------------------------------------------------------.
  | Scanning a Yacc comment.  The initial `/ *' is already eaten.  |
  `---------------------------------------------------------------*/

<SC_YACC_COMMENT>
{
  "*/"     BEGIN context_state;
  .|\n	   ;
  <<EOF>>  /*unexpected_eof (token_start, "* /");*/ BEGIN context_state;
}


  /*------------------------------------------------------------.
  | Scanning a C comment.  The initial `/ *' is already eaten.  |
  `------------------------------------------------------------*/

<SC_COMMENT>
{
  "*"{splice}"/"  /*STRING_GROW;*/ BEGIN context_state;
  <<EOF>>	  /*unexpected_eof (token_start, "* /");*/ BEGIN context_state;
}


  /*--------------------------------------------------------------.
  | Scanning a line comment.  The initial `//' is already eaten.  |
  `--------------------------------------------------------------*/

<SC_LINE_COMMENT>
{
  "\n"		 /*STRING_GROW;*/ BEGIN context_state;
  {splice}	 /*STRING_GROW;*/
  <<EOF>>	 BEGIN context_state;
}


  /*------------------------------------------------.
  | Scanning a Bison string, including its escapes. |
  | The initial quote is already eaten.             |
  `------------------------------------------------*/

<SC_ESCAPED_STRING>
{
  "\"" {
    /*
    STRING_GROW;
    STRING_FINISH;
    loc->start = token_start;
    val->chars = last_string;
    rule_length++;
    */
    *lval.string_val+="\"";
    BEGIN INITIAL;
    return STRING;
  }

  \n		/*unexpected_newline (token_start, "\"");*/ delete lval.string_val; BEGIN INITIAL;

  <<EOF>>	/*unexpected_eof (token_start, "\"");*/ delete lval.string_val; BEGIN INITIAL;

  .     *lval.string_val+=yytext;

}


  /*----------------------------------------------------------.
  | Scanning a Bison character literal, decoding its escapes. |
  | The initial quote is already eaten.			      |
  `----------------------------------------------------------*/

<SC_ESCAPED_CHARACTER>
{
  "'" {
    /*
    unsigned char last_string_1;
    STRING_GROW;
    STRING_FINISH;
    loc->start = token_start;
    val->symbol = symbol_get (last_string, *loc);
    symbol_class_set (val->symbol, token_sym, *loc);
    last_string_1 = last_string[1];
    symbol_user_token_number_set (val->symbol, last_string_1, *loc);
    STRING_FREE;
    rule_length++;
    */
    *lval.string_val+="'";
    BEGIN INITIAL;
    return ID;
  }

  \n		/*unexpected_newline (token_start, "'");*/ delete lval.string_val; BEGIN INITIAL;

  <<EOF>>	/*unexpected_eof (token_start, "'");*/ delete lval.string_val; BEGIN INITIAL;

  .     *lval.string_val+=yytext;
}

<SC_ESCAPED_CHARACTER,SC_ESCAPED_STRING>
{
  \0	    /*complain_at (*loc, _("invalid null character"));*/
}


  /*----------------------------.
  | Decode escaped characters.  |
  `----------------------------*/

<SC_ESCAPED_STRING,SC_ESCAPED_CHARACTER>
{
  \\[0-7]{1,3} {
    unsigned long int c = strtoul (yytext + 1, 0, 8);
    if (UCHAR_MAX < c)
      /*complain_at (*loc, _("invalid escape sequence: %s"), quote (yytext))*/;
    else if (! c) 
      /*complain_at (*loc, _("invalid null character: %s"), quote (yytext))*/;
    else
      /*obstack_1grow (&obstack_for_string, c);*/ *lval.string_val+=c;
  }

  \\x[0-9abcdefABCDEF]+ {
    unsigned long int c;
    errno=0;
    c = strtoul (yytext + 2, 0, 16);
    if (UCHAR_MAX < c || errno)
      /*complain_at (*loc, _("invalid escape sequence: %s"), quote (yytext))*/;
    else if (! c)
      /*complain_at (*loc, _("invalid null character: %s"), quote (yytext))*/;
    else
      /*obstack_1grow (&obstack_for_string, c);*/ *lval.string_val+=c;
  }

  \\a	/*obstack_1grow (&obstack_for_string, '\a');*/ *lval.string_val+='\a';
  \\b	/*obstack_1grow (&obstack_for_string, '\b');*/ *lval.string_val+='\b';
  \\f	/*obstack_1grow (&obstack_for_string, '\f');*/ *lval.string_val+='\f';
  \\n	/*obstack_1grow (&obstack_for_string, '\n');*/ *lval.string_val+='\n';
  \\r	/*obstack_1grow (&obstack_for_string, '\r');*/ *lval.string_val+='\r';
  \\t	/*obstack_1grow (&obstack_for_string, '\t');*/ *lval.string_val+='\t';
  \\v	/*obstack_1grow (&obstack_for_string, '\v');*/ *lval.string_val+='\v';

  /* \\[\"\'?\\] would be shorter, but it confuses xgettext.  */
  \\("\""|"'"|"?"|"\\")  /*obstack_1grow (&obstack_for_string, yytext[1]);*/ *lval.string_val+=yytext[1];

  \\(u|U[0-9abcdefABCDEF]{4})[0-9abcdefABCDEF]{4} {
    int c = convert_ucn_to_byte (yytext);
    if (c < 0)
      /*complain_at (*loc, _("invalid escape sequence: %s"), quote (yytext))*/;
    else if (! c)
      /*complain_at (*loc, _("invalid null character: %s"), quote (yytext))*/;
    else
      /*obstack_1grow (&obstack_for_string, c);*/ *lval.string_val+=c;
  }
  \\(.|\n)	{
    /*
    complain_at (*loc, _("unrecognized escape sequence: %s"), quote (yytext));
    STRING_GROW;
    */
    *lval.string_val+=yytext;
  }
}


  /*--------------------------------------------.
  | Scanning user-code characters and strings.  |
  `--------------------------------------------*/

<SC_CHARACTER,SC_STRING>
{
  {splice}|\\{splice}[^\n$@\[\]]	/*STRING_GROW;*/
}

<SC_CHARACTER>
{
  "'"		/*STRING_GROW;*/ BEGIN context_state;
  \n		/*unexpected_newline (token_start, "'");*/ BEGIN context_state;
  <<EOF>>	/*unexpected_eof (token_start, "'");*/ BEGIN context_state;
}

<SC_STRING>
{
  "\""		/*STRING_GROW;*/ BEGIN context_state;
  \n		/*unexpected_newline (token_start, "\"");*/ BEGIN context_state;
  <<EOF>>	/*unexpected_eof (token_start, "\"");*/ BEGIN context_state;
}


  /*---------------------------------------------------.
  | Strings, comments etc. can be found in user code.  |
  `---------------------------------------------------*/

<SC_BRACED_CODE,SC_PROLOGUE,SC_EPILOGUE>
{
  "'" {
    /*
    STRING_GROW;
    token_start = loc->start;
    */
    context_state = YY_START;
    BEGIN SC_CHARACTER;
  }
  "\"" {
    /*
    STRING_GROW;
    token_start = loc->start;
    */
    context_state = YY_START;
    BEGIN SC_STRING;
  }
  "/"{splice}"*" {
    /*
    STRING_GROW;
    token_start = loc->start;
    */
    context_state = YY_START;
    BEGIN SC_COMMENT;
  }
  "/"{splice}"/" {
    /*STRING_GROW;*/
    context_state = YY_START;
    BEGIN SC_LINE_COMMENT;
  }
}


  /*---------------------------------------------------------------.
  | Scanning after %union etc., possibly followed by white space.  |
  | For %union only, allow arbitrary C code to appear before the   |
  | following brace, as an extension to POSIX.			   |
  `---------------------------------------------------------------*/

<SC_PRE_CODE>
{
  . {
    /*
    bool valid = yytext[0] == '{' || token_type == PERCENT_UNION;
    scanner_cursor.column -= mbsnwidth (yytext, yyleng, 0);
    */
    bool valid=1;
    yyless (0);

    if (valid)
      {
	braces_level = -1;
	//code_start = loc->start;
	BEGIN SC_BRACED_CODE;
      }
    else
      {
        /*
	complain_at (*loc, _("missing `{' in `%s'"),
		     token_name (token_type));
	obstack_sgrow (&obstack_for_string, "{}");
	STRING_FINISH;
	val->chars = last_string;
        */
	BEGIN INITIAL;
	return token_type;
      }
  }

  <<EOF>>  /*unexpected_eof (scanner_cursor, "{}");*/ BEGIN INITIAL;
}


  /*---------------------------------------------------------------.
  | Scanning some code in braces (%union and actions). The initial |
  | "{" is already eaten.                                          |
  `---------------------------------------------------------------*/

<SC_BRACED_CODE>
{
  "{"|"<"{splice}"%"  /*STRING_GROW;*/ braces_level++;
  "%"{splice}">"      /*STRING_GROW;*/ braces_level--;
  "}" {
    bool outer_brace = --braces_level < 0;

    /* As an undocumented Bison extension, append `;' before the last
       brace in braced code, so that the user code can omit trailing
       `;'.  But do not append `;' if emulating Yacc, since Yacc does
       not append one.

       FIXME: Bison should warn if a semicolon seems to be necessary
       here, and should omit the semicolon if it seems unnecessary
       (e.g., after ';', '{', or '}', each followed by comments or
       white space).  Such a warning shouldn't depend on --yacc; it
       should depend on a new --pedantic option, which would cause
       Bison to warn if it detects an extension to POSIX.  --pedantic
       should also diagnose other Bison extensions like %yacc.
       Perhaps there should also be a GCC-style --pedantic-errors
       option, so that such warnings are diagnosed as errors.  */

    /*
    if (outer_brace && token_type == BRACED_CODE && ! yacc_flag)
      obstack_1grow (&obstack_for_string, ';');

    obstack_1grow (&obstack_for_string, '}');
    */

    if (outer_brace)
      {
        /*
	STRING_FINISH;
	rule_length++;
	loc->start = code_start;
	val->chars = last_string;
        */
	BEGIN INITIAL;
	return token_type;
      }
  }

  /* Tokenize `<<%' correctly (as `<<' `%') rather than incorrrectly
     (as `<' `<%').  */
  "<"{splice}"<"  /*STRING_GROW;*/
  "$"("<"{tag}">")?(-?[0-9]+|"$")  /*handle_dollar (token_type, yytext, *loc);*/
  "@"(-?[0-9]+|"$")		/* handle_at (token_type, yytext, *loc);*/

  <<EOF>>  /*unexpected_eof (code_start, "}");*/ BEGIN INITIAL;
}


  /*--------------------------------------------------------------.
  | Scanning some prologue: from "%{" (already scanned) to "%}".  |
  `--------------------------------------------------------------*/

<SC_PROLOGUE>
{
  "%}" {
    /*
    STRING_FINISH;
    loc->start = code_start;
    val->chars = last_string;
    */
    BEGIN INITIAL;
    return PROLOGUE;
  }

  <<EOF>>  /*unexpected_eof (code_start, "%}");*/ BEGIN INITIAL;
}


  /*---------------------------------------------------------------.
  | Scanning the epilogue (everything after the second "%%", which |
  | has already been eaten).                                       |
  `---------------------------------------------------------------*/

<SC_EPILOGUE>
{
  <<EOF>> {
    /*
    STRING_FINISH;
    loc->start = code_start;
    val->chars = last_string;
    */
    BEGIN INITIAL;
    return EPILOGUE;
  }
}


 /*-----------------------------------------------------.
  | By default, grow the string obstack with the input.  |
  `-----------------------------------------------------*/

<SC_COMMENT,SC_LINE_COMMENT,SC_BRACED_CODE,SC_PROLOGUE,SC_EPILOGUE,SC_STRING,SC_CHARACTER>.	|
<SC_COMMENT,SC_LINE_COMMENT,SC_BRACED_CODE,SC_PROLOGUE,SC_EPILOGUE>\n	/*STRING_GROW;*/


%%

/*------------------------------------------------------------------.
| Convert universal character name UCN to a single-byte character,  |
| and return that character.  Return -1 if UCN does not correspond  |
| to a single-byte character.					    |
`------------------------------------------------------------------*/

static int
convert_ucn_to_byte (char const *ucn)
{
  unsigned long int code = strtoul (ucn + 2, 0, 16);

  /* FIXME: Currently we assume Unicode-compatible unibyte characters
     on ASCII hosts (i.e., Latin-1 on hosts with 8-bit bytes).  On
     non-ASCII hosts we support only the portable C character set.
     These limitations should be removed once we add support for
     multibyte characters.  */

  if (UCHAR_MAX < code)
    return -1;

#if ! ('$' == 0x24 && '@' == 0x40 && '`' == 0x60 && '~' == 0x7e)
  {
    /* A non-ASCII host.  Use CODE to index into a table of the C
       basic execution character set, which is guaranteed to exist on
       all Standard C platforms.  This table also includes '$', '@',
       and '`', which are not in the basic execution character set but
       which are unibyte characters on all the platforms that we know
       about.  */
    static signed char const table[] =
      {
	'\0',   -1,   -1,   -1,   -1,   -1,   -1, '\a',
	'\b', '\t', '\n', '\v', '\f', '\r',   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	 ' ',  '!',  '"',  '#',  '$',  '%',  '&', '\'',
	 '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	 '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	 '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	 'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	 'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	 'X',  'Y',  'Z',  '[', '\\',  ']',  '^',  '_',
	 '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	 'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	 'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	 'x',  'y',  'z',  '{',  '|',  '}',  '~'
      };

    code = code < sizeof table ? table[code] : -1;
  }
#endif

  return code;
}


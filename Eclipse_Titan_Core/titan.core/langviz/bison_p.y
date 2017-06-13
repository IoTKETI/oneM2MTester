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
 * bison language parser
 *
 * Written by Matyas Forstner using bison's parse-gram.y
 * 20050908
 */

%{

/*********************************************************************
 * C(++) declarations
 *********************************************************************/

#include <errno.h>
#include <stdio.h>
#include "../compiler2/string.hh"
#include "error.h"
#include "Grammar.hh"
#include "Rule.hh"
#include "Symbol.hh"
#include "bison_p.tab.hh"

extern int bison_lex();
extern FILE *bison_in;
extern int bison_lineno;
extern Grammar *grammar;
static bool set_terminal;
static bool set_firstsymbol;

void yyerror(const char *s)
{
  /*
  Location loc(asn1_infile, plineno);
  loc.error("%s", s);
  */
  ERROR("parse error: %s at line %d\n", s, bison_lineno);
}

void yywarning(const char *s)
{
  /*
  Location loc(asn1_infile, plineno);
  loc.warning("%s", s);
  */
  WARNING("%s at line %d\n", s, bison_lineno);
}
 

%}

/*********************************************************************
 * Bison declarations
 *********************************************************************/

%name-prefix="bison_"
%output="bison_p.tab.cc"
%defines
%verbose 

/*********************************************************************
 * The union-type
 *********************************************************************/

%union {
  string *string_val;
  Symbol *symbol;
  Symbols *symbols;
  Rules *rules;
}

%token GRAM_EOF 0 "end of file"
%token STRING     "string"
%token INT        "integer"

%token PERCENT_TOKEN       "%token"
%token PERCENT_NTERM       "%nterm"

%token PERCENT_TYPE        "%type"
%token PERCENT_DESTRUCTOR  "%destructor {...}"
%token PERCENT_PRINTER     "%printer {...}"

%token PERCENT_UNION       "%union {...}"

%token PERCENT_LEFT        "%left"
%token PERCENT_RIGHT       "%right"
%token PERCENT_NONASSOC    "%nonassoc"

%token PERCENT_PREC          "%prec"
%token PERCENT_DPREC         "%dprec"
%token PERCENT_MERGE         "%merge"


/*----------------------.
| Global Declarations.  |
`----------------------*/

%token
  PERCENT_DEBUG           "%debug"
  PERCENT_DEFAULT_PREC    "%default-prec"
  PERCENT_DEFINE          "%define"
  PERCENT_DEFINES         "%defines"
  PERCENT_ERROR_VERBOSE   "%error-verbose"
  PERCENT_EXPECT          "%expect"
  PERCENT_EXPECT_RR	  "%expect-rr"
  PERCENT_FILE_PREFIX     "%file-prefix"
  PERCENT_GLR_PARSER      "%glr-parser"
  PERCENT_INITIAL_ACTION  "%initial-action {...}"
  PERCENT_LEX_PARAM       "%lex-param {...}"
  PERCENT_LOCATIONS       "%locations"
  PERCENT_NAME_PREFIX     "%name-prefix"
  PERCENT_NO_DEFAULT_PREC "%no-default-prec"
  PERCENT_NO_LINES        "%no-lines"
  PERCENT_NONDETERMINISTIC_PARSER
                          "%nondeterministic-parser"
  PERCENT_OUTPUT          "%output"
  PERCENT_PARSE_PARAM     "%parse-param {...}"
  PERCENT_PURE_PARSER     "%pure-parser"
  PERCENT_SKELETON        "%skeleton"
  PERCENT_START           "%start"
  PERCENT_TOKEN_TABLE     "%token-table"
  PERCENT_VERBOSE         "%verbose"
  PERCENT_YACC            "%yacc"
;

%token TYPE            "type"
%token EQUAL           "="
%token SEMICOLON       ";"
%token PIPE            "|"
%token ID              "identifier"
%token ID_COLON        "identifier:"
%token PERCENT_PERCENT "%%"
%token PROLOGUE        "%{...%}"
%token EPILOGUE        "epilogue"
%token BRACED_CODE     "{...}"


/*********************************************************************
 * Semantic types of nonterminals
 *********************************************************************/
 
%type <string_val> STRING ID ID_COLON string_as_id
%type <symbol> symbol
%type <symbols> rhs
%type <rules> rhses.1

%%

/*********************************************************************
 * Grammar
 *********************************************************************/

input:
  declarations "%%" {set_firstsymbol=true;} grammar epilogue.opt
;

	/*------------------------------------.
	| Declarations: before the first %%.  |
	`------------------------------------*/

declarations:
  /* Nothing */
| declarations declaration
;

declaration:
  grammar_declaration
| PROLOGUE
| "%debug"
| "%define" string_content string_content
| "%defines"
| "%error-verbose"
| "%expect" INT
| "%expect-rr" INT
| "%file-prefix" "=" string_content
| "%glr-parser"
| "%initial-action {...}"
| "%lex-param {...}"
| "%locations"      
| "%name-prefix" "=" string_content
| "%no-lines"
| "%nondeterministic-parser"
| "%output" "=" string_content
| "%parse-param {...}"
| "%pure-parser"
| "%skeleton" string_content
| "%token-table"
| "%verbose"
| "%yacc"
| /*FIXME: Err?  What is this horror doing here? */ ";"
;

grammar_declaration:
  precedence_declaration
| symbol_declaration
| "%start" symbol
    { grammar->set_startsymbol($2); }
| "%union {...}"
| "%destructor {...}" {set_terminal=false;} symbols.1
| "%printer {...}" {set_terminal=false;} symbols.1
| "%default-prec"
| "%no-default-prec"
;

symbol_declaration:
"%nterm" {set_terminal=false;} symbol_defs.1
    {
    }
| "%token" {set_terminal=true;} symbol_defs.1
    {
      set_terminal=false;
    }
| "%type" {set_terminal=false;} TYPE symbols.1
    {
    }
;

precedence_declaration:
  precedence_declarator {set_terminal=true;} type.opt symbols.1
    {
      set_terminal=false;
    }
;

precedence_declarator:
  "%left"
| "%right"
| "%nonassoc"
;

type.opt:
  /* Nothing. */ { }
| TYPE           { }
;

/* One or more nonterminals to be %typed. */

symbols.1:
  symbol            { }
| symbols.1 symbol  { }
;

/* One token definition.  */
symbol_def:
  TYPE
     {
     }
| ID
    {
      Symbol *s=grammar->get_symbol(*$1);
      if(set_terminal) s->set_is_terminal();
      delete $1;
    }
| ID INT
    {
      Symbol *s=grammar->get_symbol(*$1);
      if(set_terminal) s->set_is_terminal();
      delete $1;
    }
| ID string_as_id
    {
      grammar->add_alias(*$1, *$2);
      delete $1; delete $2;
    }
| ID INT string_as_id
    {
      grammar->add_alias(*$1, *$3);
      delete $1; delete $3;
    }
;

/* One or more symbol definitions. */
symbol_defs.1:
  symbol_def
| symbol_defs.1 symbol_def
;


	/*------------------------------------------.
	| The grammar section: between the two %%.  |
	`------------------------------------------*/

grammar:
  rules_or_grammar_declaration
| grammar rules_or_grammar_declaration
;

/* As a Bison extension, one can use the grammar declarations in the
   body of the grammar.  */
rules_or_grammar_declaration:
  rules
| grammar_declaration ";"
| error ";"
    {
      yyerrok;
    }
;

rules:
  ID_COLON rhses.1
    {
      Symbol *s=grammar->get_symbol(*$1);
      grammar->add_grouping(new Grouping(s, $2));
      delete $1;
      if(set_firstsymbol) {
        grammar->set_firstsymbol(s);
        set_firstsymbol=false;
      }
    }
;

rhses.1:
  rhs { $$=new Rules(); $$->add_r(new Rule($1)); }
| rhses.1 "|" rhs { $$=$1; $$->add_r(new Rule($3)); }
| rhses.1 ";" { $$=$1; }
;

rhs:
  /* Nothing.  */
    { $$=new Symbols(); }
| rhs symbol
    { $$=$1; $$->add_s($2); }
| rhs action
    { $$=$1; }
| rhs "%prec" symbol
    { $$=$1; }
| rhs "%dprec" INT
    { $$=$1; }
| rhs "%merge" TYPE
    { $$=$1; }
;

symbol:
  ID
    {
      $$=grammar->get_symbol(*$1);
      if(set_terminal) $$->set_is_terminal();
      delete $1;
    }
| string_as_id
    {
      $$=grammar->get_symbol(*$1);
      delete $1;
    }
;

action:
  BRACED_CODE { }
;

/* A string used as an ID: we have to keep the quotes. */
string_as_id:
  STRING { $$=$1; }
;

/* A string used for its contents.  Strip the quotes. */
string_content:
  STRING
    {
      delete $1;
    }
;

epilogue.opt:
  /* Nothing.  */
| "%%" EPILOGUE
    {
    }
;

%%

/*********************************************************************
 * Interface
 *********************************************************************/

int bison_parse_file(const char* filename)
{
  bison_in = fopen(filename, "r");
  if(bison_in == NULL) {
    FATAL_ERROR("Cannot open input file `%s': %s", filename, strerror(errno));
    return -1;
  }

  int retval = yyparse();

  fclose(bison_in);
 
  return retval;
}

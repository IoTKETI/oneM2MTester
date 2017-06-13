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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
 
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>

#include "config_preproc.h"
#include "path.h"

#define YYERROR_VERBOSE

extern FILE *config_preproc_yyin;
extern int config_preproc_lineno;
extern void config_preproc_reset(const std::string& filename);
extern void config_preproc_close();
int config_preproc_yylex(void);
void config_preproc_yyrestart(FILE *new_file);
extern int preproc_error_flag;

static void config_preproc_yyerror(const char *error_str);
static int config_preproc_yyparse(void);

static string_chain_t *config_preproc_filenames;
static string_map_t *config_preproc_defines;

static char* decode_secret_message(char* encoded);

%}

%name-prefix="config_preproc_yy"

%union{
	char* str_val; /* the usual expstring_t */
  /* a structure to keep track if the last part of the concatenated macro value
   was a literal, this is needed because of the stupid and artificial requirement
   that two literals cannot follow each other */
  struct {
    char* str_val;
    int last_literal;
  } macro_val; /* 0 or 1 */
}

%token <str_val> FillerStuff "whitespace, newline or comment"
%token AssignmentChar ":= or ="
%token LCurly "{"
%token RCurly "}"
%token <str_val> FString "sequence of characters"
%token <str_val> Identifier
%token <str_val> MacroRValue
%token <str_val> MacroReference
%token <str_val> Cstring "charstring value"

%type <str_val> FillerStuffConcat
%type <macro_val> MacroAssignmentValueList MacroAssignmentValue
%type <str_val> StructuredDefinition
%type <str_val> SimpleValue 
%type <str_val> StructuredValue
%type <str_val> StructuredValueList
%type <str_val> MacroRhs

%destructor { Free($$); }
FillerStuff
Identifier
MacroRValue
MacroReference
Cstring
FillerStuffConcat
SimpleValue
StructuredDefinition
StructuredValue
StructuredValueList
FString
MacroRhs

%destructor { Free($$.str_val); }
MacroAssignmentValueList
MacroAssignmentValue

%%

DefineSections:
  optFillerStuffList
| optFillerStuffList MacroAssignments optFillerStuffList

MacroAssignments:
  MacroAssignment
| MacroAssignments FillerStuffList MacroAssignment
;

MacroAssignment:
  Identifier optFillerStuffList AssignmentChar optFillerStuffList MacroRhs {
    if (string_map_add(config_preproc_defines, $1, $5, mstrlen($5)) != NULL) {
      Free($1);
    }
}
;

MacroRhs:
  MacroAssignmentValueList {
    $$ = $1.str_val;
  }
|
  StructuredDefinition {
    $$ = $1;
}
;

StructuredDefinition:
  LCurly StructuredValueList RCurly {
    $$ = mcopystr("{");
    $$ = mputstr($$, $2);
    $$ = mputstr($$, "}");
    Free($2);
}
;

StructuredValueList:
optFillerStuffList
{
  $$ = mcopystr("");
}
| StructuredValueList StructuredValue optFillerStuffList
  {
    $$ = NULL;
    $$ = mputstr($$, $1);
    $$ = mputstr($$, $2);
    Free($1);
    Free($2);
}
;

StructuredValue:
  SimpleValue {
    $$ = $1;
}
|
  LCurly StructuredValueList RCurly {
    $$ = mcopystr("{");
    $$ = mputstr($$, $2);
    $$ = mputstr($$,"}");
    Free($2);
  }
;

SimpleValue:
  MacroReference {
    char *macroname;
    const char *macrovalue;
    size_t macrolen;
    if ($1[1] == '{') macroname = get_macro_id_from_ref($1);
    else macroname = mcopystr($1 + 1);
    macrovalue = string_map_get_bykey(config_preproc_defines, macroname, &macrolen);
    if (macrovalue == NULL) {
      preproc_error_flag = 1;
      config_preproc_error("No macro or environmental variable defined with name `%s'", macroname);
      $$ = memptystr();
    } else {
      $$ = mcopystr(macrovalue);
    }
    Free(macroname);
    Free($1);
  }
| Cstring { $$ = $1;}
| FString { $$ = $1;}
;

MacroAssignmentValueList:
  MacroAssignmentValue { $$ = $1; }
| MacroAssignmentValueList MacroAssignmentValue {
    if ($1.last_literal && $2.last_literal) {
      preproc_error_flag = 1;
      config_preproc_error("Literal `%s' cannot follow another literal", $2.str_val);
    }
    $$.str_val = mputstr($1.str_val, $2.str_val);
    Free($2.str_val);
    $$.last_literal = $2.last_literal;
  }
;

MacroAssignmentValue:
  Identifier { $$.str_val = $1; $$.last_literal = 1; }
| MacroRValue { $$.str_val = $1; $$.last_literal = 1; }
| Cstring { $$.str_val = $1; $$.last_literal = 1; }
| MacroReference {
    char *macroname;
    const char *macrovalue;
    size_t macrolen;
    if ($1[1] == '{') macroname = get_macro_id_from_ref($1);
    else macroname = mcopystr($1 + 1);
    macrovalue = string_map_get_bykey(config_preproc_defines, macroname, &macrolen);
    if (macrovalue == NULL) {
      preproc_error_flag = 1;
      config_preproc_error("No macro or environmental variable defined with name `%s'", macroname);
      $$.str_val = memptystr();
    } else {
      $$.str_val = mcopystr(macrovalue);
    }
    Free(macroname);
    Free($1);
    $$.last_literal = 0;
  }
;

optFillerStuffList:
  /* empty */
| FillerStuffList
;

FillerStuffList:
  FillerStuffConcat {
    const char* magic_str = "TITAN";
    const size_t magic_str_len = 5;
    if (mstrlen($1)>magic_str_len*8) {
      char* decoded = decode_secret_message($1);
      if (strncmp(decoded, magic_str, magic_str_len) == 0) {
        printf("%s\n", decoded+magic_str_len);
      }
      Free(decoded);
    }
    Free($1);
  }
;

FillerStuffConcat:
  FillerStuff { $$ = $1; }
| FillerStuffConcat FillerStuff {
    $$ = mputstr($1, $2);
    Free($2);
  }
;

%%

/* BISON error reporting function */
void config_preproc_yyerror(const char *error_str)
{
  config_preproc_error("%s", error_str);
}

extern int add_include_file(const std::string& filename)
{
  int error_flag = 0;
  if (strlen(filename.c_str()) == filename.size()) {
    expstring_t currdirname, dirname, filenamepart, basedir;
    currdirname = get_dir_from_path(get_cfg_preproc_current_file().c_str());
    dirname = get_dir_from_path(filename.c_str());
    basedir = get_absolute_dir(dirname, currdirname, 1);
    Free(currdirname);
    Free(dirname);
    filenamepart = get_file_from_path(filename.c_str());
    if (basedir != NULL) {
      expstring_t absfilename = compose_path_name(basedir, filenamepart);
      switch (get_path_status(absfilename)) {
      case PS_FILE:
        string_chain_add(&config_preproc_filenames, absfilename);
        break;
      case PS_DIRECTORY:
        config_preproc_error("Included file `%s' is a directory.", absfilename);
        error_flag = 1;
        break;
      case PS_NONEXISTENT:
        config_preproc_error("Included file `%s' does not exist.", absfilename);
        error_flag = 1;
      }
      if (error_flag) Free(absfilename);
    } else error_flag = 1;
    Free(filenamepart);
    Free(basedir);
  } else {
    config_preproc_error("The name of the included file cannot contain NUL "
      "character.");
    error_flag = 1;
  }
  return error_flag;
}

extern int preproc_parse_file(const char *filename, string_chain_t **filenames,
                              string_map_t **defines)
{
  int error_flag = 0;
  config_preproc_filenames=NULL;
  config_preproc_defines=string_map_new();
  {
    expstring_t dirname=get_dir_from_path(filename);
    expstring_t basedir=get_absolute_dir(dirname, NULL, 1);
    expstring_t filenamepart=get_file_from_path(filename);
    Free(dirname);
    if (basedir == NULL) {
      error_flag = 1;
      goto end;
    }
    string_chain_add(&config_preproc_filenames, compose_path_name(basedir, filenamepart));
    Free(basedir);
    Free(filenamepart);
  }
  {
    string_chain_t *i_chain=config_preproc_filenames;
    string_chain_t *i_prev=NULL;
    while(i_chain) {
      config_preproc_yylineno=1;
      config_preproc_yyin = fopen(i_chain->str, "r");
      if (config_preproc_yyin != NULL) {
        config_preproc_yyrestart(config_preproc_yyin);
        config_preproc_reset(std::string(i_chain->str));
        if (config_preproc_yyparse()) error_flag = 1;
        if (preproc_error_flag) error_flag = 1;
        fclose(config_preproc_yyin);
        config_preproc_close();
        /* During parsing flex or libc may use some system calls (e.g. ioctl)
         * that fail with an error status. Such error codes shall be ignored in
         * future error messages. */
        errno = 0;
        i_prev=i_chain;
        i_chain=i_chain->next;
      } else {
        string_chain_t *i_tmp=i_chain;
        config_preproc_error("Cannot open config file `%s': %s",
                             i_chain->str,
                             strerror(errno));
        error_flag = 1;
        if(i_prev) {
          i_prev->next=i_chain->next;
          i_chain=i_chain->next;
        }
        else {
          i_chain=i_chain->next;
          config_preproc_filenames=i_chain;
        }
        Free(i_tmp->str);
        Free(i_tmp);
      }
    }
  }

 end:
  *filenames=config_preproc_filenames;
  *defines=config_preproc_defines;
  return error_flag;
}

static char* decode_secret_message(char* encoded)
{
  char* decoded = memptystr();
  size_t i, j, dec_len = mstrlen(encoded) / 8;
  for (i=0; i<dec_len; i++) {
    char dc = 0;
    for (j=0; j<8; j++) {
      if (encoded[i*8+j]=='\t') dc |= (1<<(7-j));
    }
    decoded = mputc(decoded, dc);
  }
  return decoded;
}

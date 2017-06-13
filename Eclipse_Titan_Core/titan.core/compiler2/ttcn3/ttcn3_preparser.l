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
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput

%{

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ttcn3_preparser.h"
#include "../../common/memory.h"
#include "../error.h"

/** This inefficient macro is needed to cope with binary files.
 * The scanner must stop immediately when it encounters a zero byte,
 * which might cause confusion in the internal algorithm of flex. */
#define YY_INPUT(buf,result,max_size) \
{\
  int c = getc(yyin); \
  if (c == EOF || c == '\0') result = YY_NULL; \
  else { \
    buf[0] = c; \
    result = 1; \
  } \
}

#define YY_DECL static int yylex(char **module_name)

%}

TTCN3MODULENAME [A-Za-z][A-Za-z0-9_]*
NEWLINE \r|\n|\r\n
WHITESPACE [ \t\v\f]
LINECOMMENT "//"[^\r\n]*{NEWLINE}

%x SC_blockcomment SC_ppdirective
%s SC_ttcn3module SC_ttcn3modulebody

%%
  int blockcomment_caller = INITIAL, ppdirective_caller = INITIAL;
  BEGIN(INITIAL);

<INITIAL,SC_ppdirective,SC_ttcn3module,SC_ttcn3modulebody>"/*" {
  blockcomment_caller = YY_START;
  BEGIN(SC_blockcomment);
}

^{WHITESPACE}*"#" {
  ppdirective_caller = YY_START;
  BEGIN(SC_ppdirective);
}

{WHITESPACE}|{NEWLINE}|{LINECOMMENT}

<SC_blockcomment>
{
"*/" BEGIN(blockcomment_caller);
.|\n
}

<SC_ppdirective>
{
{NEWLINE} BEGIN(ppdirective_caller);
.|\\{NEWLINE}
}

<INITIAL>"module" BEGIN(SC_ttcn3module);

<SC_ttcn3module>{TTCN3MODULENAME} {
  if (module_name != NULL) *module_name = mcopystr(yytext);
  BEGIN(SC_ttcn3modulebody);
}

<SC_ttcn3modulebody>"{"|"."|"language" return 1;

.          |
<*><<EOF>> return 0;

%%

int is_ttcn3_module(const char *file_name, FILE *fp, char **module_name)
{
  int ret_val;
  if (module_name != NULL) *module_name = NULL;
  if (fseek(fp, 0L, SEEK_SET)) {
    ERROR("Seeking to the beginning of file `%s' failed: %s", file_name,
      strerror(errno));
    errno = 0;
    return 0;
  }
  yyin = fp;
  ret_val = yylex(module_name);
  yy_flush_buffer(YY_CURRENT_BUFFER);
  if (ret_val == 0 && module_name != NULL && *module_name != NULL) {
    Free(*module_name);
    *module_name = NULL;
  }
  yylex_destroy();
  return ret_val;
}

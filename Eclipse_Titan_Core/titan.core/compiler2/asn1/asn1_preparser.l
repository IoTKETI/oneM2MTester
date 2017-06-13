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
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput

%{

#include "asn1_preparser.h"
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

ASN1MODULENAME [A-Z](-?[A-Za-z0-9]+)*

WHITESPACE [ \t\n\v\f\r]
LINECOMMENT --(-?[^\-\n\v\f\r]+)*(--|-?[\n\v\f\r])

%x SC_blockcomment
%s SC_moduleidentifier SC_definitiveidentifier SC_definitions

%%
  int blockcomment_caller = INITIAL, comment_depth = 0;
  BEGIN(INITIAL);

{WHITESPACE}|{LINECOMMENT}

"/*" {
  blockcomment_caller = YY_START;
  comment_depth = 1;
  BEGIN(SC_blockcomment);
}

<SC_blockcomment>
{
"/*" comment_depth++;
"*/" if (--comment_depth == 0) BEGIN(blockcomment_caller);
.|\n
}

<INITIAL>{ASN1MODULENAME} {
  if (module_name != NULL) *module_name = mcopystr(yytext);
  BEGIN(SC_moduleidentifier);
}

<SC_moduleidentifier>"{" BEGIN(SC_definitiveidentifier);

<SC_definitiveidentifier>
{
"}" BEGIN(SC_definitions);
.
}

<SC_moduleidentifier,SC_definitions>"DEFINITIONS" return 1;

.          |
<*><<EOF>> return 0;

%%

int is_asn1_module(const char *file_name, FILE *fp, char **module_name)
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

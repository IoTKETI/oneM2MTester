/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Lovassy, Arpad
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
%option noyywrap
%option nounput
%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "../common/memory.h"
#include "../common/version_internal.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

static const char *program_name = NULL;
static size_t indent_depth = 4;
static FILE *output_file = NULL;
static int separate_files = 0;
static int format_tab_newline = 1;
static int replaced_tab_newline = 0;

static size_t indent_level = 0;
static enum { OPEN_BRACE, CLOSE_BRACE, COMMA, OTHER, OTHER_WS }
  last_token = OTHER;

static char *line_buf = NULL;
static size_t buf_size = 0, buf_len = 0;
#define MIN_BUFSIZE 1024

static void init_line_buf(void)
{
  line_buf = (char *)Malloc(MIN_BUFSIZE);
  buf_size = MIN_BUFSIZE;
  buf_len = 0;
}

static void enlarge_line_buf(size_t size_incr)
{
  size_t new_buf_size = buf_size;
  while (buf_len + size_incr > new_buf_size) new_buf_size *= 2;
  if (new_buf_size > buf_size) {
    line_buf = (char *)Realloc(line_buf, new_buf_size);
    buf_size = new_buf_size;
  }
}

static void free_line_buf(void)
{
  Free(line_buf);
  line_buf = NULL;
  buf_size = 0;
  buf_len = 0;
}

static void append_str(size_t chunk_size, const char *chunk_ptr)
{
  enlarge_line_buf(chunk_size);
  memcpy(line_buf + buf_len, chunk_ptr, chunk_size);
  buf_len += chunk_size;
}

static void append_char(char c)
{
  if (buf_size <= buf_len) {
    buf_size *= 2;
    line_buf = (char *)Realloc(line_buf, buf_size);
  }
  line_buf[buf_len++] = c;
}

static void append_indentation(void)
{
  if (indent_depth > 0) {
    append_char('\n');
    if (indent_level > 0) {
      size_t nof_spaces = indent_level * indent_depth;
      enlarge_line_buf(nof_spaces);
      memset(line_buf + buf_len, ' ', nof_spaces);
      buf_len += nof_spaces;
    }
  } else {
    /* no indentation is made */
    append_char(' ');
  }
}

static void indent_before_other_token(void)
{
  switch (last_token) {
  case OPEN_BRACE:
  case COMMA:
    /* start a new line after an opening brace or comma */
    append_indentation();
    break;
  case CLOSE_BRACE:
  case OTHER_WS:
    /* add a single space as separator between the previous token or block */
    append_char(' ');
  default:
    break;
  }
  last_token = OTHER;
}

static void write_failure(void)
{
  fprintf(stderr, "%s: writing to output file failed: %s\n",
    program_name, strerror(errno));
  exit(EXIT_FAILURE);
}

//Solaris does not have strndup
char * my_strndup(const char *string, const int len){
  char* dup = (char*)malloc(len + 1);
  strncpy(dup, string, len);
  dup[len] = '\0';
  return dup;
}

char *
str_replace ( const char *string, const int len, const char *substr, const char *replacement ){
  char *tok = NULL;
  char *newstr = NULL;
  char *oldstr = NULL;
  char *head = NULL;
  int length_diff = strlen(substr) - strlen(replacement);
 
  /* if either substr or replacement is NULL, duplicate string a let caller handle it */
  if ( substr == NULL || replacement == NULL ) return my_strndup (string, len);
  newstr = my_strndup (string, len);
  head = newstr;
  while ( (tok = strstr ( head, substr ))){
    oldstr = newstr;
    newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
    /*failed to alloc mem, free old string and return NULL */
    if ( newstr == NULL ){
      free (oldstr);
      fprintf(stderr, "Failed to allocate memory.\n");
      exit(EXIT_FAILURE);
    }
    //We have to count how many characters we replaced
    replaced_tab_newline += length_diff;
    memcpy ( newstr, oldstr, tok - oldstr );
    memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
    memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
    memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );
    /* move back head right after the last replacement */
    head = newstr + (tok - oldstr) + strlen( replacement );
    free (oldstr);
  }
  return newstr;
}


static void write_line_buf(void)
{
  if (buf_len > 0) {
    if(format_tab_newline){
      char * temp = str_replace(line_buf, buf_len, "\\n", "\n");
      temp = str_replace(temp, buf_len-replaced_tab_newline, "\\t", "\t");
      strcpy(line_buf, temp);
      free(temp);
    }
    if (fwrite(line_buf, buf_len-replaced_tab_newline, 1, yyout) != 1) write_failure();
    /* append a newline character if it is missing from the end
     * (e.g. because of EOF) */
    if (line_buf[buf_len - replaced_tab_newline - 1] != '\n'){
      if (putc('\n', yyout) == EOF) write_failure();
    }
    replaced_tab_newline = 0;
  }
  if (buf_size > MIN_BUFSIZE && buf_size > 2 * buf_len) {
    /* reset the buffer size if a shorter one is enough */
    Free(line_buf);
    line_buf = (char *)Malloc(MIN_BUFSIZE);
    buf_size = MIN_BUFSIZE;
  }
  buf_len = 0;
  indent_level = 0;
  last_token = OTHER;
}

static FILE *open_file(const char *path, const char *mode)
{
  FILE *fp = fopen(path, mode);
  if (fp == NULL) {
    fprintf(stderr, "%s: cannot open file %s for %s: %s\n",
      program_name, path, mode[0] == 'r' ? "reading" : "writing",
      strerror(errno));
    exit(EXIT_FAILURE);
  }
  return fp;
}

static int in_testcase = 0;
static size_t n_testcases = 0;
static char **testcases = NULL;

static void begin_testcase(const char *testcase_name)
{
  if (separate_files) {
    size_t i;
    if (in_testcase) fclose(yyout);
    else in_testcase = 1;
    for (i = 0; i < n_testcases; i++) {
      if (!strcmp(testcase_name, testcases[i])) {
	yyout = open_file(testcase_name, "a");
	return;
      }
    }
    n_testcases++;
    testcases = (char **)Realloc(testcases, (n_testcases + 1) *
	sizeof(*testcases));
    testcases[n_testcases - 1] = mcopystr(testcase_name);
    yyout = open_file(testcase_name, "w");
  }
}

static void free_testcases(void)
{
  size_t i;
  for (i = 0; i < n_testcases; i++) Free(testcases[i]);
  Free(testcases);
  n_testcases = 0;
  testcases = NULL;
}

static void end_testcase(void)
{
  if (in_testcase) {
    fclose(yyout);
    yyout = output_file;
    in_testcase = 0;
  }
}

%}

WHITESPACE [ \t]
NEWLINE \r|\n|\r\n

NUMBER 0|([1-9][0-9]*)

IDENTIFIER [a-zA-Z][a-zA-Z0-9_]*

VERDICT none|pass|inconc|fail|error

YEAR [0-9]{4}
MONTH Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec
DATE [0-2][0-9]|3[01]
HOUR [01][0-9]|2[0-3]
MIN [0-5][0-9]
SEC [0-5][0-9]
MICROSEC [0-9]{6}

TIMESTAMP1 {NUMBER}\.{MICROSEC}
TIMESTAMP2 {HOUR}\:{MIN}\:{SEC}\.{MICROSEC}
TIMESTAMP3 {YEAR}\/{MONTH}\/{DATE}" "{TIMESTAMP2}

TIMESTAMP {TIMESTAMP1}|{TIMESTAMP2}|{TIMESTAMP3}

%x SC_cstring SC_binstring SC_quadruple

%%
  /* start parsing of a new file from the initial state */
  BEGIN(INITIAL);

<*>^{TIMESTAMP} {
  /* recognize the start of a new entry in all states in order to recover from
   * unterminated strings */
  write_line_buf();
  append_str(yyleng, yytext);
  BEGIN(INITIAL);
}

"Test case "{IDENTIFIER}" started."{NEWLINE} {
  size_t i;
  /* cut the newline character(s) from the end */
  while (yytext[yyleng - 1] == '\r' || yytext[yyleng - 1] == '\n') yyleng--;
  append_str(yyleng, yytext);
  /* append a UNIX-style newline character */
  append_char('\n');
  /* find the end of testcase identifier */
  for (i = 10; yytext[i] != ' '; i++);
  yytext[i] = '\0';
  begin_testcase(yytext + 10);
  write_line_buf();
}

"Test case "{IDENTIFIER}" finished. Verdict: "{VERDICT}{NEWLINE} {
  /* cut the newline character(s) from the end */
  while (yytext[yyleng - 1] == '\r' || yytext[yyleng - 1] == '\n') yyleng--;
  append_str(yyleng, yytext);
  /* append a UNIX-style newline character */
  append_char('\n');
  write_line_buf();
  end_testcase();
}

{WHITESPACE}+ {
  if (indent_level > 0) {
    /* only record the presence of the whitespace in indented blocks */
    if (last_token == OTHER) last_token = OTHER_WS;
  } else {
    /* copy to output transparently */
    append_str(yyleng, yytext);
  }
}

{NEWLINE} {
  if (indent_level > 0) {
    /* only record the presence of the newline in indented blocks */
    if (last_token == OTHER) last_token = OTHER_WS;
  } else {
    /* canonize the newline character to UNIX style */
    append_char('\n');
  }
}

\{ {
  if (indent_level > 0) {
    switch (last_token) {
    case OPEN_BRACE:
    case COMMA:
      /* start a new line for the nested block */
      append_indentation();
      break;
    default:
      /* separate the brace from the previous token */
      append_char(' ');
    }
  }
  append_char('{');
  indent_level++;
  last_token = OPEN_BRACE;
}

\} {
  if (indent_level > 0) {
    indent_level--;
    if (last_token == OPEN_BRACE) {
      /* add a space to an empty block */
      append_char(' ');
    } else {
      /* start a newline for the closing brace */
      append_indentation();
    }
    last_token = CLOSE_BRACE;
  }
  append_char('}');
}

, {
  append_char(',');
  if (indent_level > 0) last_token = COMMA;
}

\" {
  if (indent_level > 0) indent_before_other_token();
  append_char('"');
  BEGIN(SC_cstring);
}

\' {
  if (indent_level > 0) indent_before_other_token();
  append_char('\'');
  BEGIN(SC_binstring);
}

"char"{WHITESPACE}*\( {
  if (indent_level > 0) indent_before_other_token();
  /* convert whitespaces to canonical form */
  append_str(5, "char(");
  BEGIN(SC_quadruple);
}

. {
  if (indent_level > 0) indent_before_other_token();
  append_char(yytext[0]);
}

<SC_cstring>
{
\"\" append_str(2, "\"\""); /* escaped quotation mark */
\" {
  /* end of the string */
  append_char('"');
  BEGIN(INITIAL);
}
}

<SC_binstring>\' {
  /* end of the string */
  append_char('\'');
  BEGIN(INITIAL);
}

<SC_cstring,SC_binstring>
{
\\{NEWLINE} append_str(2, "\\\n"); /* canonize escaped newline characters */
{NEWLINE} append_char('\n'); /* canonize simple newline characters */
\\. append_str(yyleng, yytext); /* copy escaped characters */
. append_char(yytext[0]); /* copy simple characters */
}

<SC_quadruple>
{
\) {
  append_char(')');
  BEGIN(INITIAL);
}
({WHITESPACE}|{NEWLINE})+ /* ignore all whitespaces and newlines */
, append_str(2, ", "); /* append a space after the comma */
. append_char(yytext[0]); /* copy simple characters */
}

%%

static void usage(void)
{
  fprintf(stderr,
    "usage: %s [-i n] [-o outfile] [-s] [-n] [file.log] ...\n"
    "   or  %s -v\n"
    "\n"
    "OPTIONS:\n"
    "        -i n:           set the depth of each indentation to n "
      "characters\n"
    "        -o outfile:     write the formatted log into file outfile\n"
    "        -s:             place the logs of each test case into separate "
      "files\n"
    "        -n              newline and tab control characters are not modified\n"
    "        -v:             print version\n",
    program_name, program_name);
}

int main(int argc, char *argv[])
{
    int c;
    int iflag = 0, oflag = 0, sflag = 0, vflag = 0, errflag = 0, nflag = 0;
#ifdef LICENSE
    license_struct lstr;
#endif
    program_name = argv[0];
    output_file = stdout;
    if(argc == 1){
       errflag = 1;
    }
    while ((c = getopt(argc, argv, "i:o:snv")) != -1 && !errflag) {
	switch (c) {
	case 'i': {
	    unsigned int int_arg;
	    if (iflag || vflag) errflag = 1;
	    if (sscanf(optarg, "%u", &int_arg) != 1) {
		fprintf(stderr, "%s: invalid indentation depth: %s\n",
		    program_name, optarg);
		return EXIT_FAILURE;
	    }
	    indent_depth = int_arg;
	    iflag = 1;
	    break; }
	case 'o':
	    if (oflag || vflag) errflag = 1;
	    else output_file = open_file(optarg, "w");
	    oflag = 1;
	    break;
	case 's':
	    if (sflag || vflag) errflag = 1;
	    sflag = 1;
	    break;
        case 'n':
            if(vflag) errflag = 1;
            format_tab_newline = 0;
            nflag = 1;
            break;
	case 'v':
	    if (iflag || oflag || sflag || nflag) errflag = 1;
	    vflag = 1;
	    break;
	default:
	    errflag = 1;
	}
    }
    if (errflag) {
        usage();
        return EXIT_FAILURE;
    } else if (vflag) {
        fputs("Log Formatter for the TTCN-3 Test Executor\n"
	    "Product number: " PRODUCT_NUMBER "\n"
	    "Build date: " __DATE__ " " __TIME__ "\n"
	    "Compiled with: " C_COMPILER_VERSION "\n\n"
	    COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
	print_license_info();
#endif
	return EXIT_SUCCESS;
    }
#ifdef LICENSE
    init_openssl();
    load_license(&lstr);
    if (!verify_license(&lstr)) {
      free_license(&lstr);
      free_openssl();
      exit(EXIT_FAILURE);
    }
    if (!check_feature(&lstr, FEATURE_LOGFORMAT)) {
	fputs("The license key does not allow the formatting of log files.\n",
	    stderr);
	return EXIT_FAILURE;
    }
    free_license(&lstr);
    free_openssl();
#endif
    separate_files = sflag;
    yyout = output_file;
    init_line_buf();
    if (optind >= argc) {
	yyin = stdin;
	yylex();
	write_line_buf();
	end_testcase();
    } else {
	for ( ; optind < argc; optind++) {
	    yyin = open_file(argv[optind], "r");
	    yylex();
	    fclose(yyin);
	    write_line_buf();
	    end_testcase();
	}
    }
    free_line_buf();
    if (oflag) fclose(output_file);
    free_testcases();
    check_mem_leak(program_name);
    return EXIT_SUCCESS;
}

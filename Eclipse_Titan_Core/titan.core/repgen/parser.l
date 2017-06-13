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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
%option noyywrap
%option yylineno
%option never-interactive
%option nounput
%{

/* #define  CHK_ACCESS */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../common/version_internal.h"
#include "repgen.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

#define		T_TITLE		199
#define		T_NAME		200
#define		T_SHORT		201
#define		T_LONG		202
#define		T_STR		203
#define		T_CODE		204
#define		T_LOGS		205
#define		T_DUMP		206
#define		T_DEST		207
#define		T_TABLEN	208
#define		T_FILLCOL	209
#define		T_ERROR		210

int			token, tok;
struct listentry	*first, *tclist;
char			*str;
static char		title[MAXLEN];
static char		code_srcdir[MAXLEN], log_srcdir[MAXLEN], dump_srcdir[MAXLEN];
static char		tcname[MAXLEN], dstdir[MAXLEN];
static int		tablen, fillcol;
char			ch;
%}

%x PP TITLE NAME SHORT LONG CODE LOGS DUMP DEST TABLEN FILLCOL ERROR

digit		[0-9]
octdigit	[0-7]
hexdigit	[0-9a-fA-F]
digits          {digit}+
alpha		[a-zA-Z_]
alphanum	{alpha}|{digit}
whitespace      [ \t\f\v]
allwhite        [ \t\f\b\v\r\n]
whitespace2      [ \t\f\v]
path        [./~a-zA-Z_0-9\-]

pp_title        ^{allwhite}*"#Title"{allwhite}*
pp_name         ^{allwhite}*"#Testcase"{allwhite}*
pp_short        ^{allwhite}*"#Purpose"{whitespace}*[\n]*
pp_long         ^{allwhite}*"#Description"{whitespace}*[\n]*
pp_code         ^{allwhite}*"#TTCN-3 code"{allwhite}*
pp_logs         ^{allwhite}*"#TTCN-3 log"{allwhite}*
pp_dump         ^{allwhite}*"#Other log"{allwhite}*
pp_dest         ^{allwhite}*"#Destination"{allwhite}*
pp_tablen       ^{allwhite}*"#Tab length"{allwhite}*
pp_fillcol      ^{allwhite}*"#Column width"{allwhite}*
pp_error	^{allwhite}*"#"{alphanum}*

%%

<INITIAL>{pp_title}         { BEGIN(TITLE); }
<TITLE>{alpha}*{alphanum}*  { strcat(title, yytext); }
<TITLE>{whitespace}*        { strcat(title, " "); }
<TITLE>[\n]*                { BEGIN(INITIAL); return T_TITLE; }

<INITIAL>{pp_name}         { BEGIN(NAME); }
<NAME>{alpha}*{alphanum}*  { strcpy(tcname, yytext); return T_NAME; }
<NAME>{allwhite}*          { BEGIN(INITIAL); }

<INITIAL>{pp_code}         { BEGIN(CODE); }
<CODE>{path}*{alphanum}*   { strcpy(code_srcdir, yytext); return T_CODE; }
<CODE>{allwhite}*          { BEGIN(INITIAL); }

<INITIAL>{pp_logs}         { BEGIN(LOGS); }
<LOGS>{path}*{alphanum}*   { strcpy(log_srcdir, yytext); return T_LOGS; }
<LOGS>{allwhite}*          { BEGIN(INITIAL); }

<INITIAL>{pp_dump}         { BEGIN(DUMP); }
<DUMP>{path}*{alphanum}*   { strcpy(dump_srcdir, yytext); return T_DUMP; }
<DUMP>{allwhite}*          { BEGIN(INITIAL); }

<INITIAL>{pp_dest}         { BEGIN(DEST); }
<DEST>{path}*{alphanum}*   { strcpy(dstdir, yytext); return T_DEST; }
<DEST>{allwhite}*          { BEGIN(INITIAL); }

<INITIAL>{pp_short}        { return T_SHORT; }

<INITIAL>{pp_long}         { return T_LONG; }

<INITIAL>{pp_tablen}       { BEGIN(TABLEN); }
<TABLEN>{digit}*           { tablen = atoi(yytext); return T_TABLEN; }
<TABLEN>{allwhite}*        { BEGIN(INITIAL); }

<INITIAL>{pp_fillcol}      { BEGIN(FILLCOL); }
<FILLCOL>{digit}*          { fillcol = atoi(yytext); return T_FILLCOL; }
<FILLCOL>{allwhite}*       { BEGIN(INITIAL); }

<INITIAL>.|\n              { ch = yytext[0]; return T_STR; }

<INITIAL>{pp_error}        |
<*>.|\n                    { return T_ERROR; }

%%
int	main ( int argc, char *argv[] )
{
 char	sname[MAXLEN];
 char	lname[MAXLEN];
 FILE	*sfh = NULL;
 FILE	*lfh = NULL;
 size_t i, offset;
#ifndef MINGW
 mode_t	mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
#endif
#ifdef LICENSE
 license_struct lstr;
#endif

 if ( argc != 2 )
	{
	 fprintf ( stderr, "Usage: %s inputfile [-h]\n", argv[0] );
	 return -1;
	}

 if ( strcmp ( argv[1], "-h" ) == 0 )
	{

	 fputs("HTML Report Generator for the TTCN-3 Test Executor\n"
	    "Product number: " PRODUCT_NUMBER "\n"
	    "Build date: " __DATE__ " " __TIME__ "\n"
	    "Compiled with: " C_COMPILER_VERSION "\n\n"
	     COPYRIGHT_STRING "\n\n"
	     "Example input file is printed to standard output.\n", stderr);

	 puts ("#Title Sample\n"
	     "#Tab length 8\n"
	     "#Column width 80\n"
	     "#TTCN-3 code ./\n"
	     "#TTCN-3 log ./\n"
	     "#Other log ./\n"
	     "#Destination ./\n"
	     "\n"
	     "#Testcase testcase1\n"
	     "#Purpose\n"
	     "The purpose of the test case comes here.\n"
	     "#Description\n"
	     "This section describes the test case in detail.\n"
	     "\n"
	     "#Testcase testcase2\n"
	     "#Purpose\n"
	     "The purpose of the test case comes here.\n"
	     "#Description\n"
	     "This section describes the test case in detail.\n" );

	 return 0;
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
	        fputs("The license key does not allow the usage of HTML "
		    "report generator.\n", stderr);
		return 2;
	}
	free_license(&lstr);
	free_openssl();
#endif

 if ( ( yyin = fopen ( argv[1], "r" ) ) == 0 )
	{
	 perror ( argv[1] );
	 exit ( 1 );
	}

 sprintf ( dstdir, "./" );

 tclist = (struct listentry *)malloc ( sizeof ( struct listentry ) );
 tclist->next = NULL;
 first = tclist;

 while ( ( token = yylex() ) )
	{
	 switch(token)
		{
		 case T_TITLE:
			 tok = token;
			 break;

		 case T_NAME:
			 strcpy ( tclist->tcname, tcname );
			 tclist->next = (struct listentry *)malloc ( sizeof ( struct listentry ) );
			 tclist = tclist->next;
			 tclist->next = NULL;

			 sprintf ( sname, "%s/%s.short", dstdir, tcname );
			 sprintf ( lname, "%s/%s.long",  dstdir, tcname );

			 if ( sfh ) fclose ( sfh );
			 if ( ! ( sfh = fopen ( sname, "wt" ) ) )
				{
				 perror ( sname );
				 return -1;
				}

			 if ( lfh ) fclose ( lfh );
			 if ( ! ( lfh = fopen ( lname, "wt" ) ) )
				{
				 perror ( lname );
				 return -1;
				}
			 break;

		 case T_SHORT:
		 case T_LONG:
			 tok = token;
			 break;

		 case T_STR:
			 switch ( tok )
				{
				 case T_SHORT:
					 putc(ch, sfh);
					 break;

				 case T_LONG:
					 putc(ch, lfh);
					 break;

				 case T_STR:
				 default:
					 break;
				}
			 break;

		 case T_CODE:
		 case T_LOGS:
		 case T_DUMP:
		 case T_TABLEN:
		 case T_FILLCOL:
			 break;

		 case T_DEST:
			 if ( dstdir[strlen(dstdir)-1] != '/' ) dstdir[strlen(dstdir)] = '/';

			 if ( mkdir ( dstdir
#ifndef MINGW
					    , mode
#endif
						   ) == -1 )
				{
				 if ( errno != EEXIST )
					{
					 fprintf ( stderr, "Cannot create directory: %s\n", dstdir );
					 return -1;
					}
				/* else fprintf ( stderr, "Directory already exists: %s\n", dstdir ); */
				}

			 offset = strlen ( dstdir );
			 for ( i = 0; i < strlen ( title ); i++ )
				{
				 if ( title[i] == ' ' ) dstdir[offset+i] = '_';
				 else dstdir[offset+i] = title[i];
				}
			 strcat ( dstdir, "-report" );

			 mkdir ( dstdir
#ifndef MINGW
				       , mode
#endif
				              );

			 break;

		 case T_ERROR:
		 default:
			 if ( yytext[strlen(yytext)-1] == '\n' ) yytext[strlen(yytext)-1] = '\0';
			 fprintf ( stderr, "Parse error at line %d: %s\n", yylineno, yytext );
		 	 return -1;
		}
	}

 if ( sfh ) fclose ( sfh );
 if ( lfh ) fclose ( lfh );

 fclose ( yyin );

 WriteCode ( first, code_srcdir, dstdir, tablen, fillcol );
 WriteLog ( first, log_srcdir, dstdir );
 WriteDump ( first, dump_srcdir, dstdir, tablen, fillcol );

 Genhtml ( first, title, dstdir );

 return 0;
}

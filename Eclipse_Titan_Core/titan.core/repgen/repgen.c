/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include "repgen.h"

#ifndef  LOGFORMAT
#define  LOGFORMAT   "ttcn3_logformat -s"
#endif /* LOGFORMAT */

static const unsigned char	logo[] = {
0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x64, 0x00, 0x1B, 0x00,
0xF2, 0x00, 0x00, 0x40, 0x66, 0xCC, 0x80, 0x99, 0xCC, 0xA2,
0xA8, 0xE9, 0xBF, 0xCC, 0xFF, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF,
0xFF, 0xE1, 0xE9, 0xF3, 0x4E, 0x92, 0xCB, 0x2C, 0x00, 0x00,
0x00, 0x00, 0x64, 0x00, 0x1B, 0x00, 0x00, 0x03, 0xFE, 0x08,
0xBA, 0xDC, 0xFE, 0x30, 0xCA, 0x49, 0xAB, 0xBD, 0x38, 0xEB,
0xCD, 0xBB, 0xFF, 0x60, 0x28, 0x8E, 0x64, 0x69, 0x9E, 0x68,
0x3A, 0x0A, 0x83, 0xA1, 0xBE, 0xE1, 0x31, 0x10, 0x34, 0x21,
0xC0, 0xB8, 0xD6, 0xD2, 0x45, 0x41, 0x07, 0xB9, 0xE0, 0x24,
0x50, 0xE3, 0x11, 0x0C, 0x37, 0xA1, 0xF2, 0x41, 0x2C, 0xDA,
0x0E, 0x00, 0xA8, 0x26, 0xC0, 0x1A, 0xB0, 0x92, 0x03, 0xAB,
0x15, 0x08, 0x08, 0x0C, 0xB8, 0x00, 0x81, 0xA1, 0x95, 0xF4,
0x8E, 0xCB, 0xAD, 0xEC, 0xC2, 0x8C, 0x5C, 0x54, 0xDD, 0x83,
0x48, 0x93, 0xF0, 0x8D, 0x8A, 0x09, 0x05, 0xB0, 0x45, 0xE0,
0x8C, 0xF3, 0x8F, 0x06, 0x34, 0x50, 0x3F, 0x5D, 0x34, 0x63,
0x33, 0x00, 0x33, 0x36, 0x2D, 0x85, 0x74, 0x62, 0x0A, 0x8A,
0x77, 0x04, 0x0A, 0x35, 0x2E, 0x00, 0x34, 0x12, 0x01, 0x5C,
0x01, 0x81, 0x45, 0x71, 0x18, 0x33, 0x49, 0x6E, 0x47, 0x0A,
0x81, 0x40, 0x34, 0x37, 0x34, 0x9F, 0x0B, 0xAA, 0x0C, 0x81,
0xAB, 0x8D, 0x52, 0xAF, 0x97, 0x3E, 0x04, 0xA7, 0x04, 0x52,
0x10, 0x5E, 0x4E, 0x86, 0xB9, 0x16, 0x33, 0x48, 0x54, 0x40,
0x7C, 0x06, 0x9A, 0x84, 0xA8, 0x44, 0x05, 0xBE, 0x89, 0x3E,
0x75, 0x61, 0x3E, 0xC5, 0x0A, 0x02, 0x05, 0x96, 0xCF, 0x2E,
0x78, 0x81, 0xD7, 0x93, 0x11, 0x9D, 0xB5, 0xAA, 0x7A, 0x17,
0x33, 0x3D, 0xCD, 0x5D, 0xB5, 0x3D, 0x5C, 0x78, 0x02, 0x01,
0x3E, 0x0F, 0x81, 0x3D, 0x96, 0x8A, 0x3E, 0x32, 0x74, 0x6B,
0xEC, 0x3F, 0xA8, 0xF2, 0x10, 0x7F, 0x3E, 0x05, 0xCE, 0x54,
0x1A, 0x7C, 0xBE, 0x68, 0x82, 0x32, 0xCD, 0x56, 0x27, 0x4A,
0x36, 0x92, 0x81, 0x1B, 0x95, 0x47, 0xC1, 0x81, 0x26, 0x2C,
0x48, 0x49, 0xA3, 0x46, 0xEB, 0xC0, 0x34, 0x03, 0xEC, 0x74,
0x61, 0xDB, 0xA4, 0x7C, 0xC8, 0x46, 0x86, 0x19, 0x03, 0xA0,
0x48, 0xF9, 0x13, 0x85, 0x10, 0xC6, 0x1B, 0xD9, 0xF4, 0x48,
0x39, 0x80, 0x47, 0x13, 0xAB, 0x84, 0x84, 0x00, 0xF8, 0x48,
0x65, 0x8B, 0x56, 0x4C, 0x08, 0x22, 0x77, 0x8C, 0xA3, 0x87,
0x81, 0x8F, 0xB7, 0x49, 0x24, 0x01, 0xCC, 0x0A, 0x74, 0xE3,
0x40, 0xA7, 0x1A, 0x42, 0x9D, 0x24, 0x72, 0x02, 0x45, 0x11,
0x46, 0x89, 0x78, 0xA0, 0x24, 0xDB, 0x16, 0xE1, 0x8F, 0x91,
0x99, 0x53, 0xB2, 0x64, 0x11, 0x70, 0xC3, 0x0B, 0x9A, 0x1B,
0x2C, 0xC0, 0xB0, 0x38, 0xD4, 0x45, 0xAB, 0x00, 0xA9, 0x66,
0x73, 0x99, 0xB1, 0x02, 0x07, 0xCE, 0xD9, 0x08, 0xBC, 0xE8,
0x2C, 0x5C, 0xB2, 0xC4, 0xEA, 0x91, 0xB7, 0x74, 0xF3, 0x36,
0x40, 0xA6, 0xB7, 0xAF, 0xDF, 0xBF, 0x80, 0x03, 0x0B, 0x1E,
0x4C, 0xB8, 0xB0, 0xE1, 0xC3, 0x12, 0x12, 0x00, 0x00, 0x3B };





static int	GetIndent ( char *buf, int buflen, int indent, int comment )
{
/* char	*ctemp;*/
 int	i;


 for ( i = 0; i < buflen; i++ )
	{
	 switch ( buf[i] )
		{
		 case '/':
			 if ( ( i < buflen ) && ( buf[++i] == '*') ) comment++;
			 if ( ( i < buflen ) && ( buf[i+1] == '/') ) return indent;
			 break;

		 case '*':
			 if ( ( i < buflen ) && ( buf[++i] == '/') ) comment--;
			 break;

		 case '{':
			 if ( ! comment ) indent++;
			 break;

		 case '}':
			 if ( ! comment ) indent--;
			 break;
		}
	}

 return indent;
}


static int	TruncateLine ( char *inbuf, int inbuflen, int tablen, int fillcol, FILE *wfile )
{
 int	i, colnum = 0, bufpos;
 char	*buf = malloc(inbuflen+20*tablen+1);
 if(buf == NULL) {
	fprintf(stderr, "malloc() failed.\n");
	exit(2);
 }

 bufpos = 0;
 for ( i = 0; i < inbuflen; i++ )
	{
	 buf[bufpos] = inbuf[i];
	 if ( buf[bufpos] == ' ' ) colnum = bufpos + 1;
	 else if ( buf[bufpos] == '\n' )
		{
		 buf[bufpos+1] = '\0';
		 fputs ( buf, wfile );
		 colnum = 0;
		 bufpos = -1;
		}
	 else if ( buf[bufpos] == '\t' )
		{
		 int	j;
		 for ( j = 0; j < tablen; j++ ) buf[bufpos+j] = ' ';
		 colnum += tablen;
		 bufpos += tablen-1;
		}
	 if ( ( bufpos >= fillcol ) && colnum )
		{
		 buf[colnum-1] = '\0';
		 fputs ( buf, wfile );
		 putc ( '\n', wfile );
		 bufpos -= colnum;
		 strncpy ( buf, buf + colnum, bufpos + 1 );
		 colnum = 0;
		}
	 bufpos++;
	}
 free(buf);
 return 0;
}


int	WriteCode ( struct listentry *first, char *code_srcdir, char *code_dstdir, int tablen, int fillcol )
{
 char			*ctemp;
/* char			dirname[256];*/
 DIR			*opdir;
 struct listentry	*tclist;
 struct dirent		*dp;

 if ( code_srcdir[strlen(code_srcdir)-1] == '/' ) code_srcdir[strlen(code_srcdir)-1] = '\0';

 opdir = opendir ( code_srcdir );
 if ( !opdir )
	{
	 perror ( code_srcdir );
	 return -1;
	}

 for ( dp = readdir ( opdir ); dp != NULL; dp = readdir ( opdir ) )
	{
	 ctemp = strrchr ( dp->d_name, '.' );
	 if ( ctemp && ( strcmp ( ctemp, ".ttcn" ) == 0 ) )
		{
		 char	fullname[MAXLEN];
		 char	inbuf[1024];
		 FILE	*rfile, *wfile;

		 sprintf ( fullname, "%s/%s", code_srcdir, dp->d_name );
		 if ( ( rfile = fopen ( fullname, "rt") ) == NULL )
			{
			 perror ( fullname );
			 return -1;
			}

		 while ( fgets ( inbuf, sizeof ( inbuf ), rfile ) != NULL )
			{
			 ctemp = strchr ( inbuf, 't' );
			 if ( ctemp && ( strncmp ( ctemp, "testcase", 8 ) == 0 ) )
				{
				 tclist = first;
				 while ( tclist->next != NULL )
					{
					 ctemp = strchr ( inbuf, tclist->tcname[0] );
					 if ( ctemp && ( strncmp ( ctemp, tclist->tcname, strlen ( tclist->tcname ) ) == 0 ) )
						{
						 int	indent = 0, comment = 0;
						 int	started = 0, needed = 1;

						 sprintf ( fullname, "%s/%s.ttcn", code_dstdir, tclist->tcname );
						 if ( ( wfile = fopen ( fullname, "wt") ) == NULL )
							{
							 perror ( fullname );
							 return -1;
							}

						 indent = GetIndent ( inbuf, strlen ( inbuf ), indent, comment );
						 if ( indent ) started = 1;
						 TruncateLine ( inbuf, strlen ( inbuf ), tablen, fillcol, wfile );
						 while ( needed )
							{
							 if ( fgets ( inbuf, sizeof ( inbuf ), rfile ) == NULL ) break;
							 indent = GetIndent ( inbuf, strlen ( inbuf ), indent, comment );
							 if ( ! started && indent ) started = 1;
							 if ( started && ! indent ) needed = 0;
							 TruncateLine ( inbuf, strlen ( inbuf ), tablen, fillcol, wfile );
							}
						 fclose ( wfile );
						}
					 tclist = tclist->next;
					}
				}
			}

		 fclose ( rfile );
		}
	}

 if ( closedir ( opdir ) != 0 )
	{
	 perror ( code_srcdir );
	 return -1;
	}

 return 0;
}


int	WriteLog ( struct listentry *first, char *log_srcdir, char *log_dstdir )
{
 char			cwd[MAXLEN], command[MAXLEN], log_absrcdir[MAXLEN];
 char			oldname[MAXLEN], newname[MAXLEN], tempdir[MAXLEN];
 int			used;
 DIR			*opdir;
 struct listentry	*tclist;
 struct dirent		*dp;

 sprintf ( tempdir, "%s/temp", log_dstdir );
 mkdir ( tempdir
#ifndef MINGW
		, S_IRWXU
#endif
			  );

 if ( getcwd ( cwd, sizeof ( cwd ) ) == NULL )
	{
	 remove ( tempdir );
	 perror ( "getcwd" );
	 return -1;
	}

 if ( ( log_srcdir[0] == '.' ) && ( ( log_srcdir[1] == '/' ) || ( log_srcdir[1] == '\0' ) ) )
	{
	 char *ctemp;
	 ctemp = &log_srcdir[1];
	 strcpy (log_absrcdir, cwd);
	 strcat (log_absrcdir, ctemp);
	}
 else strcpy ( log_absrcdir, log_srcdir );

 if ( log_absrcdir[strlen(log_absrcdir)-1] == '/' ) log_absrcdir[strlen(log_absrcdir)-1] = '\0';

 opdir  = opendir ( log_absrcdir );
 if ( !opdir )
	{
	 remove ( tempdir );
	 perror ( log_absrcdir );
	 return -1;
	}

 if ( chdir ( tempdir ) == -1 )
	{
	 remove ( tempdir );
	 perror ( tempdir );
	 return -1;
	}

 for ( dp = readdir ( opdir ); dp != NULL; dp = readdir ( opdir ) )
	{
	 char	*ctemp;

	 ctemp = strrchr ( dp->d_name, '.' );
	 if ( ctemp && ( strcmp ( ctemp, ".log" ) == 0 ) )
		{
		 sprintf ( command, LOGFORMAT " %s/%s >/dev/null 2>&1", log_absrcdir, dp->d_name);
		 if ( system ( command ) == -1 )
			{
			 remove ( tempdir );
			 perror ( command );
			 return -1;
			}
		}
	}

 closedir ( opdir );

 if ( chdir ( cwd ) == -1 )
	{
	 remove ( tempdir );
	 perror ( cwd );
	 return -1;
	}

 opdir  = opendir ( tempdir );
 if ( !opdir )
	{
	 remove ( tempdir );
	 perror ( tempdir );
	 return -1;
	}

 for ( dp = readdir ( opdir ); dp != NULL; dp = readdir ( opdir ) )
	{
	 used = 0;

	 if ( ( strcmp ( dp->d_name, ".." ) == 0 ) || ( strcmp ( dp->d_name, "." ) == 0 ) ) continue;

	 tclist = first;
	 while ( ( tclist->next != NULL ) && !used )
		{
		 if ( strcmp ( dp->d_name, tclist->tcname ) == 0 )
			{
			 used = 1;
			 sprintf ( oldname, "%s/temp/%s", log_dstdir, tclist->tcname );
			 sprintf ( newname, "%s/%s.ttcnlog", log_dstdir, tclist->tcname );
			 if ( rename ( oldname, newname ) == -1 )
				{
				 remove ( tempdir );
				 perror ( "rename" );
				 return -1;
				}
			}
		 tclist = tclist->next;
		}

	 if ( ! used )
		{
		 sprintf ( oldname, "%s/temp/%s", log_dstdir, dp->d_name );
		 remove ( oldname );
		}
	}

 if ( remove ( tempdir ) == -1 )
	{
	 perror ( tempdir );
	 return -1;
	}

 if ( closedir ( opdir ) != 0 )
	{
	 perror ( log_srcdir );
	 return -1;
	}

 return 0;
}


int	WriteDump ( struct listentry *first, char *dump_srcdir, char *dump_dstdir, int tablen, int fillcol )
{
/* char			*ctemp;
 char			dirname[256];*/
 char		 	fullname[MAXLEN];
 DIR			*opdir;
 struct listentry	*tclist;
 struct dirent		*dp;

 if ( dump_srcdir[strlen(dump_srcdir)-1] == '/' ) dump_srcdir[strlen(dump_srcdir)-1] = '\0';

 opdir = opendir ( dump_srcdir );
 if ( !opdir )
	{
	 perror ( dump_srcdir );
	 return -1;
	}

 for ( dp = readdir ( opdir ); dp != NULL; dp = readdir ( opdir ) )
	{
	 if ( ( strcmp ( dp->d_name, ".." ) == 0 ) || ( strcmp ( dp->d_name, "." ) == 0 ) ) continue;

	 tclist = first;
	 while ( tclist->next != NULL )
		{
		 sprintf ( fullname, "%s.dump", tclist->tcname );
		 if ( strcmp ( dp->d_name, fullname ) == 0 )
			{
			 char	inbuf[1024];
			 FILE	*rfile, *wfile;

			 sprintf ( fullname, "%s/%s", dump_srcdir, dp->d_name );
			 if ( ( rfile = fopen ( fullname, "rt") ) == NULL )
				{
				 perror ( fullname );
				 return -1;
				}

			 sprintf ( fullname, "%s/%s.dump", dump_dstdir, tclist->tcname );
			 if ( ( wfile = fopen ( fullname, "wt") ) == NULL )
				{
				 perror ( fullname );
				 return -1;
				}

			 while ( fgets ( inbuf, sizeof ( inbuf ), rfile ) != NULL )
				{
				 GetIndent ( inbuf, strlen ( inbuf ), 0, 0 );
				 TruncateLine ( inbuf, strlen ( inbuf ), tablen, fillcol, wfile );
				}
			 fclose ( rfile );
			 fclose ( wfile );
			}
		 tclist = tclist->next;
		}
	}

 if ( closedir ( opdir ) != 0 )
	{
	 perror ( dump_srcdir );
	 return -1;
	}

 return 0;
}


int	Genhtml ( struct listentry *first, char *title, char *data_dstdir )
{
 char			*ctemp;
 char			dirname[MAXLEN], filename[MAXLEN], buf[1024];
 time_t			tval;
 struct listentry	*tclist;
 FILE			*rfile, *wfile;
/* int			i;*/

 sprintf ( buf, "%s.html", data_dstdir );
 if ( ( wfile = fopen ( buf, "wt" ) ) == NULL )
	{
	 perror ( buf );
	 return -1;
	}

 ctemp = strrchr ( data_dstdir, '/' );
 strcpy ( dirname, ctemp + 1 );

 fprintf(wfile, "<html>\n"
  "<head>\n"
  "<title>TTCN-3 Test report</title>\n"
  "</head>\n"
  "<body>\n"
  "    <IMG src=\"%s/logo.gif\">\n"
  "    <script type=\"text/javascript\">\n", dirname);

 fputs ( "adatok = new Array (", wfile );

 tclist = first;
 while ( tclist->next != NULL )
	{
	 sprintf ( buf, "\"%s\", ", tclist->tcname );
	 fputs ( buf, wfile );

	 sprintf ( filename, "%s/%s.short", data_dstdir, tclist->tcname );
	 sprintf ( buf, "\"./%s/%s.short\", ", dirname, tclist->tcname );
	 if ( access ( filename, R_OK ) == 0 ) fputs ( buf, wfile );
	 else fputs ( "\"\", ", wfile );

	 sprintf ( filename, "%s/%s.long", data_dstdir, tclist->tcname );
	 sprintf ( buf, "\"./%s/%s.long\", ", dirname, tclist->tcname );
	 if ( access ( filename, R_OK ) == 0 ) fputs ( buf, wfile );
	 else fputs ( "\"\", ", wfile );

	 sprintf ( filename, "%s/%s.ttcn", data_dstdir, tclist->tcname );
	 sprintf ( buf, "\"./%s/%s.ttcn\", ", dirname, tclist->tcname );
	 if ( access ( filename, R_OK ) == 0 ) fputs ( buf, wfile );
	 else fputs ( "\"\", ", wfile );

	 sprintf ( filename, "%s/%s.ttcnlog", data_dstdir, tclist->tcname );
	 sprintf ( buf, "\"./%s/%s.ttcnlog\", ", dirname, tclist->tcname );
	 if ( access ( filename, R_OK ) == 0 ) fputs ( buf, wfile );
	 else fputs ( "\"\", ", wfile );

	 sprintf ( filename, "%s/%s.dump", data_dstdir, tclist->tcname );
	 sprintf ( buf, "\"./%s/%s.dump\",\n", dirname, tclist->tcname );
	 if ( access ( filename, R_OK ) == 0 ) fputs ( buf, wfile );
	 else fputs ( "\"\",\n", wfile );

	 tclist = tclist->next;
	}

 fprintf ( wfile, "\"\" );\nmtitle=\"Test suite: %s\";\n", title );

 fputs ("		egyelem=6;\n"
  "			tcszam=(adatok.length-1) / egyelem;\n"
  "			kijelolt=new Array(1,1,1,1);\n"
  "\n"
  "			function tcChange()\n"
  "			{\n"
  "				document.myForm.showOption.length=0;\n"
  "				index=document.myForm.testchoose.selectedIndex;\n"
  "				if (index!=-1)\n"
  "				{\n"
  "					if (adatok[index*egyelem+2]!=\"\")\n"
  "						{\n"
  "						elem = new Option(\"Detailed description\", adatok[index*egyelem+2], false, kijelolt[0]);\n"
  "						document.myForm.showOption.options[document.myForm.showOption.length]=elem;\n"
  "						}\n"
  "					if (adatok[index*egyelem+3]!=\"\")\n"
  "						{\n"
  "						elem = new Option(\"TTCN-3 code\", adatok[index*egyelem+3], false, kijelolt[1]);\n"
  "						document.myForm.showOption.options[document.myForm.showOption.length]=elem;\n"
  "						}\n"
  "					if (adatok[index*egyelem+4]!=\"\")\n"
  "						{\n"
  "						elem = new Option(\"TTCN-3 executor's log\", adatok[index*egyelem+4], false, kijelolt[2]);\n"
  "						document.myForm.showOption.options[document.myForm.showOption.length]=elem;\n"
  "						}\n"
  "					if (adatok[index*egyelem+5]!=\"\")\n"
  "						{\n"
  "						elem = new Option(\"Other type of log\", adatok[index*egyelem+5], false, kijelolt[3]);\n"
  "						document.myForm.showOption.options[document.myForm.showOption.length]=elem;\n"
  "						}\n"
  "					}\n"
  "			}\n"
  "\n"
  "			function goNext()\n"
  "			{\n"
  "				if (document.myForm.testchoose.selectedIndex<document.myForm.testchoose.length-1)\n"
  "					{\n"
  "					itemsSelected();\n"
  "					document.myForm.testchoose.selectedIndex++;\n"
  "					tcChange();\n"
  "					}\n"
  "			}\n"
  "\n"
  "			function goPrev()\n"
  "			{\n"
  "				if (document.myForm.testchoose.selectedIndex>0)\n"
  "					{\n"
  "					itemsSelected();\n"
  "					document.myForm.testchoose.selectedIndex--;\n"
  "					tcChange();\n"
  "					}\n"
  "			}\n"
  "\n"
  "			function itemsSelected()\n"
  "			{\n"
  "				if (document.myForm.showOption.length)\n"
  "				{\n"
  "					for (b=0;b<4;b++)\n"
  "						for (i=0;i<document.myForm.showOption.length;i++)\n"
  "							if (document.myForm.showOption.options[i].value==adatok[index*egyelem+b+2])\n"
  "							{\n"
  "								if (!document.myForm.showOption.options[i].selected)\n"
  "								kijelolt[b]=0;\n"
  "								else\n"
  "								kijelolt[b]=1;\n"
  "							}\n"
  "				}\n"
  "			}\n"
  "\n"
  "\n"
  "			function windShow()\n"
  "			{\n"
  "				index=document.myForm.testchoose.selectedIndex;\n"
  "				selectnumb=0;\n"
  "				for (i=0; i<document.myForm.showOption.length; i++)\n"
  "					if (document.myForm.showOption.options[i].selected) selectnumb++;\n"
  "				if (selectnumb>0)\n"
  "				{\n"
  "					mywin=window.open(\"\",index,\"toolbar,width=180,resizable=yes,scrollbars=no\");\n"
  "					mywin.document.write(\"<html> <HEAD> <TITLE> Test Case: \"+document.myForm.testchoose.options[index].text+\"</TITLE> </HEAD> <frameset rows=\\\"\");\n"
  "					csillag=\"1\";\n"
  "					for (i=0; i<selectnumb; i++)\n"
  "						csillag=\"*,\"+csillag;\n"
  "					mywin.document.write(csillag+\"\\\">\");\n"
  "					for (i=0; i<document.myForm.showOption.length; i++)\n"
  "						if (document.myForm.showOption.options[i].selected)\n"
  "							mywin.document.write(\"<frame src=\\\"\"+document.myForm.showOption.options[i].value+\"\\\">\");\n"
  "					mywin.document.write(\"</frameset></html>\");\n"
  "					mywin.document.close();\n"
  "				}\n"
  "			}\n"
  "\n"
  "			function summaryShow()\n"
  "			{\n"
  "					mywin=window.open(\"\",index,\"toolbar,resizable=yes,scrollbars=yes\");\n"
  "					mywin.document.writeln(\"<html> <HEAD> <TITLE> Summary </TITLE> </HEAD><body>\");\n"
  "					mywin.document.writeln(\"<br>\");\n"
  "					mywin.document.write(\"<P align=\\\"center\\\"><font size=\\\"10\\\">\");\n"
  "					mywin.document.write(mtitle);\n"
  "					mywin.document.writeln(\"</P>\");\n"
  "					mywin.document.write(\"<font size=\\\"5\\\">\");\n"
  "					mywin.document.writeln(\"\");\n"
  "mywin.document.writeln(\"<hr>\");\n"
  "mywin.document.writeln(\"<br>\");\n"
  "					mywin.document.write(\"<table border=\\\"2\\\" cellpading=\\\"5\\\" cellspacing=\\\"2\\\"><tr><th scope=\\\"col\\\">&nbsp Test Case &nbsp</th><th scope=\\\"col\\\">&nbsp Short Description &nbsp</th><th scope=\\\"col\\\">&nbsp Verdict &nbsp</th>\");\n", wfile);

 tclist = first;
 while ( tclist->next != NULL )
	{
	 sprintf ( buf, "mywin.document.writeln(\"<tr><td>&nbsp %s &nbsp</td><td>", tclist->tcname );
	 fputs ( buf, wfile );

	 sprintf ( filename, "%s/%s.short", data_dstdir, tclist->tcname );

	 if ( ( rfile = fopen ( filename, "rt" ) ) == NULL ) fputs ( "No description found!", wfile );
	 else
		{
		 int c;

		 while ( ( c = getc ( rfile ) ) != EOF )
			{
			 if ( c == '\n' ) putc ( ' ', wfile );
			 else if ( c == '"' )
				{
				 putc ( '\\', wfile );
				 putc ( c, wfile );
				}
			 else putc ( c, wfile );
			}
		 fclose ( rfile );
		}
	 fputs ( "</td><td ALIGN=\\\"CENTER\\\">&nbsp ", wfile );

	 sprintf ( filename, "%s/%s.ttcnlog", data_dstdir, tclist->tcname );
	 if ( ( rfile = fopen ( filename, "rt" ) ) == NULL ) fputs ( "none", wfile );
	 else
		{
		 while ( fgets ( buf, sizeof ( buf ), rfile ) != NULL )
			{
			 ctemp = strchr ( buf, 'V' );
			 while ( ctemp != NULL )
				{
				 if ( strncmp ( ctemp, "Verdict: ", 9 ) == 0 )
					{
					 if ( ctemp[strlen(ctemp)-1] == '\n' ) ctemp[strlen(ctemp)-1] = '\0';
					 fputs ( ctemp + 9, wfile );
					 ctemp = NULL;
					}
				 else ctemp = strchr ( ctemp+1, 'V' );
				}
			}
		 fclose ( rfile );
		}

	 fputs ( " &nbsp</td></tr>\");\n", wfile );

	 tclist = tclist->next;
	}

 tval = time ( NULL );
 ctemp = ctime ( &tval );
 if ( ctemp[strlen(ctemp)-1] == '\n' ) ctemp[strlen(ctemp)-1] = '\0';

 fprintf(wfile, "					mywin.document.write(\"</tr></table>\");\n"
  "					mywin.document.writeln(\"<br>\");\n"
  "					mywin.document.write(\"<font size=\\\"3\\\">\");\n"
  "					mywin.document.write(\"Date : \")\n"
  "					mywin.document.writeln(\"%s\");\n"
  "					mywin.document.write(\"</body></html>\");\n"
  "					mywin.document.close();\n"
  "			}\n"
  "\n"
  "			document.write(\"<P align=\\\"center\\\"><font size=\\\"10\\\">\");\n"
  "			document.write(mtitle);\n"
  "			document.write(\"<font size=\\\"5\\\">\");\n"
  "\n"
  "		</script>\n"
  "		<form name=\"myForm\">\n"
  "		<hr><br><SELECT size=\"12\" NAME=\"testchoose\" onChange=\"itemsSelected();tcChange()\">\n"
  "			<OPTION> Blank testcase Name ....................\n"
  "		</SELECT>\n"
  "		<SELECT multiple size=\"12\" NAME=\"showOption\" >\n"
  "			<OPTION SELECTED> Blank options Name ........................................\n"
  "		</SELECT>\n"
  "		<p>\n"
  "		<input type=\"button\" name=\"PrevButt\" value=\"Previous\" onClick=\"goPrev()\">\n"
  "		<input type=\"button\" name=\"nextButt\" value=\"Next\" onClick=\"goNext()\">\n"
  "		<input type=\"button\" name=\"newWind\" value=\"Show\" onClick=\"windShow()\">\n"
  "		<input type=\"button\" name=\"summary\" value=\"Summary\" onClick=\"summaryShow()\">\n"
  "		</p>\n"
  "		</form>\n"
  "		<script type=\"text/javascript\">\n"
  "			for (i=0; i<tcszam; i++)\n"
  "			{\n"
  "				elem = new Option(adatok[i*egyelem], \"\", false, false);\n"
  "				document.myForm.testchoose.options[i]=elem;\n"
  "			}\n"
  "			document.myForm.testchoose.selectedIndex=0;\n"
  "			tcChange();\n"
  "			document.writeln(tcszam);"
  "		</script>\n"
  "		Testcases altogether <p></p>\n"
  "		<font size=-1>Date: %s <p></p>\n"
  "	</body>\n"
  "</html>\n", ctemp, ctemp);

 fclose ( wfile );

 sprintf ( filename, "%s/logo.gif", data_dstdir );
 if ( ( wfile = fopen ( filename, "wt" ) ) == NULL )
	{
	 perror ( filename );
	 return -1;
	}

 fwrite ( &logo, sizeof ( logo ), 1, wfile );

 fclose ( wfile );

 return 0;
}

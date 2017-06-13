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
 *   Tatarka, Gabor
 *
 ******************************************************************************/
/**************************
Log filter for TTCNv3
written by Gabor Tatarka
**************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "../common/version_internal.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

#define Fail    -1
#define False   0
#define True    1

#define ERR_NONE 0
#define ERR_WRITE 1
#define ERR_INVALID_LOG 2

/*filter flag values:*/
#define Write 1
#define DontWrite 2
/*0 is: act as Wothers*/

#define TimeLength 15
#define SecondLength 9
#define DateTimeLength 27
#define MaxTimeStampLength 27
#define BufferSize 512
#define YYYYMONDD 1		/*format of Date: year/month/day*/

static const char * const MON[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

#define MaxType 15
typedef enum {/*If you add new types, add it to the end of the list, and
		also add it to EventTypeNames! ET_Unknown has no representation
		in text, and it must not be changed.Also change MaxType!*/
		ET_Unknown=-1,ET_Error,ET_Warning,ET_Portevent,ET_Timerop,
		ET_Verdictop,ET_Defaultop,ET_Action,ET_Testcase,ET_Function,
		ET_User,ET_Statictics,ET_Parallel,ET_Matching,ET_Debug,
		ET_Executor
		} EventTypes;
static const char * const EventTypeNames[] = {
		"ERROR","WARNING","PORTEVENT","TIMEROP","VERDICTOP","DEFAULTOP",
		"ACTION","TESTCASE","FUNCTION","USER","STATISTICS","PARALLEL",
		"MATCHING","DEBUG","EXECUTOR"
		};
static int Wothers=Write;
static int Wflags[MaxType];

static int IsSecond(const char *str)/*Is timestamp format Seconds*/
{
    int a;
    if(*str<'0'||*str>'9')return False;/*first digit*/
    while(*str>='0'&&*str<='9')str++;/*other digits*/
    if(*str!='.')return False;/* '.' */
    for(a=0;a<6;a++)if(*str<'0'||*str>'9')return False;/*microseconds(6 digits)*/
    return True;
}

static int IsSecond2_6(const char *str)/*does string contain sec(2).usec(6)*/
{
    int a;
    for(a=0;a<SecondLength;a++) {
	if(a==2) {
	    if(*str=='.') {
		str++;continue;
	    } else return False;
	}
	if(*str<'0'||*str>'9')return False;
	str++;
	}
    return True;
}

static int IsTime(const char *str)/*Is timestamp format Time*/
{
    int a;
    if(False==IsSecond2_6(str+6))return False;
    for(a=0;a<6;a++){
	if(a==2||a==5) {
	    if(*str==':') {
		str++;continue;
	    } else return False;
	}
	if(*str<'0'||*str>'9')return False;
	str++;
    }
    return True;
}

#ifdef YYYYMONDD /*Date format: year/month/day*/
# define FIRST_LEN 4
# define THIRD_LEN 2
#else /*Date format: day/month/year*/
# define FIRST_LEN 2
# define THIRD_LEN 4
#endif

static int IsDateTime(const char *str)/*is timestamp format Date/Time*/
{
    int a,b;
    if(False==IsTime(str+12))return False;
    for(a=0;a<FIRST_LEN;a++) {/*YYYY or DD*/
	if(*str<'0'||*str>'9')return False;
	str++;
    }
    if(*str!='/')return False;/* '/' */
    str++;
    for(a=0,b=0;a<12;a++)if(0==strncmp(str,MON[a],3)){b=1;break;}/*MON*/
    if(!b)return False;
    str+=3;
    if(*str!='/')return False;/* '/' */
    str++;
    for(a=0;a<THIRD_LEN;a++) {/*DD or YYYY*/
	if(*str<'0'||*str>'9')return False;
	str++;
    }
    return True;
}

static int ValidTS(const char *str)/*is timestamp of event valid*/
{
    if(True==IsSecond(str))return True;
    if(True==IsTime(str))return True;
    if(True==IsDateTime(str))return True;
    return False;
}

static EventTypes get_event_type(char *buf)
{
    int a;
    char *ptr1,*ptr2;
    ptr1=buf+strlen(buf);
    for(a=0;a<MaxType;a++) {
	ptr2=strstr(buf,EventTypeNames[a]);
	if(ptr2<ptr1&&ptr2!=NULL) {
	    ptr1=ptr2;
	    if(*(ptr2+strlen(EventTypeNames[a]))==' '&& *(ptr2-1)==' ' 
               && *(ptr2+strlen(EventTypeNames[a])+1)!='|')
		return a;
	}
    }
    return ET_Unknown;
}

static int ProcessFile(FILE *out,FILE *in) /*filter infile to outfile*/
{
    int b=0;
    char line_buffer[BufferSize];
    int last_event=0;
    EventTypes current_event_type=ET_Unknown;
    while(1) {
	line_buffer[0]='\0';
	if(NULL==fgets(line_buffer,sizeof(line_buffer),in)) last_event=1;
	else if(ValidTS(line_buffer)) {
	    current_event_type=get_event_type(line_buffer);
	    b++;
	}
	if(current_event_type==ET_Unknown && Wothers==Write) {
	    if(0>fprintf(out,"%s",line_buffer))return ERR_WRITE;
	} else if(Wflags[current_event_type]==Write ||
	    (Wflags[current_event_type]==0 && Wothers==Write)) {
	    if(0>fprintf(out,"%s",line_buffer))return ERR_WRITE;
	}
	if(last_event)break;
    }
    if(!b)return ERR_INVALID_LOG;
    else return ERR_NONE;
}

static void Usage(const char *progname)
{
    fprintf(stderr,
	"Usage: %s [-o outfile] [infile.log] [eventtype+|-] [...]\n"
	"    or %s option\n"
	"options:\n"
	"	-o outfile:	write merged logs into file outfile\n"
	"	-v:		print version\n"
	"	-h:		print help (usage)\n"
	"	-l:		list event types\n"
	"If there is no outfile specified output is stdout.\n"
	"If infile is omitted input is stdin.\n"
	"Including and excluding event types at the same time is not allowed.\n"
	"\n",progname,progname);
}

static void ListTypes(void)
{
    int a;
    fprintf(stderr,"Event types:\n");
    for(a=0;a<MaxType;a++)fprintf(stderr,"\t%s\n",EventTypeNames[a]);
}

int main(int argc,char *argv[])
{
    int a,b,c;
    FILE *outfile = NULL, *infile = NULL;
    char *outfile_name=NULL,*infile_name=NULL;
    char tmp[20];
    int vflag=0,oflag=0,hflag=0,lflag=0,plusflag=0,minusflag=0;
#ifdef LICENSE
    license_struct lstr;
#endif
    while ((c = getopt(argc, argv, "lhvo:")) != -1) {
	switch (c) {
	case 'o':/*set outfile*/
	    outfile_name=optarg;
	    oflag = 1;
	    break;
	case 'v':/*print version*/
	    vflag=1;
	    break;
	case 'h':/*print help (usage)*/
	    hflag=1;
	    break;
	case 'l':/*list event types*/
	    lflag=1;
	    break;
	default:
	    Usage(argv[0]);
	    return EXIT_FAILURE;
	}
    }
    if (oflag + vflag + hflag + lflag > 1) {
	Usage(argv[0]);
	return EXIT_FAILURE;
    } else if (vflag) {
        fputs("Log Filter for the TTCN-3 Test Executor\n"
	    "Product number: " PRODUCT_NUMBER "\n"
	    "Build date: " __DATE__ " " __TIME__ "\n"
	    "Compiled with: " C_COMPILER_VERSION "\n\n"
	    COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
	print_license_info();
#endif
	return EXIT_SUCCESS;
    } else if (hflag) {
	Usage(argv[0]);
	return EXIT_SUCCESS;
    } else if(lflag) {
	ListTypes();
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
	fputs("The license key does not allow the filtering of log files.\n",
	    stderr);
	return 2;
    }
    free_license(&lstr);
    free_openssl();
#endif
/*
switches: -v -o -h -l
filter parameters: parameter+ or parameter-
*/
    for(a=0;a<MaxType;a++)Wflags[a]=0;
    for(a=1;a<argc;a++) {
	if(*argv[a]=='-'){if(*(argv[a]+1)=='o')a++;continue;}/*switch*/
	if(*(argv[a]+strlen(argv[a])-1)=='-') {/*type to ignore*/
	    for(b=0,c=0;b<MaxType;b++)
		if(0==strncmp(EventTypeNames[b],argv[a],
			strlen(EventTypeNames[b]))&&strlen(EventTypeNames[b])==
			strlen(argv[a])-1) {
		    Wflags[b]=DontWrite;
		    c=1;
		}
	    if(!c) {/*Undefined type*/
		strncpy(tmp,argv[a],sizeof(tmp)-1);
		if(strlen(argv[a])>sizeof(tmp)-1)
		    for(c=2;c<5;c++)tmp[sizeof(tmp)-c]='.';
		else tmp[strlen(argv[a])-1]='\0';
		tmp[sizeof(tmp)-1]='\0';
		if(strlen(tmp))fprintf(stderr,"Warning: %s is not a valid "
		    "event-type name.\n",tmp);
		else fprintf(stderr,"Warning: `-\' without an event-type "
		    "name.\n");
	    }
	    Wothers=Write;
	    minusflag=1;
	    continue;
	}
	if(*(argv[a]+strlen(argv[a])-1)=='+') {/*type to write out*/
	    for(b=0,c=0;b<MaxType;b++)
		if(0==strncmp(EventTypeNames[b],argv[a],
			strlen(EventTypeNames[b]))&&strlen(EventTypeNames[b])==
			strlen(argv[a])-1) {
		    Wflags[b]=Write;
		    c=1;
		}
	    if(!c) {/*Undefined type*/
		strncpy(tmp,argv[a],sizeof(tmp)-1);
		if(strlen(argv[a])>sizeof(tmp)-1)
		    for(c=2;c<5;c++)tmp[sizeof(tmp)-c]='.';
		else tmp[strlen(argv[a])-1]='\0';
		tmp[sizeof(tmp)-1]='\0';
		if(strlen(tmp))fprintf(stderr,"Warning: %s is not a valid "
		    "event-type name.\n",tmp);
		else fprintf(stderr,"Warning: `+\' without an event-type "
		    "name.\n");
	    }
	    Wothers=DontWrite;
	    plusflag=1;
	    continue;
	}
	if(infile_name!=NULL) {/*only one input file at once*/
	    fprintf(stderr,"Error: more than one input file specified.\n");
	    return EXIT_FAILURE;
	}
	infile_name=argv[a];
    }
    if(minusflag&&plusflag) {/*type1+ and type2- at the same time could cause
		types that are not defined what to do with, to act based on the
		last filter definition. Thus it is not allowed.*/
	fprintf(stderr,"Error: include and exclude at the same time.\n");
	return EXIT_FAILURE;
    }
    if(infile_name==NULL)infile=stdin;/*if no infile specified use stdin*/
    else {
	infile=fopen(infile_name,"r");
	if(infile==NULL) {
	    fprintf(stderr,"Error opening %s : %s\n",infile_name,
		strerror(errno));
	    return EXIT_FAILURE;
	}
    }
    if(oflag) {
	outfile=fopen(outfile_name,"w");
	if(outfile==NULL) {
	    fprintf(stderr,"Error creating %s : %s\n",outfile_name,
		strerror(errno));
	    return EXIT_FAILURE;
	}
    } else outfile=stdout;/*if no outfile specified use stdout*/
    a=ProcessFile(outfile,infile);/*filter infile to outfile*/
    if(a==ERR_INVALID_LOG) {
	if(infile_name!=NULL)fprintf(stderr,"Error: the file %s is not a valid "
	    "log file.\n",infile_name);
	else fprintf(stderr,"Error: invalid format received from standard "
	    "input.\n");
	return EXIT_FAILURE;
    } else if(a==ERR_WRITE) {
	if(errno)fprintf(stderr,"Error writing to output: %s\n",
	    strerror(errno));
	else fprintf(stderr,"Error writing to output\n");
	return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


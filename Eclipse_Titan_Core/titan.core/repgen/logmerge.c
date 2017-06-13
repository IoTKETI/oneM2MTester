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
 *   Delic, Adam
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
/**************************
Log-file merger
written by Gabor Tatarka
**************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "../common/memory.h"
#include "../common/version_internal.h"

#ifdef LICENSE
#include "../common/license.h"
#endif

#ifdef MINGW
/* On MinGW seeking is not working in files opened in text mode due to a
 * "feature" in the underlying MSVCRT. So we open all files in binary mode. */
# define FOPEN_READ "rb"
# define FOPEN_WRITE "wb"
#else
# define FOPEN_READ "r"
# define FOPEN_WRITE "w"
#endif

#define Fail    -1
#define False   0
#define True    1

/* These lengths below represent the number of characters in the log file.
 * No NUL terminator is included in the length. */
#define TIMELENGTH 15
#define SECONDLENGTH 9
#define DATETIMELENGTH 27
#define MAXTIMESTAMPLENGTH 27

#define BUFFERSIZE 1024
#define YYYYMONDD 1		/* format of Date: year/month/day*/

static const char * const MON[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static const char *progname;
static FILE *outfile = NULL;

enum TimeStampFormats { TSF_Undefined = -1, TSF_Seconds, TSF_Time,
    TSF_DateTime };
static int TimeStampLength;
static enum TimeStampFormats TimeStampUsed = TSF_Undefined;

static int IsSecond(const char *str)/*Is timestamp format Seconds*/
{
    int i;
    if    (*str >= '0' && *str <= '9') str++; /* first digit */
    else return False;
    while (*str >= '0' && *str <= '9') str++; /* other digits */
    if (*str == '.') str++;
    else return False; /* '.' */
    for (i = 0; i < 6; i++, str++)
	if (*str < '0' || *str > '9') return False; /* microseconds(6 digits) */
    return True;
}

static int IsSecond2_6(const char *str)/*does string contain sec(2).usec(6)*/
{
    int a;
    for(a=0;a<SECONDLENGTH;a++) {
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

static int FormatMatch(const char *str,int format)/*does format of timestamp match format*/
{
    switch(format)
	{
	case TSF_Undefined:return False;
	case TSF_Seconds:if(False==IsSecond(str))return False;else return True;
	case TSF_Time:if(False==IsTime(str))return False;else return True;
	case TSF_DateTime:if(False==IsDateTime(str))return False;
				else return True;
	default:return False;
	}
}

/*
formats:
DateTime: yyyy/Mon/dd hh:mm:ss.us
Time: hh:mm:ss.us
Second: s.us
*/
static enum TimeStampFormats GetTimeStampFormat(const char *filename)
{
    /*get timestamp format used in file*/
    enum TimeStampFormats ret_val = TSF_Undefined;
    char str[MAXTIMESTAMPLENGTH + 1];
    FILE *fp = fopen(filename, FOPEN_READ);
    if (fp == NULL) {
	fprintf(stderr, "%s: warning: cannot open %s: %s\n", progname,
	    filename, strerror(errno));
	    return TSF_Undefined;
    }
    if (fgets(str, sizeof(str), fp) != NULL) {
	if (IsSecond(str)) ret_val = TSF_Seconds;
	else if (IsTime(str)) ret_val = TSF_Time;
	else if (IsDateTime(str)) ret_val = TSF_DateTime;
    }
    fclose(fp);
    return ret_val;
}

static char *GetComponentIdentifier(const char *path_name)
{
    char *ret_val;
    size_t name_len = strlen(path_name);
    size_t filename_begin = 0, i;
    size_t compid_begin, compid_end;
    int dash_found = 0;
    /* find the first character of the file name */
    for (i = 0; i < name_len; i++)
	if (path_name[i] == '/') filename_begin = i + 1;
    /* fallback values if neither '-' nor '.' is found */
    compid_begin = filename_begin;
    compid_end = name_len;
    /* find the last '-' character in the file name */
    for (i = name_len; i > filename_begin; i--)
	if (path_name[i - 1] == '-') {
	    dash_found = 1;
	    compid_begin = i;
	    break;
	}
    if (dash_found) {
	/* find the first '.' character after the '-' */
	for (i = compid_begin; i < name_len; i++)
	    if (path_name[i] == '.') {
		compid_end = i;
		break;
	    }
    } else {
	/* find the last '.' in the file name */
	for (i = name_len; i > filename_begin; i--)
	    if (path_name[i - 1] == '.') {
		compid_end = i - 1;
		break;
	    }
	/* find the last but one '.' in the file name */
	for (i = compid_end; i > filename_begin; i--)
	    if (path_name[i - 1] == '.') {
		compid_begin = i;
		break;
	    }
    }
    if (compid_end > compid_begin) {
	size_t compid_len = compid_end - compid_begin;
	ret_val = (char*)Malloc(compid_len + 1);
	memcpy(ret_val, path_name + compid_begin, compid_len);
	ret_val[compid_len] = '\0';
    } else ret_val = NULL;
    return ret_val;
}

static FILE *OpenTempFile(char **filename)
{
  FILE *fp;
#ifdef MINGW
  /* Function mkstemp() is not supported on MinGW */
  char *temp_name = tempnam(NULL, NULL);
  if (temp_name == NULL) {
    fprintf(stderr, "%s: creation of a temporary file failed: %s\n", progname,
      strerror(errno));
    exit(EXIT_FAILURE);
  }
  fp = fopen(temp_name, FOPEN_WRITE);
  if (fp == NULL) {
    fprintf(stderr, "%s: opening of temporary file `%s' failed: %s\n",
      progname, temp_name, strerror(errno));
    free(temp_name);
    exit(EXIT_FAILURE);
  }
  *filename = mcopystr(temp_name);
  free(temp_name);
#else
  int fd;
  *filename = mcopystr("/tmp/logmerge_XXXXXX");
  fd = mkstemp(*filename);
  if (fd < 0) {
    fprintf(stderr, "%s: creation of a temporary file based on template `%s' "
      "failed: %s\n", progname, *filename, strerror(errno));
    Free(*filename);
    exit(EXIT_FAILURE);
  }
  fp = fdopen(fd, FOPEN_WRITE);
  if (fp == NULL) {
    fprintf(stderr, "%s: system call fdopen() failed on temporary file `%s' "
      "(file descriptor %d): %s\n", progname, *filename, fd, strerror(errno));
    Free(*filename);
    exit(EXIT_FAILURE);
  }
#endif
  return fp;
}

static FILE **fp_list_in = NULL, *fpout;
static char **name_list_in = NULL;
static int fpout_is_closeable = 0,must_use_temp = 0;
static char **temp_file_list = NULL;
static int num_tempfiles = 0, num_infiles = 0, num_allfiles = 0,start_file = 0;
static int infiles_processed = False;
typedef struct
{
    char timestamp[MAXTIMESTAMPLENGTH+1];/*text of timestamp*/
    time_t sec; /*seconds in timestamp (since 1970)*/
    unsigned long usec; /*microseconds in timestamp (0L..1000000L)*/
    int wrap;/*if current timestamp is smaller than prev. timestamp -> wrap++;*/
    expstring_t data;/*text of logged event*/
    char *str_to_add;/*part of original filename*/
    int ignore; /* if true -> EOF */
    long start_line,line_ctr;/*line of event start (timestamp), line counter*/
}LogEvent;

static LogEvent **EventList;

static int OpenMaxFiles(int argc,char *argv[])
{
    int a=0;
    while(argc) {
	fp_list_in=(FILE **)Realloc(fp_list_in,(a+1)*sizeof(FILE *));
	errno = 0;
	fp_list_in[a]=fopen(argv[a], FOPEN_READ);
	if(fp_list_in[a]==NULL) {
	    switch(errno) {
		case 0:
		    /* Solaris may not set errno if libc cannot create a stdio
		       stream because the underlying fd is greater than 255 */
		case ENFILE:
		case EMFILE:
/*more infiles than can be opened->close one and create a tempfile for output*/
		    if(argc>0) {
			Free(EventList[--a]->str_to_add);
			Free(EventList[  a]);
			fclose(fp_list_in[a]);
			temp_file_list = (char**)Realloc(temp_file_list,
			  (num_tempfiles + 1) * sizeof(*temp_file_list));
			fpout = OpenTempFile(temp_file_list + num_tempfiles);
			num_tempfiles++;
			fpout_is_closeable=1;
		    }
		    num_infiles=a;
		    return a;
		default:
		    fprintf(stderr,"%s: error opening input file %s: %s\n",
			progname, argv[a], strerror(errno));
		    exit(EXIT_FAILURE);
	    }
	} else {
	    EventList=(LogEvent **)Realloc(EventList,
		(a+1)*sizeof(LogEvent *));
	    EventList[a]=(LogEvent *)Malloc(sizeof(LogEvent));
	    if (infiles_processed) EventList[a]->str_to_add = NULL;
	    else {
		/* cutting the component identifier portion out from the
		 * file name */
		EventList[a]->str_to_add =
		    GetComponentIdentifier(name_list_in[a + start_file]);
	    }
	    EventList[a]->ignore=True;
	    EventList[a]->data=NULL;
	    EventList[a]->wrap=0;
	    EventList[a]->sec=0;
	    EventList[a]->usec=0L;
	    EventList[a]->line_ctr=1L;
	    EventList[a]->start_line=1L;
	    ++a;
	}
	argc--;
    }
    if (must_use_temp) {
	temp_file_list = (char**)Realloc(temp_file_list,
	    (num_tempfiles + 1) * sizeof(*temp_file_list));
	fpout = OpenTempFile(temp_file_list + num_tempfiles);
	num_tempfiles++;
	fpout_is_closeable = 1;
    } else {
	fpout = outfile;
	if (outfile!=stdout) fpout_is_closeable = 1;
	else fpout_is_closeable = 0;
    }
    num_infiles=a;
    return a;/*nr. of opened files*/
}

static void CloseAllFiles(void)
{
    int i;
    if (fpout_is_closeable) fclose(fpout);
    for (i = 0; i < num_infiles; i++) {
	Free(EventList[i]->data);
	Free(EventList[i]->str_to_add);
	Free(EventList[i]);
    }
    Free(EventList);
    EventList = NULL;
    num_infiles = 0;
}

static int EventCmp(LogEvent *e1,LogEvent *e2)
/*Returns: if(event1<event2)-1;
if(event1==event2)0;
if(event1>event2)1;*/
{
    time_t tmpsec1,tmpsec2;
    tmpsec1=e1->sec;
    tmpsec2=e2->sec;
    if(tmpsec1<tmpsec2)return -1;
    if(tmpsec1>tmpsec2)return 1;
    if(e1->usec<e2->usec)return -1;
    if(e1->usec>e2->usec)return 1;
    return 0;
}

#ifdef YYYYMONDD
#define YearOffset 0
#define MonOffset 5
#define DayOffset 9
#else
#define DayOffset 0
#define MonOffset 3
#define YearOffset 7
#endif

static void TS2long(time_t *sec, unsigned long *usec, const char *TSstr)
/*converts timestamp string to two long values*/
{
    struct tm TM;
    int a;
    char *ptr,str[MAXTIMESTAMPLENGTH+1];
    strncpy(str,TSstr,MAXTIMESTAMPLENGTH);
    str[MAXTIMESTAMPLENGTH] = '\0';
    /*->this way only a copy of the timestamp string will be modified*/
    switch(TimeStampUsed) {
	case TSF_Seconds:
	    ptr=strpbrk(str,".");
	    *ptr='\0';ptr++;*(ptr+6)='\0';
	    *sec=(time_t)atol(str);
	    *usec=atol(ptr);
	    return;
	case TSF_Time:
	    TM.tm_year = 70;
	    TM.tm_mon = 0;
	    TM.tm_mday = 1;
	    TM.tm_isdst = -1;
	    *(str+2)='\0';
	    *(str+5)='\0';
	    *(str+8)='\0';
	    *(str+15)='\0';
	    TM.tm_hour = atoi(str);
	    TM.tm_min = atoi(str+3);
	    TM.tm_sec = atoi(str+6);
	    *usec = atol(str+9);
	    break;
	case TSF_DateTime:
	    *(str+YearOffset+4)='\0';*(str+MonOffset+3)='\0';
	    *(str+DayOffset+2)='\0';
	    TM.tm_year=atoi(str+YearOffset)-1900;
	    for(a=0;a<12;a++)if(!strcmp(MON[a],str+MonOffset)) {
				TM.tm_mon=a;break;
			    }
	    TM.tm_mday=atoi(str+DayOffset);TM.tm_isdst=-1;
	    ptr=str+12;
	    *(ptr+2)='\0';*(ptr+5)='\0';*(ptr+8)='\0';
	    *(ptr+15)='\0';
	    TM.tm_hour=atoi(ptr);TM.tm_min=atoi(ptr+3);
	    TM.tm_sec=atoi(ptr+6);*usec=atol(ptr+9);
	    break;
	default:
	    *sec = 0;
	    *usec = 0;
	    return;
    }
    *sec = mktime(&TM);
}

static int GetEvent(FILE *fp, LogEvent *event)
{
    time_t prev_sec;
    unsigned long prev_usec;
    for ( ; ; ) {
	/* find and read timestamp */
	if (fgets(event->timestamp, TimeStampLength + 1, fp) == NULL) {
	    event->ignore = True;
	    return False;
	}
	event->start_line=event->line_ctr;
	if (FormatMatch(event->timestamp, TimeStampUsed)) break;
    }
    prev_sec = event->sec;
    prev_usec = event->usec;
    TS2long(&event->sec, &event->usec, event->timestamp);
    if (event->sec < prev_sec ||
	(event->sec == prev_sec && event->usec < prev_usec)) {
	event->wrap = 1;
    }
    event->ignore = False;
    for ( ; ; ) {
	size_t a;
	char buf[BUFFERSIZE];
	/* read the log-event */
	if (fgets(buf, sizeof(buf), fp) == NULL) {
	    /* EOF was detected */
	    if (event->data == NULL) event->data = mcopystr("\n");
	    else if (event->data[mstrlen(event->data) - 1] != '\n')
		event->data = mputc(event->data, '\n');
	    return False;
	}
	a = strlen(buf);
	if(FormatMatch(buf,TimeStampUsed)) {/*Did we read the next event's timestamp?*/
	    fseek(fp, -1L * a, SEEK_CUR);/*"unread" next event*/
	    break;
	} else if (buf[a - 1] == '\n') event->line_ctr++;
	event->data=mputstr(event->data, buf);/*append buffer to event-data*/
    }
    return True;
}

static void WriteError(void)
{
    fprintf(stderr, "%s: error: writing to %s file failed: %s\n",
	progname, fpout == outfile ? "output" : "temporary", strerror(errno));
    exit(EXIT_FAILURE);
}

static void FlushEvent(LogEvent *event)
{
    if (fputs(event->timestamp, fpout) == EOF) WriteError();
    if (!infiles_processed && event->str_to_add != NULL) {
      if (putc(' ', fpout) == EOF) WriteError();
      if (fputs(event->str_to_add, fpout) == EOF) WriteError();
    }
    if (fputs(event->data, fpout) == EOF) WriteError();
    Free(event->data);
    event->data = NULL;
    event->ignore = True;
}

static void ProcessOpenFiles(void)
/*merge all opened files to fpout (that is stdout or outfile or a tempfile),
and clean up*/
{
    int i;
    for (i = 0; i < num_infiles; i++) {
	/* read first logged event from all opened files */
	if (!GetEvent(fp_list_in[i], EventList[i])) {
	    /* EOF or read error (e.g. file contained only one log event) */
	    fclose(fp_list_in[i]);
	    fp_list_in[i] = NULL;
	}
    }
    for ( ; ; ) {
	/* find the earliest timestamp */
	int min_index = -1;
	for (i = 0; i < num_infiles; i++) {
	    if (!EventList[i]->ignore && (min_index < 0 ||
		EventCmp(EventList[min_index], EventList[i]) > 0))
		min_index = i;
	}
	if (min_index < 0) break; /* no more events */
	FlushEvent(EventList[min_index]);
	if (fp_list_in[min_index] != NULL) {
	    /* read the next event from that file */
	    EventList[min_index]->wrap = 0;
	    if (!GetEvent(fp_list_in[min_index], EventList[min_index])) {
		/*EOF or read error*/
		fclose(fp_list_in[min_index]);
		fp_list_in[min_index] = NULL;
	    }
	    if (!infiles_processed && EventList[min_index]->wrap > 0) {
		fprintf(stderr,"%s: warning: timestamp is in wrong order "
		    "in file %s line %ld\n", progname,
		    name_list_in[min_index + start_file],
		    EventList[min_index]->start_line);
	    }
	}
    }
    for (i = 0; i < num_infiles; i++) {
      if (fp_list_in[i] != NULL) fclose(fp_list_in[i]);
    }
    Free(fp_list_in);
    fp_list_in = NULL;
}

static void DelTemp(void)
{
    int a;
    for(a=0;a<num_tempfiles;a++) {
	fprintf(stderr, "%s: deleting temporary file %s\n", progname,
	    temp_file_list[a]);
	remove(temp_file_list[a]);
	Free(temp_file_list[a]);
    }
    Free(temp_file_list);
}

static void Usage(void)
{
    fprintf(stderr,
	"Usage: %s [-o outfile] file1.log [file2.log ...]\n"
	"    or %s -v\n"
	"options:\n"
	"       -o outfile:	write merged logs into file outfile\n"
	"       -v:		print version\n"
	"If there is no outfile specified output is stdout.\n\n",
	progname,progname);
}

static void ControlChandler(int x)
{
    (void)x;
    /* the temporary files will be deleted by exit() */
    exit(EXIT_FAILURE);
}

int main(int argc,char *argv[])
{
    int a,b,c,processed_files=0,filename_count=0;
    char *outfile_name=NULL;
    int vflag=0,oflag=0;
#ifdef LICENSE
    license_struct lstr;
#endif
    progname=argv[0];
    atexit(DelTemp);
    signal(SIGINT,ControlChandler);
    while ((c = getopt(argc, argv, "vo:")) != -1) {
	switch (c) {
	case 'o':
	    outfile_name=optarg;
	    oflag = 1;
	    break;
	case 'v':
	    vflag=1;
	    break;
	default: Usage();return 0;
	}
    }
    if(oflag&&vflag){Usage();return 0;}/*both switches are used*/
	if(vflag) {
	fputs("Log Merger for the TTCN-3 Test Executor\n"
	    "Product number: " PRODUCT_NUMBER "\n"
	    "Build date: " __DATE__ " " __TIME__ "\n"
	    "Compiled with: " C_COMPILER_VERSION "\n\n"
	    COPYRIGHT_STRING "\n\n", stderr);
#ifdef LICENSE
	print_license_info();
#endif
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
	fputs("The license key does not allow the merging of log files.\n",
	    stderr);
	return 2;
    }
    free_license(&lstr);
    free_openssl();
#endif
    argc-=optind-1;argv+=optind-1;
    if(argc<2){Usage();return 0;}/*executed when no input file is given*/
    for(a=1;a<argc;a++) {/*find first file with a valid timestamp*/
	TimeStampUsed=GetTimeStampFormat(argv[a]);
	if(TimeStampUsed!=TSF_Undefined)break;
    }
    switch(TimeStampUsed) {
	case TSF_Seconds: fputs("Merging logs with timestamp "
	    "format \"seconds\" has no sense.\n", stderr); return 0;
	case TSF_Time: TimeStampLength=TIMELENGTH;break;
	case TSF_DateTime: TimeStampLength=DATETIMELENGTH;break;
	default: fputs("Unsupported timestamp format.\n", stderr); return 1;
    }
    for(a=1,c=0;a<argc;a++) {/*get files with valid timestamp format*/
	b=GetTimeStampFormat(argv[a]);
	if(TimeStampUsed==b) {/*file conains at least one valid timestamp*/
	    c++;
	    name_list_in=(char **)Realloc(name_list_in,c*sizeof(char *));
	    name_list_in[c-1] = mcopystr(argv[a]);
	} else if(b==TSF_Undefined)/*file contains no timestamp or uses a
					different format than the first match*/
		fprintf(stderr,"Warning: unknown format in %s\n",argv[a]);
	    else fprintf(stderr,"Warning: format mismatch in %s\n",argv[a]);
    }
    num_allfiles=c;
    if(num_allfiles<1){Usage();return 0;}/*no valid log file found*/
    if(oflag){/*switch [-o outfile] is used -> create outfile*/
	outfile = fopen(outfile_name, FOPEN_WRITE);
	if(outfile==NULL) {
	    fprintf(stderr,"Error creating %s %s\n",outfile_name,strerror(errno));
	    return 1;
	}
    } else {
	outfile = stdout;
    }
    while(1) {
	filename_count=num_allfiles;start_file=0;
	while(num_allfiles>0) {/*process files in name_list_in*/
	    processed_files=OpenMaxFiles(num_allfiles,name_list_in+start_file);
	    must_use_temp=True;/*if there are infiles remaining use tempfiles
				for all*/
	    if((processed_files<2)&&(num_allfiles>1)){fprintf(stderr,"Error: "
		"can not open enough files.\nMore descriptors required "
		"(set with the command `limit descriptors\')\n");return 1;}
	    if(infiles_processed==True)
		for(a=0;a<processed_files;a++) {
		    Free(EventList[a]->str_to_add);
		    EventList[a]->str_to_add = NULL;
		}
	    num_allfiles-=processed_files;
	    ProcessOpenFiles();
	    CloseAllFiles();
	    start_file+=processed_files;
	}
	must_use_temp=False;/*all infiles processed*/
	/*remove temporary files used in previous step*/
	if(infiles_processed==True)
	    for(a=0;a<filename_count;a++)remove(name_list_in[a]);
	infiles_processed=True;
	for(a=0;a<filename_count;a++)Free(name_list_in[a]);
	Free(name_list_in);
	if(num_tempfiles==0)break;/*no more file to process*/
	name_list_in=temp_file_list;/*process tempfiles*/
	num_allfiles=num_tempfiles;
	num_tempfiles=0;temp_file_list=NULL;
    }
    check_mem_leak(progname);
    return 0;
}

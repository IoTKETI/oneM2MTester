###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baji, Laszlo
#   Balasko, Jeno
#
###############################################################################
#!/usr/bin/perl
#############################################################
# The gdb cannot load the symbol table for compiler. That is why this script was written.                   
# This script creates logs for the stack.                                                                                           
# The following parameters can be used:                                                                                       
# 1st parameter  : ON or OFF. ON makes the changes for Makefiles and creates the logger C++ files.
#                         The OFF sets mandatory the -R flag. The cleaning is always recursive.  
# 2nd parameter : -R. Optional. If set the search for files is recursive in directory structure.
# 3d parameter   :  a directory or file name under titan. This sript now is only for compiler2.
# 4th parameter  :  -L main  or -L all. Optional. Omitting it, the all is used default. 
#                          The main logs only between the program function main entry and exit.
# The following parameters shall be set to restore all the changes: OFF -R subdir
# where subdir is a directory name directly under titan.
# The logfile is placed into ../titan/stacklogger directory. The format: stack_logger_MM_DD_YYYY.log.
# The date is the one when the script generated the changes in the source code.
# examples:
#   perl stacklogger.pl ON -R compiler2 -L main 
#   perl stacklogger.pl ON -R compiler2 -L all
#   perl stacklogger.pl OFF -R compiler2  
##############################################################
use 5.010;
use strict;
use warnings;
use Cwd;
use File::Copy;
use File::Basename;
use POSIX qw(strftime);

my $topdir;
my $path;
my $loggerdir;
my $pathtomain;
my $set = 0; # default off
my $recursive = 0; # default non recursive iteration
my $loglevel = 1; # this means: -L all
my $cwd = cwd();
#weird name not to macth for any string in C++ file
my $macroname = "STACK_LOGGER_2014_BL67";
my $loggerdirname = "stacklogger";

&processArgs ( \$topdir, \$path, \$loggerdir, \$pathtomain, \$set, \$recursive, \$loglevel );
my @dirs;
&getDirs ( $path, \@dirs );

my @files;
&getFiles ( $path, \@files );

if ( $set )
{
    &setLogger;
}
else
{
    &resetLogger;
}
chdir $cwd;
############################################################
sub setLogger # switches the logger on
{
    &add_objects_to_target_makes ( @dirs );
    &add_dir_to_top_make;
    &create_logger_files ( $loggerdir );
    &insert_include ( @files );
    &insert_logger_macro ( @files );
}
############################################################
sub resetLogger # switches the logger off 
{
    &remove_logger_macro ( @files );
    &remove_include ( @files );
    &delete_logger_files ( $loggerdir );
    &restore_make ( $path ); 
    &delete_dep_files ( $path );
    if ( -d $loggerdir && ( $pathtomain eq $path )) { rmdir $loggerdir or die ( "$loggerdir cannot be deleted\n"); }
}
############################################################
sub insert_logger_macro # inserts the logger macro in the files
{ 
    my @files = @_; #all the files where the logger macros are to be written
    for my $file (@files)
    { 
        remove_logger_macro ( $file );
        open ( FILE, "<", $file ) or die ( "failed to open file: $file\n" );
        my @lines = <FILE>;
        close FILE ;
        my $arrSize = scalar @lines;
        my $countline = 0;
        my $moreline = "";
 
        open ( FILE, ">", $file ) or die ( "failed to open file: $file\n" );
        for my $line (@lines) 
        {
            my $temp = $line; 
            # regex for for '::' 
            my $regex1 = qr/.*[:]{2}.*/s;
            my $found =  $line =~ $regex1;
            if ( $found  || ( $moreline ne "" )) 
            {
                $moreline = append_and_check ( $moreline, $line );
            }
            else 
            {
                print FILE $line;
            }
        }
        close FILE;
    }
}
############################################################
sub remove_logger_macro # removes the logger macros from the files
{ 
    my @files = @_; #all the files where the logger macros are to be written
    for my $file (@files)
    { 
        open ( FILE, "<", $file ) or die ( "failed to open file: $file\n" );
        my @lines = <FILE>;
        close FILE ;
        my $arrSize = scalar @lines;
 
        open ( FILE, ">", $file ) or die ( "failed to open file: $file\n" );
        for my $line (@lines) 
        {
            $line =~ s/$macroname//;
            print FILE $line;
        }
        close FILE;
    }
}
############################################################
sub append_and_check # appends the next line to $moreline if the pattern was not found
{  # it searches through more lines
    my $origline = $_[1]; # the next line to be examined and/or append
    # remove C++ style comment
    my $cppcomment = qr/\/{2}.*/s;
    my $ccomment = qr/\/\*.*\*\//s;
    # valid function definition
    my $regex2 = qr/.*[:]{2}[A-Za-z0-9_~\s]+[(].*[)][^;}]*[{]/s;
    # found ';' it cannot be a function definition
    my $regex3 = qr/.*[:]{2}.+[;].*/s;
    chomp ($_[1]);
    my $append = $_[1];
    # remove cpp comment
    $append =~ s/$cppcomment//;
    my $string = $_[0] . $append;
    # remove c comment
    $string =~ s/$ccomment//; 
    my $found = ($string =~ $regex2);
    if ( $found )
    {
       $string = "";
       $origline =~ s/\{/\{$macroname/;
     }
    print FILE $origline;
    my $invalid = ($string =~ $regex3);
    if ( $invalid )
    {
        $string = "";
    }
    return $string;
}
############################################################
sub add_dir_to_top_make # the directory of the stacklogger will be added to Makefile in the {top} dir 
{
    my $replace = ":= " . $loggerdirname . " ";
    # the compiler2 pattern is fix the stacklogger shall be written in this line to the first place
    #ALLDIRS := **insert here** common compiler2 repgen xsdconvert
    my $search =  	qr/^[\s]*ALLDIRS[\s]*:=.*compiler2/s;
    my $makefile = $topdir . "Makefile";
    open ( FILE,  "<", $makefile ) or die ( "failed to open file: $makefile\n" );
    my @list = <FILE>;
    close FILE;
    my @found = grep /$loggerdirname/, @list;
    if ( @found == 0 ) 
    {
         my $newfile = $topdir . "Makefile.orig";
         if ( -f  $newfile )  { unlink $newfile; }  
        copy ( $makefile, $newfile ) or die ( "File cannot be copied." );
        open ( FILE, ">", $makefile ) or die ( "failed to open file: $makefile\n" );
        my $count = 0;
        for  my $i ( 0 .. $#list )
        {
            if ( $list[$i] =~ $search )  
            { 
                $list[$i] =~ s/:=/$replace/; 
                $list[$i] =~ s/ {2,}/ /; # 2 spaces are replaced to 1, if there is any
                $count += 1;
            }
            print FILE $list[$i];
        }
        close FILE;
        if ( $count != 1) { die ( "The Makefile in $topdir seems to have been changed.\nPlease redesign the regex in sub add_dir_to_top_make.\n" ); }
    }
}
############################################################
sub add_objects_to_target_makes #changes the Makefiles to be compiled
{
    my @dirs = @_; # the target and its subdirectories
    if ( ! grep  {/$pathtomain$/} @dirs ) 
    {
        push  ( @dirs, $pathtomain );
    }
    my $date = strftime "%m_%d_%Y", localtime;
    my $object = ".o";
    my $objectfilename = "stack_logger_" . $date . $object;
    my $insert = "
CPPFLAGS += -I$loggerdir
COMPILER_COMMON_OBJECTS += $loggerdir/$objectfilename
MFGEN_COMMON_OBJECTS += $loggerdir/$objectfilename
TCOV2LCOV_COMMON_OBJECTS += $loggerdir/$objectfilename
";
    my $insertsubdir = "
CPPFLAGS += -I$loggerdir
";
    my $search = qr/^SUBDIRS :=/s;
    for my $dir ( @dirs )
    {
        my $makefile = $dir . "Makefile";
        open ( FILE,  "<", $makefile ) or die ( "failed to open file: $makefile\n" );
        my @list = <FILE>;
        close FILE;
        my @found = grep /$loggerdir/, @list;
         if ( @found == 0 ) 
        {
            my $newfile = $dir . "Makefile.orig";
            if ( -f  $newfile )  { unlink $newfile; }  
            copy ( $makefile, $newfile ) or die ( "File cannot be copied." );
            open ( FILE, ">", $makefile ) or die ( "failed to open file: $makefile\n" );
            my $count = 0;
            for  my $i ( 0 .. $#list )
            {
                if ( $list[$i] =~ $search )  
                { 
                    if ( $dir eq $pathtomain )
                    {
                        print  FILE $insert ; 
                    }
                    else
                    {
                        print  FILE $insertsubdir ; 
                    }
                    $count += 1;
                } 
                print FILE $list[$i];
            }
            close FILE;
            if ( $count != 1) { die ( "The Makefile in $dir seems to have been changed.\nPlease redesign the regex in sub add_objects_to_target_makes.\n" ); }
        }
    }
}
############################################################
sub restore_make #restores the original Makefiles
{
    my $path = $_[0]; #path to target directory
     if ( $path eq $pathtomain ) 
    { 
        $recursive = 1;
        &getDirs ( $path, \@dirs ); 
    }
    else 
    {
         push ( @dirs, $path ); 
    }
    for my $dir ( @dirs ) 
    {
        my $origmakefile = $dir . "Makefile.orig";
        if ( -f  $origmakefile )  
        {
            my $makefile = $dir . "Makefile";
            unlink $makefile;
            rename  ( $origmakefile, $makefile ) or die ( "File cannot be renamed." );
        }
    }
    if ( $path eq $pathtomain ) 
    {
        &restore_make ( $topdir );
    }
}
############################################################
sub create_logger_files # creates the .hh the .cc and the Makefile for the stacklogger
{ # these files are C++ construct which at construction and destruction creates a logentry for the function contained
    my $dir = $_[0]; # path to directory where the logger files are to be created
    &delete_logger_files ( $dir );
    chdir $dir;
    my $guard = $macroname . "_H";
    my $logger_header = 
 "#ifndef $guard
#define $guard
#include <stdio.h>
#define $macroname EntryRaiiObject obj ## __LINE__ (__FUNCTION__ , __FILE__);
class EntryRaiiObject {
public:
  EntryRaiiObject (const char* func, const char* file);
  ~EntryRaiiObject ();
  int doLog ();
private:
    const char* func_;
    const char* file_;
    static unsigned int level;
};

class FH
{
public:
    static  FILE* getHandler ();
    ~FH ();
private:
    FH () {};
    FH (const FH&) {};
private:
    static FILE* fh;
    static const char* file_name;
};

#endif //$guard";

    my $date = strftime "%m_%d_%Y", localtime;
    my $headerfilename = "stack_logger_" . $date . ".hh";
    my $logfilename = "stack_logger_" . $date . ".log";
    open(FILE, ">", $headerfilename) or die ( "failed to open file: $headerfilename\n" );
    print FILE $logger_header;
    close FILE;
    my $printlog = "";
    if ( $loglevel == 0 ) 
    { 
        $printlog = "if (!doLog()) { return; }";
    }
    my $logger = 
"#include \"$headerfilename\" 
#include <stdlib.h> 
#include <string.h>
FILE* FH::fh = 0;
const char* FH::file_name = \"$dir/$logfilename\";
unsigned int EntryRaiiObject::level = 0;
EntryRaiiObject::EntryRaiiObject (const char* func, const char* file) : func_(func), file_(file)
{
    $printlog
    ++level;
    fprintf ( FH::getHandler(), \"%*u   %s - Entry - %s\\n\", level, level, func_, file_); 
    fflush (FH::getHandler()); 
}
EntryRaiiObject::~EntryRaiiObject () 
{ 
    $printlog
    fprintf( FH::getHandler(), \"%*u   %s - Exit  - %s\\n\", level, level, func_, file_);
    fflush (FH::getHandler()); 
    --level;
}
int EntryRaiiObject::doLog ()
{
    static int count = 0;
    if (strcmp ( func_ , \"main\") == 0)
    {
        if ( count % 2 )
        {
             return count++ % 2;
        }
        else
        {
            return ++count % 2;
        }
    }
    return  count % 2;
}

FILE* FH::getHandler ()
{
    if ( !fh)
    {
        fh = fopen (file_name,  \"w\");
        if ( !fh ) 
        {
            printf (\"Failed to open file: %s\", file_name);
            exit (1);
        }
    }
    return fh;
}

FH::~FH ()
{
    fclose (fh);
}
";
    my $extension = ".cc";
    my $bodyfilename = "stack_logger_" . $date . $extension;
    open ( FILE, ">", $bodyfilename ) or die ( "failed to open file: $bodyfilename\n" );
    print FILE $logger;
    close FILE;
    my $make =
"TOP := ..
include \$(TOP)/Makefile.cfg
SOURCES := $bodyfilename
HEADERS := \$(SOURCES:.cc=.hh)
OBJECTS := \$(SOURCES:.cc=.o)

all run : \$(OBJECTS)

install: all

include ../Makefile.genrules
";

    open ( FILE, ">", "Makefile" ) or die ( "failed to open file: Makefile\n" );
    print FILE $make;
    close FILE;
}
############################################################
sub insert_include # inserts the header file include of the stacklogger for every files touched
{
    my @files = @_; # all files for inserting the header
    # if found previuos  Include then update
    my $regex = qr/#include <stack_logger_*/;
    for  my $includefilenamein  (@files)
    {  
         my $date = strftime "%m_%d_%Y", localtime;
         my $includefilename = "stack_logger_" . $date . ".hh";
         my $include = "#include <$includefilename>\n";
         my $includefilenameout = "$includefilenamein.new" ;
         open ( IN, "<", $includefilenamein) or die ( "failed to open file: $includefilenamein\n" );
         open ( OUT, ">", $includefilenameout) or die ( "failed to open file: $includefilenameout\n" );
         my $first_line = <IN>;
         if (  $first_line !~ $regex  ) 
         {
             print OUT $include;
             print OUT $first_line;
         }
        else 
        {
             print OUT $include;
        }
        while( <IN> )
        {
            print OUT $_;
        }
        close IN;
        close OUT;
        unlink $includefilenamein;
        rename $includefilenameout, $includefilenamein;
    }
}
############################################################
sub remove_include # removes the header file include of the stacklogger from files
{
    my @files = @_; # all files where the include is to be removed from
    # if found previuos Include then update
    my $regex = qr/#include <stack_logger_*/;
    
    for  my $file  (@files)
    {  
         open ( FILE, "<", $file) or die ( "failed to open file: $file\n" );
         my @lines = <FILE>;
         close FILE;
         my $start = 0;    
         open ( FILE, ">", $file) or die ( "failed to open file: $file\n" );
         while (  $lines[$start] =~ $regex  ) 
         {
             $start += 1;
         }
        for my $i ( $start .. $#lines )
        {
            print FILE $lines[$i];
        }
        close FILE;
    }
}
############################################################
sub getFiles # collects the .cc files recursively from the path given
{
    my($path) = $_[0]; # the target path
     if ( -f $path ) 
     { 
         push ( @files, $path ); 
         return ;
     }
    #append a trailing / if it's not there 
    $path .= '/' if($path !~ /\/$/); 
    #loop through the files contained in the directory 
    for my $eachFile (glob($path . '*')) 
    { 
    # if the file is a directory 
       if ( -d $eachFile && $recursive )
       {
            # pass the directory to the routine 
            getFiles ( $eachFile, \@files );
       }
       else
       {
            my $regex = qr/.+\.cc$/s;
            my $cpp = ($eachFile =~ $regex);
            if ( $cpp ) 
            {
                push ( @files, $eachFile );
            }
       } 
    } 
} 
############################################################
sub getDirs # collects the directories recursively from the path given
{
    my($path) = $_[0]; # the target path
    if ( -f $path )
    {
        my $dir = dirname ( $path );
        $dir .= '/' if($dir !~ /\/$/); 
        push ( @dirs,  $dir );
        return;
    }
    #append a trailing / if it's not there 
    $path .= '/' if($path !~ /\/$/); 
    push ( @dirs, $path );
    #loop through the files contained in the directory 
    for my $eachFile (glob($path . '*')) 
    { 
    # if the file is a directory 
       if ( -d $eachFile && $recursive )
       {
            # pass the directory to the routine 
            getDirs ( $eachFile, \@dirs );
       }
    } 
}
############################################################
sub delete_dep_files # deletes the dependency files that breaks the recompilation
{
    my($path) = $_[0]; # the target path
     if ( -f $path ) { return; }
    #append a trailing / if it's not there 
    $path .= '/' if($path !~ /\/$/); 
    #loop through the files contained in the directory 
    for my $eachFile (glob($path . '*')) 
    { 
    # if the file is a directory 
       if ( -d $eachFile  )
       {
            # pass the directory to the routine 
            delete_dep_files ( $eachFile );
       }
       else
       {
            my $regex = qr/.+\.d$/s;
            my $dep = ($eachFile =~ $regex);
            if ( $dep ) 
            {
                 unlink $eachFile;
            }
       } 
    } 
} 
############################################################
sub delete_logger_files # removes the logger files and their directory
{
    if ( $path ne $pathtomain) { return;} 
    my ($logpath) = $_[0];
    $logpath .= '/' if($logpath !~ /\/$/); 
    for my $file (glob($logpath . '*')) 
    { 
       if ( -f $file )
       {
            unlink $file;
       }
   }
}
############################################################
sub getMainDir # defines the directory where the main.cc is to be found
{
    my $dir = $_[0] ; # the target directory
    $dir =~ s/\/$//; 
    my $enddir = $topdir;
    $enddir =~ s/\/$//; 
    if ( -f $dir ) { $dir = dirname ( $dir ); } # if it is file, get the containing dir
    while ( $enddir ne $dir )
    {
        opendir ( DIR, $dir ) or die $!;
        my @dir = grep { /main.cc/ } readdir DIR;
        if ( @dir == 0 )
        { 
            $dir = dirname ( $dir ) ; # get the parent
        }
        else
        {
            return $dir . "/";
        }
    }
    die ( "There was no main.cc in the path given and in their parent directories up to titan\n");
}
############################################################
sub processArgs  #( \$topdir, \$path, \$loggerdir, \$target, \$set, \$recursive, \$loglevel )
{
    my @messages;
    $messages[0] = "\nUsage: perl stacklogger.pl ON|OFF [ -R ]  compiler2 | pathto/file \n";
    $messages[1] = "ON switches on the stack logger\n";
    $messages[2] = "OFF removes the stack logger\n";
    $messages[3] = "-R optional: recursive handling the path given\n";
    $messages[4] = "/pathto/file accept only a filename with extension .cc\n";
    $messages[5] = "compiler2 accept only the directory name within titan\n";
    $messages[6] = "-L main or -L all, sets the log level\n";
    $messages[7] = "Correct usage of the ";
    my $numArgs = $#ARGV + 1;

    if (  $numArgs < 1  && $numArgs > 5 ) 
    {
        print @messages;
    }
    if   (( uc ( $ARGV[0] ) ne "ON" ) && ( uc ( $ARGV[0] ) ne "OFF" ))
    {
        print $messages[7]  . "first parameter :\n";
        print $messages[1];
        print $messages[2];
        die;
    }  

    if ((( $numArgs == 3 ) || ( $numArgs == 5 )) && ($ARGV[1]  ne  "-R" ))
    {
        print $messages[7]  . "second parameter :\n";
        print $messages[3] ;
        die;
    }

    if (( $numArgs > 3 ) && ( $ARGV[1]  eq  "-R" ) ||
         ( $numArgs > 2 ) && ( $ARGV[1]  ne  "-R" ))
     {
          my $logarg1 = 3;
          my $logarg2 = 4;
          if ( $ARGV[1]  ne  "-R" )
          {
              $logarg1 -= 1;
              $logarg2 -= 1;
          }
          if (( $ARGV[$logarg1] ne "-L" ) || (( uc ( $ARGV[$logarg2] ) ne "MAIN" ) && ( uc ( $ARGV[$logarg2] ) ne "ALL" )))
          {
               print $messages[7]  . "the log level :\n";
               print $messages[6];
               die;
          }
           if ( uc $ARGV[$logarg2] eq "MAIN" )
          {
               $loglevel = 0;
           }
     }

    if ( $numArgs > 1 )  
    {
        my $ttcn3_dir = $ENV{TTCN3_DIR};
        if ( defined ( $ttcn3_dir))
        {  # the top dir is ../titan/
            $ttcn3_dir  =~ s/.*\/titan\K.+//;
            $topdir = $path = $ttcn3_dir . "/";
            my $index = 2;
            if ( $ARGV[1]  ne  "-R" ) { $index -= 1; }
            $path = $path . $ARGV[$index] ;
            $path =~ s/\/\//\//;  # replace // to /
            unless ( -d  $path || -f $path ) { die ( "$path neither file nor directory\n" ) ;}
            if ( -d $path ) {$path .= "/"; }
            $pathtomain = &getMainDir ( $path );
        }
        else  { die ( "Error: environment variable TTCN3_DIR is not defined\n" ); }
    }
    
    if (( $numArgs >= 3 ) && ( $ARGV[1]  eq  "-R" ) && ( -f $path ))
    {
        die ( "recursive iteration is only for directories\n" );
    }
    
    $loggerdir = $topdir . $loggerdirname;
    if ( uc ( $ARGV[0] )  eq  "ON" ) 
    {
        $set = 1;
        unless ( -d  $loggerdir )  
        {   # Create a directory
            mkdir ( $loggerdir, 0770 ) or die  ("can't create: $loggerdir\n");  
        }
    }

     if (( $numArgs >= 3 ) && ( $ARGV[1]  eq  "-R" ))
     {
         $recursive = 1;
     }
}

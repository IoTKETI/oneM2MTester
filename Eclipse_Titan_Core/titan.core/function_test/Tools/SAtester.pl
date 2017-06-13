#!/usr/bin/perl -w
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Beres, Szabolcs
#   Delic, Adam
#   Kovacs, Ferenc
#   Lovassy, Arpad
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Endre
#
###############################################################################
##
##  File       :  SAtester.pl
##  Description:  Tester utility for Semantic Analyser, TITAN
##  Written by :  Endre Szalai (Endre.Szalai@ericsson.com)
##

## TODO: exit status always 0, investigate why; workaround: catch the notify
## printout from the compiler

require 5.6.1;

use strict;
use Getopt::Long;

###############################################################################
###                               Global Variables
###############################################################################
# Whether to stop on test case failures (1) or not (0)
my $sa_halt_on_errors = '';
# Whether to list available test cases or not
my $sa_list_TCs = 0;
# Whether to show info or not
my $sa_info = 0;
# Whether to use matching in test case selection
my $sa_tc_select = '';
# Name of the logfile
my $sa_logFile = '';
my $sa_LOG;
# Elapsed time in this session
my $sessionTime;
# Whether to show command line info or not
my $sa_printHelp_cmd = 0;
# Whether to show detailed info or not
my $sa_printHelp_doc = 0;
# Use function-test runtime or not
my $sa_titanRuntime2 = 0;
# Enable coverage or not
my $sa_coverageEnabled = 0;
# Files existed before a test case execution
my %sa_existedFiles;
# Store input TD files from which TCs are collected
my @sa_scriptFiles;
# Store information about the TCs to execute
my @sa_tcList2Execute;
# Store test case data
my @sa_TCInfo;
# Timeout for system calls in seconds
my $sa_timeout = 30;
# Max time to wait for a license, in multiple of 10 minutes
my $max_cycles = 6;
# Execution statistics
# Number of TCs: PASSED, FAILED, ERROR verdicts,
#                abnormally terminated, memory leaked
my @sa_executionStatistics = (0, 0, 0, 0, 0);
# Command to invoke the titan compiler
my $sa_compilerCmd;
# Command to invoke the titan Makefile generator
my $sa_mfgenCmd;
# Command to invoke the runtime execution
my $sa_runtimeCmd;
# commonly used regexps
my $sa_re_TCheader = "\\n\\s*<\\s*TC\\s*-\\s*(.+?)\\s*>\\s*\\n";
my $sa_re_MODULEheader = "\\n\\s*<\\s*MODULE\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*>";
my $sa_re_MODULEbody = "${sa_re_MODULEheader}\\s*(.+?)\\n\\s*<\\s*END_MODULE\\s*>";
my $sa_re_RESULTheader = "\\n\\s*<\\s*RESULT\\s*(IF_PASS|IF_FAIL)?\\s*(LTRT|FTRT)?\\s*(POSITIVE|NEGATIVE)?\\s*(?:COUNT\\s+(\\d+))?.*?>";
my $sa_re_RESULTbody = "${sa_re_RESULTheader}\\s*(.*?)\\s*\\n\\s*<\\s*END_RESULT\\s*>";
my $sa_re_EXPECTEDbody = "\\s*<\\s*VERDICT_LEAF\\s+(PASS|FAIL)\\s*>\\s*";
my $sa_re_MemoryLeak = "(?:unallocated\\s+blocks\\s*:\\s*)|(?:Memory\\s+leakage\\s+detected)";
# separator for printouts
my $sa_separator = "===============================================================\n";


# Detailed info about the usage
my $sa_detailedInfo = '
Purpose
-------
The tester program was written especially for testing the semantic analyser
functionality in TITAN. A generic test flow looks like:
- generate input sources (TTCN-3 and/or ASN.1)
- compile them with the TITAN compiler
- check that the error messages are as expected (negative testing)
- check that the modules are compiled both with TITAN and gcc
  (positive testing)
Test cases and all information needed to execute them are stored in one file, in
the Test Description files (*.script) in EDML format. The tester program uses
this file to execute the test according to the flow described above in a fully
automatic way..
Unlike in a simple test method, where a test may be passed or failed, in
regression test it might be important, why a test case is failed (e.g. due to a
not yet implemented feature). Therefore, each test case may have two separate
expected behaviour (called leaves later on). The first is the case when the test
purpose is expected to work in a specific way (IF_PASS leaf). The other, when
the test case is expected to fail, but why it fails is also interesting (IF_FAIL
leaf). Each test case may have both leaves and a specific selector points out
which leaf is expected to occur. This also means, that a test case passes, if
the selected leaf is passed (which may be the IF_FAIL leaf). Therefore, the
tester needs to check test cases that are failed, as only in those cases the
current result is not as expected.

Features
--------
The tester program has the following features:
1. Support for one-by-one and batched execution of test cases.
2. Support for unlimited number of input modules: ASN.1,
TTCN-3 and runtime config files for TITAN.
3. Support for compilation of the modules using TITAN and
GCC. It also supports single mode execution of the test suite.
Makefile and test port files generation is automatic.
5. Automatic cleanup after each test case.
6. Flexible pattern matching for the test case printout using
Perl regexps combined with different matching logic.
7. Support for regression testing process.

Reference
---------
For a list of command line options, execute the tester program with
SAtester.pl -help

A test case structure in the EDML files looks like:
test case block:
  <TC - test case name>
compile block:
  [<COMPILE|COMPILEGCC|EXECUTE|EXECUTE_PARALLEL>]   (default is COMPILE)
leaf selector block:
  <VERDICT_LEAF (PASS|FAIL)>
module block(s):
  <MODULE TTCN|ASN|CFG modulename filename>
    ... text from here is written to the filename specified above ...
  <END_MODULE>
  ... several module sections may follow ...
result block(s):
  <RESULT IF_PASS|IF_FAIL [POSITIVE|NEGATIVE|COUNT number]>   (default is POSITIVE)
    ... pattern in Perl regexp format ...
  <END_RESULT>
  ... several result sections may follow ...

  <END_TC>

Each block header/footer must be single-line, i.e. newline is not allowed.

The compile block instructs the SAtester to:
  COMPILE    compile the modules using TITAN only
  COMPILEGCC compile the modules using TITAN and GCC afterwards
             (the Makefile is automatically generated)
  EXECUTE    after compilation, execute the executable test suite
             the first runtime configuration file is passed to TITAN
             (single mode execution)
  EXECUTE_PARALLEL  after compilation, execute the executable test suite
             (parallel mode execution)
If any of the actions fail, the test case result will be "ERROR". This block is
optional, the default value is COMPILE.

The leaf selector block helps the regression testing process, where not only the
verdict itself, but the expected verdict is interesting. The value may be PASS
or FAIL. The leaf selector selects which RESULT blocks to use when matching the
printout from the compiler(s).

A module block instructs the SAtester to produce a source module
or a runtime configuration file:
  TTCN       The module is treated as a TTCN-3 module
  ASN        The module is treated as an ASN.1 module
  CFG        The module is treated as a runtime configuration file
  modulename Name of the module
  filename   Name of the file to be produced
The text within this section is written as is to "filename". You may specify as
many modules as you need.

A result block instructs the SAtester how to check result as soon the actions in
the compile block is finished successfully.
Either:
  POSITIVE   Indicates a positive pattern match (i.e. =~ syntax in Perl)
             with the supplied pattern is needed to continue
  NEGATIVE   Indicates a negative pattern match (i.e. !~ syntax in Perl)
             with the supplied pattern is needed to continue
Or:
  COUNT      Indicates that instead of a direct pattern match, perform
             a counting of the supplied pattern. If the number of pattern-
             matches equals to the supplied number in COUNT section, the
             execution continues
  POSITIVE or NEGATIVE can be used for simple pattern match, i.e. whether the
  pattern is present or not.
  COUNT can be used to detect that a pattern how many times are present.
Result blocks are evaluated in the order of their appearence. If any of the
result block is failed, the verdict will be "FAILED" and execution continues
with the next test case (if any). Entries in the result header are optional, the
default value is "POSITIVE".

Memory leak printouts are automatically detected. If one is found, the current
verdict will be "FAIL".

Example
-------
//line comments are written for understanding only

// Name of the test case
<TC - TTCN-3::Subtypes, list of values: Anytype>
// compile with TITAN only
<COMPILE>
// "anytype" is not supported, so use the FAIL-leaf
// in result matching
<VERDICT_LEAF FAIL>
// Specify one module
<MODULE TTCN ModuleA ModuleA.ttcn>
module ModuleA {
   type anytype MyType ( 10, 11.0, Nonexi, false );
}
<END_MODULE>
// PASS-leaf of the result matching
// used if VERDICT_LEAF is "PASS"
// expecting exactly 1 "no imported or local definition nonexi"
<RESULT IF_PASS COUNT 1>
(?im)\berror\b.+?no.+?definition.+?nonexi
<END_RESULT>
// expecting exactly 1 "error", so no other error shall
// occur than the previous error
<RESULT IF_PASS COUNT 1>
(?is)\berror\b
<END_RESULT>
// expecting a notification that no code is generated due
// to the error
<RESULT IF_PASS POSITIVE>
(?im)\bnotify\b.+?\bcode\b.+?\bnot\b.+?\bgenerated\b
<END_RESULT>
// FAIL-leaf of the result matching
// used if VERDICT_LEAF is "FAIL"
// expecting exactly 1 "parse error"
<RESULT IF_FAIL COUNT 1>
(?im)parse.+?error
<END_RESULT>
// expecting exactly 1 "error", so no other error shall
// occur than the parse error
<RESULT IF_FAIL COUNT 1>
(?is)\berror\b
<END_RESULT>
<END_TC>

Information about Perl regular expressions can be found at:
www.perl.com

Guides
------
1. Use the IF_FAIL leaf only, if the test case is expected to fail because of a
missing functionality that will not be implemented in the current project. Using
IF_FAIL leaves for limitations that are expected to disappear within a project
is just an unnecessary overhead.

2. Whenever a TR is issued, this should be mentioned in the test case, within
the source module where the error occured, e.g.
<MODULE TTCN ModuleA ModuleA.ttcn>
module ModuleA {
   // TR 623: length restriction of charstrings
   type record of charstring MyType7 length(0..666-Nonexi);
}
<END_MODULE>

3. Always expect a specific error message, but be a bit flexible in pattern
matching. Always check that no other errors occured. E.g.:
<RESULT IF_PASS COUNT 1>
(?im)\berror\b.+?no.+?definition.+?nonexi
<END_RESULT>
<RESULT IF_PASS COUNT 1>
(?is)\berror\b
<END_RESULT>
<RESULT IF_PASS POSITIVE>
(?im)\bnotify\b.+?\bcode\b.+?\bnot\b.+?\bgenerated\b
<END_RESULT>
Note the non-case sensitive matching; that only 1 error is expected and that no
code is expected to be generated.

Known issues
------------
On cygwin, fork does not always work and Perl stops with an error like:
264 [main] perl 2216 fork_copy: linked dll data/bss pass 0 failed,
0x412000..0x412370, done 0, windows pid 1832, Win32 error 487

';




# easterEgg
my $sa_egg = '
                     \\\\\\\\|////
                    \\\\  - -  //
                    #(  @ @  )#
                      \  o  /
                       \ * /
---------------------oOOo--oOOo-------------------------
       "Do or do not. There is no try." - Yoda
------------------------ooooO---Ooooo-------------------
                        (   )   (   )
                         \  |   |  /
                          (_|   |_)
    -- This amazing code was created by McHalls --

';


###############################################################################
##                                  Subs
###############################################################################
sub sa_commandLineInfo();
sub sa_log($);
sub sa_processArgs($);
sub sa_processCommandLine();
sub sa_readFile($);
sub sa_writeFile($$);
sub sa_collectTCs();
sub sa_isInList(\@$);
sub sa_getTCInfo(\@$$);
sub sa_parseTCs();
sub sa_writeModulesOfTC(\@);
sub sa_deleteModulesOfTC(\@);
sub sa_fetchExecutableName();
sub sa_executeCommand($);
sub sa_compileTC(\@);
sub sa_printResult($$);
sub sa_checkTCResult(\@$);
sub sa_executeTCs();

# Print command line information
sub sa_commandLineInfo () {
   sa_log("\nPerl utility to execute test cases for Semantic Analyser\n");
   sa_log("Contact: Endre Szalai (Endre.Szalai\@ericsson.com)\n");
   sa_log("Usage: SA_tester.pl\n");
   sa_log("       [-halt] [-list] [-help] [-doc] [-rt2] [-coverage]\n");
   sa_log("       [-select <pattern>]\n");
   sa_log("       [-timeout <timeout>]\n");
   sa_log("       [<TDfile1.script>] ... [<TDfileN.script>]\n");
   sa_log("       [\"<test case name1>\"] ... [\"<test case nameM>\"]\n");
   sa_log("       [-log <logfilename>]\n");
   sa_log("Where\n");
   sa_log(" -halt            halt on any errors\n");
   sa_log(" -list            list available test cases\n");
   sa_log(" -help            display command line parameters\n");
   sa_log(" -doc             display complete documentation\n");
   sa_log(" -rt2             use function-test runtime\n");
   sa_log(" -coverage        enable coverage");
   sa_log(" <pattern>        select test cases if pattern is present in the\n");
   sa_log("                  name of the test case\n");
   sa_log(" <timeout>        maximum execution time of a system call\n");
   sa_log("                  in seconds (default is $sa_timeout)\n");
   sa_log(" <logfilename>    name of the logfile\n");
   sa_log("                  NOTE: do NOT use the .log file extension !\n");
   sa_log(" <TDfile.script>  name of the TD file(s) to collect test cases from\n");
   sa_log(" <test case name> name of the test case(s) to execute\n");
   sa_log("If no TDfile(s) are defined, all script files from the current\n");
   sa_log("directory are considered.\n");
   sa_log("If no test cases are defined, all test cases from the script files\n");
   sa_log("are executed.\n\n");
}

# Log entries
sub sa_log($) {
   print $_[0];
   if (defined($sa_LOG)) { print $sa_LOG $_[0]; }
}

# Process the command line
sub sa_processArgs($) {
   if ($_[0] =~ /\.script$/m) { $sa_scriptFiles[scalar @sa_scriptFiles] = $_[0]; }
   else { $sa_tcList2Execute[scalar @sa_tcList2Execute] = $_[0]; }
}
sub sa_processCommandLine() {

   die unless (GetOptions('halt'      => \$sa_halt_on_errors,
                          'list'      => \$sa_list_TCs,
                          'credit'    => \$sa_info,
                          'help'      => \$sa_printHelp_cmd,
                          'doc'       => \$sa_printHelp_doc,
                          'rt2'       => \$sa_titanRuntime2,
                          'coverage'  => \$sa_coverageEnabled,
                          'log=s'     => \$sa_logFile,
                          'timeout=s' => \$sa_timeout,
                          'select=s'  => \$sa_tc_select,
                          'maxcycle=s'=> \$max_cycles,
                          "<>"        => \&sa_processArgs));
   if ($sa_info) {print $sa_egg; exit(1);}
   if ($sa_logFile ne '') {
      die "Never use '.log' extension as TITAN's cleanup may remove it, specify a different name\n"
         unless ($sa_logFile !~ /.*\.log$/is);
      open ($sa_LOG, ">$sa_logFile") or die "Cannot open log file: '$sa_logFile': $!\n";}
   if ($sa_printHelp_cmd) {sa_commandLineInfo(); exit(1);}
   if ($sa_printHelp_doc) {sa_log ($sa_detailedInfo); exit(1);}
}

# Read and return the content of a file
sub sa_readFile($) {
   # $_[0] : name of the file
   my $Buffer;
   sa_log("Parsing file $_[0]...\n");
   open (TMP, "$_[0]") or die "Cannot open script file: '$_[0]': $!\n";
      read(TMP, $Buffer, -s "$_[0]");
   close (TMP);
   # remove carriage returns
   # Unix/windows: \n \r combo
   if ($Buffer =~ /\n/s) {$Buffer =~ s/\r//g;}
   # Mac: only \r
   else {$Buffer =~ s/\r/\n/g;}
   return $Buffer;
}

# Write a file
sub sa_writeFile($$) {
   # $_[0] : name of the file
   # $_[1] : content of the file
   sa_log("Flushing file $_[0]...\n");
   open (TMP, ">$_[0]") or die "Cannot open file: '$_[0]': $!\n";
      print(TMP $_[1]);
   close (TMP);
}

# Checks whether a TC is on the list
sub sa_isInList(\@$) {
   my $poi = $_[0];
   foreach my $val (@{$poi}) {
      if ($val eq $_[1]) {return 1;}
   }
   return 0;
}
# Collect test cases from command line and/or script files
sub sa_collectTCs() {
   # No script files are defined, collect all script files from
   # current directory
   if (scalar @sa_scriptFiles == 0) {
      my @list = split (/\s/, `ls *.script`);
      foreach my $filename (@list) { $sa_scriptFiles[scalar @sa_scriptFiles] = $filename; }
   }
   # no test cases specified, collect from available script files
   if (scalar @sa_tcList2Execute == 0) {
      foreach my $filename (@sa_scriptFiles) {
         my $Buffer = sa_readFile($filename);
         while ($Buffer =~ s/^.*?${sa_re_TCheader}//s) {
            my $tcName = $1;
            if (sa_isInList(@sa_tcList2Execute,$tcName))
              	{sa_log( "WARNING: Test case name is not unique: '$tcName'\n");}
            # execute test case if match with pattern or no pattern available
            if ((not $sa_tc_select) or
                ($sa_tc_select and ($tcName =~ /$sa_tc_select/))) {
               $sa_tcList2Execute[scalar @sa_tcList2Execute] = $tcName;
            }
         }
      }
   }
   my $sa_nrOfTCs2Execute = scalar @sa_tcList2Execute;
   my $nrOfScriptFiles = scalar @sa_scriptFiles;
   sa_log( "$sa_nrOfTCs2Execute test cases from $nrOfScriptFiles script files to be executed\n");
   my $tmp = ($sa_halt_on_errors) ? "halting on errors\n" : "ignoring errors\n";
   sa_log( "Mode: $tmp");
}


# Gather execution information according to:
# [N] Nth test case
#   [0] test case name
#   [1] "COMPILE", "COMPILEGCC" or "EXECUTE"
#   [2][N] Nth module
#        [0] module type ("ASN", "TTCN" or "CFG")
#        [1] module name (if ASN or TTCN)/execution mode (if CFG)
#        [2] filename to use for the module
#        [3] content of the module
#   [3]{'PASS'|'FAIL'}[N] Nth result for expected result leaf PASS|FAIL
#        [0] result type ("POSITIVE" or "NEGATIVE")
#        [1] match expression
#        [2] number of coccurances (if "COUNT" used in RESULT)
#   [4] name of the source script file
#   [5] expected test case result leaf selector (PASS, FAIL)
#   [6] verdict of the TC
#   [7] flag whether memory leak is detected or not (defined/not defined)
#   [8] flag whether abnormal termination is detected or not (defined/not defined)
sub sa_getTCInfo(\@$$) {
   my ($poi, $Buffer, $filename) = @_;
   my $tcidx = scalar @{$poi};
   if ($Buffer =~ s/${sa_re_TCheader}//s) { $poi->[$tcidx][0] = $1; }
   else { die "ERROR: Cannot find test case name in current block\n"; }
   if ($Buffer =~ s/<\s*EXECUTE\s*>//m) { $poi->[$tcidx][1] = 'EXECUTE'; }
   elsif ($Buffer =~ s/<\s*EXECUTE_PARALLEL\s*>//m) { $poi->[$tcidx][1] = 'EXECUTE_PARALLEL'; }
   elsif ($Buffer =~ s/<\s*COMPILEGCC\s*>//m) { $poi->[$tcidx][1] = 'COMPILEGCC'; }
   else { $poi->[$tcidx][1] = 'COMPILE'; }
   if ($Buffer =~ /${sa_re_EXPECTEDbody}/m) { $poi->[$tcidx][5] = $1; }
   else { $poi->[$tcidx][5] = 'PASS'; }
   my $idx = 0;
   while ($Buffer =~ s/${sa_re_MODULEbody}//s) {
      $poi->[$tcidx][2][$idx][0] = $1;
      $poi->[$tcidx][2][$idx][1] = $2;
      $poi->[$tcidx][2][$idx][2] = $3;
      $poi->[$tcidx][2][$idx][3] = $4;
      $idx++;
   }
   while ($Buffer =~ s/${sa_re_RESULTbody}//s) {
      my $expectedVerdict;
      if (defined($1)) {
         $expectedVerdict = ($1 eq 'IF_FAIL') ? 'FAIL':'PASS';
      } else { $expectedVerdict = 'PASS'; }
      my $idx = (defined($poi->[$tcidx][3]{$expectedVerdict}))
               ? scalar @{$poi->[$tcidx][3]{$expectedVerdict}} : 0;
      if (defined($2)) {
        # Skip matching strings intended for the load-test run-time when
        # "-rt2" is used.  Vice versa.
        if ($2 eq 'LTRT' and $sa_titanRuntime2
            or $2 eq 'FTRT' and not $sa_titanRuntime2) { next; }
      }
      if (defined($3)) {
         $poi->[$tcidx][3]{$expectedVerdict}[$idx][0] = ($3 ne '') ? $3 : 'POSITIVE';}
      else {$poi->[$tcidx][3]{$expectedVerdict}[$idx][0] = 'POSITIVE';}
      if (defined($4)) { $poi->[$tcidx][3]{$expectedVerdict}[$idx][2] = $4; }
      $poi->[$tcidx][3]{$expectedVerdict}[$idx][1] = $5;
      eval { '' =~ /$poi->[$tcidx][3]{$expectedVerdict}[$idx][1]/ };
      if ($@) {
         sa_log("In file $filename, test case '$poi->[$tcidx][0]'\n");
         sa_log "  Syntax error in provided pattern:\n  $poi->[$tcidx][3]{$expectedVerdict}[$idx][1]\n$@\n";
         exit(1);
      }
   }
   $poi->[$tcidx][4] = $filename;
   $poi->[$tcidx][6] = 'NONE';
}
# Parse test case data
sub sa_parseTCs() {
   # give only a list of the test cases
   if ($sa_list_TCs) {
      sa_log( "Collected test cases:\n");
      my $idx = 1;
      # collect test case data from all specified script files
      foreach my $filename (@sa_scriptFiles) {
         my $Buffer = sa_readFile($filename);
         while ($Buffer =~ s/^.*?(${sa_re_TCheader}.+?<END_TC>)//s) {
            sa_getTCInfo(@sa_TCInfo, $1, $filename);
         }
      }
      foreach my $poi (@sa_TCInfo) {
         sa_log( "$idx.  '$poi->[0]'     \n");
         sa_log( "    source:$poi->[4];   leaf:$poi->[5]\n");
         $idx++;
      }
      exit(1);
   }
   foreach my $filename (@sa_scriptFiles) {
      my $Buffer = sa_readFile($filename);
      while ($Buffer =~ s/^.*?(${sa_re_TCheader}.+?<END_TC>)//s) {
         if (not sa_isInList(@sa_tcList2Execute,$2)) { next; }
         sa_getTCInfo(@sa_TCInfo, $1, $filename);
      }
   }
}

# Writes the modules of the TC
sub sa_writeModulesOfTC(\@) {
   my $root = $_[0];
   foreach my $poi (@{$root->[2]}) {
      open(TMP, "> $poi->[2]") or die "Can't create file '$poi->[2]': $!\n";
      sa_log("Module $poi->[1] (file $poi->[2]):\n");
      sa_log("$poi->[3]\n");
      print TMP $poi->[3] . "\n";
      close (TMP);
   }
}
# Deletes the files(modules) of the TC
sub sa_deleteModulesOfTC(\@) {
   my $root = $_[0];
   sa_log( "Cleanup... ");
   if (-f 'Makefile') {
    my $makefile;
    `make clean 2>&1`;
   $makefile = sa_readFile('Makefile');
   while ($makefile =~ s/^(\s*USER_SOURCES\s*=\s*)(\w+\.cc)/$1/im) {
      if (not exists($sa_existedFiles{$2})) {
         if (unlink $2) {sa_log( "$2 ");}
      }
   }
   while ($makefile =~ s/^(\s*USER_HEADERS\s*=\s*)(\w+\.hh)/$1/im) {
      if (not exists($sa_existedFiles{$2})) {
         if (unlink $2) {sa_log( "$2 ");}
      }
   }
   }
   if (unlink "Makefile") {sa_log( "Makefile ");}
   foreach my $poi (@{$root->[2]}) {
      if (unlink $poi->[2]) {sa_log( "$poi->[2] ");}
      # remove possible generated code
      if ($poi->[0] eq 'TTCN') {
         if (unlink "$poi->[1].cc") {sa_log( "$poi->[1].cc ");}
         if (unlink "$poi->[1].hh") {sa_log( "$poi->[1].hh ");}
      } else {
         my $tmp = $poi->[1];
         $tmp =~ s/\-/_/g;
         if (unlink "$tmp.cc") {sa_log( "$tmp.cc ");}
         if (unlink "$tmp.hh") {sa_log( "$tmp.hh ");}
      }
   }
   sa_log("\n");
}

# Fetch the executable name from Makefile
sub sa_fetchExecutableName() {
   my $makefile = sa_readFile('Makefile');
   if ($makefile =~ /^\s*EXECUTABLE\s*=\s*(\w+)/im) { return $1; }
   die "ERROR: Executable name is not found in generated Makefile\n";
}

# Perform a system call
# return value: (status, output)
# status = 0 on success
# status = 1 on command timeout
# status = 2 on abnormal termination
# output: collected printout from the process
sub sa_executeCommand($) {
   # Silly Perl does not allow to capture STDERR, only STDOUT.
   # As a workaround, ask the shell to redirect STDERR to STDOUT.
   # It's OK until TITAN writes everything to STDERR (as it is today)
   my $command = "$_[0] 2>&1";
   my $subRes = "";
   my $pid;
   my $exitStatus = 0;
   sa_log( "$_[0]\n");
   # run as a separate Perl program to be able to use timeout
   eval {
      local $SIG{ALRM} = sub { die "timeout\n" }; # NB: \n required
      alarm $sa_timeout; # send an alarm signal in specified time
      # call via exec to be able to catch the exit status
      # (without exec, the shell always return with 0 exit status)
      $pid = open(TMP, "exec $command |");
      die "ERROR: Cannot fork: $!\n" unless ($pid);
         while (<TMP>) { $subRes .= $_; }
      close(TMP);
      $exitStatus = $? & 128; # whether core dump occured
      alarm 0;
   };
   sa_log("$subRes\n");
   if ($@) { # an error occured (thought, not only at timeout)
      sa_log("\nSoftly killing forked process $pid\n");
      kill 1, $pid;
      sa_log("ERROR: system call '$command' is not finished within $sa_timeout seconds\n");
      return (1, $subRes);
   }
   if ($exitStatus) { # core dump occured
      sa_log("\nERROR: system call '$command' is terminated abnormally\n");
      return (2, $subRes);
   }
   return (0, $subRes);
}

# Compile the stuff now
# return value: (status, output)
# status = 0 on success
# status = 1 on ERROR
# status = 2 on test case FAIL (compiler related ERROR)
sub sa_compileTC(\@) {
   my $root = $_[0];
   my $modules2compile = '';
   my $configFile = '';
   my $subRes = '';
   my $res;
   my $cycles = 0;
   foreach my $poi (@{$root->[2]}) {
      if ($poi->[0] eq "CFG") {$configFile = $poi->[2]; next;}
      $modules2compile .= " $poi->[2]";
   }
   if ($modules2compile eq '') {
      sa_log( "WARNING: test case '$root->[0]' does not contain any modules, skipping\n");
      return (1, '');
   }
   sa_log( "Compiling sources...\n");
   my $runtimeOption = '';
   if ($sa_titanRuntime2) { $runtimeOption = '-R'; }
   do {
      if ($cycles) { sleep(60 * 10); }
      $cycles++;
      ($res, $subRes) = sa_executeCommand("$sa_compilerCmd $runtimeOption $modules2compile");
      # Purify message, when no floating license available. Sleep for a while
      # and retry.
      # -continue-without-license=yes
   } while (($subRes =~ /\-continue\-without\-license\=yes/mi) and ($cycles <= $max_cycles));
   if ($res) { return (2, $subRes); }
   my $resultBuffer = $subRes;
   if ($root->[1] eq "COMPILE") { return (0, $resultBuffer); }

   sa_log( "Generating test port skeletons...\n");
   ($res, $subRes) = sa_executeCommand("$sa_compilerCmd $runtimeOption -t -f $modules2compile");
   if ($res) { return (1, $subRes); }
   $resultBuffer .= $subRes;

   sa_log( "Generating Makefile...\n");
   ($res, $subRes) = sa_executeCommand("$sa_mfgenCmd $runtimeOption -s -f $modules2compile *.cc*");
   if ($res) { return (1, $subRes); }
   $resultBuffer .= $subRes;
   if (not (-e 'Makefile')) {
      sa_log( "ERROR: Makefile could not be generated\n");
      sa_log ($subRes);
      return (1, $subRes);
   }
   if ($root->[1] eq "EXECUTE_PARALLEL") {
     sa_log("Patching Makefile for parallel mode...\n");
     my $Buffer = sa_readFile('Makefile');
     $Buffer =~ s/^\s*TTCN3_LIB\s*=\s*(ttcn3\S*)\s*$/TTCN3_LIB = $1-parallel/im;
     unlink 'Makefile';
     sa_writeFile('Makefile', $Buffer);
   }
   sa_log("Building...\n");
   my $coverage_args = $sa_coverageEnabled ? "CXXFLAGS=\"-fprofile-arcs -ftest-coverage -g\" LDFLAGS=\"-fprofile-arcs -ftest-coverage -g -lgcov\"" : "";
   ($res, $subRes) = sa_executeCommand("make " . $coverage_args);
   if ($res) { return (1, $subRes); }
   $resultBuffer .= $subRes;
   my $exeName = sa_fetchExecutableName();
   if (not (-e $exeName)) {
      sa_log( "ERROR: GCC compilation error, no executable produced ('$exeName')\n");
      if ($sa_halt_on_errors) {
        sa_log( "\nTest execution interrupted (no cleanup).\n\n");
        exit(1);
      }

      return (1, $subRes);
   }
   if ($root->[1] eq "COMPILEGCC") { return (0, $resultBuffer); }

   # go on with execution
   sa_log("Fetched executable name: $exeName\n");
   if ($root->[1] eq "EXECUTE") {
		sa_log( "Executing in single mode...\n");
		if ($configFile eq '') {
	      sa_log( "ERROR: No runtime config file is specified in the test case\n");
	      return (1, $subRes);
	   }
	   ($res, $subRes) = sa_executeCommand("./$exeName $configFile");
	   if ($res) { return (1, $subRes); }
	   $resultBuffer .= $subRes;
	   return (0, $resultBuffer);
   } else {
	   sa_log( "Executing in parallel mode...\n");
           if ($configFile eq '') {
	     ($res, $subRes) = sa_executeCommand("$sa_runtimeCmd $exeName");
           } else {
             ($res, $subRes) = sa_executeCommand("$sa_runtimeCmd $exeName $configFile");
           }
	   if ($res) { return (1, $subRes); }
	   $resultBuffer .= $subRes;
	   return (0, $resultBuffer);
   }
}


# Print a test case result
# $_[0] name of the test case
# $_[1] verdict
sub sa_printResult($$) {
   sa_log( "\nTest case '$_[0]'   \n  '$_[1]'\n".$sa_separator);
}

# Check test case result based on RESULT lists
sub sa_checkTCResult(\@$) {
   my ($root, $resultBuffer) = @_;
   my $result = 'PASS';
   my $expectedResult = $root->[5];
   if (not defined($root->[3])) {
      sa_log( "ERROR: No RESULT section in test case '$root->[0]'\n");
      sa_printResult($root->[0], 'ERROR');
      $root->[6] = 'ERROR';
      return;
   }
   foreach my $poi (@{$root->[3]{$expectedResult}}) {
      # match with each result
      if ($poi->[1] !~ /\S/) {next;}
      if (($poi->[0] eq 'POSITIVE') and defined($poi->[2])) { # counter match
         my $counter = 0;
         my $tmpBuf = $resultBuffer;
         while ($tmpBuf =~ s/$poi->[1]//) { $counter++; }
         if ($counter != $poi->[2]) {
            sa_log( "Failing at counting: pattern '$poi->[1]'\nexpected '$poi->[2]' times, found '$counter' times\n");
            $result = 'FAIL'; last;
         } else { sa_log("Passed matching pattern (counting $counter times): '$poi->[1]'\n"); }
      } else { # simple pattern match
         if ($poi->[0] eq 'POSITIVE') {
            if ($resultBuffer !~ /$poi->[1]/si) {
               sa_log( "Failing at pattern (expected, not found):\n$poi->[1]\n");
               $result = 'FAIL'; last;
            } else { sa_log("Passed matching pattern (expected): '$poi->[1]'\n"); }
         } else { # NEGATIVE
            if ($resultBuffer =~ /$poi->[1]/si) {
               sa_log( "Failing at pattern (not expected but found):\n$poi->[1]\n");
               $result = 'FAIL'; last;
            } else { sa_log("Passed matching pattern (not expected): '$poi->[1]'\n"); }
         }
      }
   }
   # Check if there is any memory leak in compiler
   if ($resultBuffer =~ /${sa_re_MemoryLeak}/mi) {
      sa_log( "WARNING: Memory leak detected in compiler, setting verdict to 'FAIL'\n");
      $root->[7] = 'memory leak detected';
      $result = 'FAIL';
   }
   sa_printResult($root->[0], $result);
   if ($sa_halt_on_errors and ($result ne 'PASS')) {
      sa_log( "\nTest execution interrupted (no cleanup).\n\n");
      exit(1);
   }
   $root->[6] = $result;
}

# Execute test cases
sub sa_executeTCs() {
   my $flag;
   # Log general info
   sa_log($sa_separator);
   sa_log("Date     : " . `date`);
   sa_log("User     : " . `whoami`);
   if (defined($ENV{HOST})) {
      sa_log("Host     : $ENV{HOST}\n");
   } else {
      sa_log("Host     : <unknown>\n");
   }
   sa_log("Platform : " . `uname -a`);
   sa_log("g++      : " . `which g++ | grep g++`);
   sa_log("g++ vers.: " . `g++ -dumpversion`);
   if (defined($ENV{TTCN3_DIR})) {
      sa_log( "compiler : $sa_compilerCmd\n");
      sa_log( "TTCN3_DIR: $ENV{TTCN3_DIR}\n");
   } else {
      sa_log( "compiler : " . `which compiler | grep compiler`);
      sa_log( "TTCN3_DIR: <undefined>\n");
   }
   sa_log("Location : $ENV{PWD}\n");
   sa_log( `$sa_compilerCmd -v 2>&1` . "\n");
   my $sa_nrOfTCs2Execute = scalar @sa_tcList2Execute;
   my $sa_nrOfTCs2Executed = 1;
   # test case execution
   foreach my $poi (@sa_TCInfo) {
      sa_log ("\n\n$sa_separator  Executing test case: '$poi->[0]'\n");
      sa_log ("  leaf: $poi->[5] ; mode: $poi->[1] ; source: $poi->[4] ;");
      sa_log (" index: $sa_nrOfTCs2Executed of $sa_nrOfTCs2Execute\n$sa_separator");
      opendir(DIR, '.') || die "can't opendir '.': $!";
         foreach my $f (readdir(DIR)) { $sa_existedFiles{$f} = $f; }
      closedir DIR;
      sa_writeModulesOfTC(@{$poi});
      my ($res, $resultBuffer) = sa_compileTC(@{$poi});
      if ($res == 1) { # error
         sa_printResult($poi->[0], 'ERROR');
         $poi->[6] = 'ERROR';
         next;
      } elsif ($res == 2) { # compiler hanged or terminated abnormally
         sa_printResult($poi->[0], 'FAIL');
         $poi->[6] = 'FAIL';
         $poi->[8] = 'hanged or abnormal termination';
         next;
      }
      # Check result now
      sa_checkTCResult(@{$poi}, $resultBuffer);
      sa_deleteModulesOfTC(@{$poi});
      $sa_nrOfTCs2Executed++;
   }

   sa_log("\n\n$sa_separator");

   $flag = 0;
   sa_log("The following test cases passed:\n");
   sa_log("================================\n");
   foreach my $poi (@sa_TCInfo) {
      if ($poi->[6] eq 'PASS') {
         sa_log(" [$poi->[4]]: '$poi->[0]'   \n");
         $sa_executionStatistics[0]++;
         $flag = 1;
      }
   }
   if (not $flag) { sa_log(" None.\n"); }

   $flag = 0;
   sa_log("The following test cases failed:\n");
   sa_log("================================\n");
   foreach my $poi (@sa_TCInfo) {
      if ($poi->[6] eq 'FAIL') {
         sa_log(" [$poi->[4]]: '$poi->[0]'   \n");
         $sa_executionStatistics[1]++;
         $flag = 1;
      }
   }
   if (not $flag) { sa_log(" None.\n"); }

   $flag = 0;
   sa_log("The following test cases are inconclusive:\n");
   sa_log("==========================================\n");
   foreach my $poi (@sa_TCInfo) {
      if ($poi->[6] eq 'ERROR') {
         sa_log(" [$poi->[4]]: '$poi->[0]'   \n");
         $sa_executionStatistics[2]++;
         $flag = 1;
      }
   }
   if (not $flag) { sa_log(" None.\n"); }

   $flag = 0;
   sa_log("\nMemory leak detected in the following test cases:\n");
   foreach my $poi (@sa_TCInfo) {
      if (defined($poi->[7])) {
         sa_log(" [$poi->[4]]: '$poi->[0]'   \n");
         $sa_executionStatistics[4]++;
         $flag = 1;
      }
   }
   if (not $flag) { sa_log(" None.\n"); }

   $flag = 0;
   sa_log("\nAbnormal termination occured during the following test cases:\n");
   foreach my $poi (@sa_TCInfo) {
      if (defined($poi->[8])) {
         sa_log(" [$poi->[4]]: '$poi->[0]'   \n");
         $sa_executionStatistics[3]++;
         $flag = 1;
      }
   }
   if (not $flag) { sa_log(" None.\n"); }

   my $sa_nrOfTCs2Execute = scalar @sa_tcList2Execute;
   my $nrOfScriptFiles = scalar @sa_scriptFiles;
   sa_log("\n\n$sa_separator");
   sa_log("$sa_nrOfTCs2Execute test cases from $nrOfScriptFiles script files were executed\n");
   sa_log("Total number of executed test cases: $sa_nrOfTCs2Execute\n");
   sa_log("  PASSED                 test cases: $sa_executionStatistics[0]\n");
   sa_log("  FAILED                 test cases: $sa_executionStatistics[1]\n");
   sa_log("  INCONCLUSIVE           test cases: $sa_executionStatistics[2]\n");
   sa_log("  Abnormally terminated  test cases: $sa_executionStatistics[3]\n");
   sa_log("  Memory leaked          test cases: $sa_executionStatistics[4]\n");
   if ($sa_nrOfTCs2Execute != $sa_executionStatistics[0] + $sa_executionStatistics[1] +
       $sa_executionStatistics[2]) { sa_log( "INTERNAL ERROR: Statistics mismatch\n"); }

   if (defined($sa_LOG)) {sa_log("Session saved to log file '$sa_logFile'\n");}
   return ($sa_nrOfTCs2Execute==$sa_executionStatistics[0]);
}

###############################################################################
##                                   M A I N
###############################################################################

$sessionTime = time();
$sa_compilerCmd = (defined $ENV{TTCN3_DIR}) ?
                  "$ENV{TTCN3_DIR}/bin/compiler" : 'compiler';
$sa_mfgenCmd = (defined $ENV{TTCN3_DIR}) ?
                "$ENV{TTCN3_DIR}/bin/ttcn3_makefilegen" : 'ttcn3_makefilegen';
$sa_runtimeCmd = (defined $ENV{TTCN3_DIR}) ?
                  "$ENV{TTCN3_DIR}/bin/ttcn3_start" : 'ttcn3_start';
sa_processCommandLine();
sa_collectTCs();
sa_parseTCs();
my $isAllPassed = sa_executeTCs();
$sessionTime = time() - $sessionTime;
sa_log("Elapsed time in this session: $sessionTime seconds\n");
if (defined($sa_LOG)) { close $sa_LOG; }
exit($isAllPassed?0:1);

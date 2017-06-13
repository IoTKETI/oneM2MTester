#!/usr/bin/perl
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Lovassy, Arpad
#
###############################################################################

use utf8;

# to read the whole file at once, not just line-by-line
# http://www.kichwa.com/quik_ref/spec_variables.html
undef $/;

$fileindex = 0;
# number of checked JUnit files
$log_files = 0;
# number of missing </testsuite> tag, that closes the JUnit log file
$missing_end_tag = 0;

sub load {
    local $filename = shift; # 1st parameter
    local $f = "F".$fileindex++;
    if(opendir($f, $filename)) {
        # $filename is a directory
        while(local $newfilename = readdir($f)) {
            if(!($newfilename =~ /^\.(\.|git)?$/)) {  # filter . .. .git
                local $longfilename = $filename."/".$newfilename; # for Unix
                #local $longfilename = $filename."\\".$newfilename; # for DOS/Windows
                load($longfilename);
            }
        }
        closedir($f);
    }
    else {
        # $filename is a file

        # check if end tag </testsuite> is present in all the JUnit log files
        if( $filename =~ /MyJUnitlog\-\d+\.log$/ ) {
            #print("-> $filename\n");
            open(IN, $filename);
            $whole_file = <IN>;
            close(IN);

            $log_files++;
            if ( $whole_file =~ /\<\/testsuite\>/g ) {
                # JUnit log file has the closing </testsuite> tag
                #print("$filename OK\n");
            } else {
                print("ERROR: Missing </testsuite> tag in file $filename");
                $missing_end_tag++;
            }
        }
    }
}

# number of failed test cases
$fail = 0;

# number of passed test cases
$pass = 0;

#-------------------------------------------------------------------------------------------------------------
# TEST CASE tc_junitlog_close

# check if end tag </testsuite> is present in all the JUnit log files. JUnit log file is: MyJUnitlog-[0-9]+.log

print("Test case tc_junitlog_close started.\n");
load(".");
print("Number of checked JUnit log files: $log_files\n");
print("Missing end tag: $missing_end_tag\n");

if ( $missing_end_tag > 0 ) {
    print("ERROR: Missing </testsuite> tag in at least one JUnit log file\n");
    print("Test case tc_junitlog_close finished. Verdict: fail\n");
    $fail++;
}
elsif ( $log_files == 0 ) {
    print("ERROR: Missing JUnit log file (MyJUnitlog-[0-9]+.log)\n");
    print("Test case tc_junitlog_close finished. Verdict: fail\n");
    $fail++;
}
else {
    print("Test case tc_junitlog_close finished. Verdict: pass\n");
    $pass++;
}

#-------------------------------------------------------------------------------------------------------------
# OVERALL VERDICT

if ($pass+$fail > 0) {
    printf("Verdict statistics: 0 none (0.00 %%), %d pass (%.2f %%), 0 inconc (0.00 %%), %d fail (%.2f %%), 0 error (0.00 %%).\n", $pass, $pass*100/($pass+$fail), $fail, $fail*100/($pass+$fail) );
}
else {
    printf("Verdict statistics: 0 none (0.00 %%), 0 pass (0.00 %%), 0.00 inconc (0.00 %%), 0 fail (0.00 %%), 0 error (0.00 %%).\n");
}
printf("Test execution summary: %d test cases were executed. Overall verdict: %s\n", $pass+$fail, $fail > 0 ? "fail" : "pass");

#!/bin/bash
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Baranyi, Botond
#
###############################################################################

MYEXE="EmergencyLogTest"
LOGDIR="logs"
SUCCESS=0
LIST_OF_FAILED="failed_testcases.txt"
DEBUG="true"

#####################################
#init:
# sets TTCN3_DIR, if is needed
# makes executable
#####################################
init() {
  if [ "$TTCN3_DIR" == "" ]
  then
     echo "TTCN3_DIR should be set"
     exit
  fi

  make
  chmod +x ./$MYEXE
  success=0
  failed=0
  echo "Failed tests:" > ${LIST_OF_FAILED}
  rm -f logs/*
  #TODO: set cut off length according to TimeStampFormat
}

#####################################
# debug
# $1 : log file
#####################################
debug(){
  if [ "$DEBUG" == "true" ] ; then
   echo "$1"
  fi
}

#####################################
# modify_logfile
# $1 : log file
#####################################
modify_logfile() { 
  #remove timestamps, data different for run by run
  debug "Modifying file $1"
  cmd='
s/\ (No such file or directory)//g
s/^EXECUTOR_RUNTIME - MTC was created. Process id: [0-9]*\./EXECUTOR_RUNTIME - MTC was created. Process id:/g
s/rocess id: [0-9][0-9]*/rocess id: X/g
s/I\/O: [0-9][0-9]*/I\/O: Y/g
s/switches: [0-9][0-9]*/switches: S/g
s/block output operations: [0-9]*/block output operations: B/g
s/block input operations: [0-9]*/block input operations: I/g
s/system time: [0-9]*\.[0-9]*/system time: SYS/g
s/user time: [0-9]*\.[0-9]*/user time: T/g
s/seed [0-9][0-9]*[.][0-9]*/seed /g
s/returned [0-9][0-9]*[.][0-9]*/returned R/g
s/srand48.[\-]*[0-9]*.\./srand X/g
s/t1: [0-9]e-[0-9]* s/t1: T s/g
s/t1: [0-9e.-]* s/t1: T s/g
s/t: [0-9e.-]* s/t: T s/g
s/Mytime: [0-9.]*e-[0-9]* s/Mytime: T s/g
s/Mytime: [0-9.]* s/Mytime: T s/g
s/reference [0-9]* finished/reference R finished/g
s/started on [a-zA-Z0-9._-]*. Version: .*/started on X. Version: V/g
s/^[0-9.:]* //g
s/[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*/a\.b\.c\.d/g
s/The address of MC was set to [a-zA-Z0-9._-]*/The address of MC was set to X/g
s/resident set size: [0-9]*/resident set size: S/g
s/Action: Finish:[0-9][0-9]*[.][0-9]*/Action: Finish: F/g
s/Action: Elapsed time:[0-9][0-9]*[.][0-9]*/Action: Elapsed time:E/g
s/Read timer t: [0-9e.-]* s/Read timer t: T s/g
s/Read timer t1: [0-9e.-]* s/Read timer t1: T s/g
s/Action: Start:[0-9e.-]* s/Action: Start: S s/g
s/^PARALLEL_PTC.*//g
s/UNIX pathname .*$/UNIX pathname /g
s/^EXECUTOR_COMPONENT.*$//g
s/^PORTEVENT_MQUEUE.*$//g
s/^$//g
'
  sed -e "$cmd" < "$1" > "${LOGDIR}/tmp"
  
  sed -e "/^$/d" < "${LOGDIR}/tmp" > "$1"
  #mv "${LOGDIR}/tmp" "$1"
}

#####################################
# run_and_modify_logfile
# $1 : cfg file
# $2 : cfg file base without extension
#####################################
run_and_modify_logfile() {
  echo $TTCN3_DIR/bin/ttcn3_start $MYEXE $1
  $TTCN3_DIR/bin/ttcn3_start $MYEXE $1
  cp $LOGDIR/${MYEXE}-mtc.log $LOGDIR/${2}-mtc.log
  cp $LOGDIR/${MYEXE}-hc.log $LOGDIR/${2}-hc.log
  
  #remove timestamp, diff data:
  modify_logfile "${LOGDIR}/${2}-mtc.log"
  modify_logfile "${LOGDIR}/${2}-hc.log"
  
  #emergency log handling
  if [ -e "$LOGDIR/${MYEXE}-mtc.log_emergency" ]
  then
   debug "Emergency log file name: $LOGDIR/${2}-mtc.log_emergency"
   cp "$LOGDIR/${MYEXE}-mtc.log_emergency" "$LOGDIR/${2}-mtc.log_emergency"
   modify_logfile "$LOGDIR/${2}-mtc.log_emergency"
  fi
  
  i=3
  while [ -e "$LOGDIR/${MYEXE}-$i.log" ]
  do
       cp "$LOGDIR/${MYEXE}-$i.log" "$LOGDIR/${2}-$i.log"
       modify_logfile "$LOGDIR/${2}-$i.log"
       let "i = $i + 1"
  done
}
#####################################
# create EmergencyLogCommentedOut (ELCO) file from the original log file
# $1:original config file
# output "${1}_ELCO.cfg"
#####################################
create_ELCO_config() {
  elco_cfg=`basename $1 ".cfg"`
  elco_cfg="${elco_cfg}_ELCO.cfg"
  cp $1 "${elco_cfg}"
  cmd='
s/^\*\.Emergency/#\*\.Emergency/g'
  sed -e "$cmd" < "${elco_cfg}" > tmp
  mv tmp "${elco_cfg}"
  debug "OLD  cfg name: $1"
  debug "ELCO cfg name: ${elco_cfg}"
} 
#####################################
# compare_with_ELCO
# $1: componenent id (mtc, hc, 3, 4 etc)
#####################################
compare_with_ELCO() {
 diff -u "$LOGDIR/${elco_cfg_base}-$1.log" "$LOGDIR/${orig_cfg_base}-$1.log"  > "${LOGDIR}/diff_${orig_cfg_base}_$1.log"  
 if [ "$?" -eq $SUCCESS ] 
 then
   debug ">>>tc $orig_cfg_base $1 part success<<<"
   let "success = $success + 1"
 else
   debug ">>>tc $orig_cfg_base $1 part failed<<<"   
   echo "tc $orig_cfg_base $1 part failed" >> ${LIST_OF_FAILED}
   let "failed = $failed + 1"
 fi
 debug "success: ${success} failed: ${failed}"
}

#####################################
# compare_with_expected
# $1: componenent id (mtc, hc, 3, 4 etc)
# $2: expected log
#####################################
compare_with_expected() { 
 diff -u "$2" "${LOGDIR}/${orig_cfg_base}-$1.log" > "${LOGDIR}/diff_${orig_cfg_base}_$1.log"
 if [ "$?" -eq $SUCCESS ] 
 then
   debug ">>>tc $orig_cfg_base $1 part success<<<"
   let "success = $success + 1"
 else
   debug ">>>tc $orig_cfg_base $1 part failed<<<"
   echo "tc $orig_cfg_base $1 part failed" >> ${LIST_OF_FAILED}
   let "failed = $failed + 1"
 fi
 debug "success: ${success} failed: ${failed}"
}

#####################################
# compare_with_expected_emergency
# $1: componenent id (mtc, hc, 3, 4 etc)
# $2: expected log
#####################################
compare_with_expected_emergency() { 
 #debug "==>diff -u $4 ${LOGDIR}/${orig_cfg_base}-$1.log_emergency  > ${LOGDIR}/diff_${orig_cfg_base}_$1.log_emergency"
 diff -u "$2" "${LOGDIR}/${orig_cfg_base}-$1.log_emergency"  > "${LOGDIR}/diff_${orig_cfg_base}_$1.log_emergency"
 if [ "$?" -eq $SUCCESS ] 
 then
   debug ">>>tc $orig_cfg_base $1 part success<<<"
   let "success = $success + 1"
 else
   debug ">>>tc $orig_cfg_base $1 part failed<<<"
   echo "tc $orig_cfg_base $1 emergency part failed" >> ${LIST_OF_FAILED}
   let "failed = $failed + 1"
 fi
 debug "success: ${success} failed: ${failed}"
}
#####################################
# run_and_compare_log_files
# $1 first config file
# $2 2nd config file
# $3 output file (diff_)
#####################################
run_and_compare_log_files() {
 #running with orig cfg:
 rm  ${LOGDIR}/${MYEXE}*.log
 orig_cfg_base=`basename $1 ".cfg"`
 run_and_modify_logfile "$1" "$orig_cfg_base"
 
 if [ "$2" == "" ]; then
   #running with modified cfg:
   create_ELCO_config "$1"
 else
   elco_cfg=$2
 fi
 
 elco_cfg_base=`basename "$elco_cfg" ".cfg"`
 debug "ELCO cfg name: ${elco_cfg}"
 run_and_modify_logfile "${elco_cfg}"  "$elco_cfg_base"

 #diff:
 compare_with_ELCO "mtc"
 compare_with_ELCO "hc"
  
 i=3
 while [ -e "$LOGDIR/${elco_cfg_base}-$i.log" -a -e "$LOGDIR/${orig_cfg_base}-$i.log" ] 
 do
   compare_with_ELCO "$i"
   let "i = $i + 1" 
 done
  
}
#####################################
# run_and_compare_log_file_with_expected
# $1 first config file
# $2 expected mtc log file
# $3 expected hc log file
# $4 expected emergency log file
#####################################
run_and_compare_log_file_with_expected() {
  rm -f ${LOGDIR}/${MYEXE}*.log*
  #running with orig cfg:
  orig_cfg_base=`basename $1 ".cfg"`
  run_and_modify_logfile "$1" "$orig_cfg_base"


  debug "cfg:  $1"
  debug "mtclog:  $2"
  debug "hclog: $3"
  #diff mtc with the expected:
  if [ -e "$2" ]; then
     modify_logfile "$2"
     compare_with_expected "mtc" "$2"
  else
    debug ">>>tc $orig_cfg_base mtc part failed, not existing expected log file $2<<<"
    echo "tc $orig_cfg_base mtc part failed" >> ${LIST_OF_FAILED}
    let "failed = $failed + 1"
  fi
  
  #diff:
  
  if [ "$3" != "" ]; then
   if [ -e "$3" ]; then
      modify_logfile "$3"
      compare_with_expected "hc" "$3"
   else
     debug ">>>tc $orig_cfg_base mtc part failed, not existing expected log file $3<<<"
     echo "tc $orig_cfg_base mtc part failed" >> ${LIST_OF_FAILED}
     let "failed = $failed + 1"
   fi
  fi  
  
  #mtc emergency log kezeles
  if [ "$4" != "" ]; then
   if [ -e "$4" ]; then
      modify_logfile "$4"
      compare_with_expected_emergency "mtc" "$4"
   else
     debug ">>>tc $orig_cfg_base mtc part failed, not existing expected log file $4<<<"
     echo "tc $orig_cfg_base mtc part failed, not existing expected log file $4" >> ${LIST_OF_FAILED}
     let "failed = $failed + 1"
   fi
  fi
}

#####################################
# evaluate
#####################################
evaluate() {
  echo "Summary: success: ${success}, failed: ${failed} "
  if [ $failed -eq 0 ]
  then
   echo "Overall verdict: success"
   exit_code=0
  else
   echo "Overall verdict: failed"
   cat ${LIST_OF_FAILED} |
   while read  line
   do
    echo $line
   done
   exit_code=1
  fi
  
  exit $exit_code
}

#### MAIN ###

# see the files "logs/diff*.log" !

init
#======= Buffer All ========
run_and_compare_log_files EL_BufferAll_1.cfg  #compatibility test with minimal coverage (compare logs with/without EL setting if EL-event does not happen
run_and_compare_log_files EL_BufferAll_2.cfg  #compatibility test with coverage "tc_parallel_portconn" (compare logs with/without EL setting if EL-event does not happen) - NOT STABILE, hc FAILED
run_and_compare_log_files EL_BufferAll_3.cfg EL_BufferAll_3_NOEL.cfg #the same log expected with two different log files: EL and NOEL, EL-event occurs in the first case - PASSED
run_and_compare_log_files EL_BufferAll_4.cfg  #compatibility test with "tc_timer" (compare logs with/without EL setting if EL-event does not happen) - hc FAILED
run_and_compare_log_files EL_BufferAll_5.cfg  #compatibility test "with Titan_LogTest.tc_encdec" - NOT STABILE, mtc PASSED, hc PASSED
run_and_compare_log_files EL_BufferAll_6.cfg  #compatibility test with "tc_function_rnd and tc_encdec" - mtc PASSED, hc PASSED
run_and_compare_log_file_with_expected "EL_BufferAll_7.cfg"  "EL_BufferAll_7_mtc_expected.log"  # - mtc PASSED
run_and_compare_log_file_with_expected "EL_BufferAll_7A.cfg"  "EL_BufferAll_7A_mtc_expected.log"  # - mtc PASSED
run_and_compare_log_files EL_BufferAll_8.cfg   #compatibility test with MAXIMAL coverage - NOT STABILE; mtc PASSED hc FAILED (1 line order problem,
run_and_compare_log_file_with_expected "EL_BufferAll_9.cfg"  "EL_BufferAll_9_mtc_expected.log"  # - More EL-event can be logged after each other - mtc FAILED ( 1 line missing), hc FAILED  
run_and_compare_log_file_with_expected "EL_BufferAll_10.cfg"  "EL_BufferAll_10_mtc_expected.log"  # - More EL-event with big buffer - mtc PASSED
run_and_compare_log_file_with_expected "EL_BufferAll_11.cfg"  "EL_BufferAll_11_mtc_expected.log"  # - More EL-event with big buffer - mtc PASSED
run_and_compare_log_file_with_expected "EL_BufferAll_12.cfg"  "EL_BufferAll_12_mtc_expected.log"  # - More EL-event with very small buffer - (buffer size=2) - mtc FAILED, one line missing
run_and_compare_log_file_with_expected "EL_BufferAll_13.cfg"  "EL_BufferAll_13_mtc_expected.log"  # - More EL-event with very small buffer - (buffer size=3) - mtc FAILED, one line missing
#======= Buffer Masked ========
run_and_compare_log_files EL_BufferMasked_1.cfg  #compatibility test with minimal coverage (compare logs with/without EL setting if EL-event does not happen
run_and_compare_log_files EL_BufferMasked_2.cfg  #compatibility test with coverage "tc_parallel_portconn" (compare logs with/without EL setting if EL-event does not happen) - NOT STABILE
run_and_compare_log_files EL_BufferMasked_4.cfg  #compatibility test with "tc_timer" (compare logs with/without EL setting if EL-event does not happen) - PASSED
run_and_compare_log_files EL_BufferMasked_5.cfg  #compatibility test "with Titan_LogTest.tc_encdec" - NOT STABILE
run_and_compare_log_files EL_BufferMasked_3.cfg EL_BufferMasked_3_NOEL.cfg #the same log expected with two different log files: EL and NOEL, EL-event occurs in the first case - PASSED
run_and_compare_log_files EL_BufferMasked_6.cfg  #compatibility test with "tc_function_rnd and tc_encdec" - PASSED
run_and_compare_log_file_with_expected "EL_BufferMasked_7.cfg"  "EL_BufferMasked_7_mtc_expected.log" "" "EL_BufferMasked_7_mtc_expected.log_emergency" # - PASSED
run_and_compare_log_files EL_BufferMasked_8.cfg  #compatibility test with MAXIMAL coverage - NOT STABILE
run_and_compare_log_file_with_expected "EL_BufferMasked_9.cfg"  "EL_BufferMasked_9_mtc_expected.log" ""  "EL_BufferMasked_9_mtc_expected.log_emergency" # - More EL-event can be logged after each other - PASSED 
run_and_compare_log_file_with_expected "EL_BufferMasked_10.cfg"  "EL_BufferMasked_10_mtc_expected.log" ""  "EL_BufferMasked_10_mtc_expected.log_emergency" # - More EL-event with big buffer - PASSED
run_and_compare_log_file_with_expected "EL_BufferMasked_11.cfg"  "EL_BufferMasked_11_mtc_expected.log"  ""  "EL_BufferMasked_11_mtc_expected.log_emergency" # - More EL-event with big buffer - PASSED
run_and_compare_log_file_with_expected "EL_BufferMasked_12.cfg"  "EL_BufferMasked_12_mtc_expected.log"  "" "EL_BufferMasked_12_mtc_expected.log_emergency" # - More EL-event with very small buffer - (buffer size=2) - mtc.log_emergency FAILED (wrong order)
run_and_compare_log_file_with_expected "EL_BufferMasked_13.cfg"  "EL_BufferMasked_13_mtc_expected.log"  "" "EL_BufferMasked_13_mtc_expected.log_emergency" # - More EL-event with very small buffer - (buffer size=3) - mtc.log_emergency FAILED (wrong order)
#Stat: 63 success/ 0 failed 
evaluate

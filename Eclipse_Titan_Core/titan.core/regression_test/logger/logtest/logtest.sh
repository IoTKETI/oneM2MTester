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


#Description:
#The goal of this script is to compare the logfile generation result for two different Titan version.
#If switches are not applied the original logfiles given as input will be compared with the just generated log files
#This is the use case in folder regression_test/logger/logtest


#Inputs
# old_logfilename.txt -if "-o" not used
# old_logfilename_modified.txt -if "-o" not used
# Outputs:
# old_logfilename.txt -if "-o" used
# old_logfilename_modified.txt -if "-o" used
# Console_old_logfilename.txt
# new_logfilename.txt
# new_logfilename_modified.txt
# Console_new_logfilename.txt

create_old=0
cfg_filename="Titan_LogTest.cfg"
old_logfilename="original_merged_log"
new_logfilename="new_merged_log"
generate_makefile=0



##### FUNCTIONS #######

####################
# showUsage
####################
showUsage()
{
  echo "Usage: $0 <options>"
  echo "Options:"
  echo " -o create old versions"
  echo " -c [config filename] e.g.: Titan_LogTest.cfg"
  echo " -O [Old log file name (before modification)]"
  echo " -N [New log file name (before modification)]"
  echo " -g generate makefiles. Mandatory if -o applied"
  echo "Prerequisite: TTCN3_DIR set for the new ttcn3 dir"
}

####################
# init
####################
init() {
  echo "Init called"

  if [ "$TTCN3_DIR_OLD" == "" ]
  then
    TTCN3_DIR_OLD="/mnt/TTCN/Releases/TTCNv3-1.8.pl5"
  fi
  echo "TTCN3_DIR_OLD:${TTCN3_DIR_OLD}"
  
  if [ "$TTCN3_DIR" == "" ]
  then
    TTCN3_DIR="/export/localhome/TCC/ethbaat/XmlTest/install"
  fi
  TTCN3_DIR_NEW=${TTCN3_DIR}
  
  echo "TTCN3_DIR:${TTCN3_DIR}"
  
  LD_LIBRARY_PATH_ORIG=$LD_LIBRARY_PATH
  
  SYSTEM_NAME=`uname -n`
  HOST_NAME=`hostname`
  export HOST_NAME
  if [ "$USER" == "" ]
  then
    export USER=$(whoami)
  fi
  if [ "$USER" == "" ]
  then
    export USER=$(/usr/ucb/whoami)
  fi
}

####################
# create_log
# $1: name of the cfg file
# $2: name of the merged log file
# $3: name of the merged modified log file (timestamps removed)
####################
create_log() {

echo "Create_log called"

make clean

if [ "${generate_makefile}" == "1" ]
then
 ttcn3_makefilegen -fg Titan_LogTest.ttcn Titan_LogTestDefinitions.ttcn *.cc *.hh
fi

make
echo "ttcn3_start Titan_LogTest $1"
ttcn3_start Titan_LogTest  "$1" | tee "Console_$2"

#ttcn3_logmerge *.log >  "$2"
#logmerge the order of the loglines can be different per executions
# therefore the logs will be appended instead of merge
echo "Appended logs:" > "$2"
list=$(ls  Titan_LogTest*.log | xargs) 
for i in $list
do
 cat $i >> "$2"
done

cmd='
s/^.*\(EXECUTOR_COMPONENT - TTCN-3 Parallel Test Component started on\).*\( Component reference:\).*$/\1 \2/g
s/^.*\(PORTEVENT_UNQUALIFIED - Port internal_port is waiting for connection from\).*$/\1/g
s/^.*\(EXECUTOR_UNQUALIFIED - The local IP address of the control connection to\).*$/\1/g
s/^.*\(EXECUTOR_COMPONENT - TTCN-3 Main Test Component started on\).*$/\1/g
s/^.*\(EXECUTOR_RUNTIME - TTCN-3 Host Controller started on\).*$/\1/g
s/^.*\(EXECUTOR_UNQUALIFIED - The address of MC was set to\).*$/\1/g
s/^.*\(EXECUTOR_RUNTIME - MTC was created. Process id\).*$/\1/g
s/^.*\(Random number generator was initialized with seed\)\(.*\)$/\1/g
s/^.*\(Function rnd() returned\)\(.*\)$/\1/g
s/^.*\(PARALLEL_PTC - PTC was created. Component reference: \).*\(testcase name:\).*\(, process id:\)\(.*\)$/\1\2\3/g
s/^.*\(ERROR_UNQUALIFIED Titan_LogTest.ttcn\)\(.*\)\(Dynamic test case error: Assignment of an unbound integer value\).*/\1 \3/g
s/^.*\(TIMEROP_READ Titan_LogTest.ttcn\)\(.*\)/\1/g
s/^.*\(USER_UNQUALIFIED Titan_LogTest.ttcn\)\(.*\)/\1/g
s/^.*\(PARALLEL_PTC - PTC with component reference\)\(.*\)/\1/g
s/^.*\(PARALLEL_PTC - MTC finished.\)\(.*\)//g
s/^.*\(EXECUTOR_RUNTIME - Maximum number of open file descriptors\).*//g
s/^.\{15\} //g
'
# TODO: s/^.*\(PARALLEL_PTC - MTC finished.\)\(.*\)/\1/g should be used instead of s/^.*\(PARALLEL_PTC - MTC finished.\)\(.*\)//g
# TODO: s/^.*\(EXECUTOR_RUNTIME - Maximum number of open file descriptors\).*//g kiszedese
# This line removes the random/seldom line from the log
sed -e "$cmd" -e "/^$/d" < "$2" > "$3"

}

####################
# create_log_old
# $1: name of the cfg file
# $2: name of the merged log file
# $3: name of the merged modified log file (timestamps removed)
####################
create_log_old() {
  TTCN3_DIR_ORIG=${TTCN3_DIR}
  TTCN3_DIR=${TTCN3_DIR_OLD}
  PATH_ORIG=$PATH}
  PATH=${TTCN3_DIR}/bin:${PATH}
  LD_LIBRARY_PATH_ORIG=${LD_LIBRARY_PATH}
  LD_LIBRARY_PATH=${TTCN3_DIR}/lib:${LD_LIBRARY_PATH}
  export TTCN3_DIR PATH LD_LIBRARY_PATH
  
  create_log $1 $2 $3
  
  TTCN3_DIR=${TTCN3_DIR_ORIG}
  PATH=${PATH_ORIG}
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH_ORIG}
  export TTCN3_DIR PATH LD_LIBRARY_PATH
}

####################
# create_log_new
# $1: name of the cfg file
# $2: name of the merged log file
# $3: name of the merged modified log file (timestamps removed)
####################
create_log_new() {
  echo ">>>>create_log_new args: $1 $2 $3"
  TTCN3_DIR_ORIG=${TTCN3_DIR}
  TTCN3_DIR=${TTCN3_DIR_NEW}
  PATH_ORIG=${PATH}
  PATH=${TTCN3_DIR}/bin:${PATH}
  LD_LIBRARY_PATH_ORIG=${LD_LIBRARY_PATH}
  LD_LIBRARY_PATH=${TTCN3_DIR}/lib:${LD_LIBRARY_PATH}
  export TTCN3_DIR PATH LD_LIBRARY_PATH
  
  create_log $1 $2 $3
  
  TTCN3_DIR=${TTCN3_DIR_ORIG}
  PATH=${PATH_ORIG}
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH_ORIG}
  export TTCN3_DIR PATH LD_LIBRARY_PATH
}
####################
# compare
# $1: name of the old merged modified log file
# $2: name of the new merged modified log file
####################
compare() {
 echo "Comparison follows"
 diff "$1" "$2"
 result=$?
 echo "Comparison result is $result"
 exit $result
}

####################### EOF FUNCTIONS ###################################

#===== MAIN =======
while getopts "hoc:O:N:g" o
do
  case "$o" in 
    h) showUsage; exit 1;;
    o) create_old=1;;
    c) cfg_filename=$OPTARG;;
    O) old_logfilename=$OPTARG;;
    N) new_logfilename=$OPTARG;;
    g) generate_makefile=1;;
    [?]) showUsage; exit 1;;
  esac
done

init

if [ "${create_old}" == "1" ]
then 
  echo ">>>CREATES OLD<<<"
  create_log_old "${cfg_filename}" "${old_logfilename}.txt" "${old_logfilename}_modified.txt"
else
  echo ">>>OLD LOG NOT CREATED<<<"
  echo ">>>${create_old}<<<"
fi
  
create_log_new "${cfg_filename}" "${new_logfilename}.txt" "${new_logfilename}_modified.txt"

compare "${old_logfilename}_modified.txt" "${new_logfilename}_modified.txt"

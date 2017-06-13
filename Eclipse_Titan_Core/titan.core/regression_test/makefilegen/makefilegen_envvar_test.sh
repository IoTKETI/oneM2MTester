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
#   Szabo, Bence Janos
#
###############################################################################

MAKEFILEGEN_ENVVAR_TEST_FOLDER="makefilegen_envvar_test/TpdEnvVarTestMain"
TPD_FILENAME="TpdEnvVarTestMain.tpd"

#Environment variables
#TEST1_DIR="../testA/Test1"
#TESTFOLDER1_DIR="src"
#TESTFOLDER2_DIR="src"
#TEST_DIR="../Test"

#Setting environment variables
#export TEST1_DIR
#export TESTFOLDER1_DIR
#export TESTFOLDER2_DIR
#export TEST_DIR

#Running makefilegen command
$TTCN3_DIR/bin/ttcn3_makefilegen -fg -t $MAKEFILEGEN_ENVVAR_TEST_FOLDER/$TPD_FILENAME
if [ $? -ne 0 ]; then
  echo "Makefilegen envvar test failed! Overall verdict: fail"
  rm -r $MAKEFILEGEN_ENVVAR_TEST_FOLDER/bin
  exit 1
fi

#Running make command
eval "cd $MAKEFILEGEN_ENVVAR_TEST_FOLDER/bin && make"
if [ $? -ne 0 ]; then
  echo "Makefilegen envvar test failed! Overall verdict: fail"
  eval "cd .."
  rm -r bin
  exit 1
else
  echo "Makefilegen envvar test valid! Overall verdict: pass"
  eval "cd .."
  rm -r bin
  exit 0
fi

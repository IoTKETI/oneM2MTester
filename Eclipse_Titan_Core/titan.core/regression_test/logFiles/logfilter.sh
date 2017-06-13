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

LOGFILE="lfilter.log2"
FILTERED_LOG_FILE="filtered.log"
EXPECTED_LOG_FILE="filtered_e.log2"

../../Install/bin/ttcn3_logfilter -o $FILTERED_LOG_FILE $LOGFILE MATCHING+ PARALLEL+
diff $FILTERED_LOG_FILE $EXPECTED_LOG_FILE
if [ $? -ne 0 ]; then
  echo "Logfilter test failed! Overall verdict: fail"
  exit 1
else
  echo "Logfilter test valid! Overall verdict: pass"
  exit 0
fi

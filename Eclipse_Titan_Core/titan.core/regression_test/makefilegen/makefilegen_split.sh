#!/bin/bash
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Szabo, Bence Janos
#
###############################################################################


#Running makefilegen command with splitting types
$TTCN3_DIR/bin/ttcn3_makefilegen -f -S -o TempMakefile -U none
if [ $? -ne 0 ]; then
  echo "Makefilegen '-U none' split test failed! Overall verdict: fail"
  exit 1
fi

$TTCN3_DIR/bin/ttcn3_makefilegen -f -S -o TempMakefile -U type
if [ $? -ne 0 ]; then
  echo "Makefilegen '-U type' split test failed! Overall verdict: fail"
  exit 1
fi

$TTCN3_DIR/bin/ttcn3_makefilegen -f -S -o TempMakefile -U 2
if [ $? -ne 0 ]; then
  echo "Makefilegen '-U 2' split test failed! Overall verdict: fail"
  exit 1
fi

# Negative tests
$TTCN3_DIR/bin/ttcn3_makefilegen -f -S -o TempMakefile -U 1a
if [ $? -ne 1 ]; then
  echo "Makefilegen '-U 1a' negative split test failed! Overall verdict: fail"
  exit 1
fi

$TTCN3_DIR/bin/ttcn3_makefilegen -f -S -o TempMakefile -U a1
if [ $? -ne 1 ]; then
  echo "Makefilegen '-U a1' negative split test failed! Overall verdict: fail"
  exit 1
fi

echo "Makefilegen split tests are valid! Overall verdict: pass"

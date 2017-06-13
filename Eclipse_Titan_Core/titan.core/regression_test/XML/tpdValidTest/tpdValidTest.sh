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
#   Pilisi, Gergely
#
###############################################################################

TPD_DIR="./tpdTest/"
XSD_DIR="$TTCN3_DIR/etc/xsd/"
XSD="TPD.xsd"

find $TPD_DIR -type f -name *.tpd | xargs -I {} xmllint --noout --schema $XSD_DIR/$XSD {}

if [ $? -ne 0 ]; then
  echo "Not every .tpd is valid! Overall verdict: fail"
  exit 1
else
  echo "Every .tpd is valid! Overall verdict: pass"
  exit 0
fi

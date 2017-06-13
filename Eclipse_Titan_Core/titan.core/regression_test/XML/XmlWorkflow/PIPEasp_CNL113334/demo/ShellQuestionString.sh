#!/bin/sh
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Raduly, Csaba
#
###############################################################################
# SCCSID: %Z%%M% %I% %G%
#

head -1 > /tmp/$$.input
if xterm -title "Question from TTCN" -e sh -c "echo ''; cat /tmp/$$.input; echo ''; echo 'Give the Answer and Press RETURN'; head -1 > /tmp/$$.output"
  then
    cat /tmp/$$.output
    /bin/rm -f /tmp/$$.input
    /bin/rm -f /tmp/$$.output
    exit 0
  else
    /bin/rm -f /tmp/$$.input
    /bin/rm -f /tmp/$$.output
    exit 1
fi

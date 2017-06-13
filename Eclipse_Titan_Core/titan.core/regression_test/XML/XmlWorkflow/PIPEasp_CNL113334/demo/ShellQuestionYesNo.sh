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
if xterm -title "Question from TTCN" -e sh -c "echo ''; cat /tmp/$$.input; echo ''; echo 'Answer Yes or No and Press RETURN'; head -1 > /tmp/$$.output"
  then
    ans=`cat /tmp/$$.output`
    case $ans in
      Yes*) echo "Yes";;
      yes*) echo "Yes";;
      YES*) echo "Yes";;
      Y*) echo "Yes";;
      No*) echo "No";;
      no*) echo "No";;
      NO*) echo "No";;
      N*) echo "No";;
      *) echo $ans;;
    esac
    /bin/rm -f /tmp/$$.input
    /bin/rm -f /tmp/$$.output
    exit 0
  else
    /bin/rm -f /tmp/$$.input
    /bin/rm -f /tmp/$$.output
    exit 1
fi

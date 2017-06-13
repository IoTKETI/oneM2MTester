#!/bin/bash
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Balasko, Jeno
#   Beres, Szabolcs
#   Kovacs, Ferenc
#
###############################################################################

# We need a CVSROOT.
. ~/.titan_builder
rm -rf *.{py,pyc,sh} TTCNv3/etc/autotest
cvs -t co TTCNv3/etc/autotest 2>&1 | tee cvs_autotest_output.txt && ln -sf TTCNv3/etc/autotest/*.{py,sh} .
if [ "$1" = "-only_vobtests" ] ; then ./titan_builder.sh -c vobtests_on_x86_64_linux_tcclab1,vobtests_on_sparc_solaris_esekits3013 1>/dev/null
elif [ "$1" = "-daily" ] ; then ./titan_builder.sh -c x86_64_linux_tcclab3_your_last_chance 1>/dev/null
else ./titan_builder.sh -c x86_64_linux_tcclab5,x86_64_linux_tcclab4,x86_64_linux_tcclab5_clang,x86_64_linux_tcclab1,x86_64_linux_tcclab2,x86_64_linux_tcclab3,sparc_solaris_esekits3013,i386_solaris_bangjohansen,x86_64_linux_esekilxxen1843,vobtests_on_x86_64_linux_tcclab1 1>/dev/null
fi

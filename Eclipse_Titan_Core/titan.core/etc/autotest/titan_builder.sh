#!/bin/bash -x
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
#   Kovacs, Ferenc
#
###############################################################################


# Load platform specific settings.
CONFIG_FILE=${HOME}/.titan_builder
if [ -f ${CONFIG_FILE} ]; then . ${CONFIG_FILE}; fi

if [ ! -n "`mount | grep \"/view/eferkov_tcc/vobs/ttcn\"`" ]; then sshfs -o ro,reconnect,transform_symlinks titanrt@147.214.15.153:/view/eferkov_tcc/vobs/ttcn /home/titanrt/titan_nightly_builds/vobs/ttcn; fi
if [ ! -n "`mount | grep \"/proj/TTCN/www/ttcn/root/titan-testresults\"`" ]; then sshfs -o reconnect,transform_symlinks titanrt@147.214.15.96:/proj/TTCN/www/ttcn/root/titan-testresults /home/titanrt/titan_nightly_builds/web; fi

./titan_builder.py "$@"

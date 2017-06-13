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
#
###############################################################################
#!/bin/bash

# The view is currently for `eferkov', but it's not important if CS 129 is
# loaded.  The same for the directory used for publishing.  The license file
# and the archive directory are for `eferkov' as well.  Update the files here
# frequently as they change in CVS.
sshfs -o ro,reconnect,transform_symlinks titanrt@147.214.15.153:/view/eferkov_tcc/vobs/ttcn /home/titanrt/titan_nightly_builds/vobs/ttcn
sshfs -o reconnect,transform_symlinks titanrt@147.214.15.96:/proj/TTCN/www/ttcn/root/titan-testresults /home/titanrt/titan_nightly_builds/web

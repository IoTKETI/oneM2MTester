###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   >
#   Balasko, Jeno
#
###############################################################################
#!/bin/sh

set -xe

# set environment
. ./path.sh

# build titan from source on solaris
mkdir -p titan
cd titan
# .tar is exported with git archive -o t.tar
tar xf ../src/t.tar
cp ../Makefile.personal .
{ make || kill $$; } |tee make.log
{ make install || kill $$; } |tee make.install.log
# TODO: make release, cp doc/*.pdf


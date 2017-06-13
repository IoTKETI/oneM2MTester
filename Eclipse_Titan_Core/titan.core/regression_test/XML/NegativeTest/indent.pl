#!/usr/bin/perl -wpli.orig
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

use strict;

s!\\n!\n!g;
s!\\t!\t!g;
s!\("<!("\n<!g;

s!{ {!{\n  {!g;
s!}, {!},\n  {!g;
s!} }!}\n}!g;


#!/usr/bin/perl -wln
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

next unless my ($txt) = m{Action: '[\da-fA-F]+'O \("(.+)"\)};

$txt =~ s/\\n/\n/g;
$txt =~ s/\\t/\t/g;

print $txt;

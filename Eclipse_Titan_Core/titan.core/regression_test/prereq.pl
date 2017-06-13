#!/usr/bin/perl -wT
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
# No use warnings; in 5.005 but we have the -w flag

if ($] < 5.006) {
  # ancient perl, we must be on Solaris :(
  my @perlloc = qw( /proj/TTCN/Tools/perl-5.10.1/bin/perl /mnt/TTCN/Tools/perl-5.10.1/bin/perl );
  foreach (@perlloc) {
    if (-x $_) {
      warn "Let's try with $_ instead";
      exec( $_, '-wT', $0, @ARGV ) or die "That didn't work either: $!";
    }
  }
}
else {
require Test::More;
Test::More->import(tests => 2 + 3);
}

my $level = shift @ARGV || 0;

ok( exists $ENV{TTCN3_LICENSE_FILE}, 'TTCN3_LICENSE_FILE defined' );
ok( -f     $ENV{TTCN3_LICENSE_FILE}, "TTCN3_LICENSE_FILE ($ENV{TTCN3_LICENSE_FILE}) exists" );

SKIP: {
skip('Running directly; no info about our parent', 3) if $level < 1;

is($ENV{CXX}        , $ENV{BASE_CXX}    , 'CXX         is the same');
is($ENV{XMLDIR}     , $ENV{BASE_XML}    , 'XMLDIR      is the same');
is($ENV{OPENSSL_DIR}, $ENV{BASE_OPENSSL}, 'OPENSSL_DIR is the same');
}


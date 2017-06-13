#!/usr/bin/perl -wl
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
$"='';

use Config;


sub signame ($) {
  my $signr = $_[0];
  if ($Config{sig_name} && $Config{sig_num}) {
    my @sig_name;
    my %sig_num;
    my @names = split ' ', $Config{sig_name};
    @sig_num{@names} = split ' ', $Config{sig_num};
    foreach (@names) {
      $sig_name[$sig_num{$_}] ||= $_;
    }
    
    return $sig_name[$signr];
  }
  else {
    return $signr;
  }
}


#
# This script runs the makefilegen test repeatedly, with every possible combination
# of the makefilegen options below:
#
my @options = qw( a g m R s );
# -a    use absolute pathnames
# -g    Makefile for GNU make
# -m    always use makedepend for deps, even for GNU make
# -R    RT2
# -s    single mode
#
# -l    NO!!! dynamic linking: ruins the build

#use 5.010;
#sub mix {@_ ? map {my $x = $_; map "$x$_", mix(@_[1..$#_])} @{$_[0]} : ""}
#print for mix @options;

# Generate the power set of @options
# From http://rosettacode.org/wiki/Power_set#Perl
sub p{
  @_
  ? map { $_,[$_[0],@$_] } p (@_[1..$#_])
  : []
}

my @powerset = p (@options);

#use Data::Dump qw(pp);
#use Data::Dumper;
#print Dumper \ @x;

#print @$_ for @x;
foreach (@powerset) {
  my $rc = system("echo \\'@$_\\'; make clean all MFGEN_FLAGS=-d@$_ >make_@$_.out 2>&1 || echo FAIL");
  # The -d option is there ------------------------------------^ to always have an option after the dash
  # `makefilegen -` will cause an error otherwise
  die "Child caught a SIG" . signame($rc & 127) if ($rc & 127);
}

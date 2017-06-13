###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Kovacs, Ferenc
#   Raduly, Csaba
#
###############################################################################
#!/usr/bin/perl -w

use strict;

if ($] < 5.006) {
  # ancient perl, we must be on Solaris :(
  my @perlloc = qw( /proj/TTCN/Tools/perl-5.10.1/bin/perl /mnt/TTCN/Tools/perl-5.10.1/bin/perl );
  foreach (@perlloc) {
    if (-x $_) {
      #warn "Let's try with $_ instead";
      exec( $_, '-w', $0, @ARGV ) or die "That didn't work either: $!";
    }
  }
}

use vars qw($v);
use Test::Harness; #

if (0 == scalar @ARGV) {
  @ARGV = glob('../../playground/sema/TTCN3_*/  ../../playground/sema/SA_6_TD/  ../../playground/sema/ASN_*/  ver/  xer/  import_of*/  HQ46602/');
  # options/ does not run through harness
}

# Run the file named "t" in each directory.
# Note that the current directory is still Semantic_Analyser, not the subdir.
# The "t" script should change dirs if wanted.
runtests( map { s!/$!!; $_ . '/t' } @ARGV );

__END__

my %args = (
	exec => [ 'make', 'check', '--no-print-directory', '-s', '-C' ],
	verbosity => ($v || 0),
#	show_count => 1,
	color => 1
);
my $harness = TAP::Harness->new( \%args );

# If no argument, collect all sub-directories

$harness->runtests(@ARGV);

__END__

Common runner of all tests.

#!/usr/bin/perl -w
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Baranyi, Botond
#   Raduly, Csaba
#
###############################################################################


use strict;

if ($] < 5.006) {
  # ancient perl, we must be on Solaris :(
  my @perlloc = qw( /proj/TTCN/Tools/perl-5.10.1/bin/perl /mnt/TTCN/Tools/perl-5.10.1/bin/perl );
  foreach (@perlloc) {
    if (-x $_) {
      warn "Let's try with $_ instead";
      exec( $_, $0, @ARGV ) or die "That didn't work either: $!";
    }
  }
}
else {
  require Test::More;
  Test::More->import('no_plan');
}

#use re qw(debug);

my %versions = (
'converter'    => '',
'UTF8'         => '',
'Txerasntypes' => 'R8A',
'Txerboolean'  => 'R8B',
'Txerenum'     => 'R8E',
'Txerfloat'    => 'R8F',
'Txerint'      => 'R8J',
'Txernested'   => 'R8N',
'Txerobjclass' => '',
'Txersets'     => '',
'Txerstring'   => 'R8Z',
'Txerbinstr'   => 'R99999A',
'Asntypes'     => '',
'AsnValues'    => '',
'EmptyUnion'   => '',
'ObjectClass'  => '',
'ObjectClassWithSyntax' => '',
'Sets'      => '',
'SetValues' => '',
'junk' => '',
# built-in modules for the logger
'TitanLoggerApi' => '',
'TitanLoggerControl' => '',
'PreGenRecordOf' => '',
);



while ( <> ) {
  chomp;

  if ( my ($mod, $type, $md5, $rev) =
    m/^
    (\w+) # module name
    \s+
    (TTCN-3|ASN\.1) # type
    \s+
    (?: # dodge the compilation date
    # Hopefully not affected by i18n: this is American style mm dd yyyy
    [A-Z][a-z][a-z] # month
    \s
    [\s\d]\d # day of the month
    \s
    \d{4} # year
    \s
    \d\d # hour
    :
    \d\d # min
    :
    \d\d # sec
    \s+
    )?
    ([0-9a-fA-F]{32})?
    \s*
    (R\d+\w?)?/x ) {
    $md5 ||= '';
    $rev ||= '';
    #print "$mod\t, $type\t, $md5, $rev";
    if (ok(exists $versions{$mod}, " \tKnown module $mod")) {
      #    got, exp, name
      is ($rev, $versions{$mod}, " \tVersion for  $mod");
      delete $versions{$mod};
    }
    else {
      #die "Unknown module name $mod";
      SKIP: {
      skip( " \tversion check for unknown module $mod", 1 );
      }
    }
  }
  #else {warn $_;}
}

#done_testing();

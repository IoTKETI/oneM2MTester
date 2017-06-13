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
#   Kovacs, Ferenc
#   Pandi, Krisztian
#   Raduly, Csaba
#   Szabo, Bence Janos
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


use strict;

die "Need an argument" unless 0 < @ARGV;
open(PRJ, '<' . $ARGV[0]) or die "open prj: $!";

my $prj_dir = $ARGV[0]; # ARGV[0] is the .prj name
my $local_prj = 0;
# Strip all non-slash characters (filename) after the last slash;
# this keeps the directory part. If ARGV[0] was just a filename with no dirs,
# then it's the current directory.
do {
  $prj_dir = './';
  $local_prj = 1;
} unless $prj_dir =~ s!/[^/]+$!/!;

# Pick up parameters from the environment
my $split = defined $ENV{SPLIT_FLAG} ? $ENV{SPLIT_FLAG} : '';
my $rt2   = defined $ENV{RT2}        ? '-R'     : '';

my %files;
my @excluded;
my $cfgfile;

# Line-by-line analysis of an XML file...
while ( <PRJ> )
{
  chomp;
  my $loc;
  if (($loc) = m/path="([^"]+)"/ or ($loc) = m/<(?:Module|Other_Source)>([^<]+)<\/(?:Module|Other_Source)>/) {
    # If it has no path, it's next to the .prj
    # If it has no absolute path, base it off the .prj dir
    if ($loc !~ m{^/}) {
      $loc = $prj_dir . $loc;
    }
    # Testports are stored locally
    $loc =~ s!^.*vobs/.*?/.*?/TestPorts!..!;
    $files{ $loc } = 1;
  }
  elsif ( ($loc) = m{<UnUsed_List>([^<]+)</UnUsed_List>} ) {
    die "Didn't expect another UnUsed_List" if scalar @excluded;
    @excluded = split /,/, $loc;
  }
  elsif ( m[<Config>([^<]+)</Config>] ) {
    $cfgfile = $1;
    # If it has no path, it's next to the .prj
    if ($cfgfile !~ m{/}) {
      $cfgfile = $prj_dir . $cfgfile;
    }
  }
}

close(PRJ) or die "close prj: $!";

# hash slice deletion: deletes all keys in @excluded from %files
delete @files{ @excluded };

my @files = keys %files;


# Filter out files which are in the current directory.
# They should not be symlinked
my @symlinkfiles = $local_prj ? @files : grep { $_ !~ m(^./) } @files;

my @xsdfiles = grep(/.xsd/, @files);

#filter xsd files from all files
my %in_xsdfiles = map {$_ => 1} @xsdfiles;
my @files_without_xsd  = grep {not $in_xsdfiles{$_}} @files;

# Symlink all files into bin/
print "Symlinking " . scalar @symlinkfiles . " files\n";
system("ln -s @symlinkfiles ./");

# Remove the path from the filenames
map { s!.+/!!g } @files_without_xsd;
map { s!.+/!!g } @xsdfiles;
#add the xsd files as other files
my $prefix = "-O ";
my $xsdfiles2 = join " ", map { $prefix . $_ } @xsdfiles;
my $outfile = 'files.txt';
open (FILE, ">> $outfile") || die "problem opening $outfile\n";
my $files_without_xsd_string = join(' ', @files_without_xsd);
print FILE $files_without_xsd_string;
close FILE;

# Generate the makefile
print("LD_LIBRARY_PATH is $ENV{LD_LIBRARY_PATH}\n");#temporary debug printout
print("$ENV{TTCN3_DIR}/bin/ttcn3_makefilegen -gs $split $rt2 -e XmlTest -J files.txt $xsdfiles2 \n");

system(   "\$TTCN3_DIR/bin/ttcn3_makefilegen -gs $split $rt2 -e XmlTest -o Makefile.1 -J files.txt $xsdfiles2");

unlink $outfile;

# Post-process the generated makefile
open(MAKEFILE_IN , '<' . 'Makefile.1') or die "open input: $!";
open(MAKEFILE_OUT, '>' . 'Makefile'  ) or die "open output: $!";

$\ = $/;
# Uncomment the TTCN3_DIR in the makefile
#    (which points to the TTCN3_DIR set in Makefile.cfg)
# Add the bin directory to the fromt of the PATH,
#    so calls to xsd2ttcn without full path still find the "right" one.
while (<MAKEFILE_IN>) {
  chomp;

  # Don't bother editing these settings in the Makefile by hand
  next if s{^(PLATFORM|CXX|CPPFLAGS|CXXFLAGS|LDFLAGS|OPENSSL_DIR|XMLDIR) [:+]?=.*}
  {# $1 Overridden by Makefile.cfg};

  # remove Makefile.1 from OTHER_FILES (added by makefilegen due to -o)
  next if s!(OTHER_FILES = .*) Makefile.1!$1!;

  # Always put in location info; emit GCC-style messages
  next if s/(COMPILER_FLAGS) =.*/$1 := /;

  # RT2 flag is added into TTCN3_COMPILER by Makefile.regression
  next if s!\$\(TTCN3_DIR\)/bin/compiler!\$(TTCN3_COMPILER)!;

  # Include common settings for the regression test.
  # It overrides local settings in the Makefile.
  if ( /Rules for building the executable/ ) {
    print MAKEFILE_OUT <<MKF;
TOPDIR := ../../..
include   ../../../Makefile.regression
export PATH+=:\$(TTCN3_DIR)/bin:
export LD_LIBRARY_PATH+=:\$(ABS_SRC):\$(TTCN3_DIR)/lib:
MKF
  }
}
continue {
  print MAKEFILE_OUT;
}

# Add the 'run' target and a rule to rebuild the Makefile

print MAKEFILE_OUT <<MMM;
run: \$(TARGET) $cfgfile
\t./\$^

Makefile: prj2mk.pl ../src/xmlTest.prj
\tmake -C .. bin/Makefile

MMM

close(MAKEFILE_OUT) or die "close output: $!";
close(MAKEFILE_IN) or die "close input: $!";
unlink('Makefile.1');

__END__

perl -nwle 'print $1 if /path="([^"]+)"/' xmlTest.prj | xargs ttcn3_makefilegen -gs -e XmlTest

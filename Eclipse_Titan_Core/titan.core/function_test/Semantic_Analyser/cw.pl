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
#   Lovassy, Arpad
#   Raduly, Csaba
#
###############################################################################

use strict;

my $numtests; # Gotcha! If you assign a value here, it will take effect
               # _after_ the BEGIN block!

my %first_line_of_module = (); #First line after header by file
my %msg_hash;    # repository of expected messages
# A key is an expected message regex.
# A value is a hash reference:
#     the key is the filename and line number combined with a ':'
#     the value is an array reference;
#        [0] is the number of times the message was found
#            (this is initially zero and it's expected to become precisely 1)
#        [1] is the number of times the message is expected to be found on that line
#            (usually one)
# Here's a Data::Dump of one entry in the hash:
# '^In union field' => {
#   'atr_not_on_record_SE.ttcn:4' => [0, 1],
#   'b64_clash_SE.ttcn:24' => [0, 3],
#   'aa_not_in_record_SE.ttcn:4' => [0, 1]
# },

my $need_error = 0;

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
else
{
  unless ($^C or scalar grep { $_ !~ /^-/; } @ARGV) {
    # Syntax check, or no arguments which look like filenames
    warn 'No arguments given';
    exit 0;
  }

  $numtests = $^C; # If running under -c, pretend to have one test

  #$DB::single = 1;

  # Parse commandline; read each file and collect the expected error messages
  foreach my $arg (@ARGV) {
    if ($arg =~ /^-(.*)/) {
      next if (length $1); # dash followed by something: must be an option
      last; # dash on its own: end of "all" files, start of "out-of-date" files
    }

    # now $arg should be a filename

    if ($arg =~ /_S[WEY]\.(ttcn|asn1?)/) {
      $need_error = 1;
    }

    open( TESTFILE, "< $arg" ) or die "open $arg: $!/$^E";
    my $current_lineno = 1;
    # Can't rely on $. because continuation lines need to pretend to be
    # at the same line as the first line ending in backslash.
    while (<TESTFILE>) {
      chomp;
      if ( /^module / ) {
        #search for the module, and store the line number of the match
        #print "\$first_line_of_module\{ $arg \} = $.\n";
        $first_line_of_module{$arg} = $.;
      }
      while ( s/\\$// ) { # line ends with backslash
        my $next_line = <TESTFILE>;
        last unless defined $next_line;
        chomp $next_line;
        $_ .= $next_line;
      }
      next unless s!//(.+?)//(\d*)!!;    # If //regex// not found, read another line
      my $rex_text = $1;
      my $multiplier = ($2 || 1);

      $msg_hash{$rex_text}->{"$arg:$current_lineno"} = [0, $multiplier];
      ++$numtests;

      redo; # there may be multiple regexes in the same line
    }
    continue {
      $current_lineno = $. + 1;
    } # next line
    close(TESTFILE) or die "close $arg: $!/$^E";
  }

  #warn "Collected $numtests";

  require Test::More;
  # If no regexes found, pretend to have one test.
  Test::More->import( tests => $numtests || 1 );
}



# Something nobody expects
use constant cardinal_jimenez => "Spanish Inquisition";

$ENV{TTCN3_DIR} ||= '../../Install';
my $compiler = $ENV{TTCN3_DIR} . '/bin/compiler';

# Don't confuse Test::Harness
my $quiet = exists $ENV{'HARNESS_ACTIVE'};

if ($0 =~ /SE\.t$/) {
  # If run as a .t, test a single file
  $0 =~ s!^t/!!; # remove the directory prefix, if any
  @ARGV = ( $0 . 'tcn' );
}


my $num_expected = scalar keys %msg_hash;
if ($need_error and 0 == $num_expected) {
  die "No expected errors! Files with _S[WYE] are supposed to contain errors/warnings!";
}

# Transfer messages from the hash to an array.
# Hash keys must be strings, regexes don't work.
# There is no such limitation for array elements.
# There is only sequential access to the messages from this point.
my @mess_ages;
while (my ($key, $val) = each %msg_hash) {
  push @mess_ages, [ qr/$key/, $val ];
}

my @unexpected_msgs;

# Empty the hash so hopefully nobody uses it
%msg_hash = ();
undef %msg_hash;

####### run the compiler and filter the output #######

warn "$compiler @ARGV "  unless $quiet;

my $compiler_pid;
eval {
local $SIG{ALRM} = sub { die "alarm\n" }; # NB: \n required
alarm 600; # seconds, maximum time to wait for the compiler

$compiler_pid = run_compiler ($compiler);

#use Data::Dumper;
# DUMP DUMP DUMP DUMP DUMP DUMP DUMP DUMP DUMP DUMP DUMP DUMP
#print ("After input: ", Dumper \ @mess_ages) if exists $ENV{CW_DUMP};

alarm 0;
$_=0;
}; #eval

if ( $_ ) {
  die ">>$_<<" unless $@ eq "alarm\n";   # propagate unexpected errors
  # timed out
  kill 'TERM', $compiler_pid;
  close( PIPE );
  die "Titan compiler timeout :(" ;
}
else {
  # didn't
}

if ( close(PIPE) ) {
  # compiler exited with success
  # TO DO: fail if there _were_ error regexes
}
else {
  # compiler exited with nonzero
  if ($!) {
    die "close pipe: $!/$^E";
  }
  else {
    # TO DO: fail if there were no error regexes
  }
}


####### Now check what we have found. #######

if ($num_expected == 0) {
  is (scalar @unexpected_msgs, $num_expected, "No messages")
}

{
  foreach my $el (sort {$a->[0] cmp $b->[0]} (@mess_ages, @unexpected_msgs) ) {
    my $e  = $el->[0]; # regex
    my $hr = $el->[1]; # hash ref

    foreach my $loc (sort keys %$hr) {
      my $found = $$hr{$loc};
      if ($found->[0] eq cardinal_jimenez) {
        fail("unexpected message: [$e]");
        print STDERR "$loc:error: unexpected message; [$e]\n"; # unless $quiet; # GCC-like error message
      }
      else {
        if( is( $found->[0], $found->[1], "Finding /$e/ at $loc" ) ) {} # do nothing
        else{
          my $reason = ($found->[0] == 0) ? "not found  message" : "found too many time";
          print STDERR "$loc:error: $reason; [$e]\n"; # unless $quiet;
        }
      }
    }    # foreach location
  }    # foreach message
}

# Transfer all "unexpected" messages into expected messages in the TTCN-3/ASN.1 file
if (exists $ENV{HACK}) {
  foreach my $arg (@ARGV) {
    next if $arg =~ /^-/;
#warn "patching $arg";
    open( TESTFILE, '<' . $arg ) or die "open  $arg: $!/$^E";
    my @content = <TESTFILE>;
    close( TESTFILE )          or die "close $arg: $!/$^E";
    my $is_asn1 = $arg =~ /\.asn/;

    chomp @content; # all lines
    s!//\s*(.+)$!/* $1 */! for @content;

    foreach my $el ( @mess_ages ) {
      my $e  = $el->[0]; # regex
      my $hr = $el->[1]; # hash ref
      while (my ($loc, $found) = each %$hr) {
        if ($loc =~ /$arg/) {
          my $line = $loc;
          $line =~ s/.*://;
          print "found [$e] at $line  x $found->[0]\n";
          #print $content[$line -1]; # array is 0-based
          my $regex = $e;
          $regex =~ s/\(\?-\w+:(.+)\)/$1/;
          my $mult = ($found->[0] > 1) ? $found->[0]+1 : '';
          # This "+1" is an empirical hack----------^^

          if ($is_asn1 and $content[$line -1] !~ m!--\t//!) {
            # If this is an ASN.1 file and this is the first time appending
            # to this particular line, append a '--' first,
            # because ASN.1 doesn't treat // as comment
            $content[$line -1] .= ' --';
          }
          $content[$line -1] .= "\t//$regex//$mult";
        }
      }
    }
    #
    open( TESTFILE, '>' . $arg . '3' ) or die "open  $arg: $!/$^E";
    local $, = "\n";
    print TESTFILE @content;
    close( TESTFILE ) or die "close $arg: $!/$^E";
#    last;
  }
  exit 0;
}

#############################################################################

sub run_compiler {
  my $compiler = shift;

  my $c_pid = open( PIPE, "$compiler @ARGV 2>&1 1>/dev/null | " )
  #                                                           tee compiler.output.parsed |
    or die "open pipe: $!/$^E";

  my $last_loc;
INPUT: while (<PIPE>) {

    #warn $. . \';\' . $_;
    chomp;
    my ( $loc, $error );

    # Titan errors look like:
    # file (for messages applied directly to the module, e.g. circular import)
    # file:line
    # file:line.col
    # file:line.col-col
    # file:line.col-line.col
    # file:line:col (in GCC-mode)
    if (
      /^\s*
        (\S+?:\d+)            # file:line
        (?:                   # non-capturing group
        \. \d+                #   dot and the column number, or
        | \. \d+ - \d+        #   dot and two columns, or
        | \. \d+ - \d+ \. \d+ #   dot, column, dash, line, dot, column, or
        | \: \d+              #   GCC style, colon and column
        )?                    # maybe
        :                     # colon
        \s*                   # maybe some whitespace
        (.+)      # message
        /x
      )
    {
      $loc = $last_loc = $1;
      $error = $2;

      #warn "loc=$1\nerr=$2\n";
    check:
      $error =~ s/note: //;    # compiler -i swallows "note". Compensate it here for -g
      my $found_some = 0;

      # see if one of the "registered" errors matches
      foreach my $e ( @mess_ages ) {
        if ( $error =~ $e->[0] ) {

          my ($found_loc) = $loc =~ /^\s*(\S+?:\d+)/;

          if ( exists $e->[1]->{$found_loc} ) {
            ++$e->[1]->{$found_loc}->[0];

            ++$found_some;    # the message was expected
          }

          # else right message, wrong place
        }
      }

      unless ($found_some) {

        # Looks like an error but wasn\'t matched.
        # Sneak it into the list of expected messages
        # with a marker that says: "this was unexpected".

        $error = quotemeta($error); # don\'t try to guess metacharacters, escape all non-word characters
        $error =~ s/\\(\s)/$1/g;    # but don\'t escape whitespace, readability suffers

        push @unexpected_msgs, [ qr/^$error$/, { $loc => [ cardinal_jimenez, 0 ] }  ];
        # Don't bother checking if $error is already present (for another unexpected).
        # Duplicating the array element works just as well as finding the existing
        # array element and adding to its inner hash.
      }
    }    # if (it looks like an error message)
    elsif (/^\S*compiler\S*: ((?:error|warning): .*?[``](\S+)[''].*)/) { # redundant `' help nedit syntax highlight
      # mycompiler: error: Cannot recognize file `ASN1_Invalid_module_identifier-A.asn3\' ....
      $loc = "$2:$first_line_of_module{$2}";   # assume it contains a `filename\' , pretend to be on the first line after the header
      $error = $1;
      goto check;
    }
    elsif (/^\s*(\S*: )?((?:warning|error|note): .+)/
      or   /^\s*(\S*: )(In .+)/)
    {       # an error/warning without line number
      my $fname = $1;
      if ( defined $fname ) {
				#printf "fname:%s\n", $fname;
        $fname =~ s/: //; # cuts the ": " from the end of the string
        $loc = "$fname:$first_line_of_module{$fname}"; # line number of the line containing "module"
      }
      else {     
        $loc = $last_loc;
      }
      #$loc = defined($1) ? $1 : $last_loc; # guess that it belongs to the last seen line number
      #                   (usually a \'note\' from the error context)
      $error = $2;
      goto check;
    }
  }    # while

  return $c_pid;
}
__END__

Compiler wrapper
First, it reads the TTCN-3 files and extracts the expected messages.
These are regular expressions delimited by // and //
(because // is a comment in TTCN-3 and //xxx// looks somewhat like a sed/Perl/Javascript
regular expression: /xxx/).

Next, it runs the compiler and checks for the expected messages in the output.

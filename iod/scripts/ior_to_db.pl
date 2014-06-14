#! /usr/bin/env perl

use Time::Piece;
use strict;

my %result = ();

sub
flush {
  my $keys = join(', ', keys %result); 
  my $vals = join(', ', values %result);
  print "INSERT IGNORE INTO ior2 ( $keys ) VALUES ( $vals );\n";
  %result = ();
}

sub
convert_sz {
  my ($sz, $hr, $units) = @_;
  my $conversion; 
  if ( $hr =~ /bytes/ ) {
    $conversion = 1;
  } elsif ( $hr =~ /MB/ ) {
    $conversion = 1000 * 1000;
  } elsif ( $hr =~ /MiB/ ) {
    $conversion = 1024 * 1024;
  } elsif ( $hr =~ /GB/ ) {
    $conversion = 1000 * 1000 * 1000;
  } elsif ( $hr =~ /GiB/ ) {
    $conversion = 1024 * 1024 * 1024;
  } else {
    die "unknown size unit $hr";
  }
  my $ret =  $sz * $conversion / $units; 
  return $ret;
}

sub
insert {
  $result{$_[0]} = "'$_[1]'";
}

sub
toepoch {
  my $str = shift;
  my $time = Time::Piece->strptime($str, "%a %b %e %H:%M:%S %Y");
  $time -= $time->localtime->tzoffset;
  return $time->epoch();
}

while(<>) {

  my @tokens = split;

  if (/^IOR-(.*):/) { 
    flush() if (scalar keys %result);
    insert('version', $1);
  } elsif (/^Began: (.*)$/) {
    insert('date', $1);
    insert('epoch', toepoch($1));
  } elsif (/^Command line used: (.*)$/) {
    insert('fullargs', $1);
  } elsif (/^Machine: (.*)$/) {
    insert('host', $1);
  } elsif (/test (filename)\s+= (.*)$/) {
    insert($1,$2);
  } elsif (/(api)\s*= (.*)$/) {
    insert($1,$2);
  } elsif (/(access)\s*= (.*)$/) {
    insert($1,$2);
	} elsif (/ordering in a.*= (.*)$/) {
    insert('intra_ordering', $1);
	} elsif (/ordering inter.*= (.*)$/) {
    insert('inter_ordering', $1);
  } elsif (/clients\s+= (\d+) \((\d+) per/) {
    insert('np', $1);
    insert('ppn', $2);
  } elsif (/(repititions)\s*= (.*)$/) {
    insert($1,$2);
  } elsif (/(xfersize)\s*= (\d+) (.*)$/) {
    insert($1, convert_sz($2,$3,1));
  } elsif (/(aios)\s*= (.*)$/) {
    insert($1,$2);
  } elsif (/^IOD Persist Time: (.*)$/) {
    insert('persist_time', $1); 
  } elsif (/(repititions)\s*= (.*)$/) {
    insert($1,$2);
  } elsif (/(blocksize)\s*= (\d+) (.*)$/) {
    insert("$1_kb", convert_sz($2,$3,1024));
    insert("$1", convert_sz($2,$3,1));
  } elsif (/aggregate.*= (\d+) (.*)$/) {
    insert('total_size_mb', convert_sz($1,$2,1048576));
  } elsif (/^Max (Write)/ or /^Max (Read)/) {
    insert("max_" .lc($1), $tokens[2]);
  } elsif (/^Finished: (.*)$/) {
    my $end = toepoch($1);
    my ($begin) = $result{'epoch'} =~ /(\d+)/;
    my $elap = $end - $begin;
    insert('total_elapsed_seconds', $elap); 
  } elsif (/^remove/) {
    insert('unlink_time', $tokens[7]);
  } elsif (/^(read)/ or /^(write)/) {
    if (scalar @tokens == 9) {
      # don't get the summary line
      insert("$1_bw", $tokens[1]);
      insert("$1_open_time", $tokens[4]);
      insert("$1_op_time", $tokens[5]);
      insert("$1_close_time", $tokens[6]);
      insert("$1_total_time", $tokens[7]);
    }
  }

}

flush() if (scalar keys %result); # last one

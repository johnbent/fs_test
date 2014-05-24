#! /usr/bin/env perl

# pipe an IOD debug log into this and it will count the
# number of times that 'INFO EXIT' is called for each
# function for which it is called.

my %hash = ();

while(<>) {
  chomp;
  next unless /INFO EXIT/;
  my @tokens = split /\s+/;
  $key = $tokens[7]; 
  $hash{$key}++;
}

my @keys = sort { $hash{$b} <=> $hash{$a} } keys %hash;
foreach my $key ( @keys ) {
  printf "%-20s %6d\n", $key, $hash{$key};
}

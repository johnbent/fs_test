#! /bin/env perl

use strict;

my $file = shift @ARGV;
my @mods = @ARGV;
my %keys = ();

sub
Usage {
    warn "Usage: $0 /path/to/config/file [key=set,of,values]+\n";
    exit 0;
}

sub
TestKeys {
    foreach my $key ( keys %keys ) {
        print $key . "\n";
        my @vals = @{$keys{$key}};
        foreach my $val ( @vals ) {
            print "\t$val\n";
        }
    }
}

sub
readfile {
    (my $filename) = @_;
    open FILE,"$filename" or die $!;
    my @lines = <FILE>;
    close FILE or die $!;
    return @lines;
}

sub
modify {
    my ($k, $v) = @_;
    my $modified = 0; 
    foreach my $key ( keys %keys ) {
        if ( $k eq $key ) {
            #print "MATCH: match on $k\n";
            my @vals = @{$keys{$key}};
            my $which_val = rand(scalar @vals);
            $v = $vals[$which_val];
            #print "REPLACE $key with $v\n";
            $modified = 1;
        }
    }
    return ($k, $v);
}

Usage() unless ( -e $file && scalar @mods > 0);

foreach my $mod ( @mods ) {
    my ($key,$val) = split(/=/,$mod);
    Usage() unless length($val);
    my @vals = split(/,/,$val);
    #print "$key = @vals\n";
    $keys{$key} = \@vals;
}

#TestKeys();

my @lines = readfile($file);

open OUTPUT,">$file" or die $!;
foreach my $line ( @lines ) {
    #print "LINE" . $line . "\n";
    my ($k, $v, @rest) = split /\s/, $line ;
    #print "$k -> $v\n";
    ($k, $v) = modify($k, $v);
    #print $line;
    print "$k $v @rest\n";
    print OUTPUT "$k $v @rest\n";
}
close OUTPUT or die $!;

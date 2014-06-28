#! /bin/env perl

use IPC::Open2;

use strict;

my $bigstep=0;
my $littlestep;
my $cname = "containerA";

my $finishes = "";
my $totals = "";
my $failures = "";

my $cur_agg_count = 0;
my $cur_agg_total = 0;
my $cur_agg_type = "";
my $cur_agg_units = "";
my @data;
my @daos;

my $program = "./M8_demo.sh";
my $data_query = "./query_data.sh";

if ( 0 ) {
    $program = "cat log.nopause.1";
    undef $data_query;
}

my %summaries = (
    '2.5' => 'Write Array TID=1',
    '2.7' => 'Write Blob TID=1',
    '2.9' => 'Write KV TID=1',
    '3.3' => 'Read Array TID=1 BB',
    '3.5' => 'Read Blob TID=1 BB',
    '3.7' => 'Read KV TID=1 BB',
    '4.5' => 'Read Array TID=1 DAOS',
    '4.6' => 'Read Blob TID=1 DAOS',
    '4.7' => 'Read KV TID=1 DAOS',
    '5.3' => 'Read Array TID=1.1 BB local',
    '5.4' => 'Read Array TID=1.1 BB remote',
    '5.7' => 'Read Blob TID=1.1 BB local',
    '5.8' => 'Read Blob TID=1.1 BB remote',
    '5.11' => 'Read KV TID=1.1 BB local',
    '5.12' => 'Read KV TID=1.1 BB remote',
    '6.2'  => 'Write Array TID=2',
    '6.3'  => 'Write Blob TID=2',
    '6.4'  => 'Write KV TID=2',
    '6.7'  => 'Write Array TID=4',
    '6.8'  => 'Write Blob TID=4',
    '6.9'  => 'Write KV TID=4',
    '6.12'  => 'Write Array TID=3',
    '6.13'  => 'Write Blob TID=3',
    '6.14'  => 'Write KV TID=3',
    '7.1.1' => 'Read Array TID=1 DAOS',
    '7.1.2' => 'Read Blob TID=1 DAOS',
    '7.1.3' => 'Read KV TID=1 DAOS',
    '7.2.1' => 'Read Array TID=2 Mixed',
    '7.2.2' => 'Read Blob TID=2 Mixed',
    '7.2.3' => 'Read KV TID=2 Mixed',
    '7.3'   => 'Read Array TID=4 Mixed',
    '7.4'   => 'Read Blob TID=4 Mixed',
    '7.5'   => 'Read KV TID=4 Mixed',
    '8.4'   => 'Read Array TID=4 DAOS',
    '8.5'   => 'Read Blob TID=4 DAOS',
    '8.6'   => 'Read KV TID=4 DAOS',
);

sub roll_agg() {
    if ($cur_agg_count > 0) {
        my $step = "$bigstep $littlestep";
        if (defined $summaries{$littlestep}) {
            $step = "$littlestep $summaries{$littlestep}"; 
        } else {
            die "$step not defined in " . join(", ", keys %summaries ) . "\n";
        }
        my $msg = "$step $cur_agg_type total " 
                . $cur_agg_total / $cur_agg_count
                . " $cur_agg_units\n"; 
        print $msg;
        $totals .= "\t$msg";
        $cur_agg_count = 0;
        $cur_agg_total = 0;
    } else {
        print "not rolling count $cur_agg_count $cur_agg_total $cur_agg_type\n";
    }
}

sub dump_storage() {
    if (defined $data_query) {
        open(DUMP, "./query_data.sh $cname |") || die "open";
        while(<DUMP>) {
            $data[$bigstep] .= "\t$_";
            print;
        }
        close DUMP or die "close";
    } else {
        $data[$bigstep] .= "\tENOENT\n";
    }
    if (-e "/mnt/daos/$cname" ) {
        print "Query daos\n";
        open(DAOS, "daos_query.sh /mnt/daos/$cname |") || die "open";
        while(<DAOS>) {
            next if (/^#/); # skip comments
            $daos[$bigstep] .= "\t$_"; 
            print;
        }
        close DAOS or warn "close";
    } else {
        $daos[$bigstep] = "\tENOENT\n";
    }
}
    
#print join(", ", keys %summaries) . "\n"; 

open2(\*DEMOOUT, \*DEMOIN, "$program") || die "open";
while(<DEMOOUT>) {
    print;
    if (/Press enter to continue/) {
        print "autoprogress\n";
        # could do <STDIN> here to pause
        print DEMOIN "\n";
    } elsif (/Step (\d):/) {
        print "On Big Step $1\n";
        $bigstep = $1;
        roll_agg();
        dump_storage();
        #last if $bigstep > 2; # exit early for debugging
    } elsif (/Step (.*) -/) {
        print "On Little Step $1\n";
        roll_agg();
        $littlestep = $1;
    } elsif (/(.*) performance: AGG: (.*) (.*) \[ (.*)\/(.*) S \]/) {
        roll_agg() unless $cur_agg_type eq $1;
        $cur_agg_count += 1;
        $cur_agg_total += $2;
        $cur_agg_type = $1;
        $cur_agg_units = $3;
        print "AGG $1 $2 $3 $cur_agg_count\n";
    } elsif (/^Finish .* second/) {
        $finishes .= "\t$bigstep $littlestep $_";
    } elsif (/failed/) {
        print "UH OH $_";
        $failures .= "\t$bigstep $littlestep $_";
        die "Encountered failure. Exit early\n";
    }
}
close DEMOOUT or warn "close";
close DEMOIN or warn "close";

END {
    roll_agg();
    for my $i (0 .. $#data) {
        print "Data at bigstep $i\n";
        print $data[$i];
    }

    for my $i (0 .. $#daos) {
        print "DAOS at bigstep $i\n";
        print $daos[$i];
    }

    print "FINISHES\n";
    print $finishes;

    print "TOTALS\n";
    print $totals;

    if (length($failures)) {
        print "FAILURES\n";
        print $failures;
    }
}

#! /usr/bin/env perl
#
# Author wen <wen@Saturn>
# Version 0.1
# Copyright (C) 2016 wen <wen@Saturn>
# Modified On 2016-01-29 13:43
# Created  2016-01-29 13:43
#
use strict;
use warnings;
use Data::Dumper;

my $trace;
my %pidmap = ();
my $num_threads = 0;
my @sched_seq;
my @pid_array;

open ($trace, $ARGV[0]) or die "Unable to open $ARGV[0]";

while (chomp(my $str = <$trace>)) {
	my @pids = $str =~ /token from (\d+)\[(\d+)\]\<\d+\> to (\d+)\[(\d+)\]\<\d+\>/g;
	if (scalar @pids == 4) {
		if (not exists $pidmap{$pids[0]}) {
			$pidmap{$pids[0]} = 'x';
			push @pid_array, $pids[0];
			$num_threads = $num_threads + 1;
		}
		if (not exists $pidmap{$pids[2]}) {
			$pidmap{$pids[2]} = 'x';
			push @pid_array, $pids[2];
			$num_threads = $num_threads + 1;
		}
#		if ($pids[0] != $pids[2]) {
			my @sch = ($pids[2], $pids[3]);
			push @sched_seq, \@sch;
#		}
	}
}
@pid_array = sort @pid_array;
foreach my $line_ref (@sched_seq) {
	foreach my $pid (@pid_array) {
		if ($pid eq @$line_ref[0]) {
			print "|\t@$line_ref[1]";
		} else {
			print "|\t ";
		}
	}
	print "|\n";
}

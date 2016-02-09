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
	my @pids = $str =~ /token from (\d+) to (\d+)/g;
	if (scalar @pids == 2) {
		if (not exists $pidmap{$pids[0]}) {
			$pidmap{$pids[0]} = 'x';
			push @pid_array, $pids[0];
			$num_threads = $num_threads + 1;
		}
		if (not exists $pidmap{$pids[1]}) {
			$pidmap{$pids[1]} = 'x';
			push @pid_array, $pids[1];
			$num_threads = $num_threads + 1;
		}
		if ($pids[0] != $pids[1]) {
			push @sched_seq, $pids[1];
		}
	}
}
#print Dumper(%pidmap);
@pid_array = sort @pid_array;
foreach my $line_ref (@sched_seq) {
	foreach my $pid (@pid_array) {
		if ($pid eq $line_ref) {
			print "|\tx";
		} else {
			print "|\t ";
		}
	}
	print "|\n";
}

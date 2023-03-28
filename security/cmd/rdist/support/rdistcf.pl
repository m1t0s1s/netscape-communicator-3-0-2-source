#! /usr/usc/bin/perl
#
# $Id: rdistcf.pl,v 1.1 1996/01/31 23:30:32 dkarlton Exp $
#
# compact-rdist-output: takes output of cooper's rdist and sorts 
# it by host, producing a more compact report for casual perusal.
#
# Based on suggestions by J Greely.  code by Steve Romig.
# 

# Basic idea: for each line of rdsit output (assumed to be of form
# "host: message", record the list of hosts that it was seen on.  Sort
# the messages in order of increasing number of hosts, and print a
# report that lists each set of hosts and the messages that appeared for
# those hosts, running from least number of hosts to the greatest.
#
# In other words, if rdist was run on bird, tree, fish and fruit, the
# output might look something like this:
#
# fish:
#     installing /export/local/bin/less
#
# bird tree fruit:
#     updating /export/local/bin/less
#

#
# Format to use for printing the start of the report.
#
format STDOUT =
user:        @<<<<<<<<<<<<<<<<<<<<<<<<<
             $user
directories: ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	     $dirs
~~           ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	     $dirs
hosts:	     ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
             $hosts
~~           ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
             $hosts
.

#
# Format to use for body
#
format BODY = 
^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<~~
$host_string
@*
$host_data

.

#
# Use this to sort the lists of hosts by increasing number of hosts.
#
sub increasing {
    $host_number{$a} <=> $host_number{$b};
}

if ($#ARGV == 2) {
    $user = shift(@ARGV);
    $dirs = shift(@ARGV);
    $hosts = shift(@ARGV);
    write;
    print "\n";
} elsif ($#ARGV != -1) {
    die "usage: compact-rdist-output [user dirs hosts]\n";
}

#
# read lines of rdist output, deal with the types of messages that 
# appear
#
$lineno = 0;
while (<>) {
    #
    # ignore these
    #
    next if /: updating of /;
    next if /: updating host /;
    next if /^updating /;
    next if /^notify /;
    #
    # if it's a host: line, save it...
    #
    if (/^(\S+):[ \t]+(.*)$/) {
	$data = $2;
	if (defined($seen{$data})) {
	    $seen{$data} .= " $1";
	} else {
	    $seen{$data} = $1;
	}
    #
    # otherwise print it at the top of the report
    #
    } else {
	print "$_\n";
    }
}

#
# for each distinct message, work out the list of hosts that it 
# appeared on (sorted in alphabetical order) and update the 
# report text for that list of hosts.
#
foreach $line (keys %seen) {
    @host_list = sort split(/ /, $seen{$line});
    $host_string = join(' ', @host_list);
    $seen{$line} = $host_string;

    #
    # save the number of hosts in this list - use that for 
    # sorting the report later
    #
    $host_number{$host_string} = $#host_list;
    #
    # append this message to the text for this list of hosts
    #
    $host_text{$host_string} .= "    " . $line . "\n";
}

$: = " \n";	# Break characters
$~ = BODY;	# Format to use

#
# sort the host lists by increasing number, and print the 
# report for each.  The text data is sorted numerically by
# line number.
#
foreach $host_string (sort increasing keys %host_number) {
    #
    # Split the text data into an array, then do a sort
    # and put the result back into a normal string variable.
    #
    $host_data = join("\n", sort split(/\n/, $host_text{$host_string}));

    # print the data
    write;
}

exit 0;


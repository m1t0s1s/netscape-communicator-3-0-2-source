#!/usr/usc/bin/perl
#
# $Id: rdistvf.pl,v 1.1 1996/01/31 23:30:33 dkarlton Exp $
#
# rdistvfilter - Verbose filter of rdist messages.  Takes the output from
#		 rdist and outputs a much nicer, though verbose, form.
#
# [mcooper]	5/9/88
#

$tmp = "/tmp/rdistfilter.$$";
open(OUTF, "|sort >$tmp") || die "Can not open tmp file.\n";

while (<>) {
    chop;

    # Remove any garbage we might find
    s/[\000-\007\016-\037]//g;

    #
    # The point of the below code is to try to extract and save the
    # name of the host messages are for.  If an input line doesn't
    # look like a normal rdist message, it's probably something like
    # "Permission denied".  In this case, we assume the last saved host
    # name is associated with this message and print the line with that
    # host name.
    #
    if ((/updating of /) || (/updating host /)) {
	@Fields = split;
	$Host = @Fields[2];
	$Host =~ s/\..*//; 	# Strip domain name
    } elsif (/:/) {
	@Fields = split;
	$Host = @Fields[0];
	$Host =~ s/://;
	$Host =~ s/\..*//; 	# Strip domain name

	$tmpname = $Host . ":";
	printf OUTF "%-12s", $tmpname;
	for ($i = 1; $i <= $#Fields; $i++) {
	    printf OUTF " %s", $Fields[$i];
	}
	printf OUTF "\n";
    } elsif ($_) {
	if ($Host) {
	    $tmpname = $Host . ":";
	    printf OUTF "%-12s", $tmpname;
	}
	printf OUTF "%s\n", $_;
    }
}

close(OUTF);
open(OUTF, "$tmp") || die "Cannot open tmp file.\n";
$last = "";
$ll = "";
$lc = 0;

while (<OUTF>) {
    ($current) = split(/\t| /);
    if ($last && ($last ne $current)) {
	printf "\n";
    }
    $last = $current;
    $_ =~ s/\n//;
    if ($ll eq $_) {
	$lc++;
    } else {
	printf $_;
	if ($lc > 1) {
	    $cs = sprintf(" (x%d)", $lc);
	} else {
	    $cs = "";
	}
	printf "\n"; # printf "%s\n", $cs;
	$lc = 0;
	$cs = "";
	$ll = $_;
    }
}

unlink $tmp;

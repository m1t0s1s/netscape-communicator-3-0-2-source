#!/usr/local/bin/perl5

# script to generate data that should be pasted into allxpstr.h -- 
# this is used by convhtml.sh

$string_number = 1000;


sub read_js {

    $js_var_count = 0;
    @js_vars = ();
    %js_values = ();

    my $nxt = "";
    
  EOF: while ( 1 ) {
      my $v = 0;
      my $r = 0;
      my $all = $nxt;

      $nxt = "";

    DONE: while (!$v) {
	$_ = <>;
	chop($_);

	last DONE if (length($_) == 0);
	$nxt = $_;
	( $v, $r ) = m/^var ([a-z0-9_]+) *= *(.*)/i;
	last DONE if ($v);

	my $new1 = $_;
	my $new2 = $new1;
	my $all2;

	$_ = $all;
	( $all2 ) = m/(.*)\\042 *\+ */;
	if (! $all2 ) {
	    $all2 = $all;
	} else {
	    $_ = $new1;
	    ( $new2 ) = m/^ *\\042(.*)/;
	    if (! $new2 ) { $new2 = $_; }
	}

	$all = $all2 . $new2;
    }

      if ( length($all) > 0) {
	  $_ = $all;
	  ( $v, $r ) = m/^var ([a-z0-9_]+) *= *\\042(.*)\\042;/i;

	  $js_vars[$js_var_count] = $v;
	  $js_values{$v} = $r;
	  $js_var_count++;
      }
      last EOF if (length($nxt) == 0);
  }
}

sub print_keys {

    print "/* NOTE: THE FOLLOWING STRINGS SHOULD *NOT* BE LOCALIZED!\n\n";
    print "   These strings (from SA_VARNAMES_BEGIN to SA_VARNAMES_END)\n";
    print "   are not user-visible; changing them will break the Security\n";
    print "   Advisor dialog.  The only reason they are in this file is so\n";
    print "   that they will end up in the resource segment rather than\n";
    print "   static-data segment on Windows.  Probably this should be\n";
    print "   done in some other way, so that these don't clutter up the\n";
    print "   localization data, but for now...\n";
    print " */\n";

    print "ResDef(SA_VARNAMES_BEGIN, ";
    print "(SEC_DIALOG_STRING_BASE + $string_number), \"\")\n";
    $string_number++;

    foreach $v (@js_vars) {
	my $r = $v;
	$r =~ tr#A-Z#a-z#;
	$v =~ s/^SA_/SA_VARNAME_/;
	print "ResDef($v, ";
	print "(SEC_DIALOG_STRING_BASE + $string_number), \"$r\")\n";
	$string_number++;
    }

    print "ResDef(SA_VARNAMES_END, ";
    print "(SEC_DIALOG_STRING_BASE + $string_number), \"\")\n";
    $string_number++;
}


sub print_values {
    my $v;

    print "ResDef(SA_STRINGS_BEGIN, ";
    print "(SEC_DIALOG_STRING_BASE + $string_number), \"\")\n";
    $string_number++;

    foreach $v (@js_vars) {
	my $r = $js_values{$v};

	$v =~ tr#a-z#A-Z#;
	my $str = "ResDef(${v}, (SEC_DIALOG_STRING_BASE + $string_number),";
	print $str;

	if ((length($r) + length($str)) > 75) {
	    print "\n";
	}

	if (length($r) > 200) {
	    my $v2;
	    my $L;
	    ($v2 = $v) =~ tr#A-Z#a-z#;
	    $L = length($r);
	    printf STDERR "DANGER: string $v2 is $L long.\n";
	}

	print " \"";

	# Wrap the lines at <72 characters; but try to back up to whitespace
	# to word-wrap them, just so they end up being more readable.
	while (length($r) > 72) {
	    my $L = 72;
	    $_ = substr($r,0,$L);
	    ( $r2 ) = m/(.*?) [^ ]+$/;
	    if ( $r2 && length($r2) > 0) {
		$L = length($r2);
	    }
	    print substr($r,0,$L), "\\\n";
	    $r = substr($r,$L);
	}
	if (length($r) > 0) {
	    print "$r";
	}

	print "\")\n";
	$string_number++;

	$all = "";
    }

    print "ResDef(SA_STRINGS_END, ";
    print "(SEC_DIALOG_STRING_BASE + $string_number), \"\")\n";
    $string_number++;
}

read_js;
print_values;
print "\n";
print_keys;

exit 0;


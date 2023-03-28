#! /usr/local/bin/perl

# This file walks the tree looking at all the RCS files,
# and prints the log information for all RCS versions which
# meet the date criteria passed on the command line.

sub find {
    chop($cwd = `pwd`);
    foreach $topdir (@_) {
	(($topdev,$topino,$topmode,$topnlink) = stat($topdir))
	  || (warn("Can't stat $topdir: $!\n"), next);
	if (-d _) {
	    if (chdir($topdir)) {
		($dir,$_) = ($topdir,'.');
		$name = $topdir;
		&wanted;
		$topdir =~ s,/$,, ;
		&finddir($topdir,$topnlink);
	    }
	    else {
		warn "Can't cd to $topdir: $!\n";
	    }
	}
	else {
	    unless (($dir,$_) = $topdir =~ m#^(.*/)(.*)$#) {
		($dir,$_) = ('.', $topdir);
	    }
	    $name = $topdir;
	    chdir $dir && &wanted;
	}
	chdir $cwd;
    }
}

sub finddir {
    local($dir,$nlink) = @_;
    local($dev,$ino,$mode,$subcount);
    local($name);

    # Get the list of files in the current directory.

    opendir(DIR,'.') || (warn "Can't open $dir: $!\n", return);
    local(@filenames) = readdir(DIR);
    closedir(DIR);

    if ($nlink == 2) {        # This dir has no subdirectories.
	for (@filenames) {
	    next if $_ eq '.';
	    next if $_ eq '..';
	    $name = "$dir/$_";
	    $nlink = 0;
	    &wanted;
	}
    }
    else {                    # This dir has subdirectories.
	$subcount = $nlink - 2;
	for (@filenames) {
	    next if $_ eq '.';
	    next if $_ eq '..';
	    $nlink = $prune = 0;
	    $name = "$dir/$_";
	    &wanted;
	    if ($subcount > 0) {    # Seen all the subdirs?

		# Get link count and check for directoriness.

		($dev,$ino,$mode,$nlink) = lstat($_) unless $nlink;
		
		if (-d _) {

		    # It really is a directory, so do it recursively.

		    if (!$prune && chdir $_) {
			&finddir($name,$nlink);
			chdir '..';
		    }
		    --$subcount;
		}
	    }
	}
    }
}

if ($#ARGV < 1) {
  print "Error, you must provide at least two arguments, the first is the\n";
  print "rcs date tag, and the rest are the directories to search.\n";
  exit(1);
}

$date=$ARGV[0];
shift(@ARGV);

sub wanted {
  if (/.*,v$/ && ! ($dir =~ /\/Attic$/)) {
    open(RLOG,"rlog -d\"$date\" $name|");
    $state="waiting";
    reading: while (<RLOG>) {
      if (/^================================================/) { last reading; }
      if ($state eq "waiting") {
	if (/^--------/) { $state="found"; }
      } elsif ($state eq "found") {
	print "\nfile: $name; $_";
	$state="log";
      } elsif ($state eq "log") {
	if (/^--------/) { $state="found"; }
	else { print $_; }
      }
    }
    close(RLOG);
  }
}

&find(@ARGV);

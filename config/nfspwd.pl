#! /usr/local/bin/perl

require "fastcwd.pl";

$_ = &fastcwd;
if (m@^/[uh]/@o || s@^/tmp_mnt/@/@o) {
    print("$_\n");
} elsif ((($user, $rest) = m@^/usr/people/(\w+)/(.*)@o)
      && readlink("/u/$user") eq "/usr/people/$user") {
    print("/u/$user/$rest\n");
} else {
    chop($host = `hostname`);
    print("/h/$host$_\n");
}

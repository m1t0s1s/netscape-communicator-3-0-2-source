#! /usr/local/bin/perl

unshift(@INC, '/usr/lib/perl');
unshift(@INC, '/usr/local/lib/perl');

require "fastcwd.pl";

$cur = &fastcwd;
chdir($ARGV[0]);
$newcur = &fastcwd;
$newcurlen = length($newcur);

# Skip common separating / unless $newcur is "/"
$cur = substr($cur, $newcurlen + ($newcurlen > 1));
print $cur;

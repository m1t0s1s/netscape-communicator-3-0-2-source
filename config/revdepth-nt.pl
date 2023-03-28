#! /usr/local/bin/perl

require "/ns/config/fastcwd.pl";

$cur = &fastcwd;
chdir($ARGV[0]);
$newcur = &fastcwd;
$newcurlen = length($newcur);

# Skip common separating / unless $newcur is "/"
$cur = substr($cur, $newcurlen + ($newcurlen > 1));
print $cur;

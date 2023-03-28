#! /usr/local/bin/perl
open(PA_TAGS, "<pa_tags.h");
open(HASH, "|/usr/local/bin/gperf -T -t -l -Npa_LookupTag -p -k1,\$,2,3 > gperf.out.$$");
print HASH 'struct pa_TagTable { char *name; int id; };
%%
';

open(RMAP, ">pa_hash.rmap");
$nextval = 0;

while (<PA_TAGS>) {
  if (/^#[ 	]*define[ 	]([A-Z_][A-Z0-9_]+)[ 	]*(.*)/) {
    $var = $1;
    $val = $2;
    $val =~ s/"//g;
    $pre = $var;
    $pre =~ s/_.*//;
    $post = $var;
    $post =~ s/$pre//;
    $post =~ s/_//;
    if ($pre eq "PT") {
      $strings{$post} = $val;
    } elsif ($pre eq "P") {
      if ($strings{$post} ne "") {
        print HASH $strings{$post} . ", $var\n";
      }
      if ($var ne "P_UNKNOWN" && $var ne "P_MAX") {
	while ($nextval < $val) {
	  print RMAP "/* $nextval */\t\"\",\n";
	  $nextval++;
	}
        print RMAP "/*$val*/\t\"$strings{$post}\",\n";
	$nextval = $val + 1;
      }
    }
  }
}
close(PA_TAGS);
close(HASH);
close(RMAP);
open(C, "<gperf.out.$$");
unlink("gperf.out.$$");
open(T, "<pa_hash.template");

while (<T>) {
  if (/^\@begin/) {
    ($name, $start, $end) =
      m#\@begin[ 	]*([A-Za-z0-9_]+)[ 	]*/([^/]*)/[ 	]*/([^/]*)/#;
    $line = <C> until (eof(C) || $line =~ /$start/);
    if ($line =~ /$start/) {
      $template{$name} .= $line;
      do {
	$line = <C>;
	$template{$name} .= $line;
      } until ($line =~ /$end/ || eof(C));
    }
  } elsif (/^\@include/) {
    ($name) = /\@include[ 	]*(.*)$/;
    print $template{$name};
  } elsif (/^\@sub/) {
    ($name, $old, $new) =
      m#\@sub[ 	]*([A-Za-z0-9_]*)[ 	]/([^/]*)/([^/]*)/#;
    $template{$name} =~ s/$old/$new/g;
  } elsif (/^\@/) {
    ;
  } else {
    print $_;
  }
}

#!/usr/bin/perl
#
#	Perl file to generate unicode.rc for Windows build
#	The result will be put into ns/cmd/winfe/res/unicode.rc
#	Currently I check in the result
#	Maybe we should put this into the automatic build process
#	This perl script simply gather severl files and dump into
#	the ns/cmd/winfe/res/unicode.rc file
#
open ( STDOUT , "> y:\\ns\\cmd\\winfe\\res\\unicode.rc" ) || die " Cannot create unicode.rc" ;
print "/* File: unicode.rc 				*/\n";
print "/* Created by unicode.perl 			*/\n";
print "/* Please don't edit it 			*/\n";
print "/* If you have any problem/question 		*/\n";
print "/* Please contact Frank Tang 			*/\n";
print "/* 		<ftang@netscape.com> x2913	*/\n";

@codeset = (
	"cp1252", "8859-2", "8859-7", "8859-9", 
	"macsymbo" , "macdingb",
	"cp1251" ,  "cp1250", "koi8r",
	"sjis", "ksc5601", 
	"big5", "gb2312"
	);

foreach $codeset (@codeset)
{
	printf "$codeset.uf RCDATA\nBEGIN\n" ;

	open (FILE, "< y:\\ns\\lib\\libi18n\\unicode\\ufrmtbl\\$codeset.uf") ||
		die "Cannot open $codeset.uf";
	print <FILE>;
	close(FILE);

	printf "END\n\n\n";

	printf "$codeset.ut RCDATA\nBEGIN\n" ;

	open (FILE, "< y:\\ns\\lib\\libi18n\\unicode\\utotbl\\$codeset.ut") ||
		die "Cannot open $codeset.ut";
	print <FILE>;
	close(FILE);

	printf "END\n\n\n";
}

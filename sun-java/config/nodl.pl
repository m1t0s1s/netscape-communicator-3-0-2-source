#! /usr/local/bin/perl

# Print out the nodltab.
# Usage: nodl.pl table-name sym1 sym2 ... symN

$table = $ARGV[0];
shift(@ARGV);

print "/* Automatically generated file; do not edit */\n\n";

print "#include \"prtypes.h\"\n\n";
print "#include \"prlink.h\"\n\n";

foreach $symbol (@ARGV) {
    print "extern void ",$symbol,"();\n";
}
print "\n";

print "PRStaticLinkTable ",$table,"[] = {\n";
foreach $symbol (@ARGV) {
    print "  { \"",$symbol,"\", ",$symbol," },\n";
}
print "  { 0, 0, },\n";
print "};\n";

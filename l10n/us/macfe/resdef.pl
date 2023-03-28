#! /usr/local/bin/perl

# This script reads a series of lines of the form:
# resource 'STR ' ( id+100, "name", purgeable)(NAME, ID, "STRING")
# and generates stuff like:

open(OUT, ">xpstring.r");
print OUT "// This is a generated file, do not edit it\n";
print OUT "#include \"Types.r\"\n";
while (<>) {
  ($enum, $eid) =
    /[ 	]*([A-Za-z0-9_]+)[ 	]*\=[ 	]*([A-Za-z_x0-9]+),/;
	 if ($enum ne "") {
    print OUT "#define $enum $eid \n";
  }
  ($name, $id, $string) =
    /ResDef[ 	]*\([ 	]*([A-Za-z0-9_]+)[ 	]*,[ 	]*([\(\)0-9A-Za-z_x\-+ ]+)[ 	\01]*,[ 	]*(".*")[ 	]*\)/;
	 if ($name ne "") {
    print OUT "resource 'STR ' (($id)+7000, \"$name\", purgeable)\n{\n\t";

	$_ = $string;
	s/([^.:])\\n/$1 /g;
	s/(\\n) /$1/g;
	print OUT "$_";
	
	print OUT ";\n};\n";
  }
}
close(OUT);

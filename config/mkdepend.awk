BEGIN {
	sourcefile = FILE
}

{
	if( $1 == "#line" ) {
		dependancies[$3] = 1;
	}
}


END {
	printf(".\\$(OBJDIR)\\%s.obj: ", sourcefile);
	for (d in dependancies) {
		if( index(d, ":\\") != 3 ) {
			printf(" \\\n        %s", d);
		}
	}
	printf("\n\n");
}

BEGIN { 
    print "char *opnames[256] = {" 
}
/^[a-z]/ { 
    printf("   \"%s\",\n", $1); i++; 
}
END { 
    while(i++ < 256)  {
	print "   \"??\",";
    }
	print "};" 
}

BEGIN { 
    print "short opcode_length[256] = {"
}
/^[a-z]/ { 
    printf("   %s,\t\t/* %s */\n", $2, $1); i++;
}
END { 
    while(i++ < 256) {
	print "   -1,"; 
    }
	print "};"
}

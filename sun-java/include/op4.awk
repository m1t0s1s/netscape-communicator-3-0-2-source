BEGIN { 
    print "char *opcode_in_out[][2] = {"
}
/^[a-z]/ { 
    if ($3 == "-") $3 = "";
    if ($4 == "-") $4 = ""; \
    printf("   {\"%s\", \"%s\"}, \t\t/* %s */\n", $3, $4, $1);
}
END { 
    print "};"
}

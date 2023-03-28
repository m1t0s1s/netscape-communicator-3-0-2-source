BEGIN    {
	print "typedef enum {" 
}  
/^[a-z]/ {
	printf("  opc_%s = %d,\n", $1, i++); 
}  
END   {
	printf("  opc_first_unused_index = %d\n} opcode_type;\n", i++) 
}  

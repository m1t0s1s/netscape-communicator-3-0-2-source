#include <stdio.h>
#include "seccomon.h" 
#include "secasn1.h"
#include "secitem.h"
#include "secdert.h"
#include "secder.h"
/*
/* SMIME -Test 001- IA5String string  Simple test to check encoding and decoding API. 
/* Author: Annalee L.Garcia
*/  


int main(int argc, char **argv)
{

    PRArenaPool *arena;
    SECItem encoded_item, encoded_output,decoded_data;
    SECItem *dummy;
	SECStatus rv;
	void *mark;
	int rc=0;
	
    printf ("\n Begin sim_pos_ia5string string test \n");
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    encoded_output.data = NULL;
	decoded_data.data = NULL;

    if ( ! arena ) {
			printf ("Error in creating arena, test failed \n");
			goto loser;
	}
  
	encoded_item.data = "This is the test_string that is going to be encoded and decoded. The output will then be compared";
	encoded_item.len = strlen(encoded_item.data);
	
    dummy = SEC_ASN1EncodeItem (arena, &encoded_output, &encoded_item,
								SEC_IA5StringTemplate);
	
	if (dummy == NULL) {
	    printf ("Encoding item failed  \n");
		goto loser;
	}

	rv = SEC_ASN1DecodeItem (arena, &decoded_data, SEC_IA5StringTemplate,
							   &encoded_output); 

	if (rv != SECSuccess) {
		printf ("Decoding failed \n");
		goto loser;
	}
    else {
		printf ("Decoded output is %s \n", decoded_data.data);
	}
	rc = strncmp(encoded_item.data,decoded_data.data,decoded_data.len);
	if (rc == 0)
		printf ("sim_pos_ia5string string-Encoding and decoding: Test passed \n");
	else {
		printf ("Wrong decoded output  \n");
		goto loser;
	}
				
    goto end_prog;

loser: printf("sim_pos_ia5string string-Encoding and decoding:Test failed\n");
	   goto end_prog;
	   
end_prog:	PORT_FreeArena(arena, PR_FALSE);


}

#include <stdio.h>
#include "seccomon.h" 
#include "secasn1.h"
#include "secitem.h"
#include "secdert.h"
#include "secder.h"
/*
/* SMIME -Test 001- UTCTime  Simple test to check encoding and decoding API. 
/* Author: Annalee L.Garcia
*/  


int main(int argc, char **argv)
{

    PRArenaPool *arena;
    SECItem encoded_item, encoded_output,decoded_data;
    SECItem *dummy;
	char test_ia5string[25];
	SECStatus rv;
	void *mark;
	int rc=0;
	
    printf ("\n Begin sim_pos_utc test \n");
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    encoded_output.data = NULL;
	decoded_data.data = NULL;

    if ( ! arena ) {
			printf ("Error in creating arena, test failed \n");
			goto loser;
	}
  
	encoded_item.data = "920930232017";
	encoded_item.len = strlen(encoded_item.data);
	printf ("UTCTIME is %s with len %d \n", encoded_item.data,encoded_item.len);
	
    dummy = SEC_ASN1EncodeItem (arena, &encoded_output, &encoded_item,
								SEC_UTCTimeTemplate);
	
	if (dummy == NULL) {
	    printf ("Encoding item failed  \n");
		goto loser;
	}

	rv = SEC_ASN1DecodeItem (arena, &decoded_data, SEC_UTCTimeTemplate,
							   &encoded_output); 

	if (rv != SECSuccess) {
		printf ("Decoding failed \n");
		goto loser;
	}
    else {
		printf ("Decoded output is %s with len %d\n", decoded_data.data, decoded_data.len);
	}
	rc = strncmp(encoded_item.data,decoded_data.data,decoded_data.len);
	if (rc == 0)
		printf ("sim_pos_utc-Encoding and decoding UTCTime: Test passed \n");
	else {
		printf ("Wrong decoded output  \n");
		goto loser;
	}
				
    goto end_prog;
	
loser: printf("sim_pos_utc-Encoding and decoding UTCTime:Test failed\n");
	   goto end_prog;
	   
end_prog:	PORT_FreeArena(arena, PR_FALSE);


}

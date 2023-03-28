#include <stdio.h>
#include "seccomon.h" 
#include "secasn1.h"
#include "secitem.h"
#include "secdert.h"
#include "secder.h"
/*
/* SMIME -Test 001- Integer Simple test to check encoding and decoding API. */
/* Author: Annalee L.Garcia
/*  */


int main(int argc, char **argv)
{

    PRArenaPool *arena;
    SECItem encoded_value, encoded_item, encoded_output,decoded_data;
    SECItem *dummy;
	unsigned long test_long, output_long;
	SECStatus rv;
	void *mark;
	

	printf ("\n Begin Unsigned Integer Test \n");
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    encoded_value.data = NULL;
	decoded_data.data = NULL;

    if ( ! arena ) {
			printf ("Error in creating arena, test failed \n");
			goto loser;
	}
 
    test_long	= 77777777777777;
	printf ("Integer to be encoded is %d \n", test_long);

    dummy= SEC_ASN1EncodeUnsignedInteger(arena, &encoded_value, test_long);

	if (dummy == NULL) {

	    printf ("Encoding value failed  \n");
		goto loser;
	}

	encoded_item.data = encoded_value.data;
	encoded_item.len = encoded_value.len;
	
    dummy = SEC_ASN1EncodeItem (arena, &encoded_output, &encoded_item,
								SEC_IntegerTemplate);
	
	if (dummy == NULL) {

	    printf ("Encoding item failed  \n");
		goto loser;
	}

	rv = SEC_ASN1DecodeItem (arena, &decoded_data, SEC_IntegerTemplate,
							   &encoded_output); 

	if (rv != SECSuccess) {
		printf ("Decoding failed \n");
		goto loser;
	}
    else {
		output_long = DER_GetInteger (&decoded_data);
		printf ("Decoded output is %d \n", output_long);
	}
	if (test_long == output_long)
		printf ("sim_pos_uint-Encode and decode Unsigned Int:Test passed \n");
	else {
		printf ("Wrong decoded output  \n");
		goto loser;
	}
				
    goto end_prog;
	
loser: printf("sim_pos_uint-Encode and decode Unsigned Int:Test failed\n");
	   goto end_prog;
	   
end_prog:	PORT_FreeArena(arena, PR_FALSE);


}

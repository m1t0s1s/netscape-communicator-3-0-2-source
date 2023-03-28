#include <stdio.h>
#include "seccomon.h" 
#include "secasn1.h"
#include "secitem.h"
#include "secdert.h"
#include "secder.h"
/*
/* SMIME -Test 001- Object Identifier Simple test to check encoding and decoding API. */
/* Author: Annalee L.Garcia
/*  */


int main(int argc, char **argv)
{

    PRArenaPool *arena;
    SECItem encoded_value, encoded_item, encoded_output,decoded_data;
    SECItem *dummy;
	int output_long;
	char test_string[75];
	SECStatus rv;
	void *mark;
	
    printf ("\n Begin sim_str_objid test \n");
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    encoded_value.data = NULL;
	decoded_data.data = NULL;

    if ( ! arena ) {
			printf ("Error in creating arena, test failed \n");
			goto loser;
	}

	/* Encode a string Object ID - test should fail - OBj ID shouldn't be of
	 string type. */
	printf ("sim_str_objid - this test should fail \n");
	
	strcpy (test_string,"test to see if objid is okay to take string arguments ");;
	
	dummy= SEC_ASN1EncodeInteger(arena, &encoded_value, test_string);

    printf ("Encoding should fail  \n");
	
	if (dummy == NULL) {

		printf (" Test passed \n");
		goto end_prog;
	}
	else  
		goto loser;

	
loser: printf("sim_str_objid-Encoding and decoding Object Identifier:Test failed\n");
	   goto end_prog;
	   
end_prog:	PORT_FreeArena(arena, PR_FALSE);


}

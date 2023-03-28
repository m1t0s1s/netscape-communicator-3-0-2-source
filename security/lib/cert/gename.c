#include "cert.h"
#include "gename.h"

extern int SEC_ERROR_EXTENSION_VALUE_INVALID;

#define ERROR_BREAK(error) PORT_SetError(error); rv = SECFailure; break;

/* We only want to deal with the URI field for now.  Skip other fields
   during the decoding process for now.
 */
static const SEC_ASN1Template CERT_URITemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | 6 ,
          offsetof(CERTGeneralName, name.uri), SEC_IA5StringTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_DirectoryNameTemplate[] = {
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT | 4,
          offsetof(CERTGeneralName, derDirectoryName), SEC_AnyTemplate,
          sizeof (CERTGeneralName)}
};

static const SEC_ASN1Template CERT_OtherNamesTemplate[] = {
    { SEC_ASN1_ANY | SEC_ASN1_OPTIONAL,
          offsetof(CERTGeneralName, name.other), SEC_AnyTemplate,
          sizeof (CERTGeneralName)}
};

const SEC_ASN1Template CERT_GeneralNameTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
};

void cert_DestroyGenName (CERTGeneralName **genNames)
{
    CERTGeneralName **genName;

    if (!genNames || !*genNames)
	return;
    
    genName = genNames;
    while (genName) {
	if ((*genName)->type == certDirectoryName) {
	    CERT_DestroyName(&((*genName)->name.directoryName));
	}
	else {    
	    PORT_Free ((*genName)->name.other.data);
	    PORT_Free (*genName);
	}
	++genName;
    }
    PORT_Free (*genNames);
}

SECStatus
cert_EncodeGeneralName (PRArenaPool *arena, CERTGeneralName **genNames,
			SECItem ***encodedGenName)
{
    CERTGeneralName **genName;
    SECItem **encodedName;
    SECStatus rv = SECSuccess;
    int entriesCount = 0;
    void *mark;

    PORT_Assert (arena);
    
    /* there is nothing to do */
    if (!genNames || !*genNames)
	return (SECSuccess);
    
    genName = genNames;

    /* Count all entries so we don't have to reallocate buffer.
       At the same time validate data.
     */
    while (*genName) {
	switch ((*genName)->type) {
	    case certURI :
		if ((*genName)->name.uri.data == NULL)  {
		    ERROR_BREAK (SEC_ERROR_EXTENSION_VALUE_INVALID);
		}
		break;

	    case certDNSName:
	    case certEDIPartyName:
	    case certIPAddress:
	    case certOtherName:
	    case certRegisterID:
	    case certRFC822Name:
	    case certX400Address:
		if ((*genName)->name.other.data == NULL ||
		   ((*genName)->name.other.data[0] & 0x1f) != (*genName)->type - 1) {
		    ERROR_BREAK (SEC_ERROR_EXTENSION_VALUE_INVALID);
		}
		else {
		    SEC_ASN1Template template;
		    SECItem decodedData;

		    template.kind = SEC_ASN1_ANY;
		    template.offset = 0;
		    template.sub = NULL;
		    template.size = sizeof (SECItem);
		    decodedData.data = NULL;
		    
		    /* The only thing we can check here is the outter tag is
		       encoded properly.
		     */
		    rv = SEC_ASN1DecodeItem
			 (NULL, &decodedData, &template, &((*genName)->name.other));
		    PORT_Free (decodedData.data);
		}
		break;

	    case certDirectoryName:		
		if (!((*genName)->name.directoryName.rdns) ||
		    !*((*genName)->name.directoryName.rdns)) {
		    ERROR_BREAK (SEC_ERROR_EXTENSION_VALUE_INVALID);			
		}
		break;
		
	    default:
		ERROR_BREAK (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	}
	if (rv != SECSuccess)
	    break;
	++entriesCount;
        ++genName;
    }
    if (*genName || rv != SECSuccess)
	return (SECFailure);
    
    if (entriesCount == 0)
	return (SECSuccess);

    mark = PORT_ArenaMark (arena); 
    /* allocate enough space to hold the encoded entries */
    encodedName = (SECItem **)PORT_ArenaZAlloc
		  (arena, sizeof (SECItem *) * (entriesCount + 1));
    if (encodedName == NULL)  {
	PORT_ArenaRelease (arena, mark);
	return (SECFailure);
    }

    entriesCount = 0;
    genName = genNames;
    *encodedGenName = encodedName;
    while (*genName) {
	switch ((*genName)->type) {
	    case certURI :
		encodedName[entriesCount] = SEC_ASN1EncodeItem
					    (arena, (SECItem *)NULL,
					     *genName, CERT_URITemplate);
		break;
		
	    case certDNSName:
	    case certEDIPartyName:
	    case certIPAddress:
	    case certOtherName:
	    case certRegisterID:
	    case certRFC822Name:
	    case certX400Address:
		/* for these types, we expect the value is already encoded */
		encodedName[entriesCount] = SEC_ASN1EncodeItem
					    (arena, (SECItem *)NULL,
					     *genName, CERT_OtherNamesTemplate);
		break;
	    case certDirectoryName:
		SEC_ASN1EncodeItem (arena, &((*genName)->derDirectoryName),
				    &((*genName)->name.directoryName), CERT_NameTemplate);
		if ((*genName)->derDirectoryName.data == NULL) {
		    ERROR_BREAK (SEC_ERROR_EXTENSION_VALUE_INVALID);
		}
			
			
		encodedName[entriesCount] = SEC_ASN1EncodeItem
					    (arena, (SECItem *)NULL,
					     *genName, CERT_DirectoryNameTemplate);
		break;
	}
	if (!encodedName[entriesCount++]) {
	    rv = SECFailure;
	    break;
	}
	++genName;
    }
    if (*genName) {
	PORT_ArenaRelease (arena, mark);
	return (SECFailure);
    }
    return (SECSuccess);
}    

SECStatus
cert_DecodeGeneralName (PRArenaPool *arena, CERTGeneralName ***genNames,
			SECItem **encodedGenName)
{
    CERTGeneralName **genName = NULL;
    SECItem **encodedName;
    SECStatus rv = SECSuccess;
    void *mark;
    int entriesCount = 0;

    PORT_Assert (arena);
    
    /* there is nothing to decode */
    if (!encodedGenName || !*encodedGenName)
	return (SECSuccess);
    
    *genNames = NULL;
    encodedName = encodedGenName;

    for (entriesCount = 0;*encodedName; ++entriesCount, ++encodedName) {} 

    if (entriesCount == 0)
	return (SECSuccess);

    mark = PORT_ArenaMark (arena);
    genName = (CERTGeneralName **)PORT_ArenaZAlloc
	      (arena, sizeof (CERTGeneralName *) * (entriesCount + 1));
    if (!genName) {
	PORT_ArenaRelease (arena, mark);
	return (SECFailure);
    }
    
    encodedName = encodedGenName;
    *genNames = genName;
    while (*encodedName) {
	*genName = (CERTGeneralName *)PORT_ArenaZAlloc
		   (arena, sizeof (CERTGeneralName));
	if (!*genName)
	    break;

	(*genName)->type = ((*encodedName)->data[0] & 0x1f) + 1;
	switch ((*genName)->type) {
	    case certURI:
		rv = SEC_ASN1DecodeItem
		     (arena,*genName, CERT_URITemplate, *encodedName);
		break;
		
	    case certDNSName:
	    case certEDIPartyName:
	    case certIPAddress:
	    case certOtherName:
	    case certRegisterID:
	    case certRFC822Name:
	    case certX400Address:
	    /* Since we don't support these for now, just save them in the encoded format */
		(*genName)->name.other.data = PORT_ArenaZAlloc
					  (arena, (*encodedName)->len);
		if (!(*genName)->name.other.data)
		    rv = SECFailure;
		else
		    PORT_Memcpy ((*genName)->name.other.data, (*encodedName)->data,
			     (*genName)->name.other.len = (*encodedName)->len);
		break;

	    case certDirectoryName:
		rv = SEC_ASN1DecodeItem
		     (arena, *genName, CERT_DirectoryNameTemplate, *encodedName);
		if (rv != SECSuccess)
		    break;
		rv = SEC_ASN1DecodeItem
		     (arena, &((*genName)->name.directoryName), CERT_NameTemplate,
		      &((*genName)->derDirectoryName));
		break;
	}
	if (rv != SECSuccess)
	    break;
	++genName;
	++encodedName;
    }

    if (*encodedName) {
	PORT_ArenaRelease (arena,mark);
	*genNames = NULL;
	return (SECFailure);
    }
    return (SECSuccess);
}

/* For Directory name type, if derFormat is TRUE, then value must be of type CERTName *;
   otherwise, it must be of type SECItem *
 */
void *
CERT_GetGeneralNameByType (CERTGeneralName **genNames,
			   CERTGeneralNameType type, PRBool derFormat)
{
    CERTGeneralName **genName;
    
    if (!genNames)
	return (NULL);
    genName = genNames;

    while (*genName) {
	if ((*genName)->type == type) {
	    switch (type) {
		case certDNSName:
		case certEDIPartyName:
		case certIPAddress:
		case certOtherName:
		case certRegisterID:
		case certRFC822Name:
		case certX400Address:
		    return (&(*genName)->name.other);
		case certURI:
		    return (&(*genName)->name.uri);
		    break;
		case certDirectoryName:
		    if (derFormat)
			return (&(*genName)->derDirectoryName);
		    else
			return (&(*genName)->name.directoryName);
		    break;
	    }
	}
	++genName;
    }
    return (NULL);
}

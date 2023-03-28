#ifndef _GENAME_H_
#define _GENAME_H_

#include "prarena.h"
#include "seccomon.h"
#include "secoidt.h"
#include "mcom_db.h"
#include "secasn1.h"
#include "secder.h"
#include "certt.h"

/************************************************************************/
SEC_BEGIN_PROTOS

extern const SEC_ASN1Template CERT_GeneralNameTemplate[];

extern SECStatus
cert_EncodeGeneralName (PRArenaPool *arena, CERTGeneralName **genNames,
		   SECItem ***encodedGenName);

extern SECStatus
cert_DecodeGeneralName (PRArenaPool *arena, CERTGeneralName ***genNames,
		   SECItem **encodedGenName);

extern void
cert_DestroyGenName (CERTGeneralName **genNames);

/************************************************************************/
SEC_END_PROTOS

#endif

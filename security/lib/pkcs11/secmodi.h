/*
 * Internal header file included only by files in pkcs11 dir, or in
 * pkcs11 specific client and server files.
 */
#ifndef _SECMODI_H_
#define _SECMODI_H_ 1
#include "pkcs11.h"
#include "prlock.h"
#include "mcom_db.h"
#include "secoidt.h"
#include "secdert.h"
#include "certt.h"
#include "secmodti.h"

#ifdef PKCS11_USE_THREADS
#define PK11_USE_THREADS(x) x
#else
#define PK11_USE_THREADS(x)
#endif

SEC_BEGIN_PROTOS

/* proto-types */
SECMODModule * SECMOD_NewModule(void); /* create a new module */
SECMODModule * SECMOD_NewInternal(void); /* create an internal module */

/* Data base functions */
void SECMOD_InitDB(char *);
SECMODModuleList * SECMOD_ReadPermDB(void);
SECStatus  SECMOD_DeletePermDB(SECMODModule *);
SECStatus  SECMOD_AddPermDB(SECMODModule *);
/*void SECMOD_ReferenceModule(SECMODModule *); */

/* Library functions */
SECStatus SECMOD_LoadModule(SECMODModule *);
SECStatus SECMOD_UnloadModule(SECMODModule *);

void SECMOD_SlotDestroyModule(SECMODModule *module, PRBool fromSlot);
CK_RV pk11_notify(CK_SESSION_HANDLE session, CK_NOTIFICATION event,
                                                         CK_VOID_PTR pdata);

SEC_END_PROTOS

#define PK11_GETTAB(x) ((CK_FUNCTION_LIST_PTR)((x)->functionList))
#define PK11_SETATTRS(x,id,v,l) (x)->type = (id); \
		(x)->pValue=(v); (x)->ulValueLen = (l);
#endif


/*
 * Definition of Security Module Data Structure. There is a separate data
 * structure for each loaded PKCS #11 module.
 */
#ifndef _SECMOD_H_
#define _SEDMOD_H_
#include "secmodt.h"

SEC_BEGIN_PROTOS

/* protoypes */
extern void SECMOD_init(char *dbname);
extern SECMODModuleList *SECMOD_GetDefaultModuleList(void);
extern SECMODListLock *SECMOD_GetDefaultModuleListLock(void);

/* lock management */
extern SECMODListLock *SECMOD_NewListLock(void);
extern void SECMOD_DestroyListLock(SECMODListLock *);
extern void SECMOD_GetReadLock(SECMODListLock *);
extern void SECMOD_ReleaseReadLock(SECMODListLock *);
extern void SECMOD_GetWriteLock(SECMODListLock *);
extern void SECMOD_ReleaseWriteLock(SECMODListLock *);

/* list managment */
extern void SECMOD_RemoveList(SECMODModuleList **,SECMODModuleList *);
extern void SECMOD_AddList(SECMODModuleList *,SECMODModuleList *,SECMODListLock *);

/* Operate on modules by name */
extern SECMODModule *SECMOD_FindModule(char *name);
extern SECStatus SECMOD_DeleteModule(char *name, int *type);
extern SECStatus SECMOD_DeleteInternalModule(char *name);

/* database/memory management */
extern SECMODModule *SECMOD_NewModule(void);
extern SECMODModuleList *SECMOD_NewModuleListElement(void);
extern SECMODModule *SECMOD_GetInternalModule(void);
extern SECMODModule *SECMOD_GetFIPSInternal(void);
extern SECMODModule *SECMOD_ReferenceModule(SECMODModule *module);
extern void SECMOD_DestroyModule(SECMODModule *module);
extern SECMODModuleList *SECMOD_DestroyModuleListElement(SECMODModuleList *);
extern void SECMOD_DestroyModuleList(SECMODModuleList *);
extern SECMODModule *SECMOD_DupModule(SECMODModule *old);
extern SECStatus SECMOD_AddModule(SECMODModule *newModule);
extern PK11SlotInfo *SECMOD_FindSlot(SECMODModule *module,char *name);

SEC_END_PROTOS

#endif

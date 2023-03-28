/*
 * The following handles the loading, unloading and management of
 * various PCKS #11 modules
 */
#include "seccomon.h"
#include "pkcs11.h"
#include "secmod.h"
#include "prlink.h"
#include "pk11func.h"
#include "secmodi.h"

extern void FC_GetFunctionList(void);

/*
 * load a new module into our address space and initialize it.
 */
SECStatus
SECMOD_LoadModule(SECMODModule *mod) {
    PRLibrary *library = NULL;
    CK_C_GetFunctionList entry;
    char * full_name;
    CK_INFO info;
    CK_ULONG slotCount = 0;


    if (mod->loaded) return SECSuccess;

    /* intenal modules get loaded from their internal list */
    if (mod->internal) {
	/* internal, statically get the C_GetFunctionList function */
	if (mod->isFIPS) {
	    entry = (CK_C_GetFunctionList) FC_GetFunctionList;
	} else {
	    entry = C_GetFunctionList;
	}
    } else {
	/* Not internal, load the DLL and look up C_GetFunctionList */
	if (mod->dllName == NULL) {
	    return SECFailure;
	}

#ifdef POSSIBLY_EVIL
	/* look up the library name */
#ifndef NSPR20
	full_name = PR_GetLibName(PR_GetLibraryPath(),mod->dllName);
#else
	full_name = PR_GetLibraryName(PR_GetLibraryPath(),mod->dllName);
#endif /* NSPR20 */
	if (full_name == NULL) {
	    return SECFailure;
	}
#else
	full_name = PORT_Strdup(mod->dllName);
#endif

	/* load the library. If this succeeds, then we have to remember to
	 * unload the library if anything goes wrong from here on out...
	 */
	library = PR_LoadLibrary(full_name);
	mod->library = (void *)library;
	PORT_Free(full_name);
	if (library == NULL) {
	    return SECFailure;
	}

	/*
	 * now we need to get the entry point to find the function pointers
	 */
	entry = (CK_C_GetFunctionList)
#ifndef NSPR20
			PR_FindSymbol("C_GetFunctionList",library);
#else
			PR_FindSymbol(library, "C_GetFunctionList");
#endif /* NSPR20 */
	if (entry == NULL) {
	    PR_UnloadLibrary(library);
	    return SECFailure;
	}
    }

    /*
     * We need to get the function list
     */
    if ((*entry)((CK_FUNCTION_LIST_PTR *)&mod->functionList) != CKR_OK) 
								goto fail;

    /* Now we initialize the module */
    if (PK11_GETTAB(mod)->C_Initialize(NULL) != CKR_OK) goto fail;

    /* check the version number */
    if (PK11_GETTAB(mod)->C_GetInfo(&info) != CKR_OK) goto fail2;
    if (info.cryptokiVersion.major != 2) goto fail2;


    /* If we don't have a common name, get it from the PKCS 11 module */
    if ((mod->commonName == NULL) || (mod->commonName[0] == 0)) {
	mod->commonName = PK11_MakeString(mod->arena,NULL,
	   (char *)info.libraryDescription, sizeof(info.libraryDescription));
	if (mod->commonName == NULL) goto fail2;
    }
    

    /* initialize the Slots */
    if (PK11_GETTAB(mod)->C_GetSlotList(FALSE, NULL, &slotCount) == CKR_OK) {
	CK_SLOT_ID *slotIDs;
	int i;
	CK_RV rv;

	mod->slots = (PK11SlotInfo **)PORT_ArenaAlloc(mod->arena,
					sizeof(PK11SlotInfo *) * slotCount);
	if (mod->slots == NULL) goto fail2;

	slotIDs = (CK_SLOT_ID *) PORT_Alloc(sizeof(CK_SLOT_ID)*slotCount);
	if (slotIDs == NULL) {
	    goto fail2;
	}  
	rv = PK11_GETTAB(mod)->C_GetSlotList(FALSE, slotIDs, &slotCount);
	if (rv != CKR_OK) {
	    PORT_Free(slotIDs);
	    goto fail2;
	}

	/* Initialize each slot */
	for (i=0; i < slotCount; i++) {
	    mod->slots[i] = PK11_NewSlotInfo();
	    PK11_InitSlot(mod,slotIDs[i],mod->slots[i]);
	    /* look down the slot info table */
	    PK11_LoadSlotList(mod->slots[i],mod->slotInfo,mod->slotInfoCount);
	}
	mod->slotCount = slotCount;
	PORT_Free(slotIDs);
    }
	


    
    mod->loaded = PR_TRUE;
    return SECSuccess;
fail2:
    PK11_GETTAB(mod)->C_Finalize(NULL);
fail:
    mod->functionList = NULL;
    if (library) PR_UnloadLibrary(library);
    return SECFailure;
}

SECStatus
SECMOD_UnloadModule(SECMODModule *mod) {
    PRLibrary *library;

    if (!mod->loaded) {
	return SECFailure;
    }

    PK11_GETTAB(mod)->C_Finalize(NULL);
    mod->loaded = PR_FALSE;
    
    /* do we want the semantics to allow unloading the internal library?
     * if not, we should change this to SECFailure and move it above the
     * mod->loaded = PR_FALSE; */
    if (mod->internal) {
	return SECSuccess;
    }

    library = (PRLibrary *)mod->library;
    /* paranoia */
    if (library == NULL) {
	return SECFailure;
    }

    PR_UnloadLibrary(library);
    return SECSuccess;
}

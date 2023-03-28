/*
 * Initialize the PCKS 11 subsystem
 */
#include "seccomon.h"
#include "secmod.h"
#include "prlock.h"
#include "secmodi.h"
#include "pk11func.h"


static  SECMODModuleList *modules = NULL;
static  SECMODModule *internalModule = NULL;
static SECMODListLock *moduleLock = NULL;

void SECMOD_init(char *dbname) {
    SECMODModuleList *thisModule;
    int found=0;


    /* don't initialize twice */
    if (modules) return;

    SECMOD_InitDB(dbname);

    /*
     * read in the current modules from the database
     */
    modules = SECMOD_ReadPermDB();

    /* make sure that the internal module is loaded */
    for (thisModule = modules; thisModule ; thisModule = thisModule->next) {
	if (thisModule->module->internal) {
	    found++;
	    internalModule = SECMOD_ReferenceModule(thisModule->module);
	    break;
	}
    }

    if (!found) {
	thisModule = modules;
	modules = SECMOD_NewModuleListElement();
	modules->module = SECMOD_NewInternal();
	PORT_Assert(modules->module != NULL);
	modules->next = thisModule;
	SECMOD_AddPermDB(modules->module);
	internalModule = SECMOD_ReferenceModule(modules->module);
    }

    /* load it first... we need it to verify the external modules
     * which we are loading.... */
    SECMOD_LoadModule(internalModule);

    /* Load each new module */
    for (thisModule = modules; thisModule ; thisModule = thisModule->next) {
	SECMOD_LoadModule(thisModule->module);
    }

    moduleLock = SECMOD_NewListLock();
}

/*
 * retrieve the internal module
 */
SECMODModule *
SECMOD_GetInternalModule(void) {
   return internalModule;
}

/*
 * get the list of PKCS11 modules that are available.
 */
SECMODModuleList *SECMOD_GetDefaultModuleList() { return modules; }
SECMODListLock *SECMOD_GetDefaultModuleListLock() { return moduleLock; }



/*
 * find a module by name, and add a reference to it.
 * return that module.
 */
SECMODModule *SECMOD_FindModule(char *name) {
    SECMODModuleList *mlp;
    SECMODModule *module = NULL;

    SECMOD_GetReadLock(moduleLock);
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    module = mlp->module;
	    SECMOD_ReferenceModule(module);
	    break;
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);

    return module;
}


/*
 * find a module by name and delete it of the module list
 */
SECStatus
SECMOD_DeleteModule(char *name, int *type) {
    SECMODModuleList *mlp;
    SECMODModuleList **mlpp;
    SECStatus rv = SECFailure;


    *type = SECMOD_EXTERNAL;

    SECMOD_GetWriteLock(moduleLock);
    for(mlpp = &modules,mlp = modules; 
				mlp != NULL; mlpp = &mlp->next, mlp = *mlpp) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    /* don't delete the internal module */
	    if (!mlp->module->internal) {
		SECMOD_RemoveList(mlpp,mlp);
		/* delete it after we release the lock */
		rv = SECSuccess;
	    } else if (mlp->module->isFIPS) {
		*type = SECMOD_FIPS;
	    } else {
		*type = SECMOD_INTERNAL;
	    }
	    break;
	}
    }
    SECMOD_ReleaseWriteLock(moduleLock);


    if (rv == SECSuccess) {
 	SECMOD_DeletePermDB(mlp->module);
	SECMOD_DestroyModuleListElement(mlp);
    }
    return rv;
}

/*
 * find a module by name and delete it of the module list
 */
SECStatus
SECMOD_DeleteInternalModule(char *name) {
    SECMODModuleList *mlp;
    SECMODModuleList **mlpp;
    SECStatus rv = SECFailure;

    SECMOD_GetWriteLock(moduleLock);
    for(mlpp = &modules,mlp = modules; 
				mlp != NULL; mlpp = &mlp->next, mlp = *mlpp) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    /* don't delete the internal module */
	    if (mlp->module->internal) {
		rv = SECSuccess;
		SECMOD_RemoveList(mlpp,mlp);
	    } 
	    break;
	}
    }
    SECMOD_ReleaseWriteLock(moduleLock);

    if (rv == SECSuccess) {
	SECMODModule *newModule,*oldModule;

	if (mlp->module->isFIPS) {
	    newModule = SECMOD_NewInternal();
	} else {
	    newModule = SECMOD_GetFIPSInternal();
	}
	if (newModule == NULL) {
	    SECMODModuleList *last,*mlp2;
	   /* we're in pretty deep trouble if this happens...Security
	    * is hosed for sure... try to put the old module back on
	    * the list */
	   SECMOD_GetWriteLock(moduleLock);
	   for(mlp2 = modules; mlp2 != NULL; mlp2 = mlp->next) {
		last = mlp2;
	   }

	   if (last == NULL) {
		modules = mlp;
	   } else {
		SECMOD_AddList(last,mlp,NULL);
	   }
	   SECMOD_ReleaseWriteLock(moduleLock);
	   return SECFailure; /* sigh */
	}
	oldModule = internalModule;
	internalModule = SECMOD_ReferenceModule(newModule);
	SECMOD_AddModule(internalModule);
	SECMOD_DestroyModule(oldModule);
 	SECMOD_DeletePermDB(mlp->module);
	SECMOD_DestroyModuleListElement(mlp);
    }
    return rv;
}

SECStatus
SECMOD_AddModule(SECMODModule *newModule) {
    SECStatus rv;
    SECMODModuleList *mlp, *newListElement, *last = NULL;

    rv = SECMOD_LoadModule(newModule);
    if (rv != SECSuccess) {
	return rv;
    }

    newListElement = SECMOD_NewModuleListElement();
    if (newListElement == NULL) {
	return SECFailure;
    }

    SECMOD_AddPermDB(newModule);

    newListElement->module = newModule;

    SECMOD_GetWriteLock(moduleLock);
    /* Added it to the end (This is very inefficient, but Adding a module
     * on the fly should happen maybe 2-3 times through the life this program
     * on a given computer, and this list should be *SHORT*. */
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	last = mlp;
    }

    if (last == NULL) {
	modules = newListElement;
    } else {
	SECMOD_AddList(last,newListElement,NULL);
    }
    SECMOD_ReleaseWriteLock(moduleLock);
    return SECSuccess;
}

PK11SlotInfo *SECMOD_FindSlot(SECMODModule *module,char *name) {
    int i;
    char *string;

    for (i=0; i < module->slotCount; i++) {
	PK11SlotInfo *slot = module->slots[i];

	if (PK11_IsPresent(slot)) {
	    string = PK11_GetTokenName(slot);
	} else {
	    string = PK11_GetSlotName(slot);
	}
	if (PORT_Strcmp(name,string) == 0) {
	    return PK11_ReferenceSlot(slot);
	}
    }
    return NULL;
}

SECStatus
PK11_GetModInfo(SECMODModule *mod,CK_INFO *info) {
    CK_RV crv;

    crv = PK11_GETTAB(mod)->C_GetInfo(info);
    return (crv == CKR_OK) ? SECSuccess : SECFailure;
}

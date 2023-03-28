/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */

#include "sec.h"
#include "sslimpl.h"
#include "fortezza.h"
#include "preenc.h"

PEContext *PE_CreateContext(PEHeader *header, void *window_id)
{
    PEContext *pe_cx;
    FortezzaKEAContext *kea_cx;
    FortezzaContext *fort_cx;
    SECStatus rv;
    PEFortezzaHeader pe_fortezza;
    
    /* check PEHeader to make sure its fortezza -> checks int type field */
    if ((GetInt2(header->type) != PRE_FORTEZZA_FILE) &&
        (GetInt2(header->type) != PRE_FORTEZZA_STREAM)) {
        /* XXX Set an error */
        return NULL;
    }
    
    PORT_Assert(header != NULL);
    
    pe_fortezza = header->u.fortezza;
    
    kea_cx = SECMOZ_FindCardBySerial(window_id, pe_fortezza.serial);
    if (kea_cx == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    fort_cx = FortezzaCreateContext(kea_cx, Decrypt);
    if (fort_cx == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    rv = FortezzaUnwrapKey(fort_cx, NULL, pe_fortezza.key);
    if (rv == SECFailure) {
        /* XXX set an error */
        return NULL;
    }
    
    rv = FortezzaLoadIV(fort_cx, pe_fortezza.iv);
    if (rv == SECFailure) {
        /* XXX set an error */
        return NULL;
    }
    
    pe_cx = PORT_ZAlloc(sizeof(PEContext));
    if (pe_cx == NULL) {
        /* XXX set an error */
        return NULL;
    }
    
    pe_cx->type = pec_fortezza;
    pe_cx->base_context = fort_cx;
    pe_cx->decrypt = (SSLCipher) Fortezza_Op;
    pe_cx->destroy = (SSLDestroy) FortezzaDestroyContext;
    
    return(pe_cx);
}

void PE_DestroyContext(PEContext *pe_cx, PRBool freeit)
{
    if (pe_cx->base_context) {
        FortezzaDestroyContext(pe_cx->base_context, freeit);
    }
    if (freeit) PORT_Free(pe_cx);
    
    return;
}

SECStatus PE_Decrypt(PEContext *pe_cx, unsigned char *out, unsigned int *part,
                    unsigned int maxOut, unsigned char *in, unsigned int inLen)
{
    SECStatus rv;
    
    if (pe_cx->decrypt && pe_cx->base_context) {
        rv = pe_cx->decrypt(pe_cx->base_context, out, part, maxOut, in, inLen);
        return(rv);
    }
    
    return(SECFailure);
}


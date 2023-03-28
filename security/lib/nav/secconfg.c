/*
 * Security config through JavaScript 
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secconfg.c,v 1.7.2.2 1997/04/13 22:09:26 jwz Exp $
 */

#ifndef AKBAR

#include "xp.h"
#include "jsapi.h"
#include "jscompat.h"
#include "jsdate.h"
#include "prefapi.h"
#include "cert.h"
#include "prtime.h"
#include "secder.h"
#include "secitem.h"

#ifdef XP_MAC
#include "certdb.h"
#else
#include "../cert/certdb.h"
#endif


PR_STATIC_CALLBACK(PRBool)
CertGetProp(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

PR_STATIC_CALLBACK(void)
CertFinalize(JSContext *cx, JSObject *obj);


/*
 * global stuff
 */
static JSPropertySpec noProps[1] = { { NULL } };
static JSObject *securityConfig = NULL;
static JSObject *globalConfig = NULL;
static JSContext *configContext = NULL;
static JSObject *certDBObject = NULL;
static JSObject *certIndexProto = NULL;
static JSObject *nicknameIndex = NULL;
static JSObject *dnIndex = NULL;
static JSObject *emailIndex = NULL;
static PRBool certIndexAllowCtr = PR_TRUE;
static JSObject *certProto = NULL;

typedef enum {
    certPropNickname,
    certPropEmailAddr,
    certPropDN,
    certPropTrust,
    certPropIsPerm,
    certPropNotBefore,
    certPropNotAfter,
    certPropSerialNumber,
    certPropIssuerName,
    certPropIssuer,
    certPropSubjectNext,
    certPropSubjectPrev
} certPropType;

typedef enum {
    indexTypeNickname,
    indexTypeEmail,
    indexTypeDN
} certIndexType;


/*
 * Certificate class
 */
static JSClass certClass = {
    "Certificate",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    CertGetProp,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    CertFinalize
};

JSObject *
UTCTimeToDateObject(JSContext *cx, SECItem *utctime)
{
    JSObject *retObj;
    SECStatus rv;
    PRTime prTime;
    int64 time;
    
    /* convert DER time */
    rv = DER_UTCTimeToTime(&time, utctime);
    if ( rv != SECSuccess ) {
	return(NULL);
    }
    PR_ExplodeGMTTime(&prTime, time);
    retObj = js_NewDateObject(cx,
			      prTime.tm_year,
			      prTime.tm_mon,
			      prTime.tm_mday,
			      prTime.tm_hour,
			      prTime.tm_min,
			      prTime.tm_sec);

    return(retObj);
}

PR_STATIC_CALLBACK(void)
CertFinalize(JSContext *cx, JSObject *obj)
{
    SECItem *certKey;
    
    /* get the cert database lookup key from the object */
    certKey = (SECItem *)JS_GetPrivate(cx, obj);
    if ( certKey != NULL ) {
	SECITEM_FreeItem(certKey, PR_TRUE);
    }

    return;
}

JSObject *
NewCertObject(CERTCertificate *cert)
{
    JSObject *certObj;
    PRBool ret;
    
    /*
     * since we always implement our own getProperty, we don't need to
     * define this object as a property of another object
     */
    certObj = JS_NewObject(configContext, &certClass, certProto, certDBObject);
    if ( certObj != NULL ) {
	/* private data on the cert object is the database lookup key */
	ret = JS_SetPrivate(configContext, certObj,
			    (void *)SECITEM_DupItem(&cert->certKey));
    }

    return(certObj);
}

JSObject *
DefineCertObject(CERTCertificate *cert, char *name, JSObject *parent)
{
    JSObject *certObj;
    PRBool ret;
    
    certObj = JS_DefineObject(configContext, parent, name, &certClass,
			      certProto, JSPROP_ENUMERATE);
    if ( certObj != NULL ) {
	/* private data on the cert object is the database lookup key */
	ret = JS_SetPrivate(configContext, certObj,
			    (void *)SECITEM_DupItem(&cert->certKey));
    }

    return(certObj);
}

PR_STATIC_CALLBACK(PRBool)
CertCtr(JSContext *cx, JSObject *obj, PRUintn argc, jsval *argv, jsval *rval)
{
    PRBool ret;
    CERTCertificate *cert = NULL;
    CERTCertificate *impcert = NULL;
    JSString *certStr;
    JSObject *certObj;
    
    if ( argc == 0 ) {
	ret = JS_SetPrivate(cx, obj, NULL);
	return(PR_TRUE);
    }
    
    if ( argc != 1 ) {
	goto loser;
    }
    
    if ( !JSVAL_IS_STRING(argv[0]) ) {
	goto loser;
    }
    
    /* this string is a base64 encoded certificate */
    certStr = JSVAL_TO_STRING(argv[0]);
    
    /* decode the package that the cert came in */
    impcert = CERT_DecodeCertFromPackage(JS_GetStringBytes(certStr),
					 JS_GetStringLength(certStr));
    
    if ( impcert == NULL ) {
	goto loser;
    }

    /* load the cert into the temporary database */
    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
				   &impcert->derCert,
				   NULL, PR_FALSE, PR_TRUE);

    CERT_DestroyCertificate(impcert);

    if ( cert == NULL ) {
	goto loser;
    }

    /* create a cert object from the cert */
    certObj = NewCertObject(cert);
    
    if ( certObj == NULL ) {
	goto loser;
    }
    
    /* return the cert object */
    *rval = OBJECT_TO_JSVAL(certObj);
    
    ret = PR_TRUE;
    goto done;
loser:
    ret = PR_FALSE;
done:

    /* NOTE - we are not destroying 'cert'.  We are leaving a reference
     * to it around so that it does not disappear on us before we need it.
     * This is the only way to keep a temp cert around.  It will leak
     * the cert, but that should be small enough that it is ok.
     */
    return(ret);
}

PR_STATIC_CALLBACK(PRBool)
CertGetProp(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    certPropType type;
    CERTCertificate *cert = NULL;
    SECItem *certKey = NULL;
    char *retStr;
    PRBool retBool;
    JSObject *retObj;
    int retType;
    PRBool freeRetStr;
    CERTCertificate *tmpCert = NULL;
    PRBool ret;
    JSString *jsstr;
    
    freeRetStr = PR_FALSE;

    /* only support tinyid props */
    if ( !JSVAL_IS_INT(id) ) {
	goto loser;
    }
    
    /* get the cert database lookup key from the object */
    certKey = (SECItem *)JS_GetPrivate(cx, obj);
    if ( certKey == NULL ) {
	goto loser;
    }
    
    /* find the cert */
    cert = CERT_FindCertByKey(CERT_GetDefaultCertDB(), certKey);
    if ( cert == NULL ) {
	goto loser;
    }
    
    /*
     * get the property value from the cert
     */
    retType = JSVAL_STRING;
    type = JSVAL_TO_INT(id);
    switch(type) {
      case certPropNickname:
	retStr = cert->nickname;
	break;
      case certPropEmailAddr:
	retStr = cert->emailAddr;
	break;
      case certPropDN:
	retStr = cert->subjectName;
	break;
      case certPropTrust:
	retStr = CERT_EncodeTrustString(cert->trust);
	freeRetStr = PR_TRUE;
	break;
      case certPropIsPerm:
	retType = JSVAL_BOOLEAN;
	retBool = cert->isperm;
	break;
      case certPropNotBefore:
	retType = JSVAL_OBJECT;
	retObj = UTCTimeToDateObject(cx, &cert->validity.notBefore);
	break;
      case certPropNotAfter:
	retType = JSVAL_OBJECT;
	retObj = UTCTimeToDateObject(cx, &cert->validity.notAfter);
	break;
#ifdef XP_WIN16
}
	switch(type) {
#endif
      case certPropSerialNumber:
	retStr = CERT_Hexify(&cert->serialNumber,1);
	freeRetStr = PR_TRUE;
	break;
      case certPropIssuerName:
	retStr = cert->issuerName;
	break;
      case certPropIssuer:
	tmpCert = CERT_FindCertIssuer(cert);
	if ( tmpCert == NULL ) {
	    goto loser;
	}
	retObj = NewCertObject(tmpCert);
	CERT_DestroyCertificate(tmpCert);
	retType = JSVAL_OBJECT;
	break;
      case certPropSubjectNext:
	tmpCert = CERT_NextSubjectCert(cert);
	if ( tmpCert == NULL ) {
	    goto loser;
	}
	retObj = NewCertObject(tmpCert);
	CERT_DestroyCertificate(tmpCert);
	retType = JSVAL_OBJECT;
	break;
      case certPropSubjectPrev:
	tmpCert = CERT_PrevSubjectCert(cert);
	if ( tmpCert == NULL ) {
	    goto loser;
	}
	retObj = NewCertObject(tmpCert);
	CERT_DestroyCertificate(tmpCert);
	retType = JSVAL_OBJECT;
	break;
    }

    switch (retType) {
      case JSVAL_OBJECT:
	*vp = OBJECT_TO_JSVAL(retObj);
	break;
      case JSVAL_STRING:
	jsstr = JS_NewStringCopyZ(cx, retStr);
	if ( jsstr == NULL ) {
	    goto loser;
	}
	*vp = STRING_TO_JSVAL(jsstr);
	break;
      case JSVAL_BOOLEAN:
	*vp = BOOLEAN_TO_JSVAL(retBool);
	break;
    }
    
    ret = PR_TRUE;
    goto done;
loser:
    ret = PR_FALSE;
done:
    if ( freeRetStr ) {
	PORT_Free(retStr);
    }

    if ( cert != NULL ) {
	CERT_DestroyCertificate(cert);
    }
    
    return(ret);
}

static JSPropertySpec certProps[] = {
    { "nickname", certPropNickname,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "emailAddr", certPropEmailAddr,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "dn", certPropDN,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "trust", certPropTrust,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "isPerm", certPropIsPerm,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "notBefore", certPropNotBefore,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "notAfter", certPropNotAfter,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "serialNumber", certPropSerialNumber,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "issuerName", certPropIssuerName,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "issuer", certPropIssuer,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "subjectNext", certPropSubjectNext,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    { "subjectPrev", certPropSubjectPrev,
	  JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ,NULL, NULL },
    {0}
};


static SECStatus
InitCertClass(void)
{
    /*
     * create the cert index class
     */

    certProto = JS_InitClass(configContext, securityConfig, NULL,
			     &certClass, CertCtr, 1,
			     certProps, NULL, NULL, NULL);
    if ( certProto == NULL ) {
	goto loser;
    }

    return(SECSuccess);
loser:
    return(SECFailure);
}

PR_STATIC_CALLBACK(PRBool)
CertIndexCtr(JSContext *cx, JSObject *obj, PRUintn argc, jsval *argv,
	     jsval *rval)
{
    return(certIndexAllowCtr);
}

PR_STATIC_CALLBACK(PRBool)
CertIndexGetProp(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    CERTCertDBHandle *handle = NULL;
    certIndexType type;
    char *name;
    CERTCertificate *cert = NULL;
    PRBool ret;
    JSString *jsstr;
    
    if ( !JSVAL_IS_STRING(id) ) {
	goto loser;
    }

    jsstr = JSVAL_TO_STRING(id);
    
    name = JS_GetStringBytes(jsstr);
    
    /* get the db handle from the javascript object */
    handle = CERT_GetDefaultCertDB();
    if ( handle == NULL ) {
	goto loser;
    }
    
    /* get the index type of this object */
    type = (certIndexType)JSVAL_TO_INT(JS_GetPrivate(cx, obj));

    switch ( type ) {
      case indexTypeNickname:
	cert = CERT_FindCertByNickname(handle, name);
	break;
      case indexTypeEmail:
	cert = CERT_FindCertByEmailAddr(handle, name);
	break;
      case indexTypeDN:
	cert = CERT_FindCertByNameString(handle, name);
	break;
    }

    if ( cert == NULL ) {
	goto loser;
    }

    *vp = OBJECT_TO_JSVAL(NewCertObject(cert));
    
    ret = PR_TRUE;
    goto done;

loser:
    ret = PR_FALSE;
done:
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    
    return(ret);
}

typedef struct {
    JSContext *cx;
    JSObject *obj;
    certIndexType type;
} enumCertState;

PR_STATIC_CALLBACK(SECStatus)
EnumCertCallback(CERTCertificate *cert, SECItem *k, void *data)
{
    enumCertState *state;
    char *name;
    
    state = (enumCertState *)data;
    
    switch ( state->type ) {
      case indexTypeNickname:
	name = cert->nickname;
	break;
      case indexTypeEmail:
	name = cert->emailAddr;
	break;
      case indexTypeDN:
	name = cert->subjectName;
	break;
    }

    if ( name && ( *name != '\0' ) ) {
	(void)DefineCertObject(cert, name, state->obj);
    }
    
    return(SECSuccess);
}

PR_STATIC_CALLBACK(PRBool)
CertIndexEnum(JSContext *cx, JSObject *obj)
{
    enumCertState state;
    SECStatus rv;
    CERTCertDBHandle *handle;
    
    /* get the index type of this object */
    state.type = (certIndexType)JSVAL_TO_INT(JS_GetPrivate(cx, obj));

    state.cx = cx;
    state.obj = obj;

    handle = CERT_GetDefaultCertDB();
    if ( handle == NULL ) {
	goto loser;
    }
    
    rv = SEC_TraversePermCerts(handle, EnumCertCallback, (void *)&state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    return(PR_TRUE);
loser:
    return(PR_FALSE);
}

/* Called from JavaScript */
PR_STATIC_CALLBACK(JSBool)
SecConfPrintString(JSContext *cx, JSObject *obj, unsigned int argc,
		   jsval *argv, jsval *rval)
{
    if (argc >= 1 && JSVAL_IS_STRING(argv[0])) {
	const char *msg = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	JSString * str = JS_NewStringCopyZ(cx, msg);
		
	if (str == NULL) {
	    return(JS_FALSE);
	}

	printf(msg);
	*rval= STRING_TO_JSVAL(str);
    }
    return(JS_TRUE);
}

/*
 * add a cert to the permanent database
 * args are:
 *	cert object
 *	trust
 *	nickname [optional]
 */
PR_STATIC_CALLBACK(JSBool)
CertDBAddCert(JSContext *cx, JSObject *obj, unsigned int argc,
	      jsval *argv, jsval *rval)
{
    JSObject *certObj;
    CERTCertificate *cert = NULL;
    JSString *nicknameStr;
    JSString *trustStr;
    SECItem *certKey;
    CERTCertTrust trust;
    PRBool ret;
    SECStatus rv;
    char *nickname;
    
    if ( ( argc == 3 ) || ( argc == 2 ) ) {
	if ( !JSVAL_IS_OBJECT(argv[0]) ) {
	    goto loser;
	}
	if ( !JSVAL_IS_STRING(argv[1]) ) {
	    goto loser;
	}
	if ( argc == 3 ) {
	    if ( !JSVAL_IS_STRING(argv[2]) ) {
		goto loser;
	    }
	    nicknameStr = JSVAL_TO_STRING(argv[2]);
	}
	certObj = JSVAL_TO_OBJECT(argv[0]);
	trustStr = JSVAL_TO_STRING(argv[1]);
	nickname = JS_GetStringBytes(nicknameStr);
	
	/* get cert database lookup key from object */
	certKey = (SECItem *)JS_GetPrivate(cx, certObj);
	if ( certKey == NULL ) {
	    goto loser;
	}
	
	/* lookup cert in database */
	cert = CERT_FindCertByKey(CERT_GetDefaultCertDB(), certKey);
	if ( cert == NULL ) {
	    goto loser;
	}
	
	/* decode the trust flags string */
	rv = CERT_DecodeTrustString(&trust, JS_GetStringBytes(trustStr));
	if ( rv != SECSuccess ) {
	    goto loser;
	}

	/*
	 * if no nickname was passed in, then there must already be a
	 * nickname for the cert's subject name
	 */
	if ( ( nickname == NULL ) || ( *nickname == '\0' ) ) {
	    if ( ( cert->subjectList == NULL ) ||
		( cert->subjectList->entry == NULL ) ||
		( cert->subjectList->entry->nickname == NULL ) ) {
		goto loser;
	    }

	    /* force zero length string case to null */
	    nickname = NULL;
	}

	/* add the cert to the perm database */
	if ( cert->isperm ) {
	    goto loser;
	} else {
	    rv = CERT_AddTempCertToPerm(cert, nickname, &trust);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	}
	
    }
    
    ret = PR_TRUE;
    goto done;
    
loser:
    ret = PR_FALSE;
done:
    if ( cert != NULL ) {
	CERT_DestroyCertificate(cert);
    }
    
    return(ret);
}

static SECStatus
DeleteCertCB(CERTCertificate *cert, void *arg)
{
    SECStatus rv;
    
    rv = SEC_DeletePermCertificate(cert);

    return(SECSuccess);
}

PR_STATIC_CALLBACK(JSBool)
CertDBDelCert(JSContext *cx, JSObject *obj, unsigned int argc,
	      jsval *argv, jsval *rval)
{
    CERTCertificate *cert = NULL;
    SECItem *certKey;
    PRBool ret;
    PRBool deleteAll = PR_FALSE;
    SECStatus rv;
    JSObject *certObj;
    
    if ( ( argc < 1 ) || ( argc > 2 ) ) {
	goto loser;
    }

    if ( !JSVAL_IS_OBJECT(argv[0]) ) {
	goto loser;
    }
    
    if ( argc == 2 ) {
	if ( !JSVAL_IS_BOOLEAN(argv[1]) ) {
	    goto loser;
	}
	
	deleteAll = JSVAL_TO_BOOLEAN(argv[1]);
    }
    
    certObj = JSVAL_TO_OBJECT(argv[0]);

    /* get cert database lookup key from object */
    certKey = (SECItem *)JS_GetPrivate(cx, certObj);
    if ( certKey == NULL ) {
	goto loser;
    }
	
    /* lookup cert in database */
    cert = CERT_FindCertByKey(CERT_GetDefaultCertDB(), certKey);
    if ( cert == NULL ) {
	goto loser;
    }

    if ( deleteAll ) {
	rv = CERT_TraversePermCertsForSubject(CERT_GetDefaultCertDB(),
					      &cert->derSubject, DeleteCertCB,
					      NULL);
    } else {
	rv = SEC_DeletePermCertificate(cert);
    }
    
    ret = PR_TRUE;
    goto done;
    
loser:
    ret = PR_FALSE;
done:
    if ( cert != NULL ) {
	CERT_DestroyCertificate(cert);
    }
    
    return(ret);
}

/*
 * Cert DB object stuff
 */
static JSClass certDBClass = {
    "SecurityConfig",
    0,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSPropertySpec certDBProps[] = {
    {0}
};

static JSFunctionSpec certDBMethods[] = {
    { "addCert", CertDBAddCert, 2},
    { "deleteCert", CertDBDelCert, 2},
    { NULL, NULL, 0}
};

static SECStatus
InitCertDB(void)
{

    /*
     * create the cert database object
     */
    certDBObject = JS_DefineObject(configContext, securityConfig, 
				   "certDB", &certDBClass, 
				   NULL, JSPROP_ENUMERATE);

    if ( JS_DefineProperties(configContext, certDBObject, certDBProps) ==
	PR_FALSE ) {
	goto loser;
    }

    if ( JS_DefineFunctions(configContext, certDBObject, certDBMethods) ==
	PR_FALSE ) {
	goto loser;
    }

    return(SECSuccess);
loser:
    return(SECFailure);
}

/*
 * cert index stuff
 */
static JSClass certIndexClass = {
    "CertificateIndex",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_PropertyStub,
    CertIndexGetProp,
    JS_PropertyStub,
    CertIndexEnum,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static SECStatus
InitCertIndex(void)
{
    PRBool ret;
    
    /*
     * create the cert index class
     */

    certIndexProto = JS_InitClass(configContext, securityConfig, NULL,
				  &certIndexClass, CertIndexCtr, 0,
				  NULL, NULL, NULL, NULL);
    if ( certIndexProto == NULL ) {
	goto loser;
    }

    /* create the nickname index object */
    nicknameIndex = JS_DefineObject(configContext, certDBObject,
				    "nickname", &certIndexClass, 
				    certIndexProto,
				    JSPROP_ENUMERATE | JSPROP_PERMANENT |
				    JSPROP_READONLY);
    if ( nicknameIndex == NULL ) {
	goto loser;
    }
    
    ret = JS_SetPrivate(configContext, nicknameIndex,
		       (void *)INT_TO_JSVAL(indexTypeNickname));
    if ( ret == PR_FALSE ) {
	goto loser;
    }

    /* create the email index object */
    emailIndex = JS_DefineObject(configContext, certDBObject,
				 "email", &certIndexClass, 
				 certIndexProto,
				 JSPROP_ENUMERATE | JSPROP_PERMANENT |
				 JSPROP_READONLY);

    if ( emailIndex == NULL ) {
	goto loser;
    }
    
    ret = JS_SetPrivate(configContext, emailIndex,
		       (void *)INT_TO_JSVAL(indexTypeEmail));
    if ( ret == PR_FALSE ) {
	goto loser;
    }
    
    /* create the DN index object */
    dnIndex = JS_DefineObject(configContext, certDBObject,
			      "dn", &certIndexClass, 
			      certIndexProto,
			      JSPROP_ENUMERATE | JSPROP_PERMANENT |
			      JSPROP_READONLY);

    if ( dnIndex == NULL ) {
	goto loser;
    }
    
    ret = JS_SetPrivate(configContext, dnIndex,
		       (void *)INT_TO_JSVAL(indexTypeDN));
    if ( ret == PR_FALSE ) {
	goto loser;
    }

    /* disable the constructor for certIndex class */
    certIndexAllowCtr = PR_FALSE;

    return(SECSuccess);

loser:
    return(SECFailure);
}

/*
 * security config object stuff
 */
static JSPropertySpec secConfProps[] = {
    {0}
};

static JSClass secConfClass = {
    "SecurityConfig",
    0,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

static JSFunctionSpec secConfMethods[] = {
    { "printString", SecConfPrintString, 1},
    { NULL, NULL, 0}
};

PRBool
SECNAV_InitConfigObject()
{
    SECStatus rv;
    
    PREF_GetConfigContext(&configContext);
    PREF_GetGlobalConfigObject(&globalConfig);


    securityConfig = JS_DefineObject(configContext, globalConfig, 
				     "SecurityConfig", &secConfClass, 
				     NULL, JSPROP_ENUMERATE);
    if ( securityConfig == NULL ) {
	goto loser;
    }
    
    if ( JS_DefineProperties(configContext, securityConfig, secConfProps) ==
	PR_FALSE ) {
	goto loser;
    }

    if ( JS_DefineFunctions(configContext, securityConfig, secConfMethods) ==
	PR_FALSE ) {
	goto loser;
    }

    /*
     * Create the certDB object
     */
    rv = InitCertDB();

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /*
     * Create the certIndex objects
     */
    rv = InitCertIndex();

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /*
     * Create the cert Class
     */
    rv = InitCertClass();

    if ( rv != SECSuccess ) {
	goto loser;
    }

    return(PR_TRUE);

loser:
    return(PR_FALSE);
}

PRBool
SECNAV_RunInitialSecConfig(void)
{
    JSBool ok;
    jsval rv;
    char testfn[] = "typeof(SecurityConfig.initSecurityConfig);";
    char buf[] = "SecurityConfig.initSecurityConfig()";

    /* test to see if the function SecurityConfig.initSecurityConfig()
     * is defined in the JS context
     */
    ok = JS_EvaluateScript(configContext, globalConfig, testfn,
			   strlen(testfn), 0, 0, &rv);
    if (ok && JSVAL_IS_STRING(rv)) {
	const char *fn = JS_GetStringBytes(JSVAL_TO_STRING(rv));
	/* if the function isn't defined, return */
	if (XP_STRCMP(fn,"function")) {
	    return(PR_FALSE);
	}
    }

    /* function is defined...execute it*/
    ok = JS_EvaluateScript(configContext, globalConfig, buf, strlen(buf),
			   0, 0, &rv);

    if (ok) {
    }
    return(PR_TRUE);
}

#endif /* AKBAR */

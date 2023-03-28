#ifndef _CERTLDAP_H_
#define _CERTLDAP_H_

typedef enum {
    certLdapPostCert,
    certLdapGetCert
} CertLdapOpType;

typedef enum {
    certLdapSearchMail,
    certLdapSearchCN
} CertLdapSearchType;

typedef enum {
    certLdapBindWait,
    certLdapSearchWait,
    certLdapPostWait,
    certLdapPWBindWait,
    certLdapDone,
    certLdapError
} CertLdapState;

typedef struct _certLdapOpDesc CertLdapOpDesc;

typedef void (* CertLdapCB)(struct _certLdapOpDesc *op);
    
struct _certLdapOpDesc {
    PRArenaPool *arena;
    char *servername;
    int serverport;
    char *base;
    int numnames;
    char **names;
    SECItem *rawcerts;
    CertLdapSearchType searchType;
    CertLdapOpType type;
    SECItem postData;
    CertLdapCB cb;
    void *cbarg;
    SECStatus rv;
    void *window;
    SECNAVDeleteCertCallback *redrawcb;
    struct _SECStatusDialog *statdlg;
    struct _CertLdapConnData *connData;
    char *mailAttrName;
    PRBool isSecure;
};

typedef struct _CertLdapConnData {
    URL_Struct *urlStr;
    CertLdapOpDesc *op;
    LDAP *ld;
    CertLdapState state;
    int msgid;
    char *postdn;
    unsigned long fd;
} CertLdapConnData;

#define ATTR_USER_CERT "userCertificate;binary"
#define ATTR_SMIME_CERT "userSMimeCertificate;binary"
#define ATTR_MAIL "mail"

SEC_BEGIN_PROTOS

CertLdapConnData *SECNAV_CertLdapLoad(URL_Struct *urlStr);
int SECNAV_CertLdapProcess(CertLdapConnData *connData);
int SECNAV_CertLdapInterrupt(CertLdapConnData *connData);

SEC_END_PROTOS

#endif /* _CERTLDAP_H_ */

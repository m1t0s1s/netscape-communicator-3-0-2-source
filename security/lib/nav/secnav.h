#ifndef _SECNAV_H_
#define _SECNAV_H_

/*
 * Stuff that the Navigator needs from the security library.
 *
 * $Id: secnav.h,v 1.45.2.12 1997/05/24 00:23:43 jwz Exp $
 */

#include "ntypes.h"
#include "prarena.h"
#include "mcom_db.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secdert.h"
#include "cryptot.h"
#include "keyt.h"
#include "certt.h"
#include "secmodt.h"

#define SEC_CLIENT_CERT_PAGE "https://certs.netscape.com/client.html"

typedef struct {
    void 	*redrawCallback;
    void 	*sa_state;
    void	*dlg_state;
    void 	*proto_win;
    void	*slot_ptr;
} SECNAVPKCS12ImportArg;

typedef struct {
    char *nickname;
} SECNAVPKCS12ExportArg;

typedef enum {
    pwdGenericExport,
    pwdGenericImport,
    pwdIntegrityImport,
    pwdIntegrityExport,
    pwdEncryptionImport,
    pwdEncryptionExport
} secnavP12PWDPrompt;

typedef struct {
    secnavP12PWDPrompt	prompt_type;
    void *wincx;
} SECNAVPKCS12PwdPromptArg;

SEC_BEGIN_PROTOS
/************************************************************************/

/*
** Return a list of named selections for key generation.
**	"spec" a string describing defaults or limitations of choices
*/
char **SECNAV_GetKeyChoiceList(char *type, char *pqg);

/*
** Generate and record a public/private key pair on behalf of a form KEYGEN.
**	"proto_win" window context to use for prompting for key password
**	"form" is needed to do a postponed submit
**	"choiceString" one of the strings returned by SECNAV_GetKeyChoiceList
**	"challenge" is an ascii string of challenge data from the CA
**	"typeString" is an ascii string specifying the type of key to generate
**	"pqgString" is a comma seperated list of base64 encoded pqg values
**	"pValue" is where to put the result string representing the
**		public key
**	"pDone" is where to record that the dialog(s) successfully
**		completed
** The return value says whether to wait (for dialogs) or not.
** If not, then *pValue should contain the result (or NULL, if there
** was some error/problem).
*/
PRBool SECNAV_GenKeyFromChoice(void *proto_win, LO_Element *form,
			       char *choiceString, char *challenge,
			       char *typeString, char *pqgString,
			       char **pValue, PRBool *pDone);

/*
** Get the key file password.  Can be used as the callback for various
** SEC routines that need passwords.
**	"arg" is the window context to use when prompting for the password.
*/
char * SECNAV_PromptPassword(PK11SlotInfo *slot,void *wincx);

#ifdef notdef
SECItem *SECNAV_GetPWKey(void *arg, SECKEYKeyDBHandle *keydb);

/*
** Forget the current key file password
*/
void SECNAV_ForgetPWKey(void);
#endif

/*
** Forget all private information
*/
void SECNAV_Logout(void);

/*
** Pass the password related preferences into the security library
** 	"proto_win" is a window context, in case we need to put up a dialog
**	"usePW" is true for "use a password", false otherwise
**	"askPW" is when to ask for the password:
**		-1 = every time its needed
**		0  = once per session
**		1  = after 'n' minutes of inactivity
**	"timeout" is the number of inactive minutes to forget the password
**
** NOTE: The first two parameters (cx, usePW) are actually ignored; for this
**	reason,	we have created the following function, which should be used
**	instead.  Once all FEs have made the transition, this function will
**	go away.
*/
void SECNAV_SetPasswordPrefs(void *proto_win, PRBool usePW, int askPW,
			     int timeout);

/*
** Pass to the security library the preferences describing when to ask
** for a password.
**	"askPW" is when to ask for the password:
**		-1 = every time its needed
**		 0 = once per session
**		 1 = after 'n' minutes of inactivity
**	"timeout" is the number of inactive minutes to forget the password
**		(its value is only relevant when the value of askPW is 1)
*/
void SECNAV_SetPasswordAskPrefs(int askPW, int timeout);

/*
** Ask the backend if a password is in use or not.  (Only the backend knows
** for sure!)  The FEs can use this to decide whether the button in the
** Password preferences panel should say "Set Password" or "Change Password".
*/
PRBool SECNAV_GetPasswordEnabled(void);

/*
** Change the user password, or set it if one is not already in use.
**	"proto_win" is the main window, so that I can pop up a dialog
*/
void SECNAV_ChangePassword(void *proto_win);

/* callback for getting client auth data */
int SECNAV_GetClientAuthData(void *arg,
			     int fd,
			     CERTDistNames *caNames,
			     CERTCertificate **pRetCert,
			     SECKEYPrivateKey **pRetKey);

/* callback for handling unknown server certificates */
int SECNAV_HandleBadCert(void *arg, int fd);

int SECNAV_ConfirmServerCert(void *arg, int fd, PRBool checkSig,
			     PRBool isServer);

/*
** Initialize and shutdown the security library for mozilla
*/
void SECNAV_Init(void);
void SECNAV_Shutdown(void);

/* get the hostname part of a URL, excluding the :portnumber */
char *SECNAV_GetHostFromURL(char *url);

/* typedef for completion callback for pw setup panel */
typedef SECStatus (*SECNAVPWSetupCallback)(void *arg, PRBool cancel);

/* bring up the password setup panel */
void SECNAV_MakePWSetupDialog(void *proto_win, SECNAVPWSetupCallback cb1,
			      SECNAVPWSetupCallback cb2, PK11SlotInfo *slot,
			      void *cbarg);
/* bring up the password change dialog */
void SECNAV_MakePWChangeDialog(void *proto_win, PK11SlotInfo *slot);

/* bring up key generation dialog */
SECStatus SEC_MakeKeyGenDialog(void *proto_win, LO_Element *form,
			       int keysize, char *challenge, KeyType type,
			       PQGParams *pqg, char **pValue, PRBool *pDone);

/* set up all the mozilla specific options and callbacks for an ssl socket */
int SECNAV_SetupSecureSocket(int fd, char *url, MWContext *cx);


/* typedef for callback to FEs when java principal is deleted */
typedef void (*SECNAVJavaPrinCallback)(void *arg);

/*
 * functions to edit/delete java principals and privieleges dialogs
 */
SECStatus SECNAV_IsJavaPrinCert(void *proto_win, char *javaPrin);

void SECNAV_DeleteJavaPrincipal(void *proto_win, char *javaPrin,
				SECNAVJavaPrinCallback f,
				void *arg);

void SECNAV_EditJavaPrivileges(void *proto_win, char *javaPrin);

void SECNAV_signedAppletPrivileges(void *proto_win, char *javaPrin, 
				   char *javaTarget, char *risk, 
				   int isCert);

#define PERMISSION_NOT_SET 0
#define ALLOWED_FOREVER    1
#define ALLOWED_SESSION    2
#define FORBIDDEN_FOREVER  3
#define BLANK_SESSION      4

/* typedef for callback to FEs when cert is deleted */

typedef struct {
    void (*callback)(void *arg);
    void *cbarg;
} SECNAVDeleteCertCallback;

/*
 * functions to create certificate preferences dialogs
 */
void SECNAV_ViewUserCertificate(CERTCertificate *cert, void *proto_win,
				void *arg);

SECNAVDeleteCertCallback *
SECNAV_MakeDeleteCertCB(void (* callback)(void *), void *cbarg);

void SECNAV_DeleteUserCertificate(CERTCertificate *cert, void *proto_win,
				  void *arg);

void SECNAV_NewUserCertificate(MWContext *cx);

void SECNAV_EditSiteCertificate(CERTCertificate *cert, void *proto_win,
				void *arg);

void SECNAV_DeleteSiteCertificate(CERTCertificate *cert, void *proto_win,
				  void *arg);
void SECNAV_DeleteEmailCertificate(CERTCertificate *cert, void *proto_win,
				   void *arg);

/*
 * Brings up the security info dialog when the key icon is clicked
 */
void SECNAV_BrowserSecInfoDialog(void *proto_win, URL_Struct *url);

/*
 * Temporary API for bringing up security advisor.
 */
void SECNAV_SecurityAdvisor(void *proto_win, URL_Struct *url);

/* tell the security library that we are sending data to the server at
 * the other end of this fd, so that we can warn the user if necessary.
 */
void SECNAV_Posting(int fd);

/* tell the security library that we are performing a HEAD operation
 * to the server at the other end of this fd, so that we don't generate
 * user warnings.
 */
void SECNAV_HTTPHead(int fd);

/*
 * generic (and relatively unsafe!) munging...
 */
char *SECNAV_MungeString(const char *string);
char *SECNAV_UnMungeString(const char *string);

void SECNAV_MakeCADownloadPanel(void *proto_win,
				CERTCertificate *cert,
				CERTDERCerts *derCerts);
void SECNAV_MakeUserCertDownloadDialog(void *proto_win,
				       CERTCertificate *cert,
				       CERTDERCerts *derCerts);

void SECNAV_MakeEmailCertDownloadDialog(void *proto_win,
					CERTCertificate *cert,
					CERTDERCerts *derCerts);

int SECNAV_MakeClientAuthDialog(void *proto_win,
				int fd,
				CERTDistNames *caNames);

/*
 * internal url stuff for certificate buttons
 */
char *SECNAV_MakeCertButtonString(CERTCertificate *cert);
void SECNAV_HandleInternalSecURL(URL_Struct *url, MWContext *cx);

/*
 * configure ciphers
 */
void SECNAV_MakeCiphersDialog(void *proto_win, int version);

/* internal interface */
char *secmoz_GetCertSNString(CERTCertificate *cert);

/* Client auth prefs stuff */
CERTCertNicknames *SECNAV_GetDefUserCertList(CERTCertDBHandle *handle,
								void *wincx);
SECStatus SECNAV_SetDefUserCertList(char *chosen);

/* zero a password string */
void SECNAV_ZeroPassword(char *password);

/*
 * Stuff to support the security: URL type
 */
char *SECNAV_SecURLContentType(char *which);
int SECNAV_SecURLData(char *which, NET_StreamClass *stream,
		   struct MWContext_ *cx);
int SECNAV_SecHandleSecurityAdvisorURL(MWContext *cx, const char *which);

NET_StreamClass *
SECNAV_CertificateStream(FO_Present_Types format_out, void *data,
			 URL_Struct *urls, MWContext *cx);

NET_StreamClass *
SECNAV_RevocationStream(FO_Present_Types format_out, void *data,
			URL_Struct *urls, MWContext *cx);

NET_StreamClass *
SECNAV_CrlStream(FO_Present_Types format_out, void *data,
		 URL_Struct *urls, MWContext *cx);

/* wrapper for FE_SecurityDialog() that allows security code to manage
 * the preferences
 */
PRBool SECNAV_SecurityDialog(MWContext *context, int state);

#ifdef FORTEZZA

NET_StreamClass *
SECNAV_MakePreencryptedStream(FO_Present_Types format_out, void *data,
			      URL_Struct *url, MWContext *cx);

void
SECNAV_SetPreencryptedSocket(void *data, int fd);

#endif /* FORTEZZA */

void
SECNAV_DisplayCertDialog(MWContext *cx, CERTCertificate *cert);

SECItem *
SECNAV_GetKeyDBPassword(void *arg, SECKEYKeyDBHandle *keydb);

PRBool
SECNAV_GetKeyDBPasswordStatus(void);

SECItem *
SECNAV_GetExportPassword(void *arg);

PRBool 
SECNAV_WriteExportFile(void *ctxt, SECItem *data);

SECItem * 
SECNAV_ReadExportFile(void *ctxt);

SECItem *
SECNAV_NicknameCollisionCallback(SECItem *old_nick, PRBool *cancel, void *wincx);

/* get the user's default email cert based on the pref */
CERTCertificate *
SECNAV_GetDefaultEMailCert(void *win_cx);

/* these are used by AutoAdmin to configure Security info */
int
SECNAV_InitConfigObject(void);

int
SECNAV_RunInitialSecConfig(void);

/* typedef for FindInit and FindNext */
typedef struct SECNAVPolicyFindStr  SECNAVPolicyFind;

SECNAVPolicyFind *
SECNAV_FindInit(const char * cipherFamily, /* see CIPHER_FAMILY_* */
		PRBool policyFilter, 	 /* include iff allowed by policy */
		PRBool preferenceFilter);/* include iff preference chosen */

long	SECNAV_FindNext(SECNAVPolicyFind * snpf);

void	SECNAV_FindEnd(SECNAVPolicyFind * snpf);

/* alg is chosen as a "preference". */
PRBool	SECNAV_AlgPreferred( long protocolID);

/* alg is allowed by export policy module */
PRBool	SECNAV_AlgAllowed(   long protocolID);

/* alg is both allowed by policy and chosen by preference */
PRBool	SECNAV_AlgOKToDo(    long protocolID);

long 	SECNAV_getMaxKeyGenBits(void);

/* re-read crypto algorithms Preferences from file. */
void	SECNAV_PrefsRefresh(void);

/* modify boolean crypto algorithm preference, in memory and on disk. */
void	SECNAV_PrefBoolUpdate(long protocolID, PRBool prefval);

/* initially read crypto policy and crypto algorithm prefrences */
void 	SECNAV_PolicyInit(void);

/* return pointer to preference name for protocol/algorithm */
const char * SECNAV_GetProtoName(long protocolID);

/* return pointer to localized (?) description string for protocol/algorithm */
const char * SECNAV_GetProtoDesc(long protocolID);


typedef void (* CERTSelectCallback)(CERTCertificate *cert,
				    void *proto_win, void *arg);

SECStatus
SECNAV_SelectCert(CERTCertDBHandle *handle, char *name, int messagestr,
		  void *proto_win, CERTSelectCallback callback, void *cbarg);

void SECNAV_DefaultEmailCertInit(void);

void
SECNAV_VerifyCertificate(CERTCertificate *cert, void *proto_win, void *arg);

/* Returns a string, a comma separated list of the country codes in all
 * the "user certs".  Or returns NULL if no user certs exists.
 */
char * SECNAV_GetUserCertCountries(void);

SECStatus SECNAV_ImportPKCS12Object (void *arg, PRBool cancel);
void SECNAV_ExportPKCS12Object (CERTCertificate *cert, void *proto_win, void *arg);

void SECNAV_SearchLDAPDialog(void *window, SECNAVDeleteCertCallback *cb,
			     char *emailAddrs);
void SECNAV_PublishSMimeCertDialog(void *window);
SEC_END_PROTOS


#endif /* _SECNAV_H_ */

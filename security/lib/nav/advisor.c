/* -*- Mode: C; tab-width: 8 -*-
 * Security Advisor for Mozilla
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: advisor.c,v 1.12.2.31 1997/05/24 00:23:37 jwz Exp $
 */

#include "xp.h"
#include "xpgetstr.h"
#include "prtypes.h"
#include "htmldlgs.h"
#include "secprefs.h"
#include "secnav.h"
#include "cert.h"
#include "ssl.h"
#ifndef NSPR20
#include "prglobal.h"
#else
#include "prtypes.h"
#include "prlog.h"
#endif /* NSPR20 */
#include "prefapi.h"
#include "sslproto.h"
#include "pk11func.h"
#include "secmod.h"
#include "pk11dlgs.h"
#include "structs.h"
#include "ntypes.h"
#include "secpkcs7.h"
#include "pkcs12.h"
#include "msgcom.h"
#include "fe_proto.h"

#ifdef AKBAR
# include "nspr/prtime.h"
#else  /* !AKBAR */
# include "prtime.h"
#endif /* !AKBAR */

#ifdef XP_MAC
# include "certdb.h"
#else
# include "../cert/certdb.h"
#endif

#ifdef JAVA
#include "javasec.h"
#endif

extern int MK_OUT_OF_MEMORY;
extern int XP_SA_ALG_AND_BITS_FORMAT;
extern int SEC_ERROR_CERT_ADDR_MISMATCH;

extern int SA_STRINGS_BEGIN;
extern int SA_STRINGS_END;
extern int SA_VARNAMES_BEGIN;
extern int SA_VARNAMES_END;
extern int SA_VIEW_BUTTON_LABEL;
extern int SA_GET_CERT_BUTTON_LABEL;
extern int SA_GET_CERTS_BUTTON_LABEL;
extern int SA_VERIFY_BUTTON_LABEL;
extern int SA_DELETE_BUTTON_LABEL;
extern int SA_EXPORT_BUTTON_LABEL;
extern int SA_IMPORT_BUTTON_LABEL;
extern int SA_EDIT_BUTTON_LABEL;
extern int SA_SSL2_CONFIG_LABEL;
extern int SA_SSL3_CONFIG_LABEL;
extern int SA_SMIME_CONFIG_LABEL;
extern int SA_VIEW_CERT_BUTTON_LABEL;
extern int SA_EDIT_PRIVS_BUTTON_LABEL;
extern int SA_REMOVE_BUTTON_LABEL;
extern int SA_SET_PASSWORD_LABEL;
extern int SA_CHANGE_PASSWORD_LABEL;
extern int SA_VIEW_EDIT_BUTTON_LABEL;
extern int SA_ADD_BUTTON_LABEL;
extern int SA_LOGOUT_ALL_BUTTON_LABEL;
extern int SA_SEARCH_DIR_BUTTON_LABEL;
extern int SA_SEND_CERT_BUTTON_LABEL;
extern int SA_PAGE_INFO_LABEL;

static char *all_sa_strings = 0;


static int
init_sa_strings(void)
{
  int status = 0;
  if (all_sa_strings)
    return 0;
  else
    {
      int n_strings = 0;
      int total_size = 0;
      int i = 0;
      char **vars = 0;
      char **localized = 0;
      char *out = 0;

      n_strings = (SA_STRINGS_END - SA_STRINGS_BEGIN);
      XP_ASSERT(n_strings == (SA_VARNAMES_END - SA_VARNAMES_BEGIN));


      vars = (char **) PORT_Alloc (sizeof(char*) * (n_strings + 1));
      if (!vars)
        {
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
      PORT_Memset(vars, 0, (sizeof(char*) * (n_strings + 1)));


      localized = (char **) PORT_Alloc (sizeof(char*) * (n_strings + 1));
      if (!localized)
        {
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}
      PORT_Memset(localized, 0, (sizeof(char*) * (n_strings + 1)));


      for (i = 1; i < n_strings; i++)
	{
	  const char *str;

	  str = XP_GetString(SA_VARNAMES_BEGIN + i);
	  if (!str)
	    {
	      status = MK_OUT_OF_MEMORY;
	      goto FAIL;
	    }
	  vars[i] = PORT_Strdup(str);
	  if (!vars[i])
	    {
	      status = MK_OUT_OF_MEMORY;
	      goto FAIL;
	    }

	  str = XP_GetString(SA_STRINGS_BEGIN + i);
	  if (!str)
	    {
	      status = MK_OUT_OF_MEMORY;
	      goto FAIL;
	    }
	  localized[i] = PORT_Strdup(str);
	  if (!localized[i])
	    {
	      status = MK_OUT_OF_MEMORY;
	      goto FAIL;
	    }

	  total_size += PORT_Strlen(vars[i]);
	  total_size += PORT_Strlen(localized[i]);
	  total_size += 50;
	}

      all_sa_strings = (char *) PORT_Alloc(total_size + 200);
      if (!all_sa_strings)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

      out = all_sa_strings;
      *out = 0;

#ifdef AKBAR
      XP_STRCAT(out, "var Event = null;\n");	/* Kludge! */
      out += XP_STRLEN(out);
#endif

      for (i = 1; i < n_strings; i++)
	{
	  const char *s;
	  PORT_Strcpy(out, "var ");
	  PORT_Strcat(out, vars[i]);
	  PORT_Strcat(out, " = \"");
	  out += PORT_Strlen(out);

	  PORT_Free(vars[i]);
	  vars[i] = 0;

	  for (s = localized[i]; s && *s; s++)
	    {
	      if (*s == '"' || *s == '\t' || *s == CR || *s == LF)
		{
		  XP_SPRINTF(out, "\\%03o",
			     (unsigned int) ((unsigned char *) s));
		  out += PORT_Strlen(out);
		}
	      else
		{
		  *out++ = *s;
		}
	    }

	  *out = '\0';
	  PORT_Free(localized[i]);
	  localized[i] = 0;

	  PORT_Strcat(out, "\";\n");
	  out += PORT_Strlen(out);
	}

    FAIL:
      if (vars)
	{
	  for (i = 0; i < n_strings; i++)
	    if (vars[i]) PORT_Free(vars[i]);
	  PORT_Free(vars);
	}

      if (localized)
	{
	  for (i = 0; i < n_strings; i++)
	    if (localized[i]) PORT_Free(localized[i]);
	  PORT_Free(localized);
	}
    }

  return status;
}



/* These defines are used for passing information to the Info Panel (drawn in
 * drawstatus() in secprefs.html). The numbers here match numbers in the
 * javascript for the context-sensitive messaging feaure
 *
 * Encrypt errors are passed in a javascript array, hence the quotes
 */

#define NOINFO 0
#define COMPOSE 1
#define MAIL_MESSAGE 2
#define NEWS_MESSAGE 3
#define SNEWS_MESSAGE 4
#define NAVIGATOR 5
#define MAIL_THREADS 6
#define NEWS_THREADS 7

#define ENC_NOERR 0    /* Encryption is possible or encrypted message is okay*/
#define SIG_NOERR 0    /* Signing is possible or signed message is okay */
  /* Errors for COMPOSE context */

#define ENC_NORECIPIENT 1      /* There is a NULL recipient list. */
#define ENC_CERTMISSING 2      /* No cert for intended recipient */
#define ENC_CERTEXPIRED 3      /* Have recipient cert, but it is expired */
#define ENC_CERTREVOKED 4      /* The recipient cert has been revoked */
#define ENC_CERTNOALIAS 5      /* There is no cert for the recipient alias */
#define ENC_NEWSGROUP 6        /* Recipient is a news/discussion group, no encryption */
#define ENC_CERTINVALID     7
#define ENC_CERTUNTRUSTED   8
#define ENC_ISSUERUNTRUSTED 9
#define ENC_ISSUERUNKNOWN   10
#define ENC_CERT_UNKNOWN_ERROR 11


#define SIG_CERTMISSING 1      /* User does not have a cert to sign with */

/* Errors for MAIL_MESSAGE context and NEWS_MESSAGE context where appropriate*/
#define ENC_NOTENCRYPTED 1    /* Clear-text message */
#define ENC_FORANOTHER 2      /* Encrypted for another user, not readable */
#define ENC_UNKNOWN_ERROR 3   /* Uh, something else. */

#define SIG_NOTSIGNED 1       /* The message is not signed */
#define SIG_INVALID 2         /* The signer's cert is invalid (expired, etc) */
#define SIG_TAMPERED 3        /* The signed content is bogus */
#define SIG_ADDR_MISMATCH 4   /* "From" header and Cert email addrs differ */

/* Errors for NEWS_MESSAGE context  that differ from MAIL_MESSAGE context */
#define ENC_SECURE    0 
#define ENC_NOTSECURE 1

extern int XP_SECURITY_ADVISOR;
extern int XP_SECURITY_ADVISOR_TITLE_STRING;
extern int SECADV_STRINGS_0;
extern int SECADV_STRINGS_END;
extern int XP_CERT_SELECT_EDIT_STRING;
extern int XP_CERT_SELECT_DEL_STRING;
extern int XP_CERT_SELECT_VIEW_STRING;
extern int XP_CERT_SELECT_VERIFY_STRING;

extern int SEC_ERROR_NOT_A_RECIPIENT;
extern int SEC_ERROR_PKCS7_BAD_SIGNATURE;
extern int SEC_ERROR_NO_MEMORY;

/* errors for PKCS 12 */
extern int SEC_ERROR_EXPORTING_CERTIFICATES;
extern int SEC_ERROR_IMPORTING_CERTIFICATES;

/* errors from CERT_VerifyCert() */
extern int SEC_ERROR_UNTRUSTED_CERT;
extern int SEC_ERROR_UNTRUSTED_ISSUER;
extern int SEC_ERROR_UNKNOWN_ISSUER;
extern int SEC_ERROR_EXPIRED_CERTIFICATE;
extern int SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE;
extern int SEC_ERROR_CRL_EXPIRED;
extern int SEC_ERROR_KRL_EXPIRED;
extern int SEC_ERROR_REVOKED_KEY;
extern int SEC_ERROR_REVOKED_CERTIFICATE;
extern int SEC_ERROR_CERT_NOT_VALID;
extern int SEC_ERROR_CA_CERT_INVALID;
extern int SEC_ERROR_BAD_DER;
extern int SEC_ERROR_CRL_BAD_SIGNATURE;
extern int SEC_ERROR_KRL_BAD_SIGNATURE;
extern int SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID;
extern int SEC_ERROR_NO_KRL;
extern int SEC_ERROR_BAD_SIGNATURE;

extern void MimeGetCompositionSecurityInfo(MWContext *context,
					   char **sender_ret,
					   char **mail_recipients_ret,
					   char **news_recipients_ret,
					   XP_Bool *encrypt_p_ret,
					   XP_Bool *sign_p_ret,
					   int *delivery_error_ret);

extern void MimeGetSecurityInfo(MWContext *context, const char *url,
				SEC_PKCS7ContentInfo **encrypted_cinfo_return,
				SEC_PKCS7ContentInfo **signed_cinfo_return,
				char **sender_addr_return,
				int32 *decode_error_return,
				int32 *verify_error_return);
extern char *MSG_ExtractRFC822AddressMailboxes (const char *line);
extern char *MSG_RemoveDuplicateAddresses (const char *addrs, const char *other_addrs);
extern int MSG_ParseRFC822Addresses (const char *line, char **names, char **addresses);
#ifdef AKBAR
extern void MSG_Command (MWContext *context, MSG_CommandType command);
#endif /* !AKBAR */

typedef struct _nnNode {
    char *nickname;
    struct _nnNode *next;
} nnNode;

typedef struct {
    CERTCertDBHandle *handle;
    MWContext *globalMWContext;
    SEC_PKCS7ContentInfo *encrypted_p7contentinfo;
    SEC_PKCS7ContentInfo *signed_p7contentinfo;
    unsigned int curpage;
    unsigned int pwasktype;
    unsigned int pwaskinterval;
    char *sslcertname;
    char *mailcertname;
    PRBool warnentersecure;
    PRBool warnleavesecure;
    PRBool warnmixed;
    PRBool warnsendclear;
    PRBool enablessl2;
    PRBool enablessl3;
    PRBool passwordset;
    PRBool encryptalways;
    PRBool signalways;
    PRBool signnewsalways;
    PRBool encryptthis;
    PRBool oldencryptthis;
    PRBool signthis;
    PRBool oldsignthis;
    /* stuff for HTTP security info, only valid for infocontext == 5 */
    PRBool ssl_encrypted;
    PRBool ssl_mixed;
    PRBool nav_chrome_missing;
    PRBool nav_missing_menubar;
    PRBool nav_missing_toolbar;
    PRBool nav_missing_personalbar;
    PRBool nav_missing_location;
    PRBool nav_missing_status;
    PRBool nav_java;
    PRBool nav_js;
    char *nav_site_name;
    char *nav_creator_name;
    CERTCertificate *server_cert;
#if 0
    PRBool fwdwarning;
    PRBool rewarning;
#endif
    unsigned int infocontext;
    char *encrypterrors;
    char *encrypterrorstr;
    char *algorithmdesc;
    unsigned int signerror;
    char *signerrorstr;
    char *signtime;
    char *signername;
    char *signeraddr;
    char *senderaddr;
    char *destnames;
    char *mycerts;
    char *mysslcerts;
    char *mymailcerts;
    char *peoplescerts;
    char *sitecerts;
    char *cacerts;
    char *crmodules;
    char *principals;
    char *certs_to_fetch;
    PRArenaPool *tmparena;
    nnNode *mycertsList;
    nnNode *mysslcertsList;
    nnNode *mymailcertsList;
    nnNode *sitecertsList;
    nnNode *cacertsList;
    nnNode *peoplescertsList;
    URL_Struct *url;
    void *proto_win;
} SecurityAdvisorState;

CERTCertificate *
SECNAV_GetDefaultEMailCert(void *win_cx)
{
    char *charvalue = NULL;
    CERTCertificate *cert = NULL;
    int ret;
    
    ret = PREF_CopyCharPref("security.default_mail_cert", &charvalue);
    if ( charvalue ) {
	cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
						  charvalue);
	if (cert == NULL) {
	    /* WE NEED TO GET THE WINDOW CONTEXT IN OUR CALL!! */
	    if (win_cx != NULL) {
	    	cert = PK11_FindCertFromNickname(charvalue,win_cx);
	    }
	}
	PORT_Free(charvalue);
    }

    return(cert);
}

static SECStatus
setupDefaultEmailCert(void)
{
    CERTCertificate *cert;
    SECStatus rv = SECFailure;
    
    cert = SECNAV_GetDefaultEMailCert(NULL);

    if ( cert && ( cert->subjectList->emailAddr == NULL ) ) {
	/* the user's cert does not have an e-mail address entry, so add one */

	if ( cert->emailAddr && ( cert->rawKeyUsage & KU_KEY_ENCIPHERMENT ) ) {
	    rv = CERT_SaveSMimeProfile(cert, NULL, NULL);
	    /* don't care if it fails */
	}
    }

    return(rv);
}

int
PR_CALLBACK SECNAV_MailCertPrefChange(const char *prefname, void *arg)
{
    SECStatus rv;
    
    rv = setupDefaultEmailCert();
    return rv;
}

static void
initDefaultMailCertPref(void)
{
    char *charvalue = NULL;
    CERTCertificate *cert = NULL;
    int ret;
    CERTCertNicknames *names;
    int i;
    
    ret = PREF_CopyCharPref("security.default_mail_cert", &charvalue);
    if ( ( charvalue == NULL ) || ( *charvalue == '\0' ) ) {
	/* the user does not have a preference value */
	names = CERT_GetCertNicknames(CERT_GetDefaultCertDB(),
				      SEC_CERT_NICKNAMES_USER, NULL);
	
	if ( names ) {
	    for ( i = 0; i < names->numnicknames; i++ ) {
		cert =
		    CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
						       names->nicknames[i]);

		if ( cert != NULL ) {
		    if ( cert->emailAddr ) {
			/* cert has an email addr, so make it the default */
			ret = PREF_SetCharPref("security.default_mail_cert",
					       names->nicknames[i]);
#ifdef AKBAR
			/* We don't have PREF_RegisterCallback() in Akbar. */
			SECNAV_MailCertPrefChange("security.default_mail_cert",
						  names->nicknames[i]);
#endif /* AKBAR */
			ret = PREF_SavePrefFile();
			CERT_DestroyCertificate(cert);
			break;
		    }
		    
		    CERT_DestroyCertificate(cert);
		}
	    }

	    CERT_FreeNicknames(names);
	}
    }

    return;
}

void
SECNAV_DefaultEmailCertInit(void)
{
    SECStatus rv;
    
#ifndef AKBAR
    /* We don't have PREF_RegisterCallback() in Akbar. */
    PREF_RegisterCallback("security.default_mail_cert",
			  SECNAV_MailCertPrefChange,
			  NULL);
#endif /*!AKBAR */
    
    rv = setupDefaultEmailCert();
    
    if ( rv != SECSuccess ) {
	initDefaultMailCertPref();
    }
    
    return;
}

#ifdef XP_WIN
static char *secprefsfooter = NULL;
static char *secprefsfooter2 = "";
extern char *wfe_LoadResourceString(char *);
#endif


/* Converts a C string to JavaScript, properly quoted.
   if add_quotes is true, wraps the whole thing in double-quotes too.
 */
static char *
sa_make_js_string(const char *string, PRBool add_quotes)
{
  const char *s;
  char *result, *out;
  int L = 0;

  if (!string) string = "";

  /* put a \ before \ or "
     turn CRLF into LF
     put \n\ before CR or LF
   */

  for (s = string; *s; s++)
    {
      L++;
      if (*s == CR && s[1] == LF)
	s++;
      if (*s == CR || *s == LF)
	L += 3;
      else if (*s == '\\' || *s == '"')
	L++;
    }
  if (add_quotes)
    L += 2;

  result = (char *) PORT_Alloc(L+1);
  if (!result) return 0;

  out = result;

  if (add_quotes)
    *out++ = '"';

  for (s = string; *s; s++)
    {
      if (*s == CR && s[1] == LF)
	s++;
      if (*s == CR || *s == LF)
	{
	  *out++ = '\\';
	  *out++ = 'n';
	  *out++ = '\\';
	}
      else if (*s == '\\' || *s == '"')
	*out++ = '\\';
      *out++ = *s;
    }

  if (add_quotes)
    *out++ = '"';

  *out = 0;
  return result;
}


/* This kludgy little hack appends data to the `dest' string.
   prefix, if non-null, is appended as-is.
   new_string is wrapped in double-quotes and properly escaped as
     a JavaScript string, then appended.
   then suffix, if non-null, is appended as-is.

   dest is realloc'ed larger as needed, and returned.
 */
static char *
sa_push_js_string(char *dest,
		  const char *prefix,
		  const char *new_string,
		  const char *suffix)
{
  char *s, *result;
  if (!new_string) new_string = "";
  s = sa_make_js_string(new_string, PR_TRUE);
  if (!s) return dest; /* #### MK_OUT_OF_MEMORY */

  result = PR_sprintf_append(dest, "%s%s%s",
			     (prefix ? prefix : ""),
			     s,
			     (suffix ? suffix : ""));
  if (s) PORT_Free(s);
  return result;
}


/* This is like sa_push_js_string, but appends an integer instead of
   a string.  realloc, prefix, and suffix behavior are the same.
 */
static char *
sa_push_js_int(char *dest,
	       const char *prefix,
	       int value,
	       const char *suffix)
{
  return PR_sprintf_append(dest, "%s%i%s",
			   (prefix ? prefix : ""),
			   value,
			   (suffix ? suffix : ""));
}


static XPDialogStrings *
getAdvisorStrings(SecurityAdvisorState *state)
{
    XPDialogStrings *strings;
    char *varstring;

#ifdef XP_WIN
    if ( secprefsfooter == NULL ) {
	secprefsfooter = wfe_LoadResourceString("secprefs");
    }
#endif
    
    /* get empty strings */
    strings = XP_GetDialogStrings(XP_SECURITY_ADVISOR);
    if ( strings == NULL ) {
	return(NULL);
    }
    

    /* This does a lot of realloc'ing, but it's a hell of a lot easier to
       deal with than one enormous call to PR_smprintf. */

#define PUSH_RAW(NAME,VALUE) \
    varstring = PR_sprintf_append(varstring, "var " NAME "=%s;\n", VALUE)
#define PUSH_INT(NAME,VALUE) \
    varstring = PR_sprintf_append(varstring, "var " NAME "=%d;\n", VALUE)
#define PUSH_BOOL(NAME,VALUE) \
    varstring = PR_sprintf_append(varstring, "var " NAME "=%s;\n", \
				  (VALUE ? "true" : "false"))
#define PUSH_STRING(NAME,VALUE) \
    varstring = sa_push_js_string(varstring, "var " NAME "=", VALUE, ";\n")


    /* Close off the `sa_handle' kludge... */
    varstring = PORT_Strdup("';\n");

    PUSH_INT    ("sa_curpage",               state->curpage);
    PUSH_STRING ("sa_selected_ssl_cert",     state->sslcertname);
    PUSH_STRING ("sa_selected_mail_cert",    state->mailcertname);
    PUSH_BOOL   ("sa_warn_enter_secure",     state->warnentersecure);
    PUSH_BOOL   ("sa_warn_leave_secure",     state->warnleavesecure);
    PUSH_BOOL   ("sa_warn_mixed",            state->warnmixed);
    PUSH_BOOL   ("sa_warn_send_clear",       state->warnsendclear);
    PUSH_BOOL   ("sa_enable_ssl2",           state->enablessl2);
    PUSH_BOOL   ("sa_enable_ssl3",           state->enablessl3);
    PUSH_BOOL   ("sa_password_set",          state->passwordset);
    PUSH_INT    ("sa_password_ask_type",     state->pwasktype);
    PUSH_INT    ("sa_password_ask_interval", state->pwaskinterval);
    PUSH_BOOL   ("sa_encrypt_always",        state->encryptalways);
    PUSH_BOOL   ("sa_sign_mail_always",      state->signalways);
    PUSH_BOOL   ("sa_sign_news_always",      state->signnewsalways);
    PUSH_BOOL   ("sa_encrypt_this",          state->encryptthis);
    PUSH_BOOL   ("sa_sign_this",             state->signthis);
/*  PUSH_BOOL   ("sa_fwd_warning",           state->fwdwarning); */
/*  PUSH_BOOL   ("sa_re_warning",            state->rewarning); */
    PUSH_INT    ("sa_info_context",          state->infocontext);
    PUSH_RAW    ("sa_encrypt_errors",        state->encrypterrors);
    PUSH_STRING ("sa_encrypt_error_str",     state->encrypterrorstr);
    PUSH_STRING ("sa_encryption_algorithm",  state->algorithmdesc);
    PUSH_INT    ("sa_sign_error",            state->signerror);
    PUSH_STRING ("sa_sign_error_str",        state->signerrorstr);
    PUSH_STRING ("sa_signing_time",          state->signtime);
    PUSH_STRING ("sa_signer_name",           state->signername);
    PUSH_STRING ("sa_signer_addr",           state->signeraddr);
    PUSH_STRING ("sa_sender_addr",           state->senderaddr);
    PUSH_RAW    ("sa_dest_names",            state->destnames);
    PUSH_RAW    ("sa_your_certs",            state->mycerts);
    PUSH_RAW    ("sa_your_ssl_certs",        state->mysslcerts);
    PUSH_RAW    ("sa_your_mail_certs",       state->mymailcerts);
    PUSH_RAW    ("sa_people_certs",          state->peoplescerts);
    PUSH_RAW    ("sa_site_certs",            state->sitecerts);
    PUSH_RAW    ("sa_signers_certs",         state->cacerts);
    PUSH_RAW    ("sa_cr_modules",            state->crmodules);
    PUSH_RAW    ("sa_principals",            state->principals);
    PUSH_BOOL   ("sa_nav_info_encrypted",    state->ssl_encrypted);
    PUSH_BOOL   ("sa_nav_info_mixed",        state->ssl_mixed);
    PUSH_BOOL   ("sa_nav_info_chrome_missing", state->nav_chrome_missing);
    PUSH_BOOL   ("sa_nav_info_missing_menubar", state->nav_missing_menubar);
    PUSH_BOOL   ("sa_nav_info_missing_toolbar", state->nav_missing_toolbar);
    PUSH_BOOL   ("sa_nav_info_missing_personalbar",
		                               state->nav_missing_personalbar);
    PUSH_BOOL   ("sa_nav_info_missing_location", state->nav_missing_location);
    PUSH_BOOL   ("sa_nav_info_missing_status", state->nav_missing_status);
    PUSH_BOOL   ("sa_nav_info_java",         state->nav_java);
    PUSH_BOOL   ("sa_nav_info_js",           state->nav_js);
    PUSH_STRING ("sa_nav_info_site_name",    state->nav_site_name);
    PUSH_STRING ("sa_nav_info_creator_name", state->nav_creator_name);

#undef PUSH_RAW
#undef PUSH_INT
#undef PUSH_BOOL
#undef PUSH_STRING

    /* make sure we will work on win16 */
    PORT_Assert( ( PORT_Strlen(secprefsheader) +
		  PORT_Strlen(varstring) +
		  PORT_Strlen(all_sa_strings) ) < 30000 );

#ifdef XP_WIN
    PORT_Assert( PORT_Strlen(secprefsfooter) < 60000 );
#else
    PORT_Assert( PORT_Strlen(secprefsfooter) < 30000 );
#endif
    PORT_Assert( PORT_Strlen(secprefsfooter2) < 30000 );
    
    XP_SetDialogString(strings, 0, secprefsheader);
    /* The value of sa_handle is in slot 1. */
    XP_CopyDialogString(strings, 2, varstring);
    XP_SetDialogString(strings, 3, all_sa_strings);
    XP_SetDialogString(strings, 4, secprefsfooter);
    XP_SetDialogString(strings, 5, secprefsfooter2);

    PORT_Free(varstring);

    return(strings);
}

static void
addNickname(PRArenaPool *arena, char *nickname, nnNode **headPtr)
{
    nnNode *node;
    int len;
    
    node = *headPtr;
    while ( node ) {
	if ( PORT_Strcmp(nickname, node->nickname) == 0 ) {
	    return;
	}
	node = node->next;
    }
    /* if we get here then this is a new node */
    node = PORT_ArenaAlloc(arena, sizeof(nnNode));
    
    if ( node != NULL ) {
	len = PORT_Strlen(nickname) + 1;
	node->nickname = PORT_ArenaAlloc(arena,len);
	if ( node->nickname != NULL ) {
	    PORT_Memcpy(node->nickname, nickname, len);
	    node->next = *headPtr;
	    *headPtr = node;
	}
    }
    return;
}

SECStatus
collectCertStrings(CERTCertificate *cert, SECItem *dbkey, void *arg)
{
    SecurityAdvisorState *state;
    CERTCertTrust *trust;
    char *nickname;
    char *emailAddr;
    PRBool usercert = PR_FALSE;
    
    state = (SecurityAdvisorState *)arg;

    /* look at the base cert info stuff, not the dbEntry since
     * we may have been called from a PKCS#11 module */
    nickname = cert->nickname;
    trust = cert->trust;
    emailAddr = cert->emailAddr;

    if ( nickname ) {
					  
	/* it is my cert */
	if ( ( trust->sslFlags & CERTDB_USER ) ||
	    ( trust->emailFlags & CERTDB_USER ) ||
	    ( trust->objectSigningFlags & CERTDB_USER ) ) {
	    usercert = PR_TRUE;
	    addNickname(state->tmparena, nickname, &state->mycertsList);
	    
	    /* it is my SSL cert */
	    if ( trust->sslFlags & CERTDB_USER ) {
		addNickname(state->tmparena, nickname, &state->mysslcertsList);
	    }
	    /* it is my mail cert */
	    if ( ( cert->emailAddr != NULL ) &&
		( trust->emailFlags & CERTDB_USER ) ) {
		addNickname(state->tmparena, nickname,
			    &state->mymailcertsList);
	    }
	}

	/* it is an SSL site */
	if ( trust->sslFlags & CERTDB_VALID_PEER ) {
	    addNickname(state->tmparena, nickname, &state->sitecertsList);
	}
    
	/* CA certs */
	if ( ( trust->sslFlags & CERTDB_VALID_CA ) ||
	    ( trust->emailFlags & CERTDB_VALID_CA ) ||
	    ( trust->objectSigningFlags & CERTDB_VALID_CA ) ) {

	    /* Don't display CAs that are invisible */
	    if ( ! ( ( trust->sslFlags & CERTDB_INVISIBLE_CA ) ||
		    (trust->emailFlags & CERTDB_INVISIBLE_CA ) ||
		    (trust->objectSigningFlags & CERTDB_INVISIBLE_CA ) ) ) {
		addNickname(state->tmparena, nickname, &state->cacertsList);
	    }
	}
    }

    if ( emailAddr != NULL ) {
	/* Other peoples e-mail certs */
	if ( ( !usercert ) && ( trust->emailFlags & CERTDB_VALID_PEER ) ) {
	    addNickname(state->tmparena, emailAddr,
			&state->peoplescertsList);
	}
    }
    
    return(SECSuccess);
}

static char *
sa_make_cert_list(nnNode *head)
{
    nnNode *node;
    char *retstr;
    
    if ( head ) {
	retstr = sa_push_js_string(NULL, "sa_init_sorted_array(",
				   head->nickname, NULL);
	node = head->next;
	while ( node ) {
	    retstr = sa_push_js_string(retstr, ",", node->nickname, NULL);
	    node = node->next;
	}

	retstr = PR_sprintf_append(retstr, ")");
    } else {
	retstr = PORT_Strdup("sa_init_sorted_array()");
    }
    
    return(retstr);
}

SECStatus
getCertVarStrings(SecurityAdvisorState *state, void *proto_win)
{
    SECStatus rv;
    CERTCertDBHandle *handle = state->handle;
    
    /* free any strings hanging around */
    if ( state->mycerts ) {
	PORT_Free(state->mycerts);
	state->mycerts = NULL;
    }
    if ( state->mysslcerts ) {
	PORT_Free(state->mysslcerts);
	state->mysslcerts = NULL;
    }
    if ( state->mymailcerts ) {
	PORT_Free(state->mymailcerts);
	state->mymailcerts = NULL;
    }
    if ( state->peoplescerts ) {
	PORT_Free(state->peoplescerts);
	state->peoplescerts = NULL;
    }
    if ( state->sitecerts ) {
	PORT_Free(state->sitecerts);
	state->sitecerts = NULL;
    }
    if ( state->cacerts ) {
	PORT_Free(state->cacerts);
	state->cacerts = NULL;
    }

    state->tmparena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (state->tmparena == NULL) {
	goto loser;
    }
    state->mycertsList = NULL;
    state->mysslcertsList = NULL;
    state->mymailcertsList = NULL;
    state->sitecertsList = NULL;
    state->cacertsList = NULL;
    state->peoplescertsList = NULL;
	
    rv = SEC_TraversePermCerts(handle, collectCertStrings, (void *)state);
    if (rv == SECSuccess) {
	rv=PK11_TraverseSlotCerts(collectCertStrings,(void *)state,proto_win);
    }

    state->mycerts      = sa_make_cert_list(state->mycertsList);
    state->mysslcerts   = sa_make_cert_list(state->mysslcertsList);
    state->mymailcerts  = sa_make_cert_list(state->mymailcertsList);
    state->peoplescerts = sa_make_cert_list(state->peoplescertsList);
    state->sitecerts    = sa_make_cert_list(state->sitecertsList);
    state->cacerts      = sa_make_cert_list(state->cacertsList);
    
    PORT_FreeArena(state->tmparena, PR_FALSE);
    
loser:
    if ( ( state->mysslcerts == NULL ) ||
	( state->mymailcerts == NULL ) ||
	( state->peoplescerts == NULL ) ||
	( state->sitecerts == NULL ) ||
	( state->cacerts == NULL ) ) {
	return(SECFailure);
    } else {
	return(SECSuccess);
    }
}

static char *
sa_get_algorithm_string(SEC_PKCS7ContentInfo *cinfo)
{
  SECAlgorithmID *algid;
  SECOidTag algtag;
  const char *alg_name;
  int key_size;
  if (!cinfo) return 0;
  algid = SEC_PKCS7GetEncryptionAlgorithm(cinfo);
  if (!algid) return 0;
  algtag = SECOID_GetAlgorithmTag(algid);
  alg_name = SECOID_FindOIDTagDescription(algtag);

  key_size = SEC_PKCS7GetKeyLength(cinfo);

  if (!alg_name || !*alg_name)
    return 0;
  else if (key_size > 0)
    return PR_smprintf(XP_GetString(XP_SA_ALG_AND_BITS_FORMAT),
		       key_size, alg_name);
  else
    return PORT_Strdup(alg_name);
}


/*
 * Fill the context and error values and name strings for context-sensitive
 * Security Advisor "Security Info" display.  See "function displaystatus() 
 * in secprefs.html
 */
SECStatus
getStatusValues(SecurityAdvisorState *state, void *proto_win, URL_Struct *url)
{

    char *sender = 0, *mail_recipients = 0, *news_recipients = 0, *tempname = 0;
    XP_Bool encryptthis = FALSE, signthis = FALSE, encrypted_b = FALSE, 
      signed_b = FALSE, snews_hint = FALSE;
    int delivery_error = 0;
    int32 decode_error = 0, verify_error = 0;
    int errorflag = 0, errorcount=0;
    SEC_PKCS7ContentInfo *encrypted_p7contentinfo = 0;
    SEC_PKCS7ContentInfo *signed_p7contentinfo = 0;
    CERTCertDBHandle *handle = state->handle;
    CERTCertificate *signercert, *receivercert;

    if (state->encrypterrors) {
      PORT_Free(state->encrypterrors);
      state->encrypterrors = NULL;
    }

    if (state->encrypterrorstr) {
      PORT_Free(state->encrypterrorstr);
      state->encrypterrorstr = NULL;
    }

    if (state->algorithmdesc) {
      PORT_Free(state->algorithmdesc);
      state->algorithmdesc = NULL;
    }

    if (state->signerrorstr) {
      PORT_Free(state->signerrorstr);
      state->signerrorstr = NULL;
    }

    if (state->signtime) {
      PORT_Free(state->signtime);
      state->signtime = NULL;
    }

    if (state->signername) {
      PORT_Free(state->signername);
      state->signername = NULL;
    }

    if (state->signeraddr) {
      PORT_Free(state->signeraddr);
      state->signeraddr = NULL;
    }

    if (state->senderaddr) {
      PORT_Free(state->senderaddr);
      state->senderaddr = NULL;
    }

    if (state->destnames) {
      PORT_Free(state->destnames);
      state->destnames = NULL;
    }

    if (state->nav_site_name) {
      PORT_Free(state->nav_site_name);
      state->nav_site_name = NULL;
    }

    if (state->nav_creator_name) {
      PORT_Free(state->nav_creator_name);
      state->nav_creator_name = NULL;
    }

    if (state->server_cert) {
      CERT_DestroyCertificate(state->server_cert);
      state->server_cert = NULL;
    }

    if (state->certs_to_fetch) {
	PORT_Free(state->certs_to_fetch);
	state->certs_to_fetch = NULL;
    }

    switch (state->infocontext) {

#ifndef MOZ_LITE
      case COMPOSE: {

	char *all_mailboxes = 0, *mailboxes = 0, *mailbox_list = 0;
	const char *mailbox = 0, *newsbox = 0;
	int count = 0;
	XP_Bool valid_list=PR_TRUE;

        MimeGetCompositionSecurityInfo((MWContext *)proto_win, &sender, &mail_recipients, 
				       &news_recipients, &encryptthis, &signthis, &delivery_error);

	state->oldencryptthis = state->encryptthis = encryptthis;
	state->oldsignthis = state->signthis = signthis;

	/* Initial value of javascript strings set here, closing paren added
	 * after the recipient check loop.  The closing paren will replace
	 * the trailing comman to make javascript happy
	 */

	PORT_Assert(!state->encrypterrors);
	PORT_Assert(!state->destnames);
        state->encrypterrors = PORT_Strdup("sa_init_array(");
	state->destnames = PORT_Strdup("sa_init_array(");

	/* We want either an encrypterrors list with at most one ENC_NOERR entry
	 * or at least one non-ENC_NOERR entry to keep the list short.
	 */

	all_mailboxes = MSG_ExtractRFC822AddressMailboxes(mail_recipients);
	mailboxes = MSG_RemoveDuplicateAddresses(all_mailboxes, 0);
	if (all_mailboxes) PORT_Free(all_mailboxes);
	
	if (mailboxes)
	  count = MSG_ParseRFC822Addresses (mailboxes, 0, &mailbox_list);

	/* An empty mail_recipient list is noteworthy */
	valid_list = (XP_Bool)count;

	errorflag=(valid_list ? ENC_NOERR : ENC_NORECIPIENT);

	if (mailboxes) PORT_Free(mailboxes);

	mailbox = mailbox_list;
	for (; count > 0; count--)
	  {
	    errorflag=ENC_NOERR;
	    receivercert = CERT_FindCertByEmailAddr(handle, (char *)mailbox);

	    if (receivercert == NULL) {
	      errorflag= ENC_CERTMISSING;

	    } else if (CERT_VerifyCertNow(CERT_GetDefaultCertDB(),
					  receivercert,
					  TRUE,
					  certUsageEmailRecipient,
					  (MWContext *)proto_win)
		       == SECFailure) {
	      int xperr = XP_GetError();
	      if (xperr == SEC_ERROR_UNTRUSTED_CERT)
		errorflag = ENC_CERTUNTRUSTED;
	      else if (xperr == SEC_ERROR_UNTRUSTED_ISSUER)
		errorflag = ENC_ISSUERUNTRUSTED;
	      else if (xperr == SEC_ERROR_UNKNOWN_ISSUER)
		errorflag = ENC_ISSUERUNKNOWN;
	      else if (xperr == SEC_ERROR_EXPIRED_CERTIFICATE ||
		       xperr == SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE ||
		       xperr == SEC_ERROR_CRL_EXPIRED ||
		       xperr == SEC_ERROR_KRL_EXPIRED)
		errorflag = ENC_CERTEXPIRED;
	      else if (xperr == SEC_ERROR_REVOKED_KEY ||
		       xperr == SEC_ERROR_REVOKED_CERTIFICATE)
		errorflag = ENC_CERTREVOKED;
	      else if (xperr == SEC_ERROR_CERT_NOT_VALID ||
		       xperr == SEC_ERROR_BAD_SIGNATURE ||
		       xperr == SEC_ERROR_CA_CERT_INVALID ||
		       xperr == SEC_ERROR_BAD_DER ||
		       xperr == SEC_ERROR_CRL_BAD_SIGNATURE ||
		       xperr == SEC_ERROR_KRL_BAD_SIGNATURE ||
		       xperr == SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID ||
		       xperr == SEC_ERROR_NO_KRL)
		errorflag = ENC_CERTINVALID;
	      else
		errorflag = ENC_CERT_UNKNOWN_ERROR;

	    } else { /* CERT_VerifyCert() passed */
	      errorflag = ENC_NOERR;
	    }
	    if (errorflag != ENC_NOERR) {
	      state->encrypterrors = sa_push_js_int(state->encrypterrors,
						    NULL, errorflag, ",");
	      state->destnames = sa_push_js_string(state->destnames,
						   NULL, mailbox, ",");
	      state->certs_to_fetch = PR_sprintf_append(state->certs_to_fetch,
							"%s,", mailbox);
	      errorcount++;
	    }
	    if (receivercert) CERT_DestroyCertificate(receivercert);
	    mailbox += PORT_Strlen(mailbox) + 1;
	  }
	if(mailbox_list) PORT_Free(mailbox_list);

	newsbox = (news_recipients ? PORT_Strtok(news_recipients,",") : 0);
	while(newsbox) {
	  state->encrypterrors = sa_push_js_int(state->encrypterrors,
						NULL, ENC_NEWSGROUP, ",");
	  state->destnames = sa_push_js_string(state->destnames,
					       NULL, newsbox, ",");
	  errorcount++;
	  newsbox = PORT_Strtok(NULL,",");
	}

	if (errorcount == 0) {
	  state->encrypterrors = sa_push_js_int(state->encrypterrors,
						NULL,
						(valid_list
						 ? ENC_NOERR
						 : ENC_NORECIPIENT),
						",");
	  state->destnames = PR_sprintf_append(state->destnames, "\"\",");
	}

	/* Replace trailing commas with closing paren */
	state->destnames[PORT_Strlen(state->destnames) - 1] = ')';
	state->encrypterrors[PORT_Strlen(state->encrypterrors) - 1] = ')';


	/* XXX Is simply checking for whether we have one sufficient ? */

	if (state->mymailcerts)
	  state->signerror = SIG_NOERR;
	else
	  state->signerror = SIG_CERTMISSING;

	/* XXX On WinNT, passing 'sender' in signername causes a javascript bug,
	 * along the lines of "var sa_signer_name=""common name<email@domain.top>"""
	 * which causes parse errors in JS, halting the whole dialog.
	 */
	break;
      }
      case SNEWS_MESSAGE:
      case NEWS_MESSAGE:
      case MAIL_MESSAGE: {
	int sign_error = 0, encrypt_error = 0;
	char *commonname = 0;
	char *addr = 0;
	char *sender_addr = 0;
	XP_Bool news_p = (state->infocontext == NEWS_MESSAGE ||
			  state->infocontext == SNEWS_MESSAGE);

	/* Look up the security status of the message.  Note that decode_error
	   may be set even if we don't have any content-infos -- that will
	   happen if the message is encrypted but totally mangled.
	 */
	MimeGetSecurityInfo((MWContext *)proto_win, url->address,
			    &encrypted_p7contentinfo,
			    &signed_p7contentinfo,
			    &sender_addr,
			    &decode_error, &verify_error);

	state->encrypted_p7contentinfo = encrypted_p7contentinfo;
	state->signed_p7contentinfo = signed_p7contentinfo;
	
	state->destnames = PORT_Strdup("sa_init_array(\"\")");
	state->encrypterrors = PORT_Strdup("sa_init_array(");

	PORT_Assert(!state->senderaddr);
	state->senderaddr = sender_addr;
	sender_addr = 0;


	if (news_p)
	  {
	    /* News messages are never encrypted.  This is error protection in
	       case something went wonky in MimeGetSecurityInfo(). */
	    XP_ASSERT(!encrypted_p7contentinfo);
	    if (encrypted_p7contentinfo && !signed_p7contentinfo)
	      {
		signed_p7contentinfo = encrypted_p7contentinfo;
		encrypted_p7contentinfo = 0;
	      }
	  }

	if (!encrypted_p7contentinfo &&
	    !signed_p7contentinfo &&
	    verify_error &&
	    !decode_error)
	  {
	    /* Something's gone all bass-ackwards on us.
	       This must be a decrypt-error (like "can't do that algorithm")
	       but it came to us in `verify_error' instead.
	     */
	    decode_error = verify_error;
	    verify_error = 0;
	  }

	/* Decide whether the message was encrypted/signed (whether valid
	   or not.)
	 */
	encrypted_b = (decode_error ||
		       (encrypted_p7contentinfo &&
			SEC_PKCS7ContentIsEncrypted(encrypted_p7contentinfo))||
		       (signed_p7contentinfo &&
			SEC_PKCS7ContentIsEncrypted(signed_p7contentinfo)));
	signed_b = (verify_error ||
		    (encrypted_p7contentinfo &&
		     SEC_PKCS7ContentIsSigned(encrypted_p7contentinfo))||
		    (signed_p7contentinfo &&
		     SEC_PKCS7ContentIsSigned(signed_p7contentinfo)));

	/* Pull the names/addresses out of the certs, if possible.
	 */
	if (signed_p7contentinfo &&
	    SEC_PKCS7ContentIsSigned(signed_p7contentinfo))
	  {
	    commonname =
	      SEC_PKCS7GetSignerCommonName(signed_p7contentinfo);
	    addr = SEC_PKCS7GetSignerEmailAddress(signed_p7contentinfo);
	  }
	if (encrypted_p7contentinfo &&
	    SEC_PKCS7ContentIsSigned(encrypted_p7contentinfo))
	  {
	    if (!commonname)
	      commonname =
		SEC_PKCS7GetSignerCommonName(encrypted_p7contentinfo);
	    if (!addr)
	      addr =
		SEC_PKCS7GetSignerEmailAddress(encrypted_p7contentinfo);
	  }

	PORT_Assert(!state->signername);
	PORT_Assert(!state->signeraddr);
	state->signername = PORT_Strdup((commonname && *commonname)
					? commonname
					: (addr ? addr : ""));
	state->signeraddr = PORT_Strdup(addr ? addr : "");
	if (commonname) PORT_Free(commonname);
	if (addr) PORT_Free(addr);
	commonname = 0;
	addr = 0;


	/* Describe the encryption algorithm used.
	 */
	if (encrypted_b)
	  {
	    if (encrypted_p7contentinfo &&
		SEC_PKCS7ContentIsEncrypted(encrypted_p7contentinfo))
	      state->algorithmdesc =
		sa_get_algorithm_string(encrypted_p7contentinfo);

	    if (!state->algorithmdesc &&
		signed_p7contentinfo &&
		SEC_PKCS7ContentIsEncrypted(signed_p7contentinfo))
	      state->algorithmdesc =
		sa_get_algorithm_string(signed_p7contentinfo);
	  }


	/* Decide which of the hokey signature-error-codes to pass to the
	   JavaScript code (in `var sa_sign_error').
	 */
	if (!signed_b)
	  sign_error = SIG_NOTSIGNED;
	else if (verify_error == 0)
	  state->signerror = SIG_NOERR;
	else if (verify_error == SEC_ERROR_PKCS7_BAD_SIGNATURE)
	  sign_error = SIG_TAMPERED;
	else if (verify_error == SEC_ERROR_CERT_ADDR_MISMATCH)
	  sign_error = SIG_ADDR_MISMATCH;
	else
	  {
	    sign_error = SIG_INVALID;
	    if (verify_error != -1)
	      state->signerrorstr = PORT_Strdup(XP_GetString(verify_error));
	  }

	/* Decide which of the hokey encryption-error-codes to pass to the
	   JavaScript code (in `var sa_encrypt_errors').
	 */
	if (news_p)
	  {
	    /* If it's a news message, set the encryption bits based on the
	       SSL connection, rather than the message. */
	    int seclevel = XP_GetSecurityStatus((MWContext *)proto_win);
	    if (state->infocontext != SNEWS_MESSAGE ||
		seclevel == SSL_SECURITY_STATUS_OFF ||
		seclevel == SSL_SECURITY_STATUS_NOOPT)
	      encrypt_error = ENC_NOTSECURE;
	    else
	      encrypt_error = ENC_SECURE;
	  }
	else if (!encrypted_b)
	  encrypt_error = ENC_NOTENCRYPTED;
	else if (decode_error == 0)
	  encrypt_error = ENC_NOERR;
	else if (decode_error == SEC_ERROR_NOT_A_RECIPIENT)
	  encrypt_error = ENC_FORANOTHER;
	else
	  {
	    encrypt_error = ENC_UNKNOWN_ERROR;
	    if (decode_error != -1)
	      state->encrypterrorstr = PORT_Strdup(XP_GetString(decode_error));
	  }

	state->signerror = sign_error;

	state->encrypterrors = sa_push_js_int(state->encrypterrors,
					      NULL, encrypt_error, ")");

	/* Figure out what signing-time to pass to the JavaScript.
	 */
	if (signed_p7contentinfo &&
	    SEC_PKCS7ContentIsSigned(signed_p7contentinfo))
	  {
	    SECItem *item = SEC_PKCS7GetSigningTime(signed_p7contentinfo);
	    state->signtime = (item ? DER_UTCTimeToAscii(item) : 0);
	  }

	break;
      }

#endif /* MOZ_LITE */
      case NAVIGATOR: {
	  /* We still don't fully deal with all mixed cases.  There's also no
	   * way to tell if java or js created the window, so we can't fill
	   * in nav_java, nav_js, or nav_creator_name.
	   */
	  extern int NET_FindURLInCache(URL_Struct * URL_s, MWContext *ctxt);
	  Chrome chrome;

	  /* We have to call this to fill in the URL struct */
	  NET_FindURLInCache(url, (MWContext *)proto_win);
	  state->ssl_encrypted = url->security_on;
	  if (state->ssl_encrypted) {
	      state->server_cert = CERT_DupCertificate(url->certificate);
	      if (XP_GetSecurityStatus((MWContext *)proto_win) ==
		  SSL_SECURITY_STATUS_OFF) {
		  state->ssl_mixed = PR_TRUE;
	      }
	  }

#ifndef AKBAR
	  FE_QueryChrome((MWContext *)proto_win, &chrome);
	  state->nav_missing_menubar = !chrome.show_menu;
	  state->nav_missing_toolbar = !chrome.show_button_bar;
	  state->nav_missing_personalbar = !chrome.show_directory_buttons;
	  state->nav_missing_location = !chrome.show_url_bar;
	  state->nav_missing_status = !chrome.show_bottom_status_bar;
	  state->nav_chrome_missing = state->nav_missing_menubar ||
	                              state->nav_missing_toolbar ||
	                              state->nav_missing_personalbar ||
	                              state->nav_missing_location ||
	                              state->nav_missing_status;
#endif /* AKBAR */
	  state->nav_site_name = NET_ParseURL(url->address, GET_HOST_PART);
	  break;
      }

      case MAIL_THREADS:
      case NEWS_THREADS:
	break;
      case NOINFO:
      default:{
	PORT_Assert(!state->encrypterrors);
	state->encrypterrors = PORT_Strdup("sa_init_array(0)");
	state->signerror = SIG_NOERR;
	PORT_Assert(!state->destnames);
	state->destnames = PORT_Strdup("sa_init_array(\"\")");
	break;
      }
    }

    if (sender) PORT_Free(sender);
    if (mail_recipients) PORT_Free(mail_recipients);
    if (news_recipients) PORT_Free(news_recipients);

    return(SECSuccess);
}

SECStatus
getCryptoModuleStrings(SecurityAdvisorState *state)
{
    /* free any strings hanging around */
    SECMODModuleList *mlp;
    SECMODModuleList *modules = SECMOD_GetDefaultModuleList();
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();

    if ( state->crmodules ) {
	PORT_Free(state->crmodules);
	state->crmodules = NULL;
    }
    state->crmodules = PORT_Strdup("sa_init_array(");
    SECMOD_GetReadLock(moduleLock);
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	state->crmodules = sa_push_js_string(state->crmodules,
					     NULL,
					     mlp->module->commonName,
					     (mlp->next ? "," : ""));
    }
    SECMOD_ReleaseReadLock(moduleLock);
    state->crmodules = PR_sprintf_append(state->crmodules, ")");
    
    return(SECSuccess);
}

SECStatus
getJavaPrincipalStrings(SecurityAdvisorState *state)
{
    char *java_principals = NULL;
    
    /* free any strings hanging around */
    if ( state->principals ) {
	PORT_Free(state->principals);
	state->principals = NULL;
    }

#if defined(JAVA) && !defined(AKBAR)
    java_principals = (char *)java_netscape_security_getPrincipals();
# endif /* JAVA && !AKBAR */

    if (java_principals != NULL) {
	/* java_principals string is already escaped in java. For example
	 * java_principals is "\\\"signtest\\\""
	 */
	state->principals = 
		PR_sprintf_append(state->principals, "sa_init_array(%s", 
				  java_principals);
	state->principals = PR_sprintf_append(state->principals, ")");
    } else {
        state->principals = PORT_Strdup("sa_init_array()");
    }

    return(SECSuccess);
}


SECStatus
setInfoContext(SecurityAdvisorState *state, void *context, URL_Struct *url)
{
    if (((MWContext *)context)->type == MWContextMessageComposition) {
	state->infocontext = COMPOSE;
    } else if (!url || !url->address || !*url->address) {
	state->infocontext = NOINFO;
    } else {
	/* Guess the content-type based on the URL.  (Kinda bogus...) */
	switch (NET_URL_Type(url->address)) {
	  case MAILBOX_TYPE_URL:
#ifdef IMAP_TYPE_URL
	  case IMAP_TYPE_URL:
#endif
	    state->infocontext = MAIL_MESSAGE;
	    break;

	  case NEWS_TYPE_URL:
	    if (url->address[0] == 's' || url->address[0] == 'S')
		state->infocontext = SNEWS_MESSAGE;
	    else
		state->infocontext = NEWS_MESSAGE;
	    break;

	  default:
	    state->infocontext = NAVIGATOR;
	    break;
	}

#ifndef MOZ_LITE
	/* If we didn't already guess that it's a mail or news message, then
	 * see if we can guess any better than that: if libmime gives us a
	 * cinfo, then it's definitely a message, regardless of what kind
	 * of URL it came from.
	 */
	if (state->infocontext == NAVIGATOR) {
	    SEC_PKCS7ContentInfo *encrypted_p7contentinfo = 0;
	    SEC_PKCS7ContentInfo *signed_p7contentinfo = 0;
	    int32 decode_error = 0, verify_error = 0;

	    MimeGetSecurityInfo((MWContext *)context,
				url->address,
				&encrypted_p7contentinfo,
				&signed_p7contentinfo,
				NULL, /* &sender_addr */
				&decode_error, &verify_error);

	    if (encrypted_p7contentinfo || signed_p7contentinfo ||
		decode_error || verify_error) {
		state->infocontext = MAIL_MESSAGE;

		if (encrypted_p7contentinfo)
		    SEC_PKCS7DestroyContentInfo(encrypted_p7contentinfo);
		if (signed_p7contentinfo)
		    SEC_PKCS7DestroyContentInfo(signed_p7contentinfo);
	    }
	}
#endif /* MOZ_LITE */
    }

#if !defined(AKBAR) && !defined(MOZ_LITE)
    if ((state->infocontext == NAVIGATOR) ||
	(state->infocontext == NOINFO)) {
	MSG_Pane *pane;
	MSG_FolderInfo *folder;
	MSG_NewsHost *host;

	pane = MSG_FindPaneOfContext((MWContext *)context, MSG_THREADPANE);
	if (pane != NULL) {
	    folder = MSG_GetThreadFolder(pane);
	    if (folder != NULL) {
		host = MSG_GetNewsHostForFolder(folder);
		if (host == NULL) {
		    state->infocontext = MAIL_THREADS;
		} else {
		    state->infocontext = NEWS_THREADS;
		    if (MSG_IsNewsHostSecure(host) == PR_TRUE) {
			state->ssl_encrypted = PR_TRUE;
		    } else {
			state->ssl_encrypted = PR_FALSE;
		    }
		}
	    }
	}
    }
#endif /* !AKBAR && !MOZ_LITE */

    return(SECSuccess);
}


static void
freeSecurityAdvisorState(SecurityAdvisorState *state)
{
    if ( state ) {
	if ( state->mycerts ) {
	    PORT_Free(state->mycerts);
	}
	if ( state->mysslcerts ) {
	    PORT_Free(state->mysslcerts);
	}
	if ( state->mymailcerts ) {
	    PORT_Free(state->mymailcerts);
	}
	if ( state->peoplescerts ) {
	    PORT_Free(state->peoplescerts);
	}
	if ( state->sitecerts ) {
	    PORT_Free(state->sitecerts);
	}
	if ( state->cacerts ) {
	    PORT_Free(state->cacerts);
	}
	if ( state->sslcertname ) {
	    PORT_Free(state->sslcertname);
	}
	if ( state->mailcertname ) {
	    PORT_Free(state->mailcertname);
	}
	if ( state->crmodules ) {
	    PORT_Free(state->crmodules);
	}
	if ( state->principals ) {
	    PORT_Free(state->principals);
	}
	if ( state->encrypterrors ) {
	    PORT_Free(state->encrypterrors);
	}
	if (state->destnames) {
	    PORT_Free(state->destnames);
	}
	if ( state->nav_site_name ) {
	    PORT_Free(state->nav_site_name);
	}
	if ( state->nav_creator_name ) {
	    PORT_Free(state->nav_creator_name);
	}
	if ( state->server_cert ) {
	    CERT_DestroyCertificate(state->server_cert);
	}
	if (state->encrypted_p7contentinfo) {
	  SEC_PKCS7DestroyContentInfo(state->encrypted_p7contentinfo);
	}
	if (state->signed_p7contentinfo) {
	  SEC_PKCS7DestroyContentInfo(state->signed_p7contentinfo);
	}
	if (state->certs_to_fetch) {
	    PORT_Free(state->certs_to_fetch);
	}
	PORT_Free(state);
    }

    return;
}

extern void FE_SecurityOptionsChanged(MWContext *);

int
SECNAV_AskPrefToPK11(int asktype) {
   switch (asktype) {
   case 1:
	return -1;
   case 2:
	return 1;
   }
   return 0;
}

static SECStatus
setSecurityPrefs(SecurityAdvisorState *state)
{
    XP_Bool value;
    int ret;
    char *charvalue;
    PK11SlotInfo *slot;
    
    value = (XP_Bool)state->enablessl2;
    ret = PREF_SetBoolPref("security.enable_ssl2", value);
    SSL_EnableDefault(SSL_ENABLE_SSL2, value);
    
    value = (XP_Bool)state->enablessl3;
    ret = PREF_SetBoolPref("security.enable_ssl3", value);
    SSL_EnableDefault(SSL_ENABLE_SSL3, value);
    
    value = (XP_Bool)state->warnentersecure;
    ret = PREF_SetBoolPref("security.warn_entering_secure", value);
    
    value = (XP_Bool)state->warnleavesecure;
    ret = PREF_SetBoolPref("security.warn_leaving_secure", value);
    
    value = (XP_Bool)state->warnmixed;
    ret = PREF_SetBoolPref("security.warn_viewing_mixed", value);
    
    value = (XP_Bool)state->warnsendclear;
    ret = PREF_SetBoolPref("security.warn_submit_insecure", value);

    charvalue = state->sslcertname;
    ret = PREF_SetCharPref("security.default_personal_cert", charvalue);
    SECNAV_SetDefUserCertList(charvalue);

    charvalue = state->mailcertname;
    ret = PREF_SetCharPref("security.default_mail_cert", charvalue);

#ifdef AKBAR
    /* We don't have PREF_RegisterCallback() in Akbar. */
    SECNAV_MailCertPrefChange("security.default_mail_cert", charvalue);
#endif /* AKBAR */
    
    ret = PREF_SetIntPref("security.ask_for_password",
			  (int32)state->pwasktype);
    ret = PREF_SetIntPref("security.password_lifetime",
			  (int32)state->pwaskinterval);

    slot = PK11_GetInternalKeySlot();
    PK11_SetSlotPWValues(slot,SECNAV_AskPrefToPK11(state->pwasktype),
							state->pwaskinterval);
    PK11_FreeSlot(slot);
    
    value = (XP_Bool)state->encryptalways;
    ret = PREF_SetBoolPref("mail.encrypt_outgoing_mail", value);

    value = (XP_Bool)state->signalways;
    ret = PREF_SetBoolPref("mail.crypto_sign_outgoing_mail", value);

    value = (XP_Bool)state->signnewsalways;
    ret = PREF_SetBoolPref("mail.crypto_sign_outgoing_news", value);

#if 0
    value = (XP_Bool)state->fwdwarning;
    ret = PREF_SetBoolPref("mail.warn_forward_encrypted", value);

    value = (XP_Bool)state->rewarning;
    ret = PREF_SetBoolPref("mail.warn_reply_unencrypted", value);
#endif

    ret = PREF_SavePrefFile();

#ifndef MOZ_LITE
#ifdef AKBAR
    if ((XP_Bool)state->oldencryptthis != (XP_Bool)state->encryptthis)
		  MSG_Command(state->globalMWContext, MSG_SendEncrypted);

    if ((XP_Bool)state->oldsignthis != (XP_Bool)state->signthis)
		  MSG_Command(state->globalMWContext, MSG_SendSigned);

#else /* !AKBAR */

    if(state->oldencryptthis != state->encryptthis) 
      MSG_SetCompBoolHeader(MSG_FindPane(state->globalMWContext, MSG_COMPOSITIONPANE),
			  MSG_ENCRYPTED_BOOL_HEADER_MASK,
			  (XP_Bool)state->encryptthis);
    if(state->oldsignthis != state->signthis)
      MSG_SetCompBoolHeader(MSG_FindPane(state->globalMWContext, MSG_COMPOSITIONPANE),
			  MSG_SIGNED_BOOL_HEADER_MASK,
			  (XP_Bool)state->signthis);

#if defined(XP_MAC) || defined(XP_WIN) || defined(XP_UNIX) /* #### waiting for other FEs to catch up */
    if(state->oldencryptthis != state->encryptthis ||
       state->oldsignthis != state->signthis)
      FE_SecurityOptionsChanged(state->globalMWContext);
#endif

#endif /* !AKBAR */
#endif /* MOZ_LITE */

    return(SECSuccess);
}

static SECStatus
getSecurityPrefs(SecurityAdvisorState *state)
{
    XP_Bool value;
    int ret;
    char *charvalue;
    int32 intvalue;
    
    ret = PREF_GetBoolPref("security.enable_ssl2", &value);
    state->enablessl2 = (PRBool)value;
    
    ret = PREF_GetBoolPref("security.enable_ssl3", &value);
    state->enablessl3 = (PRBool)value;
    
    ret = PREF_GetBoolPref("security.warn_entering_secure", &value);
    state->warnentersecure = (PRBool)value;
    
    ret = PREF_GetBoolPref("security.warn_leaving_secure", &value);
    state->warnleavesecure = (PRBool)value;
    
    ret = PREF_GetBoolPref("security.warn_viewing_mixed", &value);
    state->warnmixed = (PRBool)value;
    
    ret = PREF_GetBoolPref("security.warn_submit_insecure", &value);
    state->warnsendclear = (PRBool)value;
    
    ret = PREF_CopyCharPref("security.default_personal_cert", &charvalue);
    state->sslcertname = charvalue;

    ret = PREF_CopyCharPref("security.default_mail_cert", &charvalue);
#ifdef AKBAR
    /* We don't have PREF_RegisterCallback() in Akbar. */
    SECNAV_MailCertPrefChange("security.default_mail_cert", charvalue);
#endif /* AKBAR */
    state->mailcertname = charvalue;

    ret = PREF_GetIntPref("security.ask_for_password", &intvalue);
    
    state->pwasktype = intvalue;

    state->passwordset = !PK11_NeedPWInit();
    
    ret = PREF_GetIntPref("security.password_lifetime", &intvalue);
    
    state->pwaskinterval = intvalue;

    ret = PREF_GetBoolPref("mail.encrypt_outgoing_mail", &value);

    state->encryptalways = (PRBool)value;

    ret = PREF_GetBoolPref("mail.crypto_sign_outgoing_mail", &value);

    state->signalways = (PRBool)value;

    ret = PREF_GetBoolPref("mail.crypto_sign_outgoing_news", &value);

    state->signnewsalways = (PRBool)value;

#if 0
    ret = PREF_GetBoolPref("mail.warn_forward_encrypted", &value);

    state->fwdwarning = (PRBool)value;

    ret = PREF_GetBoolPref("mail.warn_reply_unencrypted", &value);

    state->rewarning = (PRBool)value;
#endif /* 0 */

    return(SECSuccess);
}

static SecurityAdvisorState *
getSecurityAdvisorState(CERTCertDBHandle *handle,void *proto_win, URL_Struct *url)
{
    SECStatus rv;
    SecurityAdvisorState *state;
    
    state = (SecurityAdvisorState *)PORT_ZAlloc(sizeof(SecurityAdvisorState));
    if ( state == NULL ) {
	goto loser;
    }

    state->handle = handle;
    state->curpage = 1;
    state->url = url;
    state->proto_win = proto_win;
    
    rv = setInfoContext(state, proto_win, url);
    if ( rv != SECSuccess ) {
        goto loser;
    }

    rv = getSecurityPrefs(state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = getCertVarStrings(state,proto_win);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = getCryptoModuleStrings(state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = getJavaPrincipalStrings(state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = getStatusValues(state, proto_win, url);
    if (rv != SECSuccess) {
        goto loser;
    }

    return(state);

loser:
    freeSecurityAdvisorState(state);
    
    return(NULL);
}

static void
collectAdvisorState(SecurityAdvisorState *state, char **argv, int argc)
{
    char *tmpstr;
    
    tmpstr = XP_FindValueInArgs("submit_current_page", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->curpage = PORT_Atoi(tmpstr);
    }
    
    tmpstr = XP_FindValueInArgs("submit_ssl_cert", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
        if (state->sslcertname) PORT_Free(state->sslcertname);
	state->sslcertname = PORT_Strdup(tmpstr);
    }
    
    tmpstr = XP_FindValueInArgs("submit_mail_cert", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
        if (state->mailcertname) PORT_Free(state->mailcertname);
	state->mailcertname = PORT_Strdup(tmpstr);
    }
    
    tmpstr = XP_FindValueInArgs("submit_enter", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->warnentersecure = ( 't' == tmpstr[0] );
    }
    
    tmpstr = XP_FindValueInArgs("submit_leave", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->warnleavesecure = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_mixed", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->warnmixed = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_send", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->warnsendclear = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_ssl2", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->enablessl2 = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_ssl3", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->enablessl3 = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_password_ask_type", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->pwasktype = PORT_Atoi(tmpstr);
    }

    tmpstr = XP_FindValueInArgs("submit_password_ask_interval", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->pwaskinterval = PORT_Atoi(tmpstr);
    }

    tmpstr = XP_FindValueInArgs("submit_encrypt_always", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->encryptalways = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_sign_mail_always", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->signalways = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_sign_news_always", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->signnewsalways = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_encrypt_this", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->encryptthis = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_sign_this", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->signthis = ( 't' == tmpstr[0] );
    }

#if 0
    tmpstr = XP_FindValueInArgs("submit_fwd_warning", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->fwdwarning = ( 't' == tmpstr[0] );
    }

    tmpstr = XP_FindValueInArgs("submit_re_warning", argv, argc);
    XP_ASSERT(tmpstr);
    if ( tmpstr ) {
	state->rewarning = ( 't' == tmpstr[0] );
    }
#endif /* 0 */


    return;
}

typedef struct {
    SecurityAdvisorState *state;
    XPDialogState *dlgstate;
} DeleteCallbackArg;


static void
redrawCallback(void *arg)
{
    SECStatus rv;
    DeleteCallbackArg *cbarg = (DeleteCallbackArg *)arg;
    int ret;
    XPDialogStrings *strings = NULL;

    /* update the certificate strings */
    rv = getCertVarStrings(cbarg->state,cbarg->dlgstate->window);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* update the security module strings */
    rv = getCryptoModuleStrings(cbarg->state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* Update the java principals strings */
    rv = getJavaPrincipalStrings(cbarg->state);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = getStatusValues(cbarg->state, cbarg->state->proto_win,
			 cbarg->state->url);
    if (rv != SECSuccess) {
        goto loser;
    }

    /* make the dialog strings */
    strings = getAdvisorStrings(cbarg->state);

    /* redraw the dialog */
    ret = XP_RedrawRawHTMLDialog(cbarg->dlgstate, strings, 1);
    
loser:
    if ( strings ) {
	XP_FreeDialogStrings(strings);
    }
    
    PORT_Free(arg);

    return;
}

void
SECNAV_AdvisorRedraw(void *arg) {
    redrawCallback(arg);
    return;
}

static PRBool
securityAdvisorHandler(XPDialogState *dlgstate, char **argv, int argc,
		       unsigned int button)
{
    SecurityAdvisorState *state;
    SECStatus rv;
    char *buttonString;
    char *tmpstring;
    char *certname;
    char *moduleName;
    CERTCertificate *cert;
    PRBool sitepage;
    
    state = (SecurityAdvisorState *)dlgstate->arg;

    collectAdvisorState(state, argv, argc);
    
    if ( button == 0 ) {
	tmpstring = XP_FindValueInArgs("submit_current_page", argv, argc);
	XP_ASSERT(tmpstring);
	if ( tmpstring == NULL ) {
	    goto loser;
	}
	state->curpage = PORT_Atoi(tmpstring);
	
	buttonString = XP_FindValueInArgs("button", argv, argc);
	XP_ASSERT(buttonString);
	if ( buttonString != NULL ) {
	    certname = XP_FindValueInArgs("certs", argv, argc);
	    sitepage = PR_FALSE;
	    switch ( state->curpage ) {
	      case 1: /* secinfo_page */
		if ((PORT_Strcasecmp(buttonString,
			     XP_GetString(SA_VIEW_BUTTON_LABEL)) == 0) ||
		    (PORT_Strcasecmp(buttonString,
			     XP_GetString(SA_VIEW_CERT_BUTTON_LABEL)) == 0)) {
		    if (state->infocontext == NAVIGATOR) {
			cert = state->server_cert;
			if (cert) {
			    SECNAV_DisplayCertDialog(dlgstate->window, cert);
			}
		    } else if (state->signed_p7contentinfo ||
			state->encrypted_p7contentinfo) {
			SEC_PKCS7SignerInfo **signerinfos;
			SEC_PKCS7ContentInfo *ci = state->signed_p7contentinfo;
			XP_ASSERT(ci);
			if (!ci) ci = state->encrypted_p7contentinfo;

			/* Finding the signers cert */
			switch(ci->contentTypeTag->offset) {
			  default:
			  case SEC_OID_PKCS7_DATA:
			  case SEC_OID_PKCS7_DIGESTED_DATA:
			  case SEC_OID_PKCS7_ENVELOPED_DATA:
			  case SEC_OID_PKCS7_ENCRYPTED_DATA:
			    /* Could only get here if SEC_PKCS7ContentIsSigned
			     * is broken. */
			    {
				PORT_Assert (0);
				cert=NULL;
			    }
			  break;
			  case SEC_OID_PKCS7_SIGNED_DATA:
			    {
				SEC_PKCS7SignedData *sdp;
				sdp = ci->content.signedData;
				signerinfos = sdp->signerInfos;
				cert = signerinfos[0]->cert;
			    }
			  break;
			  case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
			    {
				SEC_PKCS7SignedAndEnvelopedData *saedp;
				saedp = ci->content.signedAndEnvelopedData;
				signerinfos = saedp->signerInfos;
				cert = signerinfos[0]->cert;
			    }
			  break;
			} /* finding the signer cert */
			if (cert) {
			    SECNAV_DisplayCertDialog(dlgstate->window,cert);
			}
		    }
		} else if ( PORT_Strcasecmp(buttonString,
				  XP_GetString(SA_GET_CERT_BUTTON_LABEL)) == 0){
		    SECNAV_NewUserCertificate((MWContext *)dlgstate->proto_win);
		} else if ( PORT_Strcasecmp(buttonString,
				    XP_GetString(SA_PAGE_INFO_LABEL)) == 0){
		    FE_GetURL((MWContext *)dlgstate->proto_win,
			      NET_CreateURLStruct("about:document",
						  NET_DONT_RELOAD));
#ifndef MOZ_LITE
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_GET_CERTS_BUTTON_LABEL)) == 0){
#ifndef AKBAR
		    DeleteCallbackArg *cbarg;
			
		    cbarg = (DeleteCallbackArg *)
			PORT_Alloc(sizeof(DeleteCallbackArg));
		    if ( cbarg ) {
			cbarg->dlgstate = dlgstate;
			cbarg->state = state;

			SECNAV_SearchLDAPDialog(dlgstate->window,
						SECNAV_MakeDeleteCertCB(redrawCallback, (void *)cbarg),
						state->certs_to_fetch);
		    }
#endif /* !AKBAR */
#endif /* MOZ_LITE */
		}
		break; /* Info panel */
	      case 7: /* your_certs_page */
		if ( PORT_Strcasecmp(buttonString,
				     XP_GetString(SA_VIEW_BUTTON_LABEL)) == 0 ) {
		    /* bring up the viewing window */
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VIEW_STRING,
					       dlgstate->window,
					       SECNAV_ViewUserCertificate,
					       NULL);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_VERIFY_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VERIFY_STRING,
					       dlgstate->window,
					       SECNAV_VerifyCertificate,
					       (void *)certUsageEmailRecipient);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_DELETE_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			DeleteCallbackArg *cbarg;
			
			cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
			if ( cbarg ) {
			    cbarg->dlgstate = dlgstate;
			    cbarg->state = state;
			    rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
						   certname,
						   XP_CERT_SELECT_DEL_STRING,
						   dlgstate->window,
						   SECNAV_DeleteUserCertificate,
						   SECNAV_MakeDeleteCertCB(redrawCallback, (void *)cbarg));
			}
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_EXPORT_BUTTON_LABEL)) == 0){
		    if(certname)
		    {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VIEW_STRING,
					       dlgstate->window,
					       SECNAV_ExportPKCS12Object,
					       (void *)certname);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_GET_CERT_BUTTON_LABEL)) == 0){
		    SECNAV_NewUserCertificate((MWContext *)dlgstate->proto_win);
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_IMPORT_BUTTON_LABEL)) == 0){
		    SECNAVPKCS12ImportArg *p12imp_arg;

		    p12imp_arg = (SECNAVPKCS12ImportArg *)PORT_ZAlloc(
		    	sizeof(SECNAVPKCS12ImportArg));
		    if(p12imp_arg) {
			p12imp_arg->redrawCallback = (void *)redrawCallback;
			p12imp_arg->sa_state = state;
			p12imp_arg->dlg_state = dlgstate;
			p12imp_arg->proto_win = dlgstate->window;
			p12imp_arg->slot_ptr = PK11_GetInternalKeySlot();

			if(PK11_NeedLogin((PK11SlotInfo *)p12imp_arg->slot_ptr) &&
				PK11_NeedUserInit((PK11SlotInfo *)p12imp_arg->slot_ptr)) {
			    /* bring up the set password window */
			    SECNAV_MakePWSetupDialog(dlgstate->window, NULL,
						     SECNAV_ImportPKCS12Object, 
						     (PK11SlotInfo *)p12imp_arg->slot_ptr,
						     p12imp_arg);
			    break;
		        } else {
			    SECNAV_ImportPKCS12Object(p12imp_arg, PR_FALSE);
			}
		    } else {
			FE_Alert(dlgstate->window, 
			   XP_GetString(SEC_ERROR_NO_MEMORY));
		    }
		}
		break;
	      case 8: /* people_certs_page */
		if ( PORT_Strcasecmp(buttonString,
				     XP_GetString(SA_VIEW_BUTTON_LABEL)) == 0 ) {
		    /* bring up the viewing window */
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VIEW_STRING,
					       dlgstate->window,
					       SECNAV_ViewUserCertificate,
					       NULL);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_VERIFY_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VERIFY_STRING,
					       dlgstate->window,
					       SECNAV_VerifyCertificate,
					       (void *)certUsageEmailRecipient);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_DELETE_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			DeleteCallbackArg *cbarg;
			
			cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
			if ( cbarg ) {
			    cbarg->dlgstate = dlgstate;
			    cbarg->state = state;

			    rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
						   certname,
						   XP_CERT_SELECT_DEL_STRING,
						   dlgstate->window,
						   SECNAV_DeleteEmailCertificate,
						   SECNAV_MakeDeleteCertCB(redrawCallback, (void *)cbarg));
			}
		    }
#ifndef MOZ_LITE
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_SEARCH_DIR_BUTTON_LABEL)) == 0){
#ifndef AKBAR
		    DeleteCallbackArg *cbarg;
			
		    cbarg = (DeleteCallbackArg *)
			PORT_Alloc(sizeof(DeleteCallbackArg));
		    if ( cbarg ) {
			cbarg->dlgstate = dlgstate;
			cbarg->state = state;

			SECNAV_SearchLDAPDialog(dlgstate->window,
						SECNAV_MakeDeleteCertCB(redrawCallback, (void *)cbarg), NULL);
		    }
#endif /* !AKBAR */
#endif /* MOZ_LITE */
		}
	    
		break;
		
	      case 9: /* site_certs_page */
		sitepage = PR_TRUE;
	      case 10: /* signers_certs_page */
		if ( PORT_Strcasecmp(buttonString,
				     XP_GetString(SA_EDIT_BUTTON_LABEL)) == 0 ) {
		    /* bring up the edit window */
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_EDIT_STRING,
					       dlgstate->window,
					       SECNAV_EditSiteCertificate,
					       NULL);
			
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_VERIFY_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       certname,
					       XP_CERT_SELECT_VERIFY_STRING,
					       dlgstate->window,
					       SECNAV_VerifyCertificate,
					       sitepage ?
					         (void *)certUsageSSLServer :
					         (void *)certUsageVerifyCA);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_DELETE_BUTTON_LABEL)) == 0){
		    if ( certname ) {
			DeleteCallbackArg *cbarg;
			
			cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
			if ( cbarg ) {
			    cbarg->dlgstate = dlgstate;
			    cbarg->state = state;

			    rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
						   certname,
						   XP_CERT_SELECT_DEL_STRING,
						   dlgstate->window,
						   SECNAV_DeleteSiteCertificate,
						   SECNAV_MakeDeleteCertCB(redrawCallback, (void *)cbarg));
			
			}
			
		    }
		}
		break;
	      case 3: /* navigator_page */
		if ( PORT_Strcasecmp(buttonString,
				     XP_GetString(SA_SSL2_CONFIG_LABEL)) == 0 ) {
		    SECNAV_MakeCiphersDialog(dlgstate->window,
					     SSL_LIBRARY_VERSION_2);
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_SSL3_CONFIG_LABEL)) == 0 ) {
		    SECNAV_MakeCiphersDialog(dlgstate->window,
					     SSL_LIBRARY_VERSION_3_0);
		}
		break;

#ifndef MOZ_LITE
	      case 4: /* messenger_page */
		if (!PORT_Strcasecmp(buttonString, 
				     XP_GetString(SA_SMIME_CONFIG_LABEL))) {
		    SECNAV_MakeCiphersDialog(dlgstate->window,
					     SMIME_LIBRARY_VERSION_1_0);
		} else if (!PORT_Strcasecmp(buttonString, 
					    XP_GetString(SA_SEND_CERT_BUTTON_LABEL))) {
#ifndef AKBAR
		    SECNAV_PublishSMimeCertDialog(dlgstate->window);
#endif /* !AKBAR */
		}
		
	        break;
#endif /* MOZ_LITE */

	      case 5: /* applet_certs_page */
		if ( PORT_Strcasecmp(buttonString, 
				     XP_GetString(SA_VIEW_CERT_BUTTON_LABEL)) == 0 ) {
		    /* bring up the viewing window */
		    if ( certname ) {
			SECStatus rv = SECNAV_IsJavaPrinCert(dlgstate->window, 
							     certname);
			if (rv == SECSuccess) {
			    rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
						   certname,
						   XP_CERT_SELECT_VIEW_STRING,
						   dlgstate->window,
						   SECNAV_ViewUserCertificate,
						   NULL);
			}
		    }
		} else if ( PORT_Strcasecmp(buttonString, 
					    XP_GetString(SA_EDIT_PRIVS_BUTTON_LABEL)) == 0) {
		    /* bring up the edit window */
		    if ( certname ) {
			SECNAV_EditJavaPrivileges(dlgstate->window, certname);
		    }
		} else if ( PORT_Strcasecmp(buttonString,
					    XP_GetString(SA_REMOVE_BUTTON_LABEL)) == 0) {
		    if ( certname ) {
			DeleteCallbackArg *cbarg;
			
			cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
			if ( cbarg ) {
			    cbarg->dlgstate = dlgstate;
			    cbarg->state = state;
			    SECNAV_DeleteJavaPrincipal(dlgstate->window,
						       certname,
						       redrawCallback,
						       (void *)cbarg);
			}
		    }
		}
		break;
	      case 2: /* passwords_page */
		if (( PORT_Strcasecmp(buttonString, XP_GetString(SA_SET_PASSWORD_LABEL)) == 0 ) ||
		   ( PORT_Strcasecmp(buttonString, XP_GetString(SA_CHANGE_PASSWORD_LABEL)) == 0)) {
		    if (PK11_NeedPWInit()) {
			/* bring up the set password window */
			SECNAV_MakePWSetupDialog(dlgstate->window, NULL, NULL,
						 PK11_GetInternalKeySlot(),
						 NULL);
		    } else {
		    	/* bring up the change password window */
			SECNAV_MakePWChangeDialog(dlgstate->window,
						  PK11_GetInternalKeySlot());
		    }
		}
		break;
	      case 11: /* modules_page */
		moduleName = XP_FindValueInArgs("sa_cr_modules", argv, argc);
		if (PORT_Strcasecmp(buttonString, XP_GetString(SA_VIEW_EDIT_BUTTON_LABEL)) == 0 ) {
		    SECNAV_EditModule(dlgstate->window,moduleName);
		} else if (PORT_Strcasecmp(buttonString, XP_GetString(SA_ADD_BUTTON_LABEL)) == 0 ) {
		    DeleteCallbackArg *cbarg;
			
		    cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
		     if ( cbarg ) {
		    	cbarg->dlgstate = dlgstate;
		    	cbarg->state = state;
		    	SECNAV_NewSecurityModule(dlgstate->window,
							redrawCallback,
							(void *)cbarg);
		    }
		} else if (PORT_Strcasecmp(buttonString,
					   XP_GetString(SA_DELETE_BUTTON_LABEL)) == 0 ) {
		    if ( moduleName ) {
			DeleteCallbackArg *cbarg;
			
			cbarg = (DeleteCallbackArg *)
			    PORT_Alloc(sizeof(DeleteCallbackArg));
			if ( cbarg ) {
			    cbarg->dlgstate = dlgstate;
			    cbarg->state = state;
			    SECNAV_DeleteSecurityModule(dlgstate->window,
							 moduleName,
							 redrawCallback,
							 (void *)cbarg);
			}
			
		    }
		} else if (PORT_Strcasecmp(buttonString,
					   XP_GetString(SA_LOGOUT_ALL_BUTTON_LABEL)) == 0 ) {
		    SECNAV_LogoutAllSecurityModules(dlgstate->window);
		}
		break;
	    }
	}
	
	return(PR_TRUE);
    }
    /* OK, Cancel, or Help button */
    switch ( button ) {
      case XP_DIALOG_OK_BUTTON:
	rv = setSecurityPrefs(state);
	break;
      case XP_DIALOG_CANCEL_BUTTON:
	break;
    }
    goto done;
    
loser:
    freeSecurityAdvisorState(state);
    return(PR_TRUE);
    
done:
    
    freeSecurityAdvisorState(state);
    return(PR_FALSE);
}


static XPDialogInfo advisorDialog = {
    0,
    securityAdvisorHandler,
#ifdef AKBAR
    640, 480
#else  /* !AKBAR */
    0, 0
#endif /* !AKBAR */
};

void
SECNAV_SecurityAdvisor(void *proto_win, URL_Struct *url)
{
    XPDialogStrings *strings;
    SecurityAdvisorState *state;
#ifndef AKBAR
    int32 width, height;
#endif /* !AKBAR */
    int status;
    
    status = init_sa_strings();
    if (status < 0) return; /* #### */


    state = getSecurityAdvisorState(CERT_GetDefaultCertDB(),proto_win, url);

    if ( state == NULL ) {
	return;
    }


#if defined(DEBUG_dhugo)
    if (url) {
      XP_Trace("Advisor passed url: %s",url->address);
    } else {
      XP_Trace("Advisor passed url: NULL");
    }
#endif

    state->globalMWContext = (MWContext *)proto_win;

    /* generate the advisor strings */
    strings = getAdvisorStrings(state);

#ifndef AKBAR
    /* try to make the advisor a reasonable size if we have a big screen */
    if ( advisorDialog.width == 0 ) {
	FE_GetScreenSize((MWContext *)proto_win, &width, &height);
    
	width = ( ( width * 3 ) >> 2 );
	height = ( ( height * 3 ) >> 2 );
	if ( width < 640 ) {
	    width = 640;
	} else if ( width > 800 ) {
	    width = 800;
	}

	if ( height < 400 ) {
	    height = 400;
	} else if ( height > 600 ) {
	    height = 600;
	}
	
	advisorDialog.width = width;
	advisorDialog.height = height;
    }
#endif /* !AKBAR */
    
    /* put up the dialog */
    XP_MakeRawHTMLDialog(proto_win, &advisorDialog,
			 XP_SECURITY_ADVISOR_TITLE_STRING,
			 strings, 1, (void *)state);
    
    XP_FreeDialogStrings(strings);
    return;
}

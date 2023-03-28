/* -*- Mode:C; tab-width: 8 -*-
   dialogs.c --- Preference dialogs.
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Spence Murray <spence@netscape.com>, 30-Sep-95.
 */

#define PREFS_NET
#define PREFS_SOCKS
#define PREFS_DISK_CACHE
#define PREFS_VERIFY
#define PREFS_CLEAR_CACHE_BUTTONS
#define PREFS_SIG
#define PREFS_NEWS_MAX
#define PREFS_8BIT
/* #define PREFS_EMPTY_TRASH */
/* #define PREFS_QUEUED_DELIVERY */

#include "mozilla.h"
#include "xlate.h"
#include "xfe.h"
#include "felocale.h"
#include "nslocks.h"

#ifdef DUPLICATE_SEC_PREFS
#include "sslproto.h"
#include "secnav.h"
#include "cert.h"
#include "mozjava.h"
#endif /* DUPLICATE_SEC_PREFS */


#include "prnetdb.h"
#include "e_kit.h"
#include "pref_helpers.h"

#ifdef MOCHA
#include "libmocha.h"
#endif /* MOCHA */

#include <X11/IntrinsicP.h>     /* for w->core.height */

#include <X11/Shell.h>
#include <Xm/Label.h>
#include <Xm/CascadeBG.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>

#ifdef REQUEST_LANGUAGE
#include <Xm/ArrowBG.h>
#endif

#ifndef _STDC_
#define _STDC_ 1
#define HACKED_STDC 1
#endif

#include <XmL/Folder.h>

#ifdef HACKED_STDC
#undef HACKED_STDC
#undef _STDC_
#endif

#include "ssl.h"  /* security stuff from include dir */
#include "xp_sec.h"

#include "msgcom.h"

#include "il.h"                 /* Image library definitions */


/* For sys_errlist and sys_nerr */
#include <sys/errno.h>
/*extern char *sys_errlist[];*/
extern int sys_nerr;


#include <nspr/prprf.h>
#include <libi18n.h>
#include "fonts.h"


/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_FILE_DOES_NOT_EXIST;
extern int XFE_FTP_PROXY_HOST;
extern int XFE_GLOBAL_MAILCAP_FILE;
extern int XFE_GLOBAL_MIME_FILE;
extern int XFE_GOPHER_PROXY_HOST;
extern int XFE_HOST_IS_UNKNOWN;
extern int XFE_HTTPS_PROXY_HOST;
extern int XFE_HTTP_PROXY_HOST;
extern int XFE_MAIL_HOST;
extern int XFE_NEWS_HOST;
extern int XFE_NEWS_RC_DIRECTORY;
extern int XFE_NO_PORT_NUMBER;
extern int XFE_PRIVATE_MAILCAP_FILE;
extern int XFE_PRIVATE_MIME_FILE;
extern int XFE_SOCKS_HOST;
extern int XFE_TEMP_DIRECTORY;
extern int XFE_WAIS_PROXY_HOST;
extern int XFE_WARNING;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_GENERAL;
extern int XFE_PASSWORDS;
extern int XFE_PERSONAL_CERTIFICATES;
extern int XFE_SITE_CERTIFICATES;
extern int XFE_UNKNOWN_ERROR;
extern int XFE_APPEARANCE;
extern int XFE_BOOKMARKS;
extern int XFE_COLORS;
extern int XFE_FONTS;
extern int XFE_APPLICATIONS;
extern int XFE_HELPERS;
extern int XFE_IMAGES;
extern int XFE_LANGUAGES;
extern int XFE_CACHE;
extern int XFE_CONNECTIONS;
extern int XFE_PROXIES;
extern int XFE_PROTOCOLS;
extern int XFE_LANG;
extern int XFE_APPEARANCE;
extern int XFE_COMPOSE_DLG;
extern int XFE_SERVERS;
extern int XFE_IDENTITY;
extern int XFE_ORGANIZATION;
extern int XFE_CHANGE_PASSWORD;
extern int XFE_SET_PASSWORD;
extern int XFE_PRIVATE_MIMETYPE_RELOAD;
extern int XFE_PRIVATE_MAILCAP_RELOAD;
extern int XFE_PRIVATE_RELOADED_MIMETYPE;
extern int XFE_PRIVATE_RELOADED_MAILCAP;

extern char* fe_mn_getmailbox(void);

void fe_attach_field_to_labels (Widget value_field, ...);

void fe_browse_file_of_text (MWContext *context, Widget text_field,
                             Boolean dirp);

void fe_UnmanageChild_safe (Widget w);

/* The last prefs page displayed, by folder */
static int current_general_page = xfe_GENERAL_OFFSET;
static int current_mailnews_page = xfe_MAILNEWS_OFFSET;
static int current_network_page = xfe_NETWORK_OFFSET;
static int current_security_page = xfe_SECURITY_OFFSET;

/* Preferences
 */

struct fe_prefs_data
{
  MWContext *context;

  Widget shell, form;

  /*=== General prefs folder ===*/
  Widget general_form;

  /* Appearance page */
  Widget styles1_selector, styles1_page;
  Widget icons_p, text_p, both_p, tips_p;
  Widget browser_p, mail_p, news_p;
  Widget blank_p, home_p, home_text;
  Widget underline_p;
  Widget expire_days_p, expire_days_text, never_expire_p;

  /* Applications page */
  Widget apps_selector, apps_page;
  Widget tmp_text;
  Widget telnet_text, tn3270_text, rlogin_text, rlogin_user_text;

#if 0
  /* Bookmarks page */
  Widget bookmarks_selector, bookmarks_page;
  Widget book_text;
#endif

  /* Colors page */
  Widget colors_selector, colors_page;

  /* Fonts page */
  Widget fonts_selector, fonts_page;
  Widget prop_family_pulldown, fixed_family_pulldown;
  Widget prop_family_option, fixed_family_option;
  Widget prop_size_pulldown, fixed_size_pulldown;
  Widget prop_size_option, fixed_size_option;
  Widget prop_size_field, fixed_size_field;
  Widget prop_size_toggle, fixed_size_toggle;
  int fonts_changed;

  /* Helpers page */
  Widget helpers_selector, helpers_page;
  int helpers_changed;

  struct fe_prefs_helpers_data *helpers;

  /* Images page */
  Widget images_selector, images_page;
  Widget auto_p, dither_p, closest_p;
  Widget while_loading_p, after_loading_p;

#ifdef REQUEST_LANGUAGE
  /* Languages page */
  Widget languages_selector, languages_page;
#endif

  /*=== Mail/News prefs folder ===*/
  Widget mailnews_form;

  /* Appearance page */
  Widget appearance_selector, appearance_page;
  Widget fixed_message_font_p, var_message_font_p;
  Widget cite_plain_p, cite_bold_p, cite_italic_p, cite_bold_italic_p;
  Widget cite_normal_p, cite_bigger_p, cite_smaller_p;
  Widget cite_color_text;
  Widget mail_horiz_p, mail_vert_p, mail_stack_p, mail_tall_p;
  Widget news_horiz_p, news_vert_p, news_stack_p, news_tall_p;

  /* Compose page */
  Widget compose_selector, compose_page;
  Widget eightbit_toggle, qp_toggle;
  Widget deliverAuto_toggle, deliverQ_toggle;
  Widget mMailOutSelf_toggle, mMailOutOther_text;
  Widget nMailOutSelf_toggle, nMailOutOther_text;
  Widget mCopyOut_text;
  Widget nCopyOut_text;
  Widget autoquote_toggle;

  /* Servers page */
  Widget dirs_selector, dirs_page;
  Widget srvr_text, user_text;
  Widget pop_toggle;
  Widget movemail_text;
  Widget external_toggle, builtin_toggle;
  Widget no_limit, msg_limit, limit_text;
  Widget msg_remove, msg_leave;
  Widget check_every, check_never, check_text;
  Widget smtp_text, maildir_text;
  Widget newshost_text, newsrc_text, newsmax_text;

  /* Identity page */
  Widget identity_selector, identity_page;
  Widget user_name_text, user_mail_text, user_org_text, user_sig_text;

  /* Organization page */
  Widget organization_selector, organization_page;
  Widget emptyTrash_toggle, rememberPswd_toggle;
  Widget threadmail_toggle, threadnews_toggle;
  Widget mdate_toggle, mnum_toggle, msubject_toggle, msender_toggle;
  Widget ndate_toggle, nnum_toggle, nsubject_toggle, nsender_toggle;

  /*=== Network prefs folder ===*/
  Widget network_form;

  /* Cache page */
  Widget cache_selector, cache_page;
  Widget memory_text, memory_clear;
  Widget disk_text, disk_dir, disk_clear;
  Widget verify_label, once_p, every_p, expired_p;
  Widget cache_ssl_p;

  /* Nework page */
  Widget network_selector, network_page;
  Widget buf_label, buf_text;
  Widget conn_label, conn_text;

  /* Proxies page */
  Widget proxies_selector, proxies_page;
  Widget no_proxies_p, manual_p, manual_browse;
  Widget auto_proxies_p, proxy_label, proxy_text;
  Widget proxy_reload;

  Widget ftp_text, ftp_port;
  Widget gopher_text, gopher_port;
  Widget http_text, http_port;
  Widget https_text, https_port;
  Widget wais_text, wais_port;
  Widget no_text;
  Widget socks_text, socks_port;

  /* Protocols page */
  Widget protocols_selector, protocols_page;
  Widget cookie_p;	/* Show alert before accepting a cookie */
  Widget anon_ftp_p;	/* Use email address for anon ftp */
  Widget email_form_p;	/* Show alert before submiting a form by email */

  /* Languages page */
  Widget lang_selector, lang_page;
#ifdef JAVA
  Widget java_toggle;
#endif
#ifdef MOCHA
  Widget javascript_toggle;
#endif


#ifdef DUPLICATE_SEC_PREFS
  /*=== Security prefs folder ===*/
  Widget security_form;

  /* General Security page */
  Widget sec_general_selector, sec_general_page;
  Widget enter_toggle, leave_toggle, mixed_toggle, submit_toggle;
  Widget ssl2_toggle, ssl3_toggle;

  /* Security Passwords page */
  Widget sec_passwords_selector, sec_passwords_page;
  Widget once_toggle, every_toggle, periodic_toggle;
  Widget periodic_text, change_password;

  /* Personal Certificates page */
  Widget personal_selector, personal_page;
  Widget pers_list, pers_info, pers_delete_cert, pers_new_cert;
  Widget site_default, pers_label;
  Widget pers_cert_menu;
  char *deleted_user_cert;

  /* Site Certificates page */
  Widget site_selector, site_page;
  Widget all_list, all_edit_cert, all_delete_cert;
  Widget all_label;
  char *deleted_site_cert;
#endif /* DUPLICATE_SEC_PREFS */
};

void
FE_SetPasswordAskPrefs(MWContext *context, int askPW, int timeout)
{
  /* Xfe's notions of askpassword state is 0, 1 or 2 while that of backend is
     -1, 0 or 1. That is why we add 1. */
  fe_globalPrefs.ask_password = askPW + 1;
  if (fe_globalPrefs.ask_password == 2)
    fe_globalPrefs.password_timeout = timeout;

  /* Save the pref file */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
  {
    if (context != NULL)
      fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));
  }
}

void
FE_SetPasswordEnabled(MWContext *context, PRBool usePW)
{
#ifdef DUPLICATE_SEC_PREFS
  char *s;

  /* If context is displaying the pref now, then change these values
     rightaway in the pref that is shown to the user. */
  if (context && CONTEXT_DATA(context)->currentPrefDialog &&
      CONTEXT_DATA(context)->fep && 
      XtIsManaged(CONTEXT_DATA(context)->fep->change_password)) {
    struct fe_prefs_data *fep = CONTEXT_DATA(context)->fep;

    if (usePW)
      s = XP_GetString(XFE_CHANGE_PASSWORD);
    else
      s = XP_GetString(XFE_SET_PASSWORD);
    fe_SetString (fep->change_password, XmNlabelString, s);
  }
#endif /* DUPLICATE_SEC_PREFS */
}


void
fe_MarkHelpersModified(MWContext *context)
{
  /* Mark that the helpers have been modified. We need this
   * to detect if we need to save/reload all these helpers.
   * For doing this, the condition we adopt is
   * (1) User clicked on 'OK' button for the Edit Helpers Dialog
   *	This also covers the "New" case as the same Dialog
   *	is shown for the "New" and "Edit".
   * (2) User deleted a mime/type and confirmed it.
   */
  if (context && CONTEXT_DATA(context)->fep)
    CONTEXT_DATA(context)->fep->helpers_changed = TRUE;
}


/*======================== A pile of callbacks  ========================*/

static void
fe_set_proxies_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->no_proxies_p)
    {
      XtVaSetValues (fep->manual_p, XmNset, False, 0);
      XtVaSetValues (fep->auto_proxies_p, XmNset, False, 0);
      XtVaSetValues (fep->manual_browse, XmNsensitive, False, 0);
      XtVaSetValues (fep->proxy_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->proxy_reload, XmNsensitive, False, 0);
    }
  else if (widget == fep->manual_p)
    {
      XtVaSetValues (fep->no_proxies_p, XmNset, False, 0);
      XtVaSetValues (fep->auto_proxies_p, XmNset, False, 0);
      XtVaSetValues (fep->manual_browse, XmNsensitive, True, 0);
      XtVaSetValues (fep->proxy_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->proxy_reload, XmNsensitive, False, 0);
    }
  else if (widget == fep->auto_proxies_p)
    {
      XtVaSetValues (fep->manual_p, XmNset, False, 0);
      XtVaSetValues (fep->no_proxies_p, XmNset, False, 0);
      XtVaSetValues (fep->manual_browse, XmNsensitive, False, 0);
      XtVaSetValues (fep->proxy_text, XmNsensitive, True, 0);
      XtVaSetValues (fep->proxy_reload, XmNsensitive, True, 0);
    }
  else
    abort ();
}

static void
fe_proxy_reload_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *s1;

  XtVaGetValues (fep->proxy_text, XmNvalue, &s1, 0);
  if (*s1) {
    NET_SetProxyServer(PROXY_AUTOCONF_URL, s1);
    fe_globalPrefs.proxy_url = XP_STRDUP (s1);
    XP_FREE (s1);
  }
  NET_ReloadProxyConfig (fep->context);
}

static void fe_make_proxies_dialog (MWContext *ctx,
				    struct fe_prefs_data *data);

static void
fe_view_proxies_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_make_proxies_dialog (fep->context, fep);
}

/* forward definition for the cert callbacks */
static void fe_prefs_destroy_cb (Widget, XtPointer, XtPointer);

#ifdef DUPLICATE_SEC_PREFS
static void
fe_cert_pulldown_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XtPointer data;
  XmStringTable str_list = 0;
  int n;
  int cert_type = -1;
  CERTCertNicknames *certnames;

  XtVaGetValues (fep->all_list, XmNitems, &str_list, XmNitemCount, &n, 0);
  XmListDeleteItems (fep->all_list, str_list, n);
  str_list = 0;

  XtVaGetValues (widget, XmNuserData, &data, 0);
  cert_type = (int) data;

  /* Change the values in the Scrolled List */

  certnames = CERT_GetCertNicknames (CERT_GetDefaultCertDB(), cert_type,
				     fep->context);

  XP_ASSERT (certnames);
  if (!certnames) return;

  str_list = (XmStringTable) XP_ALLOC (sizeof (XmString) * 
					certnames->numnicknames);
  for (n = 0; n < certnames->numnicknames; n++)
    str_list[n] = XmStringCreateLocalized (certnames->nicknames[n]);

  XtVaSetValues (fep->all_list, XmNitems, str_list, XmNitemCount, n, 0);

  if (certnames->numnicknames)
    XmListSelectPos (fep->all_list, 1, False);

  for (n = 0; n < certnames->numnicknames; n++)
    XmStringFree (str_list [n]);
  XP_FREE (str_list);
  CERT_FreeNicknames (certnames);
}


static void
fe_all_edit_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *certname;
  XmString *str;
  int n = 0;

  XtVaGetValues (fep->all_list,
		 XmNselectedItems, &str,
		 XmNselectedItemCount, &n,
		 0);
  if (n) {
    XmStringGetLtoR (str[0], XmFONTLIST_DEFAULT_TAG, &certname);
    if (certname) {
      MWContext *context = fep->context;

      SECNAV_EditSiteCertificate (context, certname);
      XP_FREE (certname);
    }
  }
}

static void
fe_redraw_site_cert_cb (void *closure)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  if (fep->deleted_site_cert) {
    XmString str = XmStringCreateLocalized (fep->deleted_site_cert);
    XmListDeleteItem (fep->all_list, str);
    XmStringFree (str);
    XP_FREE (fep->deleted_site_cert);
  }
}

static void
fe_all_delete_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *certname;
  XmString *str;
  int n = 0;

  XtVaGetValues (fep->all_list,
		 XmNselectedItems, &str,
		 XmNselectedItemCount, &n,
		 0);
  if (n) {
    XmStringGetLtoR (str[0], XmFONTLIST_DEFAULT_TAG, &certname);
    fep->deleted_site_cert = XP_STRDUP (certname);
    if (certname) {
      MWContext *context = fep->context;

      SECNAV_DeleteSiteCertificate (context, certname,
				    (SECNAVDeleteCertCallback) fe_redraw_site_cert_cb, fep);
      XP_FREE (certname);
    }
  }
}

static void
fe_cert_info_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *certname;
  XmString *str;
  int n = 0;

  XtVaGetValues (fep->pers_list,
		 XmNselectedItems, &str,
		 XmNselectedItemCount, &n,
		 0);
  if (n) {
    XmStringGetLtoR (str[0], XmFONTLIST_DEFAULT_TAG, &certname);
    if (certname) {
      MWContext *context = fep->context;

      SECNAV_ViewUserCertificate (context, certname);
      XP_FREE (certname);
    }
  }
}

static void
fe_redraw_user_cert_cb (void *closure)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  if (fep->deleted_user_cert) {
    XmString str = XmStringCreateLocalized (fep->deleted_user_cert);
    XmListDeleteItem (fep->pers_list, str);
    XmStringFree (str);
    XP_FREE (fep->deleted_user_cert);
  }
}

static void
fe_cert_delete_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *certname;
  XmString *str;
  int n = 0;

  XtVaGetValues (fep->pers_list,
		 XmNselectedItems, &str,
		 XmNselectedItemCount, &n,
		 0);
  if (n) {
    XmStringGetLtoR (str[0], XmFONTLIST_DEFAULT_TAG, &certname);
    fep->deleted_user_cert = XP_STRDUP (certname);
    if (certname) {
      MWContext *context = fep->context;

      SECNAV_DeleteUserCertificate (context, certname, 
				    (SECNAVDeleteCertCallback) fe_redraw_user_cert_cb, fep);
      XP_FREE (certname);
    }
  }
}

static void fe_prefs_destroy_dialog (struct fe_prefs_data *);

static void
fe_cert_new_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  MWContext *context = fep->context;

  fe_prefs_destroy_dialog (fep);
  SECNAV_NewUserCertificate (context);
}

static void
fe_ssl_enable_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  MWContext *context = fep->context;
  int ssl_version;

  XtVaGetValues (widget, XmNuserData, &ssl_version, 0);

  SECNAV_MakeCiphersDialog(context, ssl_version);
}
#endif /* DUPLICATE_SEC_PREFS */

static void
fe_browse_to_text_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  Widget wtext;
  XtVaGetValues (widget, XmNuserData, &wtext, 0);
  if (!wtext) return;
  fe_browse_file_of_text (fep->context, wtext, False);
}

static void
fe_browse_movemail_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->movemail_text, True);
}

static void
fe_browse_maildir_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->maildir_text, True);
}

static void
fe_browse_tmp_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->tmp_text, True);
}

#if 0
static void
fe_browse_book_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->book_text, False);
}
#endif

/* Toggle callback for all the toggle buttons in organization pane */
static void
fe_org_toggle_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  /* mail Sorting */
  else if (widget == fep->mdate_toggle) {
    XtVaSetValues (fep->mnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->mnum_toggle) {
    XtVaSetValues (fep->mdate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->msubject_toggle) {
    XtVaSetValues (fep->mdate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->mnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->msender_toggle) {
    XtVaSetValues (fep->mdate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->mnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
  }
  /* news Sorting */
  else if (widget == fep->ndate_toggle) {
    XtVaSetValues (fep->nnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->nnum_toggle) {
    XtVaSetValues (fep->ndate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->nsubject_toggle) {
    XtVaSetValues (fep->ndate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsender_toggle, XmNset, False, 0);
  }
  else if (widget == fep->nsender_toggle) {
    XtVaSetValues (fep->ndate_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nnum_toggle, XmNset, False, 0);
    XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
  }
  else
    abort ();
}

#ifdef PREFS_VERIFY
static void
fe_cache_verify_toggle_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->once_p)
    {
      XtVaSetValues (fep->every_p, XmNset, False, 0);
      XtVaSetValues (fep->expired_p, XmNset, False, 0);
    }
  else if (widget == fep->every_p)
    {
      XtVaSetValues (fep->once_p, XmNset, False, 0);
      XtVaSetValues (fep->expired_p, XmNset, False, 0);
    }
  else if (widget == fep->expired_p)
    {
      XtVaSetValues (fep->once_p, XmNset, False, 0);
      XtVaSetValues (fep->every_p, XmNset, False, 0);
    }
  else
    abort ();
}
#endif/* PREFS_VERIFY */

static void
fe_cache_dither_toggle_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->dither_p)
    {
      XtVaSetValues (fep->auto_p, XmNset, False, 0);
      XtVaSetValues (fep->closest_p, XmNset, False, 0);
    }
  else if (widget == fep->closest_p)
    {
      XtVaSetValues (fep->auto_p, XmNset, False, 0);
      XtVaSetValues (fep->dither_p, XmNset, False, 0);
    }
  else if (widget == fep->auto_p)
    {
      XtVaSetValues (fep->closest_p, XmNset, False, 0);
      XtVaSetValues (fep->dither_p, XmNset, False, 0);
    }
  else
    abort ();
}

static void
fe_cache_display_toggle_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->while_loading_p)
    XtVaSetValues (fep->after_loading_p, XmNset, False, 0);
  else if (widget == fep->after_loading_p)
    XtVaSetValues (fep->while_loading_p, XmNset, False, 0);
  else
    abort ();
}


#ifdef PREFS_DISK_CACHE
static void
fe_disk_cache_browse_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->disk_dir, True);
}
#endif /* PREFS_DISK_CACHE */

#ifdef PREFS_CLEAR_CACHE_BUTTONS
static void
fe_clear_mem_cache_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  if (XFE_Confirm (fep->context, fe_globalData.clear_mem_cache_message))
    {
      int real_size = fe_globalPrefs.memory_cache_size * 1024;
      NET_SetMemoryCacheSize (0);
      NET_SetMemoryCacheSize (real_size);
    }
}

#ifdef PREFS_DISK_CACHE
static void
fe_clear_disk_cache_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  if (XFE_Confirm (fep->context, fe_globalData.clear_disk_cache_message))
    {
      int real_size = fe_globalPrefs.disk_cache_size * 1024;
      NET_SetDiskCacheSize (0);
      NET_CleanupCacheDirectory (FE_CacheDir, "cache");
      NET_WriteCacheFAT (0, False);
      NET_SetDiskCacheSize (real_size);
    }
}
#endif /* PREFS_DISK_CACHE */
#endif /* PREFS_CLEAR_CACHE_BUTTONS */

#ifdef PREFS_SIG
static void
fe_browse_sig_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->user_sig_text, False);
}
#endif


static void
fe_browse_newsrc_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_browse_file_of_text (fep->context, fep->newsrc_text, True);
}

static void
fe_mail_pop_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->pop_toggle)
   {
      XtVaSetValues (fep->builtin_toggle, XmNset, False, 0);
      XtVaSetValues (fep->external_toggle, XmNset, False, 0);
      XtVaSetValues (fep->movemail_text, XmNsensitive, False, 0);
      if ( !ekit_PopServer() ) {
        XtVaSetValues (fep->srvr_text, XmNsensitive, True, 0);
      }
      if ( !ekit_PopName() ) {
          XtVaSetValues (fep->user_text, XmNsensitive, True, 0);
      }
   }
  else if (widget == fep->builtin_toggle)
   {
      XtVaSetValues (fep->pop_toggle, XmNset, False, 0);
      XtVaSetValues (fep->srvr_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->user_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->movemail_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->external_toggle, XmNset, False, 0);
   }
  else if (widget == fep->external_toggle)
   {
      XtVaSetValues (fep->pop_toggle, XmNset, False, 0);
      XtVaSetValues (fep->srvr_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->user_text, XmNsensitive, False, 0);
      XtVaSetValues (fep->builtin_toggle, XmNset, False, 0);
      XtVaSetValues (fep->movemail_text, XmNsensitive, True, 0);
   }
  else
    abort ();
}

#if 0
static void
fe_change_pass_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  MWContext *context = fep->context;

  SECNAV_ChangePassword(context);
}
#endif

#ifdef DUPLICATE_SEC_PREFS
static void
fe_set_pass_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->once_toggle)
   {
      XtVaSetValues (fep->every_toggle, XmNset, False, 0);
      XtVaSetValues (fep->periodic_toggle, XmNset, False, 0);
      XtVaSetValues (fep->periodic_text, XmNsensitive, False, 0);
   }
  else if (widget == fep->every_toggle)
   {
      XtVaSetValues (fep->once_toggle, XmNset, False, 0);
      XtVaSetValues (fep->periodic_toggle, XmNset, False, 0);
      XtVaSetValues (fep->periodic_text, XmNsensitive, False, 0);
   }
  else if (widget == fep->periodic_toggle)
   {
      XtVaSetValues (fep->once_toggle, XmNset, False, 0);
      XtVaSetValues (fep->every_toggle, XmNset, False, 0);
      XtVaSetValues (fep->periodic_text, XmNsensitive, True, 0);
   }
  else
    abort ();
}
#endif /* DUPLICATE_SEC_PREFS */

static void
fe_msg_limit_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->no_limit)
    {
      XtVaSetValues (fep->msg_limit, XmNset, False, 0);
      XtVaSetValues (fep->limit_text, XmNsensitive, False, 0);
    }
  else if (widget == fep->msg_limit)
    {
      XtVaSetValues (fep->no_limit, XmNset, False, 0);
      if ( !ekit_MsgSizeLimit() ) {
          XtVaSetValues (fep->limit_text, XmNsensitive, True, 0);
      }
    }
  else
    abort ();
}

static void
fe_msg_on_server_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->msg_remove)
    {
      XtVaSetValues (fep->msg_leave, XmNset, False, 0);
    }
  else if (widget == fep->msg_leave)
    {
      XtVaSetValues (fep->msg_remove, XmNset, False, 0);
    }
  else
    abort ();
}

static void
fe_msg_check_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->check_every)
    {
      XtVaSetValues (fep->check_never, XmNset, False, 0);
      XtVaSetValues (fep->check_text, XmNsensitive, True, 0);
    }
  else if (widget == fep->check_never)
    {
      XtVaSetValues (fep->check_every, XmNset, False, 0);
      XtVaSetValues (fep->check_text, XmNsensitive, False, 0);
    }
  else
    abort ();
}

static void
fe_mail_eightbit_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
#ifdef PREFS_8BIT
  else if (widget == fep->eightbit_toggle)
    XtVaSetValues (fep->qp_toggle, XmNset, False, 0);
  else if (widget == fep->qp_toggle)
    XtVaSetValues (fep->eightbit_toggle, XmNset, False, 0);
#endif /* PREFS_8BIT */
#ifdef PREFS_QUEUED_DELIVERY
  else if (widget == fep->deliverAuto_toggle)
    XtVaSetValues (fep->deliverQ_toggle, XmNset, False, 0);
  else if (widget == fep->deliverQ_toggle)
    XtVaSetValues (fep->deliverAuto_toggle, XmNset, False, 0);
#endif
  else
    abort ();
}

static void
fe_styles_toolbar_toggle_cb (Widget widget, XtPointer closure,
			     XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->icons_p)
    {
      XtVaSetValues (fep->text_p, XmNset, False, 0);
      XtVaSetValues (fep->both_p, XmNset, False, 0);
    }
  else if (widget == fep->text_p)
    {
      XtVaSetValues (fep->icons_p, XmNset, False, 0);
      XtVaSetValues (fep->both_p, XmNset, False, 0);
    }
  else if (widget == fep->both_p)
    {
      XtVaSetValues (fep->text_p, XmNset, False, 0);
      XtVaSetValues (fep->icons_p, XmNset, False, 0);
    }
  else
    {
      abort ();
    }
}

static void
fe_styles_start_toggle_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->blank_p)
    {
      XtVaSetValues (fep->home_p, XmNset, False, 0);
      XtVaSetValues (fep->home_text, XmNsensitive, False, 0);
    }
  else if (widget == fep->home_p)
    {
      XtVaSetValues (fep->blank_p, XmNset, False, 0);
      XtVaSetValues (fep->home_text, XmNsensitive, True, 0);
    }
  else
    abort ();
}

static void
fe_styles_startup_page_toggle_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->browser_p)
    {
      XtVaSetValues (fep->mail_p, XmNset, False, 0);
      XtVaSetValues (fep->news_p, XmNset, False, 0);
    }
  else if (widget == fep->mail_p)
    {
      XtVaSetValues (fep->browser_p, XmNset, False, 0);
      XtVaSetValues (fep->news_p, XmNset, False, 0);
    }
  else if (widget == fep->news_p)
    {
      XtVaSetValues (fep->browser_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_p, XmNset, False, 0);
    }
  else
    abort ();
}


static void
fe_relayout_all_contexts(void)
{
	struct fe_MWContext_cons	*cons;

	for (cons = fe_all_MWContexts; cons; cons = cons->next)
	{
		switch (cons->context->type)
		{
		case MWContextBrowser:
		case MWContextMail:
		case MWContextNews:
			fe_ReLayout(cons->context, 0);
			break;
		default:
			break;
		}
	}
}

/* This disabled the backends notion of the font cache.
 *
 * FE caches fonts for each LO_TextAttr in the LO_TextAttr->FE_Data. Once
 * the user changes either the default family/face, size, proportional/fixed,
 * scaled/non-scaled, we will need to clear the font cache that backend
 * maintains.
 */
static void
fe_InvalidateFontData(void)
{
	struct fe_MWContext_cons	*cons;

	/*
	 * free the Asian font groups, which are also caches
	 */
	fe_FreeFontGroups();

	for (cons = fe_all_MWContexts; cons; cons = cons->next)
	{
		switch (cons->context->type)
		{
		case MWContextBrowser:
		case MWContextMail:
		case MWContextNews:
			LO_InvalidateFontData(cons->context);
			break;
		default:
			break;
		}
	}
}



static void
fe_allow_font_scaling_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	XmToggleButtonCallbackStruct	*cb;
	XtPointer			data;
	fe_FontFamily			*family;
	struct fe_prefs_data		*fep;
	int				i;
	fe_FontPitch			*pitch;
	unsigned int			allowScaling;
	
	cb = (XmToggleButtonCallbackStruct *) call_data;

	fep = (struct fe_prefs_data *) closure;
	fep->fonts_changed = 1;

	XtVaGetValues(widget, XmNuserData, &data, 0);
	pitch = (fe_FontPitch *) data;

	family = NULL;
	for (i = 0; i < pitch->numberOfFamilies; i++)
	{
		family = &pitch->families[i];
		if (family->selected)
		{
			break;
		}
		else
		{
			family = NULL;
		}
	}
	if (!family)
	{
		return;
	}

	
	if (cb->set)
	{
		allowScaling = 1;
	}
	else
	{
		allowScaling = 0;
	}

	/*
	 * sanity check
	 */
	if (family->pointSizes->size)
	{
		allowScaling = 0;
		XtVaSetValues(widget, XmNset, False, 0);
		XtVaSetValues(widget, XmNsensitive, False, 0);
	}
	else if (family->numberOfPointSizes == 1)
	{
		allowScaling = 1;
		XtVaSetValues(widget, XmNset, True, 0);
		XtVaSetValues(widget, XmNsensitive, False, 0);
	}

	if (allowScaling != family->allowScaling) {
	 	fe_FreeFontSizeTable(family);
		family->allowScaling = allowScaling;
		fe_InvalidateFontData();
		fe_relayout_all_contexts();
	}
}


static void
fe_get_scaled_font_size(struct fe_prefs_data *fep)
{
	char		*buf;
	fe_FontCharSet	*charset = NULL;
	char		*endptr;
	fe_FontFamily	*family;
	Widget		field;
	int		i;
	int		j;
	int		oldSize;
	fe_FontPitch	*pitch;

	for (i = 0; i < INTL_CHAR_SET_MAX; i++)
	{
		if (fe_FontCharSets[i].selected)
		{
			charset = &fe_FontCharSets[i];
		}
	}
	if (!charset)
	{
		return;
	}

	for (i = 0; i < 2; i++)
	{
		field = (i ? fep->fixed_size_field : fep->prop_size_field);
		pitch = &charset->pitches[i];
		XtVaGetValues(field, XmNvalue, &buf, 0);
		family = NULL;
		for (j = 0; j < pitch->numberOfFamilies; j++)
		{
			family = &pitch->families[j];
			if (family->selected)
			{
				break;
			}
			else
			{
				family = NULL;
			}
		}
		if (!family)
		{
			continue;
		}
		oldSize = family->scaledSize;
		errno = 0;
		family->scaledSize = ((int) (strtod(buf, &endptr) * 10));
		if (errno || (family->scaledSize < 0))
		{
			family->scaledSize = 0;
		}
		if (family->scaledSize != oldSize)
		{
			fep->fonts_changed = 1;
			fe_FreeFontSizeTable(family);
			fe_InvalidateFontData();
		}
		free(buf);
	}
}


static void
fe_font_size_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	XtPointer		data;
	fe_FontFamily		*family;
	struct fe_prefs_data	*fep;
	int			i;
	fe_FontPitch		*pitch;
	fe_FontSize		*size;
	int			thisSize;

	fep = (struct fe_prefs_data *) closure;
	fep->fonts_changed = 1;

	XtVaGetValues(widget, XmNuserData, &data, 0);
	pitch = (fe_FontPitch *) data;

	family = NULL;
	for (i = 0; i < pitch->numberOfFamilies; i++)
	{
		family = &pitch->families[i];
		if (family->selected)
		{
			break;
		}
		else
		{
			family = NULL;
		}
	}
	if (!family)
	{
		return;
	}

	thisSize = atoi(XtName(widget));

	for (i = 0; i < family->numberOfPointSizes; i++)
	{
		size = &family->pointSizes[i];
		if (size->size == thisSize)
		{
			size->selected = 1;
		}
		else
		{
			size->selected = 0;
		}
	}

	fe_FreeFontSizeTable(family);
	fe_InvalidateFontData();
	fe_relayout_all_contexts();
}


static void
fe_set_new_font_sizes(struct fe_prefs_data *fep, fe_FontPitch *pitch)
{
	int			ac;
	Arg			av[50];
	Widget			button;
	fe_FontFamily		*family;
	int			i;
	Widget			*kids;
	char			label[16];
	char			name[16];
	Cardinal		nkids;
	Widget			optionMenu;
	fe_FontSize		*size;
	Widget			toggle;
	XmString		xms;

	xms = NULL;

	if (pitch->sizeMenu == fep->prop_size_pulldown)
	{
		optionMenu = fep->prop_size_option;
		toggle = fep->prop_size_toggle;
	}
	else
	{
		optionMenu = fep->fixed_size_option;
		toggle = fep->fixed_size_toggle;
	}

	XtVaGetValues(pitch->sizeMenu, XmNchildren, &kids,
		XmNnumChildren, &nkids, 0);
	for (i = nkids - 1; i >= 0; i--)
	{
		XtDestroyWidget(kids[i]);
	}

	family = NULL;
	for (i = 0; i < pitch->numberOfFamilies; i++)
	{
		family = &pitch->families[i];
		if (family->selected)
		{
			break;
		}
		else
		{
			family = NULL;
		}
	}
	if (!family)
	{
		return;
	}

	if (family->allowScaling)
	{
		XtVaSetValues(toggle, XmNset, True, 0);
	}
	else
	{
		XtVaSetValues(toggle, XmNset, False, 0);
	}

	if (family->pointSizes->size)
	{
		XtVaSetValues(toggle, XmNsensitive, False, 0);
	}
	else
	{
		if (family->numberOfPointSizes == 1)
		{
			XtVaSetValues(toggle, XmNsensitive, False, 0);
		}
		else
		{
			XtVaSetValues(toggle, XmNsensitive, True, 0);
		}
	}

	XtVaSetValues(toggle, XmNuserData, pitch, NULL);

	for (i = 0; i < family->numberOfPointSizes; i++)
	{
		size = &family->pointSizes[i];
		(void) PR_snprintf(name, sizeof(name), "%d", size->size);
		ac = 0;
		if (size->size)
		{
			(void) PR_snprintf(label, sizeof(label), "%d.%d",
				size->size / 10, size->size % 10);
			xms = XmStringCreateLtoR(label,
				XmFONTLIST_DEFAULT_TAG);
			XtSetArg(av[ac], XmNlabelString, xms); ac++;
		}
		button = XmCreatePushButtonGadget(pitch->sizeMenu, name,
			av, ac);
		if (xms)
		{
			XmStringFree(xms);
			xms = NULL;
		}
		XtVaSetValues(button, XmNuserData, pitch, NULL);
		XtAddCallback (button, XmNactivateCallback, fe_font_size_cb,
			fep);
		if ((i == 0) || (size->selected))
		{
			XtVaSetValues(optionMenu, XmNmenuHistory, button, 0);
		}
		XtManageChild(button);
	}
}


static void
fe_font_family_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	XtPointer		data;
	fe_FontFamily		*family;
	struct fe_prefs_data	*fep;
	int			i;
	fe_FontPitch		*pitch;

	fep = (struct fe_prefs_data *) closure;
	fep->fonts_changed = 1;

	XtVaGetValues(widget, XmNuserData, &data, 0);
	pitch = (fe_FontPitch *) data;

	pitch->selectedFamily = NULL;
	for (i = 0; i < pitch->numberOfFamilies; i++)
	{
		family = &pitch->families[i];
		if (!strcmp(family->name, XtName(widget)))
		{
			family->selected = 1;
		}
		else
		{
			family->selected = 0;
		}
	}

	fe_set_new_font_sizes(fep, pitch);
	fe_InvalidateFontData();
	fe_relayout_all_contexts();
}


static void
fe_set_new_font_families(struct fe_prefs_data *fep, fe_FontPitch *pitch,
	Widget familyMenu, Widget sizeMenu)
{
	int			ac;
	Arg			av[50];
	Widget			button;
	fe_FontFamily		*family;
	int			i;
	Widget			*kids;
	Cardinal		nkids;
	Widget			optionMenu;

	pitch->sizeMenu = sizeMenu;

	if (pitch->sizeMenu == fep->prop_size_pulldown)
	{
		optionMenu = fep->prop_family_option;
	}
	else
	{
		optionMenu = fep->fixed_family_option;
	}

	XtVaGetValues(familyMenu, XmNchildren, &kids,
		XmNnumChildren, &nkids, 0);
	for (i = nkids - 1; i >= 0; i--)
	{
		XtDestroyWidget(kids[i]);
	}

	ac = 0;
	for (i = 0; i < pitch->numberOfFamilies; i++)
	{
		family = &pitch->families[i];
		button = XmCreatePushButtonGadget(familyMenu, family->name, av, ac);
		XtVaSetValues(button, XmNuserData, pitch, NULL);
		XtAddCallback (button, XmNactivateCallback, fe_font_family_cb,
			fep);
		if ((i == 0) || (family->selected))
		{
			XtVaSetValues(optionMenu, XmNmenuHistory, button, 0);
		}
		XtManageChild(button);
	}

	fe_set_new_font_sizes(fep, pitch);
}


static void
fe_font_charset_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
	fe_FontCharSet		*charset;
	int			charsetID;
	XtPointer		data;
	struct fe_prefs_data	*fep;
	int			i;

	fep = (struct fe_prefs_data *) closure;

	XtVaGetValues(widget, XmNuserData, &data, 0);
	charsetID = (int) data;

	for (i = 0; i < INTL_CHAR_SET_MAX; i++)
	{
		fe_FontCharSets[i].selected = 0;
	}
	charset = &fe_FontCharSets[charsetID];
	charset->selected = 1;

	fe_set_new_font_families(fep, &charset->pitches[0],
		fep->prop_family_pulldown, fep->prop_size_pulldown);

	fe_set_new_font_families(fep, &charset->pitches[1],
		fep->fixed_family_pulldown, fep->fixed_size_pulldown);
}


static void
fe_msg_font_toggle_cb (Widget widget, XtPointer closure,
			       XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->fixed_message_font_p)
    {
      XtVaSetValues (fep->var_message_font_p, XmNset, False, 0);
    }
  else if (widget == fep->var_message_font_p)
    {
      XtVaSetValues (fep->fixed_message_font_p, XmNset, False, 0);
    }
  else
    abort ();
}

static void
fe_styles_cite_font_toggle_cb (Widget widget, XtPointer closure,
			       XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->cite_plain_p)
    {
      XtVaSetValues (fep->cite_bold_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_italic_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bold_italic_p, XmNset, False, 0);
    }
  else if (widget == fep->cite_bold_p)
    {
      XtVaSetValues (fep->cite_plain_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_italic_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bold_italic_p, XmNset, False, 0);
    }
  else if (widget == fep->cite_italic_p)
    {
      XtVaSetValues (fep->cite_plain_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bold_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bold_italic_p, XmNset, False, 0);
    }
  else if (widget == fep->cite_bold_italic_p)
    {
      XtVaSetValues (fep->cite_plain_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bold_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_italic_p, XmNset, False, 0);
    }
  else
    abort ();
}


static void
fe_styles_cite_size_toggle_cb (Widget widget, XtPointer closure,
			       XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->cite_normal_p)
    {
      XtVaSetValues (fep->cite_bigger_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_smaller_p, XmNset, False, 0);
    }
  else if (widget == fep->cite_bigger_p)
    {
      XtVaSetValues (fep->cite_normal_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_smaller_p, XmNset, False, 0);
    }
  else if (widget == fep->cite_smaller_p)
    {
      XtVaSetValues (fep->cite_normal_p, XmNset, False, 0);
      XtVaSetValues (fep->cite_bigger_p, XmNset, False, 0);
    }
  else
    abort ();
}

static void
fe_mail_pane_layout_toggle_cb (Widget widget, XtPointer closure,
			       XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->mail_horiz_p)
    {
      XtVaSetValues (fep->mail_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_stack_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->mail_vert_p)
    {
      XtVaSetValues (fep->mail_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_stack_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->mail_stack_p)
    {
      XtVaSetValues (fep->mail_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->mail_tall_p)
    {
      XtVaSetValues (fep->mail_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->mail_stack_p, XmNset, False, 0);
    }
  else
    abort ();
}

static void
fe_news_pane_layout_toggle_cb (Widget widget, XtPointer closure,
			       XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->news_horiz_p)
    {
      XtVaSetValues (fep->news_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->news_stack_p, XmNset, False, 0);
      XtVaSetValues (fep->news_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->news_vert_p)
    {
      XtVaSetValues (fep->news_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->news_stack_p, XmNset, False, 0);
      XtVaSetValues (fep->news_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->news_stack_p)
    {
      XtVaSetValues (fep->news_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->news_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->news_tall_p, XmNset, False, 0);
    }
  else if (widget == fep->news_tall_p)
    {
      XtVaSetValues (fep->news_horiz_p, XmNset, False, 0);
      XtVaSetValues (fep->news_vert_p, XmNset, False, 0);
      XtVaSetValues (fep->news_stack_p, XmNset, False, 0);
    }
  else
    abort ();
}


static void
fe_styles_expire_toggle_cb (Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  char *text, dummy;
  int n;
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)call_data;
  XtVaGetValues (fep->expire_days_text, XmNvalue, &text, 0);
  if (1 != sscanf (text, " %d %c", &n, &dummy))
    n = -1;
  if (!cb->set)
    XtVaSetValues (widget, XmNset, True, 0);
  else if (widget == fep->expire_days_p)
    {
      XtVaSetValues (fep->never_expire_p, XmNset, False, 0);
      XtVaSetValues (fep->expire_days_text, XmNsensitive, True, 0);
      if (n < 0)
	XtVaSetValues (fep->expire_days_text, XmNvalue, "9", 0);
      XmProcessTraversal (fep->expire_days_text, XmTRAVERSE_CURRENT);
    }
  else if (widget == fep->never_expire_p)
    {
      XtVaSetValues (fep->expire_days_p, XmNset, False, 0);
      XtVaSetValues (fep->expire_days_text, XmNsensitive, False, 0);
      if (n < 0)
	XtVaSetValues (fep->expire_days_text, XmNvalue, "", 0);
    }
  else
    abort ();
  free (text);
}

static void
fe_styles_expire_now_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  if (XFE_Confirm (fep->context, fe_globalData.expire_now_message))
    {
      GH_ClearGlobalHistory ();
      fe_RefreshAllAnchors ();
    }
}

/*======================= Appearance Preferences ========================*/

static void
fe_make_styles1_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget toolbar_label, icons_p, text_p, both_p;
  Widget tips_label, tips_p;
  Widget label2, frame2, form2;
  Widget startup_label, browser_p, mail_p, news_p;
  Widget start_label, blank_p, home_p, home_text;
  Widget label3, frame3, form3;
  Widget underline_label, underline_p;
  Widget expire_label, expire_days_p, never_expire_p, expire_now_button;
  Widget expire_days_text, expire_days_label;
  Widget kids [100];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->styles1_selector, "styles", av, ac);
  XtManageChild (form);
  fep->styles1_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "toolbarsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "toolbarsBox", av, ac);


  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame1); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "startupFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form2 = XmCreateForm (frame2, "startupBox", av, ac);


  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame2); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame3 = XmCreateFrame (form, "linkFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form3 = XmCreateForm (frame3, "linkBox", av, ac);


  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "toolbarsBoxLabel", av, ac);
  label2 = XmCreateLabelGadget (frame2, "startupBoxLabel", av, ac);
  label3 = XmCreateLabelGadget (frame3, "linkBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = toolbar_label = XmCreateLabelGadget (form1, "toolbarLabel",
						    av, ac);
  kids [i++] = icons_p = XmCreateToggleButtonGadget (form1, "icons", av,ac);
  kids [i++] = text_p = XmCreateToggleButtonGadget (form1, "text", av,ac);
  kids [i++] = both_p = XmCreateToggleButtonGadget (form1, "both", av,ac);

  kids [i++] = tips_label = XmCreateLabelGadget (form1, "toolbarTipsLabel",
						    av, ac);
  kids [i++] = tips_p = fe_CreateToolTipsDemoToggle(form1, "enabled", av,ac);

  XtVaSetValues (toolbar_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, icons_p,
		 0);
  XtVaSetValues (icons_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 30,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (text_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 54,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (both_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 74,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (tips_label,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, icons_p,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, icons_p,
		 0);
  XtVaSetValues (tips_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, icons_p,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, icons_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  ac = 0;
  i = 0;
  kids [i++] = start_label = XmCreateLabelGadget (form2, "startLabel", av, ac);
  kids [i++] = blank_p = XmCreateToggleButtonGadget (form2, "blank", av, ac);
  kids [i++] = home_p = XmCreateToggleButtonGadget (form2, "home", av, ac);
  kids [i++] = home_text = fe_CreateTextField (form2, "homeText", av, ac);

  kids [i++] = startup_label =
		XmCreateLabelGadget (form2, "startupLabel", av, ac);
  kids [i++] = browser_p =
		XmCreateToggleButtonGadget (form2, "browser", av, ac);
  kids [i++] = mail_p = XmCreateToggleButtonGadget (form2, "mail", av, ac);
  kids [i++] = news_p = XmCreateToggleButtonGadget (form2, "news", av, ac);

  XtVaSetValues (startup_label,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, browser_p,
                 0);
  XtVaSetValues (browser_p,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 30,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (mail_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 54,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (news_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 74,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (start_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, blank_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, blank_p,
                 0);
  XtVaSetValues (blank_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, browser_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 30,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (home_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, blank_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, blank_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (home_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, home_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, home_p,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  ac = 0;
  i = 0;
  kids [i++] = underline_label = XmCreateLabelGadget (form3, "underlineLabel",
						      av, ac);
  kids [i++] = underline_p = XmCreateToggleButtonGadget (form3, "underline",
							 av, ac);

  kids [i++] = expire_label = XmCreateLabelGadget (form3, "expireLabel",
						   av, ac);
  kids [i++] = never_expire_p = XmCreateToggleButtonGadget (form3,
							    "expireNever",
							    av, ac);
  kids [i++] = expire_days_p = XmCreateToggleButtonGadget (form3,
							   "expireAfter",
							   av, ac);
  kids [i++] = expire_days_text = fe_CreateTextField (form3, "expireDays",
						     av, ac);
  kids [i++] = expire_days_label = XmCreateLabelGadget (form3,
							"expireDaysLabel",
							av, ac);
  kids [i++] = expire_now_button = XmCreatePushButtonGadget (form3,
							     "expireNow",
							     av, ac);

  XtVaSetValues (underline_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, underline_p,
		 0);
  XtVaSetValues (underline_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (expire_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, never_expire_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, never_expire_p,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, underline_p,
		 0);
  XtVaSetValues (never_expire_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, underline_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, underline_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (expire_days_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, underline_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, never_expire_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (expire_days_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, underline_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, expire_days_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (expire_days_label,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, underline_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, expire_days_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (expire_now_button,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, expire_days_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, expire_days_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, expire_days_label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fe_attach_field_to_labels (underline_p, underline_label, expire_label, 0);

  {
    Dimension h1, h2, h3, h4, h5, h;
    XtVaGetValues (expire_label, XmNheight, &h1, 0);
    XtVaGetValues (never_expire_p, XmNheight, &h2, 0);
    XtVaGetValues (expire_days_p, XmNheight, &h3, 0);
    XtVaGetValues (expire_days_text, XmNheight, &h4, 0);
    XtVaGetValues (expire_days_label, XmNheight, &h5, 0);
    h = h1;
    if (h < h2) h = h2;
    if (h < h3) h = h3;
    if (h < h4) h = h4;
    if (h < h5) h = h5;
    XtVaSetValues (expire_label, XmNheight, h, 0);
    XtVaSetValues (never_expire_p, XmNheight, h, 0);
    XtVaSetValues (expire_days_p, XmNheight, h, 0);
    XtVaSetValues (expire_days_text, XmNheight, h, 0);
    XtVaSetValues (expire_days_label, XmNheight, h, 0);
    XtVaSetValues (expire_now_button, XmNheight, h, 0);
  }

  XtManageChildren (kids, i);
  XtManageChild (label3);
  XtManageChild (form3);

  XtAddCallback (icons_p, XmNvalueChangedCallback,
		 fe_styles_toolbar_toggle_cb, fep);
  XtAddCallback (text_p, XmNvalueChangedCallback,
		 fe_styles_toolbar_toggle_cb, fep);
  XtAddCallback (both_p, XmNvalueChangedCallback,
		 fe_styles_toolbar_toggle_cb, fep);

  XtAddCallback (blank_p, XmNvalueChangedCallback,
		 fe_styles_start_toggle_cb, fep);
  XtAddCallback (home_p, XmNvalueChangedCallback,
		 fe_styles_start_toggle_cb, fep);

  XtAddCallback (browser_p, XmNvalueChangedCallback,
		 fe_styles_startup_page_toggle_cb, fep);
  XtAddCallback (mail_p, XmNvalueChangedCallback,
		 fe_styles_startup_page_toggle_cb, fep);
  XtAddCallback (news_p, XmNvalueChangedCallback,
		 fe_styles_startup_page_toggle_cb, fep);

  XtAddCallback (expire_days_p, XmNvalueChangedCallback,
		 fe_styles_expire_toggle_cb, fep);
  XtAddCallback (never_expire_p, XmNvalueChangedCallback,
		 fe_styles_expire_toggle_cb, fep);

  XtAddCallback (expire_now_button, XmNactivateCallback,
		 fe_styles_expire_now_cb, fep);

  fep->icons_p = icons_p;
  fep->text_p = text_p;
  fep->both_p = both_p;
  fep->tips_p = tips_p;
  fep->browser_p = browser_p;
  fep->mail_p = mail_p;
  fep->news_p = news_p;
  fep->blank_p = blank_p;
  fep->home_p = home_p;
  fep->home_text = home_text;
  fep->underline_p = underline_p;
  fep->expire_days_p = expire_days_p;
  fep->never_expire_p = never_expire_p;
  fep->expire_days_text = expire_days_text;

  i = 0;
  kids [i++] = frame1;
  kids [i++] = frame2;
  kids [i++] = frame3;
  XtManageChildren (kids, i);
}

#if 0
static void
fe_make_colors_page (MWContext *context, struct fe_prefs_data *fep)
{
  fep->colors_page = 0;
}
#endif

#if 0
static void
fe_make_bookmarks_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label2, frame2, form2;
  Widget book_label, book_text, book_browse;
  Widget kids [100];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->bookmarks_selector, "bookmarks", av, ac);
  XtManageChild (form);
  fep->bookmarks_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "dirsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form2 = XmCreateForm (frame2, "dirsBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "dirsBoxLabel", av, ac);

  i = 0;
  ac = 0;
  kids [i++] = book_label = XmCreateLabelGadget (form2, "bookLabel", av,ac);
  kids [i++] = book_text = fe_CreateTextField (form2, "bookText", av, ac);
  kids [i++] = book_browse =XmCreatePushButtonGadget(form2,"bookBrowse",av,ac);

  XtVaSetValues (book_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, book_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, book_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, book_text,
                 0);

  XtVaSetValues (book_text,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (book_browse,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_WIDGET,
                 XmNleftWidget, book_text,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);
  XtVaSetValues (book_browse, XmNheight, book_text->core.height, 0);

  XtAddCallback (book_browse, XmNactivateCallback, fe_browse_book_cb, fep);

  fep->book_text = book_text;

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  i = 0;
  kids [i++] = frame2;
  XtManageChildren (kids, i);
}
#endif /* 0 */

static void
fe_make_fonts_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget kids [100];
  int i;
  int j;
  Widget encoding_label, encoding_pulldown, encoding_menu;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Widget proportional_label, prop_size_label;
  Widget fixed_label, fixed_size_label;
  int selectedCharSet;
  fe_FontCharSet *charset;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->fonts_selector, "fonts", av, ac);
  XtManageChild (form);
  fep->fonts_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "fontsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "fontsBox", av, ac);


  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "fontsBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = encoding_label = XmCreateLabelGadget (form1, "encodingLabel",
	av, ac);

  XtVaGetValues (CONTEXT_WIDGET (context), XtNvisual, &v, XtNcolormap, &cmap,
	XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  encoding_pulldown = XmCreatePulldownMenu (form1, "encodingPullDown", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, encoding_pulldown); ac++;
  kids [i++] = encoding_menu = fe_CreateOptionMenu (form1, "encodingMenu", av,
	ac);

  selectedCharSet = 0;
  ac = 0;
  for (j = 0; j < INTL_CHAR_SET_MAX; j++) {
	Widget		button;

	charset = &fe_FontCharSets[fe_SortedFontCharSets[j]];
	if (!charset->name) {
		continue;
	}
	button = XmCreatePushButtonGadget (encoding_pulldown, charset->name, av, ac);
	XtVaSetValues(button, XmNuserData, fe_SortedFontCharSets[j], NULL);
	XtAddCallback (button, XmNactivateCallback, fe_font_charset_cb, fep);
	if (charset->selected) {
		XtVaSetValues (encoding_menu, XmNmenuHistory, button, 0);
		selectedCharSet = j;
	}
	XtManageChild (button);
  }
  charset = &fe_FontCharSets[fe_SortedFontCharSets[selectedCharSet]];

  ac = 0;
  kids [i++] = proportional_label = XmCreateLabelGadget (form1,
	"proportionalLabel", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  fep->prop_family_pulldown = XmCreatePulldownMenu (form1, "propPullDown", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, fep->prop_family_pulldown); ac++;
  kids [i++] = fep->prop_family_option = fe_CreateOptionMenu (form1,
	"proportionalMenu", av, ac);

  ac = 0;
  kids [i++] = prop_size_label = XmCreateLabelGadget (form1, "propSizeLabel",
	av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  fep->prop_size_pulldown = XmCreatePulldownMenu (form1, "propSizePullDown", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, fep->prop_size_pulldown); ac++;
  kids [i++] = fep->prop_size_option = fe_CreateOptionMenu (form1, "propSizeMenu",
	av, ac);

  ac = 0;
  kids [i++] = fep->prop_size_field = fe_CreateTextField (form1, "propSizeField",
	av, ac);

  ac = 0;
  kids [i++] = fep->prop_size_toggle = XmCreateToggleButtonGadget (form1,
	"propSizeToggle", av,ac);

  XtAddCallback(fep->prop_size_toggle, XmNvalueChangedCallback,
	fe_allow_font_scaling_cb, fep);

  fe_set_new_font_families(fep, &charset->pitches[0], fep->prop_family_pulldown,
	fep->prop_size_pulldown);

  ac = 0;
  kids [i++] = fixed_label = XmCreateLabelGadget (form1,
	"fixedLabel", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  fep->fixed_family_pulldown = XmCreatePulldownMenu (form1, "fixedPullDown", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, fep->fixed_family_pulldown); ac++;
  kids [i++] = fep->fixed_family_option = fe_CreateOptionMenu (form1,
	"fixedMenu", av, ac);

  ac = 0;
  kids [i++] = fixed_size_label = XmCreateLabelGadget (form1, "fixedSizeLabel",
	av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  fep->fixed_size_pulldown = XmCreatePulldownMenu (form1, "fixedSizePullDown",
	av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, fep->fixed_size_pulldown); ac++;
  kids [i++] = fep->fixed_size_option = fe_CreateOptionMenu (form1, "fixedSizeMenu",
	av, ac);

  ac = 0;
  kids [i++] = fep->fixed_size_field = fe_CreateTextField (form1, "fixedSizeField",
	av, ac);

  ac = 0;
  kids [i++] = fep->fixed_size_toggle = XmCreateToggleButtonGadget (form1,
	"fixedSizeToggle", av,ac);

  XtAddCallback(fep->fixed_size_toggle, XmNvalueChangedCallback,
	fe_allow_font_scaling_cb, fep);

  fe_set_new_font_families(fep, &charset->pitches[1],
	fep->fixed_family_pulldown, fep->fixed_size_pulldown);

  XtVaSetValues (encoding_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, encoding_menu,
		 0);
  XtVaSetValues (encoding_menu,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (proportional_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->prop_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->prop_family_option,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, encoding_menu,
		 0);
  XtVaSetValues (fep->prop_family_option,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, encoding_menu,
		 XmNtopOffset, 20,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, encoding_menu,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (prop_size_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->prop_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->prop_family_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, fep->prop_family_option,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fep->prop_size_option,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->prop_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->prop_family_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, prop_size_label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fep->prop_size_field,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->prop_size_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->prop_size_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, fep->prop_size_option,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (fep->prop_size_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, fep->prop_size_field,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, prop_size_label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (fixed_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->fixed_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->fixed_family_option,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, fep->prop_family_option,
		 0);
  XtVaSetValues (fep->fixed_family_option,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, fep->prop_size_toggle,
		 XmNtopOffset, 20,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, fep->prop_family_option,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fixed_size_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->fixed_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->fixed_family_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, fep->prop_family_option,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fep->fixed_size_option,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->fixed_family_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->fixed_family_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, fixed_size_label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fep->fixed_size_field,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, fep->fixed_size_option,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, fep->fixed_size_option,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, fep->fixed_size_option,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (fep->fixed_size_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, fep->fixed_size_field,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, fixed_size_label,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fe_attach_field_to_labels (encoding_menu, encoding_label,
	proportional_label, fixed_label, 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_apps_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;

  Widget tmp_label, tmp_text, tmp_browse;

  Widget telnet_label, telnet_text;
  Widget tn3270_label, tn3270_text;
  Widget rlogin_label, rlogin_text;
  Widget rlogin_user_label, rlogin_user_text;

  Widget kids [50];
  int i;

  /* Create the enclosing form */
  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->apps_selector, "apps", av, ac);
  XtManageChild (form);
  fep->apps_page = form;

  /* Create the frame and form for the application files */
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "appsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "appsBox", av, ac);

  /* Create labels */
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "appsBoxLabel", av, ac);

  /* Create the widgets */
  ac = 0;
  i = 0;
  kids [i++] = telnet_label = XmCreateLabelGadget (form1, "telnetLabel",av,ac);
  kids [i++] = tn3270_label = XmCreateLabelGadget (form1, "tn3270Label",av,ac);
  kids [i++] = rlogin_label = XmCreateLabelGadget (form1, "rloginLabel",av,ac);
  kids [i++] = rlogin_user_label = XmCreateLabelGadget (form1,
                                                        "rloginUserLabel",
                                                        av, ac);
  kids [i++] = telnet_text = fe_CreateTextField (form1, "telnetText", av, ac);
  kids [i++] = tn3270_text = fe_CreateTextField (form1, "tn3270Text", av, ac);
  kids [i++] = rlogin_text = fe_CreateTextField (form1, "rloginText", av, ac);
  kids [i++] = rlogin_user_text = fe_CreateTextField (form1, "rloginUserText",
                                                     av, ac);
  kids [i++] = tmp_label = XmCreateLabelGadget (form1, "tmpLabel", av,ac);
  kids [i++] = tmp_text = fe_CreateTextField (form1, "tmpText", av, ac);
  kids [i++] = tmp_browse = XmCreatePushButtonGadget(form1,"tmpBrowse",av,ac);


  XtVaSetValues (telnet_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, telnet_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, telnet_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, telnet_text,
                 0);
  XtVaSetValues (tn3270_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, tn3270_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, tn3270_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, tn3270_text,
                 0);
  XtVaSetValues (rlogin_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, rlogin_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, rlogin_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, rlogin_text,
                 0);
  XtVaSetValues (rlogin_user_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, rlogin_user_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, rlogin_user_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, rlogin_user_text,
                 0);
  XtVaSetValues (tmp_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, tmp_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, tmp_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, tmp_text,
                 0);

  XtVaSetValues (telnet_text,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);
  XtVaSetValues (tn3270_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, telnet_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, telnet_text,
                 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNrightWidget, telnet_text,
                 0);
  XtVaSetValues (rlogin_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, tn3270_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, tn3270_text,
                 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNrightWidget, tn3270_text,
                 0);
  XtVaSetValues (rlogin_user_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, rlogin_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, rlogin_text,
                 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNrightWidget, rlogin_text,
                 0);
  XtVaSetValues (tmp_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, rlogin_user_text,
                 XmNbottomAttachment, XmATTACH_FORM,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, rlogin_user_text,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNrightOffset, tmp_browse->core.width + 10,
                 0);
  XtVaSetValues (tmp_browse,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, tmp_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);

  XtVaSetValues (tmp_browse, XmNheight, tmp_text->core.height, 0);

  fep->telnet_text = telnet_text;
  fep->tn3270_text = tn3270_text;
  fep->rlogin_text = rlogin_text;
  fep->rlogin_user_text = rlogin_user_text;
  fep->tmp_text = tmp_text;

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  if (fe_globalData.nonterminal_text_translations)
    {
      XtOverrideTranslations (telnet_text,
                              fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (tn3270_text,
                              fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (rlogin_text,
                              fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (rlogin_user_text,
                              fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (tmp_text,
                              fe_globalData.nonterminal_text_translations);
    }

  fe_attach_field_to_labels (telnet_text,  tmp_label,
                             telnet_label, tn3270_label,
                             rlogin_label, rlogin_user_label,
                             0);

  XtAddCallback (tmp_browse, XmNactivateCallback, fe_browse_tmp_cb, fep);

  /* Manage the frames */
  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_helpers_page (MWContext *context, struct fe_prefs_data *fep)
{
  /* Create the HELPER page */
  fep->helpers = fe_helpers_make_helpers_page(fep->context, fep->helpers_selector);
}

static void
fe_make_images_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget colors_label, dither_p, closest_p, auto_p;
  Widget display_label, while_p, after_p;
  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->images_selector, "images", av, ac);
  XtManageChild (form);
  fep->images_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "imagesFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "imagesBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "imagesBox", av, ac);
  ac = 0;
  i = 0;
  kids [i++] = colors_label = XmCreateLabelGadget (form1, "colors", av, ac);
  kids [i++] = auto_p = XmCreateToggleButtonGadget (form1, "auto", av, ac);
  kids [i++] = dither_p = XmCreateToggleButtonGadget (form1, "dither", av, ac);
  kids [i++] = closest_p= XmCreateToggleButtonGadget (form1, "closest",av, ac);
  kids [i++] = display_label = XmCreateLabelGadget (form1, "display", av, ac);
  kids [i++] = while_p = XmCreateToggleButtonGadget (form1, "while", av, ac);
  kids [i++] = after_p = XmCreateToggleButtonGadget (form1, "after", av, ac);

  XtVaSetValues (colors_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, auto_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, auto_p,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, auto_p,
		 0);
  XtVaSetValues (auto_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (dither_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, auto_p,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, auto_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (closest_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, dither_p,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, dither_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (display_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, while_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, while_p,
		 XmNleftAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (while_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, closest_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, closest_p,
		 0);
  XtVaSetValues (after_p,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, while_p,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  XtAddCallback (auto_p, XmNvalueChangedCallback,
		 fe_cache_dither_toggle_cb, fep);
  XtAddCallback (dither_p, XmNvalueChangedCallback,
		 fe_cache_dither_toggle_cb, fep);
  XtAddCallback (closest_p, XmNvalueChangedCallback,
		 fe_cache_dither_toggle_cb, fep);
  XtAddCallback (while_p, XmNvalueChangedCallback,
		 fe_cache_display_toggle_cb, fep);
  XtAddCallback (after_p, XmNvalueChangedCallback,
		 fe_cache_display_toggle_cb, fep);

  fe_attach_field_to_labels (auto_p, colors_label, display_label, 0);

  fep->auto_p = auto_p;
  fep->dither_p = dither_p;
  fep->closest_p = closest_p;
  fep->while_loading_p = while_p;
  fep->after_loading_p = after_p;


  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

#ifdef REQUEST_LANGUAGE
static void
fe_make_languages_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget list1, right, left;
  Widget others, text, list2, up, down;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->languages_selector, "languages", av, ac);
  XtManageChild (form);
  fep->languages_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "languagesFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "languagesBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "languagesBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNvisibleItemCount, 10); ac++;
  list1 = XmCreateScrolledList (form1, "list1", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNarrowDirection, XmARROW_RIGHT); ac++;
  right = XmCreateArrowButtonGadget (form1, "right", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNarrowDirection, XmARROW_LEFT); ac++;
  left = XmCreateArrowButtonGadget (form1, "left", av, ac);

  ac = 0;
  others = XmCreateLabelGadget (form1, "others", av, ac);

  ac = 0;
  text = XmCreateTextField (form1, "text", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNvisibleItemCount, 10); ac++;
  list2 = XmCreateScrolledList (form1, "list2", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNarrowDirection, XmARROW_UP); ac++;
  up = XmCreateArrowButtonGadget (form1, "up", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNarrowDirection, XmARROW_DOWN); ac++;
  down = XmCreateArrowButtonGadget (form1, "down", av, ac);

  XtVaSetValues (list1,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (right,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, list2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, list1,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (left,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, right,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, list1,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (others,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, right,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, others,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (list2,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, right,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (up,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, list2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, list2,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (down,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, list2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, up,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtManageChild (list1);

  XtManageChild (form1);

  XtManageChild (label1);
  XtManageChild (frame1);
}
#endif /* REQUEST_LANGUAGE */

/*======================== Mail/News Preferences ========================*/


/* appearance of mail and news windows
 */

static void
fe_make_appearance_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget label2, frame2, form2;
  Widget fixed_label, fixed_message_font_p, var_message_font_p;
  Widget quoted_label;
  Widget qstyle_label, plain1_p, bold_p, italic_p, bold_italic_p;
  Widget qsize_label, plain2_p, bigger_p, smaller_p;
  Widget qcolor_label, qcolor_text;
  Widget mPane_label, mail_horiz_p, mail_vert_p, mail_stack_p, mail_tall_p;
  Widget nPane_label, news_horiz_p, news_vert_p, news_stack_p, news_tall_p;
  Widget kids [60];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->appearance_selector, "appearance", av, ac);
  XtManageChild (form);
  fep->appearance_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "msgFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form1 = XmCreateForm (frame1, "msgBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame1); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "layoutFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form2 = XmCreateForm (frame2, "layoutBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "msgBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = qstyle_label = XmCreateLabelGadget(form1, "qstyleLabel", av,ac);
  kids [i++] = plain1_p = XmCreateToggleButtonGadget (form1, "plain", av, ac);
  kids [i++] = bold_p = XmCreateToggleButtonGadget (form1, "bold", av, ac);
  kids [i++] = italic_p = XmCreateToggleButtonGadget (form1, "italic", av, ac);
  kids [i++] = bold_italic_p = XmCreateToggleButtonGadget (form1, "boldItalic",
                                                           av, ac);
  kids [i++] = qsize_label = XmCreateLabelGadget(form1, "qsizeLabel", av,ac);
  kids [i++] = plain2_p = XmCreateToggleButtonGadget (form1, "normal", av, ac);
  kids [i++] = bigger_p = XmCreateToggleButtonGadget (form1, "bigger", av, ac);
  kids [i++] = smaller_p = XmCreateToggleButtonGadget (form1, "smaller",av,ac);

  kids [i++] = qcolor_label = XmCreateLabelGadget(form1, "qcolorLabel", av,ac);
  kids [i++] = qcolor_text = fe_CreateTextField (form1, "qcolorText", av, ac);

  kids [i++] = fixed_label = XmCreateLabelGadget(form1, "fixedLabel", av,ac);
  kids [i++] = quoted_label = XmCreateLabelGadget(form1, "quotedLabel", av,ac);
  kids [i++] = fixed_message_font_p = XmCreateToggleButtonGadget (form1,
							"fixedMsgFont",av,ac);
  kids [i++] = var_message_font_p = XmCreateToggleButtonGadget (form1,
							"varMsgFont",av,ac);

  XtVaSetValues (fixed_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNtopOffset, 10,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 10,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (fixed_message_font_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, fixed_label,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, plain1_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (var_message_font_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, fixed_message_font_p,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, fixed_message_font_p,
                 XmNleftAttachment, XmATTACH_WIDGET,
                 XmNleftWidget, fixed_message_font_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (quoted_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNtopOffset, 90,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 10,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (qstyle_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, plain1_p,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, plain1_p,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, plain1_p,
                 0);
  XtVaSetValues (plain1_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, quoted_label,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 15,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (bold_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, plain1_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 30,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (italic_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, bold_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 45,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (bold_italic_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, italic_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 60,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (qsize_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, plain2_p,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, plain2_p,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, plain2_p,
                 0);
  XtVaSetValues (plain2_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, plain1_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, plain1_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (bigger_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, plain2_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, bold_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (smaller_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, bigger_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, italic_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (qcolor_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, qcolor_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, qcolor_text,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, qcolor_text,
                 0);
  XtVaSetValues (qcolor_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, plain2_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, plain2_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  XtAddCallback (plain1_p, XmNvalueChangedCallback,
                 fe_styles_cite_font_toggle_cb, fep);
  XtAddCallback (bold_p, XmNvalueChangedCallback,
                 fe_styles_cite_font_toggle_cb, fep);
  XtAddCallback (italic_p, XmNvalueChangedCallback,
                 fe_styles_cite_font_toggle_cb, fep);
  XtAddCallback (bold_italic_p, XmNvalueChangedCallback,
                 fe_styles_cite_font_toggle_cb, fep);

  XtAddCallback (plain2_p, XmNvalueChangedCallback,
                 fe_styles_cite_size_toggle_cb, fep);
  XtAddCallback (bigger_p, XmNvalueChangedCallback,
                 fe_styles_cite_size_toggle_cb, fep);
  XtAddCallback (smaller_p, XmNvalueChangedCallback,
                 fe_styles_cite_size_toggle_cb, fep);

  XtAddCallback (fixed_message_font_p, XmNvalueChangedCallback,
                 fe_msg_font_toggle_cb, fep);
  XtAddCallback (var_message_font_p, XmNvalueChangedCallback,
                 fe_msg_font_toggle_cb, fep);

  fep->fixed_message_font_p = fixed_message_font_p;
  fep->var_message_font_p = var_message_font_p;

  fep->cite_plain_p = plain1_p;
  fep->cite_bold_p = bold_p;
  fep->cite_italic_p = italic_p;
  fep->cite_bold_italic_p = bold_italic_p;

  fep->cite_normal_p = plain2_p;
  fep->cite_bigger_p = bigger_p;
  fep->cite_smaller_p = smaller_p;

  fep->cite_color_text = qcolor_text;

  /* mail/news pane layout */

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "layoutBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = mPane_label = XmCreateLabelGadget(form2, "mPaneLabel", av,ac);
  kids [i++] = mail_horiz_p = XmCreateToggleButtonGadget (form2, "mHoriz", av, ac);
  kids [i++] = mail_vert_p = XmCreateToggleButtonGadget (form2, "mVert", av, ac);
  kids [i++] = mail_stack_p = XmCreateToggleButtonGadget (form2, "mStack",av,ac);
  kids [i++] = mail_tall_p = XmCreateToggleButtonGadget (form2, "mTall",av,ac);

  kids [i++] = nPane_label = XmCreateLabelGadget(form2, "nPaneLabel", av,ac);
  kids [i++] = news_horiz_p = XmCreateToggleButtonGadget (form2, "nHoriz", av, ac);
  kids [i++] = news_vert_p = XmCreateToggleButtonGadget (form2, "nVert", av, ac);
  kids [i++] = news_stack_p = XmCreateToggleButtonGadget (form2, "nStack",av,ac);
  kids [i++] = news_tall_p = XmCreateToggleButtonGadget (form2, "nTall",av,ac);

  XtVaSetValues (mPane_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mail_horiz_p,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mail_horiz_p,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, mail_horiz_p,
                 0);
  XtVaSetValues (mail_horiz_p,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 15,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (mail_vert_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mail_horiz_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 40,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (mail_stack_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mail_horiz_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 65,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (mail_tall_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mail_horiz_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_POSITION,
                 XmNleftPosition, 90,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (nPane_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, news_horiz_p,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, news_horiz_p,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, news_horiz_p,
                 0);
  XtVaSetValues (news_horiz_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, mail_horiz_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, mail_horiz_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (news_vert_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, news_horiz_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, mail_vert_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (news_stack_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, news_vert_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, mail_stack_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (news_tall_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, news_vert_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, mail_tall_p,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  XtAddCallback (mail_horiz_p, XmNvalueChangedCallback,
                 fe_mail_pane_layout_toggle_cb, fep);
  XtAddCallback (mail_vert_p, XmNvalueChangedCallback,
                 fe_mail_pane_layout_toggle_cb, fep);
  XtAddCallback (mail_stack_p, XmNvalueChangedCallback,
                 fe_mail_pane_layout_toggle_cb, fep);
  XtAddCallback (mail_tall_p, XmNvalueChangedCallback,
                 fe_mail_pane_layout_toggle_cb, fep);

  XtAddCallback (news_horiz_p, XmNvalueChangedCallback,
                 fe_news_pane_layout_toggle_cb, fep);
  XtAddCallback (news_vert_p, XmNvalueChangedCallback,
                 fe_news_pane_layout_toggle_cb, fep);
  XtAddCallback (news_stack_p, XmNvalueChangedCallback,
                 fe_news_pane_layout_toggle_cb, fep);
  XtAddCallback (news_tall_p, XmNvalueChangedCallback,
                 fe_news_pane_layout_toggle_cb, fep);

  fep->mail_horiz_p = mail_horiz_p;
  fep->mail_vert_p = mail_vert_p;
  fep->mail_stack_p = mail_stack_p;
  fep->mail_tall_p = mail_tall_p;
  fep->news_horiz_p = news_horiz_p;
  fep->news_vert_p = news_vert_p;
  fep->news_stack_p = news_stack_p;
  fep->news_tall_p = news_tall_p;

  i = 0;
  kids [i++] = frame1;
  kids [i++] = frame2;
  XtManageChildren (kids, i);
}


/* behavior of composition window
 */

static void
fe_make_compose_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget eightbit_label, eightbit_toggle, qp_toggle;
#ifdef PREFS_QUEUED_DELIVERY
  Widget deliver_label, deliverAuto_toggle, deliverQ_toggle;
#endif

  Widget label2, frame2, form2;
  Widget mMailOut_label, mMailOutSelf_toggle, mMailOutOther_label,
	 mMailOutOther_text;
  Widget nMailOut_label, nMailOutSelf_toggle, nMailOutOther_label,
	 nMailOutOther_text;

  Widget label3, frame3, form3;
  Widget mCopyOut_label, mCopyOut_text, mCopyOut_browse;
  Widget nCopyOut_label, nCopyOut_text, nCopyOut_browse;

  Widget autoquote_toggle;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->compose_selector, "compose", av, ac);
  XtManageChild (form);
  fep->compose_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  frame1 = XmCreateFrame (form, "composeFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "composeBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "composeBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = eightbit_label = XmCreateLabelGadget (form1, "8bitLabel",
						     av, ac);
  kids [i++] = eightbit_toggle = XmCreateToggleButtonGadget (form1,
							     "8bitToggle",
							     av, ac);
  kids [i++] = qp_toggle   = XmCreateToggleButtonGadget (form1, "qpToggle",
							 av, ac);
#ifdef PREFS_QUEUED_DELIVERY
  kids [i++] = deliver_label = XmCreateLabelGadget (form1, "deliverLabel",
						     av, ac);
  kids [i++] = deliverAuto_toggle = XmCreateToggleButtonGadget (form1,
							"deliverAutoToggle",
							av, ac);
  kids [i++] = deliverQ_toggle = XmCreateToggleButtonGadget (form1,
							"deliverQToggle",
							av, ac);
#endif

  XtVaSetValues (eightbit_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, eightbit_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, eightbit_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, eightbit_toggle,
		 0);
  XtVaSetValues (eightbit_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (qp_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, eightbit_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, eightbit_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, eightbit_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#ifdef PREFS_QUEUED_DELIVERY
  XtVaSetValues (deliver_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, deliverAuto_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, deliverAuto_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, deliverAuto_toggle,
		 0);
  XtVaSetValues (deliverAuto_toggle,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, eightbit_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, eightbit_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (deliverQ_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, deliverAuto_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, deliverAuto_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, deliverAuto_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#endif
  XtAddCallback (eightbit_toggle, XmNvalueChangedCallback,
		 fe_mail_eightbit_toggle_cb, fep);
  XtAddCallback (qp_toggle, XmNvalueChangedCallback,
		 fe_mail_eightbit_toggle_cb, fep);
#ifdef PREFS_QUEUED_DELIVERY
  XtAddCallback (deliverAuto_toggle, XmNvalueChangedCallback,
		 fe_mail_eightbit_toggle_cb, fep);
  XtAddCallback (deliverQ_toggle, XmNvalueChangedCallback,
		 fe_mail_eightbit_toggle_cb, fep);
#endif

  fep->eightbit_toggle = eightbit_toggle;
  fep->qp_toggle = qp_toggle;
#ifdef PREFS_QUEUED_DELIVERY
  fep->deliverAuto_toggle = deliverAuto_toggle;
  fep->deliverQ_toggle = deliverQ_toggle;
#endif

  fe_attach_field_to_labels (eightbit_toggle, eightbit_label,
#ifdef PREFS_QUEUED_DELIVERY
			     deliver_label,
#endif
			     0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  /* Email a copy of outgoing messages to */

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame1); ac++;
  frame2 = XmCreateFrame (form, "mMailOutFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "mMailOutBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "mMailOutBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = mMailOut_label = XmCreateLabelGadget (form2, "mMailOutLabel",
						     av, ac);
  kids [i++] = mMailOutSelf_toggle = XmCreateToggleButtonGadget (form2,
						"mMailOutSelfToggle",
						av, ac);
  kids [i++] = mMailOutOther_label = XmCreateLabelGadget (form2,
						"mMailOutOtherLabel",
						av, ac);
  kids [i++] = mMailOutOther_text = fe_CreateTextField (form2,
						"mMailOutOtherText",
						av, ac);
  kids [i++] = nMailOut_label = XmCreateLabelGadget (form2, "nMailOutLabel",
						     av, ac);
  kids [i++] = nMailOutSelf_toggle = XmCreateToggleButtonGadget (form2,
						"nMailOutSelfToggle",
						av, ac);
  kids [i++] = nMailOutOther_label = XmCreateLabelGadget (form2,
						"nMailOutOtherLabel",
						av, ac);
  kids [i++] = nMailOutOther_text = fe_CreateTextField (form2,
						"nMailOutOtherText",
						av, ac);

  /* HACK the heights. Assumes textwidget is taller. */
  mMailOut_label->core.height = mMailOutSelf_toggle->core.height =
	mMailOutOther_label->core.height = mMailOutOther_text->core.height;
  nMailOut_label->core.height = nMailOutSelf_toggle->core.height =
	nMailOutOther_label->core.height = nMailOutOther_text->core.height;

  XtVaSetValues (mMailOut_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mMailOutSelf_toggle,
		 0);
  XtVaSetValues (mMailOutSelf_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (mMailOutOther_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mMailOutSelf_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (mMailOutOther_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mMailOutOther_label,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (nMailOut_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, nMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, nMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, nMailOutSelf_toggle,
		 0);
  XtVaSetValues (nMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mMailOutSelf_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nMailOutOther_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, nMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, nMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, nMailOutSelf_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nMailOutOther_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, nMailOutSelf_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, nMailOutSelf_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, nMailOutOther_label,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  fep->mMailOutSelf_toggle  = mMailOutSelf_toggle;
  fep->mMailOutOther_text   = mMailOutOther_text;
  fep->nMailOutSelf_toggle  = nMailOutSelf_toggle;
  fep->nMailOutOther_text   = nMailOutOther_text;

  fe_attach_field_to_labels (mMailOutSelf_toggle, mMailOut_label,
				nMailOut_label, 0);

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  /*** frame3: Copy outgoing messages to ***/

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame2); ac++;
  frame3 = XmCreateFrame (form, "mCopyOutFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form3 = XmCreateForm (frame3, "mCopyOutBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label3 = XmCreateLabelGadget (frame3, "mCopyOutBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = mCopyOut_label = XmCreateLabelGadget (form3, "mCopyOutLabel",
						     av, ac);
  kids [i++] = mCopyOut_text = fe_CreateTextField (form3,
						"mCopyOutText",
						av, ac);
  kids [i++] = mCopyOut_browse = XmCreatePushButtonGadget(form3,
						"mCopyOutBrowse",av,ac);

  kids [i++] = nCopyOut_label = XmCreateLabelGadget (form3, "nCopyOutLabel",
						     av, ac);
  kids [i++] = nCopyOut_text = fe_CreateTextField (form3,
						"nCopyOutText",
						av, ac);
  kids [i++] = nCopyOut_browse = XmCreatePushButtonGadget(form3,
						"nCopyOutBrowse",av,ac);


  /* HACK the heights. Assumes textwidget is taller. */
  mCopyOut_browse->core.height = mCopyOut_label->core.height =
					mCopyOut_text->core.height;
  nCopyOut_browse->core.height = nCopyOut_label->core.height =
					nCopyOut_text->core.height;

  XtVaSetValues (mCopyOut_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mCopyOut_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mCopyOut_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mCopyOut_text,
		 0);
  XtVaSetValues (mCopyOut_text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, mCopyOut_browse->core.width + 10,
		 0);
  XtVaSetValues (mCopyOut_browse,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mCopyOut_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (nCopyOut_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, nCopyOut_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, nCopyOut_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, nCopyOut_text,
		 0);
  XtVaSetValues (nCopyOut_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mCopyOut_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mCopyOut_text,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, nCopyOut_browse->core.width + 10,
		 0);
  XtVaSetValues (nCopyOut_browse,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, nCopyOut_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  fep->mCopyOut_text  = mCopyOut_text;
  fep->nCopyOut_text  = nCopyOut_text;

  fe_attach_field_to_labels (mCopyOut_text, mCopyOut_label, nCopyOut_label, 0);

  XtVaSetValues (mCopyOut_browse, XmNuserData, mCopyOut_text, 0);
  XtAddCallback (mCopyOut_browse, XmNactivateCallback, fe_browse_to_text_cb,
			fep);
  XtVaSetValues (nCopyOut_browse, XmNuserData, nCopyOut_text, 0);
  XtAddCallback (nCopyOut_browse, XmNactivateCallback, fe_browse_to_text_cb,
			fep);

  XtManageChildren (kids, i);
  XtManageChild (label3);
  XtManageChild (form3);

  /*** Automatically quote messages when replying */
  ac = 0;
  i = 0;
  kids [i++] = autoquote_toggle = XmCreateToggleButtonGadget (form,
				"autoQuoteToggle", av, ac);
  XtVaSetValues (autoquote_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 20,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, frame3,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fep->autoquote_toggle  = autoquote_toggle;

  XtManageChildren (kids, i);

  /*************/
  i = 0;
  kids [i++] = frame1;
  kids [i++] = frame2;
  kids [i++] = frame3;
  XtManageChildren (kids, i);
}

static void
fe_make_servers_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;

  Widget label2, frame2, form2;
  Widget smtp_label, smtp_text;

  Widget label3, frame3, form3;
  Widget srvr_label, srvr_text;
  Widget pop_toggle;
  Widget movemail_text, movemail_browse;
  Widget builtin_toggle, external_toggle;
  Widget user_label, user_text;

  Widget limit_label, no_limit, msg_limit;
  Widget msg_label, msg_remove, msg_leave;
  Widget check_label, check_every, check_never;
  Widget check_text, check_suffix, limit_text, limit_suffix;
  Widget maildir_label, maildir_text, maildir_browse;

  Widget label4, frame4, form4;
  Widget newshost_label, newshost_text;
  Widget newsrc_label, newsrc_text, newsrc_browse;
#ifdef PREFS_NEWS_MAX
  Widget newsmax_label, newsmax_text, newsmax_suff;
#endif

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form = XmCreateForm (fep->dirs_selector, "dirs", av, ac);
  XtManageChild (form);
  fep->dirs_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "outMailFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "outMailBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "outMailBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame2); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame3 = XmCreateFrame (form, "inMailFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form3 = XmCreateForm (frame3, "inMailBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label3 = XmCreateLabelGadget (frame3, "inMailBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame3); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame4 = XmCreateFrame (form, "newsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form4 = XmCreateForm (frame4, "newsBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label4 = XmCreateLabelGadget (frame4, "newsBoxLabel", av, ac);

  i = 0;
  ac = 0;
  kids [i++] = smtp_label = XmCreateLabelGadget (form2, "smtpLabel", av,ac);
  kids [i++] = smtp_text = fe_CreateTextField (form2, "smtpText", av, ac);

  XtVaSetValues (smtp_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, smtp_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, smtp_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, smtp_text,
		 0);
  XtVaSetValues (smtp_text,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_FORM,
		 XmNbottomOffset, 10,
                 XmNrightAttachment, XmATTACH_FORM,
		 0);

  fe_attach_field_to_labels (smtp_text, smtp_label, 0);

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);


  i = 0;
  ac = 0;
  kids [i++] = srvr_label = XmCreateLabelGadget (form3, "srvrLabel", av,ac);
  kids [i++] = srvr_text = fe_CreateTextField (form3, "srvrText", av, ac);

  kids [i++] = pop_toggle   = XmCreateToggleButtonGadget (form3, "popToggle",
                                                         av, ac);

  kids [i++] = movemail_text = fe_CreateTextField (form3,
						"movemailText", av, ac);
  kids [i++] = movemail_browse = XmCreatePushButtonGadget(form3,
						"movemailBrowse",av,ac);
  kids [i++] = builtin_toggle = XmCreateToggleButtonGadget (form3,
						"builtinToggle", av, ac);
  kids [i++] = external_toggle = XmCreateToggleButtonGadget (form3,
						"externalToggle", av, ac);

  kids [i++] = user_label = XmCreateLabelGadget (form3,
						"userLabel", av,ac);
  kids [i++] = user_text = fe_CreateTextField (form3,
						"userText", av, ac);

  kids [i++] = limit_label = XmCreateLabelGadget (form3, "limitLabel",
						     av, ac);
  kids [i++] = no_limit = XmCreateToggleButtonGadget (form3, "noLimit",
							     av, ac);
  kids [i++] = msg_limit = XmCreateToggleButtonGadget (form3, "msgLimit",
							     av, ac);
  kids [i++] = limit_text = fe_CreateTextField (form3, "limitText", av, ac);
  kids [i++] = limit_suffix = XmCreateLabelGadget (form3, "limitSuffix",
						     av, ac);

  /* This assumes that the text widget's height is the largest */
  limit_label->core.height = no_limit->core.height = 
    msg_limit->core.height = limit_suffix->core.height = limit_text->core.height;

  kids [i++] = msg_label = XmCreateLabelGadget (form3, "msgLabel",
						     av, ac);
  kids [i++] = msg_remove = XmCreateToggleButtonGadget (form3, "msgRemove",
							     av, ac);
  kids [i++] = msg_leave = XmCreateToggleButtonGadget (form3, "msgLeave",
							     av, ac);

  kids [i++] = check_label = XmCreateLabelGadget (form3, "checkLabel",
						     av, ac);
  kids [i++] = check_every = XmCreateToggleButtonGadget (form3, "checkEvery",
							     av, ac);
  kids [i++] = check_never = XmCreateToggleButtonGadget (form3, "checkNever",
							     av, ac);
  kids [i++] = check_text = fe_CreateTextField (form3, "checkText", av, ac);
  kids [i++] = check_suffix = XmCreateLabelGadget (form3, "checkSuffix",
						     av, ac);
  /* This assumes that the text widget's height is the largest */
  check_label->core.height = check_every->core.height = check_never->core.height =
    check_suffix->core.height = check_text->core.height;

  kids [i++] = maildir_label = XmCreateLabelGadget (form3, "mailDirLabel",av,ac);
  kids [i++] = maildir_text = fe_CreateTextField(form3, "mailDirText",av,ac);
  kids [i++] = maildir_browse = XmCreatePushButtonGadget (form3, "mailDirBrowse",
                                                         av, ac);

  /* This assumes that the text widget's height is the largest */
  maildir_label->core.height = maildir_browse->core.height 
    = maildir_text->core.height;

  XtVaSetValues (pop_toggle,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, srvr_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, srvr_text,
                 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (builtin_toggle,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, user_label,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, pop_toggle,
                 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (external_toggle,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, movemail_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, movemail_text,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, builtin_toggle,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, movemail_text,
                 XmNalignment, XmALIGNMENT_BEGINNING,
                 0);
  XtVaSetValues (srvr_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, srvr_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, srvr_text,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, srvr_text,
		 0);
  XtVaSetValues (srvr_text,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (user_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, user_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, user_text,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, user_text,
		 0);
  XtVaSetValues (user_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, srvr_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, srvr_text,
                 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (movemail_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, builtin_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, user_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, movemail_browse->core.width + 10,
		 0);
  XtVaSetValues (movemail_browse,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, movemail_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, movemail_text,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (limit_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, no_limit,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, no_limit,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, no_limit,
		 0);
  XtVaSetValues (no_limit,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, movemail_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, movemail_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (msg_limit,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, no_limit,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, no_limit,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, no_limit,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (limit_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, no_limit,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, no_limit,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, msg_limit,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (limit_suffix,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, no_limit,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, no_limit,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, limit_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (msg_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, msg_remove,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, msg_remove,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, msg_remove,
		 0);
  XtVaSetValues (msg_remove,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, no_limit,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, no_limit,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (msg_leave,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, msg_remove,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, msg_remove,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, msg_remove,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (check_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, check_every,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, check_every,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, check_every,
		 0);
  XtVaSetValues (check_every,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, msg_remove,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, msg_remove,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (check_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, check_every,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, check_every,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, check_every,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (check_suffix,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, check_every,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, check_every,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, check_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (check_never,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, check_every,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, check_every,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, check_suffix,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (maildir_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, maildir_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, maildir_text,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, maildir_text,
		 0);
  XtVaSetValues (maildir_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, check_every,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, check_every,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, maildir_browse->core.width + 10,
		 0);
  XtVaSetValues (maildir_browse,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, maildir_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, maildir_text,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtAddCallback (pop_toggle, XmNvalueChangedCallback,
                 fe_mail_pop_toggle_cb, fep);
  XtAddCallback (builtin_toggle, XmNvalueChangedCallback,
                 fe_mail_pop_toggle_cb, fep);
  XtAddCallback (external_toggle, XmNvalueChangedCallback,
                 fe_mail_pop_toggle_cb, fep);
  XtAddCallback (movemail_browse, XmNactivateCallback,
		 fe_browse_movemail_cb, fep);

  XtAddCallback (no_limit, XmNvalueChangedCallback,
		 fe_msg_limit_toggle_cb, fep);
  XtAddCallback (msg_limit, XmNvalueChangedCallback,
		 fe_msg_limit_toggle_cb, fep);
  XtAddCallback (msg_remove, XmNvalueChangedCallback,
		 fe_msg_on_server_toggle_cb, fep);
  XtAddCallback (msg_leave, XmNvalueChangedCallback,
		 fe_msg_on_server_toggle_cb, fep);
  XtAddCallback (check_every, XmNvalueChangedCallback,
		 fe_msg_check_toggle_cb, fep);
  XtAddCallback (check_never, XmNvalueChangedCallback,
		 fe_msg_check_toggle_cb, fep);

  XtAddCallback (maildir_browse, XmNactivateCallback,
		 fe_browse_maildir_cb, fep);

  XtManageChildren (kids, i);
  XtManageChild (label3);
  XtManageChild (form3);

  fe_attach_field_to_labels (srvr_text, pop_toggle, builtin_toggle,
			     external_toggle, no_limit, msg_remove,
			     check_every, maildir_text, 0);

  if (fe_globalData.nonterminal_text_translations)
    {
      XtOverrideTranslations (smtp_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (srvr_text,
			      fe_globalData.nonterminal_text_translations);
    }

  /**/
  ac = 0;
  i = 0;
  kids [i++] = newshost_label = XmCreateLabelGadget (form4, "newshostLabel",
                                                     av, ac);
  kids [i++] = newsrc_label = XmCreateLabelGadget (form4, "newsrcLabel",av,ac);
  kids [i++] = newshost_text = fe_CreateTextField(form4, "newshostText",av,ac);
  kids [i++] = newsrc_text = fe_CreateTextField (form4, "newsrcText", av, ac);
  kids [i++] = newsrc_browse = XmCreatePushButtonGadget (form4, "newsrcBrowse",
                                                         av, ac);
#ifdef PREFS_NEWS_MAX
  kids [i++] = newsmax_label= XmCreateLabelGadget (form4,"newsMaxLabel",av,ac);
  kids [i++] = newsmax_text = fe_CreateTextField(form4, "newsMaxText", av, ac);
  kids [i++] = newsmax_suff = XmCreateLabelGadget(form4,"newsMaxSuffix",av,ac);
#endif

  XtVaSetValues (newshost_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, newshost_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, newshost_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, newshost_text,
                 0);
  XtVaSetValues (newsrc_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, newsrc_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, newsrc_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, newsrc_text,
                 0);
#ifdef PREFS_NEWS_MAX
  XtVaSetValues (newsmax_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, newsmax_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, newsmax_text,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_WIDGET,
                 XmNrightWidget, newsmax_text,
                 0);
#endif

  XtVaSetValues (newshost_text,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);
  XtVaSetValues (newsrc_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, newshost_text,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, newshost_text,
                 XmNrightAttachment, XmATTACH_FORM,
                 XmNrightOffset, newsrc_browse->core.width + 10,
                 0);
  XtVaSetValues (newsrc_browse,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, newsrc_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, newsrc_text,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);
#ifdef PREFS_NEWS_MAX
  XtVaSetValues (newsmax_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
                 XmNtopWidget, newsrc_text,
                 XmNbottomAttachment, XmATTACH_FORM,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNleftWidget, newsrc_text,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (newsmax_suff,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNtopWidget, newsmax_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                 XmNbottomWidget, newsmax_text,
                 XmNleftAttachment, XmATTACH_WIDGET,
                 XmNleftWidget, newsmax_text,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);
#endif
  fe_attach_field_to_labels (newshost_text, newshost_label, newsrc_label,
#ifdef PREFS_NEWS_MAX
                             newsmax_label,
#endif
                             0);

  XtVaSetValues (newsrc_browse, XmNheight, newsrc_text->core.height, 0);
  XtAddCallback (newsrc_browse, XmNactivateCallback, fe_browse_newsrc_cb, fep);

  fep->newshost_text = newshost_text;
  fep->newsrc_text = newsrc_text;
#ifdef PREFS_NEWS_MAX
  fep->newsmax_text = newsmax_text;
#endif

  XtManageChildren (kids, i);
  XtManageChild (label4);
  XtManageChild (form4);

  if (fe_globalData.nonterminal_text_translations)
    {
      XtOverrideTranslations (newshost_text,
                              fe_globalData.nonterminal_text_translations);
#ifdef PREFS_NEWS_MAX
      XtOverrideTranslations (newsrc_text,
                              fe_globalData.nonterminal_text_translations);
#endif
    }


  /**/

  fep->user_text = user_text;
  fep->smtp_text = smtp_text;
  fep->srvr_text = srvr_text;
  fep->pop_toggle = pop_toggle;
  fep->movemail_text = movemail_text;
  fep->builtin_toggle = builtin_toggle;
  fep->external_toggle = external_toggle;

  fep->limit_text = limit_text;
  fep->no_limit = no_limit;
  fep->msg_limit = msg_limit;
  fep->msg_remove = msg_remove;
  fep->msg_leave = msg_leave;
  fep->check_text = check_text;
  fep->check_every = check_every;
  fep->check_never = check_never;
  fep->maildir_text = maildir_text;

  i = 0;
  kids [i++] = frame2;
  kids [i++] = frame3;
  kids [i++] = frame4;
  XtManageChildren (kids, i);
}

static void
fe_make_identity_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget name_label, name_text;
  Widget mail_label, mail_text;
  Widget org_label, org_text;
#ifdef PREFS_SIG
  Widget sig_label, sig_text, sig_browse;
#endif
  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->identity_selector, "identity", av, ac);
  XtManageChild (form);
  fep->identity_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "identityFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form1 = XmCreateForm (frame1, "identityBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "identityBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = name_label = XmCreateLabelGadget (form1, "nameLabel", av,ac);
  kids [i++] = mail_label = XmCreateLabelGadget (form1, "mailLabel", av,ac);
  kids [i++] = org_label  = XmCreateLabelGadget (form1, "orgLabel", av,ac);
#ifdef PREFS_SIG
  kids [i++] = sig_label  = XmCreateLabelGadget (form1, "sigLabel", av,ac);
#endif
  kids [i++] = name_text = fe_CreateTextField (form1, "nameText", av, ac);
  kids [i++] = mail_text = fe_CreateTextField (form1, "mailText", av, ac);
  kids [i++] = org_text  = fe_CreateTextField (form1, "orgText", av, ac);
#ifdef PREFS_SIG
  kids [i++] = sig_text  = fe_CreateTextField (form1, "sigText", av, ac);
  kids [i++] = sig_browse= XmCreatePushButtonGadget (form1, "sigBrowse",av,ac);
#endif

  XtVaSetValues (name_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, name_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, name_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, name_text,
		 0);
  XtVaSetValues (mail_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mail_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mail_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mail_text,
		 0);
  XtVaSetValues (org_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, org_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, org_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, org_text,
		 0);
#ifdef PREFS_SIG
  XtVaSetValues (sig_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, sig_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, sig_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, sig_text,
		 0);
#endif

  XtVaSetValues (name_text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (mail_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, name_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, name_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (org_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mail_text,
#ifdef PREFS_SIG
		 XmNbottomAttachment, XmATTACH_NONE,
#else
		 XmNbottomAttachment, XmATTACH_FORM,
#endif
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mail_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
#ifdef PREFS_SIG
  XtVaSetValues (sig_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, org_text,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNbottomOffset, 10,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, org_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, sig_browse->core.width + 10,
		 0);
  XtVaSetValues (sig_browse,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, org_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (sig_browse, XmNheight, sig_text->core.height, 0);
  XtAddCallback (sig_browse, XmNactivateCallback, fe_browse_sig_cb, fep);
#endif

  fep->user_name_text = name_text;
  fep->user_mail_text = mail_text;
  fep->user_org_text = org_text;
#ifdef PREFS_SIG
  fep->user_sig_text = sig_text;
#endif

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);


  if (fe_globalData.nonterminal_text_translations)
    {
      XtOverrideTranslations (name_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (mail_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (org_text,
			      fe_globalData.nonterminal_text_translations);
#ifdef PREFS_SIG
      XtOverrideTranslations (sig_text,
			      fe_globalData.nonterminal_text_translations);
#endif
    }

  fe_attach_field_to_labels (name_text,
			     name_label, mail_label, org_label,
#ifdef PREFS_SIG
			     sig_label,
#endif
			     0);

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_organization_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
#if PREFS_EMPTY_TRASH
  Widget emptyTrash_toggle;
#endif
  Widget rememberPswd_toggle;

  Widget label2, frame2, form2;
  Widget line1, line2, threadmail_toggle, threadnews_toggle;

  Widget label3, frame3, form3;
  Widget msort_label, mdate_toggle, mnum_toggle, msubject_toggle,
	 msender_toggle;
  Widget nsort_label, ndate_toggle, nnum_toggle, nsubject_toggle,
	 nsender_toggle;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->organization_selector, "organization", av, ac);
  XtManageChild (form);
  fep->organization_page = form;

  /*** frame1: General ***/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  frame1 = XmCreateFrame (form, "organizationFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form1 = XmCreateForm (frame1, "organizationBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "organizationBoxLabel", av, ac);

  ac = 0;
  i = 0;
#if PREFS_EMPTY_TRASH
  kids [i++] = emptyTrash_toggle = XmCreateToggleButtonGadget (form1, "trashToggle", av,ac);
#endif /* PREFS_EMPTY_TRASH */
  kids [i++] = rememberPswd_toggle = XmCreateToggleButtonGadget (form1, "passwordToggle", av,ac);

#if PREFS_EMPTY_TRASH
  XtVaSetValues (emptyTrash_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (rememberPswd_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, emptyTrash_toggle,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#else
  XtVaSetValues (rememberPswd_toggle,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
#endif

#if PREFS_EMPTY_TRASH
  fep->emptyTrash_toggle = emptyTrash_toggle;
#endif
  fep->rememberPswd_toggle = rememberPswd_toggle;

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  /*** frame2: threading ***/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame1); ac++;
  frame2 = XmCreateFrame (form, "threadFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "threadBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "threadBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = line1  = XmCreateLabelGadget (form2, "line1Label", av,ac);
  kids [i++] = line2  = XmCreateLabelGadget (form2, "line2Label", av,ac);
  kids [i++] = threadmail_toggle = XmCreateToggleButtonGadget (form2, "threadMailToggle", av, ac);
  kids [i++] = threadnews_toggle = XmCreateToggleButtonGadget (form2, "threadNewsToggle", av, ac);

  XtVaSetValues (line1,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (line2,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, line1,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (threadmail_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, line2,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (threadnews_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, threadmail_toggle,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fep->threadmail_toggle = threadmail_toggle;
  fep->threadnews_toggle = threadnews_toggle;

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  /*** frame3: sorting ***/
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame2); ac++;
  frame3 = XmCreateFrame (form, "mnSortFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  form3 = XmCreateForm (frame3, "mnSortBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label3 = XmCreateLabelGadget (frame3, "mnSortBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = msort_label  = XmCreateLabelGadget (form3, "mSortLabel", av,ac);
  kids [i++] = mdate_toggle = XmCreateToggleButtonGadget (form3, "mSortDateToggle", av, ac);
  kids [i++] = mnum_toggle = XmCreateToggleButtonGadget (form3, "mSortNumToggle", av, ac);
  kids [i++] = msubject_toggle = XmCreateToggleButtonGadget (form3, "mSortSubjectToggle", av, ac);
  kids [i++] = msender_toggle = XmCreateToggleButtonGadget (form3, "mSortSenderToggle", av, ac);
  kids [i++] = nsort_label  = XmCreateLabelGadget (form3, "nSortLabel", av,ac);
  kids [i++] = ndate_toggle = XmCreateToggleButtonGadget (form3, "nSortDateToggle", av, ac);
  kids [i++] = nnum_toggle = XmCreateToggleButtonGadget (form3, "nSortNumToggle", av, ac);
  kids [i++] = nsubject_toggle = XmCreateToggleButtonGadget (form3, "nSortSubjectToggle", av, ac);
  kids [i++] = nsender_toggle = XmCreateToggleButtonGadget (form3, "nSortSenderToggle", av, ac);

  XtAddCallback (mdate_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (mnum_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (msubject_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (msender_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (ndate_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (nnum_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (nsubject_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);
  XtAddCallback (nsender_toggle, XmNvalueChangedCallback, fe_org_toggle_cb, fep);

  XtVaSetValues (msort_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mdate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mdate_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mdate_toggle,
		 0);
  XtVaSetValues (mdate_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (mnum_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mdate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mdate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mdate_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (msubject_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mdate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mdate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mnum_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (msender_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mdate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mdate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, msubject_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nsort_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ndate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ndate_toggle,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, ndate_toggle,
		 0);
  XtVaSetValues (ndate_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mdate_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mdate_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nnum_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ndate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ndate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, ndate_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nsubject_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ndate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ndate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, nnum_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (nsender_toggle,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ndate_toggle,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ndate_toggle,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, nsubject_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fe_attach_field_to_labels (mdate_toggle, msort_label, nsort_label, 0);

  fep->mdate_toggle    = mdate_toggle;
  fep->mnum_toggle     = mnum_toggle;
  fep->msubject_toggle = msubject_toggle;
  fep->msender_toggle  = msender_toggle;
  fep->ndate_toggle    = ndate_toggle;
  fep->nnum_toggle     = nnum_toggle;
  fep->nsubject_toggle = nsubject_toggle;
  fep->nsender_toggle  = nsender_toggle;

  XtManageChildren (kids, i);
  XtManageChild (label3);
  XtManageChild (form3);

  /*** final ***/
  i = 0;
  kids [i++] = frame1;
  kids [i++] = frame2;
  kids [i++] = frame3;
  XtManageChildren (kids, i);
}

/*======================== Network Preferences ========================*/

static void
fe_make_cache_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget mem_label, mem_text, mem_k, mem_clear = 0;
#ifdef PREFS_DISK_CACHE
  Widget disk_label, disk_text, disk_k, disk_clear = 0;
  Widget disk_dir_label, disk_dir, browse;
#endif
#ifdef PREFS_VERIFY
  Widget verify_label, once_p, every_p, expired_p;
#endif
  Widget cache_ssl_p;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->cache_selector, "cache", av, ac);
  XtManageChild (form);
  fep->cache_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "cacheFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "cacheBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "cacheBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = mem_label = XmCreateLabelGadget (form1, "memoryLabel",
						    av, ac);
  kids [i++] = mem_text = fe_CreateTextField (form1, "memoryText", av, ac);
  kids [i++] = mem_k = XmCreateLabelGadget (form1, "k", av, ac);
#ifdef PREFS_CLEAR_CACHE_BUTTONS
  kids [i++] = mem_clear = XmCreatePushButtonGadget (form1, "memClear", av,ac);
  XtAddCallback (mem_clear, XmNactivateCallback, fe_clear_mem_cache_cb, fep);
#endif

  XtVaSetValues (mem_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mem_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mem_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, mem_text,
		 0);
  XtVaSetValues (mem_text,
		 XmNtopAttachment, XmATTACH_FORM,
#ifdef PREFS_DISK_CACHE
		 XmNbottomAttachment, XmATTACH_NONE,
#else
		 XmNbottomAttachment, XmATTACH_FORM,
#endif
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (mem_k,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mem_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mem_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mem_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#ifdef PREFS_CLEAR_CACHE_BUTTONS
  XtVaSetValues (mem_clear,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, mem_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, mem_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, mem_k,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#endif

#ifdef PREFS_DISK_CACHE
  kids [i++] = disk_label = XmCreateLabelGadget (form1, "diskLabel",
						    av, ac);
  kids [i++] = disk_text = fe_CreateTextField (form1, "diskText", av, ac);
  kids [i++] = disk_k = XmCreateLabelGadget (form1, "k", av, ac);
#ifdef PREFS_CLEAR_CACHE_BUTTONS
  kids [i++] = disk_clear = XmCreatePushButtonGadget (form1,"diskClear",av,ac);
#endif
  kids [i++] = disk_dir_label = XmCreateLabelGadget (form1, "dirLabel",
						     av, ac);
  kids [i++] = disk_dir = fe_CreateTextField (form1, "cacheDirText", av, ac);
  kids [i++] = browse = XmCreatePushButtonGadget (form1, "browse", av, ac);
#endif

#ifdef PREFS_VERIFY
  kids [i++] = verify_label = XmCreateLabelGadget (form1, "verifyLabel",
						   av, ac);
  kids [i++] = once_p = XmCreateToggleButtonGadget (form1, "onceToggle",
						    av, ac);
  kids [i++] = every_p = XmCreateToggleButtonGadget (form1, "everyToggle",
						     av, ac);
  kids [i++] = expired_p = XmCreateToggleButtonGadget (form1, "expiredToggle",
						       av, ac);
#endif

#ifdef PREFS_CLEAR_CACHE_BUTTONS
  XtAddCallback (disk_clear, XmNactivateCallback, fe_clear_disk_cache_cb, fep);
#endif
  kids [i++] = cache_ssl_p =
    XmCreateToggleButtonGadget (form1, "cacheSSLToggle", av, ac);

  XtVaSetValues (disk_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, disk_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, disk_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, disk_text,
		 0);
  XtVaSetValues (disk_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mem_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mem_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (disk_k,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, disk_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, disk_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, disk_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#ifdef PREFS_CLEAR_CACHE_BUTTONS
  XtVaSetValues (disk_clear,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, disk_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, disk_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, disk_k,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  if (disk_clear->core.width < mem_clear->core.width)
    XtVaSetValues (disk_clear, XmNwidth, mem_clear->core.width, 0);
#endif
  XtVaSetValues (disk_dir_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, disk_dir,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, disk_dir,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, disk_dir,
		 0);
  XtVaSetValues (disk_dir,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, disk_text,
#ifdef PREFS_VERIFY
		 XmNbottomAttachment, XmATTACH_NONE,
#else
		 XmNbottomAttachment, XmATTACH_FORM,
#endif
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, disk_text,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, browse,
		 0);
  XtVaSetValues (browse,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, disk_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

#ifdef PREFS_VERIFY
  XtVaSetValues (verify_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, once_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, once_p,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, once_p,
		 0);
  XtVaSetValues (once_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, disk_dir,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, disk_dir,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (every_p,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, once_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, once_p,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, once_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (expired_p,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, every_p,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, every_p,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, every_p,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (cache_ssl_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, once_p,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtAddCallback (once_p, XmNvalueChangedCallback,
		 fe_cache_verify_toggle_cb, fep);
  XtAddCallback (every_p, XmNvalueChangedCallback,
		 fe_cache_verify_toggle_cb, fep);
  XtAddCallback (expired_p, XmNvalueChangedCallback,
		 fe_cache_verify_toggle_cb, fep);

  XtVaSetValues (browse, XmNheight, disk_dir->core.height, 0);

  XtAddCallback (browse, XmNactivateCallback, fe_disk_cache_browse_cb, fep);
#endif /* PREFS_DISK_CACHE */

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  fe_attach_field_to_labels (mem_text,
			     mem_label, disk_label, disk_dir_label,
#ifdef PREFS_VERIFY
			     verify_label,
#endif
			     0);

  XtManageChildren (kids, i);

  fep->memory_text = mem_text;
  fep->memory_clear = mem_clear;
#ifdef PREFS_DISK_CACHE
  fep->disk_text = disk_text;
  fep->disk_dir = disk_dir;
  fep->disk_clear = disk_clear;
#endif
#ifdef PREFS_VERIFY
  fep->verify_label = verify_label;
  fep->once_p = once_p;
  fep->every_p = every_p;
  fep->expired_p = expired_p;
#endif
  fep->cache_ssl_p = cache_ssl_p;

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_network_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
#ifdef PREFS_NET
  Widget label2, frame2, form2;
  Widget buf_explain, buf_label, buf_text, buf_k;
  Widget conn_explain, conn_label, conn_text, conn_suffix;
#endif
  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->network_selector, "cache", av, ac);
  XtManageChild (form);
  fep->network_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "netFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "netBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "netBoxLabel", av, ac);

  i = 0;
  ac = 0;
  kids [i++] = conn_explain = XmCreateLabelGadget (form2, "connExplain",
						   av, ac);
  kids [i++] = conn_label = XmCreateLabelGadget (form2, "connLabel", av, ac);
  kids [i++] = conn_text = fe_CreateTextField (form2, "connText", av, ac);
  kids [i++] = conn_suffix = XmCreateLabelGadget (form2, "connSuffix", av, ac);
  kids [i++] = buf_explain = XmCreateLabelGadget (form2, "bufExplain", av, ac);
  kids [i++] = buf_label = XmCreateLabelGadget (form2, "bufLabel", av, ac);
  kids [i++] = buf_text = fe_CreateTextField (form2, "bufText", av, ac);
  kids [i++] = buf_k = XmCreateLabelGadget (form2, "k", av, ac);

  XtVaSetValues (conn_explain,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (conn_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, conn_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, conn_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, conn_text,
		 0);
  XtVaSetValues (conn_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, conn_explain,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (conn_suffix,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, conn_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, conn_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, conn_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (buf_explain,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, conn_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (buf_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, buf_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, buf_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, buf_text,
		 0);
  XtVaSetValues (buf_text,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, buf_explain,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, conn_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (buf_k,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, buf_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, buf_text,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, buf_text,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  fe_attach_field_to_labels (conn_text, buf_label, conn_label, 0);

  XtManageChildren (kids, i);

  fep->buf_label = buf_label;
  fep->buf_text = buf_text;
  fep->conn_label = conn_label;
  fep->conn_text = conn_text;

  i = 0;
  kids [i++] = frame2;
  XtManageChildren (kids, i);
}


static void
fe_make_protocols_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget frame1, label1, form1;
  Widget cookie_p, anon_ftp_p, email_form_p;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->protocols_selector, "protocols", av, ac);
  XtManageChild (form);
  fep->protocols_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "alertsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "alertsBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "alertsBoxLabel", av, ac);

  i = 0;
  ac = 0;
  kids[i++] = cookie_p =
		XmCreateToggleButtonGadget (form1, "cookieToggle", av, ac);
  kids[i++] = email_form_p =
		XmCreateToggleButtonGadget (form1, "emailformToggle", av, ac);

  XtVaSetValues (cookie_p,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (email_form_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, cookie_p,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  i = 0;
  ac = 0;
  kids[i++] = anon_ftp_p =
		XmCreateToggleButtonGadget (form, "anonftpToggle", av, ac);
  XtVaSetValues (anon_ftp_p,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, frame1,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtManageChildren (kids, i);

  fep->cookie_p = cookie_p;
  fep->anon_ftp_p = anon_ftp_p;
  fep->email_form_p = email_form_p;

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_lang_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget java_toggle, javascript_toggle;

  Widget kids[10];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->lang_selector, "languages", av, ac);
  XtManageChild (form);
  fep->lang_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "javaFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "javaBox", av, ac);

#if defined(JAVA) || defined(MOCHA)
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "javaBoxLabel", av, ac);

  ac = 0;
  i = 0;
#ifdef JAVA
  kids [i++] = java_toggle =
    XmCreateToggleButtonGadget (form1, "javaToggle", av, ac);

  XtVaSetValues (java_toggle,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  fep->java_toggle = java_toggle;
#endif /* JAVA */

#ifdef MOCHA
  kids [i++] = javascript_toggle =
    XmCreateToggleButtonGadget (form1, "javascriptToggle", av, ac);

  XtVaSetValues (javascript_toggle,
#ifdef JAVA
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, java_toggle,
#else /* JAVA */
		 XmNtopAttachment, XmATTACH_FORM,
#endif /* JAVA */
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  fep->javascript_toggle = javascript_toggle;
#endif /* MOCHA */

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

#endif /* JAVA || MOCHA */

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}


static void
fe_make_proxies_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;

  Widget proxies_label, no_proxies_p;
  Widget manual_p, manual_browse;
  Widget auto_proxies_p, proxy_label, proxy_text;
  Widget proxy_reload;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->proxies_selector, "proxies", av, ac);
  XtManageChild (form);
  fep->proxies_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame1 = XmCreateFrame (form, "proxiesFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "proxiesBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "proxiesBox", av, ac);

  ac = 0;
  i = 0;
  kids[i++] = proxies_label =
		XmCreateLabelGadget (form1, "proxiesLabel", av, ac);
  kids[i++] = no_proxies_p =
		XmCreateToggleButtonGadget (form1, "noProxiesToggle", av, ac);
  kids[i++] = manual_p =
		XmCreateToggleButtonGadget (form1, "manualToggle", av, ac);
  kids[i++] = manual_browse =
		XmCreatePushButtonGadget (form1, "manualBrowse", av, ac);
  kids[i++] = auto_proxies_p =
		XmCreateToggleButtonGadget (form1, "autoToggle", av, ac);
  kids[i++] = proxy_label =
		XmCreateLabelGadget (form1, "locationLabel", av, ac);
  kids[i++] = proxy_text =
		fe_CreateTextField (form1, "locationText", av, ac);
  kids[i++] = proxy_reload =
		XmCreatePushButtonGadget (form1, "proxyReload", av, ac);

  XtVaSetValues (proxies_label,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 40,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);

  XtVaSetValues (no_proxies_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, proxies_label,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 50,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (manual_p,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, manual_browse,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, manual_browse,
                 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 50,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, manual_browse,
                 0);

  XtVaSetValues (manual_browse,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, no_proxies_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, manual_p->core.width + 50,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (auto_proxies_p,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, manual_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 50,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);

  XtVaSetValues (proxy_label,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, proxy_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, proxy_text,
                 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, proxy_text,
                 0);

  XtVaSetValues (proxy_text,
                 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, auto_proxies_p,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, manual_browse,
                 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, proxy_reload->core.width + 20,
                 0);

  XtVaSetValues (proxy_reload,
                 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, proxy_text,
                 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, proxy_text,
                 XmNleftAttachment, XmATTACH_NONE,
                 XmNrightAttachment, XmATTACH_FORM,
                 0);

  XtAddCallback (manual_browse, XmNactivateCallback,
                 fe_view_proxies_cb, fep);

  XtAddCallback (no_proxies_p, XmNvalueChangedCallback,
                 fe_set_proxies_cb, fep);

  XtAddCallback (manual_p, XmNvalueChangedCallback,
                 fe_set_proxies_cb, fep);

  XtAddCallback (auto_proxies_p, XmNvalueChangedCallback,
                 fe_set_proxies_cb, fep);

  XtAddCallback (proxy_reload, XmNactivateCallback,
                 fe_proxy_reload_cb, fep);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  fep->no_proxies_p = no_proxies_p;
  fep->manual_p = manual_p;
  fep->manual_browse = manual_browse;
  fep->auto_proxies_p = auto_proxies_p;
  fep->proxy_text = proxy_text;
  fep->proxy_reload = proxy_reload;

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

#define PREFS_PROXIES_DIALOG 1
static int proxies_done = -1;

static void fe_proxy_verify_page (struct fe_prefs_data *fep);
static void fe_proxy_reset (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs);

static void
fe_proxy_cancel_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  proxies_done = 0;
}

static void
fe_proxy_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  proxies_done = 0;
}

static void
fe_proxy_done_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *s1, *s2, buf [1024];

  /* Been here - we should really look at the call_data */
  if (!proxies_done) return;
  proxies_done = 0;

  fe_proxy_verify_page (fep);

/*  XtUnmanageChild (fep->shell); */

#define SNARF(SLOT1,SLOT2) \
  if (fe_globalPrefs.SLOT1) free (fe_globalPrefs.SLOT1); \
  XtVaGetValues (fep->SLOT2, XmNvalue, &fe_globalPrefs.SLOT1, 0)

  SNARF (no_proxy, no_text);

#define SNARFP(NAME,SUFFIX,empty_port_ok) \
  /*if (fe_globalPrefs.NAME##_##SUFFIX) free (fe_globalPrefs.NAME##_##SUFFIX);*/ \
  XtVaGetValues (fep->NAME##_text, XmNvalue, &s1, 0);                   \
  XtVaGetValues (fep->NAME##_port, XmNvalue, &s2, 0);                   \
  if (*s1 && *s2) PR_snprintf (buf, sizeof (buf), "%.400s:%.400s", s1, s2); \
  else if (*s1 && empty_port_ok) PR_snprintf (buf,                      \
                                            sizeof (buf), "%.900s", s1); \
  else *buf = 0;                                                        \
  fe_globalPrefs.NAME##_##SUFFIX = strdup (buf)

  SNARFP (ftp,    proxy, False);
  SNARFP (gopher, proxy, False);
  SNARFP (http,   proxy, False);
#ifdef HAVE_SECURITY
  SNARFP (https,          proxy, False);
#endif
  SNARFP (wais,   proxy, False);
#ifdef PREFS_SOCKS
  SNARFP (socks,  host,  True);
#endif
#undef SNARFP

  /* ============================================================ */

  /* Install the new preferences first, in case the news host changed
     or something. */

  NET_SetSocksHost (fe_globalPrefs.socks_host);
  NET_SetProxyServer (FTP_PROXY, fe_globalPrefs.ftp_proxy);
  NET_SetProxyServer (HTTP_PROXY, fe_globalPrefs.http_proxy);
#ifdef HAVE_SECURITY
  NET_SetProxyServer (HTTPS_PROXY, fe_globalPrefs.https_proxy);
#endif
  NET_SetProxyServer (GOPHER_PROXY, fe_globalPrefs.gopher_proxy);
  NET_SetProxyServer (WAIS_PROXY, fe_globalPrefs.wais_proxy);
  NET_SetProxyServer (NO_PROXY, fe_globalPrefs.no_proxy);

  /* Save the preferences at the end, so that if we've found some setting that
     crashes, it won't get saved... */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fep->context, XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
}

static void
fe_make_proxies_dialog (MWContext *context, struct fe_prefs_data *fep)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  int i;

  Widget kids [50];
  Widget label1, frame1, form1;
#ifdef PREFS_SOCKS
  Widget socks_label, socks_text, socks_plabel, socks_port;
#endif
  Widget ftp_label, ftp_text, ftp_plabel, ftp_port;
  Widget gopher_label, gopher_text, gopher_plabel, gopher_port;
  Widget http_label, http_text, http_plabel, http_port;
#ifdef HAVE_SECURITY
  Widget https_label, https_text, https_plabel, https_port;
#endif
  Widget wais_label, wais_text, wais_plabel, wais_port;
  Widget no_label, no_text;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;
  dialog = XmCreatePromptDialog (mainw, "proxies_prefs", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
					 XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNokCallback, fe_proxy_done_cb, fep);
  XtAddCallback (dialog, XmNcancelCallback, fe_proxy_cancel_cb, fep);
  XtAddCallback (dialog, XmNapplyCallback,  fe_proxy_done_cb, fep);
  XtAddCallback (dialog, XmNdestroyCallback, fe_proxy_destroy_cb, fep);

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (dialog, "proxies", av, ac);
  XtManageChild (form);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame1 = XmCreateFrame (form, "proxiesFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "proxiesBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "proxiesBox", av, ac);
  ac = 0;

  i = 0;
  kids[i++] = ftp_label    = XmCreateLabelGadget (form1, "ftpLabel", av, ac);
  kids[i++] = gopher_label = XmCreateLabelGadget (form1, "gopherLabel",av,ac);
  kids[i++] = http_label   = XmCreateLabelGadget (form1, "httpLabel", av, ac);
#ifdef HAVE_SECURITY
  kids[i++] = https_label  = XmCreateLabelGadget (form1, "httpsLabel", av, ac);
#endif
  kids[i++] = wais_label   = XmCreateLabelGadget (form1, "waisLabel", av, ac);
  kids[i++] = no_label     = XmCreateLabelGadget (form1, "noLabel", av, ac);
#ifdef PREFS_SOCKS
  kids[i++] = socks_label  = XmCreateLabelGadget (form1, "socksLabel", av, ac);
#endif

  ac = 0;
  kids[i++] = ftp_text    = fe_CreateTextField (form1, "ftpText", av, ac);
  kids[i++] = gopher_text = fe_CreateTextField (form1, "gopherText", av, ac);
  kids[i++] = http_text   = fe_CreateTextField (form1, "httpText", av, ac);
#ifdef HAVE_SECURITY
  kids[i++] = https_text  = fe_CreateTextField (form1, "httpsText", av, ac);
#endif
  kids[i++] = wais_text   = fe_CreateTextField (form1, "waisText", av, ac);
  kids[i++] = no_text     = fe_CreateTextField (form1, "noText", av, ac);
#ifdef PREFS_SOCKS
  kids[i++] = socks_text  = fe_CreateTextField (form1, "socksText", av, ac);
#endif

  ac = 0;
  kids[i++] = ftp_port    = fe_CreateTextField (form1, "ftpPort", av, ac);
  kids[i++] = gopher_port = fe_CreateTextField (form1, "gopherPort", av, ac);
  kids[i++] = http_port   = fe_CreateTextField (form1, "httpPort", av, ac);
#ifdef HAVE_SECURITY
  kids[i++] = https_port  = fe_CreateTextField (form1, "httpsPort", av, ac);
#endif
  kids[i++] = wais_port   = fe_CreateTextField (form1, "waisPort", av, ac);
#ifdef PREFS_SOCKS
  kids[i++] = socks_port   = fe_CreateTextField (form1, "socksPort", av, ac);
#endif

  ac = 0;
  kids[i++] = ftp_plabel    = XmCreateLabelGadget (form1, "portLabel", av,ac);
  kids[i++] = gopher_plabel = XmCreateLabelGadget (form1, "portLabel", av,ac);
  kids[i++] = http_plabel   = XmCreateLabelGadget (form1, "portLabel", av,ac);
#ifdef HAVE_SECURITY
  kids[i++] = https_plabel  = XmCreateLabelGadget (form1, "portLabel", av,ac);
#endif
  kids[i++] = wais_plabel   = XmCreateLabelGadget (form1, "portLabel", av,ac);
#ifdef PREFS_SOCKS
  kids[i++] = socks_plabel  = XmCreateLabelGadget (form1, "portLabel", av,ac);
#endif

  XtVaSetValues (ftp_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ftp_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ftp_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, ftp_text,
		 0);
  XtVaSetValues (gopher_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, gopher_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, gopher_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, gopher_text,
		 0);
  XtVaSetValues (http_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, http_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, http_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, http_text,
		 0);
#ifdef HAVE_SECURITY
  XtVaSetValues (https_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, https_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, https_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, https_text,
		 0);
#endif
  XtVaSetValues (wais_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, wais_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, wais_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, wais_text,
		 0);
  XtVaSetValues (no_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, no_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, no_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, no_text,
		 0);
#ifdef PREFS_SOCKS
  XtVaSetValues (socks_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, socks_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, socks_text,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, socks_text,
		 0);
#endif

  XtVaSetValues (ftp_text,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
#if 1
		 XmNrightAttachment, XmATTACH_FORM,
#else
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, ftp_plabel,
#endif
		 0);
  XtVaSetValues (gopher_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, ftp_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, ftp_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, ftp_text,
		 0);
  XtVaSetValues (http_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, gopher_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, gopher_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, gopher_text,
		 0);
#ifdef HAVE_SECURITY
  XtVaSetValues (https_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, http_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, http_text,
		 0);
#endif
  XtVaSetValues (wais_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNtopAttachment, XmATTACH_WIDGET,
#ifdef HAVE_SECURITY
		 XmNtopWidget, https_text,
#else
		 XmNtopWidget, http_text,
#endif
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, http_text,
		 0);
  XtVaSetValues (no_text,
#ifdef PREFS_SOCKS
		 XmNbottomAttachment, XmATTACH_NONE,
#else
		 XmNbottomAttachment, XmATTACH_FORM,
#endif
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, wais_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, wais_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, wais_text,
		 0);
#ifdef PREFS_SOCKS
  XtVaSetValues (socks_text,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, no_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, no_text,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, no_text,
		 0);
#endif
  XtVaSetValues (ftp_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, ftp_port,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, ftp_port,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, ftp_port,
		 0);
  XtVaSetValues (gopher_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, gopher_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, gopher_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, ftp_plabel,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (http_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, http_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, http_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, gopher_plabel,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#ifdef HAVE_SECURITY
  XtVaSetValues (https_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, https_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, https_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_plabel,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#endif
  XtVaSetValues (wais_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, wais_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, wais_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_plabel,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#ifdef PREFS_SOCKS
  XtVaSetValues (socks_plabel,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, socks_text,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, socks_text,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, wais_plabel,
		 XmNleftOffset, 0,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
#endif
  XtVaSetValues (ftp_port,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (gopher_port,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, gopher_plabel,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, gopher_plabel,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, ftp_port,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, ftp_port,
		 0);
  XtVaSetValues (http_port,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, http_plabel,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, http_plabel,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, gopher_port,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, gopher_port,
		 0);
#ifdef HAVE_SECURITY
  XtVaSetValues (https_port,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, https_plabel,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, https_plabel,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_port,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, http_port,
		 0);
#endif
  XtVaSetValues (wais_port,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, wais_plabel,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, wais_plabel,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, http_port,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, http_port,
		 0);
#ifdef PREFS_SOCKS
  XtVaSetValues (socks_port,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, socks_plabel,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, socks_plabel,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, wais_port,
		 XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNrightWidget, wais_port,
		 0);
#endif

  fe_attach_field_to_labels (ftp_text,
			     ftp_label, gopher_label, http_label,
#ifdef HAVE_SECURITY
			     https_label,
#endif
			     wais_label, no_label,
#ifdef PREFS_SOCKS
			     socks_label,
#endif
			     0);

  XtVaSetValues (ftp_text,
		 XmNrightOffset, (ftp_plabel->core.width + 10 +
				  ftp_port->core.width + 10),
		 0);

  if (fe_globalData.nonterminal_text_translations)
    {
      XtOverrideTranslations (ftp_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (ftp_port,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (gopher_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (gopher_port,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (http_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (http_port,
			      fe_globalData.nonterminal_text_translations);
#ifdef HAVE_SECURITY
      XtOverrideTranslations (https_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (https_port,
			      fe_globalData.nonterminal_text_translations);
#endif
      XtOverrideTranslations (wais_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (wais_port,
			      fe_globalData.nonterminal_text_translations);
#ifdef PREFS_SOCKS
      XtOverrideTranslations (no_text,
			      fe_globalData.nonterminal_text_translations);
      XtOverrideTranslations (socks_text,
			      fe_globalData.nonterminal_text_translations);
#endif
    }

  fep->ftp_text = ftp_text;
  fep->ftp_port = ftp_port;
  fep->gopher_text = gopher_text;
  fep->gopher_port = gopher_port;
  fep->http_text = http_text;
  fep->http_port = http_port;
#ifdef HAVE_SECURITY
  fep->https_text = https_text;
  fep->https_port = https_port;
#endif
  fep->wais_text = wais_text;
  fep->wais_port = wais_port;
  fep->no_text = no_text;
#ifdef PREFS_SOCKS
  fep->socks_text = socks_text;
  fep->socks_port = socks_port;
#endif

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);

  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  fe_proxy_verify_page (fep);
  fe_proxy_reset (fep, &fe_globalPrefs);

  /* Reset this for next time */
  proxies_done = -1;

  while (proxies_done)
    fe_EventLoop ();

  /* Destroy the dialog */

  /* This needs to be commented out, otherwise it hangs on Solaris 2.5
     in the Japanese environment when you click inside a text widget and
     give it focus, and then try to quit the dialog using OK or Cancel.
     -- erik
  XtUnrealizeWidget (dialog);
  */

  XtDestroyWidget (dialog);
}

#ifdef DUPLICATE_SEC_PREFS
/*======================== Security Preferences ========================*/

static void
fe_make_sec_passwords_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label2, frame2, form2;
  Widget pass_label;
  Widget banner_label;
  Widget ask_label, periodic_text, periodic_label;
  Widget once_toggle, every_toggle, periodic_toggle;
  Widget change_password;
  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->sec_passwords_selector, "passwords", av, ac);
  XtManageChild (form);
  fep->sec_passwords_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame2 = XmCreateFrame (form, "passwordsFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "passwordsBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "passwordsBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = pass_label = XmCreateLabelGadget (form2, "passLabel", av, ac);

  kids [i++] = ask_label = XmCreateLabelGadget (form2, "askLabel", av, ac);
  kids [i++] = once_toggle =
    XmCreateToggleButtonGadget (form2, "onceToggle", av, ac);
  kids [i++] = every_toggle =
    XmCreateToggleButtonGadget (form2, "everyToggle", av, ac);
  kids [i++] = periodic_toggle =
    XmCreateToggleButtonGadget (form2, "periodicToggle", av, ac);
  kids [i++] = periodic_text =
    fe_CreateTextField (form2, "periodicText", av, ac);
  kids [i++] = periodic_label =
    XmCreateLabelGadget (form2, "periodicLabel", av, ac);

  kids [i++] = banner_label =
    XmCreateLabelGadget (form2, "bannerLabel", av, ac);

  kids [i++] = change_password =
    XmCreatePushButtonGadget (form2, "changePassword", av, ac);

  XtVaSetValues (change_password, XmNsensitive, False, 0);


  XtVaSetValues (banner_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (pass_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, change_password,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, change_password,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, change_password,
		 0);
  XtVaSetValues (change_password,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, banner_label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_POSITION,
		 XmNleftPosition, 30,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (ask_label,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, pass_label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, once_toggle,
		 0);

  XtVaSetValues (once_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, change_password,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, change_password,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (every_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, once_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, once_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (periodic_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, every_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, every_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (periodic_text,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, periodic_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, periodic_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtVaSetValues (periodic_label,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, periodic_text,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, periodic_text,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtAddCallback (once_toggle, XmNvalueChangedCallback,
                 fe_set_pass_toggle_cb, fep);

  XtAddCallback (every_toggle, XmNvalueChangedCallback,
                 fe_set_pass_toggle_cb, fep);

  XtAddCallback (periodic_toggle, XmNvalueChangedCallback,
                 fe_set_pass_toggle_cb, fep);

#if 0
  XtAddCallback (change_password, XmNactivateCallback,
                 fe_change_pass_toggle_cb, fep);
#endif

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  fep->once_toggle = once_toggle;
  fep->every_toggle = every_toggle;
  fep->periodic_toggle = periodic_toggle;
  fep->periodic_text = periodic_text;
  fep->change_password = change_password;

  i = 0;
  kids [i++] = frame2;
  XtManageChildren (kids, i);
}

static void
fe_make_sec_general_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label2, frame2, form2;
  Widget label3, frame3, form3;
  Widget alert_label;
  Widget enter_toggle, leave_toggle, mixed_toggle, submit_toggle;
  Widget ssl2_toggle, ssl3_toggle, ssl2_configure, ssl3_configure;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->sec_general_selector, "images", av, ac);
  XtManageChild (form);
  fep->sec_general_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame2 = XmCreateFrame (form, "securityFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form2 = XmCreateForm (frame2, "securityBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNtopWidget, frame2); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame3 = XmCreateFrame (form, "sslFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form3 = XmCreateForm (frame3, "sslBox", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label2 = XmCreateLabelGadget (frame2, "securityBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = alert_label = XmCreateLabelGadget (form2, "alertLabel", av, ac);
  kids [i++] = enter_toggle =
    XmCreateToggleButtonGadget (form2, "enterToggle", av, ac);
  kids [i++] = leave_toggle =
    XmCreateToggleButtonGadget (form2, "leaveToggle", av, ac);
  kids [i++] = mixed_toggle =
    XmCreateToggleButtonGadget (form2, "mixedToggle", av, ac);
  kids [i++] = submit_toggle =
    XmCreateToggleButtonGadget (form2, "submitToggle", av, ac);

  XtVaSetValues (alert_label,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (enter_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, alert_label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (leave_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, enter_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, enter_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (mixed_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, leave_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, leave_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (submit_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, mixed_toggle,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, mixed_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label2);
  XtManageChild (form2);

  fep->enter_toggle = enter_toggle;
  fep->leave_toggle = leave_toggle;
  fep->mixed_toggle = mixed_toggle;
  fep->submit_toggle = submit_toggle;

  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label3 = XmCreateLabelGadget (frame3, "sslBoxLabel", av, ac);

  ac = 0;
  i = 0;
  kids [i++] = ssl2_toggle =
    XmCreateToggleButtonGadget (form3, "ssl2Toggle", av, ac);
  kids [i++] = ssl3_toggle =
    XmCreateToggleButtonGadget (form3, "ssl3Toggle", av, ac);
  kids [i++] = ssl2_configure =
    XmCreatePushButtonGadget (form3, "configure", av, ac);
  kids [i++] = ssl3_configure =
    XmCreatePushButtonGadget (form3, "configure", av, ac);

  XtVaSetValues (ssl2_toggle,
                 XmNtopAttachment, XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_NONE,
                 XmNleftAttachment, XmATTACH_FORM,
                 XmNrightAttachment, XmATTACH_NONE,
                 0);
  XtVaSetValues (ssl3_toggle,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, ssl2_toggle,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (ssl2_configure,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, ssl2_toggle,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (ssl3_configure,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, ssl2_configure,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, ssl2_configure,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);

  XtAddCallback (ssl2_configure, XmNactivateCallback,
		 fe_ssl_enable_cb, fep);
  XtAddCallback (ssl3_configure, XmNactivateCallback,
		 fe_ssl_enable_cb, fep);
  XtVaSetValues(ssl2_configure, XmNuserData, SSL_LIBRARY_VERSION_2, 0);
  XtVaSetValues(ssl3_configure, XmNuserData, SSL_LIBRARY_VERSION_3_0, 0);

  XtManageChildren (kids, i);
  XtManageChild (label3);
  XtManageChild (form3);

  fep->ssl2_toggle= ssl2_toggle;
  fep->ssl3_toggle = ssl3_toggle;

  i = 0;
  kids [i++] = frame2;
  kids [i++] = frame3;
  XtManageChildren (kids, i);
}


static void
fe_make_personal_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget pers_list, pers_info, pers_delete_cert, pers_new_cert;
  Widget site_default/*, site_text*/, pers_label;
  Widget pers_cert_pulldown, pers_cert_menu;

  Widget kids [50];
  int i;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->personal_selector, "personal", av, ac);
  XtManageChild (form);
  fep->personal_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
  frame1 = XmCreateFrame (form, "personalFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "personalBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "personalBox", av, ac);

  ac = 0;
  i = 0;

  /* Setup the scrolled List */

  {
    XmStringTable str_list = 0;
    int n;
    CERTCertNicknames *certnames;
    
    certnames = CERT_GetCertNicknames (CERT_GetDefaultCertDB(),
				      SEC_CERT_NICKNAMES_USER,
				       fep->context);
    if (!certnames) abort;

    str_list = (XmStringTable) XP_ALLOC (sizeof (XmString) * 
					certnames->numnicknames);
    for (n = 0; n < certnames->numnicknames; n++)
      str_list[n] = XmStringCreateLocalized (certnames->nicknames[n]);

    ac = 0;
    XtSetArg (av[ac], XmNvisibleItemCount, 5); ac++;
    XtSetArg (av[ac], XmNselectionPolicy, XmBROWSE_SELECT); ac++;
    if (certnames->numnicknames) {
      XtSetArg (av[ac], XmNitemCount, n); ac++;
      XtSetArg (av[ac], XmNitems, str_list); ac++;
    }
    pers_list = XmCreateScrolledList (form1, "persList", av, ac);
    XtManageChild (pers_list);
    kids [i++] = XtParent (pers_list);

    if (certnames->numnicknames)
      XmListSelectPos (pers_list, 1, False);

    for (n = 0; n < certnames->numnicknames; n++)
      XtFree (str_list [n]);
    XP_FREE (str_list);
    CERT_FreeNicknames (certnames);
  }

  kids [i++] = pers_label = XmCreateLabelGadget (form1, "persLabel", av, ac);
  kids [i++] = pers_info = XmCreatePushButtonGadget (form1, "persInfo", av, ac);
  kids [i++] = pers_delete_cert =
		XmCreatePushButtonGadget (form1, "persDeleteCert", av, ac);
  kids [i++] = pers_new_cert =
		XmCreatePushButtonGadget (form1, "persNewCert",av, ac);
  kids [i++] = site_default =
		XmCreateLabelGadget (form1, "siteDefault", av, ac);


  XtVaGetValues (CONTEXT_WIDGET (context), XtNvisual, &v, XtNcolormap, &cmap,
	XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;

  pers_cert_pulldown = XmCreatePulldownMenu (form1, "personalCertPulldown", av, ac);

  {
    CERTCertNicknames *sec_nicknames =
      SECNAV_GetDefUserCertList(CERT_GetDefaultCertDB(), fep->context);
    int tmpi;
    Widget b;

    for (tmpi=0; tmpi<sec_nicknames->numnicknames; tmpi++)
      {
	b = XmCreatePushButtonGadget(pers_cert_pulldown,
				     "personalCerts", av, ac);
	fe_SetString(b, XmNlabelString, sec_nicknames->nicknames[tmpi]);
	XtManageChild(b);
    }
    /* Free the nicknames */
    CERT_FreeNicknames(sec_nicknames);
  }

  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, pers_cert_pulldown); ac++;
  kids [i++] = pers_cert_menu = fe_CreateOptionMenu (form1, "personalCertMenu", av, ac);

  XtVaSetValues (pers_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (XtParent (pers_list),
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, pers_label,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNbottomOffset, pers_cert_menu->core.height + 20,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 10,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNrightOffset, pers_new_cert->core.width + 10,
		 0);
  XtVaSetValues (pers_info,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNtopOffset, pers_label->core.height, /*  + 10, */
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, pers_list,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (pers_delete_cert,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, pers_info,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, pers_info,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (pers_new_cert,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, pers_delete_cert,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, pers_info,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (site_default,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, pers_cert_menu,
		 XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNbottomWidget, pers_cert_menu,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
		 XmNrightWidget, pers_cert_menu,
		 0);
  XtVaSetValues (pers_cert_menu,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_NONE,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  XtAddCallback (pers_info, XmNactivateCallback,
		 fe_cert_info_cb, fep);
  XtAddCallback (pers_delete_cert, XmNactivateCallback,
		 fe_cert_delete_cb, fep);
  XtAddCallback (pers_new_cert, XmNactivateCallback,
		 fe_cert_new_cb, fep);

  fep->pers_list = pers_list;
  fep->pers_info = pers_info;
  fep->pers_delete_cert = pers_delete_cert;
  fep->pers_new_cert = pers_new_cert;
  fep->pers_cert_menu = pers_cert_menu;

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}

static void
fe_make_site_cert_page (MWContext *context, struct fe_prefs_data *fep)
{
  Arg av [50];
  int ac;
  Widget form;
  Widget label1, frame1, form1;
  Widget all_label, all_list, all_edit_cert, all_delete_cert;
  Widget cert_pulldown, cert_menu;
  int cert_type;

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  Widget kids [50];
  int i;

  ac = 0;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  form = XmCreateForm (fep->site_selector, "site", av, ac);
  XtManageChild (form);
  fep->site_page = form;

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  frame1 = XmCreateFrame (form, "siteFrame", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
  label1 = XmCreateLabelGadget (frame1, "siteBoxLabel", av, ac);

  ac = 0;
  XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  form1 = XmCreateForm (frame1, "siteBox", av, ac);


  XtVaGetValues (CONTEXT_WIDGET (context), XtNvisual, &v, XtNcolormap, &cmap,
	XtNdepth, &depth, 0);

  i = 0;
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;

  cert_pulldown = XmCreatePulldownMenu (form1, "certPulldown", av, ac);

  {
    Widget b;

    kids [i++] = b = XmCreatePushButtonGadget (cert_pulldown, "allCerts", av, ac);
    cert_type = SEC_CERT_NICKNAMES_ALL;
    XtVaSetValues(b, XmNuserData, cert_type, NULL);
    XtAddCallback (b, XmNactivateCallback, fe_cert_pulldown_cb, fep);

    kids [i++] = b = XmCreatePushButtonGadget (cert_pulldown, "siteCerts", av, ac);
    cert_type = SEC_CERT_NICKNAMES_SERVER;
    XtVaSetValues(b, XmNuserData, cert_type, NULL);
    XtAddCallback (b, XmNactivateCallback, fe_cert_pulldown_cb, fep);

    kids [i++] = b = XmCreatePushButtonGadget (cert_pulldown, "authCerts", av, ac);
    cert_type = SEC_CERT_NICKNAMES_CA;
    XtVaSetValues(b, XmNuserData, cert_type, NULL);
    XtAddCallback (b, XmNactivateCallback, fe_cert_pulldown_cb, fep);

    XtManageChildren (kids, i);
  }

  i = 0;
  ac = 0;
  XtSetArg (av [ac], XmNsubMenuId, cert_pulldown); ac++;
  kids [i++] = cert_menu = fe_CreateOptionMenu (form1, "certMenu", av, ac);

  /* Setup the scrolled List */

  {
    XmStringTable str_list = 0;
    int n;
    CERTCertNicknames *certnames;
    
    certnames = CERT_GetCertNicknames (CERT_GetDefaultCertDB(),
				      SEC_CERT_NICKNAMES_ALL,
				       fep->context);
    if (!certnames) return;

    str_list = (XmStringTable) XP_ALLOC (sizeof (XmString) * 
					certnames->numnicknames);
    for (n = 0; n < certnames->numnicknames; n++)
      str_list[n] = XmStringCreateLocalized (certnames->nicknames[n]);

    ac = 0;
    XtSetArg (av[ac], XmNvisibleItemCount, 5); ac++;
    XtSetArg (av[ac], XmNselectionPolicy, XmBROWSE_SELECT); ac++;
    if (certnames->numnicknames) {
      XtSetArg (av[ac], XmNitemCount, n); ac++;
      XtSetArg (av[ac], XmNitems, str_list); ac++;
    }
    all_list = XmCreateScrolledList (form1, "allList", av, ac);
    XtManageChild (all_list);
    kids [i++] = XtParent (all_list);

    if (certnames->numnicknames)
      XmListSelectPos (all_list, 1, False);

    for (n = 0; n < certnames->numnicknames; n++)
      XtFree (str_list [n]);
    XP_FREE (str_list);
    CERT_FreeNicknames (certnames);
  }

  kids [i++] = all_label = XmCreateLabelGadget (form1, "allLabel", av, ac);
  kids [i++] = all_edit_cert =
		XmCreatePushButtonGadget (form1, "allEditCert", av, ac);
  kids [i++] = all_delete_cert =
		XmCreatePushButtonGadget (form1, "allDeleteCert", av, ac);

  XtVaSetValues (all_label,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (cert_menu,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, all_label,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (XtParent (all_list),
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, cert_menu,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNbottomOffset, 10,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNleftOffset, 10,
		 XmNrightAttachment, XmATTACH_NONE,
		 0);
  XtVaSetValues (all_edit_cert,
		 XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNtopWidget, XtParent (all_list),
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_WIDGET,
		 XmNleftWidget, XtParent (all_list),
		 XmNleftOffset, 10,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (all_delete_cert,
		 XmNtopAttachment, XmATTACH_WIDGET,
		 XmNtopWidget, all_edit_cert,
		 XmNbottomAttachment, XmATTACH_NONE,
		 XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		 XmNleftWidget, all_edit_cert,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtManageChildren (kids, i);
  XtManageChild (label1);
  XtManageChild (form1);

  XtAddCallback (all_edit_cert, XmNactivateCallback,
		 fe_all_edit_cb, fep);
  XtAddCallback (all_delete_cert, XmNactivateCallback,
		 fe_all_delete_cb, fep);

#if 0
  fe_attach_field_to_labels (auto_p, colors_label, display_label, 0);
#endif

  fep->all_list = all_list;
  fep->all_edit_cert = all_edit_cert;
  fep->all_delete_cert = all_delete_cert;

  i = 0;
  kids [i++] = frame1;
  XtManageChildren (kids, i);
}
#endif /* DUPLICATE_SEC_PREFS */


/*======================== Preferences functions ========================*/

IL_ColorRenderMode
fe_pref_string_to_dither_mode (char *s)
{
  if (!strcasecmp (s, "True") || !strcasecmp (s, "Dither"))
    return ilDither;
  else if (!strcasecmp (s, "False") || !strcasecmp (s, "ClosestColor"))
    return ilClosestColor;

  /* In the absence of a reasonable value, dithering is auto-selected */
  return ilAuto;
}

static char*
fep_to_dither_mode_pref_string (struct fe_prefs_data *fep)
{
  Boolean b;
    
  XtVaGetValues (fep->auto_p, XmNset, &b, 0);
  if (b)
    return "Auto";

  XtVaGetValues (fep->dither_p, XmNset, &b, 0);
  if (b)
    return "Dither";

  XtVaGetValues (fep->closest_p, XmNset, &b, 0);
  if (b)
    return "ClosestColor";

  assert(0);
  return NULL;                  /* Satisfy the compiler */
}


/*
 * fe_general_reset_1
 */
static void
fe_general_reset_1 (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs)
{
  char buf [1024];

  if (current_general_page == xfe_PREFS_STYLES)
    {
      int state = (prefs->toolbar_icons_p && prefs->toolbar_text_p ? 0
		   : prefs->toolbar_icons_p ? 1 : 2);

      /*#### XP_Bool fixed_message_font_p; */
      /*#### int msg_sort_style; */
      /*#### int msg_thread_p; */

      XtVaSetValues (fep->both_p,  XmNset, (state == 0), 0);
      XtVaSetValues (fep->icons_p, XmNset, (state == 1), 0);
      XtVaSetValues (fep->text_p,  XmNset, (state == 2), 0);
      XtVaSetValues (fep->tips_p,  XmNset, prefs->toolbar_tips_p, 0);
	  
      XtVaSetValues (fep->blank_p, XmNset,
		     (!prefs->home_document ||
		      !*prefs->home_document), 0);

      XtVaSetValues (fep->home_p, XmNset,
		     (prefs->home_document &&
		      *prefs->home_document), 0);

      XtVaSetValues (fep->home_text,
		     XmNvalue, prefs->home_document,
		     XmNsensitive, (prefs->home_document &&
				    *prefs->home_document &&
                                    !ekit_HomePage()),
		     0);

      state = prefs->startup_mode;
      XtVaSetValues (fep->browser_p, XmNset, (state == 0), 0);
      XtVaSetValues (fep->mail_p,    XmNset, (state == 1), 0);
      XtVaSetValues (fep->news_p,    XmNset, (state == 2), 0);

      XtVaSetValues (fep->underline_p, XmNset, prefs->underline_links_p, 0);
      XtVaSetValues (fep->expire_days_p,
		     XmNset, prefs->global_history_expiration >= 0, 0);
      if (prefs->global_history_expiration >= 0)
	PR_snprintf (buf, sizeof (buf), "%d", prefs->global_history_expiration);
      else
	*buf = 0;
      XtVaSetValues (fep->expire_days_text,
		     XmNsensitive, prefs->global_history_expiration >= 0,
		     XmNvalue, buf, 0);
      XtVaSetValues (fep->never_expire_p,
		     XmNset, prefs->global_history_expiration < 0, 0);
    }
  else if (current_general_page == xfe_PREFS_FONTS)
    {
    }
  else if (current_general_page == xfe_PREFS_APPS)
    {
      XtVaSetValues (fep->telnet_text, XmNvalue, prefs->telnet_command, 0);
      XtVaSetValues (fep->tn3270_text, XmNvalue, prefs->tn3270_command, 0);
      XtVaSetValues (fep->rlogin_text, XmNvalue, prefs->rlogin_command, 0);
      XtVaSetValues (fep->rlogin_user_text, XmNvalue,
		     prefs->rlogin_user_command, 0);
      XtVaSetValues (fep->rlogin_text, XmNvalue, prefs->rlogin_command, 0);
      XtVaSetValues (fep->tmp_text, XmNvalue, prefs->tmp_dir, 0);
    }
  else if (current_general_page == xfe_PREFS_HELPERS)
    {
      /* Parse stuff out of the .mime.types and .mailcap files.
       */
      fe_helpers_build_mime_list(fep->helpers);
    }
  else if (current_general_page == xfe_PREFS_IMAGES)
    {
      IL_ColorRenderMode render_mode =
          fe_pref_string_to_dither_mode(prefs->dither_images);
      
      XtVaSetValues (fep->auto_p, XmNset, render_mode == ilAuto, 0);
      XtVaSetValues (fep->dither_p, XmNset, render_mode == ilDither, 0);
      XtVaSetValues (fep->closest_p, XmNset, render_mode == ilClosestColor, 0);
      XtVaSetValues (fep->while_loading_p, XmNset, prefs->streaming_images, 0);
      XtVaSetValues (fep->after_loading_p, XmNset,!prefs->streaming_images, 0);
    }
  else if (current_general_page == xfe_PREFS_LANGUAGES)
    {
	/* @@@ not done yet -- erik */
  } else
    {
      abort ();
    }
}

static void
fe_mailnews_reset_1 (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs)
{
  if (current_mailnews_page == xfe_PREFS_APPEARANCE)
    {
      XtVaSetValues (fep->cite_plain_p,
		     XmNset, prefs->citation_font == MSG_PlainFont, 0);
      XtVaSetValues (fep->cite_bold_p,
		     XmNset, prefs->citation_font == MSG_BoldFont, 0);
      XtVaSetValues (fep->cite_italic_p,
		     XmNset, prefs->citation_font == MSG_ItalicFont, 0);
      XtVaSetValues (fep->cite_bold_italic_p,
		     XmNset, prefs->citation_font == MSG_BoldItalicFont, 0);

      XtVaSetValues (fep->cite_normal_p,
		     XmNset, prefs->citation_size == MSG_NormalSize, 0);
      XtVaSetValues (fep->cite_bigger_p,
		     XmNset, prefs->citation_size == MSG_Bigger, 0);
      XtVaSetValues (fep->cite_smaller_p,
		     XmNset, prefs->citation_size == MSG_Smaller, 0);

      XmTextFieldSetString(fep->cite_color_text, prefs->citation_color);

      XtVaSetValues (fep->fixed_message_font_p,
		     XmNset, prefs->fixed_message_font_p, 0);
      XtVaSetValues (fep->var_message_font_p,
		     XmNset, !prefs->fixed_message_font_p, 0);

      XtVaSetValues (fep->mail_horiz_p,
		     XmNset, prefs->mail_pane_style == FE_PANES_NORMAL, 0);
      XtVaSetValues (fep->mail_vert_p,
		     XmNset, prefs->mail_pane_style == FE_PANES_HORIZONTAL, 0);
      XtVaSetValues (fep->mail_stack_p,
		     XmNset, prefs->mail_pane_style == FE_PANES_STACKED, 0);
      XtVaSetValues (fep->mail_tall_p,
		     XmNset, prefs->mail_pane_style == FE_PANES_TALL_FOLDERS, 0);

      XtVaSetValues (fep->news_horiz_p,
		     XmNset, prefs->news_pane_style == FE_PANES_NORMAL, 0);
      XtVaSetValues (fep->news_vert_p,
		     XmNset, prefs->news_pane_style == FE_PANES_HORIZONTAL, 0);
      XtVaSetValues (fep->news_stack_p,
		     XmNset, prefs->news_pane_style == FE_PANES_STACKED, 0);
      XtVaSetValues (fep->news_tall_p,
		     XmNset, prefs->news_pane_style == FE_PANES_TALL_FOLDERS, 0);

    }
  else if (current_mailnews_page == xfe_PREFS_COMPOSITION)
    {
#ifdef PREFS_8BIT
      XtVaSetValues (fep->eightbit_toggle, XmNset, !prefs->qp_p, 0);
      XtVaSetValues (fep->qp_toggle,       XmNset,  prefs->qp_p, 0);
#endif
#ifdef PREFS_QUEUED_DELIVERY
      XtVaSetValues (fep->deliverAuto_toggle,
			XmNset, !prefs->queue_for_later_p, 0);
      XtVaSetValues (fep->deliverQ_toggle,
			XmNset, prefs->queue_for_later_p, 0);
#endif
      XtVaSetValues (fep->mMailOutOther_text, XmNvalue, prefs->mail_bcc, 0);
      XtVaSetValues (fep->nMailOutOther_text, XmNvalue, prefs->news_bcc, 0);

      XtVaSetValues (fep->mCopyOut_text, XmNvalue, prefs->mail_fcc, 0);
      XtVaSetValues (fep->nCopyOut_text, XmNvalue, prefs->news_fcc, 0);

      XtVaSetValues (fep->autoquote_toggle,
			XmNset, prefs->autoquote_reply, 0);
      XtVaSetValues (fep->mMailOutSelf_toggle, XmNset, prefs->mailbccself_p, 0);
      XtVaSetValues (fep->nMailOutSelf_toggle, XmNset, prefs->newsbccself_p, 0);
      /*#### XP_Bool send_formatted_text_p; */
    }
  else if (current_mailnews_page == xfe_PREFS_SERVERS)
    {
      XP_Bool b;

#ifdef PREFS_NEWS_MAX
      char buf [1024];
#endif
      XtVaSetValues (fep->newshost_text, XmNvalue, prefs->newshost, 0);
      XtVaSetValues (fep->newsrc_text, XmNvalue, prefs->newsrc_directory, 0);
#ifdef PREFS_NEWS_MAX
      {
	if (prefs->news_max_articles > 0) {
	  if (prefs->news_max_articles > 3500)
	    prefs->news_max_articles = 3500;
	  PR_snprintf (buf, sizeof (buf), "%d", prefs->news_max_articles);
	} else
	  *buf = 0;
	XtVaSetValues (fep->newsmax_text, XmNvalue, buf, 0);
      }
#endif
      XtVaSetValues (fep->smtp_text, XmNvalue, prefs->mailhost, 0);

      XtVaSetValues (fep->movemail_text, XmNvalue, prefs->movemail_program, 0);
      XtVaSetValues (fep->srvr_text, XmNvalue, prefs->pop3_host, 0);
      XtVaSetValues (fep->user_text, XmNvalue, prefs->pop3_user_id, 0);

      if (prefs->use_movemail_p) {

	b = prefs->builtin_movemail_p;
	XtVaSetValues (fep->builtin_toggle, XmNset, b, 0);
	XtVaSetValues (fep->external_toggle, XmNset, !b, 0);
	XtVaSetValues (fep->movemail_text, XmNsensitive, !b, 0);
	XtVaSetValues (fep->pop_toggle, XmNset, False, 0);
	XtVaSetValues (fep->srvr_text, XmNsensitive, False, 0);
	XtVaSetValues (fep->user_text, XmNsensitive, False, 0);

      } else {

	XtVaSetValues (fep->pop_toggle, XmNset, True, 0);
        if ( !ekit_PopServer() ) {
	  XtVaSetValues (fep->srvr_text, XmNsensitive, True, 0);
        }
        if ( !ekit_PopName() ) {
	    XtVaSetValues (fep->user_text, XmNsensitive, True, 0);
        }
	XtVaSetValues (fep->builtin_toggle, XmNset, False, 0);
	XtVaSetValues (fep->external_toggle, XmNset, False, 0);
	XtVaSetValues (fep->movemail_text, XmNsensitive, False, 0);

      }

      /* int pop3_msg_size_limit; */
      b = prefs->pop3_msg_size_limit_p;
      XtVaSetValues (fep->no_limit, XmNset, !b, 0);
      XtVaSetValues (fep->msg_limit, XmNset, b, 0);
      if ( !ekit_MsgSizeLimit() ) {
          XtVaSetValues (fep->limit_text, XmNsensitive, b, 0);
      }

      PR_snprintf (buf, sizeof (buf), "%d", abs(prefs->pop3_msg_size_limit));
      XtVaSetValues (fep->limit_text, XmNvalue, buf, 0);

      /* XP_Bool pop3_leave_mail_on_server; */
      b = prefs->pop3_leave_mail_on_server;
      XtVaSetValues (fep->msg_remove, XmNset, !b, 0);
      XtVaSetValues (fep->msg_leave, XmNset, b, 0);

      /* int biff_interval; */
      b = prefs->auto_check_mail;
      XtVaSetValues (fep->check_never, XmNset, !b, 0);
      XtVaSetValues (fep->check_every, XmNset, b, 0);
      XtVaSetValues (fep->check_text, XmNsensitive, b, 0);

      PR_snprintf (buf, sizeof (buf), "%d", prefs->biff_interval);
      XtVaSetValues (fep->check_text, XmNvalue, buf, 0);

      XtVaSetValues (fep->maildir_text, XmNvalue, prefs->mail_directory, 0);

      /*####  char *history_file; */
    }
  else if (current_mailnews_page == xfe_PREFS_IDENTITY)
    {
      fe_SetTextField (fep->user_name_text, prefs->real_name);
      XtVaSetValues (fep->user_mail_text, XmNvalue,
		     prefs->email_address, 0);
      fe_SetTextField (fep->user_org_text, prefs->organization);
#ifdef PREFS_SIG
      XtVaSetValues (fep->user_sig_text, XmNvalue,
		     prefs->signature_file, 0);
#endif
      /* #### int anonymity_level; */
    }
  else if (current_mailnews_page == xfe_PREFS_ORGANIZATION)
    {
#ifdef PREFS_EMPTY_TRASH
      XtVaSetValues (fep->emptyTrash_toggle, XmNset, prefs->emptyTrash, 0);
#endif
      XtVaSetValues (fep->rememberPswd_toggle, XmNset, prefs->rememberPswd, 0);
      XtVaSetValues (fep->threadmail_toggle, XmNset, prefs->mail_thread_p, 0);
      XtVaSetValues (fep->threadnews_toggle, XmNset,
			!prefs->no_news_thread_p, 0);
      switch (prefs->mail_sort_style) {
	default:	/* FALL THROUGH */
	case 0:		/* DATE by default */
	  XtVaSetValues (fep->mdate_toggle,    XmNset, True,  0);
	  XtVaSetValues (fep->mnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->msender_toggle,  XmNset, False, 0);
	  break;
	case 1:		/* Message Number by default */
	  XtVaSetValues (fep->mdate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->mnum_toggle,     XmNset, True,  0);
	  XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->msender_toggle,  XmNset, False, 0);
	  break;
	case 2:		/* Subject by default */
	  XtVaSetValues (fep->mdate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->mnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->msubject_toggle, XmNset, True,  0);
	  XtVaSetValues (fep->msender_toggle,  XmNset, False, 0);
	  break;
	case 3:		/* Sender by default */
	  XtVaSetValues (fep->mdate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->mnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->msubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->msender_toggle,  XmNset, True,  0);
	  break;
      }
      switch (prefs->news_sort_style) {
	default:	/* FALL THROUGH */
	case 0:		/* DATE by default */
	  XtVaSetValues (fep->ndate_toggle,    XmNset, True,  0);
	  XtVaSetValues (fep->nnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->nsender_toggle,  XmNset, False, 0);
	  break;
	case 1:		/* Message Number by default */
	  XtVaSetValues (fep->ndate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->nnum_toggle,     XmNset, True,  0);
	  XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->nsender_toggle,  XmNset, False, 0);
	  break;
	case 2:		/* Subject by default */
	  XtVaSetValues (fep->ndate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->nnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->nsubject_toggle, XmNset, True,  0);
	  XtVaSetValues (fep->nsender_toggle,  XmNset, False, 0);
	  break;
	case 3:		/* Sender by default */
	  XtVaSetValues (fep->ndate_toggle,    XmNset, False, 0);
	  XtVaSetValues (fep->nnum_toggle,     XmNset, False, 0);
	  XtVaSetValues (fep->nsubject_toggle, XmNset, False, 0);
	  XtVaSetValues (fep->nsender_toggle,  XmNset, True,  0);
	  break;
      }
#ifdef PREFS_EMPTY_TRASH
    XtVaSetValues (fep->emptyTrash_toggle, XmNsensitive, False, 0);
#endif
    }
  else
    {
      abort ();
    }
}

static void
fe_proxy_reset (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs)
{
  char buf [1024];
  char *s;

#define FROB(SLOT,SUFFIX,DEF)                               \
  strcpy (buf, prefs->SLOT##_##SUFFIX);                     \
  s = strrchr (buf, ':');                                   \
  if (s)                                                    \
    *s++ = 0;                                               \
  else                                                      \
    s = DEF;                                                \
  XtVaSetValues (fep->SLOT##_text, XmNvalue, buf, 0);       \
  XtVaSetValues (fep->SLOT##_port, XmNvalue, s, 0)

  FROB (ftp,    proxy, "");
  FROB (gopher, proxy, "");
  FROB (http,   proxy, "");
#ifdef HAVE_SECURITY
  FROB (https,    proxy, "");
#endif
  FROB (wais,   proxy, "");
#ifdef PREFS_SOCKS
  FROB (socks,  host,  "1080");
#endif
#undef FROB
  XtVaSetValues (fep->no_text, XmNvalue, prefs->no_proxy, 0);
}

static void
fe_network_reset_1 (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs)
{
  if (current_network_page == xfe_PREFS_CACHE)
    {
      char buf [255];
      PR_snprintf (buf, sizeof (buf), "%d", prefs->memory_cache_size);
      XtVaSetValues (fep->memory_text, XmNvalue, buf, 0);
#ifdef PREFS_DISK_CACHE
      PR_snprintf (buf, sizeof (buf), "%d", prefs->disk_cache_size);
      XtVaSetValues (fep->disk_text, XmNvalue, buf, 0);
      XtVaSetValues (fep->disk_dir, XmNvalue, prefs->cache_dir, 0);
#endif
#ifdef PREFS_VERIFY
      XtVaSetValues (fep->once_p,    XmNset, prefs->verify_documents == 0, 0);
      XtVaSetValues (fep->every_p,   XmNset, prefs->verify_documents == 1, 0);
      XtVaSetValues (fep->expired_p, XmNset, prefs->verify_documents == 2, 0);
#endif
      XtVaSetValues (fep->cache_ssl_p, XmNset, prefs->cache_ssl_p, 0);
    }
  else if (current_network_page == xfe_PREFS_NETWORK)
    {
#ifdef PREFS_NET
      char buf [1024];
      PR_snprintf (buf, sizeof (buf), "%d", prefs->max_connections);
      XtVaSetValues (fep->conn_text, XmNvalue, buf, 0);
      PR_snprintf (buf, sizeof (buf), "%d", prefs->network_buffer_size);
      XtVaSetValues (fep->buf_text, XmNvalue, buf, 0);
#endif /* PREFS_NET */
    }
  else if (current_network_page == xfe_PREFS_PROXIES)
    {
      int state = 0;

      state = prefs->proxy_mode;
      XtVaSetValues (fep->no_proxies_p,   XmNset, (state == 0), 0);
      XtVaSetValues (fep->manual_p,       XmNset, (state == 1), 0);
      XtVaSetValues (fep->manual_browse,  XmNsensitive, (state == 1), 0);
      XtVaSetValues (fep->auto_proxies_p, XmNset, (state == 2), 0);

      XtVaSetValues (fep->proxy_text, XmNvalue, prefs->proxy_url, 0);
      if (state != 2) {
	XtVaSetValues (fep->proxy_text, XmNsensitive, False, 0);
	XtVaSetValues (fep->proxy_reload, XmNsensitive, False, 0);
      }
    }
  else if (current_network_page == xfe_PREFS_PROTOCOLS)
    {
      /* accept_cookie has 3 states. We are not implementing the disable cookie
	 state. */
      XtVaSetValues (fep->cookie_p, XmNset,
		     prefs->accept_cookie==NET_WhineAboutCookies, 0);
      XtVaSetValues (fep->anon_ftp_p, XmNset, prefs->email_anonftp, 0);
      XtVaSetValues (fep->email_form_p, XmNset, prefs->email_submit, 0);
    }
  else if (current_network_page == xfe_PREFS_LANG)
    {
#ifdef JAVA
      XtVaSetValues (fep->java_toggle, XmNset, !prefs->disable_java, 0);
#endif
#ifdef MOCHA
      XtVaSetValues (fep->javascript_toggle, XmNset, !prefs->disable_javascript, 0);
#endif
    }
  else
    {
      abort ();
    }
}

#ifdef DUPLICATE_SEC_PREFS
static void
fe_security_reset_1 (struct fe_prefs_data *fep, XFE_GlobalPrefs *prefs)
{
  if (current_security_page == xfe_PREFS_SEC_GENERAL)
    {
#ifdef HAVE_SECURITY
      XtVaSetValues (fep->enter_toggle, XmNset, prefs->enter_warn, 0);
      XtVaSetValues (fep->leave_toggle, XmNset, prefs->leave_warn, 0);
      XtVaSetValues (fep->mixed_toggle, XmNset, prefs->mixed_warn, 0);
      XtVaSetValues (fep->submit_toggle, XmNset, prefs->submit_warn, 0);
      XtVaSetValues (fep->ssl2_toggle, XmNset, prefs->ssl2_enable, 0);
      XtVaSetValues (fep->ssl3_toggle, XmNset, prefs->ssl3_enable, 0);
#endif /* HAVE_SECURITY */
    }
  else if (current_security_page == xfe_PREFS_SEC_PASSWORDS)
    {
      int state = prefs->ask_password;
      char buf[255];
      char *s;

#if 0
      if (SECNAV_GetPasswordEnabled())
	s = XP_GetString(XFE_CHANGE_PASSWORD);
      else
#endif
	s = XP_GetString(XFE_SET_PASSWORD);
      fe_SetString (fep->change_password, XmNlabelString, s);
      
      XtVaSetValues (fep->every_toggle, XmNset, (state == 0), 0);
      XtVaSetValues (fep->once_toggle, XmNset, (state == 1), 0);
      XtVaSetValues (fep->periodic_toggle, XmNset, (state == 2), 0);

      PR_snprintf (buf, sizeof (buf), "%d", prefs->password_timeout);
      XtVaSetValues (fep->periodic_text, XmNvalue, buf, 0);

      if (state != 2)
	XtVaSetValues (fep->periodic_text, XmNsensitive, False, 0);
    }
  else if (current_security_page == xfe_PREFS_SEC_PERSONAL)
    {
      Widget w;
      int nkids = 0;
      Widget *kids;
      int i;

      XtVaGetValues(fep->pers_cert_menu, XmNsubMenuId, &w, 0);
      XtVaGetValues(w, XmNnumChildren, &nkids, XmNchildren, &kids, 0);
      for (i=0; i<nkids; i++) {
	XmString xms;
	char *s = NULL;
	XtVaGetValues(kids[i], XmNlabelString, &xms, 0);
	XmStringGetLtoR(xms, XmFONTLIST_DEFAULT_TAG, &s);
	XmStringFree(xms);
	if (s && !strcmp(s, prefs->def_user_cert)) {
	  XtVaSetValues(fep->pers_cert_menu, XmNmenuHistory, kids[i], 0);
	  XtFree(s);
	  break;
	}
	if (s) XtFree(s);
      }
    }
  else if (current_security_page == xfe_PREFS_SEC_SITE)
    {
    }
  else
    {
      abort ();
    }
}
#endif /* DUPLICATE_SEC_PREFS */

static void fe_prefs_destroy_dialog (struct fe_prefs_data *);

static Widget
fe_prefs_get_form (struct fe_prefs_data *fep)
{
  Widget w = NULL;

  if (fep->general_form)
  {
    w = fep->general_form;
  }
  else if (fep->mailnews_form)
  {
    w = fep->mailnews_form;
  }
  else if (fep->network_form)
  {
    w = fep->network_form;
  }

#ifdef DUPLICATE_SEC_PREFS
  else if (fep->security_form)
  {
    w = fep->security_form;
  }
#endif /* DUPLICATE_SEC_PREFS */

  return w;
}


/*
 * fe_prefs_set_current_page
 */
static void
fe_prefs_set_current_page (struct fe_prefs_data *fep, int reset)
{
  int current_page;

  if (fep->general_form)
  {
    XtVaGetValues (fep->general_form, XmNactiveTab, &current_page, 0);
    current_general_page = current_page + xfe_GENERAL_OFFSET;
    if (reset)
    {
      /* Throw away the current information about mime types */
      NET_CleanupFileFormat(NULL);
      fe_globalData.privateMimetypeFileModifiedTime = 0;
      NET_CleanupMailCapList(NULL);
      fe_globalData.privateMailcapFileModifiedTime = 0;
 
      /* Re-load the default settings */
      fe_RegisterConverters();

      fe_general_reset_1 (fep, &fe_defaultPrefs);
    }
  }
  else if (fep->mailnews_form)
  {
    XtVaGetValues (fep->mailnews_form, XmNactiveTab, &current_page, 0);
    current_mailnews_page = current_page + xfe_MAILNEWS_OFFSET;
    if (reset) fe_mailnews_reset_1 (fep, &fe_defaultPrefs);
  }
  else if (fep->network_form)
  {
    XtVaGetValues (fep->network_form, XmNactiveTab, &current_page, 0);
    current_network_page = current_page + xfe_NETWORK_OFFSET;
    if (reset) fe_network_reset_1 (fep, &fe_defaultPrefs);
  }
#ifdef DUPLICATE_SEC_PREFS
  else if (fep->security_form)
  {
    XtVaGetValues (fep->security_form, XmNactiveTab, &current_page, 0);
    current_security_page = current_page + xfe_SECURITY_OFFSET;
    if (reset) fe_security_reset_1 (fep, &fe_defaultPrefs);
  }
#endif /* DUPLICATE_SEC_PREFS */
}


static void
fe_prefs_reset_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  fe_prefs_set_current_page(fep, TRUE);
}

static void fe_prefs_cancel_cb (Widget widget,
				XtPointer closure,
				XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;

  /* On a cancel of the general page, we will need undo any converter
   * changes that were made in the helpers page
   */
  if (fep->general_form && fep->helpers_changed)
    fe_prefs_set_current_page (fep, TRUE);

  fe_prefs_destroy_dialog (fep);
}

static void
fe_prefs_destroy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
/*  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;*/

  /* Simulate a cancel on destroy */
  fe_prefs_cancel_cb(widget, closure, call_data);
}

static void fe_general_verify_page (struct fe_prefs_data *);
static void fe_mailnews_verify_page (struct fe_prefs_data *);
static void fe_network_verify_page (struct fe_prefs_data *);
static void fe_security_verify_page (struct fe_prefs_data *);

static void
fe_general_done_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  MWContext *context = fep->context;
  Boolean b; /* paranoia: possibility of sizeof(Bool) != sizeof(Boolean) */

  XP_Bool old_underline_links = fe_globalPrefs.underline_links_p;
#if 0
  int old_followed_color = fe_globalPrefs.followed_link_color;
  int old_unfollowed_color = fe_globalPrefs.unfollowed_link_color;
#endif
  int old_history_expiration = fe_globalPrefs.global_history_expiration;

  Boolean regenerate_p = False;

  fe_general_verify_page (fep);

  /*  XtUnmanageChild (fep->shell); */

#define SNARF(SLOT1,SLOT2) \
  if (fe_globalPrefs.SLOT1) free (fe_globalPrefs.SLOT1); \
  XtVaGetValues (fep->SLOT2, XmNvalue, &fe_globalPrefs.SLOT1, 0)

  /* ============================================================ Bookmarks */

  SNARF (tmp_dir, tmp_text);
#if 0
  SNARF (bookmark_file, book_text);
#endif

  /* ============================================================ Helpers */


  /* ============================================================ Apps */

  SNARF (telnet_command, telnet_text);
  SNARF (tn3270_command, tn3270_text);
  SNARF (rlogin_command, rlogin_text);
  SNARF (rlogin_user_command, rlogin_user_text);

  /* ============================================================ Styles */

  XtVaGetValues (fep->icons_p, XmNset, &b, 0);
  fe_globalPrefs.toolbar_icons_p = b;
  XtVaGetValues (fep->text_p, XmNset, &b, 0);
  fe_globalPrefs.toolbar_text_p = b;
  XtVaGetValues (fep->both_p, XmNset, &b, 0);
  if (b)
    fe_globalPrefs.toolbar_icons_p = fe_globalPrefs.toolbar_text_p = True;
  XtVaGetValues (fep->tips_p, XmNset, &b, 0);
  fe_globalPrefs.toolbar_tips_p = b;

  XtVaGetValues (fep->browser_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.startup_mode = 0;
  XtVaGetValues (fep->mail_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.startup_mode = 1;
  XtVaGetValues (fep->news_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.startup_mode = 2;

  {
    char *old_home = fe_globalPrefs.home_document;
    struct fe_MWContext_cons *cons = NULL;

    XtVaGetValues (fep->blank_p, XmNset, &b, 0);
    if (b)
      fe_globalPrefs.home_document = strdup ("");
    else
      XtVaGetValues (fep->home_text, XmNvalue,&fe_globalPrefs.home_document,0);

    fe_globalPrefs.home_document = fe_StringTrim(fe_globalPrefs.home_document);

    /* Sensitize home menu item and toolbar button in all contexts */
    b = (fe_globalPrefs.home_document && *fe_globalPrefs.home_document);
    for (cons = fe_all_MWContexts; cons; cons = cons->next) {
      if (cons->context->type == MWContextBrowser) {
	if (CONTEXT_DATA (cons->context)->home_menuitem)
	  XtVaSetValues (CONTEXT_DATA (cons->context)->home_menuitem,
				XmNsensitive, b, 0);
	if (CONTEXT_DATA (cons->context)->show_toolbar_p &&
		CONTEXT_DATA (cons->context)->home_button)
	  XtVaSetValues (CONTEXT_DATA (cons->context)->home_button,
				XmNsensitive, b, 0);
      }
    }

    if (old_home) free (old_home);
  }

  /* ============================================================ Fonts */

  fe_get_scaled_font_size(fep);

  XtVaGetValues (fep->underline_p, XmNset, &b, 0);
  fe_globalPrefs.underline_links_p = b;

  XtVaGetValues (fep->never_expire_p, XmNset, &b, 0);

  fe_globalPrefs.global_history_expiration = -1;
  if (! b)
    {
      char *text, dummy;
      XtVaGetValues (fep->expire_days_text, XmNvalue, &text, 0);
      sscanf (text, " %d %c", &fe_globalPrefs.global_history_expiration,
              &dummy);
      if (fe_globalPrefs.global_history_expiration < 0)
        fe_globalPrefs.global_history_expiration = -1;
      free (text);
    }

  /* ============================================================ Images */

  if (fe_globalPrefs.dither_images)
      free (fe_globalPrefs.dither_images);
  fe_globalPrefs.dither_images = strdup (fep_to_dither_mode_pref_string (fep));
  XtVaGetValues (fep->while_loading_p, XmNset, &b, 0);
  fe_globalPrefs.streaming_images = b;


  /* ============================================================ */
#undef SNARF

  /* Install the new preferences first, in case the news host changed
     or something. */
  fe_InstallPreferences (fep->context);


  if ((fe_globalPrefs.toolbar_icons_p !=
        CONTEXT_DATA (fep->context)->show_toolbar_icons_p) ||
       (fe_globalPrefs.toolbar_text_p !=
        CONTEXT_DATA (fep->context)->show_toolbar_text_p))
    {
      CONTEXT_DATA (fep->context)->show_toolbar_icons_p
        = fe_globalPrefs.toolbar_icons_p;
      CONTEXT_DATA (fep->context)->show_toolbar_text_p
        = fe_globalPrefs.toolbar_text_p;
      if (CONTEXT_DATA (fep->context)->show_toolbar_p)
          regenerate_p = True;
    }

  if (fep->fonts_changed)
    {
      fep->fonts_changed = 0;
      fe_ReLayout (fep->context, 0);
    }
  else if (
#if 0
           old_followed_color != fe_globalPrefs.followed_link_color ||
           old_unfollowed_color != fe_globalPrefs.unfollowed_link_color ||
#endif
           old_history_expiration != fe_globalPrefs.global_history_expiration)
    {
      fe_RefreshAllAnchors ();
    }

  if (old_underline_links != fe_globalPrefs.underline_links_p)
    {
      struct fe_MWContext_cons *rest;
      for (rest = fe_all_MWContexts; rest; rest = rest->next)
        {
          Dimension w = 0, h = 0;

	  /* Should we allow mail/news? ### */ 
	  if (rest->context->type != MWContextBrowser) continue;

          XtVaGetValues (CONTEXT_DATA (rest->context)->drawing_area,
                         XmNwidth, &w, XmNheight, &h, 0);
          fe_ClearArea (rest->context, 0, 0, w, h);
          fe_RefreshArea (rest->context,
                          CONTEXT_DATA (rest->context)->document_x,
                          CONTEXT_DATA (rest->context)->document_y, w, h);
        }
    }

  /* Rebuilding a mail/news window results in a core dump right now.
   */
  if (regenerate_p && (fep->context->type == MWContextBrowser))
    {
#if 0
      /* AAAAUUUUGGGHHHH!!! AAAAUUUUUUUUGGGGGHHHHHHHH!!! AAAAAAUUUUGGGGGHHH!!!
         Motif is such a categorical piece of shit.  I hate my job, I hate
         my life, I want to destroy passers-by.

         We can't destroy the widget which is the parent of the dialog box
         here, because if we do, once we return to the command loop, we get
         a free memory reference processing  ButtonUp on the Preferences
         button on the Options menu in the menubar.  That's right, in the
         menubar.  It's processing a ButtonUp there after this dialog goes
         away, probably because of some bullshit to do with it being a modal
         dialog.  And it gets destroyed prematurely, which I THOUGHT was
         exactly what that fucking two-stage-destroy bullshit was supposed to
         prevent.

         AAAUUUUGGGGHHHHHH!!!!!!  Some sonofabitch is gonna pay for this one
         of these days, I fucking swear.  I'm going to go Postal Worker and
         take the whole lot of them out.

         So, instead of calling fe_RebuildWindow(), let's try just munging
         the widgets in the toolbar.
       */

      Dimension w, h, old_h;
/*      Dimension top_w = CONTEXT_DATA (fep->context)->top_area->core.width;*/
      Dimension top_h = CONTEXT_DATA (fep->context)->top_area->core.height;
      int delta;
      Widget *kids = 0;
      Cardinal nkids = 0;
      int i;
/*      XtUnmanageChild (CONTEXT_DATA (fep->context)->toolbar);*/
      XtUnrealizeWidget (CONTEXT_DATA (fep->context)->top_area);
      XtVaGetValues (CONTEXT_DATA (fep->context)->toolbar,
                     XmNwidth, &w, XmNheight, &h,
                     XmNchildren, &kids, XmNnumChildren, &nkids, 0);

      old_h = kids[0]->core.height;

      /* Gag!! The fucking toolbar keeps wanting to go to two lines... */
      XtVaGetValues (XtParent (CONTEXT_DATA (fep->context)->toolbar),
                     XmNwidth, &w, 0);
      CONTEXT_DATA (fep->context)->show_toolbar_icons_p
        = fe_globalPrefs.toolbar_icons_p;
      for (i = 0; i < nkids; i++)
        {
          Pixmap p, ip;
          if (!strcmp ("spacer", XtName (kids [i]))) /* gag me */
            continue;
          p  = fe_ToolbarPixmap (fep->context, i, False, False);
          ip = fe_ToolbarPixmap (fep->context, i, True,  False);
          if (! ip) ip = p;
          XtVaSetValues (kids [i],
                         XmNlabelType, (fe_globalPrefs.toolbar_icons_p
                                        ? XmPIXMAP : XmSTRING),
                         XmNlabelPixmap, p,
                         XmNlabelInsensitivePixmap, ip,
                         0);
        }

      /* Maybe it will help to break this all the way down. */
/*      XtUnrealizeWidget (CONTEXT_DATA (fep->context)->top_area);*/

      h = kids[0]->core.height;
      XtVaSetValues (CONTEXT_DATA (fep->context)->toolbar,
                     XmNwidth, w, XmNheight, h, 0);

      XtManageChild (CONTEXT_DATA (fep->context)->toolbar);

      /* Force the width back to what it was before.  Don't force the height
         back, since that might legitimately have changed.
       */
/*      XtManageChild (CONTEXT_DATA (fep->context)->top_area);*/

      XtRealizeWidget (CONTEXT_DATA (fep->context)->top_area);
      XtManageChild (CONTEXT_DATA (fep->context)->top_area);

      delta = h - old_h;
      XtVaSetValues (CONTEXT_DATA (fep->context)->top_area,
                     XmNheight, top_h + delta, 0);
/*      XtResizeWidget (CONTEXT_DATA (fep->context)->top_area,
                      top_w, top_h + delta, 0); */
      XtUnmanageChild (XtParent (CONTEXT_DATA (fep->context)->top_area));
      XtManageChild (XtParent (CONTEXT_DATA (fep->context)->top_area));
#endif
    }

  /* This seems like a better way to do things.
   *
   * First destroy the dialog and it's children. We've extracted
   * the user-entered pref data, so we don't need it any more.
   * Then call our event loop and let all the X events get handled.
   * Finally, call fe_RebuildWindow() to blow away the toolbar
   * widgets, and reconstruct them correctly.
   */

  /* Destroy the folder widget */
  fe_prefs_destroy_dialog (fep);
  fe_EventLoop ();

  /* Now rebuild the window if necessary */
  if (regenerate_p && (context->type == MWContextBrowser))
    fe_RebuildWindow (context);

  /* Save the preferences at the end, so that if we've found some setting that
     crashes, it won't get saved... */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));
}


static void
fe_mailnews_done_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  MWContext *fep_context = fep->context;
  Boolean b; /* paranoia: possibility of sizeof(Bool) != sizeof(Boolean) */
  int i;
  Boolean reload_required_p = False;
  char *s;

  MSG_FONT citation_font;
  MSG_CITATION_SIZE citation_size;
  char *citation_color;

  fe_mailnews_verify_page (fep);

  /*  XtUnmanageChild (fep->shell); */

#define SNARF(SLOT1,SLOT2) \
  if (fe_globalPrefs.SLOT1) free (fe_globalPrefs.SLOT1); \
  XtVaGetValues (fep->SLOT2, XmNvalue, &fe_globalPrefs.SLOT1, 0)

  /* ============================================================ Appearance */

  XtVaGetValues (fep->fixed_message_font_p, XmNset, &b, 0);
  if (b != fe_globalPrefs.fixed_message_font_p) reload_required_p = True;
  fe_globalPrefs.fixed_message_font_p = b;

  citation_font = fe_globalPrefs.citation_font;
  XtVaGetValues (fep->cite_plain_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_font = MSG_PlainFont;
  XtVaGetValues (fep->cite_bold_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_font = MSG_BoldFont;
  XtVaGetValues (fep->cite_italic_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_font = MSG_ItalicFont;
  XtVaGetValues (fep->cite_bold_italic_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_font = MSG_BoldItalicFont;
  if (citation_font != fe_globalPrefs.citation_font) reload_required_p = True;

  citation_size = fe_globalPrefs.citation_size;
  XtVaGetValues (fep->cite_normal_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_size = MSG_NormalSize;
  XtVaGetValues (fep->cite_bigger_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_size = MSG_Bigger;
  XtVaGetValues (fep->cite_smaller_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.citation_size = MSG_Smaller;
  if (citation_size != fe_globalPrefs.citation_size) reload_required_p = True;

  citation_color = fe_globalPrefs.citation_color;
  fe_globalPrefs.citation_color = XmTextFieldGetString(fep->cite_color_text);
  if (citation_color && fe_globalPrefs.citation_color &&
      strcasecmp(citation_color, fe_globalPrefs.citation_color))
	reload_required_p = True;
  if (citation_color) XtFree(citation_color);

  MSG_SetCitationStyle (fep->context,
                        fe_globalPrefs.citation_font,
                        fe_globalPrefs.citation_size,
                        fe_globalPrefs.citation_color);

  XtVaGetValues (fep->mail_horiz_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.mail_pane_style = FE_PANES_NORMAL;
  XtVaGetValues (fep->mail_vert_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.mail_pane_style = FE_PANES_HORIZONTAL;
  XtVaGetValues (fep->mail_stack_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.mail_pane_style = FE_PANES_STACKED;
  XtVaGetValues (fep->mail_tall_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.mail_pane_style = FE_PANES_TALL_FOLDERS;

  XtVaGetValues (fep->news_horiz_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.news_pane_style = FE_PANES_NORMAL;
  XtVaGetValues (fep->news_vert_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.news_pane_style = FE_PANES_HORIZONTAL;
  XtVaGetValues (fep->news_stack_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.news_pane_style = FE_PANES_STACKED;
  XtVaGetValues (fep->news_tall_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.news_pane_style = FE_PANES_TALL_FOLDERS;

  /* ============================================================ Mail */

  SNARF (movemail_program, movemail_text);
  SNARF (pop3_host, srvr_text);

  SNARF (pop3_user_id, user_text);

  XtVaGetValues (fep->pop_toggle, XmNset, &b, 0);
  fe_globalPrefs.use_movemail_p = !b;

  if (fe_globalPrefs.use_movemail_p) {
    XtVaGetValues (fep->builtin_toggle, XmNset, &b, 0);
    fe_globalPrefs.builtin_movemail_p = b;
  }

  SNARF (mailhost, smtp_text);

  if (fe_globalPrefs.real_name) {
    free (fe_globalPrefs.real_name);
  }
  fe_globalPrefs.real_name = fe_GetTextField (fep->user_name_text);

  SNARF (email_address, user_mail_text);

  if (fe_globalPrefs.organization) {
    free (fe_globalPrefs.organization);
  }
  fe_globalPrefs.organization = fe_GetTextField (fep->user_org_text);

  /* Backend doesn't store mail_directory. So it doesn't know if this
     really changed or not when we install the prefs. We have to do this
     explicitly here. */
  XtVaGetValues(fep->maildir_text, XmNvalue, &s, 0);
  if (strcmp(s, fe_globalPrefs.mail_directory)) {
    if (fe_globalPrefs.mail_directory) free(fe_globalPrefs.mail_directory);
    fe_globalPrefs.mail_directory = s;
    MSG_MailDirectoryChanged (fep->context);
  }
#ifdef PREFS_SIG
  SNARF (signature_file, user_sig_text);
#endif

  XtVaGetValues (fep->msg_limit, XmNset, &b, 0);
  fe_globalPrefs.pop3_msg_size_limit_p = b;
  {
    char c, *s = 0;
    int n;
    XtVaGetValues (fep->limit_text, XmNvalue, &s, 0);
    fe_globalPrefs.pop3_msg_size_limit = 0;
    if (1 == sscanf (s, " %d %c", &n, &c))
      fe_globalPrefs.pop3_msg_size_limit = n;
    free (s);
  }

  XtVaGetValues (fep->msg_leave, XmNset, &b, 0);
  fe_globalPrefs.pop3_leave_mail_on_server = b;

  XtVaGetValues (fep->check_every, XmNset, &b, 0);
  fe_globalPrefs.auto_check_mail = b;
  if (b)
  {
    char c, *s = 0;
    int n;
    XtVaGetValues (fep->check_text, XmNvalue, &s, 0);
    fe_globalPrefs.biff_interval = 0;
    if (1 == sscanf (s, " %d %c", &n, &c))
      fe_globalPrefs.biff_interval = n;
    free (s);
  }

#ifdef PREFS_8BIT
  XtVaGetValues (fep->qp_toggle, XmNset, &b, 0);
  fe_globalPrefs.qp_p = b;
  /* eightbit_toggle?? */
#endif
#ifdef PREFS_QUEUED_DELIVERY
  XtVaGetValues (fep->deliverQ_toggle,
			XmNset, &fe_globalPrefs.queue_for_later_p, 0);
#endif
  SNARF (mail_bcc, mMailOutOther_text);
  SNARF (news_bcc, nMailOutOther_text);
  SNARF (mail_fcc, mCopyOut_text);
  SNARF (news_fcc, nCopyOut_text);

  XtVaGetValues (fep->autoquote_toggle, XmNset,
			&fe_globalPrefs.autoquote_reply, 0);
  XtVaGetValues (fep->mMailOutSelf_toggle, XmNset,
			&fe_globalPrefs.mailbccself_p, 0);
  XtVaGetValues (fep->nMailOutSelf_toggle, XmNset,
			&fe_globalPrefs.newsbccself_p, 0);

  SNARF (newshost, newshost_text);
  /*  SNARF (newsrc_directory, newsrc_text); */
  /* Backend doesn't store newsrc_directory. So it doesn't know if this
     really changed or not when we install the prefs. We have to do this
     explicitly here. */
  XtVaGetValues(fep->newsrc_text, XmNvalue, &s, 0);
  if (strcmp(s, fe_globalPrefs.newsrc_directory)) {
    if (fe_globalPrefs.newsrc_directory) free(fe_globalPrefs.newsrc_directory);
    fe_globalPrefs.newsrc_directory = s;
    /* set FE_UserNewsRC to the new newsrc directory */
    FE_UserNewsRC = s;
    MSG_NewsDirectoryChanged (fep->context);
  }
/*  SNARF (news_max_articles, newsmax_text); */
#ifdef PREFS_NEWS_MAX
  {
    char c, *s = 0;
    int n;
    XtVaGetValues (fep->newsmax_text, XmNvalue, &s, 0);
    fe_globalPrefs.news_max_articles = 0;
    if (1 == sscanf (s, " %d %c", &n, &c))
      fe_globalPrefs.news_max_articles = n;
    free (s);
  }
#endif
  /* ============================================================ Organization */
#ifdef PREFS_EMPTY_TRASH
    XtVaGetValues (fep->emptyTrash_toggle, XmNset, &fe_globalPrefs.emptyTrash, 0);
#endif
    XtVaGetValues (fep->rememberPswd_toggle, XmNset, &fe_globalPrefs.rememberPswd, 0);
    XtVaGetValues (fep->threadmail_toggle, XmNset, &fe_globalPrefs.mail_thread_p, 0);
    XtVaGetValues (fep->threadnews_toggle, XmNset, &b, 0);
    fe_globalPrefs.no_news_thread_p = !b;

    i = 0;
    XtVaGetValues (fep->mdate_toggle, XmNset, &b, 0);
    if (!b) {
      i++;
      XtVaGetValues (fep->mnum_toggle,  XmNset, &b, 0);
      if (!b) {
	i++;
	XtVaGetValues (fep->msubject_toggle,  XmNset, &b, 0);
	if (!b) {
	  i++;
	  XtVaGetValues (fep->msender_toggle,  XmNset, &b, 0);
	}
      }
    }
    if (i < 0 || i > 3) i = 0;
    fe_globalPrefs.mail_sort_style = i;

    i = 0;
    XtVaGetValues (fep->ndate_toggle, XmNset, &b, 0);
    if (!b) {
      i++;
      XtVaGetValues (fep->nnum_toggle,  XmNset, &b, 0);
      if (!b) {
	i++;
	XtVaGetValues (fep->nsubject_toggle,  XmNset, &b, 0);
	if (!b) {
	  i++;
	  XtVaGetValues (fep->nsender_toggle,  XmNset, &b, 0);
	}
      }
    }
    if (i < 0 || i > 3) i = 0;
    fe_globalPrefs.news_sort_style = i;


  /* ============================================================ */

  /* Install the new preferences first, in case the news host changed
     or something. */
  fe_InstallPreferences (fep->context);

  /* Save the preferences at the end, so that if we've found some setting that
     crashes, it won't get saved... */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fep->context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));

  /* Destroy the folder widget */
  fe_prefs_destroy_dialog (fep);

  /* Reload the mail/news window, if required */
  if (reload_required_p)
    fe_ReLayout (fep_context, NET_NORMAL_RELOAD);
}

static void
fe_network_done_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;
  char *s1;
  Boolean b; /* paranoia: possibility of sizeof(Bool) != sizeof(Boolean) */

  fe_network_verify_page (fep);

  /*  XtUnmanageChild (fep->shell); */

#define SNARF(SLOT1,SLOT2) \
  if (fe_globalPrefs.SLOT1) free (fe_globalPrefs.SLOT1); \
  XtVaGetValues (fep->SLOT2, XmNvalue, &fe_globalPrefs.SLOT1, 0)

# define SNARFB(SLOT1,SLOT2) \
    XtVaGetValues (fep->SLOT1, XmNset, &b, 0); \
    fe_globalPrefs.SLOT2 = b

  /* ============================================================ Cache */

  {
    char *text, dummy;
    int size = 0;
    XtVaGetValues (fep->memory_text, XmNvalue, &text, 0);
    if (1 == sscanf (text, " %d %c", &size, &dummy) &&
	size >= 0)
      fe_globalPrefs.memory_cache_size = size;
    free (text);
#ifdef PREFS_DISK_CACHE
    XtVaGetValues (fep->disk_text, XmNvalue, &text, 0);
    if (1 == sscanf (text, " %d %c", &size, &dummy) &&
	size >= 0)
      fe_globalPrefs.disk_cache_size = size;
    free (text);
    SNARF (cache_dir, disk_dir);
#endif /* PREFS_DISK_CACHE */
#ifdef PREFS_VERIFY
  XtVaGetValues (fep->once_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.verify_documents = 0;
  XtVaGetValues (fep->every_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.verify_documents = 1;
  XtVaGetValues (fep->expired_p, XmNset, &b, 0);
  if (b) fe_globalPrefs.verify_documents = 2;
  XtVaGetValues (fep->cache_ssl_p, XmNset, &b, 0);
  fe_globalPrefs.cache_ssl_p = b;
  
#endif

#ifdef PREFS_NET
    XtVaGetValues (fep->buf_text, XmNvalue, &text, 0);
    if (1 == sscanf (text, " %d %c", &size, &dummy) &&
	size > 0)
      fe_globalPrefs.network_buffer_size = size;
    free (text);
    XtVaGetValues (fep->conn_text, XmNvalue, &text, 0);
    if (1 == sscanf (text, " %d %c", &size, &dummy) &&
	size > 0)
      {
	if (size > 20) size = 20;
	fe_globalPrefs.max_connections = size;
      }
    free (text);
#endif /* PREFS_NET */
  }

  /* ============================================================ Proxies */
 
  XtVaGetValues (fep->proxy_text, XmNvalue, &s1, 0);
  if (*s1) {
    NET_SetProxyServer(PROXY_AUTOCONF_URL, s1);
    fe_globalPrefs.proxy_url = XP_STRDUP (s1);
  } else {
    if (fe_globalPrefs.proxy_url) XP_FREE (fe_globalPrefs.proxy_url);
    fe_globalPrefs.proxy_url = 0;
  }

  XtVaGetValues (fep->no_proxies_p, XmNset, &b, 0);
  if (b) {
    NET_SelectProxyStyle(PROXY_STYLE_NONE);
    fe_globalPrefs.proxy_mode = 0;
  }

  XtVaGetValues (fep->manual_p, XmNset, &b, 0);
  if (b) {
    NET_SelectProxyStyle(PROXY_STYLE_MANUAL);
    fe_globalPrefs.proxy_mode = 1;
  }

  XtVaGetValues (fep->auto_proxies_p, XmNset, &b, 0);
  if (b) {
    NET_SelectProxyStyle(PROXY_STYLE_AUTOMATIC);
    fe_globalPrefs.proxy_mode = 2;
  }

  /* ============================================================ Protocols */
  XtVaGetValues (fep->cookie_p, XmNset, &b, 0);
  if (b)
    fe_globalPrefs.accept_cookie = NET_WhineAboutCookies;
  else
    fe_globalPrefs.accept_cookie = NET_SilentCookies;
  XtVaGetValues (fep->anon_ftp_p, XmNset, &b, 0);
  fe_globalPrefs.email_anonftp = b;
  XtVaGetValues (fep->email_form_p, XmNset, &b, 0);
  fe_globalPrefs.email_submit = b;

  /* ============================================================ Languages */
#ifdef JAVA
  XtVaGetValues (fep->java_toggle, XmNset, &b, 0);
  fe_globalPrefs.disable_java = !b;
  LJ_SetJavaEnabled(!fe_globalPrefs.disable_java);
#endif
#ifdef MOCHA
  XtVaGetValues (fep->javascript_toggle, XmNset, &b, 0);
  fe_globalPrefs.disable_javascript = !b;
  LM_SwitchMocha(!fe_globalPrefs.disable_javascript);
#endif

  /* ============================================================ */

  /* Install the new preferences first, in case the news host changed
     or something. */
  fe_InstallPreferences (fep->context);

  /* Save the preferences at the end, so that if we've found some setting that
     crashes, it won't get saved... */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fep->context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));

  /* Destroy the folder widget */
  fe_prefs_destroy_dialog (fep);
}

#ifdef DUPLICATE_SEC_PREFS
static void
fe_security_done_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  struct fe_prefs_data *fep = (struct fe_prefs_data *) closure;

  fe_security_verify_page (fep);

  /*  XtUnmanageChild (fep->shell); */

#define SNARF(SLOT1,SLOT2) \
  if (fe_globalPrefs.SLOT1) free (fe_globalPrefs.SLOT1); \
  XtVaGetValues (fep->SLOT2, XmNvalue, &fe_globalPrefs.SLOT1, 0)

  /* ============================================================ Security */

#ifdef HAVE_SECURITY
  {
    Boolean b;
# define SNARFB(SLOT1,SLOT2) \
    XtVaGetValues (fep->SLOT1, XmNset, &b, 0); \
    fe_globalPrefs.SLOT2 = b

    SNARFB (enter_toggle, enter_warn);
    SNARFB (leave_toggle, leave_warn);
    SNARFB (mixed_toggle, mixed_warn);
    SNARFB (submit_toggle, submit_warn);
    SNARFB (ssl2_toggle, ssl2_enable);
    SSL_EnableDefault(SSL_ENABLE_SSL2, fe_globalPrefs.ssl2_enable);

    SNARFB (ssl3_toggle, ssl3_enable);
    SSL_EnableDefault(SSL_ENABLE_SSL3, fe_globalPrefs.ssl3_enable);
# undef SNARFB
  }
#endif /* HAVE_SECURITY */

  /* ============================================================ Security */

  {
    Boolean ask_pass, b;
    char *s1;
    int timeout;

    XtVaGetValues (fep->every_toggle, XmNset, &b, 0);
    if (b) fe_globalPrefs.ask_password = ask_pass = 0;

    XtVaGetValues (fep->once_toggle, XmNset, &b, 0);
    if (b) fe_globalPrefs.ask_password = ask_pass = 1;

    XtVaGetValues (fep->periodic_toggle, XmNset, &b, 0);
    if (b) fe_globalPrefs.ask_password = ask_pass = 2;

    XtVaGetValues (fep->periodic_text, XmNvalue, &s1, 0);
    fe_globalPrefs.password_timeout = timeout = atoi (s1);
    XP_FREE (s1);
  }

  /* ============================================================ */
  {
    char *s = NULL;
    Widget w;
    XmString xms;
    XtVaGetValues(fep->pers_cert_menu, XmNmenuHistory, &w, 0);
    XtVaGetValues(w, XmNlabelString, &xms, 0);
    XmStringGetLtoR(xms, XmFONTLIST_DEFAULT_TAG, &s);
    if (s) {
      fe_globalPrefs.def_user_cert = s;
    }
  }
  /* ============================================================ */

  /* Install the new preferences first, in case the news host changed
     or something. */
  fe_InstallPreferences (fep->context);

  /* Save the preferences at the end, so that if we've found some setting that
     crashes, it won't get saved... */
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror (fep->context, XP_GetString( XFE_ERROR_SAVING_OPTIONS));

  /* Destroy the folder widget */
  fe_prefs_destroy_dialog (fep);
}
#endif /* DUPLICATE_SEC_PREFS */


void
_myDestroyWidget (Widget widget)
{
  XP_ASSERT (widget);
  XtDestroyWidget (widget);
}

static void
fe_prefs_destroy_dialog (struct fe_prefs_data *fep)
{
  int i;
  WidgetList tabs;

  fe_prefs_set_current_page(fep, FALSE);

  /* This seems to be the only way to make the Microline
   * folder widget happy. Without doing this, BuildTabList
   * in Folder.c dumps in XtIsSubclass.
   * Yes, hack central.
   */

  XtVaGetValues (fe_prefs_get_form (fep),
		XmNtabCount, &i,
		XmNtabWidgetList, &tabs,
		NULL);

  /*  XtUnrealizeWidget (fep->shell); */

  while (i--) {
/*    printf("deleting %d\n", i); */
    _myDestroyWidget (tabs[i]);
  }

  /* ### not the best way to do this */
  XtRemoveCallback (fep->shell, XmNokCallback, fe_general_done_cb, fep);
  XtRemoveCallback (fep->shell, XmNokCallback, fe_mailnews_done_cb, fep);
  XtRemoveCallback (fep->shell, XmNokCallback, fe_network_done_cb, fep);
#ifdef DUPLICATE_SEC_PREFS
  XtRemoveCallback (fep->shell, XmNokCallback, fe_security_done_cb, fep);
#endif /* DUPLICATE_SEC_PREFS */
  XtRemoveCallback (fep->shell, XmNcancelCallback, fe_prefs_reset_cb, fep);
  XtRemoveCallback (fep->shell, XmNapplyCallback,  fe_prefs_cancel_cb, fep);
  XtRemoveCallback (fep->shell, XmNdestroyCallback, fe_prefs_destroy_cb, fep);

  if (CONTEXT_DATA (fep->context)->currentPrefDialog) {
    XP_ASSERT (fep->shell == CONTEXT_DATA (fep->context)->currentPrefDialog);
    CONTEXT_DATA (fep->context)->currentPrefDialog = 0;
    CONTEXT_DATA (fep->context)->fep = NULL;
  }

  _myDestroyWidget (fep->shell);

  /* Cleanup helpers */
  if (fep->helpers) {
    fe_helpers_prepareForDestroy(fep->helpers);
    XP_FREE(fep->helpers);
  }

  /* Proxies page has been implemented using a local event loop and as a
   * modal widget of the context (and not the prefdialog). So if the pref
   * dialog got destroyed, the proxies page would still remain. Bring it
   * down.
   */
  proxies_done = 0;

  free (fep);
}
  
#define CHECK_FILE(W,DESC,MSG,N)				\
  {								\
    char *text = 0;						\
    XtVaGetValues (W, XmNvalue, &text, 0);			\
    text = fe_StringTrim (text);				\
    if (!text || !*text || stat (text, &st))			\
      {								\
        PR_snprintf (MSG, 200,					\
		XP_GetString(XFE_FILE_DOES_NOT_EXIST),		\
                DESC, (text ? text : ""));			\
        MSG += strlen (MSG);					\
      }								\
    if (text) free (text);					\
  }

#define CHECK_HOST_1(TEXT,DESC,MSG,N)				      \
  {								      \
    int d; char c;						      \
    PRHostEnt hpbuf;						      \
    char dbbuf[PR_NETDB_BUF_SIZE];				      \
    if (TEXT && 4 == sscanf (TEXT, " %d.%d.%d.%d %c",		      \
                                &d, &d, &d, &d, &c))		      \
      /* IP addresses are ok */ ;				      \
    else if (!TEXT || !*TEXT ||					      \
	     !PR_gethostbyname(TEXT,&hpbuf, dbbuf, sizeof(dbbuf), 0)) \
      {								      \
        PR_snprintf (MSG, 200,					      \
		    XP_GetString(XFE_HOST_IS_UNKNOWN),		      \
                    DESC, (TEXT ? TEXT : ""));			      \
         MSG += strlen (MSG);					      \
      }								      \
  }

#define CHECK_HOST(W,DESC,MSG,N)				\
  {								\
    char *text = 0;						\
    XtVaGetValues (W, XmNvalue, &text, 0);			\
    text = fe_StringTrim (text);				\
    CHECK_HOST_1 (text, DESC, MSG, N);				\
    if (text) free (text);					\
  }

#define CHECK_PROXY(W1,W2,DESC,PORT_REQUIRED,MSG,N)		\
  {								\
    char *text = 0;						\
    XtVaGetValues (W1, XmNvalue, &text, 0);			\
    text = fe_StringTrim (text);				\
    if (text && *text)						\
      {								\
        CHECK_HOST_1 (text, DESC, MSG, N);			\
        free (text);						\
        text = 0;						\
        XtVaGetValues (W2, XmNvalue, &text, 0);			\
        text = fe_StringTrim (text);				\
        if ((!text || !*text) && PORT_REQUIRED)			\
          {							\
            PR_snprintf (MSG, 200,				\
			XP_GetString(XFE_NO_PORT_NUMBER), DESC); \
            MSG += strlen (MSG);				\
          }							\
      }								\
    if (text) free (text);					\
  }


static void
fe_general_verify_page (struct fe_prefs_data *fep)
{
  char buf [10000];
  char *buf2, *warning;
  struct stat st;
  int size;

  buf2 = buf;
  strcpy (buf, XP_GetString(XFE_WARNING));
  buf2 = buf + strlen (buf);
  warning = buf2;
  size = buf + sizeof (buf) - warning;

  if (current_general_page == xfe_PREFS_APPS)
    {
      CHECK_FILE (fep->tmp_text,
		  XP_GetString(XFE_TEMP_DIRECTORY), warning, size);
    }
  else if (current_general_page == xfe_PREFS_FONTS)
    {
      /* Nothing to check here */
    }
  else if (current_general_page == xfe_PREFS_HELPERS)
    {
      /* No longer needs, these files is not modifiable via dialog*/
    }
  else if (current_general_page == xfe_PREFS_IMAGES)
    {
      /* Nothing to check here */
    }
  else if (current_general_page == xfe_PREFS_STYLES)
    {
      /* Nothing to check here */
      /* #### home page is a URL - should search for a file if "file:",
	 or should check host name if anything else. */
    }
  else if (current_general_page == xfe_PREFS_LANGUAGES)
    {
      /* Nothing to check here */
    }
  else if (current_general_page)
    {
      abort ();
    }

  if (*buf2)
    FE_Alert (fep->context, fe_StringTrim (buf));
}

static void
fe_mailnews_verify_page (struct fe_prefs_data *fep)
{
  char buf [10000];
  char *buf2, *warning;
  struct stat st;
  int size;

  buf2 = buf;
  strcpy (buf, XP_GetString(XFE_WARNING));
  buf2 = buf + strlen (buf);
  warning = buf2;
  size = buf + sizeof (buf) - warning;

  if (current_mailnews_page == xfe_PREFS_APPEARANCE)
    {
      /* Nothing to check here yet */
    }
  else if (current_mailnews_page == xfe_PREFS_COMPOSITION)
    {
      /* Nothing to check here */
      /* #### validate bcc addresses somehow? */
    }
  else if (current_mailnews_page == xfe_PREFS_SERVERS)
    {
      CHECK_HOST (fep->smtp_text, XP_GetString(XFE_MAIL_HOST), warning, size);
      CHECK_HOST (fep->newshost_text,
		  XP_GetString(XFE_NEWS_HOST), warning, size);
      CHECK_FILE (fep->newsrc_text,
		  XP_GetString(XFE_NEWS_RC_DIRECTORY),
		  warning, size);
    }
  else if (current_mailnews_page == xfe_PREFS_IDENTITY)
    {
      /* Nothing to check here */
    }
  else if (current_mailnews_page == xfe_PREFS_ORGANIZATION)
    {
      /* Nothing to check here */
    }
  else if (current_mailnews_page)
    {
      abort ();
    }

  if (*buf2)
    FE_Alert (fep->context, fe_StringTrim (buf));
}

void
fe_SetMailNewsSortBehavior(MWContext* context, XP_Bool thread, int sortcode)
{
  XP_ASSERT(context->type == MWContextMail || context->type == MWContextNews);
  if (context->type == MWContextMail || context->type == MWContextNews) {
    MSG_SetToggleStatus(context, MSG_ThreadMessages,
			thread ? MSG_Checked : MSG_Unchecked);
    switch (sortcode) {
    case 0:
      MSG_Command(context, MSG_SortByDate);
      break;
    case 1:
      MSG_Command(context, MSG_SortByMessageNumber);
      break;
    case 2:
      MSG_Command(context, MSG_SortBySubject);
      break;
    case 3:
      MSG_Command(context, MSG_SortBySender);
      break;
    }
  }
}

static void
fe_proxy_verify_page (struct fe_prefs_data *fep)
{
  char buf [10000];
  char *buf2, *warning;
  int size;

  Widget shell = 0;

  /* Display errors on top of the pref dialog rather than the proxy page
   * as the proxy page will automatically go away. That would take this
   * error dialog with it. So the user wont see the error message.
   *
   * The real problem is that we should remove the way the proxy page
   * is being displayed with the
   *	while(proxy_done)
   *		fe_EventLoop();
   * This is a diaster. We could just make this a modal dialog and not have
   * do the above. But it is too late to change this in b6 for 3.0
   * - dp
   */
  if (fep->shell)
    shell = fep->shell;

  buf2 = buf;
  strcpy (buf, XP_GetString(XFE_WARNING));
  buf2 = buf + strlen (buf);
  warning = buf2;
  size = buf + sizeof (buf) - warning;

  CHECK_PROXY (fep->ftp_text,   fep->ftp_port,
	       XP_GetString(XFE_FTP_PROXY_HOST),   True, warning, size);
  CHECK_PROXY (fep->gopher_text,fep->gopher_port,
	       XP_GetString(XFE_GOPHER_PROXY_HOST),True, warning, size);
  CHECK_PROXY (fep->http_text,  fep->http_port,
	       XP_GetString(XFE_HTTP_PROXY_HOST),  True, warning, size);
#ifdef HAVE_SECURITY
  CHECK_PROXY (fep->https_text, fep->https_port,
	       XP_GetString(XFE_HTTPS_PROXY_HOST), True, warning, size);
#endif
  CHECK_PROXY (fep->wais_text,   fep->wais_port,
	       XP_GetString(XFE_WAIS_PROXY_HOST),  True, warning, size);
  CHECK_PROXY (fep->socks_text,  fep->socks_port,
	       XP_GetString(XFE_SOCKS_HOST),      False, warning, size);
      /* #### check no_proxy */

  if (*buf2)
    if (shell)
      fe_Alert_2(shell, fe_StringTrim(buf));
    else
      FE_Alert (fep->context, fe_StringTrim (buf));
}
static void
fe_network_verify_page (struct fe_prefs_data *fep)
{
  char buf [10000];
  char *buf2, *warning;
  int size;

  buf2 = buf;
  strcpy (buf, XP_GetString(XFE_WARNING));
  buf2 = buf + strlen (buf);
  warning = buf2;
  size = buf + sizeof (buf) - warning;

  if ((current_network_page == xfe_PREFS_CACHE) ||
      (current_network_page == xfe_PREFS_NETWORK) ||
      (current_network_page == xfe_PREFS_PROXIES) ||
      (current_network_page == xfe_PREFS_PROTOCOLS) ||
      (current_network_page == xfe_PREFS_LANG))
    {
      /* Nothing to check here */
    }
  else if (current_network_page)
    {
      abort ();
    }

  if (*buf2)
    FE_Alert (fep->context, fe_StringTrim (buf));
}

#ifdef DUPLICATE_SEC_PREFS
static void
fe_security_verify_page (struct fe_prefs_data *fep)
{
  char buf [10000];
  char *buf2, *warning;

  buf2 = buf;
  strcpy (buf, XP_GetString(XFE_WARNING));
  buf2 = buf + strlen (buf);
  warning = buf2;


  if (current_security_page == xfe_PREFS_SEC_GENERAL)
    {
      /* Nothing to check here */
    }
  else if (current_security_page == xfe_PREFS_SEC_PASSWORDS)
    {
      /* Nothing to check here */
    }
  else if (current_security_page == xfe_PREFS_SEC_PERSONAL)
    {
      /* Nothing to check here */
    }
  else if (current_security_page == xfe_PREFS_SEC_SITE)
    {
      /* Nothing to check here */
    }
  else if (current_security_page)
    {
      abort ();
    }

  if (*buf2)
    FE_Alert (fep->context, fe_StringTrim (buf));
}
#endif /* DUPLICATE_SEC_PREFS */

#undef CHECK_FILE
#undef CHECK_HOST
#undef CHECK_PROXY

void
fe_InstallPreferences (MWContext *context)
{
  /* OPTIONS MENU
     show_toolbar_p
     show_url_p
     show_directory_buttons_p
     autoload_images_p
     fancy_ftp_p
     show_security_bar_p
   */

  /* APPLICATIONS
     tn3270_command
     telnet_command
     rlogin_command
     rlogin_user_command
   */

  /* CACHE
   */
  FE_CacheDir = fe_globalPrefs.cache_dir;
  NET_SetMemoryCacheSize (fe_globalPrefs.memory_cache_size * 1024);
  NET_SetDiskCacheSize (fe_globalPrefs.disk_cache_size * 1024);
  NET_SetCacheUseMethod (fe_globalPrefs.verify_documents == 1
			 ? CU_CHECK_ALL_THE_TIME
			 : fe_globalPrefs.verify_documents == 2
			 ? CU_NEVER_CHECK
			 : CU_CHECK_PER_SESSION);
  NET_DontDiskCacheSSL(!fe_globalPrefs.cache_ssl_p);

  /* COLORS
   */

  /* COMPOSITION
   */
  MIME_ConformToStandard (fe_globalPrefs.qp_p);
  /* #### send_formatted_text_p */
  /* #### queue_for_later_p */

  MSG_SetDefaultHeaderContents (context,
				MSG_BCC_HEADER_MASK, fe_globalPrefs.mail_bcc,
				FALSE);
  MSG_SetDefaultHeaderContents (context,
				MSG_BCC_HEADER_MASK, fe_globalPrefs.news_bcc,
				TRUE);
  MSG_SetDefaultHeaderContents (context,
				MSG_FCC_HEADER_MASK, fe_globalPrefs.mail_fcc,
				FALSE);
  MSG_SetDefaultHeaderContents (context,
				MSG_FCC_HEADER_MASK, fe_globalPrefs.news_fcc,
				TRUE);
  MSG_SetDefaultBccSelf(context, fe_globalPrefs.mailbccself_p, FALSE);
  MSG_SetDefaultBccSelf(context, fe_globalPrefs.newsbccself_p, TRUE);

  /* DIRECTORIES
   */
  FE_TempDir = fe_globalPrefs.tmp_dir;
  /* bookmark_file */
  FE_GlobalHist = fe_globalPrefs.history_file;

  /* FONTS
     doc_csid
     font specs
   */
  MSG_SetCitationStyle (context,
			fe_globalPrefs.citation_font, 
			fe_globalPrefs.citation_size,
			fe_globalPrefs.citation_color);

  /* HELPERS
     global_mime_types_file
     private_mime_types_file
     global_mailcap_file
     private_mailcap_file
   */

  /* IDENTITY
     real_name
     email_address
     organization
     signature_file
     #### anonymity_level
   */

  /* IMAGES
   */
  if (context)
    IL_SetPreferences (context,
		       fe_globalPrefs.streaming_images,
		       fe_pref_string_to_dither_mode
                       (fe_globalPrefs.dither_images));

  /* MAIL
   */
  NET_SetMailRelayHost ((char *)
			((fe_globalPrefs.mailhost && *fe_globalPrefs.mailhost)
			 ? fe_globalPrefs.mailhost : "localhost"));

  /* use_movemail_p */
  /* movemail_program */
  MSG_SetPopHost (fe_globalPrefs.pop3_host);
  NET_SetPopUsername (fe_globalPrefs.pop3_user_id);
  NET_LeavePopMailOnServer (fe_globalPrefs.pop3_leave_mail_on_server);
  NET_SetPopMsgSizeLimit (fe_globalPrefs.pop3_msg_size_limit_p ? 
			  fe_globalPrefs.pop3_msg_size_limit : 
			  -fe_globalPrefs.pop3_msg_size_limit);
  MSG_SetBiffInterval(fe_globalPrefs.auto_check_mail ? 
		      fe_globalPrefs.biff_interval * 60 : -1);
  MSG_SetBiffStatFile(fe_globalPrefs.use_movemail_p ? fe_mn_getmailbox() :
		      NULL);
  MSG_SetAutoQuoteReply(fe_globalPrefs.autoquote_reply);
  NET_SetPopPassword (fe_globalPrefs.rememberPswd
		      ? fe_globalPrefs.pop3_password : NULL);

  {
   MWContext* mailctx = XP_FindContextOfType(context, MWContextMail);
   if (mailctx) {
     fe_SetMailNewsSortBehavior(mailctx, 
				fe_globalPrefs.mail_thread_p,
				fe_globalPrefs.mail_sort_style);
   }
  }
  {
   MWContext* newsctx = XP_FindContextOfType(context, MWContextNews);
   if (newsctx) {
     fe_SetMailNewsSortBehavior(newsctx, 
				!fe_globalPrefs.no_news_thread_p,
				fe_globalPrefs.news_sort_style);
   }
  }
     

/*XXX mail_thread_p*/
/*XXX no_news_thread_p*/
/*XXX mail_sort_style */
/*XXX news_sort_style */

  /* NETWORK
   */
  NET_ChangeMaxNumberOfConnections (50);
  NET_ChangeMaxNumberOfConnectionsPerContext (fe_globalPrefs.max_connections);
  NET_ChangeSocketBufferSize (fe_globalPrefs.network_buffer_size * 1024);

  /* NEWS
   */
  FE_UserNewsHost = fe_globalPrefs.newshost; NET_SetNewsHost (FE_UserNewsHost);
  FE_UserNewsRC = fe_globalPrefs.newsrc_directory;
  NET_SetNumberOfNewsArticlesInListing (fe_globalPrefs.news_max_articles);
  NET_SetCacheXOVER (fe_globalPrefs.news_cache_xover);
  MSG_SetAutoShowFirstUnread(fe_globalPrefs.show_first_unread_p);

  /* PROTOCOLS
   */
  NET_SendEmailAddressAsFTPPassword(fe_globalPrefs.email_anonftp);
  NET_WarnOnMailtoPost(fe_globalPrefs.email_submit);
  NET_SetCookiePrefs(fe_globalPrefs.accept_cookie);

  /* LANGUAGES
   */
#ifdef JAVA
  LJ_SetJavaEnabled(!fe_globalPrefs.disable_java);
#endif
#ifdef MOCHA
  LM_SwitchMocha(!fe_globalPrefs.disable_javascript);
#endif

  /* PROXIES
   */
  NET_SetSocksHost (fe_globalPrefs.socks_host);
  NET_SetProxyServer (FTP_PROXY, fe_globalPrefs.ftp_proxy);
  NET_SetProxyServer (HTTP_PROXY, fe_globalPrefs.http_proxy);
#ifdef HAVE_SECURITY
  NET_SetProxyServer (HTTPS_PROXY, fe_globalPrefs.https_proxy);
#endif
  NET_SetProxyServer (GOPHER_PROXY, fe_globalPrefs.gopher_proxy);
  NET_SetProxyServer (WAIS_PROXY, fe_globalPrefs.wais_proxy);
  NET_SetProxyServer (NO_PROXY, fe_globalPrefs.no_proxy);

  if (fe_globalPrefs.proxy_mode == 0)
    NET_SelectProxyStyle(PROXY_STYLE_NONE);
  else if (fe_globalPrefs.proxy_mode == 1)
    NET_SelectProxyStyle(PROXY_STYLE_MANUAL);
  else if (fe_globalPrefs.proxy_mode == 2) {
    NET_SetProxyServer(PROXY_AUTOCONF_URL, fe_globalPrefs.proxy_url);
    NET_SelectProxyStyle(PROXY_STYLE_AUTOMATIC);
  }

#ifdef DUPLICATE_SEC_PREFS
  SECNAV_SetPasswordAskPrefs (
			   fe_globalPrefs.ask_password - 1, /* hack? naaah. */
			   fe_globalPrefs.password_timeout);
  /* SECURITY
     enter_warn
     leave_warn
     mixed_warn
     submit_warn
   */
  SSL_EnableDefault(SSL_ENABLE_SSL2, fe_globalPrefs.ssl2_enable);
  SSL_EnableDefault(SSL_ENABLE_SSL3, fe_globalPrefs.ssl3_enable);

  (void)SECNAV_SetDefUserCertList(fe_globalPrefs.def_user_cert);
#endif /* DUPLICATE_SEC_PREFS */

  /* STYLES 1
     toolbar_icons_p
     toolbar_text_p
     home_document
     underline_links_p
   */
  MSG_SetPlaintextFont (context, fe_globalPrefs.fixed_message_font_p);
  /* #### msg_sort_style */
  /* #### msg_thread_p */

  if (fe_globalPrefs.global_history_expiration <= 0)
    GH_SetGlobalHistoryTimeout (-1);
  else
    GH_SetGlobalHistoryTimeout (fe_globalPrefs.global_history_expiration
				* 24 * 60 * 60);

  /* BOOKMARKS
     put_added_urls_under
     begin_bookmark_menu_under
   */

  /* PRINT
     print_command
     print_reversed
     print_color
     print_landscape
     print_paper_size
   */

/* ========================================================================= */
#ifdef NEW_DECODERS
  if (context &&
      (!CONTEXT_DATA(context)->currentPrefDialog ||
	!CONTEXT_DATA(context)->fep ||
	CONTEXT_DATA(context)->fep->helpers_changed)) {
    XP_Bool reload = FALSE;

    /* Save the current preferences out */
    if (fe_globalData.privateMimetypeFileModifiedTime != 0 &&
	fe_isFileExist(fe_globalPrefs.private_mime_types_file) &&
	fe_isFileChanged(fe_globalPrefs.private_mime_types_file,
			 fe_globalData.privateMimetypeFileModifiedTime, 0)) {
      /* Ask users about overwriting or reloading */
      char *msg = PR_smprintf(XP_GetString(XFE_PRIVATE_MIMETYPE_RELOAD),
			      fe_globalPrefs.private_mime_types_file);
      if (msg) {
	reload = XFE_Confirm (context, msg);
	XP_FREE(msg);
      }
    }
    if (reload)
      NET_CleanupFileFormat(NULL);
    else
      NET_CleanupFileFormat(fe_globalPrefs.private_mime_types_file);

    reload = FALSE;
    if (fe_globalData.privateMailcapFileModifiedTime != 0 &&
	fe_isFileExist(fe_globalPrefs.private_mailcap_file) &&
	fe_isFileChanged(fe_globalPrefs.private_mailcap_file,
			 fe_globalData.privateMailcapFileModifiedTime, NULL)) {
      /* Ask users about overwriting or reloading */
      char *msg = PR_smprintf(XP_GetString(XFE_PRIVATE_MAILCAP_RELOAD),
			      fe_globalPrefs.private_mailcap_file);
      if (msg) {
	reload = XFE_Confirm(context, msg);
	XP_FREE(msg);
      }
    }
    if (reload)
      NET_CleanupMailCapList(NULL);
    else
      NET_CleanupMailCapList(fe_globalPrefs.private_mailcap_file);

    /* Load the preferences file again */
    fe_RegisterConverters ();
  }
#endif /* NEW_DECODERS */
}

void
fe_GeneralPrefsDialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form, tmp;
  XmString str;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  struct fe_prefs_data *fep = NULL;

  fep = (struct fe_prefs_data *) malloc (sizeof (struct fe_prefs_data));
  memset (fep, 0, sizeof (struct fe_prefs_data));

  fep->context = context;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;
  dialog = XmCreatePromptDialog (mainw, "general_prefs", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNokCallback, fe_general_done_cb, fep);
  XtAddCallback (dialog, XmNcancelCallback, fe_prefs_reset_cb, fep);
  XtAddCallback (dialog, XmNapplyCallback,  fe_prefs_cancel_cb, fep);
  XtAddCallback (dialog, XmNdestroyCallback, fe_prefs_destroy_cb, fep);

  form = XtVaCreateManagedWidget("form",
    xmlFolderWidgetClass, dialog,
    XmNshadowThickness, 2,
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
    XmNtabPlacement, XmFOLDER_LEFT,
    XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
    XmNbottomOffset, 3,
    XmNspacing, 1,
    NULL);

#define ADD_TAB(VAR,STRING) \
  str = XmStringCreateLocalized (STRING); \
  tmp = XmLFolderAddTabForm (form, str); \
  XmStringFree (str); \
  fep->VAR = tmp

  ADD_TAB(styles1_selector, XP_GetString( XFE_APPEARANCE ) );
#if 0
  ADD_TAB(bookmarks_selector, XP_GetString( XFE_BOOKMARKS ) );
#endif
#ifdef notyet
  ADD_TAB(colors_selector, XP_GetString( XFE_COLORS ) );
#endif
  ADD_TAB(fonts_selector, XP_GetString( XFE_FONTS ) );
  ADD_TAB(apps_selector, XP_GetString( XFE_APPLICATIONS ) );
  ADD_TAB(helpers_selector, XP_GetString( XFE_HELPERS ) );
  ADD_TAB(images_selector, XP_GetString( XFE_IMAGES ) );

#ifdef REQUEST_LANGUAGE
  ADD_TAB(languages_selector, XP_GetString( XFE_LANGUAGES ) );
#endif

#undef ADD_TAB

  fep->shell = dialog;
  fep->general_form = form;

  CONTEXT_DATA (context)->currentPrefDialog = dialog;
  CONTEXT_DATA (fep->context)->fep = fep;

#if 0
  fe_make_bookmarks_page (context, fep);
#endif
  fe_make_styles1_page (context, fep);
  fe_make_apps_page (context, fep);

  /* Helpers */
  /* Before making the helpers page see if we want to reload the
   * mailcap and mimetype files if they changed on disk.
   */
  {
    XP_Bool needs_reload = FALSE;

    /* Mimetypes file */
    if (fe_globalData.privateMimetypeFileModifiedTime != 0 &&
	fe_isFileExist(fe_globalPrefs.private_mime_types_file) &&
	fe_isFileChanged(fe_globalPrefs.private_mime_types_file,
			 fe_globalData.privateMimetypeFileModifiedTime, NULL)) {
      char *msg = PR_smprintf(XP_GetString(XFE_PRIVATE_RELOADED_MIMETYPE),
			      fe_globalPrefs.private_mime_types_file);
      if (msg) {
	fe_Alert_2(dialog, msg);
	XP_FREE(msg);
      }
      needs_reload = TRUE;
    }

    /* The mailcap File */
    if (fe_globalData.privateMailcapFileModifiedTime != 0 &&
	fe_isFileExist(fe_globalPrefs.private_mailcap_file) &&
	fe_isFileChanged(fe_globalPrefs.private_mailcap_file,
			 fe_globalData.privateMailcapFileModifiedTime, NULL)) {
      char *msg = PR_smprintf(XP_GetString(XFE_PRIVATE_RELOADED_MAILCAP),
			      fe_globalPrefs.private_mailcap_file);
      if (msg) {
	fe_Alert_2(dialog, msg);
	XP_FREE(msg);
      }
      needs_reload = TRUE;
    }

    if (needs_reload) {
      NET_CleanupMailCapList(NULL);
      NET_CleanupFileFormat(NULL);
      fe_RegisterConverters();
    }
  }
  fe_make_helpers_page (context, fep);
#ifdef notyet
  fe_make_colors_page (context, fep);
#endif
  fe_make_fonts_page (context, fep);
  fe_make_images_page (context, fep);

#ifdef REQUEST_LANGUAGE
  fe_make_languages_page (context, fep);
#endif

#define FROB(WHICH) { \
    int save_page = current_general_page; \
    current_general_page = xfe_PREFS_##WHICH; \
    fe_general_reset_1 (fep, &fe_globalPrefs); \
    current_general_page = save_page; \
    }
  FROB(STYLES)
#if 0
  FROB(BOOKMARKS)
#endif
  FROB(HELPERS)
  FROB(APPS)
  FROB(FONTS)
  FROB(IMAGES)

#ifdef REQUEST_LANGUAGE
  FROB(LANGUAGES)
#endif

#undef FROB

  fe_general_verify_page (fep);

  XtManageChild (form);
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  XmLFolderSetActiveTab (form, current_general_page-xfe_GENERAL_OFFSET, True);
}

void
fe_MailNewsPrefsDialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form, tmp;
  XmString str;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  struct fe_prefs_data *fep = NULL;

  fep = (struct fe_prefs_data *) malloc (sizeof (struct fe_prefs_data));
  memset (fep, 0, sizeof (struct fe_prefs_data));

  fep->context = context;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;
  dialog = XmCreatePromptDialog (mainw, "mailnews_prefs", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNokCallback, fe_mailnews_done_cb, fep);
  XtAddCallback (dialog, XmNcancelCallback, fe_prefs_reset_cb, fep);
  XtAddCallback (dialog, XmNapplyCallback,  fe_prefs_cancel_cb, fep);
  XtAddCallback (dialog, XmNdestroyCallback, fe_prefs_destroy_cb, fep);

  form = XtVaCreateManagedWidget("form",
    xmlFolderWidgetClass, dialog,
    XmNshadowThickness, 2,
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
    XmNtabPlacement, XmFOLDER_LEFT,
    XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
    XmNbottomOffset, 3,
    XmNspacing, 1,
    NULL);

#define ADD_TAB(VAR,STRING) \
  str = XmStringCreateLocalized (STRING); \
  tmp = XmLFolderAddTabForm (form, str); \
  XmStringFree (str); \
  fep->VAR = tmp

  ADD_TAB(appearance_selector, XP_GetString( XFE_APPEARANCE ) );
  ADD_TAB(compose_selector, XP_GetString( XFE_COMPOSE_DLG ) );
  ADD_TAB(dirs_selector, XP_GetString( XFE_SERVERS ) );
  ADD_TAB(identity_selector, XP_GetString( XFE_IDENTITY ) );
  ADD_TAB(organization_selector,XP_GetString( XFE_ORGANIZATION ) );
#undef ADD_TAB

  fep->shell = dialog;
  fep->mailnews_form = form;

  CONTEXT_DATA (context)->currentPrefDialog = dialog;
  CONTEXT_DATA (fep->context)->fep = fep;

  fe_make_appearance_page (context, fep);
  fe_make_compose_page (context, fep);
  fe_make_servers_page (context, fep);
  fe_make_identity_page (context, fep);
  fe_make_organization_page (context, fep);
#define FROB(WHICH) { \
    int save_page = current_mailnews_page; \
    current_mailnews_page = xfe_PREFS_##WHICH; \
    fe_mailnews_reset_1 (fep, &fe_globalPrefs); \
    current_mailnews_page = save_page; \
    }
  FROB(APPEARANCE)
  FROB(COMPOSITION)
  FROB(SERVERS)
  FROB(IDENTITY)
  FROB(ORGANIZATION)
#undef FROB

  fe_mailnews_verify_page (fep);

  XtManageChild (form);
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  XmLFolderSetActiveTab (form, current_mailnews_page-xfe_MAILNEWS_OFFSET, True);
}

void
fe_NetworkPrefsDialog (MWContext *context)
{
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form, tmp;
  XmString str;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  struct fe_prefs_data *fep = NULL;

  fep = (struct fe_prefs_data *) malloc (sizeof (struct fe_prefs_data));
  memset (fep, 0, sizeof (struct fe_prefs_data));

  fep->context = context;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;
  dialog = XmCreatePromptDialog (mainw, "network_prefs", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNokCallback, fe_network_done_cb, fep);
  XtAddCallback (dialog, XmNcancelCallback, fe_prefs_reset_cb, fep);
  XtAddCallback (dialog, XmNapplyCallback,  fe_prefs_cancel_cb, fep);
  XtAddCallback (dialog, XmNdestroyCallback, fe_prefs_destroy_cb, fep);

  form = XtVaCreateManagedWidget("form",
    xmlFolderWidgetClass, dialog,
    XmNshadowThickness, 2,
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
    XmNtabPlacement, XmFOLDER_LEFT,
    XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
    XmNbottomOffset, 3,
    XmNspacing, 1,
    NULL);

#define ADD_TAB(VAR,STRING) \
  str = XmStringCreateLocalized (STRING); \
  tmp = XmLFolderAddTabForm (form, str); \
  XmStringFree (str); \
  fep->VAR = tmp

  ADD_TAB(cache_selector,   XP_GetString( XFE_CACHE ) );
  ADD_TAB(network_selector, XP_GetString( XFE_CONNECTIONS ) );
  ADD_TAB(proxies_selector, XP_GetString( XFE_PROXIES ) );
  ADD_TAB(protocols_selector, XP_GetString( XFE_PROTOCOLS ) );
  ADD_TAB(lang_selector, XP_GetString( XFE_LANG ) );
#undef ADD_TAB

  fep->shell = dialog;
  fep->network_form = form;

  CONTEXT_DATA (context)->currentPrefDialog = dialog;
  CONTEXT_DATA (fep->context)->fep = fep;

  fe_make_cache_page (context, fep);
  fe_make_network_page (context, fep);
  fe_make_proxies_page (context, fep);
  fe_make_protocols_page (context, fep);
  fe_make_lang_page (context, fep);

#define FROB(WHICH) { \
    int save_page = current_network_page; \
    current_network_page = xfe_PREFS_##WHICH; \
    fe_network_reset_1 (fep, &fe_globalPrefs); \
    current_network_page = save_page; \
    }
  FROB(CACHE)
  FROB(NETWORK)
  FROB(PROXIES)
  FROB(PROTOCOLS)
  FROB(LANG)
#undef FROB

  fe_network_verify_page (fep);

  XtManageChild (form);
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  XmLFolderSetActiveTab (form, current_network_page-xfe_NETWORK_OFFSET, True);
}

void
fe_SecurityPrefsDialog (MWContext *context)
{
#ifdef DUPLICATE_SEC_PREFS
  Widget mainw = CONTEXT_WIDGET (context);
  Widget dialog, form, tmp;
  XmString str;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  struct fe_prefs_data *fep = NULL;

  fep = (struct fe_prefs_data *) malloc (sizeof (struct fe_prefs_data));
  memset (fep, 0, sizeof (struct fe_prefs_data));

  fep->context = context;

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNnoResize, True); ac++;
  dialog = XmCreatePromptDialog (mainw, "security_prefs", av, ac);

  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog,
                                                XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

#ifdef DUPLICATE_SEC_PREFS
  XtAddCallback (dialog, XmNokCallback, fe_security_done_cb, fep);
#endif /* DUPLICATE_SEC_PREFS */
  XtAddCallback (dialog, XmNcancelCallback, fe_prefs_reset_cb, fep);
  XtAddCallback (dialog, XmNapplyCallback,  fe_prefs_cancel_cb, fep);
  XtAddCallback (dialog, XmNdestroyCallback, fe_prefs_destroy_cb, fep);

  form = XtVaCreateManagedWidget("form",
    xmlFolderWidgetClass, dialog,
    XmNshadowThickness, 2,
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
#ifdef ALLOW_TAB_ROTATE
    XmNtabPlacement, XmFOLDER_LEFT,
    XmNrotateWhenLeftRight, FALSE,
#endif /* ALLOW_TAB_ROTATE */
    XmNbottomOffset, 3,
    XmNspacing, 1,
    NULL);

#define ADD_TAB(VAR,STRING) \
  str = XmStringCreateLocalized (STRING); \
  tmp = XmLFolderAddTabForm (form, str); \
  XmStringFree (str); \
  fep->VAR = tmp

  ADD_TAB(sec_general_selector, XP_GetString( XFE_GENERAL ) );
  ADD_TAB(sec_passwords_selector, XP_GetString( XFE_PASSWORDS));
  ADD_TAB(personal_selector, XP_GetString( XFE_PERSONAL_CERTIFICATES));
  ADD_TAB(site_selector, XP_GetString( XFE_SITE_CERTIFICATES));
#undef ADD_TAB

  fep->shell = dialog;
  fep->security_form = form;

  CONTEXT_DATA (context)->currentPrefDialog = dialog;
  CONTEXT_DATA (fep->context)->fep = fep;

  fe_make_sec_general_page (context, fep);
  fe_make_sec_passwords_page (context, fep);
  fe_make_personal_page (context, fep);
  fe_make_site_cert_page (context, fep);

#define FROB(WHICH) { \
    int save_page = current_security_page; \
    current_security_page = xfe_PREFS_##WHICH; \
    fe_security_reset_1 (fep, &fe_globalPrefs); \
    current_security_page = save_page; \
    }
  FROB(SEC_GENERAL)
  FROB(SEC_PASSWORDS)
  FROB(SEC_PERSONAL)
  FROB(SEC_SITE)
#undef FROB

  fe_security_verify_page (fep);

  XtManageChild (form);
  fe_NukeBackingStore (dialog);
  XtManageChild (dialog);

  XmLFolderSetActiveTab (form, current_security_page-xfe_SECURITY_OFFSET, True);
#else  /* !DUPLICATE_SEC_PREFS */
  fe_GetURL(context,
	    NET_CreateURLStruct("about:security?advisor", NET_DONT_RELOAD),
	    FALSE);
#endif /* !DUPLICATE_SEC_PREFS */
}



void
fe_VerifyDiskCache (MWContext *context)
{
#ifdef PREFS_DISK_CACHE
  struct stat st;
  char file [1024];
  char message [4000];
  static Boolean done = False;
  if (done) return;
  done = True;

  if (fe_globalPrefs.disk_cache_size <= 0)
    return;

  strcpy (file, fe_globalPrefs.cache_dir);
  if (file [strlen (file) - 1] == '/')
    file [strlen (file) - 1] = 0;

  if (stat (file, &st))
    {
      /* Doesn't exist - try to create it. */
      if (mkdir (file, 0700))
	{
	  /* Failed. */
#ifdef DEBUG_jwz  /* this is the modern way */
          char *error = 0;
          if (errno >= 0)
            error = strerror (errno);
          if (!error || !*error)
            error = XP_GetString( XFE_UNKNOWN_ERROR );
#else /* !DEBUG_jwz */
	  char *error = ((errno >= 0 && errno < sys_nerr)
			 ? sys_errlist [errno]
			 : XP_GetString( XFE_UNKNOWN_ERROR));
#endif /* !DEBUG_jwz */
	  PR_snprintf (message, sizeof (message),
		    fe_globalData.create_cache_dir_message,
		    file, error);
	}
      else
	{
	  /* Suceeded. */
	  PR_snprintf (message, sizeof (message),
			fe_globalData.created_cache_dir_message, file);
	}
    }
  else if (! (st.st_mode & S_IFDIR))
    {
      /* Exists but isn't a directory. */
      PR_snprintf (message, sizeof (message),
		    fe_globalData.cache_not_dir_message, file);
    }
  else
    {
      /* The disk cache is ok! */
      *message = 0;
    }

  if (*message)
    {
      PR_snprintf(message + strlen (message), sizeof (message),
	      fe_globalData.cache_suffix_message,
	      fe_globalPrefs.disk_cache_size, fe_progclass);
      FE_Alert (context, message);
    }
#endif /* PREFS_DISK_CACHE */
}

/* -*- Mode: C; tab-width: 8 -*-
   prefs.c --- reading and writing X preferences settings
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 */

/* for XP_GetString() */
#include <xpgetstr.h>

#include "mozilla.h"
#include "xfe.h"
#include "fonts.h"
#include "libi18n.h"

#ifdef EDITOR
#include "xeditor.h"
#endif /*EDITOR*/

#ifdef Bool
#undef Bool
#endif

extern int XFE_WELCOME_HTML;
extern int XFE_TILDE_USER_SYNTAX_DISALLOWED;


/* If this is defined, we write ~/ to the preferences file as an
   abreviation of $HOME.  If not defined, we still parse ~/ from
   the file, but the next time the preferences are saved, full
   paths will be written.

   Writing out ~/ all the time makes Netscape 1.0 and 1.1 unable
   to read these preferences files.  Hmm...
 */
#define USE_TWIDDLE

static void
prefs_write_tag (FILE *fp, char *tag)
{
  fputs (tag, fp);
  if (strlen (tag) < 7)
    fputs (":\t\t\t", fp);
  else if (strlen (tag) < 15)
    fputs (":\t\t", fp);
  else
    fputs (":\t", fp);
}

static void
prefs_write_string (FILE *fp, char *tag, char *value, XP_Bool multi_line_p)
{
  prefs_write_tag (fp, tag);
  while (value && *value)
    {
      if (*value == '\n')
	{
	  if (multi_line_p && value[1])
	    fputs ("\n\t\t\t", fp);
          else if (multi_line_p)
	    fputs ("\n", fp);
	  else
	    fputc (' ', fp);
	}
      else
	{
	  fputc (*value, fp);
	}
      value++;
    }
  fputc ('\n', fp);
}


static void
prefs_write_path (FILE *fp, char *tag, char *value)
{
  char *suffix = 0;
#ifdef USE_TWIDDLE
  const char *home = getenv ("HOME");
  if (home && value && !strncmp (home, value, strlen (home)))
    {
      suffix = value + strlen (home);
      if (home[strlen(home)-1] == '/')
	;
      else if (suffix[0] == '/')
	suffix++;
      else if (suffix[0] != 0)
	suffix = 0;
    }
#endif /* USE_TWIDDLE */
  prefs_write_tag (fp, tag);
  if (suffix)
    fputs ("~/", fp);
  fputs (suffix ? suffix : (value ? value : ""), fp);
  fputs ("\n", fp);
}


static void
prefs_write_int (FILE *fp, char *tag, int value)
{
  prefs_write_tag (fp, tag);
  fprintf (fp, "%d\n", value);
}


static void
prefs_write_bool (FILE *fp, char *tag, XP_Bool value)
{
  prefs_write_tag (fp, tag);
  fputs (value ? "True\n" : "False\n", fp);
}

#if 0
static void
prefs_write_color(FILE* fp, char* tag, LO_Color* value)
{
  prefs_write_tag (fp, tag);
  fprintf(fp, "#%02x%02x%02x\n", value->red, value->green, value->blue);
}

static Boolean
prefs_parse_color(LO_Color* color, char* string)
{
    if (!LO_ParseRGB(string, &color->red, &color->green, &color->blue)) {
	color->red = color->blue = color->green = 0; /* black */
	return FALSE;
    }
    return TRUE;
}
#endif /* 0 */


#ifdef DEBUG_jwz
extern XP_Bool IL_AnimationLoopsEnabled;
extern XP_Bool IL_AnimationsEnabled;
#endif /* DEBUG_jwz */



/* saves out the global preferences.
 * 
 * returns True on success and FALSE
 * on failure (unable to open prefs file)
 */
PUBLIC Bool
XFE_SavePrefs (char * filename, XFE_GlobalPrefs *prefs)
{

  FILE * fp = fopen(filename, "w");
  fe_FontSettings *next;
  fe_FontSettings *set;

#ifdef DEBUG_jwz
  prefs->anim_loop_p = IL_AnimationLoopsEnabled;
  prefs->anim_p = IL_AnimationsEnabled;
#endif /* DEBUG_jwz */

  if(!fp)
    return(FALSE);

  fprintf (fp,
	   "# Netscape Preferences File\n"
	   "# Version: %s\n"
	   "# This is a generated file!  Do not edit.\n"
	   "\n",
	   fe_version);

  /* OPTIONS MENU
   */
  prefs_write_bool (fp, "SHOW_TOOLBAR", prefs->show_toolbar_p);
  prefs_write_bool (fp, "SHOW_URL", prefs->show_url_p);
  prefs_write_bool (fp, "SHOW_DIRECTORY_BUTTONS",
		    prefs->show_directory_buttons_p);
  prefs_write_bool (fp, "SHOW_MENUBAR", prefs->show_menubar_p);
  prefs_write_bool (fp, "SHOW_BOTTOM_STATUS_BAR", prefs->show_bottom_status_bar_p);
  prefs_write_bool (fp, "AUTOLOAD_IMAGES", prefs->autoload_images_p);
  prefs_write_bool (fp, "FTP_FILE_INFO", prefs->fancy_ftp_p);
#ifdef HAVE_SECURITY
  prefs_write_bool (fp, "SHOW_SECURITY_BAR", prefs->show_security_bar_p);
#endif
  fputs ("\n", fp);

  /* APPLICATIONS
   */
  prefs_write_string (fp, "TN3270", prefs->tn3270_command, FALSE);
  prefs_write_string (fp, "TELNET", prefs->telnet_command, FALSE);
  prefs_write_string (fp, "RLOGIN", prefs->rlogin_command, FALSE);
  prefs_write_string (fp, "RLOGIN_USER", prefs->rlogin_user_command, FALSE);
#ifdef DEBUG_jwz
  prefs_write_string (fp, "OTHER_BROWSER", prefs->other_browser, FALSE);
#endif /* DEBUG_jwz */
  fputs ("\n", fp);

  /* CACHE
   */
  prefs_write_int    (fp, "MEMORY_CACHE_SIZE", prefs->memory_cache_size);
  prefs_write_int    (fp, "DISK_CACHE_SIZE", prefs->disk_cache_size);
  prefs_write_path   (fp, "CACHE_DIR", prefs->cache_dir);
  prefs_write_int    (fp, "VERIFY_DOCUMENTS", prefs->verify_documents);
  prefs_write_bool    (fp, "CACHE_SSL_PAGES", prefs->cache_ssl_p);
  fputs ("\n", fp);

  /* COLORS
   */

  /* COMPOSITION
   */
  prefs_write_bool   (fp, "8BIT_MAIL_AND_NEWS", !prefs->qp_p);
  prefs_write_bool   (fp, "SEND_FORMATTED_TEXT", !prefs->send_formatted_text_p);
  prefs_write_bool   (fp, "QUEUE_FOR_LATER", prefs->queue_for_later_p);
  prefs_write_bool   (fp, "MAIL_BCC_SELF", prefs->mailbccself_p);
  prefs_write_bool   (fp, "NEWS_BCC_SELF", prefs->newsbccself_p);
  prefs_write_string (fp, "MAIL_BCC", prefs->mail_bcc, FALSE);
  prefs_write_string (fp, "NEWS_BCC", prefs->news_bcc, FALSE);
  prefs_write_path   (fp, "MAIL_FCC", prefs->mail_fcc);
  prefs_write_path   (fp, "NEWS_FCC", prefs->news_fcc);
  prefs_write_bool (fp, "MAIL_AUTOQUOTE_REPLY", prefs->autoquote_reply);
  fputs ("\n", fp);

  /* DIRECTORIES
   */
  prefs_write_path (fp, "TMPDIR", prefs->tmp_dir);
  prefs_write_path (fp, "BOOKMARKS_FILE", prefs->bookmark_file);
  prefs_write_path (fp, "HISTORY_FILE",   prefs->history_file);
  fputs ("\n", fp);

  /* FONTS
   */
  prefs_write_int (fp, "DOC_CSID", prefs->doc_csid);
  prefs_write_string (fp, "FONT_CHARSET", fe_GetFontCharSetSetting(), FALSE);
  set = fe_GetFontSettings();
  next = set;
  while (next)
  {
	prefs_write_string (fp, "FONT_SPEC", next->spec, FALSE);
	next = next->next;
  }
  fe_FreeFontSettings(set);
  prefs_write_int (fp, "CITATION_FONT", (int) prefs->citation_font);
  prefs_write_int (fp, "CITATION_SIZE", (int) prefs->citation_size);
  prefs_write_string (fp, "CITATION_COLOR", prefs->citation_color, FALSE);
  fputs ("\n", fp);

  /* HELPERS
   */
  prefs_write_path (fp, "MIME_TYPES", prefs->global_mime_types_file);
  prefs_write_path (fp, "PERSONAL_MIME_TYPES", prefs->private_mime_types_file);
  prefs_write_path (fp, "MAILCAP", prefs->global_mailcap_file);
  prefs_write_path (fp, "PERSONAL_MAILCAP", prefs->private_mailcap_file);
  fputs ("\n", fp);

  /* IDENTITY
   */
  prefs_write_string (fp, "REAL_NAME", prefs->real_name, FALSE);
  prefs_write_string (fp, "EMAIL_ADDRESS", prefs->email_address,FALSE);

#ifdef DEBUG_jwz
  prefs_write_string (fp, "EMAIL_ADDRESSES", prefs->email_addresses, TRUE);
  fputs ("\n", fp);
#endif /* DEBUG_jwz */

  prefs_write_string (fp, "ORGANIZATION", prefs->organization, TRUE);
  prefs_write_path   (fp, "SIGNATURE_FILE", prefs->signature_file);
  prefs_write_int    (fp, "SIGNATURE_DATE", (int) prefs->signature_date);
  prefs_write_int    (fp, "ANONYMITY_LEVEL", prefs->anonymity_level);
  fputs ("\n", fp);

  /* IMAGES
   */
  prefs_write_string (fp, "DITHER_IMAGES", prefs->dither_images, FALSE);
  prefs_write_bool   (fp, "STREAMING_IMAGES", prefs->streaming_images);
#ifdef DEBUG_jwz
  prefs_write_bool   (fp, "ANIMATIONS", prefs->anim_p);
  prefs_write_bool   (fp, "ANIMATION_LOOPS", prefs->anim_loop_p);
#endif /* DEBUG_jwz */
  fputs ("\n", fp);

  /* MAIL
   */
  prefs_write_string (fp, "MAILHOST", prefs->mailhost, FALSE);
  prefs_write_path   (fp, "MAIL_DIR", prefs->mail_directory);
  prefs_write_int    (fp, "BIFF_INTERVAL", prefs->biff_interval);
  prefs_write_bool   (fp, "AUTO_CHECK_MAIL", prefs->auto_check_mail);
  prefs_write_bool   (fp, "USE_MOVEMAIL", prefs->use_movemail_p);
  prefs_write_bool   (fp, "BUILTIN_MOVEMAIL", prefs->builtin_movemail_p);
  prefs_write_path   (fp, "MOVEMAIL_PROGRAM", prefs->movemail_program);
  prefs_write_string (fp, "POP3_HOST", prefs->pop3_host, FALSE);
  prefs_write_string (fp, "POP3_USER_ID", prefs->pop3_user_id, FALSE);
  prefs_write_bool   (fp, "POP3_LEAVE_ON_SERVER",
		      prefs->pop3_leave_mail_on_server);
  prefs_write_int    (fp, "POP3_MSG_SIZE_LIMIT",
		      prefs->pop3_msg_size_limit_p ? prefs->pop3_msg_size_limit :
		      -prefs->pop3_msg_size_limit);
  prefs_write_string (fp, "MAIL_FOLDER_COLUMNS", prefs->mail_folder_columns,
		      FALSE);
  prefs_write_string (fp, "MAIL_MESSAGE_COLUMNS", prefs->mail_message_columns,
		      FALSE);
  prefs_write_string (fp, "MAIL_SASH_GEOMETRY", prefs->mail_sash_geometry,
		      FALSE);
  prefs_write_bool   (fp, "AUTO_EMPTY_TRASH", prefs->emptyTrash);
  prefs_write_bool   (fp, "REMEMBER_MAIL_PASSWORD", prefs->rememberPswd);
  prefs_write_string (fp, "MAIL_PASSWORD",
		      prefs->rememberPswd ? prefs->pop3_password : "",
		      FALSE);
  prefs_write_bool   (fp, "THREAD_MAIL", prefs->mail_thread_p);
  prefs_write_int    (fp, "MAIL_PANE_STYLE", prefs->mail_pane_style);
  prefs_write_int    (fp, "MAIL_SORT_STYLE", prefs->mail_sort_style);
  prefs_write_bool   (fp, "MOVEMAIL_WARN", prefs->movemail_warn);

  fputs ("\n", fp);

  /* NETWORK
   */
  prefs_write_int    (fp, "MAX_CONNECTIONS", prefs->max_connections);
  prefs_write_int    (fp, "SOCKET_BUFFER_SIZE", prefs->network_buffer_size);
  fputs ("\n", fp);

  /* PROTOCOLS
   */
  prefs_write_bool   (fp, "USE_EMAIL_FOR_ANON_FTP", prefs->email_anonftp);
  prefs_write_bool   (fp, "ALERT_EMAIL_FORM_SUBMIT", prefs->email_submit);
  prefs_write_int    (fp, "ACCEPT_COOKIE", prefs->accept_cookie);
  fputs ("\n", fp);

  /* NEWS
   */
  prefs_write_string (fp, "NNTPSERVER", prefs->newshost, FALSE);
  prefs_write_path   (fp, "NEWSRC_DIR", prefs->newsrc_directory);
  prefs_write_int    (fp, "NEWS_MAX_ARTICLES", prefs->news_max_articles);
  prefs_write_bool   (fp, "NEWS_CACHE_HEADERS", prefs->news_cache_xover);
  prefs_write_bool   (fp, "SHOW_FIRST_UNREAD", prefs->show_first_unread_p);
  prefs_write_string (fp, "NEWS_FOLDER_COLUMNS", prefs->news_folder_columns,
		      FALSE);
  prefs_write_string (fp, "NEWS_MESSAGE_COLUMNS", prefs->news_message_columns,
		      FALSE);
  prefs_write_string (fp, "NEWS_SASH_GEOMETRY", prefs->news_sash_geometry,
		      FALSE);
  prefs_write_bool   (fp, "THREAD_NEWS", !prefs->no_news_thread_p);
  prefs_write_int    (fp, "NEWS_PANE_STYLE", prefs->news_pane_style);
  prefs_write_int    (fp, "NEWS_SORT_STYLE", prefs->news_sort_style);
  fputs ("\n", fp);

  /* PROXIES
   */
  prefs_write_int    (fp, "PROXY_MODE", prefs->proxy_mode);
  prefs_write_string (fp, "PROXY_URL", prefs->proxy_url, FALSE);

  prefs_write_string (fp, "SOCKS_HOST", prefs->socks_host, FALSE);

  prefs_write_string (fp, "FTP_PROXY", prefs->ftp_proxy, FALSE);
  prefs_write_string (fp, "HTTP_PROXY", prefs->http_proxy, FALSE);
#ifdef HAVE_SECURITY
  prefs_write_string (fp, "HTTPS_PROXY", prefs->https_proxy, FALSE);
#endif
  prefs_write_string (fp, "GOPHER_PROXY", prefs->gopher_proxy, FALSE);
  prefs_write_string (fp, "WAIS_PROXY", prefs->wais_proxy, FALSE);
  prefs_write_string (fp, "NO_PROXY", prefs->no_proxy, FALSE);
  fputs ("\n", fp);

  /* SECURITY
   */
  prefs_write_string (fp, "LICENSE_ACCEPTED", prefs->license_accepted, FALSE);

#ifdef HAVE_SECURITY
  prefs_write_int  (fp, "ASK_PASSWORD", prefs->ask_password);
  prefs_write_int  (fp, "PASSWORD_TIMEOUT", prefs->password_timeout);

  prefs_write_bool (fp, "WARN_ENTER_SECURE", prefs->enter_warn);
  prefs_write_bool (fp, "WARN_LEAVE_SECURE", prefs->leave_warn);
  prefs_write_bool (fp, "WARN_MIXED_SECURE", prefs->mixed_warn);
  prefs_write_bool (fp, "WARN_SUBMIT_INSECURE", prefs->submit_warn);
#endif /* HAVE_SECURITY -- jwz */

#ifdef JAVA
  prefs_write_bool (fp, "DISABLE_JAVA", prefs->disable_java);
#else /* jwz */
  prefs_write_bool (fp, "DISABLE_JAVA", TRUE);
#endif

#ifdef MOCHA
  prefs_write_bool (fp, "DISABLE_JAVASCRIPT", prefs->disable_javascript);
#else /* jwz */
  prefs_write_bool (fp, "DISABLE_JAVASCRIPT", TRUE);
#endif /* MOCHA */

#ifdef HAVE_SECURITY /* jwz */
  prefs_write_string (fp, "DEFAULT_USER_CERT",
		      (!strcasecomp(prefs->def_user_cert, prefs->email_address)
		       ? "do the default"
		       : prefs->def_user_cert),
		      FALSE);
  prefs_write_string (fp, "DEFAULT_MAIL_CERT",
		      (!strcasecomp(prefs->def_mail_cert, prefs->email_address)
		       ? "do the default"
		       : prefs->def_mail_cert),
		      FALSE);
  prefs_write_bool (fp, "ENABLE_SSL2", prefs->ssl2_enable);
  prefs_write_bool (fp, "ENABLE_SSL3", prefs->ssl3_enable);
  prefs_write_string (fp, "CIPHER", prefs->cipher, FALSE);

  /* New 4.0 preferences for 3.01... */
  fputs ("\n", fp);
  prefs_write_bool (fp, "CRYPTO_SIGN_OUTGOING_MAIL",
		    prefs->crypto_sign_outgoing_mail);
  prefs_write_bool (fp, "CRYPTO_SIGN_OUTGOING_NEWS",
		    prefs->crypto_sign_outgoing_news);
  prefs_write_bool (fp, "ENCRYPT_OUTGOING_MAIL",
		    prefs->encrypt_outgoing_mail);
  prefs_write_bool (fp, "WARN_FORWARD_ENCRYPTED",
		    prefs->warn_forward_encrypted);
  prefs_write_bool (fp, "WARN_REPLY_UNENCRYPTED",
		    prefs->warn_reply_unencrypted);

  fputs ("\n", fp);
  prefs_write_int (fp, "SECURITY_KEYGEN_MAXBITS",
		   prefs->security_keygen_maxbits);

  prefs_write_bool (fp, "SECURITY_SSL2_DES_64",
		    prefs->security_ssl2_des_64);
  prefs_write_bool (fp, "SECURITY_SSL2_DES_EDE3_192",
		    prefs->security_ssl2_des_ede3_192);
  prefs_write_bool (fp, "SECURITY_SSL2_RC2_40",
		    prefs->security_ssl2_rc2_40);
  prefs_write_bool (fp, "SECURITY_SSL2_RC2_128",
		    prefs->security_ssl2_rc2_128);
  prefs_write_bool (fp, "SECURITY_SSL2_RC4_40",
		    prefs->security_ssl2_rc4_40);
  prefs_write_bool (fp, "SECURITY_SSL2_RC4_128",
		    prefs->security_ssl2_rc4_128);

  prefs_write_bool (fp, "SECURITY_SSL3_FORTEZZA_FORTEZZA_SHA",
		    prefs->security_ssl3_fortezza_fortezza_sha);
  prefs_write_bool (fp, "SECURITY_SSL3_FORTEZZA_NULL_SHA",
		    prefs->security_ssl3_fortezza_null_sha);
  prefs_write_bool (fp, "SECURITY_SSL3_FORTEZZA_RC4_SHA",
		    prefs->security_ssl3_fortezza_rc4_sha);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_DES_EDE3_SHA",
		    prefs->security_ssl3_rsa_des_ede3_sha);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_DES_SHA",
		    prefs->security_ssl3_rsa_des_sha);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_NULL_MD5",
		    prefs->security_ssl3_rsa_null_md5);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_RC2_40_MD5",
		    prefs->security_ssl3_rsa_rc2_40_md5);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_RC4_40_MD5",
		    prefs->security_ssl3_rsa_rc4_40_md5);
  prefs_write_bool (fp, "SECURITY_SSL3_RSA_RC4_128_MD5",
		    prefs->security_ssl3_rsa_rc4_128_md5);

  prefs_write_bool (fp, "SECURITY_SMIME_DES",
		    prefs->security_smime_des);
  prefs_write_bool (fp, "SECURITY_SMIME_DES_EDE3",
		    prefs->security_smime_des_ede3);
  prefs_write_bool (fp, "SECURITY_SMIME_RC2_40",
		    prefs->security_smime_rc2_40);
  prefs_write_bool (fp, "SECURITY_SMIME_RC2_64",
		    prefs->security_smime_rc2_64);
  prefs_write_bool (fp, "SECURITY_SMIME_RC2_128",
		    prefs->security_smime_rc2_128);
  prefs_write_bool (fp, "SECURITY_SMIME_RC5_B64R16K40",
		    prefs->security_smime_rc5_b64r16k40);
  prefs_write_bool (fp, "SECURITY_SMIME_RC5_B64R16K64",
		    prefs->security_smime_rc5_b64r16k64);
  prefs_write_bool (fp, "SECURITY_SMIME_RC5_B64R16K128",
		    prefs->security_smime_rc5_b64r16k128);

  fputs ("\n", fp);
#endif /* HAVE_SECURITY */

  /* STYLES (GENERAL APPEARANCE)
   */
  prefs_write_bool (fp, "TOOLBAR_ICONS", prefs->toolbar_icons_p);
  prefs_write_bool (fp, "TOOLBAR_TEXT", prefs->toolbar_text_p);
  prefs_write_bool (fp, "TOOLBAR_TIPS", prefs->toolbar_tips_p);
  prefs_write_string (fp, "HOME_DOCUMENT", prefs->home_document,FALSE);

  prefs_write_int  (fp, "STARTUP_MODE", prefs->startup_mode);

  prefs_write_bool (fp, "UNDERLINE_LINKS", prefs->underline_links_p);
  prefs_write_int (fp, "HISTORY_EXPIRATION",
		   prefs->global_history_expiration);
  prefs_write_bool (fp, "FIXED_MESSAGE_FONT", prefs->fixed_message_font_p);
/*** no longer valid. we need one each for mail and news - dp
  prefs_write_int  (fp, "SORT_MESSAGES", prefs->msg_sort_style);
  prefs_write_bool (fp, "THREAD_MESSAGES", prefs->msg_thread_p);
***/
  fputs ("\n", fp);

  /* BOOKMARK
   */
  prefs_write_string (fp, "ADD_URLS", prefs->put_added_urls_under, FALSE);
  prefs_write_string (fp, "BOOKMARK_MENU", prefs->begin_bookmark_menu_under,
		      FALSE);
  fputs ("\n", fp);

  /* PRINT
   */
  prefs_write_string (fp, "PRINT_COMMAND", prefs->print_command, FALSE);
  prefs_write_bool   (fp, "PRINT_REVERSED", prefs->print_reversed);
  prefs_write_bool   (fp, "PRINT_COLOR", prefs->print_color);
  prefs_write_bool   (fp, "PRINT_LANDSCAPE", prefs->print_landscape);
  prefs_write_int    (fp, "PRINT_PAPER", prefs->print_paper_size);
#ifdef DEBUG_jwz
  prefs_write_int    (fp, "PRINT_SIZE", prefs->print_font_size);
  prefs_write_bool   (fp, "PRINT_HEADER_P", prefs->print_header_p);
  prefs_write_bool   (fp, "PRINT_FOOTER_P", prefs->print_footer_p);
  prefs_write_string (fp, "PRINT_HEADER", prefs->print_header, FALSE);
  prefs_write_string (fp, "PRINT_FOOTER", prefs->print_footer, FALSE);
  prefs_write_int    (fp, "PRINT_HMARGIN", prefs->print_hmargin);
  prefs_write_int    (fp, "PRINT_VMARGIN", prefs->print_vmargin);
#endif /* DEBUG_jwz */

#ifdef EDITOR
  fputs ("\n", fp);
  prefs_write_bool(fp, "EDITOR_CHARACTER_TOOLBAR",
		   prefs->editor_character_toolbar);
  prefs_write_bool(fp, "EDITOR_PARAGRAPH_TOOLBAR",
		   prefs->editor_paragraph_toolbar);
  prefs_write_string(fp, "EDITOR_AUTHOR_NAME",
		     prefs->editor_author_name, FALSE);
  prefs_write_string(fp, "EDITOR_HTML_EDITOR",
		     prefs->editor_html_editor, FALSE);
  prefs_write_string(fp, "EDITOR_IMAGE_EDITOR",
		     prefs->editor_image_editor, FALSE);
  prefs_write_string(fp, "EDITOR_DOCUMENT_TEMPLATE",
		     prefs->editor_document_template, FALSE);
  prefs_write_int(fp, "EDITOR_AUTOSAVE_PERIOD", prefs->editor_autosave_period);
  prefs_write_bool(fp, "EDITOR_CUSTOM_COLORS",
		   prefs->editor_custom_colors);
  prefs_write_color(fp, "EDITOR_BACKGROUND_COLOR",
		    &prefs->editor_background_color);
  prefs_write_color(fp, "EDITOR_NORMAL_COLOR",
		    &prefs->editor_normal_color);
  prefs_write_color(fp, "EDITOR_LINK_COLOR",
		    &prefs->editor_link_color);
  prefs_write_color(fp, "EDITOR_ACTIVE_COLOR",
		    &prefs->editor_active_color);
  prefs_write_color(fp, "EDITOR_FOLLOWED_COLOR",
		    &prefs->editor_followed_color);
  prefs_write_string(fp, "EDITOR_BACKGROUND_IMAGE",
		     prefs->editor_background_image, FALSE);
  prefs_write_bool(fp, "EDITOR_MAINTAIN_LINKS",
		   prefs->editor_maintain_links);
  prefs_write_bool(fp, "EDITOR_KEEP_IMAGES",
		   prefs->editor_keep_images);
  prefs_write_string(fp, "EDITOR_PUBLISH_LOCATION",
		     prefs->editor_publish_location, FALSE);
  prefs_write_string(fp, "EDITOR_BROWSE_LOCATION",
		     prefs->editor_browse_location, FALSE);
  prefs_write_string(fp, "EDITOR_PUBLISH_USERNAME",
		     prefs->editor_publish_username, FALSE);
  if (prefs->editor_save_publish_password) {
      prefs_write_string(fp, "EDITOR_PUBLISH_PASSWORD",
			 prefs->editor_publish_password, FALSE);
  }
  prefs_write_bool(fp, "EDITOR_COPYRIGHT_HINT",
		   prefs->editor_copyright_hint);
#endif

  fclose (fp);

  return (TRUE);
}


static void xfe_prefs_from_environment (XFE_GlobalPrefs *prefs);


/* reads in the global preferences.
 * 
 * returns True on success and FALSE
 * on failure (unable to open prefs file)
 *
 * the prefs structure must be zero'd at creation since
 * this function will free any existing char pointers 
 * passed in and will malloc new ones.
 */
PUBLIC Bool
XFE_ReadPrefs(char * filename, XFE_GlobalPrefs *prefs)
{
  FILE * fp;
  char buffer [1024];
  char *name, *value, *colon;

  /* First set all slots to the builtin defaults.
     Then, let the environment override that.
     Then, let the file override the environment.

     This means that, generally, the environment variables are only
     consulted when there is no preferences file on disk - once the
     preferences get saved, the prevailing env. var values will have
     been captured in it.
   */

  XFE_DefaultPrefs (xfe_PREFS_ALL, prefs);
  xfe_prefs_from_environment (prefs);

  fp = fopen (filename, "r");

  if (!fp)
    return(FALSE);

  if (! fgets(buffer, 1024, fp))
    *buffer = 0;

  while (fgets (buffer, 1024, fp))
    {
      name = XP_StripLine (buffer);

      if (*name == '#')
	continue;  /* comment */

      colon = strchr (name, ':');
		
      if (!colon)
	continue;  /* not a name/value pair */

      *colon= '\0';  /* terminate */

      value = XP_StripLine (colon+1);

# define BOOLP(x) (strcasecmp((x),"TRUE") ? FALSE : TRUE)

      /* OPTIONS MENU
       */
      if (!strcasecmp ("SHOW_TOOLBAR", name))
	prefs->show_toolbar_p = BOOLP (value);
      else if (!strcasecmp ("SHOW_URL", name))
	prefs->show_url_p = BOOLP (value);
      else if (!strcasecmp ("SHOW_DIRECTORY_BUTTONS", name))
	prefs->show_directory_buttons_p = BOOLP (value);
      else if (!strcasecmp ("SHOW_MENUBAR", name))
	prefs->show_menubar_p = BOOLP (value);
      else if (!strcasecmp ("SHOW_BOTTOM_STATUS_BAR", name))
	prefs->show_bottom_status_bar_p = BOOLP (value);
      else if (!strcasecmp ("AUTOLOAD_IMAGES", name))
	prefs->autoload_images_p = BOOLP (value);
      else if (!strcasecmp ("FTP_FILE_INFO", name))
	prefs->fancy_ftp_p = BOOLP (value);
#ifdef HAVE_SECURITY
      else if (!strcasecmp ("SHOW_SECURITY_BAR", name))
	prefs->show_security_bar_p = BOOLP (value);
#endif


      /* APPLICATIONS
       */
      else if (!strcasecmp ("TN3270", name))
	StrAllocCopy (prefs->tn3270_command, value);
      else if (!strcasecmp ("TELNET", name))
	StrAllocCopy (prefs->telnet_command, value);
      else if (!strcasecmp ("RLOGIN", name))
	StrAllocCopy (prefs->rlogin_command, value);
      else if (!strcasecmp ("RLOGIN_USER", name))
	StrAllocCopy (prefs->rlogin_user_command, value);
#ifdef DEBUG_jwz
      else if (!strcasecmp ("OTHER_BROWSER", name))
	StrAllocCopy (prefs->other_browser, value);
#endif /* DEBUG_jwz */

      /* CACHE
       */
      else if (!strcasecmp ("MEMORY_CACHE_SIZE", name))
	prefs->memory_cache_size = atoi (value);
      else if (!strcasecmp ("DISK_CACHE_SIZE", name))
	prefs->disk_cache_size = atoi (value);
      else if (!strcasecmp ("CACHE_DIR", name))
	StrAllocCopy (prefs->cache_dir, value);
      else if (!strcasecmp ("VERIFY_DOCUMENTS", name))
	prefs->verify_documents = atoi (value);
      else if (!strcasecmp ("CACHE_SSL_PAGES", name))
	prefs->cache_ssl_p = BOOLP (value);

      /* COLORS
       */
	
      /* COMPOSITION
       */
      else if (!strcasecmp ("8BIT_MAIL_AND_NEWS", name))
	prefs->qp_p = !BOOLP (value);
      else if (!strcasecmp ("SEND_FORMATTED_TEXT", name))
	prefs->send_formatted_text_p = !BOOLP (value);
      else if (!strcasecmp ("QUEUE_FOR_LATER", name))
	prefs->queue_for_later_p = BOOLP (value);
      else if (!strcasecmp ("MAIL_BCC_SELF", name))
	prefs->mailbccself_p = BOOLP (value);
      else if (!strcasecmp ("NEWS_BCC_SELF", name))
	prefs->newsbccself_p = BOOLP (value);
      else if (!strcasecmp ("MAIL_BCC", name))
	StrAllocCopy (prefs->mail_bcc, value);
      else if (!strcasecmp ("NEWS_BCC", name))
	StrAllocCopy (prefs->news_bcc, value);
      else if (!strcasecmp ("MAIL_FCC", name))
	StrAllocCopy (prefs->mail_fcc, value);
      else if (!strcasecmp ("NEWS_FCC", name))
	StrAllocCopy (prefs->news_fcc, value);
      else if (!strcasecmp ("MAIL_AUTOQUOTE_REPLY", name))
	prefs->autoquote_reply = BOOLP (value);

      /* DIRECTORIES
       */
      else if (!strcasecmp ("TMPDIR", name))
	StrAllocCopy (prefs->tmp_dir, value);
      else if (!strcasecmp ("BOOKMARKS_FILE", name))
	StrAllocCopy (prefs->bookmark_file, value);
      else if (!strcasecmp ("HISTORY_FILE", name))
	StrAllocCopy (prefs->history_file, value);

      /* FONTS
       */
      else if (!strcasecmp ("DOC_CSID", name))
	prefs->doc_csid = atoi (value);
      else if (!strcasecmp ("FONT_CHARSET", name))
	fe_ReadFontCharSet(value);
      else if (!strcasecmp ("FONT_SPEC", name))
	fe_ReadFontSpec(value);
      else if (!strcasecmp ("CITATION_FONT", name))
	prefs->citation_font = (MSG_FONT) atoi (value);
      else if (!strcasecmp ("CITATION_SIZE", name))
	prefs->citation_size = (MSG_CITATION_SIZE) atoi (value);
      else if (!strcasecmp ("CITATION_COLOR", name))
	StrAllocCopy (prefs->citation_color, value);

      /* HELPERS
       */
      else if (!strcasecmp ("MIME_TYPES", name))
	StrAllocCopy (prefs->global_mime_types_file, value);
      else if (!strcasecmp ("PERSONAL_MIME_TYPES", name))
	StrAllocCopy (prefs->private_mime_types_file, value);
      else if (!strcasecmp ("MAILCAP", name))
	StrAllocCopy (prefs->global_mailcap_file, value);
      else if (!strcasecmp ("PERSONAL_MAILCAP", name))
	StrAllocCopy (prefs->private_mailcap_file, value);

      /* IDENTITY
       */
      else if (!strcasecmp ("REAL_NAME", name))
	StrAllocCopy (prefs->real_name, value);
      else if (!strcasecmp ("EMAIL_ADDRESS", name))
	StrAllocCopy (prefs->email_address, value);
#ifdef DEBUG_jwz
      else if (!strcasecmp ("EMAIL_ADDRESSES", name))
        {
          /* do continuation lines */
          value = strdup (value);
          while (1)
            {
              char buffer2 [1024];
              char c = fgetc (fp);
              char *s;
              if (c != '\t' && c != ' ')
                {
                  if (c != ungetc(c, fp))
                    abort();
                  break;
                }
              if (!fgets (buffer2, sizeof(buffer2), fp))
                abort();
              s = buffer2;
              while (*s == ' ' || *s == '\t')
                s++;
              XP_StripLine (s);
              StrAllocCat (value, "\n");
              StrAllocCat (value, s);
            }

          StrAllocCopy (prefs->email_addresses, value);
        }
#endif /* DEBUG_jwz */
      else if (!strcasecmp ("ORGANIZATION", name))
	StrAllocCopy (prefs->organization, value);
      else if (!strcasecmp ("SIGNATURE_FILE", name))
	StrAllocCopy (prefs->signature_file, value);
      else if (!strcasecmp ("SIGNATURE_DATE", name))
	prefs->signature_date = (time_t) atol (value);
      else if (!strcasecmp ("ANONYMITY_LEVEL", name))
	prefs->anonymity_level = atol (value);

      /* IMAGES
       */
      else if (!strcasecmp ("DITHER_IMAGES", name))
	StrAllocCopy (prefs->dither_images, value);
      else if (!strcasecmp ("STREAMING_IMAGES", name))
	prefs->streaming_images = BOOLP (value);
#ifdef DEBUG_jwz
      else if (!strcasecmp ("ANIMATIONS", name))
	prefs->anim_p = BOOLP (value);
      else if (!strcasecmp ("ANIMATION_LOOPS", name))
	prefs->anim_loop_p = BOOLP (value);
#endif /* DEBUG_jwz */

      /* MAIL
       */
      else if (!strcasecmp ("MAILHOST", name))
	StrAllocCopy (prefs->mailhost, value);
      else if (!strcasecmp ("MAIL_DIR", name))
/*####*/	StrAllocCopy (prefs->mail_directory, value);
      else if (!strcasecmp ("BIFF_INTERVAL", name))
	prefs->biff_interval = atol(value);
      else if (!strcasecmp ("AUTO_CHECK_MAIL", name))
	prefs->auto_check_mail = BOOLP (value);
      else if (!strcasecmp ("USE_MOVEMAIL", name))
	prefs->use_movemail_p = BOOLP (value);
      else if (!strcasecmp ("BUILTIN_MOVEMAIL", name))
	prefs->builtin_movemail_p = BOOLP (value);
      else if (!strcasecmp ("MOVEMAIL_PROGRAM", name))
	StrAllocCopy (prefs->movemail_program, value);
      else if (!strcasecmp ("POP3_HOST", name))
	StrAllocCopy (prefs->pop3_host, value);
      else if (!strcasecmp ("POP3_USER_ID", name))
	StrAllocCopy (prefs->pop3_user_id, value);
      else if (!strcasecmp ("POP3_LEAVE_ON_SERVER", name))
	prefs->pop3_leave_mail_on_server = BOOLP (value);
      else if (!strcasecmp ("POP3_MSG_SIZE_LIMIT", name))
	prefs->pop3_msg_size_limit = atol (value);
      else if (!strcasecmp ("MAIL_FOLDER_COLUMNS", name))
	StrAllocCopy(prefs->mail_folder_columns, value);
      else if (!strcasecmp ("MAIL_MESSAGE_COLUMNS", name))
	StrAllocCopy(prefs->mail_message_columns, value);

      else if (!strcasecmp ("MAIL_SASH_GEOMETRY", name))
	StrAllocCopy(prefs->mail_sash_geometry, value);
      else if (!strcasecmp ("AUTO_EMPTY_TRASH", name))
	prefs->emptyTrash = BOOLP (value);
      else if (!strcasecmp ("REMEMBER_MAIL_PASSWORD", name))
	prefs->rememberPswd = BOOLP (value);
      else if (!strcasecmp ("MAIL_PASSWORD", name))
	StrAllocCopy (prefs->pop3_password, value);
      else if (!strcasecmp ("THREAD_MAIL", name))
	prefs->mail_thread_p = BOOLP (value);
      else if (!strcasecmp ("MAIL_PANE_STYLE", name))
	prefs->mail_pane_style = atoi (value);
      else if (!strcasecmp ("MAIL_SORT_STYLE", name))
	prefs->mail_sort_style = atol (value);
      else if (!strcasecmp ("MOVEMAIL_WARN", name))
        prefs->movemail_warn = BOOLP (value);

      /* NETWORK
       */
      else if (!strcasecmp ("MAX_CONNECTIONS", name))
	prefs->max_connections = atoi (value);
      else if (!strcasecmp ("SOCKET_BUFFER_SIZE", name))
	prefs->network_buffer_size = atoi (value);

      /* PROTOCOLS
       */
      else if (!strcasecmp("USE_EMAIL_FOR_ANON_FTP", name))
      prefs->email_anonftp = BOOLP(value);
      else if (!strcasecmp("ALERT_EMAIL_FORM_SUBMIT", name))
	prefs->email_submit = BOOLP(value);
      else if (!strcasecmp("ACCEPT_COOKIE", name))
	prefs->accept_cookie = atoi(value);

      /* NEWS
       */
      else if (!strcasecmp ("NNTPSERVER", name))
	StrAllocCopy (prefs->newshost, value);
      else if (!strcasecmp ("NEWSRC_DIR", name))
	StrAllocCopy (prefs->newsrc_directory, value);
      else if (!strcasecmp ("NEWS_MAX_ARTICLES", name))
	prefs->news_max_articles = atol (value);
      else if (!strcasecmp ("NEWS_CACHE_HEADERS", name))
	prefs->news_cache_xover = BOOLP (value);
      else if (!strcasecmp ("SHOW_FIRST_UNREAD", name))
	prefs->show_first_unread_p = BOOLP (value);
      else if (!strcasecmp ("NEWS_FOLDER_COLUMNS", name))
	StrAllocCopy(prefs->news_folder_columns, value);
      else if (!strcasecmp ("NEWS_MESSAGE_COLUMNS", name))
	StrAllocCopy(prefs->news_message_columns, value);
      else if (!strcasecmp ("NEWS_SASH_GEOMETRY", name))
	StrAllocCopy(prefs->news_sash_geometry, value);
      else if (!strcasecmp ("THREAD_NEWS", name))
	prefs->no_news_thread_p = !BOOLP (value);
      else if (!strcasecmp ("NEWS_PANE_STYLE", name))
	prefs->news_pane_style = atoi (value);
      else if (!strcasecmp ("NEWS_SORT_STYLE", name))
	prefs->news_sort_style = atol (value);

      /* PROXIES
       */
      else if (!strcasecmp ("PROXY_URL", name))
	StrAllocCopy (prefs->proxy_url, value);
      else if (!strcasecmp ("PROXY_MODE", name))
	prefs->proxy_mode = atol (value);
      else if (!strcasecmp ("SOCKS_HOST", name))
	StrAllocCopy (prefs->socks_host, value);
      else if (!strcasecmp ("FTP_PROXY", name))
	StrAllocCopy (prefs->ftp_proxy, value);
      else if (!strcasecmp ("HTTP_PROXY", name))
	StrAllocCopy (prefs->http_proxy, value);
#ifdef HAVE_SECURITY
      else if (!strcasecmp ("HTTPS_PROXY", name))
	StrAllocCopy (prefs->https_proxy, value);
#endif
      else if (!strcasecmp ("GOPHER_PROXY", name))
	StrAllocCopy (prefs->gopher_proxy, value);
      else if (!strcasecmp ("WAIS_PROXY", name))
	StrAllocCopy (prefs->wais_proxy, value);
      else if (!strcasecmp ("NO_PROXY", name))
	StrAllocCopy (prefs->no_proxy, value);

      /* SECURITY
       */
#ifdef HAVE_SECURITY
      else if (!strcasecmp ("ASK_PASSWORD", name))
	prefs->ask_password = atoi (value);
      else if (!strcasecmp ("PASSWORD_TIMEOUT", name))
	prefs->password_timeout = atoi (value);

      else if (!strcasecmp ("WARN_ENTER_SECURE", name))
	prefs->enter_warn = BOOLP (value);
      else if (!strcasecmp ("WARN_LEAVE_SECURE", name))
	prefs->leave_warn = BOOLP (value);
      else if (!strcasecmp ("WARN_MIXED_SECURE", name))
	prefs->mixed_warn = BOOLP (value);
      else if (!strcasecmp ("WARN_SUBMIT_INSECURE", name))
	prefs->submit_warn = BOOLP (value);
#endif /* HAVE_SECURITY -- jwz */

#ifdef JAVA
      else if (!strcasecmp ("DISABLE_JAVA", name))
	prefs->disable_java = BOOLP (value);
#endif
#ifdef MOCHA
      else if (!strcasecmp ("DISABLE_JAVASCRIPT", name))
	prefs->disable_javascript = BOOLP (value);
#endif
#ifdef HAVE_SECURITY /* jwz */
      else if (!strcasecmp ("DEFAULT_USER_CERT", name))
	StrAllocCopy (prefs->def_user_cert, value);
      else if (!strcasecmp ("DEFAULT_MAIL_CERT", name))
	StrAllocCopy (prefs->def_mail_cert, value);
      else if (!strcasecmp ("ENABLE_SSL2", name))
	prefs->ssl2_enable = BOOLP (value);
      else if (!strcasecmp ("ENABLE_SSL3", name))
	prefs->ssl3_enable = BOOLP (value);
      else if (!strcasecmp ("CIPHER", name))
	StrAllocCopy (prefs->cipher, value);
#endif /* HAVE_SECURITY -- jwz */

      else if (!strcasecmp ("LICENSE_ACCEPTED", name))
	StrAllocCopy (prefs->license_accepted, value);

#ifdef HAVE_SECURITY /* jwz */
      /* New 4.0 preferences for 3.01... */
      else if (!strcasecmp ("CRYPTO_SIGN_OUTGOING_MAIL", name))
	prefs->crypto_sign_outgoing_mail = BOOLP (value);
      else if (!strcasecmp ("CRYPTO_SIGN_OUTGOING_NEWS", name))
	prefs->crypto_sign_outgoing_news = BOOLP (value);
      else if (!strcasecmp ("ENCRYPT_OUTGOING_MAIL", name))
	prefs->encrypt_outgoing_mail = BOOLP (value);
      else if (!strcasecmp ("WARN_FORWARD_ENCRYPTED", name))
	prefs->warn_forward_encrypted = BOOLP (value);
      else if (!strcasecmp ("WARN_REPLY_UNENCRYPTED", name))
	prefs->warn_reply_unencrypted = BOOLP (value);

      else if (!strcasecmp ("SECURITY_KEYGEN_MAXBITS", name))
	prefs->security_keygen_maxbits = atol (value);

      else if (!strcasecmp ("SECURITY_SSL2_DES_64", name))
	prefs->security_ssl2_des_64 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL2_DES_EDE3_192", name))
	prefs->security_ssl2_des_ede3_192 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL2_RC2_40", name))
	prefs->security_ssl2_rc2_40 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL2_RC2_128", name))
	prefs->security_ssl2_rc2_128 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL2_RC4_40", name))
	prefs->security_ssl2_rc4_40 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL2_RC4_128", name))
	prefs->security_ssl2_rc4_128 = BOOLP (value);

      else if (!strcasecmp ("SECURITY_SSL3_FORTEZZA_FORTEZZA_SHA", name))
	prefs->security_ssl3_fortezza_fortezza_sha = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_FORTEZZA_NULL_SHA", name))
	prefs->security_ssl3_fortezza_null_sha = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_FORTEZZA_RC4_SHA", name))
	prefs->security_ssl3_fortezza_rc4_sha = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_DES_EDE3_SHA", name))
	prefs->security_ssl3_rsa_des_ede3_sha = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_DES_SHA", name))
	prefs->security_ssl3_rsa_des_sha = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_NULL_MD5", name))
	prefs->security_ssl3_rsa_null_md5 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_RC2_40_MD5", name))
	prefs->security_ssl3_rsa_rc2_40_md5 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_RC4_40_MD5", name))
	prefs->security_ssl3_rsa_rc4_40_md5 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SSL3_RSA_RC4_128_MD5", name))
	prefs->security_ssl3_rsa_rc4_128_md5 = BOOLP (value);

      else if (!strcasecmp ("SECURITY_SMIME_DES", name))
	prefs->security_smime_des = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_DES_EDE3", name))
	prefs->security_smime_des_ede3 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC2_40", name))
	prefs->security_smime_rc2_40 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC2_64", name))
	prefs->security_smime_rc2_64 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC2_128", name))
	prefs->security_smime_rc2_128 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC5_B64R16K40", name))
	prefs->security_smime_rc5_b64r16k40 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC5_B64R16K64", name))
	prefs->security_smime_rc5_b64r16k64 = BOOLP (value);
      else if (!strcasecmp ("SECURITY_SMIME_RC5_B64R16K128", name))
	prefs->security_smime_rc5_b64r16k128 = BOOLP (value);
#endif /* HAVE_SECURITY -- jwz */


      /* STYLES (GENERAL APPEARANCE)
       */
      else if (!strcasecmp ("TOOLBAR_ICONS", name))
	prefs->toolbar_icons_p = BOOLP (value);
      else if (!strcasecmp ("TOOLBAR_TEXT", name))
	prefs->toolbar_text_p = BOOLP (value);
      else if (!strcasecmp ("TOOLBAR_TIPS", name))
	prefs->toolbar_tips_p = BOOLP (value);
      else if (!strcasecmp ("STARTUP_MODE", name))
	prefs->startup_mode = atoi (value);
      else if (!strcasecmp ("HOME_DOCUMENT", name))
	StrAllocCopy (prefs->home_document, value);
      else if (!strcasecmp ("UNDERLINE_LINKS", name))
	prefs->underline_links_p = BOOLP (value);
      else if (!strcasecmp ("HISTORY_EXPIRATION", name))
	prefs->global_history_expiration = atoi (value);
      else if (!strcasecmp ("FIXED_MESSAGE_FONT", name))
	prefs->fixed_message_font_p = BOOLP (value);
/*** no longer valid. we need one each for mail and news - dp
      else if (!strcasecmp ("SORT_MESSAGES", name))
	prefs->msg_sort_style = atol (value);
      else if (!strcasecmp ("THREAD_MESSAGES", name))
	prefs->msg_thread_p = BOOLP (value);
***/

      /* BOOKMARK
       */
      else if (!strcasecmp ("ADD_URLS", name))
	StrAllocCopy (prefs->put_added_urls_under, value);
      else if (!strcasecmp ("BOOKMARK_MENU", name))
	StrAllocCopy (prefs->begin_bookmark_menu_under, value);

      /* PRINT
       */
      else if (!strcasecmp ("PRINT_COMMAND", name))
	StrAllocCopy (prefs->print_command, value);
      else if (!strcasecmp ("PRINT_REVERSED", name))
	prefs->print_reversed = BOOLP (value);
      else if (!strcasecmp ("PRINT_COLOR", name))
	prefs->print_color = BOOLP (value);
      else if (!strcasecmp ("PRINT_LANDSCAPE", name))
	prefs->print_landscape = BOOLP (value);
      else if (!strcasecmp ("PRINT_PAPER", name))
	prefs->print_paper_size = atoi (value);

#ifdef DEBUG_jwz
      else if (!strcasecmp ("PRINT_SIZE", name))
	prefs->print_font_size = atoi (value);
      else if (!strcasecmp ("PRINT_HEADER_P", name))
	prefs->print_header_p = BOOLP (value);
      else if (!strcasecmp ("PRINT_FOOTER_P", name))
	prefs->print_footer_p = BOOLP (value);
      else if (!strcasecmp ("PRINT_HEADER", name))
	  StrAllocCopy (prefs->print_header, value);
      else if (!strcasecmp ("PRINT_FOOTER", name))
	  StrAllocCopy (prefs->print_footer, value);
      else if (!strcasecmp ("PRINT_HMARGIN", name))
	prefs->print_hmargin = atoi (value);
      else if (!strcasecmp ("PRINT_VMARGIN", name))
	prefs->print_vmargin = atoi (value);
#endif /* DEBUG_jwz */

#ifdef EDITOR
      /* EDITOR */
      else if (!strcasecmp ("EDITOR_CHARACTER_TOOLBAR", name))
	  prefs->editor_character_toolbar = BOOLP(value);
      else if (!strcasecmp ("EDITOR_PARAGRAPH_TOOLBAR", name))
	  prefs->editor_paragraph_toolbar = BOOLP(value);
      else if (!strcasecmp ("EDITOR_AUTHOR_NAME", name))
	  StrAllocCopy (prefs->editor_author_name, value);
      else if (!strcasecmp ("EDITOR_HTML_EDITOR", name))
	  StrAllocCopy (prefs->editor_html_editor, value);
      else if (!strcasecmp ("EDITOR_IMAGE_EDITOR", name))
	  StrAllocCopy (prefs->editor_image_editor, value);
      else if (!strcasecmp ("EDITOR_NEW_DOCUMENT_TEMPLATE", name))
	  StrAllocCopy (prefs->editor_document_template, value);
      else if (!strcasecmp ("EDITOR_AUTOSAVE_PERIOD", name))
	  prefs->editor_autosave_period = atoi(value);
      else if (!strcasecmp ("EDITOR_CUSTOM_COLORS", name))
	  prefs->editor_custom_colors = BOOLP(value);
      else if (!strcasecmp ("EDITOR_BACKGROUND_COLOR", name))
	  prefs_parse_color(&prefs->editor_background_color, value);
      else if (!strcasecmp ("EDITOR_NORMAL_COLOR", name))
	  prefs_parse_color(&prefs->editor_normal_color, value);
      else if (!strcasecmp ("EDITOR_LINK_COLOR", name))
	  prefs_parse_color(&prefs->editor_link_color, value);
      else if (!strcasecmp ("EDITOR_ACTIVE_COLOR", name))
	  prefs_parse_color(&prefs->editor_active_color, value);
      else if (!strcasecmp ("EDITOR_FOLLOWED_COLOR", name))
	  prefs_parse_color(&prefs->editor_followed_color, value);
      else if (!strcasecmp ("EDITOR_BACKGROUND_IMAGE", name))
	  StrAllocCopy (prefs->editor_background_image, value);
      else if (!strcasecmp ("EDITOR_MAINTAIN_LINKS", name))
	  prefs->editor_maintain_links = BOOLP(value);
      else if (!strcasecmp ("EDITOR_KEEP_IMAGES", name))
	  prefs->editor_keep_images = BOOLP(value);
      else if (!strcasecmp ("EDITOR_PUBLISH_LOCATION", name))
	  StrAllocCopy (prefs->editor_publish_location, value);
      else if (!strcasecmp ("EDITOR_BROWSE_LOCATION", name))
	  StrAllocCopy (prefs->editor_browse_location, value);
      else if (!strcasecmp ("EDITOR_PUBLISH_USERNAME", name))
	  StrAllocCopy (prefs->editor_publish_username, value);
      else if (!strcasecmp ("EDITOR_PUBLISH_PASSWORD", name))
	  StrAllocCopy (prefs->editor_publish_password, value);
      else if (!strcasecmp ("EDITOR_COPYRIGHT_HINT", name))
	  prefs->editor_copyright_hint = BOOLP(value);
      /* to to add publish stuff */
#endif /*EDITOR*/
    }

  fclose (fp);

  prefs->pop3_msg_size_limit_p = (prefs->pop3_msg_size_limit > 0) ? True : False;
  prefs->pop3_msg_size_limit = abs (prefs->pop3_msg_size_limit);

  if (!prefs->dither_images ||
      !strcasecmp (prefs->dither_images, "TRUE") ||
      !strcasecmp (prefs->dither_images, "FALSE"))
    StrAllocCopy (prefs->dither_images, "Auto");

  if (!strcasecomp(prefs->def_user_cert, "do the default"))
    prefs->def_user_cert = XP_STRDUP(prefs->email_address);
  if (!strcasecomp(prefs->def_mail_cert, "do the default"))
    prefs->def_mail_cert = XP_STRDUP(prefs->email_address);

  {
    char *home = getenv ("HOME");
    if (!home) home = "";

#define FROB(SLOT)							\
    if (prefs->SLOT && *prefs->SLOT == '~')				\
      {									\
	if (prefs->SLOT[1] != '/' && prefs->SLOT[1] != 0)		\
	  {								\
	    fprintf (stderr,	XP_GetString( XFE_TILDE_USER_SYNTAX_DISALLOWED ), \
		     XP_AppName);					\
	  }								\
	else								\
	  {								\
	    char *new = (char *)					\
	      malloc (strlen (prefs->SLOT) + strlen (home) + 3);	\
	    strcpy (new, home);						\
	    if (home[strlen(home)-1] != '/')				\
	      strcat (new, "/");					\
	    if (prefs->SLOT[1] && prefs->SLOT[2])			\
	      strcat (new, prefs->SLOT + 2);				\
	    free (prefs->SLOT);						\
	    prefs->SLOT = new;						\
	  }								\
      }

    FROB(cache_dir)
    FROB(mail_fcc)
    FROB(news_fcc)
    FROB(tmp_dir)
    FROB(bookmark_file)
    FROB(history_file)
    FROB(global_mime_types_file)
    FROB(private_mime_types_file)
    FROB(global_mailcap_file)
    FROB(private_mailcap_file)
    FROB(signature_file)
    FROB(mail_directory)
    FROB(movemail_program)
    FROB(newsrc_directory)

#ifdef EDITOR
    FROB(editor_html_editor);
    FROB(editor_image_editor);
    FROB(editor_background_image);
#endif

#ifdef DEBUG_jwz
    IL_AnimationLoopsEnabled = prefs->anim_loop_p;
    IL_AnimationsEnabled = prefs->anim_p;
#endif /* DEBUG_jwz */
  }

  return (TRUE);
}

static char * xfe_organization (void);

static void
xfe_prefs_from_environment (XFE_GlobalPrefs *prefs)
{
  /* Set some slots in the prefs structure based on the environment.
   */
  char *env;
  if ((env = getenv ("WWW_HOME")))     StrAllocCopy (prefs->home_document,env);
  if ((env = getenv ("NNTPSERVER")))   StrAllocCopy (prefs->newshost, env);
  if ((env = getenv ("TMPDIR")))       StrAllocCopy (prefs->tmp_dir, env);

  if ((env = getenv ("NEWSRC")))
    {
      char *s;
      StrAllocCopy (prefs->newsrc_directory, env);
      s = strrchr (prefs->newsrc_directory, '/');
      if (s) *s = 0;
    }

  /* These damn proxy env vars *should* be set to "host:port" but apparently
     for historical reasons they tend to be set to "http://host:port/proxy/"
     so if there are any slashes in the name, extract the host and port as
     if it were a URL.

     And, as extra BS, we need to map "http://host/" to "host:80".  FMH.
   */
#define PARSE_PROXY_ENV(NAME,VAR)				\
    if ((env = getenv (NAME)))					\
      {								\
	char *new = (strchr (env, '/')				\
		     ? NET_ParseURL (env, GET_HOST_PART)	\
		     : strdup (env));				\
        if (! strchr (new, ':'))				\
	  {							\
	    char *new2 = (char *) malloc (strlen (new) + 10);	\
	    strcpy (new2, new);					\
	    strcat (new2, ":80");				\
	    free (new);						\
	    new = new2;						\
	  }							\
	if (VAR) free (VAR);					\
	VAR = new;						\
      }
  PARSE_PROXY_ENV ("ftp_proxy", prefs->ftp_proxy)
  PARSE_PROXY_ENV ("http_proxy", prefs->http_proxy)
#ifdef HAVE_SECURITY
  PARSE_PROXY_ENV ("https_proxy", prefs->https_proxy)
#endif
  PARSE_PROXY_ENV ("gopher_proxy", prefs->gopher_proxy)
  PARSE_PROXY_ENV ("wais_proxy", prefs->wais_proxy)
  /* #### hack commas in some way? */
  PARSE_PROXY_ENV ("no_proxy", prefs->no_proxy)
#undef PARSE_PROXY_ENV

  if (prefs->organization) free (prefs->organization);
  prefs->organization = xfe_organization ();
}


/* Hack yet another damned environment variable.
   Returns a string for the organization, or "" if none.
   Consults the ORGANIZATION environment variable, which
   may be a string (the organization) or 
 */
static char *
xfe_organization ()
{
  char *e;

  e = getenv ("NEWSORG");	/* trn gives this priority. */
  if (!e || !*e)
    e = getenv ("ORGANIZATION");

  if (!e || !*e)
    {
      /* GNUS does this so it must be right... */
      char *home = getenv ("HOME");
      if (!home) home = "";
      e = (char *) malloc (strlen (home) + 20);
      strcpy (e, home);
      strcat (e, "/.organization");
    }
  else
    {
      e = strdup (e);
    }

  if (*e == '/')
    {
      FILE *f = fopen (e, "r");
      if (f)
	{
	  char buf [1024];
	  char *s = buf;
	  int left = sizeof (buf) - 5;
	  int read;
	  *s = 0;
	  while (1)
	    {
	      if (fgets (s, left, f))
		read = strlen (s);
	      else
		break;
	      left -= read;
	      s += read;
	      *s++ = '\t';
	    }
	  *s = 0;
	  fclose (f);
	  free (e);
	  e = strdup (fe_StringTrim (buf));
	}
      else
	{
	  /* File unreadable - set e to "" */
	  *e = 0;
	}
    }

  if (! e) abort ();
  return e;
}


int16
XFE_GetDefaultCSID(void)
{
  int16 csid;

  csid = 0;

  if ((fe_globalData.default_url_charset != NULL)
      && (fe_globalData.default_url_charset[0] != 0))
  	csid = (INTL_CharSetNameToID((char *)fe_globalData.default_url_charset));

  if (csid)
  {
	if (csid == CS_JIS)
	{
		csid = CS_JIS_AUTO;
	}
	return csid;
  }

  return (CS_LATIN1);
}

extern void ekit_DefaultPrefs(XFE_GlobalPrefs *prefs);

void
XFE_DefaultPrefs (int which, XFE_GlobalPrefs *prefs)
{
  char *home = getenv ("HOME");
  char *tmp = getenv ("TMPDIR");
  char buf [1024];
  if (!home) home = "";
  if (!tmp || !*tmp) tmp = 0;
  if (!strcmp (home, "/"))
    home = "";

#define REPLACE_STRING(slot,string)	\
  if (slot) free (slot);		\
  slot = strdup (string)

  if (which == xfe_PREFS_ALL || which == xfe_PREFS_CACHE)
    {
#ifndef OLD_UNIX_FILES
      PR_snprintf (buf, sizeof (buf), "%.900s/.netscape/cache/", home);
#else  /* OLD_UNIX_FILES */
      PR_snprintf (buf, sizeof (buf), "%.900s/.netscape-cache/", home);
#endif /* OLD_UNIX_FILES */
      REPLACE_STRING (prefs->cache_dir, buf);
      prefs->memory_cache_size			= 3000;
      prefs->disk_cache_size			= 5000;
      prefs->verify_documents			= 0;
      prefs->cache_ssl_p			= False;
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_COMPOSITION)
    {
      prefs->qp_p = FALSE;
      prefs->send_formatted_text_p = FALSE;
      prefs->queue_for_later_p = FALSE;
      prefs->mail_bcc = 0;
      prefs->news_bcc = 0;
      PR_snprintf (buf, sizeof (buf), "%.900s/nsmail/Sent", home);
      REPLACE_STRING (prefs->mail_fcc, buf);
      REPLACE_STRING (prefs->news_fcc, buf);
      prefs->autoquote_reply = True;
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_SERVERS)
    {
      char buf [1024];
      char *id = 0, *name = 0;
      fe_DefaultUserInfo (&id, &name, False);

      REPLACE_STRING (prefs->mailhost,		"localhost");
      REPLACE_STRING (prefs->pop3_host,         "pop");
      REPLACE_STRING (prefs->pop3_user_id,      (id ? id : "unknown"));
      REPLACE_STRING (prefs->movemail_program,	"");
      if (name) free (name);
      if (id) free (id);

      PR_snprintf (buf, sizeof (buf), "%.900s/nsmail/", home);
      REPLACE_STRING (prefs->mail_directory, 	buf);

      prefs->use_movemail_p = TRUE;
      prefs->builtin_movemail_p = TRUE;
      prefs->pop3_leave_mail_on_server = FALSE;
      prefs->pop3_msg_size_limit = -50000;
      prefs->pop3_msg_size_limit_p = False;
      REPLACE_STRING(prefs->mail_folder_columns, "");
      REPLACE_STRING(prefs->mail_message_columns, "");
      prefs->mail_pane_style = FE_PANES_NORMAL;
      prefs->news_pane_style = FE_PANES_NORMAL;
      prefs->biff_interval = 10;
      prefs->auto_check_mail = True;

      REPLACE_STRING (prefs->tmp_dir,		(tmp ? tmp : "/tmp"));
#ifndef OLD_UNIX_FILES
      PR_snprintf (buf, sizeof (buf),
		    "%.900s/.netscape/bookmarks.html",	home);
#else  /* OLD_UNIX_FILES */
      PR_snprintf (buf, sizeof (buf),
		    "%.900s/.netscape-bookmarks.html",	home);
#endif /* OLD_UNIX_FILES */
      REPLACE_STRING (prefs->bookmark_file, 		buf);
#ifndef OLD_UNIX_FILES
      PR_snprintf (buf, sizeof (buf), "%.900s/.netscape/history.db", home);
#else  /* OLD_UNIX_FILES */
      PR_snprintf (buf, sizeof (buf), "%.900s/.netscape-history.db", home);
#endif /* OLD_UNIX_FILES */
      REPLACE_STRING (prefs->history_file, 		buf);

      REPLACE_STRING (prefs->newshost,		"news");
      prefs->news_max_articles			= 100;
      REPLACE_STRING (prefs->newsrc_directory, 	home);
      prefs->news_cache_xover			= FALSE;
      prefs->show_first_unread_p		= FALSE;
      REPLACE_STRING(prefs->news_folder_columns, "");
      REPLACE_STRING(prefs->news_message_columns, "");
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_FONTS)
    {
      prefs->doc_csid = XFE_GetDefaultCSID();
      prefs->citation_font = MSG_ItalicFont;
      prefs->citation_size = MSG_NormalSize;
      REPLACE_STRING (prefs->citation_color, "");
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_HELPERS)
    {
#define DIR "/usr/local/lib/netscape/"
      REPLACE_STRING (prefs->global_mime_types_file, DIR "mime.types");
      REPLACE_STRING (prefs->global_mailcap_file,    DIR "mailcap");
#undef DIR
      PR_snprintf (buf, sizeof (buf), "%.900s/.mime.types", home);
      REPLACE_STRING (prefs->private_mime_types_file, buf);
      PR_snprintf (buf, sizeof (buf), "%.900s/.mailcap", home);
      REPLACE_STRING (prefs->private_mailcap_file, buf);

      REPLACE_STRING (prefs->telnet_command,	"xterm -e telnet %h %p");
      REPLACE_STRING (prefs->tn3270_command,	"xterm -e tn3270 %h");
      REPLACE_STRING (prefs->rlogin_command,	"xterm -e rlogin %h");
      REPLACE_STRING (prefs->rlogin_user_command,"xterm -e rlogin %h -l %u");
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_IDENTITY)
    {
      char *o = xfe_organization ();
      fe_DefaultUserInfo (&prefs->email_address, &prefs->real_name, False);
      REPLACE_STRING (prefs->organization,	o);
      PR_snprintf (buf, sizeof (buf), "%.900s/.signature", home);
      REPLACE_STRING (prefs->signature_file, 	buf);
      prefs->anonymity_level = 0;
      free (o);
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_IMAGES)
    {
      REPLACE_STRING (prefs->dither_images, "Auto");
      prefs->streaming_images	= True;
#ifdef DEBUG_jwz
      prefs->anim_p = True;
      prefs->anim_loop_p = True;
#endif /* DEBUG_jwz */
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_NETWORK)
    {
      prefs->network_buffer_size		= 32;
      prefs->max_connections			= 4;
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_PROXIES)
    {
      prefs->proxy_mode = 0; /* no proxies */
      REPLACE_STRING (prefs->proxy_url,		"");

      REPLACE_STRING (prefs->ftp_proxy,		"");
      REPLACE_STRING (prefs->gopher_proxy,	"");
      REPLACE_STRING (prefs->http_proxy,	"");
#ifdef HAVE_SECURITY /* added by jwz */
      REPLACE_STRING (prefs->https_proxy,	"");
#endif /* HAVE_SECURITY */
      REPLACE_STRING (prefs->wais_proxy,	"");
      REPLACE_STRING (prefs->no_proxy,		"");
      REPLACE_STRING (prefs->socks_host,	"");
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_PROTOCOLS)
    {
      prefs->email_anonftp = False;
      prefs->email_submit = True;
      prefs->accept_cookie = NET_SilentCookies;
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_SEC_GENERAL)
    {
      prefs->ask_password	= 1;
      prefs->password_timeout	= 30;

#ifdef HAVE_SECURITY
      prefs->enter_warn		= True;
      prefs->leave_warn		= True;
      prefs->mixed_warn		= True;
      prefs->submit_warn	= True;
      prefs->ssl2_enable        = True;
      prefs->ssl3_enable        = True;
#else
      prefs->enter_warn		= False;
      prefs->leave_warn		= False;
      prefs->mixed_warn		= False;
      prefs->submit_warn	= False;
      prefs->ssl2_enable        = False;
      prefs->ssl3_enable        = False;
#endif

#ifdef JAVA
      prefs->disable_java	= False;
#endif
#ifdef MOCHA
      prefs->disable_javascript	= False;
#endif /* MOCHA */

      prefs->cipher             = NULL;
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_SEC_PASSWORDS)
    {
      /* Nothing to do for now */
    }
  if (which == xfe_PREFS_ALL || which == xfe_PREFS_SEC_PERSONAL)
    {
      REPLACE_STRING(prefs->def_user_cert, "do the default");
      REPLACE_STRING(prefs->def_mail_cert, "do the default");
    }

  /* New 4.0 preferences for 3.01... */
  if (which == xfe_PREFS_ALL)
    {
      prefs->crypto_sign_outgoing_mail = FALSE;
      prefs->crypto_sign_outgoing_news = FALSE;
      prefs->encrypt_outgoing_mail     = FALSE;
      prefs->warn_forward_encrypted    = TRUE;
      prefs->warn_reply_unencrypted    = TRUE;

      prefs->security_keygen_maxbits		 = 512;

      prefs->security_ssl2_des_ede3_192		 = TRUE;  /* Domestic */
      prefs->security_ssl2_des_64		 = TRUE;  /* Domestic */
      prefs->security_ssl2_rc2_40		 = TRUE;  /* Export   */
      prefs->security_ssl2_rc2_128		 = TRUE;  /* Domestic */
      prefs->security_ssl2_rc4_40		 = TRUE;  /* Export   */
      prefs->security_ssl2_rc4_128		 = TRUE;  /* Domestic */

      prefs->security_ssl3_fortezza_fortezza_sha = TRUE;  /* Fortezza */
      prefs->security_ssl3_fortezza_null_sha	 = FALSE; /* Fortezza */
      prefs->security_ssl3_fortezza_rc4_sha	 = TRUE;  /* Fortezza */
      prefs->security_ssl3_rsa_des_ede3_sha	 = TRUE;  /* Domestic */
      prefs->security_ssl3_rsa_des_sha		 = TRUE;  /* Domestic */
      prefs->security_ssl3_rsa_null_md5		 = FALSE; /* Export   */
      prefs->security_ssl3_rsa_rc2_40_md5	 = TRUE;  /* Export   */
      prefs->security_ssl3_rsa_rc4_40_md5	 = TRUE;  /* Export   */
      prefs->security_ssl3_rsa_rc4_128_md5	 = TRUE;  /* Domestic */

      prefs->security_smime_des			 = TRUE;  /* Domestic */
      prefs->security_smime_des_ede3		 = TRUE;  /* Domestic */
      prefs->security_smime_rc2_40		 = TRUE;  /* Export   */
      prefs->security_smime_rc2_64		 = TRUE;  /* Domestic */
      prefs->security_smime_rc2_128		 = TRUE;  /* Domestic */
      prefs->security_smime_rc5_b64r16k40 	 = TRUE;
      prefs->security_smime_rc5_b64r16k64 	 = TRUE;
      prefs->security_smime_rc5_b64r16k128	 = TRUE;
    }

  if (which == xfe_PREFS_ALL || which == xfe_PREFS_STYLES)
    {
      char *h = (
#if defined(__sgi)
		 fe_VendorAnim
		 ? XP_GetString(XFE_WELCOME_HTML) :
#endif
		 xfe_get_netscape_home_page_url());

      prefs->toolbar_icons_p			= True;
      prefs->toolbar_text_p			= True;
      prefs->toolbar_tips_p			= True;

      prefs->startup_mode			= 0;

      REPLACE_STRING (prefs->home_document, h);
      prefs->underline_links_p			= True;
      prefs->global_history_expiration		= 9;
      prefs->fixed_message_font_p		= FALSE;
/*** no longer valid. we need one each for mail and news - dp
      prefs->msg_sort_style = 0;
      prefs->msg_thread_p = TRUE;
***/
      prefs->show_toolbar_p			= True;
      prefs->show_url_p				= True;
      prefs->show_directory_buttons_p		= True;
      prefs->show_menubar_p			= True;
      prefs->show_bottom_status_bar_p		= True;
      prefs->autoload_images_p			= True;
      prefs->fancy_ftp_p			= True;
      prefs->doc_csid				= XFE_GetDefaultCSID();
#ifdef HAVE_SECURITY
      prefs->show_security_bar_p		= True;
#endif
    }

#ifdef EDITOR
    if (which == xfe_PREFS_ALL) {
	fe_EditorDefaultGetColors(&prefs->editor_background_color,
				  &prefs->editor_normal_color,
				  &prefs->editor_link_color,
				  &prefs->editor_active_color,
				  &prefs->editor_followed_color);

	/* editor_document_template is only default string allocated */
	REPLACE_STRING(prefs->editor_document_template,
		       fe_EditorDefaultGetTemplate());
	REPLACE_STRING(prefs->editor_html_editor, "xterm -e vi %f");
	prefs->editor_autosave_period = 0; /* minutes */
	prefs->editor_maintain_links = TRUE;
	prefs->editor_keep_images = TRUE;
	prefs->editor_copyright_hint = TRUE;
	prefs->editor_character_toolbar = TRUE;
	prefs->editor_paragraph_toolbar = TRUE;
    }
#endif

  if (which == xfe_PREFS_ALL) /* ### || which == xfe_PREFS_PRINT) */
    {
#if defined(__sgi) || (defined(__sun) && defined(__svr4__))
      REPLACE_STRING (prefs->print_command,	"lp");
#else
      REPLACE_STRING (prefs->print_command,	"lpr");
#endif
      prefs->print_reversed			= False;
      prefs->print_color			= True;
      prefs->print_landscape			= False;
      prefs->print_paper_size			= 0;
#ifdef DEBUG_jwz
      prefs->print_font_size                    = 0;
      prefs->print_header_p                     = True;
      prefs->print_footer_p                     = True;
      REPLACE_STRING (prefs->print_header,	"");
      REPLACE_STRING (prefs->print_footer,	"");
      prefs->print_hmargin			= 72 * 0.75;
      prefs->print_vmargin			= 72;
#endif /* DEBUG_jwz */
    }

  /*
   * This doesn't have a place in the prefs dialog yet,
   * So only set the default if we're resetting *all*
   * of the prefs.  (Like when ~/.netscape/prefs isn't on disk.)
   * DEM 4/30/96
   */
  if ( which == xfe_PREFS_ALL ) {
      prefs->movemail_warn = False;
  }

  /*
   * If the user specified alternate default prefs using the e-kit,
   * use them instead of the hard-wired values.
   */
  ekit_DefaultPrefs(prefs);
#undef REPLACE_STRING
}



/* bare-minimum implementation of 4.0 preferences in 3.01. */

#include "prefapi.h"

static XP_Bool *
pref_get_bool_slot(const char *pref_name, XFE_GlobalPrefs *prefs)
{
  if (!strcasecomp(pref_name,		     "mail.crypto_sign_outgoing_mail"))
    return &prefs->crypto_sign_outgoing_mail;
  else if (!strcasecomp(pref_name,	     "mail.crypto_sign_outgoing_news"))
    return &prefs->crypto_sign_outgoing_news;
  else if (!strcasecomp(pref_name,	     "mail.encrypt_outgoing_mail"))
    return &prefs->encrypt_outgoing_mail;
  else if (!strcasecomp(pref_name,	     "mail.warn_forward_encrypted"))
    return &prefs->warn_forward_encrypted;
  else if (!strcasecomp(pref_name,	     "mail.warn_reply_unencrypted"))
    return &prefs->warn_reply_unencrypted;
  else if (!strcasecomp(pref_name,	     "security.enable_ssl2"))
    return &prefs->ssl2_enable;
  else if (!strcasecomp(pref_name,	     "security.enable_ssl3"))
    return &prefs->ssl3_enable;
  else if (!strcasecomp(pref_name,	     "security.warn_viewing_mixed"))
    return &prefs->mixed_warn;
  else if (!strcasecomp(pref_name,	     "security.warn_entering_secure"))
    return &prefs->enter_warn;
  else if (!strcasecomp(pref_name,	     "security.warn_leaving_secure"))
    return &prefs->leave_warn;
  else if (!strcasecomp(pref_name,	     "security.warn_submit_insecure"))
    return &prefs->submit_warn;

  else if (!strcasecomp(pref_name,	     "security.smime.des"))
    return &prefs->security_smime_des;
  else if (!strcasecomp(pref_name,	     "security.smime.des_ede3"))
    return &prefs->security_smime_des_ede3;
  else if (!strcasecomp(pref_name,	     "security.smime.rc2_40"))
    return &prefs->security_smime_rc2_40;
  else if (!strcasecomp(pref_name,	     "security.smime.rc2_64"))
    return &prefs->security_smime_rc2_64;
  else if (!strcasecomp(pref_name,	     "security.smime.rc2_128"))
    return &prefs->security_smime_rc2_128;
  else if (!strcasecomp(pref_name,	     "security.smime.rc5.b64r16k128"))
    return &prefs->security_smime_rc5_b64r16k128;
  else if (!strcasecomp(pref_name,	     "security.smime.rc5.b64r16k40"))
    return &prefs->security_smime_rc5_b64r16k40;
  else if (!strcasecomp(pref_name,	     "security.smime.rc5.b64r16k64"))
    return &prefs->security_smime_rc5_b64r16k64;
  else if (!strcasecomp(pref_name,	     "security.ssl2.des_64"))
    return &prefs->security_ssl2_des_64;
  else if (!strcasecomp(pref_name,	     "security.ssl2.des_ede3_192"))
    return &prefs->security_ssl2_des_ede3_192;
  else if (!strcasecomp(pref_name,	     "security.ssl2.rc2_40"))
    return &prefs->security_ssl2_rc2_40;
  else if (!strcasecomp(pref_name,	     "security.ssl2.rc2_128"))
    return &prefs->security_ssl2_rc2_128;
  else if (!strcasecomp(pref_name,	     "security.ssl2.rc4_40"))
    return &prefs->security_ssl2_rc4_40;
  else if (!strcasecomp(pref_name,	     "security.ssl2.rc4_128"))
    return &prefs->security_ssl2_rc4_128;
  else if (!strcasecomp(pref_name,	"security.ssl3.fortezza_fortezza_sha"))
    return &prefs->security_ssl3_fortezza_fortezza_sha;
  else if (!strcasecomp(pref_name,	    "security.ssl3.fortezza_null_sha"))
    return &prefs->security_ssl3_fortezza_null_sha;
  else if (!strcasecomp(pref_name,	     "security.ssl3.fortezza_rc4_sha"))
    return &prefs->security_ssl3_fortezza_rc4_sha;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_des_ede3_sha"))
    return &prefs->security_ssl3_rsa_des_ede3_sha;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_des_sha"))
    return &prefs->security_ssl3_rsa_des_sha;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_null_md5"))
    return &prefs->security_ssl3_rsa_null_md5;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_rc2_40_md5"))
    return &prefs->security_ssl3_rsa_rc2_40_md5;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_rc4_40_md5"))
    return &prefs->security_ssl3_rsa_rc4_40_md5;
  else if (!strcasecomp(pref_name,	     "security.ssl3.rsa_rc4_128_md5"))
    return &prefs->security_ssl3_rsa_rc4_128_md5;

  else
    {
      XP_ASSERT(0);
      return 0;
    }
}

static int *
pref_get_int_slot(const char *pref_name, XFE_GlobalPrefs *prefs)
{
  if (!strcasecomp(pref_name,      "security.ask_for_password"))
    return &prefs->ask_password;
  else if (!strcasecomp(pref_name, "security.password_lifetime"))
    return &prefs->password_timeout;
  else if (!strcasecomp(pref_name, "security.keygen.maxbits"))
    return &prefs->security_keygen_maxbits;
  else
    {
      XP_ASSERT(0);
      return 0;
    }
}

static char **
pref_get_string_slot(const char *pref_name, XFE_GlobalPrefs *prefs)
{
  if (!strcasecomp(pref_name, "security.ciphers"))
    return &prefs->cipher;
  else if (!strcasecomp(pref_name, "security.default_personal_cert"))
    return &prefs->def_user_cert;
  else if (!strcasecomp(pref_name, "security.default_mail_cert"))
    return &prefs->def_mail_cert;
  else
    {
      XP_ASSERT(0);
      return 0;
    }
}

int
PREF_SetBoolPref(const char *pref, XP_Bool value)
{
  XP_Bool *slot = pref_get_bool_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  *slot = value;
  return 0;
}

int
PREF_SetIntPref(const char *pref, int32 value)
{
  int *slot = pref_get_int_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  *slot = value;
  return 0;
}

int
PREF_SetCharPref(const char *pref, const char *value)
{
  char **slot = pref_get_string_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  if (*slot) free(*slot);
  *slot = strdup(value ? value : "");
  return 0;
}

int
PREF_GetBoolPref(const char *pref, XP_Bool *return_val)
{
  XP_Bool *slot = pref_get_bool_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  *return_val = *slot;
  return 0;
}

int
PREF_GetIntPref(const char *pref, int32 *return_int)
{
  int *slot = pref_get_int_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  *return_int = *slot;
  return 0;
}

int
PREF_GetCharPref(const char *pref, char *return_buf, int *buf_len)
{
  char **slot = pref_get_string_slot (pref, &fe_globalPrefs);
  if (!slot) return -1;
  if (!*slot)
    *return_buf = 0;
  else
    strcpy(return_buf, *slot);	     /* Robert T. Morris, Jr. Memorial Bug. */
  *buf_len = strlen(return_buf);
  return 0;
}

int
PREF_CopyCharPref(const char *pref, char **return_buf)
{
  char **slot = pref_get_string_slot (pref, &fe_globalPrefs);
  *return_buf = 0;
  if (!slot) return -1;
  if (*slot)
    *return_buf = strdup(*slot);
  else
    *return_buf = strdup("");
  return 0;
}


extern int XFE_ERROR_SAVING_OPTIONS;

int
PREF_SavePrefFile(void)
{
  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    {
      fe_perror (fe_all_MWContexts->context,
		 XP_GetString(XFE_ERROR_SAVING_OPTIONS));
      return -1;
    }
  return 0;
}

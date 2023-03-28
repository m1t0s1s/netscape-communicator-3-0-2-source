/* -*- Mode: C; tab-width: 8 -*-
   prefs.c --- reading and writing X preferences settings
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
 */

#ifndef _PREFS_H_
#define _PREFS_H_

#include "msgcom.h"

/* global preferences structure.
 */
typedef struct _XFE_GlobalPrefs 
{

  /* OPTIONS MENU
   */
  XP_Bool show_toolbar_p;
  XP_Bool show_url_p;
  XP_Bool show_directory_buttons_p;
  XP_Bool show_menubar_p;
  XP_Bool show_bottom_status_bar_p;
  XP_Bool autoload_images_p;
  XP_Bool fancy_ftp_p;
#ifdef HAVE_SECURITY
  XP_Bool show_security_bar_p;
#endif

  /* APPLICATIONS
   */
  char *tn3270_command;
  char *telnet_command;
  char *rlogin_command;
  char *rlogin_user_command;

#ifdef DEBUG_jwz
  char *other_browser;
#endif /* DEBUG_jwz */

  /* CACHE
   */
  int memory_cache_size;
  int disk_cache_size;
  char *cache_dir;
  int verify_documents;
  XP_Bool cache_ssl_p;

  /* COLORS
   */


  /* COMPOSITION
   */
  XP_Bool qp_p;
/*####*/ XP_Bool send_formatted_text_p;
  XP_Bool queue_for_later_p;
  char *mail_bcc;
  char *news_bcc;
  char *mail_fcc;
  char *news_fcc;
/*XXX*/  XP_Bool mailbccself_p;
/*XXX*/  XP_Bool newsbccself_p;
/*XXX*/  XP_Bool autoquote_reply;

  /* DIRECTORIES
   */
  char *tmp_dir;
  char *bookmark_file;
/*####*/  char *history_file;

  /* FONTS
   */
  /* DEFAULT MIME CSID - used if unspecified in HTTP header */
  int16 doc_csid;
/*####*/ MSG_FONT citation_font;
/*####*/ MSG_CITATION_SIZE citation_size;
  char *citation_color;


  /* HELPERS
   */
  char *global_mime_types_file;
  char *private_mime_types_file;
  char *global_mailcap_file;
  char *private_mailcap_file;

  /* IDENTITY
   */
  char *real_name;
  char *email_address;
  char *organization;
  char *signature_file;
  time_t signature_date;
/* #### */ int anonymity_level;

#ifdef DEBUG_jwz
  char *email_addresses;
#endif /* DEBUG_jwz */

  /* IMAGES
   */
  char *dither_images;
  XP_Bool streaming_images;
#ifdef DEBUG_jwz
  XP_Bool anim_p;
  XP_Bool anim_loop_p;
#endif /* DEBUG_jwz */

  /* MAIL
   */
  char *mailhost;
  XP_Bool use_movemail_p;
  XP_Bool builtin_movemail_p;
  char *movemail_program;
  char *pop3_host;
  char *pop3_user_id;
  char *mail_directory;
  int biff_interval;
  XP_Bool auto_check_mail;
  XP_Bool pop3_leave_mail_on_server;
  XP_Bool pop3_msg_size_limit_p;
  int pop3_msg_size_limit;
  char* mail_folder_columns;
  char* mail_message_columns;
  char* mail_sash_geometry;
  XP_Bool movemail_warn;

  /* NETWORK
   */
  int max_connections;
  int network_buffer_size;

  /* PROTOCOLS
   */
  XP_Bool email_anonftp;
  XP_Bool email_submit;
  int accept_cookie;	/* 0:disable, 1:enable-nowarn, else:enable-warn */

  /* NEWS
   */
  char *newshost;
  char *newsrc_directory;
  int news_max_articles;
  XP_Bool news_cache_xover;
  XP_Bool show_first_unread_p;
  char* news_folder_columns;
  char* news_message_columns;
  char* news_sash_geometry;

  /* PROXIES
   */
  char *socks_host;

  char *ftp_proxy;
  char *http_proxy;
  char *gopher_proxy;
  char *wais_proxy;
#ifdef HAVE_SECURITY
  char *https_proxy;
#endif
  char *no_proxy;

  int proxy_mode;
  char *proxy_url;

  /* SECURITY
   */
  XP_Bool enter_warn;
  XP_Bool leave_warn;
  XP_Bool mixed_warn;
  XP_Bool submit_warn;
#ifdef JAVA
  XP_Bool disable_java;
#endif
#ifdef MOCHA
  XP_Bool disable_javascript;
#endif /* MOCHA */
  XP_Bool ssl2_enable;
  XP_Bool ssl3_enable;
  char *cipher;
  char *def_user_cert;
  char *def_mail_cert;

  int ask_password;
  int password_timeout;

  XP_Bool crypto_sign_outgoing_mail;
  XP_Bool crypto_sign_outgoing_news;
  XP_Bool encrypt_outgoing_mail;
  XP_Bool warn_forward_encrypted;
  XP_Bool warn_reply_unencrypted;

  int security_keygen_maxbits;

  XP_Bool security_ssl2_des_ede3_192;
  XP_Bool security_ssl2_des_64;
  XP_Bool security_ssl2_rc2_40;
  XP_Bool security_ssl2_rc2_128;
  XP_Bool security_ssl2_rc4_40;
  XP_Bool security_ssl2_rc4_128;

  XP_Bool security_ssl3_fortezza_fortezza_sha;
  XP_Bool security_ssl3_fortezza_null_sha;
  XP_Bool security_ssl3_fortezza_rc4_sha;
  XP_Bool security_ssl3_rsa_des_ede3_sha;
  XP_Bool security_ssl3_rsa_des_sha;
  XP_Bool security_ssl3_rsa_null_md5;
  XP_Bool security_ssl3_rsa_rc2_40_md5;
  XP_Bool security_ssl3_rsa_rc4_40_md5;
  XP_Bool security_ssl3_rsa_rc4_128_md5;

  XP_Bool security_smime_des;
  XP_Bool security_smime_des_ede3;
  XP_Bool security_smime_rc2_40;
  XP_Bool security_smime_rc2_64;
  XP_Bool security_smime_rc2_128;
  XP_Bool security_smime_rc5_b64r16k40;
  XP_Bool security_smime_rc5_b64r16k64;
  XP_Bool security_smime_rc5_b64r16k128;


  /* STYLES 1
   */
  XP_Bool toolbar_icons_p;
  XP_Bool toolbar_text_p;
  XP_Bool toolbar_tips_p;
  char *home_document;		/* "" means start blank */

  int startup_mode;		/* browser, mail or news */

  XP_Bool underline_links_p;
  int global_history_expiration;	/* days */

  /* STYLES 2
   */
/*####*/ XP_Bool fixed_message_font_p;
/*** no longer valid. we need one each for mail and news - dp
  int msg_sort_style;
  int msg_thread_p;
***/

  /* Mail and News Organization
   */
/*XXX*/  XP_Bool emptyTrash;
  XP_Bool rememberPswd;
  char *pop3_password;
  XP_Bool mail_thread_p;
  XP_Bool no_news_thread_p;

#define FE_PANES_NORMAL       0
#define FE_PANES_STACKED      1
#define FE_PANES_HORIZONTAL   2
#define FE_PANES_TALL_FOLDERS 3

  XP_Bool mail_pane_style;
  XP_Bool news_pane_style;

  int mail_sort_style;
  int news_sort_style;


  /* BOOKMARK
   */
  char *put_added_urls_under;
  char *begin_bookmark_menu_under;

  /* PRINT
   */
  char *print_command;
  XP_Bool print_reversed;
  XP_Bool print_color;
  XP_Bool print_landscape;
  int print_paper_size;

#ifdef DEBUG_jwz
  int print_font_size;
  int print_hmargin;
  int print_vmargin;
  XP_Bool print_header_p;
  XP_Bool print_footer_p;
  char *print_header;
  char *print_footer;
#endif /* DEBUG_jwz */


#ifdef EDITOR
  /* EDITOR */
  XP_Bool  editor_character_toolbar;
  XP_Bool  editor_paragraph_toolbar;

  char*    editor_author_name;
  char*    editor_html_editor;
  char*    editor_image_editor;
  char*    editor_document_template;
  int32    editor_autosave_period;
  
  XP_Bool  editor_custom_colors;
  LO_Color editor_background_color;
  LO_Color editor_normal_color;
  LO_Color editor_link_color;
  LO_Color editor_active_color;
  LO_Color editor_followed_color;
  char*    editor_background_image;

  XP_Bool  editor_maintain_links;
  XP_Bool  editor_keep_images;
  char*    editor_publish_location;
  char*    editor_publish_username;
  char*    editor_publish_password;
  XP_Bool  editor_save_publish_password;
  char*    editor_browse_location;

  XP_Bool  editor_copyright_hint;

  /* to to add publish stuff */
#endif

  /* Lawyer nonsense */
  char *license_accepted;

} XFE_GlobalPrefs;


#define xfe_PREFS_ALL		-1

/* General */
#define xfe_GENERAL_OFFSET	0
#define xfe_GENERAL(which) (which-xfe_GENERAL_OFFSET)

#define xfe_PREFS_STYLES	0
#define xfe_PREFS_FONTS		1
#define xfe_PREFS_APPS		2
#define xfe_PREFS_HELPERS	3
#define xfe_PREFS_IMAGES	4
#define xfe_PREFS_LANGUAGES	5

/* Mail/News */
#define xfe_MAILNEWS_OFFSET	10
#define xfe_MAILNEWS(which) (which-xfe_MAILNEWS_OFFSET)

#define xfe_PREFS_APPEARANCE	10
#define xfe_PREFS_COMPOSITION	11
#define xfe_PREFS_SERVERS	12
#define xfe_PREFS_IDENTITY	13
#define xfe_PREFS_ORGANIZATION	14

/* Network */
#define xfe_NETWORK_OFFSET	20
#define xfe_NETWORK(which) (which-xfe_NETWORK_OFFSET)

#define xfe_PREFS_CACHE		20
#define xfe_PREFS_NETWORK	21
#define xfe_PREFS_PROXIES	22
#define xfe_PREFS_PROTOCOLS	23
#define xfe_PREFS_LANG		24

/* Security */
#define xfe_SECURITY_OFFSET	30
#define xfe_SECURITY(which) (which-xfe_SECURITY_OFFSET)

#define xfe_PREFS_SEC_GENERAL	30
#define xfe_PREFS_SEC_PASSWORDS	31
#define xfe_PREFS_SEC_PERSONAL	32
#define xfe_PREFS_SEC_SITE	33
/* Editor Text item from the Properties pulldown menu 26FEB96RCJ */
#define xfe_PROPERTY_CHARACTER	34	/* added 26FEB96RCJ */
#define xfe_PROPERTY_LINK	35	/* added 26FEB96RCJ */
#define xfe_PROPERTY_PARAGRAPH	36	/* added 26FEB96RCJ */

/*
#define xfe_PREFS_OPTIONS
#define xfe_PREFS_PRINT	
*/


/* Fills in the default preferences; which says which ones to fill in. */
extern void XFE_DefaultPrefs(int which, XFE_GlobalPrefs *prefs);

/* reads in the global preferences.
 *
 * returns True on success and FALSE
 * on failure (unable to open prefs file)
 *
 * the prefs structure must be zero'd at creation since
 * this function will free any existing char pointers
 * passed in and will malloc new ones.
 */
extern Bool XFE_ReadPrefs(char * filename, XFE_GlobalPrefs *prefs);

/* saves out the global preferences.
 *
 * returns True on success and FALSE
 * on failure (unable to open prefs file)
 */
extern Bool XFE_SavePrefs(char * filename, XFE_GlobalPrefs *prefs);


/* Set the sorting behavior on the given mail/news context. */
extern void fe_SetMailNewsSortBehavior(MWContext* context, XP_Bool thread,
				       int sortcode);

#endif /* _PREFS_H_ */

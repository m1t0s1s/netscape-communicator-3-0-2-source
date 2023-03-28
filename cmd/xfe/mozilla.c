/* -*- Mode: C; tab-width: 8 -*-
   mozilla.c --- initialization for the X front end.
   Copyright © 1998 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jun-94.
 */

#include "mozilla.h"
#include "name.h"
#include "xfe.h"
#include "net.h"
#include "xp.h"
#include "xp_sec.h"
#include "ssl.h"
#include "np.h"
#include "outline.h"
#include "mailnews.h"
#include "fonts.h"
#include "secnav.h"
#include "secrng.h"
#include "mozjava.h"
#ifdef MOCHA
#include "libmocha.h"
#endif
#ifdef EDITOR
#include "xeditor.h"
#endif
#include "prnetdb.h"
#include "e_kit.h"
#include "hpane/HPaned.h"	/* for horizontal paned widget */

#include "bkmks.h"		/* for drag and drop in mail compose */
#include "icons.h"
#include "icondata.h"

#include "felocale.h"

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>


#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>

#include <X11/keysym.h>

#ifdef __sgi
# include <malloc.h>
#endif

#if defined(AIXV3) || defined(__osf__)
#include <sys/select.h>
#endif /* AIXV3 */

#include <arpa/inet.h>	/* for inet_*() */
#include <netdb.h>	/* for gethostbyname() */
#include <pwd.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <sys/utsname.h>

#include <sys/types.h>
#include <fcntl.h>

#include <sys/errno.h>

#include "xfe-dns.h"

/* DEBUG_JWZ from mozilla 5 */
static void fe_check_use_async_dns(void);
XP_Bool fe_UseAsyncDNS(void);


/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_RESOURCES_NOT_INSTALLED_CORRECTLY;
extern int XFE_USAGE_MSG1;
extern int XFE_USAGE_MSG2;
extern int XFE_USAGE_MSG3;
extern int XFE_VERSION_COMPLAINT_FORMAT;
extern int XFE_INAPPROPRIATE_APPDEFAULT_FILE;
extern int XFE_INVALID_GEOMETRY_SPEC;
extern int XFE_UNRECOGNISED_OPTION;
extern int XFE_APP_HAS_DETECTED_LOCK;
extern int XFE_ANOTHER_USER_IS_RUNNING_APP;
extern int  XFE_APPEARS_TO_BE_RUNNING_ON_HOST_UNDER_PID;
extern int XFE_APPEARS_TO_BE_RUNNING_ON_ANOTHER_HOST_UNDER_PID;
extern int XFE_YOU_MAY_CONTINUE_TO_USE;
extern int XFE_OTHERWISE_CHOOSE_CANCEL;
extern int XFE_EXISTED_BUT_WAS_NOT_A_DIRECTORY;
extern int XFE_EXISTS_BUT_UNABLE_TO_RENAME;
extern int XFE_UNABLE_TO_CREATE_DIRECTORY;
extern int XFE_UNKNOWN_ERROR;
extern int XFE_ERROR_CREATING;
extern int XFE_ERROR_WRITING;
extern int XFE_ERROR_SAVING_OPTIONS;
extern int XFE_CREATE_CONFIG_FILES;
extern int XFE_OLD_FILES_AND_CACHE;
extern int XFE_OLD_FILES;
extern int XFE_THE_MOTIF_KEYSYMS_NOT_DEFINED;
extern int XFE_SOME_MOTIF_KEYSYMS_NOT_DEFINED;
extern int XFE_COLORMAP_WARNING_TO_IGNORE;

#if defined(__sun) && !defined(__svr4__) && (XtSpecificationRelease != 5)
ERROR!!  Must use X11R5 with Motif 1.2 on SunOS 4.1.3!
#endif

/* This means "suppress the splash screen if there is was a URL specified
   on the command line."  With betas, we always want the splash screen to
   be printed to make sure the user knows it's beta. */
#define NON_BETA

const char *fe_progname_long;
const char *fe_progname;
const char *fe_progclass;
XtAppContext fe_XtAppContext;
void (*old_xt_warning_handler) (String nameStr, String typeStr, String classStr,
	String defaultStr, String* params, Cardinal* num_params);
XtErrorHandler old_xt_warningOnly_handler;
Display *fe_display = NULL;
Atom WM_SAVE_YOURSELF; /* For Session Manager */

char *fe_pidlock;

static int
fe_create_pidlock (const char *name, unsigned long *paddr, pid_t *ppid);

#ifdef DEBUG
int Debug;
#endif

/*#ifdef JAVA  jwz */
PRMonitor *fdset_lock;
PRThread *mozilla_thread;
PREventQueue* mozilla_event_queue;
/* #endif / * JAVA */


/* Begin - Session Manager stuff */

int  save_argc = 0;
char **save_argv = NULL;

void fe_wm_save_self_cb(Widget, XtPointer, XtPointer);
static const char* fe_locate_program_path(const char* progname);

/* End - Session Manager stuff */


#ifdef HAVE_SECURITY /* added by jwz */
static void fe_sec_logo_cb (Widget, XtPointer, XtPointer);
#endif /* !HAVE_SECURITY -- added by jwz */
static void fe_expose_thermo_cb (Widget, XtPointer, XtPointer);
static void fe_expose_LED_cb (Widget, XtPointer, XtPointer);
/* static void fe_expose_logo_cb (Widget, XtPointer, XtPointer); */
static void fe_url_text_cb (Widget, XtPointer, XtPointer);
static void fe_url_text_modify_cb (Widget, XtPointer, XtPointer);
#ifdef HAVE_SECURITY
static void fe_expose_security_bar_cb (Widget, XtPointer, XtPointer);
/*static void fe_docinfo_cb (Widget, XtPointer, XtPointer);*/
#endif

#ifndef OLD_UNIX_FILES
static XP_Bool fe_ensure_config_dir_exists (Widget toplevel);
static void fe_copy_init_files (Widget toplevel);
static void fe_clean_old_init_files (Widget toplevel);
#else  /* OLD_UNIX_FILES */
static void fe_rename_init_files (Widget toplevel);
static char *fe_renamed_cache_dir = 0;
#endif /* OLD_UNIX_FILES */

#ifdef HAVE_SECURITY /* added by jwz */
static void fe_read_screen_for_rng (Display *dpy, Screen *screen);
#endif /* !HAVE_SECURITY -- added by jwz */
static int x_fatal_error_handler(Display *dpy);
static int x_error_handler (Display *dpy, XErrorEvent *event);
static void xt_warning_handler(String nameStr, String typeStr, String classStr,
	String defaultStr, String* params, Cardinal* num_params);
static void xt_warningOnly_handler(String msg);

static fe_icon biff_unknown = { 0 };
static fe_icon biff_yes = { 0 };
static fe_icon biff_no = { 0 };

static XrmOptionDescRec options [] = {
  { "-geometry",	".geometry",		   XrmoptionSepArg, 0 },
  { "-visual",		".TopLevelShell.visualID", XrmoptionSepArg, 0 },
  { "-ncols",		".maxImageColors", 	   XrmoptionSepArg, 0 },
  { "-iconic",		".iconic",		   XrmoptionNoArg, "True" },
  { "-install",		".installColormap",	   XrmoptionNoArg, "True" },
  { "-noinstall",	".installColormap",	   XrmoptionNoArg, "False" },
  { "-no-install",	".installColormap",	   XrmoptionNoArg, "False" },
#ifdef USE_NONSHARED_COLORMAPS
  { "-share",		".shareColormap",	   XrmoptionNoArg, "True" },
  { "-noshare",		".shareColormap",	   XrmoptionNoArg, "False" },
  { "-no-share",	".shareColormap",	   XrmoptionNoArg, "False" },
#endif
  { "-mono",		".forceMono",		   XrmoptionNoArg, "True" },
  { "-gravity-works",	".windowGravityWorks",	   XrmoptionNoArg, "True" },
  { "-broken-gravity",	".windowGravityWorks",	   XrmoptionNoArg, "False" },
  { "-xrm",		0,			   XrmoptionResArg, 0 }
};

extern char *fe_fallbackResources[];

XtResource fe_Resources [] =
{
  { "linkForeground", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, link_pixel), XtRString, XtDefaultForeground },
  { "vlinkForeground", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, vlink_pixel), XtRString, XtDefaultForeground },
  { "alinkForeground", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, alink_pixel), XtRString, XtDefaultForeground },

  { "selectForeground", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, select_fg_pixel), XtRString,
    XtDefaultForeground },
  { "selectBackground", XtCBackground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, select_bg_pixel), XtRString,
    XtDefaultBackground },

  { "textBackground", XtCBackground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, text_bg_pixel), XtRString,
    XtDefaultBackground },

  { "defaultForeground", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, default_fg_pixel), XtRString,
    XtDefaultForeground },
  { "defaultBackground", XtCBackground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, default_bg_pixel), XtRString,
    XtDefaultBackground },

  { "defaultBackgroundImage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_ContextData *, default_background_image),
    XtRImmediate, "" },

#ifdef HAVE_SECURITY /* added by jwz */
  { "secureDocumentColor", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, secure_document_pixel), XtRString, "green" },
  { "insecureDocumentColor", XtCForeground, XtRPixel, sizeof (String),
    XtOffset (fe_ContextData *, insecure_document_pixel), XtRString, "red" },
#endif /* HAVE_SECURITY */

  { "linkCursor", XtCCursor, XtRCursor, sizeof (Cursor),
    XtOffset (fe_ContextData *, link_cursor), XtRString, "hand2" },
  { "busyCursor", XtCCursor, XtRCursor, sizeof (Cursor),
    XtOffset (fe_ContextData *, busy_cursor), XtRString, "watch" },

  { "saveNextLinkCursor", XtCCursor, XtRCursor, sizeof (Cursor),
    XtOffset (fe_ContextData *, save_next_link_cursor), XtRString, "hand2" },
  { "saveNextNonlinkCursor", XtCCursor, XtRCursor, sizeof (Cursor),
    XtOffset (fe_ContextData *, save_next_nonlink_cursor),
    XtRString, "crosshair" },
#ifdef EDITOR
  { "editableTextCursor", XtCCursor, XtRCursor, sizeof (Cursor),
    XtOffset (fe_ContextData *, editable_text_cursor),
    XtRString, "xterm" },
#endif

  { "confirmExit", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_ContextData *, confirm_exit_p), XtRImmediate,
    (XtPointer) True },

  { "progressInterval", "Seconds", XtRCardinal, sizeof (int),
    XtOffset (fe_ContextData *, progress_interval),
    XtRImmediate, (XtPointer) 1 },
  { "busyBlinkRate", "Microseconds", XtRCardinal, sizeof (int),
    XtOffset (fe_ContextData *, busy_blink_rate),
    XtRImmediate, (XtPointer) 500 },
  { "animRate", "Milliseconds", XtRCardinal, sizeof (int),
    XtOffset (fe_ContextData *, anim_rate), XtRImmediate, (XtPointer) 50 },
  { "hysteresis", "Pixels", XtRCardinal, sizeof (int),
    XtOffset (fe_ContextData *, hysteresis), XtRImmediate, (XtPointer) 5 },
  { "blinkingEnabled", "BlinkingEnabled", XtRBoolean, sizeof (Boolean),
    XtOffset (fe_ContextData *, blinking_enabled_p), XtRImmediate,
    (XtPointer) True }
};
Cardinal fe_ResourcesSize = XtNumber (fe_Resources);

XtResource fe_GlobalResources [] =
{
  { "encodingFilters", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, encoding_filters), XtRImmediate, "" },

  { "saveHistoryInterval", "Seconds", XtRCardinal, sizeof (int),
    XtOffset (fe_GlobalData *, save_history_interval), XtRImmediate,
    (XtPointer) 600 },

  { "terminalTextTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, terminal_text_translations),
    XtRImmediate, 0 },
  { "nonterminalTextTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, nonterminal_text_translations),
    XtRImmediate, 0 },

  { "globalTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, global_translations),
    XtRImmediate, 0 },
  { "globalTextFieldTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, global_text_field_translations),
    XtRImmediate, 0 },
  { "globalNonTextTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, global_nontext_translations),
    XtRImmediate, 0 },

  { "editingTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, editing_translations),
    XtRImmediate, 0 },
  { "singleLineEditingTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, single_line_editing_translations),
    XtRImmediate, 0 },
  { "multiLineEditingTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, multi_line_editing_translations),
    XtRImmediate, 0 },

  { "bmGlobalTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, bm_global_translations),
    XtRImmediate, 0 },
  { "mailnewsGlobalTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, mailnews_global_translations),
    XtRImmediate, 0 },
  { "mailcomposeGlobalTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, mailcompose_global_translations),
    XtRImmediate, 0 },
  { "dialogGlobalTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, dialog_global_translations),
    XtRImmediate, 0 },
  { "editorTranslations", XtCTranslations,
    XtRTranslationTable, sizeof (XtTranslations),
    XtOffset (fe_GlobalData *, editor_global_translations),
    XtRImmediate, 0 },

  { "forceMono", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, force_mono_p), XtRImmediate,
    (XtPointer) False },
  { "wmIconPolicy", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, wm_icon_policy), XtRImmediate,
    (XtPointer) NULL },
#ifdef USE_NONSHARED_COLORMAPS
  { "shareColormap", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, windows_share_cmap), XtRImmediate,
    (XtPointer) False },
#endif
  { "maxImageColors", "Integer", XtRCardinal, sizeof (int),
    XtOffset (fe_GlobalData *, max_image_colors), XtRImmediate,
    (XtPointer) 0 },

#ifdef __sgi
  { "sgiMode", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, sgi_mode_p), XtRImmediate,
    (XtPointer) False },
#endif /* __sgi */

  { "useStderrDialog", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, stderr_dialog_p), XtRImmediate,
    (XtPointer) True },
  { "useStdoutDialog", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, stdout_dialog_p), XtRImmediate,
    (XtPointer) True },

  /* #### move to prefs */
  { "documentColorsHavePriority", XtCBoolean, XtRBoolean, sizeof (Boolean),
    XtOffset (fe_GlobalData *, document_beats_user_p), XtRImmediate,
    (XtPointer) True },

# define RES_ERROR "ERROR: Resources not installed correctly!"
  { "noDocumentLoadedMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_url_loaded_message),
    XtRImmediate, RES_ERROR },
  { "optionsSavedMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, options_saved_message),
    XtRImmediate, RES_ERROR },
  { "clickToSaveMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, click_to_save_message),
    XtRImmediate, RES_ERROR },
  { "clickToSaveCancelledMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, click_to_save_cancelled_message),
    XtRImmediate, RES_ERROR },
  { "noNextURLMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_next_url_message),
    XtRImmediate, RES_ERROR },
  { "noPreviousURLMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_previous_url_message),
    XtRImmediate, RES_ERROR },
  { "noHomeURLMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_home_url_message),
    XtRImmediate, RES_ERROR },
  { "notOverImageMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, not_over_image_message),
    XtRImmediate, RES_ERROR },
  { "notOverLinkMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, not_over_link_message),
    XtRImmediate, RES_ERROR },
  { "noSearchStringMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_search_string_message),
    XtRImmediate, RES_ERROR },
  { "wrapSearchMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, wrap_search_message),
    XtRImmediate, RES_ERROR },
  { "wrapSearchBackwardMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, wrap_search_backward_message),
    XtRImmediate, RES_ERROR },
  { "noAddressesMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_addresses_message),
    XtRImmediate, RES_ERROR },
  { "noFileMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_file_message),
    XtRImmediate, RES_ERROR },
  { "noPrintCommandMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, no_print_command_message),
    XtRImmediate, RES_ERROR },
  { "bookmarksChangedMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, bookmarks_changed_message),
    XtRImmediate, RES_ERROR },
  { "bookmarkConflictMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, bookmark_conflict_message),
    XtRImmediate, RES_ERROR },
  { "bookmarksNoFormsMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, bookmarks_no_forms_message),
    XtRImmediate, RES_ERROR },
  { "reallyQuitMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, really_quit_message),
    XtRImmediate, RES_ERROR },
  { "doubleInclusionMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, double_inclusion_message),
    XtRImmediate, RES_ERROR },
  { "expireNowMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, expire_now_message),
    XtRImmediate, RES_ERROR },
  { "clearMemCacheMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, clear_mem_cache_message),
    XtRImmediate, RES_ERROR },
  { "clearDiskCacheMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, clear_disk_cache_message),
    XtRImmediate, RES_ERROR },
  { "createCacheDirErrorMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, create_cache_dir_message),
    XtRImmediate, RES_ERROR },
  { "createdCacheDirMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, created_cache_dir_message),
    XtRImmediate, RES_ERROR },
  { "cacheNotDirMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, cache_not_dir_message),
    XtRImmediate, RES_ERROR },
  { "cacheSuffixMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, cache_suffix_message),
    XtRImmediate, RES_ERROR },
  { "cubeTooSmallMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, cube_too_small_message),
    XtRImmediate, RES_ERROR },
  { "renameInitFilesMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, rename_files_message),
    XtRImmediate, RES_ERROR },
  { "overwriteFileMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, overwrite_file_message),
    XtRImmediate, RES_ERROR },
  { "unsentMailMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, unsent_mail_message),
    XtRImmediate, RES_ERROR },
  { "binaryDocumentMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, binary_document_message),
    XtRImmediate, RES_ERROR },
  { "defaultCharset", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, default_url_charset),
    XtRImmediate, "" },
  { "emptyMessageQuestion", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, empty_message_message),
    XtRImmediate, RES_ERROR },
  { "defaultMailtoText", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, default_mailto_text),
    XtRImmediate, "" },
  { "helperAppDeleteMessage", XtCString, XtRString, sizeof (String),
    XtOffset (fe_GlobalData *, helper_app_delete_message),
    XtRImmediate, "" }
# undef RES_ERROR
};
Cardinal fe_GlobalResourcesSize = XtNumber (fe_GlobalResources);

fe_GlobalData   fe_globalData   = { 0, };
XFE_GlobalPrefs fe_globalPrefs  = { 0, };
XFE_GlobalPrefs fe_defaultPrefs = { 0, };


static void
usage (void)
{
  fprintf (stderr, XP_GetString( XFE_USAGE_MSG1 ), fe_long_version + 4,
  				fe_progname);

#ifdef USE_NONSHARED_COLORMAPS
  fprintf (stderr,  XP_GetString( XFE_USAGE_MSG2 ) );
#endif

  fprintf (stderr, XP_GetString( XFE_USAGE_MSG3 ) );
}

/*******************
 * Signal handlers *
 *******************/
void fe_sigchild_handler(int signo)
{
  pid_t pid;
  int status = 0;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
#ifdef DEBUG_dp
    fprintf(stderr, "fe_sigchild_handler: Reaped pid %d.\n", pid);
#endif
  }
}

/* Netlib re-enters itself sometimes, so this has to be a counter not a flag */
static int fe_netlib_hungry_p = 0;

void
XFE_SetCallNetlibAllTheTime (MWContext * context)
{
  fe_netlib_hungry_p++;
}

void
XFE_ClearCallNetlibAllTheTime (MWContext * context)
{
  --fe_netlib_hungry_p;
  XP_ASSERT(fe_netlib_hungry_p >= 0);
}

static void  fe_EventForRNG (XEvent *event);
/* Use a static so that the difference between the running X clock
   and the time that we are about to block is a function of the
   amount of time the last processing took.
 */
static XEvent fe_last_event;


/* Process events. The idea here is to give X events priority over
   file descriptor input so that the user gets better interactive
   response under heavy i/o load.

   While we are it, we try to gather up a bunch of events to shovel
   into the security library to "improve" randomness.

   Finally, we try to block when there is really nothing to do,
   but we block in such a way as to allow java applets to get the
   cpu while we are blocked.

   Simple, eh? If you don't understand it, don't touch it. Instead
   find kipp.
   */
void
fe_EventLoop ()
{
  XtInputMask pending;
  static unsigned long device_event_count = 0;
  XP_Bool haveEvent;		/* Used only in debugging sessions. */
#ifdef DEBUG
  int first_time = 1;
  static int entry_depth = 0;
  static int spinning_wildly = 0;

  entry_depth++;
#endif

  /* Feed the security library the last event we handled. We do this here
     before peeking for another event so that the elapsed time between calls
     can have some impact on the outcome.
     */
  fe_EventForRNG (&fe_last_event);

  /* Release X lock and wait for an event to show up. This might not return
     an X event, and it will not block until there is an X event. However,
     it will block (i.e. call select) if there is no X events, timer events
     or i/o events immediately pending. Which is what we want it to do.

     XtAppPeekEvent promises to return after something is ready:
     i/o, x event or xt timer. It returns a value that indicates
     if the event structure passed to it gets filled in (we don't care
     so we ignore the value).
     */
#ifdef JAVA /*  jwz */
  PR_XUnlock();
#endif


  haveEvent = XtAppPeekEvent(fe_XtAppContext, &fe_last_event);

#ifdef JAVA /*  jwz */
  PR_XLock();
#endif


  /* Consume all the X events and timer events right now. As long as
     there are X events or Xt timer events, we will sit and spin here.
     As soon as those are exhausted we fall out of the loop.
     */
  for (;;) {
      /* Get a mask of the pending events, timers and i/o streams that
	 are ready for processing. This event will not block, but instead
	 will return 0 when there is nothing pending remaining.
	 */
      pending = XtAppPending(fe_XtAppContext);
      if (pending == 0) {
#ifdef DEBUG
	  /* Nothing to do anymore. Time to leave. We ought to be able to
	     assert that first_time is 0; that is, that we have
	     processed at least one event before breaking out of the loop.
	     Such an assert would help us detect busted Xt libraries.  However,
	     in reality, we have already determined that we do have such busted
	     libraries.  So, we'll assert that this doesn't happen more than
	     once in a row, so that we know we don't really spin wildly.
	     */
	  if (first_time) {
#ifndef I_KNOW_MY_Xt_IS_BAD
	      XP_ASSERT(spinning_wildly < 3);
#endif /* I_KNOW_MY_Xt_IS_BAD */
	      spinning_wildly++;
	  }
#endif
	  break;
      }
#ifdef DEBUG
      /* Now we have one event. It's no longer our first time */
      first_time = 0;
      spinning_wildly = 0;
#endif
      if (pending & XtIMXEvent) {
	  /* We have an X event to process. We check them first because
	     we want to be as responsive to the user as possible.
	     */
	  if (device_event_count < 300) {
	      /* While the security library is still hungry for events
		 we feed it all the events we get. We use XtAppPeekEvent
		 to grab a copy of the event that XtAppPending said was
		 available.
		 */
	      XtAppPeekEvent(fe_XtAppContext, &fe_last_event);
	      fe_EventForRNG (&fe_last_event);

	      /* If it's an interesting event, count it towards our
		 goal of 300 events. After 300 events, we don't need
		 to feed the security library as often (which makes
		 us more efficient at event processing)
		 */
	      if (fe_last_event.xany.type == ButtonPress ||
		  fe_last_event.xany.type == ButtonRelease ||
		  fe_last_event.xany.type == KeyPress ||
		  fe_last_event.xany.type == MotionNotify)
	      {
		  device_event_count++;
	      }
	  }
	  /* Feed the X event to Xt, being careful to not allow it to
	     process timers or i/o. */
	  XtAppProcessEvent(fe_XtAppContext, XtIMXEvent);
      } else if (pending & XtIMTimer) {
	  /* Let Xt dispatch the Xt timer event. For this case we fall out
	     of the loop in case a very fast timer was set.
	     */
	  XtAppProcessEvent(fe_XtAppContext, XtIMTimer);
	  break;
      } else {
	  /* Go ahead and process the i/o event XtAppPending has
	     reported. However, as soon as it is done fall out of
	     the loop. This way we return to the caller after
	     doing something.
	     */
	  XtAppProcessEvent(fe_XtAppContext, pending);

#ifdef __sgi
	  /*
	   * The SGI version of Xt has a bug where XtAppPending will mark
	   * an internal data structure again when it sees input pending on
	   * the Asian input method socket, even though XtAppProcessEvent
	   * has already put a dummy X event with zero keycode on the X
	   * event queue, which is supposed to tell Xlib to read the IM
	   * socket (when XFilterEvent is called by Xt).
	   *
	   * So we process the dummy event immediately after it has been
	   * put on the queue, so that XtAppPending doesn't mark its data
	   * structure again.
	   */
	  while (XPending(fe_display)) {
	      XtAppProcessEvent(fe_XtAppContext, XtIMXEvent);
	  }
#endif /* __sgi */

	  break;
      }
  }

  /* If netlib is anxious, service it after every event, and service it
     continuously while there are no events. */
  if (fe_netlib_hungry_p)
    do
    {
#ifdef QUANTIFY
quantify_start_recording_data();
#endif /* QUANTIFY */
      NET_ProcessNet (-1, NET_EVERYTIME_TYPE);
#ifdef QUANTIFY
quantify_stop_recording_data();
#endif /* QUANTIFY */
    }
    while (fe_netlib_hungry_p && !XtAppPending (fe_XtAppContext));
#ifdef DEBUG
  entry_depth--;
#endif
}

static void
fe_EventForRNG (XEvent *event)
{
  struct
  {
    XEvent event;
    unsigned char noise[16];
  } data;

  data.event = *event;

#ifdef HAVE_SECURITY /* added by jwz */
  /* At this point, we are idle.  Get a high-res clock value. */
  (void) RNG_GetNoise(data.noise, sizeof(data.noise));

  /* Kick security library random number generator to make it very
     hard for a bad guy to predict where the random number generator
     is at.  Initialize it with the current time, and the *previous*
     X event we read (which happens to have a server timestamp in it.)
     The X event came from before this current idle period began, and
     will be uninitialized stack data the first time through.
   */
  RNG_RandomUpdate(&data, sizeof(data));
#endif /* !HAVE_SECURITY -- added by jwz */
}


static void
fe_splash_timer (XtPointer closure, XtIntervalId *id)
{
  Boolean *flag = (Boolean *) closure;
  *flag = True;
}

char *sploosh = 0;
static Boolean plonkp = False;

Boolean
plonk (MWContext *context)
{
  int flags = 0;
  int win = False;
  Boolean timed_out = False;
  XtIntervalId id;
  Display *dpy = XtDisplay (CONTEXT_WIDGET (context));
  char *ab = "\246\247\264\272\271\177\270\265\261\246\270\255";
  char *au ="\246\247\264\272\271\177\246\272\271\255\264\267\270";
  char *blank = "\246\247\264\272\271\177\247\261\246\263\260";
  char *tmp;

  if (context->type != MWContextBrowser &&
	context->type != MWContextMail &&
	context->type != MWContextNews) return False;

#ifdef DEBUG_jwz
  plonkp = True; /* I'm tired of looking at that splash screen */
#endif


  if (plonkp)
    return False;
  plonkp = True;
  if (sploosh) free (sploosh);
  sploosh = strdup (ab);
  for (tmp = sploosh; *tmp; tmp++) *tmp -= 69;
  fe_GetURL (context, NET_CreateURLStruct (sploosh, FALSE), FALSE);

  id = XtAppAddTimeOut (fe_XtAppContext, 60 * 1000, fe_splash_timer,
			&timed_out);

  /* #### Danger, this is weird and duplicates much of the work done
     in fe_EventLoop ().
   */
  while (! timed_out)
    {
      XtInputMask pending = XtAppPending (fe_XtAppContext);

      /* AAAAAUUUUGGHHH!!!  Sleep for a second, or until an X event arrives,
	 and then loop until some input of any type has arrived.   The Xt event
	 model completely sucks and I'm tired of trying to figure out how to
	 implement this without polling, so fuck it, and fuck you too.
       */
      while (! pending)
	{
	  fd_set fds;
	  int fd = ConnectionNumber (dpy);
	  int usecs = 1000000L;
	  struct timeval tv;
	  tv.tv_sec  = usecs / 1000000L;
	  tv.tv_usec = usecs % 1000000L;
	  memset(&fds, 0, sizeof(fds));
	  FD_SET (fd, &fds);
	  (void) select (fd + 1, &fds, 0, 0, &tv);
	  pending = XtAppPending (fe_XtAppContext);
	}

      if (pending & XtIMXEvent)
	do
	  {
	    int nf = 0;
	    XEvent event;
	    XtAppNextEvent (fe_XtAppContext, &event);

	    if (event.xany.type == KeyPress /* ||
		event.xany.type == KeyRelease */)
	      {
		KeySym s = XKeycodeToKeysym (dpy, event.xkey.keycode, 0);
		switch (s)
		  {
		  case XK_Alt_L:     nf = 1<<0; break;
		  case XK_Meta_L:    nf = 1<<0; break;
		  case XK_Alt_R:     nf = 1<<1; break;
		  case XK_Meta_R:    nf = 1<<1; break;
		  case XK_Control_L: nf = 1<<2; break;
		  case XK_Control_R: nf = 1<<3; break;
		  }

		if (event.xany.type == KeyPress)
		  flags |= nf;
		else
		  flags &= ~nf;
		win = (flags == 15 || flags == 7 || flags == 11);
		if (nf && !win)
		  event.xany.type = 0;
	      }

	    if (!timed_out &&
		(event.xany.type == KeyPress ||
		 event.xany.type == ButtonPress))
	      {
		XtRemoveTimeOut (id);
		timed_out = True;
		id = 0;
	      }
	    if (event.xany.type)
	      XtDispatchEvent (&event);
	    pending = XtAppPending (fe_XtAppContext);
	  }
	while ((pending & XtIMXEvent) && !timed_out);

      if ((pending & (~XtIMXEvent)) && !timed_out)
	do
	  {
	    XtAppProcessEvent (fe_XtAppContext, (~XtIMXEvent));
	    pending = XtAppPending (fe_XtAppContext);
	  }
	while ((pending & (~XtIMXEvent)) && !timed_out);

      /* If netlib is anxious, service it after every event, and service it
	 continuously while there are no events. */
      if (fe_netlib_hungry_p && !timed_out)
	do
	  {
#ifdef QUANTIFY
quantify_start_recording_data();
#endif /* QUANTIFY */
	    NET_ProcessNet (-1, NET_EVERYTIME_TYPE);
#ifdef QUANTIFY
quantify_stop_recording_data();
#endif /* QUANTIFY */
	  }
	while (fe_netlib_hungry_p && !XtAppPending (fe_XtAppContext)
	       && !timed_out);
    }

  if (sploosh) {
    free (sploosh);
    sploosh = 0;
  }

  /* We timed out. But the context might have been destroyed by now
     if the user killed it with the window manager WM_DELETE. So we
     validate the context. */
  if (!fe_contextIsValid(context)) return True;

  if (SHIST_GetCurrent (&context->hist))
    /* If there's a current URL, then the remote control code must have
       kicked in already.  Don't clear it all. */
    return 1;

  sploosh = strdup (win ? au : blank);
  for (tmp = sploosh; *tmp; tmp++) *tmp -= 69;
  fe_GetURL (context, NET_CreateURLStruct (sploosh, FALSE), FALSE);
  return win;
}


/* #define NEW_VERSION_CHECKING */


#ifdef NEW_VERSION_CHECKING
static Bool fe_version_checker (XrmDatabase *db,
				XrmBindingList bindings,
				XrmQuarkList quarks,
				XrmRepresentation *type,
				XrmValue *value,
				XPointer closure);
#endif /* NEW_VERSION_CHECKING */

static void
fe_sanity_check_resources (Widget toplevel)
{
  XrmDatabase db = XtDatabase (XtDisplay (toplevel));
  XrmValue value;
  char *type;
  char full_name [2048], full_class [2048];
  int count;
  PR_snprintf (full_name, sizeof (full_name), "%s.version", fe_progname);
  PR_snprintf (full_class, sizeof (full_class), "%s.version", fe_progclass);

#ifdef NEW_VERSION_CHECKING

  if (XrmGetResource (db, full_name, full_class, &type, &value) == True)
    {
      /* We have found the resource "Netscape.version: N.N".
	 Versions past 1.1b3 does not use that, so that must
	 mean an older resource db is being picked up.
	 Complain, and refuse to run until this is fixed.
       */
      char str [1024];
      strncpy (str, (char *) value.addr, value.size);
      str [value.size] = 0;

#define VERSION_COMPLAINT_FORMAT XP_GetString( XFE_VERSION_COMPLAINT_FORMAT )

      fprintf (stderr, VERSION_COMPLAINT_FORMAT,
	       fe_progname, fe_version, str, fe_progclass);
      exit (-1);
    }

  {
    const char *s1;
    char *s2;
    XrmName name_list [100];
    XrmClass class_list [100];
    char clean_version [255];

    /* Set clean_version to be a quark-safe version of fe_version,
       ie, "1.1b3N" ==> "v11b3N". */
    s2 = clean_version;
    *s2++ = 'v';
    for (s1 = fe_version; *s1; s1++)
      if (*s1 != ' ' && *s1 != '\t' && *s1 != '.' && *s1 != '*' && !s1 != '?')
	*s2++ = *s1;
    *s2 = 0;

    XrmStringToNameList (full_name, name_list);
    XrmStringToClassList (full_class, class_list);

    XrmEnumerateDatabase (db, name_list, class_list, XrmEnumOneLevel,
			  fe_version_checker, (XtPointer) clean_version);

    /* If we've made it to here, then our iteration over the database
       has not turned up any suspicious "version" resources.  So now
       look for the version tag to see if we have any resources at all.
     */
    PR_snprintf (full_name, sizeof (full_name), 
		    "%s.version.%s", fe_progname, clean_version);
    PR_snprintf (full_class, sizeof (full_class),
		    "%s.version.%s", fe_progclass, clean_version);
    XrmStringToNameList (full_name, name_list);
    XrmStringToClassList (full_class, class_list);
    if (XrmGetResource (db, full_name, full_class, &type, &value) != True)
      {
	/* We have not found "Netscape.version.WRONG: WRONG", but neither
	   have we found "Netscape.version.RIGHT: RIGHT".  Die.
	 */
	fprintf (stderr, XP_GetString( XFE_INAPPROPRIATE_APPDEFAULT_FILE ),
	       fe_progname, fe_progclass);
	exit (-1);
      }
  }

#else /* !NEW_VERSION_CHECKING */

  if (XrmGetResource (db, full_name, full_class, &type, &value) == True)
    {
      char str [1024];
      strncpy (str, (char *) value.addr, value.size);
      str [value.size] = 0;
      if (strcmp (str, fe_version))
	{
	  fprintf (stderr,
		 "%s: program is version %s, but resources are version %s.\n\n"
	"\tThis means that there is an inappropriate `%s' file installed\n"
	"\tin the app-defaults directory.  Check these environment variables\n"
	"\tand the directories to which they point:\n\n"
	"	$XAPPLRESDIR\n"
	"	$XFILESEARCHPATH\n"
	"	$XUSERFILESEARCHPATH\n\n"
	"\tAlso check for this file in your home directory, or in the\n"
	"\tdirectory called `app-defaults' somewhere under /usr/lib/.",
		   fe_progname, fe_version, str, fe_progclass);
	  exit (-1);
	}
    }
  else
    {
      fprintf (stderr, "%s: couldn't find our resources?\n\n"
	"\tThis could mean that there is an inappropriate `%s' file\n"
	"\tinstalled in the app-defaults directory.  Check these environment\n"
	"\tvariables and the directories to which they point:\n\n"
	"	$XAPPLRESDIR\n"
	"	$XFILESEARCHPATH\n"
	"	$XUSERFILESEARCHPATH\n\n"
	"\tAlso check for this file in your home directory, or in the\n"
	"\tdirectory called `app-defaults' somewhere under /usr/lib/.",
	       fe_progname, fe_progclass);
      exit (-1);
    }
#endif /* !NEW_VERSION_CHECKING */

  /* Now sanity-check the geometry resource. */
  PR_snprintf (full_name, sizeof (full_name),
		"%s._no_._such_._resource_.geometry", fe_progname);
  PR_snprintf (full_class, sizeof (full_class),
		"%s._no_._such_._resource_.Geometry", fe_progclass);
  if (XrmGetResource (db, full_name, full_class, &type, &value) == True)
    {
      fprintf (stderr, XP_GetString( XFE_INVALID_GEOMETRY_SPEC ),
	       fe_progname,
	       fe_progname, (char *) value.addr,
	       fe_progclass, (char *) value.addr,
	       fe_progclass);
      exit (-1);
    }

  /* Now sanity-check the XKeysymDB.  You fuckers. */
  count = 0;
  if (XStringToKeysym ("osfActivate")  == 0) count++;	/* 1 */
  if (XStringToKeysym ("osfCancel")    == 0) count++;	/* 2 */
  if (XStringToKeysym ("osfPageLeft")  == 0) count++;	/* 3 */
  if (XStringToKeysym ("osfLeft")      == 0) count++;	/* 4 */
  if (XStringToKeysym ("osfPageUp")    == 0) count++;	/* 5 */
  if (XStringToKeysym ("osfUp")        == 0) count++;	/* 6 */
  if (XStringToKeysym ("osfBackSpace") == 0) count++;	/* 7 */
  if (count > 0)
    {
		char *str;

		if ( count == 7 )
			str = XP_GetString( XFE_THE_MOTIF_KEYSYMS_NOT_DEFINED );
		else
			str = XP_GetString( XFE_SOME_MOTIF_KEYSYMS_NOT_DEFINED );

      fprintf (stderr, str, fe_progname, XP_AppName );
      /* Don't exit for this one, just limp along. */
    }
}


#ifdef NEW_VERSION_CHECKING
static Bool
fe_version_checker (XrmDatabase *db,
		    XrmBindingList bindings,
		    XrmQuarkList quarks,
		    XrmRepresentation *type,
		    XrmValue *value,
		    XPointer closure)
{
  char *name [100];
  int i = 0;
  char *desired_tail = (char *) closure;
  assert (quarks);
  assert (desired_tail);
  if (!quarks || !desired_tail)
    return False;
  while (quarks[i])
    name [i++] = XrmQuarkToString (quarks[i]);

  if (i != 3 ||
      !!strcmp (name[0], fe_progclass) ||
      !!strcmp (name[1], "version") ||
      name[2][0] != 'v')
    /* this is junk... */
    return False;

  if (value->addr &&
      !strncmp (fe_version, (char *) value->addr, value->size) &&
      i > 0 && !strcmp (desired_tail, name [i-1]))
    {
      /* The resource we have found is the one we want/expect.  That's ok. */
    }
  else
    {
      /* complain. */
      char str [1024];
      strncpy (str, (char *) value->addr, value->size);
      str [value->size] = 0;
      fprintf (stderr, VERSION_COMPLAINT_FORMAT,
	       fe_progname, fe_version, str, fe_progclass);
      exit (-1);
    }

  /* Always return false, to continue iterating. */
  return False;
}
#endif /* NEW_VERSION_CHECKING */




#ifdef __linux
 /* On Linux, Xt seems to prints 
       "Warning: locale not supported by C library, locale unchanged"
    no matter what $LANG is set to, even if it's "C" or undefined.

    This is because `setlocale(LC_ALL, "")' always returns 0, while
    `setlocale(LC_ALL, NULL)' returns "C".  I think the former is
    incorrect behavior, even if the intent is to only support one
    locale, but that's how it is.

    The Linux 1.1.59 man page for setlocale(3) says

       At the moment, the only supported locales are the portable
       "C" and "POSIX" locales, which makes  calling  setlocale()
       almost  useless.	Support for other locales will be there
       Real Soon Now.

    Yeah, whatever.  So until there is real locale support, we might
    as well not try and initialize Xlib to use it.

    The reason that defining `BROKEN_I18N' does not stub out all of
    `lose_internationally()' is that we still need to cope with
    `BROKEN_I18N'.  By calling `setlocale (LC_CTYPE, NULL)' we can
    tell whether the "C" locale is defined, and therefore whether
    the `nls' dir exists, and therefore whether Xlib will dump core
    the first time the user tries to paste.
 */
# define BROKEN_I18N
# define NLS_LOSSAGE
#endif /* __linux */

#if defined(__sun) && !defined(__svr4__)	/* SunOS 4.1.3 */
 /* All R5-based systems have the problem that if the `nls' directory
    doesn't exist, trying to cut or paste causes a core dump in Xlib.
    This controls whether we try to generate a warning about this.
  */
# define NLS_LOSSAGE
#endif /* SunOS 4.1.3 */

#ifdef __386BSD__				/* BSDI is also R5 */
# define NLS_LOSSAGE
#endif /* BSDI */

static void
lose_internationally (void)
{
  char *locale;
  Boolean losing, was_default;

/*
 * For whatever reason, Japanese titles don't work unless you call
 * XtSetLanguageProc on DEC. We call XtAppInitialize later, which
 * calls setlocale. (See the XtAppInitialize call for more
 * comments.) -- erik
 */
#ifdef __osf__
  XtSetLanguageProc(NULL, NULL, NULL);
#else /* __osf__ */
  setlocale (LC_CTYPE, "");
  setlocale (LC_TIME,  "");
  setlocale (LC_COLLATE, "");
  fe_InitCollation();
#endif /* __osf__ */

  losing = !XSupportsLocale();
  locale = setlocale (LC_CTYPE, NULL);
  if (! locale) locale = "C";

  was_default = !strcmp ("C", locale);

  if (losing && !was_default)
    {
      fprintf (stderr, "%s: locale `%s' not supported by Xlib; trying `C'.\n",
	       fe_progname, locale);
      setlocale (LC_CTYPE, "C");
      putenv ("LC_CTYPE=C");
      losing = !XSupportsLocale();
    }

  if (losing)
    {
      fprintf (stderr, (was_default
			? "%s: locale `C' not supported.\n"
			: "%s: locale `C' not supported either.\n"),
	       fe_progname);

      fprintf (stderr,
#ifdef NLS_LOSSAGE
    "\n"
    "	If the $XNLSPATH directory does not contain the proper config files,\n"
    "	%s will crash the first time you try to paste into a text\n"
    "	field.  (This is a bug in the X11R5 libraries against which this\n"
    "	program was linked.)\n"
    "\n"
    "	Since neither X11R4 nor X11R6 come with these config files, we have\n"
    "	included them with the %s distribution.  The normal place\n"
    "	for these files is "
# if defined (__linux)
	       "/usr/X386/lib/X11/nls/"
# elif defined (__386BSD__)
	       "/usr/X11/lib/X11/nls/"
# else /* SunOS 4.1.3, and a guess at anything else... */
	       "/usr/lib/X11/nls/"
# endif
	       ".  If you can't create that\n"
    "	directory, you should set the $XNLSPATH environment variable to\n"
    "	point at the place where you installed the files.\n"
    "\n",
	       XP_AppName, XP_AppName

#else  /* !NLS_LOSSAGE */

    "	Perhaps the $XNLSPATH environment variable is not set correctly?\n"

#endif /* !NLS_LOSSAGE */
	       );
    }

#ifdef BROKEN_I18N
  losing = True;
#endif /* BROKEN_I18N */

  /* If we're in a losing state, calling these will cause Xlib to exit.
     It's arguably better to try and limp along. */
  if (! losing)
    {
      /*
      XSetLocaleModifiers (NULL);
      */
      XSetLocaleModifiers ("");
      /*
      XtSetLanguageProc (NULL, NULL, NULL);
      */
    }
}


static char *fe_accept_language = NULL;

char *
FE_GetAcceptLanguage(void)
{
	return fe_accept_language;
}


static void
fe_fix_fds (void)
{
  /* Bad Things Happen if stdin, stdout, and stderr have been closed
     (as by the `sh' incantation "netscape 1>&- 2>&-").  I think what
     happens is, the X connection gets allocated to one of these fds,
     and then some random library writes to stderr, and the connection
     gets hosed.  I think this must be a general problem with X programs,
     but I'm not sure.  Anyway, cause them to be open to /dev/null if
     they aren't closed.  This must be done before any other files are
     opened.

     We do this by opening /dev/null three times, and then closing those
     fds, *unless* any of them got allocated as #0, #1, or #2, in which
     case we leave them open.  Gag.
   */
  int fd0 = open ("/dev/null", O_RDWR);
  int fd1 = open ("/dev/null", O_RDWR);
  int fd2 = open ("/dev/null", O_RDWR);
  if (fd0 > 2U) close (fd0);
  if (fd1 > 2U) close (fd1);
  if (fd2 > 2U) close (fd2);
}


extern char **environ;

static void
fe_fix_environment (Display *dpy)
{
  char buf [1024];
  static Boolean env_hacked_p = False;

  if (env_hacked_p)
    return;
  env_hacked_p = True;

  /* Store $DISPLAY into the environment, so that the $DISPLAY variable that
     the spawned processes inherit is the same as the value of -display passed
     in on our command line (which is not necessarily the same as what our
     $DISPLAY variable is.)
   */
  PR_snprintf (buf, sizeof (buf), "DISPLAY=%.900s", DisplayString (dpy));
  if (putenv (strdup (buf)))
    abort ();
}

static void
fe_hack_uid(void)
{
  /* If we've been run as setuid or setgid to someone else (most likely root)
     turn off the extra permissions.  Nobody ought to be installing Mozilla
     as setuid in the first place, but let's be extra special careful...
     Someone might get stupid because of movemail or something.
  */
  setgid (getgid ());
  setuid (getuid ());

  /* Is there anything special we should do if running as root?
     Features we should disable...?
     if (getuid () == 0) ...
   */
}

#ifdef JAVA
static void fe_java_callback (XtPointer closure, int *fd, XtIntervalId *id)
{
#ifdef JAVA  /* jwz */
    XP_ASSERT(*fd == PR_GetEventQueueSelectFD(mozilla_event_queue));
    LJ_ProcessEvent();
#endif
}

void FE_TrackJavaConsole(int on, void *notused)
{
    struct fe_MWContext_cons *cl = fe_all_MWContexts;
    Widget widget;
    while (cl) {
	MWContext *cx = cl->context;
	CONTEXT_DATA(cx)->show_java_console_p = on;
	if ((widget = CONTEXT_DATA(cx)->show_java) != NULL) {
	    XtVaSetValues(widget, XmNset, on, 0);
	}
	cl = cl->next;
    }
}
#endif /* JAVA */

static void fe_late_java_init(void)
{
#ifdef JAVA /* jwz */
  LOCK_FDSET();
  XtAppAddInput (fe_XtAppContext, PR_GetEventQueueSelectFD(mozilla_event_queue),
		 (XtPointer) (XtInputReadMask),
		 fe_java_callback, 0);
  UNLOCK_FDSET();
  LJ_SetConsoleShowCallback(FE_TrackJavaConsole, 0);
#endif /* JAVA */
}

static Boolean fe_command_line_done = False;

Display *fe_dpy_kludge;
Screen *fe_screen_kludge;

static char *fe_home_dir;
static char *fe_config_dir;

char *XP_AppLanguage = 0;

/*
 * build_simple_user_agent_string
 */
static void
build_simple_user_agent_string(char *versionLocale)
{
    char buf [1024];
    int totlen;

    if (XP_AppVersion) {
	free((void *)XP_AppVersion);
	XP_AppVersion = NULL;
    }

    /* SECNAV_Init must have a language string in XP_AppVersion.
     * If missing, it is supplied here.  This string is very short lived.
     * After SECNAV_Init returns, build_user_agent_string() will build the 
     * string that is expected by existing servers.  
     */
    if (!versionLocale || !*versionLocale) {	
	versionLocale = " [en]";   /* default is english for SECNAV_Init() */
    }

    /* be safe about running past end of buf */
    totlen = sizeof buf;
    memset(buf, 0, totlen);
#if 0
    strncpy(buf, fe_version, totlen - 1);
#else
    /* #### KLUDGE!  Impersonate 4.0b4 so that the policy files are accepted */
    strncpy(buf, "4.0b4", totlen - 1);
#endif
    totlen -= strlen(buf);
    strncat(buf, versionLocale, totlen - 1);

    XP_AppVersion = strdup (buf);
    /* if it fails, leave XP_AppVersion NULL */
}

/*
 * build_user_agent_string
 */
static void
build_user_agent_string(char *versionLocale)
{
    char buf [1024];
    struct utsname uts;

#ifndef __sgi
    if (fe_VendorAnim) abort (); /* only SGI has one of these now. */
#endif /* __sgi */

    strcpy (buf, fe_version);

#ifdef GOLD
    strcat(buf, "Gold");
#endif

    if ( ekit_Enabled() ) { 
        strcat(buf, "C");
        if ( ekit_UserAgent() ) {
            strcat(buf, "-");
            strcat(buf, ekit_UserAgent());
        }
    }

    if (versionLocale) {
	strcat (buf, versionLocale);
    }

    strcat (buf, " (X11; ");
#ifdef HAVE_SECURITY  /* jwz */
    strcat (buf, XP_SecurityVersion (FALSE));
#else
    strcat(buf, "N");
#endif
    strcat (buf, "; ");

    if (uname (&uts) < 0)
      {
#if defined(__sun)
	strcat (buf, "SunOS");
#elif defined(__sgi)
	strcat (buf, "IRIX");
#elif defined(__386BSD__)
	strcat (buf, "BSD/386");
#elif defined(__OSF1__)
	strcat (buf, "OSF1");
#elif defined(AIXV3)
	strcat (buf, "AIX");
#elif defined(_HPUX_SOURCE)
	strcat (buf, "HP-UX");
#elif defined(__linux)
	strcat (buf, "Linux");
#elif defined(_ATT4)
	strcat (buf, "NCR/ATT");
#elif defined(__USLC__)
	strcat (buf, "UNIX_SV");
#elif defined(sony)
	strcat (buf, "NEWS-OS");
#elif defined(nec_ews)
	strcat (buf, "NEC/EWS-UX/V");
#elif defined(SNI)
	strcat (buf, "SINIX-N");
#else
	ERROR!! run "uname -s" and put the result here.
#endif
        strcat (buf, " uname failed");

	fprintf (real_stderr,
	     "%s: uname() failed; can't tell what system we're running on\n",
		 fe_progname);
      }
    else
      {
	char *s;

#if defined(_ATT4)
	strcpy(uts.sysname,"NCR/ATT");
#endif   /* NCR/ATT */

#if defined(nec_ews)
        strcpy(uts.sysname,"NEC");
#endif   /* NEC */


	PR_snprintf (buf + strlen(buf), sizeof(buf) - strlen(buf),
			"%.100s %.100s", uts.sysname, uts.release);

	/* If uts.machine contains only hex digits, throw it away.
	   This is so that every single god damned AIX machine in
	   the world doesn't generate a unique vendor ID string. */
	s = uts.machine;
	while (s && *s)
	  {
	    if (!isxdigit (*s))
	      break;
	    s++;
	  }
	if (s && *s)
	  PR_snprintf (buf + strlen (buf), sizeof(buf)-strlen(buf),
			" %.100s", uts.machine);
      }

    strcat (buf, ")");


#if defined(__sun) && !defined(__svr4__)	/* compiled on SunOS 4.1.3 */
    if (uts.release [0] == '5')
      {
	fprintf (stderr,
       "%s: this is a SunOS 4.1.3 executable, and you are running it on\n"
     "	SunOS %s (Solaris.)  It would be a very good idea for you to\n"
     "	run the Solaris-specific binary instead.  Bad Things may happen.\n\n",
		 fe_progname, uts.release);
      }
#endif /* 4.1.3 */

    assert(strlen(buf) < sizeof buf);
    if (strlen(buf) >= sizeof(buf)) abort(); /* stack already damaged */

    if (XP_AppVersion) {
	free((void *)XP_AppVersion);
	XP_AppVersion = NULL;
    }
    XP_AppVersion = strdup (buf);
    if (!XP_AppVersion) {
	XP_AppVersion = "";
    }
}

Widget fe_toplevel = 0; /* #### gag! */


/*
 *    Sorry folks, C++ has come to town. *Some* C++ compilers/linkers require
 *    the program's main() to be in a C++ file. I've added a losing C++ main()
 *    in cplusplusmain.cc. This guy will call mozilla_main(). Look, I'm really
 *    sorry about this. I don't know what to say. C++ loses, Cfront loses,
 *    we are now steeped in all of their lossage. We will descend into the
 *    burning pit of punctuation used for obscure language features, our
 *    links will go asunder, when we seek out our proper inheritance, from
 *    the mother *and* father from which we were begat, we shall be awash,
 *    for we will be without identity. We shall try to be pure, but shall find
 *    ourselves unable to instanciate. We shall be damned for all eternity,
 *    for we have danced with the Barney, and shall be statically bound unto
 *    him. Oh, woe unto us... djw
 */
int
#ifdef CPLUSPLUS_LINKAGE
mozilla_main
#else
main
#endif /*EDITOR*/
(int argc, char** argv)
{
  Widget toplevel;
  Display *dpy;
  int i;
  struct sigaction act;

  char *s;
  char **remote_commands = 0;
  int remote_command_count = 0;
  int remote_command_size = 0;
  unsigned long remote_window = 0;

  XP_StatStruct stat_struct;

  MWContext* biffcontext;

  char versionLocale[32];

  versionLocale[0] = 0;

  fe_progname = argv [0];
  s = strrchr (fe_progname, '/');
  if (s) fe_progname = s + 1;

  real_stderr = stderr;
  real_stdout = stdout;

  fe_hack_uid();	/* Do this real early */

  /* DEBUG_JWZ from mozilla 5 */
  /* 
   * Check the environment for MOZILLA_NO_ASYNC_DNS.  It would be nice to
   * make this either a pref or a resource/option.  But, at this point in
   * the game neither prefs nor xt resources have been initialized and read.
   */
  fe_check_use_async_dns();

  if (fe_UseAsyncDNS())
  {
    XFE_InitDNS_Early(argc,argv);   /* Do this early (before Java, NSPR.) */
  }

#if defined(NSPR) || defined(JAVA) /* jwz */
  /*
  ** Initialize the runtime, preparing for multi-threading. Make mozilla
  ** a higher priority than any java applet. Java's priority range is
  ** 1-10, and we're mapping that to 11-20 (in sysThreadSetPriority).
  */
  PR_Init("mozilla", 24, 1, 0);

# ifdef JAVA
  LJ_SetProgramName(argv[0]);
  PR_XLock();
  mozilla_thread = PR_CurrentThread();
  fdset_lock = PR_NewNamedMonitor(0, "mozilla-fdset-lock");

  /*
  ** Create a pipe used to wakeup mozilla from select. A problem we had
  ** to solve is the case where a non-mozilla thread uses the netlib to
  ** fetch a file. Because the netlib operates by updating mozilla's
  ** select set, and because mozilla might be in the middle of select
  ** when the update occurs, sometimes mozilla never wakes up (i.e. it
  ** looks hung). Because of this problem we create a pipe and when a
  ** non-mozilla thread wants to wakeup mozilla we write a byte to the
  ** pipe.
  */
  mozilla_event_queue = PR_CreateEventQueue("mozilla-event-queue");
  if (mozilla_event_queue == NULL) {
      fprintf(stderr, "%s: failed to initialize mozilla_event_queue!\n", fe_progname);
      exit(-1);
  }
# endif /* JAVA */
#endif /* NSPR || JAVA */


  fe_fix_fds ();

#ifdef MEMORY_DEBUG
#ifdef __sgi
  mallopt (M_DEBUG, 1);
  mallopt (M_CLRONFREE, 0xDEADBEEF);
#endif
#endif

#ifdef DEBUG
      {
	  extern int il_debug;
	  if(getenv("ILD"))
	      il_debug=atoi(getenv("ILD"));
      }

  if(stat(".WWWtraceon", &stat_struct) != -1)
      MKLib_trace_flag = 2;
#endif

  /* user agent stuff used to be here */
  fe_progclass = XFE_PROGCLASS_STRING;
  XP_AppName = fe_progclass;
  XP_AppCodeName = "Mozilla";


  save_argv = (char **)malloc(argc*sizeof(char*));

  save_argc = 1;
  /* Hack the -help and -version arguments before opening the display. */
  for (i = 1; i < argc; i++)
    {
      if (!strcasecmp (argv [i], "-h") ||
	  !strcasecmp (argv [i], "-help") ||
	  !strcasecmp (argv [i], "--help"))
	{
	  usage ();
	  exit (0);
	}
#ifdef JAVA
      else if (!strcasecmp (argv [i], "-java") ||
	       !strcasecmp (argv [i], "--java"))
	{
	  extern int start_java(int argc, char **argv);
	  start_java(argc-i, &argv[i]);
	  exit (0);
	}
#endif /* JAVA */
      else if (!strcasecmp (argv [i], "-v") ||
	       !strcasecmp (argv [i], "-version") ||
	       !strcasecmp (argv [i], "--version"))
	{
	  fprintf (stderr, "%s\n", fe_long_version + 4);
	  exit (0);
	}
      else if (!strcasecmp (argv [i], "-remote") ||
	       !strcasecmp (argv [i], "--remote"))
	{
	  if (remote_command_count == remote_command_size)
	    {
	      remote_command_size += 20;
	      remote_commands =
		(remote_commands
		 ? realloc (remote_commands,
			    remote_command_size * sizeof (char *))
		 : calloc (remote_command_size, sizeof (char *)));
	    }
	  i++;
	  if (!argv[i] || *argv[i] == '-' || *argv[i] == 0)
	    {
	      fprintf (stderr, "%s: invalid `-remote' option \"%s\"\n",
		       fe_progname, argv[i] ? argv[i] : "");
	      usage ();
	      exit (-1);
	    }
	  remote_commands [remote_command_count++] = argv[i];
	}
      else if (!strcasecmp (argv [i], "-raise") ||
	       !strcasecmp (argv [i], "--raise") ||
	       !strcasecmp (argv [i], "-noraise") ||
	       !strcasecmp (argv [i], "--noraise"))
	{
	  char *r = argv [i];
	  if (r[0] == '-' && r[1] == '-')
	    r++;
	  if (remote_command_count == remote_command_size)
	    {
	      remote_command_size += 20;
	      remote_commands =
		(remote_commands
		 ? realloc (remote_commands,
			    remote_command_size * sizeof (char *))
		 : calloc (remote_command_size, sizeof (char *)));
	    }
	  remote_commands [remote_command_count++] = r;
	}
      else if (!strcasecmp (argv [i], "-id") ||
	       !strcasecmp (argv [i], "--id"))
	{
	  char c;
	  if (remote_command_count > 0)
	    {
	      fprintf (stderr,
		"%s: the `-id' option must preceed all `-remote' options.\n",
		       fe_progname);
	      usage ();
	      exit (-1);
	    }
	  else if (remote_window != 0)
	    {
	      fprintf (stderr, "%s: only one `-id' option may be used.\n",
		       fe_progname);
	      usage ();
	      exit (-1);
	    }
	  i++;
	  if (argv[i] &&
	      1 == sscanf (argv[i], " %ld %c", &remote_window, &c))
	    ;
	  else if (argv[i] &&
		   1 == sscanf (argv[i], " 0x%lx %c", &remote_window, &c))
	    ;
	  else
	    {
	      fprintf (stderr, "%s: invalid `-id' option \"%s\"\n",
		       fe_progname, argv[i] ? argv[i] : "");
	      usage ();
	      exit (-1);
	    }
	 }
       else  /* Save these args  for session manager */
         {
	     save_argv[save_argc] = 0;
	     save_argv[save_argc] = (char*)malloc((strlen(argv[i])+1)*
				sizeof(char*)); 
             strcpy(save_argv[save_argc], argv[i]);
	     save_argc++;
         }
     }

  lose_internationally ();

#if defined(__sun)
  {
    /* The nightmare which is XKeysymDB is unrelenting. */
    /* It'd be nice to check the existence of the osf keysyms here
       to know whether we're screwed yet, but we must set $XKEYSYMDB
       before initializing the connection; and we can't query the
       keysym DB until after initializing the connection. */
    char *override = getenv ("XKEYSYMDB");
    struct stat st;
    if (override && !stat (override, &st))
      {
	/* They set it, and it exists.  Good. */
      }
    else
      {
	char *try1 = "/usr/lib/X11/XKeysymDB";
	char *try2 = "/usr/openwin/lib/XKeysymDB";
	char *try3 = "/usr/openwin/lib/X11/XKeysymDB";
	char buf [1024];
	if (override)
	  {
	    fprintf (stderr,
		     "%s: warning: $XKEYSYMDB is %s,\n"
		     "	but that file doesn't exist.\n",
		     argv [0], override);
	  }

	if (!stat (try1, &st))
	  {
	    PR_snprintf (buf, sizeof (buf), "XKEYSYMDB=%.900s", try1);
	    putenv (buf);
	  }
	else if (!stat (try2, &st))
	  {
	    PR_snprintf (buf, sizeof (buf), "XKEYSYMDB=%.900s", try2);
	    putenv (buf);
	  }
	else if (!stat (try3, &st))
	  {
	    PR_snprintf (buf, sizeof (buf), "XKEYSYMDB=%.900s", try3);
	    putenv (buf);
	  }
	else
	  {
	    fprintf (stderr,
		     "%s: warning: no XKeysymDB file in either\n"
		     "	%s, %s, or %s\n"
		     "	Please set $XKEYSYMDB to the correct XKeysymDB"
		     " file.\n",
		     argv [0], try1, try2, try3);
	  }
      }
  }
#endif /* sun */

  toplevel = XtAppInitialize (&fe_XtAppContext, (char *) fe_progclass, options,
			      sizeof (options) / sizeof (options [0]),
			      &argc, argv, fe_fallbackResources, 0, 0);
  fe_toplevel = toplevel; /* #### gag! */

  fe_progname_long = fe_locate_program_path (argv [0]);

/*
 * We set all locale categories except for LC_CTYPE back to "C" since
 * we want to be careful, and since we already know that libnet depends
 * on the C locale for strftime for HTTP, NNTP and RFC822. We call
 * XtSetLanguageProc before calling XtAppInitialize, which makes the latter
 * call setlocale for all categories. Hence the setting back to "C"
 * stuff here. See other comments near the XtSetLanguageProc call. -- erik
 */
#ifdef __osf__
  /*
   * For some strange reason, we cannot set LC_COLLATE to "C" on DEC,
   * otherwise the text widget misbehaves, e.g. deleting one byte instead
   * of two when backspacing over a Japanese character. Grrr. -- erik
   */
  /* setlocale(LC_COLLATE, "C"); */
  fe_InitCollation();

  setlocale(LC_MONETARY, "C");
  setlocale(LC_NUMERIC, "C");
  /*setlocale(LC_TIME, "C");*/
  setlocale(LC_MESSAGES, "C");
#endif /* __osf__ */

  fe_display = dpy = XtDisplay (toplevel);

#ifdef JAVA  /* jwz */
  /* Now we can turn the nspr clock on */
  PR_StartEvents(0);
#endif

  /* For security stuff... */
  fe_dpy_kludge = dpy;
  fe_screen_kludge = XtScreen (toplevel);


  /* Initialize the security library. This must be done prior
     to any calls that will cause X event activity, since the
     event loop calls security functions.
   */

  XtGetApplicationNameAndClass (dpy,
				(char **) &fe_progname,
				(char **) &fe_progclass);

  /* Things blow up if argv[0] has a "." in it. */
  {
    char *s = (char *) fe_progname;
    while ((s = strchr (s, '.')))
      *s = '_';
  }


  /* If there were any -command options, then we are being invoked in our
     role as a controller of another Mozilla window; find that other window,
     send it the commands, and exit. */
  if (remote_command_count > 0)
    {
      int status = fe_RemoteCommands (dpy, (Window) remote_window,
				      remote_commands);
      exit (status);
    }
  else if (remote_window)
    {
      fprintf (stderr,
	       "%s: the `-id' option may only be used with `-remote'.\n",
	       fe_progname);
      usage ();
      exit (-1);
    }

  fe_sanity_check_resources (toplevel);

  {
    char	clas[128];
    XrmDatabase	db;
    char	*locale;
    char	name[128];
    char	*type;
    XrmValue	value;

    db = XtDatabase(dpy);
    PR_snprintf(name, sizeof(name), "%s.versionLocale", fe_progname);
    PR_snprintf(clas, sizeof(clas), "%s.VersionLocale", fe_progclass);
    if (XrmGetResource(db, name, clas, &type, &value) && *((char *) value.addr))
      {
        PR_snprintf(versionLocale, sizeof(versionLocale), " [%s]",
		(char *) value.addr);

        XP_AppLanguage = strdup((char*)value.addr);

        fe_version_and_locale = PR_smprintf("%s%s", fe_version, versionLocale);
        if (!fe_version_and_locale)
	  {
	    fe_version_and_locale = (char *) fe_version;
	  }
      }
    if (!XP_AppLanguage) {
	XP_AppLanguage = strdup("en");
    }

    PR_snprintf(name, sizeof(name), "%s.httpAcceptLanguage", fe_progname);
    PR_snprintf(clas, sizeof(clas), "%s.HttpAcceptLanguage", fe_progclass);
    if (XrmGetResource(db, name, clas, &type, &value) && *((char *) value.addr))
      {
	fe_accept_language = (char *) value.addr;
      }

    locale = fe_GetNormalizedLocaleName();
    if (locale)
      {
        PR_snprintf(name, sizeof(name), "%s.localeCharset.%s", fe_progname,
		locale);
	free((void *) locale);
      }
    else
      {
        PR_snprintf(name, sizeof(name), "%s.localeCharset.C", fe_progname);
      }
    PR_snprintf(clas, sizeof(clas), "%s.LocaleCharset.Locale", fe_progclass);
    if (XrmGetResource(db, name, clas, &type, &value) && *((char *) value.addr))
      {
	fe_LocaleCharSetID = INTL_CharSetNameToID((char *) value.addr);
	if (fe_LocaleCharSetID == CS_UNKNOWN)
	  {
	    fe_LocaleCharSetID = CS_LATIN1;
	  }
      }
    else
      {
	fe_LocaleCharSetID = CS_LATIN1;
      }
    INTL_CharSetIDToName(fe_LocaleCharSetID, fe_LocaleCharSetName);
  }

  /* Now that we've got a display and know we're going to behave as a
     client rather than a remote controller, fix the $DISPLAY setting. */
  fe_fix_environment (dpy);

  /* Hack remaining arguments - assume things beginning with "-" are
     misspelled switches, instead of file names.  Magic for -help and
     -version has already happened.
   */
  for (i = 1; i < argc; i++)
    if (*argv [i] == '-')
      {
	fprintf (stderr, XP_GetString( XFE_UNRECOGNISED_OPTION ),
		 fe_progname, argv [i]);
	usage ();
	exit (-1);
      }


  /* Must be called before XFE_ReadPrefs(). */
  fe_InitFonts(dpy);

  /* Setting fe_home_dir likewise must happen early. */
  fe_home_dir = getenv ("HOME");
  if (!fe_home_dir || !*fe_home_dir)
    {
      /* Default to "/" in case a root shell is running in dire straits. */
      struct passwd *pw = getpwuid(getuid());

      fe_home_dir = pw ? pw->pw_dir : "/";
    }
  else
    {
      char *slash;

      /* Trim trailing slashes just to be fussy. */
      while ((slash = strrchr(fe_home_dir, '/')) && slash[1] == '\0')
	*slash = '\0';
    }

  /* See if another instance is running.  If so, don't use the .db files */
  if (fe_ensure_config_dir_exists (toplevel))
    {
      char *name;
      unsigned long addr;
      pid_t pid;

      name = PR_smprintf ("%s/lock", fe_config_dir);
      addr = 0;
      pid = 0;
      if (name == NULL)
	/* out of memory -- what to do? */;
      else if (fe_create_pidlock (name, &addr, &pid) == 0)
	{
	  /* Remember name in fe_pidlock so we can unlink it on exit. */
	  fe_pidlock = name;
	}
      else 
	{
	  char *fmt = NULL;
	  char *lock = name ? name : ".netscape/lock";

	  fmt = PR_sprintf_append(fmt, XP_GetString(XFE_APP_HAS_DETECTED_LOCK),
				  XP_AppName, lock);
	  fmt = PR_sprintf_append(fmt,
				  XP_GetString(XFE_ANOTHER_USER_IS_RUNNING_APP),
				  XP_AppName,
				  fe_config_dir);

	  if (addr != 0 && pid != 0)
	    {
	      struct in_addr inaddr;
	      PRHostEnt *hp, hpbuf;
	      char dbbuf[PR_NETDB_BUF_SIZE];
	      const char *host;

	      inaddr.s_addr = addr;
	      hp = PR_gethostbyaddr((char *)&inaddr, sizeof inaddr, AF_INET,
				    &hpbuf, dbbuf, sizeof(dbbuf), 0);
	      host = (hp == NULL) ? inet_ntoa(inaddr) : hp->h_name;
	      fmt = PR_sprintf_append(fmt,
		      XP_GetString(XFE_APPEARS_TO_BE_RUNNING_ON_HOST_UNDER_PID),
		      host, (unsigned)pid);
	    }

	  fmt = PR_sprintf_append(fmt,
				  XP_GetString(XFE_YOU_MAY_CONTINUE_TO_USE),
				  XP_AppName);
	  fmt = PR_sprintf_append(fmt,
				  XP_GetString(XFE_OTHERWISE_CHOOSE_CANCEL),
				  XP_AppName, lock, XP_AppName);

	  if (fmt)
	    {
#ifndef DEBUG_jwz  /* don't bother ever asking this question: just assume
                      the answer is "yes, run anyway and disable access
                      to the locked database files."
                   */
	      if (!fe_Confirm_2 (toplevel, fmt))
		exit (0);
#endif /* DEBUG_jwz */
	      free (fmt);
	    }

	  /* Between the time the dialog came up, and the user hit OK,
	     they might have gone to a shell and deleted the lock file
	     or exited the other copy.  So, check to see if the lock
	     file is still there -- if it isn't, then don't bother
	     running in no-database mode, because now everything's fine
	     again.
	   */
	  if (!fe_pidlock && fe_create_pidlock (name, &addr, &pid) == 0)
	    {
	      fe_pidlock = name;
	      name = 0;
	    }
	  else
	    {
	      /* Keep on-disk databases from being open-able. */
	      dbSetOrClearDBLock (LockOutDatabase);
	    }

	  if (name) free (name);
	}
    }

  {
    char buf [1024];
    PR_snprintf (buf, sizeof (buf), "%s/%s", fe_home_dir,
#ifdef OLD_UNIX_FILES
		".netscape-preferences"
#else
		".netscape/preferences"
#endif
		);

    /* Install the default signal handlers */
    act.sa_handler = fe_sigchild_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction (SIGCHLD, &act, NULL);

    fe_InitializeGlobalResources (toplevel);

#ifndef OLD_UNIX_FILES
    /* The preferences file always comes from $HOME/.netscape/preferences */
    fe_globalData.user_prefs_file = strdup (buf);

    /* Maybe we want to copy an old set of files before reading them
       with their historically-current names. */
    fe_copy_init_files (toplevel);

#else  /* OLD_UNIX_FILES */

    /* The preferences file always comes from $HOME/.netscape-preferences */
    fe_globalData.user_prefs_file = strdup (buf);

    /* Maybe we want to rename an old set of files before reading them
       with their historically-current names. */
    fe_rename_init_files (toplevel);

#endif /* OLD_UNIX_FILES */

    /* Must be called before XFE_DefaultPrefs */
    ekit_Initialize(toplevel);

    /* Load up the preferences. */
    XFE_DefaultPrefs (xfe_PREFS_ALL, &fe_defaultPrefs);

    /* must be called after fe_InitFonts() */
    XFE_ReadPrefs ((char *) fe_globalData.user_prefs_file,
		   &fe_globalPrefs);

#ifndef OLD_UNIX_FILES
    fe_clean_old_init_files (toplevel); /* finish what we started... */
#else  /* OLD_UNIX_FILES */
    /* kludge kludge - the old name is *stored* in the prefs, so if we've
       renamed the cache directory, rename it in the preferences too. */
    if (fe_renamed_cache_dir)
      {
	if (fe_globalPrefs.cache_dir)
	  free (fe_globalPrefs.cache_dir);
	fe_globalPrefs.cache_dir = fe_renamed_cache_dir;
	fe_renamed_cache_dir = 0;
	if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file,
			    &fe_globalPrefs))
	  fe_perror_2 (toplevel, XP_GetString( XFE_ERROR_SAVING_OPTIONS ) );
      }
#endif /* OLD_UNIX_FILES */

    GH_SetGlobalHistoryTimeout (fe_globalPrefs.global_history_expiration
				* 24 * 60 * 60);
  }

  fe_RegisterConverters ();  /* this must be before InstallPreferences(),
				and after fe_InitializeGlobalResources(). */

#ifdef SAVE_ALL
  SAVE_Initialize ();	     /* Register some more converters. */
#endif /* SAVE_ALL */

  ekit_LockPrefs();

  /* SECNAV_INIT needs this defined, but build_user_agent_string cannot 
   * be called until after SECNAV_INIT, so call this simplified version.
   */
  build_simple_user_agent_string(versionLocale);

  fe_InstallPreferences (0);

  /*
  ** Initialize the security library.
  **
  ** MAKE SURE THIS IS DONE BEFORE YOU CALL THE NET_InitNetLib, CUZ
  ** OTHERWISE THE FILE LOCK ON THE HISTORY DB IS LOST!
  */
#if HAVE_SECURITY /* jwz */
  SECNAV_Init();
#endif

  /* Must be called after ekit, since user agent is configurable */
  /* Must also be called after SECNAV_Init, since crypto is dynamic */
  build_user_agent_string(versionLocale);

#ifdef HAVE_SECURITY /* added by jwz */
  RNG_SystemInfoForRNG ();
  RNG_FileForRNG (fe_globalPrefs.history_file);
  fe_read_screen_for_rng (dpy, XtScreen (toplevel));
#endif /* !HAVE_SECURITY -- added by jwz */

  /* Initialize the network library. */
  NET_InitNetLib (fe_globalPrefs.network_buffer_size * 1024, 50);

  /* DEBUG_JWZ from mozilla 5 */
  if (fe_UseAsyncDNS())
    {
      /* Finish initializing the nonblocking DNS code. */
      XFE_InitDNS_Late(fe_XtAppContext);
    }


#ifdef MOCHA
  /* Initialize libmocha after netlib and before plugins. */
  LM_InitMocha ();
#endif

  NPL_Init();

  /* Do some late java init */
  fe_late_java_init();
  
  fe_InitCommandActions ();
  fe_InitMouseActions ();
#ifdef EDITOR
  fe_EditorInitActions();
#endif

  XSetErrorHandler (x_error_handler);
  XSetIOErrorHandler (x_fatal_error_handler);

  old_xt_warning_handler = XtAppSetWarningMsgHandler(fe_XtAppContext,
						     xt_warning_handler);
  old_xt_warningOnly_handler =  XtAppSetWarningHandler(fe_XtAppContext,
						       xt_warningOnly_handler);

  /* At this point, everything in argv[] is a document for us to display.
     Create a window for each document.
   */
  if (argc > 1)
    for (i = 1; i < argc; i++)
      {
	char buf [2048];
	if (argv [i][0] == '/')
	  {
	    /* It begins with a / so it's a file. */
	    URL_Struct *url;
	    PR_snprintf (buf, sizeof (buf), "file:%.900s", argv [i]);
	    url = NET_CreateURLStruct (buf, FALSE);
#ifdef NON_BETA
	    plonkp = True;
#endif
	    fe_MakeWindow (toplevel, 0, url, NULL, MWContextBrowser, FALSE);
	  }
	else if (argv [i][0] == 0)
	  {
	    /* Empty-string means "no document." */
	    fe_MakeWindow (toplevel, 0, 0, NULL, MWContextBrowser, FALSE);
	  }
	else
	  {
	    PRHostEnt hpbuf;
	    char dbbuf[PR_NETDB_BUF_SIZE];
	    char *s = argv [i];
	    while (*s && *s != ':' && *s != '/')
	      s++;
	    if (*s == ':' || PR_gethostbyname (argv [i], &hpbuf, dbbuf,
					       sizeof(dbbuf), 0))
	      {
		/* There is a : before the first / so it's a URL.
		   Or it is a host name, which are also magically URLs.
		 */
		URL_Struct *url = NET_CreateURLStruct (argv [i], FALSE);
#ifdef NON_BETA
		plonkp = True;
#endif
		fe_MakeWindow (toplevel, 0, url, NULL, MWContextBrowser,
			       FALSE);
	      }
	    else
	      {
		/* There is a / or end-of-string before : so it's a file. */
		char cwd [1024];
		URL_Struct *url;
		if (! getcwd (cwd, sizeof cwd))
		  {
		    fprintf (stderr, "%s: getwd: %s\n", fe_progname, cwd);
		    break;
		  }
		PR_snprintf (buf, sizeof (buf),
				"file:%.900s/%.900s", cwd, argv [i]);
		url = NET_CreateURLStruct (buf, FALSE);
#ifdef NON_BETA
		plonkp = True;
#endif
		fe_MakeWindow (toplevel, 0, url, NULL, MWContextBrowser,
			       FALSE);
	      }
	  }
      }
  else
    {
      URL_Struct *url = 0;
      char *bufp = NULL;

      if (fe_globalPrefs.startup_mode == 1)
	bufp = XP_STRDUP ("mailbox:");
      else if (fe_globalPrefs.startup_mode == 2)
	bufp = XP_STRDUP ("news:");
      else
	{
	  if (fe_globalPrefs.home_document && *fe_globalPrefs.home_document)
	    bufp = XP_STRDUP (fe_globalPrefs.home_document);
	}

      url = NET_CreateURLStruct (bufp, FALSE);
      fe_MakeWindow (toplevel, 0, url, NULL, MWContextBrowser, FALSE);
      if (bufp) XP_FREE (bufp);
    }

  /* Create the biffcontext, to keep the new-mail notifiers up-to-date. */
  biffcontext = XP_NEW_ZAP(MWContext);
  biffcontext->type = MWContextBiff;
  biffcontext->funcs = fe_BuildDisplayFunctionTable();
  CONTEXT_DATA(biffcontext) = XP_NEW_ZAP(fe_ContextData);
  XP_AddContextToList(biffcontext);
  
  MSG_BiffInit(biffcontext);

  fe_command_line_done = True;

  while (1)
    fe_EventLoop ();

  return (0);
}

static void
set_shell_iconic_p (Widget shell, Boolean iconic_p)
{
  /* Because of fucked-uppedness in Xt/Shell.c, one can't change the "iconic"
     flag of a shell after it has been created but before it has been realized.
     That resource is only checked when the shell is Initialized (ie, when
     XtCreatePopupShell is called) instead of the more obvious times like:
     when it is Realized (before the window is created) or when it is Managed
     (before window is mapped).

     The Shell class's SetValues method does not modify wm_hints.initial_state
     if the widget has not yet been realized - it ignores the request until
     afterward, when it's too late.
   */
  WMShellWidget wmshell = (WMShellWidget) shell;
  XtVaSetValues (shell, XtNiconic, iconic_p, 0);
  wmshell->wm.wm_hints.flags |= StateHint;
  wmshell->wm.wm_hints.initial_state = (iconic_p ? IconicState : NormalState);
}

#ifdef _HPUX_SOURCE
/* Words cannot express how much HPUX SUCKS!
   Sometimes rename("/u/jwz/.MCOM-cache", "/u/jwz/.netscape-cache")
   will fail and randomly corrupt memory (but only in the case where
   the first is a directory and the second doesn't exist.)  To avoid
   this, we fork-and-exec `mv' instead of using rename().  Fuckers.
 */
# include <sys/wait.h>

# undef rename
# define rename hpux_sucks_wet_farts_from_dead_pigeons
static int
rename (const char *source, const char *target)
{
  struct sigaction newact, oldact;
  pid_t forked;
  int ac = 0;
  char **av = (char **) malloc (10 * sizeof (char *));
  av [ac++] = strdup ("/bin/mv");
  av [ac++] = strdup ("-f");
  av [ac++] = strdup (source);
  av [ac++] = strdup (target);
  av [ac++] = 0;
	
  /* Setup signals so that SIGCHLD is ignored as we want to do waitpid */
  newact.sa_handler = SIG_DFL;
  newact.sa_flags = 0;
  sigfillset(&newact.sa_mask);
  sigaction (SIGCHLD, &newact, &oldact);

  switch (forked = fork ())
    {
    case -1:
      while (--ac >= 0)
	free (av [ac]);
      free (av);
      /* Reset SIGCHLD signal hander before returning */
      sigaction(SIGCHLD, &oldact, NULL);
      return -1;	/* fork() failed (errno is meaningful.) */
      break;
    case 0:
      {
	execvp (av[0], av);
	exit (-1);	/* execvp() failed (this exits the child fork.) */
	break;
      }
    default:
      {
	/* This is the "old" process (subproc pid is in `forked'.) */
	int status = 0;

	/* wait for `mv' to terminate. */
	pid_t waited_pid = waitpid (forked, &status, 0);

	/* Reset SIGCHLD signal hander before returning */
	sigaction(SIGCHLD, &oldact, NULL);

	while (--ac >= 0)
	  free (av [ac]);
	free (av);

#if 0
	/* Ok, I tried to be a good boy and notice that `mv' had exited
	   with a non-0 code, but this just doesn't fucking work.  No
	   matter what the exit code is, `status' is 133, whereas (by
	   my reading of sys/wait.h) an exit code of 0 should yield a
	   status of 0, and an exit code of 1 should yield a status
	   of 256.  So fuck it, assume mv always succeeds, and the luser
	   will read their stderr.
	 */
	errno = 0;
	if (waited_pid <= 0)
	  return -1; /* huh? */
	else if (waited_pid != forked)
	  return -1; /* huh? */
	else if (WIFEXITED (status))
	  /* Return 0 if `mv' exited normally, -1 otherwise.
	     We unfortunately have no sensible `errno' value to return. */
	  return (WEXITSTATUS (status) == 0 ? 0 : -1);
	else
	  return -1;
#else
	return 0;
#endif
	break;
      }
    }
}
#endif /* !HPUX */



/* With every release, we change the names or locations of the init files.
   Is this a feature?

              1.0                       1.1                      2.0
  ========================== ======================== ========================
  .MCOM-bookmarks.html       .netscape-bookmarks.html .netscape/bookmarks.html
  .MCOM-HTTP-cookie-file     .netscape-cookies        .netscape/cookies
  .MCOM-preferences          .netscape-preferences    .netscape/preferences
  .MCOM-global-history       .netscape-history        .netscape/history
  .MCOM-cache/               .netscape-cache/         .netscape/cache/
  .MCOM-cache/MCOM-cache-fat .netscape-cache/index    .netscape/cache/index.db
  .MCOM-newsgroups-*         .netscape-newsgroups-*   .netscape/newsgroups-*
   <none>                     <none>                  .netscape/cert.db
   <none>                     <none>                  .netscape/cert-nameidx.db
   <none>                     <none>                  .netscape/key.db
   <none>                     <none>                  .netscape/popstate
   <none>                     <none>                  .netscape/cachelist
   <none>                     <none>                  nsmail/
   <none>                     <none>                  .netscape/mailsort

  Files that never changed:

  .newsrc-*                  .newsrc-*                .newsrc-*
  .snewsrc-*                 .snewsrc-*               .snewsrc-*
 */

#ifdef OLD_UNIX_FILES


static void
fe_rename_init_files (Widget toplevel)
{
  struct stat st;
  char file1 [1024];
  char file2 [1024];
  char buf [1024];
  char *s1, *s2;
  Boolean asked = False;

  PR_snprintf (file1, sizeof (file1), "%s/", fe_home_dir);
  strcpy (file2, file1);
  s1 = file1 + strlen (file1);
  s2 = file2 + strlen (file2);

#define FROB(NAME1,NAME2,GAG)						\
  strcpy (s1, NAME1);							\
  strcpy (s2, NAME2);							\
  if (stat (file2, &st))						\
    if (!stat (file1, &st))						\
      {									\
	if (! asked)							\
	  {								\
	    if (fe_Confirm_2 (toplevel,					\
			      fe_globalData.rename_files_message))	\
	      asked = True;						\
	    else							\
	      return;							\
	  }								\
	if (rename (file1, file2))					\
	  {								\
	    PR_snprintf (buf, sizeof (buf), "renaming %s to %s:", s1, s2); \
	    fe_perror_2 (toplevel, buf);				\
	  }								\
        else if (GAG)							\
	  fe_renamed_cache_dir = strdup (file2);			\
      }

  FROB (".MCOM-bookmarks.html",		  ".netscape-bookmarks.html", False);
  FROB (".MCOM-HTTP-cookie-file",	  ".netscape-cookies", False);
  FROB (".MCOM-preferences",		  ".netscape-preferences", False);
  FROB (".MCOM-global-history",		  ".netscape-history", False);
  FROB (".MCOM-cache",			  ".netscape-cache", True);
  FROB (".netscape-cache/MCOM-cache-fat", ".netscape-cache/index", False);
#undef FROB
}

#else  /* !OLD_UNIX_FILES */


/*extern char *sys_errlist[];*/
extern int sys_nerr;

static XP_Bool
fe_ensure_config_dir_exists (Widget toplevel)
{
  char *dir, *fmt;
  static char buf [2048];
  struct stat st;
  XP_Bool exists;

  dir = PR_smprintf ("%s/.netscape", fe_home_dir);
  if (!dir)
    return FALSE;

  exists = !stat (dir, &st);

  if (exists && !(st.st_mode & S_IFDIR))
    {
      /* It exists but is not a directory!
	 Rename the old file so that we can create the directory.
	 */
      char *loser = (char *) XP_ALLOC (XP_STRLEN (dir) + 100);
      XP_STRCPY (loser, dir);
      XP_STRCAT (loser, ".BAK");
      if (rename (dir, loser) == 0)
	{
	  fmt = XP_GetString( XFE_EXISTED_BUT_WAS_NOT_A_DIRECTORY );
	  exists = FALSE;
	}
      else
	{
	  fmt = XP_GetString( XFE_EXISTS_BUT_UNABLE_TO_RENAME );
	}

      PR_snprintf (buf, sizeof (buf), fmt, XP_AppName, dir, loser);
      XP_FREE (loser);
      fe_Alert_2 (toplevel, buf);
      if (exists)
	{
	  free (dir);
	  return FALSE;
	}
    }

  if (!exists)
    {
      /* ~/.netscape/ does not exist.  Create the directory.
       */
      if (mkdir (dir, 0700) < 0)
	{
	  fmt = XP_GetString( XFE_UNABLE_TO_CREATE_DIRECTORY );
#ifdef DEBUG_jwz  /* this is the modern way */
          char *es = 0;
          if (errno >= 0)
            es = strerror (errno);
          if (!es || !*es)
            es = XP_GetString( XFE_UNKNOWN_ERROR );
	  PR_snprintf (buf, sizeof (buf), fmt, XP_AppName, dir, es);
#else /* !DEBUG_jwz */
	  PR_snprintf (buf, sizeof (buf), fmt, XP_AppName, dir,
		   ((errno >= 0 && errno < sys_nerr)
		    ? sys_errlist [errno] : XP_GetString( XFE_UNKNOWN_ERROR )));
#endif /* !DEBUG_jwz */
	  fe_Alert_2 (toplevel, buf);
	  free (dir);
	  return FALSE;
	}
    }

  fe_config_dir = dir;
  return TRUE;
}

#ifndef SUNOS4
void
fe_atexit_handler(void)
{
  fe_MinimalNoUICleanup();
  if (fe_pidlock) unlink (fe_pidlock);
  fe_pidlock = NULL;
}
#endif /* SUNOS4 */

#ifndef INADDR_LOOPBACK
#define	INADDR_LOOPBACK		0x7F000001
#endif

#define MAXTRIES	100

static int
fe_create_pidlock (const char *name, unsigned long *paddr, pid_t *ppid)
{
  char hostname[64];		/* should use MAXHOSTNAMELEN */
  PRHostEnt *hp, hpbuf;
  char dbbuf[PR_NETDB_BUF_SIZE];
  unsigned long myaddr, addr;
  struct in_addr inaddr;
  char *signature;
  int len, tries, rval;
  char buf[1024];		/* should use PATH_MAX or pathconf */
  char *colon, *after;
  pid_t pid;

  if (gethostname (hostname, sizeof hostname) < 0 ||
      (hp = PR_gethostbyname (hostname, &hpbuf, dbbuf, sizeof(dbbuf), 0)) == NULL)
    inaddr.s_addr = INADDR_LOOPBACK;
  else
    memcpy (&inaddr, hp->h_addr, sizeof inaddr);

  myaddr = inaddr.s_addr;
  signature = PR_smprintf ("%s:%u", inet_ntoa (inaddr), (unsigned)getpid ());
  tries = 0;
  addr = 0;
  pid = 0;
  while ((rval = symlink (signature, name)) < 0)
    {
      if (errno != EEXIST)
	break;
      len = readlink (name, buf, sizeof buf - 1);
      if (len > 0)
	{
	  buf[len] = '\0';
	  colon = strchr (buf, ':');
	  if (colon != NULL)
	    {
	      *colon++ = '\0';
	      if ((addr = inet_addr (buf)) != (unsigned long)-1)
		{
		  pid = strtol (colon, &after, 0);
		  if (pid != 0 && *after == '\0')
		    {
		      if (addr != myaddr)
			{
			  /* Remote pid: give up even if lock is stuck. */
			  break;
			}

		      /* signator was a local process -- check liveness */
		      if (kill (pid, 0) == 0 || errno != ESRCH)
			{
			  /* Lock-owning process appears to be alive. */
			  break;
			}
		    }
		}
	    }
	}

      /* Try to claim a bogus or stuck lock. */
      (void) unlink (name);
      if (++tries > MAXTRIES)
	break;
    }

  if (rval == 0)
    {
      struct sigaction act, oldact;

      act.sa_handler = fe_Exit;
      act.sa_flags = 0;
      sigfillset (&act.sa_mask);

      /* Set SIGINT, SIGTERM and SIGHUP to our fe_Exit(). If these signals
       * have already been ignored, dont install our handler because we
       * could have been started up in the background with a nohup.
       */
      sigaction (SIGINT, NULL, &oldact);
      if (oldact.sa_handler != SIG_IGN)
	sigaction (SIGINT, &act, NULL);

      sigaction (SIGHUP, NULL, &oldact);
      if (oldact.sa_handler != SIG_IGN)
	sigaction (SIGHUP, &act, NULL);

      sigaction (SIGTERM, NULL, &oldact);
      if (oldact.sa_handler != SIG_IGN)
	sigaction (SIGTERM, &act, NULL);

#ifndef SUNOS4
      /* atexit() is not available in sun4. We need to find someother
       * mechanism to do this in sun4. Maybe we will get a signal or
       * something.
       */

      /* Register a atexit() handler to remove lock file */
      atexit(fe_atexit_handler);
#endif /* SUNOS4 */
    }
  free (signature);
  *paddr = addr;
  *ppid = pid;
  return rval;
}


/* This copies one file to another, optionally setting the permissions.
 */
static void
fe_copy_file (Widget toplevel, const char *in, const char *out, int perms)
{
  char buf [1024];
  FILE *ifp, *ofp;
  int n;
  ifp = fopen (in, "r");
  if (!ifp) return;
  ofp = fopen (out, "w");
  if (!ofp)
    {
      PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_ERROR_CREATING ), out);
      fe_perror_2 (toplevel, buf);
      return;
    }

  if (perms)
    fchmod (fileno (ofp), perms);

  while ((n = fread (buf, 1, sizeof(buf), ifp)) > 0)
    while (n > 0)
      {
	int w = fwrite (buf, 1, n, ofp);
	if (w < 0)
	  {
	    PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_ERROR_WRITING ), out);
	    fe_perror_2 (toplevel, buf);
	    return;
	  }
	n -= w;
      }
  fclose (ofp);
  fclose (ifp);
}


/* This does several things that need to be done before the preferences
   file is loaded:

   - cause the directory ~/.netscape/ to exist,
     and warn if it couldn't be created

   - cause these files to be *copied* from their older
     equivalents, if there are older versions around

     ~/.netscape/preferences
     ~/.netscape/bookmarks.html
     ~/.netscape/cookies
 */
static XP_Bool fe_copied_init_files = FALSE;

static void
fe_copy_init_files (Widget toplevel)
{
  struct stat st1;
  struct stat st2;
  char file1 [512];
  char file2 [512];
  char *s1, *s2;

  if (!fe_config_dir)
    /* If we were unable to cause ~/.netscape/ to exist, give up now. */
    return;

  PR_snprintf (file1, sizeof (file1), "%s/", fe_home_dir);
  strcpy (file2, file1);
  s1 = file1 + strlen (file1);
  s2 = file2 + strlen (file2);

#define FROB(OLD1, OLD2, NEW, PERMS)			\
  strcpy (s1, OLD1);					\
  strcpy (s2, NEW);					\
  if (!stat (file2, &st2))				\
    ;    /* new exists - leave it alone */		\
  else if (!stat (file1, &st1))				\
    {							\
      fe_copied_init_files = TRUE;			\
      fe_copy_file (toplevel, file1, file2, 0);		\
    }							\
  else							\
    {							\
      strcpy (s1, OLD2);				\
      if (!stat (file1, &st1))				\
	{						\
          fe_copied_init_files = TRUE;			\
	  fe_copy_file (toplevel, file1, file2, 0);	\
        }						\
    }

  FROB(".netscape-preferences",
       ".MCOM-preferences",
       ".netscape/preferences",
       0)
  FROB(".netscape-bookmarks.html",
       ".MCOM-bookmarks.html",
       ".netscape/bookmarks.html",
       0)

  FROB(".netscape-cookies",
       ".MCOM-HTTP-cookie-file",
       ".netscape/cookies",
       (S_IRUSR | S_IWUSR))		/* rw only by owner */

#undef FROB
}


/* This does several things that need to be done after the preferences
   file is loaded:

   - if files were renamed, change the historical values in the preferences
     file to the new values, and save it out again

   - offer to delete any obsolete ~/.MCOM or ~/.netscape- that are still
     sitting around taking up space
 */
static void
fe_clean_old_init_files (Widget toplevel)
{
  char buf[1024];

  char nbm[512],    mbm[512];
  char ncook[512],  mcook[512];
  char npref[512],  mpref[512];
  char nhist[512],  mhist[512];
  char ncache[512], mcache[512];

  XP_Bool old_stuff = FALSE;
  XP_Bool old_cache = FALSE;

  char *slash;
  struct stat st;

  if (!fe_copied_init_files)
    return;

  /* History and cache always go in the new place by default,
     no matter what they were set to before. */
  if (fe_globalPrefs.history_file) free (fe_globalPrefs.history_file);
  PR_snprintf (buf, sizeof (buf), "%s/.netscape/history.db", fe_home_dir);
  fe_globalPrefs.history_file = strdup (buf);

  if (fe_globalPrefs.cache_dir) free (fe_globalPrefs.cache_dir);
  PR_snprintf (buf, sizeof (buf), "%s/.netscape/cache/", fe_home_dir);
  fe_globalPrefs.cache_dir = strdup (buf);

  /* If they were already keeping their bookmarks file in a different
     place, don't change that preferences setting. */
  PR_snprintf (buf, sizeof (buf), "%s/.netscape-bookmarks.html", fe_home_dir);
  if (!fe_globalPrefs.bookmark_file ||
      !XP_STRCMP (fe_globalPrefs.bookmark_file, buf))
    {
      if (fe_globalPrefs.bookmark_file) free (fe_globalPrefs.bookmark_file);
      PR_snprintf (buf, sizeof (buf), "%s/.netscape/bookmarks.html",
		   fe_home_dir);
      fe_globalPrefs.bookmark_file = strdup (buf);
    }

  /* If their home page was set to their bookmarks file (and that was
     the default location) then move it to the new place too. */
  PR_snprintf (buf, sizeof (buf), "file:%s/.netscape-bookmarks.html",
	       fe_home_dir);
  if (!fe_globalPrefs.home_document ||
      !XP_STRCMP (fe_globalPrefs.home_document, buf))
    {
      if (fe_globalPrefs.home_document) free (fe_globalPrefs.home_document);
      PR_snprintf (buf, sizeof (buf), "file:%s/.netscape/bookmarks.html",
		   fe_home_dir);
      fe_globalPrefs.home_document = strdup (buf);
    }

  fe_copied_init_files = FALSE;

  if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file, &fe_globalPrefs))
    fe_perror_2 (toplevel, "error saving options");


  PR_snprintf (nbm, sizeof (nbm), "%s/", fe_home_dir);

  strcpy (mbm, nbm);    strcat (mbm,   ".MCOM-bookmarks.html");
  strcpy (ncook, nbm);  strcat (ncook, ".netscape-cookies");
  strcpy (mcook, nbm);  strcat (mcook, ".MCOM-HTTP-cookie-file");
  strcpy (npref, nbm);  strcat (npref, ".netscape-preferences");
  strcpy (mpref, nbm);  strcat (mpref, ".MCOM-preferences");
  strcpy (nhist, nbm);  strcat (nhist, ".netscape-history");
  strcpy (mhist, nbm);  strcat (mhist, ".MCOM-global-history");
  strcpy (ncache, nbm); strcat (ncache,".netscape-cache");
  strcpy (mcache, nbm); strcat (mcache,".MCOM-cache");
                        strcat (nbm,   ".netscape-bookmarks.html");

  if (stat (nbm, &st) == 0   || stat (mbm, &st) == 0 ||
      stat (ncook, &st) == 0 || stat (mcook, &st) == 0 ||
      stat (npref, &st) == 0 || stat (mpref, &st) == 0 ||
      stat (nhist, &st) == 0 || stat (mhist, &st) == 0)
    old_stuff = TRUE;

  if (stat (ncache, &st) == 0 || stat (mcache, &st) == 0)
    old_stuff = old_cache = TRUE;

  if (old_stuff)
    {
      Boolean doit;
      char *fmt = XP_GetString( XFE_CREATE_CONFIG_FILES );

      char *foo =
	(old_cache
	 ? XP_GetString( XFE_OLD_FILES_AND_CACHE )
	 : XP_GetString( XFE_OLD_FILES ) );

      PR_snprintf (buf, sizeof (buf), fmt, XP_AppName, foo);

      doit = (Bool) ((int) fe_dialog (toplevel, "convertQuestion", buf,
				  TRUE, 0, TRUE, FALSE, 0));
      if (doit)
	{
	  DIR *dir;

	  /* Nuke the simple files */
	  unlink (nbm);   unlink (mbm);
	  unlink (ncook); unlink (mcook);
	  unlink (npref); unlink (mpref);
	  unlink (nhist); unlink (mhist);

	  /* Nuke the 1.1 cache directory and files in it */
	  dir = opendir (ncache);
	  if (dir)
	    {
	      struct dirent *dp;
	      strcpy (buf, ncache);
	      slash = buf + strlen (buf);
	      *slash++ = '/';
	      while ((dp = readdir (dir)))
		{
		  strcpy (slash, dp->d_name);
		  unlink (buf);
		}
	      closedir (dir);
	      rmdir (ncache);
	    }

	  /* Nuke the 1.0 cache directory and files in it */
	  dir = opendir (mcache);
	  if (dir)
	    {
	      struct dirent *dp;
	      strcpy (buf, mcache);
	      slash = buf + strlen (buf);
	      *slash++ = '/';
	      while ((dp = readdir (dir)))
		{
		  if (strncmp (dp->d_name, "cache", 5) &&
		      strcmp (dp->d_name, "index") &&
		      strcmp (dp->d_name, "MCOM-cache-fat"))
		    continue;
		  strcpy (slash, dp->d_name);
		  unlink (buf);
		}
	      closedir (dir);
	      rmdir (mcache);
	    }

	  /* Now look for the saved-newsgroup-listings in ~/. and nuke those.
	     (We could rename them, but I just don't care.)
	   */
	  slash = strrchr (nbm, '/');
	  slash[1] = '.';
	  slash[2] = 0;
	  dir = opendir (nbm);
	  if (dir)
	    {
	      struct dirent *dp;
	      while ((dp = readdir (dir)))
		{
		  if (dp->d_name[0] != '.')
		    continue;
		  if (!strncmp (dp->d_name, ".MCOM-newsgroups-", 17) ||
		      !strncmp (dp->d_name, ".netscape-newsgroups-", 21))
		    {
		      strcpy (slash+1, dp->d_name);
		      unlink (nbm);
		    }
		}
	      closedir (dir);
	    }
	}
    }
}

#endif /* !OLD_UNIX_FILES */


static Boolean fe_first_window_p = True;

#if 0
extern void fe_ContestCallback (Widget, XtPointer, XtPointer);
#endif

/*
 * Message Composition
 */

static void
fe_mc_field_changed(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = fe_WidgetToMWContext(widget);
  MSG_HEADER_SET msgtype = (MSG_HEADER_SET) closure;
  char* value = 0;
  XP_ASSERT (context);
  if (!context) return;
  value = fe_GetTextField(widget);
  MSG_SetHeaderContents (context, msgtype, value);
}


static void
fe_mc_field_lostfocus(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = fe_WidgetToMWContext(widget);
  MSG_HEADER_SET msgtype = (MSG_HEADER_SET) closure;
  char* value = 0;
  char* newvalue;
  XP_ASSERT (context);
  if (!context) return;
  value = fe_GetTextField(widget);
  newvalue = MSG_UpdateHeaderContents(context, msgtype, value);
  if (newvalue) {
    fe_SetTextField(widget, newvalue);
    MSG_SetHeaderContents(context, msgtype, newvalue);
    XP_FREE(newvalue);
  }
}

extern void
fe_browse_file_of_text (MWContext *context, Widget text_field, Boolean dirp);

static void
fe_mailto_browse_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_browse_file_of_text(context, CONTEXT_DATA(context)->mcFcc, FALSE); 
}



#define Off(field) XtOffset(fe_ContextData*, field)
#define ISLABEL		0x0001
#define ADDBROWSE	0x0002
#define ISMENU		0x0004
#define ISDROPSITE	0x0008
static struct {
  char* name;
  int offset;
  MSG_HEADER_SET msgtype;
  int flags;
} description[] = {
#ifdef DEBUG_jwz
  /* hairball multi-account kludge */
  {"from",	Off(mcFrom),		MSG_FROM_HEADER_MASK, ISMENU},
#else
  {"from",	Off(mcFrom),		MSG_FROM_HEADER_MASK, ISLABEL},
#endif
  {"replyTo", 	Off(mcReplyTo),		MSG_REPLY_TO_HEADER_MASK, ISDROPSITE},
  {"to",	Off(mcTo),		MSG_TO_HEADER_MASK, ISDROPSITE},
  {"cc",	Off(mcCc),		MSG_CC_HEADER_MASK, ISDROPSITE},
  {"bcc",	Off(mcBcc),		MSG_BCC_HEADER_MASK, ISDROPSITE},
  {"fcc",	Off(mcFcc),		MSG_FCC_HEADER_MASK,ADDBROWSE},
  {"newsgroups",Off(mcNewsgroups),	MSG_NEWSGROUPS_HEADER_MASK, 0},
  {"followupTo",Off(mcFollowupTo),	MSG_FOLLOWUP_TO_HEADER_MASK,0},
  {"subject", 	Off(mcSubject),		MSG_SUBJECT_HEADER_MASK, 0},
  {"attachments",Off(mcAttachments),	MSG_ATTACHMENTS_HEADER_MASK, ISDROPSITE},
};
#define NUM (sizeof(description) / sizeof(description[0]))

#ifdef DEBUG_jwz
/* hairball multi-account kludge */
static void fe_from_addr_cb (Widget, XtPointer, XtPointer);

static int last_addr = 0;

/* If I thought anyone other than me would ever see this code,
   I'd put this stuff in preferences.  But laziness prevails.

   See also `me_too' kludge in libmsg/messages.c
 */
static char **from_addrs = 0;
static int from_addrs_count = 0;

static void
init_from_addrs (void)
{
  if (!from_addrs && fe_globalPrefs.email_addresses)
    {
      char *s = strdup (fe_globalPrefs.email_addresses);
      char *state = 0;
      char *token = strtok_r (s, "\n", &state);
      from_addrs_count = 0;
      from_addrs = (char **) calloc (sizeof(char*), 100);
      do {
        from_addrs[from_addrs_count++] = token;
      } while ((token = strtok_r (0, "\n", &state)));
    }
}


/* Returns a comma-separated list of all the email addresses of this user.
 */
const char *
FE_UsersMailAddresses (void)
{
  const char *result = 0;
  if (!result)
    {
      int i;
      char *out;
      out = (char *) malloc (2048);  /* sue me */
      *out = 0;
      result = out;
      init_from_addrs();
      for (i = 0; i < from_addrs_count; i++)
        {
          char *a = strdup (from_addrs[i]);
          char *s = strchr (a, '\t');
          if (s) *s = 0;
          if (out != result)
            strcat (out, ", ");
          strcat (out, a);
          out += strlen (out);
        }
    }
  return result;
}


static char *
extract_addr(const char *mailbox)
{
  char *s = strchr(mailbox, '<');
  if (s)
    {
      char *s2 = strdup(s+1);
      char *s3 = strchr(s2, '>');
      if (s3) *s3 = 0;
      s3 = strchr(s2, '\t');
      if (s3) *s3 = 0;
      return s2;
    }
  else
    return (strdup(mailbox));
}
#endif /* DEBUG_jwz */


#ifdef DEBUG_jwz
static void
fe_set_from_addr_timer_kludge (XtPointer closure, XtIntervalId *id)
{
# ifdef DEBUG
  fprintf(real_stderr, "fe_set_from_addr_timer_kludge\n");
# endif
  fe_from_addr_cb (closure, (XtPointer) last_addr, 0);
}
#endif


static void
fe_create_composition_widgets(MWContext* context, Widget pane)
{

  Widget form[NUM+1];
  Widget label[NUM];
  Widget widget[NUM];
  Widget other[NUM];
  Widget kids[10]; 
  XmFontList fontList;
  Arg av [20];
  int ac = 0;
  int i;
  char buf[100];
  int maxwidth = 0;
  
  fe_ContextData* data = CONTEXT_DATA(context);

  fontList = fe_GetFont (context, 3, LO_FONT_FIXED);
  XtVaSetValues(pane, XmNseparatorOn, False, 0);

  for (i=0 ; i<NUM ; i++) {
    int flags = description[i].flags;
    ac = 0;
    form[i] = XmCreateForm(pane, "mailto_field", av, ac);
    other[i] = NULL;
    if (flags & (ADDBROWSE)) {
      ac = 0;
      XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
      XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
      XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
      other[i] = XmCreatePushButtonGadget(form[i], "browseButton", av, ac);
      XtAddCallback(other[i], XmNactivateCallback, fe_mailto_browse_cb,
		    context);
    }
    ac = 0;
    XtSetArg(av[ac], XmNfontList, fontList); ac++;
    XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    if (other[i]) {
      XtSetArg(av[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
      XtSetArg(av[ac], XmNrightWidget, other[i]); ac++;
    } else {
      XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    }
    if (flags & ISLABEL) {
      XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
      widget[i] = XmCreateLabelGadget(form[i], description[i].name, av, ac);
#ifdef DEBUG_jwz
  /* hairball multi-account kludge */
    } else if (flags & ISMENU) {

      int j;
      Visual *v = 0;
      Colormap cmap = 0;
      Cardinal depth = 0;
      Widget button = 0, popup_menu = 0;
      int nkids;
      Widget *kids;
      Widget parent = form[i];
      Arg av2 [20];
      int ac2 = 0;
      char *last_fcc = 0;

      init_from_addrs();
      nkids = from_addrs_count;
      kids = (Widget *) calloc (sizeof(Widget), nkids + 1);

      fe_getVisualOfContext (context, &v, &cmap, &depth);

      ac2 = 0;
      XtSetArg (av2[ac2], XmNvisual, v); ac2++;
      XtSetArg (av2[ac2], XmNcolormap, cmap); ac2++;
      XtSetArg (av2[ac2], XmNdepth, depth); ac2++;

      /* Decide which identity to default to based on the pathname of
         the selected folder.
       */
      {
        MWContext *c = XP_FindContextOfType(context, MWContextMail);
        if (c)
          {
            extern const char *MSG_SelectedFolderName (MWContext *);
            const char *folder = MSG_SelectedFolderName(c);
            int i;
# ifdef DEBUG
            fprintf(real_stderr, "\ncurrent folder is %s\n",
                    (folder ? folder : "(null)"));
# endif
            init_from_addrs();
            if (folder)
              for (i = 0; i < from_addrs_count; i++)
                {
                  char *data = strdup(from_addrs[i]);
                  char *fcc = strchr(data, '\t');
                  if (fcc)
                    {
                      char *s;
                      fcc++;
                      s = strchr(fcc, '\t');
                      if (s) *s = 0;
                      s = strrchr(fcc, '/');
                      if (s) s[1] = 0;

# ifdef DEBUG
                      fprintf(real_stderr, "comparing %s to %s\n",
                              folder, fcc);
# endif
                      if (!strncmp(fcc, folder, strlen(fcc)))
                        {
                          if (last_fcc && !strcmp (fcc, last_fcc))
                            {
# ifdef DEBUG
                              fprintf(real_stderr,
                                      "match 2: %d %s\n", i, fcc);
# endif
                            }
                          else
                            {
# ifdef DEBUG
                              fprintf(real_stderr,
                                      "match!  setting identity to %d\n", i);
# endif
                              if (last_fcc) free(last_fcc);
                              last_fcc = strdup(fcc);
                              last_addr = i;
                              /* break; */  /* keep going, match later ones */
                            }
                        }
                    }
                  free (data);
                }
          }
      }


      if (last_fcc) free(last_fcc);

# ifdef DEBUG
      fprintf(real_stderr,"making `from' menu:\n");
# endif

      popup_menu = XmCreatePulldownMenu (parent, "menu", av2, ac2);
      for (j = 0; j < nkids; j++)
	{
	  Boolean selected_p = (j == last_addr);
          char *mailbox = strdup(from_addrs[j]);
          char *s = strchr(mailbox, '\t');
	  XmString xmstring;
          if (s) *s = 0;
          xmstring = XmStringCreate (mailbox, XmFONTLIST_DEFAULT_TAG);

# ifdef DEBUG
          fprintf(real_stderr, "    %s%s\n", mailbox,
                  (selected_p ? " (selected)" : ""));
# endif

          if (selected_p)
            {
              free (fe_globalPrefs.email_address);
              fe_globalPrefs.email_address = extract_addr(mailbox);
            }
          free (mailbox);

	  ac2 = 0;
	  XtSetArg (av2[ac2], XmNvisual, v); ac2++;
	  XtSetArg (av2[ac2], XmNcolormap, cmap); ac2++;
	  XtSetArg (av2[ac2], XmNdepth, depth); ac2++;
	  XtSetArg (av2[ac2], XmNlabelString, xmstring); ac2++;

	  kids[j] = XmCreatePushButtonGadget (popup_menu, "button", av2, ac2);
	  XtAddCallback (kids[j], XmNactivateCallback, fe_from_addr_cb,
			 (XtPointer) j);

	  if (selected_p)
	    XtVaSetValues (popup_menu, XmNmenuHistory, kids[j], 0);

	  XmStringFree (xmstring);
	}
      XtManageChildren (kids, j);


      XtSetArg (av[ac], XmNsubMenuId, popup_menu); ac++;
      button = fe_CreateOptionMenu (parent, "button", av, ac);

      /* lose the dumb label */
      {
	Widget lg = XmOptionLabelGadget(button);
	if (lg) XtUnmanageChild(lg);
      }

      widget[i] = button;

      free(kids);

#endif /* DEBUG_jwz */
    } else {
      widget[i] = fe_CreateTextField(form[i], description[i].name, av, ac);
      if (fe_globalData.nonterminal_text_translations) {
	XtOverrideTranslations(widget[i],
			       fe_globalData.nonterminal_text_translations);
      }
      XtAddCallback(widget[i], XmNvalueChangedCallback, fe_mc_field_changed,
		    (XtPointer) description[i].msgtype);
      XtAddCallback(widget[i], XmNlosingFocusCallback, fe_mc_field_lostfocus,
		    (XtPointer) description[i].msgtype);
    }
    ac = 0;
    XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(av[ac], XmNrightWidget, widget[i]); ac++;
    PR_snprintf(buf, sizeof (buf), "%sLabel", description[i].name);
    label[i] = XmCreateLabelGadget(form[i], buf, av, ac);
    if (maxwidth < label[i]->core.width) {
      maxwidth = label[i]->core.width;
    }

    *((Widget*) ((char*) data + description[i].offset)) = widget[i];

    /* Do drag & drop */
    if (flags & ISDROPSITE) {
      fe_dnd_CreateDrop(widget[i], fe_mc_dropfunc, context);
    }
  }
  for (i=0 ; i<NUM ; i++) {
    XtVaSetValues(widget[i], XmNleftOffset, maxwidth, 0);
    kids[0] = label[i];
    kids[1] = widget[i];
    kids[2] = other[i];
    XtManageChildren(kids, other[i] ? 3 : 2);

#ifdef DEBUG_jwz
  /* hairball multi-account kludge */
    if (description[i].flags & ISMENU)
      widget[i]->core.height *= 1.8;	/* so shoot me */
#endif

    XtVaSetValues(form[i],
		  XmNpaneMinimum, widget[i]->core.height,
		  XmNpaneMaximum, widget[i]->core.height,
		  0);
  }

  /* #### warning this is cloned in mailnews.c (fe_set_compose_wrap_state) */

  ac = 0;
  XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  XtSetArg (av[ac], XmNfontList, fontList); ac++;
  data->mcBodyText = XmCreateScrolledText(pane, "mailto_bodyText", av, ac);
  if (fe_LocaleCharSetID & MULTIBYTE) {
    Dimension columns;
    XtVaGetValues(data->mcBodyText, XmNcolumns, &columns, NULL);
    XtVaSetValues(data->mcBodyText, XmNcolumns, (columns + 1) / 2, NULL);
  }
  XtManageChild(data->mcBodyText);

  form[NUM] = XtParent(data->mcBodyText);
  XtManageChildren(form, NUM + 1);
  /* XtManageChild(form[NUM]); Unfortunately, this makes the window be too
     small. */

#ifdef DEBUG_jwz
  /* We have to indirect through a timer, instead of calling fe_from_addr_cb
     here, because some of the things fe_from_addr_cb does won't work yet --
     so by using a timer, we execute them as soon as the app returns to the
     command loop, and all is well.
   */
  XtAppAddTimeOut (fe_XtAppContext, 0, fe_set_from_addr_timer_kludge, pane);
#endif
}



#ifdef DEBUG_jwz
/* hairball multi-account kludge */

extern const char *MSG_CompositionWindowNewsgroups(MWContext *context);

static void
fe_from_addr_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MSG_HEADER_SET msgtype = MSG_FROM_HEADER_MASK;
  MWContext* context = fe_WidgetToMWContext(widget);
  int i = (int) closure;
  char *value = strdup(from_addrs[i]);
  char *mailbox = value;
  char *fcc = strchr(mailbox, '\t');
  char *org = fcc ? strchr(fcc+1, '\t') : 0;

  Bool dont_show_fcc = (call_data == 0);

  if (fcc) *fcc++ = 0;
  if (org) *org++ = 0;

  XP_ASSERT (context);
  if (!context) return;
  last_addr = i;

  MSG_SetHeaderContents (context, msgtype, mailbox);

  free(fe_globalPrefs.email_address);
  fe_globalPrefs.email_address = extract_addr(mailbox);

# ifdef DEBUG
  fprintf (real_stderr, "setting return address to %s%s\n",
           fe_globalPrefs.email_address,
           (dont_show_fcc ? " (early)" : ""));
# endif

  if (!MSG_CompositionWindowNewsgroups(context))  /* if news, don't change */
    {
      char *old_fcc = 0;
      const char *slash;
      Widget w = CONTEXT_DATA(context)->mcFcc;

      slash = strrchr(fcc, '/');

      XtVaGetValues(w, XmNvalue, &old_fcc, 0);

      if (!!strncmp (fcc, old_fcc, slash - fcc))
	{				/* doesn't have correct prefix */
          if (fe_globalPrefs.mail_fcc)
            free (fe_globalPrefs.mail_fcc);
          fe_globalPrefs.mail_fcc = strdup(fcc);

          MSG_SetHeaderContents (context, MSG_FCC_HEADER_MASK, fcc);
          XtVaSetValues(w, XmNvalue, fcc, 0);
# ifdef DEBUG
          fprintf(real_stderr,"setting FCC to %s (was %s)\n", fcc, old_fcc);
# endif
	}
# ifdef DEBUG
      else
        fprintf(real_stderr,"FCC is unchanged (%s).\n", old_fcc);
# endif

      if (!!strcmp (org, fe_globalPrefs.organization))
	{
          free (fe_globalPrefs.organization);
          fe_globalPrefs.organization = strdup(org);
# ifdef DEBUG
          fprintf(real_stderr,"setting org to \"%s\"\n",
                  fe_globalPrefs.organization);
# endif
	}
# ifdef DEBUG
      else
        fprintf(real_stderr, "organization is unchanged (%s).\n",
                fe_globalPrefs.organization);
# endif


      if (w && !dont_show_fcc && !XtIsManaged(XtParent(w)))
	MSG_Command(context, MSG_ShowFCC);
    }

  free (value);
}
#endif



void
FE_MsgShowHeaders (MWContext *context, MSG_HEADER_SET headers)
{
  fe_ContextData* data = CONTEXT_DATA(context);
  int i;
  int oncount = 0;
  int offcount = 0;
  Widget on[20];
  Widget off[20];
  XP_ASSERT(context->type == MWContextMessageComposition);
  for (i=0 ; i<NUM ; i++) {
    Widget widget = *((Widget*) ((char*) data + description[i].offset));
    Widget parent = XtParent(widget);
    if (headers & description[i].msgtype) {
      on[oncount++] = parent;
    } else {
      off[offcount++] = parent;
    }
  }
  if (offcount) XtUnmanageChildren(off, offcount);
  if (oncount) XtManageChildren(on, oncount);
}

#undef NUM
#undef ADDBROWSE
#undef ISLABEL
#undef Off

/*
 * This makes an empty browser context widgets for now
 */
void
fe_MakeChromeWidgets(Widget shell, MWContext *context)
{
  Arg av [20];
  int ac = 0;

  Widget mainw;
  Widget   pane = 0;	/* Layout uses this a lot */
  Widget scroller = 0;

  fe_ContextData* data = CONTEXT_DATA(context);

  data->widget = shell;

  XmGetColors (XtScreen (shell),
		 fe_cmap(context),
		 data->default_bg_pixel,
		 &data->fg_pixel,
		 &data->top_shadow_pixel,
		 &data->bottom_shadow_pixel,
		 NULL); 

  /* "mainw" is the parent of the top level of vertically stacked areas. */
  ac = 0;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  if (data->hasChrome) {
    XtSetArg (av[ac], XmNnoResize, !data->chrome.allow_resize); ac++;
  }
  XtSetArg (av[ac], XmNborderWidth, 0); ac++;
  mainw = XmCreateForm (shell, "mainform", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNborderWidth, 0); ac++;
  XtSetArg (av[ac], XmNmarginWidth, 0); ac++;
  XtSetArg (av[ac], XmNmarginHeight, 0); ac++;
  pane = XmCreatePanedWindow (mainw, "pane", av, ac);

  XtVaSetValues (pane,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
  		 XmNbottomAttachment, XmATTACH_FORM,
		 0);

  /* The actual work area */
  scroller = fe_MakeScrolledWindow (context, pane, "scroller");
  XtVaSetValues (scroller, XmNborderWidth, 0, 0);


  if (scroller)
    XtManageChild (scroller);
  XtManageChild (pane);
  XtManageChild (mainw);

  data->main_pane = pane;

  if (scroller) {
    XtVaSetValues (scroller, XmNinitialFocus,
		   data->drawing_area, 0);
    XtVaSetValues (pane, XmNinitialFocus, scroller, 0);
  }
  XtVaSetValues (mainw, XmNinitialFocus, pane, 0);
  XtVaSetValues (shell, XmNinitialFocus, mainw, 0);

  fe_HackTranslations (context, shell);
}

/*
 * Hacks to create a new context. The DOs and DONTs.
 * 1. If you want the logo, then the logo's size depends
 *    on the existence of more than one of location, toolbar,
 *    menubar, directory_buttons. If one-only or none exists,
 *    then logo will be small.
 * 2. Absolutely needed for fe_ContextData:
 *	widget		: shell
 *	mainw		: form
 *	top_area	: form (for logo to work)
 *	pane 		: pane (for everything else to work)
 *	drawing_area	: drawing area (for thermo stuff to work)
 */
void
fe_MakeSaveToDiskContextWidgets(Widget shell, MWContext *context)
{
  Arg av [20];
  int ac = 0;

  Widget mainw;
  Widget  top_area;
  Widget   logo = 0;
  Widget   pane = 0;
  Widget    saving_label, saving_value, url_label, url_value;
  Widget   messb = 0;
  Widget    cancel_button;
  Widget  line2;
  Widget  dashboard;
#ifdef HAVE_SECURITY
  Widget   sec_logo = 0;
#endif
  Widget   mouse_doc, thermo, slider;

  Dimension logo_width = 0, logo_height = 0;
  Dimension st = 0;
  Pixmap logo_pixmap;
  Pixel pix = 0;
  Widget kids[20];

  fe_ContextData* data = CONTEXT_DATA(context);

  data->widget = shell;

  XmGetColors (XtScreen (shell),
               fe_cmap(context),
               data->default_bg_pixel,
               &data->fg_pixel,
               &data->top_shadow_pixel,
               &data->bottom_shadow_pixel,
	       NULL); 

  /* "mainw" is the parent of the top level of vertically stacked areas.
   */
  ac = 0;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  if (data->hasChrome) {
    XtSetArg (av[ac], XmNnoResize, !data->chrome.allow_resize); ac++;
  }
  mainw = XmCreateForm (shell, "download", av, ac);

  ac = 0;
  top_area = XmCreateForm (mainw, "topArea", av, ac);

  /* fe_LogoPixmap() Needs this early... */
  data->top_area = top_area;

  XtVaGetValues (top_area, XmNbackground, &pix, 0);

  logo_pixmap = fe_LogoPixmap (context, &logo_width, &logo_height, TRUE);

  ac = 0;
  XtSetArg (av [ac], XmNbackground, pix); ac++;
  XtSetArg (av [ac], XmNwidth, logo_width); ac++;
  XtSetArg (av [ac], XmNheight, logo_height); ac++;
  XtSetArg (av [ac], XmNresizable, False); ac++;
  XtSetArg (av [ac], XmNlabelType, XmPIXMAP); ac++;
  XtSetArg (av [ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
  XtSetArg (av [ac], XmNlabelPixmap, logo_pixmap); ac++;
  XtSetArg (av [ac], XmNlabelInsensitivePixmap, logo_pixmap); ac++;
  /* Can't be a gadget, so we can draw on its window. */
  logo = XmCreatePushButton (top_area, "logo", av, ac);

  XtVaGetValues (logo, XmNshadowThickness, &st, 0);
  XtVaSetValues (logo, XmNwidth,  logo_width  + st + st,
			XmNheight, logo_height + st + st, 0);

  ac = 0;
  pane = XmCreateForm(top_area, "pane", av, ac);
  url_label = XmCreateLabelGadget (pane, "downloadURLLabel", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNeditable, False); ac++;
  url_value = fe_CreateTextField (pane, "downloadURLValue", av, ac);
  saving_label = XmCreateLabelGadget(pane, "downloadFileLabel", av, ac);
  ac = 0;
  XtSetArg (av [ac], XmNeditable, False); ac++;
  saving_value = fe_CreateTextField(pane, "downloadFileValue", av, ac);

  /* To center the cancel button we create a MessageBox around it */
  ac = 0;
  XtSetArg (av [ac], XmNdialogType, XmDIALOG_TEMPLATE); ac++;
  messb = XmCreateMessageBox(top_area, "messagebox", av, ac);
  ac = 0;
  cancel_button = XmCreatePushButtonGadget(messb, "cancel", av, ac);
  XtAddCallback(cancel_button, XmNactivateCallback, fe_AbortCallback, context);
  XtManageChild(cancel_button);

  /* We have to do this explicitly because of AIX versions come
     with these buttons by default */
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_SEPARATOR));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_OK_BUTTON));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_CANCEL_BUTTON));
  fe_UnmanageChild_safe(XmMessageBoxGetChild(messb, XmDIALOG_HELP_BUTTON));

  ac = 0;
  line2 = XmCreateSeparatorGadget (mainw, "line2", av, ac);

  ac = 0;
  dashboard = XmCreateForm (mainw, "dashboard", av, ac);

  /* fe_SecurityPixmap() needs this early. */
  data->dashboard = dashboard;

  /* The wholine
   */
#ifdef HAVE_SECURITY
  {
    Dimension w, h;
    Pixmap p = fe_SecurityPixmap (context, &w, &h, 0);
    ac = 0;
    XtSetArg (av [ac], XmNresizable, False); ac++;
    XtSetArg (av [ac], XmNlabelType, XmPIXMAP); ac++;
    XtSetArg (av [ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
    XtSetArg (av [ac], XmNlabelPixmap, p); ac++;
    XtSetArg (av [ac], XmNwidth, w); ac++;
    XtSetArg (av [ac], XmNheight, h); ac++;
    XtSetArg (av [ac], XmNshadowThickness, 2); ac++;
    sec_logo = XmCreatePushButtonGadget (dashboard, "docinfoLabel", av, ac);
    XtAddCallback (sec_logo, XmNactivateCallback, fe_sec_logo_cb, context);
  }
#endif /* !HAVE_SECURITY */

  ac = 0;
  mouse_doc = XmCreateLabelGadget (dashboard, "mouseDocumentation", av, ac);

  /* The status thermometer. */
  ac = 0;
  thermo = XmCreateForm (dashboard, "thermo", av, ac);
  slider = XmCreateDrawingArea (thermo, "slider", av, ac);
  XtAddCallback (slider, XmNexposeCallback, fe_expose_thermo_cb, context);
  XtManageChild (slider);

  if (context->type == MWContextSaveToDisk) {
    /* CHEAT CHEAT CHEAT... I dont know why fe_MakeLEDGCs is
       using this to make the gc for the thermo stuff. I will
       sail with the wind for now. Unsetting of this is done
       in the url exit routine. Probably. Ah! it doesn't matter */
    data->drawing_area = slider;
  }


  /* =======================================================================
     Attachments.  This has to be done after the widgets are created,
     since they're interdependent, sigh.
     =======================================================================
   */
  XtVaSetValues(url_label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		0);
  XtVaSetValues(url_value,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, url_label,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, url_label,
		0);
  XtVaSetValues(saving_label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, url_value,
		0);
  XtVaSetValues(saving_value,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, saving_label,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, saving_label,
		XmNrightAttachment, XmATTACH_FORM,
		0);
  XP_ASSERT(url_label->core.width > 0 && saving_label->core.width > 0);
  if (url_label->core.width < saving_label->core.width)
    XtVaSetValues (url_label, XmNwidth, saving_label->core.width, 0);
  else
    XtVaSetValues (saving_label, XmNwidth, url_label->core.width, 0);

  XtVaSetValues (url_label, XmNheight, url_value->core.height, 0);
  XtVaSetValues (saving_label, XmNheight, saving_value->core.height, 0);

  XtVaSetValues (top_area,
		 XmNtopAttachment,XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_WIDGET,
		 XmNbottomWidget, line2,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  XtVaSetValues (pane,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_WIDGET,
  		 XmNrightWidget, logo,
		 0);
  XtVaSetValues(messb,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, pane,
		XmNbottomAttachment, XmATTACH_FORM,
		0);
  XtVaSetValues (line2,
		 XmNbottomAttachment, XmATTACH_WIDGET,
		 XmNbottomWidget, dashboard,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (logo,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

  XtVaSetValues (dashboard,
		 XmNtopAttachment, XmATTACH_NONE,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);

#ifdef HAVE_SECURITY
  if (sec_logo) {
    XtVaGetValues (sec_logo, XmNshadowThickness, &st, 0);
    XtVaSetValues (sec_logo,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_NONE,
		   XmNtopOffset, st, XmNbottomOffset, st,
		   0);
  }
#endif /* HAVE_SECURITY */

  XtVaSetValues (mouse_doc,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
#ifdef HAVE_SECURITY
		 XmNleftAttachment, (sec_logo
				       ? XmATTACH_WIDGET : XmATTACH_FORM),
		 XmNleftWidget, sec_logo,
#else  /* !HAVE_SECURITY */
		 XmNleftAttachment, XmATTACH_FORM,
#endif /* !HAVE_SECURITY */
		 XmNrightAttachment, (thermo
				       ? XmATTACH_WIDGET : XmATTACH_FORM),
		 XmNrightWidget, thermo,
		 0);
    XtVaSetValues (thermo,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);
    XtVaGetValues (thermo, XmNshadowThickness, &st, 0);
    XtVaSetValues (slider,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   XmNtopOffset, st, XmNbottomOffset, st,
		   XmNleftOffset, st, XmNrightOffset, st,
		   0);
  /* ========================================================================
     Managing the widgets
     ========================================================================
   */
  ac = 0;
  kids[ac++] = url_label;	/* pane */
  kids[ac++] = url_value;
  kids[ac++] = saving_value;
  kids[ac++] = saving_label;
  XtManageChildren (kids, ac);

  XtManageChild(pane);		/* top_area */
  XtManageChild(messb);
  XtManageChild(logo);
  XtManageChild(top_area);

  ac = 0;			/* dashboard */
  kids[ac++] = thermo;
  XtManageChild (slider);
#ifdef HAVE_SECURITY
  if (sec_logo) kids[ac++] = sec_logo;
#endif /* !HAVE_SECURITY */
  kids[ac++] = mouse_doc;
  XtManageChildren (kids, ac);
  XtManageChild(dashboard);

  XtManageChild (line2);
  XtManageChild (mainw);

  data->logo = logo;
  data->top_area = top_area;
  data->main_pane = pane;
  data->wholine = mouse_doc;
  data->thermo = slider;
  data->dashboard = dashboard;
#ifdef HAVE_SECURITY
  data->security_logo = sec_logo;
#endif

  /* MWContextSaveToDisk uses two of the existing widgets:
   *		url_text
   *		url_label
   * for its own purpose. These will be set by fe_MakeSaveAsStream
   * as that is (only) where the saving filename is available.
   */
  data->url_text = url_value;
  data->url_label = saving_value;

  if (XtWindow (mainw))
    fe_MakeLEDGCs (context);

  fe_FixLogoMargins (context);

  XtVaSetValues (mainw, XmNinitialFocus, pane, 0);
  XtVaSetValues (shell, XmNinitialFocus, mainw, 0);

 fe_HackTranslations (context, shell);
}

#ifdef JAVA

#include <Xm/Label.h>

/*
** MOTIF sucks. What else can you say?
*/
Widget
FE_MakeAppletSecurityChrome(Widget parent, char* warning)
{
    Widget warningWindow, label, form, sep, skinnyDrawingArea;
    XmString cwarning;
    Arg av[20];
    Widget sec_logo = 0;
    int ac = 0;
    MWContext* someRandomContext = NULL;
    Colormap cmap;
    Pixel fg;
    Pixel bg;

    someRandomContext = XP_FindContextOfType(NULL, MWContextBrowser);
    if (!someRandomContext)
	someRandomContext = XP_FindContextOfType(NULL, MWContextMail);
    if (!someRandomContext)
	someRandomContext = XP_FindContextOfType(NULL, MWContextNews);
    if (!someRandomContext)
	someRandomContext = fe_all_MWContexts->context;

    cmap = fe_cmap(someRandomContext);
    fg = CONTEXT_DATA(someRandomContext)->fg_pixel;
    bg = CONTEXT_DATA(someRandomContext)->default_bg_pixel;

    ac = 0;
    XtSetArg(av[ac], XmNspacing, 0); ac++;
    XtSetArg(av[ac], XmNheight, 20); ac++; /* XXX motif sucks */
    XtSetArg(av[ac], XmNmarginHeight, 0); ac++;
    XtSetArg(av[ac], XmNmarginWidth, 0); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    XtSetArg(av[ac], XmNforeground, fg); ac++;
    XtSetArg(av[ac], XmNbackground, bg); ac++;
    XtSetArg(av[ac], XmNresizable, False); ac++;
    warningWindow = XmCreateForm(parent, "javaWarning", av, ac);

    /* We make this widget to give the seperator something to seperate. */
    ac = 0;
    XtSetArg(av[ac], XmNheight, 1); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    XtSetArg(av[ac], XmNbackground, bg); ac++;
    XtSetArg(av[ac], XmNresizePolicy, XmRESIZE_NONE); ac++;
    skinnyDrawingArea = XmCreateDrawingArea(warningWindow, "javaDA", av, ac);

    ac = 0;
    sep = XmCreateSeparatorGadget(warningWindow, "javaSep", av, ac);

    ac = 0;
    XtSetArg(av[ac], XmNspacing, 0); ac++;
    XtSetArg(av[ac], XmNmarginHeight, 0); ac++;
    XtSetArg(av[ac], XmNmarginWidth, 0); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    XtSetArg(av[ac], XmNforeground, fg); ac++;
    XtSetArg(av[ac], XmNbackground, bg); ac++;
    XtSetArg(av[ac], XmNresizable, False); ac++;
    form = XmCreateForm(warningWindow, "javaForm", av, ac);

    cwarning = XmStringCreate (warning, XmFONTLIST_DEFAULT_TAG);
    ac = 0;
    XtSetArg(av[ac], XmNhighlightThickness, 0); ac++;
    XtSetArg(av[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(av[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    XtSetArg(av[ac], XmNforeground, fg); ac++;
    XtSetArg(av[ac], XmNbackground, bg); ac++;
    XtSetArg(av[ac], XmNlabelString, cwarning); ac++;
    label = XmCreateLabelGadget (form, "mouseDocumentation", av, ac);
    XmStringFree(cwarning);

#ifdef HAVE_SECURITY
    {
	Dimension w, h;
	Pixmap p = fe_SecurityPixmap(NULL, &w, &h, 0);
	ac = 0;
	XtSetArg(av[ac], XmNresizable, False); ac++;
	XtSetArg(av[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(av[ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(av[ac], XmNlabelPixmap, p); ac++;
	XtSetArg(av[ac], XmNwidth, w); ac++;
	XtSetArg(av[ac], XmNheight, h); ac++;
	XtSetArg(av[ac], XmNcolormap, cmap); ac++;
	XtSetArg (av [ac], XmNshadowThickness, 2); ac++;
	sec_logo = XmCreatePushButtonGadget (form, "javaSecLogo", av, ac);
    }
#endif /* !HAVE_SECURITY */

    /* Now that widgets have been created, do the form attachments */
    XtVaSetValues(skinnyDrawingArea,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_NONE,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  0);

    XtVaSetValues(sep,
		  XmNtopAttachment, XmATTACH_WIDGET,
		  XmNtopWidget, skinnyDrawingArea,
		  XmNbottomAttachment, XmATTACH_NONE,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  0);

    XtVaSetValues(form,
		  XmNtopAttachment, XmATTACH_WIDGET,
		  XmNtopWidget, sep,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  0);

#ifdef HAVE_SECURITY
    if (sec_logo) {
	XtVaSetValues(sec_logo,
		      XmNtopAttachment, XmATTACH_FORM,
		      XmNbottomAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNleftOffset, 4,
		      XmNrightAttachment, XmATTACH_NONE,
		      0);
    }
#endif

    XtVaSetValues(label,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_FORM,
#ifdef HAVE_SECURITY
		  XmNleftAttachment, XmATTACH_WIDGET,
		  XmNleftWidget, sec_logo,
#else  /* !HAVE_SECURITY */
		  XmNleftAttachment, XmATTACH_FORM,
#endif /* !HAVE_SECURITY */
		  XmNrightAttachment, XmATTACH_FORM,
		  0);

    XtManageChild(label);
    if (sec_logo)
	XtManageChild(sec_logo);
    XtManageChild(form);
    XtManageChild(sep);
    XtManageChild(skinnyDrawingArea);
    XtManageChild(warningWindow);
    return warningWindow;
}
#endif

MSG_BIFF_STATE biffstate = MSG_BIFF_Unknown;

static void
fe_update_biff_icon(Widget widget)
{
  if (widget) {
    XtVaSetValues(widget, XmNlabelPixmap,
		  biffstate == MSG_BIFF_NewMail ? biff_yes.pixmap :
		  biffstate == MSG_BIFF_NoMail ? biff_no.pixmap :
		  biff_unknown.pixmap,
		  0);
  }
}

void FE_UpdateBiff(MSG_BIFF_STATE state)
{
  struct fe_MWContext_cons *cons;
  if (biffstate != state) {
    biffstate = state;
    for (cons = fe_all_MWContexts ; cons ; cons = cons->next) {
      fe_update_biff_icon(CONTEXT_DATA(cons->context)->bifficon);

      /* Change the desktop icon for Mail and MessageComposition */
      if ((cons->context->type == MWContextMail) ||
	  (cons->context->type == MWContextMessageComposition)) {
	fe_InitIcons(cons->context);
      }
    }
  }
}




static void
fe_biff_clicked(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;

  /* Takes advantage of the fact that fe_MakeWindow will just return an
     existing mail context rather than create a new one.*/
  MWContext* mail_context =
    fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)), NULL,
		  0, NULL, MWContextMail, FALSE);

  fe_mn_getnewmail(NULL, mail_context, NULL);
}


/*
 * Sash Geometry is saved as
 * 	<pane-config>: w x h [;<pane-config>:wxh]
 *           %d       %dx%d
 *	or
 *	w x h
 *	%dx%d
 * for each type of pane configuration we store the widths and heights
 */

static char *
fe_ParseSashGeometry(char *old_geom_str, int pane_config,
			unsigned int *w, unsigned int *h, Boolean find_mode_p)
{
    char *geom_str = old_geom_str;
    char buf[100], *s, *t;
    int n;
    int tmppane, tmpw, tmph;

    char new_geom_str[512];
    int len = 0;

    new_geom_str[0] = '\0';

    while (geom_str && *geom_str) {

	for (s=buf, t=geom_str; *t && *t != ';'; ) *s++=*t++;
	*s = '\0';
	if (*t == ';') t++;
	geom_str = t;
	/* At end of this:
	 * geom_str - points to next section
	 * buf - has copy of this section
	 * t, s - tmp
	 */

	for (s = buf; *s && *s != ':'; s++);
	if (*s == ':') { 	/* New format */
	    n = sscanf(buf,"%d:%dx%d", &tmppane, &tmpw, &tmph);
	    if (n < 3) n = 0;
	}
	else {			/* Old format */
	    n = sscanf(buf,"%dx%d", &tmpw, &tmph);
	    if (n < 2) n = 0;
	    tmppane = FE_PANES_NORMAL;
	}
	/* At the end of this:
	 * n - '0' indicates badly formed section
	 * tmppane, tmpw, tmph - if n > 0 indicate %d:%dx%d
	 */

	if (find_mode_p && (n > 0) && tmppane == pane_config) {
	    *w = tmpw;
	    *h = tmph;
	    return(old_geom_str);
	}
	if (n != 0 && tmppane != pane_config) {
	    if (len != 0) {
		PR_snprintf(&new_geom_str[len], sizeof(new_geom_str)-len, ";");
		len++;
	    }
	    PR_snprintf(&new_geom_str[len], sizeof(new_geom_str)-len, 
				"%d:%dx%d", tmppane, tmpw, tmph);
	    len = strlen(new_geom_str);
	}
    }
    if (find_mode_p)
	return(NULL);
    else {
	if (*new_geom_str)
	    s = PR_smprintf("%d:%dx%d;%s", pane_config, *w, *h, new_geom_str);
	else
	    s = PR_smprintf("%d:%dx%d", pane_config, *w, *h);
	return(s);
    }
}

char *
fe_MakeSashGeometry(char *old_geom_str, int pane_config,
			unsigned int w, unsigned int h)
{
    char *new_geom_str =
	fe_ParseSashGeometry(old_geom_str, pane_config, &w, &h, False);
    return(new_geom_str);
}


void
fe_GetSashGeometry(char *geom_str, int pane_config,
			unsigned int *w, unsigned int *h)
{
    (void) fe_ParseSashGeometry(geom_str, pane_config, w, h, True);
    return;
}

static XtPointer
fe_tooltip_mappee(Widget widget, XtPointer data)
{
    if (XtIsSubclass(widget, xmManagerWidgetClass)) {
	if (fe_IsOptionMenu(widget)) {
	    fe_WidgetAddToolTips(widget);
	} else if (fe_ManagerCheckGadgetToolTips(widget, NULL)) {
	    fe_ManagerAddGadgetToolTips(widget, NULL);
	}
    } else if (XtIsSubclass(widget, xmPushButtonWidgetClass)
	       ||
	       XtIsSubclass(widget, xmToggleButtonWidgetClass)
	       ||
	       XtIsSubclass(widget, xmCascadeButtonWidgetClass)) {
	fe_WidgetAddToolTips(widget);
    }

    return NULL;
}



static XtPointer
fe_documentString_mappee(Widget widget, XtPointer data)
{
    if (XtIsSubclass(widget, xmPushButtonWidgetClass) ||
	XtIsSubclass(widget, xmToggleButtonWidgetClass) ||
	/* Cover the Gadgets too */
	XtIsSubclass(widget, xmPushButtonGadgetClass) ||
	XtIsSubclass(widget, xmToggleButtonGadgetClass))
      {
	fe_WidgetAddDocumentString((MWContext *)data, widget);
      }
  
    return NULL;
}

void
fe_MakeWidgets (Widget shell, MWContext *context)
{
  Arg av [22];
  int ac = 0;

  Widget mainw;
  Widget menubar = 0;
  Widget top_area = 0, top_left_area = 0, url_label = 0, url_text = 0;
  Widget mid_left_area = 0;
  Widget logo = 0;
  Dimension logo_width = 0, logo_height = 0;

  Widget list_pane = 0;
  Widget pane = 0;

#ifdef LEDGES
  Widget top_pane = 0, bottom_pane = 0;
  Widget top_ledge = 0, bottom_ledge = 0;
#endif
#ifdef HAVE_SECURITY
  Widget sec_logo = 0, sec_led = 0;
#endif
  Widget scroller = 0, dashboard = 0, mouse_doc = 0, thermo = 0, slider = 0;
  Widget led_base = 0, led = 0;
  Widget line1 = 0, line2 = 0;
  Widget toolbar = 0;
  Widget urlbar = 0;
  Widget bifficon = 0;
  Widget top_guy = 0;

  Widget scroller_parent = 0;

  Dimension st = 0;

  fe_ContextData* data = CONTEXT_DATA(context);

  data->widget = shell;

#ifdef DEBUG_jwz
  if (context->type == MWContextMail ||
      context->type == MWContextNews)
    data->autoload_images_p = False;
#endif /* DEBUG_jwz */

  {
    XmGetColors (XtScreen (shell),
		 fe_cmap(context),
		 data->default_bg_pixel,
		 &data->fg_pixel,
		 &data->top_shadow_pixel,
		 &data->bottom_shadow_pixel,
		 NULL);
  }

  /* "mainw" is the parent of the top level of vertically stacked areas.
   */
  ac = 0;
  {
    Widget *kids;
    int nkids = 0;
    XtVaGetValues (shell, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
    if (nkids)
      {
	/* Aha!  The shell already has kids; that must mean we're being
	   called by fe_rebuild_window() and should just reuse the old
	   widgets as much as possible, to avoid thrashing the shell.
	   This is a COMPLETE HACK and MOTIF REALLY REALLY SUCKS.

	   We still have to re-make mainw (there seems to be no way to
	   reuse it and have the attachments still work) but in order
	   to keep the shell the same size, we need to first extract
	   the width and height from the old mainw and store them into
	   the new one.
	 */
	Dimension width = 0, height = 0;
	while (nkids--)
	  {
		if (!strcmp(XtName(*kids), "form"))
		  {
			mainw = *kids;
			break;
		  }
		kids++;
	  }
	XtVaGetValues (mainw, XmNwidth, &width, XmNheight, &height, 0);
	if (width <= 0 || height <= 0) abort ();
	XtSetArg (av[ac], XmNwidth, width); ac++;
	XtSetArg (av[ac], XmNheight, height); ac++;
	XtDestroyWidget (mainw);

	/* After destroying we should clear all widget store variables. */
	/* XXX There are tons of these. Will need to add all of them - dp */
	data->back_button    = 0;		/* Toolbar */
	data->home_button    = 0;
	data->abort_button   = 0;
	data->print_button   = 0;
	data->delayed_button = 0;
	data->forward_button = 0;

	data->menubar        = 0;

	data->bookmark_menu  = 0;
	data->windows_menu   = 0;
	data->history_menu   = 0;

	data->delayed_menuitem = 0;
	data->abort_menuitem = 0;
	data->back_menuitem = 0;
	data->forward_menuitem = 0;
	data->home_menuitem = 0;
	data->delete_menuitem = 0;
	data->back_popupitem = 0;
	data->forward_popupitem = 0;
	data->cut_menuitem = 0;
	data->copy_menuitem = 0;
	data->paste_menuitem = 0;
	data->paste_quoted_menuitem = 0;
	data->findAgain_menuitem = 0;
	data->reloadFrame_menuitem = 0;
	data->mailto_menuitem = 0;
	data->saveAs_menuitem = 0;
	data->uploadFile_menuitem = 0;
	data->print_menuitem = 0;

	data->url_label = 0;
	data->url_text = 0;
	data->logo = 0;
	data->top_area = 0;
	data->toolbar = 0;
	data->dashboard = 0;
#ifdef LEDGES
	data->top_ledge = 0;
	data->bottom_ledge = 0;
#endif
#ifdef HAVE_SECURITY
	data->security_bar = 0;
	data->security_logo = 0;
#endif
#ifdef JAVA
	data->show_java = 0;
#endif
	data->main_pane = 0;
	data->drawing_area = 0;
	data->scrolled = 0;
	data->hscroll = 0;
	data->vscroll = 0;
	data->wholine = 0;
	data->thermo = 0;
	data->LED = 0;
	data->bifficon = 0;
	data->thermo_gc = 0;
	data->LED_gc = 0;
	data->folderform = 0;
	data->messageform = 0;
	data->folderlist = 0;
	data->messagelist = 0;
	data->hysteresis = 0;
      }
  }
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  if (data->hasChrome) {
    XtSetArg (av[ac], XmNnoResize, !data->chrome.allow_resize); ac++;
  }
  mainw = XmCreateForm (shell, "form", av, ac);

  if (data->show_menubar_p)
    menubar = fe_MakeMenubar (mainw, context);

  if (
       data->show_toolbar_p
       ||
       data->show_directory_buttons_p
       ||
       data->show_url_p
#ifdef EDITOR
       ||
       data->show_character_toolbar_p
       ||
       data->show_paragraph_toolbar_p
#endif /*EDITOR*/
       ) {
      /* Title area
       */
#if 1
      Pixmap logo_pixmap;
#endif
      int count = 0;
      Pixel bg = 0;

      ac = 0;
      top_area = XmCreateForm (mainw, "topArea", av, ac);

      /* fe_LogoPixmap() Needs this early... */
      data->top_area = top_area;

      if (data->show_toolbar_p
	  ||
	  data->show_directory_buttons_p
	  ||
	  data->show_url_p) {
        ac = 0;
	top_left_area = XmCreateForm (top_area, "topLeftArea", av, ac);
	if (data->show_toolbar_p)
        {
#ifdef EDITOR
	  if (context->type == MWContextEditor
	      &&
	      !(data->show_toolbar_icons_p && data->show_toolbar_text_p)) {
	    Widget f;
	    ac = 0;
	    XtSetArg(av[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	    f = XmCreateFrame(top_left_area, "toolbarFrame", av, ac);
	    toolbar = fe_MakeToolbar(f, context, False);
	    XtManageChild(toolbar);
	    toolbar = f;
	  } else
#endif /*EDITOR*/
	  toolbar = fe_MakeToolbar(top_left_area, context, False);

	  count++;
	}
	if (data->show_url_p)
	{
	  url_label = XmCreateLabelGadget (top_left_area, "urlLabel", av, ac);
	  url_text = fe_CreateTextField (top_left_area, "urlText", av, ac);
	  count++;
	}
	if (data->show_directory_buttons_p)
	{
	  urlbar = fe_MakeToolbar (top_left_area, context, True);
	  count++;
	}
      }

#ifdef EDITOR
      if (data->show_character_toolbar_p || data->show_paragraph_toolbar_p) {

        mid_left_area = fe_EditorCreatePropertiesToolbar(context,
						 top_area, "midLeftArea");

	if (!top_left_area) {
	  top_left_area = mid_left_area;
	  mid_left_area = NULL;
	  count++;
	}
      }
#endif /*EDITOR*/

      if (count <= 0) abort ();

      XtVaGetValues (top_area, XmNbackground, &bg, 0);

#if 1
      logo_pixmap =
#endif
	fe_LogoPixmap (context, &logo_width, &logo_height, (count > 1));

      ac = 0;
      XtSetArg (av [ac], XmNbackground, bg); ac++;
      XtSetArg (av [ac], XmNwidth, logo_width); ac++;
      XtSetArg (av [ac], XmNheight, logo_height); ac++;
#if 0
      logo = XmCreateDrawingArea (top_area, "logo", av, ac);
      XtAddCallback (logo, XmNexposeCallback, fe_expose_logo_cb, context);
#else
      XtSetArg (av [ac], XmNresizable, False); ac++;
      XtSetArg (av [ac], XmNlabelType, XmPIXMAP); ac++;
      XtSetArg (av [ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
      XtSetArg (av [ac], XmNlabelPixmap, logo_pixmap); ac++;
      XtSetArg (av [ac], XmNlabelInsensitivePixmap, logo_pixmap); ac++;
      /* Can't be a gadget, so we can draw on its window. */
      logo = XmCreatePushButton (top_area, "logo", av, ac);

      /* Gag, now that we've created it, we can increase its height by the
	 size of its borders... */
      XtVaGetValues (logo, XmNshadowThickness, &st, 0);
      XtVaSetValues (logo,
		     XmNwidth,  logo_width  + st + st,
		     XmNheight, logo_height + st + st,
		     0);
      XtAddCallback (logo, XmNactivateCallback,
		     (
# if defined(__sgi)
		      fe_VendorAnim ? fe_SGICallback :
# endif
# if 1
		      fe_NetscapeCallback
# else
		      fe_ContestCallback
# endif
		      ),
		     context);
#endif
  }

  if (url_text)
    {
      History_entry *h = SHIST_GetCurrent (&context->hist);
      XtAddCallback (url_text, XmNactivateCallback, fe_url_text_cb, context);
      XtAddCallback (url_text, XmNmodifyVerifyCallback, fe_url_text_modify_cb,
		     context);
      XtVaSetValues (url_text,
		     XmNvalue, (h && h->address ? h->address : ""),
		     XmNcursorPosition, 0,
		     0);
    }

#ifdef HAVE_SECURITY
  /* In a client with security, there may be a colored bar between the top
     area and the main document (or the pane which contains the ledges and
     the document.)  If security is not present, this is just a thin
     separator line.
   */
  if (data->show_security_bar_p)
    {
      ac = 0;
      line1 = XmCreateForm (mainw, "securityBar", av, ac);
      ac = 0;
      sec_led = XmCreateDrawingArea (line1, "LED", av, ac);
      XtAddCallback (sec_led, XmNexposeCallback, fe_expose_security_bar_cb,
		     context);
    }
  else
#endif /* !HAVE_SECURITY */
       if (top_area)
    {
      ac = 0;
      line1 = XmCreateSeparatorGadget (mainw, "line1", av, ac);
    }

  /* Pane (parent of ledges and main work area)
   */
  ac = 0;
  if ((context->type == MWContextMail &&
       (fe_globalPrefs.mail_pane_style == FE_PANES_HORIZONTAL ||
	fe_globalPrefs.mail_pane_style == FE_PANES_TALL_FOLDERS)) ||
      ((context->type == MWContextNews &&
	(fe_globalPrefs.news_pane_style == FE_PANES_HORIZONTAL ||
	 fe_globalPrefs.news_pane_style == FE_PANES_TALL_FOLDERS))))

    pane = XmCreateHPanedWindow(mainw, "pane", av, ac);
  else
    pane = XmCreatePanedWindow(mainw, "pane", av, ac);

  scroller_parent = pane;

  if (context->type == MWContextMessageComposition) {
    fe_create_composition_widgets(context, pane);
    goto PANEPOPULATED;		/* ### I'm too lazy ### */
  }

  if (context->type == MWContextMail || context->type == MWContextNews) {
    static int mailFolderColumnWidths[] = {10, 6, 6};
    static int newsFolderColumnWidths[] = {10, 3, 6, 6};
    static int messageColumnWidths[] = {20, 3, 3, 30, 14};
    static char* mailFolderHeaders[] = {
      "Depth", "Name", "Unread", "Total"
    };
    static char* newsFolderHeaders[] = {
      "Depth", "Name", "Sub", "Unread", "Total"
    };
    static char* messageHeaders[] = {
      "Thread", "Sender", "Flag", "UnreadMsg", "Subject", "Date"
    };
    int n;
    int* widths;
    char** headers;
    unsigned int w = 200, h = 200;
    char *sash_geometry;
    int pane_style = 0;

    ac = 0;
    XtSetArg(av[ac], XmNallowResize, True) ; ac++;

    if ((context->type == MWContextMail &&
	 (fe_globalPrefs.mail_pane_style == FE_PANES_TALL_FOLDERS)) ||
	(context->type == MWContextNews &&
	 (fe_globalPrefs.news_pane_style == FE_PANES_TALL_FOLDERS)))
      {
	ac = 0;
	data->folderform = XmCreateForm(pane, "folderform", av, ac);
      }

    if ((context->type == MWContextMail &&
	 (fe_globalPrefs.mail_pane_style == FE_PANES_NORMAL)) ||
	(context->type == MWContextNews &&
	 (fe_globalPrefs.news_pane_style == FE_PANES_NORMAL)))
      list_pane = XmCreateHPanedWindow(pane, "listpane", av, ac);
    else
      list_pane = XmCreatePanedWindow(pane, "listpane", av, ac);

    if ((context->type == MWContextMail &&
	 (fe_globalPrefs.mail_pane_style == FE_PANES_TALL_FOLDERS)) ||
	(context->type == MWContextNews &&
	 (fe_globalPrefs.news_pane_style == FE_PANES_TALL_FOLDERS)))
      {
	ac = 0;
	data->messageform = XmCreateForm(list_pane, "messageform", av, ac);
	scroller_parent = list_pane;
      }
    else
      {
	ac = 0;
	data->folderform = XmCreateForm(list_pane, "folderform", av, ac);
	data->messageform = XmCreateForm(list_pane, "messageform", av, ac);
      }

    XtManageChild(data->folderform);
    XtManageChild(data->messageform);
    ac = 0;
    XtSetArg(av[ac], XmNallowResize, True) ; ac++;
    XtSetArg(av[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(av[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    if (context->type == MWContextNews) {
      n = 4;
      widths = newsFolderColumnWidths;
      headers = newsFolderHeaders;
    } else {
      n = 3;
      widths = mailFolderColumnWidths;
      headers = mailFolderHeaders;
    }
    data->folderlist =
      fe_OutlineCreate(context, data->folderform, "folderlist",
		       av, ac,
		       0, n, widths,
		       fe_folder_datafunc,
		       fe_folder_clickfunc,
		       fe_folder_iconfunc, context,
		       FE_DND_NONE,
		       (context->type == MWContextMail ?
			&fe_globalPrefs.mail_folder_columns :
			&fe_globalPrefs.news_folder_columns));
    fe_dnd_CreateDrop(data->folderlist, fe_folder_dropfunc, context);
    XtManageChild(data->folderlist);
    data->messagelist =
      fe_OutlineCreate(context, data->messageform, "messagelist",
		       av, ac, 5, 5, messageColumnWidths,
		       fe_message_datafunc,
		       fe_message_clickfunc,
		       fe_message_iconfunc, context,
		       (context->type == MWContextMail ?
			FE_DND_MAIL_MESSAGE :
			FE_DND_NEWS_MESSAGE),
		       (context->type == MWContextMail ?
			&fe_globalPrefs.mail_message_columns :
			&fe_globalPrefs.news_message_columns));
    fe_dnd_CreateDrop(data->messagelist, fe_message_dropfunc, context);
    XtManageChild(data->folderlist);
    XtManageChild(data->messagelist);
    XtManageChild(XtParent(data->folderlist));
    XtManageChild(XtParent(data->messagelist));
    XtManageChild(list_pane);
    fe_OutlineSetHeaders(data->folderlist, headers);
    fe_OutlineSetHeaders(data->messagelist, messageHeaders);

    /* Set sizes of panes */

    if (context->type == MWContextNews) {
      sash_geometry = fe_globalPrefs.news_sash_geometry;
      pane_style = fe_globalPrefs.news_pane_style;
    }
    else {
      sash_geometry = fe_globalPrefs.mail_sash_geometry;
      pane_style = fe_globalPrefs.mail_pane_style;
    }
    w = h = 0;
    if (sash_geometry && *sash_geometry)
      fe_GetSashGeometry(sash_geometry, pane_style, &w, &h);

    switch (pane_style) {
        case FE_PANES_NORMAL:   
	    if (w <= 0) w = 200;
	    if (h <= 0) h = 200;
            XtVaSetValues(data->folderform, XmNwidth, w, XmNheight, h, 0);
            XtVaSetValues(data->messageform, XmNheight, h, 0);
            break;
        case FE_PANES_HORIZONTAL:       
	    if (w <= 0) w = 200;
	    if (h <= 0) h = 100;
            XtVaSetValues(data->folderform, XmNwidth, w, XmNheight, h, 0);
            XtVaSetValues(data->messageform, XmNwidth, w, 0);
            break;
        case FE_PANES_STACKED:  
	    if (w <= 0) w = 200;
	    if (h <= 0) h = 100;
            XtVaSetValues(data->folderform, XmNheight, w, 0);
            XtVaSetValues(data->messageform, XmNheight, h, 0);
            break;
        case FE_PANES_TALL_FOLDERS:
	    XtVaSetValues(data->folderform, XmNwidth, w, 0);
	    XtVaSetValues(data->messageform, XmNheight, h, 0);
	    break;
        default:
	  abort();
	  break;
    }

    /* I dont know why this is there. It doesn't work without this  - dp */
    XtVaSetValues(data->folderlist, XmNwidth, 1, 0);
    XtVaSetValues(data->messagelist, XmNwidth, 1, 0);
  }


#ifdef LEDGES
  /* Top ledge (sometimes unmapped)
   */
  ac = 0;
  XtSetArg (av[ac], XmNskipAdjust, True); ac++;
  XtSetArg (av[ac], XmNbackground,
	    data->default_bg_pixel); ac++;
  top_pane  = XmCreateForm (pane, "topPane", av, ac);
  top_ledge = XmCreateDrawingArea (top_pane, "topLedge", av, ac);
  XtManageChild (top_ledge);
#endif

  /* The actual work area
   */
  scroller = fe_MakeScrolledWindow (context, scroller_parent, "scroller");

#ifdef LEDGES
  /* The bottom ledge (sometimes unmapped)
   */
  ac = 0;
  XtSetArg (av[ac], XmNskipAdjust, True); ac++;
  XtSetArg (av[ac], XmNbackground,
	    data->default_bg_pixel); ac++;
  bottom_pane  = XmCreateForm (pane, "bottomPane", av, ac);
  bottom_ledge = XmCreateDrawingArea (bottom_pane, "bottomLedge", av, ac);
  XtManageChild (bottom_ledge);
#endif

PANEPOPULATED:

  /* Create bottom status area */

  if (data->show_bottom_status_bar_p) {
    ac = 0;
    line2 = XmCreateSeparatorGadget (mainw, "line2", av, ac);

    ac = 0;
    dashboard = XmCreateForm (mainw, "dashboard", av, ac);

    /* fe_SecurityPixmap() needs this early. */
    data->dashboard = dashboard;

    /* The wholine
     */
#ifdef HAVE_SECURITY
    {
      Dimension w, h;
      Pixmap p = fe_SecurityPixmap (context, &w, &h, 0);
      ac = 0;
      XtSetArg (av [ac], XmNresizable, False); ac++;
      XtSetArg (av [ac], XmNlabelType, XmPIXMAP); ac++;
      XtSetArg (av [ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
      XtSetArg (av [ac], XmNlabelPixmap, p); ac++;
      XtSetArg (av [ac], XmNwidth, w); ac++;
      XtSetArg (av [ac], XmNheight, h); ac++;
      XtSetArg (av [ac], XmNshadowThickness, 2); ac++;
      sec_logo = XmCreatePushButtonGadget (dashboard, "docinfoLabel", av, ac);
      XtAddCallback (sec_logo, XmNactivateCallback, fe_sec_logo_cb, context);
    }
#endif /* !HAVE_SECURITY */

    ac = 0;
    mouse_doc = XmCreateLabelGadget (dashboard, "mouseDocumentation", av, ac);

    /* The status thermometer. */
    ac = 0;
    thermo = XmCreateForm (dashboard, "thermo", av, ac);
    slider = XmCreateDrawingArea (thermo, "slider", av, ac);
    XtAddCallback (slider, XmNexposeCallback, fe_expose_thermo_cb, context);
    XtManageChild (slider);

    /* The run light. */
    if (! logo)
    {
      ac = 0;
      led_base = XmCreateForm (dashboard, "power", av, ac);
      led = XmCreateDrawingArea (led_base, "LED", av, ac);
      XtAddCallback (led, XmNexposeCallback, fe_expose_LED_cb, context);
      XtManageChild (led);
    }

    /* The biff icon. */
    {
      Pixel bg = data->default_bg_pixel;
      XtVaGetValues (dashboard, XmNbackground, &bg, 0);

      fe_MakeIcon(context, bg, &biff_unknown, NULL,
		BiffU.width, BiffU.height, BiffU.mono_bits, BiffU.color_bits,
		BiffU.mask_bits, FALSE);
      fe_MakeIcon(context, bg, &biff_yes, NULL,
		BiffY.width, BiffY.height, BiffY.mono_bits, BiffY.color_bits,
		BiffY.mask_bits, FALSE);
      fe_MakeIcon(context, bg, &biff_no, NULL,
		BiffN.width, BiffN.height, BiffN.mono_bits, BiffN.color_bits,
		BiffN.mask_bits, FALSE);
      ac = 0;
      XtSetArg (av [ac], XmNresizable, False); ac++;
      XtSetArg (av [ac], XmNlabelType, XmPIXMAP); ac++;
      XtSetArg (av [ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
      XtSetArg (av [ac], XmNheight, biff_unknown.height); ac++;
      XtSetArg (av [ac], XmNlabelPixmap, biff_unknown.pixmap); ac++;
      XtSetArg (av [ac], XmNshadowThickness, 2); ac++;
      bifficon = XmCreatePushButtonGadget (dashboard, "biffButton", av, ac);
      XtAddCallback(bifficon, XmNactivateCallback, fe_biff_clicked, context);
      fe_update_biff_icon(bifficon);
    }
  }


  /* =======================================================================
     Attachments.  This has to be done after the widgets are created,
     since they're interdependent, sigh.
     =======================================================================
   */

  if (menubar)
    XtVaSetValues (menubar,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);
  if (top_area)
    XtVaSetValues (top_area,
		   XmNtopAttachment, (menubar
				      ? XmATTACH_WIDGET
				      : XmATTACH_FORM),
		   XmNtopWidget, menubar,
		   XmNbottomAttachment, XmATTACH_NONE,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);
  if (line1)
    XtVaSetValues (line1,
		   XmNtopAttachment, ((top_area || menubar)
				      ? XmATTACH_WIDGET
				      : XmATTACH_FORM),
		   XmNtopWidget, (top_area ? top_area : menubar),
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);
  XtVaSetValues (
		 pane,
		   XmNtopAttachment, ((top_area || menubar || line1)
				      ? XmATTACH_WIDGET
				      : XmATTACH_FORM),
		 XmNtopWidget, (line1 ? line1 : top_area ? top_area : menubar),
		 XmNbottomAttachment, (line2
				       ? XmATTACH_WIDGET
				       : XmATTACH_FORM),
  		 XmNbottomWidget, line2,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 0);
  if (line2)
    XtVaSetValues (line2,
		   XmNbottomAttachment, (dashboard
					 ? XmATTACH_WIDGET
					 : XmATTACH_FORM),
		   XmNbottomWidget, dashboard,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);

  /* Children of top_left_area */
  if (toolbar) {
    XtVaSetValues (toolbar,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_NONE,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, /*XmATTACH_FORM*/ XmATTACH_NONE,
		   0);
    top_guy = toolbar;
  }
  if (url_label) {
    XtVaSetValues(url_label,
		  XmNtopAttachment, (top_guy? XmATTACH_WIDGET: XmATTACH_FORM),
		  XmNtopWidget, top_guy,
		  XmNbottomAttachment, XmATTACH_NONE,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_NONE,
		  0);
  }
  if (url_text) {
    XtVaSetValues(url_text,
		  XmNtopAttachment, (top_guy? XmATTACH_WIDGET: XmATTACH_FORM),
		  XmNtopWidget, top_guy,
		  XmNbottomAttachment,
	             (data->show_directory_buttons_p
		      ? XmATTACH_NONE
		      : XmATTACH_FORM),
		  XmNleftAttachment, XmATTACH_WIDGET,
		  XmNleftWidget, url_label,
		  XmNrightAttachment, XmATTACH_FORM,
		  0);
    top_guy = url_text;
  }
  if (urlbar)
    XtVaSetValues(urlbar,
		  XmNtopAttachment, (top_guy? XmATTACH_WIDGET: XmATTACH_FORM),
		  XmNtopWidget, top_guy,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM /*XmATTACH_NONE*/,
		  0);
  if (top_left_area)
    XtVaSetValues (top_left_area,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment,
		     (mid_left_area? XmATTACH_WIDGET: XmATTACH_FORM),
		   XmNbottomWidget, mid_left_area,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, (logo ? XmATTACH_WIDGET :XmATTACH_FORM),
		   XmNrightWidget, logo,
		   0);
  if (logo)
    XtVaSetValues (logo,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment,
		     (mid_left_area? XmATTACH_WIDGET: XmATTACH_FORM),
		   XmNbottomWidget, mid_left_area,
		   XmNleftAttachment, XmATTACH_NONE,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);
   if (mid_left_area)
    XtVaSetValues (mid_left_area,
		   XmNtopAttachment, XmATTACH_NONE,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_NONE,
		   0);

  if (dashboard)
    XtVaSetValues (dashboard,
		   XmNtopAttachment, XmATTACH_NONE,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_FORM,
		   0);

#ifdef HAVE_SECURITY
  if (sec_logo) {
    XtVaGetValues (sec_logo, XmNshadowThickness, &st, 0);
    XtVaSetValues (sec_logo,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
		   XmNleftAttachment, XmATTACH_FORM,
		   XmNrightAttachment, XmATTACH_NONE,
		   XmNtopOffset, st, XmNbottomOffset, st,
		   0);
  }
#endif /* HAVE_SECURITY */

  if (mouse_doc)
    XtVaSetValues (mouse_doc,
		   XmNtopAttachment, XmATTACH_FORM,
		   XmNbottomAttachment, XmATTACH_FORM,
#ifdef HAVE_SECURITY
		   XmNleftAttachment, (sec_logo
				       ? XmATTACH_WIDGET : XmATTACH_FORM),
		   XmNleftWidget, sec_logo,
#else  /* !HAVE_SECURITY */
		   XmNleftAttachment, XmATTACH_FORM,
#endif /* !HAVE_SECURITY */
		   XmNrightAttachment, (thermo
				       ? XmATTACH_WIDGET : XmATTACH_FORM),
		   XmNrightWidget, thermo,
		   0);
  if (thermo)
    {
      XtVaSetValues (thermo,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_WIDGET,
		     XmNrightWidget, led_base ? led_base : bifficon,
		     0);
      XtVaGetValues (thermo, XmNshadowThickness, &st, 0);
      XtVaSetValues (slider,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_FORM,
		     XmNtopOffset, st, XmNbottomOffset, st,
		     XmNleftOffset, st, XmNrightOffset, st,
		     0);
    }
  if (led_base)
    {
      XtVaSetValues (led_base,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_WIDGET,
		     XmNrightWidget, bifficon,
		     0);
      XtVaGetValues (led_base, XmNshadowThickness, &st, 0);
      XtVaSetValues (led,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_FORM,
		     XmNtopOffset, st, XmNbottomOffset, st,
		     XmNleftOffset, st, XmNrightOffset, st,
		     0);
    }

  if (bifficon) {
    XtVaSetValues(bifficon,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopOffset, st, XmNbottomOffset, st,
		  0);
  }

#ifdef HAVE_SECURITY
  if (sec_led)
    {
      if (! line1) abort ();
      XtVaGetValues (line1, XmNshadowThickness, &st, 0);
      XtVaSetValues (sec_led,
		     XmNtopAttachment, XmATTACH_FORM,
		     XmNbottomAttachment, XmATTACH_FORM,
		     XmNleftAttachment, XmATTACH_FORM,
		     XmNrightAttachment, XmATTACH_FORM,
		     XmNtopOffset, st, XmNbottomOffset, st,
		     XmNleftOffset, st, XmNrightOffset, st,
		     0);
      XtManageChild (sec_led);
    }
#endif /* !HAVE_SECURITY */

#ifdef LEDGES
  XtVaGetValues (top_pane, XmNshadowThickness, &st, 0);
  XtVaSetValues (top_ledge,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNtopOffset, st, XmNbottomOffset, st,
		 XmNleftOffset, st, XmNrightOffset, st,
		 0);
  XtVaGetValues (bottom_pane, XmNshadowThickness, &st, 0);
  XtVaSetValues (bottom_ledge,
		 XmNtopAttachment, XmATTACH_FORM,
		 XmNbottomAttachment, XmATTACH_FORM,
		 XmNleftAttachment, XmATTACH_FORM,
		 XmNrightAttachment, XmATTACH_FORM,
		 XmNtopOffset, st, XmNbottomOffset, st,
		 XmNleftOffset, st, XmNrightOffset, st,
		 0);
#endif /* LEDGES */

  /* Silly URL label tricks. */
  if (url_label &&
      (!data->edited_label_string ||
       !data->unedited_label_string ||
       !data->netsite_label_string))
    {
      XtResource res[]
	= { { "editedLabelString", XmCXmString, XmRXmString, sizeof (XmString),
	      XtOffset (fe_ContextData *, edited_label_string),
	      XtRString, "Error!" },
	    { "uneditedLabelString", XmCXmString, XmRXmString,sizeof(XmString),
	      XtOffset (fe_ContextData *, unedited_label_string),
	      XtRString, "Error!" },
	    { "netsiteLabelString", XmCXmString, XmRXmString,sizeof(XmString),
	      XtOffset (fe_ContextData *, netsite_label_string),
	      XtRString, "Error!" } };
      XtGetApplicationResources (url_label, (XtPointer) data,
				 &res[0], 3, 0, 0);
    }

  /*
   *    Map over toolbar and dashboard to add tooltips.
   */
  if (top_area)
    fe_WidgetTreeWalk(top_area, fe_tooltip_mappee, NULL);
  if (dashboard)
    fe_WidgetTreeWalk(dashboard, fe_tooltip_mappee, NULL);

  /* ========================================================================
     Managing the widgets
     ========================================================================
   */
  {
    Widget kids [20];

    /* manage kids of top area */
    if (top_area)
      {
	/* children of top_left_area */
	ac = 0;
	if (toolbar)   kids[ac++] = toolbar;
	if (url_label) kids[ac++] = url_label;
	if (url_text)  kids[ac++] = url_text;
	if (urlbar)    kids[ac++] = urlbar;
	if (ac != 0)
	  XtManageChildren (kids, ac);
	/* children of top_area */
	ac = 0;
	kids[ac++] = top_left_area;
	if (mid_left_area)
	  kids[ac++] = mid_left_area;
	if (logo)
	  kids[ac++] = logo;
	XtManageChildren (kids, ac);
      }

    if (scroller) XtManageChild(scroller);

#ifdef LEDGES
    /* manage kids of middle area */
    ac = 0;
    kids[ac++] = top_pane;
    kids[ac++] = middle_pane;
    kids[ac++] = bottom_pane;
    XtManageChildren (kids, ac);
#endif

    /* manage kids of bottom area */
    if (dashboard)
      {
	ac = 0;
#ifdef HAVE_SECURITY
	if (sec_logo) kids[ac++] = sec_logo;
#endif /* !HAVE_SECURITY */
	if (mouse_doc)  kids[ac++] = mouse_doc;
	if (thermo)     kids[ac++] = thermo;
	if (led_base)   kids[ac++] = led_base;
	if (bifficon)	kids[ac++] = bifficon;
	if (! ac) abort ();
	XtManageChildren (kids, ac);
      }

    /* manage top level */
    ac = 0;
    if (menubar)  kids[ac++] = menubar;
    if (top_area) kids[ac++] = top_area;
    if (line1)    kids[ac++] = line1;
    kids[ac++] = pane;
    if (line2)     kids[ac++] = line2;
    if (dashboard) kids[ac++] = dashboard;
    XtManageChildren (kids, ac);
  }
  XtManageChild (mainw);

  /* Fix up the geometry if this isn't the first window (take off the
     position part of the spec.) */
  {
    char *geom = 0;
    XtVaGetValues (shell, XmNgeometry, &geom, 0);
    if (! geom)
      {
	XtVaGetValues (XtParent (shell), XmNgeometry, &geom, 0);
	XtVaSetValues (shell, XmNgeometry, geom, 0);
      }
    if (!fe_first_window_p)
      {
	if (geom)
	  {
	    int x, y;
	    unsigned int w, h;
	    int flags = XParseGeometry (geom, &x, &y, &w, &h);
	    if (flags & (WidthValue | HeightValue))
	      {
		char new_geom [255];
		PR_snprintf (new_geom, sizeof (new_geom), "=%dx%d", w, h);
		geom = strdup (new_geom);
		XtVaSetValues (shell, XmNgeometry, geom, 0);
	      }
	    else
	      {
		XtVaSetValues (shell, XmNgeometry, 0, 0);
	      }
	  }
      }
  }
  fe_first_window_p = False;

  /* After processing the first window (and any/all URLs from the command line)
     throw away the -iconic flag.  That applies only to windows created during
     startup, not subsequent ones. */
  if (! fe_command_line_done)
    {
      Boolean iconic = False;
      XtVaGetValues (XtParent (shell), XmNiconic, &iconic, 0);
      set_shell_iconic_p (shell, iconic);
    }

  data->logo = logo;
  data->menubar = menubar;
  data->top_area = top_area;
  data->url_label = url_label;
  data->url_text = url_text;
#ifdef LEDGES
  data->top_ledge = top_ledge;
  data->bottom_ledge = bottom_ledge;
#endif
  data->main_pane = pane;
  data->wholine = mouse_doc;
  data->thermo = slider;
  data->LED = led;
  data->toolbar = toolbar;
  data->dashboard = dashboard;
#ifdef HAVE_SECURITY
  data->security_bar = sec_led;
  data->security_logo = sec_logo;
#endif
  data->bifficon = bifficon;

  if (XtWindow (mainw))
    fe_MakeLEDGCs (context);

  fe_FixLogoMargins (context);

  if (scroller) {
    XtVaSetValues (scroller, XmNinitialFocus,
		   data->drawing_area, 0);
    XtVaSetValues (pane, XmNinitialFocus, scroller, 0);
  }
  XtVaSetValues (mainw, XmNinitialFocus, pane, 0);
  XtVaSetValues (shell, XmNinitialFocus, mainw, 0);

  if (menubar)
    fe_WidgetTreeWalkChildren(menubar, fe_documentString_mappee, context);

 fe_HackTranslations (context, shell);
}

void
fe_FixLogoMargins (MWContext *context)
{
  Widget logo = CONTEXT_DATA (context)->logo;
  Dimension logo_width, logo_height;
  /* If the logo is visible, but it has come out taller than wide, meaning
     that the other crap stretched it, then center it vertically instead. */
  if (logo)
    {
      Dimension w = 0, h = 0;
      int count = (
		   (CONTEXT_DATA (context)->show_toolbar_p ? 1 : 0) +
		   (CONTEXT_DATA (context)->show_directory_buttons_p ? 1 : 0) +
		   (CONTEXT_DATA (context)->show_url_p ? 1 : 0));
      if (context->type == MWContextSaveToDisk) count = 2;
      fe_LogoPixmap (context, &logo_width, &logo_height, (count > 1));
      XtVaGetValues (logo, XmNwidth, &w, XmNheight, &h, 0);
      if (h > logo_height)
	{
	  Dimension off = (h - logo_height) / 2;
	  if (context->type == MWContextSaveToDisk)
	    XtVaSetValues (logo, XmNheight, logo_height, 0);
	  else
	    XtVaSetValues (logo,
			   XmNheight, logo_height,
			   XmNtopOffset,    off,
			   XmNbottomOffset, off,
			   0);
	}
    }
}


#ifdef LEDGES
static void
fe_configure_ledges (MWContext *context, int top_p, int bot_p)
{
  Widget top_ledge = CONTEXT_DATA (context)->top_ledge;
  Widget bot_ledge = CONTEXT_DATA (context)->bottom_ledge;
  Widget k1[2], k2[2];
  int c1 = 0, c2 = 0;

  if (top_p)
    k1[c1++] = top_ledge;
  else
    k2[c2++] = top_ledge;

  if (bot_p)
    k1[c1++] = bot_ledge;
  else
    k2[c2++] = bot_ledge;

  if (c1) XtManageChildren (k1, c1);
  if (c2) XtUnmanageChildren (k2, c2);
}
#endif /* LEDGES */


#ifdef HAVE_SECURITY /* added by jwz */
static void
fe_sec_logo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  SECNAV_SecurityAdvisor (context, SHIST_CreateURLStructFromHistoryEntry (context,
			       SHIST_GetCurrent (&context->hist)));
}
#endif /* HAVE_SECURITY */

static void
fe_expose_thermo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UpdateGraph (context, False);
}

static void
fe_expose_LED_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UpdateLED (context, False);
}

#ifdef HAVE_SECURITY
static void
fe_expose_security_bar_cb (Widget widget, XtPointer closure,
			   XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UpdateSecurityBar (context);
}

#if 0
static void
fe_docinfo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  fe_DocInfoDialog (context);
}
#endif /* 0 */

#endif /* HAVE_SECURITY */

static void
fe_frob_label (MWContext *context, Boolean edited_p, Boolean netsite_p)
{
  Widget label = CONTEXT_DATA (context)->url_label;
  XmString old = 0;
  XmString new = (edited_p
		  ? CONTEXT_DATA (context)->edited_label_string
		  : (netsite_p
		     ? CONTEXT_DATA (context)->netsite_label_string
		     : CONTEXT_DATA (context)->unedited_label_string));
  if (!label)
    return;
  XtVaGetValues (label, XmNlabelString, &old, 0);
  if (!XmStringCompare (old, new))
    XtVaSetValues (label, XmNlabelString, new, 0);
  XmStringFree (old);
}

static Boolean label_being_frobbed = False;

static void
fe_url_text_modify_cb (Widget text, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  if (!label_being_frobbed)
    fe_frob_label (context, True, False);
}


#define MOZILLA_URL_PROP "_MOZILLA_URL"
static Atom XA_MOZILLA_URL = 0;

static void
fe_store_url_prop (MWContext *context, URL_Struct *url)
{
  Widget widget = CONTEXT_WIDGET (context);
  Display *dpy = XtDisplay (widget);
  Window window = XtWindow (widget);
  if (! XA_MOZILLA_URL)
    XA_MOZILLA_URL = XInternAtom (dpy, MOZILLA_URL_PROP, False);
  if (url && url->address && *url->address)
    XChangeProperty (dpy, window, XA_MOZILLA_URL, XA_STRING, 8,
      PropModeReplace, (unsigned char *) url->address, strlen (url->address));
}


void
fe_SetURLString (MWContext *context, URL_Struct *url)
{
  Widget text = CONTEXT_DATA (context)->url_text;

  /* Fixing a bug in motif that sends junk values
	 if SetValues is done on a widget that has
	 modifyVerifyCallback
	 And I really hate the way XtCallbackList is being
	 handled. There is no way to easily copy one. I prefer
	 to use explicitly XtCallbackRec * instead of hiding this
	 and using XtCallbackList. Makes this code more readable.
  */
  XtCallbackRec *cblist, *text_cb;
  int i;

  if (! text) return;
  if (!url || (sploosh && url && !strcmp (url->address, sploosh)))
    url = 0;
  if (label_being_frobbed) abort ();
  /* Inhibit fe_url_text_modify_cb() for this call. */
  label_being_frobbed = True;
  XtVaSetValues (text, XmNcursorPosition, 0, 0);

  /* Get and save the modifyVerifyCallbacklist */
  XtVaGetValues (text, XmNmodifyVerifyCallback, &text_cb, 0);
  for(i=0; text_cb[i].callback != NULL; i++);
  i++;
  cblist = (XtCallbackRec *) malloc(sizeof(XtCallbackRec)*i);
  for(i=0; text_cb[i].callback != NULL; i++)
	cblist[i] = text_cb[i];
  cblist[i] = text_cb[i];

  /* NULL the modifyVerifyCallback before doing setvalues */
  XtVaSetValues (text, XmNmodifyVerifyCallback, NULL, 0);

  XtVaSetValues (text, XmNvalue, (url && url->address ? url->address : ""), 0);

  /* Setback the modifyVerifyCallback List from our saved List */
  XtVaSetValues (text, XmNmodifyVerifyCallback, cblist, 0);
  XP_FREE(cblist);

  label_being_frobbed = False;
  fe_frob_label (context, False, url && url->is_netsite);

  fe_store_url_prop (context, url);
}

static void
fe_url_text_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  URL_Struct *url_struct;
  MWContext *context = (MWContext *) closure;
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  char *text = 0;
  if (cb->reason != XmCR_ACTIVATE) abort ();
  XtVaGetValues (widget, XmNvalue, &text, 0);
  if (! text) abort ();
  text = fe_StringTrim (text);
  if (*text)
    {
      url_struct = NET_CreateURLStruct (text, FALSE);
      fe_GetURL (context, url_struct, FALSE);
    }
  free (text);
  fe_NeutralizeFocus (context);
}

void
fe_AbortCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XP_InterruptContext (context);

  /* If we were downloading, cleanup the file we downloaded to. */
  if (context->type == MWContextSaveToDisk) {
    char *filename = XmTextFieldGetString (CONTEXT_DATA(context)->url_label);
    XP_FileRemove(filename, xpTemporary);
  }
}


#include <X11/Xmu/Error.h>
#include <X11/Xproto.h>

static int
x_fatal_error_handler(Display *dpy)
{
  fe_MinimalNoUICleanup();
  if (fe_pidlock) unlink (fe_pidlock);
  fe_pidlock = NULL;

  /* This function is not expected to return */
  exit(-1);
}

static int
x_error_handler (Display *dpy, XErrorEvent *event)
{
  /* For a couple of not-very-good reasons, we sometimes try to free color
     cells that we haven't allocated, so ignore BadAccess errors from
     XFreeColors().  (This only happens in low color situations when
     substitutions have been performed and something has gone wrong...)
   */
  if (event->error_code == BadAccess &&
      event->request_code == X_FreeColors)
    return 0;

  /* fvwm (a shitty eviscerated version of twm) tends to issue passive grabs
     on the right mouse button.  This will cause any Motif program to get a
     BadAccess when calling XmCreatePopupMenu(), and will cause it to be
     possible to have popup menus that are still up while no mouse button
     is depressed.  No doubt there's some arcane way to configure fvwm to
     not do this, but it's much easier to just punt on this particular error
     message.
   */
  if (event->error_code == BadAccess &&
      event->request_code == X_GrabButton)
    return 0;

  fprintf (real_stderr, "\n%s: ", fe_progname);
  XmuPrintDefaultErrorMessage (dpy, event, real_stderr);
  return 0;
}

static void xt_warning_handler(String nameStr, String typeStr,
	String classStr, String defaultStr,
	String* params, Cardinal* num_params)
{
  /* Xt error handler to ignore colormap warnings */
  if (nameStr && *nameStr && !strcmp(nameStr, "noColormap")) {
	/* Ignore all colormap warning messages */
	return;
  }

  (*old_xt_warning_handler)(nameStr, typeStr, classStr, defaultStr,
			   params, num_params);
}

static void xt_warningOnly_handler(String msg)
{
  static char *ignore_me = NULL;
  int len = 0;

  if (!ignore_me) {
      ignore_me = XP_GetString(XFE_COLORMAP_WARNING_TO_IGNORE);
      if (ignore_me && *ignore_me)
	  len = strlen(ignore_me);
  }

  /* Xt error handler to ignore colormap warnings */
  if (msg && *msg && len && strlen(msg) >= len &&
      !strncasecmp(msg, ignore_me, len)) {
      /* Ignore all colormap warning messages */
#ifdef DEBUG_dp
      fprintf(stderr, "dp, I am ignoring \"%s\"\n", msg);
#endif /* DEBUG_dp */
	return;
  }

  if (old_xt_warningOnly_handler && *old_xt_warningOnly_handler)
      (*old_xt_warningOnly_handler)(msg);
}

#ifdef HAVE_SECURITY /* added by jwz */
static void
fe_read_screen_for_rng (Display *dpy, Screen *screen)
{
    XImage *image;
    size_t image_size;
    int32 coords[4];
    int x, y, w, h;

    assert (dpy && screen);
    if (!dpy || !screen) return;

    RNG_GenerateGlobalRandomBytes(coords,  sizeof(coords));

    x = coords[0] & 0xFFF;
    y = coords[1] & 0xFFF;
    w = (coords[2] & 0x7f) | 0x40; /* make sure w is in range [64..128) */
    h = (coords[3] & 0x7f) | 0x40; /* same for h */

    x = (screen->width - w + x) % (screen->width - w);
    y = (screen->height - h + y) % (screen->height - h);
    image = XGetImage(dpy, screen->root, x, y, w, h, ~0, ZPixmap);
    if (!image) return;
    image_size = (image->width * image->height * image->bits_per_pixel) / 8;
    if (image->data)
	RNG_RandomUpdate(image->data, image_size);
    XDestroyImage(image);
}
#endif /* !HAVE_SECURITY -- added by jwz */

/*****************************
 * Context List manipulation *
 *****************************/

/* XFE maintains a list of all contexts that it created. This is different
   from that maintained by xp as that wouldnt have bookmark, addressbook etc.
   contexts. */

Boolean fe_contextIsValid( MWContext *context )
{
  struct fe_MWContext_cons *cons = fe_all_MWContexts;
  for (; cons && cons->context != context; cons = cons->next);
  return (cons != NULL);
}

void
fe_getVisualOfContext(MWContext *context, Visual **v_ptr, Colormap *cmap_ptr,
				Cardinal *depth_ptr)
{
    Widget mainw = CONTEXT_WIDGET(context);

    while(!XtIsWMShell(mainw) && (XtParent(mainw)!=0))
	mainw = XtParent(mainw);

    XtVaGetValues (mainw,
			XtNvisual, v_ptr,
			XtNcolormap, cmap_ptr,
			XtNdepth, depth_ptr, 0);

    return;
}
/*******************************
 * Session Manager stuff  
 *******************************/
Boolean 
fe_add_session_manager(MWContext *context)
{
	Widget shell;

  	if (context->type != MWContextBrowser &&
        	context->type != MWContextMail &&
        	context->type != MWContextNews) return False;


	someGlobalContext = context;
	shell  = CONTEXT_WIDGET(context);

  	WM_SAVE_YOURSELF = XmInternAtom(XtDisplay(shell),
                "WM_SAVE_YOURSELF", False);
        XmAddWMProtocols(shell,
                                &WM_SAVE_YOURSELF,1);
        XmAddWMProtocolCallback(shell,
                WM_SAVE_YOURSELF,
                fe_wm_save_self_cb, someGlobalContext);
	return True;
}

void fe_wm_save_self_cb(Widget w, XtPointer clientData, XtPointer callData)
{
	Widget shell = w;

	if (save_argv[0])
	  XP_FREE(save_argv[0]);
	save_argv[0]= 0;

	/*always use long name*/
	StrAllocCopy(save_argv[0], fe_progname_long);

	/* Has to register with a valid toplevel shell */
	XSetCommand(XtDisplay(shell),
                XtWindow(shell), save_argv, save_argc);

	/* On sgi: we will get this every 15 mins. So dont ever exit here.
	** The atexit handler fe_atexit_hander() will take care of removing
	** the pid_lock file.
	*/
	fe_MinimalNoUICleanup();
}

Boolean
fe_is_absolute_path(char *filename)
{
	if ( filename && *filename && filename[0] == '/')
		return True;
	return False;
}

Boolean
fe_is_working_dir(char *filename, char** progname)
{
   *progname = 0;

   if ( filename && *filename )
   {
	if ( strlen(filename)>1 && 
		   filename[0] == '.' && filename[1] == '/')
	{
          char *str;
          char *name;

	  name = filename;

	  str = strrchr(name, '/');
	  if ( str ) name = str+1;

	  *progname = (char*)malloc((strlen(name)+1)*sizeof(char));
	  strcpy(*progname, name);

	  return True;
	}
	else if (strchr(filename, '/'))
	{
	  *progname = (char*)malloc((strlen(filename)+1)*sizeof(char));
	  strcpy(*progname, filename);
	  return True;
	}
    }
    return False;
}

Boolean 
fe_found_in_binpath(char* filename, char** dirfile)
{
    char *binpath = 0;
    char *dirpath = 0;
    struct stat buf;

    *dirfile = 0;
    binpath = getenv("PATH");

    if ( binpath )
    {
    	binpath = XP_STRDUP(binpath);
	dirpath = XP_STRTOK(binpath, ":");
        while(dirpath)
        {
	   *dirfile = 0;
	   if ( dirpath[strlen(dirpath)-1] == '/' )
           {
	        dirpath[strlen(dirpath)-1] = '\0';
           }

	   *dirfile = PR_smprintf("%s/%s", dirpath, filename);

 	   if ( !stat(*dirfile, &buf) )
           {
	   	XP_FREE(binpath);
		return True;
           }
	   dirpath = XP_STRTOK(NULL,":");
	   XP_FREE(*dirfile);
        }
	XP_FREE(binpath);
    }
    return False;
}

char *
fe_expand_working_dir(char *cwdfile)
{
   char *dirfile = 0;
   char *string;

   string = getcwd(NULL, MAXPATHLEN);

   dirfile = (char *)malloc((strlen(string)+strlen("/")+strlen(cwdfile)+1) 
			*sizeof(char));
   strcpy(dirfile, string);
   strcat(dirfile,"/");
   strcat(dirfile, cwdfile);
   XP_FREE(string);
   return dirfile;
}

/****************************************
 * This function will return either of these
 * type of the exe file path:
 * 1. return FULL Absolute path if user specifies a full path
 * 2. return FULL Absolute path if the program was found in user 
 *    current working dir 
 * 3. return relative path (ie. the same as it is in fe_progname)
 *    if program was found in binpath
 *
 ****************************************/
const char *
fe_locate_program_path(const char *fe_prog)
{
  char *ret_path = 0;
  char *dirfile = 0;
  char *progname = 0;

  StrAllocCopy(progname, fe_prog);

  if ( fe_is_absolute_path(progname) )
  {
	StrAllocCopy(ret_path, progname);
        XP_FREE(progname);
	return ret_path;
  }
  else if ( fe_is_working_dir(progname, &dirfile) )
  {
	ret_path = fe_expand_working_dir(dirfile);
        XP_FREE(dirfile);
        XP_FREE(progname);
        return ret_path;
  }
  else if ( fe_found_in_binpath(progname, &ret_path) )
  {
        if ( fe_is_absolute_path(ret_path) )
        {
	   /* Found in the bin path; then return only the exe filename */
	   /* Always use bin path as the search path for the file 
	      for consecutive session*/
	   XP_FREE(ret_path);
	   ret_path = progname;
       	   XP_FREE(dirfile);
	}
	else if (fe_is_working_dir(ret_path, &dirfile) )
	{
		XP_FREE(ret_path);
		XP_FREE(progname);
		ret_path = fe_expand_working_dir(dirfile);
        	XP_FREE(dirfile);
	}
	return ret_path;
  }
  else
  {
      XP_FREE(ret_path);
      XP_FREE(dirfile);
      XP_FREE(progname);
      fprintf(stderr, "%s: not found in PATH!\n", 
	fe_progname);
    
      exit(-1);
  }
}

void fe_GetProgramDirectory(char *path, int len)
{
	char * separator;

	strncpy (path, fe_progname_long, len);
	if (( separator = XP_STRRCHR(path, '/') ))
	{
		separator[1] = 0;
	}
	return;
}


/* DEBUG_JWZ from mozilla 5 */

/*
 * Make Async DNS optional at runtime.
 */
static XP_Bool _use_async_dns = True;

XP_Bool fe_UseAsyncDNS(void)
{
  return _use_async_dns;
}

static void fe_check_use_async_dns(void)
{
  char * c = getenv ("MOZILLA_NO_ASYNC_DNS");
        
  _use_async_dns = True;

  if (c && *c)
    {
      /* Just in case make sure the var is not [fF0] (for funky logic) */
      if (*c != 'f' && *c != 'F' && *c != '0')
        {
          _use_async_dns = False;
        }
    }
}



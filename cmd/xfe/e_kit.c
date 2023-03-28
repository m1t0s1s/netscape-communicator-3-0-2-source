/**********************************************************************
 e_kit.c
 By Daniel Malmer
 10 Jan 1996

 Implementation for client customization.
 Copyright \051 1995 Netscape Communications Corporation, all rights reserved.
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mozilla.h"
#include "xfe.h"
#include "xp_md5.h"
#include "e_kit.h"
#include "e_kit_patch.h"
#include "e_kit_resources.h"
#include "icondata.h"
#include "menu.h"
#include "net.h"
#include "xpgetstr.h"
#ifdef EDITOR
#include "xeditor.h"
#endif /*EDITOR*/

/* Messages declared in xfe_err.h */
extern int XFE_EKIT_LOCK_FILE_NOT_FOUND;
extern int XFE_EKIT_MODIFIED;
extern int XFE_EKIT_CANT_OPEN;
extern int XFE_EKIT_FILESIZE;
extern int XFE_EKIT_CANT_READ;
extern int XFE_ANIM_CANT_OPEN;
extern int XFE_ANIM_READING_SIZES;
extern int XFE_ANIM_READING_NUM_COLORS;
extern int XFE_ANIM_READING_COLORS;
extern int XFE_ANIM_READING_FRAMES;
extern int XFE_ANIM_NOT_AT_EOF;
extern int XFE_EKIT_ABOUT_MESSAGE;

#define HTTP_OFF 42
#define MAX_BUTTONS 32
#define MAX_RESOURCE_NAME_LEN 64

struct fe_icon_data* anim_custom_large = NULL;
struct fe_icon_data* anim_custom_small = NULL;

static int ekit_errno = 0;

static Widget top_level = NULL;

static Boolean load_resources(Widget top_level);

struct button {
    char* name;
    char* url;
    char* (*url_routine)(void);
};

#define lock_pref(name, field) {\
  if ( (tmp = resource_val(name)) != NULL ) {\
    if ( fe_globalPrefs.field ) {\
      free(fe_globalPrefs.field);\
    }\
    fe_globalPrefs.field = strdup(tmp);\
    fe_defaultPrefs.field = tmp;\
  }\
}

#define lock_pref_int(name, field) {\
    if ( (tmp = resource_val(name)) != NULL ) {\
        fe_globalPrefs.field = atoi(tmp);\
        fe_defaultPrefs.field = atoi(tmp);\
    }\
  }


static char* resource_val(char* name);
static int read_colors(FILE* file, int num_colors);
static int count_buttons(char* bar, struct button buttons[], int max_buttons);
static Boolean resource_locked(char* resource);
static void fe_add_to_menu(Widget menubar, char* menu_name,
                           struct button buttons[], int num_buttons);
static void callback(Widget w, XtPointer closure, XtPointer client_data);
static Boolean load_resources(Widget top_level);
static Boolean load_animation(char* filename);
static char* get_database_filename(Widget top_level);
static void reset_default_resources(Widget top_level);
static void free_resource_list(XtResourceList list);
static XtResourceList create_resource_list(struct button* buttons, 
                                           int num_buttons, char* suffix);
static void un_funkify(char* funk);
static char* resource_name_to_value(char* resource_name);
static Boolean ekit_show_about_usenet(void);

extern char* xfe_go_get_url_whats_new(void);
extern char* xfe_go_get_url_whats_cool(void);
extern char* xfe_go_get_url_manual(void);
extern char* xfe_go_get_url_inet_search(void);
extern char* xfe_go_get_url_inet_directory(void);
extern char* xfe_go_get_url_software(void);

extern char* xfe_go_get_url_netscape(void);
extern char* xfe_go_get_url_galleria(void);
extern char* xfe_go_get_url_inet_white(void);

extern char* xfe_go_get_url_plugins(void);
extern char* xfe_go_get_url_reg(void);
extern char* xfe_go_get_url_relnotes(void);
extern char* xfe_go_get_url_faq(void);
extern char* xfe_go_get_url_usenet(void);
extern char* xfe_go_get_url_security(void);
extern char* xfe_go_get_url_feedback(void);
extern char* xfe_go_get_url_support(void);
extern char* xfe_go_get_url_howto(void);
extern char* xfe_go_get_url_inet_about(void);
extern char* xfe_go_get_url_destinations(void);

static struct button directory_buttons[] = {
{"whatsNew",        NULL,	xfe_go_get_url_whats_new},
{"whatsCool",       NULL,	xfe_go_get_url_whats_cool},
{"inetIndex",       NULL,	xfe_go_get_url_destinations},
{"inetSearch",      NULL,	xfe_go_get_url_inet_search},
{"inetWhite",       NULL,	xfe_go_get_url_inet_white},
{"upgrade",         NULL,	xfe_go_get_url_software},
};

static struct button directory_menu[] = {
{"netscape",        NULL,	xfe_go_get_url_netscape},
{"whatsNew",        NULL,	xfe_go_get_url_whats_new},
{"whatsCool",       NULL,	xfe_go_get_url_whats_cool},
{"directoryMenu4",  NULL,	NULL},
{"galleria",        NULL,	xfe_go_get_url_galleria},
{"inetIndex",       NULL,	xfe_go_get_url_destinations},
{"inetSearch",      NULL,	xfe_go_get_url_inet_search},
{"inetWhite",       NULL,	xfe_go_get_url_inet_white},
{"inetAbout",       NULL,	xfe_go_get_url_inet_about},
{"directoryMenu10", NULL,	NULL},
{"directoryMenu11", NULL,	NULL},
{"directoryMenu12", NULL,	NULL},
{"directoryMenu13", NULL,	NULL},
{"directoryMenu14", NULL,	NULL},
{"directoryMenu15", NULL,	NULL},
{"directoryMenu16", NULL,	NULL},
{"directoryMenu17", NULL,	NULL},
{"directoryMenu18", NULL,	NULL},
{"directoryMenu19", NULL,	NULL},
{"directoryMenu20", NULL,	NULL},
{"directoryMenu21", NULL,	NULL},
{"directoryMenu22", NULL,	NULL},
{"directoryMenu23", NULL,	NULL},
{"directoryMenu24", NULL,	NULL},
{"directoryMenu25", NULL,	NULL},
};

static struct button help_menu[] = {
{"aboutplugins",    NULL,	xfe_go_get_url_plugins},
{"registration",    NULL,	xfe_go_get_url_reg},
{"upgrade",         NULL,	xfe_go_get_url_software},
{"helpMenu4",       NULL,	NULL},
{"manual",          NULL,	xfe_go_get_url_manual},
{"relnotes",        NULL,	xfe_go_get_url_relnotes},
{"faq",             NULL,	xfe_go_get_url_faq},
{"aboutSecurity",   NULL,	xfe_go_get_url_security},
{"helpMenu9",       NULL,	NULL},
{"feedback",        NULL,	xfe_go_get_url_feedback},
{"support",         NULL,	xfe_go_get_url_support},
{"howTo",           NULL,	xfe_go_get_url_howto},
{"helpMenu13", NULL,		NULL},
{"helpMenu14", NULL,		NULL},
{"helpMenu15", NULL,		NULL},
{"helpMenu16", NULL,		NULL},
{"helpMenu17", NULL,		NULL},
{"helpMenu18", NULL,		NULL},
{"helpMenu19", NULL,		NULL},
{"helpMenu20", NULL,		NULL},
{"helpMenu21", NULL,		NULL},
{"helpMenu22", NULL,		NULL},
{"helpMenu23", NULL,		NULL},
{"helpMenu24", NULL,		NULL},
{"helpMenu25", NULL,		NULL},
};

/*
 * On X, the news help menu has "About Usenet".
 * So, we have to repeat this stuff here, with the
 * addition of the aboutUsenet menu item.
 */
static struct button news_help_menu[] = {
{"aboutplugins",    NULL,	xfe_go_get_url_plugins},
{"registration",    NULL,	xfe_go_get_url_reg},
{"upgrade",         NULL,	xfe_go_get_url_software},
{"helpMenu4",       NULL,	NULL},
{"manual",          NULL,	xfe_go_get_url_manual},
{"relnotes",        NULL,	xfe_go_get_url_relnotes},
{"faq",             NULL,	xfe_go_get_url_faq},
{"aboutUsenet",     NULL,	xfe_go_get_url_usenet},
{"aboutSecurity",   NULL,	xfe_go_get_url_security},
{"helpMenu9",       NULL,	NULL},
{"feedback",        NULL,	xfe_go_get_url_feedback},
{"support",         NULL,	xfe_go_get_url_support},
{"howTo",           NULL,	xfe_go_get_url_howto},
{"helpMenu13", NULL,		NULL},
{"helpMenu14", NULL,		NULL},
{"helpMenu15", NULL,		NULL},
{"helpMenu16", NULL,		NULL},
{"helpMenu17", NULL,		NULL},
{"helpMenu18", NULL,		NULL},
{"helpMenu19", NULL,		NULL},
{"helpMenu20", NULL,		NULL},
{"helpMenu21", NULL,		NULL},
{"helpMenu22", NULL,		NULL},
{"helpMenu23", NULL,		NULL},
{"helpMenu24", NULL,		NULL},
{"helpMenu25", NULL,		NULL},
};

static int num_help_menu_items;
static int num_news_help_menu_items;
static int num_directory_menu_items;
static int num_directory_buttons;

/*
 * ekit_error
 * Pops up an error dialog if an ekit error has occured.
 * Error strings are found in 'xfe_err.h'.
 */
static void
ekit_error(void)
{
  if ( ekit_errno ) {
    fe_dialog(top_level, "error", XP_GetString(ekit_errno), 
              FALSE, 0, TRUE, FALSE, 0);
  }
}


/*
 * un_funkify
 * Un-obscures resource strings.
 */
static void
un_funkify(char* funk)
{
    char* ptr;

    for ( ptr = funk; *ptr != '\0'; ptr++ ) {
        *ptr-= 69;
    }
}


/*
 * create_resource_list
 * Creates a list suitable for passing to XtGetApplicationResources.
 * This is done at run-time so we can change the names of the buttons
 * without having to change several dozen resource names as well.
 */
static XtResourceList
create_resource_list(struct button* buttons, int num_buttons, char* suffix)
{
    int i;
    static char name[MAX_BUTTONS][MAX_RESOURCE_NAME_LEN];
    XtResourceList list = (XtResourceList) malloc( (num_buttons+1) *
                                                   sizeof(*list));
    
#define diff(a, b) (Cardinal) ((char*) (a) - (char*) (b))

    assert(num_buttons <= MAX_BUTTONS);

    for ( i = 0; i < num_buttons; i++ ) {
        assert((strlen(buttons[i].name) + strlen(suffix) + 3) < 
                      MAX_RESOURCE_NAME_LEN);
        sprintf(name[i], "%s%sUrl", buttons[i].name, suffix);
        list[i].resource_name   = name[i];
        list[i].resource_class  = XtCString;
        list[i].resource_type   = XtRString;
        list[i].resource_size   = sizeof(String);
        list[i].resource_offset = diff(&(buttons[i].url), buttons);
        list[i].default_type    = XtRImmediate;
        list[i].default_addr    = NULL;
    }

    list[i].resource_name = NULL;

    return list;
}


/*
 * free_resource_list
 * Frees a resource list that was allocated by create_resource_list.
 */
static void
free_resource_list(XtResourceList list)
{
    free(list);
}


/*
 * reset_default_resources
 * For the purposes of the e-kit, some properties of the Navigator
 * are specified with XResources.  In order to prevent the user
 * from changing these resources, we reset them here to their
 * default value.
 * 'fe_ekitDefaultDatabase' is a string that represents the default
 * value of each of the e-kit resources.  It is specified in e_kit.ad,
 * which is turned into a header file by ad2c.
 */
static void
reset_default_resources(Widget top_level)
{
    XrmDatabase source;
    XrmDatabase target;
    Boolean override = True;
    char* string = strdup(fe_ekitDefaultDatabase);

    un_funkify(string);

    target = XrmGetDatabase(XtDisplay(top_level));
    source = XrmGetStringDatabase(string);

    XrmCombineDatabase(source, &target, override); 

    free(string);
}


/* The following have 42 added to each character to prevent user fiddling */
/* "Netscape." */
#define NSCP "\170\217\236\235\215\213\232\217\130"
/* "Netscape.animationFile" */
#define ANIM_FILE NSCP "\213\230\223\227\213\236\223\231\230\160\223\226\217"
/* "Netscape.logoButtonUrl" */
#define LOGO_URL NSCP "\226\231\221\231\154\237\236\236\231\230\177\234\226"
/* "Netscape.homePage" */
#define HOME_PAGE NSCP "\222\231\227\217\172\213\221\217"
/* "Netscape.userAgent" */
#define USER_AGENT NSCP "\237\235\217\234\153\221\217\230\236"
/* "Netscape.autoconfigUrl" */
#define AUTOCFG_URL NSCP "\213\237\236\231\215\231\230\220\223\221\177\234\226"
/* "Netscape.popServer" */
#define POP_SERVER NSCP "\232\231\232\175\217\234\240\217\234"
/* "Netscape.popName" */
#define POP_NAME NSCP "\232\231\232\170\213\227\217"
/* "Netscape.javaClasspath" */
#define JAVA_PATH NSCP "\224\213\240\213\155\226\213\235\235\232\213\236\222"
/* "Netscape.xFileSearchPath" */
#define X_PATH NSCP "\242\160\223\226\217\175\217\213\234\215\222\172\213\236\222"
/* "Netscape.pluginPath" */
#define PLUGIN_PATH NSCP "\232\226\237\221\223\230\172\213\236\222"


/*
 * ekit_AnimationFile
 * Returns the name of the e-kit animation file.
 * If it return NULL, then there is no custom animation file.
 */
char*
ekit_AnimationFile(void)
{
    return resource_name_to_value(ANIM_FILE);
}


/*
 * ekit_LogoUrl
 * Returns the URL that will be loaded if the logo button is pressed.
 * It returns NULL if the user didn't specify one.
 */
char*
ekit_LogoUrl(void)
{
    return resource_name_to_value(LOGO_URL);
}


/*
 * ekit_HomePage
 * Returns the Home Page to be loaded at start, or NULL if not specified.
 */
char*
ekit_HomePage(void)
{
    return resource_name_to_value(HOME_PAGE);
}


/*
 * ekit_UserAgent
 * Returns the string to be appended to the User Agent, or NULL if not 
 * specified.
 */
char*
ekit_UserAgent(void)
{
    return resource_name_to_value(USER_AGENT);
}


/*
 * ekit_AutoconfigUrl
 * Returns the Autoconfig Url, or NULL if not specified.
 */
char*
ekit_AutoconfigUrl(void)
{
    return resource_name_to_value(AUTOCFG_URL);
}


/*
 * ekit_PopServer
 * Returns the Pop Server, or NULL if not specified.
 */
char*
ekit_PopServer(void)
{
    return resource_name_to_value(POP_SERVER);
}


/*
 * ekit_PopName
 * Returns the Pop Name, or NULL if not specified.
 */
char*
ekit_PopName(void)
{
    return resource_name_to_value(POP_NAME);
}


/*
 * ekit_MsgSizeLimit
 * Returns the Msg Size Limit
 */
Boolean
ekit_MsgSizeLimit(void)
{
    return resource_locked("Netscape.maxSize");
}


/*
 * env_var_append
 * If the given environmental variable doesn't have a value,
 * just sets the variable to the given value.
 * Otherwise appends the value to the variable, separated
 * by a colon.
 */
static void
env_var_append(char* var, char* value)
{
    char* old_value;
    char* new_value;
 
    old_value = getenv(var);
 
    new_value = (char*) malloc(strlen(var) + 1 +
                               (old_value ? (strlen(old_value) + 1) : 0) +
                               strlen(value) + 1);
 
    strcpy(new_value, var);
    strcat(new_value, "=");
    strcat(new_value, old_value ? old_value : "");
    strcat(new_value, old_value ? ":" : "");
    strcat(new_value, value);
 
    putenv(new_value);

    free(new_value);
}


/*
 * update_preferences
 * Sets the current and default values of customizable paramaters to
 * the value specified by the e-kit.
 */
static void
update_preferences(void)
{
  char* tmp;

  if ( resource_locked("maxSize") ) {
      fe_defaultPrefs.pop3_msg_size_limit_p = True;
      fe_globalPrefs.pop3_msg_size_limit_p = True;
  }

  if ( (tmp = resource_name_to_value(JAVA_PATH)) != NULL ) {
      env_var_append("CLASSPATH", tmp);
  }

  if ( (tmp = resource_name_to_value(X_PATH)) != NULL ) {
      env_var_append("XFILESEARCHPATH", tmp);
  }

  if ( (tmp = resource_name_to_value(PLUGIN_PATH)) != NULL ) {
      env_var_append("NPX_PLUGIN_PATH", tmp);
  }

  lock_pref("ftpProxy", ftp_proxy);
  lock_pref("gopherProxy", gopher_proxy);
  lock_pref("httpProxy", http_proxy);
#ifdef HAVE_SECURITY
  lock_pref("httpsProxy", https_proxy);
#endif
  lock_pref("waisProxy", wais_proxy);
  lock_pref("socksServer", socks_host);
  lock_pref("noProxy", no_proxy);
  lock_pref("autoconfigUrl", proxy_url);
  lock_pref("homePage", home_document);
  lock_pref("nntpServer", newshost);
  lock_pref("smtpServer", mailhost);
  lock_pref("popServer", pop3_host);

  lock_pref("userOrganization", organization);
  lock_pref("userAddr", email_address);
  lock_pref("userName", real_name);
  lock_pref("popName", pop3_user_id);

  lock_pref_int("proxyMode", proxy_mode);
  lock_pref_int("leaveOnServer", pop3_leave_mail_on_server);
  lock_pref_int("maxSize", pop3_msg_size_limit);
  lock_pref_int("askForPassword", ask_password);
  lock_pref_int("passwordLifetime", password_timeout);
#if 0
  /* use_password is no longer in prefs.h? */
  lock_pref_int("usePassword", use_password);
#endif
#ifdef JAVA
  lock_pref_int("disableJava", disable_java);
#endif
#ifdef MOCHA
  lock_pref_int("disableJavascript", disable_javascript);
#endif
  lock_pref_int("enableSSL2", ssl2_enable);
  lock_pref_int("enableSSL3", ssl3_enable);
  lock_pref_int("movemailWarn", movemail_warn);

  /*
   * For SGI's migration to Netscape mail.
   * malmer 7/15/96
   */
  lock_pref_int("useMovemailP", use_movemail_p);
  lock_pref_int("builtinMovemailP", builtin_movemail_p);
  lock_pref("movemailProgram", movemail_program);
  lock_pref_int("biffInterval", biff_interval);
}


static int ekit_loaded = 0;

/*
 * ekit_LockPrefs
 * First, set resources back to their default values.
 * If this is an Enterprise Kit binary, try to read in
 * the lock file.  If it doesn't exist, or has been changed,
 * complain and die.
 */
void
ekit_LockPrefs(void)
{
    if ( ekit_Enabled() == FALSE || ekit_loaded == FALSE ) return;

    update_preferences();

    if ( ekit_AnimationFile() ) {
      if ( load_animation(ekit_AnimationFile()) == False ) {
        ekit_error();
      }
    }

    if ( ekit_AutoconfigUrl() ) {
      NET_SetNoProxyFailover();
    }
}


/*
 * get_database_filename
 * Looks for the Netscape.cfg file.
 * "path" represents the search path for the config file.
 * %E is replaced with the root directory that is specified
 * at configure time by the e-kit locker, which is a binary
 * patch program.
 */
static char*
get_database_filename(Widget top_level)
{
    String filename;
    SubstitutionRec sub = {'E', NULL};
    Cardinal num_substitutions = 1;
    String path = "%E/%L/%T/%N%C%S:"
                  "%E/%l/%T/%N%C%S:"
                  "%E/%T/%N%C%S:"
                  "%E/%L/%T/%N%S:"
                  "%E/%l/%T/%N%S:"
                  "%E/%T/%N%S";

    sub.substitution = ekit_root();

    filename = XtResolvePathname(XtDisplay(top_level),
                                 "app-defaults",
                                 "Netscape",
                                 ".cfg",
                                 path,
                                 &sub,
                                 num_substitutions,
                                 NULL);


    if ( filename == NULL ) {
      ekit_errno = XFE_EKIT_LOCK_FILE_NOT_FOUND;
    }

    return filename;
}


/*
 * unobscure_string
 * The e-kit compiler adds a number between 40 and 49 to each letter.
 * This turns it back into readable text.
 */
static Boolean
unobscure_string(char* string)
{
    int offset = 0;

    for ( ; *string; string++, offset++ ) {
        *string = *string - ( 40 + (offset%10));
    }

    return True;
}


/*
 * verify_string
 * The first line in the resource file is a comment that contains the
 * md5 hash for the rest of the file.
 */
static Boolean
verify_string(char* string)
{
    char* hash = string + 2;
    char* rest_of_string = strchr(string, '\n');

    if ( string[0] != '!' || string[1] != ' ' || rest_of_string == NULL ) {
        ekit_errno = XFE_EKIT_MODIFIED;
        return False;
    }

    *rest_of_string = '\0';
    rest_of_string++;

    if ( strcmp(XP_Md5PCPrintable(rest_of_string, strlen(rest_of_string)),
                hash) ) {
        ekit_errno = XFE_EKIT_MODIFIED;
        return False;
    }

    *strchr(string, '\0') = '\n';

    return True;
}


/*
 * get_file_size
 * Given a file pointer, returns the size of the file in bytes.
 */
static int
get_file_size(FILE* file)
{
    struct stat buf;

    if ( fstat(fileno(file), &buf) == -1 ) {
        return -1;
    }

    return buf.st_size;
}


/*
 * get_database_string
 * Given the name of the Netscape.lock file, this routine returns
 * a character string that represents the data in the file.  The
 * string will be in XResource file format.
 * eg:  "Netscape.homePage: http://home.netscape.com"
 */
static char*
get_database_string(char* filename)
{
    FILE* file;
    int file_size;
    char* string;

    if ( (file = fopen(filename, "r")) == NULL ) {
        ekit_errno = XFE_EKIT_CANT_OPEN;
        return NULL;
    }

    if ( (file_size = get_file_size(file)) < 0 ) {
        ekit_errno = XFE_EKIT_FILESIZE;
        return NULL;
    }

    if ( (string = (char*) malloc((file_size+1) * sizeof(char))) == NULL ) {
        ekit_errno = XFE_EKIT_CANT_READ;
        return NULL;
    }

    if ( fread(string, sizeof(char), file_size, file) != file_size ) {
        free(string);
        ekit_errno = XFE_EKIT_CANT_READ;
        return NULL;
    }

    string[file_size] = '\0';

    return string;
}


/*
 * load_resources
 * If there is a locked resource file 'Netscape.lock', merge its
 * resources with the current database.  This routine is called
 * after XtAppInitialize handles app-defaults, .Xdefaults, command 
 * line switches, etc.  So, the lock file has the last say.
 */
static Boolean
load_resources(Widget top_level)
{
    String filename;
    XrmDatabase source;
    XrmDatabase target;
    Boolean override = True;
    char* db_string;

    if ( (filename = get_database_filename(top_level)) == NULL  ||
         (db_string = get_database_string(filename))   == NULL  ||
         unobscure_string(db_string)                   == False ||
         verify_string(db_string)                      == False ) {
        return False;
    }

    source = XrmGetStringDatabase(db_string);
    target = XrmGetDatabase(XtDisplay(top_level));
    
    XrmCombineDatabase(source, &target, override);

    return True;
}


/*
 * callback
 * Callback that is invoked when a menu item is selected or directory
 * button is pushed, and a Url is to be loaded.
 */
static void
callback(Widget w, XtPointer closure, XtPointer client_data)
{
    char* url;
    char buf[1024], *in, *out;
    MWContext* context = (MWContext*) closure;
 
    fe_UserActivity (context);
 
    XtVaGetValues(w, XmNuserData, &url, NULL);

    if ( url ) {
        for (in = url, out = buf; *in; in++, out++) *out = *in - HTTP_OFF;
        *out = 0;
#ifdef EDITOR
		if (EDT_IS_EDITOR(context))
			fe_GetURLInBrowser(context, NET_CreateURLStruct(buf, FALSE));
		else
#endif
        fe_GetURL (context, NET_CreateURLStruct(buf, FALSE), FALSE);
    }
}


/*
 * count_buttons
 * Returns the number of buttons in a list of buttons.
 */
static int
count_buttons(char* bar, struct button buttons[], int max_buttons)
{
    char buf[256];
    int count;
    char* type;
    XrmValue value;
    char* value_str;

    for ( count = 0; count < max_buttons; count++ ) {
        sprintf(buf, "Netscape*%s*%s.labelString", bar, buttons[count].name);
        XrmGetResource(XrmGetDatabase(XtDisplay(top_level)), 
                       buf, XmCString, &type, &value);
        value_str = (char*) value.addr;
        if ( value_str == NULL || !strcmp(value_str, "") ) break;
        if ( !strcmp(value_str, "-") ) buttons[count].name = "";
    }

    return count;
}


/*
 * ekit_AddDirectoryMenu
 * Adds menu items to the Directory menu.
 */
void
ekit_AddDirectoryMenu(Widget menubar)
{
  fe_add_to_menu(menubar, "directory", directory_menu, num_directory_menu_items);
}


/*
 * ekit_AddHelpMenu
 * Adds menu items to the Help menu.
 */
void
ekit_AddHelpMenu(Widget menubar, MWContextType type)
{
  if ( type == MWContextNews && ekit_show_about_usenet() ) {
      fe_add_to_menu(menubar, "help", news_help_menu, num_news_help_menu_items);
  } else {
      fe_add_to_menu(menubar, "help", help_menu, num_help_menu_items);
  }
}


/*
 * fix_url
 * Adds HTTP_OFF to the Url.
 */
static char*
fix_url(char* url)
{
    char* ptr;
    char* new_url = strdup(url);

    for ( ptr = new_url; *ptr; ptr++ ) {
        *ptr+= HTTP_OFF;
    }

    return new_url;
}


/*
 * ekit_GetDirectoryButtons
 */
struct fe_button*
ekit_GetDirectoryButtons(void)
{
    int i;
    int num_buttons = ekit_NumDirectoryButtons();
    struct fe_button* buttons = (struct fe_button*) 
                                malloc(num_buttons * sizeof(struct fe_button));

    for ( i = 0; i < num_buttons; i++ ) {
        buttons[i].name     = directory_buttons[i].name;
        buttons[i].callback = callback;
        buttons[i].type     = SELECTABLE;
        buttons[i].var      = NULL;
        buttons[i].offset   = NULL;
        if ( directory_buttons[i].url && *(directory_buttons[i].url) ) {
            buttons[i].userdata = (XtPointer) fix_url(directory_buttons[i].url);
        } else {
            if ( directory_buttons[i].url_routine ) {
                buttons[i].userdata = (XtPointer) directory_buttons[i].url_routine();
            }
        }
    }

    return buttons;
}


/*
 * ekit_NumDirectoryButtons
 */
int
ekit_NumDirectoryButtons(void)
{
  return num_directory_buttons;
}


/*
 * fe_add_to_menu
 * By Daniel Malmer
 * 16 Jan 1996
 *
 * Alter the directory menu array, based on customizations.
 */
static void
fe_add_to_menu(Widget menubar, char* menu_name, struct button buttons[],
               int num_buttons)
{
    int i;
    Widget cascade;
    Widget menu;
    struct fe_button* items;
    fe_ContextData* data;
    MWContext* context;
 
    context = fe_WidgetToMWContext(menubar);

    data = CONTEXT_DATA(context);

    items = (struct fe_button*) malloc((num_buttons+1) * sizeof(*items));
 
    cascade = XtNameToWidget(menubar, menu_name);
    XtVaGetValues(cascade, XmNsubMenuId, &menu, NULL);
 
    /* Terminate the list */
    items[num_buttons].name = NULL;
 
    for ( i = 0; i < num_buttons; i++ ) {
        items[i].name     = buttons[i].name;
        items[i].callback = callback;
        items[i].type     = SELECTABLE;
        items[i].var      = NULL;
        items[i].offset   = NULL;
        items[i].userdata = (XtPointer) buttons[i].url;
        if ( buttons[i].url && *(buttons[i].url) ) {
            items[i].userdata = (XtPointer) fix_url(buttons[i].url);
        } else {
            if ( buttons[i].url_routine ) {
                items[i].userdata = (XtPointer) buttons[i].url_routine();
            }
        }
    }
 
    fe_CreateSubmenu(menu, items, context, context, CONTEXT_DATA(context));
 
    free(items);
}


/*
 * read_frames
 * Reads animation frames from the animation input file.
 */
static int
read_frames(FILE* file, int num_frames, int width, int height, 
                        struct fe_icon_data* anim)
{
    int i;
    int j;
    int n = width*height;
    int bytes_per_line = (width+7)/8;
    int total_bytes = bytes_per_line*height;

    for ( i = 0; i < num_frames; i++ ) {
        anim[i].width = width;
        anim[i].height = height;

        anim[i].mono_bits = (uint8*) malloc(total_bytes);
        anim[i].mask_bits = (uint8*) malloc(total_bytes);
        anim[i].color_bits = (uint8*) malloc(n);

        if ( fread(anim[i].mono_bits, sizeof(uint8), total_bytes, file) != 
             total_bytes ) {
            return -1;
        }

        if ( fread(anim[i].mask_bits, sizeof(uint8), total_bytes, file) != 
             total_bytes ) {
            return -1;
        }

        if ( fread(anim[i].color_bits, sizeof(uint8), n, file) != n ) {
            return -1;
        }

        for ( j = 0; j < width*height; j++ ) {
            anim[i].color_bits[j]+= fe_n_icon_colors;
        }
    }

    return 0;
}


/*
 * read_colors
 * Reads color data from the animation data file.
 */
static int
read_colors(FILE* file, int num_colors)
{
    int i;
    uint16 r;
    uint16 g;
    uint16 b;

    for ( i = 0; i < num_colors; i++ ) {
        if ( fscanf(file, " %hx %hx %hx", &r, &g, &b) != 3 ) {
            return -1;
        }
        fe_icon_colors[fe_n_icon_colors+i][0] = r;
        fe_icon_colors[fe_n_icon_colors+i][1] = g;
        fe_icon_colors[fe_n_icon_colors+i][2] = b;
    }

    if ( getc(file) != '\n' ) {
      return -1;
    }

    return 0;
}


/*
 * load_animation
 * Given the name of an animation file, reads in data from
 * that file.
 */
static Boolean
load_animation(char* filename)
{
    int num_frames_small;
    int width_small;
    int height_small;
    int num_frames_large;
    int width_large;
    int height_large;
    int num_colors;
    FILE* file;

    if ( filename == NULL ) {
	return False;
    }

    if ( (file = fopen(filename, "r")) == NULL ) {
        ekit_errno = XFE_ANIM_CANT_OPEN;
        return False;
    }

    if ( fscanf(file, " %d %d %d ", 
                      &num_frames_large, 
                      &width_large, 
                      &height_large) != 3 ||
        fscanf(file, " %d %d %d ", 
                      &num_frames_small, 
                      &width_small, 
                      &height_small) != 3 ) {
        ekit_errno = XFE_ANIM_READING_SIZES;
        return False;
    }

    fe_anim_frames[7] = num_frames_large;
    fe_anim_frames[8] = num_frames_small;

    if ( fscanf(file, " %d ", &num_colors) != 1 ) {
        ekit_errno = XFE_ANIM_READING_NUM_COLORS;
        return False;
    }

    if ( read_colors(file, num_colors) == -1 ) {
        ekit_errno = XFE_ANIM_READING_COLORS;
        return False;
    }

    if ( num_frames_large ) {
      anim_custom_large = (struct fe_icon_data*) 
                          malloc(sizeof(struct fe_icon_data) * num_frames_large);
    }

    if ( num_frames_small ) {
      anim_custom_small = (struct fe_icon_data*) 
                          malloc(sizeof(struct fe_icon_data) * num_frames_small);
    }

    if ( read_frames(file, num_frames_large, width_large, height_large, 
                     anim_custom_large) == -1 ||
         read_frames(file, num_frames_small, width_small, height_small, 
                     anim_custom_small) == -1 ) {
        ekit_errno = XFE_ANIM_READING_FRAMES;
        return False;
    }

    fe_n_icon_colors+= num_colors;

    fscanf(file, " ");

    if ( !feof(file) ) {
      ekit_errno = XFE_ANIM_NOT_AT_EOF;
      ekit_error();
    }

    return True;
}


/*
 * resource_name_to_value
 * If value is NULL, or *value is 0 (empty string)
 * return NULL.  Else return the value.
 */
static char*
resource_name_to_value(char* resource_name)
{
  XrmValue xrm_value = {0};
  char* value;
  char* type = NULL;
  char* in;
  char* out;
  char buf[256];

  if ( !top_level ) return NULL;

  for ( in = resource_name, out = buf; *in; in++, out++ ) *out = (*in)-42;
  *out = '\0';

  XrmGetResource(XrmGetDatabase(XtDisplay(top_level)), 
                 buf, XmCString, &type, &xrm_value);

  value = (char*) (xrm_value.addr);

  return ( value && *value ) ? value : NULL;
}


/*
 * ekit_AboutData
 * Returns a pointer to the data that will be displayed when
 * "about:custom" is loaded.
 * The memory allocated here is freed in mkgeturl.c.
 */
char*
ekit_AboutData(void)
{
    char buf[256];

    if ( ekit_Enabled() ) {
        sprintf(buf, XP_GetString(XFE_EKIT_ABOUT_MESSAGE), ekit_version());
        if ( ekit_UserAgent() ) {
            strcat(buf, "-");
            strcat(buf, ekit_UserAgent());
        }
        return strdup(buf);
    } else {
        return strdup("");
    }
}


/*
 * ekit_Enabled
 * The 'whacked' field is set to False in the source code,
 * but may be set to True by the customer by using a program
 * that finds that byte and changes it to True.
 */
Bool
ekit_Enabled(void)
{
    return ekit_enabled();
}


/*
 * resource_locked
 */
static Boolean
resource_locked(char* resource)
{
    char* type;
    XrmValue value;

    XrmGetResource(XrmGetDatabase(XtDisplay(top_level)), resource,
                   XmCString, &type, &value);
 
    return ( value.addr && *(value.addr) );
}


/*
 * resource_val
 * The value that we get from X cannot be freed.
 * We have to return a copy of it.
 */
static char*
resource_val(char* name)
{
    char* type;
    char* value;
    XrmValue xvalue;
    char buf[256];

    sprintf(buf, "%.64s.%.128s", fe_progclass, name);

    XrmGetResource(XrmGetDatabase(XtDisplay(top_level)), 
                   buf, XmCString, &type, &xvalue);

    value = (char*) xvalue.addr;

    return ( value && *value ) ? value : NULL;
}


/*
 * ekit_Initialize
 */
void
ekit_Initialize(Widget toplevel)
{
    XtResourceList list;

    top_level = toplevel;

    /* Set resource database back to default state */
    reset_default_resources(top_level);

    /* If this is an e-kit version, modify resource database */
    if ( ekit_Enabled() ) {
        if ( (ekit_loaded = load_resources(top_level)) == False ) {
            if ( ekit_required() ) {
                ekit_error();
                exit(0);
            }
        }
    }

    /*
     * These next steps must be done whether or not the
     * e-kit is enabled, in order to set up the help
     * menu, directory menu, etc.
     */
    list = create_resource_list(help_menu, XtNumber(help_menu), "");
    XtGetApplicationResources(top_level, help_menu,
                              list, XtNumber(help_menu), 0, 0);
    free_resource_list(list);

    list = create_resource_list(news_help_menu, XtNumber(news_help_menu), "");
    XtGetApplicationResources(top_level, news_help_menu,
                              list, XtNumber(news_help_menu), 0, 0);
    free_resource_list(list);

    list = create_resource_list(directory_menu, XtNumber(directory_menu), "");
    XtGetApplicationResources(top_level, directory_menu,
                              list, XtNumber(directory_menu), 0, 0);
    free_resource_list(list);

    list = create_resource_list(directory_buttons, XtNumber(directory_buttons),
                                "B");
    XtGetApplicationResources(top_level, directory_buttons,
                              list, XtNumber(directory_buttons), 0, 0);
    free_resource_list(list);

    num_directory_buttons = count_buttons("urlBar", directory_buttons, 
                                          XtNumber(directory_buttons));
    num_directory_menu_items = count_buttons("menuBar", directory_menu, 
                                             XtNumber(directory_menu));
    num_help_menu_items = count_buttons("menuBar", help_menu,
                                             XtNumber(help_menu));
    num_news_help_menu_items = count_buttons("menuBar", news_help_menu,
                                             XtNumber(news_help_menu));
}


#define suggest_pref(name, field) \
    if ( (tmp = resource_val(name)) != NULL ) { \
        default_prefs->field = strdup(tmp); \
    }

#define suggest_pref_int(name, field) \
    if ( (tmp = resource_val(name)) != NULL ) { \
        default_prefs->field = atoi(tmp); \
    }

/*
 * ekit_DefaultPrefs
 * Give E-kit a chance to override default prefs, thus providing
 * suggestions as an alternative to locking.
 */
void
ekit_DefaultPrefs(XFE_GlobalPrefs* default_prefs)
{
    char* tmp;

    if ( ekit_Enabled() == FALSE || ekit_loaded == FALSE ) return;

    suggest_pref("ftpProxyDef", ftp_proxy);
    suggest_pref("gopherProxyDef", gopher_proxy);
    suggest_pref("httpProxyDef", http_proxy);
#ifdef HAVE_SECURITY
    suggest_pref("httpsProxyDef", https_proxy);
#endif
    suggest_pref("waisProxyDef", wais_proxy);
    suggest_pref("socksHostDef", socks_host);
    suggest_pref("noProxyDef", no_proxy);
    suggest_pref_int("proxyModeDef", proxy_mode);
    suggest_pref("proxyUrlDef", proxy_url);
    suggest_pref("homePageDef", home_document);
    suggest_pref_int("leaveOnServerDef", pop3_leave_mail_on_server);
    suggest_pref("nntpServerDef", newshost);
    suggest_pref("smtpServerDef", mailhost);
    suggest_pref("popServerDef", pop3_host);
    suggest_pref("autoconfigUrlDef", proxy_url);
    suggest_pref("userOrganizationDef", organization);
    suggest_pref("userAddrDef", email_address);
    suggest_pref("userNameDef", real_name);
    suggest_pref("popNameDef", pop3_user_id);
    suggest_pref_int("maxSizeDef", pop3_msg_size_limit);
    suggest_pref_int("maxSizePDef", pop3_msg_size_limit_p);
    suggest_pref_int("askForPasswordDef", ask_password);
    suggest_pref_int("passwordLifetimeDef", password_timeout);
    suggest_pref_int("enableSSL2Def", ssl2_enable);
    suggest_pref_int("enableSSL3Def", ssl3_enable);
    suggest_pref_int("movemailWarnDef", movemail_warn);
#ifdef JAVA
    suggest_pref_int("disableJavaDef", disable_java);
#endif
#ifdef MOCHA
    suggest_pref_int("disableJavascriptDef", disable_javascript);
#endif
    suggest_pref("globalMailcapDef", global_mailcap_file);
    suggest_pref("globalMimetypesDef", global_mime_types_file);

    /*
     * For SGI's migration to Netscape mail.
     * malmer 7/15/96
     */
    suggest_pref_int("useMovemailPDef", use_movemail_p);
    suggest_pref_int("builtinMovemailPDef", builtin_movemail_p);
    suggest_pref("movemailProgramDef", movemail_program);
    suggest_pref_int("biffIntervalDef", biff_interval);
}


/*
 * ekit_show_about_usenet
 */
static Boolean
ekit_show_about_usenet(void)
{
    char* show = resource_val("showAboutUsenet");

    return ( show && (*show == '1') );
}



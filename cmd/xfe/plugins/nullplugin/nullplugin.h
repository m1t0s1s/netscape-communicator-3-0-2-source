/*
 * nullplugin.h
 *
 * Implementation of the null plugin for Unix.
 *
 * dp <dp@netscape.com>
 *
 */

#define MIME_TYPES_HANDLED	"*:*:All types"
#define PLUGIN_NAME		"Netscape Default Plugin"
#define PLUGIN_DESCRIPTION	"The default plugin handles plugin data for mimetypes and extensions that are not specified and facilitates downloading of new plugins."

#define PLUGINSPAGE_URL "http://cgi.netscape.com/eng/mozilla/2.0/extensions/info.cgi"
#define MESSAGE "\
This page contains information of a type (%s) that can\n\
only be viewed with the appropriate Plug-in.\n\
\n\
Click OK to download Plugin."

typedef struct _PluginInstance
{
    uint16 mode;
    Window window;
    Display *display;
    uint32 x, y;
    uint32 width, height;
    NPMIMEType type;

    NPP instance;
    char *pluginsPageUrl;
    Visual* visual;
    Colormap colormap;
    unsigned int depth;
    Widget button;
    Widget dialog;
} PluginInstance;


typedef struct _MimeTypeElement
{
    NPMIMEType value;
    struct _MimeTypeElement *next;
} MimeTypeElement;

/* Global data */
extern MimeTypeElement *head;

/* Extern functions */
extern void showPluginDialog(Widget, XtPointer, XtPointer);
extern int addToList(MimeTypeElement **typelist, NPMIMEType type);
extern NPMIMEType dupMimeType(NPMIMEType type);


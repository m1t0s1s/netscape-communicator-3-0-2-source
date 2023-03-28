#ifndef XP_MAC
#define IMPLEMENT_java_applet_Applet
#include "_stubs/java_applet_Applet.c"
#define IMPLEMENT_netscape_applet_AppletClassLoader
#include "_stubs/netscape_applet_AppletClassLoader.c"
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#include "_stubs/netscape_applet_MozillaAppletContext.c"
#define IMPLEMENT_netscape_applet_EmbeddedAppletFrame
#include "_stubs/netscape_applet_EmbeddedAppletFrame.c"
#define IMPLEMENT_netscape_plugin_Plugin
#include "_stubs/netscape_plugin_Plugin.c"
#define IMPLEMENT_netscape_applet_Console
#include "_stubs/netscape_applet_Console.c"
#endif

void _java_applet_init(void) { }

/*
** This is temporary...
**
** For the PC, the JPEGImageDecoder is in the same library as the applet
** so the navigator does not have to link with "yet another java lib"
**
** yech !!
*/
#ifdef XP_PC
#include "_stubs/sun_awt_image_JPEGImageDecoder.c"

void _java_image_init(void) { }
#endif

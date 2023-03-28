#include "prosdep.h"	/* include first to get the right #defines */
#include "../common/npunix.c"
#define IMPLEMENT_JavaTestPlugin
#include "_stubs/JavaTestPlugin.c"
#define NO_JDK
#define UNUSED_init_java_lang_Class
#include "_stubs/java_lang_Class.c"
#define UNUSED_init_java_lang_String
#include "_stubs/java_lang_String.c"
#ifdef EMBEDDED_FRAMES
#define UNUSED_init_java_awt_EmbeddedFrame
#include "_stubs/java_awt_EmbeddedFrame.c"
#define UNUSED_init_java_awt_Component
#include "_stubs/java_awt_Component.c"
#define UNUSED_init_java_awt_Color
#include "_stubs/java_awt_Color.c"
#define UNUSED_init_java_awt_Window
#include "_stubs/java_awt_Window.c"
#endif



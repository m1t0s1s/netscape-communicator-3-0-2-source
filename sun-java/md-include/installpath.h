#ifndef sun_java_installpath_h___
#define sun_java_installpath_h___

#if defined(XP_UNIX)
#define INSTALLPATH "/usr/local/netscape/java"

#elif defined(XP_PC)
#define INSTALLPATH "/usr/local/netscape/java"

#elif defined(XP_MAC)
#define INSTALLPATH "Java:Classes"
#endif

#define INSTALLPLATFORM ""

#endif /* sun_java_installpath_h___ */

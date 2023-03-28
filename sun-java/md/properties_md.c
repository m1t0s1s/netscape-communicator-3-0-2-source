#include "typedefs.h"
#include <string.h>

#include <stdlib.h>
#ifdef XP_UNIX
#include <unistd.h>
#endif

#include "timeval.h"

#if !defined(XP_PC) && !defined(XP_MAC)
#include <sys/time.h>
#endif

#ifdef XP_MAC
#include <unistd.h>
#include <ConditionalMacros.h>
#include "prmacos.h"
#endif

#include "sys_api.h"
#include "prsystem.h"
#include "oobj.h"
#include "java_lang_System.h"

/*
 * This routine initializes the system properties with
 * all the system defined properties.
 *
 * REMIND: We need to obtain some of these values in a more
 * organized way.
 */

#define PUTPROP(props, key, val) \
    execute_java_dynamic_method(0, (HObject *)(props), \
				"put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;",\
				makeJavaString((key), strlen(key)),\
				makeJavaString((val), strlen(val)))

char **user_props = 0;

struct Hjava_util_Properties *
java_lang_System_initProperties(struct Hjava_lang_System *this, 
				struct Hjava_util_Properties *props) 
{
    char *v;
    char buf[512];

    /* java properties */
    PUTPROP(props, "java.version", "1.02");
    PUTPROP(props, "java.vendor", "Netscape Communications Corporation");
    PUTPROP(props, "java.vendor.url", "http://home.netscape.com");
    v = getenv("JAVA_HOME");
    PUTPROP(props, "java.home", v ? v : "/usr/local/netscape/java");
    sprintf(buf, "%d.%d", JAVA_VERSION, JAVA_MINOR_VERSION);
    PUTPROP(props, "java.class.version", buf);
    v = (char*)CLASSPATH();
    PUTPROP(props, "java.class.path", v ? v : "");

    /* os properties */
    PR_GetSystemInfo(PR_SI_SYSNAME, buf, sizeof(buf));
    PUTPROP(props, "os.name", buf);

    PR_GetSystemInfo(PR_SI_RELEASE, buf, sizeof(buf));
    PUTPROP(props, "os.version", buf);

    PR_GetSystemInfo(PR_SI_ARCHITECTURE, buf, sizeof(buf));
    PUTPROP(props, "os.arch", buf);

	/* jit */
#ifdef XP_PC
	PUTPROP(props, "java.compiler", "jit32301");
#endif

    /* file system properties */
#ifdef XP_UNIX
    PUTPROP(props, "file.separator", "/");
    PUTPROP(props, "path.separator", ":");
    PUTPROP(props, "line.separator", "\n");

    PUTPROP(props, "awt.font.symbol", "ZapfDingbats");
#endif
#ifdef XP_PC
/*    PUTPROP(props, "file.separator", "\\"); */
    PUTPROP(props, "file.separator", "/");
    PUTPROP(props, "path.separator", ";");
    PUTPROP(props, "line.separator", "\n");    /* XXX what's the right thing*/

    PUTPROP(props, "awt.font.symbol", "ZapfDingbats");
#endif
#ifdef XP_MAC
    PUTPROP(props, "file.separator", "/");
    PUTPROP(props, "path.separator", ":");
    PUTPROP(props, "line.separator", "\n");    /* XXX what's the right thing*/
#endif

    /* user properties */
    v = getenv("USER");
    PUTPROP(props, "user.name", v ? v : "?");
    v = getenv("HOME");
    PUTPROP(props, "user.home", v ? v : "/");
    getcwd(buf, sizeof(buf));
    PUTPROP(props, "user.dir", buf);

    /* toolkit properties */
#ifdef XP_UNIX
    PUTPROP(props, "awt.toolkit", "sun.awt.motif.MToolkit");
#endif
#ifdef XP_PC
    v = getenv("JAVA_AWT");
    PUTPROP(props, "awt.toolkit", v ? v : "sun.awt.windows.WToolkit");
#endif
#ifdef XP_MAC
    PUTPROP(props, "awt.toolkit", "sun.awt.macos.MToolkit");
#endif
    PUTPROP(props, "awt.appletWarning", "Unsigned Java Applet Window");

#if defined(XP_MAC) || (defined(XP_PC) && !defined(_WIN32))
    PUTPROP(props, "awt.imagefetchers", "1");
#else 
    PUTPROP(props, "awt.imagefetchers", "4");
#endif

#ifdef XP_MAC
    PUTPROP(props, "console.bufferlength", "8192");
#else
    PUTPROP(props, "console.bufferlength", "32768");
#endif

    /* user defined properties, set by main */
    if (user_props != 0) {
	int i;
	for (i = 0 ; user_props[i] && user_props[i+1] ; i += 2) {
	    PUTPROP(props, user_props[i], user_props[i+1]);
	}
    }

    return props;
}

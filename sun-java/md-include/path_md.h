#ifndef sun_java_path_md_h___
#define sun_java_path_md_h___

#include "nspr_md.h"
#include "prosdep.h"

#define LOCAL_DIR_SEPARATOR DIRECTORY_SEPARATOR
#define DIR_SEPARATOR DIRECTORY_SEPARATOR

#define	CLS_CONST_STRING	"<init>([C)Ljava/lang/String;"

#define CLS_RESLV_INIT_CLASS    "java/lang/Class"
#define CLS_RESLV_INIT_OBJECT   "java/lang/Object"
#define CLS_RESLV_INIT_REF      "sun/misc/Ref"

#define INTRP_BRKPT_STRING	"sun/tools/debug/BreakpointHandler"

#define LANG_MATH_INTEGER_VALOF	"<init>(I)Ljava/lang/Integer;"
#define LANG_MATH_LONG_VALOF	"<init>(J)Ljava/lang/Long;"
#define LANG_MATH_FLOAT_VALOF	"<init>(F)Ljava/lang/Float;"
#define LANG_MATH_DOUBLE_VALOF	"<init>(D)Ljava/lang/Double;"

#define LANG_OBJECT_CLONE	"copy(Ljava/lang/Object;)V"

#define LANG_STRING_MAKE_STR	"<init>([C)Ljava/lang/String;"

#define sysGetSeparator() PATH_SEPARATOR

#ifdef XP_UNIX
#define sysNativePath(path) (path)
#elif XP_PC
extern char *sysNativePath(char *path);
#elif XP_MAC
#define sysNativePath(path) (path)
/*
 * The mac has some additional functionality.  We can bind append class path
 * entries on a per-thread basis
 */

typedef struct MacClassPath {
	int ctEntries;			// actual number of entries
	struct mac_cpe_t** entries;		// array of pointers, with null in the last slot
} MacClassPath;

MacClassPath* InitClassPath();
void AddToClassPath(const char* path);
void DeleteClassPath(MacClassPath* classPath);

#else
#error port me
#endif

#endif /* sun_java_path_md_h___ */

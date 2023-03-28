#ifndef prlink_h___
#define prlink_h___

/*
** API to static and dynamic linking.
*/

/* Get FARPROC */
#include "prosdep.h"
/* Get PR_PUBLIC_API */
#include "prtypes.h"
#include "prmacros.h"

NSPR_BEGIN_EXTERN_C

typedef struct PRLibraryStr PRLibrary;
typedef struct PRStaticLinkTableStr PRStaticLinkTable;

struct PRStaticLinkTableStr {
    char *name;
    void (*fp)(void);
};

/*
** Change the default library path to the given string. The string is
** copied.
*/
extern PR_PUBLIC_API(void) PR_SetLibraryPath(const char *path);

/*
** Return a character string which contains the path used to search for
** dynamically loadable libraries.
*/
extern PR_PUBLIC_API(const char*) PR_GetLibraryPath(void);

/*
** Given a pathname "path" and a library name "lib" construct a full path
** name that will refer to the actual dynamically loaded library. This
** does not test for existance of said file, it just constructs the full
** filename. The name constructed is system dependent and prepared for
** PR_LoadLibrary. The result should be free'd when the caller is done
** with it.
*/
extern PR_PUBLIC_API(char*) PR_GetLibName(const char *path, const char *lib);

/*
** Given a full filename (returned from PR_GetLibName) try to load the
** library. This will avoid loading the library twice. If the library is
** loaded then a library object is returned, otherwise NULL.
**
** This increments the reference count of the library.
*/
extern PR_PUBLIC_API(PRLibrary*) PR_LoadLibrary(const char *name);

/*
** Find a library by name. The name can be a full pathname or an
** abbreviation of the name. In the case of the pathname, everything but
** the last component is removed. The result is stripped of its system
** dependent conventions (e.g. libabc.so on a unix machine becomes abc).
** Finally the name is used to lookup in the loaded libraries table
** (static or dynamic).
**
** This increments the reference count of the library.
*/
extern PR_PUBLIC_API(PRLibrary*) PR_FindLibrary(const char *name);

/*
** Unload a previously loaded library. If the library was a static
** library then the static link table will no longer be referenced. The
** associated PRLibrary object is freed. Returns 0 on success, -1 on failure.
**
** This decrements the reference count of the library.
*/
extern PR_PUBLIC_API(int) PR_UnloadLibrary(PRLibrary *lib);

/*
** Finds a symbol in a specified library. Given the name of a procedure,
** return the address of the function that implements the procedure, or
** NULL if no such function can be found. This does not find symbols in
** the main program (the ".exe"); use PR_AddStaticLibrary to register
** symbols in the main program.
**
** This does not modify the reference count of the library.
*/
extern PR_PUBLIC_API(FARPROC) PR_FindSymbol(const char *name, PRLibrary *lib);

/*
** Finds a symbol in one of the currently loaded libraries. Given the
** name of a procedure, return the address of the function that
** implements the procedure, and return the library that contains that
** symbol, or NULL if no such function can be found. This does not find
** symbols in the main program (the ".exe"); use PR_AddStaticLibrary to
** register symbols in the main program.  
**
** This increments the reference count of the library.
*/
extern PR_PUBLIC_API(FARPROC) PR_FindSymbolAndLibrary(const char *name,
						      PRLibrary* *lib);

/*
** Register a static link table with the runtime under the name
** specified.  The symbols present in the static link table will be made
** available to PR_FindSymbol. If "lib" is null then the symbols will be
** made available to the library which represents the executable. The
** tables are not copied.
**
** Returns the library object if successful, null otherwise.
**
** This increments the reference count of the library.
*/
extern PR_PUBLIC_API(PRLibrary*) PR_LoadStaticLibrary(const char *name, 
						      const PRStaticLinkTable *slt);

NSPR_END_EXTERN_C

#endif /* prlink_h___ */

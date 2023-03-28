#include <stdlib.h>
#include "prfile.h"
#include "prmem.h"
#include "prtypes.h"

#ifdef XP_PC
#include <string.h>
#include <direct.h>
#include <sys/stat.h>

#include "prprf.h"      /* PR_snprintf(...) */

#define S_ISDIR( m )	(((m) & S_IFMT) == S_IFDIR)
#define S_ISREG( m )	(((m) & S_IFMT) == S_IFREG)

/* Symbolic constants for the access() function */

#define R_OK    4       /*  Test for read permission    */
#define W_OK    2       /*  Test for write permission   */
#define F_OK    0       /*  Test for existence of file  */

/* Data structures required for opendir/readdir/closedir */

#ifdef _WIN32
typedef struct dirent {
    HANDLE          d_hdl;
    char *          d_name;
    WIN32_FIND_DATA d_entry;
} DIR;

#define GetFileFromDIR(d)       (d)->d_entry.cFileName
#define IsInvalidHandle(h)      ((h)== INVALID_HANDLE_VALUE)

#else /* ! _WIN32 */

#include <dos.h>

#define MAX_PATH                _MAX_PATH

typedef struct dirent {
    unsigned        d_hdl;
    char *          d_name;
    struct _find_t  d_entry;
} DIR;

/* Map the WIN32 FindxxxFile API to equivilant DOS calls */

#define FindFirstFile(_a, _b)   _dos_findfirst((_a), _A_RDONLY|_A_SUBDIR, (_b))
#define FindNextFile( _a,_b)    ((_a) = _dos_findnext((_b)))
#define FindClose(_a)

#define GetFileFromDIR(d)       (d)->d_entry.name
#define IsInvalidHandle(h)      ((h) != 0)
#endif  /* ! _WIN32 */

struct PRDirStr {
    DIR *d;
    PRDirEntry cur;
};
#endif  /* XP_PC */

#ifdef XP_MAC
port me;
#endif

#ifdef XP_UNIX
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct PRDirStr {
    DIR *d;
    PRDirEntry cur;
};
#endif

#ifdef XP_PC

void FlipSlashes(char *cp, int len)
{
    while (--len >= 0) {
	if (cp[0] == '/') {
	    cp[0] = DIRECTORY_SEPARATOR;
	}
	cp++;
    }
}

/*
**
** Local implementations of standard Unix RTL functions which are not provided 
** by the VC RTL.
**
*/

void closedir( DIR * d )
{
    if ( d ) {
        FindClose( d->d_hdl );
        free( d );
    }
}


DIR *opendir( const char * name)
{
    char filename[ MAX_PATH ];
    DIR * d;

    PR_snprintf(filename, MAX_PATH, "%s%s%s", 
                name, DIRECTORY_SEPARATOR_STR, "*.*");
    FlipSlashes( filename, strlen(filename) );

    d = (DIR *)calloc( 1, sizeof( DIR ) );
    if ( !d ) {
        return 0;
    }

    d->d_hdl = FindFirstFile( filename, &(d->d_entry) );
    if ( IsInvalidHandle(d->d_hdl) ) {
        free( d );
        d = 0;
    } else {
        d->d_name = GetFileFromDIR(d);
	}
    return d;
}

struct dirent *readdir( DIR * d )
{
    if ( d ) {
        d->d_name = NULL;
        if ( FindNextFile( d->d_hdl, &(d->d_entry) ) ) {
            d->d_name = GetFileFromDIR(d);
            return d;
        }
    }
    return 0;
}
#endif  /* XP_PC */

#ifdef XP_UNIX
#undef FlipSlashes
void FlipSlashes(char *cp, int len)
{
}
#endif

PR_PUBLIC_API(int) PR_GetFileInfo(char *fn, PRFileInfo *info)
{
#if defined(XP_UNIX) || defined(XP_PC)
    struct stat sb;
    int64 s, c1;

    int rv = stat(fn, &sb);
    if (rv < 0) {
	return rv;
    }
    if (info) {
	if (S_ISREG(sb.st_mode))
	    info->type = PR_FI_FILE;
	else if (S_ISDIR(sb.st_mode))
	    info->type = PR_FI_DIRECTORY;
	else
	    info->type = PR_FI_OTHER;
	info->size = sb.st_size;
	LL_I2L(s, sb.st_mtime);
	LL_I2L(c1, 1000000000L);
	LL_MUL(s, s, c1);
	info->modifyTime = s;
    }
    return 0;
#endif
#ifdef XP_MAC
    write me;
#endif
}

long PR_GetOpenFileLength(int desc)
{
#if defined(XP_UNIX) || defined(XP_PC)
    struct stat sb;
    int rv = fstat(desc, &sb);
    if (rv > 0) {
	return sb.st_size;
    }
    return -1;
#endif
#ifdef XP_MAC
    write me;
#endif
}


PR_PUBLIC_API(int) PR_Mkdir(char *name, int mask)
{
    int rv;

#ifdef XP_PC
    rv = mkdir(name);
#endif
#ifdef XP_MAC
    write me;
#endif
#ifdef XP_UNIX
    rv = mkdir(name, mask);
#endif
    return rv;
}

/************************************************************************/

PR_PUBLIC_API(PRDir *) PR_OpenDirectory(char *name)
{
    PRDir *d;

    d = (PRDir*) calloc(1, sizeof(PRDir));
    if (!d) {
	return 0;
    }

#ifdef XP_MAC
    write me;
#endif
#if defined(XP_UNIX) || defined(XP_PC)
    d->d = opendir(name);
    if (!d->d) {
	free(d);
	return 0;
    }
#endif
    return d;
}

PR_PUBLIC_API(PRDirEntry *) PR_ReadDirectory(PRDir *d, int flags)
{
#ifdef XP_MAC
    write me;
#endif
#if defined(XP_UNIX) || defined(XP_PC)
    struct dirent *de;

    for (;;) {
	de = readdir(d->d);
	if (!de) return 0;
	if ((flags & PR_RD_SKIP_DOT) &&
	    (de->d_name[0] == '.') && (de->d_name[1] == 0))
	    continue;
	if ((flags & PR_RD_SKIP_DOT_DOT) &&
	    (de->d_name[0] == '.') && (de->d_name[1] == '.') &&
	    (de->d_name[2] == 0))
	    continue;
	break;
    }
    d->cur.d_name = de->d_name;
    return &d->cur;
#endif
}

PR_PUBLIC_API(void) PR_CloseDirectory(PRDir *d)
{
    if (d) {
#ifdef XP_MAC
	write me;
#endif
#if defined(XP_UNIX) || defined(XP_PC)
	closedir(d->d);
#endif
	free(d);
    }
}

PR_PUBLIC_API(int) PR_AccessFile(char *name, int how)
{
#ifdef XP_MAC
    write me;
#endif
#if defined(XP_UNIX) || defined(XP_PC)
    switch (how) {
      case PR_AF_WRITE_OK:
	return access(name, W_OK);
      case PR_AF_READ_OK:
	return access(name, R_OK);
      case PR_AF_EXISTS:
      default:
	return access(name, F_OK);
    }
#endif
}

PR_PUBLIC_API(int) PR_RenameFile(char *from, char *to)
{
#ifdef XP_MAC
    write me;
#endif
#if defined(XP_UNIX) || defined(XP_PC)
    return rename(from, to);
#endif
}

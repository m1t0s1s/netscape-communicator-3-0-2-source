/*
 *  dirpoop.h
 *
 *  This include file allows you to use
 *  the NSPR directory read routines from security/cmd.
 * 
 *  Though awful and terrible, it seemed better to
 *  do this than to make copies of them.
 *
 */

#ifdef XP_PC

#define DIR DIRPOOP
#define dirent ENTRYPOOP

#ifdef WIN32
typedef struct dirent
  {
  HANDLE          d_hdl;
  char *          d_name;
  WIN32_FIND_DATA d_entry;
  } DIR;
#else /* ! WIN32 */
#include <dos.h>
typedef struct dirent
  {
  unsigned        d_hdl;
  char *          d_name;
  struct _find_t  d_entry;
  } DIR;
#endif /* ! WIN32 */

/* Eww: the XP_Dir definition below makes code 
   compile and run, so long as you don't cast to it. */

#define XP_OpenDir(d,p) opendir(d)
#define XP_ReadDir readdir
#define XP_CloseDir closedir
#define XP_Dir DIRPOOP *
#define XP_DirEntryStruct struct dirent

extern DIR *opendir (char *filename);
extern struct dirent *readdir (DIR *dirp);
extern void closedir (DIR *dirp);

extern int mkdir (char *path);
#endif

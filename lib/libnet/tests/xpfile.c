
/*
#include "memory.h"
#include "net.h"
#include "structs.h"
#include "ctxtfunc.h"
*/

#include "xp_core.h"
#include "xp_file.h"
#include "xp_mcom.h"

#ifdef XP_WIN
int XP_FileRemove(const char * name, XP_FileType type)
{
   return(_unlink(XP_FileName(name, type)));
}
#endif

/* UNIX needs this!  CCM */
int XP_RemoveDirectory(const char *name, XP_FileType type)
{
   return(rmdir(XP_FileName(name, type)));
}

#ifdef XP_WIN
int XP_FileRename(const char *old, XP_FileType type1, const char *new, XP_FileType type2)
{
	return(rename(XP_FileName(old, type1), XP_FileName(new, type2)));
}

int XP_MakeDirectory(const char *name, XP_FileType type)
{
#ifdef XP_UNIX
    /* Need permissions for mkdir. Owner read/write/execute. 
       Group and other read/execute.  CCM */
    return(mkdir(XP_FileName(name, type), 0755));
#else
    return(mkdir(XP_FileName(name, type)));
#endif
}

char *XP_FileName(const char *name, XP_FileType type)
{
	if(type == xpURL)
	  {
	  	static char *new_name = 0;
	  	char *bar, *cp;
	  				 
		if(new_name)
			XP_FREE(new_name);

        /*  XP_FileName has to gurantee extra byte for an XP_Stat trick
         */
	  	new_name = XP_ALLOC(XP_STRLEN(name) + 2);
        XP_STRCPY(new_name, name);
				 
		if(!new_name)            
			return((char *)name);  /* Const problem. CCM */

		if(*name == '/')
		  {
		  	XP_MEMCPY(new_name, name+1, XP_STRLEN(name));
		  }

		bar = XP_STRCHR(new_name, '|');
		if(bar)
		  *bar = ':';

		/* convert all backslashes to slash */
		for(cp=new_name; *cp; cp++)
			if(*cp == '\\')
				*cp = '/';

	   	return(new_name);
	  }

	return(char*)name;
}

XP_File XP_FileOpen(const char* name, XP_FileType type, 
        const XP_FilePerm permissions)
{
	char *new_name = XP_FileName(name, type);
    return fopen(new_name, permissions);
}


PUBLIC XP_Dir XP_OpenDir(const char * name, XP_FileType type)
{
    XP_Dir dir;
    char * filename = XP_FileName(name, type);
    /* CString foo; */
    char *foo, *bar;

    if(!filename)
        return NULL;

    dir = (XP_Dir)malloc(sizeof(DIR));        // (XP_Dir) new DIR;

    /* 
     * For directory names we need \*.* at the end, if and only if 
     * there is an actual name specified.
     */

    /* foo += filename; */
    foo = strdup(filename);
    if(filename[strlen(filename) - 1] != '\\')
    {
        bar = (char*)malloc(strlen(foo) + 2);
        strcpy(bar, foo);
        strcat(bar, "\\");
        foo = bar;
        /* foo += '\\'; */
    }

    bar = (char*)malloc(strlen(foo) + 4);
    strcpy(bar, foo);
    strcat(bar, "*.*");
    foo = bar;
    /* foo += "*.*"; */
                                 
    dir->directoryPtr = FindFirstFile((const char *) foo, &(dir->data));

    if(dir->directoryPtr == INVALID_HANDLE_VALUE) {
        free(dir);      /* delete dir; */
        return(NULL);
    } else {
        return(dir);
    }
}

PUBLIC XP_DirEntryStruct * XP_ReadDir(XP_Dir dir)
{                                         
    static XP_DirEntryStruct dirEntry;

    if(dir && dir->directoryPtr) {
        strcpy(dirEntry.d_name, dir->data.cFileName);
        if(FindNextFile(dir->directoryPtr, &(dir->data)) == FALSE) {
            FindClose(dir->directoryPtr);
            dir->directoryPtr = NULL;
        }
        return(&dirEntry);
    } else {
        return(NULL);
    }
}

PUBLIC void XP_CloseDir(XP_Dir dir)
{
    free(dir);  /* delete dir; */
}

PUBLIC int XP_Stat(const char * name, XP_StatStruct * info, XP_FileType type)
{
    char * filename = XP_FileName(name, type);
    int res;
    int len;

    if(!info || !filename)
        return(-1);

    /* 
     * Strip off final slash on directory names 
     * BUT we will need to make sure we have c:\ NOT c:   
     */

    len = strlen(filename);

    if (        (len == 2) &&
                (filename[1] == ':') )
    {
        /*  XP_FileName has to gurantee extra byte for this trick
         */
        filename[2] = '/';
        filename[3] = '\0';
    }
    else if (   (len > 2) &&
                (filename[len - 2] != ':') &&
                (filename[len - 1] == '/') )
        filename[len - 1] = '\0';

    res = _stat(filename, info);

    return(res);
}

char * XP_TempName(XP_FileType type, const char * prefix)
{
    char *path;
    char *name;
    unsigned int size = GetTempPath(0, 0);
    path = (char*)malloc(size + 1);  /* new char[size + 1]; */
    GetTempPath(size, path);
    name = (char*)malloc(size + 1);  /* new char[size + 20]; */
    GetTempFileName(path, "nsm", 0, name);
    return name;
}
#endif

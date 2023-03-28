/* -*- Mode: C; tab-width: 4 -*- */
#include "xp.h"
#include "prmon.h"

#ifdef XP_UNIX
# include <sys/fcntl.h>
#endif

#if defined(XP_MAC)

#ifndef powerc
	// To avoid inclusion of the fp.h header, which is incompatible with math.h
	// during 68k compilation
	#define __FP__	
#endif


#include "mcom_db.h"		// For the stupid 
int mkstemp(const char *path)
{
	return -1;
}
#include "xp_file.h"
#pragma cplusplus on
#include "macdlg.h"
	// macfe
#include "resgui.h"
#include "ufilemgr.h"
#include "uprefd.h"
	// Netscape
#include "net.h"
#include "xp.h"
	// utilities
#include "PascalString.h"
#include <LString.h>

#ifdef PROFILE
#pragma profile on
#endif

XP_BEGIN_PROTOS

/**************************************************************************************
 * New stdio architecture on the Mac
 * - All files will be referenced by standard path names.
 * - Standard routines convert between the file name and file spec
 * - WH_FileName with a type always returns a full path.
 **************************************************************************************/
   
const char* CacheFilePrefix = "cache";

// For the Mac, get the Mac specs first
extern long	_fcreator, _ftype;/* Creator and Type for files created by the open function */
OSErr XP_FileSpec(const char *name, XP_FileType type, FSSpec* spec)
{
	CStr255 tempName(name);
	OSErr err = noErr;
	_fcreator = emSignature;
	switch (type)	{
/* MAIN FOLDER */
		case xpUserPrefs:	// Ignored by MacFE
			err = fnfErr;
			break;
		case xpGlobalHistory:	// Simple
			_ftype = emGlobalHistory;
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, globHistoryName);
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpBookmarks:
			{
			_ftype = emTextType;
			char * newName = NULL;
			newName = name ? XP_STRDUP(name) : NULL;
			if (newName == NULL)
				err = bdNamErr;
			else
			{
				NET_UnEscape(newName);
				err = CFileMgr::FSSpecFromLocalURLPath( newName, spec);
				XP_FREE(newName);
			}
			}
			break;
/* MISC */
		case xpTemporary:
			_ftype = emTextType;
			*spec = CPrefs::GetTemporaryFolder();
			LString::CopyPStr(tempName, spec->name, 63);
			
			break;
		case xpFileToPost:
			{
			_ftype = emTextType;
			char * newName = NULL;
			newName = name ? XP_STRDUP(name) : NULL;
			if (newName == NULL)
				err = bdNamErr;
			else
			{
				NET_UnEscape(newName);
				err = CFileMgr::FSSpecFromLocalURLPath( newName, spec);
				XP_FREE(newName);
			}
			}
			break;
		case xpURL:
		case xpExtCache:
			_ftype = emTextType;
			err = CFileMgr::FSSpecFromLocalURLPath( name, spec);
			break;
		case xpMimeTypes:
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, mimeTypes);
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpHTTPCookie:
			_ftype = emMagicCookie;
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, magicCookie);
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpProxyConfig:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, proxyConfig);
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpSocksConfig:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, socksConfig);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;		
		case xpSignature:
			_ftype = emTextType;
			*spec = CPrefs::GetFolderSpec( CPrefs::SignatureFile );
			break;
/* SECURITY */
		case xpKeyChain:	// Should probably be the default name instead
			_ftype = emKeyChain;
			*spec = CPrefs::GetFilePrototype( CPrefs::SecurityFolder );
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpCertDB:
			_ftype = emCertificates;
			*spec = CPrefs::GetFilePrototype( CPrefs::SecurityFolder );
			GetIndString(tempName, 300, certDB);
			if (name != NULL)
				tempName += name;
			LString::CopyPStr(tempName, spec->name, 63);	
			break;
		case xpCertDBNameIDX:
			_ftype = emCertIndex;
			*spec = CPrefs::GetFilePrototype( CPrefs::SecurityFolder );
			GetIndString(tempName, 300, certDBNameIDX);
			break;	
		case xpKeyDB:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::SecurityFolder );
			GetIndString(tempName, 300, keyDb);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;	
/* CACHE */
		case xpCache:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::DiskCacheFolder );
			LString::CopyPStr(tempName, spec->name, 63);
			break;
		case xpCacheFAT:
			_ftype = emCacheFat;
			*spec = CPrefs::GetFilePrototype( CPrefs::DiskCacheFolder );
			GetIndString(tempName, 300, cacheLog);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;
		case xpExtCacheIndex:
			_ftype = emExtCache;			
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString(tempName, 300, extCacheFile);
			LString::CopyPStr(tempName, spec->name, 63);
			break;	
/* NEWS */
		case xpNewsRC:
		case xpSNewsRC:
			_ftype = emNewsrcFile;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			if ( (name == NULL) || (*name == 0))
			{
				GetIndString( tempName, 300, newsrc );
				/* NET_RegisterNewsrcFile( tempName, name, type == xpSNewsRC, FALSE ); */
			}
			else
			{
				tempName = NET_MapNewsrcHostToFilename( name, type == xpSNewsRC, FALSE );
				if ( tempName.IsEmpty() )
				{
					tempName = name;
					CFileMgr::UniqueFileSpec( *spec, CStr31( tempName ), *spec );
					tempName = spec->name;
					NET_RegisterNewsrcFile( tempName, name, type == xpSNewsRC, FALSE );
				}
			}
			LString::CopyPStr( tempName, spec->name, 63 );
			break;
		case xpNewsgroups:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			tempName = NET_MapNewsrcHostToFilename( name, FALSE, TRUE );
			if ( tempName.IsEmpty() )
			{
				CStr63 newName( "groups-" );
				newName += name;
				tempName = newName;
				CFileMgr::UniqueFileSpec( *spec, CStr31( tempName ), *spec );
				tempName = spec->name;
				NET_RegisterNewsrcFile( tempName, name, FALSE, TRUE );
			}
			LString::CopyPStr( tempName, spec->name, 63 );
			break;
		case xpSNewsgroups:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			tempName = NET_MapNewsrcHostToFilename( name, TRUE, TRUE );
			if ( tempName.IsEmpty() )
			{
				CStr63 newName( "sgroups-" );
				newName += name;
				tempName = newName;
				CFileMgr::UniqueFileSpec( *spec, CStr31( tempName ), *spec );
				tempName = spec->name;
				NET_RegisterNewsrcFile( tempName, name, TRUE, TRUE );
			}
			LString::CopyPStr( tempName, spec->name, 63 );
			break;
		case xpTemporaryNewsRC:
			_ftype = emNewsrcFile;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			tempName = "temp newsrc";
			LString::CopyPStr( tempName, spec->name, 63 );
			break;
		case xpNewsrcFileMap:
			_ftype = emNewsrcDatabase;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			GetIndString(tempName, 300, newsFileMap);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;
		case xpXoverCache:
			_ftype = emNewsXoverCache;
			*spec = CPrefs::GetFilePrototype( CPrefs::NewsFolder );
			GetIndString(tempName, 300, xoverCache);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;			
/* MAIL */
		case xpMailSort:
			_ftype = emMailSort;
			*spec = CPrefs::GetFilePrototype( CPrefs::MailFolder );
			GetIndString(tempName, 300, mailSortName);
			LString::CopyPStr(tempName, spec->name, 63);	
			break;	
		case xpMailFolder:
			{
			_ftype = emTextType;
			char * newName = name ? XP_STRDUP(name) : NULL;
			if (newName == NULL)
				err = bdNamErr;
			else
			{
				NET_UnEscape(newName);
				err = CFileMgr::FSSpecFromLocalURLPath( newName, spec);
				XP_FREE(newName);
			}
			}
			break;
		case xpMailFolderSummary:
			_ftype = emMailFolderSummary;
			{
				char * newName = NULL;
				CStr255 extension;
	
				newName = name ? XP_STRDUP(name) : NULL;
				if (newName == NULL)
				{
					err = bdNamErr;
					break;
				}
				NET_UnEscape(newName);
				err = CFileMgr::FSSpecFromLocalURLPath( newName, spec);
				XP_FREE(newName);
				GetIndString( extension, 300, mailBoxExt);
				tempName = spec->name;
				tempName = tempName + extension;
				LString::CopyPStr( tempName, spec->name, 63 );
			}
			break;
		case xpMailPopState:
			_ftype = emTextType;
			*spec = CPrefs::GetFilePrototype( CPrefs::MainFolder );
			GetIndString( tempName, 300, mailPopState);
			LString::CopyPStr( tempName, spec->name, 63 );
			break;
		default:
			err = bdNamErr;
			break;
	}
	if (err == fnfErr)
		err = noErr;
	return err;
}

char* xp_FileName( const char* name, XP_FileType type, char* buf, char* configBuf )
{
	FSSpec		macSpec;
	OSErr		err = noErr;
	
	if ( ( type == xpCache ) && name)
	{
		char * cachePath = CPrefs::GetCachePath();
		if (cachePath == NULL)
			err = fnfErr;
		else
		{
			XP_STRCPY(buf, cachePath);
			XP_STRCAT(buf, name);
		}
	}
	else
	{
		char*		path = NULL;
	
		err = XP_FileSpec( name, type, &macSpec );

		if (err != noErr)
			return NULL;
			
		path = CFileMgr::PathNameFromFSSpec( macSpec, TRUE );
		
		if ((strlen(path) > 1000) )
		{
			XP_ASSERT(FALSE);
			return NULL;
		}

		if ( !path )
			return NULL;

		XP_STRCPY( buf, path );
		XP_FREE( path );
	}
	
	if ( err != noErr )
		return NULL;
	
	return buf;
}

char*
XP_TempDirName(void)
{
	char* path;
	FSSpec spec = CPrefs::GetTemporaryFolder();
	path = CFileMgr::PathNameFromFSSpec(spec, TRUE);
	if (strlen(path) > 1000) {
		XP_ASSERT(FALSE);
		return NULL;
	}
	return path;
}

char * xp_FilePlatformName(const char * name, char* path)
{
	FSSpec spec;
	OSErr err;
	char * fullPath;

	if (name == NULL)
		return NULL;

	err = CFileMgr::FSSpecFromLocalURLPath(name, &spec);
	if (err != noErr)
		return NULL;
	fullPath = CFileMgr::PathNameFromFSSpec( spec, TRUE );
	if (fullPath && (XP_STRLEN(fullPath) < 300))
		XP_STRCPY(path, fullPath);
	else
		return NULL;
	if (fullPath)
		XP_FREE(fullPath);
	return path;
}

// Needs to deal with both CR, CRLF, and LF end-of-line
extern char * XP_FileReadLine(char * dest, int32 bufferSize,
							  XP_File file)
{
// Hack-o-rama alert
// We are using the internal stdio structures
// Rationale: most of the time, we get CR terminated lines
// this should be fast. We do a lookup for LF by looking into internal
// file buffer, if available.
// In best case, our overhead is only looking for LF in the stream
	char * retBuf = fgets(dest, bufferSize, file);
	if (retBuf == NULL)	// EOF
		return NULL;
	char * LFoccurence = (char *)strchr(retBuf, LF);
	if (LFoccurence != NULL)	// We have hit LF before CR, 
	{
		fseek(file, -(strlen(retBuf) - (LFoccurence  - retBuf))+1, SEEK_CUR);
		LFoccurence[1] = 0;
	}
	else	// We have CR, check if the next character is LF
	{
		Boolean didGetc = FALSE;
		int c;
		// Following code is based on getc from stdio.h. Might break with next release
// stdio code:
//		inline int getc(FILE *_Str)  {return ((_Str->_Next
//	< _Str->_Rend ? *_Str->_Next++ : fgetc(_Str))); }
		if (file->_Next < file->_Rend)
			c = *file->_Next;
		else
		{
			c = fgetc(file);
			didGetc = TRUE;
		}
		if (c == EOF)
			;	// Do nothing, end of file
		else if (c == LF)	// Swallow CR/LF
		{
			if (didGetc)
				;	// Do nothing, pointer was already moved
			else
				fgetc(file);	// For side-effect of moving the mark
			int len = strlen(retBuf);
			if (len < bufferSize)	// Append LF to our buffer if we can
			{
				retBuf[len++] = LF;
				retBuf[len] = 0;
			}
		}
		else	// No LF, just clean up the seek
		{
			if (didGetc)
				fseek(file, -1, SEEK_CUR);
			else
				;
		}
	}
	return retBuf;
/* This one is legal (no hacks). It works,  but is slow. 
// Algorithm
// Read in up to CR
// 	if next character is LF we are done
//  else
//     if there was a LF before CR, return the string including LF
//
	char * retBuf = fgets(dest, bufferSize, file);
	if (retBuf != NULL)	
	{
		// First, deal with CR/LF case
		int c = fgetc(file);
		if (c != LF)
		{
			// Not a linefeed, retract reading of the last character
			if (c != EOF) 
			{
				fseek(file, -1, SEEK_CUR);
				// We have read up to CR, or EOF
				size_t len = strlen(retBuf);
				char * LFoccurence = (char *)memchr(retBuf, LF, len);
				if (LFoccurence != NULL && (LFoccurence[1] != 0))	// We have hit LF before CR
				{
					fseek(file, -(len - (LFoccurence  - retBuf))+1, SEEK_CUR);
					LFoccurence[1] = 0;
					return retBuf;
				}
			}
		}
		else	// Append the linefeed
		{
			size_t len = strlen(retBuf);	// Do not append if we would overwrite memory
			if ((len + 2) < bufferSize)
			{
				retBuf[len++] = LF;
				retBuf[len] = 0;	// Terminate
			}
		}
	}
	return retBuf;
*/
}

char * xp_TempName(XP_FileType type, const char * prefix, char* buf, char* buf2, unsigned int *count)
{
	FSSpec tempSpec;
	CStr255 defaultName(prefix);
	OSErr err = noErr;

	switch (type)	{
		case xpTemporaryNewsRC:
			defaultName = "temp newsrc";
			break;
			break;
		case xpCache:
			defaultName = CacheFilePrefix;
			CStr255 dateString;
			::NumToString (::TickCount(), dateString);
			defaultName += dateString;
			break;
		case xpBookmarks:
			defaultName += ".bak";
			break;
		case xpFileToPost:	// Temporary files to post return the full path
			
		case xpTemporary:
		default:
			if (defaultName.IsEmpty())
				defaultName = "temp";
			break;
	}
	
	
	if (type == xpFileToPost)	
		// Temporary post files still go to Temporary, and not post directory
		err = XP_FileSpec(defaultName, xpTemporary, &tempSpec);
	else
		err = XP_FileSpec(defaultName, type, &tempSpec);

	if (err && err != fnfErr)
		return NULL;

	FSSpec finalSpec;
	err = CFileMgr::UniqueFileSpec(tempSpec, tempSpec.name, finalSpec);
	if (err != noErr)
	{
		XP_ASSERT(FALSE);
		return NULL;
	}
	if ((type == xpFileToPost) || (type == xpBookmarks) || (type == xpMailFolder) )	// Post file needs full pathname
	{
		char * tempPath = CFileMgr::PathNameFromFSSpec( finalSpec, TRUE );
		tempPath = CFileMgr::EncodeMacPath( tempPath );
		if (tempPath == NULL)
			return NULL;
		else if (XP_STRLEN(tempPath) > 500)	// Buffer overflow check
		{
			XP_FREE(tempPath);
			return NULL;
		}
			XP_STRCPY(buf, tempPath);
		XP_FREE(tempPath);
	}
	else
		XP_STRCPY(buf, CStr63(finalSpec.name));
	return buf;
}



// XP_OpenDir is not handling different directories yet. 
XP_Dir XP_OpenDir( const char* name, XP_FileType type )
{
	CInfoPBRec		pb;
	OSErr			err;
	DirInfo*		dipb;
	struct dirstruct *		dir = XP_NEW(struct dirstruct);

	if (dir == NULL)
		return NULL;

	XP_TRACE(( "opening file: %s", name ));
	dir->type = type;
	
	err = XP_FileSpec( name, type, &dir->dirSpecs );
	if ( err != noErr )
	{
		XP_DELETE(dir);
		return NULL;
	}

	dipb = (DirInfo*)&pb;

	pb.hFileInfo.ioNamePtr = dir->dirSpecs.name;
	pb.hFileInfo.ioVRefNum = dir->dirSpecs.vRefNum;
	pb.hFileInfo.ioDirID = dir->dirSpecs.parID;
	pb.hFileInfo.ioFDirIndex = 0;	/* use ioNamePtr and ioDirID */

	err = PBGetCatInfoSync( &pb );

	/* test that we have gotten a directory back, not a file */
	if ( (err != noErr ) || !( dipb->ioFlAttrib & 0x0010 ) )
	{
		XP_DELETE( dir );
		return NULL;
	}
	dir->dirSpecs.parID = pb.hFileInfo.ioDirID;
	dir->index = pb.dirInfo.ioDrNmFls;

	return (XP_Dir)dir;
}

void XP_CloseDir( XP_Dir dir )
{
	if ( dir )
		XP_DELETE(dir);
}

int XP_FileNumberOfFilesInDirectory(const char * dir_name, XP_FileType type)
{
	FSSpec spec;
	OSErr err = XP_FileSpec(dir_name, type, &spec);
	if ((err != noErr) && (err != fnfErr))
		goto loser;
	CInfoPBRec pb;
	pb.hFileInfo.ioNamePtr = spec.name;
	pb.hFileInfo.ioVRefNum = spec.vRefNum;
	pb.hFileInfo.ioDirID = spec.parID;
	pb.hFileInfo.ioFDirIndex = 0;	/* use ioNamePtr and ioDirID */
	err = PBGetCatInfoSync(&pb);
	if (err != noErr)
		goto loser;
	return pb.dirInfo.ioDrNmFls;
loser:
	return 10000;	/* ???? */
}

XP_DirEntryStruct * XP_ReadDir(XP_Dir dir)
{
tryagain:
	if (dir->index <= 0)
		return NULL;
	CInfoPBRec cipb;
	DirInfo	*dipb=(DirInfo *)&cipb;
	dipb->ioCompletion = NULL;
	dipb->ioFDirIndex = dir->index;
	dipb->ioVRefNum = dir->dirSpecs.vRefNum; // Might need to use vRefNum, not sure
	dipb->ioDrDirID = dir->dirSpecs.parID;
	dipb->ioNamePtr = (StringPtr)&dir->dirent.d_name;
	OSErr err = PBGetCatInfoSync (&cipb);
	/* We are traversing the directory backwards */
	if (err != noErr)
	{
		if (dir->index > 1)
		{
			dir->index--;
			goto tryagain;
		}
		else
			return NULL;
	}
	p2cstr((StringPtr)&dir->dirent.d_name);

	/* Mail folders are treated in a special way */

	if ((dir->type == xpMailFolder) ||
		(dir->type == xpMailFolderSummary))
	{
		char * newName = XP_STRDUP(dir->dirent.d_name);
		newName = CFileMgr::EncodeMacPath(newName);
		if (newName)
		{
			XP_STRCPY(dir->dirent.d_name, newName);
			XP_FREE(newName);
		}
	}

	dir->index--;
	return &dir->dirent;
}

int XP_MakeDirectory(const char* name, XP_FileType type)
{
	FSSpec spec;
	OSErr err = XP_FileSpec(name, type, &spec);
	if ((err != noErr) && (err != fnfErr))	// Should not happen
		return -1;
	short refNum;
	long dirID;
	err = CFileMgr::CreateFolderInFolder(
				spec.vRefNum, spec.parID, spec.name,	// Name of the new folder
				&refNum, &dirID);
	if (err == noErr)
		return 0;
	else
		return -1;
}

int XP_FileRename( const char* from, XP_FileType fromtype,
				   const char* to, XP_FileType totype )
{
	FSSpec		spec;
	OSErr		err;
	CStr255		cto;
	
	char* fromName = WH_FileName( from, fromtype );
	char* toName = WH_FileName( to, totype );
	
	if ( fromName && toName )
	{
		err = CFileMgr::FSSpecFromPathname( toName, &spec );
		if ( err == noErr )
			::FSpDelete( &spec );
		cto = (CStr255)spec.name;
			
		err = CFileMgr::FSSpecFromPathname( fromName, &spec );
		if ( err == noErr )
			err = ::FSpRename( &spec, cto );
	}
	else
		err = -1;
	if ( fromName )
		XP_FREE( fromName );
	if ( toName )
		XP_FREE( toName );

	if ( err == noErr )
		return 0;
	return -1;
}

int XP_FileRemove( const char* name, XP_FileType type )
{
	FSSpec		spec;
	OSErr		err;

	err = XP_FileSpec( name, type, &spec);

	if ( err == noErr )
	{
		err = ::FSpDelete( &spec );
		if ( type == xpNewsRC || type == xpNewsgroups )
	        NET_UnregisterNewsHost( name, FALSE );
		else if ( type == xpSNewsRC || type == xpSNewsgroups)
	        NET_UnregisterNewsHost( name, TRUE );
	}
	
	if ( err == noErr )
		return 0;
	return -1;
}

int XP_FileTruncate( const char* name, XP_FileType type, int32 len )
{
	FSSpec		spec;
	OSErr		err, err2;
	short refNum;

	if (len < 0)
		return EINVAL;

	err = XP_FileSpec( name, type, &spec);
	if (err != noErr)
		return EINVAL;

	err = FSpOpenDF(&spec, fsRdWrPerm, &refNum);
	if (err != noErr)
		return EACCES;
	
	err = SetEOF(refNum, len);
	
	err2 = FSClose(refNum);
	if ((err != noErr) || (err2 != noErr))
		return EIO;
	return 0;
}

XP_END_PROTOS

#ifdef PROFILE
#pragma profile off
#endif

#endif	/* XP_MAC */

XP_BEGIN_PROTOS


#if defined (XP_MAC) || defined(XP_UNIX)
/* Now that Mac is using stdio, we can share some of this.  Yay.
   Presumably Windows can share this too, but they keep these
   functions in the FE, so I don't know.
 */


int XP_Stat( const char* name, XP_StatStruct * outStat,  XP_FileType type )
{
	int		result = 0;
	
	char* newName = WH_FileName( name, type );
	if (!newName) return -1;
	result = stat( newName, outStat );
	XP_FREE(newName);
	return result;
}

XP_File XP_FileOpen( const char* name, XP_FileType type,
					 const XP_FilePerm permissions )
{
  char* newName = WH_FileName(name, type);
  XP_File result;
#ifdef XP_UNIX
  XP_StatStruct st;
  XP_Bool make_private_p = FALSE;
#endif

  /* One should never open newsrc for output directly. */
  XP_ASSERT (type != xpNewsRC || type != xpSNewsRC ||
			 !strchr (permissions, 'w'));

  if (newName == NULL)
	return NULL;

#ifdef XP_UNIX
  switch (type)
	{
	  /* These files are always private: if the user chmods them, we
		 make them -rw------ again the next time we use them, because
		 it's really important.
	   */
	case xpHTTPCookie:
	case xpKeyChain:
	case xpCertDB:
	case xpCertDBNameIDX:
	case xpKeyDB:
	case xpSecModuleDB:
	  /* Always make tmp files private, because of their short lifetime. */
	case xpTemporary:
	case xpTemporaryNewsRC:
	case xpFileToPost:
	case xpPKCS12File:
	  if (strchr (permissions, 'w'))     /* opening for output */
		make_private_p = TRUE;
	  break;

	  /* These files are created private, but if the user then goes and
		 chmods them, we let their changes stick, because it's reasonable
		 for a user to decide to give away read permission on these
		 (though by default, it makes sense for them to be more private
		 than the user's other files.)
	   */
	case xpCacheFAT:
	case xpCache:
	case xpGlobalHistory:
	case xpHotlist:
	case xpBookmarks:
	case xpMailFolder:
	  if (strchr (permissions, 'w') &&   /* opening for output */
		  XP_Stat (newName, &st, type))  /* and it doesn't exist yet */
		make_private_p = TRUE;
	  break;


	case xpProxyConfig:
	case xpSocksConfig:
	case xpNewsRC:
	case xpSNewsRC:
	case xpNewsgroups:
	case xpSNewsgroups:
	case xpURL:
	case xpMimeTypes:
	case xpSignature:
	case xpMailFolderSummary:
	case xpMailSort:
	case xpMailPopState:
	case xpExtCache:
	case xpExtCacheIndex:
	case xpXoverCache:
	  /* These files need not be more private than any other, so we
		 always create them with the default umask. */
	  break;
	default:
	  XP_ASSERT(0);
	}
#endif /* XP_UNIX */

#ifndef MCC_PROXY
  /* At this point, we'd better have an absolute path, because passing a
	 relative path to fopen() is undefined.  (For the client, at least -
	 I gather the proxy is different?) */
#endif/* !MCC_PROXY */

  result = fopen(newName, permissions);
  
#ifdef XP_MAC
 	if (result != 0)
 	{
 		int err;
 		
 		err = setvbuf(	result, 		/* file to buffer */
 						NULL,			/* allocate the buffer for us */
 						_IOFBF, 		/* fully buffer */
 						8 * 1024);		/* 8k buffer */
 						
		XP_ASSERT(err == 0); 						
 	}
#endif  

#ifdef XP_UNIX
  if (make_private_p && result)
#ifdef SCO_SV
	chmod (newName, S_IRUSR | S_IWUSR); /* rw only by owner */
#else
	fchmod (fileno (result), S_IRUSR | S_IWUSR); /* rw only by owner */
#endif
#endif /* XP_UNIX */

  XP_FREE(newName);
  return result; 
}

#endif /* XP_MAC || XP_UNIX */

#ifdef XP_UNIX

/* This is just like fclose(), except that it does fflush() and fsync() first,
   to ensure that the bits have made it to the disk before we return.
 */
int XP_FileClose(XP_File file)
{
  int filedes;

  /* This busniess with `status' and `err' is an attempt to preserve the
	 very first non-0 errno we get, while still proceeding to close the file
	 if the fflush/fsync failed for some (weird?) reason. */
  int status, err = 0;
  XP_ASSERT(file);
  if (!file) return -1;
  status = fflush(file);              /* Push the stdio buffer to the disk. */
  if (status != 0) err = errno;

  filedes = XP_Fileno (file);
  /*
   * Only fsync when the file is not read-only, otherwise
   * HPUX and IRIX return an error, and Linux syncs the disk anyway.
   */
  if (fcntl(filedes, F_GETFL) != O_RDONLY) {
    status = fsync(filedes);   /* Wait for disk buffers to go out. */
    if (status != 0 && err == 0) err = errno;
  }

  status = fclose(file);              /* Now close it. */
  if (status != 0 && err == 0) err = errno;

  errno = err;
  return status ? status : err ? -1 : 0;
}

#endif /* XP_UNIX */


XP_END_PROTOS

#ifdef XP_UNIX

#include <unistd.h>		/* for getpid */

#ifdef TRACE
#define Debug 1
#define DO_TRACE(x) if (xp_file_debug) XP_TRACE(x)


int xp_file_debug = 0;
#else
#define DO_TRACE(x)
#endif

int XP_FileRemove(const char * name, XP_FileType type)
{
	char * newName = WH_FileName(name, type);
	int result;
	if (!newName) return -1;
	result = remove(newName);
	XP_FREE(newName);
	if (result != 0)
	{
	  /* #### Uh, what is this doing?  Of course errno is set! */
		int err = errno;
		if (err != 0)
			return -1;
	}
	
	return result;
}


int XP_FileRename(const char * from, XP_FileType fromtype,
				  const char * to, XP_FileType totype)
{
	char * fromName = WH_FileName(from, fromtype);
	char * toName = WH_FileName(to, totype);
	int res = 0;
	if (fromName && toName)
		res = rename(fromName, toName);
	else
		res = -1;
	if (fromName)
		XP_FREE(fromName);
	if (toName)
		XP_FREE(toName);
	return res;
}


/* Create a new directory */

int XP_MakeDirectory(const char* name, XP_FileType type)
{
  int result;
  static XP_Bool madeXoverDir = FALSE;
  mode_t mode;
  switch (type) {
  case xpMailFolder:
	mode = 0700;
	break;
  default:
	mode = 0755;
	break;
  }
  if (type == xpXoverCache) {
	/* Once per session, we'll check that the xovercache directory is
	   created before trying to make any subdirectories of it.  Sigh. ###*/
	if (!madeXoverDir) {
	  madeXoverDir = TRUE;
	  XP_MakeDirectory("", type);
	}
  }
#ifdef __linux
  {
	/* Linux is a steaming pile.  It chokes if the parent of the
	   named directory is a symbolic link.  Dumbdumbdumb. */
	char rp[MAXPATHLEN];
	char *s, *s0 = WH_FileName (name, type);
	if (!s0) return -1;

	/* realpath is advertised to return a char* (rp) on success, and on
	   failure, return 0, set errno, and leave a partial path in rp.
	   But on Linux 1.2.3, it doesn't -- it returns 0, leaves the result
	   in rp, and doesn't set errno.  So, if errno is unset, assume all
	   is well.
	 */
	rp[0] = 0;
	errno = 0;
	s = realpath (s0, rp);
	XP_FREE(s0);
	/* WTF??? if (errno) return -1; */
	if (!s) s = rp;
	if (!*s) return -1;
	result = mkdir (s, mode);
  }
#elif defined(SUNOS4)
  {
	char rp[MAXPATHLEN];
	char *s = WH_FileName (name, type);
	char *tmp;
	if (!s) return -1;

	/* Take off all trailing slashes, because some systems (SunOS 4.1.2)
	   don't think mkdir("/tmp/x/") means mkdir("/tmp/x"), sigh... */
	PR_snprintf (rp, MAXPATHLEN-1, "%s", s);
	XP_FREE(s);
	while ((tmp = XP_STRRCHR(rp, '/')) && tmp[1] == '\0')
	  *tmp = '\0';

	result = mkdir (rp, mode);
  }
#else /* !__linux && !SUNOS4 */
  DO_TRACE(("XP_MakeDirectory called: creating: %s", name));
  {
	  char* filename = WH_FileName(name, type);
	  if (!filename) return -1;
	  result = mkdir(filename, mode);
	  XP_FREE(filename);
  }
#endif
  return result;
}


int XP_FileTruncate(const char* name, XP_FileType type, int32 length)
{
  char* filename = WH_FileName(name, type);
  int result = 0;
  if (!result) return -1;
  result = truncate(filename, length);
  XP_FREE(filename);
  return result;
}



/* Writes to a file
 */
int XP_FileWrite (const void* source, int32 count, XP_File file)
{
    int32		realCount;

    if ( count < 0 )
        realCount = XP_STRLEN( source );
    else
        realCount = count;

	return( fwrite( source, 1, realCount, file ) );
}

/* The user can set these on the preferences dialogs; the front end
   informs us of them by setting these variables.  We do "something
   sensible" if 0...
   */
PUBLIC char *FE_UserNewsHost = 0;	/* clone of NET_NewsHost, mostly... */
PUBLIC char *FE_UserNewsRC = 0;
PUBLIC char *FE_TempDir = 0;
PUBLIC char *FE_CacheDir = 0;
PUBLIC char *FE_GlobalHist = 0;


static const char *
xp_unix_config_directory(char* buf)
{
  static XP_Bool initted = FALSE;
  const char *dir = ".netscape";
  char *home;
  if (initted)
	return buf;

  home = getenv ("HOME");
  if (!home)
	home = "";

#ifdef OLD_UNIX_FILES

  sprintf (buf, "%.900s", home);
  if (buf[strlen(buf)-1] == '/')
	buf[strlen(buf)-1] = 0;

#else  /* !OLD_UNIX_FILES */

  if (*home && home[strlen(home)-1] == '/')
	sprintf (buf, "%.900s%s", home, dir);
  else
	sprintf (buf, "%.900s/%s", home, dir);

#endif /* !OLD_UNIX_FILES */

  return buf;
}


static int
xp_unix_sprintf_stat( char * buf,
                      const char * dirName,
                      const char * lang,
                      const char * fileName)
{
    int               result;
    int               len;
    XP_StatStruct     outStat;

    if (dirName == NULL || 0 == (len = strlen(dirName)) || len > 900)
	return -1;

    while (--len > 0 && dirName[len] == '/')
	/* do nothing */;		/* strip trailing slashes */
    strncpy(buf, dirName, ++len);
    buf[len++] = '/';
    buf[len]   =  0;
    if (lang != NULL && *lang == 0)
	lang  = NULL;
    if (lang != NULL) 
	sprintf(buf+len, "%.100s/%s", lang, fileName);
    else
	strcpy(buf+len, fileName);
    result = stat( buf, &outStat );
    if (result >= 0 && !S_ISREG(outStat.st_mode))
	result = -1;
    if (result < 0 && lang != NULL)
	result = xp_unix_sprintf_stat(buf, dirName, NULL, fileName);
    return result;
}


/* returns a unmalloced static
 * string that is only available for
 * temporary use.
 */
PUBLIC char *
xp_FileName (const char *name, XP_FileType type, char* buf, char* configBuf)
{
  const char *conf_dir = xp_unix_config_directory(configBuf);

  switch (type)
	{
	case xpCacheFAT:
	  {
	  	const char *cache_dir = FE_CacheDir;
		if (!cache_dir || !*cache_dir)
		  cache_dir = conf_dir;
		if (cache_dir [strlen (cache_dir) - 1] == '/')
		  sprintf (buf, "%.900sindex.db", cache_dir);
		else
		  sprintf (buf, "%.900s/index.db", cache_dir);

		name = buf;
		break;
	  }
	case xpCache:
	  {
		/* WH_TempName() returns relative pathnames for the cache files,
		   so that relative paths get written into the cacheFAT database.
		   WH_FileName() converts them to absolute if they aren't already
		   so that the logic of XP_FileOpen() and XP_FileRename() and who
		   knows what else is simpler.
		 */
		if (*name != '/')
		  {
			char *tmp = FE_CacheDir;
			if (!tmp || !*tmp) tmp = "/tmp";
			if (tmp [strlen (tmp) - 1] == '/')
			  sprintf (buf, "%.500s%.500s", tmp, name);
			else
			  sprintf (buf, "%.500s/%.500s", tmp, name);
			name = buf;
		  }
		break;
	  }

	case xpHTTPCookie:
	  {
#ifndef OLD_UNIX_FILES
		sprintf (buf, "%.900s/cookies", conf_dir);
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.900s/.netscape-cookies", conf_dir);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;
	  }
	case xpProxyConfig:
	  {
		sprintf(buf, "%.900s/proxyconf", conf_dir);
		name = buf;
		break;
	  }
	case xpTemporary:
	  {
		if (*name != '/')
		  {
			char *tmp = FE_TempDir;
			if (!tmp || !*tmp) tmp = "/tmp";
			if (tmp [strlen (tmp) - 1] == '/')
			  sprintf (buf, "%.500s%.500s", tmp, name);
			else
			  sprintf (buf, "%.500s/%.500s", tmp, name);
			name = buf;
		  }
		break;
	  }
	case xpNewsRC:
	case xpSNewsRC:
	case xpTemporaryNewsRC:
	  {
		/* In this case, `name' is "" or "host" or "host:port". */

		char *home = getenv ("HOME");
		const char *newsrc_dir = ((FE_UserNewsRC && *FE_UserNewsRC)
								  ? FE_UserNewsRC
								  : (home ? home : ""));
		const char *basename = (type == xpSNewsRC ? ".snewsrc" : ".newsrc");
		const char *suffix = (type == xpTemporaryNewsRC ? ".tmp" : "");
		if (*name)
		  sprintf (buf, "%.800s%.1s%.8s-%.128s%.4s",
				   newsrc_dir,
				   (newsrc_dir[XP_STRLEN(newsrc_dir)-1] == '/' ? "" : "/"),
				   basename, name, suffix);
		else
		  sprintf (buf, "%.800s%.1s%.128s%.4s",
				   newsrc_dir,
				   (newsrc_dir[XP_STRLEN(newsrc_dir)-1] == '/' ? "" : "/"),
				   basename, suffix);

		name = buf;
		break;
	  }
	case xpNewsgroups:
	case xpSNewsgroups:
	  {
#ifndef OLD_UNIX_FILES
		sprintf (buf, "%.800s/%snewsgroups-%.128s", 
				 conf_dir, 
				 type == xpSNewsgroups ? "s" : "", 
				 name);
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.800s/.netscape-%snewsgroups-%.128s", 
				 conf_dir, 
				 type == xpSNewsgroups ? "s" : "", 
				 name);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;
	  }

	case xpExtCacheIndex:
#ifndef OLD_UNIX_FILES
		sprintf (buf, "%.900s/cachelist", conf_dir);
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.900s/.netscape-cache-list", conf_dir);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;

	case xpGlobalHistory:
		name = FE_GlobalHist;
		break;

	case xpCertDB:
#ifndef OLD_UNIX_FILES
		if ( name ) {
			sprintf (buf, "%.900s/cert%s.db", conf_dir, name);
		} else {
			sprintf (buf, "%.900s/cert.db", conf_dir);
		}
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.900s/.netscape-certdb", conf_dir);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;
	case xpCertDBNameIDX:
#ifndef OLD_UNIX_FILES
		sprintf (buf, "%.900s/cert-nameidx.db", conf_dir);
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.900s/.netscape-certdb-nameidx", conf_dir);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;
	case xpKeyDB:
#ifndef OLD_UNIX_FILES
		if ( name ) {
			sprintf (buf, "%.900s/key%s.db", conf_dir, name);
		} else {
			sprintf (buf, "%.900s/key.db", conf_dir);
		}
#else  /* OLD_UNIX_FILES */
		sprintf (buf, "%.900s/.netscape-keydb", conf_dir);
#endif /* OLD_UNIX_FILES */
		name = buf;
		break;
	case xpSecModuleDB:
		sprintf (buf, "%.900s/secmodule.db", conf_dir);
		name = buf;
		break;

	case xpFileToPost:
	case xpSignature:
	  	/* These are always absolute pathnames. 
		 * BUT, the user can type in whatever so
		 * we can't assert if it doesn't begin
		 * with a slash
		 */
		break;

	case xpExtCache:
	case xpKeyChain:
	case xpURL:
	case xpHotlist:
	case xpBookmarks:
	case xpMimeTypes:
	case xpSocksConfig:
	case xpMailFolder:
#ifdef BSDI
	    /* In bsdi, mkdir fails if the directory name is terminated
	     * with a '/'. - dp
	     */
	    if (name[strlen(name)-1] == '/') {
		strcpy(buf, name);
		buf[strlen(buf)-1] = '\0';
		name = buf;
	    }
#endif
#ifndef MCC_PROXY
	  /*
	   * These are always absolute pathnames for the Navigator.
	   * Only the proxy (servers) may have pathnames relative
	   * to their current working directory (the servers chdir
	   * to their admin/config directory on startup.
	   *
	   */
	  if (name) XP_ASSERT (name[0] == '/');
#endif	/* ! MCC_PROXY */

	  break;

	case xpMailFolderSummary:
	  /* Convert /a/b/c/foo to /a/b/c/.foo.summary (note leading dot) */
	  {
		const char *slash;
		slash = strrchr (name, '/');
		if (name) XP_ASSERT (name[0] == '/');
		XP_ASSERT (slash);
		if (!slash) return 0;
		XP_MEMCPY (buf, name, slash - name + 1);
		buf [slash - name + 1] = '.';
		XP_STRCPY (buf + (slash - name) + 2, slash + 1);
		XP_STRCAT (buf, ".summary");
		name = buf;
		break;
	  }

	case xpMailSort:
#ifndef OLD_UNIX_FILES
	  sprintf(buf, "%.900s/mailsort", conf_dir);
#else  /* OLD_UNIX_FILES */
	  sprintf(buf, "%.900s/.netscape-mailsort", conf_dir);
#endif /* OLD_UNIX_FILES */
	  name = buf;
	  break;
	case xpMailPopState:
#ifndef OLD_UNIX_FILES
	  sprintf(buf, "%.900s/popstate", conf_dir);
#else  /* OLD_UNIX_FILES */
	  sprintf(buf, "%.900s/.netscape-popstate", conf_dir);
#endif /* OLD_UNIX_FILES */
	  name = buf;
	  break;
	case xpXoverCache:
	  sprintf(buf, "%.800s/xover-cache/%.128s", conf_dir, name);
	  name = buf;
	  break;

	case xpCryptoPolicy:
     {
         extern void fe_GetProgramDirectory(char *path, int len);
         char *policyFN = "policyMoz40P2";
         char *mozHome  = getenv("MOZILLA_HOME");
         char *lang     = getenv("LANG");
         int   result;
         char  dirName[1024];

         name = buf;
         if (!xp_unix_sprintf_stat(buf, conf_dir, lang, policyFN))
             break;
         if (!xp_unix_sprintf_stat(buf, mozHome, lang, policyFN))
             break;
         fe_GetProgramDirectory(dirName, sizeof dirName);
         if (!xp_unix_sprintf_stat(buf, dirName, lang, policyFN))
             break;

         /* couldn't find it, but must leave a valid file name in buf */
         sprintf(buf, "%.900s/%s", conf_dir, policyFN);
         break;
     }

	case xpPKCS12File:
	  /* Convert /a/b/c/foo to /a/b/c/foo.p12 (note leading dot) */
	  {
		  char *dot = NULL;
		  int len = 0;

		  if (name) 
			  XP_ASSERT (name[0] == '/');

		  dot = XP_STRRCHR(name, '.');
		  if (dot) {
			  len = dot - name + 1;
			  XP_MEMCPY(buf, name, len);
			  buf[len] = 0;
		  }/* if */

		  XP_STRCAT (buf, ".p12");
		  name = buf;
		break;
	  }


	default:
	  abort ();
	}

  return (char *) name;
}

char * xp_FilePlatformName(const char * name, char* path)
{
	if ((name == NULL) || (XP_STRLEN(name) > 1000))
		return NULL;
	XP_STRCPY(path, name);
	return path;
}

char * XP_PlatformFileToURL (const char *name)
{
	char *prefix = "file://";
	char *retVal = XP_ALLOC (XP_STRLEN(name) + XP_STRLEN(prefix) + 1);
	if (retVal)
	{
		XP_STRCPY (retVal, "file://");
		XP_STRCAT (retVal, name);
	}
	return retVal;
}

char *XP_PlatformPartialPathToXPPartialPath(const char *platformPath)
{
	/* using the unix XP_PlatformFileToURL, there is no escaping! */
	return XP_STRDUP(platformPath);
}


#define CACHE_SUBDIRS

char*
XP_TempDirName(void)
{
	char *tmp = FE_TempDir;
	if (!tmp || !*tmp) tmp = "/tmp";
	return XP_STRDUP(tmp);
}

char *
xp_TempName (XP_FileType type, const char * prefix, char* buf, char* buf2, unsigned int *count)
{
#define NS_BUFFER_SIZE	1024
  char *value = buf;
  time_t now;

  *buf = 0;
  XP_ASSERT (type != xpTemporaryNewsRC);

  if (type == xpCache)
	{
	  /* WH_TempName() must return relative pathnames for the cache files,
		 so that relative paths get written into the cacheFAT database,
		 making the directory relocatable.
	   */
	  *buf = 0;
	}
  else
	{
	  char *tmp = FE_TempDir;
	  if (!tmp || !*tmp) tmp = "/tmp";
	  XP_SPRINTF (buf, "%.500s", tmp);

	  if (!prefix || !*prefix)
		prefix = "tmp";
	}

  XP_ASSERT (!XP_STRCHR (prefix, '/'));
  if (*buf && buf[XP_STRLEN (buf)-1] != '/')
	XP_STRCAT (buf, "/");

  /* It's good to have the cache file names be pretty long, with a bunch of
	 inputs; this makes the variant part be 15 chars long, consisting of the
	 current time (in seconds) followed by a counter (to differentiate
	 documents opened in the same second) followed by the current pid (to
	 differentiate simultanious processes.)  This organization of the bits
	 has the effect that they are ordered the same lexicographically as by
	 creation time.

	 If name length was an issue we could cut the character count a lot by
	 printing them in base 72 [A-Za-z0-9@%-_=+.,~:].
   */
  now = time ((time_t *) 0);
  sprintf (buf2,
		   "%08X%03X%04X",
		   (unsigned int) now,
		   (unsigned int) *count,
		   (unsigned int) (getpid () & 0xFFFF));

  if (++(*count) > 4095) (*count) = 0; /* keep it 3 hex digits */

#ifdef CACHE_SUBDIRS
  if (type == xpCache)
	{
	  XP_StatStruct st;
	  char *s;
	  char *tmp = FE_CacheDir;
	  if (!tmp || !*tmp) tmp = "/tmp";
	  sprintf (buf, "%.500s", tmp);
	  if (buf [XP_STRLEN(buf)-1] != '/')
		XP_STRCAT (buf, "/");

	  s = buf + XP_STRLEN (buf);

	  value = s;		/* return a relative path! */

	  /* The name of the subdirectory is the bottom 5 bits of the time part,
		 in hex (giving a total of 32 directories.) */
	  sprintf (s, "%02X", (now & 0x1F));

	  if (XP_Stat (buf, &st, xpURL))		/* create the dir if necessary */
		XP_MakeDirectory (buf, xpCache);

	  s[2] = '/';
	  s[3] = 0;
	}
#endif /* !CACHE_SUBDIRS */

  XP_STRNCAT (value, prefix, NS_BUFFER_SIZE - XP_STRLEN(value));
  XP_STRNCAT (value, buf2, NS_BUFFER_SIZE - XP_STRLEN(value));
  value[NS_BUFFER_SIZE - 1] = '\0'; /* just in case */

  DO_TRACE(("WH_TempName called: returning: %s", value));

  return(value);
}


/* XP_GetNewsRCFiles returns a null terminated array
 * of pointers to malloc'd filename's.  Each filename
 * represents a different newsrc file.
 *
 * return only the filename since the path is not needed
 * or wanted.
 *
 * Netlib is expecting a string of the form:
 *  [s]newsrc-host.name.domain[:port]
 *
 * examples:
 *    newsrc-news.mcom.com
 *    snewsrc-flop.mcom.com
 *    newsrc-news.mcom.com:118
 *    snewsrc-flop.mcom.com:1191
 *
 * the port number is optional and should only be
 * there when different from the default.
 * the "s" in front of newsrc signifies that
 * security is to be used.
 *
 * return NULL on critical error or no files
 */

#define NEWSRC_PREFIX ".newsrc"
#define SNEWSRC_PREFIX ".snewsrc"

#include "nslocks.h"
#include "prnetdb.h"

PUBLIC char **
XP_GetNewsRCFiles(void)
{
	XP_Dir dir_ptr;
	XP_DirEntryStruct *dir_entry;
	char **array;
	int count=0;
	char * dirname;
	char * name;
	int len, len2;
	
	/* assume that there will never be more than 256 hosts
	 */
#define MAX_NEWSRCS 256
	array = (char **) XP_ALLOC(sizeof(char *) * (MAX_NEWSRCS+1));

	if(!array)
		return(NULL);

	XP_MEMSET(array, 0, sizeof(char *) * (MAX_NEWSRCS+1));

    name = WH_FileName ("", xpNewsRC);

	/* truncate the last slash */
	dirname = XP_STRRCHR(name, '/');
	if(dirname)
		*dirname = '\0';
	dirname = name;

	if(!dirname)
		dirname = "";

	dir_ptr = XP_OpenDir(dirname, XP_NewsRC);
	XP_FREE(name);
    if(dir_ptr == NULL)
      {
		XP_FREE(array);
		return(NULL);
      }

	len = XP_STRLEN(NEWSRC_PREFIX);
	len2 = XP_STRLEN(SNEWSRC_PREFIX);

	while((dir_entry = XP_ReadDir(dir_ptr)) != NULL
			&& count < MAX_NEWSRCS)
	  {
		char *name = dir_entry->d_name;
		char *suffix, *port;
		if (!XP_STRNCMP (name, NEWSRC_PREFIX, len))
		  suffix = name + len;
		else if (!XP_STRNCMP (name, SNEWSRC_PREFIX, len2))
		  suffix = name + len2;
		else
		  continue;

		if (suffix [strlen (suffix) - 1] == '~' ||
			suffix [strlen (suffix) - 1] == '#')
		  continue;

		if (*suffix != 0 && *suffix != '-')
		  /* accept ".newsrc" and ".newsrc-foo" but ignore
			 .newsrcSomethingElse" */
		  continue;

		if (*suffix &&
			(suffix[1] == 0 || suffix[1] == ':'))  /* reject ".newsrc-" */
		  continue;

		port = XP_STRCHR (suffix, ':');
		if (!port)
		  port = suffix + strlen (suffix);

		if (*port)
		  {
			/* If there is a "port" part, reject this file if it contains
			   non-numeric characters (meaning reject ".newsrc-host:99.tmp")
			 */
			char *s;
			for (s = port+1; *s; s++)
			  if (*s < '0' || *s > '9')
				break;
			if (*s)
			  continue;
		  }

		if (suffix != port)
		  {
			/* If there's a host name in the file, check that it is a
			   resolvable host name.

			   If this turns out to be a bad idea because of socks or
			   some other reason, then we should at least check that
			   it looks vaguely like a host name - no non-hostname
			   characters, and so on.
			 */
			char host [255];
			PRHostEnt *ok, hpbuf;
			char dbbuf[PR_NETDB_BUF_SIZE];
			if (port - suffix >= sizeof (host))
			  continue;
			strncpy (host, suffix + 1, port - suffix - 1);
			host [port - suffix - 1] = 0;
			ok = PR_gethostbyname (host, &hpbuf, dbbuf, sizeof(dbbuf), 0);
			if (!ok)
			  continue;
		  }

		StrAllocCopy(array[count++], dir_entry->d_name+1);
	  }

	XP_CloseDir (dir_ptr);
	return(array);
}

#endif /* XP_UNIX */

/*
	If we go to some sort of system-wide canonical format, routines like this make sense.
	This is intended to work on our (mail's) canonical internal file name format.
	Currently, this looks like Unix format, e.g., "/directory/subdir".
	It is NOT intended to be used on native file name formats.
	The only current use is in spool.c, to determine if a rule folder
	is an absolute or relative path (relative to mail folder directory),
	so if this gets yanked, please fix spool.c  DMB 2/13/96
*/
XP_Bool XP_FileIsFullPath(const char * name)
{
	return (*name == '/');
}

/******************************************************************************/
/* Thread-safe entry points: */

#ifndef XP_WIN

extern PRMonitor* _pr_TempName_lock;

PUBLIC char *
WH_FileName (const char *name, XP_FileType type)
{
	static char buf [1024];				/* protected by _pr_TempName_lock */
	static char configBuf [1024];		/* protected by _pr_TempName_lock */
	char* result;
#if 1 /* kill 'em all */
#ifdef NSPR
	XP_ASSERT(_pr_TempName_lock);
	PR_EnterMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	result = xp_FileName(name, type, buf, configBuf);
	if (result)
		result = XP_STRDUP(result);
#if 1 /* kill 'em all */
#ifdef NSPR
	PR_ExitMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	return result;
}

char * 
WH_FilePlatformName(const char * name)
{
	char* result;
	static char path[300];	/* Names longer than 300 are not dealt with in our stdio */
#if 1 /* kill 'em all */
#ifdef NSPR
	XP_ASSERT(_pr_TempName_lock);
	PR_EnterMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	result = xp_FilePlatformName(name, path);
#if 1 /* kill 'em all */
#ifdef NSPR
	PR_ExitMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	return result;
}

char * 
WH_TempName(XP_FileType type, const char * prefix)
{
#define NS_BUFFER_SIZE	1024
	static char buf [NS_BUFFER_SIZE];	/* protected by _pr_TempName_lock */
	static char buf2 [100];
	static unsigned int count = 0;
	char* result;
#if 1 /* kill 'em all */
#ifdef NSPR
	XP_ASSERT(_pr_TempName_lock);
	PR_EnterMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	result = xp_TempName(type, prefix, buf, buf2, &count);
	if (result)
		result = XP_STRDUP(result);
#if 1 /* kill 'em all */
#ifdef NSPR
	PR_ExitMonitor(_pr_TempName_lock);
#endif
#endif /* 0 -- kill 'em all */
	return result;
}

#endif /* !XP_WIN */

/******************************************************************************/

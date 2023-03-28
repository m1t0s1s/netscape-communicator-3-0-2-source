/*
 * Long filename translation for 16 bit windows. The main function in this
 * module is LFNConvertFromLongFilename. All other LFN* in this module exist
 * to support it.
 *
 * All of the information used to derive these procedures were taken from the
 * MSDN January 1996 edition.
 */
#if defined(XP_PC) && !defined(_WIN32) && defined(DEBUG)

/*
** Copy the volume name from the second parameter into the first
** Return 1 if there was a volume name, 0 otherwise.
*/
static int
LFNParseVolume(char *volumeOut,char *volumeIn)
{
	char c;
	
	c = *volumeIn;
	
	if ( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'Z') ) {
		if (volumeIn[1] == ':') {
			*volumeOut++ = volumeIn[1];
			*volumeOut++ = ':';
			*volumeOut++ = '\\';
			*volumeOut++ = '\0';
        	
        	return 1;
		}
	}
	
	return 0;
}

/*
** Copy the volume name from the second parameter into the first,
** using the local directory if necessary. Return 1 if the result
** contains a volume name and zero otherwise.
*/
static int
LFNGetVolume(char *volumeOut,char *volumeIn)
{
	short	drive;
	extern  void _dos_getdrive( unsigned *drive );
		
	if (LFNParseVolume(volumeOut, volumeIn))
		return 1;
	
    _dos_getdrive( &drive );
    
	*volumeOut++ = (char) (drive + 'A' - 1);
	*volumeOut++ = ':';
	*volumeOut++ = '\\';
	*volumeOut++ = '\0';
	
	return 1;
}

/* See if the provided volume supports the long filename API.
 * Return 1 if so, false otherwise. The volume must be a
 * null-terminated string containing the volume only (e.g.
 * "C:\".
 */
static int
LFNSupportedByVolume(char *volume)
{
	char fileSystemName[256];
	unsigned short flags;
	unsigned short maxFilenameLength;
	unsigned short maxPathLength;
			 short error;
			 
	fileSystemName[0] = '\0';
	flags = 0;
	maxFilenameLength = 0;
	maxPathLength = 0;
	
	__asm {

		/*
		** Set up for the int21h Get Volume Information call (71a0h).
		** ax    <- 71a0h 	the Get Volume Information Function Code
		** es:di <- address of the buffer to receive the full filesystem name
		** cx    <- length of above buffer
		** ds:dx <- address of the string containing the volume name
		**/
        
        push    ds
		/* volume name */
		mov		dx, WORD PTR [volume + 2]
		mov		ds, dx
		mov		dx, WORD PTR [volume]
		
		/* file system in/out parameter + length */
		mov		di, ss
		mov		si, di
		lea		di, fileSystemName		
		mov		cx, 256
		
		/* system call */
		mov		ax, 71a0h
		int		21h
		pop		ds
		jc		failed			/* Error if carry set */
		
		/* Success, save the results */
		mov		flags, bx
		mov		maxFilenameLength, cx
		mov		maxPathLength, dx
		mov		error, 0
	    jmp		done
	    
	failed:
		mov		error, 1
		
	done:		 
	}
	
	return error ? 0 : ((flags & 0x4000) != 0);
}

/*
 * Translate a filename into the short format if possible, and return
 * the translated, hopefully shortened to 8.3 format. In order for translation
 * to work, the following conditions must be met:
 *
 * 1) The volume (drive) on which the file is located must support the long
 *    filname format. If the specified path contains a volume then that will
 *    be used as the volume of reference. Otherwise the current drive will
 *    be used as returned by the _dos_getdrive function.
 *
 * 2) The file must exist.
 *
 * If translation fails for any reason, the original filename will be returned
 * as the translation.
 *
 * NB: The translation must be large enough to hold the final path name which
 * will usually be shorter than the input name, with the following exceptions:
 *
 *	1) mixed case names may return longer paths
 *	2) multiple occurances of '.' or ' ' may return longer paths
 *
 * For safety, use a buffer of 260.
 */
int
LFNConvertFromLongFilename(char *translation, char *filename)
{
    short 	error;
    extern  char *strcpy(char *, char *);
    
	/* Get the volume info from the filename or the system */	
	if (!LFNGetVolume(translation, filename))	
		return 0;
	/* See if the volume supports the long filename interface */
	if (!LFNSupportedByVolume(translation))
		return 0;
	
		
	/*
	** We have a volume supporting the long filename interface.
	** Ask for the short form of the filename.
	*/
	__asm {
	
		/*
		** GetPathName INT21h 7160h
		*/
		push	ds
		mov		cl, 1				/* Get *SHORT* path name */
		mov		ch, 80h				/* Keep SUBST alias in result (e.g. don't expand) */
		
		/* Source pathname */
		mov		si, WORD PTR [filename+2]
		mov		ds, si
		mov		si, WORD PTR [filename]
		
		/* Destination pathname */
		mov		di, WORD PTR [translation+2]
		mov		es, di
		mov		di, WORD PTR [translation]
		
		mov		ax, 7160h
		int		21h
		pop		ds
		
		jc		failed
		mov	    error, 0
		jmp		done

failed:
		mov		error, 1

done:
	}

	if (error != 0)
		strcpy(translation, filename);

	return error == 0;
}

#endif

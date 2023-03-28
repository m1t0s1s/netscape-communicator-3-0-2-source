/* -*- Mode: C; tab-width: 4 -*-
 *
 *	apple_double_decode.c
 *
 *	---------------------
 *
 *		Codes for decoding Apple Single/Double object parts.
 *
 *			05aug95		mym		Created.
 *			25sep95		mym		Add support to write to binhex encoding on non-mac system.
 */

#include "msg.h"
#include "appledbl.h"
#include "ad_codes.h"
#include "m_binhex.h"

extern int MK_UNABLE_TO_OPEN_TMP_FILE;
extern int MK_MIME_ERROR_WRITING_FILE;

#ifndef APPLICATION_APPLEFILE
#define APPLICATION_APPLEFILE "application/applefile"
#endif

/*
**	Static functions.
*/
PRIVATE int from_decoder(appledouble_decode_object* p_ap_decode_obj, 
			char 	*buff, 
			int 	buff_size, 
			uint32	*in_count);
PRIVATE int from_64(appledouble_decode_object* p_ap_decode_obj, 
			char 	*buff, 
			int 	size, 
			uint32 	*real_size);
PRIVATE int from_qp(appledouble_decode_object* p_ap_decode_obj, 
			char 	*buff, 
			int 	size, 
			uint32 	*real_size);
PRIVATE int from_none(appledouble_decode_object* p_ap_decode_obj, 
			char 	*buff, 
			int 	size, 
			uint32 	*real_size);

PRIVATE int decoder_seek(appledouble_decode_object* p_ap_decode_obj, 
			int 	seek_pos, 
			int 	start_pos);
			
/*
**	fetch_a_line
**	-------------
**
**		get a line from the in stream..
*/
int fetch_a_line(
	appledouble_decode_object* p_ap_decode_obj,
	char 	*buff)
{
	int  i, left;
	char *p, c = 0;
	
	if (p_ap_decode_obj->s_leftover == 0 && 
		p_ap_decode_obj->s_inbuff <= p_ap_decode_obj->pos_inbuff)
	{
		*buff = '\0';
		return errEOB;
	}
	
	if (p_ap_decode_obj->s_leftover)
	{
		for (p = p_ap_decode_obj->b_leftover, i = p_ap_decode_obj->s_leftover; i>0; i--)
			*buff++ = *p++;
			
		p_ap_decode_obj->s_leftover = 0;
	}
	
	p    = p_ap_decode_obj->inbuff   + p_ap_decode_obj->pos_inbuff;
	left = p_ap_decode_obj->s_inbuff - p_ap_decode_obj->pos_inbuff;
	
	for (i = 0; i < left; )
	{
		c = *p++; i++;
		
		if (c == CR && *p == LF)
		{
			p++; i++;	/* make sure skip both LF & CR	*/
		}

		if (c == LF || c == CR)
			break;
		
		*buff++ = c; 
	}
	p_ap_decode_obj->pos_inbuff += i;
	
	if (i == left && c != LF && c != CR)
	{
		/*
		**  we meet the buff end before we can terminate the string,
		**	save the string to the left_over buff.
		*/
		p_ap_decode_obj->s_leftover = i;
		
		for (p = p_ap_decode_obj->b_leftover; i>0; i--)
			*p++ = *(buff-i);
			
		return errEOB;
	}
	*buff = '\0';
	return NOERR;
}

void parse_param(
	char *p, 
	char **param, 			/* the param		*/
	char **define, 			/* the defination.	*/
	char **np)				/* next position.	*/
{
	while (*p == ' ' || *p == '\"' || *p == ';') p++;
	*param = p;
	
	while (*p != ' ' && *p != '=' ) p++;
	if (*p == ' ')
		*define = p+1;
	else
		*define = p+2;
		
	while (*p && *p != ';') p++;

	if (*p == ';')
		*np = p + 1;
	else
		*np = p;
}

int ap_seek_part_start(
	appledouble_decode_object* p_ap_decode_obj)
{
	int  status;
	char newline[256];
	
	while (1)
	{
		status = fetch_a_line(p_ap_decode_obj, newline);
		if(status != NOERR)
			break;
			
		if (newline[0] == '\0' && p_ap_decode_obj->boundary0 != NULL)
			return errDone;
		
		if (!XP_STRNCASECMP(newline, "--", 2))
		{
			/* we meet the start seperator, copy it and it will be our boundary */
			p_ap_decode_obj->boundary0 = XP_STRDUP(newline+2);
			return errDone;
		}
	}
	return status;
}

int ParseFileHeader(
	appledouble_decode_object *p_ap_decode_obj)
{
	int  status;
	int	 i;
	char newline[256], *p;
	char *param, *define;
	
	while (1)
	{
		status = fetch_a_line(p_ap_decode_obj, newline);
		if (newline[0] == '\0')
			break;				/* we get the end of a defination section.	*/
		
		p = newline; 
		while (1)
		{
			parse_param(p, &param, &define, &p);
			/*
			**	we only care about these params.
			*/
			if (!XP_STRNCASECMP(param, "Content-Type:", 13))
			{
				if (!XP_STRNCASECMP(define, MULTIPART_APPLEDOUBLE,
									XP_STRLEN(MULTIPART_APPLEDOUBLE)) ||
					!XP_STRNCASECMP(define, MULTIPART_HEADER_SET,
									XP_STRLEN(MULTIPART_HEADER_SET)))
					p_ap_decode_obj->messagetype = kAppleDouble;
				else
					p_ap_decode_obj->messagetype = kGeneralMine;
			}
			else if (!XP_STRNCASECMP(param, "boundary=", 9))
			{
				for (i = 0; *define && *define != '\"'; )
					p_ap_decode_obj->boundary0[i++] = *define++;
					
				p_ap_decode_obj->boundary0[i] = '\0';
			}
			else if (!XP_STRNCASECMP(param, "Content-Disposition:", 20))
			{
				if (!XP_STRNCASECMP(define, "inline", 5))
					p_ap_decode_obj->deposition = kInline;
				else
					p_ap_decode_obj->deposition = kDontCare;
			}
			else if (!XP_STRNCASECMP(param, "filename=", 9))
			{
				for (i = 0, p=define; *p && *p != '\"'; )
					p_ap_decode_obj->fname[i++] = *p++;
					
				p_ap_decode_obj->fname[i] = '\0';
			}
			
			if (*p == '\0')
				break;
		}
	}
		
	return NOERR;
}

int ap_seek_to_boundary(
	appledouble_decode_object *p_ap_decode_obj, 
	XP_Bool firstime)
{
	int  status = NOERR;
	char buff[256];
	
	while (status == NOERR)
	{
		status = fetch_a_line(p_ap_decode_obj, buff);
		if (status != NOERR)
			break;
				
		if ((!XP_STRNCASECMP(buff, "--", 2) &&
			!XP_STRNCASECMP(	buff+2, 
						p_ap_decode_obj->boundary0, 
						XP_STRLEN(p_ap_decode_obj->boundary0))) 
		  ||!XP_STRNCASECMP(	buff, 
						p_ap_decode_obj->boundary0, 
						XP_STRLEN(p_ap_decode_obj->boundary0)))
		{
			TRACEMSG(("Found boundary: %s", p_ap_decode_obj->boundary0));
			status = errDone;
			break;
		}
	}
	
	if (firstime && status == errEOB)
		status = NOERR;				/* so we can do it again.	*/

	return	status;
}

int ap_parse_header(
	appledouble_decode_object *p_ap_decode_obj, 
	XP_Bool firstime) 
{
	int	 	status, i;
	char 	newline[256], *p;
	char 	*param, *define;
	
	if (firstime)
	{
		/* do the clean ups. */
		p_ap_decode_obj->encoding = kEncodeNone;
		p_ap_decode_obj->which_part = kFinishing;
	}
		
	while (1)
	{
		status = fetch_a_line(p_ap_decode_obj, newline);
		if (status != NOERR)
			return status;		/* a possible end of buff happened.			*/
			
		if (newline[0] == '\0')
			break;				/* we get the end of a defination section.	*/
		
		p = newline;
		while (1)
		{
			parse_param(p, &param, &define, &p);
			/*
			**	we only care about these params.
			*/
			if (!XP_STRNCASECMP(param, "Content-Type:", 13))
			{
				if (!XP_STRNCASECMP(define, APPLICATION_APPLEFILE, 21))
					p_ap_decode_obj->which_part = kHeaderPortion;
				else
				{
					p_ap_decode_obj->which_part = kDataPortion;
					if (!XP_STRNCASECMP(define, "text/plain", 10))
						p_ap_decode_obj->is_binary = FALSE;
					else
						p_ap_decode_obj->is_binary = TRUE; 
				}
			}
			else if (!XP_STRNCASECMP(param, "Content-Transfer-Encoding:",26))
			{
				if (!XP_STRNCASECMP(define, "base64", 6))
					p_ap_decode_obj->encoding = kEncodeBase64;
				else if (!XP_STRNCASECMP(define, "quoted-printable", 16))
					p_ap_decode_obj->encoding = kEncodeQP;
				else 
					p_ap_decode_obj->encoding = kEncodeNone;
			}
			else if (!XP_STRNCASECMP(param, "Content-Disposition:", 20))
			{
				if (!XP_STRNCASECMP(define, "inline", 5))
					p_ap_decode_obj->deposition = kInline;
				else
					p_ap_decode_obj->deposition = kDontCare;
			}
			else if (!XP_STRNCASECMP(param, "filename=", 9))
			{
				if (p_ap_decode_obj->fname[0] == '\0')
				{
					for (i = 0; *define && *define != '\"'; )
						p_ap_decode_obj->fname[i++] = *define++;
					
					p_ap_decode_obj->fname[i] = '\0';
				}
			}
			
			if (*p == '\0')
				break;
		}
	}
	return errDone;
}

#if 0
PRIVATE void
copy_filename(MWContext* context, char* newFile, void* closure)
{
	XP_STRCPY(closure, newFile);
}
#endif

/*
**	decode the head portion.
*/
int ap_decode_file_infor(appledouble_decode_object *p_ap_decode_obj)
{
	ap_header 	head;
	ap_entry 	entries[NUM_ENTRIES + 1];
	int 		i, j;
	int			st_pt;
	uint32		in_count;
	int			status;
	char		name[64];
	
	st_pt = p_ap_decode_obj->pos_inbuff;
	
	/*
	** 	Read & verify header 
	*/
	status = from_decoder(
					p_ap_decode_obj, 
					(char *) &head, 
					26, 		/* sizeof (head), 	*/
					&in_count);
	if (status != NOERR)
		return status;
	
	if (p_ap_decode_obj->is_apple_single)
	{
		if (ntohl(head.magic) != APPLESINGLE_MAGIC)
			return errVersion;
	}
	else
	{
		if(ntohl(head.magic) != APPLEDOUBLE_MAGIC) 
			return errVersion;
	}
		
	if (ntohl(head.version) != VERSION) 
	{
		return errVersion;
	}
	
	/* read entries */
	head.entries = ntohs(head.entries);
	for (i = j = 0; i < head.entries; ++i) 
	{
		status = from_decoder(
					p_ap_decode_obj,
					(char *) (entries + j), 
					sizeof (ap_entry), 
					&in_count);
		if (status != NOERR)
			return errDecoding;
		
		/*
		**	correct the byte order now.
		*/
		entries[j].id 	  = ntohl(entries[j].id);
		entries[j].offset = ntohl(entries[j].offset);
		entries[j].length = ntohl(entries[j].length);
		/*
		**	only care about these entries...
		*/
		if (j < NUM_ENTRIES) 
		switch (entries[j].id) 
		{
			case ENT_NAME:
			case ENT_FINFO:
			case ENT_DATES:
			case ENT_COMMENT:
			case ENT_RFORK:
			case ENT_DFORK:
				++j;
				break;
		}
	}
	
	/* read name */
	for (i = 0; i < j && entries[i].id != ENT_NAME; ++i)
		;
	if (i == j) 
		return errDecoding;
		
	status = decoder_seek(
					p_ap_decode_obj, 
					entries[i].offset,
					st_pt);	
	if (status != NOERR)
		return status;
		
	if (entries[i].length > 63) 
		entries[i].length = 63;
		
	status = from_decoder(
					p_ap_decode_obj, 
					p_ap_decode_obj->fname, 
					entries[i].length,
					&in_count);
	if (status != NOERR)
		return status;

	p_ap_decode_obj->fname[in_count] = '\0';

	/* P_String version of the file name. */
	XP_STRCPY((char *)name+1, p_ap_decode_obj->fname);
	name[0] = in_count;
	
	if (p_ap_decode_obj->write_as_binhex)
	{
		/*
		**	fill out the simple the binhex head.
		*/
		binhex_header head;
		myFInfo		  myFInfo;

		status = (*p_ap_decode_obj->binhex_stream->put_block)
					(p_ap_decode_obj->binhex_stream->data_object,
					name, 
					name[0] + 2);
		if (status != NOERR)
			return status;
		
		/* get finder info */
		for (i = 0; i < j && entries[i].id != ENT_FINFO; ++i)
			;
		if (i < j) 
		{
			status = decoder_seek(p_ap_decode_obj, 
						entries[i].offset, 
						st_pt);
			if (status != NOERR)
				return status;
				
			status = from_decoder(p_ap_decode_obj, 
						(char *) &myFInfo, 
						sizeof (myFInfo), 
						&in_count);
			if (status != NOERR)
				return status;				
		}
		
		head.type 		= myFInfo.fdType;
		head.creator 	= myFInfo.fdCreator;
		head.flags 		= myFInfo.fdFlags;

		for (i = 0; i < j && entries[i].id != ENT_DFORK; ++i)
			;
		if (i < j && entries[i].length != 0)
		{
			head.dlen = entries[i].length;	/* set the data fork length 	*/
		}	
		else
		{
			head.dlen = 0;
		}
	
		for (i = 0; i < j && entries[i].id != ENT_RFORK; ++i)
			;
		if (i < j && entries[i].length != 0)
		{
			head.rlen = entries[i].length;	/* set the resource fork length */
		}
		else
		{
			head.rlen = 0;
		}

		/*
		**	and the dlen, rlen is in the host byte order, correct it if needed ...
		*/
		head.dlen = htonl(head.dlen);
		head.rlen = htonl(head.rlen);
		/*
		**	then encode them in binhex.
		*/
		status = (*p_ap_decode_obj->binhex_stream->put_block)
					(p_ap_decode_obj->binhex_stream->data_object,
					(char*)&head,
					sizeof(binhex_header));
		if (status != NOERR)
			return status;
		
		/*
		**	after we have done with the header, end the binhex header part.
		*/
		status = (*p_ap_decode_obj->binhex_stream->put_block)
					(p_ap_decode_obj->binhex_stream->data_object,
					NULL,
					0);
	}
	else
	{
	
#ifdef	XP_MAC

		ap_dates 	dates;
		HFileInfo 	*fpb;
		CInfoPBRec 	cipbr;
		IOParam 	vinfo;
		GetVolParmsInfoBuffer vp;
		DTPBRec 	dtp;
		char 		comment[256];

		char filename[64];
		StandardFileReply	reply;
					
		XP_STRCPY(filename+1, p_ap_decode_obj->fname);
		filename[0] = XP_STRLEN(p_ap_decode_obj->fname);
														
		/* CALL fe_PromtGetFileName get a new name */
#if 0
		if (FE_PromptForFileName(NULL, 
				"Save decoded file as:", 
				filename,
				FALSE,
				FALSE,
				copy_filename,
				filename) == -1)
		{
			return -1;
		} 
		
		/*
		**	Convert it to fspec now.
		*/		
		if (my_FSSpecFromPathname(filename, &fspec) != NOERR)
		{
			return -1;
		}
		
#endif
		StandardPutFile("\pSave decoded file as:", 
					(const unsigned char*)filename, 
					&reply);
					
		if (!reply.sfGood) 
		{
			return errUsrCancel;
		}
		
		XP_MEMCPY(p_ap_decode_obj->fname, 
					reply.sfFile.name+1, 
					*(reply.sfFile.name)+1);
		p_ap_decode_obj->fname[*(reply.sfFile.name)] = '\0';
			
		p_ap_decode_obj->vRefNum = reply.sfFile.vRefNum;
		p_ap_decode_obj->dirId   = reply.sfFile.parID;
	
		/* create & get info for file */
		HDelete(reply.sfFile.vRefNum, 
					reply.sfFile.parID, 
					reply.sfFile.name);

#define DONT_CARE_TYPE	0x3f3f3f3f
	
		if (HCreate(reply.sfFile.vRefNum,
					reply.sfFile.parID, 
					reply.sfFile.name, 
					DONT_CARE_TYPE, 
					DONT_CARE_TYPE) != NOERR) 
		{
			return errFileOpen;
		}
		
		fpb = (HFileInfo *) &cipbr;
		fpb->ioVRefNum = reply.sfFile.vRefNum;
		fpb->ioDirID   = reply.sfFile.parID;
		fpb->ioNamePtr = reply.sfFile.name;
		fpb->ioFDirIndex = 0;
		PBGetCatInfoSync(&cipbr);
	
		/* get finder info */
		for (i = 0; i < j && entries[i].id != ENT_FINFO; ++i)
			;
		if (i < j) 
		{
			status = decoder_seek(p_ap_decode_obj, 
							entries[i].offset, 
							st_pt);
			if (status != NOERR)
				return status;
				
			status = from_decoder(p_ap_decode_obj, 
							(char *) &fpb->ioFlFndrInfo, 
							sizeof (FInfo), 
							&in_count);
			if (status != NOERR)
				return status;
				
			status = from_decoder(p_ap_decode_obj,
							(char *) &fpb->ioFlXFndrInfo, 
							sizeof (FXInfo),
							&in_count);
		
			if (status != NOERR)
				return status;
				
			fpb->ioFlFndrInfo.fdFlags &= 0xfc00; /* clear flags maintained by finder */
		}
	
		/*
		** 	get file date info 
		*/
		for (i = 0; i < j && entries[i].id != ENT_DATES; ++i)
			;
		if (i < j) 
		{
			status = decoder_seek(p_ap_decode_obj, 
							entries[i].offset, 
							st_pt);
			if (status != NOERR)
				return status;
				
			status = from_decoder(p_ap_decode_obj,
							(char *) &dates, 
							sizeof (dates), 
							&in_count);
			if (status != NOERR)
				return status;
				
			fpb->ioFlCrDat = dates.create - CONVERT_TIME;
			fpb->ioFlMdDat = dates.modify - CONVERT_TIME;
			fpb->ioFlBkDat = dates.backup - CONVERT_TIME;
		}
		
		/*
		** 	update info 
		*/
		fpb->ioDirID = fpb->ioFlParID;
		PBSetCatInfoSync(&cipbr);
		
		/*
		** 	get comment & save it 
		*/
		for (i = 0; i < j && entries[i].id != ENT_COMMENT; ++i)
			;
		if (i < j && entries[i].length != 0) 
		{
			memset((void *) &vinfo, '\0', sizeof (vinfo));
			vinfo.ioVRefNum = fpb->ioVRefNum;
			vinfo.ioBuffer  = (Ptr) &vp;
			vinfo.ioReqCount = sizeof (vp);
			if (PBHGetVolParmsSync((HParmBlkPtr) &vinfo) == NOERR &&
				((vp.vMAttrib >> bHasDesktopMgr) & 1)) 
			{
				memset((void *) &dtp, '\0', sizeof (dtp));
				dtp.ioVRefNum = fpb->ioVRefNum;
				if (PBDTGetPath(&dtp) == NOERR) 
				{
					if (entries[i].length > 255) 
						entries[i].length = 255;
						
					status = decoder_seek(p_ap_decode_obj, 
								entries[i].offset, 
								st_pt);
					if (status != NOERR)
						return status;
						
					status = from_decoder(p_ap_decode_obj, 
								comment, 
								entries[i].length, 
								&in_count);
					if (status != NOERR)
						return status;
						
					dtp.ioDTBuffer = (Ptr) comment;
					dtp.ioNamePtr  = fpb->ioNamePtr;
					dtp.ioDirID    = fpb->ioDirID;
					dtp.ioDTReqCount = entries[i].length;
					if (PBDTSetCommentSync(&dtp) == NOERR) 
					{
						PBDTFlushSync(&dtp);
					}
				}
			}
		}
		
#else
		/*
		**	in non-mac system, creating a data fork file will be it.
		*/
#endif
	}
	
	/*
	** Get the size the data fork.
	*/
	for (i = 0; i < j && entries[i].id != ENT_DFORK; ++i)
		;
	if (i < j && entries[i].length != 0)
	{
		p_ap_decode_obj->dksize = entries[i].length;
	}	
	else
		p_ap_decode_obj->dksize = 0;
	
	/* 
	** Seek to the position to the start of the resource place. 
	*/
	for (i = 0; i < j && entries[i].id != ENT_RFORK; ++i)
		;
	if (i < j && entries[i].length != 0)
	{
		status = decoder_seek(p_ap_decode_obj, 
								entries[i].offset, 
								st_pt);
		p_ap_decode_obj->rksize = entries[i].length;
	}
	else
		p_ap_decode_obj->rksize = 0;
	

	/*
	**	Prepare a tempfile to hold the resource fork decoded by the decoder,
	**		because in binhex, resource fork appears after the data fork!!!
	*/
	if (p_ap_decode_obj->write_as_binhex)
	{
		if (p_ap_decode_obj->rksize != 0)
		{
			/* we need a temp file to hold all the resource data, because the */
			p_ap_decode_obj->tmpfname 
				= WH_TempName(xpTemporary, "apmail");
			
			p_ap_decode_obj->tmpfd 
				= XP_FileOpen(p_ap_decode_obj->tmpfname, 
								xpTemporary, 
								XP_FILE_TRUNCATE_BIN);
								
			if (p_ap_decode_obj->tmpfd == NULL)
				return errFileOpen;
		}
	}
	return NOERR;
}

/*
**	ap_decode_process_header
**
**
*/
int ap_decode_process_header(
	appledouble_decode_object* p_ap_decode_obj, 
	XP_Bool firstime)
{
	uint32 	in_count;
	int		status = NOERR;
	char	wr_buff[1024];
	
	if (firstime)
	{
		status = ap_decode_file_infor(p_ap_decode_obj);
		if (status != NOERR)
			return status;
		
		if (p_ap_decode_obj->rksize > 0)
		{
#ifdef XP_MAC
			if(!p_ap_decode_obj->write_as_binhex)
			{
				Str63	fname;
				short	refNum;
					
				fname[0] = XP_STRLEN(p_ap_decode_obj->fname);
				XP_STRCPY((char*)fname+1, p_ap_decode_obj->fname);
				 
				if (HOpenRF(p_ap_decode_obj->vRefNum,
							p_ap_decode_obj->dirId,
							fname,
							fsWrPerm,
							&refNum) != NOERR)
				{
					return (errFileOpen);
				}
				p_ap_decode_obj->fileId = refNum;
			}
#endif
		}
		else
		{
			status = errDone;
		}
	}
	
	/*
	**	Time to continue decoding all the resource data.
	*/	
	while (status == NOERR && p_ap_decode_obj->rksize > 0)
	{
		in_count = MIN(1024, p_ap_decode_obj->rksize);
		
		status = from_decoder(p_ap_decode_obj, 
							wr_buff, 
							in_count, 
							&in_count);
		
		if (p_ap_decode_obj->write_as_binhex)
		{	
			/*
			**	Write to the temp file first, because the resource fork appears after
			**	 the data fork in the binhex encoding.
			*/
			if (XP_FileWrite(wr_buff, 
							in_count, 
							p_ap_decode_obj->tmpfd)  != in_count)
			{
				status = errFileWrite;						
				break;
			}
			p_ap_decode_obj->data_size += in_count;
		}
		else
		{
#ifdef XP_MAC
			long howMuch = in_count;
			
			if (FSWrite(p_ap_decode_obj->fileId, 
							&howMuch,
							wr_buff) != NOERR)
			{
				status = errFileWrite;
				break;
			}
#else
			/* ======	Write nothing in a non mac file system	============ */
#endif
		}
		
		p_ap_decode_obj->rksize -= in_count;
	}
	
	if (p_ap_decode_obj->rksize <= 0 || status == errEOP)
	{
		if (p_ap_decode_obj->write_as_binhex)
		{
			/*
			**	No more resource data, but we are not done
			**  with tempfile yet, just seek back to the start point, 
			**		-- ready for a readback later 
			*/
			if (p_ap_decode_obj->tmpfd)
				XP_FileSeek(p_ap_decode_obj->tmpfd, 0L, 1);
		}
		
#ifdef XP_MAC
		else if (p_ap_decode_obj->fileId)	/* close the resource fork of the macfile	*/
		{
			FSClose(p_ap_decode_obj->fileId);
			p_ap_decode_obj->fileId = 0;
		}
#endif
		if (!p_ap_decode_obj->is_apple_single)
		{
			p_ap_decode_obj->left 	 = 0;
			p_ap_decode_obj->state64 = 0;
		}
		status = errDone;
	}	
	return status;
}

int ap_decode_process_data(
	appledouble_decode_object* p_ap_decode_obj, 
	XP_Bool 	firstime)
{
	char 	wr_buff[1024];
	uint32  in_count;
	int	 	status = NOERR;
	int  	retval = NOERR;
	
	if (firstime)
	{
		if (!p_ap_decode_obj->write_as_binhex)
		{
#ifdef XP_MAC
			char	*filename;
			FSSpec	fspec;
			
			fspec.vRefNum = p_ap_decode_obj->vRefNum;
			fspec.parID   = p_ap_decode_obj->dirId;
			fspec.name[0] = XP_STRLEN(p_ap_decode_obj->fname);
			XP_STRCPY((char*)fspec.name+1, p_ap_decode_obj->fname);
			
			filename = my_PathnameFromFSSpec(&fspec);
			if (p_ap_decode_obj->is_binary)
				p_ap_decode_obj->fd = 
						XP_FileOpen(filename+7, xpURL, XP_FILE_TRUNCATE_BIN);
			else
				p_ap_decode_obj->fd = 
						XP_FileOpen(filename+7, xpURL, XP_FILE_TRUNCATE);
			XP_FREE(filename);				
#else			
			if (p_ap_decode_obj->is_binary)
				p_ap_decode_obj->fd = 
						XP_FileOpen(p_ap_decode_obj->fname, xpURL, XP_FILE_TRUNCATE_BIN);
			else
				p_ap_decode_obj->fd = 
						XP_FileOpen(p_ap_decode_obj->fname, xpURL, XP_FILE_TRUNCATE);
#endif
		}
		else
		{
			;	/*	== don't need do anything to binhex stream, it is ready already  == */
		}
	}
	
	if (p_ap_decode_obj->is_apple_single && 
		p_ap_decode_obj->dksize == 0)
	{
		/* if no data in apple single, we already done then.	*/
		status = errDone;
	}

	while (status == NOERR && retval == NOERR)
	{	
		retval = from_decoder(p_ap_decode_obj,
						wr_buff, 
						1024, 
						&in_count);

		if (p_ap_decode_obj->is_apple_single)		/* we know the data fork size in 			*/
			p_ap_decode_obj->dksize -= in_count;	/*	apple single, use it to decide the end	*/
						
		if (p_ap_decode_obj->write_as_binhex)		
			status = (*p_ap_decode_obj->binhex_stream->put_block)
						(p_ap_decode_obj->binhex_stream->data_object, 
						wr_buff, 
						in_count);
		else
			status = XP_FileWrite(wr_buff, 
						in_count, 
						p_ap_decode_obj->fd) == in_count ? NOERR : errFileWrite;
								
		if (retval == errEOP ||						/* for apple double, we meet the boundary	*/
			( p_ap_decode_obj->is_apple_single && 
			  p_ap_decode_obj->dksize <= 0))		/* for apple single, we know it is ending	*/
		{
			status = errDone;
			break;
		}
	}
	
	if (status == errDone)
	{
		if (p_ap_decode_obj->write_as_binhex)		
		{
			/* CALL with data == NULL && size == 0 to end a part object in binhex encoding */			
			status = (*p_ap_decode_obj->binhex_stream->put_block)
						(p_ap_decode_obj->binhex_stream->data_object, 
						 NULL, 
						 0);
			if (status != NOERR)
				return status;
		}
		else if (p_ap_decode_obj->fd)
		{
			XP_FileClose(p_ap_decode_obj->fd);
			p_ap_decode_obj->fd = 0;		
		}

		status = errDone;
	}
	return status;
}

/*
**	Fill the data from the decoder stream.
*/
PRIVATE int from_decoder(
	appledouble_decode_object* p_ap_decode_obj, 
	char 	*buff, 
	int 	buff_size, 
	uint32	*in_count)
{
	int 	status;
	
	switch (p_ap_decode_obj->encoding)
	{
		case kEncodeQP:
			status = from_qp(p_ap_decode_obj,
						buff,
						buff_size,
						in_count);
			break;
		case kEncodeBase64:
			status = from_64(p_ap_decode_obj,
						buff,
						buff_size,
						in_count);
			break;
		case kEncodeNone:
		default:
			status = from_none(p_ap_decode_obj, 
						buff,
						buff_size,
						in_count);
			break;
	}
	return status;
}

/*
**	decoder_seek
**
**	simulate a stream seeking on the encoded stream.
*/
PRIVATE int decoder_seek(
	appledouble_decode_object* p_ap_decode_obj, 
	int 	seek_pos, 
	int 	start_pos)
{
	char 	tmp[1024];
	int  	status = NOERR;
	uint32	 in_count;
	
	/*
	**	 force a reset on the in buffer.
	*/
	p_ap_decode_obj->state64 	= 0;
	p_ap_decode_obj->left    	= 0;
	p_ap_decode_obj->pos_inbuff = start_pos;
	
	while (seek_pos > 0)
	{
		status = from_decoder(p_ap_decode_obj, 
						tmp, 
						MIN(1024, seek_pos), 
						&in_count);
		if (status != NOERR)
			break;
		
		seek_pos -= in_count;
	}
	return status;
}

#define XX 127
/*
 * Table for decoding base64
 */
static char index_64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};
#define CHAR64(c)  (index_64[(unsigned char)(c)])
#define EndOfBuff(p)	((p)->pos_inbuff >= (p)->s_inbuff)

PRIVATE int fetch_next_char_64(
	appledouble_decode_object* p_ap_decode_obj)
{
	char c;
	
	c = p_ap_decode_obj->inbuff[p_ap_decode_obj->pos_inbuff++];
	if (c == '-')
		--p_ap_decode_obj->pos_inbuff;		/* put back					*/
	
	while (c == LF || c == CR)				/* skip the CR character.	*/
	{
		if (EndOfBuff(p_ap_decode_obj))
		{
			c = 0;
			break;
		}
		
		c = p_ap_decode_obj->inbuff[p_ap_decode_obj->pos_inbuff++];
		if (c == '-')
		{
			--p_ap_decode_obj->pos_inbuff;	/* put back					*/
		}
	}
	return (int)c;
}


PRIVATE int from_64(
	appledouble_decode_object* p_ap_decode_obj, 
	char	 *buff, 
	int 	size, 
	uint32 	*real_size)
{
	int 	i, j, buf[4];
	int 	c1, c2, c3, c4;
	
	(*real_size) = 0;
	
	/*
	** 	decode 4 by 4s characters
	*/
	for (i = p_ap_decode_obj->state64; i<4; i++)
	{
		if (EndOfBuff(p_ap_decode_obj))
		{
			p_ap_decode_obj->state64 = i;
			break;
		}
		if ((p_ap_decode_obj->c[i] = fetch_next_char_64(p_ap_decode_obj)) == 0)
			break;
	}
	
	if (i != 4)
	{
		/*
		** not enough data to fill the decode buff.
		*/
		return errEOB;						/* end of buff	*/
	}
		
	while (size > 0)
	{
		c1 = p_ap_decode_obj->c[0];
		c2 = p_ap_decode_obj->c[1];
		c3 = p_ap_decode_obj->c[2];
		c4 = p_ap_decode_obj->c[3];
		
		if (c1 == '-' || c2 == '-' || c3 == '-' || c4 == '-')
		{
			return errEOP;			/* we meet the part boundary.	*/
		}
		
        if (c1 == '=' || c2 == '=') 
        {
            return errDecoding;
        }
        
        c1 = CHAR64(c1);
        c2 = CHAR64(c2);
		buf[0] = ((c1<<2) | ((c2&0x30)>>4));
		
        if (c3 != '=') 
        {
            c3 = CHAR64(c3);
		    buf[1] = (((c2&0x0F) << 4) | ((c3&0x3C) >> 2));

            if (c4 != '=') 
            {
                c4 = CHAR64(c4);
				buf[2] = (((c3&0x03) << 6) | c4);
            }
            else
            {
            	if (p_ap_decode_obj->left == 0)
            	{
	        		*buff++ = buf[0]; (*real_size)++;
	        	}
	        	*buff++ = buf[1]; (*real_size)++;
				return errEOP;       	
            }
        }
        else
        {
        	*buff++ = *buf;
        	(*real_size)++;
			return errEOP;				/* we meet the the end	*/	
        }
        /*
		** copy the content
		*/
		for (j = p_ap_decode_obj->left; j<3; )
		{
			*buff++ = buf[j++];	
			(*real_size)++;
			if (--size <= 0)
				break;
		}
		p_ap_decode_obj->left = j % 3;
			
		if (size <=0)
		{
			if (j == 3)
				p_ap_decode_obj->state64 = 0;	/* See if we used up all data, 	*/
												/* ifnot, keep the data, 		*/
												/* we need it for next time.	*/
			else
				p_ap_decode_obj->state64 = 4;
				
			break;
		}
			
		/*
		**	fetch the next 4 character group. 
		*/
		for (i = 0; i < 4; i++)
		{
			if (EndOfBuff(p_ap_decode_obj))
				break;

			if ((p_ap_decode_obj->c[i] = fetch_next_char_64(p_ap_decode_obj)) == 0)
				break;
		}

		p_ap_decode_obj->state64 = i % 4;
	
		if (i != 4)
			break;								/* some kind of end of buff met.*/
	}

	/*
	**	decide the size and status. 
	*/
	return EndOfBuff(p_ap_decode_obj) ? errEOB : NOERR;
}

/*
 * Table for decoding hexadecimal in quoted-printable
 */
static char index_hex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

#define HEXCHAR(c)  	(index_hex[(unsigned char)(c)])
#define NEXT_CHAR(p)	((int)((p)->inbuff[(p)->pos_inbuff++]))
/* 
**	quoted printable decode, as defined in RFC 1521, page18 - 20
*/
PRIVATE int from_qp(
	appledouble_decode_object* p_ap_decode_obj, 
	char	*buff, 
	int 	size, 
	uint32 	*real_size)
{
    char c;
    int c1, c2;
	
	*real_size = 0;
	
	if (p_ap_decode_obj->c[0] == '=')
	{
		/*
		** continue with the last time's left over.
		*/
		p_ap_decode_obj->c[0] = 0;
		
		c1 = p_ap_decode_obj->c[1];		p_ap_decode_obj->c[1] = 0;
		
		if ( c1 == 0)
		{
			c1 = NEXT_CHAR(p_ap_decode_obj);
			c2 = NEXT_CHAR(p_ap_decode_obj);
		}
		else
		{
			c2 = NEXT_CHAR(p_ap_decode_obj);
		}
		c = HEXCHAR(c1) << 4 | HEXCHAR(c2);
		
		size --;
		*buff ++ = c;
		(*real_size) ++;
	}

	/* 
	**	Then start to work on the new data 
	*/
	while (size > 0)
	{
		if (EndOfBuff(p_ap_decode_obj))
			break;

		c1 = NEXT_CHAR(p_ap_decode_obj);
	
		if (c1 == '=')
		{
			if (EndOfBuff(p_ap_decode_obj))
			{
				p_ap_decode_obj->c[0] = c1;
				break;				
			}
			
			c1 = NEXT_CHAR(p_ap_decode_obj);
			if (c1 != '\n') 
			{
				/*
				**	Rule #2	
				*/
				c1 = HEXCHAR(c1);
				if (EndOfBuff(p_ap_decode_obj))
				{
					p_ap_decode_obj->c[0] = '=';
					p_ap_decode_obj->c[1] = c1;
					break;
				}
				
				c2 = NEXT_CHAR(p_ap_decode_obj);
				c2 = HEXCHAR(c2);
				c = c1 << 4 | c2;
				if (c != '\r')
				{
					size --;
					*buff++ = c;
					(*real_size)++;
				}
			}
			else
			{
				/* ignore the line break -- soft line break, rule #5	*/
			}
		}
		else 
		{
			if (c1 == CR || c1 == LF)
			{
				if (p_ap_decode_obj->pos_inbuff < p_ap_decode_obj->s_inbuff)
				{
					if (p_ap_decode_obj->boundary0 && 
					 	(!XP_STRNCASECMP(p_ap_decode_obj->pos_inbuff+p_ap_decode_obj->inbuff, 
									"--",
									2) 
					&&
						!XP_STRNCASECMP(p_ap_decode_obj->pos_inbuff+p_ap_decode_obj->inbuff+2, 
									p_ap_decode_obj->boundary0, 
									XP_STRLEN(p_ap_decode_obj->boundary0))))
						{
							return errEOP;
						} 
				}
			}
			
			/* 
			**	general 8bits case, Rule #1
			*/ 
			size -- ;
			*buff++ = c1;
			(*real_size) ++;
		}
	}
	return EndOfBuff(p_ap_decode_obj) ? errEOB : NOERR;
}

/*
**	from_none
**
**	plain text transfer.
*/
PRIVATE int from_none(
	appledouble_decode_object* p_ap_decode_obj, 
	char 	*buff, 
	int 	size, 
	uint32 	*real_size)
{
	char 	c;
    int  	i, status = NOERR; 
    int		left = p_ap_decode_obj->s_inbuff - p_ap_decode_obj->pos_inbuff;
    int		total = MIN(size, left);
    
	for (i = 0; i < total; i++)
	{	
		*buff ++ = c = NEXT_CHAR(p_ap_decode_obj);
		if (c == CR || c == LF)
		{
			/* make sure the next thing is not a boundary string */
			if (p_ap_decode_obj->pos_inbuff < p_ap_decode_obj->s_inbuff)
			{
				if (p_ap_decode_obj->boundary0 && 
				 	(!XP_STRNCASECMP(p_ap_decode_obj->pos_inbuff+p_ap_decode_obj->inbuff, 
								"--",
								2) 
				&&
					!XP_STRNCASECMP(p_ap_decode_obj->pos_inbuff+p_ap_decode_obj->inbuff+2, 
								p_ap_decode_obj->boundary0, 
								XP_STRLEN(p_ap_decode_obj->boundary0))))
					{
						status = errEOP;
						break;
					} 
			}
		}
	}
	
	*real_size = i;
	if (status == NOERR)
		status = (left == i) ? errEOB : status;
	return status;
}


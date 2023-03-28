/* -*- Mode: C; tab-width: 4 -*-
*
*	Apple Double encode/decode stream
*	----------------------------------
*
*		11sep95		mym		created.
*/

#include "msg.h"
#include "appledbl.h"
#include "m_binhex.h"
#include "m_cvstrm.h"
#include "ad_codes.h"
/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_MSG_SAVE_ATTACH_AS;

#ifdef XP_MAC

extern int MK_UNABLE_TO_OPEN_TMP_FILE;
extern int MK_MIME_ERROR_WRITING_FILE;

/*	---------------------------------------------------------------------------------
**
**		The codes for Apple-double encoding stream. --- it's only useful on Mac OS
**
**	---------------------------------------------------------------------------------
*/

#define WORKING_BUFF_SIZE	2048

typedef struct	_AppledoubleEncodeObject
{
	appledouble_encode_object	ap_encode_obj;
	
	char*	buff;							/*	the working buff.					*/
	int32	s_buff;							/*	the working buff size.				*/

	XP_File	fp;								/*  file to hold the encoding			*/
	char	*fname;							/*	and the file name.					*/
	
} AppleDoubleEncodeObject;

/*
	Let's go "l" characters forward of the encoding for this write.
	Note:	
		"s" is just a dummy paramter. 
 */
PRIVATE int 
net_AppleDouble_Encode_Write (
	void *closure, CONST char* s, int32 l)
{
	int  status = 0;
	AppleDoubleEncodeObject * obj = (AppleDoubleEncodeObject*)closure;
	int32 count, size;
			
	while (l > 0)
	{
		size = obj->s_buff * 11 / 16;
		size = MIN(l, size);
		status = ap_encode_next(&(obj->ap_encode_obj), 
							obj->buff, 
							size, 
							&count);
		if (status == noErr || status == errDone)
		{
			/*
			 * we get the encode data, so call the next stream to write it to the disk.
			 */
			if (XP_FileWrite(obj->buff, count, obj->fp) != count)
				return errFileWrite;
		}
		
		if (status != noErr ) 							/* abort when error	/ done?		*/
			break;
			
		l -= size;
	}
	return status;
}

/*
** is the stream ready for writing?
 */
PRIVATE unsigned int net_AppleDouble_Encode_Ready (void *closure)
{
   return(MAX_WRITE_READY);  						/* always ready for writing 	*/ 
}


PRIVATE void net_AppleDouble_Encode_Complete (void *closure)
{
	AppleDoubleEncodeObject * obj = (AppleDoubleEncodeObject*)closure;

    ap_encode_end(&(obj->ap_encode_obj), false);	/* this is a normal ending 		*/
 	
 	if (obj->fp)
 	{
    	XP_FileClose(obj->fp);						/* done with the target file	*/

        FREEIF(obj->fname);							/* and the file name too		*/
    }
								
    FREEIF(obj->buff);								/* free the working buff.		*/
    FREE(obj);
}

PRIVATE void net_AppleDouble_Encode_Abort (void  * closure, int status)
{
	AppleDoubleEncodeObject * obj = (AppleDoubleEncodeObject*)closure;
	
    ap_encode_end(&(obj->ap_encode_obj), true);		/* it is an aborting exist... 	*/

 	if (obj->fp)
 	{   
 		XP_FileClose(obj->fp);

        XP_FileRemove (obj->fname, xpURL);			/* remove the partial file.		*/
        
        FREEIF(obj->fname);
	}
    FREEIF(obj->buff);								/* free the working buff.		*/
    FREE(obj);
}

/*
**	fe_MakeAppleDoubleEncodeStream
**	------------------------------
**
**	Will create a apple double encode stream:
**
**		-> take the filename as the input source (it needs to be a mac file.)
**		-> take a file name for the temp file we are generating.
*/

PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleEncodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         char*		src_filename,
                         char* 		dst_filename,
                         char*		separator)
{
    AppleDoubleEncodeObject* obj;
    NET_StreamClass* 	  stream;
    char*				  working_buff;
    
    TRACEMSG(("Setting up apple encode stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL) 
        return(NULL);

    obj = XP_NEW(AppleDoubleEncodeObject);
    if (obj == NULL) 
    {
        XP_FREE (stream);
        return(NULL);
    }
   
   	working_buff = (char *)XP_ALLOC(WORKING_BUFF_SIZE);
   	if (working_buff == NULL)
   	{
   		XP_FREE (obj);
   		XP_FREE (stream);
		return (NULL);
   	}
  	
    stream->name           = "Apple Double Encode";
    stream->complete       = (MKStreamCompleteFunc) 	net_AppleDouble_Encode_Complete;
    stream->abort          = (MKStreamAbortFunc) 		net_AppleDouble_Encode_Abort;
    stream->put_block      = (MKStreamWriteFunc) 		net_AppleDouble_Encode_Write;
    stream->is_write_ready = (MKStreamWriteReadyFunc) 	net_AppleDouble_Encode_Ready;
    stream->data_object    = obj;
    stream->window_id      = window_id;

	obj->fp  = XP_FileOpen(dst_filename, xpFileToPost, XP_FILE_WRITE_BIN);
	if (obj->fp == NULL)
	{
		XP_FREE (working_buff);
   		XP_FREE (obj);
   		XP_FREE (stream);
		return (NULL);
	}
	
	obj->fname = XP_STRDUP(dst_filename);
	 
	obj->buff	= working_buff;
	obj->s_buff = WORKING_BUFF_SIZE;
	
	/*
	** 	setup all the need information on the apple double encoder.
	*/
	ap_encode_init(&(obj->ap_encode_obj), 
					src_filename,					/* pass the file name of the source.	*/
					separator);
					
    TRACEMSG(("Returning stream from NET_AppleDoubleEncoder\n"));

    return stream;
}
#endif

/*
**	---------------------------------------------------------------------------------
**
**		The codes for the Apple sigle/double decoding.
**
**	---------------------------------------------------------------------------------
*/
typedef	struct AppleDoubleDecodeObject
{
	appledouble_decode_object	ap_decode_obj;
	
	char*	in_buff;						/* the temporary buff to accumulate  	*/
											/* the input, make sure the call to  	*/
											/* the dedcoder engine big enough buff 	*/
	int32	bytes_in_buff;					/* the count for the temporary buff.	*/
	
    NET_StreamClass*	binhex_stream;		/* a binhex encode stream to convert 	*/
    										/* the decoded mac file to binhex.		*/
    										
} AppleDoubleDecodeObject;

PRIVATE int
net_AppleDouble_Decode_Write (
	void * closure, const char* s, int32 l)
{
	int 	status = NOERR;
	AppleDoubleDecodeObject * obj = (AppleDoubleDecodeObject*) closure;
	int32	size;

	/*
	**	To force an effecient decoding, we should	
	**  make sure that the buff pass to the decode next is great than 1024 bytes.
	*/
	if (obj->bytes_in_buff + l > 1024)
	{
		size = 1024 - obj->bytes_in_buff;
		XP_MEMCPY(obj->in_buff+obj->bytes_in_buff, 
					s, 
					size);
		s += size;
		l -= size;
		
		status = ap_decode_next(&(obj->ap_decode_obj), 
					obj->in_buff, 
					1024);
		obj->bytes_in_buff = 0;
	}
	
	if (l > 1024)
	{
		/* we are sure that obj->bytes_in_buff == 0 at this point. */
		status = ap_decode_next(&(obj->ap_decode_obj), 
					(char *)s, 
					l);		
	}
	else
	{
		/* and we are sure we will not get overflow with the buff. */ 
		XP_MEMCPY(obj->in_buff+obj->bytes_in_buff, 
					s, 
					l);
		obj->bytes_in_buff += l;
	}
	return status;
}

PRIVATE unsigned int 
net_AppleDouble_Decode_Ready (AppleDoubleDecodeObject * obj)
{
   return(MAX_WRITE_READY); 	 						/* always ready for writing 	*/ 
}


PRIVATE void 
net_AppleDouble_Decode_Complete (void *closure)
{
	AppleDoubleDecodeObject *obj = (AppleDoubleDecodeObject *)closure;
	
	if (obj->bytes_in_buff)
	{
		
		ap_decode_next(&(obj->ap_decode_obj), 			/* do the last calls.			*/
				(char *)obj->in_buff,
				obj->bytes_in_buff);
		obj->bytes_in_buff = 0;
	}

    ap_decode_end(&(obj->ap_decode_obj), FALSE);		/* it is a normal clean up cases.*/

	if (obj->binhex_stream)
		XP_FREE(obj->binhex_stream);
	
	if (obj->in_buff)
		XP_FREE(obj->in_buff);
		
	XP_FREE(obj);
}

PRIVATE void 
net_AppleDouble_Decode_Abort (
	void *closure, int status)
{
	AppleDoubleDecodeObject *obj = (AppleDoubleDecodeObject *)closure;

	ap_decode_end(&(obj->ap_decode_obj), TRUE);			/* it is an abort. 				*/
	
	if (obj->binhex_stream)
		XP_FREE(obj->binhex_stream);
	
	if (obj->in_buff)
		XP_FREE(obj->in_buff);
		
	XP_FREE(obj);
}


/*
**	fe_MakeAppleDoubleDecodeStream_1
**	---------------------------------
**	
**		Create the apple double decode stream.
**
**		In the Mac OS, it will create a stream to decode to an apple file;
**
**		In other OS,  the stream will decode apple double object, 
**					  then encode it in binhex format, and save to the file.
*/
#ifndef XP_MAC
static void
simple_copy(MWContext* context, char* saveName, void* closure)
{
	/* just copy the filename to the closure, so the caller can get it.	*/
	XP_STRCPY(closure, saveName);
}
#endif

PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleDecodeStream_1 (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id)
{
#ifdef XP_MAC
	return fe_MakeAppleDoubleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						false,
						NULL);
#else

#if 0		/* just a test in the mac OS	*/
	NET_StreamClass *p;
	char*	url; 
	StandardFileReply	reply;
		
	StandardPutFile("\pSave binhex encoded file as:", "\pUntitled", &reply);
	if (!reply.sfGood) 
	{
		return NULL;
	}
	url = my_PathnameFromFSSpec(&(reply.sfFile));
	
	p = fe_MakeAppleDoubleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						true,
						url+7);
	XP_FREE(url);
	return (p);

#else	/* for the none mac-os to get a file name */

	NET_StreamClass *p;
	char* filename;
	
	filename = XP_ALLOC(1024);
	if (filename == NULL)
		return NULL;

	if (FE_PromptForFileName(window_id, 
				XP_GetString(MK_MSG_SAVE_ATTACH_AS),
				0,
				FALSE,
				FALSE,
				simple_copy,
				filename) == -1)
	{
		return	NULL;
	}
	
	p = fe_MakeAppleDoubleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						TRUE,
						filename);
	XP_FREE(filename);
	return (p);	

#endif
	
#endif
}

                    
PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleDecodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         XP_Bool	write_as_binhex,
                         char 		*dst_filename)	
{
    AppleDoubleDecodeObject* obj;
    NET_StreamClass* 	  stream;
    
    TRACEMSG(("Setting up apple double decode stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL) 
        return(NULL);

    obj = XP_NEW(AppleDoubleDecodeObject);
    if (obj == NULL)
    {
    	XP_FREE(stream);
        return(NULL);
    }
    
    stream->name           = "AppleDouble Decode";
    stream->complete       = (MKStreamCompleteFunc) 	net_AppleDouble_Decode_Complete;
    stream->abort          = (MKStreamAbortFunc) 		net_AppleDouble_Decode_Abort;
    stream->put_block      = (MKStreamWriteFunc) 		net_AppleDouble_Decode_Write;
    stream->is_write_ready = (MKStreamWriteReadyFunc) 	net_AppleDouble_Decode_Ready;
    stream->data_object    = obj;
    stream->window_id      = window_id;

	/*
	** setup all the need information on the apple double encoder.
	*/
	obj->in_buff = (char *)XP_ALLOC(1024);
	if (obj->in_buff == NULL)
	{
		XP_FREE(obj);
		XP_FREE(stream);
		return (NULL);
	}
	
	obj->bytes_in_buff = 0;
	
	if (write_as_binhex)
	{
		obj->binhex_stream = 
			fe_MakeBinHexEncodeStream(format_out,
							data_obj,
							URL_s,
							window_id,
							dst_filename);
		if (obj->binhex_stream == NULL)
		{
			XP_FREE(obj);
			XP_FREE(stream);
			XP_FREE(obj->in_buff);
			return NULL;
		}
		
		ap_decode_init(&(obj->ap_decode_obj), 
							FALSE,
							TRUE, 
							obj->binhex_stream);		
	}
	else
	{
		obj->binhex_stream = NULL;
		ap_decode_init(&(obj->ap_decode_obj), 
							FALSE,
							FALSE, 
							window_id);
	}

    TRACEMSG(("Returning stream from NET_AppleDoubleDecode\n"));

    return stream;
}

/*
**	fe_MakeAppleSingleDecodeStream_1
**	--------------------------------
**	
**		Create the apple single decode stream.
**
**		In the Mac OS, it will create a stream to decode object to an apple file;
**
**		In other OS,  the stream will decode apple single object, 
**					  then encode context in binhex format, and save to the file.
*/

PUBLIC NET_StreamClass * 
fe_MakeAppleSingleDecodeStream_1 (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id)
{
#ifdef XP_MAC
	return fe_MakeAppleSingleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						FALSE,
						NULL);
#else

#if 0		/* just a test in the mac OS	*/
	NET_StreamClass *p;
	char*	url; 
	StandardFileReply	reply;
		
	StandardPutFile("\pSave binhex encoded file as:", "\pUntitled", &reply);
	if (!reply.sfGood) 
	{
		return NULL;
	}
	url = my_PathnameFromFSSpec(&(reply.sfFile));
	
	p = fe_MakeAppleSingleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						true,
						url+7);
	XP_FREE(url);
	return (p);

#else	/* for the none mac-os to get a file name */

	NET_StreamClass *p;
	char* filename;
	
	filename = XP_ALLOC(1024);
	if (filename == NULL)
		return NULL;

	if (FE_PromptForFileName(window_id, 
				XP_GetString(MK_MSG_SAVE_ATTACH_AS),
				0,
				FALSE,
				FALSE,
				simple_copy,
				filename) == -1)
	{
		return	NULL;
	}
	
	p = fe_MakeAppleSingleDecodeStream(format_out, 
						data_obj, 
						URL_s, 
						window_id, 
						TRUE,
						filename);
	XP_FREE(filename);
	return (p);	

#endif

#endif
}

/*
**	Create the Apple Doube Decode stream.
**
*/
PUBLIC NET_StreamClass * 
fe_MakeAppleSingleDecodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         XP_Bool	write_as_binhex,
                         char 		*dst_filename)	
{
    AppleDoubleDecodeObject* obj;
    NET_StreamClass* 	  stream;
    
    TRACEMSG(("Setting up apple single decode stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL) 
        return(NULL);

    obj = XP_NEW(AppleDoubleDecodeObject);
    if (obj == NULL)
    {
    	XP_FREE(stream);
        return(NULL);
    }
    
    stream->name           = "AppleSingle Decode";
    stream->complete       = (MKStreamCompleteFunc) 	net_AppleDouble_Decode_Complete;
    stream->abort          = (MKStreamAbortFunc) 		net_AppleDouble_Decode_Abort;
    stream->put_block      = (MKStreamWriteFunc) 		net_AppleDouble_Decode_Write;
    stream->is_write_ready = (MKStreamWriteReadyFunc) 	net_AppleDouble_Decode_Ready;
    stream->data_object    = obj;
    stream->window_id      = window_id;

	/*
	** setup all the need information on the apple double encoder.
	*/
	obj->in_buff = (char *)XP_ALLOC(1024);
	if (obj->in_buff == NULL)
	{
		XP_FREE(obj);
		XP_FREE(stream);
		return (NULL);
	}
	
	obj->bytes_in_buff = 0;
	
	if (write_as_binhex)
	{
		obj->binhex_stream = 
			fe_MakeBinHexEncodeStream(format_out,
							data_obj,
							URL_s,
							window_id,
							dst_filename);
		if (obj->binhex_stream == NULL)
		{
			XP_FREE(obj);
			XP_FREE(stream);
			XP_FREE(obj->in_buff);
			return NULL;
		}
		
		ap_decode_init(&(obj->ap_decode_obj), 
							TRUE,
							TRUE, 
							obj->binhex_stream);		
	}
	else
	{
		obj->binhex_stream = NULL;
		ap_decode_init(&(obj->ap_decode_obj), 
							TRUE,
							FALSE, 
							window_id);
	}

    TRACEMSG(("Returning stream from NET_AppleSingleDecode\n"));

    return stream;
}

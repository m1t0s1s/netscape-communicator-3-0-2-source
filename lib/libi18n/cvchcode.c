/* -*- Mode: C; tab-width: 4 -*- */
/*	cvchcode.c	*/

#include "intlpriv.h"
#include "xp.h"
#include "libi18n.h"


/* 
  This function return csid if it finds Meta charset info

  Currently there are two way to specify Meta charset
    1. <META HTTP-EQUIV=Content-Type CONTENT="text/html; charset=iso-8859-1">
    2. <META charset=iso-8859-1>

  Right now, it scans for charset	

*/

PRIVATE int PeekMetaCharsetTag(CCCDataObject *obj, char *pSrc, int len)
{
	char *p;
	char charset[MAX_CSNAME+1];
	int  i, k;
	int  n;
	char ch;

	if (len > 1000) /* if block is big, we only try first 1000 byte */
		len = 1000;
	for (i = 0; i < len; i++, pSrc++)
	{
		if (*pSrc != '<')
			continue;

		pSrc ++ ;
		i ++ ;
		if ((i+13) > len)  /* at least need more than 13 bytes */
			break;
		switch (*pSrc)	{
		case 'm':
		case 'M':
			if (strncasecomp(pSrc, "meta", 4) == 0)
			{
				pSrc = pSrc + 4; 
				i += 4;

				n = len - i;  /* set n here to make sure it won't go 
			                     out of block boundary  */
				for (p = pSrc; n > 0; p ++, n--)
				{
					if (*p == '>' || *p == '<')
						break ;

					if (n < 9)
					{ 	
						n = 0;
						break;
					}

					if ((n > 9) && (*p == 'c' || *p == 'C') && strncasecomp(p, "charset", 7) == 0)
					{
						p += 7;
						n -= 7;
						while ((n > 0) && (*p == ' ' || *p == '\t')) /* skip spaces */
						{
							p ++ ;
							n --;
						}
						if (n <= 0 || *p != '=')
							break;
						p ++;
						n --;
						while ((n > 0) && (*p == ' ' || *p == '\t')) /* skip spaces */
						{
							n --;
							p ++ ;
						}
					
						if (n <= 0)
							break;
						/* now we go to find real charset name and convert it to csid */
						if (*p == '\"' || *p == '\'')
						{
							p ++ ;
							n -- ;
							for (k = 0; n > 0 && k < MAX_CSNAME && *p != '\'' && *p != '\"'; p++, k++, n--)
								charset[k] = *p ;
							charset[k] = '\0';
						}
						else
						{
							for (k = 0; n > 0 && k < MAX_CSNAME && *p != '\'' && *p != '\"' && *p != '>'; p++, k++, n--)
								charset[k] = *p ;
							charset[k] = '\0';
						}

						if (*charset && (n >= 0))	/* ftang : change from if (*charset && (n > 0)) */
						{
							int16 doc_csid = INTL_CharSetNameToID(charset);

							if (doc_csid == CS_UNKNOWN)
							{
								doc_csid = CS_DEFAULT;
							}
							return doc_csid;
						}

					}
				}
				if (n == 0)  /* no more data left  */
					return 0;
			}
			break;
		case 'b':
		case 'B':			/* quit if we see <BODY ...> tag */
			if (strncasecomp(pSrc, "body", 4) == 0)
				return 0;
			break;
		case 'p':
		case 'P':			/* quit if we see <P ...> tag */
			ch = *(pSrc + 1);
			if (ch == '>' || ch == ' ' || ch == '\t')
				return 0;
			break;
		}
	}

	return 0;
}



PRIVATE int
net_CharCodeConv(	CCCDataObject		*obj,
					const unsigned char	*buf,	/* buffer for conversion	*/
					int32				bufsz)	/* buffer size in bytes		*/
{
	unsigned char	*tobuf;
	int				rv;

	tobuf = (unsigned char *)obj->cvtfunc(obj, buf, bufsz);

	if (tobuf) {
		rv = (*(obj)->next_stream->put_block) (obj->next_stream->data_object,
			(const char *)tobuf, obj->len);
		if (tobuf != buf)
			XP_FREE(tobuf);
	    return(rv);
	} else {
		return(obj->retval);
	}
}

	/* Null Char Code Conversion module -- pass unconverted data downstream	*/
PRIVATE int
net_NoCharCodeConv (CCCDataObject *obj, const char *s, int32 l)
{
	return((*obj->next_stream->put_block)(obj->next_stream->data_object,s,l));
}

PRIVATE int
net_AutoCharCodeConv (CCCDataObject *obj, const char *s, int32 l)
{
	int16	doc_csid;
	unsigned char	*tobuf = NULL;
	int				rv;

/* for debugging -- erik */
#if 0
	{
		static FILE *f = NULL;

		if (!f)
		{
			f = fopen("/tmp/zzz", "w");
		}

		if (f && s && (l > 0))
		{
			(void) fwrite(s, 1, l, f);
		}
	}
#endif /* 0 */

	if (obj->cvtfunc != NULL)
		tobuf = (unsigned char *)obj->cvtfunc(obj, (unsigned char *)s, l);
	else
	{
		doc_csid = PeekMetaCharsetTag(obj, (char *)s, l) ;
	 
		/* Use 1st stream data block to guess doc Japanese CSID.	*/
		if (doc_csid == CS_ASCII)
		{
			obj->current_stream->put_block = (MKStreamWriteFunc) net_NoCharCodeConv;
			return((*obj->next_stream->put_block)(obj->next_stream->data_object,s,l));
		}

#ifndef XP_UNIX
		if (doc_csid == CS_DEFAULT)
			(void) INTL_GetCharCodeConverter(INTL_DefaultDocCharSetID(obj->current_stream->window_id), 0, obj);
		else
			(void) INTL_GetCharCodeConverter(doc_csid, 0, obj);
		obj->current_stream->window_id->win_csid = obj->to_csid;
		obj->current_stream->window_id->doc_csid = doc_csid;
#else /* !XP_UNIX */
		if (doc_csid == CS_DEFAULT)
			(void) INTL_GetCharCodeConverter(INTL_DefaultDocCharSetID(obj->current_stream->window_id),
			                       obj->current_stream->window_id->win_csid, obj);
		else
			(void) INTL_GetCharCodeConverter(doc_csid,
			                       obj->current_stream->window_id->win_csid, obj);
		obj->current_stream->window_id->win_csid = obj->to_csid;
		obj->current_stream->window_id->doc_csid = doc_csid;
#endif /* !XP_UNIX */

		/* If no conversion needed, change put_block module for successive
		 * data blocks.  For current data block, return unmodified buffer.
		 */
		if (obj->cvtfunc == NULL) 
		{
			obj->current_stream->put_block = (MKStreamWriteFunc) net_NoCharCodeConv;
			return((*obj->next_stream->put_block)(obj->next_stream->data_object,s,l));
		}

		/* For initial block, must call converter directly.  Success calls
		 * to the converter will be called directly from net_CharCodeConv()
		 */
	}

	if (tobuf == NULL)
		tobuf = (unsigned char *)obj->cvtfunc(obj, (unsigned char *)s, l);

	if (tobuf) {
		rv = (*(obj)->next_stream->put_block) (obj->next_stream->data_object,
			(const char *)tobuf, obj->len);
		if (tobuf != (unsigned char*)s)
			XP_FREE(tobuf);
    	return(rv);
	} else {
		return(obj->retval);
	}
}



	/* Auto Detect Japanese Char Code Conversion	*/
unsigned char *
autoJCCC (CCCDataObject *obj, const unsigned char *s, int32 l)
{
	int doc_csid = 0;
		/* Use 1st stream data block to guess doc Japanese CSID.	*/
	doc_csid = detect_JCSID (obj->current_stream->window_id,(const unsigned char *) s, l);
	if (doc_csid == CS_ASCII) {	/* return s unconverted and				*/
		obj->len = l;
		return (unsigned char *)s;				/* autodetect next block of stream data	*/
	}
	if (doc_csid == CS_DEFAULT) {
		doc_csid = INTL_DefaultDocCharSetID(obj->current_stream->window_id) & ~CS_AUTO;
		obj->current_stream->window_id->doc_csid = CS_DEFAULT;
	} else {
		obj->current_stream->window_id->doc_csid = (doc_csid | CS_AUTO);
	}
		/* Setup converter function for success streams data blocks	*/
	(void) INTL_GetCharCodeConverter(doc_csid, obj->to_csid, obj);

		/* If no conversion needed, change put_block module for successive
		 * data blocks.  For current data block, return unmodified buffer.
		 */
	if (obj->cvtfunc == NULL) {
		obj->current_stream->put_block = (MKStreamWriteFunc) net_NoCharCodeConv;
		obj->len = l;
		return((unsigned char *) s);
	}
		/* For initial block, must call converter directly.  Success calls
		 * to the converter will be called directly from net_CharCodeConv()
		 */
	return (unsigned char *)obj->cvtfunc (obj, (const unsigned char	*)s, l);
}

	/* One-byte-to-one-byte Char Code Conversion module.
	 * Table driven.  Table provided by FE.
	 */
int
net_1to1CCC (CCCDataObject *obj, const unsigned char *s, int32 l)
{
	(void) One2OneCCC (obj, s, l);
	return((*obj->next_stream->put_block)(obj->next_stream->data_object,
								(const char *)s, obj->len));
}

unsigned char *
One2OneCCC (CCCDataObject *obj, const unsigned char *s, int32 l)
{
	char **cvthdl;
	register unsigned char	*cp;
	char *pTable;

	cvthdl = (char **)INTL_GetSingleByteTable(obj->from_csid, obj->to_csid,obj->cvtflag);
	
	if (cvthdl != NULL)
	{
		pTable = INTL_LockTable(cvthdl);
		for (cp = (unsigned char *)s; cp < (unsigned char *)s + l; cp++)
			*cp = pTable[*cp];
	}
	obj->len = l;
	INTL_FreeSingleByteTable(cvthdl);

	return((unsigned char *)s);
}

/* is the stream ready for writing?
 */
PRIVATE unsigned int net_CvtCharCodeWriteReady (CCCDataObject * obj)
{
   return(MAX_WRITE_READY);  /* always ready for writing */
}

PRIVATE void net_CvtCharCodeComplete (CCCDataObject * obj)
{
	/* pass downstream any uncoverted characters */
	if (obj->uncvtbuf[0] != '\0')
		(*obj->next_stream->put_block)(obj->next_stream->data_object,
		(const char *)obj->uncvtbuf, strlen((char *)obj->uncvtbuf));

	(*obj->next_stream->complete)(obj->next_stream->data_object);

	XP_FREE(obj->next_stream);
    XP_FREE(obj);
    return;
}

PRIVATE void net_CvtCharCodeAbort (CCCDataObject * obj, int status)
{
	(*obj->next_stream->abort)(obj->next_stream->data_object, status);

	XP_FREE(obj->next_stream);
    XP_FREE(obj);

    return;
}

PUBLIC NET_StreamClass *
INTL_ConvCharCode (int         format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id)
{
    CCCDataObject	*obj;
    NET_StreamClass	*stream;
	register char	*cp1, *cp2;

    TRACEMSG(("Setting up display stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW_ZAP(NET_StreamClass);
    if(stream == NULL)
        return(NULL);

    stream->name           = "CharCodeConverter";
    stream->complete       = (MKStreamCompleteFunc) net_CvtCharCodeComplete;
    stream->abort          = (MKStreamAbortFunc) net_CvtCharCodeAbort;

    stream->is_write_ready = (MKStreamWriteReadyFunc) net_CvtCharCodeWriteReady;
    stream->window_id      = window_id;

    obj = XP_NEW_ZAP(CCCDataObject);
    if (obj == NULL)
        return(NULL);
    stream->data_object = obj;  /* document info object */
	obj->current_stream = stream;

		/* Legitimate charset names must be in ASCII and case insensitive	*/
	cp1 = URL_s->charset;
	if (cp1) 
	{
		cp2 = (char *)XP_ALLOC(XP_STRLEN((char *)cp1)+1);
		if (cp2 == NULL)
		{
			window_id->mime_charset = NULL;
			window_id->doc_csid = INTL_CharSetNameToID(URL_s->charset);
		}
		else
		{
			window_id->mime_charset = cp2;
			while (*cp1 != '\0') {
				*cp2 = tolower(*cp1);
				cp1++; cp2++;
    		}
    		*cp2 = '\0';
			window_id->doc_csid = INTL_CharSetNameToID(window_id->mime_charset);
		}
		/*
		 * if this is an unknown charset, set doc_csid to CS_DEFAULT
		 * so that the user can override using charset menu -- erik
		 */
		if (window_id->doc_csid == CS_UNKNOWN)
		{
			window_id->doc_csid = CS_DEFAULT;
		}
/* for debugging -- erik */
#if 0
		{
			extern FILE *real_stderr;

			(void) fprintf(real_stderr, "url charset %s\n",
				window_id->mime_charset);
		}
#endif /* 0 */
	} 
	else if (window_id->relayout == METACHARSET_FORCERELAYOUT && window_id->doc_csid != CS_DEFAULT)
	{
		window_id->relayout = METACHARSET_RELAYOUTDONE;
	}
	else
	{
		window_id->relayout = METACHARSET_NONE;
		window_id->mime_charset = NULL;
#ifdef XP_UNIX
		/*
		 * The PostScript and Text FEs inherit the doc_csid from
		 * the parent context in order to pick up the per-window
		 * default. Apparently, this already works for some
		 * reason on Windows. Don't know about Mac. -- erik
		 */
		if ((window_id->type != MWContextPostScript) &&
			(window_id->type != MWContextText))
#endif /* XP_UNIX */
		window_id->doc_csid = CS_DEFAULT;
	}

#ifdef XP_UNIX
	{
#ifdef OLD_CODE
		extern void XFE_getlocale (int16 charset, MWContext *context);
		XFE_getlocale(0, window_id);
		window_id->win_csid = INTL_DefaultWinCharSetID();
#else /* OLD_CODE */
		window_id->win_csid = INTL_DocToWinCharSetID(window_id->doc_csid ?
			window_id->doc_csid : INTL_DefaultDocCharSetID(window_id));
#endif /* OLD_CODE */
	}
#endif /* XP_UNIX */

	if (window_id->doc_csid == CS_DEFAULT || window_id->doc_csid == CS_UNKNOWN)
	{
		stream->put_block	= (MKStreamWriteFunc) net_AutoCharCodeConv;
	}
	else
	{
#ifndef XP_UNIX
		(void) INTL_GetCharCodeConverter(window_id->doc_csid, 0, obj);
		window_id->win_csid = obj->to_csid;
#else /* !XP_UNIX */
		(void) INTL_GetCharCodeConverter(window_id->doc_csid,
											window_id->win_csid, obj);
		window_id->win_csid = obj->to_csid;
#endif /* !XP_UNIX */

		if (obj->cvtfunc == NULL)
			stream->put_block	= (MKStreamWriteFunc) net_NoCharCodeConv;
    	else if (obj->cvtfunc == (CCCFunc)One2OneCCC)
    		stream->put_block	= (MKStreamWriteFunc) net_1to1CCC;
    	else
    		stream->put_block	= (MKStreamWriteFunc) net_CharCodeConv;
	}


    TRACEMSG(("Returning stream from NET_CvtCharCodeConverter\n"));
	
	/* remap content type to be to INTERNAL_PARSER
 	 */
	StrAllocCopy(URL_s->content_type, INTERNAL_PARSER);
	obj->next_stream = NET_StreamBuilder(format_out, URL_s, window_id);

    if(!obj->next_stream)
	  {
		XP_FREE(obj);
		XP_FREE(stream);
		return(NULL);
	  }

    return stream;
}

/* 
 INTL_ConvWinToMailCharCode
   Converts 8bit encoding to 7bit mail encoding. It decides which 7bit and 8bit encoding 
   to use based on current default language.
 input:
   char *pSrc;  			// Source display buffer
 output:
   char *pDest; 			//  Destination buffer
                            // pDest == NULL means either conversion fail
                            // or does OneToOne conversion
*/
PUBLIC
unsigned char *INTL_ConvWinToMailCharCode(MWContext *context, unsigned char *pSrc, uint32 block_size)
{
    CCCDataObject	*obj;
	unsigned char *pDest;
	int16 wincsid;

    obj = XP_NEW_ZAP(CCCDataObject);
	if (obj == NULL)
		return NULL;

	wincsid =  INTL_DefaultWinCharSetID(context);
	/* Converts 8bit Window codeset to 7bit Mail Codeset.   */
	(void) INTL_GetCharCodeConverter(wincsid, 
	     							 INTL_DefaultMailCharSetID(wincsid),
	     							 obj);

	if (obj->cvtfunc)
		pDest = (unsigned char *)obj->cvtfunc(obj, pSrc, block_size);
	else
		pDest = NULL ;

	XP_FREE(obj);

	if (pDest == pSrc)  /* converted to input buffer           */
	{                           /* no additional memory been allocated */
		return NULL;
	}
	return pDest ;
}

/* 
 INTL_ConvMailToWinCharCode
   Converts mail encoding to display charset which is used by current window. 
   It decides which Display charset to use based on current default language.
 input:
   MWContext *context;     the context (window ID)
   char *pSrc;  		// Source buffer
   uint32 block_size;      the length of the source buffer
 output:
   char *pDest; 		//  Destination buffer
                        // pDest == NULL means either conversion fail
                        // or does OneToOne conversion
*/
PUBLIC
unsigned char *INTL_ConvMailToWinCharCode(MWContext *context,
	unsigned char *pSrc, uint32 block_size)
{
    CCCDataObject	*obj;
	unsigned char *pDest;
	NET_StreamClass stream;

    obj = XP_NEW_ZAP(CCCDataObject);
	if (obj == NULL)
		return 0;

	(void) memset(&stream, 0, sizeof(stream));
	stream.window_id = context;
	obj->current_stream = &stream;

	/* Converts 7bit news codeset to 8bit windows Codeset.   */
	(void) INTL_GetCharCodeConverter(INTL_DefaultDocCharSetID(context), 0, obj);

	if (obj->cvtfunc)
		pDest = (unsigned char *)obj->cvtfunc(obj, pSrc, block_size);
	else
		pDest = NULL ;

	XP_FREE(obj);
	if (pSrc == pDest)
		return NULL ;
	return pDest ;
}
PUBLIC
CCCDataObject *
INTL_CreateDocToMailConverter(MWContext *context, XP_Bool isHTML, unsigned char *buffer, uint32 buffer_size)
{
      CCCDataObject* selfObj;
      int16 p_doc_csid = CS_DEFAULT;
      /* Ok!! let's create the object here
         We need the conversion obj to keep the state information */
      if((selfObj = XP_NEW_ZAP(CCCDataObject))==NULL)
              return NULL;

              /* First, let's determine the from_csid and to_csid */
              /* It is a TEXT_HTML !!! Let's use our PeekMetaCharsetTag to get a csid */
      if(isHTML)
               p_doc_csid = PeekMetaCharsetTag(selfObj, (char*) buffer, buffer_size);

      if(p_doc_csid == CS_DEFAULT)
      {
              /* got default, try to get the doc_csid from context */
              if((context == 0 ) || (context->doc_csid == CS_DEFAULT))
                      p_doc_csid = INTL_DefaultDocCharSetID(context);
              else
                      p_doc_csid = context->doc_csid;

              /* The doc_csid from the context (or default) has CS_AUTO bit. */
              /* So let's try to call the auto detection function */
              if(p_doc_csid & CS_AUTO)
                      p_doc_csid = detect_JCSID (context, buffer, buffer_size);
      }
      /* Now, we get the converter */
      (void) INTL_GetCharCodeConverter(p_doc_csid,
                                       INTL_DefaultMailCharSetID(p_doc_csid),
                                        selfObj);
      /* If the cvtfunc == NULL, we don't need to do conversion */
      if(! (selfObj->cvtfunc) )
      {
      		  XP_FREE(selfObj);
              return NULL;
      }
      else
              return selfObj;
}


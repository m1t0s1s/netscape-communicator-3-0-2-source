/*  -*- Mode: C; tab-width: 4 -*-
 *  Do stream related stuff like push data down the
 *  stream and determine which converter to use to
 *  set up the stream.
 */
#include "mkutils.h"
#include "mkstream.h"
#include "mkgeturl.h"
#include "mktcp.h"

#include "cvview.h"

#ifdef XP_UNIX
#include "cvextcon.h"
#endif


/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_ALERT_UNRECOGNIZED_ENCODING;
extern int XP_ALERT_CANTFIND_CONVERTER;


#ifdef PROFILE
#pragma profile on
#endif

typedef struct _net_ConverterStruct {
     NET_Converter * converter;        /* the converter function */
     char          * format_in;        /* the input (mime) type that the 
										* function accepts 
										*/
#ifdef NEW_DECODERS
     char          * encoding_in;      /* the input content-encoding that the
										* function accepts, or 0 for `any'.
										*/
#endif /* NEW_DECODERS */
     FO_Present_Types format_out;      /* the output type of the function */
     void          * data_obj;         /* a converter specific data object 
										* passed in at the
                                        * time of registration 
										*/
} net_ConverterStruct;

PRIVATE XP_List * net_converter_list=0;  /* a list of converters */
PRIVATE XP_List * net_decoder_list=0;    /* a list of decoders */

#define NET_MIME_EXACT_MATCH    1
#define NET_MIME_PARTIAL_MATCH  2
#define NET_WILDCARD_MATCH      3

/* will compare two mime times and return
 * NET_MIME_EXACT_MATCH if they match exactly
 * or NET_MIME_PARTIAL_MATCH if they match due
 * to a wildcard entry (image.*, or movie.*).
 *
 * if 'partial' equals '*' then
 * NET_WILDCARD_MATCH is returned
 * 
 * If there is no match then 0(zero) is returned
 */
PRIVATE int
net_compare_mime_types(char * absolute, char * partial)
{
   char *star;

   TRACEMSG(("StreamBuilder: Comparing %s and %s\n",absolute, partial));

   if(!strcasecomp(absolute, partial))
	  return(NET_MIME_EXACT_MATCH);

   if(!XP_STRCMP(partial, "*"))
	  return(NET_WILDCARD_MATCH);

   if((star = XP_STRCHR(partial, '*')) == 0)
	  return(0);  /* not a wildcard mime type */

   /* compare just the part before the slash star
    *
    */
   if(!strncasecomp(absolute, partial, (star-1)-partial))
	  return(NET_MIME_PARTIAL_MATCH);

   return(0); /* no match */
}

/* Find a converter routine to create a stream and return the stream struct
*/
PUBLIC NET_StreamClass * 
NET_StreamBuilder  (FO_Present_Types format_out,
                    URL_Struct  *URL_s,
                    MWContext   *context)
{
    net_ConverterStruct * cs_ptr;
    XP_List * ct_list_ptr = net_converter_list;
    XP_List * ce_list_ptr = net_decoder_list;

	assert (URL_s->content_type);
	
    TRACEMSG(("Entering StreamBuilder:\n\
     F-in: %s\n\
    F-out: %d\n\
      Enc: %s\n",URL_s->content_type,format_out, 
    (URL_s->content_encoding ? URL_s->content_encoding : "None Specified")));

#if 0
	/* First, determine whether there is a content-type converter for this
	   document; if there isn't one, then the only way to handle it is to
	   send it straight to disk, so try that.
	 */
	if (format_out != FO_SAVE_TO_DISK)
	  {
		while ((cs_ptr=(net_ConverterStruct *) XP_ListNextObject(ct_list_ptr))
			   != 0)
		  {
			if (format_out == cs_ptr->format_out &&
				net_compare_mime_types (URL_s->content_type,
										cs_ptr->format_in))
			  break;
		  }

		if (! cs_ptr)  /* There is no content-type converter; save it. */
		  format_out = FO_SAVE_TO_DISK;
		ct_list_ptr = net_converter_list;  /* Reset for next traversal. */
	  }
#endif


    /* if there are content encodings then
     * execute decoders first
     */
    /* go through list of converters and choose
     * the first exact or partial match we
     * find.  The order of the list
     * is guarenteed by the registration
     * routines
	 *
#ifdef NEW_DECODERS
	 * the format-out is compared as well.
#else
	 * Note that format-out is not consulting/compared when looking up
	 * encoding converters.
#endif
     */
	if (URL_s->content_encoding && *URL_s->content_encoding)
	  {
		while ((cs_ptr=(net_ConverterStruct *) XP_ListNextObject(ce_list_ptr))
			   != 0)
		  {
#ifdef NEW_DECODERS
			if (format_out == cs_ptr->format_out &&
				net_compare_mime_types (URL_s->content_type,
										cs_ptr->format_in) &&
				net_compare_mime_types (URL_s->content_encoding,
										cs_ptr->encoding_in))
#else /* !NEW_DECODERS */
			  /* the old way */
			if (net_compare_mime_types (URL_s->content_encoding,
										cs_ptr->format_in))
#endif /* !NEW_DECODERS */
			  {
				return ((NET_StreamClass *)
						((*cs_ptr->converter)
						 (format_out, cs_ptr->data_obj, URL_s, context)));
			  }
		  }

#ifndef NEW_DECODERS
		/* #### This clause is getting tripped by running FO_PRESENT
		   on foo.tar.Z - in that case, the URL has an encoding, but we
		   shoudn't match it.  Really the test should be, "is this encoding
		   known", not "did this encoding match."  Fix this! */


		/* If we get here, then this document has a content-encoding that
		   we don't have a decoder for.  If it's not one of the encodings
		   that we *know* we don't need a decoder for, then this is likely
		   to be a problem, so warn the user.  (But only in FO_PRESENT -
		   it's safe to save unknown encodings to disk, etc.)
		 */
		if (format_out == FO_PRESENT &&
			strcasecomp (URL_s->content_encoding, ENCODING_7BIT) &&
			strcasecomp (URL_s->content_encoding, ENCODING_8BIT) &&
			strcasecomp (URL_s->content_encoding, ENCODING_BINARY))
		  {
			char *msg = (char *) XP_ALLOC (strlen (URL_s->content_encoding)
										   + 100);
			if (msg)
			  {
				XP_STRCPY (msg, XP_GetString(XP_ALERT_UNRECOGNIZED_ENCODING));
				XP_STRCAT (msg, URL_s->content_encoding);
				XP_STRCAT (msg, "'.");
				FE_Alert (context, msg);
				FREE (msg);
			  }

			/* Now throw away the encoding so that we don't warn twice. */
			FREE (URL_s->content_encoding);
			URL_s->content_encoding = 0;
		  }
#endif /* !NEW_DECODERS */
	  }

	/* now search for content-type converters
     */
    /* go through list of converters and choose
     * the first exact or partial match we
     * find.  The order of the list
     * is guarenteed by the registration
     * routines
     */
    while((cs_ptr=(net_ConverterStruct *) XP_ListNextObject(ct_list_ptr)) != 0)
      {
        if(format_out == cs_ptr->format_out)
            if(net_compare_mime_types(URL_s->content_type, cs_ptr->format_in))
              {
				return( (NET_StreamClass *) (*cs_ptr->converter) (format_out, 
                                             	cs_ptr->data_obj, URL_s, context));
	          }
      }

    TRACEMSG(("Alert! did not find a converter or decoder\n"));
    FE_Alert(context, XP_GetString(XP_ALERT_CANTFIND_CONVERTER));

    return(0);  /* Uh-oh. didn't find a converter.  VERY VERY BAD! */
}

/* prints out all presentation mime types during successive calls
 * call with first equal true to reset to the beginning
 * and with first equal false to print out the next
 * converter in the list.  Returns zero when all converters
 * are printed out.
 */
MODULE_PRIVATE char *
XP_ListNextPresentationType(Bool first)
{
    static XP_List * converter_list_ptr;
    net_ConverterStruct * converter;

    /* reset list pointers 
     */
    if(first)
        converter_list_ptr = net_converter_list;

    /* print out next converter if there are any more
     */
    converter = (net_ConverterStruct *) XP_ListNextObject(converter_list_ptr);

    if(converter && converter->format_out == FO_PRESENT)
	return(converter->format_in);

    /* else */
    return(NULL);
}
	
/* prints out all encoding mime types during successive calls
 * call with first equal true to reset to the beginning
 * and with first equal false to print out the next
 * converter in the list.  Returns zero when all converters
 * are printed out.
 */
MODULE_PRIVATE char *
XP_ListNextEncodingType(Bool first)
{
    static XP_List * decoder_list_ptr;
    net_ConverterStruct * converter;

    /* reset list pointers
     */
    if(first)
      {
        decoder_list_ptr = net_decoder_list;
      }

    converter = (net_ConverterStruct *) XP_ListNextObject(decoder_list_ptr);

    if(converter)
        return(converter->format_in);

    /* else */
    return(NULL);
}
      
PRIVATE void
net_RegisterGenericConverterOrDecoder(XP_List        ** conv_list,
							  		  char            * format_in,
#ifdef NEW_DECODERS
							  		  char            * encoding_in,
#endif /* NEW_DECODERS */
                              		  FO_Present_Types  format_out,
                              		  void            * data_obj,
                              		  NET_Converter   * converter_func)
{
    XP_List * list_ptr = *conv_list;
    net_ConverterStruct * converter_struct_ptr;
    net_ConverterStruct * new_converter_struct;
    int f_in_type;

    /* check for an existing converter with the same format_in and format_out 
     * and replace it if found.
     *
     * if the list is NULL XP_ListNextObject will return 0
     */
    while((converter_struct_ptr = (net_ConverterStruct *) XP_ListNextObject(list_ptr)) != 0)
      {
	    if(format_out == converter_struct_ptr->format_out)
	        if(!strcasecomp(format_in, converter_struct_ptr->format_in)
#ifdef NEW_DECODERS
			   && ((!encoding_in && !converter_struct_ptr->encoding_in) ||
				   (encoding_in && converter_struct_ptr->encoding_in &&
					!XP_STRCMP (encoding_in,
								converter_struct_ptr->encoding_in)))
#endif /* NEW_DECODERS */
			   )
	          {
		        /* replace this converter entry with
		         * the one passed in
		         */
		        converter_struct_ptr->data_obj = data_obj;
		        converter_struct_ptr->converter = converter_func;
		        return;
	          }
       }

    /* find out the type of entry format_in is
     */
    f_in_type = NET_MIME_EXACT_MATCH;
        if(XP_STRCHR(format_in, '*'))
      {
	    if(!XP_STRCMP(format_in, "*"))
	        f_in_type = NET_WILDCARD_MATCH;
	    else
	        f_in_type = NET_MIME_PARTIAL_MATCH;
      }

    /* if existing converter not found/replaced
     * add this converter to the list in
     * its order of importance so that we can
     * take the first exact or partial fit
     */ 
    new_converter_struct = XP_NEW(net_ConverterStruct);
    if(!new_converter_struct)
	    return;

    new_converter_struct->format_in = 0;
    StrAllocCopy(new_converter_struct->format_in, format_in);
#ifdef NEW_DECODERS
    new_converter_struct->encoding_in = (encoding_in
										 ? XP_STRDUP (encoding_in) : 0);
#endif /* NEW_DECODERS */
    new_converter_struct->format_out = format_out;
    new_converter_struct->converter = converter_func;
    new_converter_struct->data_obj = data_obj;

    if(!*conv_list)
      {
	    /* create the list and add this object
	     * don't worry about order since there
	     * can't be any 
  	     */
	    *conv_list = XP_ListNew();
	    if(!*conv_list)
	        return; /* big error */

 	    XP_ListAddObject(*conv_list, new_converter_struct);
	    return;
      }

    /* if it is an exact match type 
     * then we can just add it to the beginning
     * of the list
     */
    if(f_in_type == NET_MIME_EXACT_MATCH)
      {
	    XP_ListAddObject(*conv_list, new_converter_struct);
	    TRACEMSG(("Adding Converter to Beginning of list"));
      }
    else if(f_in_type == NET_MIME_PARTIAL_MATCH)
      {
	    /* this is a partial wildcard of the form (image/(star))
         * store it at the end of all the exact match pairs
         */
        list_ptr = *conv_list;
        while((converter_struct_ptr = (net_ConverterStruct *) XP_ListNextObject(list_ptr)) != 0)
          {
            if(XP_STRCHR(converter_struct_ptr->format_in, '*'))
	          {
		        XP_ListInsertObject (*conv_list, 
									 converter_struct_ptr, 
									 new_converter_struct); 
		        return;
              }
          }
		/* else no * objects in list */
	    XP_ListAddObjectToEnd(*conv_list, new_converter_struct);
	    TRACEMSG(("Adding Converter to Middle of list"));
      }
    else
      {
	    /* this is a very wild wildcard match of the form "*" only.
         * It has the least precedence so store it at the end
	     * of the list.  (There must always be at least one
	     * item in the list (see above))
	     */
	    XP_ListAddObjectToEnd(*conv_list, new_converter_struct);
	    TRACEMSG(("Adding Converter to end of list"));
      }

    return;  /* end */
}

#ifdef NEW_DECODERS
struct net_encoding_converter
{
  char *encoding_in;
  void *data_obj;
  NET_Converter * converter_func;
};

static struct net_encoding_converter *net_all_encoding_converters;
static int net_encoding_converter_size = 0;
static int net_encoding_converter_fp = 0;

#endif /* NEW_DECODERS */

/* register a routine to decode a stream
 */
PUBLIC void
NET_RegisterEncodingConverter(char *encoding_in,
                              void          * data_obj,
                              NET_Converter * converter_func)
{

#ifdef NEW_DECODERS
	/* Don't register anything right now, but remember that we have a
	   decoder for this so that we can register it hither and yon at
	   a later date.
	 */
  if (net_encoding_converter_size <= net_encoding_converter_fp)
	{
	  net_encoding_converter_size += 10;
	  if (net_all_encoding_converters)
		net_all_encoding_converters = (struct net_encoding_converter *)
		  XP_REALLOC (net_all_encoding_converters,
					  net_encoding_converter_size
					  * sizeof (*net_all_encoding_converters));
	  else
		net_all_encoding_converters = (struct net_encoding_converter *)
		  XP_CALLOC (net_encoding_converter_size,
					 sizeof (*net_all_encoding_converters));
	}

  net_all_encoding_converters [net_encoding_converter_fp].encoding_in
	= XP_STRDUP (encoding_in);
  net_all_encoding_converters [net_encoding_converter_fp].data_obj
	= data_obj;
  net_all_encoding_converters [net_encoding_converter_fp].converter_func
	= converter_func;
  net_encoding_converter_fp++;

#else /* ! NEW_DECODERS */

  /* the old way */
    net_RegisterGenericConverterOrDecoder(&net_decoder_list,
                                          encoding_in,
                                          0, /* format_out is unused */
                                          data_obj,
                                          converter_func);

#endif /* !NEW_DECODERS */
}


#ifdef MOZILLA_CLIENT

#ifdef NEW_DECODERS
void
NET_RegisterAllEncodingConverters (char *format_in,
								   FO_Present_Types format_out)
{
  int i;

#ifdef XP_UNIX
  /* On Unix at least, there should be *some* registered... */
  assert (net_encoding_converter_fp > 0);
#endif

  for (i = 0; i < net_encoding_converter_fp; i++)
	{
	  char *encoding_in = net_all_encoding_converters [i].encoding_in;
	  void *data_obj = net_all_encoding_converters [i].data_obj;
	  NET_Converter *func = net_all_encoding_converters [i].converter_func;

	  net_RegisterGenericConverterOrDecoder (&net_decoder_list,
											 format_in,
											 encoding_in,
											 format_out,
											 data_obj,
											 func);

	  net_RegisterGenericConverterOrDecoder (&net_decoder_list,
											 format_in,
											 encoding_in,
											 format_out | FO_CACHE_ONLY,
											 data_obj,
											 func);

	  net_RegisterGenericConverterOrDecoder (&net_decoder_list,
											 format_in,
											 encoding_in,
											 format_out | FO_ONLY_FROM_CACHE,
											 data_obj,
											 func);
	}
}

#endif /* NEW_DECODERS */
#endif /* MOZILLA_CLIENT */




/*  Register a routine to convert between format_in and format_out
 *
 */
#if defined(XP_WIN) && defined(MOZILLA_CLIENT)
/*  Reroute XP handling of streams for windows.
 */
PUBLIC void
NET_RegisterContentTypeConverter (char          * format_in,
                                  FO_Present_Types format_out,
                                  void          * data_obj,
                                  NET_Converter * converter_func)
{
    PMS_RegisterContentTypeConverter(format_in, format_out,
        data_obj, converter_func, FALSE);
}

PUBLIC void
RealNET_RegisterContentTypeConverter (char          * format_in,
                                  FO_Present_Types format_out,
                                  void          * data_obj,
                                  NET_Converter * converter_func)
#else
PUBLIC void
NET_RegisterContentTypeConverter (char          * format_in,
                                  FO_Present_Types format_out,
                                  void          * data_obj,
                                  NET_Converter * converter_func)
#endif /* XP_WIN */
{
    net_RegisterGenericConverterOrDecoder(&net_converter_list,
                                          format_in,
#ifdef NEW_DECODERS
										  0,
#endif /* NEW_DECODERS */
                                          format_out,
                                          data_obj,
                                          converter_func);

}

#ifdef MOZILLA_CLIENT
#ifdef XP_UNIX

/* register a mime type and a command to be executed
 *
 * if stream_block_size is zero then the data will be completely
 * pooled and the external viewer called with the filename.
 * if stream_block_size is greater than zero the viewer will be
 * called with a popen and the stream_block_size will be used as
 * the maximum size of each buffer to pass to the viewer
 */
MODULE_PRIVATE void
NET_RegisterExternalViewerCommand(char * format_in, 
								  char * system_command, 
								  unsigned int stream_block_size) 
{
    CV_ExtViewStruct * new_obj = XP_NEW(CV_ExtViewStruct);

    if(!new_obj)
	   return;

    XP_MEMSET(new_obj, 0, sizeof(CV_ExtViewStruct));

    /* make a new copy of the command so it can be passed
     * as the data object
     */
    StrAllocCopy(new_obj->system_command, system_command);
    new_obj->stream_block_size = stream_block_size;

    /* register the routine
     */
    NET_RegisterContentTypeConverter(format_in, FO_PRESENT, new_obj, NET_ExtViewerConverter);

#ifdef NEW_DECODERS
	/* Also register the content-encoding converters on this type.
	   Those get registered only on types we know how to display,
	   either internally or externally - and don't get registered
	   on types we know nothing about.
	 */
	NET_RegisterAllEncodingConverters (format_in, FO_PRESENT);
#endif
}

/* removes all external viewer commands
 */
PUBLIC void
NET_ClearExternalViewerConverters(void)
{
    XP_List * list_ptr = net_converter_list;
    net_ConverterStruct * converter_struct_ptr;


	while((converter_struct_ptr = 
						(net_ConverterStruct *) XP_ListNextObject(list_ptr)))
	  {
		if(converter_struct_ptr->converter == NET_ExtViewerConverter)
		  {
			list_ptr = net_converter_list; /* reset to beginning */
			XP_ListRemoveObject(list_ptr, converter_struct_ptr);
			
			FREE(converter_struct_ptr->data_obj);
			FREE(converter_struct_ptr->format_in);
#ifdef NEW_DECODERS
			FREE(converter_struct_ptr->encoding_in);
#endif /* NEW_DECODERS */
			FREE(converter_struct_ptr);
		  }
	  }
}

#ifdef NEW_DECODERS
/* removes all registered converters of any kind
 */
PUBLIC void
NET_ClearAllConverters(void)
{
    net_ConverterStruct * converter_struct_ptr;
    XP_List * list_ptr;

	list_ptr = net_converter_list;
	while((converter_struct_ptr = 
						(net_ConverterStruct *) XP_ListNextObject(list_ptr)))
	  {
		list_ptr = net_converter_list; /* reset to beginning */
		XP_ListRemoveObject(list_ptr, converter_struct_ptr);
		FREEIF(converter_struct_ptr->format_in);
		FREEIF(converter_struct_ptr->encoding_in);
# ifdef XP_UNIX
		/* total kludge!! */
		if(converter_struct_ptr->converter == NET_ExtViewerConverter)
		  FREEIF(converter_struct_ptr->data_obj);
# endif /* XP_UNIX */
		FREE(converter_struct_ptr);
	  }

	list_ptr = net_decoder_list;
	while((converter_struct_ptr = 
						(net_ConverterStruct *) XP_ListNextObject(list_ptr)))
	  {
		list_ptr = net_decoder_list; /* reset to beginning */
		XP_ListRemoveObject(list_ptr, converter_struct_ptr);
		FREEIF(converter_struct_ptr->format_in);
		FREEIF(converter_struct_ptr->encoding_in);
# ifdef XP_UNIX
		/* total kludge!! */
		if(converter_struct_ptr->converter == NET_ExtViewerConverter)
		  FREEIF(converter_struct_ptr->data_obj);
# endif /* XP_UNIX */
		FREE(converter_struct_ptr);
	  }
}
#endif /* NEW_DECODERS */


/* register an external program to act as a content-type
 * converter
 */
MODULE_PRIVATE void
NET_RegisterExternalConverterCommand(char * format_in,
									 int    format_out,
                                     char * system_command,
                                     char * new_format)
{
    CV_ExtConverterStruct * new_obj = XP_NEW(CV_ExtConverterStruct);

    if(!new_obj)
       return;

    memset(new_obj, 0, sizeof(CV_ExtConverterStruct));

    /* make a new copy of the command so it can be passed
     * as the data object
     */
    StrAllocCopy(new_obj->command,    system_command);
    StrAllocCopy(new_obj->new_format, new_format);
    new_obj->is_encoding_converter = FALSE;

    /* register the routine
     */
    NET_RegisterContentTypeConverter(format_in, format_out, new_obj, NET_ExtConverterConverter);
}

/* register an external program to act as a content-encoding
 * converter
 */
PUBLIC void
NET_RegisterExternalDecoderCommand (char * format_in,
									char * format_out,
									char * system_command)
{
    CV_ExtConverterStruct * new_obj = XP_NEW(CV_ExtConverterStruct);

    if(!new_obj)
       return;

    memset(new_obj, 0, sizeof(CV_ExtConverterStruct));

    /* make a new copy of the command so it can be passed
     * as the data object
     */
    StrAllocCopy(new_obj->command,    system_command);
    StrAllocCopy(new_obj->new_format, format_out);
    new_obj->is_encoding_converter = TRUE;

    /* register the routine
     */
    NET_RegisterEncodingConverter (format_in, new_obj,
								   NET_ExtConverterConverter);
}


#endif /* XP_UNIX */
#endif /* MOZILLA_CLIENT */

/*  NewMKStream
 *  Utility to create a new initialized NET_StreamClass object
 */
NET_StreamClass *
NET_NewStream          (char                 *name,
                        MKStreamWriteFunc     process,
                        MKStreamCompleteFunc  complete,
                        MKStreamAbortFunc     abort,
                        MKStreamWriteReadyFunc ready,
                        void                 *streamData,
                        MWContext            *windowID)
{
    NET_StreamClass *stream = XP_NEW (NET_StreamClass);
    if (!stream)
        return nil;
        
    stream->name        = name;
    stream->window_id   = windowID;
    stream->data_object = streamData;
    stream->put_block   = process;
    stream->complete    = complete;
    stream->abort       = abort;
    stream->is_write_ready = ready;
    
    return stream;
}

#ifdef PROFILE
#pragma profile off
#endif


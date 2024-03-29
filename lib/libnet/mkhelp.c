/* -*- Mode: C; tab-width: 4 -*- */
/***************************************************
 *
 * Routines to parse HTML help mapping file
 */

#include "mkutils.h"
#include "xp.h"
#include "mkparse.h"

extern int MK_OUT_OF_MEMORY;

#define DEFAULT_HELP_ID			   "DEFAULT"
#define DEFAULT_HELP_WINDOW_NAME   "_HelpWindow"
#define DEFAULT_HELP_WINDOW_HEIGHT 500
#define DEFAULT_HELP_WINDOW_WIDTH  400

/* @@@ Global hack.  See below */
PRIVATE URL_Struct * frame_content_for_pre_exit_routine=NULL;

typedef struct _HTMLHelpParseObj {
	XP_Bool  in_id_mapping;
	int32	 window_width;
	int32	 window_height;
	char	*window_name;
	char    *id_value;
	char    *default_id_value;
	char    *url_to_map_file;
	char    *id;
	char    *line_buffer;
	int32    line_buffer_size;
	char    *content_target;
	XP_List *frame_group_stack;
} HTMLHelpParseObj;

typedef struct {
	char *address;
	char *target;
} frame_set_struct;

PRIVATE void
simple_exit(URL_Struct *URL_s, int status, MWContext *window_id)
{
	if(status != MK_CHANGING_CONTEXT)
		NET_FreeURLStruct(URL_s);
}

PRIVATE void
net_help_free_frame_group_struct(frame_set_struct *obj)
{
	XP_ASSERT(obj);
	if(!obj)
		return;
	FREEIF(obj->address);
	FREEIF(obj->target);
}

/* load the HTML help mapping file and search for
 * the id or text to load a specific document
 */
PUBLIC void
NET_GetHTMLHelpFileFromMapFile(MWContext *context,
							   char *map_file_url,
							   char *id,
							   char *search_text)
{
	URL_Struct *URL_s;

	XP_ASSERT(map_file_url && id);

	if(!map_file_url || !id)
		return;

	URL_s = NET_CreateURLStruct(map_file_url, NET_DONT_RELOAD);

	if(!URL_s)
		return;

	URL_s->fe_data = XP_STRDUP(id);

	NET_GetURL(URL_s, FO_CACHE_AND_LOAD_HTML_HELP_MAP_FILE, context, simple_exit);
}

PUBLIC HTMLHelpParseObj *
NET_ParseHTMLHelpInit(char *url_to_map_file, char *id)
{
	HTMLHelpParseObj *rv = XP_NEW(HTMLHelpParseObj);

	if(!rv)
		return(NULL);

	XP_ASSERT(url_to_map_file && id);

	if(!url_to_map_file || !id)
		return(NULL);

	XP_MEMSET(rv, 0, sizeof(HTMLHelpParseObj));

	rv->url_to_map_file = XP_STRDUP(url_to_map_file);
	rv->id              = XP_STRDUP(id);

	rv->window_height = DEFAULT_HELP_WINDOW_HEIGHT;
	rv->window_width  = DEFAULT_HELP_WINDOW_WIDTH;

	rv->frame_group_stack = XP_ListNew();

	return(rv);
}

PUBLIC void
NET_ParseHTMLHelpFree(HTMLHelpParseObj * obj)
{
	XP_ASSERT(obj);

	if(!obj)
		return;

	FREEIF(obj->window_name);
	FREEIF(obj->id_value);
	FREEIF(obj->default_id_value);
	FREEIF(obj->url_to_map_file);
	FREEIF(obj->line_buffer);
	FREEIF(obj->id);

	if(obj->frame_group_stack)
	  {
		frame_set_struct * frame_group_ptr;

		while((frame_group_ptr = XP_ListRemoveTopObject(obj->frame_group_stack)))
			net_help_free_frame_group_struct(frame_group_ptr);

		FREE(obj->frame_group_stack);
	  }

	FREE(obj);
}

/* parse a line that looks like 
 * [whitespace]=[whitespace]"token" other stuff...
 *
 * return token or NULL.
 */
PRIVATE char *
net_get_html_help_token(char *line_data, char**next_word)
{
	char * line = line_data;
	char * cp;

	if(next_word)
		*next_word = NULL;
	
	while(isspace(*line)) line++;
	
	if(*line != '=')
		return(NULL);

	line++; /* go past '=' */

	while(isspace(*line)) line++;

	if(*line != '"')
		return(NULL);

	line++; /* go past '"' */

	for(cp=line; *cp; cp++)
	  	if(*cp == '"' && *(cp-1) != '\\')
		  {
			*cp = '\0';
			if(next_word)
			  {
				*next_word = cp+1;
				while(isspace(*(*next_word))) (*next_word)++;
			  }
			break;
		  }

	return(line);
}

/* parse lines in an HTML help mapping file.
 * get window_size and name, etc...
 *
 * when the id is found function returns HTML_HELP_ID_FOUND
 * on error function returns negative error code.
 */
PRIVATE int
net_ParseHTMLHelpLine(HTMLHelpParseObj *obj, char *line_data)
{
	char *line = XP_StripLine(line_data);
	char *token;
	char *next_word;

	if(*line == '<')
	  {
		/* find and terminate the end '>' */
		XP_STRTOK(line, ">");

		token = XP_StripLine(line+1);

#define ID_MAP_TOKEN "ID_MAP"
		if(!strncasecomp(token, 
						 ID_MAP_TOKEN, 
						 sizeof(ID_MAP_TOKEN)-1))
		  {
			obj->in_id_mapping = TRUE;
		  }
#define END_ID_MAP_TOKEN "/ID_MAP"
		else if(!strncasecomp(token, 
						 END_ID_MAP_TOKEN, 
						 sizeof(END_ID_MAP_TOKEN)-1))
		  {
			obj->in_id_mapping = FALSE;
		  }
#define FRAME_GROUP_TOKEN "FRAME_GROUP"
		else if(!strncasecomp(token, 
						 FRAME_GROUP_TOKEN, 
						 sizeof(FRAME_GROUP_TOKEN)-1))
		  {
			char *cp = token + sizeof(FRAME_GROUP_TOKEN)-1;
			frame_set_struct * fgs = XP_NEW(frame_set_struct);

			while(isspace(*cp)) cp++;

			if(fgs)
			  {
				XP_MEMSET(fgs, 0, sizeof(frame_set_struct));

				next_word=NULL; /* init */

				do {
#define SRC_TOKEN "SRC"
					if(!strncasecomp(cp, SRC_TOKEN, sizeof(SRC_TOKEN)-1))
				  	  {
						char *address = net_get_html_help_token(
														cp+sizeof(SRC_TOKEN)-1,
														&next_word);
						cp = next_word;
						fgs->address = XP_STRDUP(address);
				  	  }
#define WINDOW_TOKEN "DEFAULT_WINDOW"
					else if(!strncasecomp(cp, 
									  WINDOW_TOKEN, 
									  sizeof(WINDOW_TOKEN)-1))
				      {
					    char *window = net_get_html_help_token(
													cp+sizeof(WINDOW_TOKEN)-1, 
													&next_word);
					    cp = next_word;
					    fgs->target = XP_STRDUP(window);
				      }
					else
					  {
						/* unknown attribute.  Skip to next whitespace
						 */ 
						while(*cp && !isspace(*cp)) 
							cp++;


						if(*cp)
						  {
							while(isspace(*cp)) cp++;
							next_word = cp;
						  }
						else
						  {
							next_word = NULL;
						  }
					  }

				  } while(next_word);
			
				XP_ListAddObject(obj->frame_group_stack, fgs);
			  }
		  }
#define END_FRAME_GROUP_TOKEN "/FRAME_GROUP"
		else if(!strncasecomp(token, 
						 END_FRAME_GROUP_TOKEN, 
						 sizeof(END_FRAME_GROUP_TOKEN)-1))
		  {
			frame_set_struct *fgs;

			fgs = XP_ListRemoveTopObject(obj->frame_group_stack);

			if(fgs)
				net_help_free_frame_group_struct(fgs);
		  }
	  }
	else if(!obj->in_id_mapping)
	  {
#define WINDOW_SIZE_TOKEN "WINDOW-SIZE"
		if(!strncasecomp(line, 
					 	WINDOW_SIZE_TOKEN, 
					 	sizeof(WINDOW_SIZE_TOKEN)-1))
		  {
			/* get window size */
			char *comma=0;
			char *window_size = net_get_html_help_token(line+
												sizeof(WINDOW_SIZE_TOKEN)-1, 
												NULL);

			if(window_size)
				comma = XP_STRCHR(window_size, ',');

			if(comma)
			  {
				*comma =  '\0';
				obj->window_width = XP_ATOI(window_size);
				obj->window_height = XP_ATOI(comma+1);
			  }
		  }
#define WINDOW_NAME_TOKEN "WINDOW-NAME"
		else if(!strncasecomp(line, 
						 	WINDOW_NAME_TOKEN, 
						 	sizeof(WINDOW_NAME_TOKEN)-1))
		  {
			char *window_name = net_get_html_help_token(line+
												sizeof(WINDOW_NAME_TOKEN)-1,
												NULL);

			if(window_name)
			  {
				FREEIF(obj->window_name);
				obj->window_name = XP_STRDUP(window_name);
			  }
		  }
	  }
	else
	  {
		/* id mapping pair */
		if(!strncasecomp(line, obj->id, XP_STRLEN(obj->id)))
		  {
			char *id_value = net_get_html_help_token(line+XP_STRLEN(obj->id),
													 &next_word);

			if(id_value)
			  {
			  	obj->id_value = XP_STRDUP(id_value);

				while(next_word)
				  {
					char *cp = next_word;

#define TARGET_TOKEN "TARGET"
                    if(!strncasecomp(cp,
                                     TARGET_TOKEN,
                                      sizeof(TARGET_TOKEN)-1))
                      {
                        char *target = net_get_html_help_token(
                                                    cp+sizeof(TARGET_TOKEN)-1,
                                                    &next_word);
                        cp = next_word;
                        obj->content_target = XP_STRDUP(target);
                      }
					else
					  {
                        /* unknown attribute.  Skip to next whitespace
                         */
                        while(*cp && !isspace(*cp))
                            cp++;

                        if(*cp)
                          {
                            while(isspace(*cp)) cp++;
                            next_word = cp;
                          }
                        else
                          {
                            next_word = NULL;
                          }
					  }
				  }
			  }
		
			return(HTML_HELP_ID_FOUND);
		  }
		if(!strncasecomp(line, DEFAULT_HELP_ID, sizeof(DEFAULT_HELP_ID)-1))
		  {
			char *default_id_value = net_get_html_help_token(
												line+sizeof(DEFAULT_HELP_ID)-1,
												NULL);

            if(default_id_value)
                obj->default_id_value = XP_STRDUP(default_id_value);
		  }
		
	  }

	return(0);
}

int
NET_ParseHTMLHelpPut(HTMLHelpParseObj *obj, char *str, int32 len)
{
	int32 string_len;
	char *new_line;
	int32 status;
	
	if(!obj)
        return(MK_OUT_OF_MEMORY);

    /* buffer until we have a line */
    BlockAllocCat(obj->line_buffer, obj->line_buffer_size, str, len);
    obj->line_buffer_size += len;

    /* see if we have a line */
    while((new_line = strchr_in_buf(obj->line_buffer,
                                    obj->line_buffer_size,
                                    LF))
		  || (new_line = strchr_in_buf(obj->line_buffer,
                                    obj->line_buffer_size,
                                    CR)) )
      {
        /* terminate the line */
        *new_line = '\0';

		status = net_ParseHTMLHelpLine(obj, obj->line_buffer);

        /* remove the parsed line from obj->line_buffer */
        string_len = (new_line - obj->line_buffer) + 1;
        XP_MEMCPY(obj->line_buffer,
                  new_line+1,
                  obj->line_buffer_size-string_len);
        obj->line_buffer_size -= string_len;

		if(status == HTML_HELP_ID_FOUND)
			return(HTML_HELP_ID_FOUND);
	  }

	return(0);
}

PUBLIC void
NET_HelpPreExitRoutine(URL_Struct *URL_s, int status, MWContext *context)
{

	/* @@@@.  I wanted to use the fe_data to pass the URL struct
     * of the frame content URL.  But the fe_data gets cleared
     * by the front ends.  Therefore I'm using a private global
     * store the information.  This will work fine as long
	 * as two help requests don't come in simulatainiously.
	 */
#if 0
	/* load the content url */
	if(URL_s->fe_data)	
		FE_GetURL(context, (URL_Struct *)URL_s->fe_data);
#else

	if(frame_content_for_pre_exit_routine)
	  {
		/* FE_GetURL(context, frame_content_for_pre_exit_routine); */
		/* FE_GetURL interrupts the context */
		NET_GetURL(frame_content_for_pre_exit_routine,
				   FO_CACHE_AND_PRESENT,
				   context,
				   simple_exit);
		frame_content_for_pre_exit_routine=NULL;
	  }
#endif

	

}

PRIVATE void 
net_help_init_chrome(Chrome *window_chrome, int32 w, int32 h)
{
    window_chrome->type           = MWContextBrowser;
    window_chrome->w_hint         = w;
    window_chrome->h_hint         = h;
    window_chrome->allow_resize   = TRUE;
    window_chrome->show_scrollbar = TRUE;
    window_chrome->allow_close    = TRUE;
}

PUBLIC void
NET_ParseHTMLHelpLoadHelpDoc(HTMLHelpParseObj *obj, MWContext *context)
{
	URL_Struct *URL_s;
	char *frame_address = NULL;
	char *content_address = NULL;
	MWContext *new_context;
	frame_set_struct *fgs;

	if(obj->id_value || obj->default_id_value)
		content_address = NET_MakeAbsoluteURL(obj->url_to_map_file, 
											  obj->id_value ? 
												obj->id_value : 
												obj->default_id_value);

	if(!content_address)
	  {
		FE_Alert(context, "Unable to load the requested help topic");
		return;
	  }

	fgs = XP_ListPeekTopObject(obj->frame_group_stack);

	if(fgs)
	  {
		if(fgs->address)
		  {
			frame_address = NET_MakeAbsoluteURL(obj->url_to_map_file, 
												fgs->address);
		  }
	  }

	if(frame_address)
		URL_s = NET_CreateURLStruct(frame_address, NET_DONT_RELOAD);
	else
		URL_s = NET_CreateURLStruct(content_address, NET_DONT_RELOAD);

	if(!URL_s)
		goto cleanup;

	URL_s->window_chrome = XP_NEW(Chrome);	

	if(!URL_s->window_chrome)
		goto cleanup;

	XP_MEMSET(URL_s->window_chrome, 0, sizeof(Chrome));

	if(obj->window_name)
		URL_s->window_target = XP_STRDUP(obj->window_name);
	else
		URL_s->window_target = XP_STRDUP(DEFAULT_HELP_WINDOW_NAME);

	net_help_init_chrome(URL_s->window_chrome, 
						 obj->window_width, 
						 obj->window_height);

	new_context = XP_FindNamedContextInList(NULL, URL_s->window_target);

	if(frame_address)
	  {
		URL_Struct *content_URL_s;

		/* if there is a frame_address then we load the
		 * frame first and then load the contents
		 * in the frame exit function.
		 */
		content_URL_s = NET_CreateURLStruct(content_address, NET_DONT_RELOAD);

		if(obj->content_target)
			content_URL_s->window_target = XP_STRDUP(obj->content_target);
		else if(fgs->target)
			content_URL_s->window_target = XP_STRDUP(fgs->target);

		/* doesn't work: URL_s->fe_data = (void *) content_URL_s; */

		/* hack, see pre_exit_routine_above */
		frame_content_for_pre_exit_routine = content_URL_s;

		URL_s->pre_exit_fn = NET_HelpPreExitRoutine;
	  }

	if(!new_context)
	  {
		/* this will cause the load too */
		FE_MakeNewWindow(context, 
						 URL_s, 
						 URL_s->window_target, 
						 URL_s->window_chrome);
	  }
	else
	  {
		FE_RaiseWindow(new_context);
		FE_GetURL(new_context, URL_s);
	  }

cleanup:
	FREEIF(frame_address);
	FREE(content_address);

	return;
}

typedef struct {
	HTMLHelpParseObj *parse_obj;
	MWContext        *context;
	XP_Bool           file_is_local;
} html_help_map_stream;

PRIVATE int
net_HMFConvPut(html_help_map_stream *obj, char *s, int32 l)
{
	int status = NET_ParseHTMLHelpPut(obj->parse_obj, s, l);	

	if(obj->file_is_local && status == HTML_HELP_ID_FOUND)
	  {
		/* abort since we don't need any more of the file */
		return(MK_UNABLE_TO_CONVERT);
	  }

	return(status);
}

PRIVATE int
net_HMFConvWriteReady(html_help_map_stream *obj)
{
	return(MAX_WRITE_READY);
}

PRIVATE void
net_HMFConvComplete(html_help_map_stream *obj)
{
	NET_ParseHTMLHelpLoadHelpDoc(obj->parse_obj, obj->context);
}

PRIVATE void
net_HMFConvAbort(html_help_map_stream *obj, int32 status)
{
	if(status == MK_UNABLE_TO_CONVERT)
		NET_ParseHTMLHelpLoadHelpDoc(obj->parse_obj, obj->context);
}

PUBLIC NET_StreamClass *
NET_HTMLHelpMapToURL(int         format_out,
                     void       *data_object,
                     URL_Struct *URL_s,
                     MWContext  *window_id)
{
    html_help_map_stream* obj;
    NET_StreamClass* stream;

    TRACEMSG(("Setting up display stream. Have URL: %s\n", URL_s->address));

    stream = XP_NEW(NET_StreamClass);
    if(stream == NULL)
        return(NULL);

    obj = XP_NEW(html_help_map_stream);
    if (obj == NULL)
      {
        FREE(stream);
        return(NULL);
      }

    XP_MEMSET(obj, 0, sizeof(html_help_map_stream));

	if(URL_s->cache_file || URL_s->memory_copy)
		obj->file_is_local = TRUE;
	else
		obj->file_is_local = NET_IsLocalFileURL(URL_s->address);
	
    obj->parse_obj = NET_ParseHTMLHelpInit(URL_s->address, URL_s->fe_data);

    if(!obj->parse_obj)
      {
        FREE(stream);
        FREE(obj);
      }

    obj->context = window_id;
    stream->name           = "HTML Help Map File converter";
    stream->complete       = (MKStreamCompleteFunc) net_HMFConvComplete;
    stream->abort          = (MKStreamAbortFunc) net_HMFConvAbort;
    stream->put_block      = (MKStreamWriteFunc) net_HMFConvPut;
    stream->is_write_ready = (MKStreamWriteReadyFunc) net_HMFConvWriteReady;
    stream->data_object    = obj;  /* document info object */
    stream->window_id      = window_id;

    return(stream);
}


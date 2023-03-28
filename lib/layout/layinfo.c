/* -*- Mode: C; tab-width: 4; -*-
 */
#include "xp.h"
#include "pa_parse.h"
#include "layout.h"

#define STREAM_WRITE(Str) \
		(*stream->put_block)(stream->data_object, Str, XP_STRLEN(Str))

void
lo_element_info(LO_Element *ele_list, NET_StreamClass *stream)
{
	LO_Element *eptr;
	char buf[64];
	char *tmp_ptr;

	eptr = ele_list;
	while (eptr != NULL)
	{
		switch(eptr->type)
		{
			case LO_IMAGE:
				{
					LO_ImageStruct *image;
					char *str;
					char *image_src;

					image = (LO_ImageStruct *)eptr;
					/*
					 * Extract the SRC of this image.
					 */
					PA_LOCK(str, char *, image->image_url);
					if (str != NULL)
					{
						image_src = XP_STRDUP(str);
					}
					else
					{
						image_src = NULL;
					}

					if(image_src 
						&& XP_STRNCMP(image_src, "internal-external-reconnect", 27))
					  {
						XP_STRCPY(buf,"<li>Image: <A TARGET=Internal_URL_Info HREF=\"about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(image_src);
						XP_STRCPY(buf,"\">");
						STREAM_WRITE(buf);
						tmp_ptr = NET_EscapeHTML(image_src);
						STREAM_WRITE(tmp_ptr);
						XP_FREE(tmp_ptr);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(image_src);
					  }
					PA_UNLOCK(image->image_url);
				}
				break;
			case LO_EMBED:
				{
					LO_EmbedStruct *embed;
					char *str;
					char *embed_src;

					embed = (LO_EmbedStruct *)eptr;
					/*
					 * Extract the SRC of this embed.
					 */
					PA_LOCK(str, char *, embed->embed_src);
					if (str != NULL)
					{
						embed_src = XP_STRDUP(str);
					}
					else
					{
						embed_src = NULL;
					}
					PA_UNLOCK(embed->embed_src);

					if(embed_src)
					  {
						XP_STRCPY(buf,"<li>Embed: <A TARGET=Internal_URL_Info HREF=\"about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(embed_src);
						XP_STRCPY(buf,"\">");
						STREAM_WRITE(buf);
						tmp_ptr = NET_EscapeHTML(embed_src);
						STREAM_WRITE(tmp_ptr);
						XP_FREE(tmp_ptr);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(embed_src);
					  }
				}
				break;
			case LO_JAVA:
				{
					LO_JavaAppStruct *java_app;
					char *str;
					char *java_code = NULL;
					char *java_codebase = NULL;
					char *java_archive = NULL;
					char *java_name = NULL;

					java_app = (LO_JavaAppStruct *)eptr;
					/*
					 * Extract the java class of this
					 * java applet.
					 */
					PA_LOCK(str, char *, java_app->attr_code);
					if (str != NULL)
					{
						java_code = XP_STRDUP(str);
					}
					PA_UNLOCK(java_app->attr_code);

					/*
					 * Extract the codebase of this
					 * java applet.
					 */
					PA_LOCK(str, char *, java_app->attr_codebase);
					if (str != NULL)
					{
						java_codebase = XP_STRDUP(str);
					}
					PA_UNLOCK(java_app->attr_codebase);

					/*
					 * Extract the archive of this
					 * java applet.
					 */
					PA_LOCK(str, char *, java_app->attr_archive);
					if (str != NULL)
					{
						java_archive = XP_STRDUP(str);
					}
					PA_UNLOCK(java_app->attr_archive);

					/*
					 * Extract the app name of this
					 * java applet.
					 */
					PA_LOCK(str, char *, java_app->attr_name);
					if (str != NULL)
					{
						java_name = XP_STRDUP(str);
					}
					PA_UNLOCK(java_app->attr_name);

					if(java_code)
					  {
						XP_STRCPY(buf,"<li>Applet: <A TARGET=Internal_URL_Info HREF=about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_code);
						XP_STRCPY(buf,">");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_code);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(java_code);
					  }
					if(java_codebase)
					  {
						XP_STRCPY(buf,"<li>Applet: <A TARGET=Internal_URL_Info HREF=about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_codebase);
						XP_STRCPY(buf,">");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_codebase);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(java_codebase);
					  }
					if(java_archive)
					  {
						XP_STRCPY(buf,"<li>Applet: <A TARGET=Internal_URL_Info HREF=about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_archive);
						XP_STRCPY(buf,">");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_archive);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(java_archive);
					  }
					if(java_name)
					  {
						XP_STRCPY(buf,"<li>Applet: <A TARGET=Internal_URL_Info HREF=about:");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_name);
						XP_STRCPY(buf,">");
						STREAM_WRITE(buf);
						STREAM_WRITE(java_name);
						XP_STRCPY(buf,"</A>");
						STREAM_WRITE(buf);
						XP_FREE(java_name);
					  }
				}
				break;
			/*
			 * We must recursively traverse into cells.
			 */
			case LO_CELL:
				lo_element_info(eptr->lo_cell.cell_list, stream);
				lo_element_info(eptr->lo_cell.cell_float_list, stream);
				break;
			default:
				break;
		}
		eptr = eptr->lo_any.next;
	}
}


/*
 * Prepare a bunch of information about the content of this
 * document and prints the information as HTML down
 * the passed in stream.
 *
 * Returns:
 *	-1	If the context passed does not correspond to any currently
 *		laid out document.
 *	0	If the context passed corresponds to a document that is
 *		in the process of being laid out.
 *	1	If the context passed corresponds to a document that is
 *		completly laid out and info can be found.
 */
PUBLIC intn
LO_DocumentInfo(MWContext *context, NET_StreamClass *stream)
{
	int32 doc_id;
	lo_TopState *top_state;
	lo_DocState *state;
	LO_Element **line_array;
	LO_Element *eptr;
	/* stuff we want to return info on */
	LO_ImageStruct *bg_image;
	char *url;
	char *base_url;
	lo_FormData *form_list;
	int form_num=1;

	/*
	 * Get the unique document ID, and retreive this
	 * documents layout state.
	 */
	doc_id = XP_DOCID(context);
	top_state = lo_FetchTopState(doc_id);
	if ((top_state == NULL)||(top_state->doc_state == NULL))
	{
		return(-1);
	}
	state = top_state->doc_state;

	if (top_state->layout_status != PA_COMPLETE)
	{
		char buf[64];

		XP_STRCPY(buf, "<H3>No info while document is loading</H3>\n");
		STREAM_WRITE(buf);

		return(0);
	}

	/*
	 * Extract the URL this document was accessed by.
	 */
	if (top_state->url != NULL)
	{
		char buf[64];

		url = XP_STRDUP(top_state->url);
		XP_STRCPY(buf, "<A TARGET=Internal_URL_Info HREF=about:");
		STREAM_WRITE(buf);
		STREAM_WRITE(url);
		XP_STRCPY(buf,">");
		STREAM_WRITE(buf);
		STREAM_WRITE(url);
		XP_STRCPY(buf,"</A><UL>");
		STREAM_WRITE(buf);
		XP_FREE(url);
	}
	else
	{
		url = NULL;
	}

	/*
	 * Extract the BASE URL for this document.
	 */
	if (top_state->base_url != NULL)
	{
		base_url = XP_STRDUP(top_state->base_url);
	}
	else
	{
		base_url = NULL;
	}

	/*
	 * Extract the background image, NULL if none.
	 */
	bg_image = (LO_ImageStruct *)(top_state->backdrop_image);
    if (bg_image != NULL && bg_image->image_url != NULL)
    {
        char buf[64];
		char *str_ptr;
		char *copy_of_str;

		PA_LOCK(str_ptr, char *, bg_image->image_url);

		/* layout mucks with strings so you must make
		 * a copy of them before sending them up
		 */
        copy_of_str = XP_STRDUP(str_ptr);
		if(copy_of_str)
		  {
        	XP_STRCPY(buf, "Background Image: ");
        	STREAM_WRITE(buf);
        	XP_STRCPY(buf, "<A TARGET=Internal_URL_Info HREF=about:");
        	STREAM_WRITE(buf);
        	STREAM_WRITE(copy_of_str);
        	XP_STRCPY(buf,">");
        	STREAM_WRITE(buf);
        	XP_FREE(copy_of_str);
        	copy_of_str = XP_STRDUP(str_ptr);
			if(copy_of_str)
			  {
        		STREAM_WRITE(copy_of_str);
        		XP_FREE(copy_of_str);
			  }
        	XP_STRCPY(buf,"</A><UL>");
        	STREAM_WRITE(buf);
	 	  }

		PA_UNLOCK(bg_image->image_url);
    }


	/*
	 * Is this a grid?  If so there is nothing but grid cells
	 * in it.  Return after processing it.
	 */
	if (top_state->is_grid != FALSE)
	{
		lo_GridRec *the_grid;
		int32 cell_count;
		lo_GridCellRec *cell_list;
		char buf[16];

		the_grid = top_state->the_grid;
		if (the_grid == NULL)
		{
			XP_STRCPY(buf,"</UL>");
			STREAM_WRITE(buf);
			return(1);
		}

		/*
		 * The number of cells may be less than
		 * the rows * cols because of blank cells.
		 */
		cell_count = the_grid->cell_count;

		cell_list = the_grid->cell_list;
		while (cell_list != NULL)
		{
			char *cell_url;
			MWContext *cell_context;

			/*
			 * Extract the cell's URL.
			 */
			if (cell_list->url != NULL)
			{
				cell_url = XP_STRDUP(cell_list->url);
			}
			else
			{
				cell_url = NULL;
			}

			/*
			 * Extract the cell's window context.
			 */
			cell_context = cell_list->context;

			XP_STRCPY(buf,"<LI>Frame: ");
			STREAM_WRITE(buf);

			/* recurse and print */
			LO_DocumentInfo(cell_context, stream);

			cell_list = cell_list->next;
		}
		XP_STRCPY(buf,"</UL>");
		STREAM_WRITE(buf);
		return(1);
	}

	/*
	 * Extract the list of all forms in this doc.
	 */
	form_list = top_state->form_list;
	while (form_list != NULL)
	{
		char *str;
		char *submit_url;
		char *encoding;
		char buf[128];

		XP_SPRINTF(buf, "<b>Form %d:</b><UL>", form_num++);
		STREAM_WRITE(buf);

		PA_LOCK(str, char *, form_list->action);
		/*
		 * Extract the URL this form will submit to.
		 */
		if (str != NULL)
		{
			submit_url = XP_STRDUP(str);
		}
		else
		{
			submit_url = NULL;
		}
		PA_UNLOCK(form_list->action);

		XP_STRCPY(buf, "<LI>Action URL: ");
		STREAM_WRITE(buf);
		if(submit_url)
	 	  {
			STREAM_WRITE(submit_url);
			XP_FREE(submit_url);
		  }

		PA_LOCK(str, char *, form_list->encoding);
		/*
		 * Extract the encoding this form will submit in.
		 */
		if (str != NULL)
		{
			encoding = XP_STRDUP(str);
		}
		else
		{
			encoding = NULL;
		}
		PA_UNLOCK(form_list->encoding);

		XP_STRCPY(buf, "<LI>Encoding: ");
		STREAM_WRITE(buf);
		if(encoding)
	 	  {
			STREAM_WRITE(encoding);
			XP_FREE(encoding);
		  }
		else
		  {
			XP_STRCPY(buf, APPLICATION_WWW_FORM_URLENCODED " (default)");
			STREAM_WRITE(buf);
		  }

		/*
		 * Extract the submit method
		 * (FORM_METHOD_GET or FORM_METHOD_POST).
		 */
		XP_STRCPY(buf, "<LI>Method: ");
		STREAM_WRITE(buf);

		if(form_list->method == FORM_METHOD_POST)
		    XP_STRCPY(buf, "Post");
		else
		    XP_STRCPY(buf, "Get");
		STREAM_WRITE(buf);

		XP_STRCPY(buf, "</UL>");
		STREAM_WRITE(buf);

		/* move to the next one */
		form_list = form_list->next;
	}

	/*
	 * Now traverse the element list describing important elements.
	 * Right now interesting elements are Images and Embeds.
	 */
	/*
	 * Make eptr point to the start of the element chain
	 * for this document.
	 */
#ifdef XP_WIN16
	{
		XP_Block *larray_array;

		XP_LOCK_BLOCK(larray_array, XP_Block *, state->larray_array);
		state->line_array = larray_array[0];
		XP_UNLOCK_BLOCK(state->larray_array);
	}
#endif /* XP_WIN16 */
	XP_LOCK_BLOCK(line_array, LO_Element **, state->line_array);
	eptr = line_array[0];
	XP_UNLOCK_BLOCK(state->line_array);

	lo_element_info(eptr, stream);
	lo_element_info(state->float_list, stream);

	if(1)
	  {
		char buf[16];
		XP_STRCPY(buf,"</UL>");
		STREAM_WRITE(buf);
	  }

	return(1);
}


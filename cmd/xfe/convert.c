/* -*- Mode:C; tab-width: 8 -*-
   convert.c --- Converting documents in various contorted ways.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 30-Jan-95.
 */

#include "mozilla.h"
#include "xlate.h"
#include "xfe.h"

struct output_to_string_data
{
  Boolean done;
  char *data;
  int data_size;
  int data_fp;
};


static void
fe_get_url_as_something_done (FILE *file, char *filename,
			      struct output_to_string_data *data)
			      
{
  struct stat st;
  int nread = 0;

  /* Close hateful temporary output file */
  fclose (file);

  /* Open hateful temporary file as input  */
  file = XP_FileOpen (filename, xpTemporary, XP_FILE_READ);
  if (! file)
    goto ABORT;
  fstat (fileno (file), &st);
  if (st.st_size + 1 > data->data_size)
    {
      data->data_size = st.st_size + 1;
      data->data = (char *) realloc (data->data, data->data_size);
      if (! data->data)
	goto ABORT;
    }

  while ((nread = fread (data->data + data->data_fp, 1,
			 data->data_size - data->data_fp,
			 file))
	 > 0)
    {
      data->data_fp += nread;
    }
  data->data [data->data_fp] = 0;

 ABORT:

  /* Close and delete hateful temporary input file */
  if (file)
    {
      fclose (file);
      XP_FileRemove (filename, xpTemporary);
    }

  /* Tell the wait loop to exit. */
  data->done = True;
}


static void
fe_get_url_as_text_done (PrintSetup *p)
{
  struct output_to_string_data *data =
    (struct output_to_string_data *) p->carg;
  fe_get_url_as_something_done (p->out, p->filename, data);
}

#if 0
static void
fe_get_url_as_source_done (struct save_as_data *sad)
{
  struct output_to_string_data *data =
    (struct output_to_string_data *) sad->data;
  fe_get_url_as_something_done (sad->file, sad->name, data);
}
#endif

#if 0
static void
fe_get_url_as_source_exit (URL_Struct *url, int status, MWContext *context)
{
  if (status < 0 && url->error_msg)
    FE_Alert (context, url->error_msg);
  /* Don't free the URL */
}
#endif

char *
fe_GetURLAsText (MWContext *context, URL_Struct *url, const char *prefix,
		 unsigned long *data_size)
{
  char *result;
  PrintSetup p;
  struct output_to_string_data data;

  if (! prefix) prefix = "";

  /* remove an position information from the url
   */
  XP_STRTOK(url->address, "#");
  url->position_tag = 0;

  data.done = False;
  data.data_fp = 0;
  data.data_size = 1024;
  data.data = (char *) malloc (data.data_size);

  XL_InitializeTextSetup (&p);
  p.prefix = (char *) prefix;
  p.carg = &data;
  p.url = url;
  p.completion = fe_get_url_as_text_done;
  p.filename = WH_TempName (xpTemporary, "ns");
  if (!p.filename) return NULL;
  p.out = XP_FileOpen (p.filename, xpTemporary, XP_FILE_WRITE);
  if (p.out)
    {
      XL_TranslateText (context, p.url, &p);
      while (!data.done)
	fe_EventLoop ();
    }
  result = (char *) realloc (data.data, data.data_fp + 1); /* shrink it */
  free (p.filename);
  if (data_size) *data_size = data.data_fp;
  return result;
}

#if 0
char *
fe_GetURLAsPostscript (MWContext *context, URL_Struct *url,
		       unsigned long *data_size)
{
  char *result;
  PrintSetup p;
  struct output_to_string_data data;

  data.done = False;
  data.data_fp = 0;
  data.data_size = 1024;
  data.data = (char *) malloc (data.data_size);

  XFE_InitializePrintSetup (&p);
  p.carg = &data;
  p.url = url;
  p.completion = fe_get_url_as_text_done;
  p.filename = WH_TempName (xpTemporary, "ns");
  if (!p.filename) return NULL;
  p.out = XP_FileOpen (p.filename, xpTemporary, XP_FILE_WRITE);
  if (p.out)
    {
      XL_TranslatePostscript(context, p.url, &p);
      while (!data.done)
	fe_EventLoop ();
    }
  result = (char *) realloc (data.data, data.data_fp + 1); /* shrink it */
  free (p.filename);
  if (data_size) *data_size = data.data_fp;
  return result;
}
#endif


#if 0
char *
fe_GetURLAsSource (MWContext *context, URL_Struct *url,
		   unsigned long *data_size)
{
  char *result;
  struct save_as_data *sad =
    (struct save_as_data *) calloc (sizeof (struct save_as_data), 1);
  struct output_to_string_data data;

  data.done = False;
  data.data_fp = 0;
  data.data_size = 1024;
  data.data = (char *) malloc (data.data_size);

  sad->context = context;
  sad->name = WH_TempName (xpTemporary, "ns");
  if (!p.filename) return NULL;
  sad->file = XP_FileOpen (sad->name, xpTemporary, XP_FILE_WRITE);
  sad->type = fe_FILE_TYPE_HTML;
  sad->insert_base_tag = TRUE;
  sad->done = fe_get_url_as_source_done;
  sad->data = &data;

  if (sad->file)
    {
      url->fe_data = sad;
      NET_GetURL (url, FO_CACHE_AND_SAVE_AS, sad->context,
		  fe_get_url_as_source_exit);
      while (!data.done)
	fe_EventLoop ();
      /* sad and sad->file are freed by fe_save_as_stream_complete_method() */
    }
  result = (char *) realloc (data.data, data.data_fp + 1); /* shrink it */
  if (data_size) *data_size = data.data_fp;
  return result;
}
#endif

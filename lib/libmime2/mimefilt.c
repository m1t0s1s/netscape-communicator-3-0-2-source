/* -*- Mode: C; tab-width: 4 -*-
   mimefilt.c --- test harness for libmime.a
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.

   This program reads a message from stdin and writes the output of the MIME
   parser on stdout.

   Parameters can be passed to the parser through the usual URL mechanism:

     mimefilt BASE-URL?headers=all&rot13 < in > out

   Some parameters can't be affected that way, so some additional switches
   may be passed on the command line after the URL:

     -fancy		whether fancy headers should be generated (default)

     -no-fancy	opposite; this uses the headers used in the cases of
				FO_SAVE_AS_TEXT or FO_QUOTE_MESSAGE

     -html		whether we should convert to HTML (like FO_PRESENT);
				this is the default if no ?part= is specified.

     -raw		don't convert to HTML (FO_SAVE_AS);
				this is the default if a ?part= is specified.

     -leaf		just show the leaf node structure.

     -outline	at the end, print a debugging overview of the MIME structure

   Before any output comes a blurb listing the content-type, charset, and 
   various other info that would have been put in the generated URL struct.
   It's printed to the beginning of the output because otherwise this out-
   of-band data would have been lost.  (So the output of this program is,
   in fact, a raw HTTP response.)
 */

#include "mimemsg.h"
#include "prglobal.h"

#ifdef MIME_SECURITY
#include "key.h"
#include "cert.h"
#include "secrng.h"
#include "secmod.h"
#include "pk11func.h"
#endif /* MIME_SECURITY */


#ifndef XP_UNIX
ERROR!	This is a unix-only file for the "mimefilt" standalone program.
		This does not go into libmime.a.
#endif


static char *
test_file_type (const char *filename, void *stream_closure)
{
  const char *suf = XP_STRRCHR(filename, '.');
  if (!suf)
	return 0;
  suf++;

  if (!strcasecomp(suf, "txt") ||
	  !strcasecomp(suf, "text"))
	return XP_STRDUP("text/plain");
  else if (!strcasecomp(suf, "htm") ||
		   !strcasecomp(suf, "html"))
	return XP_STRDUP("text/html");
  else if (!strcasecomp(suf, "gif"))
	return XP_STRDUP("image/gif");
  else if (!strcasecomp(suf, "jpg") ||
		   !strcasecomp(suf, "jpeg"))
	return XP_STRDUP("image/jpeg");
  else if (!strcasecomp(suf, "pjpg") ||
		   !strcasecomp(suf, "pjpeg"))
	return XP_STRDUP("image/pjpeg");
  else if (!strcasecomp(suf, "xbm"))
	return XP_STRDUP("image/x-xbitmap");
  else if (!strcasecomp(suf, "xpm"))
	return XP_STRDUP("image/x-xpixmap");
  else if (!strcasecomp(suf, "xwd"))
	return XP_STRDUP("image/x-xwindowdump");
  else if (!strcasecomp(suf, "bmp"))
	return XP_STRDUP("image/x-MS-bmp");
  else if (!strcasecomp(suf, "au"))
	return XP_STRDUP("audio/basic");
  else if (!strcasecomp(suf, "aif") ||
		   !strcasecomp(suf, "aiff") ||
		   !strcasecomp(suf, "aifc"))
	return XP_STRDUP("audio/x-aiff");
  else if (!strcasecomp(suf, "ps"))
	return XP_STRDUP("application/postscript");
  else if (!strcasecomp(suf, "p7m"))
	return XP_STRDUP("application/x-pkcs7-mime");
  else if (!strcasecomp(suf, "p7c"))
	return XP_STRDUP("application/x-pkcs7-mime");
  else if (!strcasecomp(suf, "p7s"))
	return XP_STRDUP("application/x-pkcs7-signature");
  else
	return 0;
}

static char *
test_type_icon(const char *type, void *stream_closure)
{
  if (!strncasecomp(type, "text/", 5))
	return XP_STRDUP("internal-gopher-text");
  else if (!strncasecomp(type, "image/", 6))
	return XP_STRDUP("internal-gopher-image");
  else if (!strncasecomp(type, "audio/", 6))
	return XP_STRDUP("internal-gopher-sound");
  else if (!strncasecomp(type, "video/", 6))
	return XP_STRDUP("internal-gopher-movie");
  else if (!strncasecomp(type, "application/", 12))
	return XP_STRDUP("internal-gopher-binary");
  else
	return XP_STRDUP("internal-gopher-unknown");
}

static int
test_output_fn(char *buf, int32 size, void *closure)
{
  FILE *out = (FILE *) closure;
  if (out)
	return fwrite(buf, sizeof(*buf), size, out);
  else
	return 0;
}

static int
test_output_init_fn (const char *type,
					 const char *charset,
					 const char *name,
					 const char *x_mac_type,
					 const char *x_mac_creator,
					 void *stream_closure)
{
  FILE *out = (FILE *) stream_closure;
  fprintf(out, "CONTENT-TYPE: %s", type);
  if (charset)
	fprintf(out, "; charset=\"%s\"", charset);
  if (name)
	fprintf(out, "; name=\"%s\"", name);
  if (x_mac_type || x_mac_creator)
	fprintf(out, "; x-mac-type=\"%s\"; x-mac-creator=\"%s\"",
			x_mac_type ? x_mac_type : "",
			x_mac_creator ? x_mac_type : "");
  fprintf(out, CRLF CRLF);
  return 0;
}


#ifdef MIME_TO_HTML
static int
test_reset_html_state_fn (void *stream_closure, XP_Bool abort_p)
{
  char random_close_tags[] =
		"</TABLE></TABLE></TABLE></TABLE></TABLE></TABLE>"
		"</DL></DL></DL></DL></DL></DL></DL></DL></DL></DL>"
		"</DL></DL></DL></DL></DL></DL></DL></DL></DL></DL>"
		"</B></B></B></B></B></B></B></B></B></B></B></B>"
		"</PRE></PRE></PRE></PRE></PRE></PRE></PRE></PRE>"
		"<BASEFONT SIZE=3></SCRIPT>";
  return test_output_fn(random_close_tags, XP_STRLEN(random_close_tags),
						stream_closure);
}

static void *
test_image_begin(const char *image_url, const char *content_type,
				 void *stream_closure)
{
  return ((void *) XP_STRDUP(image_url));
}

static void
test_image_end(void *image_closure, int status)
{
  char *url = (char *) image_closure;
  if (url) XP_FREE(url);
}

static char *
test_image_make_image_html(void *image_data)
{
  char *url = (char *) image_data;
#if 0
  const char *prefix = "<P><CENTER><IMG SRC=\"internal-external-reconnect:";
  const char *suffix = "\"></CENTER><P>";
#else
  const char *prefix = ("<P><CENTER><TABLE BORDER=2 CELLPADDING=20"
						" BGCOLOR=WHITE>"
						"<TR><TD ALIGN=CENTER>"
						"an inlined image would have gone here for<BR>");
  const char *suffix = "</TD></TR></TABLE></CENTER><P>";
#endif
  char *buf;
  buf = (char *) XP_ALLOC (XP_STRLEN (prefix) + XP_STRLEN (suffix) +
						   XP_STRLEN (url) + 20);
  if (!buf) return 0;
  *buf = 0;
  XP_STRCAT (buf, prefix);
  XP_STRCAT (buf, url);
  XP_STRCAT (buf, suffix);
  return buf;
}

static int test_image_write_buffer(char *buf, int32 size, void *image_closure)
{
  return 0;
}
#endif /* MIME_TO_HTML */


#ifdef MIME_LEAF_TEXT
static int leaf_count = 0;

static void *
test_leaf_begin (const char *type, MimeHeaders *hdrs, MimeHeaders *chdrs,
				 void *leaf_closure)
{
  char *name = (hdrs ? MimeHeaders_get_name(hdrs) : 0);
  if (!type) type = "UNKNOWN";
  leaf_count++;
  if (name && *name)
	printf ("Leaf %d: %s; name=\"%s\"\n", leaf_count, type, name);
  else
	printf ("Leaf %d: %s\n", leaf_count, type);
  if (name) free (name);
  return ((void *) leaf_count);
}

static int
test_leaf_write (char *buf, int32 size, void *leaf_closure)
{
  printf ("     %d: %d bytes\n", (int) leaf_closure, size);
  return 0;
}

static int
test_leaf_end (void *leaf_closure, XP_Bool status)
{
  printf ("Leaf %d close (%d)\n", (int) leaf_closure, status);
  return 0;
}

#endif /* MIME_LEAF_TEXT */



#ifdef MIME_SECURITY
static char *
test_passwd_prompt (PK11SlotInfo *slot, void *wincx)
{
  char buf[2048], *s;
  fprintf(stdout, "#### Password required: ");
  s = fgets(buf, sizeof(buf)-1, stdin);
  if (!s) return s;
  if (s[strlen(s)-1] == '\r' ||
	  s[strlen(s)-1] == '\n')
	s[strlen(s)-1] = '\0';
  return s;
}
#endif /* MIME_SECURITY */


int
test(FILE *in, FILE *out,
	 const char *url,
#ifdef MIME_TO_HTML
	 XP_Bool fancy_headers_p,
	 XP_Bool html_p,
#endif /* MIME_TO_HTML */
#ifdef MIME_LEAF_TEXT
	 XP_Bool leaf_p,
#endif /* MIME_LEAF_TEXT */
	 XP_Bool outline_p,
#ifdef MIME_SECURITY
	 XP_Bool decrypt_p,
#endif /* MIME_SECURITY */
	 XP_Bool variable_width_plaintext_p)
{
  int status = 0;
  MimeObject *obj = 0;
  MimeDisplayOptions *opt = XP_NEW(MimeDisplayOptions);
  XP_MEMSET(opt, 0, sizeof(*opt));

#if defined(MIME_TO_HTML) && defined(MIME_SECURITY)
  if (decrypt_p) html_p = FALSE;
#endif /* MIME_TO_HTML && MIME_SECURITY */

#if defined(MIME_TO_HTML) && defined(MIME_LEAF_TEXT)
  if (leaf_p) html_p = FALSE;
#endif /* MIME_TO_HTML && MIME_LEAF_TEXT */

#ifdef MIME_TO_HTML
  opt->fancy_headers_p = fancy_headers_p;
#endif /* MIME_TO_HTML */

  opt->headers = MimeHeadersSome;
  opt->no_inline_p = FALSE;
  opt->rot13_p = FALSE;

  status = mime_parse_url_options(url, opt);
  if (status < 0)
	{
	  XP_FREE(opt);
	  return MK_OUT_OF_MEMORY;
	}

  opt->url					= url;
#ifdef MIME_TO_HTML
  opt->write_html_p			= html_p;
#endif /* MIME_TO_HTML */
#ifdef MIME_LEAF_TEXT
  opt->write_leaf_p			= leaf_p;
#endif /* MIME_LEAF_TEXT */
#ifdef MIME_SECURITY
  opt->decrypt_p			= decrypt_p;
#endif /* MIME_SECURITY */
  opt->output_init_fn		= test_output_init_fn;
  opt->output_fn			= test_output_fn;
#ifdef MIME_TO_HTML
  opt->reset_html_state_fn  = test_reset_html_state_fn;
#endif /* MIME_TO_HTML */
  opt->charset_conversion_fn= 0;
  opt->rfc1522_conversion_fn= 0;
  opt->reformat_date_fn		= 0;
  opt->file_type_fn			= test_file_type;
  opt->type_description_fn	= 0;
  opt->type_icon_name_fn	= test_type_icon;
  opt->stream_closure		= out;

#ifdef MIME_TO_HTML
  opt->image_begin			= test_image_begin;
  opt->image_end			= test_image_end;
  opt->make_image_html		= test_image_make_image_html;
  opt->image_write_buffer	= test_image_write_buffer;
#endif /* MIME_TO_HTML */

#ifdef MIME_LEAF_TEXT
  opt->leaf_begin			= test_leaf_begin;
  opt->leaf_write			= test_leaf_write;
  opt->leaf_end				= test_leaf_end;
#endif /* MIME_LEAF_TEXT */

  opt->variable_width_plaintext_p = variable_width_plaintext_p;

  obj = mime_new ((MimeObjectClass *)&mimeMessageClass,
				  (MimeHeaders *) NULL,
				  MESSAGE_RFC822);
  if (!obj)
	{
	  XP_FREE(opt);
	  return MK_OUT_OF_MEMORY;
	}
  obj->options = opt;

  status = obj->class->initialize(obj);
  if (status >= 0)
	status = obj->class->parse_begin(obj);
  if (status < 0)
	{
	  XP_FREE(opt);
	  XP_FREE(obj);
	  return MK_OUT_OF_MEMORY;
	}

  while (1)
	{
	  char buf[255];
	  int size = fread(buf, sizeof(*buf), sizeof(buf), stdin);
	  if (size <= 0) break;
	  status = obj->class->parse_buffer(buf, size, obj);
	  if (status < 0)
		{
		  mime_free(obj);
		  XP_FREE(opt);
		  return status;
		}
	}

  status = obj->class->parse_eof(obj, FALSE);
  if (status >= 0)
	status = obj->class->parse_end(obj, FALSE);
  if (status < 0)
	{
	  mime_free(obj);
	  XP_FREE(opt);
	  return status;
	}

#ifdef DEBUG
  if (outline_p)
	{
	  fprintf(out, "\n\n"
		  "###############################################################\n");
	  obj->class->debug_print(obj, stderr, 0);
	  fprintf(out,
		  "###############################################################\n");
	}
#endif /* DEBUG */

  mime_free (obj);
  XP_FREE(opt);
  return 0;
}


#ifdef MIME_SECURITY
static char *
test_cdb_name_cb (void *arg, int vers)
{
  static char f[1024];
  if (vers <= 4)
	sprintf(f, "%s/.netscape/cert.db", getenv("HOME"));
  else
	sprintf(f, "%s/.netscape/cert%d.db", getenv("HOME"), vers);
  return f;
}

static char *
test_kdb_name_cb (void *arg, int vers)
{
  static char f[1024];
  if (vers <= 2)
	sprintf(f, "%s/.netscape/key.db", getenv("HOME"));
  else
	sprintf(f, "%s/.netscape/key%d.db", getenv("HOME"), vers);
  return f;
}

extern void SEC_Init(void);
#endif /* MIME_SECURITY */

int
main (int argc, char **argv)
{
  int32 i = 1;
  char *url = "";
#ifdef MIME_TO_HTML
  XP_Bool fancy_p = TRUE;
  XP_Bool html_p = TRUE;
#endif /* MIME_TO_HTML */
#ifdef MIME_LEAF_TEXT
  XP_Bool leaf_p = TRUE;
#endif /* MIME_LEAF_TEXT */
  XP_Bool outline_p = FALSE;

#ifdef MIME_SECURITY
  XP_Bool decrypt_p = FALSE;
  char filename[1000];
  CERTCertDBHandle *cdb_handle;
  SECKEYKeyDBHandle *kdb_handle;
#endif /* MIME_SECURITY */

#ifdef MIME_SECURITY
  PR_Init("mimefilt", 24, 1, 0);

  cdb_handle = (CERTCertDBHandle *)  malloc(sizeof(*cdb_handle));
  memset(cdb_handle, 0, sizeof(*cdb_handle));

  if (SECSuccess != CERT_OpenCertDB(cdb_handle, FALSE, test_cdb_name_cb, NULL))
	CERT_OpenVolatileCertDB(cdb_handle);
  CERT_SetDefaultCertDB(cdb_handle);

  RNG_RNGInit();

  kdb_handle = SECKEY_OpenKeyDB(PR_FALSE, test_kdb_name_cb, NULL);
  SECKEY_SetDefaultKeyDB(kdb_handle);

  PK11_SetPasswordFunc(test_passwd_prompt);

  sprintf(filename, "%s/.netscape/secmodule.db", getenv("HOME"));
  SECMOD_init(filename);

  SEC_Init();
#endif /* MIME_SECURITY */


  if (i < argc)
	{
	  if (argv[i][0] == '-')
		url = XP_STRDUP("");
	  else
		url = argv[i++];
	}

#ifdef MIME_TO_HTML
  if (url &&
	  (XP_STRSTR(url, "?part=") ||
	   XP_STRSTR(url, "&part=")))
	html_p = FALSE;
#endif /* MIME_TO_HTML */

  while (i < argc)
	{
#ifdef MIME_TO_HTML
	  if (!XP_STRCMP(argv[i], "-fancy"))
		fancy_p = TRUE;
	  else if (!XP_STRCMP(argv[i], "-no-fancy"))
		fancy_p = FALSE;
	  else if (!XP_STRCMP(argv[i], "-html"))
		{
		  html_p = TRUE;
# ifdef MIME_LEAF_TEXT
		  leaf_p = FALSE;
# endif /* MIME_LEAF_TEXT */
		}
	  else if (!XP_STRCMP(argv[i], "-raw"))
		{
		  html_p = FALSE;
# ifdef MIME_LEAF_TEXT
		  leaf_p = FALSE;
# endif /* MIME_LEAF_TEXT */
		}
	  else
#endif /* MIME_TO_HTML */

#ifdef MIME_LEAF_TEXT
	  if (!XP_STRCMP(argv[i], "-leaf"))
		{
		  leaf_p = TRUE;
# ifdef MIME_TO_HTML
		  html_p = FALSE;
# endif /* MIME_TO_HTML */
		}
#endif /* MIME_LEAF_TEXT */
#ifdef DEBUG
	  else if (!XP_STRCMP(argv[i], "-outline"))
		outline_p = TRUE;
#endif /* DEBUG */
#ifdef MIME_SECURITY
	  else if (!XP_STRCMP(argv[i], "-decrypt"))
		decrypt_p = TRUE;
#endif /* MIME_SECURITY */
	  else
		{
		  const char *fmt = ("usage: %s [ URL [ -outline "
# ifdef MIME_TO_HTML
							 "| -fancy | -no-fancy | -html | -raw "
# endif /* MIME_TO_HTML */
# ifdef MIME_LEAF_TEXT
							 "| -leaf "
# endif /* MIME_LEAF_TEXT */
#ifdef MIME_SECURITY
							 "| -decrypt "
#endif /* MIME_SECURITY */
							 "]]\n"
							 "     < message/rfc822 > output\n");
		  fprintf(stderr, fmt,
				  (XP_STRRCHR(argv[0], '/')
				   ? XP_STRRCHR(argv[0], '/') + 1 :
				   argv[0]));
		  i = 1;
		  goto FAIL;
		}
	  i++;
	}

  i = test(stdin, stdout, url,
#ifdef MIME_TO_HTML
		   fancy_p, html_p,
#endif /* MIME_TO_HTML */
#ifdef MIME_LEAF_TEXT
		   leaf_p,
#endif /* MIME_LEAF_TEXT */
		   outline_p,
#ifdef MIME_SECURITY
		   decrypt_p,
#endif /* MIME_SECURITY */
		   TRUE);
  fprintf(stdout, "\n");
  fflush(stdout);

 FAIL:

#ifdef MIME_SECURITY
  CERT_ClosePermCertDB(cdb_handle);
  SECKEY_CloseKeyDB(kdb_handle);
#endif /* MIME_SECURITY */

  exit(i);
}

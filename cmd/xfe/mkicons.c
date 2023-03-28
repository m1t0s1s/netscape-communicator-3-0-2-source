/* -*- Mode: C; tab-width: 8 -*-
   mkicons.c --- converting transparent GIFs to embeddable XImage data.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 17-Aug-95.
   (Danger.  Here be monsters.)
 */

#include "if.h"
#include "nspr/prcpucfg.h"

#define MAX_ANIMS 10

/* =========================================================================
   All this junk is just to get it to link.
   =========================================================================
 */

#ifdef MOCHA
void LM_SendImageEvent(MWContext *context, LO_ImageStruct *image_data,
                       void * event) { return;}
#endif

int NET_URL_Type(const char *URL) {return 0;}
    
void * FE_SetTimeout(TimeoutCallbackFunction func, void * closure,
                     uint32 msecs) { return 0; }
void FE_ClearTimeout(void *timer_id) {}
void XP_Trace(const char * format, ...) {}
void FE_ImageDelete(IL_Image *portableImage) {}
int32 NET_GetMemoryCacheSize(void) { return 1000000; }
int32 NET_GetMaxMemoryCacheSize(void) { return 1000000; }
void NET_FreeURLStruct(URL_Struct * URL_s) {}
int NET_InterruptWindow(MWContext * window_id) {return 0;}
URL_Struct *NET_CreateURLStruct(const char *url, NET_ReloadMethod reload) { return 0; }
History_entry *  SHIST_GetCurrent(History * hist) { return 0; }
int NET_GetURL (URL_Struct * URL_s, FO_Present_Types output_format,
		MWContext * context, Net_GetUrlExitFunc* exit_routine) 
{ return -1; }
Bool LO_BlockedOnImage(MWContext *c, LO_ImageStruct *image) { return FALSE; }
Bool NET_IsURLInDiskCache(URL_Struct *URL_s) {return TRUE;}
XP_Bool NET_IsLocalFileURL(char *address) {return TRUE;}

NET_StreamClass * NET_StreamBuilder (FO_Present_Types  format_out,
				     URL_Struct *anchor, MWContext *window_id)
{ return 0; }

Bool NET_AreThereActiveConnectionsForWindow(MWContext * window_id)
{ return FALSE; }
Bool NET_AreThereStoppableConnectionsForWindow(MWContext * window_id)
{ return FALSE; }
void LO_RefreshAnchors(MWContext *context) { }
void GH_UpdateGlobalHistory(URL_Struct * URL_s) { }
char * NET_EscapeHTML(const char * string) { return (char *)string; }
Bool LO_LocateNamedAnchor(MWContext *context, URL_Struct *url_struct,
			  int32 *xpos, int32 *ypos) { return FALSE; }
Bool LO_HasBGImage(MWContext *context) {return FALSE; }

void FE_UpdateStopState(MWContext *context) {}

void
FE_Alert (MWContext *context, const char *message) {}

#ifdef MOCHA
void LM_RemoveWindowContext(MWContext *context) { }
#endif

extern int il_first_write(void *dobj, const unsigned char *str, int32 len);

/* you loser */
void fe_ReLayout (MWContext *context, NET_ReloadMethod force_reload) {}

char *XP_GetBuiltinString(int16 i);

char *
XP_GetString(int16 i)
{
	return XP_GetBuiltinString(i);
}

Bool
NET_IsURLInMemCache(URL_Struct *URL_s)
{
	return FALSE;
}



/* =========================================================================
   Now it gets REALLY nasty.
   =========================================================================
 */

struct image {
  int width, height;
  char *color_bits;
  char *mono_bits;
  char *mask_bits;
};

int total_images = 0;
struct image images[500] = { { 0, }, };

int total_colors;
IL_IRGB cmap[256];


XP_Bool did_size = FALSE;
int column = 0;

int in_anim = 0;
int inactive_icon_p = 0;
int anim_frames[100] = { 0, };

XP_Bool sgi_p = FALSE;

static unsigned char *bitrev = 0;

static void
init_reverse_bits(void)
{
  if(!bitrev) 
    {
      int i, x, br;
      bitrev = (unsigned char *)XP_ALLOC(256);
      for(i=0; i<256; i++) 
        {
          br = 0;
          for(x=0; x<8; x++) 
            {
              br = br<<1;
              if(i&(1<<x))
                br |= 1;
            }
          bitrev[i] = br;
        }
    }
}

int
image_size (MWContext *context, IL_ImageStatus message,
	    IL_Image *il_image, void *data)
{
  if (il_image->bits)
    free (il_image->bits);
  if (!did_size)
    {
      fprintf (stdout, " %d, %d,\n", il_image->width, il_image->height);
      column = 2;
      did_size = TRUE;
    }

  il_image->bits = malloc (il_image->widthBytes * il_image->height);
  memset (il_image->bits, ~0, (il_image->widthBytes * il_image->height));
  if (!il_image->mask && il_image->transparent)
    {
      int size = il_image->maskWidthBytes * il_image->height;
      il_image->mask = malloc (size);
      memset (il_image->mask, ~0, size);
    }

  return 0;
}

void
image_data (MWContext *context, IL_ImageStatus message, IL_Image *il_image,
	    void *data, int start_row, int num_rows, XP_Bool update_image)
{
  int i;
  int row_parity;
  unsigned char *s, *m, *scanline, *mask_scanline, *end;

  if (message != ilComplete) abort ();
  images[total_images].width = il_image->width;
  images[total_images].height = il_image->height;
  if (il_image->depth == 1)
    images[total_images].mono_bits = il_image->bits;
  else
    images[total_images].color_bits = il_image->bits;
  if (il_image->mask)
    images[total_images].mask_bits = il_image->mask;

  for (i = 0; i < il_image->colors; i++)
    {
      int j;
      for (j = 0; j < total_colors; j++)
	{
	  if (il_image->map[i].red   == cmap[j].red &&
	      il_image->map[i].green == cmap[j].green &&
	      il_image->map[i].blue  == cmap[j].blue)
	    goto FOUND;
	}
      cmap[total_colors].red   = il_image->map[i].red;
      cmap[total_colors].green = il_image->map[i].green;
      cmap[total_colors].blue  = il_image->map[i].blue;

      total_colors++;
      if (total_colors > 100)
	abort();
    FOUND:
      ;
    }

  if (il_image->depth == 1)
    return;
  if (il_image->depth != 32) abort();

  /* Generate monochrome icon from color data. */
  scanline = il_image->bits;
  mask_scanline = il_image->mask;
  end = scanline + (il_image->widthBytes * il_image->height);
  row_parity = 0;
      
  fprintf (stdout, "\n (unsigned char*)\n \"");
  column = 2;
  while (scanline < end)
    {
      unsigned char *scanline_end = scanline + (il_image->width * 4);
      int luminance, pixel;
      int bit = 0;
      int byte = 0;

      row_parity ^= 1;
      for (m = mask_scanline, s = scanline; s < scanline_end; s += 4)
        {
          unsigned char r = s[3];
          unsigned char g = s[2];
          unsigned char b = s[1];
          if (column > 74)
            {
              fprintf (stdout, "\"\n \"");
              column = 2;
            }
          luminance = (0.299 * r) + (0.587 * g) + (0.114 * b);

          pixel =
            ((luminance < 128))                      ||
            ((r ==  66) && (g == 154) && (b == 167)); /* Magic: blue */
          byte |= pixel << bit++;

          if ((bit == 8) || ((s + 4) >= scanline_end)) {
            /* Handle transparent areas of the icon */
            if (il_image->mask)
              byte &= bitrev[*m++];

            /* If this is a grayed-out inactive icon, superimpose a 
               checkerboard mask over the data */
            if (inactive_icon_p)
              byte &= 0x55 << row_parity;

            fprintf (stdout, "\\%03o", byte);
            column += 4;
            bit = 0;
            byte = 0;
          }
        }
      scanline += il_image->widthBytes;
      mask_scanline += il_image->maskWidthBytes;
    }

  fprintf (stdout, "\",\n");
  column = 0;

  /* Mask data */
  if (il_image->mask)
    {
      scanline = il_image->mask;
      end = scanline + (il_image->maskWidthBytes * il_image->height);
      fprintf (stdout, "\n (unsigned char*)\n \"");
      column = 2;
      for (;scanline < end; scanline += il_image->maskWidthBytes)
        {
          unsigned char *scanline_end = scanline + ((il_image->width + 7) / 8);
          for (s = scanline; s < scanline_end; s++)
            {
              if (column > 74)
                {
                  fprintf (stdout, "\"\n \"");
                  column = 2;
                }
              fprintf (stdout, "\\%03o", bitrev[*s]);
              column += 4;
            }
        }

      fprintf (stdout, "\",\n");
      column = 0;
    }
  else
    {
      fprintf (stdout, "\n  0,\n");
      column = 0;
    }

  /* Color icon */
  fprintf (stdout, "\n (unsigned char*)\n \"");
  column = 2;

  scanline = il_image->bits;
  end = scanline + (il_image->widthBytes * il_image->height);
  for (;scanline < end; scanline += il_image->widthBytes)
    {
      unsigned char *scanline_end = scanline + (il_image->width * 4);
      for (s = scanline; s < scanline_end; s += 4)
        {
          unsigned char r = s[3];
          unsigned char g = s[2];
          unsigned char b = s[1];
          int j;
          for (j = 0; j < total_colors; j++)
            if (r == cmap[j].red &&
                g == cmap[j].green &&
                b == cmap[j].blue)
              {
                if (column > 74)
                  {
                    fprintf (stdout, "\"\n \"");
                    column = 2;
                  }
                fprintf (stdout, "\\%03o", j);
                column += 4;
                goto DONE;
              }
          abort();
        DONE:
          ;
        }
    }
      

  if (!in_anim)
    fprintf (stdout, "\"\n};\n\n");
  column = 0;

  if (il_image->bits) free (il_image->bits);
  il_image->bits = 0;
  if (il_image->mask) free (il_image->mask);
  il_image->mask = 0;
}


void
set_title (MWContext *context, char *title)
{
}


void
do_file (char *file)
{
  static int counter = 0;
  FILE *fp;
  char *data;
  int size;
  NET_StreamClass *stream;
  struct stat st;
  char *s;

  URL_Struct *url;
  il_container *ic;
  il_process *proc;
  MWContext cx;
  ContextFuncs fns;
  IL_IRGB trans;
  XP_Bool wrote_header = FALSE;

  memset (&cx, 0, sizeof(cx));
  memset (&fns, 0, sizeof(fns));
  
  url = XP_NEW_ZAP (URL_Struct);
  proc = XP_NEW_ZAP (il_process);

  url->address = strdup("\000\000");
  cx.funcs = &fns;

  fns.ImageSize = image_size;
  fns.ImageData = image_data;
  fns.SetDocTitle = set_title;

  {
    /* make a container */
    ic = XP_NEW_ZAP(il_container);
    ic->hash = 0;
    ic->urlhash = 0;
    ic->cx = &cx;
    ic->forced = 0;

    ic->image = XP_NEW_ZAP(IL_Image);

    ic->next = 0;
    ic->ip = proc;
    cx.imageProcess = proc;

    ic->state = IC_START;
  }

  url->fe_data = ic;

  ic->clients = XP_NEW_ZAP(il_client);
  ic->clients->cx = &cx;

  if (stat (file, &st))
    return;
  size = st.st_size;

  data = (char *) malloc (size + 1);
  fp = fopen (file, "r");
  fread (data, 1, size, fp);
  fclose (fp);

  s = strrchr (file, '.');
  if (s) *s = 0;
  s = strrchr (file, '/');
  if (s)
    s++;
  else
    s = file;

  if (in_anim && strncmp (s, "Anim", 4))
    /* once you've started anim frames, don't stop. */
    abort ();

  if ((!strcmp (s, "AnimSm00") || !strcmp (s, "AnimHuge00")) ||
      ((!strcmp (s, "AnimSm01") || !strcmp (s, "AnimHuge01")) &&
       (!in_anim ||
	anim_frames[in_anim-1] > 1)))
    {
      char *s2;
      if (in_anim)
	{
	  fprintf (stdout, "\"\n }\n};\n\n");
	  if (sgi_p)
	    {
	      fprintf (stdout, "\n#endif /* __sgi */\n\n");
	      sgi_p = FALSE;
	    }
	}

      s2 = s - 2;
      while (s2 > file && *s2 != '/')
	s2--;
      s[-1] = 0;
      if (*s2 == '/')
	s2++;

      if (strstr (s2, "sgi"))
	{
	  fprintf (stdout, "#ifdef __sgi\n");
	  sgi_p = TRUE;
	}

      fprintf (stdout, "struct fe_icon_data anim_%s_%s[] = {\n",
	       s2, (!strncmp (s, "AnimHuge", 8) ? "large" : "small"));
      wrote_header = TRUE;
      in_anim++;
    }

  if (in_anim)
    {
      if (strncmp (s, "Anim", 4)) abort ();
      if (!wrote_header)
	fprintf (stdout, "\"\n },\n\n");
      fprintf (stdout, "  /* %s */\n  { ", s);
      anim_frames[in_anim-1]++;
    }
  else
    {
      char *s2 = s;
      while (*s2)
	{
	  if (*s2 == '.') *s2 = '_';
	  s2++;
	}
      fprintf (stdout, "\nstruct fe_icon_data %s = {\n", s);
    }

  column = 0;
  did_size = FALSE;

  trans.red = trans.green = trans.blue = 0xC0;

  cx.colorSpace = NULL;

  IL_EnableTrueColor (&cx, 32,
#if defined(IS_LITTLE_ENDIAN)
		      24, 16, 8,
#elif defined(IS_BIG_ENDIAN)
		      0, 8, 16,
#else
  ERROR!  Endianness unknown.
#endif
		      8, 8, 8,
		      &trans, FALSE);

  ic->cs = cx.colorSpace;
  IL_SetPreferences (&cx, FALSE, ilClosestColor);
  url->address[0] = counter++;
  stream = IL_NewStream (FO_PRESENT,
			 (strstr (file, ".gif") ? (void *) IL_GIF :
			  strstr (file, ".jpg") ? (void *) IL_JPEG :
			  strstr (file, ".jpeg") ? (void *) IL_JPEG :
			  strstr (file, ".xbm") ? (void *) IL_XBM :
			  (void *) IL_GIF),
			 url, &cx);
  stream->put_block (stream->data_object, data, size);
  stream->complete (stream->data_object);

  free (data);

  total_images++;


}


int
main (argc, argv)
     int argc;
     char **argv;
{
  int i;

  init_reverse_bits();

  fprintf (stdout,
	   "/* -*- Mode: Fundamental -*- */\n\n#include \"icondata.h\"\n");

  for (i = 1; i < argc; i++)
    {
      char *filename = argv[i];
      inactive_icon_p = (strstr(filename, ".i.gif") != NULL);
      do_file (filename);
    }

  if (in_anim)
    {
      fprintf (stdout, "\"\n }\n};\n\n");
      if (sgi_p)
	{
	  fprintf (stdout, "\n#endif /* __sgi */\n\n");
	  sgi_p = FALSE;
	}
    }

  fprintf (stdout, "unsigned int fe_n_icon_colors = %d;\n", total_colors);

  if (in_anim)
    {
      fprintf (stdout, "unsigned int fe_anim_frames[%d] = { ", MAX_ANIMS);
      i = 0;
      while (anim_frames[i])
	{
	  fprintf (stdout, "%d%s", anim_frames[i],
		   anim_frames[i+1] ? ", " : "");
	  i++;
	}
      fprintf (stdout, " };\n\n");
    }

  fprintf (stdout, "unsigned short fe_icon_colors[256][3] = {\n");
  for (i = 0; i < total_colors; i++)
    fprintf (stdout, " { 0x%02x%02x, 0x%02x%02x, 0x%02x%02x }%s\n",
	     cmap[i].red,   cmap[i].red,
	     cmap[i].green, cmap[i].green,
	     cmap[i].blue,  cmap[i].blue,
	     (i == total_colors-1) ? "" : ",");

  if ( total_colors == 0 ) {
      fprintf(stdout, "0\n");
  }

  fprintf (stdout, "};\n");

  return 0;
}

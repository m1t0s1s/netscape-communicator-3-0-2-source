/* -*- Mode: C; tab-width: 4 -*-
 *   il.h --- Exported image library interface
 *   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 *   $Id: il.h,v 1.78.38.1 1997/03/06 13:34:26 jwz Exp $
 */

/*
 *  global defines for image lib users
 */

#ifndef _IL_H
#define _IL_H

#include "ntypes.h"

#define IL_UNKNOWN 0
#define IL_GIF	   1
#define IL_XBM     2
#define IL_JPEG    3
#define IL_PPM     4

#define IL_NOTFOUND 256

struct IL_IRGB_struct {
    uint8 index, attr;
    uint8 red, green, blue;
};

/* attr values */
#define IL_ATTR_RDONLY	      0
#define IL_ATTR_RW	          1
#define IL_ATTR_TRANSPARENT	  2
#define IL_ATTR_HONOR_INDEX   4

struct IL_RGB_struct {
    uint8 red, green, blue, pad;
    	/* windows requires the fourth byte & many compilers pad it anyway */
};

#undef ABS
#define ABS(x)     (((x) < 0) ? -(x) : (x))

/* A fast, but stupid, perceptually-weighted color distance function */
#define IL_COLOR_DISTANCE(r1, r2, g1, g2, b1, b2)                             \
  ((ABS((g1) - (g2)) << 2) + (ABS((r1) - (r2)) << 1) + (ABS((b1) - (b2))))

/* We don't distinguish between colors that are "closer" together
   than this.  The appropriate setting is a subjective matter. */
#define IL_CLOSE_COLOR_THRESHOLD  6

struct IL_Image_struct {
    void XP_HUGE *bits;
    int width, height;
    int widthBytes, maskWidthBytes;
    int depth, bytesPerPixel;
    int colors;
    int unique_colors;          /* Number of non-duplicated colors in colormap */
    IL_RGB *map;
    IL_IRGB *transparent;
    void XP_HUGE *mask;
    int validHeight;			/* number of valid scanlines */
    void *platformData;			/* used by platform-specific FE */
    Bool has_mask; 
	Bool hasUniqueColormap;
};


/* FE fixed image icons */
#define IL_IMAGE_FIRST	            0x11
#define IL_IMAGE_DELAYED	        0x11
#define IL_IMAGE_NOT_FOUND	        0x12
#define IL_IMAGE_BAD_DATA	        0x13
#define IL_IMAGE_INSECURE	        0x14
#define IL_IMAGE_EMBED		        0x15
#define IL_IMAGE_LAST		        0x15

#define IL_NEWS_FIRST				0x21
#define IL_NEWS_CATCHUP 			0x21
#define IL_NEWS_CATCHUP_THREAD 		0x22
#define IL_NEWS_FOLLOWUP 			0x23
#define IL_NEWS_GOTO_NEWSRC 		0x24
#define IL_NEWS_NEXT_ART 			0x25
#define IL_NEWS_NEXT_ART_GREY 		0x26
#define IL_NEWS_NEXT_THREAD 		0x27
#define IL_NEWS_NEXT_THREAD_GREY 	0x28
#define IL_NEWS_POST 				0x29
#define IL_NEWS_PREV_ART 			0x2A
#define IL_NEWS_PREV_ART_GREY		0x2B
#define IL_NEWS_PREV_THREAD 		0x2C
#define IL_NEWS_PREV_THREAD_GREY 	0x2D
#define IL_NEWS_REPLY 				0x2E
#define IL_NEWS_RTN_TO_GROUP 		0x2F
#define IL_NEWS_SHOW_ALL_ARTICLES 	0x30
#define IL_NEWS_SHOW_UNREAD_ARTICLES 0x31
#define IL_NEWS_SUBSCRIBE 			0x32
#define IL_NEWS_UNSUBSCRIBE 		0x33
#define IL_NEWS_FILE				0x34
#define IL_NEWS_FOLDER				0x35
#define IL_NEWS_FOLLOWUP_AND_REPLY	0x36
#define IL_NEWS_LAST				0x36

#define IL_GOPHER_FIRST		        0x41
#define IL_GOPHER_TEXT	         	0x41
#define IL_GOPHER_IMAGE	         	0x42
#define IL_GOPHER_BINARY        	0x43
#define IL_GOPHER_SOUND	        	0x44
#define IL_GOPHER_MOVIE	        	0x45
#define IL_GOPHER_FOLDER        	0x46
#define IL_GOPHER_SEARCHABLE	    0x47
#define IL_GOPHER_TELNET	        0x48
#define IL_GOPHER_UNKNOWN       	0x49
#define IL_GOPHER_LAST	        	0x49

#define IL_EDIT_FIRST               0x60
#define IL_EDIT_NAMED_ANCHOR        0x61
#define IL_EDIT_FORM_ELEMENT        0x62
#define IL_EDIT_UNSUPPORTED_TAG     0x63
#define IL_EDIT_UNSUPPORTED_END_TAG 0x64
#define IL_EDIT_JAVA                0x65
#define IL_EDIT_PLUGIN              0x66
#define IL_EDIT_LAST                0x66

/* Security Advisor and S/MIME icons */
#define IL_SA_FIRST					0x70
#define IL_SA_SIGNED				0x71
#define IL_SA_ENCRYPTED				0x72
#define IL_SA_NONENCRYPTED			0x73
#define IL_SA_SIGNED_BAD			0x74
#define IL_SA_ENCRYPTED_BAD			0x75
#define IL_SMIME_ATTACHED			0x76
#define IL_SMIME_SIGNED				0x77
#define IL_SMIME_ENCRYPTED			0x78
#define IL_SMIME_ENC_SIGNED			0x79
#define IL_SMIME_SIGNED_BAD			0x7A
#define IL_SMIME_ENCRYPTED_BAD		0x7B
#define IL_SMIME_ENC_SIGNED_BAD		0x7C
#define IL_SA_LAST					0x7C


XP_BEGIN_PROTOS

extern NET_StreamClass * IL_NewStream (FO_Present_Types format_out,
                                       void *data_obj, URL_Struct *URL_s,
                                       MWContext *context);

extern NET_StreamClass * IL_ViewStream (FO_Present_Types format_out,
                                        void *data_obj, URL_Struct *URL_s,
                                        MWContext *context);

extern int IL_GetImage(const char* url, MWContext* client,
                       LO_ImageStruct *clientData, NET_ReloadMethod reload);

extern Bool IL_FreeImage (MWContext *context, IL_Image *portableImage,
                          LO_ImageStruct *lo_image);

extern Bool IL_ReplaceImage (MWContext *context, LO_ImageStruct *new_lo_image,
                             LO_ImageStruct *old_lo_image);

extern IL_IRGB *IL_UpdateColorMap(MWContext *context, IL_IRGB *in,
                                  int in_colors, int free_colors,
                                  int *out_colors, IL_IRGB *transparent);
extern int IL_RealizeDefaultColormap(MWContext *context, IL_IRGB *bgcolor,
                                     int max_colors);

extern int IL_EnableTrueColor(MWContext *context, int bitsperpixel,
                              int rshift, int gshift, int bshift,
                              int rbits, int gbits, int bbits,
                              IL_IRGB *transparent,
                              int wild_wacky_rounding);
extern int IL_GreyScale(MWContext *context, int depth, IL_IRGB *transparent);
extern int IL_InitContext(MWContext *cx);

extern void IL_SetPreferences(MWContext *context, Bool incrementalDisplay,
                              IL_DitherMode dither_mode);

extern void IL_StartPage(MWContext *context);
extern void IL_EndPage(MWContext *context);

extern void IL_DeleteImages(MWContext *cx);

extern void IL_SamePage(MWContext *cx);

extern int IL_Type(const char *buf, int32 len);

extern void IL_DisableScaling(MWContext *cx);

extern void IL_DisableLowSrc(MWContext *cx);

extern void IL_SetByteOrder(MWContext *cx, Bool ls_bit_first,
                            Bool ls_byte_first);

extern int IL_SetTransparent(MWContext *cx, IL_IRGB *transparent);

extern void IL_NoMoreImages(MWContext *cx);

extern uint32 IL_ShrinkCache(void);
extern uint32 IL_GetCacheSize(void);
extern void IL_Shutdown(void);

extern Bool IL_PreferredStream(URL_Struct *urls);

extern void IL_UnCache(IL_Image *img);

extern int IL_ColormapTag(const char *image_url, MWContext* cx);

extern void IL_SetColorRenderMode(MWContext *cx,
                                  IL_ColorRenderMode color_render_mode);

extern void IL_ReduceImageCacheSizeTo(uint32 new_size);

extern int IL_DisplayMemCacheInfoAsHTML(FO_Present_Types format_out,
                                        URL_Struct *urls,
                                        MWContext *cx);
extern void IL_ScourContext(MWContext *cx);
extern void IL_SetImageCacheSize(uint32 new_size);

extern char *IL_HTMLImageInfo(char *url_address);
extern void IL_UseDefaultColormapThisPage(MWContext *cx);

extern void IL_InterruptContext(MWContext *cx);
extern Bool IL_AreThereLoopingImagesForContext(MWContext *cx);
XP_END_PROTOS

#endif /* _IL_H */



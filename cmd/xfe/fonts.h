/* -*- Mode: C; tab-width: 8 -*-
   fonts.h --- fonts, font selector, and related stuff
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Erik van der Poel <erik@netscape.com>, 6-Oct-95.
 */


#ifndef __xfe_fonts_h_
#define __xfe_fonts_h_

#include <lo_ele.h>
#include <X11/Xlib.h>


/*
 * defines
 */

#define FE_FONT_TYPE_X8		0x1
#define FE_FONT_TYPE_X16	0x2
#define FE_FONT_TYPE_GROUP	0x4


/* the following are macros so that we avoid the overhead of function calls */

#define FE_TEXT_EXTENTS(charset, font, string, len, fontAscent, fontDescent,  \
        overall)                                                              \
do                                                                            \
{                                                                             \
        int             direction;                                            \
        int             type;                                                 \
                                                                              \
        type = fe_CharSetInfoArray[(charset) & 0xff].type;                    \
                                                                              \
        if (type == FE_FONT_TYPE_X8)                                          \
        {                                                                     \
                XTextExtents((font), (string), (len), &direction,             \
                        (fontAscent), (fontDescent), (overall));              \
        }                                                                     \
        else if (type == FE_FONT_TYPE_X16)                                    \
        {                                                                     \
                XTextExtents16((font), (XChar2b *) (string), (len)/2,         \
                        &direction, (fontAscent), (fontDescent), (overall));  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
                (*fe_CharSetFuncsArray[fe_CharSetInfoArray[(charset) & 0xff]. \
                        info].textExtents)((font), (string), (len),           \
                        &direction, (fontAscent), (fontDescent), (overall));  \
        }                                                                     \
} while (0)


#define FE_FONT_EXTENTS(charset, font, fontAscent, fontDescent)               \
do                                                                            \
{                                                                             \
        fe_CharSetInfo  *info;                                                \
                                                                              \
        info = &fe_CharSetInfoArray[(charset) & 0xff];                        \
                                                                              \
        if (info->type == FE_FONT_TYPE_GROUP)                                 \
        {                                                                     \
                fe_GenericFontExtents(fe_CharSetFuncsArray[                   \
                        fe_CharSetInfoArray[(charset) & 0xff].info].          \
                        numberOfFonts, (font), (fontAscent), (fontDescent));  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
                *(fontAscent) = ((XFontStruct *) (font))->ascent;             \
                *(fontDescent) = ((XFontStruct *) (font))->descent;           \
        }                                                                     \
} while (0)


#define FE_DRAW_STRING(charset, dpy, d, font, gc, x, y, string, len)          \
do                                                                            \
{                                                                             \
        int  type;                                                            \
                                                                              \
        type = fe_CharSetInfoArray[(charset) & 0xff].type;                    \
                                                                              \
        if (type == FE_FONT_TYPE_X8)                                          \
        {                                                                     \
                XDrawString((dpy), (d), (gc), (x), (y), (string),             \
                        (len));                                               \
        }                                                                     \
        else if (type == FE_FONT_TYPE_X16)                                    \
        {                                                                     \
                XDrawString16((dpy), (d), (gc), (x), (y),                     \
                        (XChar2b *) (string), (len)/2);                       \
        }                                                                     \
        else                                                                  \
        {                                                                     \
                (*fe_CharSetFuncsArray[fe_CharSetInfoArray[(charset) & 0xff]. \
                        info].drawString)((dpy), (d), (font),                 \
                        (gc), (x), (y), (string), (len));                     \
        }                                                                     \
} while (0)


#define FE_DRAW_IMAGE_STRING(charset, dpy, d, font, gc, x, y, string, len)    \
do                                                                            \
{                                                                             \
        int  type;                                                            \
                                                                              \
        type = fe_CharSetInfoArray[(charset) & 0xff].type;                    \
                                                                              \
        if (type == FE_FONT_TYPE_X8)                                          \
        {                                                                     \
                XDrawImageString((dpy), (d), (gc), (x), (y), (string),        \
                        (len));                                               \
        }                                                                     \
        else if (type == FE_FONT_TYPE_X16)                                    \
        {                                                                     \
                XDrawImageString16((dpy), (d), (gc), (x), (y),                \
                        (XChar2b *) (string), (len)/2);                       \
        }                                                                     \
        else                                                                  \
        {                                                                     \
                (*fe_CharSetFuncsArray[fe_CharSetInfoArray[(charset) & 0xff]. \
                        info].drawImageString)((dpy), (d),                    \
                        (font), (gc), (x), (y), (string), (len));             \
        }                                                                     \
} while (0)


#define FE_SET_GC_FONT(charset, gc, font, flags)                              \
do                                                                            \
{                                                                             \
        fe_CharSetInfo  *info;                                                \
                                                                              \
        info = &fe_CharSetInfoArray[(charset) & 0xff];                        \
                                                                              \
        if (info->type != FE_FONT_TYPE_GROUP)                                 \
        {                                                                     \
                (gc)->font = ((XFontStruct *) (font))->fid;                   \
                *(flags) |= GCFont;                                           \
        }                                                                     \
} while (0)


/*
 * typedefs and structs
 */

typedef void *fe_Font;

typedef struct fe_FontFace fe_FontFace;

struct fe_FontFace
{
	fe_FontFace	*next;

	char		*longXLFDFontName;

	fe_Font		font;
};


typedef struct fe_FontSize fe_FontSize;

struct fe_FontSize
{
	fe_FontSize	*next;

	int		size;

	/* whether the user has selected this size in preferences */
	unsigned int	selected : 1;

	/* 0 = normal, 1 = bold, 2 = italic, 3 = boldItalic */
	fe_FontFace	*faces[4];

};


typedef struct fe_FontFamily
{
	/* displayed name of this family/foundry */
	char		*name;

	/* XLFD name of foundry */
	char		*foundry;

	/* XLFD name of family */
	char		*family;

	/* whether the user has selected this family in preferences */
	unsigned int	selected : 1;

	/* whether not to allow scaling for this family */
	unsigned int	allowScaling : 1;

	/* whether the htmlSizes have been computer or not */
	unsigned int	htmlSizesComputed : 1;

	/* one for each document size, 1-7 */
	fe_FontSize	*htmlSizes[7];

	/* selected size if scaled font */
	int		scaledSize;

	/* pointer to all pixel sizes within this family */
	fe_FontSize	*pixelSizes;

	/* allocated size of pixelSizes array */
	short		pixelAlloc;

	/* actual number of pixel sizes */
	short		numberOfPixelSizes;

	/* pointer to all point sizes within this family */
	fe_FontSize	*pointSizes;

	/* allocated size of pointSizes array */
	short		pointAlloc;

	/* actual number of point sizes */
	short		numberOfPointSizes;

} fe_FontFamily;


/* one for each pitch: proportional and fixed */
typedef struct fe_FontPitch
{
	/* pointer to size menu for this pitch */
	Widget		sizeMenu;

	/* pointer to all families of this charset/pitch */
	fe_FontFamily	*families;

	/* allocated size of families array */
	short		alloc;

	/* actual number of families */
	short		numberOfFamilies;

	/* selected family */
	fe_FontFamily	*selectedFamily;

} fe_FontPitch;


/* we have one of these per charset */
typedef struct fe_FontCharSet
{
	/* displayed name of this charset */
	char		*name;

	/* official MIME name of this charset */
	char		*mimeName;

	/* whether the user has selected this one in the preferences */
	unsigned int	selected : 1;

	/* the two pitches: 0 = proportional and 1 = fixed */
	fe_FontPitch	pitches[2];

} fe_FontCharSet;


typedef struct fe_CharSetInfo
{
	int16			charsetID;
	unsigned char		type;
	unsigned char		info;
} fe_CharSetInfo;


typedef fe_Font (*fe_LoadFontFunc)(MWContext *context, char *familyName,
	int sizeNum, int fontmask, int charset, int pitch, int faceNum,
	Display *dpy);
typedef void (*fe_TextExtentsFunc)(fe_Font font, char *string, int len,
	int *dir, int *fontAscent, int *fontDescent, XCharStruct *overall);
typedef void (*fe_DrawStringFunc)(Display *dpy, Drawable d, fe_Font font,
	GC gc, int x, int y, char *string, int len);
typedef void (*fe_DrawImageStringFunc)(Display *dpy, Drawable d, fe_Font font,
	GC gc, int x, int y, char *string, int len);

typedef struct fe_CharSetFuncs
{
	unsigned char		numberOfFonts;
	fe_LoadFontFunc		loadFont;
	fe_TextExtentsFunc	textExtents;
	fe_DrawStringFunc	drawString;
	fe_DrawImageStringFunc	drawImageString;
} fe_CharSetFuncs;


typedef struct fe_FontSettings fe_FontSettings;

struct fe_FontSettings
{
	fe_FontSettings	*next;
	char		*spec;
};


/*
 * public data
 */

extern fe_FontCharSet fe_FontCharSets[];

extern unsigned char fe_SortedFontCharSets[];

extern fe_CharSetInfo fe_CharSetInfoArray[];

extern fe_CharSetFuncs fe_CharSetFuncsArray[];


/*
 * public functions
 */

void fe_ComputeFontSizeTable(fe_FontFamily *family);

void fe_FreeFontGroups(void);

void fe_FreeFonts(void);

void fe_FreeFontSettings(fe_FontSettings *set);

void fe_FreeFontSizeTable(fe_FontFamily *family);

void fe_GenericFontExtents(int num, fe_Font font, int *fontAscent,
	int *fontDescent);

char *fe_GetFontCharSetSetting(void);

XtPointer fe_GetFont(MWContext *context, int sizeNum, int fontmask);

XtPointer fe_GetFontOrFontSet(MWContext *context, char *familyName,
			      int sizeNum, int fontmask, XmFontType *type);

XtPointer fe_GetFontOrFontSetFromFace(MWContext *context, LO_TextAttr *attr,
				      char *net_font_face, int sizeNum,
				      int fontmask, XmFontType *type);

fe_FontSettings *fe_GetFontSettings(void);

int fe_GetStrikePosition(int charset, fe_Font font);

int fe_GetUnderlinePosition(int charset);

void fe_InitFonts(Display *dpy);

fe_Font fe_LoadFont(MWContext *context, int16 *charset, char *familyName,
		    int size, int fontmask);

fe_Font fe_LoadFontFromFace(MWContext *context, LO_TextAttr *attr, int16 *charset,
			    char *net_font_face, int size, int fontmask);

void fe_ReadFontCharSet(char *charset);

void fe_ReadFontSpec(char *spec);

void fe_SetFontSettings(fe_FontSettings *set);


#endif /* __xfe_fonts_h_ */

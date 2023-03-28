/* -*- Mode: C; tab-width: 4 -*- */
#ifndef _NetscapeTypes_
#define _NetscapeTypes_

#include "xp_core.h"

/*
	netlib
*/
typedef int FO_Present_Types;
typedef struct URL_Struct_ URL_Struct;
typedef struct _NET_StreamClass NET_StreamClass;

/*
	imglib
*/

typedef struct il_container_struct il_container;
typedef struct il_container_list il_container_list;
typedef struct il_process_struct il_process;
typedef struct IL_IRGB_struct IL_IRGB;
typedef struct IL_RGB_struct IL_RGB;

/* Argument for IL_SetPreferences() */
typedef enum IL_DitherMode {
    ilClosestColor  = 0,
    ilDither        = 1,
    ilAuto          = 2
} IL_DitherMode;

/* Argument for IL_SetPreferences() */
typedef enum IL_ColorRenderMode {
    ilRGBColorCube,
    ilInstallPaletteAllowed,
    ilFixedPalette
} IL_ColorRenderMode;

typedef struct IL_Image_struct IL_Image;
typedef struct il_colorspace_struct il_colorspace;
typedef void* IL_ClientReq; /* is actually the LO_ImageStruct */
typedef enum IL_ImageStatus_ {
    ilStart,        /* No image size or data  */
	ilSetSize,	    /* redundant arg to FE_ImageSize */
	ilReset,	    /* signals the SRC portion of a LOWSRC/SRC is coming in */
	ilPartialData,	/* size & colormap & some data are valid */
	ilComplete	    /* the image is complete */
} IL_ImageStatus;

/* How to refill when there's a cache miss */
typedef enum NET_ReloadMethod
{
    NET_DONT_RELOAD,            /* use the cache */
    NET_RESIZE_RELOAD,          /* use the cache -- special for resizing */
    NET_NORMAL_RELOAD,          /* use IMS gets for reload */
    NET_SUPER_RELOAD,           /* retransfer everything */
    NET_CACHE_ONLY_RELOAD       /* Don't do anything if we miss in the cache.
                                   (For the image library) */
} NET_ReloadMethod;

/*
   plugins
*/
typedef struct _NPEmbeddedApp NPEmbeddedApp;

/*
	history
*/
typedef struct _History_entry History_entry;
typedef struct History_ History;

/*
  bookmarks (so shist.h doesn't have to include all of bkmks.h.)
*/

typedef struct BM_Entry_struct BM_Entry;

/*
	parser
*/
typedef struct _PA_Functions PA_Functions;
typedef struct PA_Tag_struct PA_Tag;

/*
	layout
*/
typedef union LO_Element_struct LO_Element;

typedef struct LO_AnchorData_struct LO_AnchorData;
typedef struct LO_Color_struct LO_Color;
typedef struct LO_TextAttr_struct LO_TextAttr;
typedef struct LO_TextInfo_struct LO_TextInfo;
typedef struct LO_TextStruct_struct LO_TextStruct;
typedef struct LO_ImageData_struct LO_ImageData;
typedef struct LO_ImageAttr_struct LO_ImageAttr;
typedef struct LO_ImageStruct_struct LO_ImageStruct;
typedef struct LO_SubDocStruct_struct LO_SubDocStruct;
typedef struct LO_EmbedStruct_struct LO_EmbedStruct;
typedef struct LO_JavaAppStruct_struct LO_JavaAppStruct;
typedef struct LO_EdgeStruct_struct LO_EdgeStruct;

typedef union LO_FormElementData_struct LO_FormElementData;

typedef struct lo_FormElementOptionData_struct lo_FormElementOptionData;
typedef struct lo_FormElementSelectData_struct lo_FormElementSelectData;
typedef struct lo_FormElementTextData_struct lo_FormElementTextData;
typedef struct lo_FormElementTextareaData_struct lo_FormElementTextareaData;
typedef struct lo_FormElementMinimalData_struct lo_FormElementMinimalData;
typedef struct lo_FormElementToggleData_struct lo_FormElementToggleData;
typedef struct lo_FormElementKeygenData_struct lo_FormElementKeygenData;

typedef struct LO_Any_struct LO_Any;
typedef struct LO_FormSubmitData_struct LO_FormSubmitData;
typedef struct LO_FormElementStruct_struct LO_FormElementStruct;
typedef struct LO_LinefeedStruct_struct LO_LinefeedStruct;
typedef struct LO_HorizRuleStruct_struct LO_HorizRuleStruct;
typedef struct LO_BulletStruct_struct LO_BulletStruct;
/* was misspelled as LO_BullettStruct */
#define LO_BullettStruct LO_BulletStruct
typedef struct LO_TableStruct_struct LO_TableStruct;
typedef struct LO_CellStruct_struct LO_CellStruct;
typedef struct LO_Position_struct LO_Position;
typedef struct LO_Selection_struct LO_Selection;
typedef struct LO_HitLineResult_struct LO_HitLineResult;
typedef struct LO_HitElementResult_struct LO_HitElementResult;
typedef union LO_HitResult_struct LO_HitResult;

/*
	XLation
*/
typedef struct PrintInfo_ PrintInfo;
typedef struct PrintSetup_ PrintSetup;

/*
	mother of data structures
*/
typedef struct MWContext_ MWContext;

/*
	Chrome structure
*/
typedef struct _Chrome Chrome;

/*
    Editor
*/
#include "edttypes.h"

typedef enum
{
  MWContextAny = -1,			/* Used as a noopt when searching for a context of a particular type */
  MWContextBrowser,				/* A web browser window */
  MWContextMail,				/* A mail reader window */
  MWContextNews,				/* A news reader window */
  MWContextMessageComposition,	/* A news-or-mail message editing window */
  MWContextSaveToDisk,			/* The placeholder window for a download */
  MWContextText,				/* non-window context for text conversion */
  MWContextPostScript,			/* non-window context for PS conversion */
  MWContextBiff,				/* non-window context for background mail
								   notification */
  MWContextJava,				/* non-window context for Java */
  MWContextBookmarks,			/* Context for the bookmarks */
  MWContextAddressBook,			/* Context for the addressbook */
  MWContextOleNetwork,			/* non-window context for the OLE network1 object */
  MWContextPrint,				/* non-window context for printing */
  MWContextDialog,				/* non-browsing dialogs. view-source/security */
  MWContextMetaFile,            /* non-window context for Windows metafile support */
  MWContextEditor,                               /* An Editor Window */
  MWContextHTMLHelp             /* HTML Help context to load map files */
} MWContextType;

#endif /* _NetscapeTypes_ */


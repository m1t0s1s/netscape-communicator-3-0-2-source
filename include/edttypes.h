/*
 *  File defines external editor types.
 *
 * These types are remapped internally to the editor.
*/

#ifndef _edt_types_h_
#define _edt_types_h_

#ifndef EDITOR_TYPES
#define ED_Element void
#define ED_Buffer void
#define ED_TagCursor void
#define ED_BitArray void
#endif

/*
 * Handle to Internal structure used for maintaining links.
*/
typedef struct ED_Link* ED_LinkId;
#define ED_LINK_ID_NONE     0

/* this id is passed to FE_GetImageData.. when it returns, we know to pass
 *  the call to EDT_SetImageData
*/
#define ED_IMAGE_LOAD_HACK_ID -10

typedef enum {
    ED_ELEMENT_SELECTION,       /* a selection instead of a single element */
    ED_ELEMENT_TEXT,
    ED_ELEMENT_IMAGE,
    ED_ELEMENT_HRULE,
    ED_ELEMENT_UNKNOWN_TAG,
    ED_ELEMENT_TARGET
} ED_ElementType;

typedef enum {
    ED_CARET_BEFORE = 0,
    ED_CARET_AFTER  = 1
} ED_CaretObjectPosition;

typedef enum {
    TF_NONE         = 0,
    TF_BOLD         = 1,
    TF_ITALIC       = 2,
    TF_FIXED        = 4,
    TF_SUPER        = 8,
    TF_SUB          = 0x10,
    TF_STRIKEOUT    = 0x20,
    TF_BLINK        = 0x40,
    TF_FONT_COLOR   = 0x80,     /* set if font has color */
    TF_FONT_SIZE    = 0x100,    /* set if font has size */
    TF_HREF         = 0x200,
    TF_SERVER       = 0x400,
    TF_SCRIPT       = 0x800,
    TF_UNDERLINE    = 0x1000,
    TF_FONT_FACE    = 0x2000
} ED_ETextFormat;

typedef int16 ED_TextFormat;

/*
 * The names here are confusing, and have a historical basis that is
 * lost in the mists of time. The trouble is that the "ED_ALIGN_CENTER"
 * tag is really "abscenter", while the ED_ALIGN_ABSCENTER tag is
 * really "center". (and the same for the TOP and BOTTOM tags.)
 *
 * Someday, if we have a lot of spare time we could switch the names. 
 */

/* CLM: Swapped bottom and center tags -- should match latest extensions now?
 *      Note: BASELINE is not written out (this is default display mode)
*/
typedef enum {
    ED_ALIGN_DEFAULT        = -1,
    ED_ALIGN_CENTER         = 0,    /* abscenter    */
    ED_ALIGN_LEFT           = 1,    /* left         */
    ED_ALIGN_RIGHT          = 2,    /* right        */
    ED_ALIGN_TOP            = 3,    /* texttop      */
    ED_ALIGN_BOTTOM         = 4,    /* absbottom    */
    ED_ALIGN_BASELINE       = 5,    /* baseline     */
    ED_ALIGN_ABSCENTER      = 6,    /* center       */
    ED_ALIGN_ABSBOTTOM      = 7,    /* bottom       */
    ED_ALIGN_ABSTOP         = 8     /* top          */
} ED_Alignment;

#if 0
typedef enum {
    ED_ALIGN_DEFAULT        = -1,
    ED_ALIGN_ABSCENTER      = 0,  */  /* abscenter     SHOULD BE THIS   */
    ED_ALIGN_LEFT           = 1,    /* left         */
    ED_ALIGN_RIGHT          = 2,    /* right        */
    ED_ALIGN_TOP            = 3,    /* texttop      */
    ED_ALIGN_ABSBOTTOM      = 4, */   /* absbottom      SHOULD BE THIS  */
    ED_ALIGN_BASELINE       = 5,    /* baseline     */
    ED_ALIGN_CENTER         = 6, */   /* center  (NCSA) SHOULD BE THIS*/
    ED_ALIGN_BOTTOM         = 7, */   /* bottom  (NCSA)   SHOULD BE THIS*/
    ED_ALIGN_ABSTOP         = 8     /* top          */
} ED_Alignment;
#endif
/*--------------------------- HREF --------------------------------*/

struct _EDT_HREFData {
    char *pURL;
    char *pExtra;
};

typedef struct _EDT_HREFData EDT_HREFData;

/*--------------------------- Image --------------------------------*/

struct _EDT_ImageData {
    XP_Bool bIsMap;        
    char *pUseMap;              /* created with XP_ALLOC() */
    ED_Alignment align;
    char *pSrc;         
    char *pLowSrc;
    char *pName;
    char *pAlt;
    int32  iWidth;
    int32  iHeight;
    XP_Bool bWidthPercent;         /* Range: 1 - 100 if TRUE, else = pixels (default) */
    XP_Bool bHeightPercent;
    int32 iHSpace;
    int32 iVSpace;
    int32 iBorder;
/* Added by CLM: */
    int32  iOriginalWidth;        /* Width and Height we got on initial loading */
    int32  iOriginalHeight;
    EDT_HREFData *pHREFData;
    char *pExtra;
};

typedef struct _EDT_ImageData EDT_ImageData;

/*--------------------------- Character  --------------------------------*/
struct _EDT_CharacterData {
    ED_TextFormat mask;             /* bits to set or get */
    ED_TextFormat values;           /* values of the bits in the mask */
    LO_Color *pColor;               /* color if mask bit is set */
    int32 iSize;                    /* size if mask bit is set */
    EDT_HREFData *pHREFData;        /* href if mask bit is set */
    ED_LinkId linkId;               /* internal use only */
};

typedef struct _EDT_CharacterData EDT_CharacterData;

/*--------------------------- Horizonal Rule --------------------------------*/

struct _EDT_HorizRuleData {
    ED_Alignment align;         /* only allows left and right alignment */
    int32  size;                  /* value 1 to 100 indicates line thickness */     
    int32  iWidth;                /* CM: default = 100% */
    XP_Bool bWidthPercent;         /* Range: 1 - 100 if TRUE(default), else = pixels */
    XP_Bool bNoShade;              
    char *pExtra;
};

typedef struct _EDT_HorizRuleData EDT_HorizRuleData;

/*--------------------------- ContainerData --------------------------------*/

struct _EDT_ContainerData {
    ED_Alignment align;         /* only allows left and right alignment */
};

typedef struct _EDT_ContainerData EDT_ContainerData;

/*--------------------------- TableData --------------------------------*/

struct _EDT_TableData {
    ED_Alignment align; /* ED_ALIGN_LEFT, ED_ALIGN_ABSCENTER, ED_ALIGN_RIGHT */
    ED_Alignment malign; /* margin alignment: ED_ALIGN_DEFAULT, ED_ALIGN_LEFT, ED_ALIGN_RIGHT */
    int32 iRows;
    int32 iColumns;
    int32 iBorderWidth;
    int32 iCellSpacing;
    int32 iCellPadding;
    XP_Bool bWidthDefined;
    XP_Bool bWidthPercent;
    int32 iWidth;
    XP_Bool bHeightDefined;
    XP_Bool bHeightPercent;
    int32 iHeight;
    LO_Color *pColorBackground;     /* null in the default case */
    char *pExtra;
};

typedef struct _EDT_TableData EDT_TableData;

/*--------------------------- TableCaptionData --------------------------------*/

struct _EDT_TableCaptionData {
    ED_Alignment align;
    char *pExtra;
};

typedef struct _EDT_TableCaptionData EDT_TableCaptionData;

/*--------------------------- TableRowData --------------------------------*/

struct _EDT_TableRowData {
    ED_Alignment align;
    ED_Alignment valign;
    LO_Color *pColorBackground;     /* null in the default case */
    char *pExtra;
};

typedef struct _EDT_TableRowData EDT_TableRowData;

/*--------------------------- TableCellData --------------------------------*/

struct _EDT_TableCellData {
    ED_Alignment align;
    ED_Alignment valign;
    int32 iColSpan;
    int32 iRowSpan;
    XP_Bool bHeader; /* TRUE == th, FALSE == td */
    XP_Bool bNoWrap;
    XP_Bool bWidthDefined;
    XP_Bool bWidthPercent;
    int32 iWidth;
    XP_Bool bHeightDefined;
    XP_Bool bHeightPercent;
    int32 iHeight;
    LO_Color *pColorBackground;     /* null in the default case */
    char *pExtra;
};

typedef struct _EDT_TableCellData EDT_TableCellData;

/*--------------------------- MultiColumnData --------------------------------*/

struct _EDT_MultiColumnData {
    int32 iColumns;
    char *pExtra;
};

typedef struct _EDT_MultiColumnData EDT_MultiColumnData;

/*--------------------------- Page Properties --------------------------------*/
struct _EDT_MetaData {
    XP_Bool bHttpEquiv;        /* true, http-equiv="fdsfds", false name="fdsfds" */
    char *pName;            /* http-equiv's or name's value */
    char *pContent;
};

typedef struct _EDT_MetaData EDT_MetaData;

struct _EDT_PageData {
    LO_Color *pColorBackground;     /* null in the default case */
    LO_Color *pColorLink;
    LO_Color *pColorText;
    LO_Color *pColorFollowedLink;
    LO_Color *pColorActiveLink;
    char *pBackgroundImage;
    char *pTitle;
    XP_Bool bKeepImagesWithDoc;
};

typedef struct _EDT_PageData EDT_PageData;

typedef enum {
    ED_COLOR_BACKGROUND,
    ED_COLOR_LINK,
    ED_COLOR_TEXT,
    ED_COLOR_FOLLOWED_LINK
} ED_EColor;

/*
 * CLM: Java and PlugIn data structures
*/
 struct _EDT_ParamData {
    char *pName;
    char *pValue;
};
typedef struct _EDT_ParamData EDT_ParamData;

typedef int32 EDT_ParamID;

struct _EDT_PlugInData {
    EDT_ParamID   ParamID;               /* Identifies which Param list is associated */
    char          *pSrc;
    XP_Bool          bHidden;
    ED_Alignment  align;
    int32           iWidth;
    int32           iHeight;
    XP_Bool          bWidthPercent;         /* Range: 1 - 100 if TRUE, else = pixels default) */
    XP_Bool          bHeightPercent;
    XP_Bool          bForegroundPalette;    /* PC systems only. For controling 256-color palette wars */
    int32           iHSpace;
    int32           iVSpace;
    int32           iBorder;
};
typedef struct _EDT_PlugInData EDT_PlugInData;

struct _EDT_JavaData {
    EDT_ParamID   ParamID;
    char          *pCode;
    char          *pCodebase;
    char          *pName;
    ED_Alignment  align;
    char          *pSrc;
    int32           iWidth;
    int32           iHeight;
    XP_Bool          bWidthPercent;         /* Range: 1 - 100 if TRUE, else = pixels default) */
    XP_Bool          bHeightPercent;
    int32           iHSpace;
    int32           iVSpace;
    int32           iBorder;
};
typedef struct _EDT_JavaData EDT_JavaData;


/* CLM: Error codes for file writing
 *      Return 0 if no error
 */
typedef enum {
    ED_ERROR_NONE,
    ED_ERROR_READ_ONLY,           /* File is marked read-only */
    ED_ERROR_BLOCKED,             /* Can't write at this time, edit buffer blocked */
    ED_ERROR_BAD_URL,             /* URL was not a "file:" type or no string */
    ED_ERROR_FILE_OPEN,
    ED_ERROR_FILE_WRITE,
    ED_ERROR_CREATE_BAKNAME,
    ED_ERROR_DELETE_BAKFILE,
    ED_ERROR_FILE_RENAME_TO_BAK,
    ED_ERROR_CANCEL,
    ED_ERROR_FILE_EXISTS,         /* We really didn't save -- file existed and no overwrite */
    ED_ERROR_SRC_NOT_FOUND
} ED_FileError;

typedef enum {
    ED_TAG_OK,
    ED_TAG_UNOPENED,
    ED_TAG_UNCLOSED,
    ED_TAG_UNTERMINATED_STRING,
    ED_TAG_PREMATURE_CLOSE,
    ED_TAG_TAGNAME_EXPECTED
} ED_TagValidateResult;

typedef enum {
    ED_LIST_TYPE_DEFAULT,
    ED_LIST_TYPE_DIGIT,         
    ED_LIST_TYPE_BIG_ROMAN,
    ED_LIST_TYPE_SMALL_ROMAN,
    ED_LIST_TYPE_BIG_LETTERS,
    ED_LIST_TYPE_SMALL_LETTERS,
    ED_LIST_TYPE_CIRCLE,
    ED_LIST_TYPE_SQUARE,
    ED_LIST_TYPE_DISC
} ED_ListType;


struct _EDT_ListData {
    /* This should be TagType, but there are problems with the include file dependencies. */
    int8 iTagType;              /* P_UNUM_LIST, P_NUM_LIST, P_BLOCKQUOTE, */
                                /* P_DIRECTOR, P_MENU, P_DESC_LIST */
    XP_Bool bCompact;
    ED_ListType eType;
    int32 iStart;                /* automatically maps, start is one */
    char *pExtra;
};

typedef struct _EDT_ListData EDT_ListData;

typedef enum {
    ED_BREAK_NORMAL,            /* just break the line, ignore images */
    ED_BREAK_LEFT,              /* break so it passes the image on the left */
    ED_BREAK_RIGHT,             /* break past the right image */
    ED_BREAK_BOTH               /* break past both images */
} ED_BreakType;

typedef enum {
    ED_SAVE_OVERWRITE_THIS,
    ED_SAVE_OVERWRITE_ALL,
    ED_SAVE_DONT_OVERWRITE_THIS,
    ED_SAVE_DONT_OVERWRITE_ALL,
    ED_SAVE_CANCEL
} ED_SaveOption;

typedef int32 EDT_ClipboardResult;
#define EDT_COP_OK 0
#define EDT_COP_DOCUMENT_BUSY 1
#define EDT_COP_SELECTION_EMPTY 2
#define EDT_COP_SELECTION_CROSSES_TABLE_DATA_CELL 3


#ifdef FIND_REPLACE

#define ED_FIND_FIND_ALL_WORDS  1       /* used to enumerate all words in a */
                                        /*  buffer */
#define ED_FIND_MATCH_CASE      2       /* default is to ignore case */
#define ED_FIND_REPLACE         4       /* call back the replace routine */
#define ED_FIND_WHOLE_BUFFER    8       /* start search from the top */
#define ED_FIND_REVERSE         0x10    /* reverse search from this point */

typedef intn ED_FindFlags;

typedef void (*EDT_PFReplaceFunc)( void *pMWContext, 
                                  char *pFoundWord, 
                                  char **pReplaceWord );

struct _EDT_FindAndReplaceData {
    char* pSearchString;
    ED_FindFlags fflags;
    EDT_PFReplaceFunc pfReplace;
};

typedef struct _EDT_FindAndReplaceData EDT_FindAndReplaceData;

#endif

#endif

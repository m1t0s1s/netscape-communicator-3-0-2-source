/* -*- Mode: C; tab-width: 4 -*- */
/*
 * This file is included by client.h
 *
 * It can be included by hand after mcom.h
 *
 * All intermodule data structures (i.e. MWContext, etc) should be included
 *  in this file
 */
#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "ntypes.h"
#include "xp_mcom.h"

#include "lo_ele.h"
#include "shistele.h"
#include "edttypes.h"
#ifdef JAVA
#include "prlong.h"
#include "prclist.h"
#endif /* JAVA */

/* ------------------------------------------------------------------------ */
/* ============= Typedefs for the global context structure ================ */

/* will come out of the ctxtfunc.h file eventually
 */
typedef struct _ContextFuncs ContextFuncs;
/*
 *   This stuff is front end specific.  Add whatever you need.
 */

#if defined(XP_MAC) && defined(__cplusplus)
class NetscapeContext;
class CHyperView;
#endif

#if defined(XP_WIN) && defined(__cplusplus)
class CAbstractCX;
class CEditView;
class CSaveProgress;
#endif

typedef struct FEstruct_ {
#ifndef MOZILLA_CLIENT
	void * generic_data;
#elif defined(XP_WIN)
#ifdef __cplusplus
    	CAbstractCX *cx;
#else
	void *cx;
#endif
#elif defined(XP_TEXT)
	int doc_cols;
	int doc_lines;
	int cur_top_line;
	int num_anchors;
	int cur_anchor;

#elif defined(XP_UNIX)
    struct fe_ContextData *data;
#elif defined(XP_MAC)
#ifdef __cplusplus
    class NetscapeContext*	realContext;
    class CHyperView*		view;
#else
	void*					realContext;
	void*					view;
#endif

/*
** These members are only used by the EDITOR... However, if they
** are removed for non-editor builds the MWContext structure
** becomes skewed for java (and the rest of DIST)...
*/
#ifdef __cplusplus
    class CEditView*		editview;
    class CSaveProgress*	savedialog;
#else
	void*					editview;
	void*					savedialog;
#endif

#endif
} FEstruct;

#define FEUNITS_X(x,context)   ((int32) ((MWContext *)context)->convertPixX * (x))
#define FEUNITS_Y(y,context)   ((int32) ((MWContext *)context)->convertPixY * (y))

/*
    This is a generic context structure.  There should be one context
    per document window.  The context will allow assorted modules to
    pull out things like the URL of the current document as well as
    giving them a place to hand their window specific stuff.

*/

struct MWContext_ {
    MWContextType type;

    char       *url;   /* URL of current document */
    char       * name;	/* name of this context */
    History      hist;  /* Info needed for the window history module */
    FEstruct     fe;    /* Front end specific stuff */
    int          fancyFTP;  /* use fancy ftp ? */
    int          fancyNews; /* use fancy news ? */
    int          intrupt;   /* the user just interrupted things */
    int          fileSort;  /* file sorting method */
    int          graphProgress; /* should the user get visual feedback */
    int          waitingMode;  /* Has a transfer been initiated?  Once a */
                 /* transfer is started, the user cannot select another */
                 /* anchor until either the transfer is aborted, has */
                 /* started to layout, or has been recognized as a */
                 /* separate document handled through an external stream/viewer */
    int          reSize;   /* the user wants to resize the window once the */
                 /* current transfer is over */
    char       * save_as_name;	/* name to save current file as */
    char       * title;		/* title (if supplied) of current document */
    Bool       is_grid_cell;	/* Is this a grid cell */
    struct MWContext_ *grid_parent;	/* pointer to parent of grid cell */
    XP_List *	 grid_children;	/* grid children of this context */
    int      convertPixX;	/* convert from pixels to fe-specific coords */
    int      convertPixY;	/* convert from pixels to fe-specific coords */
    ContextFuncs * funcs;       /* function table of display routines */
    PrintSetup	*prSetup;	/* Info about print job */
    PrintInfo	*prInfo;	/* State information for printing process */
    il_process 	*imageProcess;	/* how you like your images */
    il_colorspace *colorSpace;  /* format/colors for images */
    il_container_list *imageList;	/* images on page */
	int32		images;			/* # of distinct images on this page */
	int16		win_csid;		/* code set ID of current window	*/
	int16		doc_csid;		/* code set ID of current document	*/
	int16		relayout;		/* tell conversion to treat relayout case */
	char		*mime_charset;	/* MIME charset from URL			*/
    struct MSG_Frame *msgframe; /* Mail/news info for this context, if any. */
    struct MSG_CompositionFrame *msg_cframe; /* ditto. */
	struct MSG_BiffFrame *biff_frame; /* Biff info for this context, if any. */
    struct BM_Frame *bmframe; /* Bookmarks info for this context, if any. */
	NPEmbeddedApp *pluginList;	/* plugins on this page */
	void *pluginReconnect; /* yet another full screen hack */

    struct MimeDisplayData *mime_data;	/* opaque data used by MIME message
										   parser (not Mail/News specific,
										   since MIME objects can show up
										   in Browser windows too.) */

#ifdef MOCHA
    struct MochaContext *mocha_context;	/* opaque handle to Mocha state */
#endif
	NPEmbeddedApp *pEmbeddedApp; /* yet another full screen hack */
	char * defaultStatus;		/* What string to show in the status area
								   whenever there's nothing better to show.
								   FE's should implement FE_Progress so that
								   if it is passed NULL or "" it will instead
								   display this string.  libmsg changes this
								   string all the time for mail and news
								   contexts. */

	int32      doc_id;			/* unique identifier for generated documents */

#ifdef JAVA
	/* See ns/sun-java/netscape/net/netStubs.c for the next 2 items: */

	/*
    ** This mysterious list is used in two ways: (1) If you're a real
    ** window context, it's a list of all dummy java contexts that were
    ** created for java's network connections. (2) If you're a dummy java
    ** context, it's where you're linked into the list of connections for
    ** the real context:
    */
	PRCList		javaContexts;

	/*
    ** Second, if you're a dummy java context, you'll need a pointer to
    ** the stream data so that you can shut down the netlib connection:
    */
	struct nsn_JavaStreamData*	javaContextStreamData;

	/*
    ** Stuff for GraphProgress. See lj_embed.c
    */
	Bool		displayingMeteors;
	int64		timeOfFirstMeteorShower;
	int16		numberOfSimultaneousShowers;

#endif /* JAVA */

	/*
    ** Put the editor stuff at the end so that people can still use the
    ** the Java DLL from the 2.0 dist build with Navigator Gold.
    */
    PRPackedBool is_editor;       /* differentiates between Editor and Browser modes */
    PRPackedBool is_new_document; /* quick access to new doc state (unsaved-no file yet)*/
    PRPackedBool display_paragraph_marks; /* True if we should display paragraph and hard-return symbols. */
    PRPackedBool edit_view_source_hack;
    PRPackedBool edit_loading_url;  /* Hack to let us run the net while in a modal dialog */
    PRPackedBool edit_saving_url;   /*  " */
	PRPackedBool edit_has_backup;   /* Editor has made a session backup */
};


/* This tells libmime.a whether it has the mime_data slot in MWContext
   (which should always be true eventually, but having this #define here
   makes life easier for me today.)  -- jwz */
#define HAVE_MIME_DATA_SLOT


/* this is avialible even in non GOLD builds. */
#define EDT_IS_EDITOR(context)       (context != NULL && context->is_editor)
#define EDT_DISPLAY_PARAGRAPH_MARKS(context) (context && context->is_editor && context->display_paragraph_marks)

#ifdef JAVA

/*
** This macro is used to recover the MWContext* from the javaContexts
** list pointer:
*/
#define MWCONTEXT_PTR(context) \
    ((MWContext*) ((char*) (context) - offsetof(MWContext,javaContexts)))

#endif /* JAVA */

/* ------------------------------------------------------------------------ */
/* ====================== NASTY UGLY SHORT TERM HACKS ===================== */

#define XP_DOCID(ctxt)			((ctxt)->doc_id)
#define XP_SET_DOCID(ctxt, id)	((ctxt)->doc_id = (id))

/* ------------------------------------------------------------------------ */
/* ============= Typedefs for the parser module structures ================ */

/*
 * I *think* (but am unsure) that these should be forked off into a
 *   parser specific client level include file
 *
 */

typedef int8 TagType;

struct PA_Tag_struct {
    TagType type;
    PRPackedBool is_end;
    uint16 newline_count;
#if defined(XP_WIN)
    union {                 /* use an anonymous union for debugging purposes*/
    	PA_Block data;
        char* data_str;
    };
#else
    PA_Block data;
#endif
    int32 data_len;
    int32 true_len;
    void *lo_data;
    struct PA_Tag_struct *next;
    ED_Element *edit_element;
};

#define PA_HAS_PDATA( tag ) (tag->pVoid != 0 )

#ifdef XP_UNIX
typedef    char *PAAllocate (intn byte_cnt);
typedef    void  PAFree (char *ptr);
#else
typedef    void *PAAllocate (unsigned int byte_cnt);
typedef    void  PAFree (void *ptr);
#endif
typedef    intn  PA_OutputFunction (void *data_object, PA_Tag *tags, intn status);

struct _PA_Functions {
    PAAllocate  *mem_allocate;
    PAFree      *mem_free;
    PA_OutputFunction   *PA_ParsedTag;
};

typedef struct PA_InitData_struct {
    PA_OutputFunction   *output_func;
} PA_InitData;

/* Structure that defines the characteristics of a new window.
 * Each entry should be structured so that 0 should be the
 * default normal value.  Currently all 0 values
 * bring up a chromeless MWContextBrowser type window of
 * arbitrary size.
 */
struct _Chrome {
    MWContextType type;			/* Must be set to the correct type you want,
								 * if doesn't exist, define one!!!
								 */
	Bool show_button_bar;        /* TRUE to display button bar */
	Bool show_url_bar;           /* TRUE to show URL entry area */
 	Bool show_directory_buttons; /* TRUE to show directory buttons */
	Bool show_bottom_status_bar; /* TRUE to show bottom status bar */
	Bool show_menu;				 /* TRUE to show menu bar */
	Bool show_security_bar;		 /* TRUE to show security bar */
    int32 w_hint, h_hint;		 /* hints for width and height */
	Bool is_modal;				 /* TRUE to make window be modal */
	Bool show_scrollbar;		 /* TRUE to show scrollbars on window */
	Bool allow_resize;			 /* TRUE to allow resize of windows */
	Bool allow_close;			 /* TRUE to allow window to be closed */
	Bool copy_history;			 /* TRUE to copy history of prototype context*/
	void (* close_callback)(void *close_arg); /* called on window close */
	void *close_arg;			 /* passed to close_callback */
};

#endif /* _STRUCTS_H_ */






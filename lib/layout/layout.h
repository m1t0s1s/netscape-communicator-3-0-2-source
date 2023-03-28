#include "xp_core.h"
#include "net.h"

#define SCARY_LEAK_FIX 1
#ifdef MOCHA
#define MOCHA_CACHES_WRITES 1
#endif

#define MEMORY_ARENAS 1

#define NEXT_ELEMENT	state->top_state->element_id++


#define LINE_BUF_INC		200
#define FONT_HASH_SIZE		127
#define DEFAULT_BASE_FONT_SIZE	3

#define PRE_TEXT_NO		0
#define PRE_TEXT_YES		1
#define PRE_TEXT_WRAP		2

#define SUBDOC_NOT		0
#define SUBDOC_IS		1
#define SUBDOC_CELL		2
#define SUBDOC_CAPTION		3

#define LO_ALIGN_DEFAULT	-1
#define LO_ALIGN_CENTER		0
#define LO_ALIGN_LEFT		1
#define LO_ALIGN_RIGHT		2
#define LO_ALIGN_TOP		3
#define LO_ALIGN_BOTTOM		4
#define LO_ALIGN_BASELINE	5
#define LO_ALIGN_NCSA_CENTER	6
#define LO_ALIGN_NCSA_BOTTOM	7
#define LO_ALIGN_NCSA_TOP	8

#ifdef EDITOR
extern char* lo_alignStrings[];
#endif

#define	S_FORM_TYPE_TEXT	"text"
#define	S_FORM_TYPE_RADIO	"radio"
#define	S_FORM_TYPE_CHECKBOX	"checkbox"
#define	S_FORM_TYPE_HIDDEN	"hidden"
#define	S_FORM_TYPE_SUBMIT	"submit"
#define	S_FORM_TYPE_RESET	"reset"
#define	S_FORM_TYPE_PASSWORD	"password"
#define	S_FORM_TYPE_BUTTON	"button"
#define	S_FORM_TYPE_IMAGE	"image"
#define	S_FORM_TYPE_FILE	"file"
#define	S_FORM_TYPE_JOT		"jot"
#define	S_FORM_TYPE_READONLY	"readonly"

#define	S_AREA_SHAPE_DEFAULT	"default"
#define	S_AREA_SHAPE_RECT	"rect"
#define	S_AREA_SHAPE_CIRCLE	"circle"
#define	S_AREA_SHAPE_POLY	"poly"
#define	S_AREA_SHAPE_POLYGON	"polygon"	/* maps to AREA_SHAPE_POLY */

#define	AREA_SHAPE_UNKNOWN	0
#define	AREA_SHAPE_DEFAULT	1
#define	AREA_SHAPE_RECT		2
#define	AREA_SHAPE_CIRCLE	3
#define	AREA_SHAPE_POLY		4

extern LO_Color lo_master_colors[];

typedef struct lo_NameList_struct {
	int32 x, y;
	LO_Element *element;
	PA_Block name; 	/* really a (char *) */
	struct lo_NameList_struct *next;
} lo_NameList;

typedef struct lo_SavedFormListData_struct {
	XP_Bool valid;
	int32 data_index;
	int32 data_count;
	PA_Block data_list; 	/* really a (LO_FormElementData **) */
	struct lo_SavedFormListData_struct *next;
} lo_SavedFormListData;

typedef void (*lo_FreeProc)(MWContext*, void*);

typedef struct lo_EmbedDataElement {
	void *data;
	lo_FreeProc freeProc;
} lo_EmbedDataElement;

typedef struct lo_SavedEmbedListData_struct {
	int32 embed_count;
	PA_Block embed_data_list; 	/* really a (lo_EmbedDataElement **) */
} lo_SavedEmbedListData;

typedef struct lo_SavedGridData_struct {
	int32 main_width;
	int32 main_height;
	struct lo_GridRec_struct *the_grid;
} lo_SavedGridData;

typedef struct lo_MarginStack_struct {
	int32 margin;
	int32 y_min, y_max;
	struct lo_MarginStack_struct *next;
} lo_MarginStack;


typedef struct lo_FontStack_struct {
	int32 tag_type;
	LO_TextAttr *text_attr;
	struct lo_FontStack_struct *next;
} lo_FontStack;


typedef struct lo_AlignStack_struct {
	intn type;
	int32 alignment;
	struct lo_AlignStack_struct *next;
} lo_AlignStack;


typedef struct lo_ListStack_struct {
	intn level;
	int type;
	int32 value;
	Bool compact;
	int bullet_type;
	int32 old_left_margin;
	int32 old_right_margin;
	struct lo_ListStack_struct *next;
} lo_ListStack;


typedef struct lo_FormData_struct {
	intn id;
	PA_Block action;
	PA_Block encoding;
	PA_Block window_target;
	int32 method;
	int32 form_ele_cnt;
	int32 form_ele_size;
	PA_Block form_elements;		/* really a (LO_Element **) */
	struct lo_FormData_struct *next;
#ifdef MOCHA
	PA_Block name;
	struct MochaObject *mocha_object;
#endif
} lo_FormData;


typedef struct lo_TableCell_struct {
	Bool cell_done;
	Bool nowrap;
	Bool is_a_header;
	int32 percent_width, percent_height;
	int32 min_width, max_width;
	int32 height, baseline;
	int32 rowspan, colspan;
	LO_SubDocStruct *cell_subdoc;
	struct lo_TableCell_struct *next;
} lo_TableCell;


typedef struct lo_TableRow_struct {
	Bool row_done;
	Bool has_percent_width_cells;
	LO_Color *bg_color;	/* default for cells to inherit */
	int32 cells;
	int32 vert_alignment;
	int32 horiz_alignment;
	lo_TableCell *cell_list;
	lo_TableCell *cell_ptr;
	struct lo_TableRow_struct *next;
} lo_TableRow;


typedef struct lo_TableCaption_struct {
	int32 vert_alignment;
	int32 horiz_alignment;
	int32 min_width, max_width;
	int32 height;
	int32 rowspan, colspan;
	LO_SubDocStruct *subdoc;
} lo_TableCaption;


typedef struct lo_table_span_struct {
	int32 dim, min_dim;
	int32 span;
	struct lo_table_span_struct *next;
} lo_table_span;

typedef struct lo_TableRec_struct {
	intn draw_borders;
	Bool has_percent_width_cells;
	Bool has_percent_height_cells;
	LO_Color *bg_color;	/* default for cells to inherit */
	int32 rows, cols;
	int32 width, height;
	int32 inner_cell_pad;
	int32 inter_cell_pad;
	lo_table_span *width_spans;
	lo_table_span *width_span_ptr;
	lo_table_span *height_spans;
	lo_table_span *height_span_ptr;
	lo_TableCaption *caption;
	LO_TableStruct *table_ele;
	lo_TableRow *row_list;
	lo_TableRow *row_ptr;
} lo_TableRec;


typedef struct lo_MultiCol_struct {
	LO_Element *end_last_line;
	int32 start_ele;
	int32 start_line, end_line;
	int32 start_y, end_y;
	int32 start_x;
	int32 cols;
	int32 col_width;
	int32 orig_margin;
	int32 orig_min_width;
	int32 orig_display_blocking_element_y;
	int32 gutter;
	Bool orig_display_blocked;
	struct lo_MultiCol_struct *next;
} lo_MultiCol;


#define	LO_PERCENT_NORMAL	0
#define	LO_PERCENT_FREE		1
#define	LO_PERCENT_FIXED	2

typedef struct lo_GridPercent_struct {
	uint8 type;
	int32 value;
} lo_GridPercent;

typedef struct lo_GridHistory_struct {
	void *hist;		/* really a SHIST history pointer */
	int32 position;
} lo_GridHistory;

typedef struct lo_GridCellRec_struct {
	int32 x, y;
	int32 width, height;
	int32 margin_width, margin_height;
	intn width_percent;
	intn height_percent;
	char *url;
	char *name;
	MWContext *context;
	void *history;		/* really a SHIST history pointer */
#ifdef NEW_FRAME_HIST
	void *hist_list;        /* really a (XP_List *) */
	lo_GridHistory *hist_array;
	int32 hist_indx;
#endif /* NEW_FRAME_HIST */
	int8 scrolling;
	Bool resizable;
	Bool no_edges;
	int16 edge_size;
	int8 color_priority;
	LO_Color *border_color;
	struct lo_GridEdge_struct *side_edges[4]; /* top, bottom, left, right */
	struct lo_GridCellRec_struct *next;
	struct lo_GridRec_struct *subgrid;
} lo_GridCellRec;

typedef struct lo_GridEdge_struct {
	int32 x, y;
	int32 width, height;
	Bool is_vertical;
	int32 left_top_bound;
	int32 right_bottom_bound;
	int32 cell_cnt;
	int32 cell_max;
	lo_GridCellRec **cell_array;
	LO_EdgeStruct *fe_edge;
	struct lo_GridEdge_struct *next;
} lo_GridEdge;

typedef struct lo_GridRec_struct {
	int32 main_width, main_height;
#ifdef NEW_FRAME_HIST
	int32 current_hist, max_hist, hist_size;
#endif /* NEW_FRAME_HIST */
	int32 grid_cell_border;
	int32 grid_cell_min_dim;
	int32 rows, cols;
	lo_GridPercent *row_percents;
	lo_GridPercent *col_percents;
	lo_GridCellRec *cell_list;
	lo_GridCellRec *cell_list_last;
	int32 cell_count;
	Bool no_edges;
	int16 edge_size;
	int8 color_priority;
	LO_Color *border_color;
	lo_GridEdge *edge_list;
	struct lo_GridRec_struct *subgrid;
} lo_GridRec;


typedef struct lo_MapRec_struct {
	char *name;
	struct lo_MapAreaRec_struct *areas;
	struct lo_MapAreaRec_struct *areas_last;
	struct lo_MapRec_struct *next;
} lo_MapRec;

typedef struct lo_MapAreaRec_struct {
	int16 type;
	int16 alt_len;
	PA_Block alt;
	int32 *coords;
	int32 coord_cnt;
	LO_AnchorData *anchor;
	struct lo_MapAreaRec_struct *next;
} lo_MapAreaRec;

#ifdef HTML_CERTIFICATE_SUPPORT
typedef struct lo_Certificate_struct {
	PA_Block name;		/* really a (char *) */
	void *cert;		/* really a (char *) */
	struct lo_Certificate_struct *next;
} lo_Certificate;
#endif /* HTML_CERTIFICATE_SUPPORT */

typedef struct lo_TopState_struct 	lo_TopState;


#ifdef TEST_16BIT
#define XP_WIN16
#endif /* TEST_16BIT */
typedef struct lo_DocState_struct {
    lo_TopState *top_state;
    PA_Tag *subdoc_tags;	/* tags that created this subdoc */
    PA_Tag *subdoc_tags_end;	/* End of subdoc tags list */

    int32 base_x, base_y;	/* upper left of document */
    int32 x, y;			/* upper left of element in progress */
    int32 width;		/* width of element in progress */
    int32 win_width;
    int32 win_height;
    int32 max_width;		/* width of widest line so far. */
    int32 min_width;		/* minimum possible line width */
    int32 break_holder;		/* holder for last break position width */
    int32 line_num;		/* line number of the next line to be placed */
    intn linefeed_state;
    int8 preformatted;		/* processing preformatted (maybe wrap) text */
    Bool breakable;		/* Working on a breakable element */
    Bool delay_align;		/* in subdoc, delay aligning lines */
    Bool display_blocked;	/* do not display during layout */
    Bool in_paragraph;		/* inside an HTML 3.0 paragraph container */
    int32 display_blocking_element_id;
    int32 display_blocking_element_y;

    int32 win_top;
    int32 win_bottom;
    int32 win_left;
    int32 win_right;
    int32 left_margin;
    int32 right_margin;
    lo_MarginStack *left_margin_stack;
    lo_MarginStack *right_margin_stack;

    int32 text_divert;	/* the tag type of a text diverter (like P_TITLE) */

#ifdef XP_WIN16
    intn larray_array_size;
    XP_Block larray_array;	/* really a (XP_Block *) */
#endif /* XP_WIN16 */
    intn line_array_size;
    XP_Block line_array;	/* really a (LO_Element **) */
    LO_Element *line_list;	/* Element list for the current line so far */
    LO_Element *end_last_line;	/* Element at end of last line */

    lo_NameList *name_list;	/* list of positions of named anchors */

    LO_Element *float_list;	/* Floating element list for the current doc */

    intn base_font_size;	/* size of font at bottom of font stack */
    lo_FontStack *font_stack;

    lo_AlignStack *align_stack;

    lo_ListStack *list_stack;

    LO_TextInfo text_info;	/* for text of last text element created */
    /* this is a buffer for a single (currently processed) line of text */
    PA_Block line_buf;		/* lock to a (char *) */
    int32 line_buf_len;
    int32 line_buf_size;
    int32 baseline;
    int32 line_height;
    int32 default_line_height;
    int32 break_pos;		/* position in middle of current element */
    int32 break_width;
    Bool last_char_CR;
    Bool trailing_space;
    Bool at_begin_line;

    LO_TextStruct *old_break;
    int32 old_break_pos;	/* in middle of older element on this line */
    int32 old_break_width;

    PA_Block current_named_anchor;	/* lock to a (char *) */
    LO_AnchorData *current_anchor;

    intn cur_ele_type;
    LO_Element *current_ele;	/* element in process, no end tag yet */

    LO_JavaAppStruct *current_java; /* java applet in process, no end tag yet */

    lo_TableRec *current_table;
    lo_GridRec *current_grid;
    lo_MultiCol *current_multicol;

    Bool start_in_form;		/* Was in a form at start of (sub)doc */
    intn form_id;		/* first form entering this (sub)doc */
    int32 form_ele_cnt;		/* first form element entering this (sub)doc */
    int32 form_data_index;	/* first form data entering this (sub)doc */
    int32 embed_count_base;	/* number of embeds before beginning this (sub)doc */
    int32 url_count_base;	/* number of anchors before beginning this (sub)doc */

    Bool must_relayout_subdoc;
    Bool allow_percent_width;
    Bool allow_percent_height;
    intn is_a_subdoc;
    int32 current_subdoc;
    XP_Block sub_documents;	/* really a lo_DocState ** array */
    struct lo_DocState_struct *sub_state;

    LO_Element *current_cell;

    Bool extending_start;
    LO_Element *selection_start;
    int32 selection_start_pos;
    LO_Element *selection_end;
    int32 selection_end_pos;
    LO_Element *selection_new;
    int32 selection_new_pos;

    LO_Color text_fg;
    LO_Color text_bg;
    LO_Color anchor_color;
    LO_Color visited_anchor_color;
    LO_Color active_anchor_color;
#ifdef EDITOR
    ED_Element *edit_current_element;
    int edit_current_offset;
    Bool edit_force_offset;
    Bool edit_relayout_display_blocked;
#endif
#ifdef MOCHA
    Bool in_relayout;	/* true if we're in lo_RelayoutSubdoc */
#endif
} lo_DocState;

#ifdef TEST_16BIT
#undef XP_WIN16
#endif /* TEST_16BIT */

#define STATE_DEFAULT_FG_RED(state)		((state)->text_fg.red)
#define STATE_DEFAULT_FG_GREEN(state)		((state)->text_fg.green)
#define STATE_DEFAULT_FG_BLUE(state)		((state)->text_fg.blue)

#define STATE_DEFAULT_BG_RED(state)		((state)->text_bg.red)
#define STATE_DEFAULT_BG_GREEN(state)		((state)->text_bg.green)
#define STATE_DEFAULT_BG_BLUE(state)		((state)->text_bg.blue)

#define STATE_UNVISITED_ANCHOR_RED(state)	((state)->anchor_color.red)
#define STATE_UNVISITED_ANCHOR_GREEN(state)	((state)->anchor_color.green)
#define STATE_UNVISITED_ANCHOR_BLUE(state)	((state)->anchor_color.blue)

#define STATE_VISITED_ANCHOR_RED(state)		((state)->visited_anchor_color.red)
#define STATE_VISITED_ANCHOR_GREEN(state)	((state)->visited_anchor_color.green)
#define STATE_VISITED_ANCHOR_BLUE(state)	((state)->visited_anchor_color.blue)

#define STATE_SELECTED_ANCHOR_RED(state)	((state)->active_anchor_color.red)
#define STATE_SELECTED_ANCHOR_GREEN(state)	((state)->active_anchor_color.green)
#define STATE_SELECTED_ANCHOR_BLUE(state)	((state)->active_anchor_color.blue)


#ifdef MEMORY_ARENAS
typedef struct lo_arena_struct {
        struct lo_arena_struct *next;
        char *limit;
        char *avail;
} lo_arena;
#endif /* MEMORY_ARENAS */

/*
** This is just like SHIST_SavedData except the fields are more strictly
** typed.
*/
typedef struct lo_SavedData {
    lo_SavedFormListData*	FormList;	/* Data for all form elements in doc */
    lo_SavedEmbedListData*	EmbedList;	/* session data for all embeds and applets in doc */
    lo_SavedGridData*		Grid;		/* saved for grid history. */
#ifdef MOCHA
    PA_Block			OnLoad;		/* JavaScript onload event handler source */
    PA_Block			OnUnload;	/* JavaScript onunload event handler source */
    PA_Block			OnFocus;	/* JavaScript onfocus event handler source */
    PA_Block			OnBlur;		/* JavaScript onblur event handler source */
#endif
} lo_SavedData;

/*
 * Top level state
 */
struct lo_TopState_struct {
#ifdef MEMORY_ARENAS
    lo_arena *first_arena;
    lo_arena *current_arena;
#endif /* MEMORY_ARENAS */
    PA_Tag *tags;		/* Held tags while layout is blocked */
    PA_Tag **tags_end;		/* End of held tags list */
    LO_Element *backdrop_image;
    PRPackedBool have_a_bg_url;
    PRPackedBool doc_specified_bg;
    PRPackedBool nothing_displayed;
    PRPackedBool in_head;	/* Still in the HEAD of the HTML doc. */
    PRPackedBool in_body;	/* In the BODY of the HTML doc. */
    PRPackedBool scrolling_doc;	/* Is this a special scrolling doc (hack) */
    PRPackedBool have_title;	/* set by first <TITLE> */
    PRPackedBool in_form;	/* true if in <FORM>...</FORM> */
    char *unknown_head_tag;	/* ignore content in this case if non-NULL */
    char *base_target;		/* Base target of urls in this document */
    char *base_url;		/* Base url of this document */
    char *url;			/* Real url of this document */
    char *name_target;		/* The #name part of the url */
    int32 element_id;
    LO_Element *layout_blocking_element;
    intn layout_status;

    LO_EmbedStruct *embed_list;     /* embeds linked in reverse order */
    LO_JavaAppStruct *applet_list;	/* applets linked in reverse order */

    lo_MapRec *map_list;	/* list of maps in doc */
    lo_MapRec *current_map;	/* Current open MAP tag */

#ifdef MOCHA
    LO_ImageStruct *image_list;	/* list of images in doc */
    LO_ImageStruct *last_image;	/* last image in image_list */
    uint32 image_count;
#endif

    lo_FormData *form_list;	/* list of forms in doc */
    intn current_form_num;

    int32 embed_count;		/* master count of embeds in this document */

    lo_SavedData savedData;	/* layout data */

    int32 total_bytes;
    int32 current_bytes;
    int32 layout_bytes;
    int32 script_bytes;
    intn layout_percent;

    lo_GridRec *the_grid;
    lo_GridRec *old_grid;

    PRPackedBool is_grid;	/* once a grid is started, ignore other tags */
    PRPackedBool in_nogrids;	/* special way to ignore tags in <noframes> */
    PRPackedBool in_noembed;	/* special way to ignore tags in <noembed> */
    PRPackedBool in_noscript;	/* special way to ignore tags in <noscript> */
    PRPackedBool in_applet;	/* special way to ignore tags in java applet */
    PRPackedBool is_binary;	/* hack for images instead of HTML doc */
    PRPackedBool insecure_images; /* secure doc with insecure images */
    PRPackedBool out_of_memory;	/* ran out of memory mid-layout */
    NET_ReloadMethod
         force_reload;		/* URL_Struct reload flag */
    intn auto_scroll;		/* Chat auto-scrolling layout, # of lines */
    intn security_level;	/* non-zero for secure docs. */
    URL_Struct *nurl;		/* the netlib url struct */

    PA_Block font_face_array;	/* really a (char **) */
    intn font_face_array_len;
    intn font_face_array_size;

    XP_Block text_attr_hash;	/* really a (LO_TextAttr **) */

#ifdef XP_WIN16
    intn ulist_array_size;
    XP_Block ulist_array;	/* really a (XP_Block *) */
#endif /* XP_WIN16 */
    XP_Block url_list;		/* really a (LO_AnchorData **) PA_Block */
    intn url_list_len;
    intn url_list_size;

    LO_Element *recycle_list;	/* List of Elements to be reused */
    LO_Element *trash;		/* List of Elements to discard later */

    lo_DocState *doc_state;

#ifdef EDITOR
    ED_Buffer *edit_buffer;
#endif

  /* jwz: moved outside of MOCHA ifdef */
    PRPackedBool resize_reload;	/* is this a resize-induced reload? */

#ifdef MOCHA
    struct pa_DocData_struct *doc_data;
    struct lo_ScriptState *script_state;
    uint script_lineno;	/* parser newline count at last <SCRIPT> tag */
    int8 in_script;		/* script type if in script, see below */
    PRPackedBool mocha_mark;
    PRPackedBool mocha_has_onload;
    uint32 mocha_loading_applets_count;
    uint32 mocha_loading_embeds_count;
    PA_Tag **mocha_write_point;
    uint32 mocha_write_level;
#ifdef MOCHA_CACHES_WRITES
	NET_StreamClass *mocha_write_stream;
#endif
#endif

#ifdef HTML_CERTIFICATE_SUPPORT
    lo_Certificate *cert_list;	/* list of certificates in doc */
#endif
};


/* Script type codes, stored in top_state->in_script. */
#define SCRIPT_TYPE_NOT		0
#define SCRIPT_TYPE_MOCHA	1
#define SCRIPT_TYPE_UNKNOWN	2


/*
	Internal Prototypes
*/

extern lo_FontStack *lo_DefaultFont(lo_DocState *, MWContext *);
extern Bool lo_FilterTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_LayoutTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_SaveSubdocTags(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FreshText(lo_DocState *);
extern void lo_HardLineBreak(MWContext *, lo_DocState *, Bool);
extern void lo_SoftLineBreak(MWContext *, lo_DocState *, Bool, intn);
extern void lo_InsertWordBreak(MWContext *, lo_DocState *);
extern void lo_FormatText(MWContext *, lo_DocState *, char *);
extern void lo_PreformatedText(MWContext *, lo_DocState *, char *);
extern void lo_FlushLineBuffer(MWContext *, lo_DocState *);
extern void lo_RecolorText(lo_DocState *);
extern void lo_PushFont(lo_DocState *, intn, LO_TextAttr *);
extern LO_TextAttr *lo_PopFont(lo_DocState *, intn);
extern void lo_PopAllAnchors(lo_DocState *);
extern void lo_FlushLineList(MWContext *, lo_DocState *, Bool);
extern LO_LinefeedStruct *lo_NewLinefeed(lo_DocState *, MWContext *);
extern void lo_HorizontalRule(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FormatImage(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FormatEmbed(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FormatJavaApp(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_JavaAppParam(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_CloseJavaApp(MWContext *, lo_DocState *, LO_JavaAppStruct *);
extern void lo_ProcessSpacerTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_ProcessBodyTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_BodyBackground(MWContext *, lo_DocState *, PA_Tag *, Bool);
extern void lo_BodyForeground(MWContext *, lo_DocState *, LO_ImageStruct *);
extern int32 lo_ValueOrPercent(char *, Bool *);
extern void lo_BreakOldElement(MWContext *, lo_DocState *);
extern void lo_CopyTextAttr(LO_TextAttr *, LO_TextAttr *);
extern LO_TextAttr *lo_FetchTextAttr(lo_DocState *, LO_TextAttr *);
extern void lo_FindLineMargins(MWContext *, lo_DocState *);
extern void lo_AddMarginStack(lo_DocState *, int32, int32, int32, int32,
				int32, int32, int32, intn);
extern LO_AnchorData *lo_NewAnchor(lo_DocState *, PA_Block, PA_Block);
extern void lo_DestroyAnchor(LO_AnchorData *anchor_data);
extern void lo_AddToUrlList(MWContext *, lo_DocState *, LO_AnchorData *);
extern void lo_AddEmbedData(MWContext *, void *, lo_FreeProc, int32);
extern lo_ListStack *lo_DefaultList(lo_DocState *);
extern void lo_PushList(lo_DocState *, PA_Tag *);
extern lo_ListStack *lo_PopList(lo_DocState *, PA_Tag *);

extern void lo_PushAlignment(lo_DocState *, PA_Tag *, int32);
extern lo_AlignStack *lo_PopAlignment(lo_DocState *);

extern lo_TopState *lo_NewTopState(MWContext *, char *);
extern lo_DocState *lo_NewLayout(MWContext *, int32, int32, int32, int32,
				lo_DocState*);
extern lo_SavedFormListData *lo_NewDocumentFormListData(void);
extern Bool lo_StoreTopState(int32, lo_TopState *);
extern lo_TopState *lo_FetchTopState(int32);
extern lo_DocState *lo_TopSubState(lo_TopState *);
extern lo_DocState *lo_CurrentSubState(lo_DocState *);

extern void lo_InheritParentState(lo_DocState *, lo_DocState *);
extern void lo_NoMoreImages(MWContext *);
extern void lo_FinishImage(MWContext *, lo_DocState *, LO_ImageStruct *);
extern void lo_BlockedImageLayout(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_PartialFormatImage(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FlushBlockage(MWContext *, lo_DocState *, lo_DocState *);
extern void lo_FinishLayout(MWContext *, lo_DocState *, int);
extern void lo_FreeLayoutData(MWContext *, lo_DocState *);
struct pa_DocData_struct;
extern LO_Element *lo_InternalDiscardDocument(MWContext *, lo_DocState *, struct pa_DocData_struct *, Bool bWholeDoc);
#ifdef SCARY_LEAK_FIX
extern void lo_FreePartialTable(MWContext *, lo_DocState *, lo_TableRec *);
extern void lo_FreePartialSubDoc(MWContext *, lo_DocState *, LO_SubDocStruct *);
#endif /* SCARY_LEAK_FIX */
extern void lo_FreeElement(MWContext *context, LO_Element *, Bool);
extern void lo_FreeImageAttributes(LO_ImageAttr *image_attr);
extern void lo_ScrapeElement(MWContext *context, LO_Element *);
extern int32 lo_FreeRecycleList(MWContext *context, LO_Element *);
extern void lo_FreeGridRec(lo_GridRec *);
extern void lo_FreeGridCellRec(MWContext *, lo_GridCellRec *);
extern void lo_FreeGridEdge(lo_GridEdge *);
extern void lo_FreeFormElementData(LO_FormElementData *element_data);

#ifdef MEMORY_ARENAS
extern void lo_InitializeMemoryArena(lo_TopState *);
extern int32 lo_FreeMemoryArena(lo_arena *);
extern char *lo_MemoryArenaAllocate(lo_TopState *, int32);
#endif /* MEMORY_ARENAS */

extern char *lo_FetchFontFace(MWContext *, lo_DocState *, char *);
extern PA_Block lo_FetchParamValue(MWContext *, PA_Tag *, char *);
extern PA_Block lo_ValueToAlpha(int32, Bool, intn *);
extern void lo_PlaceBulletStr(MWContext *, lo_DocState *);
extern void lo_PlaceBullet (MWContext *, lo_DocState *);
extern PA_Block lo_ValueToRoman(int32, Bool, intn *);
extern void lo_ConvertAllValues(MWContext *, char **, int32, uint);
extern void lo_CloseOutLayout(MWContext *, lo_DocState *);
extern void lo_CloseOutTable(MWContext *, lo_DocState *);
extern void lo_ShiftMarginsUp(MWContext *, lo_DocState *, int32);
extern void lo_ClearToLeftMargin(MWContext *, lo_DocState *);
extern void lo_ClearToRightMargin(MWContext *, lo_DocState *);
extern void lo_ClearToBothMargins(MWContext *, lo_DocState *);
extern void lo_BeginForm(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_ProcessInputTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_FreeDocumentGridData(MWContext *, lo_SavedGridData *);
extern void lo_FreeDocumentEmbedListData(MWContext *, lo_SavedEmbedListData *);
extern void lo_FreeDocumentFormListData(MWContext *, lo_SavedFormListData *);
extern void lo_SaveFormElementState(MWContext *, lo_DocState *, Bool);
extern void lo_BeginTextareaTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndTextareaTag(MWContext *, lo_DocState *, PA_Block );
extern void lo_BeginOptionTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndOptionTag(MWContext *, lo_DocState *, PA_Block );
extern void lo_BeginSelectTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndSelectTag(MWContext *, lo_DocState *);
extern void lo_ProcessKeygenTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_BeginSubDoc(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndSubDoc(MWContext *, lo_DocState *,lo_DocState *,LO_Element *);
extern void lo_BeginCell(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndCell(MWContext *, lo_DocState *, LO_Element *);
extern void lo_RecreateGrid(MWContext *, lo_DocState *, lo_GridRec *);
extern void lo_BeginGrid(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndGrid(MWContext *, lo_DocState *, lo_GridRec *);
extern void lo_BeginGridCell(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_BeginSubgrid(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndSubgrid(MWContext *, lo_DocState *, lo_GridRec *);
extern void lo_BeginMap(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndMap(MWContext *, lo_DocState *, lo_MapRec *);
extern lo_MapRec *lo_FreeMap(lo_MapRec *);
extern void lo_BeginMapArea(MWContext *, lo_DocState *, PA_Tag *);
extern lo_MapRec *lo_FindNamedMap(MWContext *, lo_DocState *, char *);
extern LO_AnchorData *lo_MapXYToAnchorData(MWContext *, lo_DocState *,
				lo_MapRec *, int32, int32);
extern void lo_BeginMulticolumn(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndMulticolumn(MWContext *, lo_DocState *, PA_Tag *,
				lo_MultiCol *);
extern void lo_BeginTable(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndTable(MWContext *, lo_DocState *, lo_TableRec *);
extern void lo_BeginTableRow(MWContext *, lo_DocState *,lo_TableRec *,PA_Tag *);
extern void lo_EndTableRow(MWContext *, lo_DocState *, lo_TableRec *);
extern void lo_BeginTableCell(MWContext *, lo_DocState *, lo_TableRec *,
				PA_Tag *, Bool);
extern void lo_EndTableCell(MWContext *, lo_DocState *);
extern void lo_BeginTableCaption(MWContext *, lo_DocState *, lo_TableRec *,
				PA_Tag *);
extern void lo_EndTableCaption(MWContext *, lo_DocState *);
extern int32 lo_GetSubDocBaseline(LO_SubDocStruct *);
extern LO_CellStruct *lo_CaptureLines(MWContext *, lo_DocState *,
				int32, int32, int32, int32, int32);
extern LO_CellStruct *lo_SquishSubDocToCell(MWContext *, lo_DocState *,
				LO_SubDocStruct *);
extern LO_Element *lo_JumpCellWall(MWContext *, lo_DocState *, LO_Element *);
extern void lo_RenumberCell(lo_DocState *, LO_CellStruct *);
extern void lo_ShiftCell(LO_CellStruct *, int32, int32);
extern int32 lo_CleanTextWhitespace(char *, int32);
extern void lo_ProcessIsIndexTag(MWContext *, lo_DocState *, PA_Tag *);
extern void lo_EndForm(MWContext *, lo_DocState *);
extern void lo_RecycleElements(MWContext *, lo_DocState *, LO_Element *);
extern void lo_relayout_recycle(MWContext *context, lo_DocState *state,
                LO_Element *elist);
extern LO_Element *lo_NewElement(MWContext *, lo_DocState *, intn,
				ED_Element *, intn edit_offset);
extern void lo_AppendToLineList(MWContext *, lo_DocState *, LO_Element *,int32);
extern void lo_SetLineArrayEntry(lo_DocState *, LO_Element *, int32);
extern LO_Element *lo_FirstElementOfLine(MWContext *, lo_DocState *, int32);
extern void lo_ClipLines(MWContext *, lo_DocState *, int32);
extern void lo_SetDefaultFontAttr(lo_DocState *, LO_TextAttr *, MWContext *);
extern intn lo_EvalAlignParam(char *str, Bool *floating);
extern intn lo_EvalVAlignParam(char *str);
extern intn lo_EvalCellAlignParam(char *str);
extern intn lo_EvalDivisionAlignParam(char *str);
extern PA_Block lo_ConvertToFELinebreaks(char *, int32, int32 *);
extern PA_Block lo_FEToNetLinebreaks(PA_Block);
extern void lo_CleanFormElementData(LO_FormElementData *);
extern Bool lo_SetNamedAnchor(lo_DocState *, PA_Block, LO_Element *);
extern void lo_AddNameList(lo_DocState *, lo_DocState *);
extern int32 lo_StripTextWhitespace(char *, int32);
extern int32 lo_StripTextNewlines(char *, int32);
extern int32 lo_ResolveInputType(char *);
extern Bool lo_IsValidTarget(char *);
extern Bool lo_IsAnchorInDoc(lo_DocState *, char *);

extern void lo_DisplaySubtext(MWContext *, int, LO_TextStruct *,
	int32, int32, Bool);
extern void lo_DisplayText(MWContext *, int, LO_TextStruct *, Bool);
extern void lo_DisplayImage(MWContext *, int, LO_ImageStruct *);
extern void lo_DisplayEmbed(MWContext *, int, LO_EmbedStruct *);
extern void lo_DisplayJavaApp(MWContext *, int, LO_JavaAppStruct *);
extern void lo_ClipImage(MWContext *, int, LO_ImageStruct *,
	int32, int32, uint32, uint32);
extern void lo_DisplaySubImage(MWContext *, int, LO_ImageStruct *,
	int32, int32, uint32, uint32);
extern void lo_DisplaySubDoc(MWContext *, int, LO_SubDocStruct *);
extern void lo_DisplayCell(MWContext *, int, LO_CellStruct *);
extern void lo_DisplayCellContents(MWContext *, int, LO_CellStruct *,
	int32, int32);
extern void lo_DisplayEdge(MWContext *, int, LO_EdgeStruct *);
extern void lo_DisplayTable(MWContext *, int, LO_TableStruct *);
extern void lo_DisplayLineFeed(MWContext *, int, LO_LinefeedStruct *, Bool);
extern void lo_DisplayHR(MWContext *, int, LO_HorizRuleStruct *);
extern void lo_DisplayBullet(MWContext *, int, LO_BullettStruct *);
extern void lo_DisplayFormElement(MWContext *, int, LO_FormElementStruct *);
extern void LO_HighlightSelection(MWContext *, Bool);

extern void lo_RefreshDocumentArea(MWContext *, lo_DocState *,
	int32, int32, uint32, uint32);
extern intn lo_GetCurrentGridCellStatus(lo_GridCellRec *);
extern char *lo_GetCurrentGridCellUrl(lo_GridCellRec *);
extern void lo_GetGridCellMargins(MWContext *, int32 *, int32 *);
extern LO_Element *lo_XYToGridEdge(MWContext *, lo_DocState *, int32, int32);
extern LO_Element *lo_XYToDocumentElement(MWContext *, lo_DocState *,
	int32, int32, Bool, Bool, int32 *, int32 *);
extern LO_Element *lo_XYToDocumentElement2(MWContext *, lo_DocState *,
	int32, int32, Bool, Bool, Bool, int32 *, int32 *);
extern LO_Element *lo_XYToCellElement(MWContext *, lo_DocState *,
	LO_CellStruct *, int32, int32, Bool, Bool);
extern LO_Element *lo_XYToNearestCellElement(MWContext *, lo_DocState *,
	LO_CellStruct *, int32, int32);
extern void lo_RegionToLines (MWContext *, lo_DocState *, int32 x, int32 y,
	uint32 width, uint32 height, Bool dontCrop,
	int32* topLine, int32* bottomLine);
extern int32 lo_PointToLine (MWContext *, lo_DocState *, int32 x, int32 y);
extern void lo_GetLineEnds(MWContext *, lo_DocState *,
	int32 line, LO_Element** retBegin, LO_Element** retEnd);
extern void lo_CalcAlignOffsets(lo_DocState *, LO_TextInfo *, intn,
	int32, int32, int16 *, int32 *, int32 *, int32 *);
extern intn LO_CalcPrintArea(MWContext *, int32 x, int32 y,
	uint32 width, uint32 height, int32* top, int32* bottom);
extern Bool lo_FindWord(MWContext *, lo_DocState *,
	LO_Position *where, LO_Selection *word);

/* Useful Macros */

#define	COPY_COLOR(from_color, to_color)	\
	to_color.red = from_color.red;		\
	to_color.green = from_color.green;	\
	to_color.blue = from_color.blue


extern Bool lo_FindBestPositionOnLine( MWContext *pContext,
            lo_DocState* state, int32 iLine,
            int32 iDesiredX, int32 iDesiredY,
			Bool bForward, int32* pRetX, int32 *pRetY );

#ifdef EDITOR

#ifdef DEBUG
/* lo_VerifyLayout does a consistency check on the layout structure.
 * returns TRUE if the layout structure is consistent. */
extern Bool lo_VerifyLayout(MWContext *pContext);

/* PrintLayout prints the layout structure. */
extern void lo_PrintLayout(MWContext *pContext);

#endif /* DEBUG */

#endif /* EDITOR */

/*
 * Returns length in selectable parts of this element. 1 for images, length of text for text, etc.
 */
int32
lo_GetElementLength(LO_Element* eptr);

extern Bool lo_EditableElement(int iType);

/* Moves by one editable position forward or backward. Returns FALSE if it couldn't.
 * position can be denormalized on entry and may be denormalized on exit.
 */

Bool
lo_BumpEditablePosition(MWContext *context, lo_DocState *state,
    LO_Element **pEptr, int32 *pPosition, Bool direction);

#ifdef MOCHA

extern lo_TopState *
lo_GetMochaTopState(MWContext *context);

extern void
lo_MarkMochaTopState(lo_TopState *top_state, MWContext *context);

extern void
lo_BeginReflectForm(MWContext *context, lo_DocState *state, PA_Tag *tag,
                    lo_FormData *form);

extern void
lo_EndReflectForm(MWContext *context, lo_FormData *form);

extern void
lo_ReflectImage(MWContext *context, lo_DocState *doc_state, PA_Tag *tag,
                LO_ImageStruct *image_data, Bool blocked);

extern void
lo_CompileEventHandler(MWContext *context, lo_DocState *state, PA_Tag *tag,
                       LO_FormElementStruct *form_element, char *name);

extern void
lo_ReflectNamedAnchor(MWContext *context, lo_DocState *state, PA_Tag *tag,
                      lo_NameList *name_rec);

extern void
lo_ReflectLink(MWContext *context, lo_DocState *state, PA_Tag *tag,
               LO_AnchorData *anchor_data, uint index);

extern void
lo_ProcessContextEventHandlers(MWContext *context, lo_DocState *state,
                               PA_Tag *tag);

extern void
lo_RestoreContextEventHandlers(MWContext *context, lo_DocState *state,
                               PA_Tag *tag, SHIST_SavedData *saved_data);

extern void
lo_ProcessScriptTag(MWContext *context, lo_DocState *state, PA_Tag *tag);

extern void
lo_BlockScriptTag(MWContext *context, lo_DocState *state, PA_Tag *tag);

extern void
lo_UnblockScriptTag(lo_DocState *state);

extern void
lo_ClearInputStream(MWContext *context, lo_TopState *top_state);

#endif /* MOCHA */

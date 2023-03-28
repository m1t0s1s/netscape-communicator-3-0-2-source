/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t -*- 
 ******************
 * Structures used by the layout engine which will be passed
 * on to the front end in the front-end to layout interface
 * functions.
 *
 * All the major element structures have an entry
 * called FE_Data.  This is to be used by the front end to
 * store front-end specific data that the layout ill pass
 * back to it later.
 ******************/
#ifndef _LayoutElements_
#define _LayoutElements_


#include "xp_core.h"
#include "xp_mem.h"
#include "ntypes.h"
#include "edttypes.h"

/*
 * Colors - some might say that some of this should be user-customizable.
 */

#define LO_DEFAULT_FG_RED         0
#define LO_DEFAULT_FG_GREEN       0
#define LO_DEFAULT_FG_BLUE        0

#define LO_DEFAULT_BG_RED         192
#define LO_DEFAULT_BG_GREEN       192
#define LO_DEFAULT_BG_BLUE        192

#define LO_UNVISITED_ANCHOR_RED   0
#define LO_UNVISITED_ANCHOR_GREEN 0
#define LO_UNVISITED_ANCHOR_BLUE  238

#define LO_VISITED_ANCHOR_RED     85
#define LO_VISITED_ANCHOR_GREEN   26
#define LO_VISITED_ANCHOR_BLUE    139

#define LO_SELECTED_ANCHOR_RED    255
#define LO_SELECTED_ANCHOR_GREEN  0
#define LO_SELECTED_ANCHOR_BLUE   0

/*
 * Color enumeration for getting/setting defaults
 */
#define LO_COLOR_BG	0
#define LO_COLOR_FG	1
#define LO_COLOR_LINK	2
#define LO_COLOR_VLINK	3
#define LO_COLOR_ALINK	4

#define LO_NCOLORS	5


/*
 * Values for scrolling grid cells to control scrollbars
 * AUTO means only use them if needed.
 */
#define LO_SCROLL_NO	0
#define LO_SCROLL_YES	1
#define LO_SCROLL_AUTO	2

/*
 * Relative font sizes that the user can tweak from the FE
 */
#define XP_SMALL  1
#define XP_MEDIUM 2
#define XP_LARGE  3

/* --------------------------------------------------------------------------
 * Layout bit flags for text and bullet types
 */
#define LO_UNKNOWN      -1
#define LO_NONE         0
#define LO_TEXT         1
#define LO_LINEFEED     2
#define LO_HRULE        3
#define LO_IMAGE        4
#define LO_BULLET       5
#define LO_FORM_ELE     6
#define LO_SUBDOC       7
#define LO_TABLE        8
#define LO_CELL         9
#define LO_EMBED        10
#define LO_EDGE         11
#define LO_JAVA         12
#define LO_SCRIPT       13

#define LO_FONT_NORMAL      0x0000
#define LO_FONT_BOLD        0x0001
#define LO_FONT_ITALIC      0x0002
#define LO_FONT_FIXED       0x0004

#define LO_ATTR_ANCHOR      0x0008
#define LO_ATTR_UNDERLINE   0x0010
#define LO_ATTR_STRIKEOUT   0x0020
#define LO_ATTR_BLINK	    0x0040
#define LO_ATTR_ISMAP	    0x0080
#define LO_ATTR_ISFORM	    0x0100
#define LO_ATTR_DISPLAYED   0x0200
#define LO_ATTR_BACKDROP    0x0400
#define LO_ATTR_STICKY      0x0800
#define LO_ATTR_INTERNAL_IMAGE  0x1000
#define LO_ATTR_MOCHA_IMAGE 0x2000 /* Image loaded at behest of JavaScript */
#define LO_ATTR_ON_IMAGE_LIST   0x4000 /* Image on top_state->image_list */

/*
 * Different bullet types.   The front end should only ever see
 *  bullets of type BULLET_ROUND and BULLET_SQUARE
 */
#define BULLET_NONE             0
#define BULLET_BASIC            1
#define BULLET_NUM              2
#define BULLET_NUM_L_ROMAN      3
#define BULLET_NUM_S_ROMAN      4
#define BULLET_ALPHA_L          5
#define BULLET_ALPHA_S          6
#define BULLET_ROUND            7
#define BULLET_SQUARE           8

/*
 * Generic union of all the possible element structure types.
 * Defined at end of file.
 */
union LO_Element_struct;


/*
 * Anchors can be targeted to named windows now, so anchor data is
 * more than just a URL, it is a URL, and a possible target window
 * name.
 */
struct LO_AnchorData_struct {
        PA_Block anchor;        /* really a (char *) */
        PA_Block target;        /* really a (char *) */
        PA_Block alt;           /* really a (char *) */
#ifdef MOCHA
	struct MochaObject *mocha_object;
#endif
};

/*
 * Basic RGB triplet
 */
struct LO_Color_struct {
		uint8 red, green, blue;
};
 
/*
 * Attributes of the text we are currently drawing in.
 */
struct LO_TextAttr_struct {
		int16 size;
		int32 fontmask;   /* bold, italic, fixed */									 
		LO_Color fg, bg;
		Bool no_background; /* unused, text on image? */
		int32 attrmask;   /* anchor, underline, strikeout, blink(gag) */
		char *font_face;
		void *FE_Data;     /* For the front end to store font IDs */
		struct LO_TextAttr_struct *next; /* to chain in hash table */
		int16 charset;
};
 
/*
 * Information about the current text segment as rendered
 * by the front end that the layout engine can use to place
 * the text segment.
 */
struct LO_TextInfo_struct {
		intn max_width;
		int16 ascent, descent;
		int16 lbearing, rbearing;
};
 

/*
 * Bit flags for the ele_attrmask.
 */
#define	LO_ELE_BREAKABLE	0x01
#define	LO_ELE_SECURE		0x02
#define	LO_ELE_SELECTED		0x04
#define	LO_ELE_BREAKING		0x08
#define	LO_ELE_FLOATING		0x10
#define	LO_ELE_SHADED		0x20
#define	LO_ELE_DELAY_CENTER	0x40
#define	LO_ELE_DELAY_RIGHT	0x80
#define	LO_ELE_DELAY_JUSTIFY	0xc0
#define	LO_ELE_DELAY_ALIGNED	0xc0

/*
 * Bit flags for the extra_attrmask.
 */
#define LO_ELE_HIDDEN		0x01


/*
 * This is the structure that the layout module will
 * pass to the front-end and ask the front end to render
 * this text segment in the x,y location specified.
 * The selected flag in combination with the text_attr
 * structure will tell the front-end how to render the text.
 */
struct LO_TextStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
#if defined(XP_WIN)
		union {
			PA_Block text;		/* lock to a (char *) */
			char *pText;
		};
#else
		PA_Block text;		/* lock to a (char *) */
#endif
		LO_AnchorData *anchor_href;
		LO_TextAttr *text_attr;
		uint8 ele_attrmask; /* breakable, secure, selected, etc. */
		int16 text_len;
		int16 sel_start;
		int16 sel_end;
};

 
 
/*****************
 * Similar to the structures above, these are the structures
 * used to manage image elements between the layout and the 
 * front-end.
 ****************/
/*
 * It is up to the front-end to read an image file in some format
 * it understands, and place its data into this structure.
 */
struct LO_ImageData_struct {
		int32 width, height;
#ifdef XP_MAC
		int32 		x, y;
/*		RGBColor	borderColor;*/
		Bool 		isLayedOut;
#else
		unsigned char *data;
#endif
};
 
/*
 * Current status of a displayed image.
 * ImageData (above) can be reused for a single
 * image displayed multiple times, but ImageAttr is
 * unique to a particular instance of a displayed image.
 */
struct LO_ImageAttr_struct {
		int32 attrmask;   /* anchor, delayed, ismap */
		intn alignment;
		intn form_id;
		char *usemap_name;	/* Use named client-side map if here */
		void *usemap_ptr;	/* private, used by layout only! */
		intn submit_x;		/* for form, last click coord */
		intn submit_y;		/* for form, last click coord */
};
 
/*
 * This is the structure that the layout module will
 * pass to the front-end and ask the front end to render
 * this image at the x,y location specified.
 * The selected flag in combination with the image_attr
 * structure will tell the front-end how to render the image.
 * alt is alternate text to display if the image is not to
 * be displayed.
 */
struct LO_ImageStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

#ifdef MOCHA
        struct MochaObject *mocha_object;
#endif
		IL_Image *il_image;     /* Image library representation */
		void * FE_Data;
		PA_Block alt;
		LO_AnchorData *anchor_href;
		PA_Block image_url;
		PA_Block lowres_image_url;
		LO_ImageData *image_data;
		LO_ImageAttr *image_attr;
		LO_TextAttr *text_attr;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 ele_attrmask; /* floating, secure, selected, etc. */
		int16 alt_len;
		int16 sel_start;
		int16 sel_end;
        LO_ImageStruct *next_image; /* next image in list for this document */
};


struct LO_SubDocStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
		void *state;	/* Hack-o-rama to opaque the lo_DocState type */
		LO_Color *bg_color; /* To carry over into CellStruct bg_color */
		LO_AnchorData *anchor_href;
		LO_TextAttr *text_attr;
		int32 alignment;
		int32 vert_alignment;
		int32 horiz_alignment;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 ele_attrmask; /* floating, secure, selected, etc. */
		int16 sel_start;
		int16 sel_end;
};
 
 
/*
 * This is the structure that the layout module will
 * pass to the front-end and ask the front end to render
 * this form element at the x,y location specified.
 * The widget_data field holds state data for this form
 * element, such as the current text in a text entry area.
 */
#define FORM_TYPE_NONE          0
#define FORM_TYPE_TEXT          1
#define FORM_TYPE_RADIO         2
#define FORM_TYPE_CHECKBOX      3
#define FORM_TYPE_HIDDEN        4
#define FORM_TYPE_SUBMIT        5
#define FORM_TYPE_RESET         6
#define FORM_TYPE_PASSWORD      7
#define FORM_TYPE_BUTTON        8
#define FORM_TYPE_JOT           9
#define FORM_TYPE_SELECT_ONE    10
#define FORM_TYPE_SELECT_MULT   11
#define FORM_TYPE_TEXTAREA      12
#define FORM_TYPE_ISINDEX       13
#define FORM_TYPE_IMAGE         14
#define FORM_TYPE_FILE          15
#define FORM_TYPE_KEYGEN        16
#define FORM_TYPE_READONLY      17

#define FORM_METHOD_GET		0
#define FORM_METHOD_POST	1

#define TEXTAREA_WRAP_OFF	0
#define TEXTAREA_WRAP_SOFT	1
#define TEXTAREA_WRAP_HARD	2



/*
 * Union of all different form element data structures
 */
union LO_FormElementData_struct;

/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_SELECT_ONE, FORM_TYPE_SELECT_MULT.
 */

/*
 * nesting deeper and deeper, harder and harder, go, go, oh, OH, OHHHHH!!
 * Sorry, got carried away there.
 */
struct lo_FormElementOptionData_struct {
	PA_Block text_value;
        PA_Block value;
        Bool def_selected;
        Bool selected;
};


struct lo_FormElementSelectData_struct {
	int32 type;
	void *FE_Data;
	PA_Block name;
	int32 size;
        Bool multiple;
        Bool options_valid;
	int32 option_cnt;
	PA_Block options;	/* really a (lo_FormElementOptionData *) */
};

/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_TEXT, FORM_TYPE_PASSWORD, FORM_TYPE_FILE.
 */
struct lo_FormElementTextData_struct {
	int32 type;
	void *FE_Data;
	PA_Block name;
	PA_Block default_text;
	PA_Block current_text;
	int32 size;
	int32 max_size;
};


/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_TEXTAREA.
 */
struct lo_FormElementTextareaData_struct {
	int32 type;
	void *FE_Data;
	PA_Block name;
	PA_Block default_text;
	PA_Block current_text;
	int32 rows;
	int32 cols;
	uint8 auto_wrap;
};


/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_HIDDEN, FORM_TYPE_SUBMIT, FORM_TYPE_RESET,
 * 		  FORM_TYPE_BUTTON, FORM_TYPE_READONLY.
 */
struct lo_FormElementMinimalData_struct {
	int32 type;
	void *FE_Data;
	PA_Block name;
	PA_Block value;
};


/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_RADIO, FORM_TYPE_CHECKBOX.
 */
struct lo_FormElementToggleData_struct {
	int32 type;
	void *FE_Data;
	PA_Block name;
	PA_Block value;
	Bool default_toggle;
	Bool toggled;
};


/*
 * This is the element_data structure for elements whose
 * element_type = FORM_TYPE_KEYGEN.
 */
struct lo_FormElementKeygenData_struct {
	int32 type;
	PA_Block name;
	PA_Block challenge;
	PA_Block key_type;
	PA_Block pqg; /* DSA only */
	char *value_str;
	PRBool dialog_done;
};


union LO_FormElementData_struct {
	int32 type;
	lo_FormElementTextData ele_text;
	lo_FormElementTextareaData ele_textarea;
	lo_FormElementMinimalData ele_minimal;
	lo_FormElementToggleData ele_toggle;
	lo_FormElementSelectData ele_select;
	lo_FormElementKeygenData ele_keygen;
};


struct LO_FormSubmitData_struct {
	PA_Block action;
	PA_Block encoding;
	PA_Block window_target;
	int32 method;
	int32 value_cnt;
	PA_Block name_array;
	PA_Block value_array;
	PA_Block type_array;
};

 
struct LO_FormElementStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;
#ifdef MOCHA
		struct MochaObject *mocha_object;
#endif

		int32 element_index;
		LO_FormElementData *element_data;
		LO_TextAttr *text_attr;
		int32 baseline;
		uint8 ele_attrmask; /* secure, selected, etc. */
		int16 form_id;
		int16 sel_start;
		int16 sel_end;
};


/*
 * This is the structure that the layout module will
 * pass to the front-end and ask the front end to render
 * a linefeed at the x,y location specified.
 * Linefeeds are rendered as just a filled rectangle.
 * Height should be the height of the tallest element
 * on the line.
 */

#define LO_LINEFEED_BREAK_SOFT 0
#define LO_LINEFEED_BREAK_HARD 1
#define LO_LINEFEED_BREAK_PARAGRAPH 2

struct LO_LinefeedStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
		LO_AnchorData *anchor_href;
		LO_TextAttr *text_attr;
		int32 baseline;
		uint8 ele_attrmask; /* breaking, secure, selected, etc. */
		int16 sel_start;
		int16 sel_end;
		uint8 break_type;		/* is this an end-of-paragraph or a break? */
};
 
 
struct LO_HorizRuleStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		int32 end_x, end_y;
		void *FE_Data;
		uint8 ele_attrmask; /* shaded, secure, selected, etc. */
		int16 alignment;
		int16 thickness; /* 1 - 100 */
		int16 sel_start;
		int16 sel_end;
};
 
struct LO_BulletStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
		int32 bullet_type;
		LO_TextAttr *text_attr;
		uint8 ele_attrmask; /* secure, selected, etc. */
		int16 level;
		int16 sel_start;
		int16 sel_end;
};


struct LO_TableStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
		LO_AnchorData *anchor_href;
		int32 expected_y;
		int32 alignment;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 ele_attrmask; /* floating, secure, selected, etc. */
		int16 sel_start;
		int16 sel_end;
};


struct LO_CellStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void *FE_Data;
		LO_Element *cell_list;
		LO_Element *cell_list_end;
		LO_Element *cell_float_list;
		LO_Color *bg_color;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 ele_attrmask; /* secure, selected, etc. */
		int16 sel_start;
		int16 sel_end;
};


struct LO_EmbedStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void * FE_Data;
		void *session_data;
		PA_Block embed_src;
		int32 attribute_cnt;
		char **attribute_list;
		char **value_list;
		int32 alignment;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 extra_attrmask; /* hidden, etc. */
		uint8 ele_attrmask; /* floating, secure, selected, etc. */
		int32 embed_index;	/* Unique ID within this doc */
		struct LO_EmbedStruct_struct *nextEmbed;
#ifdef MOCHA
		struct MochaObject *mocha_object;
#endif
};


struct LO_JavaAppStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;

		void * FE_Data;
		void *session_data;
		PA_Block base_url;
		PA_Block attr_code;
		PA_Block attr_codebase;
		PA_Block attr_archive;
		PA_Block attr_name;
		int32 param_cnt;
		char **param_names;
		char **param_values;
		int32 alignment;
		int32 border_width;
		int32 border_vert_space;
		int32 border_horiz_space;
		uint8 ele_attrmask; /* floating, secure, selected, etc. */
		int32 embed_index;	/* Unique ID within this doc */
		Bool may_script;
		/* linked list thread for applets in the current document.
		 * should be "prev" since they're in reverse order but who's
		 * counting? */
		struct LO_JavaAppStruct_struct *nextApplet;
#ifdef MOCHA
		struct MochaObject *mocha_object;
#endif
};


struct LO_EdgeStruct_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;

		void * FE_Data;
		void *lo_edge_info;
		LO_Color *bg_color;
		int16 size;
		Bool visible;
		Bool movable;
		Bool is_vertical;
		int32 left_top_bound;
		int32 right_bottom_bound;
};


struct LO_Any_struct {
		int16 type;
		int16 x_offset;
		int32 ele_id;
		int32 x, y;
		int32 y_offset;
		int32 width, height;
		int32 line_height;
		LO_Element *next;
		LO_Element *prev;
		ED_Element *edit_element;
		int32 edit_offset;
};


union LO_Element_struct {
	int16 type;
	LO_Any lo_any;
	LO_TextStruct lo_text;
	LO_ImageStruct lo_image;
	LO_SubDocStruct lo_subdoc;
	LO_FormElementStruct lo_form;
	LO_LinefeedStruct lo_linefeed;
	LO_HorizRuleStruct lo_hrule;
	LO_BulletStruct lo_bullet;
	LO_TableStruct lo_table;
	LO_CellStruct lo_cell;
	LO_EmbedStruct lo_embed;
	LO_EdgeStruct lo_edge;
	LO_JavaAppStruct lo_java;
};


struct LO_Position_struct {
    LO_Element* element;  /* The element */
    int32 position;    /* The position within the element. */
};

/* begin always <= end. Both begin and end are included in
 * the selected region. (i.e. it's closed. Half-open would
 * be better, since then we could also express insertion
 * points.)
 */

struct LO_Selection_struct {
    LO_Position begin;
    LO_Position end;
};

/* Hit test results */

#define LO_HIT_UNKNOWN  0
#define LO_HIT_LINE  1
#define LO_HIT_ELEMENT  2

#define LO_HIT_LINE_REGION_BEFORE  0
#define LO_HIT_LINE_REGION_AFTER  1
 
#define LO_HIT_ELEMENT_REGION_BEFORE  0
#define LO_HIT_ELEMENT_REGION_MIDDLE  1
#define LO_HIT_ELEMENT_REGION_AFTER  2

struct LO_HitLineResult_struct {
    int16 type;         /* LO_HIT_LINE */
    int16 region;     /* LO_HIT_LINE_POSITION_XXX */
    LO_Selection selection; /* The line */
};

struct LO_HitElementResult_struct {
    int16 type;         /* LO_HIT_ELEMENT */
    int16 region;     /* LO_HIT_ELEMENT_POSITION_XXX */
    LO_Position position;    /* The element that was hit. */

};

union LO_HitResult_struct {
    int16 type;
    LO_HitLineResult lo_hitLine;
    LO_HitElementResult lo_hitElement;
};

/* Logical navigation chunks */

#define LO_NA_CHARACTER 0
#define LO_NA_WORD 1
#define LO_NA_LINEEDGE 2
#define LO_NA_DOCUMENT 3

#endif /* _LayoutElements_ */

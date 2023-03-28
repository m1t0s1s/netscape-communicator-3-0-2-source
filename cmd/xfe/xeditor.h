/* -*- Mode: C; tab-width: 4 -*- 
 *
 *  xeditor.h --- Editor-specific headers for the front end.
 *  Should only be include by files that work with the editor.
 *  Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 *  Created: David Williams <djw@netscape.com>, Mar-12-1996
 *
 *  RCSID: "$Id: xeditor.h,v 1.41 1996/08/20 22:13:19 palevich Exp $"
 */
#ifndef _XEDITOR_H_
#define _XEDITOR_H_

#include <edttypes.h>       /* collect EDT_<editor> types. */
#include <pa_tags.h>        /* for P_UNUM_LIST, P_NUM_LIST, etc */
#include "xfe.h"

#define XFE_EDITOR_DEFAULT_DOCUMENT_TEMPLATE_URL     \
"http://home.netscape.com/home/gold3.0_templates.html"
#define XFE_WIZARD_TEMPLATE_URL \
"http://home.netscape.com/home/gold3.0_wizard.html"
#define XFE_EDITOR_NEW_DOC_NAME "file: Untitled"


/*
 *    A useful type.
 */
typedef struct fe_NameValueItem
{
    char* name;
    char* value;
} fe_NameValueItem;

/*
 *    Put this edttype.h after Akbar merge.
 */
typedef enum {
	ED_FONTSIZE_DEFAULT    = 0, /* == ED_FONTSIZE_ZERO */
	ED_FONTSIZE_MINUS_TWO  = 1,
	ED_FONTSIZE_MINUS_ONE  = 2,
	ED_FONTSIZE_ZERO       = 3,
	ED_FONTSIZE_PLUS_ONE   = 4,
	ED_FONTSIZE_PLUS_TWO   = 5,
	ED_FONTSIZE_PLUS_THREE = 6,
	ED_FONTSIZE_PLUS_FOUR  = 7
} ED_FontSize;

#define EDT_FONTSIZE_MAX ED_FONTSIZE_PLUS_FOUR
#define EDT_FONTSIZE_MIN ED_FONTSIZE_MINUS_TWO

#define TF_ALL_MASK \
(TF_BOLD|TF_ITALIC|TF_FIXED|TF_SUPER|TF_SUB|TF_STRIKEOUT|TF_BLINK|TF_UNDERLINE)

Boolean       fe_EditorCanUndo(MWContext*, XmString* label_return);
Boolean       fe_EditorCanRedo(MWContext*, XmString* label_return);
void          fe_EditorUndo(MWContext*);
void          fe_EditorRedo(MWContext*);
Boolean       fe_EditorLineBreakCanInsert(MWContext*);
void          fe_EditorLineBreak(MWContext*, ED_BreakType type);
void          fe_EditorNonBreakingSpace(MWContext*);
void          fe_EditorParagraphMarksSetState(MWContext*, Boolean display);
Boolean       fe_EditorParagraphMarksGetState(MWContext*);
void          fe_EditorFontSizeSet(MWContext*, ED_FontSize size);
ED_FontSize   fe_EditorFontSizeGet(MWContext*);
void          fe_EditorCharacterPropertiesSet(MWContext*, ED_TextFormat);
ED_TextFormat fe_EditorCharacterPropertiesGet(MWContext*);
void          fe_EditorParagraphPropertiesSet(MWContext*, TagType);
TagType       fe_EditorParagraphPropertiesGet(MWContext*);
Boolean       fe_EditorParagraphPropertiesGetAll(MWContext*,
												 TagType*,
												 EDT_ListData*,
												 ED_Alignment*);
void          fe_EditorParagraphPropertiesSetAll(MWContext*,
												 TagType,
												 EDT_ListData*,
												 ED_Alignment);
void          fe_EditorInitActions();
Widget        fe_EditorCreatePropertiesToolbar(MWContext*, Widget, char* name);
void          fe_EditorUpdateToolbar(MWContext*, ED_TextFormat);
void          fe_EditorHorizontalRulePropertiesSet(MWContext*, EDT_HorizRuleData*);
void          fe_EditorHorizontalRulePropertiesGet(MWContext*, EDT_HorizRuleData*);
ED_Alignment  fe_EditorAlignGet(MWContext*);
void          fe_EditorAlignSet(MWContext*, ED_Alignment);

void          fe_EditorHrefInsert(MWContext* context,
								  char* p_text, char* p_url);
Boolean       fe_EditorHrefGet(MWContext* context, char** text, char** url);
void          fe_EditorHrefSetUrl(MWContext* context, char* url);
void          fe_EditorHrefClearUrl(MWContext* context);

Boolean       fe_EditorListDataGet(MWContext* context, EDT_ListData* l_data);
void          fe_EditorListDataSet(MWContext* context, EDT_ListData* l_data);
void          fe_EditorColorSet(MWContext* context, LO_Color* color);
Boolean       fe_EditorColorGet(MWContext* context, LO_Color* color);

Boolean       fe_EditorCanCut(MWContext* context);
Boolean       fe_EditorCanCopy(MWContext* context);
Boolean       fe_EditorCanPaste(MWContext* context);
Boolean       fe_EditorCut(MWContext* context, Time);
Boolean       fe_EditorCopy(MWContext* context, Time);
Boolean       fe_EditorPaste(MWContext* context, Time);
void          fe_EditorSelectTable(MWContext* context);
void          fe_EditorSelectAll(MWContext* context);
void          fe_EditorDeleteItem(MWContext* context);
void          fe_EditorRemoveLinks(MWContext* context);

MWContext*    fe_EditorNew(MWContext* context, char*);
void          fe_EditorDelete(MWContext* context);
void          fe_EditorCleanup(MWContext* context);

/*
 *    Dialog interface.
 */
typedef enum fe_EditorPropertiesDialogType {
	XFE_EDITOR_PROPERTIES_OOPS = 0,
	XFE_EDITOR_PROPERTIES_LINK,
	XFE_EDITOR_PROPERTIES_CHARACTER,
	XFE_EDITOR_PROPERTIES_PARAGRAPH,
	XFE_EDITOR_PROPERTIES_IMAGE,
	XFE_EDITOR_PROPERTIES_HRULE,
	XFE_EDITOR_PROPERTIES_HTML_TAG,
	XFE_EDITOR_PROPERTIES_DOCUMENT,
	XFE_EDITOR_PROPERTIES_IMAGE_INSERT,
	XFE_EDITOR_PROPERTIES_LINK_INSERT,
	XFE_EDITOR_PROPERTIES_TABLE,
	XFE_EDITOR_PROPERTIES_TABLE_ROW,
	XFE_EDITOR_PROPERTIES_TABLE_CELL,
	XFE_EDITOR_PROPERTIES_TARGET
} fe_EditorPropertiesDialogType;

Boolean       fe_EditorPropertiesDialogCanDo(MWContext*,
											  fe_EditorPropertiesDialogType);
void          fe_EditorPropertiesDialogDo(MWContext*,
											  fe_EditorPropertiesDialogType);
typedef enum fe_EditorDocumentPropertiesDialogType
{
 	XFE_EDITOR_DOCUMENT_PROPERTIES_OOPS = 0,
 	XFE_EDITOR_DOCUMENT_PROPERTIES_APPEARANCE,
 	XFE_EDITOR_DOCUMENT_PROPERTIES_GENERAL,
 	XFE_EDITOR_DOCUMENT_PROPERTIES_ADVANCED
} fe_EditorDocumentPropertiesDialogType;

void          fe_EditorDocumentPropertiesDialogDo(MWContext* context, 
							  fe_EditorDocumentPropertiesDialogType tab_type);

Boolean       fe_EditorHorizontalRulePropertiesCanDo(MWContext* context);
void          fe_EditorHorizontalRulePropertiesDialogDo(MWContext* context);
void          fe_EditorSetColorsDialogDo(MWContext* context);

void          fe_EditorDefaultGetColors( LO_Color*  bg_color,
										 LO_Color*  normal_color,
										 LO_Color*  link_color,
										 LO_Color*  active_color,
										 LO_Color*  followed_color);
char*         fe_EditorDefaultGetTemplate();
Boolean       fe_EditorDefaultGetLastPublishLocation(MWContext* context,
									   char** location,
									   char** username,
									   char** password);
void          fe_EditorDefaultSetLastPublishLocation(MWContext* context,
									   char* location,
									   char* username,
									   char* password);



/*
 *    Document Properties API.
 */
Boolean       fe_EditorDocumentGetColors(MWContext*,
										 char*      background_image,
										 LO_Color*  bg_color,
										 LO_Color*  normal_color,
										 LO_Color*  link_color,
										 LO_Color*  active_color,
										 LO_Color*  followed_color);
void          fe_EditorDocumentSetColors(MWContext* context,
										 char*      background_image,
										 LO_Color*  bg_color,
										 LO_Color*  normal_color,
										 LO_Color*  link_color,
										 LO_Color*  active_color,
										 LO_Color*  followed_color);
char*         fe_EditorDocumentGetTitle(MWContext* context);
void          fe_EditorDocumentSetTitle(MWContext* context, char* name);
void          fe_EditorDocumentSetMetaData(MWContext*, char* name, char* val);
char*         fe_EditorDocumentGetMetaData(MWContext*, char* name);

void          fe_EditorDocumentSetAdvancedMetaDataList(MWContext*,
													   fe_NameValueItem*);
fe_NameValueItem* fe_EditorDocumentGetAdvancedMetaDataList(MWContext*);
void          fe_EditorDocumentSetHttpEquivMetaDataList(MWContext* context,
														fe_NameValueItem*);
fe_NameValueItem* fe_EditorDocumentGetHttpEquivMetaDataList(MWContext*);
Boolean       fe_EditorPublishDialogDo(MWContext* context);
void          fe_EditorDocumentSetImagesWithDocument(MWContext*, Boolean keep);
Boolean       fe_EditorDocumentGetImagesWithDocument(MWContext*);

/*
 *    Editor preferences API.
 */
void          fe_EditorPreferencesGetLinksAndImages(MWContext* context,
													Boolean*   links, 
													Boolean*   images);
void          fe_EditorPreferencesSetLinksAndImages(MWContext* context,
													Boolean   links, 
													Boolean   images);
char*         fe_EditorPreferencesGetAuthor(MWContext* context);
void          fe_EditorPreferencesSetAuthor(MWContext* context, char*);
Boolean       fe_EditorPreferencesGetColors(MWContext*,
										 char*      background_image,
										 LO_Color*  bg_color,
										 LO_Color*  normal_color,
										 LO_Color*  link_color,
										 LO_Color*  active_color,
										 LO_Color*  followed_color);
void          fe_EditorPreferencesSetColors(MWContext* context,
										 char*      background_image,
										 LO_Color*  bg_color,
										 LO_Color*  normal_color,
										 LO_Color*  link_color,
										 LO_Color*  active_color,
										 LO_Color*  followed_color);
void          fe_EditorPreferencesSetEditors(MWContext*, char*, char*);
void          fe_EditorPreferencesGetEditors(MWContext*, char**, char**);
void          fe_EditorEditImage(MWContext* context, char* file);
void          fe_EditorEditSource(MWContext* context);
char*         fe_EditorPreferencesGetTemplate(MWContext* context);
void          fe_EditorPreferencesSetTemplate(MWContext* context, char*);
void          fe_EditorPreferencesDialogDo(MWContext*, unsigned tab_type);
Boolean       fe_EditorPreferencesGetPublishLocation(MWContext* context,
													 char** location,
													 char** username,
													 char** password);
void          fe_EditorPreferencesSetPublishLocation(MWContext* context,
													 char* location,
													 char* username,
													 char* password);
char*         fe_EditorPreferencesGetBrowseLocation(MWContext* context);
void          fe_EditorPreferencesSetBrowseLocation(MWContext*,	char*);
void          fe_EditorPreferencesGetAutoSave(MWContext* context,
											  Boolean*   enable,
											  unsigned*  time);
void          fe_EditorPreferencesSetAutoSave(MWContext* context,
											  Boolean    enable,
											  unsigned   time);

void          fe_EditorTargetPropertiesDialogDo(MWContext*);
void          fe_EditorHtmlPropertiesDialogDo(MWContext*);



typedef struct EDT_AllTableData
{
    EDT_TableData        td;
    EDT_TableCaptionData cd;
    Boolean              has_caption;
} EDT_AllTableData;

void          fe_EditorTableInsert(MWContext*, EDT_AllTableData* data);
Boolean       fe_EditorTableCanDelete(MWContext*);
Boolean       fe_EditorTableRowCanDelete(MWContext*);
Boolean       fe_EditorTableCellCanDelete(MWContext*);
Boolean       fe_EditorTableGetData(MWContext*, EDT_AllTableData*);
void          fe_EditorTableSetData(MWContext*, EDT_AllTableData*);
Boolean       fe_EditorTableRowGetData(MWContext*, EDT_TableRowData*);
void          fe_EditorTableRowSetData(MWContext*, EDT_TableRowData*);
Boolean       fe_EditorTableCellGetData(MWContext*, EDT_TableCellData*);
void          fe_EditorTableCellSetData(MWContext*, EDT_TableCellData*);
void          fe_EditorDisplayTablesSet(MWContext* context, Boolean display);
Boolean       fe_EditorDisplayTablesGet(MWContext* context);

/*
 *    Tables dialogs.
 */
void          fe_EditorTableCreateDialogDo(MWContext*);
void          fe_EditorTablePropertiesDialogDo(MWContext*, fe_EditorPropertiesDialogType);

Boolean       fe_EditorCheckUnsaved(MWContext* context);
void          fe_EditorDisplaySource(MWContext* context);
char*         fe_EditorMakeName(MWContext*, char* buf, unsigned maxsize);
void          fe_EditorOpenURLDialog(MWContext*);
void          fe_EditorRefresh(MWContext*);
MWContext*    fe_EditorGetURL(MWContext* context, char* address);
MWContext*    fe_BrowserGetURL(MWContext* context, char* address);
void          fe_EditorNotifyBackgroundChanged(MWContext*);
void          fe_EditorRefreshArea(MWContext*, int, int, unsigned, unsigned);
void          fe_EditorGetUrlExitRoutine(MWContext*, URL_Struct*, int status);

/*
 *    Misc stuff that should be elsewhere
 */
enum {
	XFE_DIALOG_YES_BUTTON = (XmDIALOG_DIR_LIST_LABEL+1),
	XFE_DAILOG_YESTOALL_BUTTON,
	XFE_DIALOG_NO_BUTTON,
	XFE_DIALOG_NOTOALL_BUTTON,
	XFE_DIALOG_CANCEL_BUTTON,
	XFE_DIALOG_DESTROY_BUTTON
};

XtPointer     fe_GetUserData(Widget);
Widget        fe_OptionMenuSetHistory(Widget, unsigned);
char*         FE_CondenseURL(char* target, char* source, unsigned target_len);
Boolean       FE_CheckAndSaveDocument(MWContext*);
int           fe_YesNoCancelDialog(MWContext*, char* name, char* message);
MWContext*    fe_GetURLInBrowser(MWContext* context, URL_Struct *url);
Boolean       fe_HintDialog(MWContext* context, char* message);
void          fe_editor_scroll(MWContext *context);
void 		  fe_editor_delete_response(Widget, XtPointer, XtPointer);

#endif  /* _XEDITOR_H_ */

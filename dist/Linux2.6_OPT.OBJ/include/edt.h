/* -*- Mode: C; tab-width: 4 -*-
*/
#ifndef _edt_h_
#define _edt_h_

#ifdef EDITOR

#ifndef _XP_Core_
#include "xp_core.h"
#endif

#ifndef _edttypes_h_
#include "edttypes.h"
#endif

XP_BEGIN_PROTOS

/*****************************************************************************
 * Layout Interface
 *****************************************************************************/

/*
 *  Create an edit buffer, ready to be pared into.
 */ 
ED_Buffer* EDT_MakeEditBuffer(MWContext *pContext);

/*
 *  Destroy an edit buffer.
 */ 
void EDT_DestroyEditBuffer(MWContext * pContext);

/* CLM:
 * Front end can call this to prevent doing 
 * bad stuff when we failed to load URL
 * and don't have a buffer
 */
XP_Bool EDT_HaveEditBuffer(MWContext * pContext);

/*  
 *  Parse a tag into an edit buffer.
 */
ED_Element* EDT_ParseTag(ED_Buffer* pEditBuffer, PA_Tag* pTag);

/*
 * Called instead of LO_ProcessTag.  This routine allows the Edit enginge to 
 *  build up an HTML tree.  Inputs and outputs are the same as LO_ProcessTag.
*/
intn EDT_ProcessTag(void *data_object, PA_Tag *tag, intn status);

/*
 * Tells the edit engine to position the insert point.  
*/
void EDT_SetInsertPoint( ED_Buffer *pBuffer, ED_Element* pElement, int iPosition, XP_Bool bStickyAfter );

/*
 * Assocates the layout element with a given edit element.  Called from by the 
 *  layout engine onces a layout element has been has been created.
*/
void EDT_SetLayoutElement( ED_Element* pElement, intn iEditOffset, intn lo_type,
            LO_Element *pLayoutElement );

/* Breaks the association between a layout element and a given edit element.
 * Called by the layout engine when a layout element is being destroyed.
 */

void EDT_ResetLayoutElement( ED_Element* pElement, intn iEditOffset,
                            LO_Element* pLayoutElement);

/*
 * Retrieve the next tag element from the editor.  Returns NULL when there are
 *  no more tags.
*/
PA_Tag* EDT_TagCursorGetNext( ED_TagCursor* pCursor );

/*
 * enclosing html for a given point in the document
*/
PA_Tag* EDT_TagCursorGetNextState( ED_TagCursor* pCursor );

/*
 * returns true if at the beginning of a paragraph or break.
*/
XP_Bool EDT_TagCursorAtBreak( ED_TagCursor *pCursor, XP_Bool* pEndTag );

/*
 * Get the current line number for the cursor
*/
int32 EDT_TagCursorCurrentLine( ED_TagCursor* pCursor );

/*
 * Get the current line number for the cursor
*/
XP_Bool EDT_TagCursorClearRelayoutState( ED_TagCursor* pCursor );


/*
 * Clone a cursor.  Used to save position state.
*/
ED_TagCursor* EDT_TagCursorClone( ED_TagCursor *pCursor );

/*
 * Delete a cloned (or otherwise) cursor
*/
void EDT_TagCursorDelete( ED_TagCursor* pCursor );

/*
 * called when LO finishes (only the first time)
*/
void EDT_FinishedLayout( MWContext *pContext );

/*
 * Call back from image library giving the height and width of an image.
*/
void EDT_SetImageInfo(MWContext *context, int32 ele_id, int32 width, int32 height);

/*
 * Test to see if the editor is currently loading a file.
*/
XP_Bool EDT_IsBlocked( MWContext *pContext );

/*****************************************************************************
 * FE Interface
 *****************************************************************************/
/* 
 * Returns ED_ERROR_NONE if files saved OK, else returns error code.
*/
ED_FileError EDT_SaveFile( MWContext * pContext,
                           char * pSourceURL,
                           char * pDestURL,
                           XP_Bool   bSaveAs,
                           XP_Bool   bKeepImagesWithDoc,
                           XP_Bool   bAutoAdjustLinks );

/* 
 * cancel a load in progress
*/
void EDT_SaveCancel( MWContext *pContext );

/*
 * Enable and disable autosave. A value of zero disables autosave.
 * AutoSave doesn't start until the document has a file.
 */

void EDT_SetAutoSavePeriod(MWContext *pContext, int32 minutes);
int32 EDT_GetAutoSavePeriod(MWContext *pContext);

void EDT_DisplaySource( MWContext *pContext );

/*
 * Edit Navagation prototypes
*/
void EDT_PreviousChar( MWContext *context, XP_Bool bSelect );
void EDT_NextChar( MWContext *context, XP_Bool bSelect );
void EDT_BeginOfLine( MWContext *context, XP_Bool bSelect );
void EDT_EndOfLine( MWContext *context, XP_Bool bSelect );
void EDT_BeginOfDocument( MWContext *context, XP_Bool bSelect );
void EDT_EndOfDocument( MWContext *context, XP_Bool bSelect );
void EDT_Up( MWContext *context, XP_Bool bSelect );
void EDT_Down( MWContext *context, XP_Bool bSelect );
void EDT_PageUp( MWContext *context, XP_Bool bSelect );
void EDT_PageDown( MWContext *context, XP_Bool bSelect );
void EDT_PreviousWord( MWContext *context, XP_Bool bSelect );
void EDT_NextWord( MWContext *context, XP_Bool bSelect );

void EDT_WindowScrolled( MWContext *context );

/*
 * Edit functional stuff...
*/
EDT_ClipboardResult EDT_DeletePreviousChar( MWContext *context );
EDT_ClipboardResult EDT_DeleteChar( MWContext *context );
void EDT_PositionCaret( MWContext *context, int32 x, int32 y );
void EDT_DoubleClick( MWContext *context, int32 x, int32 y );
void EDT_SelectObject( MWContext *context, int32 x, int32 y);
EDT_ClipboardResult EDT_ReturnKey( MWContext *context );

void EDT_Indent( MWContext *context );
void EDT_Outdent( MWContext *context );


void EDT_Reload( MWContext *pContext );

/*
 * Kludge, supports only the windows front end now.
*/
EDT_ClipboardResult EDT_KeyDown( MWContext *context, uint16 nChar, uint16 b, uint16 c );

/*
 * Insert a nonbreaking space character.  Usually wired to shift spacebar
*/
EDT_ClipboardResult EDT_InsertNonbreakingSpace( MWContext *context );

/*
 * Undo/Redo. Usage:
 * To tell if there's a command to undo:
 *  if ( EDT_GetUndoCommandID(context, 0 ) != CEDITCOMMAND_ID_NULL )
 *
 * To undo the most recent command:
 *  EDT_Undo( context );
 *
 * (Similarly for redo.)
 *
 * Use the CommandID to look up a string for saying "Undo <type of command>..."
 * on the menu.
 */

#define CEDITCOMMAND_ID_NULL 0
#define CEDITCOMMAND_ID_GENERICCOMMAND 1

void EDT_Undo( MWContext *pContext );
void EDT_Redo( MWContext *pContext );
intn EDT_GetUndoCommandID( MWContext *pContext, intn index );
intn EDT_GetRedoCommandID( MWContext *pContext, intn index );

/* Lets the front end specify the command history limit. This is used to limit the total of commands on
 * both the undo and the redo lists.
 */

intn EDT_GetCommandHistoryLimit();
void EDT_SetCommandHistoryLimit(intn limit); /* Must be greater than or equal to zero. Ignored if less than zero. */

/* Call from dialogs to batch changes for undo.  To use, call EDT_BeginBatchChanges,
 * make the changes you want, and then call EDT_EndBatchChanges. It's OK to
 * not make any changes -- in that case the undo history won't be affected.
 */

void EDT_BeginBatchChanges(MWContext *pContext);
void EDT_EndBatchChanges(MWContext *pContext);

/*
 * Used to control display of paragraph marks.
 */

void EDT_SetDisplayParagraphMarks(MWContext *pContext, XP_Bool display);
XP_Bool EDT_GetDisplayParagraphMarks(MWContext *pContext);

/*
 * Used to control display of tables in either wysiwyg or flat mode.
 */

void EDT_SetDisplayTables(MWContext *pContext, XP_Bool display);
XP_Bool EDT_GetDisplayTables(MWContext *pContext);

/*
 * need to figure out how to handle tags and parameters.
*/
void EDT_MorphContainer( MWContext *pContext, TagType t );
TagType EDT_GetParagraphFormatting( MWContext *pContext );
void EDT_SetParagraphAlign( MWContext* pContext, ED_Alignment eAlign );
ED_Alignment EDT_GetParagraphAlign( MWContext *pContext);

/*
 * Find out what's under the cursor
*/
ED_ElementType EDT_GetCurrentElementType( MWContext *pContext );

/*
 * Character Formatting (DEPRECIATED)
 * Routines should only be called when EDT_GetCurrentElementType() returns
 *  ED_ELEMENT_TEXT.
*/
ED_TextFormat EDT_GetCharacterFormatting( MWContext *pContext );
void EDT_FormatCharacter( MWContext *pContext, ED_TextFormat p);
int EDT_GetFontSize( MWContext *pContext );
void EDT_SetFontSize( MWContext *pContext, int );

XP_Bool EDT_GetFontColor( MWContext *pContext, LO_Color *pDestColor );
void EDT_SetFontColor( MWContext *pContext, LO_Color *pColor);

/*
 * Get and set character formatting.
*/ 

/*
 * EDT_GetCharacterData
 *      returns the current character formatting for the current selection 
 *      or insert point.  Should only be called when GetCurrentElementType is
 *      ED_ELEMENT_TEXT or ED_ELEMENT_SELECTION.
 *
 *  returns and EDT_CharacterData structure.  Caller is responsible for 
 *      destroying the structure by calling EDT_FreeCharacterData().
 *  
 *  CharacterData contain on return
 *      pRet->mask      // the that were deterministic (all in insertpoint case
 *                      //      and those that were consistant across the selection
 *      pRet->values    // if in the mask, 0 means clear across the selection
 *                      //                 1 means set across the selection
*/
EDT_CharacterData* EDT_GetCharacterData( MWContext *pContext );

/*
 * EDT_SetCharacterData
 *      sets the character charistics for the current insert point.
 *      
 *  the mask contains the bits to set
 *      values contains the values to set them to
 *
 *  for example if you wanted to just change the font size to 10 and leave 
 *      the rest alone:
 *
 *      pData = EDT_NewCharacterData();
 *      pData->mask = TF_FONT_SIZE
 *      pData->value = TF_FONT_SIZE
 *      pData->iSize = 5;
 *      EDT_SetCharacterData( context ,pData );
 *      EDT_FreeCharacterData(pData);
 *
*/
void EDT_SetCharacterData( MWContext *pContext, EDT_CharacterData *pData );
EDT_CharacterData* EDT_NewCharacterData();
void EDT_FreeCharacterData( EDT_CharacterData *pData );

/*
 * Returns colors of all the different fonts.
 * Must call XP_FREE( pDest ) after use.
*/
int EDT_GetExtraColors( MWContext *pContext, LO_Color **pDest );

/*
 * Selection
*/
void EDT_StartSelection(MWContext *context, int32 x, int32 y);
void EDT_ExtendSelection(MWContext *context, int32 x, int32 y);
void EDT_EndSelection(MWContext *context, int32 x, int32 y);    /* cm */
void EDT_ClearSelection(MWContext *context);

void EDT_SelectAll(MWContext *context);
void EDT_SelectTable(MWContext *context);
void EDT_SelectTableCell(MWContext *context);

XP_Block EDT_GetSelectionText(MWContext *context);
XP_Bool EDT_IsSelected( MWContext *pContext );
XP_Bool EDT_SelectionContainsLink( MWContext *pContext );

XP_Bool EDT_DirtyFlag( MWContext *pContext );
void EDT_SetDirtyFlag( MWContext *pContext, XP_Bool bValue );

/*
 * Clipboard stuff 
*/

/*CLM: This used as return value above - shouldn't this be in EDTTYPES.H? */
/* typedef int32 EDT_ClipboardResult;*/

#define EDT_COP_OK 0
#define EDT_COP_DOCUMENT_BUSY 1
#define EDT_COP_SELECTION_EMPTY 2
#define EDT_COP_SELECTION_CROSSES_TABLE_DATA_CELL 3

EDT_ClipboardResult EDT_PasteText( MWContext *pContext, char *pText );
EDT_ClipboardResult EDT_PasteHTML( MWContext *pContext, char *pHtml );
EDT_ClipboardResult EDT_PasteExternalHTML( MWContext *pContext, char *pHtml ); /* From another application. */

/*
 * Can paste a singe or multiple HREFs.  Pointer to array of HREF pointers and
 *  title pointers.
 * Title pointers can be NULL
*/
EDT_ClipboardResult EDT_PasteHREF( MWContext *pContext, char **ppHref, char **ppTitle, int iCount);

/* Depreciated */
void EDT_DropHREF( MWContext *pContext, char *pHref, char* pTitle, int32 x,
            int32 y );

/* Depreciated */
XP_Bool EDT_CanDropHREF( MWContext *pContext, int32 x, int32 y );


/* 
 * The text buffer should also be huge, it isn't
*/
EDT_ClipboardResult EDT_CopySelection( MWContext *pContext, char** ppText, 
            int32* pTextLen, XP_HUGE_CHAR_PTR* ppHtml, int32* pHtmlLen);

EDT_ClipboardResult EDT_CutSelection( MWContext *pContext, 
        char** ppText, int32* pTextLen,
        XP_HUGE_CHAR_PTR* ppHtml, int32* pHtmlLen);

/*
 * Use to enable or disable the UI for cut/copy/paste.
 * If bStrictChecking is true, will check for all conditions,
 * Otherwise, will just check if the selection is empty.
 */

EDT_ClipboardResult EDT_CanCut(MWContext *pContext, XP_Bool bStrictChecking);
EDT_ClipboardResult EDT_CanCopy(MWContext *pContext, XP_Bool bStrictChecking);
EDT_ClipboardResult EDT_CanPaste(MWContext *pContext, XP_Bool bStrictChecking);

/*
 * returns true if we can set an HREF
 */
XP_Bool EDT_CanSetHREF( MWContext *pContext );

/*
 * Get the current link under the cursor or inside the current selection.
 */
char *EDT_GetHREF( MWContext *pContext );

/*
 * Get the anchor text of the current link under the cursor
 */
char *EDT_GetHREFText( MWContext *pContext );

/*
 * Use HREF structure
*/
EDT_HREFData *EDT_GetHREFData( MWContext *pContext );
void EDT_SetHREFData( MWContext *pContext, EDT_HREFData *pData );
EDT_HREFData *EDT_NewHREFData();
EDT_HREFData *EDT_DupHREFData( EDT_HREFData *pData );
void EDT_FreeHREFData(  EDT_HREFData *pData );

/*
 * This routine can only be called when 'EDT_CanSetHREF()'
 */
void EDT_SetHREF(MWContext *pContext, char *pHREF );

/*
 * Refresh the entire document.
*/
void EDT_RefreshLayout( MWContext *pContext );

/*
 * Getting and setting properties of images.  After Getting an image's properties
 *  the EDT_ImageData must be freed with EDT_FreeImageData().  Routines should
 *  only be called when EDT_GetCurrentElementType() returns ED_ELEMENT_IMAGE.
*/
EDT_ImageData *EDT_GetImageData( MWContext *pContext );
void EDT_SetImageData( MWContext *pContext, EDT_ImageData *pData, XP_Bool bKeepImagesWithDoc );
EDT_ImageData *EDT_NewImageData();
void EDT_FreeImageData(  EDT_ImageData *pData );
void EDT_InsertImage( MWContext *pContext, EDT_ImageData *pData, XP_Bool bKeepImageWithdoc );

EDT_HorizRuleData *EDT_GetHorizRuleData( MWContext *pContext );
void EDT_SetHorizRuleData( MWContext *pContext, EDT_HorizRuleData *pData );
EDT_HorizRuleData *EDT_NewHorizRuleData();
void EDT_FreeHorizRuleData(  EDT_HorizRuleData *pData );
void EDT_InsertHorizRule( MWContext *pContext, EDT_HorizRuleData *pData );

void EDT_ToggleList( MWContext *pContext, intn iTagType); /* All-in-one-call to create numbered or unnumbered lists. */
XP_Bool EDT_GetToggleListState( MWContext *pContext, intn iTagType);

EDT_ListData *EDT_GetListData( MWContext *pContext );
void EDT_SetListData( MWContext *pContext, EDT_ListData *pData );
void EDT_FreeListData(  EDT_ListData *pData );

void EDT_InsertBreak( MWContext *pContext, ED_BreakType eBreak );

XP_Bool EDT_IsInsertPointInTable(MWContext *pContext );
XP_Bool EDT_IsInsertPointInNestedTable(MWContext *pContext );
EDT_TableData* EDT_GetTableData( MWContext *pContext );
void EDT_SetTableData( MWContext *pContext, EDT_TableData *pData );
EDT_TableData* EDT_NewTableData();
void EDT_FreeTableData(  EDT_TableData *pData );
void EDT_InsertTable( MWContext *pContext, EDT_TableData *pData);
void EDT_DeleteTable( MWContext *pContext);

XP_Bool EDT_IsInsertPointInTableCaption(MWContext *pContext );
EDT_TableCaptionData* EDT_GetTableCaptionData( MWContext *pContext );
void EDT_SetTableCaptionData( MWContext *pContext, EDT_TableCaptionData *pData );
EDT_TableCaptionData* EDT_NewTableCaptionData();
void EDT_FreeTableCaptionData(  EDT_TableCaptionData *pData );
void EDT_InsertTableCaption( MWContext *pContext, EDT_TableCaptionData *pData);
void EDT_DeleteTableCaption( MWContext *pContext);

XP_Bool EDT_IsInsertPointInTableRow(MWContext *pContext );
EDT_TableRowData* EDT_GetTableRowData( MWContext *pContext );
void EDT_SetTableRowData( MWContext *pContext, EDT_TableRowData *pData );
EDT_TableRowData* EDT_NewTableRowData();
void EDT_FreeTableRowData(  EDT_TableRowData *pData );
void EDT_InsertTableRows( MWContext *pContext, EDT_TableRowData *pData, XP_Bool bAfterCurrentRow, intn number);
void EDT_DeleteTableRows( MWContext *pContext, intn number);

XP_Bool EDT_IsInsertPointInTableCell(MWContext *pContext );
EDT_TableCellData* EDT_GetTableCellData( MWContext *pContext );
void EDT_SetTableCellData( MWContext *pContext, EDT_TableCellData *pData );
EDT_TableCellData* EDT_NewTableCellData();
void EDT_FreeTableCellData(  EDT_TableCellData *pData );
void EDT_InsertTableCells( MWContext *pContext, EDT_TableCellData *pData, XP_Bool bAfterCurrentCell, intn number);
void EDT_DeleteTableCells( MWContext *pContext, intn number);
void EDT_InsertTableColumns( MWContext *pContext, EDT_TableCellData *pData, XP_Bool bAfterCurrentCell, intn number);
void EDT_DeleteTableColumns( MWContext *pContext, intn number);

XP_Bool EDT_IsInsertPointInMultiColumn(MWContext *pContext );
EDT_MultiColumnData* EDT_GetMultiColumnData( MWContext *pContext );
void EDT_SetMultiColumnData( MWContext *pContext, EDT_MultiColumnData *pData );
EDT_MultiColumnData* EDT_NewMultiColumnData();
void EDT_FreeMultiColumnData(  EDT_MultiColumnData *pData );
void EDT_InsertMultiColumn( MWContext *pContext, EDT_MultiColumnData *pData);
void EDT_DeleteMultiColumn( MWContext *pContext);

EDT_PageData *EDT_GetPageData( MWContext *pContext );
EDT_PageData *EDT_NewPageData();
void EDT_SetPageData( MWContext *pContext, EDT_PageData *pData );
void EDT_FreePageData(  EDT_PageData *pData );

/* Get and Set MetaData.  
 *  Get the count of meta data objects. 
 *  enumerate through them (0 based).
 *  If the name changes, a new meta data is created or may overwrite a different
 *     name value pair.
 */
intn EDT_MetaDataCount( MWContext *pContext );
EDT_MetaData* EDT_GetMetaData( MWContext *pContext, intn n );
EDT_MetaData* EDT_NewMetaData();
void EDT_SetMetaData( MWContext *pContext, EDT_MetaData *pMetaData );
void EDT_DeleteMetaData( MWContext *pContext, EDT_MetaData *pMetaData );
void EDT_FreeMetaData( EDT_MetaData *pMetaData );

/*******************************************************
 * CLM: JAVA, PLUG-IN interface
*/

EDT_JavaData *EDT_GetJavaData( MWContext *pContext );
EDT_JavaData *EDT_NewJavaData();
void EDT_SetJavaData( MWContext *pContext, EDT_JavaData *pData );
void EDT_FreeJavaData(  EDT_JavaData *pData );

EDT_PlugInData *EDT_GetPlugInData( MWContext *pContext );
EDT_PlugInData *EDT_NewPlugInData();
void EDT_SetPlugInData( MWContext *pContext, EDT_PlugInData *pData );
void EDT_FreePlugInData(  EDT_PlugInData *pData );

/* Parameters: NAME=VALUE pairs used by both Java and PlugIns */
EDT_ParamData* EDT_GetParamData( MWContext *pContext, intn n );
EDT_ParamData* EDT_NewParamData();
void EDT_SetParamData( MWContext *pContext, EDT_ParamData *pParamData );
void EDT_DeleteParamData( MWContext *pContext, EDT_ParamData *pParamData );
void EDT_FreeParamData( EDT_ParamData *pParamData );

/*
 * Get and Set Named Anchors (Targets)
*/
char *EDT_GetTargetData( MWContext *pContext );
void EDT_SetTargetData( MWContext *pContext, char *pTargetName );
void EDT_InsertTarget( MWContext *pContext, char* pTargetName );
char *EDT_GetAllDocumentTargets( MWContext *pContext );

/*CLM: Check current file-update time and 
 *     return TRUE if it is different
 *     Save the newly-found time in edit buffer class
*/
XP_Bool EDT_IsFileModified( MWContext* pContext );

/* CLM: Read a file and build a targets list just like the current doc list */
char *EDT_GetAllDocumentTargetsInFile( MWContext *pContext, char *pHref);

/*
 * Returns a list of all local documents associated with 
 *  the current buffer.
*/
char* EDT_GetAllDocumentFiles( MWContext *pContext );

/*
 * Get and Set UnknownTags
*/
char *EDT_GetUnknownTagData( MWContext *pContext );
void EDT_SetUnknownTagData( MWContext *pContext, char *pUnknownTagData );
void EDT_InsertUnknownTag( MWContext *pContext, char* pUnknownTagData );
/* CLM: Validate: check for matching quotes, "<" and ">" if bNoBrackets is FALSE
 *      Skip (and strip out) <> if bNoBrackets is TRUE,
 *      used for Attributes-only, such as MOCHA string in HREF
*/
ED_TagValidateResult EDT_ValidateTag( char *pData, XP_Bool bNoBrackets );

/*
 * Called by the front end when the user presses the cancel button on the 
 *  Modal dialog
*/
void EDT_ImageLoadCancel( MWContext *pContext );

/*****************************************************************************
 * Property Dialogs
 *****************************************************************************/

#if 0
/*
 * String allocation functions for parameters passed to EDT_Property functions
*/
char *EDT_StringDup(char *pDupString);      
void EDT_StringFree(char* pString);


void EDT_GetPageProperties( ED_PageProperties *pProps );
void EDT_SetPageProperties( ED_PageProperties *pProps );

void EDT_GetParagraphProperties( ED_ParagraphProperties *pProps );
void EDT_SetParagraphProperties( ED_ParagraphProperties *pProps );

#endif

/*****************************************************************************
 * Utility stuff.
 *****************************************************************************/

/* for debug purposes. */
char *EDT_TagString(int32 tagType);

#ifdef DEBUG
void EDT_VerifyLayoutElement( MWContext *pContext, LO_Element *pLoElement, 
        XP_Bool bPrint );
#endif

/* Cross platform macros */
/* we may changed how we define editor status */
#define EDT_IS_NEW_DOCUMENT(context) (context != NULL && context->is_editor && context->is_new_document)
#define EDT_NEW_DOCUMENT(context,b)  if(context != NULL) context->is_new_document=(context->is_editor&&b)

/* Helper to gray UI items not allowed when inside Java Script
 * Note that the return value is TRUE if mixed selection, 
 *   allowing the non-script text to be changed.
 * Current Font Size, Color, and Character attributes will suppress
 *   setting other attributes, so it is OK to call these when mixed
*/
XP_Bool EDT_IsJavaScript(MWContext * pMWContext);

/* Helper to use for enabling Character properties 
 * (Bold, Italic, etc., but DONT use for clearing (TF_NONE)
 *  or setting Java Script (Server or Client)
 * Tests for:
 *   1. Good edit buffer and not blocked because of some action,
 *   2. Caret or selection is NOT entirely within Java Script, 
 *   3. Caret or selection has some text or is mixed selection
 *      (thus FALSE if single non-text object is selected)
*/
XP_Bool EDT_CanSetCharacterAttribute(MWContext * pMWContext);


#ifdef FIND_REPLACE

XP_Bool EDT_FindAndReplace(MWContext *pMWContext, EDT_FindAndReplaceData *pData );

#endif


XP_END_PROTOS

#endif  /* EDITOR   */
#endif  /* _edt_h_  */


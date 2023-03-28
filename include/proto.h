/* -*- Mode: C; tab-width: 8 -*- 

   This file should contain prototypes of all public functions for all
   modules in the client library.

   This file will be included automatically when source includes "client.h".
   By the time this file is included, all global typedefs have been executed.
*/

/* make sure we only include this once */
#ifndef _PROTO_H_
#define _PROTO_H_

#include "ntypes.h"

#if defined(__sun)
# include <nspr/sunos4.h>
#endif /* __sun */


XP_BEGIN_PROTOS

/* put your prototypes here..... */

/* --------------------------------------------------------------------------
 * Parser stuff
 */

extern intn PA_ParserInit(PA_Functions *);
extern NET_StreamClass *PA_BeginParseMDL(FO_Present_Types, void *,
	URL_Struct *, MWContext *);
extern intn PA_ParseBlock(void *, const char *, int);
extern void PA_MDLComplete(void *);
extern void PA_MDLAbort(void *, int);

/* --------------------------------------------------------------------------
 * Layout stuff
 */

extern LO_FormElementStruct *
LO_ReturnNextFormElement(MWContext *context,
                         LO_FormElementStruct *current_element);
extern LO_FormElementStruct *
LO_ReturnPrevFormElement(MWContext *context,
                         LO_FormElementStruct *current_element);
extern LO_FormElementStruct *
LO_ReturnNextFormElementInTabGroup(MWContext *context,
                                   LO_FormElementStruct *current_element,
						 		   XP_Bool go_backwards);

extern intn LO_ProcessTag(void *, PA_Tag *, intn);
extern void LO_RefreshArea(MWContext *context, int32 left, int32 top,
	uint32 width, uint32 height);
extern LO_Element *LO_XYToElement(MWContext *, int32, int32);
extern LO_Element *LO_XYToNearestElement(MWContext *, int32, int32);
extern void LO_MoveGridEdge(MWContext *context, LO_EdgeStruct *edge,
	int32 x, int32 y);
extern void LO_SetImageInfo(MWContext *context, int32 ele_id,
	int32 width, int32 height);
extern void LO_SetUserOverride(Bool override);
extern void LO_SetDefaultBackdrop(char *url);
extern void LO_SetDefaultColor(intn type, uint8 red, uint8 green, uint8 blue);
extern Bool LO_ParseRGB(char *rgb, uint8 *red, uint8 *green, uint8 *blue);
extern void LO_ClearBackdropBlock(MWContext *context,
	LO_ImageStruct *image, Bool fg_ok);
extern void LO_ClearEmbedBlock(MWContext *context, LO_EmbedStruct *embed);
extern Bool LO_BlockedOnImage(MWContext *, LO_ImageStruct *image);
extern void LO_CloseAllTags(MWContext *);
extern void LO_DiscardDocument(MWContext *);
extern LO_FormSubmitData *LO_SubmitForm(MWContext *context,
	LO_FormElementStruct *form_element);
extern LO_FormSubmitData *LO_SubmitImageForm(MWContext *context,
	LO_ImageStruct *image, int32 x, int32 y);
extern void LO_ResetForm(MWContext *context,
	LO_FormElementStruct *form_element);
extern LO_FormElementStruct *
LO_FormRadioSet(MWContext *context, LO_FormElementStruct *form_element);
extern void LO_SaveFormData(MWContext *context);
extern void LO_CloneFormData(SHIST_SavedData *, MWContext *context,
	URL_Struct *url_struct);
extern void LO_HighlightAnchor(MWContext *context, LO_Element *element,Bool on);
#ifdef OLD_POS_HIST
extern void LO_SetDocumentPosition(MWContext *context, int32 x, int32 y);
#endif /* OLD_POS_HIST */
extern void LO_StartSelection(MWContext *context, int32 x, int32 y);
extern void LO_ExtendSelection(MWContext *context, int32 x, int32 y);
extern void LO_EndSelection(MWContext *context);
extern void LO_ClearSelection(MWContext *context);
extern XP_Block LO_GetSelectionText(MWContext *context);
extern Bool LO_FindText(MWContext *context, char *text,
        LO_Element **start_ele_loc, int32 *start_position,
        LO_Element **end_ele_loc, int32 *end_position,
        Bool use_case, Bool forward);
extern Bool LO_FindGridText(MWContext *context, MWContext **ret_context,
	char *text,
        LO_Element **start_ele_loc, int32 *start_position,
        LO_Element **end_ele_loc, int32 *end_position,
        Bool use_case, Bool forward);
extern Bool LO_SelectAll(MWContext *context);
extern void LO_SelectText(MWContext *context, LO_Element *start,int32 start_pos,
        LO_Element *end, int32 end_pos, int32 *x, int32 *y);
extern void LO_RefreshAnchors(MWContext *context);
extern Bool LO_HaveSelection(MWContext *context);
extern void LO_GetSelectionEndpoints(MWContext *context,
	LO_Element **start, LO_Element **end, int32 *start_pos, int32 *end_pos);
extern void LO_FreeSubmitData(LO_FormSubmitData *submit_data);
extern void LO_FreeDocumentFormListData(MWContext *context, void *form_data);
extern void LO_FreeDocumentEmbedListData(MWContext *context, void *embed_data);
extern void LO_FreeDocumentGridData(MWContext *context, void *grid_data);
extern void LO_FreeDocumentAppletData(MWContext *context, void *applet_data);
extern void LO_RedoFormElements(MWContext *context);
extern void LO_InvalidateFontData(MWContext *context);
extern Bool LO_HasBGImage(MWContext *context);
extern Bool LO_LocateNamedAnchor(MWContext *context, URL_Struct *url_struct,
	int32 *xpos, int32 *ypos);
extern int32 LO_EmptyRecyclingBin(MWContext *context);
extern LO_AnchorData *LO_MapXYToAreaAnchor(MWContext *context,
	LO_ImageStruct *image, int32 x, int32 y);
extern intn LO_DocumentInfo(MWContext *context, NET_StreamClass *stream);
extern intn LO_ChangeFontSize(intn size, char *size_str);
extern int16 LO_WindowWidthInFixedChars(MWContext *context);
#ifndef NEW_FRAME_HIST
#define NEW_FRAME_HIST 1
#endif
#ifdef NEW_FRAME_HIST
extern void LO_CleanupGridHistory(MWContext *context);
extern void LO_UpdateGridHistory(MWContext *context);
extern Bool LO_BackInGrid(MWContext *context);
extern Bool LO_ForwardInGrid(MWContext *context);
extern Bool LO_GridCanGoForward(MWContext *context);
extern Bool LO_GridCanGoBackward(MWContext *context);
#endif /* NEW_FRAME_HIST */
extern Bool LO_Click( MWContext *context, int32 x, int32 y, Bool requireCaret );
extern void LO_Hit(MWContext *context, int32 x, int32 y,
	Bool requireCaret, LO_HitResult *result);
extern int32 LO_TextElementWidth(MWContext *context, LO_TextStruct *text_ele, int charOffset);

extern void LO_AddEmbedData(MWContext* context, LO_EmbedStruct* embed, void* data);
extern void LO_CopySavedEmbedData(MWContext* context, SHIST_SavedData* newdata);


#ifdef EDITOR

/* --------------------------------------------------------------------------
 * Layout stuff specific to the editor
 */

extern void LO_PositionCaret( MWContext *context, int32 x, int32 y );
extern void LO_DoubleClick( MWContext *context, int32 x, int32 y );
void LO_PositionCaretBounded(MWContext *context, int32 x, int32 y,
			int32 minY, int32 maxY );

extern ED_Buffer* LO_GetEDBuffer( MWContext *context);
extern void LO_GetEffectiveCoordinates( MWContext *pContext, LO_Element *pElement, int32 position,
     int32* pX, int32* pY, int32* pWidth, int32* pHeight );
extern void LO_UpDown( MWContext *pContext, LO_Element *pElement, int32 position, int32 iDesiredX, Bool bSelect, Bool bForward );
extern Bool LO_PreviousPosition( MWContext *pContext,LO_Element *pElement, intn iOffset, 
			ED_Element **ppEdElement, intn* pOffset);
extern Bool LO_NextPosition( MWContext *pContext,LO_Element *pElement, intn iOffset, 
			ED_Element **ppEdElement, intn* pOffset);
extern Bool LO_NavigateChunk( MWContext *pContext, intn chunkType, Bool bSelect, Bool bForward );
extern Bool LO_ComputeNewPosition( MWContext *context, intn chunkType,
    Bool bSelect, Bool bDeselecting, Bool bForward,
    LO_Element** pElement, int32* pPosition );

extern LO_Element* LO_BeginOfLine( MWContext *pContext, LO_Element *pElement );
extern LO_Element* LO_EndOfLine( MWContext *pContext, LO_Element *pElement);
extern LO_Element* LO_FirstElementOnLine( MWContext *pContext,
	int32 x, int32 y, int32 *pLineNum);
extern void LO_StartSelectionFromElement( MWContext *context, LO_Element *eptr, 
            int32 new_pos );
extern void LO_ExtendSelectionFromElement( MWContext *context, LO_Element *eptr, 
            int32 position, Bool bFromStart );
extern Bool LO_SelectElement( MWContext *context, LO_Element *eptr, 
	int32 position, Bool bFromStart );
extern void LO_SelectRegion( MWContext *context, LO_Element *begin,
	int32 beginPosition, LO_Element *end, int32 endPosition,
	Bool fromStart, Bool forward  );
extern Bool LO_IsSelected( MWContext *context );
extern Bool LO_IsSelectionStarted( MWContext *context );
extern void LO_GetSelectionEndPoints( MWContext *context, 
	LO_Element **ppStart, intn *pStartOffset, LO_Element **ppEnd, 
	intn *pEndOffset, Bool *pbFromStart, Bool *pbSingleElementSelection );
extern void LO_GetSelectionNewPoint( MWContext *context, 
	LO_Element **ppNew, intn *pNewOffset);
extern LO_Element* LO_PreviousEditableElement( LO_Element *pElement );
extern LO_Element* LO_NextEditableElement( LO_Element *pElement );
extern LO_ImageStruct* LO_NewImageElement( MWContext* context );
extern void LO_SetBackgroundImage( MWContext *context, char *pUrl );
extern void LO_RefetchWindowDimensions( MWContext *pContext );
extern void LO_Relayout( MWContext *context, ED_TagCursor *pCursor,
	int32 iLine, int iStartEditOffset, XP_Bool bDisplayTables );

#endif /*EDITOR*/

extern void LO_SetBaseURL( MWContext *pContext, char *pURL );
extern char* LO_GetBaseURL( MWContext *pContext );

extern Bool LO_Click( MWContext *context, int32 x, int32 y, Bool requireCaret );
extern void LO_Hit(MWContext *context, int32 x, int32 y, Bool requireCaret, LO_HitResult *result);
extern void LO_SelectObject( MWContext *context, int32 x, int32 y);

/*
 * This is probably the wrong place to add this, but tough,
 * this shit shouldn't have to exist anyways.
 */
extern XP_List *XP_GetGlobalContextList(void);
extern void XP_AddContextToList(MWContext *context);
extern Bool XP_IsContextInList(MWContext *context);
extern void XP_RemoveContextFromList(MWContext *context);
extern MWContext *XP_FindNamedContextInList(MWContext * context, char *name);
extern MWContext *XP_FindContextOfType(MWContext *, MWContextType);
extern MWContext *XP_FindSomeContext(void);
extern Bool XP_FindNamedAnchor(MWContext * context, URL_Struct * url, 
			       int32 *xpos, int32 *ypos);
extern void XP_RefreshAnchors(void);
extern void XP_InterruptContext(MWContext * context);
extern Bool XP_IsContextBusy(MWContext * context);
extern Bool XP_IsContextStoppable(MWContext * context);
extern void XP_UpdateParentContext(MWContext * context);
extern int XP_GetSecurityStatus(MWContext *pContext);
extern int XP_ContextCount(MWContextType cxType, XP_Bool bTopLevel);

XP_END_PROTOS

# endif /* _PROTO_H_ */


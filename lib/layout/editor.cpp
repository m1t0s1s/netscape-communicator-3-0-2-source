/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Public interface and shared subsystem data.
//

#ifdef EDITOR

#include "editor.h"

CBitArray *edt_setNoEndTag = 0;
CBitArray *edt_setWriteEndTag = 0;
CBitArray *edt_setHeadTags = 0;
CBitArray *edt_setSoloTags = 0;
CBitArray *edt_setBlockFormat = 0;
CBitArray *edt_setCharFormat = 0;
CBitArray *edt_setList = 0;
CBitArray *edt_setUnsupported = 0;
CBitArray *edt_setTextContainer = 0;
CBitArray *edt_setListContainer = 0;
CBitArray *edt_setParagraphBreak = 0;
CBitArray *edt_setFormattedText = 0;
CBitArray *edt_setContainerSupportsAlign = 0;
CBitArray *edt_setIgnoreWhiteSpace = 0;
CBitArray *edt_setSuppressNewlineBefore = 0;
CBitArray *edt_setRequireNewlineAfter = 0;


//-----------------------------------------------------------------------------
//  BitArrays
//-----------------------------------------------------------------------------

static XP_Bool bBitsInited = FALSE;

void edt_InitBitArrays() {

    if( bBitsInited ){
        return;
    }
    bBitsInited = TRUE;

    int size = P_MAX + 1; // +1 because CEditRootElements are of type P_MAX.

    edt_setNoEndTag = new CBitArray(      size, 
                                    P_TEXT,
                                    P_PARAGRAPH, 
                                    P_IMAGE, 
                                    P_NEW_IMAGE, 
                                    P_INPUT, 
                                    P_LIST_ITEM,
                                    P_HRULE,
                                    P_DESC_TITLE,
                                    P_DESC_TEXT,
                                    P_LINEBREAK,
                                    P_WORDBREAK,
                                    P_INDEX,
                                    P_BASE,
                                    P_LINK,
                                    P_META,
                                    P_GRID_CELL,
                                    BIT_ARRAY_END );

    edt_setWriteEndTag = new CBitArray(   size, 
                                    P_PARAGRAPH, 
                                    P_LIST_ITEM,
                                    P_DESC_TITLE,
                                    P_DESC_TEXT,
                                    BIT_ARRAY_END );


    edt_setHeadTags = new CBitArray(      size,
                                    P_INDEX,
                                    P_BASE,
                                    P_LINK,
                                    BIT_ARRAY_END );

    edt_setSoloTags = new CBitArray(      size,
                                    P_TEXT,
                                    P_IMAGE,
                                    P_NEW_IMAGE,
                                    P_HRULE,
                                    P_LINEBREAK,
                                    P_WORDBREAK,
                                    P_AREA,
                                    P_INPUT,
                                    BIT_ARRAY_END );


    edt_setBlockFormat = new CBitArray(   size,
                                    P_ANCHOR,
                                    P_CENTER,
                                    P_DIVISION,
                                    P_BLINK,
                                    BIT_ARRAY_END );


    //
    // Character formatting tags
    //
    edt_setCharFormat = new CBitArray(    size,
                                    P_BOLD,
                                    P_ITALIC,
                                    P_FONT,
                                    P_EMPHASIZED,
                                    P_STRONG,
                                    P_SAMPLE,
                                    P_KEYBOARD,
                                    P_VARIABLE,
                                    P_ANCHOR,
                                    P_STRIKEOUT,
                                    P_UNDERLINE,
                                    P_FIXED,
                                    P_EMPHASIZED,
                                    P_STRONG,
                                    P_CODE,
                                    P_SAMPLE,
                                    P_KEYBOARD,
                                    P_VARIABLE,
                                    P_CITATION,
                                    P_CENTER,
                                    P_BLINK,
                                    P_BIG,
                                    P_SMALL,
                                    P_SUPER,
                                    P_SUB,
                                    P_SERVER,
                                    P_SCRIPT,
                                    BIT_ARRAY_END );


    edt_setList = new CBitArray( size,
                                    P_UNUM_LIST,
                                    P_NUM_LIST,
                                    P_DESC_LIST,
                                    P_MENU,
                                    P_DIRECTORY,
                                    P_BLOCKQUOTE,
                                    BIT_ARRAY_END );

    edt_setUnsupported = new CBitArray( size,
            P_TEXTAREA,
            P_SELECT,
            P_OPTION,
            P_INPUT,
            P_EMBED,
            P_NOEMBED,
            P_JAVA_APPLET,
            P_FORM,
            P_MAP,
            P_AREA,
            P_KEYGEN,
            P_BASE,
            P_HYPE,
            P_LINK,
            P_SUBDOC,
            P_CELL,
            P_WORDBREAK,
            P_NOBREAK,
            P_COLORMAP,
            P_INDEX,
            P_PARAM,
            P_SPACER,
            P_NOSCRIPT,
            P_CERTIFICATE,
            P_PAYORDER,
#ifndef SUPPORT_MULTICOLUMN
                                    P_MULTICOLUMN,
#endif
                                    P_UNKNOWN,
                                    BIT_ARRAY_END );


    edt_setTextContainer = new CBitArray( size,
                                    P_TITLE,
                                    P_HEADER_1,
                                    P_HEADER_2,
                                    P_HEADER_3,
                                    P_HEADER_4,
                                    P_HEADER_5,
                                    P_HEADER_6,
                                    P_PARAGRAPH,
                                    P_ADDRESS,
                                    P_PREFORMAT,
                                    P_LIST_ITEM,
                                    P_DESC_TITLE,
                                    P_DESC_TEXT,
                                    BIT_ARRAY_END );

    edt_setContainerSupportsAlign = new CBitArray( size,

                                    P_HEADER_1,
                                    P_HEADER_2,
                                    P_HEADER_3,
                                    P_HEADER_4,
                                    P_HEADER_5,
                                    P_HEADER_6,

    // unfortunately, HRules end paragraph containment, implictly.  So we 
    //  either need to change the way we deal with <HR> or change the 
    //  code.  until then, we can't generate "align=center" as part of a 
    //  paragraph.
                                    //P_PARAGRAPH,

                                    //P_ADDRESS,
                                    //P_PREFORMAT,
                                    //P_LIST_ITEM,
                                    //P_DESC_TITLE,
                                    //P_DESC_TEXT,
                                    BIT_ARRAY_END );


    edt_setListContainer = new CBitArray( size,
                                    P_UNUM_LIST,
                                    P_NUM_LIST,
                                    P_MENU,
                                    P_DIRECTORY,
                                    P_BLOCKQUOTE,
                                    BIT_ARRAY_END );

    edt_setParagraphBreak = new CBitArray( size,
                                    P_TITLE,
                                    P_HEADER_1,
                                    P_HEADER_2,
                                    P_HEADER_3,
                                    P_HEADER_4,
                                    P_HEADER_5,
                                    P_HEADER_6,
                                    P_PARAGRAPH,
                                    P_LINEBREAK,
                                    P_ADDRESS,
                                    P_PREFORMAT,
                                    P_LIST_ITEM,
                                    P_UNUM_LIST,
                                    P_NUM_LIST,
                                    P_LINEBREAK,
                                    P_HRULE,
                                    P_DESC_LIST,
                                    P_DESC_TITLE,
                                    P_DESC_TEXT,
                                    BIT_ARRAY_END );


                    
    edt_setFormattedText = new CBitArray( size,
                                    P_PREFORMAT,
                                    P_PLAIN_PIECE,
                                    P_PLAIN_TEXT,
                                    P_LISTING_TEXT,
                                    BIT_ARRAY_END );

    edt_setIgnoreWhiteSpace = new CBitArray( size,
                                    P_TABLE,
                                    P_TABLE_ROW,
                                    BIT_ARRAY_END );

    //
    // Suppress printing a newline before these tags.
    //
    edt_setSuppressNewlineBefore = new CBitArray(    size,
                                    P_LINEBREAK,
                                    BIT_ARRAY_END );

    //
    // Print a newline after these tags.
    //

    edt_setRequireNewlineAfter = new CBitArray(    size,
                                    P_LINEBREAK,
                                    BIT_ARRAY_END );


}

//-----------------------------------------------------------------------------
// External Interface 
//-----------------------------------------------------------------------------

// usage:
// GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) ; // For no return value
// GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) return-value; // to return a return value

#define GET_EDIT_BUF_OR_RETURN(CONTEXT, BUFFER) CEditBuffer* BUFFER = LO_GetEDBuffer( CONTEXT );\
    if (!pEditBuffer) return

ED_Buffer* EDT_MakeEditBuffer(MWContext *pContext){
    return new CEditBuffer(pContext);
}

XP_Bool EDT_HaveEditBuffer(MWContext * pContext){
  return (LO_GetEDBuffer( pContext ) != NULL);
}

ED_FileError EDT_SaveFile( MWContext * pContext,
                           char * pSourceURL,
                           char * pDestURL,
                           XP_Bool   bSaveAs,
                           XP_Bool   bKeepImagesWithDoc,
                           XP_Bool   bAutoAdjustLinks ) {
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) ED_ERROR_BLOCKED;
    return pEditBuffer->SaveFile( pSourceURL, pDestURL, bSaveAs,
                                  bKeepImagesWithDoc, bAutoAdjustLinks );
}

void EDT_SaveCancel( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    if( pEditBuffer->m_pSaveObject){
        pEditBuffer->m_pSaveObject->Cancel();
    }
}

void EDT_SetAutoSavePeriod(MWContext *pContext, int32 minutes){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetAutoSavePeriod( minutes );
}

int32 EDT_GetAutoSavePeriod(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) -1;
    return pEditBuffer->GetAutoSavePeriod();
}

//
// we are blocked if we are blocked or saving an object.
//
XP_Bool EDT_IsBlocked( MWContext *pContext ){
    CEditBuffer *pEditBuffer = LO_GetEDBuffer( pContext );
    return (! pEditBuffer || pEditBuffer->IsBlocked() || pEditBuffer->m_pSaveObject != 0);
}

//
// Streams the edit buffer to a big buffer.
//
void EDT_DisplaySource( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);

    char *pSource = 0;

    pEditBuffer->DisplaySource();

    //pEditBuffer->WriteToBuffer( &pSource );
    //FE_DisplaySource( pContext, pSource );
}


void EDT_DestroyEditBuffer(MWContext * pContext){
    if ( pContext ) {
	    int32 doc_id;
	    lo_TopState *top_state;

	    /*
	     * Get the unique document ID, and retreive this
	     * documents layout state.
	     */
	    doc_id = XP_DOCID(pContext);
	    top_state = lo_FetchTopState(doc_id);
	    if ((top_state == NULL)||(top_state->doc_state == NULL))
	    {
		    return;
	    }
	    CEditBuffer *pEditBuffer = top_state->edit_buffer;
        delete pEditBuffer;
        top_state->edit_buffer = NULL;
    }
}


//
// Hooked into the GetURL state machine.  We do intermitent processing to 
//  let the layout engine to the initial processing and fetch all the nested
//  images.
//
// Returns: 1 - Done ok, continuing.
//    0 - Done ok, stopping.
//   -1 - not done, error.
//
intn EDT_ProcessTag(void *data_object, PA_Tag *tag, intn status){
    pa_DocData *pDocData = (pa_DocData*)data_object;
    int parseRet = OK_CONTINUE;
    intn retVal;

    if( !EDT_IS_EDITOR(pDocData->window_id) ){
        return LO_ProcessTag( data_object, tag, status );
    }

    if( pDocData->edit_buffer == 0 ){
        pDocData->edit_buffer = EDT_MakeEditBuffer( pDocData->window_id );
    }

    // if parsing is complete and we didn't end up with anything, force a NOBR
    //  space.
    if( tag == NULL && status == PA_COMPLETE ) {
        CEditRootDocElement* pRoot = pDocData->edit_buffer->m_pRoot;
        XP_Bool bNoData = ! pRoot || ! pRoot->GetChild();
        if ( bNoData ){
            pDocData->edit_buffer->DummyCharacterAddedDuringLoad();
            PA_Tag *pNewTag = XP_NEW( PA_Tag );
            XP_BZERO( pNewTag, sizeof( PA_Tag ) );

            // Non-breaking-spaces cause a memory leak when they are laid out in 3.0b5.
            // See bug 23404.
            // And anyway, we strip out this character.
            // So it can be any character.
            // edt_SetTagData( pNewTag, NON_BREAKING_SPACE_STRING );
            edt_SetTagData( pNewTag, "$" );

            pDocData->edit_buffer->ParseTag( pDocData, pNewTag, status );

            PA_FreeTag( pNewTag );
        }
    }

    if( tag ){
        parseRet = pDocData->edit_buffer->ParseTag( pDocData, tag, status );
    }

    if( parseRet != OK_IGNORE ){

#if 0
        //
        // Check to see if the tag went away.  Text without an Edit Element
        //
        // LTNOTE: this case should now return OK_IGNORE, don't check.
        //
        if( tag
                && tag->type == P_TEXT 
                && tag->data 
                && tag->edit_element == 0 )
            {
                PA_FREE( tag->data );
                return 1;
            }
#endif
        retVal = LO_ProcessTag( data_object, tag, status );
        if( tag == 0 ){
            //pDocData->edit_buffer->FinishedLoad();
        }
        return retVal;
    }
    else{
    	PA_FreeTag(tag);
        return 1;
    }
}


static char *loTypeNames[] = {
    "LO_NONE",
    "LO_TEXT",
    "LO_LINEFEED",
    "LO_HRULE",
    "LO_IMAGE",
    "LO_BULLET",
    "LO_FORM_ELE",
    "LO_SUBDOC",
    "LO_TABLE",
    "LO_CELL",
    "LO_EMBED",
};


//
// Debug routine
//
void EDT_ShowElement( LO_Element* le ){
    char buff[512];
    CEditTextElement *pText;
    XP_TRACE(("  Type:          %s", loTypeNames[le->type] ));
    XP_TRACE(("  ele_id:        %d", le->lo_any.ele_id ));
    XP_TRACE(("  edit_element   0x%x", le->lo_any.edit_element));
    XP_TRACE(("  edit_offset    %d", le->lo_any.edit_offset));
    if( le->type == LO_TEXT && le->lo_any.edit_element->IsA( P_TEXT ) ){
        pText = le->lo_any.edit_element->Text();
        XP_TRACE(("  lo_element(text)     '%s'", le->lo_text.text ));
        XP_TRACE(("  edit_element(text)   '%s'", pText->GetText()));
        strncpy( buff, pText->GetText()+le->lo_any.edit_offset,
                    le->lo_text.text_len );
        buff[le->lo_text.text_len] = '\0';
        XP_TRACE(("  edit_element text( %d, %d ) = '%s' ", 
                le->lo_any.edit_offset, le->lo_text.text_len, buff));
    }
}

void EDT_SetInsertPoint( ED_Buffer *pBuffer, ED_Element* pElement, int iPosition, XP_Bool bStickyAfter ){
    if( pBuffer->IsBlocked() ){ return; }
    if ( ! pElement->IsLeaf() ){
        XP_ASSERT(FALSE);
        return;
    }
    pBuffer->SetInsertPoint( pElement->Leaf(), iPosition, bStickyAfter );
}


void EDT_SetLayoutElement( ED_Element* pElement, intn iEditOffset, 
        intn lo_type, LO_Element *pLayoutElement ){
    // XP_TRACE(("EDT_SetLayoutElement pElement(0x%08x) iEditOffset(%d) pLayoutElement(0x%08x)", pElement, iEditOffset, pLayoutElement));
    if( pElement != 0 && pElement->IsLeaf()) {
         pElement->Leaf()->SetLayoutElement( iEditOffset, lo_type, pLayoutElement );
    }
}

void EDT_ResetLayoutElement( ED_Element* pElement, intn iEditOffset, 
        LO_Element *pLayoutElement ){
    /* The lifetime of a layout element can be greater than that of the
       corresponding edit element. An example is when you browse to the home page,
       hit edit, then cancel the save.
       When EDT_ResetLayoutElement happens, pElement
       may already have been deleted.

       We were only doing this book keeping to be neat. So it's OK to disable it.
     */
#if 0
    XP_TRACE(("EDT_ResetLayoutElement pElement(0x%08x) iEditOffset(%d) pLayoutElement(0x%08x)", pElement, iEditOffset, pLayoutElement));
    if( pElement != 0 && pElement->IsLeaf()) {
         pElement->Leaf()->ResetLayoutElement( iEditOffset, pLayoutElement );
    }
#endif
}

#ifdef DEBUG
void EDT_VerifyLayoutElement( MWContext *pContext, LO_Element *pLoElement, 
        XP_Bool bPrint ){
    CEditElement *pElement = pLoElement->lo_any.edit_element;
    if( pElement ){
        if( !pElement->IsLeaf() ){
            // die here.
        }

        LO_Element *pEle;
        intn iLayoutOffset;
        if( !pElement->Leaf()->GetLOElementAndOffset( 
                    pLoElement->lo_any.edit_offset, TRUE,
                    pEle, 
                    iLayoutOffset ) ) {
            XP_TRACE((" couldn't find layout element %8x(%d) from edit_element %8x",
                    pLoElement, pLoElement->lo_any.ele_id, pElement ));
            XP_ASSERT(FALSE);
            return;
        }

        if( pEle != pLoElement ){
            XP_TRACE(("Found wrong layout element\n"
                        "    Original Element %8x(%d)\n"
                        "    Found Element %8x(%d)\n",
                    pLoElement, pLoElement->lo_any.ele_id,
                    pEle, pEle != NULL ? pEle->lo_any.ele_id : 0));
            XP_ASSERT(FALSE);
        }        
    }   
}
#endif //DEBUG

PA_Tag* EDT_TagCursorGetNext( ED_TagCursor* pCursor ){
    return pCursor->GetNextTag();
}

PA_Tag* EDT_TagCursorGetNextState( ED_TagCursor* pCursor ){
    return pCursor->GetNextTagState();
}

XP_Bool EDT_TagCursorAtBreak( ED_TagCursor* pCursor, XP_Bool* pEndTag ){
    return pCursor->AtBreak(pEndTag);
}

int32 EDT_TagCursorCurrentLine( ED_TagCursor* pCursor ){
    return pCursor->CurrentLine();
}

XP_Bool EDT_TagCursorClearRelayoutState( ED_TagCursor* pCursor ){
    return pCursor->ClearRelayoutState();
}


ED_TagCursor* EDT_TagCursorClone( ED_TagCursor *pCursor ){
    return pCursor->Clone();
}

void EDT_TagCursorDelete( ED_TagCursor* pCursor ){
    delete pCursor;
}

//-----------------------------------------------------------------------------
// FE Keyboard interface.
//-----------------------------------------------------------------------------

EDT_ClipboardResult EDT_KeyDown( MWContext *pContext, uint16 nChar, uint16 b, uint16 c ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    if( pEditBuffer && nChar >= ' ' ){
        return pEditBuffer->InsertChar(nChar, TRUE);
    }
    return EDT_COP_OK;
}

EDT_ClipboardResult EDT_InsertNonbreakingSpace( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    EDT_ClipboardResult result = EDT_COP_OK;
    const char *p;
    // Nonbreaking space can be string for multibyte
    p = INTL_NonBreakingSpace(pContext);
    for (; *p; p++){
        result = pEditBuffer->InsertChar(*p, TRUE);
        if ( result != EDT_COP_OK) break;
    }
    return result;
}

EDT_ClipboardResult EDT_DeletePreviousChar( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->DeletePreviousChar();
}

EDT_ClipboardResult EDT_DeleteChar( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->DeleteNextChar();
}

//---------------------------------------------------------------------------
//  Navigation
//---------------------------------------------------------------------------
void EDT_PreviousChar( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    // pEditBuffer->PreviousChar( bSelect );
    pEditBuffer->NavigateChunk( bSelect, LO_NA_CHARACTER, FALSE );
}

void EDT_NextChar( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_CHARACTER, TRUE );
}

void EDT_Up( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->UpDown( bSelect, FALSE );
}

void EDT_Down( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->UpDown( bSelect, TRUE );
}

void EDT_BeginOfLine( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_LINEEDGE, FALSE );
}

void EDT_EndOfLine( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_LINEEDGE, TRUE );
}

void EDT_BeginOfDocument( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_DOCUMENT, FALSE );
}

void EDT_EndOfDocument( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_DOCUMENT, TRUE );
}

void EDT_PageUp( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    //pEditBuffer->EndOfDocument( bSelect );
}

void EDT_PageDown( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    //pEditBuffer->EndOfDocument( bSelect );
}

void EDT_PreviousWord( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_WORD, FALSE );
}

void EDT_NextWord( MWContext *pContext, XP_Bool bSelect ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->NavigateChunk( bSelect, LO_NA_WORD, TRUE );
}

//
// Kind of a bizarre routine, Order of calls is as follows:
//  FE_OnMouseDown      - determines x and y in layout coords. calls:
//  EDT_PositionCaret   - clears movement and calls
//  LO_PositionCaret    - figures out which layout element is under caret and calls
//  EDT_SetInsertPoint  - which sets the edit element and offset and then calls
//  FE_DisplayTextCaret - which actually positions the caret.
//
// There are some more levels in there, but from an external view, this is how
//  it occurs.
//
void EDT_PositionCaret( MWContext *pContext, int32 x, int32 y ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->PositionCaret(x, y);
}

void EDT_DoubleClick( MWContext *pContext, int32 x, int32 y ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->StartSelection(x,y, TRUE);
}

void EDT_SelectObject( MWContext *pContext, int32 x, int32 y ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SelectObject(x,y);
}

void EDT_WindowScrolled( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->WindowScrolled();
}

EDT_ClipboardResult EDT_ReturnKey( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->ReturnKey(TRUE);
}

void EDT_Indent( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    {
        CEditSelection selection;
        pEditBuffer->GetSelection(selection);
        if ( selection.CrossesSubDocBoundary() ) {
            return;
        }
    }
    CChangeIndentCommand* pCommand = new CChangeIndentCommand(pEditBuffer, kIndentCommandID);
    pEditBuffer->Indent();
    pEditBuffer->AdoptAndDo(pCommand);
}

void EDT_Outdent( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeIndentCommand* pCommand = new CChangeIndentCommand(pEditBuffer, kIndentCommandID);
    pEditBuffer->Outdent();
    pEditBuffer->AdoptAndDo(pCommand);
}

void EDT_ToggleList(MWContext *pContext, intn iTagType){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    EDT_ListData * pData = NULL;
	TagType nParagraphFormat = EDT_GetParagraphFormatting(pContext);
    XP_Bool bIsMyList = FALSE;

    EDT_BeginBatchChanges(pContext);

    if ( nParagraphFormat == P_LIST_ITEM ) {
        pData = EDT_GetListData(pContext);
        bIsMyList = ( pData && pData->iTagType == iTagType );
    }

    if ( nParagraphFormat == P_LIST_ITEM && bIsMyList ) {
        // WILL THIS ALONE UNDO THE LIST?
        EDT_MorphContainer(pContext, P_PARAGRAPH);
        EDT_Outdent( pContext );
    } else {
        if ( !pData ) {
            // Must be indendented
            EDT_Indent(pContext);
            // Create a numbered list item
            if (nParagraphFormat != P_LIST_ITEM ) {
                EDT_MorphContainer(pContext, P_LIST_ITEM);
            }
            pData = EDT_GetListData(pContext);
        }

        if ( pData && (pData->iTagType != iTagType) ){
            pData->iTagType = iTagType;
            pData->eType = ED_LIST_TYPE_DEFAULT;
            EDT_SetListData(pContext, pData);
        }
    }
    if ( pData ) {
        EDT_FreeListData(pData);
    }

    EDT_EndBatchChanges(pContext);
}

XP_Bool EDT_GetToggleListState(MWContext *pContext, intn iTagType)
{
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
   	TagType nParagraphFormat = EDT_GetParagraphFormatting( pContext );
    EDT_ListData * pData = NULL;
    XP_Bool bIsMyList = FALSE;
    if ( nParagraphFormat == P_LIST_ITEM ) {
        pData = EDT_GetListData(pContext);
        bIsMyList = ( pData && pData->iTagType == iTagType );
    }
    if ( pData ) {
        EDT_FreeListData( pData );
    }
    return bIsMyList;
}

void EDT_SetParagraphAlign( MWContext *pContext, ED_Alignment eAlign ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeParagraphCommand* pCommand = new CChangeParagraphCommand(pEditBuffer, kParagraphAlignCommandID);
    pEditBuffer->SetParagraphAlign( eAlign );
    pEditBuffer->AdoptAndDo(pCommand);
}

ED_Alignment EDT_GetParagraphAlign( MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) ED_ALIGN_LEFT;
    return pEditBuffer->GetParagraphAlign();
}

void EDT_MorphContainer( MWContext *pContext, TagType t ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->AdoptAndDo(new CChangeParagraphCommand(pEditBuffer, kMorphContainerCommandID));
    pEditBuffer->MorphContainer( t );
}

void EDT_FormatCharacter( MWContext *pContext, ED_TextFormat tf ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->FormatCharacter( tf );
}

EDT_CharacterData* EDT_GetCharacterData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    return pEditBuffer->GetCharacterData();
}

void EDT_SetCharacterData( MWContext *pContext, EDT_CharacterData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetCharacterData( pData );
}

EDT_CharacterData* EDT_NewCharacterData(){
    EDT_CharacterData* pData = XP_NEW( EDT_CharacterData );
    XP_BZERO( pData, sizeof( EDT_CharacterData ));
    return pData;
}

void EDT_FreeCharacterData( EDT_CharacterData *pData ){
    if( pData->pHREFData ){
        EDT_FreeHREFData( pData->pHREFData );
    }
    if( pData->pColor ){
    	XP_FREE( pData->pColor );
    }
    XP_FREE( pData );
}


ED_BitArray* EDT_BitArrayCreate(){
    return new CBitArray(P_MAX, 0);
}

void EDT_BitArrayDestroy( ED_BitArray* pBitArray){
    delete pBitArray;    
}

XP_Bool EDT_BitArrayTest( ED_BitArray* pBitArray, TagType t ){
    return (*pBitArray)[t];
}

ED_TextFormat EDT_GetCharacterFormatting( MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    return pEditBuffer->GetCharacterFormatting();
}

/* CM: "Current" is superfluous! */
ED_ElementType EDT_GetCurrentElementType( MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) ED_ELEMENT_TEXT;
    return pEditBuffer->GetCurrentElementType();
}

TagType EDT_GetParagraphFormatting( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) P_UNKNOWN;
    return pEditBuffer->GetParagraphFormatting( );
}

int EDT_GetFontSize( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    return pEditBuffer->GetFontSize();
}

void EDT_SetFontSize( MWContext *pContext, int iSize ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetFontSize(iSize);
}

//
// Some macros for packing and unpacking colors.
//

XP_Bool EDT_GetFontColor( MWContext *pContext, LO_Color* pDestColor ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    ED_Color i = pEditBuffer->GetFontColor();
    if( i.IsDefined() ){
        pDestColor->red = EDT_RED(i);
        pDestColor->green = EDT_GREEN(i);
        pDestColor->blue = EDT_BLUE(i);
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void EDT_SetFontColor( MWContext *pContext, LO_Color *pColor){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    if( pColor ){
        pEditBuffer->SetFontColor( EDT_LO_COLOR( pColor ) );
    }
    else {
        pEditBuffer->SetFontColor(ED_Color::GetUndefined());
    }
}

void EDT_StartSelection(MWContext *pContext, int32 x, int32 y){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->StartSelection(x,y, FALSE);
}

void EDT_ExtendSelection(MWContext *pContext, int32 x, int32 y){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->ExtendSelection(x,y);
}

void EDT_EndSelection(MWContext *pContext, int32 x, int32 y){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->EndSelection(x, y);
}

void EDT_SelectAll(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SelectAll();
}

void EDT_SelectTable(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SelectTable();
}

void EDT_SelectTableCell(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SelectTableCell();
}

void EDT_ClearSelection(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CEditSelection selection;
    pEditBuffer->GetSelection(selection);
    selection.m_end = selection.m_start; // Leave cursor at left, so table cell change data works.
    CSetSelectionCommand* pSelectCommand = new CSetSelectionCommand(pEditBuffer, selection);
    pEditBuffer->AdoptAndDo(pSelectCommand);
}

EDT_ClipboardResult EDT_PasteText( MWContext *pContext, char *pText ) {
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    CPasteCommand* pCommand = new CPasteCommand( pEditBuffer, kPasteTextCommandID, TRUE );
    EDT_ClipboardResult result = pEditBuffer->PasteText( pText );
    pEditBuffer->AdoptAndDo(pCommand);
    return result;
}

EDT_ClipboardResult EDT_PasteExternalHTML( MWContext *pContext, char *pText ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    CPasteCommand* pCommand = new CPasteCommand( pEditBuffer, kPasteTextCommandID, TRUE );
    EDT_ClipboardResult result = pEditBuffer->PasteExternalHTML( pText );
    pEditBuffer->AdoptAndDo(pCommand);
    return result;
}

EDT_ClipboardResult EDT_PasteHTML( MWContext *pContext, char *pText ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    CPasteCommand* pCommand = new CPasteCommand( pEditBuffer, kPasteHTMLCommandID, TRUE );
    EDT_ClipboardResult result = pEditBuffer->PasteHTML( pText, FALSE );
    pEditBuffer->AdoptAndDo(pCommand);
    return result;
}

EDT_ClipboardResult EDT_CopySelection( MWContext *pContext, char **ppText, int32* pTextLen,
            XP_HUGE_CHAR_PTR* ppHtml, int32* pHtmlLen){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->CopySelection( ppText, pTextLen, (char **)ppHtml, pHtmlLen );
}

EDT_ClipboardResult EDT_CutSelection( MWContext *pContext, char **ppText, int32* pTextLen,
            XP_HUGE_CHAR_PTR* ppHtml, int32* pHtmlLen){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    EDT_ClipboardResult result = pEditBuffer->CutSelection( ppText, pTextLen, (char **)ppHtml, pHtmlLen );
    return result;
}

XP_Bool EDT_DirtyFlag( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->GetDirtyFlag();
}

void EDT_SetDirtyFlag( MWContext *pContext, XP_Bool bValue ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetDirtyFlag( bValue );
}

EDT_ClipboardResult EDT_CanCut(MWContext *pContext, XP_Bool bStrictChecking){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->CanCut( bStrictChecking );
}

EDT_ClipboardResult EDT_CanCopy(MWContext *pContext, XP_Bool bStrictChecking){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->CanCopy( bStrictChecking );
}

EDT_ClipboardResult EDT_CanPaste(MWContext *pContext, XP_Bool bStrictChecking){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    return pEditBuffer->CanPaste( bStrictChecking );
}

XP_Bool EDT_CanSetHREF( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->CanSetHREF( );
}

char *EDT_GetHREF( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetHREF( );
}

char *EDT_GetHREFText(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetHREFText();
}

void EDT_SetHREF(MWContext *pContext, char *pHREF ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetHREF( pHREF, 0 );
}

/*
 * Returns colors of all the different fonts.
 * Must call XP_FREE( pDest ) after use.
*/
int EDT_GetExtraColors( MWContext *pContext, LO_Color **pDest ){
    return 0;
}

void EDT_RefreshLayout( MWContext *pContext ){
    LO_RefetchWindowDimensions( pContext );
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->RefreshLayout();
}

XP_Bool EDT_IsSelected( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsSelected();
}

//  Check current file-update time and 
//  return TRUE if it is different
XP_Bool EDT_IsFileModified( MWContext* pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsFileModified();
}

//   This should probably be moved into CEditBuffer,
//     Note that we return FALSE if we are text type
//     and not selected, even if caret is in a link, 
//     so don't use this to test for link.
// TODO: Move this into CEditBuffer::SelectionContainsLink()
//       after CharacterData functions are implemented
XP_Bool EDT_SelectionContainsLink( MWContext *pContext ){
    if( EDT_IsSelected(pContext) ){                             
        ED_ElementType type = EDT_GetCurrentElementType(pContext);
        if( type == ED_ELEMENT_SELECTION ){
            EDT_CharacterData *pData = EDT_GetCharacterData(pContext);
            if( pData ){
                // We aren't certain about HREF state accross
                //  entire selection if mask bit is off
                XP_Bool bUncertain = ( (pData->mask & TF_HREF) == 0 ||
                                    ((pData->mask & TF_HREF) &&
                                     (pData->values & TF_HREF)) );
                EDT_FreeCharacterData(pData);
                return bUncertain;
            }
        }
        else if ( type == ED_ELEMENT_IMAGE ){
            LO_Element *pStart;
            //
            // Grab the current selection.
            //            
            LO_GetSelectionEndPoints( pContext, &pStart, 0, 0, 0, 0, 0 );
            // Check if image has an HREF
            return (pStart && pStart->lo_image.anchor_href != NULL );
        }
    }
    return FALSE;
}

void EDT_DropHREF( MWContext *pContext, char *pHref, char* pTitle, int32 x,
            int32 y ){
    EDT_BeginBatchChanges(pContext);
    EDT_PositionCaret( pContext, x, y );
    EDT_PasteText( pContext, pHref );
    EDT_PasteText( pContext, pTitle );
    EDT_EndBatchChanges(pContext);
}

EDT_ClipboardResult EDT_PasteHREF( MWContext *pContext, char **ppHref, char **ppTitle, int iCount){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) EDT_COP_DOCUMENT_BUSY;
    EDT_ClipboardResult result = EDT_COP_DOCUMENT_BUSY;
    CPasteCommand* command = new CPasteCommand( pEditBuffer, kPasteHREFCommandID, TRUE );
    result = pEditBuffer->PasteHREF( ppHref, ppTitle, iCount );
    pEditBuffer->AdoptAndDo(command);
    return result;
}

XP_Bool EDT_CanDropHREF( MWContext *pContext, int32 x, int32 y ){
    return TRUE;
}

EDT_HREFData *EDT_GetHREFData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    EDT_HREFData* pData = EDT_NewHREFData();
    if(pData){
        //TODO: Rewrite GetHREF to handle the target!
        char *pURL = pEditBuffer->GetHREF();
        if(pURL){
            pData->pURL = XP_STRDUP(pURL);
            return pData;
        }
    }
    return NULL;
}

void EDT_SetHREFData( MWContext *pContext, EDT_HREFData *pData ){
    /* Allow clearing existing HREF with NULL pURL */
    if(pData /*&& pData->pURL*/){
        EDT_SetHREF(pContext, pData->pURL);
    }
}

EDT_HREFData *EDT_NewHREFData() {
    EDT_HREFData* pData = XP_NEW( EDT_HREFData );
    if( pData ){
        XP_BZERO( pData, sizeof( EDT_HREFData ) );
        return pData;
    }
    return NULL;
}

EDT_HREFData *EDT_DupHREFData( EDT_HREFData *pOldData) {
    EDT_HREFData* pData = EDT_NewHREFData();
    if( pOldData ){
        if(pOldData->pURL) pData->pURL = XP_STRDUP( pOldData->pURL );
        if(pOldData->pExtra) pData->pExtra = XP_STRDUP( pOldData->pExtra );
    }
    return pData;
}


void EDT_FreeHREFData(  EDT_HREFData *pData ) {
    //Move this to an EditBuffer function?
    if(pData){
        if(pData->pURL) XP_FREE(pData->pURL);
        if(pData->pExtra) XP_FREE(pData->pExtra);
        XP_FREE(pData);
    }
}

void EDT_FinishedLayout( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->FinishedLoad();
}

// Page

EDT_PageData *EDT_GetPageData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetPageData();
}

EDT_PageData *EDT_NewPageData(){
    EDT_PageData* pData = XP_NEW( EDT_PageData );
    if( pData ){
        XP_BZERO( pData, sizeof( EDT_PageData ) );
        pData->bKeepImagesWithDoc = TRUE;
        return pData;
    }
    return NULL;
}

void EDT_SetPageData( MWContext *pContext, EDT_PageData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->AdoptAndDo(new CChangePageDataCommand(pEditBuffer, kChangePageDataCommandID));
    pEditBuffer->SetPageData( pData );
}

void EDT_FreePageData( EDT_PageData *pData ){
    CEditBuffer::FreePageData( pData );
}

// Meta

intn EDT_MetaDataCount( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    return pEditBuffer->MetaDataCount();
}

EDT_MetaData* EDT_GetMetaData( MWContext *pContext, intn n ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetMetaData( n );
}

EDT_MetaData* EDT_NewMetaData(){
    EDT_MetaData *pData = XP_NEW( EDT_MetaData );
    if( pData ){
        XP_BZERO( pData, sizeof( EDT_MetaData ) );
        return pData;
    }
    return NULL;
}

void EDT_SetMetaData( MWContext *pContext, EDT_MetaData *pMetaData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->AdoptAndDo(new CSetMetaDataCommand(pEditBuffer, pMetaData, FALSE, kSetMetaDataCommandID));
}

void EDT_DeleteMetaData( MWContext *pContext, EDT_MetaData *pMetaData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->AdoptAndDo(new CSetMetaDataCommand(pEditBuffer, pMetaData, TRUE, kDeleteMetaDataCommandID));
}

void EDT_FreeMetaData( EDT_MetaData *pMetaData ){
    CEditBuffer::FreeMetaData( pMetaData );
}

// Image 

EDT_ImageData *EDT_GetImageData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetImageData();
}

void EDT_SetImageData( MWContext *pContext, EDT_ImageData *pData, XP_Bool bKeepImagesWithDoc ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(pEditBuffer, kSetImageDataCommandID);
    pEditBuffer->SetImageData( pData, bKeepImagesWithDoc );
    pEditBuffer->AdoptAndDo(pCommand);
}


EDT_ImageData *EDT_NewImageData(){
    return edt_NewImageData( );
}

void EDT_FreeImageData( EDT_ImageData *pData ){
    edt_FreeImageData( pData );
}

void EDT_InsertImage( MWContext *pContext, EDT_ImageData *pData, XP_Bool bKeepImagesWithDoc ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CPasteCommand* command = new CPasteCommand(pEditBuffer, kInsertImageCommandID, FALSE, TRUE);
    pEditBuffer->LoadImage( pData, bKeepImagesWithDoc, FALSE );
    pEditBuffer->AdoptAndDo(command);
}

void EDT_ImageLoadCancel( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    if( pEditBuffer->m_pLoadingImage ){
        delete pEditBuffer->m_pLoadingImage;
    }
}

void EDT_SetImageInfo(MWContext *pContext, int32 ele_id, int32 width, int32 height){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    if( pEditBuffer->m_pLoadingImage ){
        pEditBuffer->m_pLoadingImage->SetImageInfo( ele_id, width, height );
    }
}

// HorizRule 

EDT_HorizRuleData *EDT_GetHorizRuleData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetHorizRuleData();
}

void EDT_SetHorizRuleData( MWContext *pContext, EDT_HorizRuleData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(pEditBuffer, kSetHorizRuleDataCommandID);
    pEditBuffer->SetHorizRuleData( pData );
    pEditBuffer->AdoptAndDo(pCommand);
}

EDT_HorizRuleData* EDT_NewHorizRuleData(){
    return CEditHorizRuleElement::NewData( );
}

void EDT_FreeHorizRuleData( EDT_HorizRuleData *pData ){
    CEditHorizRuleElement::FreeData( pData );
}

void EDT_InsertHorizRule( MWContext *pContext, EDT_HorizRuleData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertHorizRule( pData );
}

// Targets
char *EDT_GetTargetData( MWContext *pContext ){
   GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
   return pEditBuffer->GetTargetData();
}

void EDT_SetTargetData( MWContext *pContext, char *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(pEditBuffer, kSetTargetDataCommandID);
    pEditBuffer->SetTargetData( pData );
    pEditBuffer->AdoptAndDo(pCommand);
}

void EDT_InsertTarget( MWContext *pContext, char* pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CPasteCommand* command = new CPasteCommand(pEditBuffer, kInsertTargetCommandID, FALSE);
    pEditBuffer->InsertTarget( pData );
    pEditBuffer->AdoptAndDo(command);
}

char *EDT_GetAllDocumentTargets( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetAllDocumentTargets();
}

char *EDT_GetAllDocumentTargetsInFile( MWContext *pContext, char *pHref ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetAllDocumentTargetsInFile(pHref);
}

char *EDT_GetAllDocumentFiles( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetAllDocumentFiles();
}


// Unknown Tags
char *EDT_GetUnknownTagData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetUnknownTagData();
}

void EDT_SetUnknownTagData( MWContext *pContext, char *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(pEditBuffer, kSetUnknownTagDataCommandID);
    pEditBuffer->SetUnknownTagData( pData );
    pEditBuffer->AdoptAndDo(pCommand);
}

void EDT_InsertUnknownTag( MWContext *pContext, char* pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CPasteCommand* command = new CPasteCommand(pEditBuffer, kInsertUnknownTagCommandID, FALSE);
    pEditBuffer->InsertUnknownTag( pData );
    pEditBuffer->AdoptAndDo(command);
}

ED_TagValidateResult EDT_ValidateTag( char *pData, XP_Bool bNoBrackets ){
    return CEditIconElement::ValidateTag( pData, bNoBrackets );
}

// List 

EDT_ListData *EDT_GetListData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetListData();
}

void EDT_SetListData( MWContext *pContext, EDT_ListData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CSetListDataCommand* pCommand = new CSetListDataCommand(pEditBuffer, *pData);
    pEditBuffer->AdoptAndDo(pCommand);
}

void EDT_FreeListData( EDT_ListData *pData ){
    CEditListElement::FreeData( pData );
}

// Break

void EDT_InsertBreak( MWContext *pContext, ED_BreakType eBreak ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    // A break is treated like a character as far as the typing command goes.
    // So we don't do a command at this level.
    pEditBuffer->InsertBreak( eBreak );
}

// Tables

XP_Bool EDT_IsInsertPointInTable(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInTable();
}

XP_Bool EDT_IsInsertPointInNestedTable(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInNestedTable();
}

EDT_TableData* EDT_GetTableData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetTableData();
}

void EDT_SetTableData( MWContext *pContext, EDT_TableData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);

    // jhp: We have to do this here, outside of the command.
    //CLM: Experimental kludge: allow changing row/columns by
    //      deleting old table and inserting a new one
    CEditInsertPoint ip;
    pEditBuffer->GetInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        pEditBuffer->BeginBatchChanges(kSetTableDataCommandID);
        EDT_TableData* pOldData = pTable->GetData();
        if ( pOldData && (pOldData->iRows != pData->iRows || 
                          pOldData->iColumns != pData->iColumns) ){
    		pEditBuffer->AdoptAndDo(new CDeleteTableCommand(pEditBuffer));
            // Get rid of extra carriage return before old table. 
            EDT_DeletePreviousChar(pContext);     
            EDT_InsertTable(pContext, pData);
        } else {
            CSetTableDataCommand* pCommand = new CSetTableDataCommand(pEditBuffer, pData);
            pEditBuffer->AdoptAndDo(pCommand);
        }
        if( pOldData ){
            CEditTableElement::FreeData(pOldData);
        }
        pEditBuffer->EndBatchChanges();
    }
}

EDT_TableData* EDT_NewTableData() {
    return CEditTableElement::NewData();
}

void EDT_FreeTableData(  EDT_TableData *pData ) {
    CEditTableElement::FreeData( pData );
}

void EDT_InsertTable( MWContext *pContext, EDT_TableData *pData){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertTable( pData );
}

void EDT_DeleteTable( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteTable();
}

// Table Caption

XP_Bool EDT_IsInsertPointInTableCaption(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInTableCaption();
}

EDT_TableCaptionData* EDT_GetTableCaptionData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetTableCaptionData();
}

void EDT_SetTableCaptionData( MWContext *pContext, EDT_TableCaptionData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CSetTableCaptionDataCommand* pCommand = new CSetTableCaptionDataCommand(pEditBuffer, pData);
    pEditBuffer->AdoptAndDo(pCommand);
}

EDT_TableCaptionData* EDT_NewTableCaptionData() {
    return CEditCaptionElement::NewData();
}

void EDT_FreeTableCaptionData(  EDT_TableCaptionData *pData ) {
    CEditCaptionElement::FreeData( pData );
}

void EDT_InsertTableCaption( MWContext *pContext, EDT_TableCaptionData *pData){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertTableCaption( pData );
}

void EDT_DeleteTableCaption( MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteTableCaption();
}

// Table Row

XP_Bool EDT_IsInsertPointInTableRow(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInTableRow();
}

EDT_TableRowData* EDT_GetTableRowData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetTableRowData();
}

void EDT_SetTableRowData( MWContext *pContext, EDT_TableRowData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CSetTableRowDataCommand* pCommand = new CSetTableRowDataCommand(pEditBuffer, pData);
    pEditBuffer->AdoptAndDo(pCommand);
}

EDT_TableRowData* EDT_NewTableRowData() {
    return CEditTableRowElement::NewData();
}

void EDT_FreeTableRowData(  EDT_TableRowData *pData ) {
    CEditTableRowElement::FreeData( pData );
}

void EDT_InsertTableRows( MWContext *pContext, EDT_TableRowData *pData, XP_Bool bAfterCurrentRow, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertTableRows( pData, bAfterCurrentRow, number );
}

void EDT_DeleteTableRows( MWContext *pContext, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteTableRows(number);
}

// Table Cell

XP_Bool EDT_IsInsertPointInTableCell(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInTableCell();
}

EDT_TableCellData* EDT_GetTableCellData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetTableCellData();
}

void EDT_SetTableCellData( MWContext *pContext, EDT_TableCellData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    CSetTableCellDataCommand* pCommand = new CSetTableCellDataCommand(pEditBuffer, pData);
    pEditBuffer->AdoptAndDo(pCommand);
}

EDT_TableCellData* EDT_NewTableCellData() {
    return CEditTableCellElement::NewData();
}

void EDT_FreeTableCellData(  EDT_TableCellData *pData ) {
    CEditTableCellElement::FreeData( pData );
}

void EDT_InsertTableCells( MWContext *pContext, EDT_TableCellData *pData, XP_Bool bAfterCurrentColumn, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertTableCells( pData, bAfterCurrentColumn, number );
}

void EDT_DeleteTableCells( MWContext *pContext, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteTableCells(number);
}

void EDT_InsertTableColumns( MWContext *pContext, EDT_TableCellData *pData, XP_Bool bAfterCurrentColumn, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertTableColumns( pData, bAfterCurrentColumn, number );
}

void EDT_DeleteTableColumns( MWContext *pContext, intn number){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteTableColumns(number);
}

// MultiColumn

XP_Bool EDT_IsInsertPointInMultiColumn(MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->IsInsertPointInMultiColumn();
}

EDT_MultiColumnData* EDT_GetMultiColumnData( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) NULL;
    return pEditBuffer->GetMultiColumnData();
}

void EDT_SetMultiColumnData( MWContext *pContext, EDT_MultiColumnData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);

    // jhp: We have to do this here, outside of the command.
    //CLM: Experimental kludge: allow changing row/columns by
    //      deleting old table and inserting a new one
    CEditInsertPoint ip;
    pEditBuffer->GetInsertPoint(ip);
    CEditMultiColumnElement* pMultiColumn = ip.m_pElement->GetMultiColumnIgnoreSubdoc();
    if ( pMultiColumn ){
        pEditBuffer->BeginBatchChanges(kSetMultiColumnDataCommandID);
        EDT_MultiColumnData* pOldData = pMultiColumn->GetData();
        if ( pOldData && (pOldData->iColumns != pData->iColumns) ){
    		pEditBuffer->AdoptAndDo(new CDeleteMultiColumnCommand(pEditBuffer));
            EDT_InsertMultiColumn(pContext, pData);
        } else {
            CSetMultiColumnDataCommand* pCommand = new CSetMultiColumnDataCommand(pEditBuffer, pData);
            pEditBuffer->AdoptAndDo(pCommand);
        }
        if( pOldData ){
            CEditMultiColumnElement::FreeData(pOldData);
        }
        pEditBuffer->EndBatchChanges();
    }
}

EDT_MultiColumnData* EDT_NewMultiColumnData() {
    return CEditMultiColumnElement::NewData();
}

void EDT_FreeMultiColumnData(  EDT_MultiColumnData *pData ) {
    CEditMultiColumnElement::FreeData( pData );
}

void EDT_InsertMultiColumn( MWContext *pContext, EDT_MultiColumnData *pData){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->InsertMultiColumn( pData );
}

void EDT_DeleteMultiColumn( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->DeleteMultiColumn();
}


// Undo/Redo

void EDT_Undo( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->Undo( );
}

void EDT_Redo( MWContext *pContext ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->Redo( );
}

intn EDT_GetCommandHistoryLimit(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) 0;
    return pEditBuffer->GetCommandHistoryLimit( );
}

void EDT_SetCommandHistoryLimit(MWContext *pContext, intn newLimit){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetCommandHistoryLimit( newLimit );
}

intn EDT_GetUndoCommandID( MWContext *pContext, intn index ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) CEDITCOMMAND_ID_NULL;
    CEditCommand* command = pEditBuffer->GetUndoCommand( index );
    if ( command ) return command->GetID();
    else return CEDITCOMMAND_ID_NULL;
}

intn EDT_GetRedoCommandID( MWContext *pContext, intn index ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) CEDITCOMMAND_ID_NULL;
    CEditCommand* command = pEditBuffer->GetRedoCommand( index );
    if ( command ) return command->GetID();
    else return CEDITCOMMAND_ID_NULL;
}

void EDT_BeginBatchChanges(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->BeginBatchChanges(kGroupOfChangesCommandID);
}

void EDT_EndBatchChanges(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->EndBatchChanges( );
}

void EDT_SetDisplayParagraphMarks(MWContext *pContext, XP_Bool display){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetDisplayParagraphMarks( display );
}

XP_Bool EDT_GetDisplayParagraphMarks(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->GetDisplayParagraphMarks();
}

void EDT_SetDisplayTables(MWContext *pContext, XP_Bool display){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer);
    pEditBuffer->SetDisplayTables( display );
}

XP_Bool EDT_GetDisplayTables(MWContext *pContext){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->GetDisplayTables();
}

#ifdef FIND_REPLACE

XP_Bool EDT_FindAndReplace(MWContext *pContext, 
                EDT_FindAndReplaceData *pData ){
    GET_EDIT_BUF_OR_RETURN(pContext, pEditBuffer) FALSE;
    return pEditBuffer->FindAndReplace( pData );

}
#endif  // FIND_REPLACE

#endif // EDITOR

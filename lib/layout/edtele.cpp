/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Public interface and shared subsystem data.
//

#ifdef EDITOR

#include "editor.h"

#define  EDT_IS_SCRIPT(tf) (0 != (tf & (TF_SERVER | TF_SCRIPT)))

//-----------------------------------------------------------------------------
// CEditElement
//-----------------------------------------------------------------------------

//
// This version of the constructor is used to create a child element.
//
CEditElement::CEditElement(CEditElement *pParent, TagType tagType, char* pData): 
        m_tagType(tagType), 
        m_pParent(pParent), 
        m_pNext(0), 
        m_pChild(0),
        m_pTagData(0)
{
    if ( pData ) {
        SetTagData(pData);
    }
    CommonConstructor();
}

CEditElement::CEditElement(CEditElement *pParent, PA_Tag *pTag) :
        m_tagType(pTag->type), 
        m_pParent(pParent), 
        m_pNext(0), 
        m_pChild(0),
        m_pTagData(0)
{
    char *locked_buff;

    PA_LOCK(locked_buff, char *, pTag->data);
    if( locked_buff && *locked_buff != '>'){
        SetTagData( locked_buff );
    }
    PA_UNLOCK(pTag->data);
    CommonConstructor();
}

void CEditElement::CommonConstructor(){
    if( m_pParent ){
        CEditElement* pE = m_pParent->GetChild();

        //
        // there already is a child, add this child to the end of the list.
        //
        if( pE ){
            while( pE->m_pNext != 0 ){
                pE = pE->m_pNext;
            }
            pE->RawSetNextSibling(this);
        }
        else {
            // make this the first child.
            m_pParent->RawSetChild(this);
        }
    }
}


CEditElement::CEditElement( IStreamIn *pIn, CEditBuffer* /* pBuffer */ ):
        m_pParent(0), 
        m_pNext(0), 
        m_pChild(0),
        m_pTagData(0)
{
    m_tagType = (TagType) pIn->ReadInt();
    m_pTagData = pIn->ReadZString();
}

CEditElement::~CEditElement(){
    Finalize();
}

void CEditElement::Finalize(){
    Unlink();
    DeleteChildren();
    if( m_pTagData ) {
        XP_FREE(m_pTagData);
        m_pTagData = NULL;
    }
}

void
CEditElement::SetTagData(char* tagData)
{
    if( m_pTagData ) {
        XP_FREE(m_pTagData);
    }
    if( tagData ){
        m_pTagData = XP_STRDUP(tagData);
    }
    else {
        m_pTagData = tagData;
    }
}

void CEditElement::StreamOut( IStreamOut *pOut ){
    pOut->WriteInt( GetElementType() );
    pOut->WriteInt( m_tagType );
    pOut->WriteZString( m_pTagData );
}

XP_Bool CEditElement::ShouldStreamSelf( CEditSelection& local, CEditSelection& selection)
{
    return ( local.EqualRange( selection ) || ! local.Contains(selection));
}

void CEditElement::PartialStreamOut( IStreamOut* pOut, CEditSelection& selection) {
    // partially stream out each child.
    CEditSelection local;
    GetAll(local);
    if ( local.Intersects(selection) ) {
        XP_Bool bWriteSelf = ShouldStreamSelf(local, selection);
        if ( bWriteSelf ) {
            StreamOut(pOut);
        }
        CEditElement* pChild;
        for ( pChild = GetChild(); pChild; pChild = pChild->GetNextSibling() ) {
            pChild->PartialStreamOut(pOut, selection);
        }
        if ( bWriteSelf ) {
            pOut->WriteInt((int32)eElementNone);
        }
    }
}

XP_Bool CEditElement::ClipToMe(CEditSelection& selection, CEditSelection& local) {
    // Returns TRUE if selection intersects with "this".
    GetAll(local);
    return local.ClipTo(selection);
}

void CEditElement::GetAll(CEditSelection& selection) {
    CEditLeafElement* pFirstMostChild = CEditLeafElement::Cast(GetFirstMostChild());
    if ( ! pFirstMostChild ) {
        XP_ASSERT(FALSE);
        return;
    }
    selection.m_start.m_pElement = GetFirstMostChild()->Leaf();
    selection.m_start.m_iPos = 0;
    CEditLeafElement* pLast = GetLastMostChild()->Leaf();
    CEditLeafElement* pNext = pLast->NextLeaf();
    if ( pNext ) {
        selection.m_end.m_pElement = pNext;
        selection.m_end.m_iPos = 0;
    }
    else {
     // edge of document. Can't select any further.
        selection.m_end.m_pElement = pLast;
        selection.m_end.m_iPos = pLast->GetLen();
    }
    selection.m_bFromStart = FALSE;
}

EEditElementType CEditElement::GetElementType()
{
    return eElement;
}

// 
// static function calls the appropriate stream constructor
//

CEditElement* CEditElement::StreamCtor( IStreamIn *pIn, CEditBuffer *pBuffer ){
    CEditElement* pResult = StreamCtorNoChildren(pIn, pBuffer);
    if ( pResult ) {
        pResult->StreamInChildren(pIn, pBuffer);
    }
    return pResult;
}

void CEditElement::StreamInChildren(IStreamIn* pIn, CEditBuffer* pBuffer){
    CEditElement* pChild;
    while ( (pChild = CEditElement::StreamCtor(pIn, pBuffer)) != NULL ) {
        pChild->InsertAsLastChild(this);
    }
}

CEditElement* CEditElement::StreamCtorNoChildren( IStreamIn *pIn, CEditBuffer *pBuffer ){

    EEditElementType eType = (EEditElementType) pIn->ReadInt();
    switch( eType ){
        case eElementNone:
            return 0;

        case eElement:
            return new CEditElement( pIn, pBuffer );

        case eTextElement:
            return new CEditTextElement( pIn, pBuffer  );

        case eImageElement:
            return new CEditImageElement( pIn, pBuffer  );

        case eHorizRuleElement:
            return new CEditHorizRuleElement( pIn, pBuffer  );

        case eBreakElement:
            return new CEditBreakElement( pIn, pBuffer  );

        case eContainerElement:
            return new CEditContainerElement( pIn, pBuffer  );

        case eListElement:
            return new CEditListElement( pIn, pBuffer  );

        case eIconElement:
            return new CEditIconElement( pIn, pBuffer  );

        case eTargetElement:
            return new CEditTargetElement( pIn, pBuffer  );

        case eTableElement:
             return new CEditTableElement( pIn, pBuffer  );

        case eCaptionElement:
             return new CEditCaptionElement( pIn, pBuffer  );

        case eTableRowElement:
             return new CEditTableRowElement( pIn, pBuffer  );

        case eTableCellElement:
             return new CEditTableCellElement( pIn, pBuffer  );

        case eMultiColumnElement:
             return new CEditMultiColumnElement( pIn, pBuffer  );

       default:
            XP_ASSERT(0);
    }
    return 0;
}


//
// Scan up the tree looking to see if we are within 'tagType'.  If we stop and
//  we are not at the top of the tree, we found the tag we are looking for.
//
XP_Bool CEditElement::Within( int tagType ){
    CEditElement* pParent = GetParent();
    while( pParent && pParent->GetType() != tagType ){
        pParent = pParent->GetParent();
    }
    return (pParent != 0);
}

CEditBuffer* CEditElement::GetEditBuffer(){
    CEditRootDocElement *pE = GetRootDoc();
    if( pE ){
        return pE->GetBuffer();
    }
    else {
        return 0;
    }
}

XP_Bool CEditElement::InFormattedText(){
    CEditElement* pParent = GetParent();

    if( IsA( P_TEXT) && (Text()->m_tf & (TF_SERVER|TF_SCRIPT)) != 0 ){
        return TRUE;
    }

    while( pParent && BitSet( edt_setFormattedText, pParent->GetType() ) == 0 ){
        pParent = pParent->GetParent();
    }
    return (pParent != 0);
}


//
// Fills in the data member of the tag, as well as the type informaiton.
//
void CEditElement::SetTagData( PA_Tag* pTag, char* pTagData){
    PA_Block buff;
    char *locked_buff;
    int iLen;

    pTag->type = m_tagType;
    pTag->edit_element = this;

    iLen = XP_STRLEN(pTagData);
    buff = (PA_Block)PA_ALLOC((iLen+1) * sizeof(char));
    if (buff != NULL)
    {
        PA_LOCK(locked_buff, char *, buff);
        XP_BCOPY(pTagData, locked_buff, iLen);
        locked_buff[iLen] = '\0';
        PA_UNLOCK(buff);
    }
    else { 
        // LTNOTE: out of memory, should throw an exception
        return;
    }

    pTag->data = buff;
    pTag->data_len = iLen;
    pTag->next = NULL;
    return;
}

PA_Tag* CEditElement::TagOpen( int /* iEditOffset */ ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    if( GetTagData() ){
        SetTagData( pTag, GetTagData() );
    }
    else {
        SetTagData( pTag, ">" );
    }
    return pTag;
}

PA_Tag* CEditElement::TagEnd( ){
    if( TagHasClose( m_tagType ) || BitSet( edt_setWriteEndTag, m_tagType ) ){
        PA_Tag *pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        pTag->type = m_tagType;
        pTag->is_end = TRUE;
        pTag->edit_element = this;
        return pTag;
    }
    return 0;
}


XP_Bool CEditElement::Reduce( CEditBuffer* /* pBuffer */ ){
    if( !BitSet( edt_setSoloTags,  GetType()  ) &&  GetChild() == 0 ){
        return TRUE;
    }
    else if( BitSet( edt_setCharFormat,  GetType()  ) ){
        CEditElement *pNext = GetNextSibling();
        if( pNext && pNext->GetType() == GetType() ){
            // FONTs and Anchors need better compares than this.
            Merge(pNext);

            // make sure it stays in the tree so it dies a natural death (because
            //  it has no children)
            pNext->InsertAfter(this);
            return FALSE;
        }
    }
    return FALSE;
}

int CEditElement::GetDefaultFontSize(){
    TagType t = GetType();
    if( !BitSet( edt_setTextContainer,  t  ) ){
        CEditElement* pCont = FindContainer();
        if( pCont ){
            t = pCont->GetType();
        }
        else {
            return 0;       // no default font size
        }
    }

    switch( t ){
        case P_HEADER_1:
            return 6;
        case P_HEADER_2:
            return 5;
        case P_HEADER_3:
            return 4;
        case P_HEADER_4:
            return 3;
        case P_HEADER_5:
            return 2; 
        case P_HEADER_6:
            return 1;
        default:
            return 3;
    }
}

CEditInsertPoint CEditElement::IndexToInsertPoint(ElementIndex index, XP_Bool bStickyAfter) {
    if ( index < 0 ) {
        XP_ASSERT(FALSE);
        index = 0;
    }
    CEditElement* pChild;
    CEditElement* pLastChild = NULL;
    ElementIndex childCount = 0;
    // XP_TRACE(("IndexToInsertPoint: 0x%08x (%d) index = %d", this, GetElementIndex(), index));
    for ( pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        pLastChild = pChild;
        childCount = pChild->GetPersistentCount();
        if ( index < childCount
            || (index == childCount && ! bStickyAfter) ){
            return pChild->IndexToInsertPoint(index, bStickyAfter);
        }
            
        index -= childCount;
    }
    if ( ! pLastChild ) { // No children at all
        childCount = GetPersistentCount();
        if ( index > childCount ){
            XP_ASSERT(FALSE);
            index = childCount;
        }
        return CEditInsertPoint(this, index);
    }
    // Ran off end of children
    return pLastChild->IndexToInsertPoint(childCount, bStickyAfter);
}

CPersistentEditInsertPoint CEditElement::GetPersistentInsertPoint(ElementOffset offset){
    XP_ASSERT(FALSE);   // This method should never be called
    return CPersistentEditInsertPoint(GetElementIndex() + offset);
}

ElementIndex CEditElement::GetPersistentCount()
{
    ElementIndex count = 0;
    for ( CEditElement* c = GetChild();
        c;
        c = c->GetNextSibling() ) {
        count += c->GetPersistentCount();
    }
    return count;
}

ElementIndex CEditElement::GetElementIndex()
{
    CEditElement* parent = GetParent();
    if ( parent )
        return parent->GetElementIndexOf(this);
    else
        return 0;
}

ElementIndex CEditElement::GetElementIndexOf(CEditElement* child)
{
    ElementIndex index = GetElementIndex();
    for ( CEditElement* c = GetChild();
        c;
        c = c->GetNextSibling() ) {
        if ( child == c )
        {
            return index;
        }
        index += c->GetPersistentCount();
    }
    XP_ASSERT(FALSE); // Couldn't find this child.
    return index;
}

void CEditElement::FinishedLoad( CEditBuffer* pBuffer ){
    CEditElement* pNext;
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pNext ) {
        pNext = pChild->GetNextSibling();
        if ( IsAcceptableChild(*pChild) ){
            pChild->FinishedLoad(pBuffer);
        }
        else {
#ifdef DEBUG
            XP_TRACE(("Removing an unacceptable child. Parent type %d child type %d.\n",
                GetElementType(), pChild->GetElementType()));
#endif
            delete pChild;
        }
    }
}

void CEditElement::EnsureSelectableSiblings(CEditBuffer* pBuffer)
{
    // To ensure that we can edit before or after the table,
    // make sure that there is a container before and after the table.

    CEditElement* pParent = GetParent();
    if ( ! pParent ) {
        return;
    }
    {
        // Make sure the previous sibling exists and is a container
        CEditElement* pPrevious = GetPreviousSibling();
        if ( ! pPrevious || !pPrevious->IsContainer() ) {
            pPrevious = CEditContainerElement::NewDefaultContainer( NULL, 
                            pParent->GetDefaultAlignment() );
            pPrevious->InsertBefore(this);
			pPrevious->FinishedLoad(pBuffer);
        }
    }
    {
        // Make sure the next sibling exists and is container
        CEditElement* pNext = GetNextSibling();
        if ( ! pNext || pNext->IsEndContainer() || !pNext->IsContainer() ) {
            pNext = CEditContainerElement::NewDefaultContainer( NULL, 
                            pParent->GetDefaultAlignment() );
            pNext->InsertAfter(this);
			pNext->FinishedLoad(pBuffer);
        }
    }
}

//-----------------------------------------------------------------------------
//  Reverse navagation (left)
//-----------------------------------------------------------------------------

// these routines are a little expensive.  If we need to, we can make the linked
//  lists of elements, doubly linked.
//
CEditElement* CEditElement::GetPreviousSibling(){
    if( GetParent() == 0 ){
        return 0;
    }

    // point to our first sibling.
    CEditElement *pSibling = GetParent()->GetChild();

    // if we are the first sibling, then there is no previous sibling.
    if ( pSibling == this ){
        return 0;
    }

    // if we get an Exception in this loop, the tree is fucked up!
    while( pSibling->GetNextSibling() != this ){
        pSibling = pSibling->GetNextSibling();
    }
    return pSibling;
    
}

void CEditElement::SetChild(CEditElement *pChild){
#if 0
#ifdef DEBUG
    if ( pChild ){
        XP_ASSERT(IsAcceptableChild(*pChild));
    }
#endif
#endif
    RawSetChild(pChild);
}

void CEditElement::SetNextSibling(CEditElement* pNext){
#if 0
#ifdef DEBUG
    if ( m_pParent && pNext) {
        XP_ASSERT(m_pParent->IsAcceptableChild(*pNext));
    }
#endif
#endif
    RawSetNextSibling(pNext);
}

CEditElement* CEditElement::GetLastChild(){
    CEditElement* pChild;
    if( (pChild = GetChild()) == 0 ){
        return 0;
    }
    while( pChild->GetNextSibling() ){
        pChild = pChild->GetNextSibling();
    }
    return pChild;
}

CEditElement* CEditElement::GetFirstMostChild(){
    CEditElement* pChild = this;
    CEditElement* pPrev = pChild;
    while( pPrev ){
        pChild = pPrev;
        pPrev = pPrev->GetChild();
    }
    return pChild;
}

CEditElement* CEditElement::GetLastMostChild(){
    CEditElement* pChild = this;
    CEditElement* pNext = pChild;
    while( pNext ){
        pChild = pNext;
        pNext = pNext->GetLastChild();
    }
    return pChild;
}

CEditTableCellElement* CEditElement::GetTableCell() {
    // Returns containing cell, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTableCell() ) {
            break;
        }
        if ( pElement->IsSubDoc() && pElement != this ) {
            return NULL;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableCellElement*) pElement;
}

CEditTableCellElement* CEditElement::GetTableCellIgnoreSubdoc() {
    // Returns containing cell, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTableCell() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableCellElement*) pElement;
}

CEditTableRowElement* CEditElement::GetTableRow() {
    // Returns containing cell, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTableRow() ) {
            break;
        }
        if ( pElement->IsSubDoc() && pElement != this ) {
            return NULL;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableRowElement*) pElement;
}

CEditTableRowElement* CEditElement::GetTableRowIgnoreSubdoc() {
    // Returns containing cell, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTableRow() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableRowElement*) pElement;
}

CEditCaptionElement* CEditElement::GetCaption() {
    // Returns containing tavle, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsCaption() ) {
            break;
        }
        if ( pElement->IsSubDoc() && pElement != this ) {
            return NULL;
        }
        pElement = pElement->GetParent();
    }
    return (CEditCaptionElement*) pElement;
}

CEditCaptionElement* CEditElement::GetCaptionIgnoreSubdoc() {
    // Returns containing table, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsCaption() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditCaptionElement*) pElement;
}

CEditTableElement* CEditElement::GetTable() {
    // Returns containing table, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTable() ) {
            break;
        }
        if ( pElement->IsSubDoc() && pElement != this ) {
            return NULL;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableElement*) pElement;
}

CEditTableElement* CEditElement::GetTableIgnoreSubdoc() {
    // Returns containing table, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTable() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditTableElement*) pElement;
}

CEditElement* CEditElement::GetTopmostTableOrMultiColumn() {
    // Returns containing table, or NULL if none.
    CEditElement* pResult = NULL;
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTable() || pElement->IsMultiColumn()) {
            pResult = pElement;
        }
        pElement = pElement->GetParent();
    }
    return pResult;
}

CEditElement* CEditElement::GetTableOrMultiColumnIgnoreSubdoc() {
    // Returns containing table, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsTable() || pElement->IsMultiColumn() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return pElement;
}

CEditElement* CEditElement::GetSubDocOrMultiColumnSkipRoot() {
    // Returns containing sub-doc, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsMultiColumn() ||
            (pElement->IsSubDoc() && !pElement->IsRoot() ) ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return pElement;
}

CEditMultiColumnElement* CEditElement::GetMultiColumn() {
    // Returns containing MultiColumn, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsMultiColumn() ) {
            break;
        }
        if ( pElement->IsSubDoc() && pElement != this ) {
            return NULL;
        }
        pElement = pElement->GetParent();
    }
    return (CEditMultiColumnElement*) pElement;
}

CEditMultiColumnElement* CEditElement::GetMultiColumnIgnoreSubdoc() {
    // Returns containing MultiColumn, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsMultiColumn() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditMultiColumnElement*) pElement;
}

CEditSubDocElement* CEditElement::GetSubDoc() {
    // Returns containing sub-doc, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsSubDoc() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditSubDocElement*) pElement;
}

CEditSubDocElement* CEditElement::GetSubDocSkipRoot() {
    // Returns containing sub-doc, or NULL if none.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsSubDoc() && !pElement->IsRoot() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditSubDocElement*) pElement;
}


CEditRootDocElement* CEditElement::GetRootDoc(){
    // Returns containing root.
    CEditElement* pElement = this;
    while ( pElement ) {
        if ( pElement->IsRoot() ) {
            break;
        }
        pElement = pElement->GetParent();
    }
    return (CEditRootDocElement*) pElement;
}

ED_Alignment CEditElement::GetDefaultAlignment(){
    if ( m_pParent )
        return m_pParent->GetDefaultAlignment();
/*
    return ED_ALIGN_LEFT;
*/
    return ED_ALIGN_DEFAULT;
}

ED_Alignment CEditElement::GetDefaultVAlignment(){
    if ( m_pParent )
        return m_pParent->GetDefaultVAlignment();
    return ED_ALIGN_TOP;
}

CEditElement* CEditElement::UpLeft( PMF_EditElementTest pmf, void *pTestData ){
    CEditElement *pPrev = this;
    CEditElement* pRet;

    while( (pPrev = pPrev->GetPreviousSibling()) != NULL ){
        pRet = pPrev->DownLeft( pmf, pTestData );
        if( pRet ){
            return pRet;
        }
    }
    if( GetParent() ){
        return GetParent()->UpLeft( pmf, pTestData );
    }
    else{
        return 0;
    }
}

//
// All the children come before the node.
//
CEditElement* CEditElement::DownLeft( PMF_EditElementTest pmf, void *pTestData, 
            XP_Bool /* bIgnoreThis */ ){
    CEditElement *pChild;
    CEditElement *pRet;

    pChild = GetLastChild();
    while( pChild != NULL ){
        if( (pRet = pChild->DownLeft( pmf, pTestData )) != NULL ){
            return pRet;
        }
        pChild = pChild->GetPreviousSibling();
    }
    if( TestIsTrue( pmf, pTestData ) ){ 
        return this;
    }
    return 0;
}


CEditElement* CEditElement::FindPreviousElement( PMF_EditElementTest pmf, 
        void *pTestData ){
    return UpLeft( pmf, pTestData );
}

//-----------------------------------------------------------------------------
// forward navagation (right)
//-----------------------------------------------------------------------------
CEditElement* CEditElement::UpRight( PMF_EditElementTest pmf, void *pTestData ){
    CEditElement *pNext = this;
    CEditElement* pRet;

    while( (pNext = pNext->GetNextSibling()) != NULL ){
        pRet = pNext->DownRight( pmf, pTestData );
        if( pRet ){
            return pRet;
        }
    }
    if( GetParent() ){
        return GetParent()->UpRight( pmf, pTestData );
    }
    else{
        return 0;
    }
}


CEditElement* CEditElement::DownRight( PMF_EditElementTest pmf, void *pTestData, 
            XP_Bool bIgnoreThis ){

    CEditElement *pChild;
    CEditElement *pRet;

    if( !bIgnoreThis && TestIsTrue( pmf, pTestData ) ){ 
        return this;
    }
    pChild = GetChild();
    while( pChild != NULL ){
        if( (pRet = pChild->DownRight( pmf, pTestData )) != NULL ){
            return pRet;
        }
        pChild = pChild->GetNextSibling();
    }
    return 0;
}

CEditElement* CEditElement::FindNextElement( PMF_EditElementTest pmf, 
        void *pTestData ){
    CEditElement *pRet;

    pRet = DownRight( pmf, pTestData, TRUE );
    if( pRet ){
        return pRet;
    }
    return UpRight( pmf, pTestData );
}


// 
// routine looks for a valid text block for positioning during editing.
//
XP_Bool CEditElement::FindText( void* /*pVoid*/ ){ 
    CEditElement *pPrev;

    //
    // if this is a text block and the layout element actually points to 
    //  something, return it.
    //
    if( GetType() == P_TEXT ){
        CEditTextElement *pText = Text();
        if( pText->GetLen() == 0 ){
            //
            // Find only empty text blocks that occur at the beginning of
            //  a paragraph (as a paragraph place holder)
            //
            pPrev = FindPreviousElement( &CEditElement::FindTextAll, 0 );
            if( pPrev && pPrev->FindContainer() == FindContainer() ){
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

XP_Bool CEditElement::FindImage( void* /*pVoid*/ ){ 
    return IsImage() ;
}

XP_Bool CEditElement::FindTarget( void* /*pVoid*/ ){ 
    return GetElementType() == eTargetElement ;
}

// 
// routine looks for a valid text block for positioning during editing.
//
XP_Bool CEditElement::FindLeaf( void* pVoid ){ 
    if( !IsLeaf() ){
        return FALSE;
    }
    if( IsA(P_TEXT) ){
        return FindText( pVoid );
    }
    else {
        return TRUE;
    }
}


XP_Bool CEditElement::FindTextAll( void* /*pVoid*/ ){ 

    //
    // if this is a text block and the layout element actually points to 
    //  something, return it.
    //
    if( GetType() == P_TEXT ){
        return TRUE;
    }
    return FALSE;
}

XP_Bool CEditElement::FindLeafAll( void* /*pVoid*/ ){ 

    //
    // if this is a text block and the layout element actually points to 
    //  something, return it.
    //
    if( IsLeaf() ){
        return TRUE;
    }
    return FALSE;
}

XP_Bool CEditElement::SplitContainerTest( void* /*pVoid*/ ){ 
    return BitSet( edt_setTextContainer,  GetType()  );
}

XP_Bool CEditElement::SplitFormattingTest( void* pVoid ){ 
    return (void*)GetType() == pVoid;
}

XP_Bool CEditElement::FindContainer( void* /*pVoid*/ ){
    return IsContainer();
}


//-----------------------------------------------------------------------------
//  Default printing routines.
//-----------------------------------------------------------------------------

void CEditElement::PrintOpen( CPrintState *pPrintState ){
    InternalPrintOpen(pPrintState, m_pTagData);
}

void CEditElement::InternalPrintOpen( CPrintState *pPrintState, char* pTagData ){
    if( !(BitSet( edt_setCharFormat, GetType())
        || BitSet( edt_setSuppressNewlineBefore, GetType())) ){
        pPrintState->m_pOut->Write( "\n", 1 );
        pPrintState->m_iCharPos = 0;
    }

    if( pTagData && *pTagData != '>' ){
        char *pStr = pTagData;
        while( *pStr == ' ' ) pStr++;
        // Trim trailing white-space in-place
        {
            intn len = XP_STRLEN(pStr);
            while ( len > 1 && pStr[len-2] == ' ') {
                len--;
            }
            if ( len > 1 ) {
                pStr[len-1] = '>';
                pStr[len] = '\0';
            }
        }
            
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s %s", 
                EDT_TagString(GetType()),pStr);
    }
    else {
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s>", 
                EDT_TagString(GetType()) );
    }

    if ( BitSet( edt_setRequireNewlineAfter, GetType()) ){
        pPrintState->m_pOut->Write( "\n", 1 );
        pPrintState->m_iCharPos = 0;
    }
}

void CEditElement::PrintEnd( CPrintState *pPrintState ){
    if( TagHasClose( GetType() ) || BitSet( edt_setWriteEndTag,  GetType()  ) ){
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "</%s>", 
                EDT_TagString(GetType()) );
        if( !BitSet( edt_setCharFormat, GetType() ) ){
            pPrintState->m_pOut->Write( "\n", 1 );
            pPrintState->m_iCharPos = 0;
        }
    }
}


//-----------------------------------------------------------------------------
// Insertion routines
//-----------------------------------------------------------------------------
CEditElement* CEditElement::InsertAfter( CEditElement *pPrev ){
    XP_ASSERT(m_pParent == NULL);
    m_pParent = pPrev->GetParent();
    SetNextSibling(pPrev->GetNextSibling());
    pPrev->SetNextSibling(this);
    return m_pParent;
}

CEditElement* CEditElement::InsertBefore( CEditElement *pNext ){
    XP_ASSERT(m_pParent == NULL);
    XP_ASSERT(pNext != NULL);
    CEditElement *pPrev = pNext->GetPreviousSibling();
    if( pPrev == 0 ){
        InsertAsFirstChild( pNext->GetParent() );
    }
    else {
        m_pParent = pNext->GetParent();
        SetNextSibling(pPrev->m_pNext);
        pPrev->SetNextSibling(this);
    }
    return m_pParent;
}


void CEditElement::InsertAsFirstChild( CEditElement *pParent ){
    XP_ASSERT(m_pParent == NULL);
    m_pParent = pParent;
    SetNextSibling(pParent->GetChild());
    pParent->SetChild(this);
}

void CEditElement::InsertAsLastChild( CEditElement *pParent ){
    XP_ASSERT(m_pParent == NULL);
    m_pParent = pParent;
    SetNextSibling( 0 );
    CEditElement *pPrev = pParent->GetLastChild();
    if( pPrev == 0 ){
        pParent->SetChild(this);
    }
    else {
        pPrev->SetNextSibling(this);
    }
}



CEditElement* CEditElement::Split( CEditElement *pLastChild, 
            CEditElement* pCloneTree,
            PMF_EditElementTest pmf,
            void *pData ){

    CEditElement *pClone = Clone();
    pClone->SetChild(pCloneTree);

    if( pLastChild->m_pNext ){
        if( pCloneTree != 0 ){
            pCloneTree->SetNextSibling(pLastChild->m_pNext);
        }
        else {
            pClone->SetChild(pLastChild->m_pNext);
        }
        pLastChild->SetNextSibling( 0 );    
    }

    //
    // Reparent all the children that have been moved to the clone.
    //
    CEditElement* pNext = pClone->m_pChild;
    while( pNext ){
        pNext->m_pParent = pClone;
        pNext = pNext->GetNextSibling();
    }


    //
    // If we are at the container point
    //
    if( TestIsTrue( pmf, pData ) ){
        return pClone->InsertAfter( this );
    }
    else {
        return GetParent()->Split( this, pClone, pmf, pData );
    }

}

CEditElement* CEditElement::Clone( CEditElement *pParent ){
    return new CEditElement(pParent, GetType(), GetTagData());
}


//
// Copied formatting, returns the bottom of the chain (Child most formatting
//  element)
//
CEditElement* CEditElement::CloneFormatting( TagType endType ){
    if( GetType() == endType ){
        return 0;
    }
    else {
        return Clone( GetParent()->CloneFormatting(endType) );
    }
}

XP_Bool CEditElement::IsFirstInContainer(){
    CEditElement *pPrevious, *pContainer, *pPreviousContainer;

    pPrevious = PreviousLeaf();
    if( pPrevious ){
        pContainer = FindContainer();
        pPreviousContainer = pPrevious->FindContainer();
        if( pContainer != pPreviousContainer ){
            return TRUE;       
        }
        else {
            return FALSE;
        }
    }
    else {
        return TRUE;
    }
}

CEditTextElement* CEditElement::PreviousTextInContainer(){
    CEditElement *pPrevious, *pContainer, *pPreviousContainer;
    pPrevious = FindPreviousElement( &CEditElement::FindText, 0 );
    if( pPrevious ){
        pContainer = FindContainer();
        pPreviousContainer = pPrevious->FindContainer();
        if( pContainer != pPreviousContainer ){
            return 0;
        }
        else {
            return pPrevious->Text();
        }
    }
    else {
        return 0;
    }
}


CEditTextElement* CEditElement::TextInContainerAfter(){
    CEditElement *pNext, *pContainer, *pNextContainer;

    pNext = FindNextElement( &CEditElement::FindText, 0 );
    if( pNext ){
        pContainer = FindContainer();
        pNextContainer = pNext->FindContainer();
        if( pContainer != pNextContainer ){
            return 0;       
        }
        else {
            return pNext->Text();
        }
    }
    else {
        return 0;
    }
}

CEditLeafElement* CEditElement::PreviousLeafInContainer(){
    CEditElement *pPrevious, *pContainer, *pPreviousContainer;
    pPrevious = PreviousLeaf();
    if( pPrevious ){
        pContainer = FindContainer();
        pPreviousContainer = pPrevious->FindContainer();
        if( pContainer != pPreviousContainer ){
            return 0;
        }
        else {
            return pPrevious->Leaf();
        }
    }
    else {
        return 0;
    }
}


CEditLeafElement* CEditElement::LeafInContainerAfter(){
    CEditElement *pNext, *pContainer, *pNextContainer;

    pNext = NextLeaf();
    if( pNext ){
        pContainer = FindContainer();
        pNextContainer = pNext->FindContainer();
        if( pContainer != pNextContainer ){
            return 0;       
        }
        else {
            return pNext->Leaf();
        }
    }
    else {
        return 0;
    }
}

CEditElement* CEditElement::GetRoot(){
    CEditElement* pRoot = this;
    while ( pRoot ) {
        CEditElement* pParent = pRoot->GetParent();
        if ( ! pParent ) break;
        pRoot = pParent;
    }
    return pRoot;
}

CEditElement* CEditElement::GetCommonAncestor(CEditElement* pOther){
    if ( this == pOther ) return this;

    // Start at root, and trickle down to find where they diverge

    CEditElement* pRoot = GetRoot();
    CEditElement* pRootOther = pOther->GetRoot();
    if ( pRoot != pRootOther ) return NULL;

    CEditElement* pCommon = pRoot;

    while ( pCommon ){
        CEditElement* pAncestor = pCommon->GetChildContaining(this);
        CEditElement* pAncestorOther = pCommon->GetChildContaining(pOther);
        if ( pAncestor != pAncestorOther ) break;
        pCommon = pAncestor;
    }
    return pCommon;
}

CEditElement* CEditElement::GetChildContaining(CEditElement* pDescendant){
    CEditElement* pParent = pDescendant;
    while ( pParent ) {
        CEditElement* pTemp = pParent->GetParent();
        if ( pTemp == this ) break; // Our direct child
        pParent = pTemp;
    }
    return pParent;
}

//
// Unlink from parent, but keep children
//
void CEditElement::Unlink(){
    CEditElement* pParent = GetParent();
    if( pParent ){
        CEditElement *pPrev = GetPreviousSibling();
        if( pPrev ){
            pPrev->SetNextSibling( GetNextSibling() );
        }
        else {
            pParent->SetChild(GetNextSibling());
        }
        m_pParent = 0;
        SetNextSibling( 0 );
    }

}

// LTNOTE: 01/01/96
//  We take take the characteristics of the thing that follows this paragraph
//  instead of keeping this paragraphs characteristics.
//
// jhp -- paste takes characteristic of second, but backspace takes
// characteristic of first, so we need a flag to control it.
// bCopyAppendAttributes == TRUE for the second (cut/paste)
// bCopyAppendAttributes == FALSE for the first (backspace/delete)

void CEditElement::Merge( CEditElement *pAppendBlock, XP_Bool bCopyAppendAttributes ){
    CEditElement* pChild = GetLastChild();
    CEditElement* pAppendChild = pAppendBlock->GetChild();

    // LTNOTE: 01/01/96 - I don't think this should be happening anymore.
    //   The way we deal with leaves and containers should keep this from 
    //   occuring...
    //
    // Check to see if pAppendBlock is a child of this.
    //    
    // LTNOTE: this is a case where we have
    // UL: 
    //   text: some text 
    //   LI: 
    //      text: XXX some More text
    //   LI:
    //      text: and some more.
    //
    // Merge occurs before xxx
    //  
    CEditElement *pAppendParent = pAppendBlock->GetParent();
    CEditElement *pAppendBlock2 = pAppendBlock;
    XP_Bool bDone = FALSE;
    while( !bDone && pAppendParent ){
        if( pAppendParent == this ){
            pChild = pAppendBlock2->GetPreviousSibling();            
            bDone = TRUE;
        }
        else {
            pAppendBlock2 = pAppendParent;
            pAppendParent = pAppendParent->GetParent();
        }
    }

    pAppendBlock->Unlink();

    if( pChild == 0 ){
        m_pChild = pAppendChild;
    }
    else {
        CEditElement *pOldNext = pChild->m_pNext;
        pChild->SetNextSibling( pAppendChild );

        // kind of sleazy.  we haven't updated the link, GetLastChild will
        //  return the end of pAppendBlock's children.
        GetLastChild()->SetNextSibling( pOldNext );
    }
    pAppendBlock->m_pChild = 0;

    while( pAppendChild ){
        pAppendChild->m_pParent = this;
        pAppendChild = pAppendChild->GetNextSibling();
    }

    //
    // LTNOTE: whack the container type to reflect the thing we just pulled 
    //  up. 
    //
    //XP_ASSERT( IsContainer() && pAppendBlock->IsContainer() );
    if( bCopyAppendAttributes && IsContainer() && pAppendBlock->IsContainer() ){
        Container()->CopyAttributes( pAppendBlock->Container() );
    }

    delete pAppendBlock;
}


#if 0
void CEditElement::Merge( CEditElement *pAppendBlock ){
    CEditElement* pChild = GetLastChild();
    CEditElement* pAppendChild = pAppendBlock->GetChild();

    //
    // Check to see if pAppendBlock is a child of this.
    //    
    // LTNOTE: this is a case where we have
    // UL: 
    //   text: some text 
    //   LI: 
    //      text: XXX some More text
    //   LI:
    //      text: and some more.
    //
    // Merge occurs before xxx
    //  
    CEditElement *pAppendParent = pAppendBlock->GetParent();
    CEditElement *pAppendBlock2 = pAppendBlock;
    XP_Bool bDone = FALSE;
    while( !bDone && pAppendParent ){
        if( pAppendParent == this ){
            pChild = pAppendBlock2->GetPreviousSibling();            
            bDone = TRUE;
        }
        else {
            pAppendBlock2 = pAppendParent;
            pAppendParent = pAppendParent->GetParent();
        }
    }

    pAppendBlock->Unlink();

    if( pChild == 0 ){
        m_pChild = pAppendChild;
    }
    else {
        CEditElement *pOldNext = pChild->m_pNext;
        pChild->m_pNext = pAppendChild;

        // kind of sleazy.  we haven't updated the link, GetLastChild will
        //  return the end of pAppendBlock's children.
        GetLastChild()->m_pNext = pOldNext;
    }
    pAppendBlock->m_pChild = 0;

    while( pAppendChild ){
        pAppendChild->m_pParent = this;
        pAppendChild = pAppendChild->GetNextSibling();
    }
    // delete pAppendBlock;
}
#endif

// By default, yes for any non-atomic tag
XP_Bool CEditElement::IsContainerContainer(){
    return !BitSet( edt_setNoEndTag, GetType());
}

//
// Search up the tree for the element that contains us.
// - start with us.
CEditContainerElement* CEditElement::FindContainer(){
    CEditElement *pRet = this;
    while( pRet ){
        if ( pRet->IsSubDoc() && pRet != this ) break;
        if( pRet->IsContainer() ){
            return pRet->Container();
        }
        pRet = pRet->GetParent();
    }
    return 0;
}

//
// Search up the tree for the element that contains us.
// - skip us.

CEditContainerElement* CEditElement::FindEnclosingContainer(){
    CEditElement *pRet = this->GetParent();
    if ( pRet ){
        return pRet->FindContainer();
    }
    return 0;
}

#if 0
CEditElement* CEditElement::FindList(){
    CEditElement *pRet = this;
    while( pRet = pRet->GetParent() ){
        if( pRet->IsList() ){
            return pRet;
        }
    }
    return 0;
}
#endif


void CEditElement::FindList( CEditContainerElement*& pContainer, 
            CEditListElement*& pList ) {
    
    pList = 0;
    XP_Bool bDone = FALSE;
    pContainer = FindEnclosingContainer();
    while( !bDone ){
        if( pContainer->GetParent()->IsList() ){
            pList = pContainer->GetParent()->List();
            bDone = TRUE;
        }
        else {
            CEditElement *pParentContainer = pContainer->FindEnclosingContainer();
            if( pParentContainer ){
                pContainer = pParentContainer->Container();
            }
            else {
                bDone = TRUE;
            }
        }
    }
}


CEditElement* CEditElement::FindContainerContainer(){
    CEditElement *pRet = this;
    while( pRet ){
        // Don't need explicit subdoc test because
        // sub-docs are paragraph containers.
        if ( pRet->IsContainerContainer() ){
            return pRet;
        }
        pRet = pRet->GetParent();
    }
    return 0;
}


#ifdef DEBUG
void CEditElement::ValidateTree(){
    CEditElement* pChild;
    CEditElement* pLoopFinder;
    // Make sure that all of our children point to us.
    // Makes sure all children are valid.
    // Make sure we don't have an infinite loop of children.
    // Use a second pointer that goes
    // around the loop twice as fast -- if it ever catches up with
    // pChild, there's a loop. (And yes, since you're wondering,
    // we did run into infinite loops here...)
    pChild = GetChild();
    pLoopFinder = pChild;
    while( pChild ){
        if( pChild->GetParent() != this ){
            XP_ASSERT(FALSE);
        }
        XP_ASSERT(IsAcceptableChild(*pChild));
        pChild->ValidateTree();
        for ( int i = 0; i < 2; i++ ){
            if ( pLoopFinder ) {
                pLoopFinder = pLoopFinder->GetNextSibling();
                if (pLoopFinder == pChild) {
                    XP_ASSERT(FALSE);
                    return;
                }
            }
        }
        pChild = pChild->GetNextSibling();
    }
}
#endif

CEditElement* CEditElement::Divide(int /* iOffset */){
    return this;
}

XP_Bool CEditElement::DeleteElement(CEditElement* pTellIfKilled){
    CEditElement *pKill = this;
    CEditElement *pParent;
    XP_Bool bKilled = FALSE;

    do {
        pParent = pKill->GetParent();
        pKill->Unlink();
        if( pKill == pTellIfKilled ){
            bKilled = TRUE;
        }
        delete pKill;
        pKill = pParent;
    } while( pKill->GetChild() == 0 );
    return bKilled;
}

void CEditElement::DeleteChildren(){
    CEditElement* pNext;
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pNext ) {
        pNext = pChild->GetNextSibling();
        pChild->Unlink();
        pChild->DeleteChildren();
        delete pChild;
    }
}

// class CEditSubDocElement

CEditSubDocElement::CEditSubDocElement(CEditElement *pParent, int tagType, char* pData)
    : CEditElement(pParent, tagType, pData)
{
}

CEditSubDocElement::CEditSubDocElement(CEditElement *pParent, PA_Tag *pTag)
    : CEditElement(pParent, pTag)
{
}

CEditSubDocElement::CEditSubDocElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditElement(pStreamIn, pBuffer)
{
}

void CEditSubDocElement::FinishedLoad( CEditBuffer* pBuffer ){
    CEditElement* pChild = GetChild();
    if ( ! pChild ) {
        // Subdocs have to have children.
        // Put an empty paragraph into any empty subdoc.
        pChild = CEditContainerElement::NewDefaultContainer( this, 
                        GetDefaultAlignment() );
        new CEditTextElement(pChild, 0);
    }
    // Put any leaves into a container.
    CEditContainerElement* pContainer = NULL;
    CEditElement* pNext;
    for ( ;
        pChild;
        pChild = pNext) {
        pNext = pChild->GetNextSibling(); // We might unlink pChild
        if ( ! IsAcceptableChild(*pChild) ){
            if ( pChild->IsLeaf() ){
                if ( ! pContainer ){
                    pContainer = CEditContainerElement::NewDefaultContainer(NULL, GetDefaultAlignment());
                    pContainer->InsertAfter(pChild);
                }
                pChild->Unlink();
                pChild->InsertAsLastChild(pContainer);
            }
            else {
                XP_TRACE(("CEditSubDocElement: deleteing child of unknown type %d.\n",
                    pChild->GetElementType()));
                delete pChild;
                pChild = NULL;
            }
        }
        else {
            if ( pContainer ){
                pContainer->FinishedLoad(pBuffer);
                pContainer = NULL;
            }
        }
        if ( pChild ) {
            pChild->FinishedLoad(pBuffer);
        }
    }
    if ( pContainer ){
        pContainer->FinishedLoad(pBuffer);
        pContainer = NULL;
    }
}

XP_Bool CEditSubDocElement::Reduce( CEditBuffer* /* pBuffer */ ){
    return FALSE;
}

// class CEditTableElement

CEditTableElement::CEditTableElement(intn columns, intn rows)
    : CEditElement(0, P_TABLE, 0),
      m_backgroundColor(),
      m_align(ED_ALIGN_LEFT),
      m_malign(ED_ALIGN_DEFAULT)
{
    EDT_TRY {
        for (intn row = 0; row < rows; row++ ){
            CEditTableRowElement* pRow = new CEditTableRowElement(columns);
            pRow->InsertAsFirstChild(this);
        }
    }
    EDT_CATCH_ALL {
        Finalize();
        EDT_RETHROW;
    }
}

CEditTableElement::CEditTableElement(CEditElement *pParent, PA_Tag *pTag, ED_Alignment align)
    : CEditElement(pParent, P_TABLE),
      m_backgroundColor(),
      m_align(align),
      m_malign(ED_ALIGN_DEFAULT)
{
    switch( m_align ) {
    case ED_ALIGN_CENTER:
        /* OK case -- 'center' tag. */
        m_align = ED_ALIGN_ABSCENTER;
        break;
    case ED_ALIGN_LEFT:
    case ED_ALIGN_ABSCENTER:
    case ED_ALIGN_RIGHT:
        break;
    default:
        XP_ASSERT(FALSE);
        /* Falls through */
    case ED_ALIGN_DEFAULT:
        m_align = ED_ALIGN_LEFT;
        break;
    }
    if( pTag ){
        EDT_TableData *pData = ParseParams(pTag);
        pData->align = m_align;
        SetData(pData);
        FreeData(pData);
    }
}

CEditTableElement::CEditTableElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditElement(pStreamIn, pBuffer),
      m_backgroundColor()
{
    m_align = (ED_Alignment) pStreamIn->ReadInt();
    m_malign = (ED_Alignment) pStreamIn->ReadInt();
}

PA_Tag* CEditTableElement::TagOpen( int iEditOffset ){
    return InternalTagOpen(iEditOffset, FALSE);
}

PA_Tag* CEditTableElement::InternalTagOpen( int iEditOffset, XP_Bool bForPrinting){
    PA_Tag *pRet = 0;
    PA_Tag* pTag;

    // create the DIV tag if we need to.
    if( m_align == ED_ALIGN_ABSCENTER || m_align == ED_ALIGN_RIGHT ){
        pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        if( m_align== ED_ALIGN_RIGHT ){
            SetTagData( pTag, "ALIGN=right>");
            pTag->type = P_DIVISION;
        }
        else {
            SetTagData( pTag, ">");
            pTag->type = P_CENTER;
        }
        pRet = pTag;
    }
    // create the actual table tag

    EDT_TableData* pTableData = GetData();
    char* pTagData = CreateTagData(pTableData, bForPrinting);
    if ( pTagData ) {
        pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        SetTagData( pTag, pTagData );
        XP_FREE(pTagData);
    }
    else {
        pTag = CEditElement::TagOpen( iEditOffset );
    }
    FreeData(pTableData);

    // link the tags together.
    if( pRet == 0 ){
        pRet = pTag;
    }
    else {
        pRet->next = pTag;
    }
    return pRet;
}

PA_Tag* CEditTableElement::TagEnd( ){
    PA_Tag *pRet = CEditElement::TagEnd();
    if( m_align == ED_ALIGN_ABSCENTER || m_align == ED_ALIGN_RIGHT ){
        PA_Tag* pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        pTag->is_end = TRUE;
        if( m_align == ED_ALIGN_RIGHT ){
            pTag->type = P_DIVISION;
        }
        else {
            pTag->type = P_CENTER;
        }

        if( pRet == 0 ){
            pRet = pTag;
        }
        else {
            pRet->next = pTag;
        }
    }
    return pRet;
}

intn CEditTableElement::GetRows(){
    intn rows = 0;
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsTableRow() ) {
            rows++;
        }
    }
    return rows;
}

intn CEditTableElement::GetColumns(){
    intn maxCols = 0;
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsTableRow() ){
            intn cols = pChild->TableRow()->GetCells();
            if (cols > maxCols) {
                maxCols = cols;
            }
        }
    }
    return maxCols;
}

void CEditTableElement::InsertRows(intn iPosition, intn number, CEditTableElement* pSource){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn rows = GetRows();
    intn cols = GetColumns();
    if ( number < 0 || iPosition < 0 || iPosition > rows){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    if ( iPosition == 0 ) {
        // Insert at the start.
        for ( intn row = 0; row < number; row++ ){
            CEditTableRowElement* pRow;
            if ( pSource ) {
                pRow = pSource->FindRow(number - row - 1);
                pRow->Unlink();
            }
            else {
                pRow = new CEditTableRowElement(cols);
            }
            pRow->InsertAsFirstChild(this);
            AdjustCaption();
        }
    }
    else {
        CEditTableRowElement* pExisting = FindRow(iPosition-1);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        for ( intn row = 0; row < number; row++ ){
            CEditTableRowElement* pRow;
            if ( pSource ) {
                pRow = pSource->FindRow(0);
                pRow->Unlink();
            }
            else {
                pRow = new CEditTableRowElement(cols);
            }
            pRow->InsertAfter(pExisting);
        }
    }
}

void CEditTableElement::InsertColumns(intn iPosition, intn number, CEditTableElement* pSource){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn rows = GetRows();
    intn cols = GetColumns();
    if ( number < 0 || iPosition < 0 || iPosition > cols){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    for(intn row = 0; row < rows; row++ ) {
        CEditTableRowElement* pExisting = FindRow(row);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        CEditTableRowElement* pSourceRow = pSource ? pSource->FindRow(0) : NULL;
        pExisting->InsertCells(iPosition, number, pSourceRow);
        delete pSourceRow;
    }
}

void CEditTableElement::DeleteRows(intn iPosition, intn number, CEditTableElement* pUndoContainer){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn rows = GetRows();
    if ( number < 0 || iPosition < 0 || iPosition >= rows
        || iPosition + number > rows){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    for ( intn row = iPosition + number - 1; row >= iPosition; row-- ) {
        CEditTableRowElement* pExisting = FindRow(row);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        if ( pUndoContainer ) {
            pExisting->Unlink();
            pExisting->InsertAsFirstChild(pUndoContainer);
        }
        else {
            delete pExisting;
        }
    }
}

void CEditTableElement::DeleteColumns(intn iPosition, intn number, CEditTableElement* pUndoContainer){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn rows = GetRows();
    intn cols = GetColumns();
    if ( number < 0 || iPosition < 0 || iPosition >= cols
        || iPosition + number > cols){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    for ( intn row = 0; row < rows; row++ ) {
        CEditTableRowElement* pExisting = FindRow(row);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        CEditTableRowElement* pUndoRow = NULL;
        if ( pUndoContainer ) {
            pUndoRow = new CEditTableRowElement();
            pUndoRow->InsertAsLastChild(pUndoContainer);
        }
        pExisting->DeleteCells(iPosition, number, pUndoRow);
    }
}

CEditTableRowElement* CEditTableElement::FindRow(intn number){
    intn count = 0;
    if ( number < 0 ) {
        XP_ASSERT(FALSE);
        return NULL;
    }
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsTableRow() ) {
            if ( count++ == number ) {
                return (CEditTableRowElement*) pChild;
            }
        }
    }
    return NULL;
}

CEditTableCellElement* CEditTableElement::FindCell(intn column, intn row) {
    CEditTableRowElement* pRow = FindRow(row);
    if ( pRow ) {
        return pRow->FindCell(column);
    }
    return NULL;
}

CEditCaptionElement* CEditTableElement::GetCaption(){
    // Normally first or last, but we check everybody to be robust.
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsCaption() ) {
            return (CEditCaptionElement*) pChild;
        }
    }
    return NULL;
}

void CEditTableElement::SetCaption(CEditCaptionElement* pCaption){
    DeleteCaption();
    if ( pCaption ){
        EDT_TableCaptionData* pData = pCaption->GetData();
        XP_Bool bAddAtTop = ! ( pData->align == ED_ALIGN_BOTTOM || pData->align == ED_ALIGN_ABSBOTTOM);
        CEditCaptionElement::FreeData(pData);
        if ( bAddAtTop ){
            pCaption->InsertAsFirstChild(this);
        }
        else {
            pCaption->InsertAsLastChild(this);
        }
    }
}

void CEditTableElement::DeleteCaption(){
    CEditCaptionElement* pOldCaption = GetCaption();
    delete pOldCaption;
}

void CEditTableElement::FinishedLoad( CEditBuffer* pBuffer ){
    // From experimentation, we know that the 2.0 navigator dumps
    // tags that aren't in td, tr, or caption cells into the doc
    // before the table.
    //
    // That's a little too radical for me. So what I do is
    // wrap tds in trs, put anything else
    // into a caption

    CEditCaptionElement* pCaption = NULL;
    CEditTableRowElement* pRow = NULL;
    CEditElement* pNext;
    
    if ( GetRows() <= 0 ) {
        // Force a row
        CEditTableRowElement* pTempRow = new CEditTableRowElement();
        pTempRow->InsertAsFirstChild(this);
    }
    
	CEditElement* pChild;
    for ( pChild = GetChild();
        pChild;
        pChild = pNext) {
        pNext = pChild->GetNextSibling(); // We might unlink pChild
        if ( ! IsAcceptableChild(*pChild) ){
            if ( pChild->IsTableCell() ){
                if ( ! pRow ){
                    pRow = new CEditTableRowElement();
                    pRow->InsertAfter(pChild);
                }
                pChild->Unlink();
                pChild->InsertAsLastChild(pRow);
                if ( pCaption ){
                    pCaption->FinishedLoad(pBuffer);
                    pCaption = NULL;
                }
            }
            else {
                // Put random children into a caption
                if ( ! pCaption ){
                    pCaption = new CEditCaptionElement();
                    pCaption->InsertAfter(pChild);
                }
                pChild->Unlink();
                pChild->InsertAsLastChild(pCaption);
                if ( pRow ){
                    pRow->FinishedLoad(pBuffer);
                    pRow = NULL;
                }
            }
        }
        else {
            if ( pRow ){
                pRow->FinishedLoad(pBuffer);
                pRow = NULL;
            }
            if ( pCaption ){
                pCaption->FinishedLoad(pBuffer);
                pCaption = NULL;
            }
        }
        pChild->FinishedLoad(pBuffer);
    }
    if ( pRow ){
        pRow->FinishedLoad(pBuffer);
        pRow = NULL;
    }
    if ( pCaption ){
        pCaption->FinishedLoad(pBuffer);
        pCaption = NULL;
    }

    // Merge all captions together.
    pCaption = NULL;
    for ( pChild = GetChild();
        pChild;
        pChild = pNext) {
        pNext = pChild->GetNextSibling(); // We might unlink pChild
        if ( pChild->IsCaption() ){
            if ( pCaption ) {
                // This is an extra caption. Merge its contents into the other caption.
                pChild->Unlink();
                CEditElement* pItem;
                while ( ( pItem = pChild->GetChild() ) != NULL ){
                    pItem->Unlink();
                    pItem->InsertAsLastChild(pCaption);
                }
                delete pChild;
            }
            else {
                pCaption = (CEditCaptionElement*) pChild;
            }
        }
    }
    AdjustCaption();

    EnsureSelectableSiblings(pBuffer);
}

void CEditTableElement::AdjustCaption() {
    CEditCaptionElement* pCaption = GetCaption();
    if ( pCaption ) {
        // Now move the caption to the start or the end of the table, as appropriate.
        pCaption->Unlink();
        SetCaption(pCaption);
    }
}

void CEditTableElement::StreamOut( IStreamOut *pOut){
    CEditElement::StreamOut( pOut );
    pOut->WriteInt( (int32)m_align );
    pOut->WriteInt( (int32)m_malign );
}

void CEditTableElement::SetData( EDT_TableData *pData ){
    char *pNew = CreateTagData(pData, FALSE);
    CEditElement::SetTagData( pNew );
    m_align = pData->align;
    m_malign = pData->malign; // Keep track of alignment seperately to avoid crashing when editing.
    if ( pNew ) {
        free(pNew); // XP_FREE?
    }
}

char* CEditTableElement::CreateTagData(EDT_TableData *pData, XP_Bool bPrinting) {
    char *pNew = 0;
    m_align = pData->align;
    m_malign = pData->malign;
    if( bPrinting && pData->malign != ED_ALIGN_DEFAULT ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s ", EDT_AlignString(pData->malign) );
    }
    if ( pData->iBorderWidth ){
        pNew = PR_sprintf_append( pNew, "BORDER=%d ", pData->iBorderWidth );
    }
    if ( pData->iCellSpacing != 1 ){
        pNew = PR_sprintf_append( pNew, "CELLSPACING=%d ", pData->iCellSpacing );
    }
    if ( pData->iCellPadding != 1 ){
        pNew = PR_sprintf_append( pNew, "CELLPADDING=%d ", pData->iCellPadding );
    }
    if( pData->bWidthDefined ){
        if( pData->bWidthPercent ){
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld%%\" ", 
                                      (long)min(100,max(1,pData->iWidth)) );
        }
        else {
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld\" ", (long)pData->iWidth );
        }
    }
    if( pData->bHeightDefined ){
        if( pData->bHeightPercent ){
            pNew = PR_sprintf_append( pNew, "HEIGHT=\"%ld%%\" ", 
                                      (long)min(100,max(1, pData->iHeight)) );
        }
        else {
            pNew = PR_sprintf_append( pNew, "HEIGHT=\"%ld\" ", (long)pData->iHeight );
        }
    }
    if ( pData->pColorBackground ) {
        SetBackgroundColor(EDT_LO_COLOR(pData->pColorBackground));
        pNew = PR_sprintf_append( pNew, "BGCOLOR=\"#%06lX\" ", GetBackgroundColor().GetAsLong() );
    }
    else {
        SetBackgroundColor(ED_Color::GetUndefined());
    }
    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }

    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }
    return pNew;
}

EDT_TableData* CEditTableElement::GetData( ){
    EDT_TableData *pRet;
    PA_Tag* pTag = CEditElement::TagOpen(0);
    pRet = ParseParams( pTag );
    // Alignment isn't kept in the tag data, because tag data is used for editing,
    // and we crash if we try to edit an aligned table, because layout doesn't
    // set up the data structures that we need.
    pRet->align = m_align;
    pRet->malign = m_malign;
    PA_FreeTag( pTag );
    return pRet;
}

static char *tableParams[] = {
    PARAM_ALIGN,
    PARAM_BORDER,
    PARAM_CELLSPACE,
    PARAM_CELLPAD,
    PARAM_WIDTH,
    PARAM_HEIGHT,
    PARAM_BGCOLOR,
    0
};

EDT_TableData* CEditTableElement::ParseParams( PA_Tag *pTag ){
    EDT_TableData *pData = NewData();
    ED_Alignment malign;
    
    malign = edt_FetchParamAlignment( pTag, ED_ALIGN_DEFAULT );
    // Center not supported in 3.0
    if( malign == ED_ALIGN_RIGHT || malign == ED_ALIGN_LEFT || malign == ED_ALIGN_DEFAULT ){
        pData->malign = malign;
    }

    pData->iRows = GetRows();
    pData->iColumns = GetColumns();
    pData->iBorderWidth = edt_FetchParamInt(pTag, PARAM_BORDER, 0);
    pData->iCellSpacing = edt_FetchParamInt(pTag, PARAM_CELLSPACE, 1);
    pData->iCellPadding = edt_FetchParamInt(pTag, PARAM_CELLPAD, 1);
    pData->bWidthDefined = edt_FetchDimension( pTag, PARAM_WIDTH, 
                    &pData->iWidth, &pData->bWidthPercent,
                    100, TRUE );
    pData->bHeightDefined = edt_FetchDimension( pTag, PARAM_HEIGHT, 
                    &pData->iHeight, &pData->bHeightPercent,
                    100, TRUE );
    pData->pColorBackground = edt_MakeLoColor(edt_FetchParamColor( pTag, PARAM_BGCOLOR ));
    pData->pExtra = edt_FetchParamExtras( pTag, tableParams );

    return pData;
}

EDT_TableData* CEditTableElement::NewData(){
    EDT_TableData *pData = XP_NEW( EDT_TableData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->iRows = 0;
    pData->iColumns = 0;
    pData->align = ED_ALIGN_LEFT;
    pData->malign = ED_ALIGN_DEFAULT;
    pData->iBorderWidth = 0;
    pData->iCellSpacing = 1;
    pData->iCellPadding = 1;
    pData->bWidthDefined = FALSE;
    pData->bWidthPercent = TRUE;
    pData->iWidth = 100;
    pData->bHeightDefined = FALSE;
    pData->bHeightPercent = TRUE;
    pData->iHeight = 100;
    pData->pColorBackground = 0;
    pData->pExtra = 0;
    return pData;
}

void CEditTableElement::FreeData( EDT_TableData *pData ){
    if( pData->pColorBackground ) XP_FREE( pData->pColorBackground );
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

void CEditTableElement::SetBackgroundColor( ED_Color iColor ){
    m_backgroundColor = iColor;
}

ED_Color CEditTableElement::GetBackgroundColor(){
    return m_backgroundColor;
}

void CEditTableElement::PrintOpen( CPrintState *pPrintState ){
    pPrintState->m_iCharPos = 0;
    pPrintState->m_pOut->Printf( "\n"); 

    PA_Tag *pTag = InternalTagOpen(0, TRUE);
    while( pTag ){
        char *pData = 0;
        if( pTag->data ){
            PA_LOCK( pData, char*, pTag->data );
        }

        if( pData && *pData != '>' ) {
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s %s", 
                    EDT_TagString(pTag->type), pData );
        }
        else {
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s>", 
                    EDT_TagString(pTag->type));
        }

        if( pTag->data ){
            PA_UNLOCK( pTag->data );
        }

        PA_Tag* pDelTag = pTag;
        pTag = pTag->next;
        PA_FreeTag(pDelTag);
    }
}

void CEditTableElement::PrintEnd( CPrintState *pPrintState ){
    PA_Tag *pTag = TagEnd();
    while( pTag ){
        char *pData = 0;
        if( pTag->data ){
            PA_LOCK( pData, char*, pTag->data );
        }

        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "</%s>",
            EDT_TagString( pTag->type ) );

        if( pTag->data ){
            PA_UNLOCK( pTag->data );
        }

        PA_Tag* pDelTag = pTag;
        pTag = pTag->next;
        PA_FreeTag(pDelTag);
    }

    pPrintState->m_iCharPos = 0;
    pPrintState->m_pOut->Printf( "\n"); 
}

// class CEditTableRowElement

CEditTableRowElement::CEditTableRowElement()
    : CEditElement((CEditElement*) NULL, P_TABLE_ROW),
      m_backgroundColor()
{
}

CEditTableRowElement::CEditTableRowElement(intn columns)
    : CEditElement((CEditElement*) NULL, P_TABLE_ROW),
      m_backgroundColor()
{
    EDT_TRY {
        for ( intn column = 0; column < columns; column++ ) {
            CEditTableCellElement* pCell = new CEditTableCellElement();
            pCell->InsertAsFirstChild(this);
        }
    }
    EDT_CATCH_ALL {
        Finalize();
        EDT_RETHROW;
    }
}

CEditTableRowElement::CEditTableRowElement(CEditElement *pParent, PA_Tag *pTag)
    : CEditElement(pParent, P_TABLE_ROW),
      m_backgroundColor()
{
    if( pTag ){
        char *locked_buff;
            
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditTableRowElement::CEditTableRowElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditElement(pStreamIn, pBuffer),
      m_backgroundColor()
{
}

intn CEditTableRowElement::GetCells(){
    intn cells = 0;
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsTableCell() ){
            cells++;
        }
    }
    return cells;
}

intn CEditTableRowElement::GetRowIndex(){
    CEditTableElement* pTable = GetTable();
    intn rowIndex = 0;
    if ( pTable ) {
        for ( CEditElement* pChild = pTable->GetChild();
            pChild;
            pChild = pChild->GetNextSibling()) {
            if ( pChild == this ){
                return rowIndex;
            }
            if ( pChild->IsTableRow() ){
                rowIndex++;
            }
        }
        XP_ASSERT(FALSE);
    }
    else {
        XP_ASSERT(FALSE);
    }
    return rowIndex;
}

void CEditTableRowElement::FinishedLoad( CEditBuffer* pBuffer ){
    // Wrap anything that's not a table cell in a table cell.
    CEditTableCellElement* pCell = NULL;
    CEditElement* pNext;
    
    if ( ! GetChild() ) {
        // Force a cell
        CEditTableCellElement* pTempCell = new CEditTableCellElement();
        pTempCell->InsertAsFirstChild(this);
    }

    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pNext) {
        pNext = pChild->GetNextSibling(); // We might unlink pChild
        if ( ! IsAcceptableChild(*pChild) ){
            if ( ! pCell ){
                pCell = new CEditTableCellElement();
                pCell->InsertAfter(pChild);
            }
            pChild->Unlink();
            pChild->InsertAsLastChild(pCell);
        }
        else {
            if ( pCell ){
                pCell->FinishedLoad(pBuffer);
                pCell = NULL;
            }
        }
        pChild->FinishedLoad(pBuffer);
    }
    if ( pCell ){
        pCell->FinishedLoad(pBuffer);
        pCell = NULL;
    }
}

void CEditTableRowElement::InsertCells(intn iPosition, intn number, CEditTableRowElement* pSource){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn cells = GetCells();
    if ( number < 0 || iPosition < 0 || iPosition > cells){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    if ( iPosition == 0 ) {
        // Insert at the start.
        for ( intn row = 0; row < number; row++ ){
            CEditTableCellElement* pCell;
            if ( pSource ) {
                pCell = pSource->FindCell(number - row - 1);
                pCell->Unlink();
            }
            else {
                pCell = new CEditTableCellElement();
            }
            pCell->InsertAsFirstChild(this);
        }
    }
    else {
        CEditTableCellElement* pExisting = FindCell(iPosition-1);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        for ( intn row = 0; row < number; row++ ){
            CEditTableCellElement* pCell;
            if ( pSource ) {
                pCell = pSource->FindCell(0);
                pCell->Unlink();
            }
            else {
                pCell = new CEditTableCellElement();
            }
            pCell->InsertAfter(pExisting);
        }
    }
}

void CEditTableRowElement::DeleteCells(intn iPosition, intn number, CEditTableRowElement* pUndoContainer){
    if ( number == 0 ) {
        return; /* A waste of time, but legal. */
    }
    intn cells = GetCells();
    if ( number < 0 || iPosition < 0 || iPosition >= cells
        || iPosition + number > cells){
        /* Illegal. */
        XP_ASSERT(FALSE);
        return;
    }
    for ( intn cell = iPosition + number - 1; cell >= iPosition; cell-- ) {
        CEditTableCellElement* pExisting = FindCell(cell);
        if ( ! pExisting ) {
            XP_ASSERT(FALSE);
            return;
        }
        if ( pUndoContainer ) {
            pExisting->Unlink();
            pExisting->InsertAsFirstChild(pUndoContainer);
        }
        else {
            delete pExisting;
        }
    }
}

CEditTableCellElement* CEditTableRowElement::FindCell(intn number){
    intn count = 0;
    if ( number < 0 ) {
        XP_ASSERT(FALSE);
        return NULL;
    }
    for ( CEditElement* pChild = GetChild();
        pChild;
        pChild = pChild->GetNextSibling()) {
        if ( pChild->IsTableCell() ) {
            if ( count++ == number ) {
                return (CEditTableCellElement*) pChild;
            }
        }
    }
    return NULL;
}

void CEditTableRowElement::SetData( EDT_TableRowData *pData ){
    char *pNew = 0;
    if( pData->align != ED_ALIGN_DEFAULT ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s ", EDT_AlignString(pData->align) );
    }

    if( pData->valign != ED_ALIGN_DEFAULT ){
        pNew = PR_sprintf_append( pNew, "VALIGN=%s ", EDT_AlignString(pData->valign) );
    }

    if ( pData->pColorBackground ) {
        SetBackgroundColor(EDT_LO_COLOR(pData->pColorBackground));
        pNew = PR_sprintf_append( pNew, "BGCOLOR=\"#%06lX\" ", GetBackgroundColor().GetAsLong() );
    }
    else {
        SetBackgroundColor(ED_Color::GetUndefined());
    }

    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }

    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }
    SetTagData( pNew );
    if ( pNew ) {
        free(pNew);
    }
}

EDT_TableRowData* CEditTableRowElement::GetData( ){
    EDT_TableRowData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char *tableRowParams[] = {
    PARAM_ALIGN,
    PARAM_VALIGN,
    PARAM_BGCOLOR,
    0
};

EDT_TableRowData* CEditTableRowElement::ParseParams( PA_Tag *pTag ){
    EDT_TableRowData* pData = NewData();
    ED_Alignment align;
    
    align = edt_FetchParamAlignment( pTag, ED_ALIGN_DEFAULT );
    if( align == ED_ALIGN_RIGHT || align == ED_ALIGN_LEFT || align == ED_ALIGN_ABSCENTER ){
        pData->align = align;
    }
    align = edt_FetchParamAlignment( pTag, ED_ALIGN_DEFAULT, TRUE );
    if( align == ED_ALIGN_ABSTOP || align == ED_ALIGN_ABSBOTTOM || align == ED_ALIGN_ABSCENTER
        || align == ED_ALIGN_BASELINE ){
        pData->valign = align;
    }
    pData->pColorBackground = edt_MakeLoColor(edt_FetchParamColor( pTag, PARAM_BGCOLOR ));
    pData->pExtra = edt_FetchParamExtras( pTag, tableRowParams );

    return pData;
}

EDT_TableRowData* CEditTableRowElement::NewData(){
    EDT_TableRowData* pData = XP_NEW( EDT_TableRowData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->align = ED_ALIGN_DEFAULT;
    pData->valign = ED_ALIGN_DEFAULT;
    pData->pColorBackground = 0;
    pData->pExtra = 0;
    return pData;
}

void CEditTableRowElement::FreeData( EDT_TableRowData *pData ){
    if( pData->pColorBackground ) XP_FREE( pData->pColorBackground );
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

void CEditTableRowElement::SetBackgroundColor( ED_Color iColor ){
    m_backgroundColor = iColor;
}

ED_Color CEditTableRowElement::GetBackgroundColor(){
    return m_backgroundColor;
}

// class CEditCaptionElement

CEditCaptionElement::CEditCaptionElement()
    : CEditSubDocElement((CEditElement*) NULL, P_CAPTION)
{
}

CEditCaptionElement::CEditCaptionElement(CEditElement *pParent, PA_Tag *pTag)
    : CEditSubDocElement(pParent, P_CAPTION)
{
    if( pTag ){
        char *locked_buff;
            
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditCaptionElement::CEditCaptionElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditSubDocElement(pStreamIn, pBuffer)
{
}

void CEditCaptionElement::SetData( EDT_TableCaptionData *pData ){
    char *pNew = 0;
    if( pData->align == ED_ALIGN_ABSBOTTOM ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s ", EDT_AlignString(pData->align) );
    }
    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }

    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }
    SetTagData( pNew );
    if ( pNew ) {
        free(pNew);
    }
}

EDT_TableCaptionData* CEditCaptionElement::GetData( ){
    EDT_TableCaptionData* pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char *tableCaptionParams[] = {
    PARAM_ALIGN,
    0
};

EDT_TableCaptionData* CEditCaptionElement::ParseParams( PA_Tag *pTag ){
    EDT_TableCaptionData* pData = NewData();
    pData->align = edt_FetchParamAlignment( pTag, ED_ALIGN_ABSTOP );
    pData->pExtra = edt_FetchParamExtras( pTag, tableCaptionParams );
    return pData;
}

EDT_TableCaptionData* CEditCaptionElement::NewData(){
    EDT_TableCaptionData* pData = XP_NEW( EDT_TableCaptionData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->align = ED_ALIGN_CENTER;
    pData->pExtra = 0;
    return pData;
}

void CEditCaptionElement::FreeData( EDT_TableCaptionData *pData ){
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

// class CEditTableCellElement

CEditTableCellElement::CEditTableCellElement()
    : CEditSubDocElement((CEditElement*) NULL, P_TABLE_DATA),
      m_backgroundColor()
{
}

CEditTableCellElement::CEditTableCellElement(XP_Bool bIsHeader)
    : CEditSubDocElement((CEditElement*) NULL, bIsHeader ? P_TABLE_HEADER : P_TABLE_DATA),
      m_backgroundColor()
{
}

CEditTableCellElement::CEditTableCellElement(CEditElement *pParent, PA_Tag *pTag)
    : CEditSubDocElement(pParent, pTag->type),
      m_backgroundColor()
{
    if( pTag ){
        char *locked_buff;
            
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditTableCellElement::CEditTableCellElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditSubDocElement(pStreamIn, pBuffer),
      m_backgroundColor()
{
}

XP_Bool CEditTableCellElement::IsTableCell(){
    return TRUE;
}

EEditElementType CEditTableCellElement::GetElementType(){
    return eTableCellElement;
}

ED_Alignment CEditTableCellElement::GetDefaultAlignment(){
    return ED_ALIGN_DEFAULT;
/*
    EDT_TableCellData* pData = GetData();
    ED_Alignment result = IsTableData() ? ED_ALIGN_LEFT : ED_ALIGN_ABSCENTER;
    if ( pData->align != ED_ALIGN_DEFAULT ) {
        result = pData->align;
    }
    FreeData(pData);
    return result;
*/
}

XP_Bool CEditTableCellElement::IsTableData(){
    return GetType() == P_TABLE_DATA;
}

intn CEditTableCellElement::GetRowIndex(){
    CEditTableRowElement* pTableRow = GetTableRow();
    if ( pTableRow ) {
        return pTableRow->GetRowIndex();
    }
    else {
        XP_ASSERT(FALSE);
    }
    return 0;
}

intn CEditTableCellElement::GetColumnIndex(){
    CEditTableRowElement* pTableRow = GetTableRow();
    intn cellIndex = 0;
    if ( pTableRow ) {
        for ( CEditElement* pChild = pTableRow->GetChild();
            pChild;
            pChild = pChild->GetNextSibling()) {
            if ( pChild == this ){
                return cellIndex;
            }
            if ( pChild->IsTableCell() ){
                cellIndex++;
            }
        }
        XP_ASSERT(FALSE);
    }
    else {
        XP_ASSERT(FALSE);
    }
    return cellIndex;
}


void CEditTableCellElement::SetData( EDT_TableCellData *pData ){
    // bHeader is actually stored as the tag data
    if ( pData->bHeader ) {
        SetType(P_TABLE_HEADER);
    }
    else {
        SetType(P_TABLE_DATA);
    }
    char *pNew = 0;
    if( pData->align != ED_ALIGN_DEFAULT ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s ", EDT_AlignString(pData->align) );
    }
    if( pData->valign != ED_ALIGN_DEFAULT ){
        pNew = PR_sprintf_append( pNew, "VALIGN=%s ", EDT_AlignString(pData->valign) );
    }
    if ( pData->iColSpan != 1 ){
        pNew = PR_sprintf_append( pNew, "COLSPAN=\"%d\" ", pData->iColSpan );
    }
    if ( pData->iRowSpan != 1 ){
        pNew = PR_sprintf_append( pNew, "ROWSPAN=\"%d\" ", pData->iRowSpan );
    }
    if ( pData->bNoWrap ){
        pNew = PR_sprintf_append( pNew, "NOWRAP " );
    }
    if( pData->bWidthDefined ){
        if( pData->bWidthPercent ){
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld%%\" ", 
                                      (long)min(100,max(1,pData->iWidth)) );
        }
        else {
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld\" ", (long)pData->iWidth );
        }
    }
    if( pData->bHeightDefined ){
        if( pData->bHeightPercent ){
            pNew = PR_sprintf_append( pNew, "HEIGHT=\"%ld%%\" ", 
                                      (long)min(100,max(1, pData->iHeight)) );
        }
        else {
            pNew = PR_sprintf_append( pNew, "HEIGHT=\"%ld\" ", (long)pData->iHeight );
        }
    }

    if ( pData->pColorBackground ) {
        SetBackgroundColor(EDT_LO_COLOR(pData->pColorBackground));
        pNew = PR_sprintf_append( pNew, "BGCOLOR=\"#%06lX\" ", GetBackgroundColor().GetAsLong() );
    }
    else {
        SetBackgroundColor(ED_Color::GetUndefined());
    }

    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }

    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }
    SetTagData( pNew );
    if ( pNew ) {
        free(pNew);
    }
}

EDT_TableCellData* CEditTableCellElement::GetData( ){
    EDT_TableCellData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char *tableCellParams[] = {
    PARAM_ALIGN,
    PARAM_VALIGN,
    PARAM_COLSPAN,
    PARAM_ROWSPAN,
    PARAM_NOWRAP,
    PARAM_WIDTH,
    PARAM_HEIGHT,
    PARAM_BGCOLOR,
    0
};

EDT_TableCellData* CEditTableCellElement::ParseParams( PA_Tag *pTag ){
    EDT_TableCellData *pData = NewData();
    
    ED_Alignment align;
    
    align = edt_FetchParamAlignment( pTag, ED_ALIGN_DEFAULT );
    if( align == ED_ALIGN_RIGHT || align == ED_ALIGN_LEFT || align == ED_ALIGN_ABSCENTER ){
        pData->align = align;
    }

    align = edt_FetchParamAlignment( pTag, ED_ALIGN_DEFAULT, TRUE );
    if( align == ED_ALIGN_ABSTOP || align == ED_ALIGN_ABSBOTTOM || align == ED_ALIGN_ABSCENTER
        || align == ED_ALIGN_BASELINE ){
        pData->valign = align;
    }

    pData->iColSpan = edt_FetchParamInt(pTag, PARAM_COLSPAN, 1);
    pData->iRowSpan = edt_FetchParamInt(pTag, PARAM_ROWSPAN, 1);
    pData->bHeader = GetType() == P_TABLE_HEADER;
    pData->bNoWrap = edt_FetchParamBoolExist(pTag, PARAM_NOWRAP );
    pData->bWidthDefined = edt_FetchDimension( pTag, PARAM_WIDTH, 
                    &pData->iWidth, &pData->bWidthPercent,
                    100, TRUE );
    pData->bHeightDefined = edt_FetchDimension( pTag, PARAM_HEIGHT, 
                    &pData->iHeight, &pData->bHeightPercent,
                    100, TRUE );

    pData->pColorBackground = edt_MakeLoColor(edt_FetchParamColor( pTag, PARAM_BGCOLOR ));
    pData->pExtra = edt_FetchParamExtras( pTag, tableCellParams );

    return pData;
}

EDT_TableCellData* CEditTableCellElement::NewData(){
    EDT_TableCellData *pData = XP_NEW( EDT_TableCellData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->align = ED_ALIGN_DEFAULT;
    pData->valign = ED_ALIGN_DEFAULT;
    pData->iColSpan = 1;
    pData->iRowSpan = 1;
    pData->bHeader = FALSE;
    pData->bNoWrap = FALSE;
    pData->bWidthDefined = FALSE;
    pData->bWidthPercent = TRUE;
    pData->iWidth = 100;
    pData->bHeightDefined = FALSE;
    pData->bHeightPercent = TRUE;
    pData->iHeight = 100;
    pData->pColorBackground = 0;
    pData->pExtra = 0;
    return pData;
}

void CEditTableCellElement::FreeData( EDT_TableCellData *pData ){
    if( pData->pColorBackground ) XP_FREE( pData->pColorBackground );
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

void CEditTableCellElement::SetBackgroundColor( ED_Color iColor ){
    m_backgroundColor = iColor;
}

ED_Color CEditTableCellElement::GetBackgroundColor(){
    return m_backgroundColor;
}

// class CEditMultiColumnElement

CEditMultiColumnElement::CEditMultiColumnElement(intn columns)
    : CEditElement(0, P_MULTICOLUMN, 0)
{
    EDT_MultiColumnData* pData = NewData();
    pData->iColumns = columns;
    SetData(pData);
    FreeData(pData);
}

CEditMultiColumnElement::CEditMultiColumnElement(CEditElement *pParent, PA_Tag *pTag)
    : CEditElement(pParent, P_MULTICOLUMN)
{
    if( pTag ){
        char *locked_buff;
            
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditMultiColumnElement::CEditMultiColumnElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
    : CEditElement(pStreamIn, pBuffer)
{
}

void CEditMultiColumnElement::FinishedLoad( CEditBuffer* pBuffer ){
    EnsureSelectableSiblings(pBuffer);
    CEditElement::FinishedLoad(pBuffer);
}

void CEditMultiColumnElement::SetData( EDT_MultiColumnData *pData ){
    char *pNew = 0;
    if ( pData->iColumns > 1){
        pNew = PR_sprintf_append( pNew, "COLS=%d ", pData->iColumns );
    }
    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }
    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }
    SetTagData( pNew );
    if ( pNew ) {
        free(pNew);
    }
}

EDT_MultiColumnData* CEditMultiColumnElement::GetData( ){
    EDT_MultiColumnData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char *multiColumnParams[] = {
    PARAM_COLS,
    0
};

EDT_MultiColumnData* CEditMultiColumnElement::ParseParams( PA_Tag *pTag ){
    EDT_MultiColumnData *pData = NewData();
    
    pData->iColumns = edt_FetchParamInt(pTag, PARAM_COLS, 1);
    pData->pExtra = edt_FetchParamExtras( pTag, multiColumnParams );

    return pData;
}

EDT_MultiColumnData* CEditMultiColumnElement::NewData(){
    EDT_MultiColumnData *pData = XP_NEW( EDT_MultiColumnData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->iColumns = 1;
    pData->pExtra = 0;
    return pData;
}

void CEditMultiColumnElement::FreeData( EDT_MultiColumnData *pData ){
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

//-----------------------------------------------------------------------------
// CEditRootDocElement
//-----------------------------------------------------------------------------
void CEditRootDocElement::PrintOpen( CPrintState *pPrintState ){
    m_pBuffer->PrintDocumentHead( pPrintState );
}

void CEditRootDocElement::PrintEnd( CPrintState *pPrintState ){
    m_pBuffer->PrintDocumentEnd( pPrintState );
}

void CEditRootDocElement::FinishedLoad( CEditBuffer *pBuffer ){
    CEditSubDocElement::FinishedLoad(pBuffer);
    if ( ! GetLastChild() || ! GetLastChild()->IsEndContainer() ) {
        CEditEndContainerElement* pEndContainer = new CEditEndContainerElement(NULL);
        CEditEndElement* pEnd = new CEditEndElement(pEndContainer);
        pEndContainer->InsertAsLastChild(this);
    }
}

#ifdef DEBUG
void CEditRootDocElement::ValidateTree(){
    CEditElement::ValidateTree();
    XP_ASSERT(GetParent() == NULL);
    XP_ASSERT(GetChild() != NULL);
    XP_ASSERT(GetLastMostChild()->GetElementType() == eEndElement);
}

#endif


//-----------------------------------------------------------------------------
// CEditLeafElement
//-----------------------------------------------------------------------------


XP_Bool CEditLeafElement::PrevPosition(ElementOffset iOffset, 
                CEditLeafElement*& pNew, ElementOffset& iNewOffset ){

    XP_Bool bPreviousElement = FALSE;
    XP_Bool bMoved = TRUE;

    iNewOffset = iOffset - 1;
    pNew = this;

    if( iNewOffset == 0 ){

        CEditElement *pPrev = PreviousLeafInContainer();
        if( pPrev ){
            if( pPrev->IsLeaf()){
                bPreviousElement = TRUE;
            }
        }
    }

    //
    // we need to set the position to the end of the previous element.
    //
    if( iNewOffset < 0 || bPreviousElement ){
        CEditLeafElement *pPrev = PreviousLeaf();
        if( pPrev ){
            if( pPrev->IsLeaf()){
                pNew = pPrev;
                iNewOffset = pNew->Leaf()->GetLen();
            }
        }
        else{
            // no previous element, we are at the beginning of the buffer.
            iNewOffset = iOffset;
            bMoved = FALSE;
        }
    }
    return bMoved;
}

XP_Bool CEditLeafElement::NextPosition(ElementOffset iOffset, 
                CEditLeafElement*& pNew, ElementOffset& iNewOffset ){
    LO_Element* pRetText;
    CEditLeafElement *pNext;
    int iRetPos;
    XP_Bool retVal = TRUE;

    pNew = this;
    iNewOffset = iOffset + 1;

    //
    // if the new offset is obviously greater, move past it.  If
    //  Make sure the layout engine actually layed out the element.
    //
    if( iNewOffset > Leaf()->GetLen() ||
        !Leaf()->GetLOElementAndOffset( iOffset, FALSE, pRetText, iRetPos ))
    {
        pNext = NextLeaf();
        if( pNext ){
            pNew = pNext;
            if( pNext->PreviousLeafInContainer() == 0 ){
                iNewOffset = 0;
            }
            else {
                iNewOffset = 1;
            }
        }
        else {
            iNewOffset = iOffset;
            retVal = FALSE;
        }
    }
    return retVal;
}

void CEditLeafElement::DisconnectLayoutElements(LO_Element* pElement){
    // Clear back pointer out of the edit elements.
    LO_Element* e = pElement;
    while ( e && e->lo_any.edit_element == this ){
        e->lo_any.edit_element = 0;
        while ( e && e->lo_any.edit_element == 0 ){
            e = e->lo_any.next;
        }
    }
}

void CEditLeafElement::SetLayoutElementHelper( intn desiredType, LO_Element** pElementHolder,
                                              intn iEditOffset, intn lo_type, 
                                              LO_Element* pLoElement){
    // XP_TRACE(("SetLayoutElementHelper this(0x%08x) iEditOffset(%d) pLoElement(0x%08x)", this, iEditOffset, pLoElement));

    if( lo_type == desiredType ){
        // iEditOffset is:
        // 0 for normal case,
        // -1 for end of document element.
        // > 0 for text items after a line wrap.
        //     Don't disconnect layout elements because this is
        //     called for each layout element of a wrapped text
        //     edit element. Since DisconnectLayoutElements goes
        //     ahead and zeros out all the "next" elements with
        //     the same back pointer, we end up trashing later
        //     elements if the elements are set in some order
        //     other than first-to-last. This happens when just the 2nd line of a two-line
        //     text element is being relaid out.
        if ( iEditOffset <= 0 ) {
            // Don't reset the old element, because functions like lo_BreakOldElement depend upon the
            // old element having a good value.
            *pElementHolder = pLoElement;
        }
        pLoElement->lo_any.edit_element = this;
        pLoElement->lo_any.edit_offset = iEditOffset;
    }
    else {
        // We get called back whenever linefeeds are generated. That's so that
        // breaks can find their linefeed. But it also means we are
        // called back more often than nescessary.
    //    XP_TRACE(("  ignoring inconsistent type.\n"));
    }
}

void CEditLeafElement::ResetLayoutElementHelper( LO_Element** pElementHolder, intn iEditOffset, 
            LO_Element* pLoElement ){
    // XP_TRACE(("ResetLayoutElementHelper this(0x%08x) iEditOffset(%d) pLoElement(0x%08x)", this, iEditOffset, pLoElement));
    if ( iEditOffset <= 0 ) {
        if ( *pElementHolder == pLoElement ){
            *pElementHolder = NULL;
        }
        else {
            XP_TRACE(("  Woops -- we're currently pointing to 0x%08x\n", *pElementHolder));
        }
    }
}

CEditElement* CEditLeafElement::Divide(int iOffset){
    CEditElement* pNext;
    if( iOffset == 0 ){
        return this;
    }

    pNext = LeafInContainerAfter();
    if( iOffset >= GetLen() ){
        if( pNext != 0 ){
            return pNext;
        }
        else {
            CEditTextElement* pPrev;
            CEditTextElement* pNew;
            if( IsA( P_TEXT )) {
                pPrev = this->Text();
            }
            else {
                pPrev = PreviousTextInContainer();
            }

            if( pPrev ){
                pNew = pPrev->CopyEmptyText();
            }
            else {
                pNew = new CEditTextElement((CEditElement*)0,0);
            }
            pNew->InsertAfter(this);
            return pNew;
        }
    }

    // offset is in the middle somewhere Must be a TEXT.
    XP_ASSERT( IsA(P_TEXT) );
    CEditTextElement* pNew = Text()->CopyEmptyText();
    pNew->SetText( Text()->GetText()+iOffset );
    Text()->GetText()[iOffset] = 0;
    pNew->InsertAfter(this);
    return pNew;
}

CEditTextElement* CEditLeafElement::CopyEmptyText(CEditElement *pParent){
       return new CEditTextElement(pParent,0);
}

CPersistentEditInsertPoint CEditLeafElement::GetPersistentInsertPoint(ElementOffset offset){
    ElementIndex len = GetPersistentCount();
    if ( offset > len )
    {
#ifdef DEBUG_EDIT_LAYOUT
        XP_ASSERT(FALSE);
#endif
        offset = len;
    }
    return CPersistentEditInsertPoint(GetElementIndex() + offset, offset == 0);
}

//
// Default implementation
//
PA_Tag* CEditLeafElement::TagOpen( int /* iEditOffset */ ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    if( GetTagData() ){
        SetTagData( pTag, GetTagData() );
    }
    else {
        SetTagData( pTag, ">" );
    }
    return pTag;
}

ElementIndex CEditLeafElement::GetPersistentCount()
{
    return GetLen();
}

void CEditLeafElement::GetAll(CEditSelection& selection) {
    selection.m_start.m_pElement = this;
    selection.m_start.m_iPos = 0;
    selection.m_end.m_pElement = this;
    selection.m_end.m_iPos = Leaf()->GetLen();
    selection.m_bFromStart = FALSE;
}


#ifdef DEBUG
void CEditLeafElement::ValidateTree(){
    CEditElement::ValidateTree();
    CEditElement *pChild = GetChild();
    XP_ASSERT(!pChild); // Must have no children.
    // Parent must exist and be a container
    CEditElement *pParent = GetParent();
    XP_ASSERT(pParent); 
    XP_ASSERT(pParent->IsContainer()); 
}
#endif


//-----------------------------------------------------------------------------
// CEditContainerElement
//-----------------------------------------------------------------------------
//
// Static constructor to create default element
//
CEditContainerElement* CEditContainerElement::NewDefaultContainer( CEditElement *pParent,
        ED_Alignment align  ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    pTag->type = P_PARAGRAPH;
    CEditContainerElement *pRet = new CEditContainerElement( pParent, pTag, align );
    PA_FreeTag( pTag );        
    return pRet;
}

CEditContainerElement::CEditContainerElement(CEditElement *pParent, PA_Tag *pTag,
                ED_Alignment defaultAlign ): 
            CEditElement(pParent, pTag ? pTag->type : P_PARAGRAPH), 
            m_defaultAlign( defaultAlign ),
            m_align( defaultAlign )
{ 

    if( pTag ){
        EDT_ContainerData *pData = ParseParams( pTag );
        SetData( pData );
        FreeData( pData );
    }
}

CEditContainerElement::CEditContainerElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer): 
        CEditElement( pStreamIn, pBuffer )
{
    m_defaultAlign = (ED_Alignment) pStreamIn->ReadInt();
    m_align = (ED_Alignment) pStreamIn->ReadInt();
}

void CEditContainerElement::StreamOut( IStreamOut *pOut ){
    CEditElement::StreamOut( pOut );
    pOut->WriteInt( m_defaultAlign );
    pOut->WriteInt( m_align );
}

void CEditContainerElement::StreamInChildren(IStreamIn* pIn, CEditBuffer* pBuffer){
    CEditElement::StreamInChildren(pIn, pBuffer);
    if ( GetChild() == NULL ){
        // I need a dummy child.
        CEditTextElement* pChild = new CEditTextElement(this, 0);
        // Creating it automaticly inserts it
    }
}

ElementIndex CEditContainerElement::GetPersistentCount(){
    // One extra element: the end of the last item in the container.
    return CEditElement::GetPersistentCount() + 1;
}

void CEditContainerElement::FinishedLoad( CEditBuffer *pBuffer ){
    {
        if ( GetType() != P_PREFORMAT ) {
            // Don't allow more than one space before non-text elements
            CEditElement* pNext;
            for ( CEditElement* pChild = GetChild();
                pChild;
                pChild = pNext ) {
                pNext = pChild->GetNextSibling();
                if ( pChild->IsText() && ! (pNext && pNext->IsText()) ){
                    CEditTextElement* pTextChild = pChild->Text();
                    char* pText = pTextChild->GetText();
                    int size = pTextChild->GetLen();
                    XP_Bool trimming = FALSE;
                    while ( size > 1 && pText[size-1] == ' ' && pText[size-2] == ' ') {
                        // More than one character of white space at the end of
                        // a text element will get laid out as one character.
                        trimming = TRUE;
                        size--;
                    }
                    if ( trimming ) {
                        if (size <= 0 ) {
                            delete pChild;
                        }
                        else {
                            char* pCopy = XP_STRDUP(pText);
                            pCopy[size] = '\0';
                            pTextChild->SetText(pCopy);
                            XP_FREE(pCopy);
                        }
                    }
                }
            }
        }
    }
    if ( GetChild() == NULL ){
        // I need a dummy child.
        CEditTextElement* pChild = new CEditTextElement(this, 0);
        // Creating it automaticly inserts it
		pChild->FinishedLoad(pBuffer);
    }
    CEditElement::FinishedLoad(pBuffer);
}

PA_Tag* CEditContainerElement::TagOpen( int iEditOffset ){
    PA_Tag *pRet = 0;
    PA_Tag* pTag;

    // create the DIV tag if we need to.
    if( !SupportsAlign() && (m_align == ED_ALIGN_ABSCENTER || m_align == ED_ALIGN_RIGHT ) ){
        pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        if( m_align== ED_ALIGN_RIGHT ){
            SetTagData( pTag, "ALIGN=right>");
            pTag->type = P_DIVISION;
        }
        else {
            SetTagData( pTag, ">");
            pTag->type = P_CENTER;
        }
        pRet = pTag;
    }

    // create the actual paragraph tag
    if( GetTagData() ){
        pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        SetTagData( pTag, GetTagData() );
    }
    else {
        pTag = CEditElement::TagOpen( iEditOffset );
    }

    // link the tags together.
    if( pRet == 0 ){
        pRet = pTag;
    }
    else {
        pRet->next = pTag;
    }
    return pRet;
}

PA_Tag* CEditContainerElement::TagEnd( ){
    PA_Tag *pRet = CEditElement::TagEnd();
    if( !SupportsAlign() && m_align == ED_ALIGN_ABSCENTER || m_align == ED_ALIGN_RIGHT ){
        PA_Tag* pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        pTag->is_end = TRUE;
        if( m_align == ED_ALIGN_RIGHT ){
            pTag->type = P_DIVISION;
        }
        else {
            pTag->type = P_CENTER;
        }

        if( pRet == 0 ){
            pRet = pTag;
        }
        else {
            pRet->next = pTag;
        }
    }
    return pRet;
}

EEditElementType CEditContainerElement::GetElementType()
{
    return eContainerElement;
}

XP_Bool CEditContainerElement::IsPlainFirstContainer(){
    if ( GetType() == P_PARAGRAPH
        && IsFirstContainer() ) {
        ED_Alignment alignment = GetAlignment();
        ED_Alignment defaultAlignment = GetDefaultAlignment();
        if ( alignment == ED_ALIGN_DEFAULT
            || alignment == defaultAlignment
            || ( alignment == ED_ALIGN_LEFT
                && defaultAlignment == ED_ALIGN_DEFAULT )) {
            return TRUE;
        }
    }
    return FALSE;
}

XP_Bool CEditContainerElement::IsFirstContainer(){
    CEditSubDocElement* pSubDoc = GetSubDoc();
    return ! pSubDoc || pSubDoc->GetChild() == this;
}

XP_Bool CEditContainerElement::SupportsAlign(){
    return BitSet( edt_setContainerSupportsAlign, GetType() );
}

void CEditContainerElement::PrintOpen( CPrintState *pPrintState ){
    if ( ShouldSkipTags() ) return;

    pPrintState->m_iCharPos = 0;
    pPrintState->m_pOut->Printf( "\n"); 

    PA_Tag *pTag = TagOpen(0);
    while( pTag ){
        char *pData = 0;
        if( pTag->data ){
            PA_LOCK( pData, char*, pTag->data );
        }

        if( pData && *pData != '>' ) {
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s %s", 
                    EDT_TagString(pTag->type), pData );
        }
        else {
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s>", 
                    EDT_TagString(pTag->type));
        }

        if( pTag->data ){
            PA_UNLOCK( pTag->data );
        }

        PA_Tag* pDelTag = pTag;
        pTag = pTag->next;
        PA_FreeTag(pDelTag);
    }
}

XP_Bool CEditContainerElement::ShouldSkipTags(){
    XP_Bool parentIsTableCell = GetParent() && GetParent()->IsTableCell();
    return parentIsTableCell && IsPlainFirstContainer() || IsEmpty();
}

void CEditContainerElement::PrintEnd( CPrintState *pPrintState ){
    if ( ShouldSkipTags() ) return;

    PA_Tag *pTag = TagEnd();
    while( pTag ){
        char *pData = 0;
        if( pTag->data ){
            PA_LOCK( pData, char*, pTag->data );
        }

        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "</%s>",
            EDT_TagString( pTag->type ) );

        if( pTag->data ){
            PA_UNLOCK( pTag->data );
        }

        PA_Tag* pDelTag = pTag;
        pTag = pTag->next;
        PA_FreeTag(pDelTag);
    }

    pPrintState->m_iCharPos = 0;
    pPrintState->m_pOut->Printf( "\n"); 
}



CEditElement* CEditContainerElement::Clone( CEditElement* pParent ){
    PA_Tag *pTag = TagOpen(0);

    // LTNOTE: we get division tags at the beginning of paragraphs
    //  for their alignment. Ignore these tags, we get it directly from the
    //  container itself.
    //  Its a total hack.
    //
    if( pTag->type == P_DIVISION || pTag->type == P_CENTER ){
        PA_Tag *pDelTag = pTag;
        pTag = pTag->next;
        PA_FreeTag( pDelTag );
    }
    CEditElement *pRet = new CEditContainerElement(pParent, pTag, m_align );
    PA_FreeTag( pTag );
    return pRet;
}

// Property Getting and Setting Stuff.

EDT_ContainerData* CEditContainerElement::NewData(){
    EDT_ContainerData *pData = XP_NEW( EDT_ContainerData );
    pData->align = ED_ALIGN_LEFT;
    return pData;
}

void CEditContainerElement::FreeData( EDT_ContainerData *pData ){
    XP_FREE( pData );
}

void CEditContainerElement::SetData( EDT_ContainerData *pData ){
    char *pNew = 0;

    m_align = pData->align;
    if( m_align == ED_ALIGN_CENTER ) m_align = ED_ALIGN_ABSCENTER;

    if( SupportsAlign() 
            && ( m_align == ED_ALIGN_ABSCENTER || m_align == ED_ALIGN_RIGHT ) ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s>", EDT_AlignString(m_align) );
    }
    else {
        pNew = PR_sprintf_append( pNew, ">" );
    }
    SetTagData( pNew );
    free(pNew);
}

EDT_ContainerData* CEditContainerElement::GetData( ){
    EDT_ContainerData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

EDT_ContainerData* CEditContainerElement::ParseParams( PA_Tag *pTag ){
    EDT_ContainerData *pData = NewData();
    ED_Alignment align;
    
    align = edt_FetchParamAlignment( pTag, m_defaultAlign );
    if( align == ED_ALIGN_CENTER ) align =  ED_ALIGN_ABSCENTER;

    if( align == ED_ALIGN_RIGHT || align == ED_ALIGN_LEFT  || align == ED_ALIGN_ABSCENTER){
        pData->align = align;
    }
    return pData;
}

void CEditContainerElement::SetAlignment( ED_Alignment eAlign ){
    EDT_ContainerData *pData = GetData();
    pData->align = eAlign;
    SetData( pData );
    FreeData( pData );
}

void CEditContainerElement::CopyAttributes(CEditContainerElement *pFrom ){
    SetType( pFrom->GetType() );
    SetAlignment( pFrom->GetAlignment() );
}

XP_Bool CEditContainerElement::IsEmpty(){
    CEditElement *pChild = GetChild();
    return ( pChild == 0 
                || ( pChild->IsA( P_TEXT )
                    && pChild->Text()->GetLen() == 0
                    && pChild->LeafInContainerAfter() == 0 )
            );
}

#ifdef DEBUG
void CEditContainerElement::ValidateTree(){
    CEditElement::ValidateTree();
    CEditElement* pChild = GetChild();
    XP_ASSERT(pChild); // Must have at least one child.
}
#endif

// 
// Sets the alignment if the container is empty (or almost empty).  Used only
//  during the parse phase to handle things like <p><center><img>
//
void CEditContainerElement::AlignIfEmpty( ED_Alignment eAlign ){
    CEditElement *pChild = GetChild();
    while( pChild ){
        if( pChild->IsA( P_TEXT ) 
                && pChild->Leaf()->GetLen() != 0 ){
            return;
        }
        pChild = pChild->GetNextSibling();
    }
    SetAlignment( eAlign );
}


//-----------------------------------------------------------------------------
// CEditListElement
//-----------------------------------------------------------------------------

CEditListElement::CEditListElement( 
        CEditElement *pParent, PA_Tag *pTag ):
            CEditElement( pParent, pTag->type ){
   
    if( pTag ){
        char *locked_buff;
                
        PA_LOCK(locked_buff, char *, pTag->data );
        SetTagData( locked_buff );
        PA_UNLOCK(pTag->data);
    }
}

CEditListElement::CEditListElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer): 
        CEditElement( pStreamIn, pBuffer )
{
}

PA_Tag* CEditListElement::TagOpen( int /* iEditOffset */ ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    SetTagData( pTag, GetTagData() ? GetTagData() : ">" );
    return pTag;
}

CEditElement* CEditListElement::Clone( CEditElement *pParent ){ 
    PA_Tag* pTag = TagOpen(0);
    CEditElement* pNew = new CEditListElement( pParent, pTag );
    PA_FreeTag( pTag );
    return pNew;
}

// Property Getting and Setting Stuff.

EDT_ListData* CEditListElement::NewData(){
    EDT_ListData *pData = XP_NEW( EDT_ListData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->iTagType = P_UNUM_LIST;
    pData->bCompact = FALSE;
    pData->eType = ED_LIST_TYPE_DEFAULT;
    pData->iStart = 1;
    pData->pExtra = 0;
    return pData;
}

void CEditListElement::FreeData( EDT_ListData *pData ){
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

void CEditListElement::SetData( EDT_ListData *pData ){
    char *pNew = 0;

    SetType( pData->iTagType );
    
    if( pData->bCompact ){
        pNew = PR_sprintf_append( pNew, " COMPACT");
    }
    if( pData->iStart != 1 ){
        pNew = PR_sprintf_append( pNew, " START=%d", pData->iStart );
    }
    switch( pData->eType ){
        case ED_LIST_TYPE_DEFAULT:
            break;
        case ED_LIST_TYPE_DIGIT:
            SetType( P_NUM_LIST );
            break;
        case ED_LIST_TYPE_BIG_ROMAN:
            SetType( P_NUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=I" );
            break;
        case ED_LIST_TYPE_SMALL_ROMAN:
            SetType( P_NUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=i" );
            break;
        case ED_LIST_TYPE_BIG_LETTERS:
            SetType( P_NUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=A" );
            break;
        case ED_LIST_TYPE_SMALL_LETTERS:
            SetType( P_NUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=a" );
            break;
        case ED_LIST_TYPE_CIRCLE:
            SetType( P_UNUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=CIRCLE" );
            break;
        case ED_LIST_TYPE_SQUARE:
            SetType( P_UNUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=SQUARE" );
            break;
        case ED_LIST_TYPE_DISC:
            SetType( P_UNUM_LIST );
            pNew = PR_sprintf_append( pNew, " TYPE=DISC" );
            break;
    }

    if( pData->pExtra  ){
        pNew = PR_sprintf_append( pNew, " %s", pData->pExtra );
    }

    if( pNew ){
        pNew = PR_sprintf_append( pNew, ">" );
    }

    SetTagData( pNew );

    if ( pNew ){
        free(pNew);
    }
}

EDT_ListData* CEditListElement::GetData( ){
    EDT_ListData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char* listParams[] = {
    PARAM_TYPE,
    PARAM_COMPACT,
    PARAM_START,
    0
};

EDT_ListData* CEditListElement::ParseParams( PA_Tag *pTag ){
    EDT_ListData *pData = NewData();
    char *pType;

    pData->iTagType = pTag->type;
    pData->bCompact = edt_FetchParamBoolExist( pTag, PARAM_COMPACT );

    pType = edt_FetchParamString( pTag, PARAM_TYPE );
    if( pType == 0 ){
        if( pTag->type == P_NUM_LIST ){
            pData->eType = ED_LIST_TYPE_DIGIT;
        }
        else {
            pData->eType = ED_LIST_TYPE_DEFAULT;
        }
    }
    else {
        switch (*pType){
        case 'A':
            pData->eType = ED_LIST_TYPE_BIG_LETTERS;
            break;
        case 'a':
            pData->eType = ED_LIST_TYPE_SMALL_LETTERS;
            break;
        case 'i':
            pData->eType = ED_LIST_TYPE_SMALL_ROMAN;
            break;
        case 'I':
            pData->eType = ED_LIST_TYPE_BIG_ROMAN;
            break;
        case 'c':
        case 'C':
            pData->eType = ED_LIST_TYPE_CIRCLE;
            break;
        case 'd':
        case 'D':
            pData->eType = ED_LIST_TYPE_DISC;
            break;
        case 's':
        case 'S':
            pData->eType = ED_LIST_TYPE_SQUARE;
            break;
        default:
            pData->eType = ED_LIST_TYPE_DEFAULT;
            break;
        }
    }

    pData->iStart = edt_FetchParamInt( pTag, PARAM_START, 1 );
    pData->pExtra = edt_FetchParamExtras( pTag, listParams );
    return pData;
}

void CEditListElement::CopyData(CEditListElement* pOther){
    EDT_ListData* pData = pOther->GetData();
    SetData(pData);
    FreeData(pData);
}

#ifdef DEBUG
void CEditListElement::ValidateTree(){
    CEditElement::ValidateTree();
    // Must have at least one child.
    XP_ASSERT(GetChild());
    // We rely on the children to check that
    // they can have a list as a parent.
}
#endif

EEditElementType CEditListElement::GetElementType(){
    return eListElement;
}

//-----------------------------------------------------------------------------
// CEditTextElement
//-----------------------------------------------------------------------------

CEditTextElement::CEditTextElement(CEditElement *pParent, char *pText):
        CEditLeafElement(pParent, P_TEXT), 
        m_pFirstLayoutElement(0),
        m_tf(TF_NONE),
        m_iFontSize(DEF_FONTSIZE),
        m_href(ED_LINK_ID_NONE),
        m_color(ED_Color::GetUndefined()),
        m_pFace(NULL),
        m_pScriptExtra(NULL)

{
    if( pText && *pText ){
        m_pText = XP_STRDUP(pText);             // should be new??
        m_textSize = XP_STRLEN(m_pText)+1;
    }
    else {
        m_pText = 0;
        m_textSize = 0;
    }
}

CEditTextElement::CEditTextElement( IStreamIn *pIn, CEditBuffer* pBuffer ):
        CEditLeafElement(pIn, pBuffer), 
        m_pFirstLayoutElement(0),
        m_tf(TF_NONE),
        m_iFontSize(DEF_FONTSIZE),
        m_href(ED_LINK_ID_NONE),
        m_pText(0),
        m_color(ED_Color::GetUndefined()),
        m_pFace(NULL),
        m_pScriptExtra(NULL)
{
    m_tf = (ED_TextFormat)pIn->ReadInt( );
    
    m_iFontSize = pIn->ReadInt( );
    
    m_color = pIn->ReadInt( );
    
    if( m_tf & TF_HREF ){
        char* pHref = pIn->ReadZString();
        char* pExtra = pIn->ReadZString();
        SetHREF( pBuffer->linkManager.Add( pHref, pExtra ));
        XP_FREE( pHref );
        XP_FREE( pExtra );
    }

    if ( m_tf & TF_FONT_FACE ) {
        m_pFace = pIn->ReadZString();
    }

    if ( EDT_IS_SCRIPT(m_tf) ) {
        m_pScriptExtra = pIn->ReadZString();
    }

    m_pText = pIn->ReadZString();
    if( m_pText ){
        m_textSize = XP_STRLEN( m_pText );
    }
    else {
        m_textSize = 0;
    }
}

CEditTextElement::~CEditTextElement(){
    DisconnectLayoutElements((LO_Element*) m_pFirstLayoutElement);
    if ( m_pText) XP_FREE(m_pText); 
    if( m_href != ED_LINK_ID_NONE ) m_href->Release();
    if ( m_pFace ) {
        XP_FREE(m_pFace);
        m_pFace = NULL;
    }
    if ( m_pScriptExtra ) {
        XP_FREE(m_pScriptExtra);
        m_pScriptExtra = NULL;
    }
}

void CEditTextElement::StreamOut( IStreamOut *pOut ){
    CEditSelection all;
    GetAll(all);
    PartialStreamOut(pOut, all );
}

void CEditTextElement::PartialStreamOut( IStreamOut *pOut, CEditSelection& selection ){
    CEditSelection local;
    if ( ClipToMe(selection, local) ) {

        // Fake a stream out.
        CEditLeafElement::StreamOut( pOut  );
    
        pOut->WriteInt( m_tf );
    
        pOut->WriteInt( m_iFontSize );
    
        pOut->WriteInt( m_color.GetAsLong() );
    
        if( m_tf & TF_HREF ){
            pOut->WriteZString( m_href->hrefStr );
            pOut->WriteZString( m_href->pExtra );
        }

        if( m_tf & TF_FONT_FACE ){
            pOut->WriteZString( m_pFace );
        }

        if( EDT_IS_SCRIPT(m_tf) ){
            pOut->WriteZString( m_pScriptExtra );
        }

        pOut->WritePartialZString( m_pText, local.m_start.m_iPos, local.m_end.m_iPos);

        // No children
        pOut->WriteInt((int32)eElementNone);
    }
}


EEditElementType CEditTextElement::GetElementType()
{
    return eTextElement;
}

void CEditTextElement::SetLayoutElement( intn iEditOffset, intn lo_type, 
                        LO_Element* pLoElement ){
    SetLayoutElementHelper(LO_TEXT, (LO_Element**) &m_pFirstLayoutElement,
        iEditOffset, lo_type, pLoElement);
}

void CEditTextElement::ResetLayoutElement( intn iEditOffset, 
            LO_Element* pLoElement ){
    ResetLayoutElementHelper((LO_Element**) &m_pFirstLayoutElement,
        iEditOffset, pLoElement);
}

void CEditTextElement::SetText( char *pText ){
    if( m_pText ){
        XP_FREE( m_pText );
    }
    if( pText && *pText ){
        m_pText = XP_STRDUP(pText);             // should be new??
        m_textSize = XP_STRLEN(m_pText)+1;
    }
    else {
        m_pText = 0;
        m_textSize = 0;
    }
}


void CEditTextElement::SetColor( ED_Color iColor ){ 
    m_color = iColor; 
    // Dont set color if we are a script
    if(!iColor.IsDefined() || EDT_IS_SCRIPT(m_tf) ){
        m_tf  &= ~TF_FONT_COLOR;
    }
    else {
        m_tf |= TF_FONT_COLOR;
    }
}

void CEditTextElement::SetFontSize( int iSize ){
    m_iFontSize = iSize; 
    if( m_iFontSize > 7 ) m_iFontSize = 7;
    // if( m_iFontSize < 1 ) m_iFontSize = 1;
    
    //Override (use default for) ALL font size requests for Java Script
    if( EDT_IS_SCRIPT(m_tf) ){
        iSize = 0;
    }
    
    // Use 0 (or less) to signify changing to default
    if( iSize <= 0 ){
        m_iFontSize = GetDefaultFontSize();
    }
    if( m_iFontSize == GetDefaultFontSize() ){
        m_tf  &= ~TF_FONT_SIZE;
    }
    else {
        m_tf |= TF_FONT_SIZE;
    }
}

void CEditTextElement::SetFontFace( char* pFace ){
    if ( m_pFace ) {
        XP_FREE(m_pFace);
        m_pFace = NULL;
        m_tf &= ~TF_FONT_FACE;
    }
    if ( pFace ) {
        m_tf |= TF_FONT_FACE;
        m_pFace = XP_STRDUP(pFace);
    }
}

void CEditTextElement::SetScriptExtra( char* pScriptExtra ){
    if ( m_pScriptExtra ) {
        XP_FREE(m_pScriptExtra);
        m_pScriptExtra = NULL;
    }
    if ( pScriptExtra ) {
        m_pScriptExtra = XP_STRDUP(pScriptExtra);
    }
}

void CEditTextElement::SetHREF( ED_LinkId id ){
    if( m_href != ED_LINK_ID_NONE ){
        m_href->Release();
    }
    m_href = id;
    // Dont set HREF if we are a script
    if( id != ED_LINK_ID_NONE && !EDT_IS_SCRIPT(m_tf) ){
        id->AddRef();
        m_tf |= TF_HREF;
    }
    else {
        m_tf &= ~TF_HREF;
    }
}

int CEditTextElement::GetFontSize(){ 
    if( m_tf & TF_FONT_SIZE ){
        return m_iFontSize;
    }
    else {
        return GetDefaultFontSize();
    }
}

char* CEditTextElement::GetFontFace(){ 
    return m_pFace;
}

char* CEditTextElement::GetScriptExtra(){ 
    return m_pScriptExtra;
}

void CEditTextElement::ClearFormatting(){
    if ( m_tf & TF_HREF) {
        if ( m_href != ED_LINK_ID_NONE ) {
            m_href->Release();
        }
        else {
            XP_ASSERT(FALSE);
        }
    }
    m_href = ED_LINK_ID_NONE;
    m_tf = 0;
    SetFontSize( GetDefaultFontSize() );
    m_color = ED_Color::GetUndefined();
    
    /* We preserve font face since there's no user API for changing it.
     * Once we can set the font face, clear instead of preserving.
     */
    if ( m_pFace ) {
        m_tf = TF_FONT_FACE;
    }

    /* We don't preserve the script extra because the SCRIPT attribute is cleared.
     */
    SetScriptExtra(NULL);
}

void CEditTextElement::SetData( EDT_CharacterData *pData ){

    // Either Java Script type
    ED_TextFormat tfJava = TF_SERVER | TF_SCRIPT;

    // if we are setting server or script,
    //   then it is the only thing we need to do
    if( (pData->mask & pData->values ) & tfJava ){
        ClearFormatting();
        m_tf = (pData->mask & pData->values ) & tfJava;
        return;
    }
    // If we are already a Java Script type,
    //   and we are NOT removing that type (mask bit is not set)
    //   then ignore all other attributes
    if( (0 != (m_tf & TF_SERVER) && 
         0 == (pData->mask & TF_SERVER)) ||
        (0 != (m_tf & TF_SCRIPT) && 
         0 == (pData->mask & TF_SCRIPT)) ){
        return;
    }

    if( pData->mask & TF_HREF ){
        SetHREF( pData->linkId );
    }
    if( pData->mask & TF_FONT_COLOR ){
        if( (pData->values & TF_FONT_COLOR) ){
            SetColor( EDT_LO_COLOR(pData->pColor) );
        }
        else {
            SetColor( ED_Color::GetUndefined() );
        }
    }
    if( pData->mask & TF_FONT_SIZE ){
        if( pData->values & TF_FONT_SIZE ){
            SetFontSize( pData->iSize );
        }
        else {
            SetFontSize( GetDefaultFontSize() );
            XP_ASSERT( (m_tf & TF_FONT_SIZE) == 0 );
        }
    }

    // remove the elements we've already handled.
    ED_TextFormat mask = pData->mask & ~( TF_HREF 
                                        | TF_FONT_SIZE
                                        | TF_FONT_COLOR
                                        | TF_SCRIPT
                                        | TF_SERVER );

    ED_TextFormat values = pData->values & mask;

    m_tf = (m_tf & ~mask) | values;
}

EDT_CharacterData* CEditTextElement::GetData(){
    EDT_CharacterData *pData = EDT_NewCharacterData();
    pData->mask = -1;
    pData->values = m_tf;
    pData->iSize = GetFontSize();
    if( m_tf & TF_FONT_COLOR ){
        pData->pColor = edt_MakeLoColor( GetColor() );
    }
    if( m_tf & TF_HREF ){
        if( m_href != ED_LINK_ID_NONE ){
            pData->pHREFData = m_href->GetData();
        }
        pData->linkId = m_href;
    }
    return pData;
}

void CEditTextElement::MaskData( EDT_CharacterData*& pData ){
    if( pData == 0 ){
        pData = GetData();
        return;
    }

    ED_TextFormat differences = pData->values ^ m_tf;
    if( differences ){
        pData->mask &= ~differences;

        if( (pData->mask & pData->values) & TF_FONT_COLOR 
                    && EDT_LO_COLOR( pData->pColor ) != GetColor() ){
            pData->mask &= ~TF_FONT_COLOR;
            XP_FREE( pData->pColor );
            pData->pColor = 0;
        }
        if( (pData->mask & pData->values) & TF_HREF 
                    && (pData->linkId != m_href ) ){
            pData->mask &= ~TF_HREF;
        }
        if( (pData->mask & pData->values) & TF_FONT_SIZE 
                    && (pData->iSize != m_iFontSize ) ){
            pData->mask &= ~TF_FONT_SIZE;
        }
    }
}


//
// insert a character into the text buffer. At some point we should make sure we
//  don't insert two spaces together.
//
XP_Bool CEditTextElement::InsertChar( int iOffset, int newChar ){
    //
    // If we are inserting a space at a point where there already is a space,
    //  don't do it.
    //
    if( newChar == ' '  && !InFormattedText() ) {
        if( m_pText ){
            XP_Bool bUnderSpace = (m_pText[iOffset] == ' ');    
            if( bUnderSpace  || !( iOffset != 0 && m_pText[iOffset-1] != ' ')){
                return bUnderSpace;
            }
        }
        else {
            CEditElement* pPrevious = PreviousTextInContainer();
            if( (pPrevious == 0 )
                || (pPrevious->Text()->GetText()[pPrevious->Text()->GetLen()-1]
                        == ' ' ) 
                    ){
                return FALSE;
            }
        }
    }

    // guarantee the buffer size
    //
    int iSize = GetLen()  + 1;
    if( iSize + 1 >= m_textSize ){
        int newSize = GROW_TEXT( m_textSize + 1 );
        char *pNewBuf = (char*)XP_ALLOC( newSize );
        if( pNewBuf == 0 ){
            // out of memory, should probably throw.
            return FALSE; 
        }
        if( m_pText ){
            XP_STRCPY( pNewBuf, m_pText );
            XP_FREE( m_pText );
        }
        else {
            pNewBuf[0] = 0;
        }
        m_pText = pNewBuf;
        m_textSize = newSize;
    }

    //
    // create an empty space in the string.  We have to start from the end of 
    //  the string and move backward.
    //
    int i = iSize - 1;
    while( i >= iOffset ){
        m_pText[i+1] = m_pText[i];
        i--;
    }
    m_pText[iOffset] = newChar;
    return TRUE;
}

void CEditTextElement::DeleteChar( MWContext *pContext, int iOffset ){
    
    int i = iOffset;
    char c = 1;
    int clen;

    clen = INTL_CharLen(pContext->win_csid, (unsigned char *)m_pText+iOffset);

    if( m_pText[iOffset] ){
        while( c != 0 ){
            m_pText[i] = c = m_pText[i+clen];
            i++;
        }
    }
}

char* CEditTextElement::DebugFormat(){
    static char buf[1024];
    strcpy(buf, "( " );
    int i;
    if(m_tf & TF_BOLD)         strcat(buf,"B ");
    if(m_tf & TF_ITALIC)       strcat(buf,"I ");
    if(m_tf & TF_FIXED)        strcat(buf,"TT ");
    if(m_tf & TF_SUPER)        strcat(buf,"SUP ");
    if(m_tf & TF_SUB)          strcat(buf,"SUB ");
    if(m_tf & TF_STRIKEOUT)    strcat(buf,"SO ");
    if(m_tf & TF_UNDERLINE)    strcat(buf,"U ");
    if(m_tf & TF_BLINK)        strcat(buf,"BL ");
    if(m_tf & TF_SERVER)       strcat(buf,"SERVER ");
    if(m_tf & TF_SCRIPT)       strcat(buf,"SCRIPT ");

    if(m_pScriptExtra){
        strcat(buf, m_pScriptExtra);
        strcat(buf," ");
    }

    if ( m_tf & TF_FONT_FACE && m_pFace ) {
        strcat( buf, m_pFace );
        strcat( buf, " ");
    }

    if( m_tf & TF_FONT_COLOR ){ 
        char color_str[8];
        PR_snprintf(color_str, 8, "#%06lX ", GetColor().GetAsLong() );
        strcat( buf, color_str );
    }

    if( m_tf & TF_FONT_SIZE ){
        i = XP_STRLEN(buf);
        buf[i] = (GetFontSize() < 3  ? '-': '+');
        buf[i+1] = '0' + abs(GetFontSize() - 3);
        buf[i+2] = ' ';
        buf[i+3] = 0;
    }

    if( m_tf & TF_HREF ){ 
        strcat( buf, m_href->hrefStr );
    }

    strcat(buf, ") " );
    return buf;
}   

//
// Create a list of tags that represents the character formatting for this
//  text element.
//
void CEditTextElement::FormatOpenTags(PA_Tag*& pStart, PA_Tag*& pEnd){


    if(m_tf & TF_BOLD){ edt_AddTag( pStart, pEnd, P_BOLD, FALSE ); }
    if(m_tf & TF_ITALIC){ edt_AddTag( pStart, pEnd, P_ITALIC, FALSE ); }
    if(m_tf & TF_FIXED){ edt_AddTag( pStart, pEnd, P_FIXED, FALSE ); }
    if(m_tf & TF_SUPER){ edt_AddTag( pStart,pEnd, P_SUPER, FALSE ); }
    if(m_tf & TF_SUB){ edt_AddTag( pStart,pEnd, P_SUB, FALSE ); }
    if(m_tf & TF_STRIKEOUT){ edt_AddTag( pStart,pEnd, P_STRIKEOUT, FALSE ); }
    if(m_tf & TF_UNDERLINE){ edt_AddTag( pStart,pEnd, P_UNDERLINE, FALSE ); }
    if(m_tf & TF_BLINK){ edt_AddTag( pStart,pEnd, P_BLINK, FALSE ); }

    // Face and color have to come before size for old browsers.
    if( m_tf & TF_FONT_FACE ){
        char buf[200];
        XP_STRCPY( buf, " FACE=\"" );
        strcat( buf, m_pFace );
        strcat( buf, "\">" );
        edt_AddTag( pStart,pEnd, P_FONT, FALSE, buf );
    }

    if( GetColor().IsDefined() ){ 
        char buf[20];
        PR_snprintf(buf, 20, "COLOR=\"#%06lX\">", GetColor().GetAsLong() );
        edt_AddTag( pStart,pEnd, P_FONT, FALSE, buf );
    }

    if( m_tf & TF_FONT_SIZE ){
        char buf[20];
        XP_STRCPY( buf, " SIZE=" );
        int i = XP_STRLEN(buf);
        buf[i] = (GetFontSize() < 3  ? '-': '+');
        buf[i+1] = '0' + abs(GetFontSize() - 3);
        buf[i+2] = '>';
        buf[i+3] = 0;
        edt_AddTag( pStart,pEnd, P_FONT, FALSE, buf );
    }

    if( m_tf & TF_HREF){ 
        char *pBuf = PR_smprintf("HREF=%s %s\">", edt_MakeParamString(m_href->hrefStr),
            (m_href->pExtra ? m_href->pExtra : "") );        
        edt_AddTag( pStart,pEnd, P_ANCHOR, FALSE, pBuf );
        free( pBuf );
    }

    // if EVAL, hack the character attributes...
    if(m_tf & TF_SERVER){ 
        edt_AddTag( pStart, pEnd, P_SERVER, FALSE );
        return;
    }

    if(m_tf & TF_SCRIPT){ 
        edt_AddTag( pStart, pEnd, P_SCRIPT, FALSE );
        return;
    }

}

void CEditTextElement::FormatTransitionTags(CEditTextElement* /* pNext */,
            PA_Tag*& /* pStart */, PA_Tag*& /* pEnd */){
}

void CEditTextElement::FormatCloseTags(PA_Tag*& pStart, PA_Tag*& pEnd){

    // WARNING: order here is real important, Open and Close must match
    //  exactly.

    // if EVAL must be last
    if(m_tf & TF_SCRIPT){ 
        edt_AddTag( pStart, pEnd, P_SCRIPT, TRUE );
    }

    if(m_tf & TF_SERVER){ 
        edt_AddTag( pStart, pEnd, P_SERVER, TRUE );
    }

    //if(m_href){ edt_AddTag( pStart,pEnd, P_ITALIC, FALSE ); }
    if( m_tf & TF_HREF ){ edt_AddTag( pStart,pEnd, P_ANCHOR, TRUE );}
    if( m_tf & TF_FONT_SIZE ){edt_AddTag( pStart,pEnd, P_FONT, TRUE );}
    if( GetColor().IsDefined() ){edt_AddTag( pStart,pEnd, P_FONT, TRUE );}
    if( m_tf & TF_FONT_FACE ) edt_AddTag( pStart,pEnd, P_FONT, TRUE );

    if(m_tf & TF_BLINK ){ edt_AddTag( pStart,pEnd, P_BLINK, TRUE); }
    if(m_tf & TF_STRIKEOUT ){ edt_AddTag( pStart,pEnd, P_STRIKEOUT, TRUE); }
    if(m_tf & TF_UNDERLINE ){ edt_AddTag( pStart,pEnd, P_UNDERLINE, TRUE); }
    if(m_tf & TF_SUB ){ edt_AddTag( pStart,pEnd, P_SUB, TRUE); }
    if(m_tf & TF_SUPER ){ edt_AddTag( pStart,pEnd, P_SUPER, TRUE); }
    if(m_tf & TF_FIXED ){ edt_AddTag( pStart, pEnd, P_FIXED, TRUE); }
    if(m_tf & TF_ITALIC ){ edt_AddTag( pStart, pEnd, P_ITALIC, TRUE); }
    if(m_tf & TF_BOLD ){ edt_AddTag( pStart, pEnd, P_BOLD, TRUE); }


}

//
// Create a text tag.  We are going to be feeding these tags to the layout 
//  engine.  The layout element this edit element points to is going to change.
//
PA_Tag* CEditTextElement::TagOpen( int iEditOffset ){
    // LTNOTE: I think that we only want to do this if we are the first text
    //  in the container, other wise, we don't do anything.
    PA_Tag *pStart = 0;
    PA_Tag *pEnd = 0;
    XP_Bool bFirst = IsFirstInContainer();

    if( GetLen() == 0 && (!bFirst || (bFirst && LeafInContainerAfter() != 0))){
        return 0;
    }

    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );

    FormatOpenTags( pStart, pEnd );    

    if( pStart == 0 ){
        pStart = pTag;
    }
    if( pEnd ){
        pEnd->next = pTag;
    }
    pEnd = pTag;

    if(GetLen() == 0){
        // As of 3.0b5, a non-breaking space always caused a memory leak when
        // it was laid out. See bug 23404
        // So use a different character here. The character is
        // removed by code in CEditBuffer::Relayout
        SetTagData(pTag, ".");
        //SetTagData(pTag, NON_BREAKING_SPACE_STRING);
        //SetTagData(pTag, "");
    }
    else {
        SetTagData(pTag, GetText()+iEditOffset);
    }
    if( iEditOffset==0 ){
        m_pFirstLayoutElement = 0;
    }

    FormatCloseTags( pStart, pEnd );

    return pStart;
}

XP_Bool CEditTextElement::GetLOTextAndOffset( ElementOffset iEditOffset, XP_Bool bStickyAfter, LO_TextStruct*&  pRetText, 
        int& iLayoutOffset ){
    
    LO_TextStruct *pText;
    LO_Element* pLoElement = m_pFirstLayoutElement;
    XP_ASSERT(!pLoElement || pLoElement->lo_any.edit_offset == 0);
    intn len = GetLen();

    while(pLoElement != 0){
        if( pLoElement->type == LO_TEXT ){
            // if we have scanned past our edit element, we can't find the 
            // insert point.
            pText = &(pLoElement->lo_text);
            if( pText->edit_element != this){
                /* assert(0)
                */
                return FALSE;
            }

            int16 textLen = pText->text_len;
            int32 loEnd = pText->edit_offset + textLen;

            //
            // See if we are at the dreaded lopped of the last space case.
            //
            if( loEnd+1 == iEditOffset ){
                // ok, we've scaned past the end of the Element
                if( len == iEditOffset && m_pText[iEditOffset-1] == ' '){
                    // we wraped to the next element.  Return the next Text Element.
                    CEditElement *pNext = FindNextElement(&CEditElement::FindText,0);
                    if( pNext ){
                        return pNext->Text()->GetLOTextAndOffset( 0, FALSE, pRetText, iLayoutOffset );
                    }

                    // we should have all these cases handled now.

                    // jhp Nope. (a least as of Beta 2) If you have a very narrow document,
                    // and you
                    // type in two lines of text, and you're at the end of the
                    // second line, and you type a space, and the
                    // space causes a line wrap, then you end up here.
                    // XP_ASSERT(FALSE);

                    return FALSE;
                }
            }

            if( loEnd > iEditOffset || (loEnd == iEditOffset
                && (!bStickyAfter || len == iEditOffset || textLen == 0 ) ) ){
                // we've found the right element, return the information.
                iLayoutOffset = iEditOffset - pText->edit_offset;
                pRetText = pText;
                return TRUE;
            }
            
        }
        pLoElement = pLoElement->lo_any.next;
    }
    return FALSE;
}

LO_TextStruct* CEditTextElement::GetLOText( int iEditOffset ){
    
    LO_TextStruct *pText;
    LO_Element* pLoElement = m_pFirstLayoutElement;

    while(pLoElement != 0){
        if( pLoElement->type == LO_TEXT ){
            // if we have scanned past our edit element, we can't find the 
            // insert point.
            pText = &(pLoElement->lo_text);
            if( pText->edit_element != this){
                return 0;
            }
            if( pText->edit_offset > iEditOffset ){
                return 0;
            }

            if( pText->edit_offset + pText->text_len >= iEditOffset ){
                // we've found the right element, return the information.
                return pText;
            }
            
        }
        pLoElement = pLoElement->lo_any.next;
    }
    return 0;
}

void CEditTextElement::PrintTagOpen( CPrintState *pPrintState, TagType t, 
            ED_TextFormat tf, char* pExtra ){
    if ( ! pExtra ) pExtra = ">";
    pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s%s", 
                EDT_TagString(t), pExtra );
    pPrintState->m_formatStack.Push(tf);
    pPrintState->m_elementStack.Push(this);
}


void CEditTextElement::PrintFormatDifference( CPrintState *ps, 
        ED_TextFormat bitsDifferent ){
    if(bitsDifferent & TF_BOLD){ PrintTagOpen( ps, P_BOLD, TF_BOLD ); } 
    if(bitsDifferent & TF_ITALIC){ PrintTagOpen( ps, P_ITALIC, TF_ITALIC ); } 
    if(bitsDifferent & TF_FIXED){ PrintTagOpen( ps, P_FIXED, TF_FIXED ); } 
    if(bitsDifferent & TF_SUPER){ PrintTagOpen( ps, P_SUPER, TF_SUPER ); } 
    if(bitsDifferent & TF_SUB){ PrintTagOpen( ps, P_SUB, TF_SUB ); } 
    if(bitsDifferent & TF_STRIKEOUT){ PrintTagOpen( ps, P_STRIKEOUT, TF_STRIKEOUT ); } 
    if(bitsDifferent & TF_UNDERLINE){ PrintTagOpen( ps, P_UNDERLINE, TF_UNDERLINE ); } 
    if(bitsDifferent & TF_BLINK){ PrintTagOpen( ps, P_BLINK, TF_BLINK ); } 

    // Face and color have to preceed FONT_SIZE, or else old browsers will
    // reset the font_size to zero when they encounter the unknown tags.

    if( bitsDifferent & TF_FONT_FACE ){
        ps->m_iCharPos += ps->m_pOut->Printf( "<FONT FACE=\"%s\">", m_pFace);
        ps->m_formatStack.Push(TF_FONT_FACE);
        ps->m_elementStack.Push(this);
    }

    if( bitsDifferent & TF_FONT_COLOR ){
        ps->m_iCharPos += ps->m_pOut->Printf( "<FONT COLOR=\"#%06lX\">", 
                    GetColor().GetAsLong() );
        ps->m_formatStack.Push(TF_FONT_COLOR);
        ps->m_elementStack.Push(this);
    }

    if( bitsDifferent & TF_FONT_SIZE ){
        char buf[4];
        buf[0] = (GetFontSize() < 3  ? '-': '+');
        buf[1] = '0' + abs(GetFontSize() - 3);
        buf[2] = 0;
        ps->m_iCharPos += ps->m_pOut->Printf( "<FONT SIZE=%s>", buf);
        ps->m_formatStack.Push(TF_FONT_SIZE);
        ps->m_elementStack.Push(this);
    }

    if( bitsDifferent & TF_HREF){ 
        // doesn't use the output routine because HREF can be large.
        char *pS1 = "";
        char *pS2 = "";
        if( m_href->pExtra ){
            pS1 = " ";
            pS2 = m_href->pExtra;
        }
        ps->m_iCharPos += ps->m_pOut->Printf( 
            "<A HREF=%s%s%s>", edt_MakeParamString(m_href->hrefStr), pS1, pS2 );

        ps->m_formatStack.Push(TF_HREF);
        ps->m_elementStack.Push(this);
    }

    if(bitsDifferent & TF_SCRIPT){
        PrintTagOpen( ps, P_SCRIPT, TF_SCRIPT, GetScriptExtra() ); } 
    if(bitsDifferent & TF_SERVER){ PrintTagOpen( ps, P_SERVER, TF_SERVER, GetScriptExtra() ); } 
}

void CEditTextElement::PrintFormat( CPrintState *ps, 
                                    CEditTextElement *pFirst, 
                                    ED_TextFormat mask ){
    ED_TextFormat bitsCommon = 0;
    ED_TextFormat bitsDifferent = 0;

    ComputeDifference(pFirst, mask, bitsCommon, bitsDifferent);

    CEditTextElement *pNext = (CEditTextElement*) TextInContainerAfter();

    if( bitsCommon ){
        // While we're in a run of the same style, there's nothing to do.
        // Avoid unnescessary recursion so we don't blow the stack on Mac or Win16
        while ( pNext && SameAttributes(pNext) ){
            pNext = (CEditTextElement*) pNext->TextInContainerAfter();
        }
        if( pNext ){
            pNext->PrintFormat( ps, pFirst, bitsCommon );
        }
        else {
            // we hit the end, so we have to open everything
            pFirst->PrintFormatDifference( ps, bitsCommon);
        }
    }
    if( bitsDifferent ){
        pFirst->PrintFormatDifference( ps, bitsDifferent );
    }
}

XP_Bool CEditTextElement::SameAttributes(CEditTextElement *pCompare){
    ED_TextFormat bitsCommon;
    ED_TextFormat bitsDifferent;
    ComputeDifference(pCompare, -1, bitsCommon, bitsDifferent);
    return ( bitsCommon == m_tf);
}

void CEditTextElement::ComputeDifference(CEditTextElement *pFirst, 
        ED_TextFormat mask, ED_TextFormat& bitsCommon, ED_TextFormat& bitsDifferent){
    bitsCommon = m_tf & mask;
    bitsDifferent = mask & ~bitsCommon;

    if( (bitsCommon & TF_FONT_SIZE)&& (GetFontSize() != pFirst->GetFontSize())){
        bitsCommon &= ~TF_FONT_SIZE;
        bitsDifferent |= TF_FONT_SIZE;
    }

    if( (bitsCommon & TF_FONT_COLOR)&& (GetColor() != pFirst->GetColor())){
        bitsCommon &= ~TF_FONT_COLOR;
        bitsDifferent |= TF_FONT_COLOR;
    }

    if( bitsCommon & TF_FONT_FACE ) {
        char* a = GetFontFace();
        char* b = pFirst->GetFontFace();
        XP_Bool bA = a != NULL;
        XP_Bool bB = b != NULL;
        if ((bA ^ bB) ||
            (bA && bB && XP_STRCMP(a, b) != 0)) {
            bitsCommon &= ~TF_FONT_FACE;
            bitsDifferent |= TF_FONT_FACE;
        }
    }

    if( (bitsCommon & TF_HREF)&& (GetHREF() != pFirst->GetHREF())){
        bitsCommon &= ~TF_HREF;
        bitsDifferent |= TF_HREF;
    }
}

void CEditTextElement::PrintTagClose( CPrintState *ps, TagType t ){
    ps->m_iCharPos += ps->m_pOut->Printf( "</%s>", 
                EDT_TagString(t) );
}

void CEditTextElement::PrintPopFormat( CPrintState *ps, int iStackTop ){

    while( ps->m_formatStack.m_iTop >= iStackTop ){
        
        ED_TextFormat tf = (ED_TextFormat) ps->m_formatStack.Pop();
        ps->m_elementStack.Pop();

        if( tf & TF_SCRIPT){ PrintTagClose( ps, P_SCRIPT ); } 
        if( tf & TF_SERVER){ PrintTagClose( ps, P_SERVER ); } 
        if( tf & TF_BOLD){ PrintTagClose( ps, P_BOLD ); } 
        if( tf & TF_ITALIC){ PrintTagClose( ps, P_ITALIC ); } 
        if( tf & TF_FIXED){ PrintTagClose( ps, P_FIXED ); } 
        if( tf & TF_SUPER){ PrintTagClose( ps, P_SUPER ); } 
        if( tf & TF_SUB){ PrintTagClose( ps, P_SUB ); } 
        if( tf & TF_STRIKEOUT){ PrintTagClose( ps, P_STRIKEOUT ); } 
        if( tf & TF_UNDERLINE){ PrintTagClose( ps, P_UNDERLINE ); } 
        if( tf & TF_BLINK){ PrintTagClose( ps, P_BLINK ); } 
        if( tf & TF_FONT_SIZE){ PrintTagClose( ps, P_FONT ); } 
        if( tf & TF_FONT_COLOR){ PrintTagClose( ps, P_FONT ); } 
        if( tf & TF_FONT_FACE){ PrintTagClose( ps, P_FONT ); } 
        if( tf & TF_HREF){ PrintTagClose( ps, P_ANCHOR ); } 
    }
}

ED_TextFormat CEditTextElement::PrintFormatClose( CPrintState *ps ){
    XP_Bool bDiff = FALSE;
    ED_TextFormat bitsNeedFormat = m_tf ;

    if( ps->m_formatStack.IsEmpty()){
        return bitsNeedFormat;        
    }
    int i = -1;
    while( !bDiff && ++i <= ps->m_formatStack.m_iTop ){
        ED_TextFormat tf = (ED_TextFormat) ps->m_formatStack[i];
        if( m_tf & tf ){
            if( tf == TF_FONT_COLOR 
                    && GetColor() != ps->m_elementStack[i]->GetColor()){
                bDiff = TRUE;
            }
            else if( tf == TF_FONT_SIZE 
                    && GetFontSize() != ps->m_elementStack[i]->GetFontSize()){
                bDiff = TRUE;
            }
            else if( tf == TF_FONT_FACE 
                    && XP_STRCMP(GetFontFace(), ps->m_elementStack[i]->GetFontFace()) != 0){
                bDiff = TRUE;
            }
            else if( tf == TF_HREF
                    && GetHREF() != ps->m_elementStack[i]->GetHREF() ){
                bDiff = TRUE;
            }
        }
        else {
            bDiff = TRUE;
        }

        // if the stuff is on the stack, then it doesn't need to be formatted
        if( !bDiff ){
            bitsNeedFormat &=  ~tf;
        }
    }
    PrintPopFormat( ps, i );
    return bitsNeedFormat;

}

// for use within 
void CEditTextElement::PrintWithEscapes( CPrintState *ps ){
    char *p = GetText();

    edt_PrintWithEscapes( ps, p, !InFormattedText() );
}

void CEditTextElement::PrintLiteral( CPrintState *ps ){
    int iLen = GetLen();
    ps->m_pOut->Write( GetText(), iLen );
    ps->m_iCharPos += iLen;
}

void CEditTextElement::PrintOpen( CPrintState *ps){
    ED_TextFormat bitsToFormat = PrintFormatClose( ps );

    CEditTextElement *pNext = (CEditTextElement*) TextInContainerAfter();
    if( pNext ){
        pNext->PrintFormat( ps, this, bitsToFormat );
    }
    else {
        PrintFormatDifference( ps, bitsToFormat );
    }
    
    if( m_tf & (TF_SERVER|TF_SCRIPT) ){
        PrintLiteral( ps );
    }
    else {
        PrintWithEscapes( ps );
    }

    if( pNext == 0 ) {
        PrintPopFormat( ps, 0 );
    }
}


XP_Bool CEditTextElement::Reduce( CEditBuffer *pBuffer ){
    if( GetLen() == 0 ){
        if( pBuffer->m_pCurrent == this 
                || (IsFirstInContainer()
                        && LeafInContainerAfter() == 0)
            ){
            return FALSE;
        }
        else {
            return TRUE;
        }
    }
    else {
        return FALSE;
    }
}

#ifdef DEBUG
void CEditTextElement::ValidateTree(){
    CEditLeafElement::ValidateTree();
    LO_TextStruct *pText = (LO_TextStruct*) GetLayoutElement();

    if( GetType() != P_TEXT  ){
        XP_ASSERT(FALSE);
    }
#if 0
    if(GetLen() != 0 && pText == 0 ){
        XP_ASSERT(FALSE);
    }
#endif
    if(m_pText && pText ) {
        if (pText->edit_element != this){
            XP_ASSERT(FALSE);
        }
        if (pText->edit_offset != 0){
            XP_ASSERT(FALSE);
        }
    }
    XP_Bool bHasLinkTag = (m_tf & TF_HREF) != 0;
    XP_Bool bHasLinkRef = (m_href != ED_LINK_ID_NONE);
    if( bHasLinkTag != bHasLinkRef ){
        XP_ASSERT(FALSE);
    };
}
#endif	//	DEBUG


//-----------------------------------------------------------------------------
// Insertion routines.
//-----------------------------------------------------------------------------

//
// Split text object into two text objects.  Parent is the same.
//
CEditElement* CEditTextElement::SplitText( int iOffset ){
    CEditTextElement* pNew = new CEditTextElement( 0, m_pText+iOffset );
    m_pText[iOffset] = 0;
    GetParent()->Split( this, pNew, &CEditElement::SplitContainerTest, 0 );
    return  pNew;
}

CEditTextElement* CEditTextElement::CopyEmptyText( CEditElement *pParent ){
    CEditTextElement* pNew = new CEditTextElement(pParent, 0);

    pNew->m_tf = m_tf;
    pNew->m_iFontSize = m_iFontSize;
    pNew->m_color = m_color;
    if( (m_tf & TF_HREF) && m_href != ED_LINK_ID_NONE){
        pNew->SetHREF(m_href);
    }
    if ( m_pFace ) {
        pNew->SetFontFace(m_pFace);
    }
    if ( m_pScriptExtra ) {
        pNew->SetScriptExtra(m_pScriptExtra);
    }
    return pNew;
}

void CEditTextElement::DeleteText(){
    CEditElement *pKill = this;
    CEditElement *pParent;

    do {
        pParent = pKill->GetParent();
        pKill->Unlink();
        delete pKill;
        pKill = pParent;
    } while( BitSet( edt_setCharFormat, pKill->GetType() ) && pKill->GetChild() == 0 );
}

//----------------------------------------------------------------------------
// CEditImageElement
//----------------------------------------------------------------------------

//
// LTNOTE: these should be static functions on CEditImageData..
//
EDT_ImageData* edt_NewImageData(){
    EDT_ImageData *pData = XP_NEW( EDT_ImageData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->bIsMap = FALSE;
    pData->pUseMap = 0;
    pData->align = ED_ALIGN_DEFAULT;
    pData->pSrc = 0;
    pData->pLowSrc = 0;
    pData->pName = 0;
    pData->pAlt = 0;
    pData->iWidth = 0;
    pData->iHeight = 0;
    pData->bWidthPercent = FALSE;
    pData->bHeightPercent = FALSE;
    pData->iHSpace = 0;
    pData->iVSpace = 0;
    pData->iBorder = -1;
    pData->pHREFData = NULL;
    pData->pExtra = 0;
    return pData;
}

inline char *edt_StrDup( char *pStr ){
    if( pStr ){ 
        return XP_STRDUP( pStr );
    }
    else {
        return 0;
    }
}

EDT_ImageData* edt_DupImageData( EDT_ImageData *pOldData ){
    EDT_ImageData *pData = XP_NEW( EDT_ImageData );

    pData->pUseMap = edt_StrDup(pOldData->pUseMap);
    pData->pSrc = edt_StrDup(pOldData->pSrc);
    pData->pName = edt_StrDup(pOldData->pName);
    pData->pLowSrc = edt_StrDup(pOldData->pLowSrc);
    pData->pAlt = edt_StrDup(pOldData->pAlt);
    pData->pExtra = edt_StrDup(pOldData->pExtra);

    pData->bIsMap = pOldData->bIsMap;
    pData->align = pOldData->align;
    pData->iWidth = pOldData->iWidth;
    pData->iHeight = pOldData->iHeight;
    pData->bWidthPercent = pOldData->bWidthPercent;
    pData->bHeightPercent = pOldData->bHeightPercent;
    pData->iHSpace = pOldData->iHSpace;
    pData->iVSpace = pOldData->iVSpace;
    pData->iBorder = pOldData->iBorder;

    if( pOldData->pHREFData ){
        pData->pHREFData = EDT_DupHREFData( pOldData->pHREFData );
    }
    else {
        pData->pHREFData = 0;
    }
        
    return pData;
}


void edt_FreeImageData( EDT_ImageData *pData ){
    if( pData->pUseMap ) XP_FREE( pData->pUseMap );
    if( pData->pSrc ) XP_FREE( pData->pSrc );
    if( pData->pLowSrc ) XP_FREE( pData->pLowSrc );
    if( pData->pName ) XP_FREE( pData->pName );
    if( pData->pAlt ) XP_FREE( pData->pAlt );
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    if( pData->pHREFData ) EDT_FreeHREFData( pData->pHREFData );
    XP_FREE( pData );
}

//
// Constructors, Streamers..
//
CEditImageElement::CEditImageElement( CEditElement *pParent, PA_Tag* pTag,
                ED_LinkId href ):
            CEditLeafElement( pParent, P_IMAGE ), 
            m_pLoImage(0),
            m_pParams(0),
            m_iHeight(0),
            m_iWidth(0),
            m_iSaveIndex(0),
            m_iSaveLowIndex(0),
            m_href(ED_LINK_ID_NONE),
            m_align( ED_ALIGN_DEFAULT),
            m_bSizeWasGiven(FALSE),
            m_bSizeIsBogus(FALSE)
{
    // must set href before calling SetImageData to allow GetDefaultBorder to work
    if( href != ED_LINK_ID_NONE ){
        SetHREF( href );
    }
    if( pTag ){
        EDT_ImageData *pData = ParseParams( pTag );
        if(  pData->iBorder == -1 ){
            pData->iBorder = GetDefaultBorder();
        }
        SetImageData( pData );
        edt_FreeImageData( pData );
    }
}

CEditImageElement::CEditImageElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer):
            CEditLeafElement(pStreamIn, pBuffer), 
            m_pLoImage(0),
            m_pParams(0), 
            m_href(ED_LINK_ID_NONE),
            m_align(ED_ALIGN_DEFAULT) {
    m_align = (ED_Alignment) pStreamIn->ReadInt();
    m_bSizeWasGiven = (XP_Bool) pStreamIn->ReadInt();
    m_bSizeIsBogus = (XP_Bool) pStreamIn->ReadInt();
    m_iHeight = pStreamIn->ReadInt();
    m_iWidth = pStreamIn->ReadInt();
    m_pParams = pStreamIn->ReadZString();
    char *pHrefString = pStreamIn->ReadZString();
    char *pExtra = pStreamIn->ReadZString();
    if( pHrefString ){
        SetHREF( pBuffer->linkManager.Add( pHrefString, pExtra ));
        XP_FREE( pHrefString );
        if( pExtra ) XP_FREE( pExtra );
    }
}

CEditImageElement::~CEditImageElement(){
    DisconnectLayoutElements((LO_Element*) m_pLoImage);
    if( m_pParams) XP_FREE( m_pParams ); 
    if( m_href != ED_LINK_ID_NONE ) m_href->Release();
}

void CEditImageElement::SetLayoutElement( intn iEditOffset, intn lo_type, 
                        LO_Element* pLoElement ){
    SetLayoutElementHelper(LO_IMAGE, (LO_Element**) &m_pLoImage,
        iEditOffset, lo_type, pLoElement);
}

void CEditImageElement::ResetLayoutElement( intn iEditOffset, 
            LO_Element* pLoElement ){
    ResetLayoutElementHelper((LO_Element**) &m_pLoImage,
        iEditOffset, pLoElement);
}

LO_Element* CEditImageElement::GetLayoutElement(){
    return (LO_Element*)m_pLoImage;
}

void CEditImageElement::StreamOut( IStreamOut *pOut){
    CEditLeafElement::StreamOut( pOut );
    
    pOut->WriteInt( (int32)m_align );
    pOut->WriteInt( m_bSizeWasGiven );
    pOut->WriteInt( m_bSizeIsBogus );
    pOut->WriteInt( m_iHeight );
    pOut->WriteInt( m_iWidth );

    // We need to make the Image Src and LowSrc Parameters absolute.
    // We do this by first converting the parameter string to and Edit
    //  data structure and then making the paths absolute.
    EDT_ImageData *pData = GetImageData();
    CEditBuffer *pBuffer = GetEditBuffer();
    if( pData && pBuffer && pBuffer->GetBaseURL() ){
        char *p;
        if( pData->pSrc ) {
            p = NET_MakeAbsoluteURL( pBuffer->GetBaseURL(), pData->pSrc );
            if( p ){
                XP_FREE( pData->pSrc );
                pData->pSrc = p;
            }
        }
        if( pData->pLowSrc ) {
            p = NET_MakeAbsoluteURL( pBuffer->GetBaseURL(), pData->pLowSrc );
            if( p ){
                XP_FREE( pData->pLowSrc );
                pData->pLowSrc = p;
            }
        }

        // Format the parameters for output.
        p = FormatParams( pData, TRUE );
        pOut->WriteZString( p );
        XP_FREE(p);
        edt_FreeImageData( pData );
    }
    else {
        pOut->WriteZString( m_pParams );
    }


    if( m_href != ED_LINK_ID_NONE ){
        pOut->WriteZString( m_href->hrefStr );
        pOut->WriteZString( m_href->pExtra );
    }
    else {
        pOut->WriteZString( 0 );
        pOut->WriteZString( 0 );
    }
}

EEditElementType CEditImageElement::GetElementType()
{
    return eImageElement;
}

PA_Tag* CEditImageElement::TagOpen( int /* iEditOffset */ ){
    char *pCur = 0;
    PA_Tag *pRetTag = 0;
    PA_Tag *pEndTag = 0;

    if( m_href != ED_LINK_ID_NONE ){
        char *pBuf = PR_smprintf("HREF=%s %s>", edt_MakeParamString(m_href->hrefStr),
            (m_href->pExtra ? m_href->pExtra : "") );        
        edt_AddTag( pRetTag, pEndTag, P_ANCHOR, FALSE, pBuf );
        free( pBuf );
    }

    if( m_pParams ){
        if( SizeIsKnown() ){    
            pCur = PR_smprintf("%s HEIGHT=%ld WIDTH=%ld ", m_pParams, 
                        (long)m_iHeight, (long)m_iWidth );
        }
        else {
            // We don't know the size, but if we don't give a size,
            // and if the image isn't known, we will block and
            // crash. So we put in a bogus size here, and take it
            // out in FinishedLoad iff there's image data available.
            m_bSizeIsBogus = TRUE;
            pCur = PR_smprintf("%s HEIGHT=24 WIDTH=22", m_pParams );
        }
    }        

    // For editing, we don't support baseline, left or right alignment of images.

    if( m_align != ED_ALIGN_DEFAULT && m_align != ED_ALIGN_BASELINE 
                && m_align != ED_ALIGN_LEFT && m_align != ED_ALIGN_RIGHT ){
        pCur = PR_sprintf_append(pCur, "ALIGN=%s ", EDT_AlignString(m_align) );
    }
    
    pCur = PR_sprintf_append(pCur, ">");

    edt_AddTag( pRetTag, pEndTag, P_IMAGE, FALSE, pCur );
    pEndTag->edit_element = this;

    if( m_href != ED_LINK_ID_NONE ){
        edt_AddTag( pRetTag, pEndTag, P_ANCHOR, TRUE );
    }
    free( pCur );
    return pRetTag;
}

void CEditImageElement::PrintOpen( CPrintState *pPrintState ){
    char *pCur = 0;
    char *pAlignStr = "";

    if( m_href != ED_LINK_ID_NONE ){
        char *pS1 = "";
        char *pS2 = "";
        if( m_href->pExtra ){
            pS1 = " ";
            pS2 = m_href->pExtra;
        }
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( 
            "<A HREF=%s%s%s>", edt_MakeParamString(m_href->hrefStr), pS1, pS2 );
    }

    // Use special versions of params which has BORDER removed if it's the default

    char* pParams = NULL;
    {
        EDT_ImageData* pData = GetImageData();
        pParams = FormatParams(pData, TRUE);
        EDT_FreeImageData(pData);
    }

    if( SizeIsKnown() && ! m_bSizeIsBogus ){
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<IMG %sHEIGHT=%ld WIDTH=%ld", 
                    (pParams ? pParams : " " ),
                    (long)m_iHeight,
                    (long)m_iWidth );
    }
    else {
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<IMG %s", 
                    (pParams ? pParams : " " ));
    }

    if ( pParams )
        XP_FREE(pParams);

    if( m_align != ED_ALIGN_DEFAULT && m_align != ED_ALIGN_BASELINE ){
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( " ALIGN=%s", 
                EDT_AlignString(m_align));
    }

    pPrintState->m_pOut->Write( ">", 1 );
    pPrintState->m_iCharPos++;
    
    if( m_href != ED_LINK_ID_NONE ){
        pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( 
            "</A>" );        
    }

}

static char *imageParams[] = {
    PARAM_ISMAP,
    PARAM_USEMAP,
    PARAM_SRC,
    PARAM_LOWSRC,
    PARAM_NAME,
    PARAM_ALT,
    PARAM_WIDTH,
    PARAM_HEIGHT,
    PARAM_ALIGN,
    PARAM_HSPACE,
    PARAM_VSPACE,
    PARAM_BORDER,
    PARAM_ALIGN,
    0
};

EDT_ImageData* CEditImageElement::ParseParams( PA_Tag *pTag ){
    PA_Block buff;

    EDT_ImageData* pData = edt_NewImageData();

    buff = PA_FetchParamValue(pTag, PARAM_ISMAP);
    if (buff != NULL){
        pData->bIsMap = TRUE;
        PA_FREE(buff);
    }
    pData->pUseMap = edt_FetchParamString( pTag, PARAM_USEMAP );
    pData->pSrc = edt_FetchParamString( pTag, PARAM_SRC );
    pData->pLowSrc = edt_FetchParamString( pTag, PARAM_LOWSRC );
    pData->pName = edt_FetchParamString( pTag, PARAM_NAME );
    pData->pAlt = edt_FetchParamString( pTag, PARAM_ALT );
    
    // Get width dimension from string and parse for "%" Default = 100%
    edt_FetchDimension( pTag, PARAM_WIDTH, 
                        &pData->iWidth, &pData->bWidthPercent,
                        0, FALSE );
    edt_FetchDimension( pTag, PARAM_HEIGHT, 
                        &pData->iHeight, &pData->bHeightPercent,
                        0, FALSE );
    m_bSizeWasGiven = pData->iWidth != 0 && pData->iHeight != 0;

    pData->align = edt_FetchParamAlignment( pTag, ED_ALIGN_BASELINE );

    pData->iHSpace = edt_FetchParamInt( pTag, PARAM_HSPACE, 0 );
    pData->iVSpace = edt_FetchParamInt( pTag, PARAM_VSPACE, 0 );
    pData->iBorder = edt_FetchParamInt( pTag, PARAM_BORDER, -1 );
    pData->pHREFData = 0;

    pData->pExtra = edt_FetchParamExtras( pTag, imageParams );

    return pData;
}

EDT_ImageData* CEditImageElement::GetImageData(){
    EDT_ImageData *pRet;

    // LTNOTE: hackola time.  We don't want TagOpen to return multiple tags
    //  in this use, so we prevent it by faking out the href ID and restoring
    //  it when we are done.  Not real maintainable.

    ED_LinkId saveHref = m_href;
    m_href = ED_LINK_ID_NONE;
    PA_Tag* pTag = TagOpen(0);
    m_href = saveHref;

    pRet = ParseParams( pTag );

    if ( pRet->iBorder < 0 ) {
        pRet->iBorder = GetDefaultBorder();
    }

    if ( m_bSizeIsBogus ) {
        pRet->iWidth = 0; pRet->bWidthPercent = FALSE;
        pRet->iHeight = 0; pRet->bHeightPercent = FALSE;
    }

    if( m_href != ED_LINK_ID_NONE ){
        pRet->pHREFData = m_href->GetData();
    }
    pRet->align = m_align;
    PA_FreeTag( pTag );
    return pRet;
}

inline int edt_strlen(char* pStr){ return pStr ? XP_STRLEN( pStr ) : 0 ; }


//
// border is weird.  If we are in a link the border is default 2, if 
//  we are not in a link, border is default 0.  In order not to foist this
//  weirdness on the user, we are always explicit on our image borders.
//
int32 CEditImageElement::GetDefaultBorder(){
    return ( m_href != ED_LINK_ID_NONE ) ? 2 : 0;
}

void CEditImageElement::SetImageData( EDT_ImageData *pData ){
    char* pCur = FormatParams(pData, FALSE);

    if( m_pParams ){
        XP_FREE( m_pParams );
    }
    m_pParams = pCur;

    m_align = pData->align;
    
    if (pData->iHeight != 0 && pData->iWidth != 0 ) {
        m_bSizeWasGiven = TRUE;
        m_bSizeIsBogus = FALSE;
    }
    else {
        m_bSizeWasGiven = FALSE;
    }
    m_iHeight = pData->iHeight;
    m_iWidth = pData->iWidth;
}

char* CEditImageElement::FormatParams(EDT_ImageData* pData, XP_Bool bForPrinting){
    char *pCur = 0;

    if( pData->bIsMap ){ 
        pCur = PR_sprintf_append( pCur, "ISMAP " );
    }

    if( pData->pUseMap ){
        pCur = PR_sprintf_append( pCur, "USEMAP=%s ", edt_MakeParamString(pData->pUseMap) );
    }

    if( pData->pSrc ){
        pCur = PR_sprintf_append( pCur, "SRC=%s ", edt_MakeParamString( pData->pSrc ) );
    }

    if( pData->pLowSrc ){
        pCur = PR_sprintf_append( pCur, "LOWSRC=%s ", edt_MakeParamString( pData->pLowSrc ) );
    }

    if( pData->pName ){
        pCur = PR_sprintf_append( pCur, "NAME=%s ", edt_MakeParamString( pData->pName ) );
    }

    if( pData->pAlt ){
        pCur = PR_sprintf_append( pCur, "ALT=%s ", edt_MakeParamString( pData->pAlt ) );
    }

    if( pData->iHSpace ){
        pCur = PR_sprintf_append( pCur, "HSPACE=%ld ", (long)pData->iHSpace );
    }

    if( pData->iVSpace ){
        pCur = PR_sprintf_append( pCur, "VSPACE=%ld ", (long)pData->iVSpace );
    }

    // border is weird.  If we are in a link the border is default 2, if 
    // we are not in a link, border is default 0.  When printing, we only write out a
    // border if it's different than the default.
    //
    // Other times, we always write it out because we want it to stay the same even if the
    // default border size changes. Whew!
    {
        if( !bForPrinting || ( pData->iBorder  >=0 && pData->iBorder != GetDefaultBorder())){
            int32 border = pData->iBorder;
            if ( border < 0 )
                border = GetDefaultBorder();
            pCur = PR_sprintf_append( pCur, "BORDER=%ld ", (long)pData->iBorder );
        }
    }

    if( pData->pExtra  != 0 ){
        pCur = PR_sprintf_append( pCur, "%s ", pData->pExtra);
    }
    return pCur;
}


void CEditImageElement::FinishedLoad( CEditBuffer *pBuffer ){
    if( m_pLoImage ){
        if ( ! m_bSizeWasGiven || m_bSizeIsBogus ) {
            // The user didn't specify a size. Look at what we got.
            m_iHeight = pBuffer->m_pContext->convertPixY * m_pLoImage->height;
            m_iWidth = pBuffer->m_pContext->convertPixX * m_pLoImage->width;
            if ( m_pLoImage->il_image ) {
                // If there is image data, believe it.
                if ( m_pLoImage->il_image->height > 0 && m_pLoImage->il_image->width > 0 ) {
                    m_iHeight = pBuffer->m_pContext->convertPixY * m_pLoImage->il_image->height;
                    m_iWidth = pBuffer->m_pContext->convertPixX * m_pLoImage->il_image->width;
                    m_bSizeIsBogus = FALSE;
                }
                else {
                    // This case happens when the user canceled the original browser
                    // load, and then hit the edit button.
                    // The size is bogus.
                    // (The image is a broken link icon (24, 22).)
                    // ToDo: Figure out how to trigger a document reload once the image is
                    // finally loaded.
                    m_bSizeIsBogus = TRUE;
                    m_iHeight = 0;
                    m_iWidth = 0;
                }
            }
        }
    }
}

void CEditImageElement::SetHREF( ED_LinkId id ){
    if( m_href != ED_LINK_ID_NONE ){
        m_href->Release();
    }
    m_href = id;
    if( id != ED_LINK_ID_NONE ){
        id->AddRef();
    }
}

XP_Bool CEditImageElement::SizeIsKnown() {
    return m_iHeight > 0;
}

void CEditImageElement::MaskData( EDT_CharacterData*& pData ){
    if( pData == 0 ){
        pData = GetCharacterData();
        return;
    }
    
    // An image can have an HREF text format
    ED_TextFormat tf = ( m_href == ED_LINK_ID_NONE ) ? 0 : TF_HREF;

    ED_TextFormat differences = pData->values ^ tf;
    if( differences ){
        pData->mask &= ~differences;
        if( (pData->mask & pData->values) & TF_HREF 
                    && (pData->linkId != m_href ) ){
            pData->mask &= ~TF_HREF;
        }
    }
}

EDT_CharacterData* CEditImageElement::GetCharacterData(){
    EDT_CharacterData *pData = EDT_NewCharacterData();
    // We are only interested in HREF?
    // pData->mask = TF_HREF;
    pData->mask = -1;
    if( m_href != ED_LINK_ID_NONE ){
        pData->pHREFData = m_href->GetData();
        pData->values = TF_HREF;
    }
    pData->linkId = m_href;
    return pData;
}
//-----------------------------------------------------------------------------
// CEditHorizRuleElement
//-----------------------------------------------------------------------------


CEditHorizRuleElement::CEditHorizRuleElement( CEditElement *pParent, PA_Tag* pTag ):
            CEditLeafElement( pParent, P_HRULE ), 
            m_pLoHorizRule(0)
            {

    if( pTag ){
        char *locked_buff;
                
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditHorizRuleElement::CEditHorizRuleElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer):
            CEditLeafElement(pStreamIn, pBuffer), 
            m_pLoHorizRule(0)
            {
}

CEditHorizRuleElement::~CEditHorizRuleElement(){
    DisconnectLayoutElements((LO_Element*) m_pLoHorizRule);
}


LO_Element* CEditHorizRuleElement::GetLayoutElement(){
    return (LO_Element*)m_pLoHorizRule;
}

void CEditHorizRuleElement::SetLayoutElement( intn iEditOffset, intn lo_type, 
                        LO_Element* pLoElement ){
    SetLayoutElementHelper(LO_HRULE, (LO_Element**) &m_pLoHorizRule,
        iEditOffset, lo_type, pLoElement);
}

void CEditHorizRuleElement::ResetLayoutElement( intn iEditOffset, 
            LO_Element* pLoElement ){
    ResetLayoutElementHelper((LO_Element**) &m_pLoHorizRule,
        iEditOffset, pLoElement);
}

void CEditHorizRuleElement::StreamOut( IStreamOut *pOut ){
    CEditLeafElement::StreamOut( pOut );
}

EEditElementType CEditHorizRuleElement::GetElementType()
{
    return eHorizRuleElement;
}

// Property Getting and Setting Stuff.

EDT_HorizRuleData* CEditHorizRuleElement::NewData(){
    EDT_HorizRuleData *pData = XP_NEW( EDT_HorizRuleData );
    if( pData == 0 ){
        // throw();
        return pData;
    }
    pData->align = ED_ALIGN_CENTER;
    pData->size = DEFAULT_HR_THICKNESS;
    pData->bNoShade = 0;
    pData->iWidth = 100;
    pData->bWidthPercent = TRUE;
    pData->pExtra = 0;
    return pData;
}

void CEditHorizRuleElement::FreeData( EDT_HorizRuleData *pData ){
    if( pData->pExtra ) XP_FREE( pData->pExtra );
    XP_FREE( pData );
}

void CEditHorizRuleElement::SetData( EDT_HorizRuleData *pData ){
    char *pNew = 0;

    if( pData->align == ED_ALIGN_RIGHT || pData->align == ED_ALIGN_LEFT ){
        pNew = PR_sprintf_append( pNew, "ALIGN=%s ", EDT_AlignString(pData->align) );
    }
    if( pData->size != DEFAULT_HR_THICKNESS ){
        pNew = PR_sprintf_append( pNew, "SIZE=%ld ", (long)pData->size );
    }
    if( pData->bNoShade ){
        pNew = PR_sprintf_append( pNew, "NOSHADE " );
    }
    if( pData->iWidth ){
        if( pData->bWidthPercent ){
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld%%\" ", 
                                      (long)min(100,max(1,pData->iWidth)) );
        }
        else {
            pNew = PR_sprintf_append( pNew, "WIDTH=\"%ld\" ", (long)pData->iWidth );
        }
    }
    if( pData->pExtra){
        pNew = PR_sprintf_append( pNew, "%s ", pData->pExtra );
    }

    pNew = PR_sprintf_append( pNew, ">" );

    SetTagData( pNew );        
    free(pNew);
}

EDT_HorizRuleData* CEditHorizRuleElement::GetData( ){
    EDT_HorizRuleData *pRet;
    PA_Tag* pTag = TagOpen(0);
    pRet = ParseParams( pTag );
    PA_FreeTag( pTag );
    return pRet;
}

static char *hruleParams[] = {
    PARAM_ALIGN,
    PARAM_NOSHADE,
    PARAM_WIDTH,
    PARAM_SIZE,
    0
};

EDT_HorizRuleData* CEditHorizRuleElement::ParseParams( PA_Tag *pTag ){
    EDT_HorizRuleData *pData = NewData();
    ED_Alignment align;
    
    align = edt_FetchParamAlignment( pTag, ED_ALIGN_CENTER );
    if( align == ED_ALIGN_RIGHT || align == ED_ALIGN_LEFT ){
        pData->align = align;
    }
    // Get width dimension from string and parse for "%" Default = 100%
    edt_FetchDimension( pTag, PARAM_WIDTH, 
                        &pData->iWidth, &pData->bWidthPercent,
                        100, TRUE );
    pData->bNoShade = edt_FetchParamBoolExist( pTag, PARAM_NOSHADE );
    pData->size = edt_FetchParamInt( pTag, PARAM_SIZE, DEFAULT_HR_THICKNESS );
    pData->pExtra = edt_FetchParamExtras( pTag, hruleParams );
    return pData;
}

//-----------------------------------------------------------------------------
// CEditIconElement
//-----------------------------------------------------------------------------
static char *ppIconTags[] = {
    "align=absbotom border=0 src=\"internal-edit-named-anchor\" alt=\"%s\">",
    "align=absbotom border=0 src=\"internal-edit-form-element\">",
    "src=\"internal-edit-unsupported-tag\" alt=\"<%s\"",
    "src=\"internal-edit-unsupported-end-tag\" alt=\"</%s\"",
    "align=absbotom border=0 src=\"internal-edit-java\">",
    "align=absbotom border=0 src=\"internal-edit-plugin\">",
    0
};

CEditIconElement::CEditIconElement( CEditElement *pParent, int32 iconTag, 
                        PA_Tag* pTag )
            :
                CEditLeafElement( pParent, P_IMAGE ), 
                m_originalTagType( pTag ? pTag->type : P_UNKNOWN ),
                m_iconTag( iconTag ),
                m_bEndTag( pTag ? pTag->is_end : 0 ),
                m_pSpoofData(0),
                m_pLoIcon(0)
            {

    if( pTag ){
        char *locked_buff;
                
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            char *p = locked_buff;
            if( m_originalTagType != P_ANCHOR 
                    && m_originalTagType != P_UNKNOWN ){

                while( *p == ' ' ) p++;
                char *pSpace = " ";
                if( *p == '>' ) pSpace = "";

                char *pTagData = PR_smprintf("%s%s%s", 
                        EDT_TagString(m_originalTagType), pSpace, p );
                SetTagData( pTagData );
                SetTagData( pTag, pTagData );
                m_originalTagType = P_UNKNOWN;
                free( pTagData );
            }
            else {
                SetTagData( locked_buff );
            }
        }
        PA_UNLOCK(pTag->data);
        SetSpoofData( pTag );
    }
}

CEditIconElement::~CEditIconElement(){
    DisconnectLayoutElements((LO_Element*) m_pLoIcon);
    if( m_pSpoofData ){
        free( m_pSpoofData );
    }
}

CEditIconElement::CEditIconElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer):
            CEditLeafElement(pStreamIn, pBuffer), 
            m_pLoIcon(0)
            {
    m_originalTagType = (TagType) pStreamIn->ReadInt();
    m_iconTag = pStreamIn->ReadInt();
    m_bEndTag = (XP_Bool) pStreamIn->ReadInt();
    m_pSpoofData = pStreamIn->ReadZString();
}

void CEditIconElement::SetLayoutElement( intn iEditOffset, intn lo_type, 
                        LO_Element* pLoElement ){
    SetLayoutElementHelper(LO_IMAGE, (LO_Element**) &m_pLoIcon,
        iEditOffset, lo_type, pLoElement);
}

void CEditIconElement::ResetLayoutElement( intn iEditOffset, 
            LO_Element* pLoElement ){
    ResetLayoutElementHelper((LO_Element**) &m_pLoIcon,
        iEditOffset, pLoElement);
}

void CEditIconElement::StreamOut( IStreamOut *pOut ){
    CEditLeafElement::StreamOut( pOut );
    pOut->WriteInt( m_originalTagType );
    pOut->WriteInt( m_iconTag );
    pOut->WriteInt( m_bEndTag );
    pOut->WriteZString( m_pSpoofData );
}

PA_Tag* CEditIconElement::TagOpen( int /* iEditOffset */ ){
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    SetTagData( pTag, m_pSpoofData );
    return pTag;
}

#define IS_WHITE(p) ((p) == ' ' || (p) == '\r' || (p) == '\n')

//
// Do some hueristic validation to make sure 
//
ED_TagValidateResult CEditIconElement::ValidateTag( char *pData, XP_Bool bNoBrackets ){
    char *p = pData;

    while( *p && IS_WHITE(*p) ){
        p++;
    }
    if( !bNoBrackets ){
        if( *p != '<' ){
            return ED_TAG_UNOPENED;
        }
        else {
            p++;
        }
        if( *p == '/' ){
            p++;
        }

        if( IS_WHITE(*p) ){
            return ED_TAG_TAGNAME_EXPECTED;
        }
    }

    // Is this a comment?
    XP_Bool isComment = p[0] == '!' && p[1] == '-' && p[2] == '-';

    if ( isComment ) {
        char* start = p;
        while( *p ){
            p++;
        }
        if ( ! bNoBrackets ) {
            --p;
        }

        XP_Bool endComment = (p >= start + 6) && p[-2] == '-' && p[-1] == '-';
        if ( ! endComment ) {
            return ED_TAG_PREMATURE_CLOSE; // ToDo: Use a specific error message
        }
    }
    else {
        // look for unterminated strings.
        while( *p && *p != '>' ){
            if( *p == '"' || *p == '\'' ){
                char quote = *p++;
                while( *p && *p != quote ){
                    p++;
                }
                if( *p == 0 ){
                    return ED_TAG_UNTERMINATED_STRING;
                }
            }
            p++;
        }
    }
    if( bNoBrackets ){
        if( *p == 0 ){
            return ED_TAG_OK;
        }
        else {
            XP_ASSERT( *p == '>' );
            return ED_TAG_PREMATURE_CLOSE;
        }
    }   
    else if( *p == '>' ){
        p++;
        while( IS_WHITE( *p ) ){
            p++;
        }
        if( *p != 0 ){
            return ED_TAG_PREMATURE_CLOSE;
        }
        else {
            return ED_TAG_OK;
        }
    }
    else {
        XP_ASSERT( *p == 0 );
        return ED_TAG_UNCLOSED;
    }
}

//
// Restore the original tag type so we save the right tag type when we output
//  we print.
//
void CEditIconElement::PrintOpen( CPrintState *pPrintState ){
    if( m_originalTagType != P_UNKNOWN ){
        TagType tSave = GetType();
        SetType( m_originalTagType );
        CEditLeafElement::PrintOpen( pPrintState );
        SetType( tSave );
    }
    else {
        if( m_bEndTag ){
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "</%s", 
                GetTagData());
        }
        else {
            pPrintState->m_iCharPos += pPrintState->m_pOut->Printf( "<%s", 
                GetTagData());
        }
    }
}

void CEditIconElement::PrintEnd( CPrintState *pPrintState ){
    if( m_originalTagType != P_UNKNOWN ){
        TagType tSave = GetType();
        SetType( m_originalTagType );
        CEditLeafElement::PrintEnd( pPrintState );
        SetType( tSave );
    }
}

char* CEditIconElement::GetData(){
    int iLen = XP_STRLEN( GetTagData() );
    char *p = (char*)XP_ALLOC( iLen+3 );
    char *pRet = p;
    *p++ = '<';
    if( m_bEndTag ){
        *p++ = '/';
    }
    XP_STRCPY( p, GetTagData() );
    return pRet;
}

XP_Bool IsHTMLCommentTag(char* string) {
    return string && string[0] == '!'
        && string[1] == '-'
        && string[2] == '-';
}

void CEditIconElement::SetSpoofData( PA_Tag* pTag ){
    
    if( m_iconTag == EDT_ICON_UNSUPPORTED_TAG 
            || m_iconTag == EDT_ICON_UNSUPPORTED_END_TAG ){

        char *pTagString;
        PA_LOCK(pTagString, char *, pTag->data );
    
        char *pData = PR_smprintf( ppIconTags[m_iconTag], 
                    edt_QuoteString( pTagString ));
        
        XP_Bool bDimensionFound = FALSE;
        XP_Bool bComment = m_originalTagType == P_UNKNOWN &&
            IsHTMLCommentTag(pTagString);

        if ( ! bComment ) {
            int32 height;
            int32 width;
            XP_Bool bPercent;
        
            edt_FetchDimension( pTag, PARAM_HEIGHT, &height, &bPercent, -1, TRUE );
            if( height != -1 ){
                pData = PR_sprintf_append( pData, " HEIGHT=%d%s", height, 
                            (bPercent ? "%" : "" ) );
                bDimensionFound = TRUE;
            }

            edt_FetchDimension( pTag, PARAM_WIDTH, &width, &bPercent, -1, TRUE );
            if( width != -1 ){
                pData = PR_sprintf_append( pData, " WIDTH=%d%s", width, 
                        (bPercent ? "%" : "" ) );
                bDimensionFound = TRUE;
            }
        }

        if( bDimensionFound ){
            pData = PR_sprintf_append(pData, " BORDER=2>");
        }
        else {
            pData = PR_sprintf_append(pData, " BORDER=0 ALIGN=ABSBOTTOM>");
        }

        if( m_pSpoofData ){
            free( m_pSpoofData );
        }
        m_pSpoofData = pData;
        PA_UNLOCK(pTag->data);
    }
    else {
        m_pSpoofData = XP_STRDUP( ppIconTags[m_iconTag] );
    }

}

void CEditIconElement::SetSpoofData( char* pData ){
    char *pNewData = PR_smprintf( ppIconTags[m_iconTag], 
                edt_QuoteString( pData ));
    if( m_pSpoofData ){
        XP_FREE( m_pSpoofData );
    }
    m_pSpoofData = pNewData;
}

void CEditIconElement::SetData( char *pData ){

    // If you assert here, you aren't validating your tag.
    XP_ASSERT( pData[0] == '<' );
    if( *pData != '<' )
        return;

    pData++;

    if( *pData == '/' ){
        pData++;
        m_bEndTag = TRUE;
    }
    SetTagData( pData );


    // Build up a tag that we can fetch parameter strings from.
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    SetTagData( pTag, pData );
    pTag->is_end = m_bEndTag;

    SetSpoofData( pTag );
    PA_FreeTag( pTag );
}

//
// We need to convert a tag handed to us by the parser into a tag we can
//  display in the editor.  In order to do this, we 'morph' the tag by swaping
//  the contents with desired tag.  We need an additional tag for swap space.
//  All this is done to insure that the memory location of 'pTag' doesn't 
//  change.  Perhaps this routine should be implemented at the CEditElement
//  layer.
//             
void CEditIconElement::MorphTag( PA_Tag *pTag ){
    PA_Tag *pNewTag = TagOpen(0);
    PA_Tag tempTag;

    tempTag = *pTag;
    *pTag = *pNewTag;
    *pNewTag = tempTag;

    PA_FreeTag( pNewTag );
}             
   
   
//-----------------------------------------------------------------------------
// CEditTargetElement
//-----------------------------------------------------------------------------
CEditTargetElement::CEditTargetElement(CEditElement *pParent, PA_Tag* pTag)
        :
            CEditIconElement( pParent, EDT_ICON_NAMED_ANCHOR,  pTag )
        {
        m_originalTagType = P_ANCHOR;
        SetData( GetData() );
}

CEditTargetElement::CEditTargetElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer)
        :
            CEditIconElement( pStreamIn, pBuffer )
        {
}

void CEditTargetElement::StreamOut( IStreamOut *pOut){
    CEditIconElement::StreamOut( pOut );
}

PA_Tag* CEditTargetElement::TagOpen( int iEditOffset ){
    // We need to create the actual anchor tag set.
    return CEditIconElement::TagOpen( iEditOffset );
}

void CEditTargetElement::SetData( char* pData ){
    char *pNew = 0;

    pNew = PR_sprintf_append( pNew, "NAME=%s>", edt_MakeParamString( pData ) );
    CEditIconElement::SetSpoofData( pData );        
    SetTagData( pNew );        
    free(pNew);
}

char* CEditTargetElement::GetData( ){
    // Get the actual tag data, not the faked up stuff at the Icon layer.
    PA_Tag *pTag = CEditLeafElement::TagOpen(0);
    char *pName = edt_FetchParamString( pTag, PARAM_NAME );
    PA_FreeTag( pTag );
    return pName;
}

//-----------------------------------------------------------------------------
// CEditBreakElement
//-----------------------------------------------------------------------------

CEditBreakElement::CEditBreakElement( CEditElement *pParent, PA_Tag* pTag ):
    CEditLeafElement( pParent, P_LINEBREAK ),
    m_pLoLinefeed(0)
{
    if( pTag ){
        char *locked_buff;
                
        PA_LOCK(locked_buff, char *, pTag->data );
        if( locked_buff ){
            SetTagData( locked_buff );
        }
        PA_UNLOCK(pTag->data);
    }
}

CEditBreakElement::CEditBreakElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer):
    CEditLeafElement(pStreamIn, pBuffer),
    m_pLoLinefeed(0)
{
}

CEditBreakElement::~CEditBreakElement(){
    DisconnectLayoutElements((LO_Element*) m_pLoLinefeed);
}


void CEditBreakElement::StreamOut( IStreamOut *pOut){
    CEditLeafElement::StreamOut( pOut );
}

EEditElementType CEditBreakElement::GetElementType(){
    return eBreakElement;
}

void CEditBreakElement::SetLayoutElement( intn iEditOffset, intn lo_type, 
                        LO_Element* pLoElement ){
    SetLayoutElementHelper(LO_LINEFEED, (LO_Element**) &m_pLoLinefeed,
        iEditOffset, lo_type, pLoElement);
}

void CEditBreakElement::ResetLayoutElement( intn iEditOffset, 
            LO_Element* pLoElement ){
    ResetLayoutElementHelper((LO_Element**) &m_pLoLinefeed,
        iEditOffset, pLoElement);
}

LO_Element* CEditBreakElement::GetLayoutElement(){
    return (LO_Element*)m_pLoLinefeed;
}

XP_Bool CEditBreakElement::GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool /* bStickyAfter */,
            LO_Element*& pRetElement, 
            int& pLayoutOffset ){ 
    pLayoutOffset = 0; 
    pRetElement = (LO_Element*)m_pLoLinefeed;
    pLayoutOffset = iEditOffset;
    return TRUE;
}

//
// if we are within PREFORMAT (or the like) breaks are returns.
//
void CEditBreakElement::PrintOpen( CPrintState *ps ){
    CEditElement *pPrev = PreviousLeafInContainer();
    while( pPrev && pPrev->IsA(P_LINEBREAK) ){
        pPrev = pPrev->PreviousLeafInContainer();
    }
    CEditElement *pNext = LeafInContainerAfter();
    while( pNext && pNext->IsA(P_LINEBREAK) ){
        pNext = pNext->LeafInContainerAfter();
    }
    XP_Bool bScriptBefore = pPrev 
                && pPrev->IsA(P_TEXT) 
                && (pPrev->Text()->m_tf & (TF_SERVER|TF_SCRIPT));
    XP_Bool bScriptAfter = pNext && pNext->IsA(P_TEXT) && (pNext->Text()->m_tf & (TF_SERVER|TF_SCRIPT));
    
    if ( !bScriptBefore && bScriptAfter ){
        // Don't print <BR>s before <SCRIPT> tags. This swallows both legal <BR> tags,
        // and also swallows '\n' at the start of scripts. If we didn't do this,
        // then we would turn <SCRIPT>\nFoo into <BR><SCRIPT>Foo.
        // The case of a legitimate <BR> before a <SCRIPT> tag is much rarer than the
        // case of a \n just after the <SCRIPT> tag. So we err on the side that lets
        // more pages work.
        // Take this code out when text styles are extended to all leaf elements.
        return;
    }
    if( InFormattedText() 
            || bScriptBefore ){
        ps->m_pOut->Write( "\n", 1 );
        ps->m_iCharPos = 0;
    }
    else {
        CEditLeafElement::PrintOpen( ps );
    }
}

#endif

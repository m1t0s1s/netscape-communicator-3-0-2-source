/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Editor save stuff. LWT 06/01/95
// this should be on the branch
//

#ifdef EDITOR

#ifndef _EDITOR_H
#define _EDITOR_H
#include "edttypep.h"       // include first so types are mapped.

//
//    Many of the system header files have C++ specific stuff in
//    them. If we just widely extern "C" {} the lot, we will lose.
//    So.. pre-include some that must be seen by C++.
//
#if defined(HPUX) || defined(SCO_SV)
#include "math.h"
#endif /*HPUX losing headers */

extern "C" {
#include "net.h"
//#include "glhist.h"
//#include "shist.h"
#include "merrors.h"
#include "xp.h"
#include "il.h"
#include "pa_parse.h"
#include "layout.h"
#include "bits.h"
#include "edt.h"
#include "pa_tags.h"
#include "pa_amp.h"
#include "xpassert.h"
#include "xp_file.h"
#include "libmocha.h"
#include "libi18n.h"
}

#include "garray.h"

// TRY / CATCH / RETHROW macros for minimal error recovery on macs
#if defined(XP_MAC)
#define EDT_TRY try
#define EDT_CATCH_ALL catch(...)
#define EDT_RETHROW throw
#else
#define EDT_TRY if(1)
#define EDT_CATCH_ALL else
#define EDT_RETHROW

#endif

#if defined(XP_MAC) || defined(XP_UNIX)
static void DebugBreak() {}
#endif

//#define NON_BREAKING_SPACE  160
//
#define NON_BREAKING_SPACE_STRING   "\xa0"

#define DEF_FONTSIZE    3
#define MAX_FONTSIZE    7
#define DEFAULT_HR_THICKNESS    2

#define EDT_IS_LIVEWIRE_MACRO(s) ( (s) && (s)[0] == '`' )

/* If SUPPORT_MULTICOLUMN is defined, the <MULTICOL> tag is recognized when
 * parsing HTML. Othewise it's treated as an unknown tag.
 */

/* #define SUPPORT_MULTICOLUMN */

//-----------------------------------------------------------------------------
// Edit Classes
//-----------------------------------------------------------------------------
class CEditElement;
class CEditLeafElement;
class CEditSubDocElement;
class CEditTableElement;
class CEditTableRowElement;
class CEditCaptionElement;
class CEditTableCellElement;
class CEditRootDocElement;
class CEditContainerElement;
class CEditListElement;
class CEditTextElement; 
class CEditImageElement;
class CEditHorizRuleElement;
class CEditIconElement;
class CEditEndElement;
class CEditBreakElement;
class CEditTargetElement;
class CEditMultiColumnElement;

class CParseState;
class CPrintState;
class IStreamOut;
class IStreamIn;
class CEditImageLoader;
class CEditSaveObject;
class CFileSaveObject;
struct ED_Link;
class CEditCommand;
class CEditCommandGroup;
class CPasteCommand;
class CStreamOutMemory;

#ifdef DEBUG
class CEditTestManager;
#endif

// Originally this was a sugared int32, but we ran into
// trouble working with big-endian architectures, like the
// Macintosh. So rather than being even cleverer with
// macros, I decided to make it a regular class.
// If this turns out to be a performance bottleneck
// we can always inline it.... jhp
class ED_Color {
public:
    ED_Color();
    ED_Color(LO_Color& pLoColor);
    ED_Color(int32 rgb);
    ED_Color(int r, int g, int b);
    ED_Color(LO_Color* pLoColor);
    XP_Bool IsDefined();
    XP_Bool operator==(const ED_Color& other) const;
    XP_Bool operator!=(const ED_Color& other) const;
    int Red();
    int Green();
    int Blue();
    LO_Color GetLOColor();
    long GetAsLong(); // 0xRRGGBB
	void SetUndefined();
	static ED_Color GetUndefined();
private:
    LO_Color m_color;
    XP_Bool m_bDefined;
};

// Copies the argument into a static buffer, and then upper-cases the
// buffer, and returns a pointer to the buffer. You don't own the
// result pointer, and you can't call twice in same line of code.

char* EDT_UpperCase(char*);

// Returns a string for the given alignment. Uses a static buffer, so
// don't call twice in the same line of code.

char* EDT_AlignString(ED_Alignment align);

// Backwards-compatable macros. Should clean this up some day.

#define EDT_COLOR_DEFAULT    (ED_Color())
#define EDT_RED(x)      ((x).Red())
#define EDT_GREEN(x)    ((x).Green())
#define EDT_BLUE(x)     ((x).Blue())
#define EDT_RGB(r,g,b)  (ED_Color(r,g,b)) 
#define EDT_LO_COLOR(pLoColor)  ED_Color(pLoColor)

// Editor selections

// template declarations
Declare_GrowableArray(int32,int32)                      // TXP_GrowableArray_int32
Declare_GrowableArray(ED_Link,ED_Link*)                 // TXP_GrowableArray_ED_Link
Declare_GrowableArray(EDT_MetaData,EDT_MetaData*)       // TXP_GrowableArray_EDT_MetaData
Declare_GrowableArray(pChar,char*)                      // TXP_GrowableArray_pChar
Declare_GrowableArray(CEditCommand,CEditCommand*)       // TXP_GrowableArray_CEditCommand

Declare_PtrStack(ED_Alignment,ED_Alignment)             // TXP_PtrStack_ED_Alignment
Declare_PtrStack(TagType,TagType)                       // TXP_PtrStack_TagType
Declare_PtrStack(CEditTextElement,CEditTextElement*)    // TXP_PtrStack_CEditTextElement
Declare_PtrStack(ED_TextFormat,int32)                   // TXP_PtrStack_ED_TextFormat
Declare_PtrStack(CEditCommandGroup,CEditCommandGroup*)  // TXP_PtrStack_CEditCommandGroup
Declare_PtrStack(CParseState,CParseState*)              // TXP_PtrStack_CParseState

#ifdef DEBUG
    #define DEBUG_EDIT_LAYOUT
#endif

typedef int32 ElementIndex; // For persistent insert points, a position from the start of the document.
typedef int32 ElementOffset; // Within one element, an offset into an element. Is 0 or 1, except for text.

class CEditInsertPoint
{
public:
    CEditInsertPoint();
    CEditInsertPoint(CEditElement* pElement, ElementOffset iPos);
    CEditInsertPoint(CEditLeafElement* pElement, ElementOffset iPos);
    CEditInsertPoint(CEditElement* pElement, ElementOffset iPos, XP_Bool bStickyAfter);

    XP_Bool IsStartOfElement();
    XP_Bool IsEndOfElement();
    XP_Bool IsStartOfContainer();
    XP_Bool IsEndOfContainer();
    XP_Bool IsStartOfDocument();
    XP_Bool IsEndOfDocument();

    XP_Bool GapWithBothSidesAllowed();
    XP_Bool IsLineBreak();
    XP_Bool IsSoftLineBreak();
    XP_Bool IsHardLineBreak();
    XP_Bool IsSpace(); // After insert point
    XP_Bool IsSpaceBeforeOrAfter(); // Before or after insert point

    // Move forward and backward. Returns FALSE if can't move
    CEditInsertPoint NextPosition();
    CEditInsertPoint PreviousPosition();

    XP_Bool operator==(const CEditInsertPoint& other );
    XP_Bool operator!=(const CEditInsertPoint& other );
    XP_Bool operator<(const CEditInsertPoint& other );
    XP_Bool operator<=(const CEditInsertPoint& other );
    XP_Bool operator>=(const CEditInsertPoint& other );
    XP_Bool operator>(const CEditInsertPoint& other );
    XP_Bool IsDenormalizedVersionOf(const CEditInsertPoint& other);
    intn Compare(const CEditInsertPoint& other );

#ifdef DEBUG
    void Print(IStreamOut& stream);
#endif

    CEditLeafElement* m_pElement;
    ElementOffset m_iPos;
    XP_Bool m_bStickyAfter;

private:
    CEditElement* FindNonEmptyElement( CEditElement *pStartElement );
};


class CEditSelection
{
public:
    CEditSelection();
    CEditSelection(CEditElement* pStart, intn iStartPos, CEditElement* pEnd, intn iEndPos, XP_Bool fromStart = FALSE);
    CEditSelection(const CEditInsertPoint& start, const CEditInsertPoint& end, XP_Bool fromStart = FALSE);

    XP_Bool operator==(const CEditSelection& other );
    XP_Bool operator!=(const CEditSelection& other );
    XP_Bool IsInsertPoint();
    CEditInsertPoint* GetActiveEdge();
    CEditInsertPoint* GetAnchorEdge();
    CEditInsertPoint* GetEdge(XP_Bool bEnd);

    // Like ==, except ignores bFromStart.
    XP_Bool EqualRange(CEditSelection& other);
    XP_Bool Intersects(CEditSelection& other); // Just a test; doesn't change this.
    XP_Bool Contains(CEditInsertPoint& point);
    XP_Bool Contains(CEditSelection& other);
    XP_Bool ContainsStart(CEditSelection& other);
    XP_Bool ContainsEnd(CEditSelection& other);
    XP_Bool ContainsEdge(CEditSelection& other, XP_Bool bEnd);
    XP_Bool CrossesOver(CEditSelection& other);

    XP_Bool ClipTo(CEditSelection& other); // True if the result is defined. No change to this if it isn't

    CEditElement* GetCommonAncestor();
    XP_Bool CrossesSubDocBoundary();
    CEditSubDocElement* GetEffectiveSubDoc(XP_Bool bEnd);

    XP_Bool ExpandToNotCrossStructures(); // TRUE if this was changed
    void ExpandToBeCutable();
    void ExpandToIncludeFragileSpaces();
    void ExpandToEncloseWholeContainers();
    XP_Bool EndsAtStartOfContainer();
    XP_Bool StartsAtEndOfContainer();
    XP_Bool AnyLeavesSelected(); // FALSE if insert point or container end.
    XP_Bool IsContainerEnd(); // TRUE if just contains the end of a container.
    // Useful for cut & delete, where you don't replace the final container mark
    XP_Bool ContainsLastDocumentContainerEnd();
    void ExcludeLastDocumentContainerEnd();

    CEditContainerElement* GetStartContainer();
    CEditContainerElement* GetClosedEndContainer(); // Not 1/2 open!

#ifdef DEBUG
    void Print(IStreamOut& stream);
#endif

    CEditInsertPoint m_start;
    CEditInsertPoint m_end;
    XP_Bool m_bFromStart;
};


class CPersistentEditInsertPoint
{
public:
    // m_bStickyAfter defaults to TRUE
    CPersistentEditInsertPoint();
    CPersistentEditInsertPoint(ElementIndex index);
    CPersistentEditInsertPoint(ElementIndex index, XP_Bool bStickyAfter);
#ifdef DEBUG
    void Print(IStreamOut& stream);
#endif
    XP_Bool operator==(const CPersistentEditInsertPoint& other );
    XP_Bool operator!=(const CPersistentEditInsertPoint& other );
    XP_Bool operator<(const CPersistentEditInsertPoint& other );
    XP_Bool operator<=(const CPersistentEditInsertPoint& other );
    XP_Bool operator>=(const CPersistentEditInsertPoint& other );
    XP_Bool operator>(const CPersistentEditInsertPoint& other );

    XP_Bool IsEqualUI(const CPersistentEditInsertPoint& other );

    // Used for undo
    // delta = this - other;
    void ComputeDifference(CPersistentEditInsertPoint& other, CPersistentEditInsertPoint& delta);
    // result = this + delta;
    void AddRelative(CPersistentEditInsertPoint& delta, CPersistentEditInsertPoint& result);

    ElementIndex m_index;
    // If this insert point falls between two elements, and
    // m_bStickyAfter is TRUE, then this insert point is attached
    // to the start of the second element. If m_bStickyAfter
    // is false, then this insert point is attached to the end of
    // the first element. It is FALSE by default. It is ignored
    // by all of the comparison operators except IsEqualUI. It
    // is ignored by ComputeDifference and AddRelative

     XP_Bool m_bStickyAfter;
};

#ifdef DEBUG
    #define DUMP_PERSISTENT_EDIT_INSERT_POINT(s, pt) { XP_TRACE(("%s %d", s, pt.m_index));}
#else
    #define DUMP_PERSISTENT_EDIT_INSERT_POINT(s, pt) {}
#endif

class CPersistentEditSelection
{
public:
    CPersistentEditSelection();
    CPersistentEditSelection(const CPersistentEditInsertPoint& start, const CPersistentEditInsertPoint& end);
#ifdef DEBUG
    void Print(IStreamOut& stream);
#endif
    XP_Bool operator==(const CPersistentEditSelection& other );
    XP_Bool operator!=(const CPersistentEditSelection& other );
    XP_Bool SelectedRangeEqual(const CPersistentEditSelection& other ); // Ignores m_bFromStart

    ElementIndex GetCount();
    XP_Bool IsInsertPoint();
    CPersistentEditInsertPoint* GetEdge(XP_Bool bEnd);

    // Used for undo
    // change this by the same way that original was changed into modified.
    void Map(CPersistentEditSelection& original, CPersistentEditSelection& modified);

    CPersistentEditInsertPoint m_start;
    CPersistentEditInsertPoint m_end;
    XP_Bool m_bFromStart;
};

#ifdef DEBUG
    #define DUMP_PERSISTENT_EDIT_SELECTION(s, sel) { XP_TRACE(("%s %d-%d %d", s,\
        sel.m_start.m_index,\
        sel.m_end.m_index,\
        sel.m_bFromStart));}
#else
    #define DUMP_PERSISTENT_EDIT_SELECTION(s, sel) {}
#endif

//
// Element type used during stream constructor
//
enum EEditElementType {
    eElementNone,
    eElement,
    eTextElement,
    eImageElement,
    eHorizRuleElement,
    eBreakElement,
    eContainerElement,
    eIconElement,
    eTargetElement,
    eListElement,
    eEndElement,
    eTableElement,
    eTableRowElement,
    eTableCellElement,
    eCaptionElement,
    eMultiColumnElement
};

//
// CEditElement
//
class CEditElement {
private:
    TagType m_tagType;
    CEditElement* m_pParent;
    CEditElement* m_pNext;      
    CEditElement* m_pChild;
    char *m_pTagData;               // hack! remainder of tag data for regen.

protected:
    char* GetTagData(){ return m_pTagData; };
    void SetTagData(char* tagData);

public:

    // ctor, dtor
    CEditElement(CEditElement *pParent, TagType tagType, char* pData = NULL);
    CEditElement(CEditElement *pParent, PA_Tag *pTag);
    CEditElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);
    virtual ~CEditElement();

    static CEditElement* StreamCtor( IStreamIn *pIn, CEditBuffer *pBuffer );
    static CEditElement* StreamCtorNoChildren( IStreamIn *pIn, CEditBuffer *pBuffer );
    virtual void StreamInChildren(IStreamIn* pIn, CEditBuffer* pBuffer);

    //
    // Accessor functions
    //
    CEditElement* GetParent(){ return m_pParent; }
    CEditElement* GetNextSibling(){ return m_pNext; }
    CEditElement* GetPreviousSibling();
    CEditElement* GetChild(){ return m_pChild; }
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild) {return TRUE;}
    void SetChild(CEditElement *pChild);
    void SetNextSibling(CEditElement* pNext);

    CEditElement* GetLastChild();
    CEditElement* GetLastMostChild();
    CEditElement* GetFirstMostChild();

    CEditCaptionElement* GetCaption(); // Returns containing cell, or NULL if none.
    CEditCaptionElement* GetCaptionIgnoreSubdoc(); // Returns containing cell, or NULL if none.
    CEditTableCellElement* GetTableCell(); // Returns containing cell, or NULL if none.
    CEditTableCellElement* GetTableCellIgnoreSubdoc(); // Returns containing cell, or NULL if none.
    CEditTableRowElement* GetTableRow(); // Returns containing row, or NULL if none.
    CEditTableRowElement* GetTableRowIgnoreSubdoc(); // Returns containing row, or NULL if none.
    CEditTableElement* GetTable(); // Returns containing table, or NULL if none.
    CEditTableElement* GetTableIgnoreSubdoc(); // Returns containing table, or NULL if none.
    CEditMultiColumnElement* GetMultiColumn(); // Returns containing multicolumn, or NULL if none.
    CEditMultiColumnElement* GetMultiColumnIgnoreSubdoc(); // Returns containing multicolumn, or NULL if none.
    CEditSubDocElement* GetSubDoc(); // Returns containing sub-doc, or NULL if none.
    CEditSubDocElement* GetSubDocSkipRoot(); // Returns containing sub-doc, or NULL if none.
    CEditRootDocElement* GetRootDoc(); // Returns containing root, or NULL if none.

    CEditElement* GetTopmostTableOrMultiColumn(); // Returns topmost containing table or MultiColumn, or NULL if none.
    CEditElement* GetTableOrMultiColumnIgnoreSubdoc(); // Returns containing table or MultiColumn, or NULL if none.
    CEditElement* GetSubDocOrMultiColumnSkipRoot();

    CEditBuffer* GetEditBuffer(); // returns the edit buffer if the element is in one.

    virtual ED_Alignment GetDefaultAlignment();
    virtual ED_Alignment GetDefaultVAlignment();

    TagType GetType(){ return m_tagType; }
    void SetType( TagType t ){ m_tagType = t; }

    //
    // Cast routines.
    //
    CEditTextElement* Text(){ 
        XP_ASSERT(IsText());
        return (CEditTextElement*)this;
    }

    virtual XP_Bool IsLeaf() { return FALSE; }
    CEditLeafElement* Leaf(){ XP_ASSERT(IsLeaf());
        return (CEditLeafElement*)this;
    }

    virtual XP_Bool IsRoot() { return FALSE; }
    CEditRootDocElement* Root(){ XP_ASSERT(IsRoot());
        return (CEditRootDocElement*)this;
    }

    virtual XP_Bool IsContainer() { return FALSE; }
    CEditContainerElement* Container(){ XP_ASSERT(IsContainer());
        return (CEditContainerElement*)this;
    }

    virtual XP_Bool IsList() { return FALSE; }
    CEditListElement* List(){ XP_ASSERT(IsList());
        return (CEditListElement*)this;
    }

    virtual XP_Bool IsBreak() { return FALSE; }
    CEditBreakElement* Break(){ XP_ASSERT(IsBreak());
        return (CEditBreakElement*)this;
    }

    virtual XP_Bool CausesBreakBefore() { return FALSE;}
    virtual XP_Bool CausesBreakAfter() { return FALSE;}
    virtual XP_Bool AllowBothSidesOfGap() { return FALSE; }

    virtual XP_Bool IsImage() { return FALSE; }
    CEditImageElement* Image(){
        // Not all P_IMAGE elements are images.  CEditIconElements have P_IMAGE 
        //  as their tagType but are not CEditImageElements.
        XP_ASSERT(m_tagType==P_IMAGE 
            && GetElementType() == eImageElement 
            && IsImage() ) ;
        return (CEditImageElement*)this;
    }

    virtual XP_Bool IsIcon() { return FALSE; }
    CEditIconElement* Icon(){ XP_ASSERT(IsIcon());
        return (CEditIconElement*)this;
    }

    CEditTargetElement* Target(){ XP_ASSERT(GetElementType() == eTargetElement);
        return (CEditTargetElement*)this;
    }

    CEditHorizRuleElement* HorizRule(){ XP_ASSERT(m_tagType==P_HRULE); 
        return (CEditHorizRuleElement*)this;
    }

    virtual XP_Bool IsRootDoc() { return FALSE; }
    CEditRootDocElement* RootDoc() { XP_ASSERT(IsRootDoc()); return (CEditRootDocElement*) this; }
    virtual XP_Bool IsSubDoc() { return FALSE; }
    CEditSubDocElement* SubDoc() { XP_ASSERT(IsSubDoc()); return (CEditSubDocElement*) this; }
    virtual XP_Bool IsTable() { return FALSE; }
    CEditTableElement* Table() { XP_ASSERT(IsTable()); return (CEditTableElement*) this; }
    virtual XP_Bool IsTableRow() { return FALSE; }
    CEditTableRowElement* TableRow() { XP_ASSERT(IsTableRow()); return (CEditTableRowElement*) this; }
    virtual XP_Bool IsTableCell() { return FALSE; }
    CEditTableCellElement* TableCell() { XP_ASSERT(IsTableCell()); return (CEditTableCellElement*) this; }
    virtual XP_Bool IsCaption() { return FALSE; }
    CEditCaptionElement* Caption() { XP_ASSERT(IsCaption()); return (CEditCaptionElement*) this; }
    virtual XP_Bool IsText() { return FALSE; }
    virtual XP_Bool IsMultiColumn() { return FALSE; }
    CEditMultiColumnElement* MultiColumn() { XP_ASSERT(IsMultiColumn()); return (CEditMultiColumnElement*) this; }

    XP_Bool IsEndOfDocument() { return GetElementType() == eEndElement; }
    virtual XP_Bool IsEndContainer() { return FALSE; }

    // Can this element contain a paragraph?
    virtual XP_Bool IsContainerContainer();

    XP_Bool IsA( int tagType ){ return m_tagType == tagType; }
    XP_Bool Within( int tagType );
    XP_Bool InFormattedText();
    CEditContainerElement* FindContainer(); // Returns this if this is a container.
    CEditContainerElement* FindEnclosingContainer(); // Starts search with parent of this.
    CEditElement* FindList();
    void FindList( CEditContainerElement*& pContainer, CEditListElement*& pList );
    CEditElement* FindContainerContainer();

    //
    // Tag Generation functions
    //
    virtual PA_Tag* TagOpen( int iEditOffset );
    virtual PA_Tag* TagEnd( );
    void SetTagData( PA_Tag* pTag, char* pTagData);

    //
    // Output routines.
    //
    virtual void PrintOpen( CPrintState *pPrintState );
    void InternalPrintOpen( CPrintState *pPrintState, char* pTagData );
    virtual void PrintEnd( CPrintState *pPrintState );
    virtual void StreamOut( IStreamOut *pOut );
    virtual void PartialStreamOut( IStreamOut *pOut, CEditSelection& selection );
    virtual XP_Bool ShouldStreamSelf( CEditSelection& local, CEditSelection& selection);

    virtual EEditElementType GetElementType();
    virtual XP_Bool Reduce( CEditBuffer *pBuffer );

    // Selection utilities
    XP_Bool ClipToMe(CEditSelection& selection, CEditSelection& local); // Returns TRUE if selection intersects with "this".
    virtual void GetAll(CEditSelection& selection); // Get selection that selects all of this.

    // iterators
    typedef XP_Bool (CEditElement::*PMF_EditElementTest)(void*);

    CEditElement* FindNextElement( PMF_EditElementTest pmf, void *pData );
    CEditElement* UpRight( PMF_EditElementTest pmf, void *pTestData );
    CEditElement* DownRight( PMF_EditElementTest pmf,void *pTestData, 
            XP_Bool bIgnoreThis = FALSE );

    CEditElement* FindPreviousElement( PMF_EditElementTest pmf, void *pTestData );
    CEditElement* UpLeft( PMF_EditElementTest pmf, void *pTestData );
    CEditElement* DownLeft( PMF_EditElementTest pmf, void *pTestData, 
            XP_Bool bIgnoreThis = FALSE );

    XP_Bool TestIsTrue( PMF_EditElementTest pmf, void *pTestData ){ 
                return (this->*pmf)(pTestData); }

    //
    // Search routines to be applied FindNextElement and FindPreviousElement
    //
    XP_Bool FindText( void* /*pVoid*/ );
    XP_Bool FindTextAll( void* /*pVoid*/ );
    XP_Bool FindLeaf( void* /*pVoid*/ );
    XP_Bool FindLeafAll( void* /*pVoid*/ );
    XP_Bool FindImage( void* /*pvoid*/ );
    XP_Bool FindTarget( void* /*pvoid*/ );
    XP_Bool SplitContainerTest( void* /*pVoid*/ );
    XP_Bool SplitFormattingTest( void* pVoid );

    XP_Bool FindContainer( void* /*pVoid*/ );

    //
    // Split the tree.  You can stop when pmf returns true.
    // 
    virtual CEditElement* Split(CEditElement *pLastChild, 
                                CEditElement* pCloneTree,
                                PMF_EditElementTest pmf,
                                void *pData);

    virtual CEditElement* Clone( CEditElement *pParent = 0 );
    CEditElement* CloneFormatting( TagType endType );
    virtual void Unlink();
    virtual void Merge( CEditElement *pAppendBlock, XP_Bool bCopyAppendAttributes = TRUE );
    

#ifdef DEBUG
    virtual void ValidateTree();
#endif
    CEditElement* InsertAfter( CEditElement *pPrev );
    CEditElement* InsertBefore( CEditElement *pNext );
    void InsertAsFirstChild( CEditElement *pParent );
    void InsertAsLastChild( CEditElement *pParent );

    XP_Bool IsFirstInContainer();
    CEditTextElement* TextInContainerAfter();
    CEditTextElement* PreviousTextInContainer();
    virtual CEditElement* Divide(int iOffset);

    CEditLeafElement* PreviousLeafInContainer();
    CEditLeafElement* LeafInContainerAfter();

    CEditLeafElement* PreviousLeaf() { return (CEditLeafElement*) FindPreviousElement( &CEditElement::FindLeaf, 0 ); }
    CEditLeafElement* NextLeaf() { return (CEditLeafElement*) FindNextElement( &CEditElement::FindLeaf, 0 ); }
    CEditLeafElement* NextLeafAll() { return (CEditLeafElement*) FindNextElement( &CEditElement::FindLeafAll, 0 ); }
    CEditContainerElement* NextContainer() { return (CEditContainerElement*) FindNextElement( &CEditElement::FindContainer, 0 ); }
    CEditContainerElement* PreviousContainer() { return (CEditContainerElement*) FindPreviousElement( &CEditElement::FindContainer, 0 ); }

    CEditElement* GetRoot();
    CEditElement* GetCommonAncestor(CEditElement* pOther); // NULL if not in same tree
    CEditElement* GetChildContaining(CEditElement* pDescendant); // NULL if not a descendant

    XP_Bool DeleteElement( CEditElement *pTellIfKilled );
    void DeleteChildren();

    // should be in Container class
    int GetDefaultFontSize();

    // To and from persistent insert points
    virtual CEditInsertPoint IndexToInsertPoint(ElementIndex index, XP_Bool bStickyAfter);
    virtual CPersistentEditInsertPoint GetPersistentInsertPoint(ElementOffset offset);
    virtual ElementIndex GetElementIndexOf(CEditElement* child); // asserts if argument is not a child.
    virtual ElementIndex GetPersistentCount();
    ElementIndex GetElementIndex();

    virtual void FinishedLoad( CEditBuffer* pBuffer );

protected:
    virtual void Finalize();
    void EnsureSelectableSiblings(CEditBuffer* pBuffer);

private:
    void CommonConstructor();
    // For CEditElement constructor, where the vtable isn't
    // set up enough to check if the item is acceptable.
    void RawSetNextSibling(CEditElement* pNext) {m_pNext = pNext; }
    void RawSetChild(CEditElement* pChild) {m_pChild = pChild; }
};


//
// CEditSubDocElement - base class for the root, captions, and table data cells
//
class CEditSubDocElement: public CEditElement {
public:
    CEditSubDocElement(CEditElement *pParent, int tagType, char* pData = NULL);
    CEditSubDocElement(CEditElement *pParent, PA_Tag *pTag);
    CEditSubDocElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsSubDoc() { return TRUE; }
    static CEditSubDocElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsSubDoc() ? (CEditSubDocElement*) pElement : 0; }

    virtual XP_Bool Reduce( CEditBuffer *pBuffer );
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild){
            return ! pChild.IsLeaf();
        }
    virtual void FinishedLoad( CEditBuffer* pBuffer );
};

class CEditTableElement: public CEditElement {
public:
    CEditTableElement(intn columns, intn rows);
    CEditTableElement(CEditElement *pParent, PA_Tag *pTag, ED_Alignment align);
    CEditTableElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsTable() { return TRUE; }
    static CEditTableElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsTable() ? (CEditTableElement*) pElement : 0; }
    virtual EEditElementType GetElementType() { return eTableElement; }
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild){
            return pChild.IsTableRow() || pChild.IsCaption();
        }
    virtual void FinishedLoad( CEditBuffer* pBuffer );
    void AdjustCaption();
    virtual PA_Tag* TagOpen(int iEditOffset);
    PA_Tag* InternalTagOpen(int iEditOffset, XP_Bool bPrinting);
    virtual PA_Tag* TagEnd();

    intn GetRows();
    intn GetColumns();
    void InsertRows(intn iPosition, intn number, CEditTableElement* pSource = NULL);
    void InsertColumns(intn iPosition, intn number, CEditTableElement* pSource = NULL);
    void DeleteRows(intn iPosition, intn number, CEditTableElement* pUndoContainer = NULL);
    void DeleteColumns(intn iPosition, intn number, CEditTableElement* pUndoContainer = NULL);
    CEditTableRowElement* FindRow(intn number);
    CEditTableCellElement* FindCell(intn column, intn row);
    CEditCaptionElement* GetCaption();
    void SetCaption(CEditCaptionElement*);
    void DeleteCaption();

    void SetData( EDT_TableData *pData );
    EDT_TableData* GetData( );
    EDT_TableData* ParseParams( PA_Tag *pTag );
    static EDT_TableData* NewData();
    static void FreeData( EDT_TableData *pData );

    void SetBackgroundColor( ED_Color iColor );
    ED_Color GetBackgroundColor();

    // Override printing and taging, since we don't allow ALIGN tag when editing.
    virtual void PrintOpen( CPrintState *pPrintState );
    virtual void PrintEnd( CPrintState *pPrintState );
    char* CreateTagData(EDT_TableData *pData, XP_Bool bPrinting);

    virtual void StreamOut( IStreamOut *pOut);

private:
    ED_Color m_backgroundColor;
    ED_Alignment m_align;
    ED_Alignment m_malign;
};

class CEditTableRowElement: public CEditElement {
public:
    CEditTableRowElement();
    CEditTableRowElement(intn cells);
    CEditTableRowElement(CEditElement *pParent, PA_Tag *pTag);
    CEditTableRowElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsTableRow() { return TRUE; }
    static CEditTableRowElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsTableRow() ? (CEditTableRowElement*) pElement : 0; }
    virtual EEditElementType GetElementType() { return eTableRowElement; }
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild){
            return pChild.IsTableCell();
        }
    virtual void FinishedLoad( CEditBuffer* pBuffer );

    intn GetRowIndex();

    intn GetCells();
    void InsertCells(intn iPosition, intn number, CEditTableRowElement* pSource = NULL);
    void DeleteCells(intn iPosition, intn number, CEditTableRowElement* pUndoContainer = NULL);
    CEditTableCellElement* FindCell(intn number);

    void SetData( EDT_TableRowData *pData );
    EDT_TableRowData* GetData( );
    EDT_TableRowData* ParseParams( PA_Tag *pTag );
    static EDT_TableRowData* NewData();
    static void FreeData( EDT_TableRowData *pData );
    void SetBackgroundColor( ED_Color iColor );
    ED_Color GetBackgroundColor();
private:
    ED_Color m_backgroundColor;
};

class CEditCaptionElement: public CEditSubDocElement {
public:
    CEditCaptionElement();
    CEditCaptionElement(CEditElement *pParent, PA_Tag *pTag);
    CEditCaptionElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsCaption() { return TRUE; }
    static CEditCaptionElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsCaption() ? (CEditCaptionElement*) pElement : 0; }
    virtual EEditElementType GetElementType() { return eCaptionElement; }

    void SetData( EDT_TableCaptionData *pData );
    EDT_TableCaptionData* GetData( );
    EDT_TableCaptionData* ParseParams( PA_Tag *pTag );
    static EDT_TableCaptionData* NewData();
    static void FreeData( EDT_TableCaptionData *pData );
};

// Table data and Table header

class CEditTableCellElement: public CEditSubDocElement {
public:
    CEditTableCellElement();
    CEditTableCellElement(XP_Bool bIsHeader);
    CEditTableCellElement(CEditElement *pParent, PA_Tag *pTag);
    CEditTableCellElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsTableCell();
    static CEditTableCellElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsTableCell() ? (CEditTableCellElement*) pElement : 0; }
    virtual EEditElementType GetElementType();
    virtual ED_Alignment GetDefaultAlignment();

    XP_Bool IsTableData();

    intn GetRowIndex();
    intn GetColumnIndex();

    void SetData( EDT_TableCellData *pData );
    EDT_TableCellData* GetData( );
    EDT_TableCellData* ParseParams( PA_Tag *pTag );
    static EDT_TableCellData* NewData();
    static void FreeData( EDT_TableCellData *pData );
    void SetBackgroundColor( ED_Color iColor );
    ED_Color GetBackgroundColor();
private:
    ED_Color m_backgroundColor;
};

class CEditMultiColumnElement: public CEditElement {
public:
    CEditMultiColumnElement(intn columns);
    CEditMultiColumnElement(CEditElement *pParent, PA_Tag *pTag);
    CEditMultiColumnElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    virtual XP_Bool IsMultiColumn() { return TRUE; }
    static CEditMultiColumnElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsMultiColumn() ? (CEditMultiColumnElement*) pElement : 0; }
    virtual EEditElementType GetElementType() { return eMultiColumnElement; }
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild){
            return pChild.IsContainer();
        }
    virtual void FinishedLoad( CEditBuffer* pBuffer );

    void SetData( EDT_MultiColumnData *pData );
    EDT_MultiColumnData* GetData( );
    EDT_MultiColumnData* ParseParams( PA_Tag *pTag );
    static EDT_MultiColumnData* NewData();
    static void FreeData( EDT_MultiColumnData *pData );

private:
};

//
// CEditRootDocElement - Top most element in the tree, tag HTML
//
class CEditRootDocElement: public CEditSubDocElement {
private:
    CEditBuffer* m_pBuffer;
public:
    CEditRootDocElement( CEditBuffer* pBuffer ): 
            CEditSubDocElement(0, P_MAX), m_pBuffer( pBuffer ){}
    virtual XP_Bool Reduce( CEditBuffer *pBuffer ){ return FALSE; }
    virtual XP_Bool ShouldStreamSelf( CEditSelection& local, CEditSelection& selection) { return FALSE; }

    virtual void FinishedLoad( CEditBuffer *pBuffer );

    virtual void PrintOpen( CPrintState *pPrintState );
    virtual void PrintEnd( CPrintState *pPrintState );

    virtual XP_Bool IsRoot() { return TRUE; }
    static CEditRootDocElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsRoot() ? (CEditRootDocElement*) pElement : 0; }
    CEditBuffer* GetBuffer(){ return m_pBuffer; }
#ifdef DEBUG
    virtual void ValidateTree();
#endif
};

class CEditLeafElement: public CEditElement {
private:
    ElementOffset m_iSize;        // fake size.
public:
    // pass through constructors.
    CEditLeafElement(CEditElement *pParent, int tagType):
            CEditElement(pParent,tagType),m_iSize(1){}

    CEditLeafElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer): 
            CEditElement(pStreamIn,pBuffer),m_iSize(1){}

    //
    // CEditElement Stuff
    //
    virtual XP_Bool IsLeaf() { return TRUE; }
    static CEditLeafElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsLeaf() ? (CEditLeafElement*) pElement : 0; }
    virtual PA_Tag* TagOpen( int iEditOffset );

    virtual void GetAll(CEditSelection& selection);
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild) {return FALSE;}
    virtual ElementIndex GetPersistentCount();

    virtual XP_Bool IsContainerContainer() { return FALSE; }

    //
    // Stuff created at this level (Leaf)
    //
    virtual void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement )=0;
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement )=0;
    virtual LO_Element* GetLayoutElement()=0;

    virtual XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter,
                LO_Element*& pRetElement, 
                int& pLayoutOffset )=0;       // if appropriate
    virtual ElementOffset GetLen(){ return m_iSize; }
    virtual void DeleteChar( MWContext *pContext, int iOffset ){ m_iSize = 0; }
    virtual CEditElement* Divide(int iOffset);
    virtual CEditTextElement* CopyEmptyText(CEditElement *pParent = 0);

    // HREF management...
    virtual void SetHREF( ED_LinkId ){}
    virtual ED_LinkId GetHREF(){ return ED_LINK_ID_NONE; }
    //
    // Leaf manipulation functions..
    //
    XP_Bool NextPosition(ElementOffset iOffset, CEditLeafElement*& pNew, ElementOffset& iNewOffset );
    XP_Bool PrevPosition(ElementOffset iOffset, CEditLeafElement*& pNew, ElementOffset& iNewOffset );

    virtual CPersistentEditInsertPoint GetPersistentInsertPoint(ElementOffset offset);

#ifdef DEBUG
    virtual void ValidateTree();
#endif
protected:
    void DisconnectLayoutElements(LO_Element* pElement);
    void SetLayoutElementHelper( intn desiredType, LO_Element** pElementHolder,
          intn iEditOffset, intn lo_type, 
          LO_Element* pLoElement);
    void ResetLayoutElementHelper( LO_Element** pElementHolder, intn iEditOffset, 
            LO_Element* pLoElement );
};

class CEditContainerElement: public CEditElement {
private:
    ED_Alignment m_align;
    ED_Alignment m_defaultAlign;
public:    

    static CEditContainerElement* NewDefaultContainer( CEditElement *pParent, 
            ED_Alignment align );
    CEditContainerElement(CEditElement *pParent, PA_Tag *pTag, 
            ED_Alignment eAlign = ED_ALIGN_DEFAULT);
    CEditContainerElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);
    virtual void StreamInChildren(IStreamIn* pIn, CEditBuffer* pBuffer);

    virtual void StreamOut( IStreamOut *pOut);
    virtual XP_Bool ShouldStreamSelf( CEditSelection& local, CEditSelection& selection) { return TRUE; }
    virtual XP_Bool IsContainer() { return TRUE; }
    static CEditContainerElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsContainer() ? (CEditContainerElement*) pElement : 0; }
    virtual XP_Bool IsContainerContainer() { return FALSE; }

    virtual ElementIndex GetPersistentCount();
    virtual void FinishedLoad( CEditBuffer *pBuffer );
   
    virtual PA_Tag* TagOpen( int iEditOffset );
    virtual PA_Tag* TagEnd( );
    virtual EEditElementType GetElementType();
    virtual void PrintOpen( CPrintState *pPrintState );
    virtual void PrintEnd( CPrintState *pPrintState );

    virtual CEditElement* Clone( CEditElement *pParent = 0);

    static EDT_ContainerData* NewData();
    static void FreeData( EDT_ContainerData *pData );

    virtual XP_Bool IsAcceptableChild(CEditElement& pChild) {return pChild.IsLeaf();}

    //
    // Implementation
    //
    void SetData( EDT_ContainerData *pData );
    EDT_ContainerData* GetData( );
    EDT_ContainerData* ParseParams( PA_Tag *pTag );

    void SetAlignment( ED_Alignment eAlign );
    ED_Alignment GetAlignment( ){ return m_align; }
    void CopyAttributes( CEditContainerElement *pContainer );
    XP_Bool IsEmpty();

    XP_Bool ShouldSkipTags();

#ifdef DEBUG
    virtual void ValidateTree();
#endif

    XP_Bool IsPlainFirstContainer();
    XP_Bool IsFirstContainer();
    XP_Bool SupportsAlign();
    void AlignIfEmpty( ED_Alignment eAlign );
};

class CEditListElement: public CEditElement {
private:
public:    
    CEditListElement( CEditElement *pParent, PA_Tag *pTag );
    CEditListElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer);

    XP_Bool IsList(){ return TRUE; }
    static CEditListElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsList() ? (CEditListElement*) pElement : 0; }
    
    virtual PA_Tag* TagOpen( int iEditOffset );
    virtual CEditElement* Clone( CEditElement *pParent = 0);

    static EDT_ListData* NewData();
    static void FreeData( EDT_ListData *pData );

    // Streaming
    virtual EEditElementType GetElementType();

    //
    // Implementation
    //
    void SetData( EDT_ListData *pData );
    EDT_ListData* GetData( );
    EDT_ListData* ParseParams( PA_Tag *pTag );

    void CopyData(CEditListElement* pOther);

#ifdef DEBUG
    virtual void ValidateTree();
#endif

};

//
// CEditTextElement
//

class CEditTextElement: public CEditLeafElement {
public:
    char* m_pText;                          // pointer to actual string.
    int m_textSize;                         // number of bytes allocated 
    LO_Element* m_pFirstLayoutElement;      
    ED_TextFormat m_tf;
    intn m_iFontSize;
    ED_Color m_color;
    ED_LinkId m_href;
    char* m_pFace;
    char* m_pScriptExtra;   // <SCRIPT> tag parameters

public:
    CEditTextElement(CEditElement *pParent, char *pText);
    CEditTextElement(IStreamIn *pStreamIn, CEditBuffer* pBuffer);
    virtual ~CEditTextElement();

    virtual XP_Bool IsText() { return TRUE; }
    static CEditTextElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsText() ? (CEditTextElement*) pElement : 0; }

    // accessor functions
    char* GetText(){ return m_pText; }
    void SetText( char* pText );
    int GetSize(){ return m_textSize; }
    ElementOffset GetLen(){ return m_pText ? XP_STRLEN( m_pText ) : 0 ; }
    LO_Element* GetLayoutElement(){ return m_pFirstLayoutElement; }
    virtual void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement );
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement );

    void SetColor( ED_Color iColor );
    ED_Color GetColor(){ return m_color; }
    void SetFontSize( int iSize );
    int GetFontSize();
    void SetFontFace(char* face);
    char* GetFontFace();
    void SetScriptExtra(char* );
    char* GetScriptExtra();
    void SetHREF( ED_LinkId );
    ED_LinkId GetHREF(){ return m_href; }
    void ClearFormatting();

    void SetData( EDT_CharacterData *pData );
    EDT_CharacterData *GetData();
    void MaskData( EDT_CharacterData*& ppData );

    //
    // utility functions
    //
    XP_Bool InsertChar( int iOffset, int newChar );
    XP_Bool GetLOTextAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter, LO_TextStruct*& pRetText, 
        int& pLayoutOffset );

    // for leaf implementation.
    void DeleteChar( MWContext *pContext, int iOffset );
    XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter,
                LO_Element*& pRetText, 
                int& pLayoutOffset ){ 
        return GetLOTextAndOffset( iEditOffset, bEditStickyAfter, *(LO_TextStruct**)&pRetText, pLayoutOffset);
    }

    LO_TextStruct* GetLOText( int iEditOffset );

    virtual PA_Tag* TagOpen( int iEditOffset );

    // output funcitons
    virtual void PrintOpen( CPrintState *ps );
    virtual void PrintWithEscapes( CPrintState *ps );
    virtual void PrintLiteral( CPrintState *ps );
    virtual void StreamOut( IStreamOut *pOut );
    virtual void PartialStreamOut( IStreamOut *pOut, CEditSelection& selection );
    virtual EEditElementType GetElementType();
    virtual XP_Bool Reduce( CEditBuffer *pBuffer );

    CEditElement* SplitText( int iOffset );
    void DeleteText();
    XP_Bool IsOffsetEnd( int iOffset ){
        ElementOffset iLen = GetLen();
        return ( iOffset == iLen
                  || ( iOffset == iLen-1  
                        && m_pText[iLen-1] == ' ')
                );
    }
    CEditTextElement* CopyEmptyText( CEditElement *pParent = 0);
    void FormatOpenTags(PA_Tag*& pStartList, PA_Tag*& pEndList);
    void FormatTransitionTags(CEditTextElement *pNext, 
            PA_Tag*& pStart, PA_Tag*& pEndList);
    void FormatCloseTags(PA_Tag*& pStartList, PA_Tag*& pEndList);
    char* DebugFormat();
    
    void PrintTagOpen( CPrintState *pPrintState, TagType t, ED_TextFormat tf, char* pExtra=0 );
    void PrintFormatDifference( CPrintState *ps, ED_TextFormat bitsDifferent );
    void PrintFormat( CPrintState *ps, CEditTextElement *pFirst, ED_TextFormat mask );
    void PrintTagClose( CPrintState *ps, TagType t );
    void PrintPopFormat( CPrintState *ps, int iStackTop );
    ED_TextFormat PrintFormatClose( CPrintState *ps );

    XP_Bool SameAttributes(CEditTextElement *pCompare);
    void ComputeDifference(CEditTextElement *pFirst, 
        ED_TextFormat mask, ED_TextFormat& bitsCommon, ED_TextFormat& bitsDifferent);

#ifdef DEBUG
    virtual void ValidateTree();
#endif

};

//-----------------------------------------------------------------------------
//  CEditImageElement
//-----------------------------------------------------------------------------

class CEditImageElement: public CEditLeafElement {
    LO_ImageStruct *m_pLoImage;
    ED_Alignment m_align;
    char *m_pParams;            // partial parameter string 

    //EDT_ImageData *pData;
    int32 m_iHeight;
    int32 m_iWidth;
    ED_LinkId m_href;
    XP_Bool m_bSizeWasGiven;
    XP_Bool m_bSizeIsBogus;
public:
    intn m_iSaveIndex;
    intn m_iSaveLowIndex;

public:
    // pass through constructors.
    CEditImageElement(CEditElement *pParent, PA_Tag* pTag = 0, 
            ED_LinkId href = ED_LINK_ID_NONE );
    CEditImageElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer); 
    virtual ~CEditImageElement();

    LO_Element* GetLayoutElement();
    virtual void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement );
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement );

    void StreamOut( IStreamOut *pOut );
    virtual EEditElementType GetElementType();


    //
    // CEditElement implementation
    //
    PA_Tag* TagOpen( int iEditOffset );
    void PrintOpen( CPrintState *pPrintState );

    //
    // CEditLeafElement Implementation
    //
    XP_Bool IsImage() { return TRUE; }
    static CEditImageElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsImage() ? (CEditImageElement*) pElement : 0; }
    void FinishedLoad( CEditBuffer *pBuffer );
    LO_ImageStruct* GetLayoutImage(){ return m_pLoImage; }

    XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter, 
                LO_Element*& pRetElement, 
                int& pLayoutOffset ){ 
        pLayoutOffset = 0; 
        pRetElement = (LO_Element*)m_pLoImage;
        pLayoutOffset = iEditOffset;
        return TRUE;
    }

    int32 GetDefaultBorder();
    void SetHREF( ED_LinkId );
    ED_LinkId GetHREF(){ return m_href; }

    //
    // Implementation
    //
    void SetImageData( EDT_ImageData *pData );
    EDT_ImageData* GetImageData( );
    EDT_ImageData* ParseParams( PA_Tag *pTag );
    char* FormatParams(EDT_ImageData* pData, XP_Bool bForPrinting);

    XP_Bool SizeIsKnown();
    
    //CLM: Since HREF is a "character" attribute,
    //  we look at images for this as well
    void MaskData( EDT_CharacterData*& pData );
    EDT_CharacterData* GetCharacterData();

};


//-----------------------------------------------------------------------------
//  CEditHRuleElement
//-----------------------------------------------------------------------------

class CEditHorizRuleElement: public CEditLeafElement {
    LO_HorizRuleStruct *m_pLoHorizRule;
public:
    // pass through constructors.
    CEditHorizRuleElement(CEditElement *pParent, PA_Tag* pTag = 0 );
    CEditHorizRuleElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer); 
    virtual ~CEditHorizRuleElement();

    void StreamOut( IStreamOut *pOut);
    virtual EEditElementType GetElementType();

    //
    // CEditLeafElement Implementation
    //
    virtual XP_Bool CausesBreakBefore() { return TRUE;}
    virtual XP_Bool CausesBreakAfter() { return TRUE;}
    virtual XP_Bool AllowBothSidesOfGap() { return TRUE; }

    virtual void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement );
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement );

    LO_Element* GetLayoutElement();
    LO_HorizRuleStruct* GetLayoutHorizRule(){ return m_pLoHorizRule; }

    XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter, 
                LO_Element*& pRetElement, 
                int& pLayoutOffset ){ 
        pLayoutOffset = 0; 
        pRetElement = (LO_Element*)m_pLoHorizRule;
        pLayoutOffset = iEditOffset;
        return TRUE;
    }

    static EDT_HorizRuleData* NewData();
    static void FreeData( EDT_HorizRuleData *pData );

    //
    // Implementation
    //
    void SetData( EDT_HorizRuleData *pData );
    EDT_HorizRuleData* GetData( );
    static EDT_HorizRuleData* ParseParams( PA_Tag *pTag );
};

//-----------------------------------------------------------------------------
//  CEditIconElement
//-----------------------------------------------------------------------------

#define EDT_ICON_NAMED_ANCHOR           0
#define EDT_ICON_FORM_ELEMENT           1
#define EDT_ICON_UNSUPPORTED_TAG        2
#define EDT_ICON_UNSUPPORTED_END_TAG    3
#define EDT_ICON_JAVA                   4
#define EDT_ICON_PLUGIN                 5

class CEditIconElement: public CEditLeafElement {
protected:
    LO_ImageStruct *m_pLoIcon;
    TagType m_originalTagType;
    int32 m_iconTag;
    XP_Bool m_bEndTag;
    char *m_pSpoofData;

public:
    // pass through constructors.
    CEditIconElement(CEditElement *pParent, int32 iconTag, PA_Tag* pTag = 0 );
    CEditIconElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer); 
    ~CEditIconElement();

    void StreamOut( IStreamOut *pOut);
    virtual EEditElementType GetElementType(){ return eIconElement; }

    static ED_TagValidateResult ValidateTag(  char *Data, XP_Bool bNoBrackets ); 

    //
    // CEditLeafElement Implementation
    //
    XP_Bool IsIcon() { return TRUE; }
    static CEditIconElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsIcon() ? (CEditIconElement*) pElement : 0; }

    virtual void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement );
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement );

    LO_Element* GetLayoutElement(){ return (LO_Element*)m_pLoIcon; }
    LO_ImageStruct* GetLayoutIcon(){ return m_pLoIcon; }

    XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter, 
                LO_Element*& pRetElement, 
                int& pLayoutOffset ){ 
        pLayoutOffset = 0; 
        pRetElement = (LO_Element*)m_pLoIcon;
        pLayoutOffset = iEditOffset;
        return TRUE;
    }

    PA_Tag* TagOpen( int iEditOffset );
    void PrintOpen( CPrintState *pPrintState );
    void PrintEnd( CPrintState *pPrintState );

    char* GetData();

    // Not currently implemented.
    void SetData( char* );

    void MorphTag( PA_Tag *pTag );
    void SetSpoofData( PA_Tag* pTag );
    void SetSpoofData( char* pData );
};

//-----------------------------------------------------------------------------
//  CEditTargetElement
//-----------------------------------------------------------------------------
class CEditTargetElement: public CEditIconElement {
private:
public:
    CEditTargetElement(CEditElement *pParent, PA_Tag* pTag = 0 );
    CEditTargetElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer); 
    void StreamOut( IStreamOut *pOut);
    EEditElementType GetElementType(){ return eTargetElement; }

    PA_Tag* TagOpen( int iEditOffset );

    // Need to be rewriten in terms of m_pTagData 
    char *GetData();
    void SetData( char* pName );
};


//-----------------------------------------------------------------------------
//  CEditBreakElement
//-----------------------------------------------------------------------------

class CEditBreakElement: public CEditLeafElement {
    LO_LinefeedStruct *m_pLoLinefeed;
public:
    // pass through constructors.
    CEditBreakElement(CEditElement *pParent, PA_Tag* pTag = 0 );
    CEditBreakElement(IStreamIn *pStreamIn, CEditBuffer *pBuffer); 
    virtual ~CEditBreakElement();

    void StreamOut( IStreamOut *pOut );
    virtual EEditElementType GetElementType();

    void PrintOpen( CPrintState *ps );

    virtual XP_Bool IsBreak() { return TRUE; }
    virtual XP_Bool CausesBreakAfter() { return TRUE;}
    static CEditBreakElement* Cast(CEditElement* pElement) {
        return pElement && pElement->IsBreak() ? (CEditBreakElement*) pElement : 0; }
    //
    // CEditLeafElement Implementation
    //
    void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement );
    virtual void ResetLayoutElement( intn iEditOffset, 
                LO_Element* pLoElement );

    LO_Element* GetLayoutElement();

    XP_Bool GetLOElementAndOffset( ElementOffset iEditOffset, XP_Bool bEditStickyAfter, 
                LO_Element*& pRetElement, 
                int& pLayoutOffset );
};

class CEditEndElement: public CEditHorizRuleElement {
public:
    CEditEndElement(CEditElement *pParent):
            CEditHorizRuleElement(pParent){
            EDT_HorizRuleData ourParams;
            ourParams.align = ED_ALIGN_LEFT;
            ourParams.size = DEFAULT_HR_THICKNESS;
            ourParams.bNoShade = FALSE;
            ourParams.iWidth = 64;
            ourParams.bWidthPercent = FALSE;
            ourParams.pExtra = 0;
            SetData(&ourParams);
            }

    virtual XP_Bool ShouldStreamSelf( CEditSelection& local, CEditSelection& selection) { return FALSE; }
    virtual void StreamOut( IStreamOut *pOut) { XP_ASSERT(FALSE); }

    void SetLayoutElement( intn iEditOffset, intn lo_type, 
                LO_Element* pLoElement ){
        CEditHorizRuleElement::SetLayoutElement(-1, lo_type, pLoElement);
    }
    
    virtual EEditElementType GetElementType() { return eEndElement; }
    virtual void PrintOpen( CPrintState *pPrintState ) {}
    virtual void PrintEnd( CPrintState *pPrintState ) {}
};

class CEditEndContainerElement: public CEditContainerElement {
public:
    CEditEndContainerElement(CEditElement *pParent) :
        CEditContainerElement(pParent, NULL, ED_ALIGN_LEFT)
        {}
    virtual void StreamOut( IStreamOut *pOut) { XP_ASSERT(FALSE); }
    virtual XP_Bool ShouldStreamSelf( CEditSelection& local, CEditSelection& selection) { return FALSE; }
    virtual XP_Bool IsAcceptableChild(CEditElement& pChild) {return pChild.GetElementType() == eEndElement;}
    virtual void PrintOpen( CPrintState *pPrintState ) {}
    virtual void PrintEnd( CPrintState *pPrintState ) {}
    virtual XP_Bool IsEndContainer() { return TRUE; }
};

//
// Macro to compute the next size of an TextBuffer.
//
#define GROW_TEXT(x)    ((x+0x20) & ~0x1f)   // grow buffer by 32 bytes

//-----------------------------------------------------------------------------
// CEditPosition
//-----------------------------------------------------------------------------

class CEditPosition {
private:
    CEditElement* m_pElement;
    int m_iOffset;
public:
    CEditPosition( CEditElement* pElement, int iOffset = 0 ):
            m_pElement(pElement), m_iOffset( iOffset ){}

    int Offset() { return m_iOffset; }
    CEditElement* Element() { return m_pElement; }
    IsPositioned(){ return m_pElement != 0; }
};


class CEditPositionComparable: public CEditPosition {
private:
    TXP_GrowableArray_int32 m_Array;
    void CalcPosition( TXP_GrowableArray_int32* pA, CEditPosition *pPos );
public:
    CEditPositionComparable( CEditElement* pElement, int iOffset = 0 ):
            CEditPosition( pElement, iOffset ) 
    {
        CalcPosition( &m_Array, this );
    }
    
    // return -1 if passed pos is before
    // return 0 if pos is the same
    // return 1 if pos is greater
    int Compare( CEditPosition *pPos );
};

//-----------------------------------------------------------------------------
// CEditBuffer (and others)..
//-----------------------------------------------------------------------------

class CParseState {
public:
    XP_Bool bLastWasSpace;
    XP_Bool m_bInTitle;
    XP_Bool m_bInHead;
    XP_Bool m_bInJavaScript;
    int m_baseFontSize;
    TXP_PtrStack_ED_Alignment m_formatAlignStack;
    TXP_PtrStack_TagType m_formatTypeStack;
    TXP_PtrStack_CEditTextElement m_formatTextStack;
    CEditTextElement *m_pNextText;
    CStreamOutMemory *m_pJavaScript;

public:
    void Reset();
    CParseState();
    ~CParseState();
};

class CPrintState {
public:
    int m_iLevel;
    int m_iCharPos;
    XP_Bool m_bTextLast;
    IStreamOut* m_pOut;
    TXP_PtrStack_ED_TextFormat m_formatStack;
    TXP_PtrStack_CEditTextElement m_elementStack;
    CEditBuffer *m_pBuffer;
public:
    void Reset( IStreamOut *pStream, CEditBuffer *pBuffer );
};


//
// This is a structure, not a class.  It cannot be instanciated with NEW and
//  should never be derived from.
//
class CEditLinkManager;

struct ED_Link {
private:
    // Should never create on of these with a constructor!  Must 
    //  create it only through CEditLinkManager::Add
    ED_Link(){}
public:
    void AddRef(){ iRefCount++; }
    void Release(){ }
    EDT_HREFData* GetData();
    intn iRefCount;
    CEditLinkManager* pLinkManager;
    int linkOffset;
    char *hrefStr;
    char *pExtra;
};

class CEditLinkManager {
private:
    TXP_GrowableArray_ED_Link m_links;

public:
    CEditLinkManager();
    ED_Link* MakeLink( char* pStr, char* pExtra, intn iRefCount = 1 );
    ED_LinkId Add( char *pHREF, char *pExtra );
    char* GetHREF( ED_LinkId id ){ return id->hrefStr; }
    int GetHREFLen( ED_LinkId id ){ return XP_STRLEN( id->hrefStr ); }
    EDT_HREFData* GetHREFData( ED_LinkId id){ return id->GetData(); } 
    void Free( ED_LinkId id );
    void AdjustAllLinks( char *pOldURL, char* pNewURL );
};

// Relayout Flags
#define RELAYOUT_NOCARET        1       // after relaying out, don't show the caret


// Commands

class CEditBuffer;

class CEditCommand {
public:
    CEditCommand(CEditBuffer*, intn id);
    virtual ~CEditCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();
    intn GetID();
#ifdef DEBUG
    virtual void Print(IStreamOut& stream);
#endif
protected:
    CEditBuffer* GetEditBuffer() { return m_editBuffer; };
private:
    CEditBuffer* m_editBuffer;
    intn m_id;
};

// CEditCommandGroup

class CEditCommandGroup : public CEditCommand {
public:
    CEditCommandGroup(CEditBuffer*, int id);
    virtual ~CEditCommandGroup();
    void AdoptAndDo(CEditCommand* pCommand);
    virtual void Undo();
    virtual void Redo();
#ifdef DEBUG
    virtual void Print(IStreamOut& stream);
#endif
    intn GetNumberOfCommands();
private:
    TXP_GrowableArray_CEditCommand m_commands;
};

/* Default */
#define EDT_CMD_LOG_MAXHISTORY 1

class CEditCommandLog {
public:
    CEditCommandLog();
    ~CEditCommandLog();

    void AdoptAndDo(CEditCommand*);
    void Undo();
    void Redo();
    void Trim();    // Trims undo and redo command lists.

    intn GetCommandHistoryLimit();
    void SetCommandHistoryLimit(intn newLimit);

    intn GetNumberOfCommandsToUndo();
    intn GetNumberOfCommandsToRedo();

    // Returns NULL if out of range
    CEditCommand* GetUndoCommand(intn);
    CEditCommand* GetRedoCommand(intn);

    void BeginBatchChanges(CEditCommandGroup* pBatch);
    void EndBatchChanges();

#ifdef DEBUG
    void Print(IStreamOut& stream);
#endif

private:
    void FinishBatchCommands();
    void InternalAdoptAndDo(CEditCommand*);
    void Trim(intn start, intn end);
    TXP_GrowableArray_CEditCommand m_commandList;
    intn m_undoSize;
    intn m_size;
    intn m_historyLimit;
    TXP_PtrStack_CEditCommandGroup m_batch;    // Aliases

#ifdef DEBUG
    friend class CEditCommandLogRecursionCheckEntry;
    XP_Bool m_bBusy;
#endif

};

// The commands

#define kNullCommandID 0
#define kTypingCommandID 1
#define kAddTextCommandID 2
#define kDeleteTextCommandID 3
#define kCutCommandID 4
#define kPasteTextCommandID 5
#define kPasteHTMLCommandID 6
#define kPasteHREFCommandID 7
#define kChangeAttributesCommandID 8
#define kIndentCommandID 9
#define kParagraphAlignCommandID 10
#define kMorphContainerCommandID 11
#define kInsertHorizRuleCommandID 12
#define kSetHorizRuleDataCommandID 13
#define kInsertImageCommandID 14
#define kSetImageDataCommandID 15
#define kInsertBreakCommandID 16
#define kChangePageDataCommandID 17
#define kSetMetaDataCommandID 18
#define kDeleteMetaDataCommandID 19
#define kInsertTargetCommandID 20
#define kSetTargetDataCommandID 21
#define kInsertUnknownTagCommandID 22
#define kSetUnknownTagDataCommandID 23
#define kGroupOfChangesCommandID 24
#define kSetListDataCommandID 25

#define kInsertTableCommandID 26
#define kDeleteTableCommandID 27
#define kSetTableDataCommandID 28

#define kInsertTableCaptionCommandID 29
#define kSetTableCaptionDataCommandID 30
#define kDeleteTableCaptionCommandID 31

#define kInsertTableRowCommandID 32
#define kSetTableRowDataCommandID 33
#define kDeleteTableRowCommandID 34

#define kInsertTableColumnCommandID 35
#define kSetTableCellDataCommandID 36
#define kDeleteTableColumnCommandID 37

#define kInsertTableCellCommandID 38
#define kDeleteTableCellCommandID 39

#define kInsertMultiColumnCommandID 40
#define kDeleteMultiColumnCommandID 41
#define kSetMultiColumnDataCommandID 42

#define kSetSelectionCommandID 43

#define kCommandIDMax 43

// This class holds a chunk of html text.

class CEditText
{
public:
    CEditText();
    ~CEditText();

    void Clear();

    // The length is the number of chars
    char* GetChars();
    int32 Length();
    char** GetPChars();
    int32* GetPLength();
private:
    char* m_pChars;
    int32 m_iLength;
    ElementIndex m_iCount;
};

// Can save and restore a region of the document. Also saves and restores current
// selection

class CEditDataSaver {
public:
    CEditDataSaver(CEditBuffer* pBuffer);
    ~CEditDataSaver();
    void DoBegin(CPersistentEditSelection& original);
    void DoEnd(CPersistentEditSelection& modified);
    void Undo();
    void Redo();

    CPersistentEditSelection* GetOriginalDocumentSelection() { return &m_originalDocument; }
private:
    CEditBuffer* m_pEditBuffer;
    CPersistentEditSelection m_originalDocument;
    CPersistentEditSelection m_modifiedDocument;
    CPersistentEditSelection m_original;
    CPersistentEditSelection m_expandedOriginal;
    CPersistentEditSelection m_expandedModified;
    CEditText m_originalText;
    CEditText m_modifiedText;
    XP_Bool m_bModifiedTextHasBeenSaved;
#ifdef DEBUG
    int m_bDoState;
#endif
};

class CInsertTableCommand
: public CEditCommand
{
public:
    CInsertTableCommand(CEditBuffer* buffer, CEditTableElement* pTable, intn id = kInsertTableCommandID);
    virtual ~CInsertTableCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    CPasteCommand* m_helper;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteTableCommand
    : public CEditCommand
{
public:
    CDeleteTableCommand(CEditBuffer* buffer, intn id = kDeleteTableCommandID);
    virtual ~CDeleteTableCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
	CEditTableElement* m_pTable;
    CPersistentEditInsertPoint m_replacePoint;
    CPersistentEditSelection m_originalSelection;
};

class CInsertTableCaptionCommand
    : public CEditCommand
{
public:
    CInsertTableCaptionCommand(CEditBuffer* buffer, EDT_TableCaptionData *pData, intn id = kInsertTableCaptionCommandID);
    virtual ~CInsertTableCaptionCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    CEditCaptionElement* m_pOldCaption;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteTableCaptionCommand
    : public CEditCommand
{
public:
    CDeleteTableCaptionCommand(CEditBuffer* buffer, intn id = kDeleteTableCaptionCommandID);
    virtual ~CDeleteTableCaptionCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    CEditCaptionElement* m_pOldCaption;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};
class CInsertTableRowCommand
    : public CEditCommand
{
public:
    CInsertTableRowCommand(CEditBuffer* buffer, EDT_TableRowData *pData, XP_Bool bAfterCurrentRow, intn number, intn id = kInsertTableRowCommandID);
    virtual ~CInsertTableRowCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    intn m_row;
    intn m_number;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteTableRowCommand
    : public CEditCommand
{
public:
    CDeleteTableRowCommand(CEditBuffer* buffer, intn rows, intn id = kDeleteTableRowCommandID);
    virtual ~CDeleteTableRowCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    XP_Bool m_bDeletedWholeTable;
    intn m_row;
    intn m_rows;
	CEditTableElement m_table;    // Holds deleted rows
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CInsertTableColumnCommand
    : public CEditCommand
{
public:
    CInsertTableColumnCommand(CEditBuffer* buffer, EDT_TableCellData *pData, XP_Bool bAfterCurrentCell, intn number, intn id = kInsertTableColumnCommandID);
    virtual ~CInsertTableColumnCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    intn m_column;
    intn m_number;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteTableColumnCommand
    : public CEditCommand
{
public:
    CDeleteTableColumnCommand(CEditBuffer* buffer, intn rows, intn id = kDeleteTableColumnCommandID);
    virtual ~CDeleteTableColumnCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    XP_Bool m_bDeletedWholeTable;
    intn m_column;
    intn m_columns;
	CEditTableElement m_table;    // Holds deleted columns
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CInsertTableCellCommand
    : public CEditCommand
{
public:
    CInsertTableCellCommand(CEditBuffer* buffer, XP_Bool bAfterCurrentCell, intn number, intn id = kInsertTableCellCommandID);
    virtual ~CInsertTableCellCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    intn m_column;
    intn m_number;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteTableCellCommand
    : public CEditCommand
{
public:
    CDeleteTableCellCommand(CEditBuffer* buffer, intn rows, intn id = kDeleteTableCellCommandID);
    virtual ~CDeleteTableCellCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    XP_Bool m_bDeletedWholeTable;
    intn m_column;
    intn m_columns;
	CEditTableRowElement m_tableRow;    // Holds deleted cells
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CInsertMultiColumnCommand
: public CEditCommand
{
public:
    CInsertMultiColumnCommand(CEditBuffer* buffer, CEditMultiColumnElement* pMultiColumn, intn id = kInsertMultiColumnCommandID);
    virtual ~CInsertMultiColumnCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
    CPasteCommand* m_helper;
    CPersistentEditSelection m_originalSelection;
    CPersistentEditSelection m_changedSelection;
};

class CDeleteMultiColumnCommand
    : public CEditCommand
{
public:
    CDeleteMultiColumnCommand(CEditBuffer* buffer, intn id = kDeleteMultiColumnCommandID);
    virtual ~CDeleteMultiColumnCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    
private:
	CEditMultiColumnElement* m_pMultiColumn;
    CPersistentEditInsertPoint m_replacePoint;
    CPersistentEditSelection m_originalSelection;
};

class CSetMultiColumnDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // data that's passed in.
    CSetMultiColumnDataCommand(CEditBuffer*, EDT_MultiColumnData *pMetaData, intn id = kSetMultiColumnDataCommandID);
    virtual ~CSetMultiColumnDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_MultiColumnData* m_pOldData;
    EDT_MultiColumnData* m_pNewData;
};

// CChangeAttributesCommand - for commands that changes the currently selected stuff

class CChangeAttributesCommand
    : public CEditCommand
{
public:
    CChangeAttributesCommand(CEditBuffer*, intn id = kChangeAttributesCommandID);
    // For when we know something about the affected range, and it isn't the current selection,
    // but it will be by the time "Do" is called.
    CChangeAttributesCommand(CEditBuffer*, CPersistentEditSelection& selection, intn id = kChangeAttributesCommandID);
    virtual ~CChangeAttributesCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    

protected:
    CPersistentEditSelection* GetOriginalDocumentSelection() { return m_saver.GetOriginalDocumentSelection(); }

private:
    CEditDataSaver m_saver;
};

// For indent & outdent, which affect whole paragraphs at a time.
// Expands the current selection to be at least a whole container.
class CChangeIndentCommand
    : public CEditCommand
{
public:
    CChangeIndentCommand(CEditBuffer*, intn id = kIndentCommandID);
    virtual ~CChangeIndentCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    

private:
    CEditDataSaver m_saver;
    CPersistentEditSelection m_selection;
};

class CSetListDataCommand
    : public CEditCommand
{
public:
    CSetListDataCommand(CEditBuffer*, EDT_ListData& listData, intn id = kSetListDataCommandID);
    virtual ~CSetListDataCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();    

private:
    EDT_ListData m_newData;
    EDT_ListData* m_pOldData;
};


// This is a paste/insert command

class CPasteCommand
    : public CEditCommand
{
public:
    // Aliases "text" until Do is done.
    CPasteCommand(CEditBuffer*, intn id, XP_Bool bClearOriginal, XP_Bool bPredictiveEnd = FALSE);
    virtual ~CPasteCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();
private:
    CEditDataSaver m_saver;
    CPersistentEditSelection m_selection;
    XP_Bool m_bClearOriginal;
    XP_Bool m_bPredictiveEnd;
};

class CAddTextCommand: public CEditCommand {
public:
	CAddTextCommand(CEditBuffer* buffer, int cmdNo = kAddTextCommandID);
	virtual ~CAddTextCommand();
	virtual void Do();
	virtual void Undo();
	virtual void Redo();

    virtual void AddCharStart();
    virtual void AddCharDeleteSpace();
    virtual void AddCharEnd();

private:
    CPersistentEditSelection m_addedText;
    XP_Bool m_bNeverUndone;
    XP_Bool m_bDeleteSpace;
    CEditDataSaver m_saver;
};

class CTypingCommand: public CEditCommand {
public:
	CTypingCommand(CEditBuffer* buffer, int cmdNo);
	virtual ~CTypingCommand();
	virtual void Do();
	virtual void Undo();
	virtual void Redo();
    void DeleteStart(CEditSelection& rangeOfDeletion); // Call before deleting.
    void DeleteEnd(); // Call after deleting.
    void AddCharStart(); // Call before adding.
    void AddCharDeleteSpace(); // Call before deleting a space.
    void AddCharEnd(); // Call after adding.
#ifdef DEBUG
    virtual void Print(IStreamOut& stream);
#endif

private:

    CEditCommandGroup m_commands; // The individual commands that make up the typing command
    CAddTextCommand* m_pAdder; // Alias, Non-NIL if we're currently adding text.
    CChangeAttributesCommand* m_pDeleter; // Alias, Non-NIL if we're currently deleting text.
};

// For commands that change the paragraph attributes of currently selected stuff

class CChangeParagraphCommand
    : public CChangeAttributesCommand
{
public:
    CChangeParagraphCommand(CEditBuffer*, intn id);
    virtual ~CChangeParagraphCommand();
    virtual void Undo();
    virtual void Redo();

private:
    void SwapAttributes();
    CEditContainerElement* FindContainer(CPersistentEditSelection& selection);
    TagType m_tag;
    ED_Alignment m_align;
};

// CChangePageDataCommand - for commands that change the document's page data

class CChangePageDataCommand
    : public CEditCommand
{
public:
    CChangePageDataCommand(CEditBuffer*, intn id = kChangePageDataCommandID);
    virtual ~CChangePageDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_PageData* m_oldData;
    EDT_PageData* m_newData;
};

// CSetMetaDataCommand - for changes to the document's meta data

class CSetMetaDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // meta data that's passed in.
    CSetMetaDataCommand(CEditBuffer*, EDT_MetaData *pMetaData, XP_Bool bDelete, intn id = kSetMetaDataCommandID);
    virtual ~CSetMetaDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    XP_Bool m_bNewItem;    // This command creates an item.
    XP_Bool m_bDelete;     // This command deletes an item.
    EDT_MetaData* m_pOldData;
    EDT_MetaData* m_pNewData;
};

class CSetTableDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // data that's passed in.
    CSetTableDataCommand(CEditBuffer*, EDT_TableData *pMetaData, intn id = kSetTableDataCommandID);
    virtual ~CSetTableDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_TableData* m_pOldData;
    EDT_TableData* m_pNewData;
};

class CSetTableCaptionDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // data that's passed in.
    CSetTableCaptionDataCommand(CEditBuffer*, EDT_TableCaptionData *pMetaData, intn id = kSetTableCaptionDataCommandID);
    virtual ~CSetTableCaptionDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_TableCaptionData* m_pOldData;
    EDT_TableCaptionData* m_pNewData;
};

class CSetTableRowDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // data that's passed in.
    CSetTableRowDataCommand(CEditBuffer*, EDT_TableRowData *pMetaData, intn id = kSetTableRowDataCommandID);
    virtual ~CSetTableRowDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_TableRowData* m_pOldData;
    EDT_TableRowData* m_pNewData;
};

class CSetTableCellDataCommand
    : public CEditCommand
{
public:
    // Note: Simply creating this command does the action -- this is so we don't have to explicitly copy the
    // data that's passed in.
    CSetTableCellDataCommand(CEditBuffer*, EDT_TableCellData *pMetaData, intn id = kSetTableCellDataCommandID);
    virtual ~CSetTableCellDataCommand();
    virtual void Undo();
    virtual void Redo();

private:
    EDT_TableCellData* m_pOldData;
    EDT_TableCellData* m_pNewData;
};

class CSetSelectionCommand
    : public CEditCommand
{
public:
    CSetSelectionCommand(CEditBuffer*, CEditSelection& selection, intn id = kSetSelectionCommandID);
    virtual ~CSetSelectionCommand();
    virtual void Do();
    virtual void Undo();
    virtual void Redo();

private:
    CPersistentEditSelection m_OldSelection;
    CPersistentEditSelection m_NewSelection;
};

// Generic timer callback

extern "C" {
void CEditTimerCallback (void * closure);
}

class CEditTimer {
public:
    CEditTimer();
    virtual ~CEditTimer();
protected:
    void SetTimeout(uint32 msecs);
    void Callback();
    void Cancel();
    XP_Bool IsTimeoutEnabled() { return m_timeoutEnabled; }
    virtual void OnCallback();
private:
    friend void CEditTimerCallback (void * closure);
    XP_Bool m_timeoutEnabled;
    void*   m_timerID;
};

class CFinishLoadTimer : public CEditTimer {
public:
    CFinishLoadTimer();
    void FinishedLoad(CEditBuffer* pBuffer);
protected:
    virtual void OnCallback();
private:
    CEditBuffer* m_pBuffer;
};

class CRelayoutTimer : public CEditTimer {
public:
    CRelayoutTimer();
    void SetEditBuffer(CEditBuffer* pBuffer);
    void Relayout(CEditElement *pStartElement, int iOffset);
    void Flush();
protected:
    virtual void OnCallback();
private:
    CEditBuffer* m_pBuffer;
    CEditElement* m_pStartElement;
    int m_iOffset;
};

class CAutoSaveTimer : public CEditTimer {
public:
    CAutoSaveTimer();
    void SetEditBuffer(CEditBuffer* pBuffer);
    void SetPeriod(int32 minutes);
    int32 GetPeriod();
    void Restart();
protected:
    virtual void OnCallback();
private:
    CEditBuffer* m_pBuffer;
    int32 m_iMinutes;
};

//
// CEditBuffer
//

// Matches LO_NA constants, plus up/down

#define EDT_NA_CHARACTER 0
#define EDT_NA_WORD 1
#define EDT_NA_LINEEDGE 2
#define EDT_NA_DOCUMENT 3
#define EDT_NA_UPDOWN 4
#define EDT_NA_COUNT 5

class CEditBuffer { 
public:
    CEditRootDocElement *m_pRoot;
    // To do: replace m_pCurrent et. al. with a CEditInsertPoint
    CEditLeafElement *m_pCurrent;
    ElementOffset m_iCurrentOffset;
    XP_Bool m_bCurrentStickyAfter;
            
    CEditElement* m_pCreationCursor;
    ED_Color m_colorText;
    ED_Color m_colorBackground;
    ED_Color m_colorLink;
    ED_Color m_colorFollowedLink;
    ED_Color m_colorActiveLink;
    char *m_pTitle;
    char *m_pBackgroundImage;
    char *m_pBodyExtra;
    CEditImageLoader *m_pLoadingImage;
    CFileSaveObject *m_pSaveObject;

    int m_hackFontSize;             // not a real implementation, just for testing
    XP_Bool m_bDirty;                  // dirty flag
    MWContext *m_pContext;

    int32 m_iDesiredX;              
    int32 m_lastTopY;
    XP_Bool m_inScroll;
    XP_Bool m_bBlocked;                // we are doing the initial layout so we 
                                    //  are blocked.
    XP_Bool m_bSelecting;
    XP_Bool m_bNoRelayout;               // maybe should be a counter
    CEditElement *m_pSelectStart;
    int m_iSelectOffset;

    int m_preformatLinePos;
    XP_Bool m_bInPreformat;             // semaphore to keep us from reentering
                                    //  NormalizePreformat


    CPrintState printState;
    CEditLinkManager linkManager;
    TXP_GrowableArray_EDT_MetaData m_metaData;

    // Get time and save in m_FileSaveTime
    // Public so CEditFileSave can call it
    void  GetFileWriteTime();

private:
    CParseState*    GetParseState();
    void            PushParseState();
    void            PopParseState();
    void            ResetParseStateStack();
    TXP_PtrStack_CParseState    m_parseStateStack;

    // CFileFileSaveObject->m_status copied here
    //  to return a result even if save object is deleted
    ED_FileError m_status;

    CEditCommandLog m_commandLog;
    CTypingCommand* m_pTypingCommand;
    XP_Bool m_bLayoutBackpointersDirty; 
    //CLM: Save the current file time to
    //     check if an outside editor changed our source
    int32 m_iFileWriteTime;

#ifdef DEBUG
    friend CEditTestManager;
    CEditTestManager* m_pTestManager;
    XP_Bool m_bSuppressPhantomInsertPointCheck;
    XP_Bool m_bSkipValidation;
#endif

public:
    CEditBuffer( MWContext* pContext );
    ~CEditBuffer();
    CEditElement* CreateElement(PA_Tag* pTag, CEditElement *pParent);
    CEditElement* CreateFontElement(PA_Tag* pTag, CEditElement *pParent);
    intn ParseTag(pa_DocData *data_doc, PA_Tag* pTag, intn status);
    void FinishedLoad();
    void FinishedLoad2();
    void DummyCharacterAddedDuringLoad();

    void PrintTree( CEditElement* m_pRoot );
    void DebugPrintTree( CEditElement* m_pRoot );
    
    void PrintDocumentHead( CPrintState *pPrintState );
    void PrintDocumentEnd( CPrintState *pPrintState );
    void AppendTitle( char* pTitleString );

    void RepairAndSet(CEditSelection& selection);
    XP_Bool Reduce( CEditElement* pRoot );
    void NormalizeTree( );
    CEditElement* FindRelayoutStart( CEditElement *pStartElement );
    void Relayout( CEditElement *pStartElement, int iOffset, 
            CEditElement *pEndElement = 0, intn relayoutFlags = 0 );
    void SetCaret();
    void InternalSetCaret(XP_Bool bRevealPosition);
    LO_Position GetEffectiveCaretLOPosition(CEditElement* pElement, intn iOffset, XP_Bool bStickyAfter);
    void RevealPosition(CEditElement* pElement, int iOffset, XP_Bool bStickyAfter);
    void RevealSelection();
    XP_Bool GetLOCaretPosition(CEditElement* pElement, int iOffset, XP_Bool bStickyAfter,
        int32& targetX, int32& targetYLow, int32& targetYHigh);
    void WindowScrolled();
    ED_ElementType GetCurrentElementType(); /* CM: "Current" is superfluous! */

    // 
    // Doesn't allocate.  Returns the base URL of the document if it has been
    //  saved.  NULL if it hasn't.
    //
    char* GetBaseURL(){ return LO_GetBaseURL( m_pContext); }

    // editor buffer commands
    EDT_ClipboardResult InsertChar( int newChar, XP_Bool bTyping );
    EDT_ClipboardResult DeletePreviousChar();
    EDT_ClipboardResult DeleteNextChar();
    EDT_ClipboardResult DeleteChar(XP_Bool bForward, XP_Bool bTyping = TRUE);
    EDT_ClipboardResult TypingDeleteSelection(CTypingCommand* cmd);
    CPersistentEditSelection GetEffectiveDeleteSelection();

    XP_Bool PreviousChar( XP_Bool bSelect );
    XP_Bool PrevPosition(CEditLeafElement *pEle, ElementOffset iOffset, CEditLeafElement*& pNew, 
            ElementOffset& iNewOffset );
    XP_Bool NextPosition(CEditLeafElement *pEle, ElementOffset iOffset, CEditLeafElement*& pNew, 
            ElementOffset& iNewOffset );
    void SelectNextChar( );
    void SelectPreviousChar( );
    void NextChar( XP_Bool bSelect );
    void UpDown( XP_Bool bSelect, XP_Bool bForward );
    void NavigateChunk( XP_Bool bSelect, intn chunkType, XP_Bool bForward );
    void EndOfLine( XP_Bool bSelect );
    //void BeginOfDocument( XP_Bool bSelect );
    //void EndOfDocument( XP_Bool bSelect );
    EDT_ClipboardResult ReturnKey(XP_Bool bTyping);
    EDT_ClipboardResult InternalReturnKey(XP_Bool bRelayout);
    void Indent();
    void IndentContainer( CEditContainerElement *pContainer, 
            CEditListElement *pList );
    void Outdent();
    void OutdentContainer( CEditContainerElement *pContainer, 
            CEditListElement *pList );
    void MorphContainer( TagType t );
    void SetParagraphAlign( ED_Alignment eAlign );
    ED_Alignment GetParagraphAlign();
    void FormatCharacter( ED_TextFormat tf );

    void SetCharacterData( EDT_CharacterData *pData );
    void SetCharacterDataSelection( EDT_CharacterData *pData );
    EDT_CharacterData* GetCharacterData( );

    void SetDisplayParagraphMarks(XP_Bool bDisplay);
    XP_Bool GetDisplayParagraphMarks();

    void SetDisplayTables(XP_Bool bDisplay);
    XP_Bool GetDisplayTables();

    ED_TextFormat GetCharacterFormatting();
    TagType GetParagraphFormatting();
    int GetFontSize();
    void SetFontSize(int n); 
    void SetFontSizeSelection(int n); 
    ED_Color GetFontColor();
    void SetFontColor(ED_Color n); 
    void SetFontColorSelection(ED_Color n); 
    void InsertLeaf(CEditLeafElement *pLeaf);
    void InsertNonLeaf( CEditElement* pNonLeaf);
    EDT_ImageData* GetImageData();
    void SetImageData( EDT_ImageData* pData, XP_Bool bKeepImagesWithDoc );
    void InsertImage( EDT_ImageData* pData );
    void LoadImage( EDT_ImageData* pData, XP_Bool bKeepImagesWithDoc, XP_Bool bReplaceImage );

    // Tables

    // Like GetInsertPoint, but handles the right edge of the
    // selection differently: If the right edge of a non-empty
    // selection is at the edge of the table cell, the insert point
    // is moved inside the table cell. This gives the
    // correct behavior for table operations.
    void GetTableInsertPoint(CEditInsertPoint& ip);

    XP_Bool IsInsertPointInTable();
    XP_Bool IsInsertPointInNestedTable();
    EDT_TableData* GetTableData();
    void SetTableData(EDT_TableData *pData);
    void InsertTable(EDT_TableData *pData);
    void DeleteTable();

    XP_Bool IsInsertPointInTableCaption();
    EDT_TableCaptionData* GetTableCaptionData();
    void SetTableCaptionData(EDT_TableCaptionData *pData);
    void InsertTableCaption(EDT_TableCaptionData *pData);
    void DeleteTableCaption();

    XP_Bool IsInsertPointInTableRow();
    EDT_TableRowData* GetTableRowData();
    void SetTableRowData(EDT_TableRowData *pData);
    void InsertTableRows(EDT_TableRowData *pData, XP_Bool bAfterCurrentRow, intn number);
    void DeleteTableRows(intn number);

    XP_Bool IsInsertPointInTableCell();
    EDT_TableCellData* GetTableCellData();
    void SetTableCellData(EDT_TableCellData *pData);
    void InsertTableCells(EDT_TableCellData *pData, XP_Bool bAfterCurrentCell, intn number);
    void DeleteTableCells(intn number);
    void InsertTableColumns(EDT_TableCellData *pData, XP_Bool bAfterCurrentColumn, intn number);
    void DeleteTableColumns(intn number);

    XP_Bool IsInsertPointInMultiColumn();
    EDT_MultiColumnData* GetMultiColumnData();
    void SetMultiColumnData(EDT_MultiColumnData *pData);
    void InsertMultiColumn(EDT_MultiColumnData *pData);
    void DeleteMultiColumn();

    EDT_PageData* GetPageData();
    void SetPageData( EDT_PageData* pData );
    static void FreePageData( EDT_PageData* pData );

    intn MetaDataCount( );
    intn FindMetaData( EDT_MetaData *pMetaData );
    intn FindMetaDataIndex( EDT_MetaData *pMetaData );
    EDT_MetaData* MakeMetaData( XP_Bool bHttpEquiv, char *pName, char*pContent );
    EDT_MetaData* GetMetaData( intn n );
    void SetMetaData( EDT_MetaData *pMetaData );
    void DeleteMetaData( EDT_MetaData *pMetaData );
    static void FreeMetaData( EDT_MetaData *pMetaData );
    void ParseMetaTag( PA_Tag *pTag );
    void PrintMetaData( CPrintState *pPrintState );

    EDT_HorizRuleData* GetHorizRuleData();
    void SetHorizRuleData( EDT_HorizRuleData* pData );
    void InsertHorizRule( EDT_HorizRuleData* pData );

    char* GetTargetData();
    void SetTargetData( char* pData );
    void InsertTarget( char* pData );
    char* GetAllDocumentTargets();
    char* GetAllDocumentTargetsInFile(char *pHref);  //CLM
    char* GetAllDocumentFiles();

    char* GetUnknownTagData();
    void SetUnknownTagData( char* pData );
    void InsertUnknownTag( char* pData );

    EDT_ListData* GetListData();
    void SetListData( EDT_ListData* pData );

    void InsertBreak( ED_BreakType eBreak, XP_Bool bTyping = TRUE );

    void SetInsertPoint( CEditLeafElement* pElement, int iOffset, XP_Bool bStickyAfter );
    void FixupInsertPoint();
    void FixupInsertPoint(CEditInsertPoint& ip);
    EDT_ClipboardResult DeleteSelection(XP_Bool bCopyAppendAttributes = TRUE);
    EDT_ClipboardResult DeleteSelection(CEditSelection& selection, XP_Bool bCopyAppendAttributes = TRUE);
    void DeleteBetweenPoints( CEditLeafElement* pBegin, CEditLeafElement* pEnd, XP_Bool bCopyAppendAttributes = TRUE );
    void PositionCaret( int32 x, int32 y);
    void StartSelection( int32 x, int32 y , XP_Bool doubleClick);
    void SelectObject( int32 x, int32 y);
    void ExtendSelection( int32 x, int32 y );
    void ExtendSelectionElement( CEditLeafElement *pElement, int iOffset, XP_Bool bStickyAfter );
    void EndSelection(int32 x, int32 y);
    void EndSelection();
    void SelectAll();
    void SelectTable();
    void SelectTableCell();
 
    void SelectRegion(CEditLeafElement *pBegin, intn iBeginPos, 
            CEditLeafElement* pEnd, intn iEndPos, XP_Bool bFromStart = FALSE,
            XP_Bool bForward = FALSE );
    void SetSelection(CEditSelection& selection);
    void SetInsertPoint(CEditInsertPoint& insertPoint);
    void SetInsertPoint(CPersistentEditInsertPoint& insertPoint);

    void GetInsertPoint( CEditLeafElement** ppLeaf, ElementOffset *pOffset, XP_Bool * pbStickyAfter);
    XP_Bool GetPropertyPoint( CEditLeafElement** ppLeaf, ElementOffset *pOffset);
    CEditElement *GetSelectedElement();
    void SelectCurrentElement();
    void ClearSelection( XP_Bool bResyncInsertPoint = TRUE, XP_Bool bKeepLeft = FALSE );
    void BeginSelection( XP_Bool bExtend = FALSE, XP_Bool bFromStart = FALSE );
    void MakeSelectionEndPoints( CEditLeafElement*& pBegin, CEditLeafElement*& pEnd );
    void MakeSelectionEndPoints( CEditSelection& selection, CEditLeafElement*& pBegin, CEditLeafElement*& pEnd );
    void GetSelection( CEditLeafElement*& pStartElement, ElementOffset& iStartOffset,
                CEditLeafElement*& pEndElement, ElementOffset& iEndOffset, XP_Bool& bFromStart );
    void FormatCharacterSelection( ED_TextFormat tf );

    int Compare( CEditElement *p1, int i1Offset, CEditElement *p2, int i2Offset );

    void FileFetchComplete(ED_FileError m_status = ED_ERROR_NONE);

    void AutoSaveCallback();
    void SetAutoSavePeriod(int32 minutes);
    int32 GetAutoSavePeriod();

    void ClearMove(XP_Bool bFlushRelayout = TRUE);
    XP_Bool IsSelected(){ return LO_IsSelected( m_pContext ); }
    XP_Bool IsSelecting(){ return m_bSelecting; }
    XP_Bool IsBlocked(){ return this == 0 || m_bBlocked; }
    XP_Bool IsPhantomInsertPoint();
    XP_Bool IsPhantomInsertPoint(CEditInsertPoint& ip);
    void ClearPhantomInsertPoint();
    XP_Bool GetDirtyFlag(){ return m_bDirty; }
    void SetDirtyFlag( XP_Bool bValue ){ m_bDirty = bValue; }

    EDT_ClipboardResult CanCut(XP_Bool bStrictChecking);
    EDT_ClipboardResult CanCut(CEditSelection& selection, XP_Bool bStrictChecking);
    EDT_ClipboardResult CanCopy(XP_Bool bStrictChecking);
    EDT_ClipboardResult CanCopy(CEditSelection& selection, XP_Bool bStrictChecking);
    EDT_ClipboardResult CanPaste(XP_Bool bStrictChecking);
    EDT_ClipboardResult CanPaste(CEditSelection& selection, XP_Bool bStrictChecking);

    XP_Bool CanSetHREF();
    char *GetHREF();
    char *GetHREFText();
    void SetHREF( char *pHREF, char *pExtra );
    void SetHREFSelection( ED_LinkId eId ); 

    // Accessor functions

    ED_FileError SaveFile(char * pSourceURL,
                          char * pDestURL,
                          XP_Bool   bSaveAs,
                          XP_Bool   bKeepImagesWithDoc,
                          XP_Bool   bAutoAdjustLinks );

    ED_FileError WriteBufferToFile( char* pFileName );

    //CLM: Get current time and return TRUE if it changed
    XP_Bool IsFileModified();

    // saves the edit buffer to a file.
    //CM: was char *pFileName, added return value 0 for success, -1 if fail
    int  WriteToFile( XP_File hFile ); 
    void WriteToBuffer( char **PpBuffer );
#ifdef DEBUG
    void DebugPrintState(IStreamOut& stream);
    void ValidateTree();
    void SuppressPhantomInsertPointCheck(XP_Bool bSuppress);
#endif
    void DisplaySource();
    char* NormalizeText( char* pSrc );
    intn NormalizePreformatText( pa_DocData *pData, PA_Tag *pTag, intn status );
    MWContext *GetContext(){ return m_pContext; }
    EDT_ClipboardResult PasteText(char *pText);
    EDT_ClipboardResult PasteHTML(char *pText, XP_Bool bUndoRedo);
    EDT_ClipboardResult PasteExternalHTML(char *pText);
    EDT_ClipboardResult PasteHTML(IStreamIn& stream, XP_Bool bUndoRedo);
    EDT_ClipboardResult PasteHREF( char **ppHref, char **ppTitle, int iCount);
    EDT_ClipboardResult CopySelection( char **ppText, int32* pTextLen,
            char **ppHtml, int32* pHtmlLen);

    XP_Bool CopyBetweenPoints( CEditElement *pBegin,
                    CEditElement *pEnd, char **ppText, int32* pTextLen,
                    char **ppHtml, int32* pHtmlLen );
    XP_Bool CopySelectionContents( CEditSelection& selection,
                    char **ppHtml, int32* pHtmlLen );
    XP_Bool CopySelectionContents( CEditSelection& selection,
                    IStreamOut& stream );
    EDT_ClipboardResult CutSelection( char **ppText, int32* pTextLen,
            char **ppHtml, int32* pHtmlLen);
    XP_Bool CutSelectionContents( CEditSelection& selection,
                    char **ppHtml, int32* pHtmlLen );

    void RefreshLayout();
    void FixupSpace( XP_Bool bTyping = FALSE);
    ED_Alignment GetCurrentAlignment();
    int32 GetDesiredX(CEditElement* pEle, intn iOffset, XP_Bool bStickyAfter);

    // Selection utility methods
    void GetInsertPoint(CEditInsertPoint& insertPoint);
    void GetInsertPoint(CPersistentEditInsertPoint& insertPoint);
    void GetSelection(CEditSelection& selection);
    void GetSelection(CPersistentEditSelection& persistentSelection);
    void SetSelection(CPersistentEditSelection& persistentSelection);

    // Navigation
    XP_Bool Move(CEditInsertPoint& pt, XP_Bool forward);
    XP_Bool Move(CPersistentEditInsertPoint& pt, XP_Bool forward);
    XP_Bool CanMove(CEditInsertPoint& pt, XP_Bool forward);
    XP_Bool CanMove(CPersistentEditInsertPoint& pt, XP_Bool forward);

    void CopyEditText(CPersistentEditSelection& selection, CEditText& text);
    void CopyEditText(CEditText& text);
    void CutEditText(CEditText& text);
    void PasteEditText(CEditText& text);

    // Persistent to regular selection conversion routines
    CEditInsertPoint PersistentToEphemeral(CPersistentEditInsertPoint& persistentInsertPoint);
    CPersistentEditInsertPoint EphemeralToPersistent(CEditInsertPoint& insertPoint);
    CEditSelection PersistentToEphemeral(CPersistentEditSelection& persistentInsertPoint);
    CPersistentEditSelection EphemeralToPersistent(CEditSelection& insertPoint);

    // Command methods
    void AdoptAndDo(CEditCommand*);
    void Undo();
    void Redo();
    void Trim();

    // History limits
    intn GetCommandHistoryLimit();
    void SetCommandHistoryLimit(intn newLimit);

    // Returns NULL if out of range
    CEditCommand* GetUndoCommand(intn);
    CEditCommand* GetRedoCommand(intn);

    void BeginBatchChanges(intn commandID);
    void EndBatchChanges();

    // Typing command methods.
    void DoneTyping();
    CTypingCommand* GetTypingCommand(); // Creates it if needed
    // Hook for typing command to inform editor it's been deleted.
    void TypingDeleted(CTypingCommand* command);

    void SyncCursor(CEditTableElement* pTable, intn iColumn, intn iRow);
    CEditSelection ComputeCursor(CEditTableElement* pTable, intn iColumn, intn iRow);
    void SyncCursor(CEditMultiColumnElement* pMultiColumn);

#ifdef FIND_REPLACE
    XP_Bool FindAndReplace( EDT_FindAndReplaceData *pData );
#endif

private:
    void ParseBodyTag(PA_Tag *pTag);
    CFinishLoadTimer m_finishLoadTimer;
    CRelayoutTimer m_relayoutTimer;
    CAutoSaveTimer m_autoSaveTimer;
    XP_Bool m_bDisplayTables;
    XP_Bool m_bDummyCharacterAddedDuringLoad;
    static XP_Bool m_bAutoSaving;
};


//
// CEditTagCursor - 
//
class CEditTagCursor {
private: // types
    enum TagPosition { tagOpen, tagEnd };

private: // data

    CEditBuffer *m_pEditBuffer;
    CEditElement *m_pCurrentElement;
    CEditPositionComparable m_endPos;
    TagPosition m_tagPosition;
    int m_stateDepth;
    int m_currentStateDepth;
    int m_iEditOffset;
    XP_Bool m_bClearRelayoutState;
    MWContext *m_pContext;
    PA_Tag* m_pStateTags;


public:  // routines
    CEditTagCursor( CEditBuffer* pEditBuffer, CEditElement *pElement, 
            int iEditOffset, CEditElement* pEndElement );
    ~CEditTagCursor();

    PA_Tag* GetNextTag();
    PA_Tag* GetNextTagState();
    XP_Bool AtBreak(XP_Bool* pEndTag);
    int32 CurrentLine();
    XP_Bool ClearRelayoutState() { return m_bClearRelayoutState; }
    CEditTagCursor* Clone();

};

//-----------------------------------------------------------------------------
//  CEditImageLoader
//-----------------------------------------------------------------------------
class CEditImageLoader {
private:
    CEditBuffer *m_pBuffer;
    EDT_ImageData *m_pImageData;
    LO_ImageStruct *m_pLoImage;
    XP_Bool m_bReplaceImage;

public:
    CEditImageLoader( CEditBuffer *pBuffer, EDT_ImageData *pImageData, 
            XP_Bool bReplaceImage ); 
    ~CEditImageLoader();

    void LoadImage();
    void SetImageInfo(int32 ele_id, int32 width, int32 height);
};


//-----------------------------------------------------------------------------
//  CEditFileBackup
//
// File backup and restore Object
//  Object can be reinititialized with a Reset.
//
//-----------------------------------------------------------------------------
class CEditFileBackup {
private:
    XP_Bool m_bInitOk;
    char *m_pBackupName;
    char *m_pFileName;

public:
    CEditFileBackup(): m_bInitOk(FALSE), m_pBackupName(0), m_pFileName(0){}

    ~CEditFileBackup(){
        Reset();
    }

    void Reset();
    XP_Bool InTransaction(){ return m_bInitOk; }
    ED_FileError BeginTransaction( char *pDestURL );
    char* FileName(){ return m_pFileName; }
    void Commit();
    void Rollback();
};


//-----------------------------------------------------------------------------
//  CFileSaveObject
//-----------------------------------------------------------------------------
class CFileSaveObject {
private:
    XP_Bool m_bOverwriteAll;
    XP_Bool m_bDontOverwriteAll;
    XP_Bool m_bDontOverwrite;
    int m_iCurFile;
    XP_File m_outFile;
    CEditFileBackup m_fileBackup;

    TXP_GrowableArray_pChar m_srcImageURLs;
    TXP_GrowableArray_pChar m_destImageFileBaseName;

protected:
    MWContext *m_pContext;
    char *m_pDestPathURL;       
    int m_iExtraFiles;
    XP_Bool m_bDeleteMe;
    ED_FileError m_status;

public:
    //
    // inherited interface.
    //
    CFileSaveObject( MWContext *pContext ) ;
    virtual ~CFileSaveObject();

    // can be a full file path.  Grabs the path part automatically.
    void SetDestPathURL( char* pDestURL );


    intn AddFile( char *pSrcURL, char *pDestURL );
    
    ED_FileError SaveFiles();

    char *GetDestName( intn index );
    char *GetSrcName( intn index );

    // Inherit from CFileSaveObject and implement FileFetchComplete.
    //
    virtual ED_FileError FileFetchComplete();

    void Cancel();

private:
    ED_FileError FetchNextFile();
/*    intn OpenOutputFile( int iFile ); */

// Netlib interface
public:
    // Made Public so we can access from file save stream
    //  (We don't open file until 
    //   libnet succeeds in finding source data)
    intn OpenOutputFile(/*int iFile*/);
    // Net Callbacks
    int NetStreamWrite( const char *block, int32 length );
    void NetFetchDone( URL_Struct *pUrl, int status, MWContext *pContext );

};


//-----------------------------------------------------------------------------
//  CEditSaveObject
//-----------------------------------------------------------------------------
class CEditSaveObject: public CFileSaveObject{
private:
    CEditBuffer *m_pBuffer;
    char *m_pSrcURL;
    char *m_pDestFileURL;
    XP_Bool m_bSaveAs;
    XP_Bool m_bKeepImagesWithDoc;
    XP_Bool m_bAutoAdjustLinks;
    XP_Bool m_bOverwriteAll;
    XP_Bool m_bDontOverwriteAll;
    XP_Bool m_bFixupImageLinks;
    intn m_backgroundIndex;

public:
    CEditSaveObject( CEditBuffer *pBuffer, char *pSrcURL, char* pDestFileURL,
                            XP_Bool bSaveAs, XP_Bool bKeepImagesWithDoc, 
                            XP_Bool bAutoAdjustLinks ); 
    ~CEditSaveObject();

    XP_Bool MakeImagesAbsolute(){ return !m_bKeepImagesWithDoc && m_bAutoAdjustLinks; }

    XP_Bool FindAllDocumentImages();
    intn AddImage( char *pSrc );
    XP_Bool IsSameURL( char* pSrcURL, char *pLocalName );

    virtual ED_FileError FileFetchComplete();
    void FixupLinks();
};


//-----------------------------------------------------------------------------
//  CEditImageSaveObject
//-----------------------------------------------------------------------------
class CEditImageSaveObject: public CFileSaveObject{
private:
    CEditBuffer *m_pBuffer;
    EDT_ImageData *m_pData;
    XP_Bool m_bReplaceImage;

public:
    intn m_srcIndex;
    intn m_lowSrcIndex;

    CEditImageSaveObject( CEditBuffer *pBuffer, EDT_ImageData *pData, 
                        XP_Bool bReplaceImage );
    ~CEditImageSaveObject();

    virtual ED_FileError FileFetchComplete();
};

//-----------------------------------------------------------------------------
//  CEditBackgroundImageSaveObject
//-----------------------------------------------------------------------------
class CEditBackgroundImageSaveObject: public CFileSaveObject{
private:
    CEditBuffer *m_pBuffer;

public:
    CEditBackgroundImageSaveObject( CEditBuffer *pBuffer );
    ~CEditBackgroundImageSaveObject();

    virtual ED_FileError FileFetchComplete();
};



//-----------------------------------------------------------------------------
//  Streams
//-----------------------------------------------------------------------------

#define STREAM_PRINTF_MAX       1024



//
// Abstract output file stream.
//
class IStreamOut {
public:
    virtual void Write( char *pBuffer, int32 iCount )=0;

    // the use of this printf function is limited to 1024 bytes and is not
    //  checked.  Default implementation is implemented in term of 'Write'
    virtual int Printf( char * pFormat, ... );

    enum EOutStreamStatus {
        EOS_NoError,
        EOS_DeviceFull
    };

    virtual EOutStreamStatus Status(){ return EOS_NoError; }

    // implemented in terms of the interface.
    void WriteInt( int32 i ){ Write( (char*)&i, sizeof( int32 ) ); }
    void WriteZString( char* pString);
    void WritePartialZString( char* pString, ElementIndex start, ElementIndex end);
};

class IStreamIn {
public:
    virtual void Read( char *pBuffer, int32 iCount )=0;
    int32 ReadInt(){ int32 i; Read((char*)&i, sizeof(int32)); return i; }
    char *ReadZString();
};


//
// Output Stream as file.
//
class CStreamOutFile: public IStreamOut {
private:
    EOutStreamStatus m_status;
    XP_File m_outputFile;  
    XP_Bool m_bBinary;
public:
    CStreamOutFile(  XP_File hFile, XP_Bool bBinary  );
    ~CStreamOutFile();
    virtual void Write( char *pBuffer, int32 iCount );
    virtual EOutStreamStatus Status(){ return m_status; }
};

//
// Stream out to net.
//
class CStreamOutNet: public IStreamOut {
private:
    NET_StreamClass * m_pStream;

public:
    CStreamOutNet( MWContext* pContext );
    ~CStreamOutNet();
    virtual void Write( char *pBuffer, int32 iCount );
};

//
// Output Stream as memory
//
class CStreamOutMemory: public IStreamOut {
private:
    int32 m_bufferSize;
    int32 m_bufferEnd;
    XP_HUGE_CHAR_PTR m_pBuffer;
public:
    CStreamOutMemory();
    ~CStreamOutMemory();
    virtual void Write( char *pBuffer, int32 iCount );
    XP_HUGE_CHAR_PTR GetText(){ return m_pBuffer; }
    int32 GetLen(){ return m_bufferEnd; }
};


class CStreamInMemory: public IStreamIn {
private:
    XP_HUGE_CHAR_PTR m_pBuffer;
    int32 m_iCurrentPos;
public:
    virtual void Read( char *pBuffer, int32 iCount );
    CStreamInMemory(char *pMem): m_pBuffer(pMem), m_iCurrentPos(0){}
};

extern CBitArray *edt_setNoEndTag;
extern CBitArray *edt_setWriteEndTag;
extern CBitArray *edt_setHeadTags;
extern CBitArray *edt_setSoloTags;
extern CBitArray *edt_setBlockFormat;
extern CBitArray *edt_setCharFormat;
extern CBitArray *edt_setList;
extern CBitArray *edt_setUnsupported;
extern CBitArray *edt_setTextContainer;
extern CBitArray *edt_setListContainer;
extern CBitArray *edt_setParagraphBreak;
extern CBitArray *edt_setFormattedText;
extern CBitArray *edt_setContainerSupportsAlign;
extern CBitArray *edt_setIgnoreWhiteSpace;
extern CBitArray *edt_setSuppressNewlineBefore;
extern CBitArray *edt_setRequireNewlineAfter;

inline XP_Bool BitSet( CBitArray* pBitArray, int i ){ if ( i < 0 ) return FALSE;
    else return (*pBitArray)[i]; }

inline int IsListContainer( TagType i ){ return BitSet( edt_setListContainer, i ); }

inline int TagHasClose( TagType i ){ return !BitSet( edt_setNoEndTag, i ); }
inline int WriteTagClose( TagType i ){ return (BitSet( edt_setWriteEndTag, i ) ||  
                TagHasClose(i)); }

//
// Utility functions.
//
char *EDT_TagString(int32 tagType);
ED_ETextFormat edt_TagType2TextFormat( TagType t );
char *edt_WorkBuf( int iSize );
char *edt_QuoteString( char* pString );
char *edt_MakeParamString( char* pString );

char *edt_FetchParamString( PA_Tag *pTag, char* param );
XP_Bool edt_FetchParamBoolExist( PA_Tag *pTag, char* param );
XP_Bool edt_FetchDimension( PA_Tag *pTag, char* param, 
                         int32 *pValue, XP_Bool *pPercent,
                         int32 nDefaultValue, XP_Bool bDefaultPercent );
ED_Alignment edt_FetchParamAlignment( PA_Tag* pTag, 
                ED_Alignment eDefault=ED_ALIGN_BASELINE, XP_Bool bVAlign = FALSE );
int32 edt_FetchParamInt( PA_Tag *pTag, char* param, int32 defValue );
ED_Color edt_FetchParamColor( PA_Tag *pTag, char* param );

// Preserves the old value of data if there's no parameter.
void edt_FetchParamString2(PA_Tag* pTag, char* param, char*& data);
void edt_FetchParamColor2( PA_Tag *pTag, char* param, ED_Color& data );

// Concatenates onto the old value
void edt_FetchParamExtras2( PA_Tag *pTag, char**ppKnownParams, char*& data);

LO_Color* edt_MakeLoColor( ED_Color c );
void edt_SetLoColor( ED_Color c, LO_Color *pColor );

void edt_InitEscapes(MWContext *pContext);
void edt_PrintWithEscapes( CPrintState *ps, char *p, XP_Bool bBreakLines );
char *edt_LocalName( char *pURL );
PA_Block PA_strdup( char* s );

inline char *edt_StrDup( char *pStr );
EDT_ImageData* edt_NewImageData();
EDT_ImageData* edt_DupImageData( EDT_ImageData *pOldData );
void edt_FreeImageData( EDT_ImageData *pData );

void edt_SetTagData( PA_Tag* pTag, char* pTagData);
void edt_AddTag( PA_Tag*& pStart, PA_Tag*& pEnd, TagType t, XP_Bool bIsEnd,
        char *pTagData = 0 );

void edt_InitBitArrays();

//
// Retreive extra parameters and return them in the form:
//
//  ' foo=bar zoo mombo="fds ifds ifds">'
//
char* edt_FetchParamExtras( PA_Tag *pTag, char**ppKnownParams );

// 
// Parse return values.
//      
#define OK_IGNORE       2
#define OK_CONTINUE     1
#define OK_STOP         0
#define NOT_OK          -1



#endif  // _EDITOR_H
#endif  // EDITOR

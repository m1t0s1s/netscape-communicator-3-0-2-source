/* -*- Mode: C++; tab-width: 4; tabs-indent-mode: nil -*- */

//
// Editor save stuff. LWT 06/01/95
// this should be on the branch
//

#ifdef EDITOR

#include "editor.h"
#include "gui.h"

#ifdef XP_WIN32
#include	<crtdbg.h>
#endif

// XP Strings
extern "C" {
#include "xpgetstr.h"
extern int XP_EDITOR_AUTO_SAVE;
}

// Defining DEBUG_AUTO_SAVE will make the document AutoSave every six seconds.
//#define DEBUG_AUTO_SAVE

// Test if ele
#define  ED_IS_NOT_SCRIPT(pEle) (0 == (pEle->Text()->m_tf & (TF_SERVER | TF_SCRIPT)))

// Semantic sugar for debugging. Validates the tree on construction and
// destruction. Place in every method that munges the tree.
//
// VALIDATE_TREE(this);

#ifdef DEBUG
class CTreeValidater {
    CEditBuffer* m_pBuffer;
public:
    CTreeValidater(CEditBuffer* pBuffer)
     : m_pBuffer(pBuffer)
    {
        m_pBuffer->ValidateTree();
    }
    ~CTreeValidater() {
        m_pBuffer->ValidateTree();
    }
};

class CSuppressPhantomInsertPointCheck {
    CEditBuffer* m_pBuffer;
public:
    CSuppressPhantomInsertPointCheck(CEditBuffer* pBuffer)
     : m_pBuffer(pBuffer)
    {
        m_pBuffer->SuppressPhantomInsertPointCheck(TRUE);
    }
    ~CSuppressPhantomInsertPointCheck() {
        m_pBuffer->SuppressPhantomInsertPointCheck(FALSE);
    }
};

// The local variable names will cause problems if you try to
// use these macros twice in the same scope. However,
// they are nescessary to ensure that some compilers (MS VC 2.2)
// keep the object alive for the duration of the scope, instead
// of constructing and destructing it at the point of declaration.
// I think this may be a bug in the compiler -- but I'd have
// to check the ANSI standard to make sure.

#define VALIDATE_TREE(BUFFER) CTreeValidater CTreeValidater_x_zzx1(BUFFER)
#define SUPPRESS_PHANTOMINSERTPOINTCHECK(BUFFER) \
    CSuppressPhantomInsertPointCheck CSuppressPhantomInsertPointCheck_x_zzx2(BUFFER)
#else

#define VALIDATE_TREE(BUFFER)
#define SUPPRESS_PHANTOMINSERTPOINTCHECK(BUFFER)

#endif

// Automated test routine hook

#ifdef DEBUG

class CEditTestManager {
public:
    CEditTestManager(CEditBuffer* pBuffer);
    XP_Bool Key(char key);
    XP_Bool Backspace();
    XP_Bool ReturnKey();

protected:

    void DumpLoElements();
    void VerifyLoElements();
    void DumpDocumentContents();

    XP_Bool ArrowTest();
    XP_Bool LArrowTest(XP_Bool bSelect);
    XP_Bool RArrowTest(XP_Bool bSelect);
    XP_Bool UArrowTest(XP_Bool bSelect);
    XP_Bool DArrowTest(XP_Bool bSelect);
    XP_Bool EveryNavigationKeyEverywhereTest();
    XP_Bool NavigateChunkCrashTest();
    XP_Bool NavChunkCrashTest(XP_Bool bSelect, int chunk, XP_Bool bDirection);
    XP_Bool DeleteKeyTest();
    XP_Bool BackspaceKeyTest();
    XP_Bool ZeroDocumentCursorTest();
    XP_Bool OneCharDocumentCursorTest();
    XP_Bool OneDayTest(int32 rounds);
    XP_Bool BoldTest(int32 rounds);

    void GetWholeDocumentSelection(CPersistentEditSelection& selection);
    void    DumpMemoryDelta();

    void JRITest();

private:
    CEditBuffer* m_pBuffer;
    XP_Bool m_bTesting;
    int m_iTriggerIndex;
    static const char* m_kTrigger;

#ifdef XP_WIN32
    _CrtMemState m_state;
#endif
};

const char* CEditTestManager::m_kTrigger = "$$$Test";

CEditTestManager::CEditTestManager(CEditBuffer* pBuffer)
    : m_pBuffer(pBuffer),
      m_iTriggerIndex(0),
      m_bTesting(FALSE)
{
#ifdef XP_WIN32
    _CrtMemCheckpoint( &m_state ); // In theorey, avoid measuring the data before we were created.
#endif
}

void CEditTestManager::DumpMemoryDelta() {
    m_pBuffer->Trim(); // Clear out undo/redo log to simplify memory statistics.
#ifdef XP_WIN32
    _CrtMemDumpAllObjectsSince( &m_state );
    _CrtMemCheckpoint( &m_state );
#else
    XP_TRACE(("Not supported on this OS.\n"));
#endif
}

XP_Bool CEditTestManager::Key(char key) {
    XP_Bool result = m_bTesting;
    if ( ! m_bTesting ){
        if ( key == m_kTrigger[m_iTriggerIndex] ) {
            m_iTriggerIndex++;
            if ( m_kTrigger[m_iTriggerIndex] == '\0' ) {
                m_bTesting = TRUE;
                XP_TRACE(("Testing on! Type # of test, or Q to quit."));
            }
        }
        else {
            m_iTriggerIndex = 0;
        }
    }
    else {
        int bQuitTesting = FALSE;
        m_bTesting = FALSE; // So when the tests type, it doesn't cause a recursion into the test code.
        intn bTestResult = -1;
        if ( key >= 'A' && key <= 'Z' ) key = key + ('a' - 'A');
        switch (key) {
            case 'q':
                bQuitTesting = TRUE;
                XP_TRACE(("Quitting test mode."));
                break;
            case 'a':
                DumpLoElements();
                break;
            case 'b':
                VerifyLoElements();
                break;
            case 'c':
                DumpDocumentContents();
                break;
            case 'd':
                DumpMemoryDelta();
                break;
#ifdef EDITOR_JAVA
            case 'j':
                JRITest();
                break;
#endif
            case '0':
                bTestResult = EveryNavigationKeyEverywhereTest();
                break;
            case '1':
                bTestResult = ArrowTest();
                break;
            case '2':
                bTestResult = BackspaceKeyTest();
                break;
            case '3':
                bTestResult = DeleteKeyTest();
                break;
            case '4':
                bTestResult = ZeroDocumentCursorTest();
                break;
            case '5':
                bTestResult = OneCharDocumentCursorTest();
                break;
            case '6':
                bTestResult = BoldTest(10); // OneDayTest(1);
                break;
            case '7':
                    m_pBuffer->m_bSkipValidation = TRUE;
                    bTestResult = OneDayTest(100);
                    m_pBuffer->m_bSkipValidation = FALSE;
                break;
            case '?':
#ifdef EDITOR_JAVA
                 XP_TRACE(("? - type this help\n"
                    "Q - quit.\n"
                    "a - print lo-elements.\n"
                    "b - verify lo-elements.\n"
                    "c - print document contents (elements, undo history, etc.)\n"
                    "d - dump memory usage delta.\n"
                    "j - java run time interface test.\n"
                    ));
#else
                 XP_TRACE(("? - type this help\n"
                    "Q - quit.\n"
                    "a - print lo-elements.\n"
                    "b - verify lo-elements.\n"
                    "c - print document contents (elements, undo history, etc.)\n"
                    "d - dump memory usage delta.\n"
                    ));
#endif // EDITOR_JAVA
                 XP_TRACE((
                    "0 - navigation key crash test.\n"
                    "1 - arrow completeness test.\n"
                    "2 - destructive backspace test.\n"
                    "3 - destructive delete test.\n"
                    "4 - empty document cursor test.\n"
                    "5 - one character document cursor test.\n"
                    "6 - one day document test - simulate editing for one whole day 1 cycle.\n"
                    "7 - one day document test - simulate editing for one whole day 500 cycle.\n"
                    ));
               break;
            default:
                 XP_TRACE(("Type ? for help, Type # of test, or Q to quit."));
                 break;
        }
        if ( ! bQuitTesting )
        {
            m_bTesting = TRUE;
        }
        if ( bTestResult >= 0 ){
            XP_TRACE(("Test %s", bTestResult ? "Passed": "Failed"));
        }
    }
    return result;
}

void CEditTestManager::DumpLoElements() {
    lo_PrintLayout(m_pBuffer->m_pContext);
}

void CEditTestManager::VerifyLoElements() {
    lo_VerifyLayout(m_pBuffer->m_pContext);
}

void CEditTestManager::DumpDocumentContents(){
    CStreamOutMemory buffer;
    m_pBuffer->printState.Reset( &buffer, m_pBuffer );
    m_pBuffer->DebugPrintTree( m_pBuffer->m_pRoot );
    m_pBuffer->DebugPrintState(buffer);
    // have to dump one line at a time. XP_TRACE has a 512 char limit on Windows.
    char* b = buffer.GetText();
    while ( *b != '\0' ) {
        char* b2 = b;
        while ( *b2 != '\0' && *b2 != '\n'){
            b2++;
        }
        char old = *b2;
        *b2 = '\0';
        XP_TRACE(("%s", b));
        *b2 = old;
        b = b2 + 1;
        if ( old == '\0' ) break;
    }
}

XP_Bool CEditTestManager::ZeroDocumentCursorTest(){
    XP_TRACE(("ZeroDocumentCursorTest"));
    // [selection][chunk][direction][edge]
    const ElementIndex kExpectedResults[2][EDT_NA_COUNT][2][2] =
    {   // no selection
        {
            {{ 0, 0}, {0, 0} }, // EDT_NA_CHARACTER
            {{ 0, 0}, {0, 0} }, // EDT_NA_WORD
            {{ 0, 0}, {0, 0} }, // EDT_NA_LINEEDGE
            {{ 0, 0}, {0, 0} }, // EDT_NA_DOCUMENT
            {{ 0, 0}, {0, 0} }  // EDT_NA_UPDOWN
        },
        // selection
        {
            {{ 0, 0}, {0, 1} }, // EDT_NA_CHARACTER
            {{ 0, 0}, {0, 1} }, // EDT_NA_WORD
            {{ 0, 0}, {0, 1} }, // EDT_NA_LINEEDGE
            {{ 0, 0}, {0, 1} }, // EDT_NA_DOCUMENT
            {{ 0, 0}, {0, 1} }  // EDT_NA_UPDOWN
        }
    };

    const char* kChunkName[EDT_NA_COUNT] = {
        "character", "word", "line edge", "document", "updown"};

    XP_Bool result = TRUE;
    // Clear everything from existing document
    EDT_SelectAll(m_pBuffer->m_pContext);
    EDT_DeleteChar(m_pBuffer->m_pContext);
    CPersistentEditInsertPoint zero(0,FALSE);
    // Verify the cursor does the right thing for every possible cursor
    for ( int select = 0; select < 2; select++ ) {
        for ( int chunk = 0; chunk < EDT_NA_COUNT; chunk++ ) {
            for ( int direction = 0; direction < 2; direction++ ) {
                m_pBuffer->SetInsertPoint(zero);
                m_pBuffer->NavigateChunk(select, chunk, direction);
                CPersistentEditSelection selection;
                m_pBuffer->GetSelection(selection);
                for ( int edge = 0; edge < 2; edge++ ) {
                    ElementIndex expected = kExpectedResults[select][chunk][direction][edge];
                    ElementIndex actual = selection.GetEdge(edge)->m_index;
                    if ( expected != actual ){
                        result = FALSE;
                        XP_TRACE(("selection: %s chunk: %s direction: %s edge: %s. Expected %d got %d",
                            select ? "TRUE" : "FALSE",
                            kChunkName[chunk],
                            direction ? "forward" : "back",
                            edge ? "end" : "start",
                            expected,
                            actual));
                    }
                }
            }
        }
    }
    return result;
}


XP_Bool CEditTestManager::OneCharDocumentCursorTest(){
    XP_TRACE(("OneCharDocumentCursorTest"));
    // [startPos][selection][chunk][direction][edge]
    const ElementIndex kExpectedResults[2][2][EDT_NA_COUNT][2][2] =
    {
        // position 0
        {   // no selection
            {
                {{ 0, 0}, {1, 1} }, // EDT_NA_CHARACTER
                {{ 0, 0}, {1, 1} }, // EDT_NA_WORD
                {{ 0, 0}, {1, 1} }, // EDT_NA_LINEEDGE
                {{ 0, 0}, {1, 1} }, // EDT_NA_DOCUMENT
                {{ 0, 0}, {0, 0} }  // EDT_NA_UPDOWN
            },
            // selection
            {
                {{ 0, 0}, {0, 1} }, // EDT_NA_CHARACTER
                {{ 0, 0}, {0, 1} }, // EDT_NA_WORD
                {{ 0, 0}, {0, 2} }, // EDT_NA_LINEEDGE
                {{ 0, 0}, {0, 2} }, // EDT_NA_DOCUMENT
                {{ 0, 0}, {0, 2} }  // EDT_NA_UPDOWN
            }
        },
        // position 1
        {   // no selection
            {
                {{ 0, 0}, {1, 1} }, // EDT_NA_CHARACTER
                {{ 0, 0}, {1, 1} }, // EDT_NA_WORD
                {{ 0, 0}, {1, 1} }, // EDT_NA_LINEEDGE
                {{ 0, 0}, {1, 1} }, // EDT_NA_DOCUMENT
                {{ 1, 1}, {1, 1} }  // EDT_NA_UPDOWN
            },
            // selection
            {
                {{ 0, 1}, {1, 2} }, // EDT_NA_CHARACTER
                {{ 0, 1}, {1, 2} }, // EDT_NA_WORD
                {{ 0, 1}, {1, 2} }, // EDT_NA_LINEEDGE
                {{ 0, 1}, {1, 2} }, // EDT_NA_DOCUMENT
                {{ 0, 1}, {1, 2} }  // EDT_NA_UPDOWN
            }
        }
    };

    const char* kChunkName[EDT_NA_COUNT] = {
        "character", "word", "line edge", "document", "updown"};

    XP_Bool result = TRUE;
    // Clear everything from existing document
    EDT_SelectAll(m_pBuffer->m_pContext);
    EDT_DeleteChar(m_pBuffer->m_pContext);
    m_pBuffer->InsertChar( 'X', FALSE );
    // Verify the cursor does the right thing for every possible cursor
    for ( int startPos = 0; startPos < 2; startPos++ ){
        CPersistentEditInsertPoint p(startPos,FALSE);
        for ( int select = 0; select < 2; select++ ) {
            for ( int chunk = 0; chunk < EDT_NA_COUNT; chunk++ ) {
                for ( int direction = 0; direction < 2; direction++ ) {
                    m_pBuffer->SetInsertPoint(p);
                    m_pBuffer->NavigateChunk(select, chunk, direction);
                    CPersistentEditSelection selection;
                    m_pBuffer->GetSelection(selection);
                    for ( int edge = 0; edge < 2; edge++ ) {
                        ElementIndex expected = kExpectedResults[startPos][select][chunk][direction][edge];
                        ElementIndex actual = selection.GetEdge(edge)->m_index;
                        if ( expected != actual ){
                            result = FALSE;
                            XP_TRACE(("position: %d selection: %s chunk: %s direction: %s edge: %s. Expected %d got %d",
                                startPos,
                                select ? "TRUE" : "FALSE",
                                kChunkName[chunk],
                                direction ? "forward" : "back",
                                edge ? "end" : "start",
                                expected,
                                actual));
                        }
                    }
                }
            }
        }
    }
    return result;
}

XP_Bool CEditTestManager::OneDayTest(int32 rounds) {
    char* kTitle = "One Day Test\n";
    XP_TRACE(("%s", kTitle));
    EDT_SelectAll(m_pBuffer->m_pContext);
    EDT_DeleteChar(m_pBuffer->m_pContext);
    EDT_PasteText(m_pBuffer->m_pContext, kTitle);
    for( int32 i = 0; i < rounds; i++ ) {
        XP_TRACE(("Round %d", i));
        const char* kTestString = "All work and no play makes Jack a dull boy.\nRedrum.\n";
        char c;
        for ( int j = 0;
              (c = kTestString[j]) != '\0';
            j++ ) {
            if ( c == '\n' || c == '\r') {
                EDT_ReturnKey(m_pBuffer->m_pContext);
            }
            else {
                EDT_KeyDown(m_pBuffer->m_pContext, c,0,0);
            }
        }
        // And delete a little.
        for ( int k = 0; k < 8; k++ ) {
            EDT_DeletePreviousChar(m_pBuffer->m_pContext);
        }
    }
    XP_TRACE(("End of %s.", kTitle));
    return TRUE;
}

XP_Bool CEditTestManager::BoldTest(int32 rounds) {
    char* kTitle = "Bold Test\n";
    XP_TRACE(("%s", kTitle));
    EDT_SelectAll(m_pBuffer->m_pContext);
    EDT_DeleteChar(m_pBuffer->m_pContext);
    EDT_PasteText(m_pBuffer->m_pContext, kTitle);
    EDT_SelectAll(m_pBuffer->m_pContext);
    for( int32 i = 0; i < rounds; i++ ) {
        XP_TRACE(("Round %d", i));
        EDT_CharacterData* normal = EDT_GetCharacterData( m_pBuffer->m_pContext );
        EDT_CharacterData* bold = EDT_GetCharacterData( m_pBuffer->m_pContext );
        bold->mask = TF_BOLD;
        bold->values = TF_BOLD;
        EDT_SetCharacterData(m_pBuffer->m_pContext, bold);
        EDT_SetCharacterData(m_pBuffer->m_pContext, normal);
        EDT_FreeCharacterData(normal);
        EDT_FreeCharacterData(bold);
    }
    XP_TRACE(("End of %s.", kTitle));
    return TRUE;
}

XP_Bool CEditTestManager::EveryNavigationKeyEverywhereTest() {
    // Does navigation work from every position?
    XP_Bool result;
    result = NavigateChunkCrashTest();
    result &= UArrowTest(FALSE);
    result &= UArrowTest(TRUE);
    result &= DArrowTest(FALSE);
    result &= DArrowTest(TRUE);
    XP_TRACE(("Done.\n"));
    return result;
}

XP_Bool CEditTestManager::ArrowTest() {
    // Does the navigation move smoothly through the document?
    XP_Bool result;
    result = LArrowTest(FALSE);
    result |= LArrowTest(TRUE);
    result |= RArrowTest(FALSE);
    result |= RArrowTest(TRUE);
#if 0
    // Too many false positives -- need to know about up/down at first/last line
    result |= UArrowTest(FALSE);
    result |= DArrowTest(FALSE);
#endif
    result |= UArrowTest(TRUE);
    result |= DArrowTest(TRUE);
    XP_TRACE(("Done.\n"));
    return result;
}

void CEditTestManager::GetWholeDocumentSelection(CPersistentEditSelection& selection){
    CEditSelection wholeDocument;
    m_pBuffer->m_pRoot->GetAll(wholeDocument);
    selection = m_pBuffer->EphemeralToPersistent(wholeDocument);
    // Ignore the last 2 positions -- it's the EOF marker
    selection.m_end.m_index -= 2;
}

XP_Bool CEditTestManager::NavigateChunkCrashTest(){
    XP_Bool result = TRUE;
    for ( int select = 0; select < 2; select++ ) {
        for ( int chunk = 0; chunk < EDT_NA_COUNT; chunk++ ) {
            for ( int direction = 0; direction < 2; direction++ ) {
                result &= NavChunkCrashTest(select, chunk, direction);
            }
        }
    }
    return result;
}

XP_Bool CEditTestManager::NavChunkCrashTest(XP_Bool bSelect, int chunk, XP_Bool bDirection){
    XP_Bool bResult = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("NavChunkCrashTest bSelect = %d chunk = %d bDirection = %d\n", bSelect, chunk, bDirection));

    for ( p = w.m_start;
        p.m_index < w.m_end.m_index;
        p.m_index++ ) {
        m_pBuffer->SetInsertPoint(p);
        m_pBuffer->NavigateChunk(bSelect, chunk, bDirection);
    }
    return bResult;
}

XP_Bool CEditTestManager::RArrowTest(XP_Bool bSelect){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("Right Arrow%s\n", bSelect ? " with shift key" : ""));

    for ( p = w.m_start;
        p.m_index < w.m_end.m_index;
        p.m_index++ ) {
        m_pBuffer->SetInsertPoint(p);
        m_pBuffer->NavigateChunk(bSelect, LO_NA_CHARACTER, TRUE);
        CPersistentEditSelection s2;
        m_pBuffer->GetSelection(s2);
        if ( (bSelect == s2.IsInsertPoint()) ||
            s2.m_end.m_index != p.m_index + 1 ) {
            XP_TRACE(("%d should be %d was %d\n", p.m_index,
                p.m_index + 1, s2.m_end.m_index));
            result = FALSE;
        }
    }
    return result;
}

XP_Bool CEditTestManager::LArrowTest(XP_Bool bSelect){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p(0,TRUE);
    XP_TRACE(("Left Arrow Test%s\n", bSelect ? " with shift key" : ""));

    for ( p.m_index = w.m_end.m_index - 1;
        p.m_index > w.m_start.m_index;
        p.m_index-- ) {

        CEditSelection startSelection;
        CPersistentEditSelection s2;

        p.m_bStickyAfter = TRUE;
        m_pBuffer->SetInsertPoint(p);

        m_pBuffer->GetSelection(startSelection);
        if ( startSelection.m_start.GapWithBothSidesAllowed() && ! bSelect ) {
            // XP_TRACE(("%d - gap with both sides allowed.", p.m_index));
            // Test that SetInsertPoint did the right thing.
            m_pBuffer->GetSelection(s2);
            if ( ! s2.IsInsertPoint() || !s2.m_start.IsEqualUI(p) ) {
                XP_TRACE(("SetInsertPoint at %d should be %d.%d was %d.%d\n", p.m_index,
                    p.m_index, p.m_bStickyAfter,
                    s2.m_start.m_index, s2.m_start.m_bStickyAfter));
                result = FALSE;
            }
            // Test hump over soft break
            m_pBuffer->NavigateChunk(bSelect, LO_NA_CHARACTER, FALSE);
            m_pBuffer->GetSelection(s2);
            CPersistentEditInsertPoint expected = p;
            expected.m_bStickyAfter = FALSE;
            if ( ! s2.IsInsertPoint() || !s2.m_start.IsEqualUI(expected) ) {
                XP_TRACE(("gap at %d should be %d.%d was %d.%d\n", p.m_index,
                    expected.m_index, expected.m_bStickyAfter,
                    s2.m_start.m_index, s2.m_start.m_bStickyAfter));
                result = FALSE;
            }
            p.m_bStickyAfter = FALSE;
            m_pBuffer->SetInsertPoint(p);
        }
        m_pBuffer->NavigateChunk(bSelect, LO_NA_CHARACTER, FALSE);
        m_pBuffer->GetSelection(s2);
        if ( bSelect == s2.IsInsertPoint() ) {
            XP_TRACE(("%d Wrong type of selection. Expected %d.%d was %d.%d\n", p.m_index,
                p.m_index, p.m_bStickyAfter,
                s2.m_start.m_index, s2.m_start.m_bStickyAfter));
            result = FALSE;
        }
        else if ( s2.m_start.m_index != p.m_index - 1 ) {
            XP_TRACE(("%d wrong edge position. Should be %d.%d was %d.%d\n", p.m_index,
                p.m_index-1, p.m_bStickyAfter,
                s2.m_start.m_index, s2.m_start.m_bStickyAfter));
            result = FALSE;
        }
    }
    return result;
}

XP_Bool CEditTestManager::UArrowTest(XP_Bool bSelect){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("Up Arrow Test%s\n", bSelect ? " with shift key" : ""));

    for ( p = w.m_end.m_index - 1;
        p.m_index > w.m_start.m_index;
        p.m_index-- ) {
        m_pBuffer->SetInsertPoint(p);
        CPersistentEditSelection s;
        m_pBuffer->GetSelection(s);
        m_pBuffer->ClearMove();
        m_pBuffer->UpDown(bSelect, FALSE);
        CPersistentEditSelection s2;
        m_pBuffer->GetSelection(s2);
        if ( s2 == s ) {
            XP_TRACE(("%d didn't change selection\n", p.m_index));
            result = FALSE;
        }
    }
    return result;
}


XP_Bool CEditTestManager::DArrowTest(XP_Bool bSelect){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("Down Arrow Test%s\n", bSelect ? " with shift key" : ""));

    for ( p = w.m_start.m_index;
        p.m_index < w.m_end.m_index;
        p.m_index++ ) {
        m_pBuffer->SetInsertPoint(p);
        CPersistentEditSelection s;
        m_pBuffer->GetSelection(s);
        m_pBuffer->ClearMove();
        m_pBuffer->UpDown(bSelect, TRUE);
        CPersistentEditSelection s2;
        m_pBuffer->GetSelection(s2);
        if ( s2 == s ) {
            XP_TRACE(("%d didn't change selection\n", p.m_index));
            result = FALSE;
        }
    }
    return result;
}

XP_Bool CEditTestManager::BackspaceKeyTest(){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("DeleteKeyTest"));
    p = w.m_end;
    p.m_index--;
    m_pBuffer->SetInsertPoint(p);
    for ( p = w.m_start.m_index;
        p.m_index < w.m_end.m_index;
        p.m_index++ ) {
        m_pBuffer->DeletePreviousChar();
    }
    return result;
}

XP_Bool CEditTestManager::DeleteKeyTest(){
    XP_Bool result = TRUE;
    CPersistentEditSelection w;
    GetWholeDocumentSelection(w);
    CPersistentEditInsertPoint p;
    XP_TRACE(("DeleteKeyTest"));

    m_pBuffer->SetInsertPoint(w.m_start);
    for ( p = w.m_start.m_index;
        p.m_index < w.m_end.m_index;
        p.m_index++ ) {
        m_pBuffer->DeleteNextChar();
    }
    return result;
}

XP_Bool CEditTestManager::Backspace() {
    XP_Bool result = m_bTesting;
    if ( ! m_bTesting ){
        m_iTriggerIndex--;
        if (m_iTriggerIndex < 0 ) m_iTriggerIndex = 0;
    }
    return m_bTesting;
}

XP_Bool CEditTestManager::ReturnKey() {
    if ( ! m_bTesting ){
        m_iTriggerIndex = 0;
    }
    return m_bTesting;
}

#ifdef EDITOR_JAVA

#include <jri.h>

extern "C" { JRIEnv* npn_getJavaEnv(void); }

void CEditTestManager::JRITest(){
    XP_TRACE(("JRITest"));
    JRIEnv* env = npn_getJavaEnv();
    if ( ! env ) return; /* No Java. */
    JRI_ExceptionClear(env);

    // To Do: Define a java class for editor plug-ins.

    struct java_lang_Class* integerClazz = JRI_FindClass(env, "java/lang/Integer");
    if ( JRI_ExceptionOccurred(env))
        return;
    JRIMethodID integerClazzmethodID = JRI_GetStaticMethodID(env, integerClazz, "Integer", "Ijava/lang/Integer;");
    if ( JRI_ExceptionOccurred(env))
        return;
    jref integer =  JRI_NewObject(env, integerClazz, integerClazzmethodID);
    if ( JRI_ExceptionOccurred(env))
        return;
}
#endif // EDITOR_JAVA

#endif // DEBUG

//-----------------------------------------------------------------------------
// CParseState / CPrintState
//-----------------------------------------------------------------------------

CParseState::CParseState()
    :m_pNextText(0),m_bInTitle(FALSE), m_bInHead(FALSE),m_bInJavaScript(FALSE),
                    m_pJavaScript(0){
}

CParseState::~CParseState() {
    delete m_pNextText;
}

void CParseState::Reset(){
    bLastWasSpace = TRUE;               // at the beginning of the document
                                        //  we should ignore leading spaces
    m_baseFontSize = 3;
    m_formatTypeStack.Reset();
    m_formatTextStack.Reset();
    m_bInHead = FALSE;
    m_bInTitle = FALSE;
    m_bInJavaScript = FALSE;
    m_pJavaScript = 0;
    delete m_pNextText;
    // force the compiler to use the non-stream constructor
    m_pNextText = new CEditTextElement((CEditElement*)0,0);
}

//-----------------------------------------------------------------------------
//  CPrintState
//-----------------------------------------------------------------------------
void CPrintState::Reset( IStreamOut* pOut, CEditBuffer *pBuffer ){
    m_pOut = pOut;
    m_iCharPos = 0; 
    m_bTextLast = FALSE;
    m_iLevel = 0;
    m_pBuffer = pBuffer;
}

//-----------------------------------------------------------------------------
// CEditLinkManager
//-----------------------------------------------------------------------------

EDT_HREFData* ED_Link::GetData(){
    EDT_HREFData* pData = EDT_NewHREFData();
    pData->pURL = XP_STRDUP( hrefStr );
    if( pExtra ){
        pData->pExtra = XP_STRDUP( pExtra );
    }
    else {
        pData->pExtra = 0;
    }
    return pData;
}

CEditLinkManager::CEditLinkManager(){
}

ED_Link* CEditLinkManager::MakeLink( char *pHREF, char *pExtra, intn iRefCount ){
    
    ED_Link *pNewLink = XP_NEW( ED_Link );
    pNewLink->iRefCount = iRefCount;
    pNewLink->pLinkManager = this;
    pNewLink->hrefStr = XP_STRDUP( pHREF );
    if( pExtra ){
        pNewLink->pExtra = XP_STRDUP( pExtra );
    }
    else {
        pNewLink->pExtra = 0;
    }
    return pNewLink;
}

void CEditLinkManager::AdjustAllLinks( char *pOldURL, char* pNewURL ){
    int i = 0;
    while( i < m_links.Size() ){
        ED_Link *pLink;
        if( (pLink = m_links[i]) != 0 ){
            if( pLink->hrefStr 
                    && pLink->hrefStr[0] != '#' 
                    &&  pLink->hrefStr[0] != '`' ){

                char *pNewHref;
                char *pStr = NET_MakeAbsoluteURL( pOldURL, pLink->hrefStr );
                int iRet = NET_MakeRelativeURL( pNewURL, 
                    pStr, 
                    &pNewHref );
                XP_FREE( pStr );

                // if the url is cross device or the same device but not with
                //  absolute pathing (starts with a slash)
                //  then perform the path conversion.

                if( iRet == NET_URL_NOT_ON_SAME_DEVICE
                        || ( iRet == NET_URL_SAME_DEVICE 
                            && pLink->hrefStr[0] != '/') ){

                    if( pLink->hrefStr != 0 ){
                        XP_FREE( pLink->hrefStr );
                    }
                    pLink->hrefStr = XP_STRDUP( pNewHref );
                }
            }
        }
        i++;
    }
}

ED_LinkId CEditLinkManager::Add( char *pHREF, char *pExtra ){
    int i = 0;
    int lastFree = -1;
    while( i < m_links.Size() ){
        if( m_links[i] != 0 ){

            // need a more intelegent way of comparing HREFs
            if( XP_STRCMP( m_links[i]->hrefStr, pHREF) == 0 
                    && (!!pExtra == !!m_links[i]->pExtra)
                    && ((pExtra && 
                            XP_STRCMP( pExtra, m_links[i]->pExtra ) == 0 ) 
                       || pExtra == 0 )
                ){
                m_links[i]->iRefCount++;
                return m_links[i];
            }
        }
        else {
            lastFree = i;
        }
        i++;
    }

    ED_Link *pNewLink = MakeLink( pHREF, pExtra );

    //
    // Store it.
    //    
    if( lastFree != -1 ){
        m_links[lastFree] = pNewLink;
    }
    else {
        lastFree = m_links.Add( pNewLink );
    }
    pNewLink->linkOffset = lastFree;
    return pNewLink;
}

void CEditLinkManager::Free(ED_LinkId id){
    if( --id->iRefCount == 0 ){
        m_links[ id->linkOffset ] = 0;
        XP_FREE( id );
    }
}

//-----------------------------------------------------------------------------
// CEditBuffer
//-----------------------------------------------------------------------------

CEditElement* CEditBuffer::CreateElement(PA_Tag* pTag, CEditElement *pParent){
    CEditElement *pElement;
    // this is a HACK.  We don't understand the tag but we want
    //  to make sure that we can re-gurgitate it to layout.  
    //  Save the parameter data so we can spew it back out.
    char *locked_buff;

    PA_LOCK(locked_buff, char *, pTag->data);
    if( locked_buff && *locked_buff != '>'){
        pElement = new CEditElement(pParent,pTag->type,locked_buff);
    }
    else {
        pElement = new CEditElement(pParent,pTag->type);
    }
    PA_UNLOCK(pTag->data);
    return pElement;
}

CParseState* CEditBuffer::GetParseState() {
    if ( m_parseStateStack.IsEmpty() ) {
        PushParseState();
    }
    return m_parseStateStack.Top();
}

void CEditBuffer::PushParseState(){
    CParseState* state = new CParseState();
    state->Reset();
    m_parseStateStack.Push(state);
}

void CEditBuffer::PopParseState(){
    if ( m_parseStateStack.IsEmpty() ) {
        XP_ASSERT(FALSE);
    }
    else {
        CParseState* state = m_parseStateStack.Pop();
        delete state;
    }
}

void CEditBuffer::ResetParseStateStack(){
    while ( ! m_parseStateStack.IsEmpty() ) {
        PopParseState();
    }
}

//
// stolen from layout
//
#define DEF_TAB_WIDTH       8

#define REMOVECHAR(p) XP_MEMMOVE( p, p+1, XP_STRLEN(p) );

void edt_ExpandTab(char *p, intn n){
    if( n != 1 ){
        XP_MEMMOVE( p+n, p+1, XP_STRLEN(p));
    }
    XP_MEMSET( p, ' ', n);
}


//
// Parse the text into a series of tags.  Each tag is at most one line in 
//  length.
//
intn CEditBuffer::NormalizePreformatText(pa_DocData *pData, PA_Tag* pTag,
            intn status ){
    char *pText;
    intn retVal = OK_CONTINUE;
    m_bInPreformat = TRUE;

    PA_LOCK(pText, char *, pTag->data);


    //
    // Calculate how big a buffer we are going to need to normalize this
    //  preformatted text.
    //    
    int iTabCount = 0;
    int iLen = 0;
    int iNeedLen;
    int iSpace;

    while( *pText ){
        if( *pText == TAB ){
            iTabCount++;
        }
        pText++;
        iLen++;
    }

    PA_UNLOCK(pTag->data);
    iNeedLen = iLen + iTabCount * DEF_TAB_WIDTH+1;

    pTag->data =  (PA_Block) PA_REALLOC( pTag->data, iNeedLen );
    pTag->data_len = iNeedLen;
    PA_LOCK(pText, char *, pTag->data);

    // LTNOTE: total hack, but make sure the buffer is 0 terminated...  Probably has
    //  bad internation ramifications.. Oh Well...
    // JHPNOTE: I'll say this is a total hack. We used to write at [data_len], which
    // smashed beyond allocated memory. As far as I can see there's no need to write here,
    // since the string is known to be zero terminated now, and all the operations
    // below keep the string zero terminated. Never the less, in the interest of keeping
    // tradition alive (and because I don't want to take the chance of introducing a
    // bug, I'll leave the correct, but unnescessary, code in place.

    pText[iNeedLen-1] = 0;
    pText[iLen] = 0;

    char* pBuf = pText;
    //
    // Now actually Normalize the preformatted text
    //
    while( *pText ){
        switch( *pText ){
        // Look for CF LF, if you see it, just treat it as LF
        case CR:
            if( pText[1] == LF ){
                REMOVECHAR(pText);
                break;
            }
            else {
                *pText = LF;
            }

        case LF:
            pText++;
            m_preformatLinePos = 0;
            break;

        case VTAB:
        case FF:
            REMOVECHAR(pText);
            break;

        case TAB:
            iSpace = DEF_TAB_WIDTH - (m_preformatLinePos % DEF_TAB_WIDTH);
            
            edt_ExpandTab( pText, iSpace );
            pText += iSpace;
            m_preformatLinePos += iSpace;
            break;
            

        default:
            pText++;
            m_preformatLinePos++;
        }
    }

    XP_Bool bBreak = FALSE;
    while( *pBuf && retVal == OK_CONTINUE ){
        pText = pBuf;
        bBreak = FALSE;

        // scan for end of line.
        while( *pText && *pText != '\n' ){
            pText++;
        }

        
        PA_Tag *pNewTag = XP_NEW( PA_Tag );
        XP_BZERO( pNewTag, sizeof( PA_Tag ) );
        pNewTag->type = P_TEXT;

        char save = *pText;
        *pText = 0;
        edt_SetTagData(pNewTag, pBuf );
        *pText = save; 

        // include the new line
        if( *pText == '\n' ){
            pText++;
            bBreak = TRUE;
        }

        pBuf = pText;
        retVal = EDT_ProcessTag( pData, pNewTag, status );
        if( bBreak ){
            pNewTag = XP_NEW( PA_Tag );
            XP_BZERO( pNewTag, sizeof( PA_Tag ) );
            pNewTag->type = P_LINEBREAK;
            retVal = EDT_ProcessTag( pData, pNewTag, status );
        }
    }

    PA_UNLOCK(pTag->data);
    m_bInPreformat = FALSE;
    return ( retVal == OK_CONTINUE) ? OK_IGNORE : retVal ;
}


static char *anchorHrefParams[] = {
    PARAM_HREF,
    0
};

static char *bodyParams[] = {
    PARAM_BACKGROUND,
    PARAM_BGCOLOR,
    PARAM_TEXT,
    PARAM_LINK,
    PARAM_ALINK,
    PARAM_VLINK,
    0
};

void NormalizeEOLsInString(char* pStr){
    int state = 0;
    char* pOut = pStr;
    for(;; ) {
        char c = *pStr++;
        if ( c == '\0' ) break;

        int charClass = 0;
        if ( c == '\r' ) charClass = 1;
        else if ( c == '\n') charClass = 2;
        switch ( state ) {
        case 0: /* Normal */
            switch ( charClass ) {
            case 0: /* Normal char */
                *pOut++ = c;
                break;
            case 1: /* CR */
                state = 1;
                *pOut++ = '\n';
                break;
            case 2: /* LF */
                state = 2;
                *pOut++ = '\n';
                break;
            }
            break;
        case 1: /* Just saw a CR */
            switch ( charClass ) {
            case 0: /* Normal char */
                *pOut++ = c;
                state = 0;
                break;
            case 1: /* CR */
                *pOut++ = '\n';
                break;
            case 2: /* LF */
                state = 0;
                /* Swallow it, back to normal */
                break;
            }
            break;
        case 2: /* Just saw a LF */
            switch ( charClass ) {
            case 0: /* Normal char */
                *pOut++ = c;
                state = 0;
                break;
            case 1: /* CR */
                /* Swallow it, back to normal */
                state = 0;
                break;
            case 2: /* LF */
                *pOut++ = '\n';
                break;
            }
            break;
        }
    }
    *pOut = '\0';
}


void NormalizeEOLsInTag(PA_Tag* pTag){
    // Convert the end-of-lines into \n characters.
    if( pTag->data ){
        char* pStr;
        PA_LOCK(pStr, char*, pTag->data);
        NormalizeEOLsInString(pStr);
        pTag->data_len = strlen( pStr );
        PA_UNLOCK( pTag->data );
    }
}

XP_Bool IsDocTypeTag(PA_Tag* pTag){
    // Is this a DocType tag?
    XP_Bool result = FALSE;
    if ( pTag->type == P_UNKNOWN ) {
        if( pTag->data ){
            char* pStr;
            const char* kDocType = "!DOCTYPE";
            unsigned int docTypeLen = XP_STRLEN(kDocType);
            PA_LOCK(pStr, char*, pTag->data);
            if ( XP_STRLEN(pStr) >= docTypeLen && XP_STRNCASECMP(pStr, kDocType, docTypeLen) == 0 ) {
                result = TRUE;
            }
            PA_UNLOCK( pTag->data );
        }
    }
    return result;
}

intn CEditBuffer::ParseTag(pa_DocData *pData, PA_Tag* pTag, intn status){
    char *pStr;
    CEditElement* pElement = 0;
    CEditElement *pContainer;
    PA_Block buff;
    TagType t;
    intn retVal = OK_CONTINUE;

    NormalizeEOLsInTag(pTag);

    /* P_STRIKE is a synonym for P_STRIKEOUT. Since pre-3.0 versions of
     * Navigator don't recognize P_STRIKE ( "<S>" ), we switch it to
     * P_STRIKEOUT ( "<STRIKE>" ). Also, it would complicate the
     * editor internals to track two versions of the same tag.
     * We test here so that we catch both the beginning and end tag.
     */
    if ( pTag->type == P_STRIKE ) {
        pTag->type = P_STRIKEOUT;
    }

    //
    // LTNOTE: make this a seperate routine.
    // If this is an end tag, match backwardly to the appropriate tag.
    //
    if( pTag->is_end && !BitSet( edt_setUnsupported, pTag->type ) && 
            pTag->type != P_UNKNOWN ){
        CEditElement *pPop = m_pCreationCursor;
        if( pTag->type == P_HEAD ){
            GetParseState()->m_bInHead = FALSE;
            return retVal;
        }
        if( pTag->type == P_TITLE ){
            GetParseState()->m_bInTitle = FALSE;
            return retVal;
        }
        if( pTag->type == P_SCRIPT && GetParseState()->m_bInJavaScript ){
            GetParseState()->m_bInJavaScript = FALSE;
            return retVal;
        }
        if( BitSet( edt_setTextContainer, pTag->type ) || BitSet( edt_setList, pTag->type ) ){
            GetParseState()->bLastWasSpace = TRUE;
        }

        while( pPop && pTag->type != pPop->GetType() ){
            pPop = pPop->GetParent();
        }

        // if this is character formatting, pop the stack.
        if( pTag->type == P_CENTER | pTag->type == P_DIVISION ){
            if( !GetParseState()->m_formatAlignStack.IsEmpty() ){
                GetParseState()->m_formatAlignStack.Pop();
            }
        }
        else if( BitSet( edt_setCharFormat, pTag->type ) && !GetParseState()->m_formatTypeStack.IsEmpty()){
            // The navigator is forgiving about the nesting of style end tags, e.g. </b> vs. </i>.
            // So you can say, for example,
            // <b>foo<i>bar</b>baz</i> and it will work as if you had said:
            // <b>foo<i>bar</i>baz</b>.

            // LTNOTE: what the fuck. This crashes sometimes??
            //delete GetParseState()->m_pNextText;
            GetParseState()->m_pNextText = 0;
            t = GetParseState()->m_formatTypeStack.Pop();
            GetParseState()->m_pNextText = GetParseState()->m_formatTextStack.Pop();

            // </a> pops all the HREFs. So if you say:
            // <a href="a">foo<a href="b">bar</a>baz, it's as if you said:
            // <a href="a">foo</a><a href="b">bar</a>baz
            // For fun, try this:
            // <a href="a">foo<a name="b">bar</a>baz
            // The 'bar' will look like it is a link, but it won't act like a link if you
            // press on it.
            // Anyway, we emulate this mess by clearing the hrefs out of the format text stack when we
            // close any P_ANCHOR tag

            if ( pTag->type == P_ANCHOR ) {
                if ( GetParseState()->m_pNextText ) {
                    GetParseState()->m_pNextText->SetHREF( ED_LINK_ID_NONE );
                }
                TXP_PtrStack_CEditTextElement& textStack = GetParseState()->m_formatTextStack;
                for ( int i = 0; i < textStack.Size(); i++) {
                    if ( textStack[i] ) {
                        textStack[i]->SetHREF( ED_LINK_ID_NONE );
                    }
                }
            }
        }
        else if( pPop && pTag->type == pPop->GetType() && pPop->GetParent() != 0 ) {
            m_pCreationCursor = pPop->GetParent();
        }
        else{
            // We have an error.  Ignore it for now.
            //DebugBreak();
        }
        if ( pTag->type == P_TABLE_HEADER || pTag->type == P_TABLE_DATA || pTag->type == P_CAPTION ) {
            PopParseState();
        }
    }        
    else {
        //
        // make sure all solo tags are placed in a paragraph.
        //
        if( pTag->type != P_TEXT 
                && (BitSet( edt_setSoloTags,  pTag->type  ) 
                        || BitSet( edt_setUnsupported,  pTag->type ) )
                && !BitSet( edt_setTextContainer,  m_pCreationCursor->GetType()  )
                && m_pCreationCursor->FindContainer() == 0 ){
            //m_pCreationCursor = new CEditElement(m_pCreationCursor,P_PARAGRAPH);
            m_pCreationCursor = CEditContainerElement::NewDefaultContainer( m_pCreationCursor, 
                        GetCurrentAlignment() );

        }

        switch(pTag->type){

            case P_TEXT:
                if( pTag->data ){
                    PA_LOCK(pStr, char*, pTag->data);
                    
                    if( GetParseState()->m_bInTitle ){
                        AppendTitle( pStr );
                        PA_UNLOCK( pTag->data );
                        return retVal;
                    }
                    else if( GetParseState()->m_bInJavaScript ){
                        GetParseState()->m_pJavaScript->Write( pStr, XP_STRLEN( pStr ) );
                        PA_UNLOCK( pTag->data );
                        return OK_IGNORE;
                    }
                    else if( !BitSet( edt_setFormattedText, m_pCreationCursor->GetType() ) 
                            && !(GetParseState()->m_pNextText->m_tf & (TF_SERVER|TF_SCRIPT))
                            && !m_pCreationCursor->InFormattedText() ){
                        NormalizeText( pStr ); 
                    }
                    else {
                        if( !m_bInPreformat ){
                            //
                            // calls EDT_ProcessTag iteratively
                            //
                            return NormalizePreformatText( pData, pTag, status );
                        }
                    }
                    // we probably need to adjust the length in the parse tag.
                    //  it works without it, but I'm not sure why.

                    // if the text didn't get reduced away.
                    // Strip white space in tables
                    XP_Bool bGoodText = *pStr && ! (( XP_IS_SPACE(*pStr) && pStr[1] == '\0' ) &&
                        BitSet( edt_setIgnoreWhiteSpace,  m_pCreationCursor->GetType()));
                    if( bGoodText ){
                        // if this text is not within a container, make one.
                        if( !BitSet( edt_setTextContainer,  m_pCreationCursor->GetType()  )
                                && m_pCreationCursor->FindContainer() == 0 ){
                            m_pCreationCursor = CEditContainerElement::NewDefaultContainer( m_pCreationCursor, 
                                    GetCurrentAlignment() );
                        }
                        CEditTextElement *pNew = GetParseState()->m_pNextText->CopyEmptyText(m_pCreationCursor);
                        pNew->SetText( pStr );

                        // create a new text element.  It adds itself as a child
                        pElement = pNew;
                    }
                    else {
                        retVal =  OK_IGNORE;
                    }
                    PA_UNLOCK( pTag->data );
                }
                break;

            //
            // Paragraphs are always inserted at the level of the current
            //  container. 
            //
            case P_HEADER_1:
            case P_HEADER_2:
            case P_HEADER_3:
            case P_HEADER_4:
            case P_HEADER_5:
            case P_HEADER_6:
            case P_DESC_TITLE:
            case P_ADDRESS:
            case P_LIST_ITEM:
            case P_DESC_TEXT:
            case P_PARAGRAPH:
            ContainerCase:
                pContainer = m_pCreationCursor->FindContainerContainer();
                if( pContainer ){
                    m_pCreationCursor = pContainer;
                }
                else {
                    m_pCreationCursor = m_pRoot;
                }
                m_pCreationCursor = pElement = new CEditContainerElement( m_pCreationCursor, 
                        pTag, GetCurrentAlignment() );
                break;

            case P_UNUM_LIST:
            case P_NUM_LIST:
            case P_DESC_LIST:
            case P_MENU:
            case P_DIRECTORY:
            case P_BLOCKQUOTE:
                pContainer = m_pCreationCursor->FindContainerContainer();
                if( pContainer ){
                    m_pCreationCursor = pContainer;
                }
                else {
                    m_pCreationCursor = m_pRoot;
                }
                m_pCreationCursor = pElement = new CEditListElement( m_pCreationCursor, pTag);
                break;

            case P_BODY:
                ParseBodyTag(pTag);
                break;

            case P_HEAD:
                GetParseState()->m_bInHead = TRUE;
                break;

        // character formatting
            case P_SCRIPT:
                if( GetParseState()->m_bInHead ){
                    if( GetParseState()->m_pJavaScript == 0 ){
                        GetParseState()->m_pJavaScript = new CStreamOutMemory();
                        // Save the <SCRIPT> tag and its parameters
                        char* kScriptTag = "<SCRIPT";
                        GetParseState()->m_pJavaScript->Write(kScriptTag, XP_STRLEN(kScriptTag));
                        char *locked_buff;
                        PA_LOCK(locked_buff, char *, pTag->data );
                        GetParseState()->m_pJavaScript->Write(locked_buff, XP_STRLEN(locked_buff));
                        PA_UNLOCK(pTag->data);
                    }
                    GetParseState()->m_bInJavaScript = TRUE;
                    return OK_IGNORE;
                }
                // intentionally fall through

            case P_SERVER:

                // For either P_SCRIPT or P_SERVER
                if( ! GetParseState()->m_bInHead ){
                    // Save tag parameters.
                    if ( GetParseState()->m_pNextText ) {
                       char *locked_buff;
                       PA_LOCK(locked_buff, char *, pTag->data );
                       GetParseState()->m_pNextText->SetScriptExtra(locked_buff);
                       PA_UNLOCK(pTag->data);
                    }
                }
                m_preformatLinePos = 0;
                // intentionally fall through.
            case P_BOLD:
            case P_STRONG:
            case P_ITALIC:
            case P_EMPHASIZED:  
            case P_VARIABLE:
            case P_CITATION:
            case P_FIXED:
            case P_CODE:
            case P_KEYBOARD:
            case P_SAMPLE:
            case P_SUPER:
            case P_SUB:
            case P_STRIKEOUT:
            case P_STRIKE:
            case P_UNDERLINE:
            case P_BLINK:
                GetParseState()->m_formatTextStack.Push(GetParseState()->m_pNextText);
                GetParseState()->m_formatTypeStack.Push(pTag->type);
                GetParseState()->m_pNextText = GetParseState()->m_pNextText->CopyEmptyText();
                GetParseState()->m_pNextText->m_tf |= edt_TagType2TextFormat(pTag->type);
                break;

            case P_SMALL:
                GetParseState()->m_formatTextStack.Push(GetParseState()->m_pNextText);
                GetParseState()->m_formatTypeStack.Push(pTag->type);
                GetParseState()->m_pNextText = GetParseState()->m_pNextText->CopyEmptyText();
                if( GetParseState()->m_pNextText->m_tf & TF_FONT_SIZE ){
                    GetParseState()->m_pNextText->SetFontSize(GetParseState()->m_pNextText->GetFontSize()-1);
                }
                else {
                    GetParseState()->m_pNextText->SetFontSize(m_pCreationCursor->GetDefaultFontSize()-1);
                }
                break;

            case P_BIG:
                GetParseState()->m_formatTextStack.Push(GetParseState()->m_pNextText);
                GetParseState()->m_formatTypeStack.Push(pTag->type);
                GetParseState()->m_pNextText = GetParseState()->m_pNextText->CopyEmptyText();
                if( GetParseState()->m_pNextText->m_tf & TF_FONT_SIZE ){
                    GetParseState()->m_pNextText->SetFontSize(GetParseState()->m_pNextText->GetFontSize()+1);
                }
                else {
                    GetParseState()->m_pNextText->SetFontSize(m_pCreationCursor->GetDefaultFontSize()+1);
                }
                break;
            
            case P_BASEFONT:
                 GetParseState()->m_baseFontSize = edt_FetchParamInt(pTag, PARAM_SIZE, 3);
                 GetParseState()->m_pNextText->SetFontSize(GetParseState()->m_baseFontSize);
                break;

            case P_FONT:
                {
                    GetParseState()->m_formatTextStack.Push(GetParseState()->m_pNextText);
                    GetParseState()->m_formatTypeStack.Push(pTag->type);
                    GetParseState()->m_pNextText = GetParseState()->m_pNextText->CopyEmptyText();
                    buff = PA_FetchParamValue(pTag, PARAM_SIZE);
                    if (buff != NULL) {
                        char *size_str;
                        PA_LOCK(size_str, char *, buff);
                        GetParseState()->m_pNextText->SetFontSize(LO_ChangeFontSize( 
                                GetParseState()->m_baseFontSize, size_str ));
                        PA_UNLOCK(buff);
                        PA_FREE(buff);
                    }
				    buff = PA_FetchParamValue(pTag, PARAM_FACE);
				    if (buff != NULL)
				    {
                        char* str;
				        PA_LOCK(str, char *, buff);
                        GetParseState()->m_pNextText->SetFontFace(str);
				        PA_UNLOCK(buff);
				        PA_FREE(buff);
				    }

                    ED_Color c = edt_FetchParamColor( pTag, PARAM_COLOR );
                    if( c.IsDefined() ){
                        GetParseState()->m_pNextText->SetColor( c );
                    }
                }
                break;

            case P_ANCHOR:
                // push the formatting stack in any case so we properly pop
                //  it in the future.
                GetParseState()->m_formatTextStack.Push(GetParseState()->m_pNextText);
                GetParseState()->m_formatTypeStack.Push(pTag->type);
                GetParseState()->m_pNextText = GetParseState()->m_pNextText->CopyEmptyText();

                pStr = edt_FetchParamString(pTag, PARAM_HREF);
                if( pStr != NULL ){
                    // collect extra stuff..
                    // LTNOTE:
                    char* pExtra = edt_FetchParamExtras( pTag, anchorHrefParams );
                    ED_LinkId id = linkManager.Add( pStr, pExtra );
                    GetParseState()->m_pNextText->SetHREF( id );
                    linkManager.Free( id );         // reference counted.
                    XP_FREE( pStr );
                }

                pStr = edt_FetchParamString(pTag, PARAM_NAME );
                if( pStr ){
                    // We don't catch this case above because by default
                    //  an anchor is a character formatting
                    if( m_pCreationCursor->FindContainer() == 0 ){
                        m_pCreationCursor = CEditContainerElement::NewDefaultContainer( 
                                m_pCreationCursor, GetCurrentAlignment() );
                    }
                    pElement = new CEditTargetElement(m_pCreationCursor, pTag);
                    m_pCreationCursor = pElement->GetParent();
                    XP_FREE( pStr );
                }
                break;

            case P_IMAGE:
            case P_NEW_IMAGE:
                {
                    pElement = new CEditImageElement(m_pCreationCursor, pTag, 
                                GetParseState()->m_pNextText->GetHREF());
                    m_pCreationCursor = pElement->GetParent();
                    GetParseState()->bLastWasSpace = FALSE;
                    break;
                }                

            case P_HRULE:
                {
                pElement = new CEditHorizRuleElement(m_pCreationCursor, pTag);
                m_pCreationCursor = pElement->GetParent();
                GetParseState()->bLastWasSpace = TRUE;
                break;
                }                

            case P_PLAIN_TEXT:
            case P_PLAIN_PIECE:
            case P_LISTING_TEXT:
            case P_PREFORMAT:
                pTag->type = P_PREFORMAT;
                m_preformatLinePos = 0;
                goto ContainerCase;


            //
            // pass these tags on so the document looks right
            //
            case P_CENTER:
                GetParseState()->m_formatAlignStack.Push(ED_ALIGN_CENTER);
                pContainer = m_pCreationCursor->FindContainer();
                if( pContainer ){
                    pContainer->Container()->AlignIfEmpty( ED_ALIGN_CENTER );
                }
                break;

            case P_DIVISION:
                {
                ED_Alignment eAlign = edt_FetchParamAlignment( pTag, ED_ALIGN_LEFT );
                GetParseState()->m_formatAlignStack.Push( eAlign );
                pContainer = m_pCreationCursor->FindContainer();
                if( pContainer ){
                    pContainer->Container()->AlignIfEmpty( eAlign );
                }
                //PA_FREE( pTag );
                //return OK_IGNORE;
                break;
                }
                
            case P_LINEBREAK:
                {
                pElement = new CEditBreakElement(m_pCreationCursor, pTag);
                m_pCreationCursor = pElement->GetParent();
                GetParseState()->bLastWasSpace = TRUE;
                break;
                }                
                
            case P_TITLE:
                GetParseState()->m_bInTitle = TRUE;
                break;

            case P_META:
                ParseMetaTag( pTag );
                break;

// If you add something to this case, also add it to edt_setUnsupported
            case P_TEXTAREA:
            case P_SELECT:
            case P_OPTION:
            case P_INPUT:
            case P_EMBED:
            case P_NOEMBED:
            case P_JAVA_APPLET:
            case P_FORM:
            case P_MAP:
            case P_AREA:
            case P_KEYGEN:
            case P_BASE:
            case P_HYPE:
            case P_LINK:
            case P_SUBDOC:
            case P_CELL:
            case P_WORDBREAK:
            case P_NOBREAK:
            case P_COLORMAP:
            case P_INDEX:
            case P_PARAM:
            case P_SPACER:
            case P_UNKNOWN:
            case P_NOSCRIPT:
            case P_CERTIFICATE:
            case P_PAYORDER:
#ifndef SUPPORT_MULTICOLUMN
            case P_MULTICOLUMN:
#endif
               if ( IsDocTypeTag(pTag) ) {
                    break; // Ignore document's theorey about it's document type.
                }

                if( !m_pCreationCursor->IsContainer() && m_pCreationCursor->FindContainer() == 0 ){
                    m_pCreationCursor = CEditContainerElement::NewDefaultContainer( 
                            m_pCreationCursor, GetCurrentAlignment() );
                }
                m_pCreationCursor = pElement = new CEditIconElement( m_pCreationCursor, 
                            pTag->is_end ? 
                                EDT_ICON_UNSUPPORTED_END_TAG
                            :
                                EDT_ICON_UNSUPPORTED_TAG, 
                            pTag );
                m_pCreationCursor->Icon()->MorphTag( pTag );
                m_pCreationCursor = m_pCreationCursor->GetParent();
                break;

            case P_TABLE:
                pContainer = m_pCreationCursor->FindContainerContainer();
                if( pContainer ){
                    m_pCreationCursor = pContainer;
                }
                else {
                    m_pCreationCursor = m_pRoot;
                }
                m_pCreationCursor = pElement = new CEditTableElement( m_pCreationCursor, pTag, GetCurrentAlignment() );
                break;

            case P_TABLE_ROW:
                {
                    CEditTableElement* pTable = m_pCreationCursor->GetTable();
                    if ( ! pTable ){
                        // We might be in a TD that someone forgot to close.
                        CEditTableCellElement* pCell = m_pCreationCursor->GetTableCell();
                        if ( pCell ){
                            pTable = pCell->GetTable();
                        }
                    }
                    if ( pTable ) {
                        m_pCreationCursor = pElement = new CEditTableRowElement( pTable, pTag );
                    }
                    else {
                        XP_TRACE(("Ignoring table row. Not in table."));
                    }
                }
                break;

            case P_TABLE_HEADER:
            case P_TABLE_DATA:
                {
                    CEditTableElement* pTable = m_pCreationCursor->GetTable();
                    if ( ! pTable ){
                        // We might be in a TD that someone forgot to close.
                        CEditTableCellElement* pCell = m_pCreationCursor->GetTableCell();
                        if ( pCell ){
                            m_pCreationCursor = pCell->GetParent();
                            pTable = m_pCreationCursor->GetTable();
                        }
                    }
                    if ( ! pTable ) {
                        XP_TRACE(("Ignoring table cell. Not in table."));
                    }
                    else {
                        CEditTableRowElement* pTableRow = m_pCreationCursor->GetTableRow();
                        if ( ! pTableRow ){
                            // They forgot to put in a table row.
                            // Create one for them.

                            pTableRow = new CEditTableRowElement();
                            pTableRow->InsertAsLastChild(pTable);
                        }
                        m_pCreationCursor = pElement = new CEditTableCellElement( pTableRow, pTag );
                        PushParseState();
                    }
                }
                break;

            case P_CAPTION:
                {
                    CEditTableElement* pTable = m_pCreationCursor->GetTable();
                    if ( pTable ) {
                        m_pCreationCursor = pElement = new CEditCaptionElement( pTable, pTag );
                        PushParseState();
                    }
                    else {
                        XP_TRACE(("Ignoring caption. Not in a table."));
                    }
                }
                break;

#ifdef SUPPORT_MULTICOLUMN

            case P_MULTICOLUMN:
                pContainer = m_pCreationCursor->FindContainerContainer();
                if( pContainer ){
                    m_pCreationCursor = pContainer;
                }
                else {
                    m_pCreationCursor = m_pRoot;
                }
                m_pCreationCursor = pElement = new CEditMultiColumnElement( m_pCreationCursor, pTag );
                break;
#endif

            // unimplemented Tags

            case P_GRID:
            case P_GRID_CELL:
            case P_NOGRIDS:
            case P_MAX:
                m_pCreationCursor = pElement = CreateElement( pTag, m_pCreationCursor );
                if( BitSet( edt_setSoloTags, m_pCreationCursor->GetType() ) ){
                    m_pCreationCursor = m_pCreationCursor->GetParent();
                }
                break;

            //
            // someones added a new tag.  Inspect pTag and look in pa_tags.h
            //
            default:
                XP_ASSERT(FALSE);
                break;

            // its ok to ignore this.
            case P_HTML:
                break;
        }

    }
    pTag->edit_element = pElement;
    return retVal;
}

void CEditBuffer::ParseBodyTag(PA_Tag *pTag){
    // Allow multiple body tags -- don't replace parameter a new one isn't found.
    edt_FetchParamString2(pTag, PARAM_BACKGROUND, m_pBackgroundImage);
    edt_FetchParamColor2( pTag, PARAM_BGCOLOR, m_colorBackground);
    edt_FetchParamColor2( pTag, PARAM_TEXT, m_colorText );
    edt_FetchParamColor2( pTag, PARAM_LINK, m_colorLink );
    edt_FetchParamColor2( pTag, PARAM_ALINK, m_colorActiveLink );
    edt_FetchParamColor2( pTag, PARAM_VLINK, m_colorFollowedLink );
    //m_pBody = m_pCreationCursor = pElement = CreateElement( pTag, m_pCreationCursor );
    edt_FetchParamExtras2( pTag, bodyParams, m_pBodyExtra );
}

// Relayout timer ... doesn't quite work perfectly.

CRelayoutTimer::CRelayoutTimer(){
    m_pBuffer = NULL;
}

void CRelayoutTimer::SetEditBuffer(CEditBuffer* pBuffer){
    m_pBuffer = pBuffer;
}

void CRelayoutTimer::OnCallback() {
    m_pBuffer->Relayout(m_pStartElement, m_iOffset);
}

void CRelayoutTimer::Flush() {
    if (IsTimeoutEnabled() ) {
        Cancel();
        OnCallback();
    }
}

void CRelayoutTimer::Relayout( CEditElement *pStartElement, int iOffset)
{
	const uint32 kRelayoutDelay = 50;
    XP_Bool bSetValues = TRUE;
    XP_Bool bInTable = pStartElement->GetTableIgnoreSubdoc() != NULL;

    if ( ! bInTable ) {
        Flush();
        m_pBuffer->Relayout(pStartElement, iOffset);
        return;
    }
	if ( IsTimeoutEnabled() ) {
		Cancel();
        if ( pStartElement != m_pStartElement ) {
            OnCallback(); // Do saved relayout now.
        }
        else {
            bSetValues = FALSE;
        }
	}
    if ( bSetValues) {
        m_pStartElement = pStartElement;
        m_iOffset = iOffset;
    }

    // Put the character into the LO_Element string so that the two data structures are consistent.
    if (pStartElement->IsText() ){
        CEditTextElement* pTextElement = pStartElement->Text();
        LO_TextStruct* pLoText = NULL;
        int iLoOffset = 0;
        if ( pTextElement->GetLOTextAndOffset(iOffset, FALSE, pLoText, iLoOffset) ) {
            if ( pLoText ) {
                int32 textLen = pLoText->text_len;
		        char *pText;
                if (textLen <= 0 ){
                    PA_FREE(pLoText->text);
                    pLoText->text = (PA_Block) PA_ALLOC(2); // char plus zero terminated.
                }
                else {
                    pLoText->text = (PA_Block) PA_REALLOC(pLoText->text, textLen + 2);
                }
		        PA_LOCK(pText, char *, pLoText->text);
                int32 bytesToMove = textLen - iLoOffset;
                if ( bytesToMove > 0){
                    XP_MEMMOVE(pText + iLoOffset +1, pText + iLoOffset, bytesToMove);
                }
                pText[iLoOffset] = pTextElement->GetText()[iOffset];
                pText[textLen+1] = '\0';
                // XP_TRACE(("pText=%08x %s %d\n", pText, pText, textLen));
		        PA_UNLOCK(pLoText->text);
                textLen++;
                pLoText->text_len = (int16) textLen;
            }
        }
    }
    SetTimeout(kRelayoutDelay);
}

CAutoSaveTimer::CAutoSaveTimer() {
     m_pBuffer = NULL;
     m_iMinutes = 0;
}

void CAutoSaveTimer::SetEditBuffer(CEditBuffer* pBuffer){
    m_pBuffer = pBuffer;
}

void CAutoSaveTimer::OnCallback() {
    m_pBuffer->AutoSaveCallback();
    Restart();
}

void CAutoSaveTimer::Restart() {
#ifdef DEBUG_AUTO_SAVE
    const int32 kMillisecondsPerMinute = 6000L; //60000L; // * 1000 / 10;
#else
    const int32 kMillisecondsPerMinute = 60000L;
#endif
    if ( m_iMinutes > 0 ) {
        SetTimeout(m_iMinutes * kMillisecondsPerMinute);
    }
}

void CAutoSaveTimer::SetPeriod(int32 minutes){
    Cancel();
    m_iMinutes = minutes;
    Restart();
}

int32 CAutoSaveTimer::GetPeriod(){
    return m_iMinutes;
}

static int iFileCount = 0;
CEditBuffer::CEditBuffer(MWContext *pContext): m_pContext(pContext), 
        m_colorText(ED_Color::GetUndefined()),
        m_colorBackground(ED_Color::GetUndefined()),
        m_colorLink(ED_Color::GetUndefined()),
        m_colorFollowedLink(ED_Color::GetUndefined()),
        m_colorActiveLink(ED_Color::GetUndefined()),
        m_pTitle(0),
        m_pBackgroundImage(0),
        m_iDesiredX(-1),
        m_bSelecting(0),
        m_bBlocked(TRUE),
        m_hackFontSize(0),
        m_lastTopY(0),
        m_inScroll(FALSE),
        m_bNoRelayout(FALSE),
        m_bDirty(0),
        m_preformatLinePos(0),
        m_bInPreformat(FALSE),
        m_iCurrentOffset(0), 
        m_pLoadingImage(0),
        m_pSaveObject(0),
        m_pCurrent(0),
        m_bCurrentStickyAfter(FALSE),
        m_pCreationCursor(0),
        m_pTypingCommand(0),
        m_bLayoutBackpointersDirty(TRUE),
        m_pBodyExtra(0),
        m_iFileWriteTime(0),
        m_bDisplayTables(TRUE),
        m_bDummyCharacterAddedDuringLoad(FALSE),
        m_status(ED_ERROR_NONE)
{
    // create a root element
    edt_InitBitArrays();
    m_pCreationCursor = m_pRoot = new CEditRootDocElement( this );
    ResetParseStateStack();

#ifdef DEBUG
    m_pTestManager = new CEditTestManager(this);
    m_bSuppressPhantomInsertPointCheck = FALSE;
    m_bSkipValidation = FALSE;
#endif

    m_relayoutTimer.SetEditBuffer(this);
    m_autoSaveTimer.SetEditBuffer(this);
#ifdef DEBUG_AUTO_SAVE
    m_autoSaveTimer.SetPeriod(1);
#endif

}

CEditBuffer::~CEditBuffer(){
#ifdef DEBUG
    delete m_pTestManager;
#endif
    delete m_pRoot;
    m_pRoot = NULL;
    // Delete meta data
    for(intn i = 0; i < m_metaData.Size(); i++ ) {
        FreeMetaData(m_metaData[i]);
    }
    ResetParseStateStack();
}

void CEditBuffer::FixupInsertPoint(){
    CEditInsertPoint ip(m_pCurrent, m_iCurrentOffset, m_bCurrentStickyAfter);
    FixupInsertPoint(ip);
    m_pCurrent = ip.m_pElement;
    m_iCurrentOffset = ip.m_iPos;
    m_bCurrentStickyAfter = ip.m_bStickyAfter;
    XP_ASSERT(m_bCurrentStickyAfter == TRUE || m_bCurrentStickyAfter == FALSE);
}

void CEditBuffer::FixupInsertPoint(CEditInsertPoint& ip){
    if( ip.m_iPos == 0 && ! IsPhantomInsertPoint(ip) ){
        CEditLeafElement *pPrev = ip.m_pElement->PreviousLeafInContainer();
        if( pPrev && pPrev->Leaf()->GetLen() != 0 ){
            ip.m_pElement = pPrev;
            ip.m_iPos = pPrev->Leaf()->GetLen();
        }
    }
    // Since we fake up spaces at the beginning of paragraph, we might get
    //  an offset of 1 (after the fake space).
    else if( ip.m_pElement->Leaf()->GetLen() == 0 ){
        ip.m_iPos = 0;
    }
}

void CEditBuffer::SetInsertPoint( CEditLeafElement* pElement, int iOffset, XP_Bool bStickyAfter ){
    LO_StartSelectionFromElement( m_pContext, 0, 0); 
    m_pCurrent = pElement;
    m_iCurrentOffset = iOffset;
    m_bCurrentStickyAfter = bStickyAfter;
    XP_ASSERT(m_bCurrentStickyAfter == TRUE || m_bCurrentStickyAfter == FALSE);
    SetCaret();
}

//
// Returns true if the insert point doesn't really exist.
//
XP_Bool CEditBuffer::IsPhantomInsertPoint(){
    if( !IsSelected() ) {
        CEditInsertPoint ip(m_pCurrent, m_iCurrentOffset, m_bCurrentStickyAfter);
        return IsPhantomInsertPoint(ip);
    }
    else {
        return FALSE;
    }
}

XP_Bool CEditBuffer::IsPhantomInsertPoint(CEditInsertPoint& ip){
    if(  ip.m_pElement
            && ip.m_pElement->IsA( P_TEXT ) 
            && ip.m_pElement->Text()->GetLen() == 0
            && !( ip.m_pElement->IsFirstInContainer() 
                    && ip.m_pElement->LeafInContainerAfter() == 0 )
                ){
        return TRUE;
    }
    else {
        return FALSE;
    }
}

//
// Force the insertpoint to be a real insert point so we can do something.
//
void CEditBuffer::ClearPhantomInsertPoint(){
    if( IsPhantomInsertPoint() ){
        CEditLeafElement *pPrev = m_pCurrent->PreviousLeafInContainer();
        if( pPrev ){
            m_pCurrent = pPrev;
            m_iCurrentOffset = m_pCurrent->Leaf()->GetLen();
            m_bCurrentStickyAfter = FALSE;
        }
        else {
            CEditLeafElement *pNext = m_pCurrent->LeafInContainerAfter();
            if( pNext ){
                m_pCurrent = pNext;
                m_iCurrentOffset = 0;
                m_bCurrentStickyAfter = FALSE;
           }
            else {
                XP_ASSERT(FALSE);
            }
        }
        Reduce( m_pCurrent->FindContainer() );
    }
}

CEditElement* CEditBuffer::FindRelayoutStart( CEditElement *pStartElement ){
    CEditElement* pOldElement = NULL;
    while ( pStartElement && pStartElement != pOldElement ) {
        pOldElement = pStartElement;
        CEditElement* pTable = pStartElement->GetTopmostTableOrMultiColumn();
        if ( m_bDisplayTables && pTable ) {
            // If this is in a table, skip before it.
            pStartElement = pTable->PreviousLeaf();
        }
        else if( !pStartElement->IsLeaf() ){
            pStartElement = pStartElement->PreviousLeaf();
        }
        else if( pStartElement->Leaf()->GetLayoutElement() == 0 ){
           pStartElement = pStartElement->PreviousLeaf();
        }
    }
    return pStartElement;
}

//
// Relayout.
//
void CEditBuffer::Relayout( CEditElement* pStartElement, 
                            int iEditOffset, 
                            CEditElement *pEndElement,
                            intn relayoutFlags ){
    CEditElement *pEdStart, *pNewStartElement;
    LO_Element *pLayoutElement;
    LO_Element *pLoStartLine;
    int iOffset;
    int32 iLineNum;

    if( m_bNoRelayout ){
        return;
    }

#if 0
    if ( pStartTopmostTable || pEndTopmostTable ) {
        // We don't know how to handle incremental table layout.
        // So we punt and redraw the whole table.
        pStartElement = m_pRoot; // hack
        iEditOffset = 0;
        pEndElement = m_pRoot->GetLastMostChild();
    }
#endif

    CEditLeafElement *pBegin, *pEnd;
    ElementOffset iBeginPos, iEndPos;
    XP_Bool bFromStart;
    XP_Bool bWasSelected = IsSelected();

    if( bWasSelected ){
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        LO_StartSelectionFromElement( m_pContext, 0, 0); 
    }

    pNewStartElement = FindRelayoutStart(pStartElement);
    if( pNewStartElement && pNewStartElement != pStartElement ){
        // we had to back up some.  Layout until we pass this point.        
        if( pEndElement == 0 ){
            pEndElement = pStartElement;
        }
        pStartElement = pNewStartElement;
        iEditOffset = pStartElement->Leaf()->GetLen();
    }

    // If the end is in a table, move it outside of the table.
    if ( pEndElement ) {
        CEditElement* pTable = pEndElement->GetTopmostTableOrMultiColumn();
        if ( m_bDisplayTables && pTable) {
            // If this is in a table, skip after it.
            pEndElement = pTable->GetLastMostChild()->NextLeaf();
        }
    }

    // laying out from the beginning of the document
    if( pNewStartElement == 0 ){
        if( pEndElement == 0 ){
            pEndElement = pStartElement;
        }
        iLineNum = 0;
        pEdStart = m_pRoot;
        iOffset = 0;
    }
    else {
        //
        // normal case
        //
        if( pStartElement->IsA(P_TEXT)){
            pLayoutElement = (LO_Element*)pStartElement->Text()->GetLOText( iEditOffset ); 
        }
        else {
            pLayoutElement = pStartElement->Leaf()->GetLayoutElement();
        }
        if( pLayoutElement == 0 ){
            // we are fucked! try something different.
            XP_TRACE(("Yellow Alert! Can't resync, plan B"));
            Relayout( pStartElement->FindContainer(), 0, pEndElement ? 
                        pEndElement : pStartElement, relayoutFlags  );
            return;
        }
        //
        // Find the first element on this line.
        pLoStartLine = LO_FirstElementOnLine( m_pContext, pLayoutElement->lo_any.x, 
                        pLayoutElement->lo_any.y, &iLineNum );

        //
        // Position the tag cursor at this position
        //
        pEdStart = pLoStartLine->lo_any.edit_element;
        iOffset = pLoStartLine->lo_any.edit_offset;
    }

 
    // Create a new cursor.
    CEditTagCursor cursor(this, pEdStart, iOffset, pEndElement);
    //CEditTagCursor *pCursor = new CEditTagCursor(this, m_pRoot, iOffset);

    LO_Relayout(m_pContext, &cursor, iLineNum, iOffset, m_bDisplayTables);

    // Search for zero-length text elements, and remove the non-breaking spaces we put
    // in.

    CEditElement* pElement= m_pRoot;
    while ( NULL != (pElement = pElement->FindNextElement(&CEditElement::FindLeafAll,0))){
        switch ( pElement->GetElementType() ) {
        case eTextElement:
            {
                CEditTextElement* pText = pElement->Text();
                intn iLen = pText->GetLen();
                if ( iLen == 0 ) {
                    LO_Element* pTextStruct = pText->GetLayoutElement();
                    if ( pTextStruct && pTextStruct->type == LO_TEXT
                            && pTextStruct->lo_text.text_len == 1
                            && pTextStruct->lo_text.text ) {
                        XP_ASSERT( pElement->Leaf()->PreviousLeafInContainer() == 0 );
                        XP_ASSERT( pElement->Leaf()->LeafInContainerAfter() == 0 );
                        // Need to strip the allocated text, because lo_bump_position cares.
                        pTextStruct->lo_text.text_len = 0;
                        PA_FREE(pTextStruct->lo_text.text);
                        pTextStruct->lo_text.text = NULL;
                    }
                }
                else {
                    // Layout stripped out all the spaces that crossed soft line breaks.
                    // Put them back in so they can be selected.
                    LO_Element* pTextStruct = pText->GetLayoutElement();
                    while ( pTextStruct && pTextStruct->type == LO_TEXT
                        && pTextStruct->lo_any.edit_element == pText ) {
                        intn iOffset = pTextStruct->lo_any.edit_offset + pTextStruct->lo_text.text_len;
                        if ( iOffset >= iLen )
                            break;
                        LO_Element* pNext = pTextStruct->lo_any.next;
                        while ( pNext && pNext->type != LO_TEXT) {
                            pNext = pNext->lo_any.next;
                        }
                        intn bytesToCopy = iLen - iOffset;
                        if ( pNext && pNext->lo_any.edit_element == pText ){
                           bytesToCopy =  pNext->lo_text.edit_offset - iOffset;
                        }
                        if ( bytesToCopy > 0 ) {
                            // Layout has stripped some characters.
                            // (Probably just a single space character.)
                            // Put them back.
                            int16 length = pTextStruct->lo_text.text_len;
                            int16 newLength = (int16) (length + bytesToCopy + 1); // +1 for '\0'
                            PA_Block newData;
                            if (pTextStruct->lo_text.text) {
                                newData  = (PA_Block) PA_REALLOC(pTextStruct->lo_text.text, newLength);
                            }
                            else {
                                newData = (PA_Block) PA_ALLOC(newLength);
                            }
                            if ( ! newData ) {
                                XP_ASSERT(FALSE); /* Out of memory. Not well tested. */
                                break;
                            }
                            pTextStruct->lo_text.text = newData;
                            pTextStruct->lo_text.text_len = (int16) (newLength - 1);
                            char *locked_buff;
                            PA_LOCK(locked_buff, char *, pTextStruct->lo_text.text);
                            if ( locked_buff ) {
                                char* source = pText->GetText();
                                for ( intn i = 0; i < bytesToCopy; i++ ){
                                    locked_buff[length + i] = source[iOffset+ i];
                                }
                                locked_buff[length + bytesToCopy] = '\0';
                            }
                            PA_UNLOCK(pNext->lo_text.text);
                            // The string is now wider. We must adjust the width.
                            LO_TextInfo text_info;
                            FE_GetTextInfo(m_pContext, &pTextStruct->lo_text, &text_info);
                            int32 delta = text_info.max_width - pTextStruct->lo_text.width;
                            pTextStruct->lo_text.width = text_info.max_width;
                            // Shrink the linefeed
                            LO_Element* pNext = pTextStruct->lo_any.next;
                            if ( pNext && pNext->type == LO_LINEFEED ) {
                                if ( pNext->lo_linefeed.width > delta ) {
                                    pNext->lo_linefeed.width -= delta;
                                    pNext->lo_linefeed.x += delta;
                                }
                            }
                        }
                        pTextStruct = pNext;
                    }
                }
            }
            break;
        case eBreakElement:
            {
                CEditBreakElement* pBreak = pElement->Break();
                LO_Element* pBreakStruct = pBreak->GetLayoutElement();
                if ( pBreakStruct && pBreakStruct->type == LO_LINEFEED) {
                    // Need to strip the allocated text, because lo_bump_position cares.
                    pBreakStruct->lo_linefeed.break_type = LO_LINEFEED_BREAK_HARD;
                    pBreakStruct->lo_linefeed.edit_element = pElement;
                    pBreakStruct->lo_linefeed.edit_offset = 0;
                }
            }
            break;
       case eHorizRuleElement:
             {
                // The linefeeds after hrules need to be widened.
                // They are zero pixels wide by default.
                CEditHorizRuleElement* pHorizRule = (CEditHorizRuleElement*) pElement;
                LO_Element* pHRuleElement = pHorizRule->GetLayoutElement();
                if ( pHRuleElement ) {
                    LO_Element* pNext = pHRuleElement->lo_any.next;
                    if ( pNext && pNext->type == LO_LINEFEED) {
                        const int32 kMinimumWidth = 7;
                        if (pNext->lo_linefeed.width < kMinimumWidth ) {
                             pNext->lo_linefeed.width = kMinimumWidth;
                        }
                    }
                }
            }
            break;
      default:
            break;
        }
        // Set end-of-paragraph marks
        if ( pElement->GetNextSibling() == 0 ){
            // We're the last leaf in the container.
            CEditLeafElement* pLeaf = pElement->Leaf();
            LO_Element* pLastElement;
            int iOffset;
            if ( pLeaf->GetLOElementAndOffset(pLeaf->GetLen(), TRUE,
                pLastElement, iOffset) ){
                LO_Element* pNextElement = pLastElement ? pLastElement->lo_any.next : 0;
                if ( pNextElement && pNextElement->type == LO_LINEFEED ) {
                    pNextElement->lo_linefeed.break_type = LO_LINEFEED_BREAK_PARAGRAPH;
                }
            }
            else {
                // Last leaf, but we're empty. Yay. Try the previous leaf.
                CEditElement* pPrevious = pElement->GetPreviousSibling();
                if ( pPrevious ) {
                    CEditLeafElement* pLeaf = pPrevious->Leaf();
                    LO_Element* pLastElement;
                    int iOffset;
                    if ( pLeaf->GetLOElementAndOffset(pLeaf->GetLen(), TRUE,
                        pLastElement, iOffset) ){
                        LO_Element* pNextElement = pLastElement ? pLastElement->lo_any.next : 0;
                        if ( pNextElement && pNextElement->type == LO_LINEFEED ) {
                            pNextElement->lo_linefeed.break_type = LO_LINEFEED_BREAK_PARAGRAPH;
                        }
                    }
                }
            }
        }
    }
    if( (relayoutFlags & RELAYOUT_NOCARET) == 0 && !bWasSelected){
        SetCaret();
    }

    if( bWasSelected ){
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
    }

    m_bLayoutBackpointersDirty = FALSE;
}

//
// Insert a character at the current edit point
//
EDT_ClipboardResult CEditBuffer::InsertChar( int newChar, XP_Bool bTyping ){
    VALIDATE_TREE(this);
    EDT_ClipboardResult result = EDT_COP_OK;
    int iChangeOffset = m_iCurrentOffset;

#ifdef DEBUG
    if ( bTyping && m_pTestManager->Key((char) newChar) )
        return EDT_COP_OK;
#endif

    CTypingCommand* cmd = NULL;
    if ( bTyping ) {
        cmd = GetTypingCommand();
    }

    if( IsSelected() ){
        if ( bTyping ) {
            result = TypingDeleteSelection(cmd);
        }
        else {
            result = DeleteSelection();
        }
        if ( result != EDT_COP_OK ) return result;
    }
    //ClearSelection();

    SetDirtyFlag( TRUE );
    ClearMove(FALSE);
    if(!IsPhantomInsertPoint() ){
        FixupInsertPoint();
    }
    if( m_pCurrent->IsA( P_TEXT )){

        // Check for space after space case.
        if ( newChar == ' ' && !m_pCurrent->InFormattedText() ) {
            CEditInsertPoint p(m_pCurrent, m_iCurrentOffset);
            // move to the right on spacebar if we are under a space.
            if( p.IsSpace() ){
                NextChar( FALSE );
                return result;
            }
            if ( p.IsSpaceBeforeOrAfter() ) {
                return result;
            }
        }

        if ( bTyping ) {
            cmd->AddCharStart();
        }
        //
        // The edit element can choose not to insert a character if it is 
        //  a space and at the insertion point there already is a space.
        //
        if( m_pCurrent->Text()->InsertChar( m_iCurrentOffset, newChar ) ){
    
            // move past the inserted character
            m_iCurrentOffset++;
            if ( bTyping ) {
                cmd->AddCharEnd();
            }

            // Reduce or die
            Reduce(m_pCurrent->FindContainer());
            // relay out the stream.
            if ( bTyping ) {
                m_relayoutTimer.Relayout(m_pCurrent, iChangeOffset);
            }
            else {
                Relayout(m_pCurrent, iChangeOffset);
            }
        }
    }
    else {
        Reduce( m_pCurrent->FindContainer());
        if( m_iCurrentOffset == 0 ){
            // insert before leaf case.
            CEditElement *pPrev = m_pCurrent->GetPreviousSibling();
            if( pPrev == 0 || !pPrev->IsA( P_TEXT ) ){
                pPrev = new CEditTextElement((CEditElement*)0,0);
                pPrev->InsertBefore( m_pCurrent );
            }
            m_pCurrent = pPrev->Leaf();
            m_iCurrentOffset = m_pCurrent->Text()->GetLen();
        }
        else {
            XP_ASSERT( m_iCurrentOffset == 1 );
            // insert after leaf case
            CEditLeafElement *pNext = (CEditLeafElement*) m_pCurrent->GetNextSibling();
            if( pNext == 0 || !pNext->IsA( P_TEXT ) ){
                CEditTextElement* pPrev = m_pCurrent->PreviousTextInContainer();
                if( pPrev ){
                    pNext = pPrev->CopyEmptyText();
                }
                else {
                    pNext = new CEditTextElement((CEditElement*)0,0);
                }
                pNext->InsertAfter( m_pCurrent );
            }
            m_pCurrent = pNext;
            m_iCurrentOffset = 0;
        }
        // now we have a text.  Do the actual insert.
        if ( bTyping ) {
            cmd->AddCharStart();
        }
        if( m_pCurrent->Text()->InsertChar( m_iCurrentOffset, newChar ) ){
            m_iCurrentOffset++;        
            if ( bTyping ) {
                cmd->AddCharEnd();
            }
            if ( bTyping ) {
                m_relayoutTimer.Relayout(m_pCurrent, iChangeOffset);
            }
            else {
                Relayout(m_pCurrent, iChangeOffset);
            }
        }
    }
    return result;
}

EDT_ClipboardResult CEditBuffer::DeletePreviousChar(){
#ifdef DEBUG
    if ( m_pTestManager->Backspace() )
        return EDT_COP_OK;
#endif
    return DeleteChar(FALSE);
}

EDT_ClipboardResult CEditBuffer::DeleteChar(XP_Bool bForward, XP_Bool bTyping){
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();
    ClearMove();
    CTypingCommand* cmd = bTyping ? GetTypingCommand() : NULL;
    EDT_ClipboardResult result = EDT_COP_OK;
    if( IsSelected() ){
        result = TypingDeleteSelection(cmd);
    }
    else {
        CEditSelection selection;
        GetSelection(selection);
        if ( Move(*selection.GetEdge(bForward), bForward ) ) {
            result = CanCut(selection, TRUE);
            if ( result == EDT_COP_OK ) {
                if ( bTyping ) {
                    cmd->DeleteStart(selection);
                }
                DeleteSelection(selection, FALSE);
                if ( bTyping ) {
                    cmd->DeleteEnd();
                }
            }
        }
    }
    return result;
}

EDT_ClipboardResult CEditBuffer::TypingDeleteSelection(CTypingCommand* cmd){
    CEditSelection selection;
    GetSelection(selection);
    selection.ExcludeLastDocumentContainerEnd();
    cmd->DeleteStart(selection);
    EDT_ClipboardResult result = DeleteSelection();
    cmd->DeleteEnd();
    return result;
}

EDT_ClipboardResult CEditBuffer::DeleteNextChar(){
    return DeleteChar(TRUE);
}

XP_Bool CEditBuffer::Move(CEditInsertPoint& pt, XP_Bool forward) {
    CEditLeafElement* dummyNewElement = pt.m_pElement;
    ElementOffset dummyNewOffset = pt.m_iPos;
    XP_Bool result = FALSE;
    if ( forward )
        result = NextPosition(dummyNewElement, dummyNewOffset,
            dummyNewElement, dummyNewOffset);
    else
        result = PrevPosition(dummyNewElement, dummyNewOffset,
            dummyNewElement, dummyNewOffset);
    if ( result ) {
        pt.m_pElement = dummyNewElement;
        pt.m_iPos = dummyNewOffset;
    }
    return result;
}

XP_Bool CEditBuffer::CanMove(CEditInsertPoint& pt, XP_Bool forward) {
    CEditInsertPoint test(pt);
    return Move(test, forward);
}

XP_Bool CEditBuffer::CanMove(CPersistentEditInsertPoint& pt, XP_Bool forward) {
    CPersistentEditInsertPoint test(pt);
    return Move(test, forward);
}


XP_Bool CEditBuffer::Move(CPersistentEditInsertPoint& pt, XP_Bool forward) {
    CEditInsertPoint insertPoint = PersistentToEphemeral(pt);
    XP_Bool result = Move(insertPoint, forward);
    if ( result ) {
        pt = EphemeralToPersistent(insertPoint);
    }
    return result;
}

void CEditBuffer::SelectNextChar( ){
    CEditLeafElement *pBegin, *pEnd;
    ElementOffset iBeginPos, iEndPos;
    XP_Bool bFromStart;
    XP_Bool bFound;

    if( IsSelected() ){
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        if( bFromStart ){
            bFound = NextPosition( pBegin, iBeginPos, pBegin, iBeginPos );
        }
        else {
            bFound = NextPosition( pEnd, iEndPos, pEnd, iEndPos );
        }
        if( bFound ){
            SelectRegion( pBegin, iBeginPos, pEnd, iEndPos, bFromStart, TRUE  );    
        }
    }
    else {
        BeginSelection( TRUE, FALSE );
    }
}


void CEditBuffer::SelectPreviousChar( ){
    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFound = FALSE;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        if( bFromStart ){
            bFound = PrevPosition( pBegin, iBeginPos, pBegin, iBeginPos );
        }
        else {
            bFound = PrevPosition( pEnd, iEndPos, pEnd, iEndPos );
        }
        if( bFound ){
            SelectRegion( pBegin, iBeginPos, pEnd, iEndPos, bFromStart, FALSE  );    
        }
    }
    else {
         BeginSelection( TRUE, TRUE );
    }
}

XP_Bool CEditBuffer::PrevPosition(CEditLeafElement *pEle, ElementOffset iOffset, 
                CEditLeafElement*& pNew, ElementOffset& iNewOffset ){

    LO_Element* pElement;
    int iLayoutOffset;
    int iNewLayoutOffset;
    
    CEditElement* pNewEditElement;
    if( pEle->GetLOElementAndOffset( iOffset, FALSE, pElement, iLayoutOffset ) ){
        XP_Bool result = LO_PreviousPosition( m_pContext, pElement, iLayoutOffset, &pNewEditElement, &iNewLayoutOffset );
        if ( result ){
            pNew = pNewEditElement->Leaf();
            iNewOffset = iNewLayoutOffset;
        }
        return result;
    }
    else {
        return pEle->PrevPosition( iOffset, pNew, iNewOffset );
    }
}

XP_Bool CEditBuffer::NextPosition(CEditLeafElement *pEle, ElementOffset iOffset, 
                CEditLeafElement*& pNew, ElementOffset& iNewOffset ){

    LO_Element* pElement;
    int iLayoutOffset;
    int iNewLayoutOffset;
    CEditElement* pNewEditElement;
   
    if( pEle->Leaf()->GetLOElementAndOffset( iOffset, FALSE, pElement, iLayoutOffset )){
        XP_Bool result = LO_NextPosition( m_pContext, pElement, iLayoutOffset, &pNewEditElement, &iNewLayoutOffset );
        // The insert point can't be the end of the document.
        if ( result && (
            ! pNewEditElement || pNewEditElement->IsEndOfDocument() ) ) {
            result = FALSE;
        }
        if ( result ){
            pNew = pNewEditElement->Leaf();
            iNewOffset = iNewLayoutOffset;
        }
        return result;
    }
    else {
        pEle->Leaf()->NextPosition( iOffset, pNew, iNewOffset );
        return TRUE;
    }
}

void CEditBuffer::NextChar( XP_Bool bSelect ){
    VALIDATE_TREE(this);
    int iSavePos = m_iCurrentOffset;
    
    if( bSelect ){
        SelectNextChar();
        return;
    }
    ClearPhantomInsertPoint();    
    ClearMove();
    ClearSelection();
    //FixupInsertPoint();
    NextPosition( m_pCurrent, m_iCurrentOffset, 
                    m_pCurrent, m_iCurrentOffset );
    SetCaret();
}



XP_Bool CEditBuffer::PreviousChar( XP_Bool bSelect ){
    VALIDATE_TREE(this);

    if( bSelect ){
        SelectPreviousChar();        
        return FALSE;       // no return value on select previous..
    }

    ClearPhantomInsertPoint();    
    ClearMove();
    ClearSelection();
    //FixupInsertPoint();
    if( PrevPosition( m_pCurrent, m_iCurrentOffset, 
                    m_pCurrent, m_iCurrentOffset )){
        SetCaret();
        return TRUE;
    }
    else {
        return FALSE;
    }
}


// 
// Find out our current layout position and move up or down from here.
//
void CEditBuffer::UpDown( XP_Bool bSelect, XP_Bool bForward ){
    VALIDATE_TREE(this);
    CEditLeafElement *pEle;
    ElementOffset iOffset;
    XP_Bool bStickyAfter;
    LO_Element* pElement;
    int iLayoutOffset;

    DoneTyping();
    
    BeginSelection();

    if( bSelect ){
        GetInsertPoint( &pEle, &iOffset, &bStickyAfter );
    }
    else {
        ClearSelection();
        ClearPhantomInsertPoint();
        pEle = m_pCurrent->Leaf();
        iOffset = m_iCurrentOffset;
        bStickyAfter = m_bCurrentStickyAfter;
    }
    
    XP_ASSERT(!pEle->IsEndOfDocument());
    pEle->GetLOElementAndOffset( iOffset, bStickyAfter, pElement, iLayoutOffset );

    LO_UpDown( m_pContext, pElement, iLayoutOffset, GetDesiredX(pEle, iOffset, bStickyAfter), bSelect, bForward );

    EndSelection();
}

int32
CEditBuffer::GetDesiredX(CEditElement* pEle, intn iOffset, XP_Bool bStickyAfter)
{
    XP_ASSERT(bStickyAfter == TRUE || bStickyAfter == FALSE);
    // m_iDesiredX is where we would move if we could.  This keeps the
    //  cursor moving, basically, straight up and down, even if there is
    //  no text or gaps
    if( m_iDesiredX == -1 ){
        LO_Element* pElement;
        int iLayoutOffset;
        pEle->Leaf()->GetLOElementAndOffset( iOffset, bStickyAfter, pElement, iLayoutOffset );
        int32 x;
        int32 y;
        int32 width;
        int32 height;
        LO_GetEffectiveCoordinates(m_pContext, pElement, iLayoutOffset, &x, &y, &width, &height);
        m_iDesiredX = x;
    }
    return m_iDesiredX;
}


void CEditBuffer::NavigateChunk( XP_Bool bSelect, intn chunkType, XP_Bool bForward ){
    if ( chunkType == EDT_NA_UPDOWN ){
        UpDown(bSelect, bForward);
    }
    else {
        VALIDATE_TREE(this);
        ClearPhantomInsertPoint();    
        ClearMove();    /* Arrow keys clear the up/down target position */
        BeginSelection();
        LO_NavigateChunk( m_pContext, chunkType, bSelect, bForward );
        EndSelection();
        DoneTyping();
    }
}

//
// Break current container into two containers.
//
EDT_ClipboardResult CEditBuffer::ReturnKey(XP_Bool bTyping){
    EDT_ClipboardResult result = EDT_COP_OK;
    if ( bTyping ) {
        VALIDATE_TREE(this);
#ifdef DEBUG
        if ( m_pTestManager->ReturnKey() )
            return result;
#endif
        CTypingCommand* cmd = GetTypingCommand();
        if( IsSelected() ){
            result = TypingDeleteSelection(cmd);
            if ( result != EDT_COP_OK ) return result;
        }
        cmd->AddCharStart();
        result = InternalReturnKey(TRUE);
        cmd->AddCharEnd();
    }
    else {
        result = InternalReturnKey(TRUE);
    }
    return result;
}

EDT_ClipboardResult CEditBuffer::InternalReturnKey(XP_Bool bUserTyped){
    EDT_ClipboardResult result = EDT_COP_OK;
    SetDirtyFlag( TRUE );

    if( IsSelected() ){
        result = DeleteSelection();
        if ( result != EDT_COP_OK ) return result;
    }

    // ClearSelection();
    ClearPhantomInsertPoint();
    FixupInsertPoint();

    CEditElement *pChangedElement = m_pCurrent;
    CEditLeafElement *pNew,*pSplitAfter;
    CEditLeafElement *pCopyFormat;
    int iChangedOffset = m_iCurrentOffset;

    pSplitAfter = m_pCurrent;        
    pCopyFormat = m_pCurrent->Leaf();

    if( m_iCurrentOffset == 0 ){
        // split at beginning of element
        if( m_pCurrent->PreviousLeafInContainer() == 0){
            CEditTextElement *pNew;
            pNew = m_pCurrent->Leaf()->CopyEmptyText();
            pNew->InsertBefore( m_pCurrent );
            pChangedElement = pNew->FindContainer();
            pSplitAfter = pNew;
        }

    }
    else {
        pNew = m_pCurrent->Divide( m_iCurrentOffset )->Leaf();
        /* Get rid of space at start of new line if user typed. */
        if( bUserTyped && !pNew->InFormattedText() 
                && pNew->IsA(P_TEXT)
                && pNew->Text()->GetLen() 
                && pNew->Text()->GetText()[0] == ' ' ){
            GetTypingCommand()->AddCharDeleteSpace();
            pNew->Text()->DeleteChar(m_pContext, 0);
            CEditLeafElement *pNext;
            if( pNew->Text()->GetLen() == 0 
                    && (pNext = pNew->TextInContainerAfter()) != 0 ){
                pNew->Unlink();
                // Should we delete pNew?
                delete pNew;
                pNew = pNext;
            }
        }
        m_pCurrent = pNew;
    }
    m_iCurrentOffset = 0;        
    m_pCurrent->GetParent()->Split( pSplitAfter, 0, 
                &CEditElement::SplitContainerTest, 0 );

    if ( bUserTyped && m_pCurrent->GetLen() == 0 && m_pCurrent->TextInContainerAfter() == 0 ){
        // An empty paragraph. If the style is one of the
        // heading styles, set it back to normal
        CEditContainerElement* pContainer = m_pCurrent->FindContainer();
        TagType tagType = pContainer->GetType();
        // To do - use one of those fancy bit arrays.
        if ( tagType >= P_HEADER_1 && tagType <=  P_HEADER_6){
            pContainer->SetType(P_PARAGRAPH);
        }
    }

    if ( bUserTyped ) {
        Reduce(m_pRoot); // Or maybe just the two containers?
    }

    Relayout(pChangedElement, iChangedOffset, 
        (pChangedElement != m_pCurrent ? m_pCurrent : 0 ));

#if 0
    // The bug is that after a return is pressed, the formatting is wrong 
    //  for the new paragrah.  Presents a problem.  If we insert an empty
    //  text element here, we get messed up because we may not be able to
    //  position the caret.  We end up reducing and loosing a pointer to
    //  the layout element.  It is a hard problem.

    CEditTextElement *pNewText = pCopyFormat->CopyEmptyText();
    pNewText->InsertBefore(m_pCurrent);
    m_pCurrent = pNewText;
    m_iCurrentOffset = 0;
#endif

    return result;
}

void CEditBuffer::Indent(){
    VALIDATE_TREE(this);
    SetDirtyFlag( TRUE );
    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd, *pCurrent;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        pCurrent = pBegin;
        XP_Bool bDone = FALSE;
        CEditContainerElement* pLastContainer = 0;
        CEditContainerElement* pContainer = 0;
        CEditListElement* pList;
        do {
            pCurrent->FindList( pContainer, pList );
            if( pContainer != pLastContainer ){
                IndentContainer( pContainer, pList );
                pLastContainer = pContainer;
            }
            bDone = (pEnd == pCurrent );    // For most cases
            pCurrent = pCurrent->NextLeafAll();
            bDone = bDone || (iEndPos == 0 && pEnd == pCurrent ); // Pesky edge conditions!
        } while( pCurrent && !bDone );

        // force layout stop displaying the current selection.
        LO_StartSelectionFromElement( m_pContext, 0, 0); 
        // probably needs to be the common ancestor.
        Relayout( pBegin->FindContainer(), 0, pEnd, RELAYOUT_NOCARET );
        // Need to force selection.
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        return;
    }

    //
    // find where cont-cont has cont
    //
    CEditContainerElement* pContainer;
    CEditListElement* pList;
    m_pCurrent->FindList( pContainer, pList );
    IndentContainer( pContainer, pList );
    Relayout( pContainer, 0, pContainer->GetLastMostChild() );
}

void CEditBuffer::IndentContainer( CEditContainerElement *pContainer, 
        CEditListElement *pList ){
    VALIDATE_TREE(this);

    CEditElement *pPrev = pContainer->GetPreviousSibling();
    CEditElement *pNext = pContainer->GetNextSibling();


    //
    // case 1
    //
    //     UL:
    //         LI:
    //     LI:         <- Indenting this guy
    //     LI:
    //     LI:
    //
    //  should result in
    //
    //     UL:
    //         LI:
    //         LI:     <-end up here         
    //     LI:
    //     LI:
    //
    //
    //  We also need to handle the case
    //
    //      UL:
    //          LI:
    //      LI:         <- indent this guy
    //      UL:         
    //          LI:
    //          LI:
    //
    //  and the second UL becomes redundant so we eliminate it.
    //
    //      UL:
    //          LI:
    //          LI:     <-Ends up here
    //          LI:     <-frm the redundant container
    //          LI:
    //
    if( pPrev && pPrev->IsList() ){
        CEditElement *pChild = pPrev->GetLastChild();
        if( pChild ){
            pContainer->Unlink();
            pContainer->InsertAfter( pChild );
        }
        else {
            pContainer->Unlink();
            pContainer->InsertAsFirstChild( pPrev );
        }
        if( pNext && pNext->IsList() ){
            pPrev->Merge( pNext );
        }
    }
    // 
    // case 0
    //
    //      LI:             <- Indenting this guy
    //
    // should result in
    //
    //      UL:
    //          LI:
    //
    //
    else if( pList == 0 ){
        PA_Tag *pTag = XP_NEW( PA_Tag );
        XP_BZERO( pTag, sizeof( PA_Tag ) );
        pTag->type = P_UNUM_LIST;
        CEditElement *pEle = new CEditListElement( 0, pTag );
        PA_FreeTag( pTag );

        pEle->InsertAfter( pContainer );
        pContainer->Unlink();
        pContainer->InsertAsFirstChild( pEle );
        return;
    }
    //
    // case 2
    //
    //     UL:
    //          LI:
    //          LI:         <- Indenting this guy
    //          UL:
    //              LI:
    //          LI:
    //          LI:
    //
    //  should result in
    //
    //     UL:
    //          LI:
    //          UL:
    //              LI:     <- Ends up here.
    //              LI:     
    //          LI:
    //          LI:
    //
    else if( pNext && pNext->IsList() ){
        CEditElement *pChild = pNext->GetChild();
        if( pChild ){
            pContainer->Unlink();
            pContainer->InsertBefore( pChild );
        }
        else {
            pContainer->Unlink();
            pContainer->InsertAsFirstChild( pPrev );
        }
    }
    //
    // case 3
    //
    //     UL:
    //          LI:
    //          LI:         <- Indenting this guy
    //          LI:
    //          LI:
    //
    //  should result in
    //
    //     UL:
    //          LI:
    //          UL:
    //              LI:     <- Ends up here.
    //          LI:
    //          LI:
    //
    else {    
        CEditElement *pNewList;
        pNewList = pList->Clone(0);
        
        
        // insert the new cont-cont between the old one and the cont
        pNewList->InsertBefore( pContainer );
        pContainer->Unlink();
        pContainer->InsertAsFirstChild( pNewList);
    }
}

void CEditBuffer::Outdent(){
    VALIDATE_TREE(this);
    SetDirtyFlag( TRUE );
    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd, *pCurrent;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        pCurrent = pBegin;
        XP_Bool bDone = FALSE;
        CEditContainerElement* pLastContainer = 0;
        CEditContainerElement* pContainer = 0;
        CEditListElement* pList;
        do {
            pCurrent->FindList( pContainer, pList );
            if( pContainer != pLastContainer ){
                OutdentContainer( pContainer, pList );
                pLastContainer = pContainer;
            }
            bDone = (pEnd == pCurrent );    // For most cases
            pCurrent = pCurrent->NextLeafAll();
            bDone = bDone || (iEndPos == 0 && pEnd == pCurrent ); // Pesky edge conditions!
        } while( pCurrent && !bDone );

        // force layout stop displaying the current selection.
        LO_StartSelectionFromElement( m_pContext, 0, 0); 
        // probably needs to be the common ancestor.
        Relayout( pBegin->FindContainer(), 0, pEnd, RELAYOUT_NOCARET );
        // Need to force selection.
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        return;
    }

    CEditContainerElement* pContainer;
    CEditListElement* pList;
    m_pCurrent->FindList( pContainer, pList );
    OutdentContainer( pContainer, pList );
    Relayout( pContainer, 0, pContainer->GetLastMostChild() );                
}

void CEditBuffer::OutdentContainer( CEditContainerElement *pContainer, 
        CEditListElement *pList ){

    //
    // case 0
    //
    //      LI:     <-- outdent this guy.
    //
    if( pList == 0 ){         
        return;                         // no work to do
    }

    CEditElement *pPrev = pContainer->GetPreviousSibling();
    CEditElement *pNext = pContainer->GetNextSibling();

    //
    // case 1
    //
    //      UL:
    //          LI:     <-Outdenting this guy
    //
    // should result in
    //
    //      LI:
    //
    // No previous or next siblings.  Just remove the List and
    //  put its container in its place.
    //
    if( pPrev == 0 && pNext == 0 ){
        pContainer->Unlink();
        pContainer->InsertAfter( pList );
        pList->Unlink();
        delete pList;
    }


    //
    // case 2
    //
    //      UL:
    //          LI:     <-Outdenting this guy
    //          LI:
    //          
    //
    //      Results in:
    //
    //      LI:         <-Outdenting this guy
    //      UL:
    //          LI:
    //
    else if( pPrev == 0 ){
        pContainer->Unlink();
        pContainer->InsertBefore( pList );
    }
    //
    // case 3
    //
    //      UL:
    //          LI:
    //          LI:     <-Outdenting this guy
    //
    //
    //      Results in:
    //
    //      UL:
    //          LI:
    //      LI:         <-Outdenting this guy
    //
    else if( pNext == 0 ){
        pContainer->Unlink();
        pContainer->InsertAfter( pList );
    }

    //
    // case 4
    //
    //      UL:
    //          LI:
    //          LI:     <-Outdenting this guy
    //          LI:
    //
    //      Results in:
    //
    //      UL:
    //          LI:
    //      LI:         <-Outdenting this guy
    //      UL:
    //          LI:
    else {
        CEditElement *pNewList = pList->Clone(0);
        while( (pNext = pContainer->GetNextSibling()) != 0 ){
            pNext->Unlink();
            pNext->InsertAsLastChild( pNewList );
        }
        pContainer->Unlink();
        pContainer->InsertAfter( pList );
        pNewList->InsertAfter( pContainer );
    }
}

ED_ElementType CEditBuffer::GetCurrentElementType(){
    CEditLeafElement *pInsertPoint;
    ElementOffset iOffset;
    XP_Bool bSingleItem;

    bSingleItem = GetPropertyPoint( &pInsertPoint, &iOffset );
    
    if( !bSingleItem ){
        if( IsSelected() ){
            return ED_ELEMENT_SELECTION;
        }
        else {
            return ED_ELEMENT_TEXT;
        }
    }
    else if( pInsertPoint->IsA(P_TEXT) ){
        // should never have single item text objects...
        XP_ASSERT(FALSE);
        return ED_ELEMENT_TEXT;
    }
    else if( pInsertPoint->IsImage() ){
        return ED_ELEMENT_IMAGE;
    }
    else if( pInsertPoint->IsA(P_HRULE) ){
        return ED_ELEMENT_HRULE;
    }
    else if( pInsertPoint->IsA(P_LINEBREAK )){
        return ED_ELEMENT_TEXT;
    }
    else if( pInsertPoint->GetElementType() == eTargetElement ){
        return ED_ELEMENT_TARGET;
    }
    else if( pInsertPoint->GetElementType() == eIconElement ){
        return ED_ELEMENT_UNKNOWN_TAG;
    }
    // current element type is unknown.
    XP_ASSERT( FALSE );
    return ED_ELEMENT_TEXT;     // so the compiler doesn't complain.
}

void CEditBuffer::MorphContainer( TagType t ){
    VALIDATE_TREE(this);
    XP_Bool bDone;

    // charley says this is happening.. Hmmmm...
    //XP_ASSERT( t != P_TEXT );
    if( t == P_TEXT ){
        // actually failed this assert. 
        return;
    }

    SetDirtyFlag( TRUE );
    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd, *pCurrent;
        CEditContainerElement* pContainer = NULL;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        pCurrent = pBegin;
        bDone = FALSE;
        do {
            pContainer = pCurrent->FindContainer();
            pContainer->SetType( t );

            bDone = (pEnd == pCurrent );    // For most cases
            pCurrent = pCurrent->NextLeafAll();
            bDone = bDone || (iEndPos == 0 && pEnd == pCurrent ); // Pesky edge conditions!
        } while( pCurrent && !bDone );

        // force layout stop displaying the current selection.
        LO_StartSelectionFromElement( m_pContext, 0, 0); 
        // probably needs to be the common ancestor.
        Relayout( pBegin->FindContainer(), 0, pEnd, RELAYOUT_NOCARET );
        // Need to force selection.
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
    }
    else {
        CEditElement *pContainer = m_pCurrent->FindContainer();
        // HACKOLA: just poke in a new tag type.
        pContainer->SetType( t );
        // Need to relayout from previous line.  Line break distance is wrong.
        Relayout( pContainer, 0, 0 );
    }
}

void CEditBuffer::SetParagraphAlign( ED_Alignment eAlign ){
    VALIDATE_TREE(this);
    XP_Bool bDone;
    
    SetDirtyFlag( TRUE );
    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd, *pCurrent;
        CEditContainerElement *pContainer = NULL;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        pCurrent = pBegin;
        bDone = FALSE;
        do {
            pContainer = pCurrent->FindContainer();
            pContainer->Container()->SetAlignment( eAlign );
            
            bDone = (pEnd == pCurrent );    // For most cases
            pCurrent = pCurrent->NextLeafAll();
            bDone = bDone || (iEndPos == 0 && pEnd == pCurrent ); // Pesky edge conditions!
        } while( pCurrent && !bDone );

        // force layout stop displaying the current selection.
        LO_StartSelectionFromElement( m_pContext, 0, 0); 
        // probably needs to be the common ancestor.
        Relayout( pBegin->FindContainer(), 0, pEnd, RELAYOUT_NOCARET );
        // Need to force selection.
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
    }
    else {
        CEditElement *pContainer = m_pCurrent->FindContainer();
        // HACKOLA: just poke in a new tag type.
        pContainer->Container()->SetAlignment( eAlign );
        // Need to relayout from previous line.  Line break distance is wrong.
        Relayout( pContainer, 0, pContainer->GetLastMostChild() );
    }
}

ED_Alignment CEditBuffer::GetParagraphAlign( ){
    CEditLeafElement *pInsertPoint;
    CEditContainerElement *pCont;
    ElementOffset iOffset;

    GetPropertyPoint( &pInsertPoint, &iOffset );
    if( pInsertPoint && (pCont = pInsertPoint->FindContainer()) != 0){
        return pCont->GetAlignment();
    } 
    else {
        return ED_ALIGN_DEFAULT;
    }
}

void CEditBuffer::SetFontSize( int iSize ){
    VALIDATE_TREE(this);
    
    SetDirtyFlag( TRUE );
    CEditSelection selection;
    GetSelection(selection);
    if ( selection.IsContainerEnd() )
        return;
    else if( selection.AnyLeavesSelected() ){
        CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(this);
        SetFontSizeSelection( iSize );
        AdoptAndDo(pCommand);
        return;
    }

    FixupInsertPoint();
    CEditLeafElement *pRight = m_pCurrent->Divide( m_iCurrentOffset )->Leaf();
    if( pRight->IsA(P_TEXT) 
            && pRight->Text()->GetLen() == 0 ) {
    //    && m_pCurrent == pRight
    // Bug 27891, this term in the conditional test should not be here, e.g. 
    // in CEditBuffer::SetCharacterData() it is not there.
        pRight->Text()->SetFontSize(iSize);
        m_pCurrent = pRight;
        m_iCurrentOffset = 0;
    }
    else {
        CEditTextElement *pNew = m_pCurrent->Leaf()->CopyEmptyText();
        pNew->InsertBefore(pRight);
        pNew->SetFontSize( iSize );
        m_pCurrent = pNew;
        m_iCurrentOffset = 0;
    }
    Relayout( m_pCurrent, 0, 0 );
}

void CEditBuffer::SetFontColor( ED_Color iColor ){
    // ToDo: Eliminate this function in favor of SetCharacterData
    VALIDATE_TREE(this);

    SetDirtyFlag( TRUE );
    CEditSelection selection;
    GetSelection(selection);
    if ( selection.IsContainerEnd() )
        return;
    else if( selection.AnyLeavesSelected() ){
        CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(this);
        SetFontColorSelection( iColor );
        AdoptAndDo(pCommand);
        return;
    }

    FixupInsertPoint();
    CEditLeafElement *pRight = m_pCurrent->Divide( m_iCurrentOffset )->Leaf();
    if( pRight->IsA(P_TEXT) 
            && pRight->Leaf()->GetLen() == 0 ) {
    //    && m_pCurrent == pRight
    // Bug 27891, this term in the conditional test should not be here, e.g. 
    // in CEditBuffer::SetCharacterData() it is not there.
        pRight->Text()->SetColor( iColor );
        m_pCurrent = pRight;
        m_iCurrentOffset = 0;
    }
    else {
        CEditTextElement *pNew = m_pCurrent->Leaf()->CopyEmptyText();
        pNew->InsertBefore(pRight);
        pNew->SetColor( iColor );
        m_pCurrent = pNew;
        m_iCurrentOffset = 0;
    }
    Relayout( m_pCurrent, 0, 0 );
}


void CEditBuffer::SetHREF( char *pHref, char *pExtra ){
    VALIDATE_TREE(this);
    ED_LinkId id;
    ED_LinkId oldId;
    CEditElement *pPrev, *pNext, *pStart, *pEnd;
    XP_Bool bSame;
    pStart = m_pCurrent;
    pEnd = 0;

    SetDirtyFlag( TRUE );

    //
    // if we are setting link data and the current selection or word
    //  has extra data, we should preserve it.    
    //
    if( pHref ){
        EDT_CharacterData* pCD = 0;
        if( pExtra == 0 ){
            pCD = GetCharacterData();
            if( pCD 
                    && (pCD->mask & TF_HREF)
                    && (pCD->values & TF_HREF)
                    && pCD->pHREFData
                    && pCD->pHREFData->pExtra ){
                pExtra = pCD->pHREFData->pExtra;
            }
        }
        id  = linkManager.Add( pHref, pExtra );
        if( pCD ){
            EDT_FreeCharacterData( pCD );
        }
    }
    else {
        id = ED_LINK_ID_NONE;
    }

    CEditSelection selection;
    GetSelection(selection);
    if ( selection.IsContainerEnd() )
        return;
    else if( selection.AnyLeavesSelected() ){
        CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(this);
        SetHREFSelection( id );
        AdoptAndDo(pCommand);
    }
    else {
        //FixupInsertPoint();
        // check all text around us.  
        oldId = m_pCurrent->Leaf()->GetHREF();
        m_pCurrent->Leaf()->SetHREF( id );
        pPrev = m_pCurrent->PreviousLeafInContainer();
        bSame = TRUE;
        while( pPrev && bSame ){
            bSame = pPrev->Leaf()->GetHREF() == oldId; 
            if( bSame ){
                pPrev->Leaf()->SetHREF( id );
                pStart = pPrev;
            }
            pPrev = pPrev->PreviousLeafInContainer();
        }
        pNext = m_pCurrent->LeafInContainerAfter();
        bSame = TRUE;
        while( pNext && bSame ){
            bSame = pNext->Leaf()->GetHREF() == oldId;
            if( bSame ){
                pNext->Leaf()->SetHREF( id );
                pEnd = pNext;
            }
            pNext = pNext->LeafInContainerAfter();
        }
        Relayout( pStart, 0, pEnd );
    }
    if( id != ED_LINK_ID_NONE ){
        linkManager.Free(id);
    }
}

// Use this only when setting one attribute at a time
// Always make Superscript and Subscript mutually exclusive - very tricky!
void CEditBuffer::FormatCharacter( ED_TextFormat tf ){
    VALIDATE_TREE(this);
    SetDirtyFlag( TRUE );
    CEditSelection selection;
    GetSelection(selection);
    if ( selection.IsContainerEnd() )
        return;

    if( selection.AnyLeavesSelected() ){
        CChangeAttributesCommand* pCommand = new CChangeAttributesCommand(this);
        FormatCharacterSelection(tf);
        AdoptAndDo(pCommand);
        return;
    }

    FixupInsertPoint();
    CEditLeafElement *pRight = m_pCurrent->Divide( m_iCurrentOffset )->Leaf();
    if( pRight->IsA(P_TEXT) && pRight->Leaf()->GetLen() == 0 ){
        if( tf == TF_NONE ){
            pRight->Text()->ClearFormatting();
        }
        else if( tf == TF_SERVER || tf == TF_SCRIPT ){
            pRight->Text()->ClearFormatting();
            pRight->Text()->m_tf = tf;
        }
        // If we are in either Java Script type, IGNORE change in format
        else if( ED_IS_NOT_SCRIPT(pRight) ){
            // Make superscript and subscript mutually-exclusive
            // if already either one type, and we are setting the opposite,
            //  clear this bit first
            if( tf == TF_SUPER && (pRight->Text()->m_tf & TF_SUB) ){
                pRight->Text()->m_tf &= ~TF_SUB;
            } else if( tf == TF_SUB && (pRight->Text()->m_tf & TF_SUPER) ){
                pRight->Text()->m_tf &= ~TF_SUPER;
            }
            pRight->Text()->m_tf ^= tf;
        }
        m_pCurrent = pRight;
        m_iCurrentOffset = 0;
    }
    else {
        // We are making new text element, so we don't need
        //   to worry about existing Java Script or Super/Subscript stuff
        CEditTextElement *pNew = m_pCurrent->Leaf()->CopyEmptyText();
        pNew->InsertBefore(pRight);
        if( tf == TF_NONE ){
            pNew->ClearFormatting();
        }
        else {
            if( tf == TF_SUPER && (pNew->Text()->m_tf & TF_SUB) ){
                pNew->Text()->m_tf &= ~TF_SUB;
            } else if( tf == TF_SUB && (pNew->Text()->m_tf & TF_SUPER) ){
                pNew->Text()->m_tf &= ~TF_SUPER;
            }
            pNew->m_tf ^= tf;
        }
        m_pCurrent = pNew;
        m_iCurrentOffset = 0;
    }
    Relayout( m_pCurrent, 0, 0 );
}

void CEditBuffer::SetCharacterDataSelection( EDT_CharacterData *pData ){
    VALIDATE_TREE(this);
    CEditLeafElement *pCurrent, *pEnd, *pBegin, *pNext;

    SetDirtyFlag( TRUE );

    //
    // Guarantee that the text blocks of the beginning and end of selection
    //  are atomic.
    //

    MakeSelectionEndPoints( pBegin, pEnd );
    pCurrent = pBegin;    

    while( pCurrent != pEnd ){
        pNext = pCurrent->NextLeafAll();
        if( pCurrent->IsA( P_TEXT ) ){
            pCurrent->Text()->SetData( pData );
        }
        // Set just the HREF for an image
        else if( pCurrent->IsImage() && 
                 (pData->mask & TF_HREF) ){
            pCurrent->Image()->SetHREF( pData->linkId );
        }
        pCurrent = pNext;
    }

    // force layout stop displaying the current selection.
    LO_StartSelectionFromElement( m_pContext, 0, 0); 
    // probably needs to be the common ancestor.
    Relayout( pBegin, 0, pEnd, RELAYOUT_NOCARET );
    // Need to force selection.
    SelectRegion(pBegin, 0, pEnd, 0);
    Reduce(pBegin->GetCommonAncestor(pEnd));
}

void CEditBuffer::SetCharacterData( EDT_CharacterData *pData ){
    VALIDATE_TREE(this);   
    SetDirtyFlag( TRUE );
    CEditSelection selection;
    GetSelection(selection);
    if ( selection.IsContainerEnd() )
        return;
    else if( selection.AnyLeavesSelected() ){
        AdoptAndDo(new CChangeAttributesCommand(this));
        SetCharacterDataSelection( pData );
        return;
    }

    FixupInsertPoint();
    CEditLeafElement *pRight = m_pCurrent->Divide( m_iCurrentOffset )->Leaf();
    if( pRight->IsA(P_TEXT) && pRight->Leaf()->GetLen() == 0 ){
        pRight->Text()->SetData( pData );
        m_pCurrent = pRight;
        m_iCurrentOffset = 0;
    }
    else {
        CEditTextElement *pNew = m_pCurrent->Leaf()->CopyEmptyText();
        pNew->InsertBefore(pRight);
        pNew->SetData( pData );
        m_pCurrent = pNew;
        m_iCurrentOffset = 0;
    }
    Relayout( m_pCurrent, 0, 0 );
}

EDT_CharacterData* CEditBuffer::GetCharacterData(){
	CEditElement *pElement = m_pCurrent;
	
	// The following assert fails once at program startup.		
	// XP_ASSERT(pElement || IsSelected());

    // 
    // While selecting, we may not have a valid region
    //
    if( IsSelecting() ){
        return EDT_NewCharacterData();
    }

    if( IsSelected() ){
        CEditSelection selection;
        EDT_CharacterData *pData = 0;

        GetSelection( selection );
        pElement = selection.m_start.m_pElement;

        //
        while( pElement && pElement != selection.m_end.m_pElement ){
            if( pElement->IsA( P_TEXT ) ){
                pElement->Text()->MaskData( pData );
            }
            else if( pElement->IsImage() ){
                // We report the HREF status if we find an image
                pElement->Image()->MaskData( pData );
            }
            pElement = pElement->NextLeaf();
        }

        // if the selection ends in the middle of a text block.
        if( pElement && selection.m_end.m_iPos != 0 
                && selection.m_end.m_pElement->IsA(P_TEXT) ){
            pElement->Text()->MaskData( pData );
        }

        if( pData == 0 ){
            return EDT_NewCharacterData();
        }
        return pData;
    }

    if( pElement && pElement->IsA(P_TEXT )){
        return pElement->Text()->GetData();;
    }


	// Try to find the previous text element and use its characterData
	if (pElement) {
		CEditTextElement *tElement = pElement->PreviousTextInContainer();
		if ( tElement){
			return tElement->GetData();
		}
	}
	
	// Create a dummy text element from scratch to guarantee
	// that we get the same EDT_CharacterData as when creating a new
	// CEditTextElement during typing.
	CEditTextElement dummy( (CEditElement *)0,0);
	return dummy.GetData();

	// return EDT_NewCharacterData();
}



EDT_PageData* CEditBuffer::GetPageData(){
    EDT_PageData* pData = EDT_NewPageData();

    pData->pColorBackground = edt_MakeLoColor( m_colorBackground );
    pData->pColorText = edt_MakeLoColor( m_colorText );
    pData->pColorLink = edt_MakeLoColor( m_colorLink );
    pData->pColorFollowedLink = edt_MakeLoColor( m_colorFollowedLink );
    pData->pColorActiveLink = edt_MakeLoColor( m_colorActiveLink );

    pData->pTitle = XP_STRDUP( ( m_pTitle ? m_pTitle : "" ) );
    pData->pBackgroundImage = ( m_pBackgroundImage 
                    ? XP_STRDUP( m_pBackgroundImage ) 
                    : 0 );

    return pData;
}

void CEditBuffer::FreePageData( EDT_PageData* pData ){
    if ( pData->pColorBackground ) XP_FREE( pData->pColorBackground );
    if ( pData->pColorText ) XP_FREE( pData->pColorText );
    if ( pData->pColorLink ) XP_FREE( pData->pColorLink );
    if ( pData->pColorFollowedLink ) XP_FREE( pData->pColorFollowedLink );
    if ( pData->pColorActiveLink ) XP_FREE( pData->pColorActiveLink );
    if ( pData->pTitle ) XP_FREE( pData->pTitle );
    if ( pData->pBackgroundImage ) XP_FREE( pData->pBackgroundImage );

    XP_FREE( pData );
}

void CEditBuffer::SetPageData( EDT_PageData* pData ){
    m_colorBackground = EDT_LO_COLOR( pData->pColorBackground );
    m_colorLink = EDT_LO_COLOR( pData->pColorLink );
    m_colorText = EDT_LO_COLOR( pData->pColorText );
    m_colorFollowedLink = EDT_LO_COLOR( pData->pColorFollowedLink );
    m_colorActiveLink = EDT_LO_COLOR( pData->pColorActiveLink );

    if( m_pTitle ) XP_FREE( m_pTitle );
    m_pTitle = (pData->pTitle
                            ? XP_STRDUP( pData->pTitle )
                            : 0);
    FE_SetDocTitle( m_pContext, m_pTitle );

	//
	//    If there was an image, and now there isn't remove it.
	//
	if (pData->pBackgroundImage == NULL ){
        if(m_pBackgroundImage != NULL) XP_FREE(m_pBackgroundImage);
		m_pBackgroundImage = NULL;
	}
	else if( EDT_IS_NEW_DOCUMENT(m_pContext) ){
        // We can't copy it now (we don't have a base URL)
        // so just use the absolute URL as the background until we save the file
        if( m_pBackgroundImage ) XP_FREE( m_pBackgroundImage );
        m_pBackgroundImage = XP_STRDUP(pData->pBackgroundImage);
    } 
    else {
        // We have a background (possibly new) and a saved file
        char *pStr = 0;        
        intn retVal = NET_MakeRelativeURL( LO_GetBaseURL(m_pContext ), 
                    pData->pBackgroundImage, 
                    &pStr);

        XP_ASSERT( retVal != NET_URL_FAIL );

        if( pData->bKeepImagesWithDoc 
                && retVal != NET_URL_SAME_DIRECTORY 
                && !EDT_IS_LIVEWIRE_MACRO( pData->pBackgroundImage ) ){
            char *pLocalName = edt_LocalName( pStr );
            XP_FREE( pStr );
            if( pLocalName != 0 ){
                
                //  Don't set m_pBackgroundImage = pLocalName!
                //      We will set name only after possible file save
                //      (in CEditBackgroundImageSaveObject::FileFetchComplete)
                m_pSaveObject = new CEditBackgroundImageSaveObject( this  );
                m_pSaveObject->AddFile( pData->pBackgroundImage, pLocalName );
                m_pSaveObject->SaveFiles();
                XP_FREE(pLocalName);                
            }
        }
        else {
            if( m_pBackgroundImage ) XP_FREE( m_pBackgroundImage );
            m_pBackgroundImage = pStr;
        }
    }

    // Note: all LO_SetDocumentColor calls 
    //       and LO_SetBackgroundImage are done in RefreshLayout
    RefreshLayout();
    SetDirtyFlag( TRUE );
}

// MetaData

intn CEditBuffer::MetaDataCount( ){
    intn i = 0;
    intn iRetVal = 0;
    while( i < m_metaData.Size() ){
        if( m_metaData[i] != 0 ){
            iRetVal++;
        }
        i++;
    }
    return iRetVal;
}

intn CEditBuffer::FindMetaData( EDT_MetaData *pMetaData ){
    intn i = 0;
    intn iRetVal = 0;
    while( i < m_metaData.Size() ){
        if( m_metaData[i] != 0 ){
            if( (!!pMetaData->bHttpEquiv == !!m_metaData[i]->bHttpEquiv)
                    && XP_STRCMP( m_metaData[i]->pName, pMetaData->pName) == 0 ){
                return iRetVal;
            }
            iRetVal++;
        }
        i++;
    }
    return -1;
}

intn CEditBuffer::FindMetaDataIndex( EDT_MetaData *pMetaData ){
    intn i = 0;
    while( i < m_metaData.Size() ){
        if( m_metaData[i] != 0 ){
            if( (!!pMetaData->bHttpEquiv == !!m_metaData[i]->bHttpEquiv)
                    && XP_STRCMP( m_metaData[i]->pName, pMetaData->pName) == 0 ){
                return i;
            }
        }
        i++;
    }
    return -1;
}

EDT_MetaData* CEditBuffer::MakeMetaData( XP_Bool bHttpEquiv, char *pName, char*pContent ){
    EDT_MetaData *pData = XP_NEW( EDT_MetaData );
    XP_BZERO( pData, sizeof( EDT_MetaData ) );

    pData->bHttpEquiv = bHttpEquiv;
    pData->pName = (pName
                            ? XP_STRDUP(pName)
                            : 0);
    pData->pContent = (pContent
                            ? XP_STRDUP(pContent)
                            : 0);
    return pData;
}

EDT_MetaData* CEditBuffer::GetMetaData( int n ){
    intn i = 0;
    intn iRetVal = 0;
    n++;
    while( n ){
        if ( i >= m_metaData.Size() ) {
            return NULL;
        }
        if( m_metaData[i] != 0 ){
            n--;
        }
        i++;
    }
    i--;
    return MakeMetaData( m_metaData[i]->bHttpEquiv,
                         m_metaData[i]->pName,
                         m_metaData[i]->pContent );
}

void CEditBuffer::SetMetaData( EDT_MetaData *pData ){
    intn i = FindMetaDataIndex( pData );
    EDT_MetaData* pNew = MakeMetaData( pData->bHttpEquiv, pData->pName, pData->pContent );
    if( i == -1 ){
        m_metaData.Add( pNew );
    }
    else {
        FreeMetaData( m_metaData[i] );
        m_metaData[i] = pNew;
    }
        
}

void CEditBuffer::DeleteMetaData( EDT_MetaData *pData ){
    intn i = FindMetaDataIndex( pData );
    if( i != -1 ){
        FreeMetaData( m_metaData[i] );
        m_metaData[i] = 0;
    }
}

void CEditBuffer::FreeMetaData( EDT_MetaData *pData ){
    if ( pData ) {
        if( pData->pName ) XP_FREE( pData->pName );
        if( pData->pContent ) XP_FREE( pData->pContent );
        XP_FREE( pData );
    }
}

void CEditBuffer::ParseMetaTag( PA_Tag *pTag ){
    XP_Bool bHttpEquiv = TRUE;
    char *pName;
    char *pContent;

    pContent = edt_FetchParamString( pTag, PARAM_CONTENT );
    pName = edt_FetchParamString( pTag, PARAM_HTTP_EQUIV );

    // if we didn't get http-equiv, try for name=
    if( pName == 0 ){
        bHttpEquiv = FALSE;
        pName = edt_FetchParamString( pTag, PARAM_NAME );
    }

    // if we got one or the other, add it to the list of meta tags.
    if( pName ){
        EDT_MetaData *pData = MakeMetaData( bHttpEquiv, pName, pContent );
        SetMetaData( pData );
        FreeMetaData( pData );
    }

    if( pName ) XP_FREE( pName );
    if( pContent ) XP_FREE( pContent );
}


// Image

EDT_ImageData* CEditBuffer::GetImageData(){
    CEditLeafElement *pInsertPoint;
    ElementOffset iOffset;
    XP_Bool bSingleItem;

    bSingleItem = GetPropertyPoint( &pInsertPoint, &iOffset );
    XP_ASSERT( bSingleItem );

    XP_ASSERT( pInsertPoint->IsA(P_IMAGE) );

    return pInsertPoint->Image()->GetImageData( );
}


#define XP_STRCMP_NULL( a, b ) XP_STRCMP( (a)?(a):"", (b)?(b):"" )

void CEditBuffer::SetImageData( EDT_ImageData* pData, XP_Bool bKeepImagesWithDoc ){
    VALIDATE_TREE(this);
    ClearSelection( TRUE, TRUE );

    XP_ASSERT( m_pCurrent->IsA(P_IMAGE) );

    // This function is only called when changing attributes of an image, not
    //   when inserting an image.
    //
    // There are 4 cases...
    //
    // 1) They change the src of the image, but not the size of the image
    //      (assume the dimensions of the new image).
    // 2) They change the src and the size of the image
    // 3) They change just the size of the image.
    // 4) They reset the size of the image.
    //  

    CEditImageElement *pImage = m_pCurrent->Image();
    EDT_ImageData *pOldData = m_pCurrent->Image()->GetImageData();

    // Case 1, 2, or 4
    if( (XP_STRCMP( pOldData->pSrc, pData->pSrc ) != 0 )
            || XP_STRCMP_NULL( pOldData->pLowSrc, pData->pLowSrc ) 
            || (pData->iHeight == 0 || pData->iWidth== 0) ){


        // if they touched height or width, assume they know what they
        //  are doing.
        //
        if( pData->iHeight == pOldData->iHeight
                && pData->iWidth == pOldData->iWidth ){

            // LoadImage will get the image size from the image.
            pData->iHeight = 0;
            pData->iWidth = 0;
        }

        LoadImage( pData, bKeepImagesWithDoc, TRUE );
        edt_FreeImageData( pOldData );
    }
    else {
        m_pCurrent->Image()->SetImageData( pData );
        Relayout( m_pCurrent->FindContainer(), 0, m_pCurrent, RELAYOUT_NOCARET );
        SelectCurrentElement();
    }
    SetDirtyFlag( TRUE );
    // Also set the HREF if it exists
    if( pData->pHREFData ){
        if( !pData->pHREFData->pURL || XP_STRLEN(pData->pHREFData->pURL) == 0 ){
            pImage->SetHREF( ED_LINK_ID_NONE );
        } else {
            // Add link data to the image
            pImage->SetHREF( linkManager.Add( pData->pHREFData->pURL,
                                              pData->pHREFData->pExtra) );
        }
    }
}

void CEditBuffer::LoadImage( EDT_ImageData* pData, XP_Bool bKeepImagesWithDoc,

            XP_Bool bReplaceImage ){
    XP_Bool bMakeAbsolute = TRUE;
    CEditImageSaveObject* pSaver = 0;
    
    if( m_pLoadingImage != 0 ){
        XP_ASSERT(0);       // can only be loading one image at a time.
        return;
    }
    if( !m_pContext->is_new_document ){
        //  You can call this with 3rd param NULL -- it is ignored
        //  (thats where just filename is placed)
        intn retVal = NET_MakeRelativeURL( LO_GetBaseURL(m_pContext ), 
                    pData->pSrc, NULL );

        XP_ASSERT( retVal != NET_URL_FAIL );

        if( bKeepImagesWithDoc 
                && retVal != NET_URL_SAME_DIRECTORY 
                && !EDT_IS_LIVEWIRE_MACRO( pData->pSrc )){
            char *pLocalName = edt_LocalName( pData->pSrc );
            if( pLocalName != 0 ){
                bMakeAbsolute = FALSE;
                pSaver = new CEditImageSaveObject( this, pData, bReplaceImage );
                pSaver->m_srcIndex = pSaver->AddFile( 
                        pData->pSrc, pLocalName );
                
            }     
        }

        // Do the low resolution image if it exists..

        if( bKeepImagesWithDoc 
                && pData->pLowSrc 
                && !EDT_IS_LIVEWIRE_MACRO( pData->pLowSrc )){
            char *pLowStr = 0;
            intn retVal = NET_MakeRelativeURL( LO_GetBaseURL(m_pContext ), 
                        pData->pLowSrc, NULL );
            char *pLocalName = edt_LocalName( pData->pLowSrc );
            if( pLocalName && retVal != NET_URL_SAME_DIRECTORY ){
                if( pSaver == 0 ) {
                    pSaver = new CEditImageSaveObject( 
                            this, pData, bReplaceImage );
                }
                bMakeAbsolute = FALSE;
                pSaver->m_lowSrcIndex = pSaver->AddFile( 
                                pData->pLowSrc, pLocalName );
            }
        }

    }

    if( bMakeAbsolute ){
        m_pLoadingImage = new CEditImageLoader( this, pData, bReplaceImage );
        m_pLoadingImage->LoadImage();
    }
    else if ( pSaver ){
        m_pSaveObject = pSaver;
        m_pSaveObject->SaveFiles();
    }
}

//
// need to do a better job of splitting and inserting in the right place.
//
void CEditBuffer::InsertImage( EDT_ImageData* pData ){
    if( IsSelected() ){
        ClearSelection();
    }
    CEditImageElement *pImage = new CEditImageElement(0);
    pImage->SetImageData( pData );
    // Add link data to the image
    if( pData->pHREFData && pData->pHREFData->pURL &&
        XP_STRLEN(pData->pHREFData->pURL) > 0){
        pImage->SetHREF( linkManager.Add( pData->pHREFData->pURL,
                                          pData->pHREFData->pExtra) );
    }
    InsertLeaf( pImage );
}


void CEditBuffer::InsertLeaf( CEditLeafElement *pLeaf ){
    VALIDATE_TREE(this);
    FixupInsertPoint();
    if( m_iCurrentOffset == 0 ){
        pLeaf->InsertBefore( m_pCurrent );
    }    
    else {
        m_pCurrent->Divide(m_iCurrentOffset);
        pLeaf->InsertAfter( m_pCurrent );
    }
    m_pCurrent = pLeaf;
    m_iCurrentOffset = 1;

    if ( ! m_bNoRelayout ){
        Reduce(pLeaf->FindContainer());
    }

    // we could be doing a better job here.  We probably repaint too much...
    CEditElement *pContainer = m_pCurrent->FindContainer();
    Relayout( pContainer, 0, pContainer->GetLastMostChild() );
    SetDirtyFlag( TRUE );
}

void CEditBuffer::InsertNonLeaf( CEditElement *pElement){
    VALIDATE_TREE(this);
    FixupInsertPoint();
    XP_ASSERT( ! pElement->IsLeaf());
    CEditElement* pStart = m_pCurrent;
    int iStartOffset = m_iCurrentOffset;
    CEditInsertPoint start(m_pCurrent, m_iCurrentOffset);
	CEditLeafElement* pRight = NULL;
    InternalReturnKey(FALSE);

    pRight = m_pCurrent;
    CEditLeafElement* pLeft = pRight->PreviousLeaf(); // Will be NULL at start of document
    pElement->InsertBefore(pRight->FindContainer());
    pElement->FinishedLoad(this);

#ifdef DEBUG
    m_pRoot->ValidateTree();
#endif
	// Put cursor at the end of what has been inserted

    if ( pRight->IsEndOfDocument() ) {
        m_pCurrent = pRight->PreviousLeaf();
        m_iCurrentOffset = m_pCurrent->GetLen();
    }
    else {
        m_pCurrent = pRight;
        m_iCurrentOffset = 0;
   }

    Relayout( pElement, 0, pRight );
    SetDirtyFlag( TRUE );
}

// Table stuff

void CEditBuffer::GetTableInsertPoint(CEditInsertPoint& ip){
    GetInsertPoint(ip);
    CEditSelection s;
    GetSelection(s);
    if ( ! s.IsInsertPoint() && s.m_end == ip && ip.IsStartOfContainer()){
        ip = ip.PreviousPosition();
    };
}

XP_Bool CEditBuffer::IsInsertPointInTable(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    return ip.m_pElement->GetTableIgnoreSubdoc() != NULL;
}

XP_Bool CEditBuffer::IsInsertPointInNestedTable(){
    XP_Bool result = FALSE;
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pFirstTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pFirstTable ) {
        CEditTableElement* pSecondTable = pFirstTable->GetParent()->GetTableIgnoreSubdoc();
        if ( pSecondTable ) {
            result = TRUE;
        }
    }
    return result;
}

EDT_TableData* CEditBuffer::GetTableData(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        return pTable->GetData();
    }
    else {
        return NULL;
    }
}

void CEditBuffer::SetTableData(EDT_TableData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        pTable->SetData( pData );
        Relayout( pTable, 0 );
        SetDirtyFlag( TRUE );
    }
}

void CEditBuffer::InsertTable(EDT_TableData *pData){
    CEditTableElement *pTable = new CEditTableElement(pData->iColumns, pData->iRows);
    if( pTable ){
        pTable->SetData( pData );
        pTable->FinishedLoad(this); // Sets default paragraphs for all the cells
        AdoptAndDo(new CInsertTableCommand(this, pTable));
    }
}

void CEditBuffer::DeleteTable(){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
		AdoptAndDo(new CDeleteTableCommand(this));
    }
}

XP_Bool CEditBuffer::IsInsertPointInTableCaption(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    // We allow the insert point to be in a table with a table caption.
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    CEditCaptionElement* pTableCaption = pTable ? pTable->GetCaption() : NULL;
    return pTableCaption != NULL;
}

EDT_TableCaptionData* CEditBuffer::GetTableCaptionData(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    // We allow the insert point to be in a table with a table caption.
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    CEditCaptionElement* pTableCaption = pTable ? pTable->GetCaption() : NULL;
    if ( pTableCaption ){
        return pTableCaption->GetData();
    }
    else {
        return NULL;
    }
}

void CEditBuffer::SetTableCaptionData(EDT_TableCaptionData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    CEditCaptionElement* pTableCaption = pTable ? pTable->GetCaption() : NULL;
    if ( pTableCaption ){
        pTableCaption->SetData( pData );
        pTable->FinishedLoad(this); // Can move caption up or down.
        Relayout( pTable, 0 );
        SetDirtyFlag( TRUE );
    }
}

void CEditBuffer::InsertTableCaption(EDT_TableCaptionData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    if ( pTable ){
        if ( ! pTable->GetCaption() ) {
            AdoptAndDo(new CInsertTableCaptionCommand(this, pData));
        }
    }
}

void CEditBuffer::DeleteTableCaption(){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableElement* pTable = ip.m_pElement->GetTableIgnoreSubdoc();
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTable ){
        if ( pTable->GetCaption() ) {
            AdoptAndDo(new CDeleteTableCaptionCommand(this));
        }
    }
}

XP_Bool CEditBuffer::IsInsertPointInTableRow(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    return ip.m_pElement->GetTableRowIgnoreSubdoc() != NULL;
}

EDT_TableRowData* CEditBuffer::GetTableRowData(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableRowElement* pTableRow = ip.m_pElement->GetTableRowIgnoreSubdoc();
    if ( pTableRow ){
        return pTableRow->GetData();
    }
    else {
        return NULL;
    }
}

void CEditBuffer::SetTableRowData(EDT_TableRowData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableRowElement* pTableRow = ip.m_pElement->GetTableRowIgnoreSubdoc();
    if ( pTableRow ){
        pTableRow->SetData( pData );
        Relayout( pTableRow, 0 );
        SetDirtyFlag( TRUE );
    }
}

void CEditBuffer::InsertTableRows(EDT_TableRowData *pData, XP_Bool bAfterCurrentRow, intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        AdoptAndDo(new CInsertTableRowCommand(this, pData, bAfterCurrentRow, number));
    }
}

void CEditBuffer::DeleteTableRows(intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
		AdoptAndDo(new CDeleteTableRowCommand(this, number));
    }
}

void CEditBuffer::SyncCursor(CEditTableElement* pTable, intn iColumn, intn iRow)
{
    CEditSelection selection = ComputeCursor(pTable, iColumn, iRow);
    SetSelection(selection);
}

CEditSelection CEditBuffer::ComputeCursor(CEditTableElement* pTable, intn iColumn, intn iRow)
{
    CEditInsertPoint insertPoint;
    insertPoint.m_pElement = m_pRoot->GetFirstMostChild()->Leaf();
    if ( pTable ) {
        intn iRows = pTable->GetRows();
        if ( iRows <= iRow ) {
            iRow = iRows - 1;
        }
        CEditTableRowElement* pTableRow = pTable->FindRow(iRow);
        if ( pTableRow ) {
            intn iColumns = pTableRow->GetCells();
            if ( iColumns <= iColumn ) {
                iColumn = iColumns - 1;
            }
            CEditTableCellElement* pTableCell = pTableRow->FindCell(iColumn);
            if ( pTableCell ) {
                insertPoint.m_pElement = pTableCell->GetLastMostChild()->Leaf();
            }
        }
    }
    insertPoint.m_iPos = insertPoint.m_pElement->GetLen();
    return CEditSelection(insertPoint, insertPoint);
}

void CEditBuffer::SyncCursor(CEditMultiColumnElement* pMultiColumn)
{
    CEditInsertPoint insertPoint;
    insertPoint.m_pElement = m_pRoot->GetFirstMostChild()->Leaf();
    if ( pMultiColumn && pMultiColumn->GetFirstMostChild()->IsLeaf() ) {
        insertPoint.m_pElement = pMultiColumn->GetFirstMostChild()->Leaf();
    }

    SetInsertPoint(insertPoint);
}

XP_Bool CEditBuffer::IsInsertPointInTableCell(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    return ip.m_pElement->GetTableCellIgnoreSubdoc() != NULL;
}

EDT_TableCellData* CEditBuffer::GetTableCellData(){
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        return pTableCell->GetData();
    }
    else {
        return NULL;
    }
}

void CEditBuffer::SetTableCellData(EDT_TableCellData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        pTableCell->SetData( pData );
        Relayout( pTableCell, 0 );
        SetDirtyFlag( TRUE );
    }
}

void CEditBuffer::InsertTableColumns(EDT_TableCellData *pData, XP_Bool bAfterCurrentCell, intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
        AdoptAndDo(new CInsertTableColumnCommand(this, pData, bAfterCurrentCell, number));
    }
}

void CEditBuffer::DeleteTableColumns(intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
		AdoptAndDo(new CDeleteTableColumnCommand(this, number));
    }
}

void CEditBuffer::InsertTableCells(EDT_TableCellData* /* pData */, XP_Bool bAfterCurrentCell, intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
		AdoptAndDo(new CInsertTableCellCommand(this, bAfterCurrentCell, number));
    }
}

void CEditBuffer::DeleteTableCells(intn number){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetTableInsertPoint(ip);
    CEditTableCellElement* pTableCell = ip.m_pElement->GetTableCellIgnoreSubdoc();
    if ( pTableCell ){
		AdoptAndDo(new CDeleteTableCellCommand(this, number));
    }
}

// Multicolumn stuff

XP_Bool CEditBuffer::IsInsertPointInMultiColumn(){
    CEditInsertPoint ip;
    GetInsertPoint(ip);
    return ip.m_pElement->GetMultiColumnIgnoreSubdoc() != NULL;
}

EDT_MultiColumnData* CEditBuffer::GetMultiColumnData(){
    CEditInsertPoint ip;
    GetInsertPoint(ip);
    CEditMultiColumnElement* pMultiColumn = ip.m_pElement->GetMultiColumnIgnoreSubdoc();
    if ( pMultiColumn ){
        return pMultiColumn->GetData();
    }
    else {
        return NULL;
    }
}

void CEditBuffer::SetMultiColumnData(EDT_MultiColumnData *pData){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetInsertPoint(ip);
    CEditMultiColumnElement* pMultiColumn = ip.m_pElement->GetMultiColumnIgnoreSubdoc();
    if ( pMultiColumn ){
        pMultiColumn->SetData( pData );
        Relayout( pMultiColumn, 0 );
        SetDirtyFlag( TRUE );
    }
}

void CEditBuffer::InsertMultiColumn(EDT_MultiColumnData *pData){
    CEditMultiColumnElement *pMultiColumn = new CEditMultiColumnElement(pData->iColumns);
    if( pMultiColumn ){
        pMultiColumn->SetData( pData );
        pMultiColumn->FinishedLoad(this); // Sets default paragraphs for all the cells
        AdoptAndDo(new CInsertMultiColumnCommand(this, pMultiColumn));
    }
}

void CEditBuffer::DeleteMultiColumn(){
    VALIDATE_TREE(this);
    CEditInsertPoint ip;
    GetInsertPoint(ip);
    CEditMultiColumnElement* pMultiColumn = ip.m_pElement->GetMultiColumnIgnoreSubdoc();
    if ( pMultiColumn ){
		AdoptAndDo(new CDeleteMultiColumnCommand(this));
    }
}

EDT_HorizRuleData* CEditBuffer::GetHorizRuleData(){
    CEditLeafElement *pInsertPoint;
    ElementOffset iOffset;
    XP_Bool bSingleItem;

    bSingleItem = GetPropertyPoint( &pInsertPoint, &iOffset );
    XP_ASSERT( bSingleItem );

    XP_ASSERT( pInsertPoint->IsA(P_HRULE) );
    return pInsertPoint->HorizRule()->GetData( );
}

void CEditBuffer::SetHorizRuleData( EDT_HorizRuleData* pData ){
    ClearSelection( TRUE, TRUE );
    XP_ASSERT( m_pCurrent->IsA(P_HRULE) );
    m_pCurrent->HorizRule()->SetData( pData );
    Relayout( m_pCurrent->FindContainer(), 0, m_pCurrent, RELAYOUT_NOCARET );
    SelectCurrentElement();
    SetDirtyFlag( TRUE );
}

void CEditBuffer::InsertHorizRule( EDT_HorizRuleData* pData ){
    if( IsSelected() ){
        ClearSelection();
    }
    CPasteCommand* command = new CPasteCommand(this, kInsertHorizRuleCommandID, FALSE);
    CEditHorizRuleElement *pHorizRule = new CEditHorizRuleElement(0);
    pHorizRule->SetData( pData );
    InsertLeaf( pHorizRule );
    AdoptAndDo(command);
}

char* CEditBuffer::GetTargetData(){
    CEditLeafElement *pInsertPoint;
    ElementOffset iOffset;
    XP_Bool bSingleItem;

    bSingleItem = GetPropertyPoint( &pInsertPoint, &iOffset );
    XP_ASSERT( bSingleItem );

    XP_ASSERT( pInsertPoint->GetElementType() == eTargetElement );
    return pInsertPoint->Target()->GetData( );
}

void CEditBuffer::SetTargetData( char* pData ){
    ClearSelection( TRUE, TRUE );
    XP_ASSERT( m_pCurrent->GetElementType() == eTargetElement );
    m_pCurrent->Target()->SetData( pData );
    Relayout( m_pCurrent->FindContainer(), 0, m_pCurrent, RELAYOUT_NOCARET );
    SelectCurrentElement();
    SetDirtyFlag( TRUE );
}

void CEditBuffer::InsertTarget( char* pData ){
    if( IsSelected() ){
        ClearSelection(TRUE, TRUE);
    }
    CEditTargetElement *pTarget = new CEditTargetElement(0);
    pTarget->SetData( pData );
    InsertLeaf( pTarget );
}

char* CEditBuffer::GetAllDocumentTargets(){
    intn iSize = 500;
    int iCur = 0;
    char *pBuf = (char*)XP_ALLOC( iSize );
    CEditElement *pNext = m_pRoot;

    pBuf[0] = 0;
    pBuf[1] = 0;

    while(NULL != (pNext = pNext->FindNextElement( &CEditElement::FindTarget, 0 )) ){
        char *pName = pNext->Target()->GetData();
        if( pName && *pName ){
            int iLen = XP_STRLEN( pName );
            if( iCur + iLen + 2 > iSize ){
                iSize = iSize + iSize;
                pBuf = (char*)XP_REALLOC( pBuf, iSize );
            }
            XP_STRCPY( &pBuf[iCur], pName );
            iCur += iLen+1;
        }
    }
    pBuf[iCur] = 0;
    return pBuf;
}

#define LINE_BUFFER_SIZE  4096

char* CEditBuffer::GetAllDocumentTargetsInFile(char *pHref){
    intn iSize = 500;
    int iCur = 0;
    char *pBuf = (char*)XP_ALLOC( iSize );
    CEditElement *pNext = m_pRoot;
    pBuf[0] = 0;
    pBuf[1] = 0;

    char *pFilename = NULL;

    // First check if URL is a local file that exists
    XP_StatStruct stat;
    XP_Bool bFreeFilename = FALSE;

#if defined(XP_MAC) || defined(XP_UNIX)
    if ( -1 != XP_Stat(pHref, &stat, xpURL) &&
        stat.st_mode & S_IFREG ) {
#else
    if ( -1 != XP_Stat(pHref, &stat, xpURL) &&
        stat.st_mode & _S_IFREG ) {
#endif
        // We can use unprocessed URL,
        //   and we don't need to free it
        pFilename = pHref;
    } 
    else {
        // We probably have a URL,
        //  get absolute URL path then convert to local format
        char *pAbsolute = NET_MakeAbsoluteURL( LO_GetBaseURL(m_pContext ), pHref );

        if( !pAbsolute || 
            !XP_ConvertUrlToLocalFile(pAbsolute, &pFilename) ){
            if( pFilename ) XP_FREE(pFilename);
            if( pAbsolute ) XP_FREE(pAbsolute);
            return NULL;
        }
        XP_FREE(pAbsolute);
        // We need to free the filename made for us
        bFreeFilename = TRUE;
    }

    // Open local file
    XP_File file = XP_FileOpen( pFilename, xpURL, XP_FILE_READ );
    if( !file ) {
        if( pFilename && bFreeFilename ){
            XP_FREE(pFilename);
        }
        return NULL;
    }

	char    pFileBuf[LINE_BUFFER_SIZE];
    char   *ptr, *pEnd, *pStart;
    size_t  count;
    PA_Tag *pTag;
    char   *pName;

    // Read unformated chunks
    while( 0 < (count = fread(pFileBuf, 1, LINE_BUFFER_SIZE, file)) ){
        // Scan from the end to find end of last tag in block
        ptr = pFileBuf + count -1;
        while( (ptr > pFileBuf) && (*ptr != '>') ) ptr--;

        //Bloody unlikely, but we didn't find a tag!
        if( ptr == pFileBuf ) continue;

        
        // Move file pointer back so next read starts
        //   1 char after the region just found
        int iBack = int(ptr - pFileBuf) -  int(count) + 1;
        fseek( file, iBack, SEEK_CUR );

        // Save the end of "tagged" region
        //  and reset to beginning
        pEnd = ptr;
        ptr = pFileBuf;

FIND_TAG:
        // Scan to beginning of any tag
        while( (ptr < pEnd) && (*ptr != '<') ) ptr++;
        if( ptr == pEnd ) continue;

        // Save start of tag
        pStart = ptr;
        
        // Skip over whitespace before tag name
        ptr++;
        while( (ptr < pEnd) && (XP_IS_SPACE(*ptr)) ) ptr++;
        if( ptr == pEnd ) continue;

        // Check for Anchor tag
        if( ((*ptr == 'a') || (*ptr == 'A')) &&
             XP_IS_SPACE(*(ptr+1)) ){
            // Find end of the tag
            while( (ptr < pEnd) && (*ptr != '>') ) ptr++;
            if( ptr == pEnd ) continue;
                        
            // Parse into tag struct so we can use 
            //   edt_FetchParamString to do the tricky stuff
            // Kludge city. pa_CreateMDLTag needs a pa_DocData solely to
            // look at the line count.
            {
                pa_DocData doc_data;
                doc_data.newline_count = 0;
                pTag = pa_CreateMDLTag(&doc_data, pStart, (ptr - pStart)+1 );
            }
            if( pTag ){
                if( pTag->type == P_ANCHOR ){
                    pName = edt_FetchParamString(pTag, PARAM_NAME);
                    if( pName ){
                        // We found a Name
                        int iLen = XP_STRLEN( pName );
                        if( iCur + iLen + 2 > iSize ){
                            iSize = iSize + iSize;
                            pBuf = (char*)XP_REALLOC( pBuf, iSize );
                        }
                        XP_STRCPY( &pBuf[iCur], pName );
                        iCur += iLen+1;
                        XP_FREE(pName);
                    }
                }
                PA_FreeTag(pTag);
                // Move past the tag we found and search again
                ptr++;
                goto FIND_TAG;
            }
            else {
                // We couldn't find a complete tag, get another block
                continue;
            }
        } else {
            // Not an anchor tag, try again
            ptr++;
            goto FIND_TAG;
        }
    }
    XP_FileClose(file);

    if( pFilename && bFreeFilename ){
        XP_FREE(pFilename);
    }

    pBuf[iCur] = 0;

    return pBuf;
}    

#ifdef FIND_REPLACE
XP_Bool CEditBuffer::FindAndReplace( EDT_FindAndReplaceData *pData ){
    return FALSE;
}
#endif



class CStretchBuffer {
private:
    char *m_pBuffer;
    intn m_iSize;
    intn m_iCur;

public:
    CStretchBuffer();
    ~CStretchBuffer(){}        // intentionall don't destroy the buffer.
    void Add( char *pString );
    char* GetBuffer(){ return m_pBuffer; }
    XP_Bool Contains( char* p );
};

CStretchBuffer::CStretchBuffer(){
    m_pBuffer = (char*)XP_ALLOC( 512 );
    m_iSize = 512;
    m_iCur = 0;
    m_pBuffer[0] = 0;
    m_pBuffer[1] = 0;
}

void CStretchBuffer::Add( char *p ){
    if( p == 0 || *p == 0 ){
        return;
    }
    int iLen = XP_STRLEN( p );
    while( m_iCur + iLen + 2 > m_iSize ){
        m_iSize = m_iSize + m_iSize;
        m_pBuffer = (char*)XP_REALLOC( m_pBuffer, m_iSize );
    }
    XP_STRCPY( &m_pBuffer[m_iCur], p );
    m_iCur += iLen+1;
    m_pBuffer[m_iCur] = 0;
}

XP_Bool CStretchBuffer::Contains( char* p ){
    char *s = GetBuffer();
    while( s && *s ){
        if( XP_STRCASECMP( s, p ) == 0 ){
            return TRUE;
        }
        s += XP_STRLEN(s)+1;
    }
    return FALSE;
}

// Returns true if file URL is in same directory as document.
// If returns TRUE, then filename (no path) is written to *ppFilename 
//   and must be freed by caller
XP_Bool GetExistingFileInSameDirectory( char *pBaseURL, char *pSrcURL, char **ppFilename )
{
    XP_Bool bRetVal = FALSE;

    if( pBaseURL && pSrcURL && ppFilename ){
        if( NET_URL_SAME_DIRECTORY == NET_MakeRelativeURL( pBaseURL, pSrcURL, ppFilename) &&
            *ppFilename ){
    
            char *pAbsoluteURL = NET_MakeAbsoluteURL( pBaseURL, *ppFilename );
            if( pAbsoluteURL ){
                // Returns TRUE if file exists
                bRetVal = XP_ConvertUrlToLocalFile(pAbsoluteURL, NULL);
                XP_FREE(pAbsoluteURL);
            }
        }
    }
    return bRetVal;
} 

char* CEditBuffer::GetAllDocumentFiles(){
    CEditElement *pNext = m_pRoot;
    CStretchBuffer buf;
    char *pRetVal = 0;

    char *pDocURL = LO_GetBaseURL(m_pContext);
    if( !pDocURL ){
        return NULL;
    }
    char *pFilename = NULL;    
    
    // Check all files to make sure they are in same directory as main file
    //   and make sure they exist (else publish will abort)
    if( m_pBackgroundImage &&
        GetExistingFileInSameDirectory( pDocURL, m_pBackgroundImage, &pFilename ) ){
        buf.Add( pFilename );
        XP_FREE(pFilename);

    }

    while(NULL != (pNext = pNext->FindNextElement( &CEditElement::FindImage, 0 )) ){
        EDT_ImageData *pData = pNext->Image()->GetImageData();
        if( pData && pData->pSrc && !buf.Contains( pData->pSrc ) &&
            GetExistingFileInSameDirectory( pDocURL, pData->pSrc, &pFilename ) ){
            buf.Add( pFilename );
            XP_FREE(pFilename);
        }
        if( pData && pData->pLowSrc && !buf.Contains( pData->pLowSrc ) &&
            GetExistingFileInSameDirectory( pDocURL, pData->pLowSrc, &pFilename ) ){
            buf.Add( pFilename );
            XP_FREE(pFilename);
        }
        EDT_FreeImageData( pData );
    }
    pRetVal = buf.GetBuffer();

    // If no files found, return NULL 
    if( pRetVal && *pRetVal == '\0' ){
        XP_FREE(pRetVal);
        pRetVal = NULL;
    }
    return pRetVal;
}

char* CEditBuffer::GetUnknownTagData(){
    CEditLeafElement *pInsertPoint;
    ElementOffset iOffset;
    XP_Bool bSingleItem;

    bSingleItem = GetPropertyPoint( &pInsertPoint, &iOffset );
    XP_ASSERT( bSingleItem );

    XP_ASSERT( pInsertPoint->IsIcon() );
    return pInsertPoint->Icon()->GetData( );
}

void CEditBuffer::SetUnknownTagData( char* pData ){
    ClearSelection( TRUE, TRUE );
    XP_ASSERT( m_pCurrent->GetElementType() == eIconElement );
    m_pCurrent->Icon()->SetData( pData );
    Relayout( m_pCurrent->FindContainer(), 0, m_pCurrent, RELAYOUT_NOCARET );
    SelectCurrentElement();
    SetDirtyFlag( TRUE );
}

void CEditBuffer::InsertUnknownTag( char *pData ){
    if( IsSelected() ){
        ClearSelection();
    }
    if ( ! pData ) {
        XP_ASSERT(FALSE);
        return;
    }

    NormalizeEOLsInString(pData);

    
    Bool bEndTag = ( XP_STRLEN(pData) > 1 && pData[1] == '/' );
    CEditIconElement *pUnknownTag = new CEditIconElement(0,
        bEndTag ? EDT_ICON_UNSUPPORTED_END_TAG :EDT_ICON_UNSUPPORTED_TAG);
    pUnknownTag->SetData( pData );
    InsertLeaf( pUnknownTag );
}

EDT_ListData* CEditBuffer::GetListData(){
    CEditContainerElement *pContainer;
    CEditListElement *pList;
    if( IsSelected() ){
        CEditContainerElement *pEndContainer;
        CEditListElement *pEndList;
        CEditSelection selection;

        // LTNOTE: this is a hack.  It doesen't handle a bunch of cases.
        // It needs to be able to handle multiple lists selected.
        // It should work a little better.

        GetSelection( selection );
        selection.m_start.m_pElement->FindList( pContainer, pList );
        selection.GetClosedEndContainer()->GetLastMostChild()->FindList( pEndContainer, pEndList );
        if( pList != pEndList ){
            return 0;
        }
    }
    else {
        m_pCurrent->FindList( pContainer, pList );
    }
    if( pList ){
        return pList->GetData( );
    }
    else {
        return 0;
    }
}

void CEditBuffer::SetListData( EDT_ListData* pData ){
    VALIDATE_TREE(this);
    CEditContainerElement *pContainer;
    CEditListElement *pList;

    if( IsSelected() ){
        CEditLeafElement *pBegin, *pEnd, *pCurrent;
        ElementOffset iBeginPos, iEndPos;
        XP_Bool bFromStart;
        GetSelection( pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
        pCurrent = pBegin;
        XP_Bool bDone = FALSE;
        do {
            pCurrent->FindList( pContainer, pList );
            if( pList ){
                pList->SetData( pData );
            }

            bDone = (pEnd == pCurrent );    // For most cases
            pCurrent = pCurrent->NextLeafAll();
            bDone = bDone || (iEndPos == 0 && pEnd == pCurrent ); // Pesky edge conditions!
        } while( pCurrent && !bDone );

        // force layout stop displaying the current selection.
        LO_StartSelectionFromElement( m_pContext, 0, 0); 

        CEditElement *pLast = pEnd;
        if( pEnd ){
            pEnd->FindList( pContainer, pList );
            if( pList ) pLast = pList->GetLastMostChild();
        }

        Relayout( pBegin->FindContainer(), 0, pLast, RELAYOUT_NOCARET );
        // Need to force selection.
        SelectRegion(pBegin, iBeginPos, pEnd, iEndPos, bFromStart );
    }
    else {

        m_pCurrent->FindList( pContainer, pList );
        if( pList ){
            pList->SetData( pData );
            Relayout( pList, 0, pList->GetLastMostChild() );
        }
    }
    SetDirtyFlag( TRUE );
}

void CEditBuffer::InsertBreak( ED_BreakType eBreak, XP_Bool bTyping ){
    VALIDATE_TREE(this);

    if( IsSelected() ){
        // ToDo: Consider cutting the selection here, like InsertChar does.
        ClearSelection();
    }

    CTypingCommand* cmd = NULL;
    if ( bTyping ) {
        cmd = GetTypingCommand();
    }
    PA_Tag *pTag = XP_NEW( PA_Tag );
    XP_BZERO( pTag, sizeof( PA_Tag ) );
    pTag->type = P_LINEBREAK;
    SetDirtyFlag( TRUE );

    switch( eBreak ){
        case ED_BREAK_NORMAL:
            break;
        case ED_BREAK_LEFT:
            edt_SetTagData( pTag, "CLEAR=LEFT>" );
            break;
        case ED_BREAK_RIGHT:
            edt_SetTagData( pTag, "CLEAR=RIGHT>" );
            break;
        case ED_BREAK_BOTH:
            edt_SetTagData( pTag, "CLEAR=BOTH>" );
            break;
    }
    if ( bTyping ){
        cmd->AddCharStart();
    }
    CEditBreakElement *pBreak = new CEditBreakElement( 0, pTag );
    InsertLeaf(pBreak);
    FixupSpace(bTyping);
    Reduce(pBreak->GetParent());
#if 0
    CEditElement *pNew = m_pCurrent->Divide(m_iCurrentOffset);
    pBreak->InsertBefore( pNew );
    m_pCurrent = pBreak;
    m_iCurrentOffset = 1;
    CEditElement *pContainer = m_pCurrent->FindContainer();
    Relayout( pContainer, 0,  pContainer->GetLastMostChild() );
#endif
    if ( bTyping ){
        cmd->AddCharEnd();
    }
}

EDT_ClipboardResult CEditBuffer::CanCut(XP_Bool bStrictChecking){
    CEditSelection selection;
    GetSelection(selection);
    return CanCut(selection, bStrictChecking);
}

EDT_ClipboardResult CEditBuffer::CanCut(CEditSelection& selection, XP_Bool bStrictChecking){
    EDT_ClipboardResult result = selection.IsInsertPoint() ? EDT_COP_SELECTION_EMPTY : EDT_COP_OK;
    if ( bStrictChecking && result == EDT_COP_OK
        && selection.CrossesSubDocBoundary() ) {
        result = EDT_COP_SELECTION_CROSSES_TABLE_DATA_CELL;
    }
    return result;
}

EDT_ClipboardResult CEditBuffer::CanCopy(XP_Bool bStrictChecking){
    return CanCut(bStrictChecking); /* In the future we may do something for read-only docs. */
}

EDT_ClipboardResult CEditBuffer::CanCopy(CEditSelection& selection, XP_Bool bStrictChecking){
    return CanCut(selection, bStrictChecking); /* In the future we may do something for read-only docs. */
}

EDT_ClipboardResult CEditBuffer::CanPaste(XP_Bool bStrictChecking){
    CEditSelection selection;
    GetSelection(selection);
    return CanPaste(selection, bStrictChecking);
}

EDT_ClipboardResult CEditBuffer::CanPaste(CEditSelection& selection, XP_Bool bStrictChecking){
    EDT_ClipboardResult result = EDT_COP_OK;
    if ( bStrictChecking
        && selection.CrossesSubDocBoundary() ) {
        result = EDT_COP_SELECTION_CROSSES_TABLE_DATA_CELL;
    }
    return result;
}

XP_Bool CEditBuffer::CanSetHREF(){
    if( IsSelected() 
            || ( m_pCurrent && m_pCurrent->Leaf()->GetHREF() != ED_LINK_ID_NONE ) ){
        return TRUE;
    }
    else {
        return FALSE;
    }
}

//  This needs to be written to:
//    Return data in EDT_HREFData struct containing:
//    pTitle:   ALL the text of elements with same HREF
//              or the image name if only an image is selected
//    pHREF     what we are getting now
//    pTarget   what we are ignoring now!
//    pMocha    "

char *CEditBuffer::GetHREF(){
    CEditLeafElement *pElement = m_pCurrent;
    
    if( IsSelecting() ){
        return 0;
    }

    if( IsSelected() ){
        ElementOffset i,i1;
        CEditLeafElement *pEnd;
        XP_Bool bFromStart;
        GetSelection( pElement, i, pEnd, i1, bFromStart );
    }
    if( pElement ){
        ED_LinkId id = pElement->Leaf()->GetHREF();
        if( id != ED_LINK_ID_NONE ){
            return linkManager.GetHREF(id) ;
        }
        else {
            return 0;
        }
    }
    return 0;
}

char* CEditBuffer::GetHREFText(){
    CEditLeafElement *pElement = m_pCurrent;
    ED_LinkId id;
    char* pBuf = 0;

    if( pElement && pElement->IsText() &&
        (id = pElement->GetHREF()) != ED_LINK_ID_NONE ){
        
        // Move back to find start of contiguous elements
        //  with same HREF
        CEditLeafElement *pStartElement = pElement;
        while ( (pElement = (CEditLeafElement*)pElement->GetPreviousSibling()) &&
                 pElement->IsText() &&
                 pElement->GetHREF() == id ) {
            pStartElement = pElement;
        }

        // We now have starting EditText element
        pElement = pStartElement;
        char* pText = pStartElement->Text()->GetText();
        pBuf = pText ? XP_STRDUP( pText ) : 0;
        if( !pBuf ){
            return 0;
        }

        // Now scan forward to accumulate text
        while ( (pElement = (CEditLeafElement*)pElement->GetNextSibling()) &&
                pElement && pElement->IsText() &&
                pElement->GetHREF() == id ){
            // Cast to access text class (we always test element first)
            // CEditTextElement *pText = (CEditTextElement*)pElement;
            
            pBuf = (char*)XP_REALLOC(pBuf, XP_STRLEN(pBuf) + pElement->Text()->GetSize() + 1);        
            if( !pBuf ){
                return 0;
            }
            char* pText = pElement->Text()->GetText();
            if ( pText ) { 
                strcat( pBuf, pText );
            }
        }
   }
   return pBuf;
}

void CEditBuffer::SetCaret(){
    InternalSetCaret(TRUE);
}

void CEditBuffer::InternalSetCaret(XP_Bool bRevealPosition){
    LO_TextStruct* pText;
    int iLayoutOffset;
    int32 winHeight, winWidth;
    int32 topX, topY;

    if( m_bNoRelayout ){
        return;
    }

    // jhp Check m_pCurrent because IsSelected won't give accurate results when
    // the document is being layed out.
    if( !IsSelected() && m_pCurrent && !m_inScroll ){

        if( m_pCurrent->IsA( P_TEXT ) ){
            if( !m_pCurrent->Text()->GetLOTextAndOffset( m_iCurrentOffset,
                    m_bCurrentStickyAfter, pText, iLayoutOffset ) ){
                return;
            }
        }

        FE_DestroyCaret(m_pContext);

        if ( bRevealPosition ) {
            RevealPosition(m_pCurrent, m_iCurrentOffset, m_bCurrentStickyAfter);
        }
        FE_GetDocAndWindowPosition( m_pContext, &topX, &topY, &winWidth, &winHeight );
        m_lastTopY = topY;

        if( m_pCurrent->IsA( P_TEXT ) ){
            if( !m_pCurrent->Text()->GetLOTextAndOffset( m_iCurrentOffset, m_bCurrentStickyAfter, 
                                                    pText, iLayoutOffset ) ){
                return;
            }

            FE_DisplayTextCaret( m_pContext, FE_VIEW, pText, iLayoutOffset ); 
        }
        else 
        {
            LO_Position effectiveCaretPosition = GetEffectiveCaretLOPosition(m_pCurrent, m_iCurrentOffset, m_bCurrentStickyAfter);
            
            switch ( effectiveCaretPosition.element->type )
            {
                case LO_IMAGE:
                    FE_DisplayImageCaret( m_pContext, 
                                & effectiveCaretPosition.element->lo_image,
                                (ED_CaretObjectPosition)effectiveCaretPosition.position );
                    break;
                default:
                    FE_DisplayGenericCaret( m_pContext,
                                & effectiveCaretPosition.element->lo_any,
                                (ED_CaretObjectPosition)effectiveCaretPosition.position );
                    break;
            }
        }
    }
}


LO_Position CEditBuffer::GetEffectiveCaretLOPosition(CEditElement* pElement, intn iOffset, XP_Bool bCurrentStickyAfter)
{
    LO_Position position;
    int iPos;

	pElement->Leaf()->GetLOElementAndOffset( iOffset, bCurrentStickyAfter, position.element, iPos);
    position.position = iPos;

    // Linefeeds only have a single position.  If you are at the end
    //  of a linefeed, you are really at the beginning of the next line.
    if( position.element->type == LO_LINEFEED && position.position == 1 ){
        //LO_Element *pNext = LO_NextEditableElement( pLoElement );
        LO_Element *pNext = position.element->lo_any.next;
        if( pNext ){
            position.element = pNext;
            position.position = 0;
        }
        else {
            position.position = 0;
        }
    }

    return position; 
}

// A utility function for ensuring that a range is visible. This handles one
// axis; it's called twice, once for each dimension.
//
// All variable names are given in terms of the vertical case. The horizontal
// case is the same.
//
// top - the position of the top edge of the visible portion of the document,
// in the document's coordinate system.
//
// length - the length of the window, in the document's coordinate system.
//
// targetLow - the first position we desire to be visible.
//
// targetHigh - the last position we desire to be visible.
//
// In case both positions can't be made visible, targetLow wins.
//
// upMargin - if we have to move up, where to
// position the targetLow relative to top.
// downMargin - if we have to move down, where to position targetLow relative to top.
//
// The margins should be in the range 0..length - 1
//
// Returns TRUE if top changed.

XP_Bool MakeVisible(int32& top, int32 length, int32 targetLow, int32 targetHigh, int32 upMargin, int32 downMargin)
{
    int32 newTop = top;
    if ( targetLow < newTop ) {
        // scroll up
        newTop = targetLow - upMargin;
        if ( newTop < 0 ) {
            newTop = 0;
        }
    }
    else if ( targetHigh >= (newTop + length) ) {
        // scroll down
        int32 potentialTop = targetHigh - downMargin;
        if ( potentialTop < 0 ) {
            potentialTop = 0;
        }
        if ( potentialTop < targetLow && targetLow < potentialTop + length ) {
            newTop = potentialTop;
        }
        else {
            // Too narrow to show all of cursor. Show just the top part.
            newTop = targetLow - upMargin;
            if ( newTop < 0 ) {
                newTop = 0;
            }
        }
    }

    XP_Bool changed = newTop != top;
    top = newTop;
    return changed;
}

void CEditBuffer::RevealPosition(CEditElement* pElement, int iOffset, XP_Bool bStickyAfter)
{
    int32 winHeight, winWidth;
    int32 topX, topY;
    int32 targetX, targetYLow, targetYHigh;

    if ( ! pElement ) {
       XP_ASSERT(FALSE);
       return;
    }
    FE_GetDocAndWindowPosition( m_pContext, &topX, &topY, &winWidth, &winHeight );

    if ( ! GetLOCaretPosition(pElement, iOffset, bStickyAfter, targetX, targetYLow, targetYHigh) )
        return;

    // XP_TRACE(("top %d %d target %d %d %d", topX, topY, targetX, targetYLow, targetYHigh));

    // The visual position policy is that when moving up or down we stick close to
    // the existing margin, but when moving left or right we move to 2/3rds of the
    // way across the screen. (If we ever support right-to-left text we'll
    // have to revisit this.)

    const int32 kUpMargin = ( winHeight > 30 ) ? 10 : (winHeight / 3);
    const int32 kDownMargin = winHeight - kUpMargin;
    const int32 kLeftRightMargin = winWidth * 2 / 3;
    XP_Bool updateX = MakeVisible(topX, winWidth, targetX, targetX, kLeftRightMargin, kLeftRightMargin);
    XP_Bool updateY = MakeVisible(topY, winHeight, targetYLow, targetYHigh, kUpMargin, kDownMargin);
    
    if ( updateX || updateY ) {
        // XP_TRACE(("new top %d %d", topX, topY));
        XP_Bool bHaveCaret = ! IsSelected();
        if ( bHaveCaret){
            FE_DestroyCaret(m_pContext);
        }
#ifdef XP_UNIX
        FE_ScrollDocTo(m_pContext, FE_VIEW, topX, topY);
#else
        FE_SetDocPosition(m_pContext, FE_VIEW, topX, topY);
#endif
        if ( bHaveCaret){
            InternalSetCaret(FALSE);
        }
    }
}

XP_Bool CEditBuffer::GetLOCaretPosition(CEditElement* pElement, int iOffset, XP_Bool bStickyAfter,
    int32& targetX, int32& targetYLow, int32& targetYHigh)
{
    if ( ! pElement ) {
       XP_ASSERT(FALSE);
       return FALSE;
    }
    LO_Position position = GetEffectiveCaretLOPosition(pElement, iOffset, bStickyAfter);
    FE_GetCaretPosition(m_pContext, &position, &targetX, &targetYLow, &targetYHigh);
    return TRUE;
}

void CEditBuffer::WindowScrolled(){

    if( m_inScroll ){
        return;
    }
    m_inScroll = TRUE;
    if( !IsSelected() ){
        InternalSetCaret(FALSE);
    }

    m_inScroll = FALSE;

 #if 0
    // This code keeps the cursor on the screen, which is pretty weird behavior for
    // a text editor. That's not how any WYSIWYG editor works -- I'm guessing it's
    // some crufty key-based editor feature. -- jhp

    LO_Element* pElement;
    int iLayoutOffset;
    int32 winHeight, winWidth;
    int32 topX, topY;
    int32 iDesiredY;

    if( m_inScroll ){
        return;
    }
    m_inScroll = TRUE;


    FE_GetDocAndWindowPosition( m_pContext, &topX, &topY, &winWidth, &winHeight );


    // if there is a current selection, we have nothing to do.
    if( !IsSelected() ){

        m_pCurrent->Leaf()->GetLOElementAndOffset( m_iCurrentOffset, m_bCurrentStickyAfter,
                                                    pElement, iLayoutOffset );

        // m_iDesiredX is where we would move if we could.  This keeps the
        //  cursor moving, basically, straight up and down, even if there is
        //  no text or gaps
        if( m_iDesiredX == -1 ){
            m_iDesiredX = pElement->lo_any.x + 
                    (m_pCurrent->IsA(P_TEXT) 
                        ? LO_TextElementWidth( m_pContext, 
                                (LO_TextStruct*) pElement, iLayoutOffset)
                        : 0 ); 
        }


        if( pElement->lo_any.y < topY ){
            iDesiredY = (pElement->lo_any.y - m_lastTopY) + topY;
            // caret is above the current window
            LO_PositionCaret( m_pContext, m_iDesiredX, iDesiredY );
        }
        else if( pElement->lo_any.y+pElement->lo_any.height > topY+winHeight ){
            // caret is below the current window
            iDesiredY = (pElement->lo_any.y - m_lastTopY) + topY;
            LO_PositionCaret( m_pContext, m_iDesiredX, iDesiredY);
        }
        else {
            SetCaret(); 
        }
    }
    m_lastTopY = topY;
    m_inScroll = FALSE;

#endif
}

void CEditBuffer::DebugPrintTree(CEditElement* pElement){
    const size_t kIndentBufSize = 256;
    char indent[kIndentBufSize];
    char *pData;
    char *pData1;
    PA_Tag *pTag;

    int indentSize = printState.m_iLevel*2;
    if ( indentSize >= kIndentBufSize ) {
        indentSize = kIndentBufSize - 1;
    }

    CEditElement *pNext = pElement->GetChild();
	int i;
    for(i = 0; i < indentSize; i++ ){
        indent[i] = ' ';
    }
    indent[i] = '\0';

    if( pElement->IsA(P_TEXT) ) {
        pData = pElement->Text()->GetText();
        pData1 = pElement->Text()->DebugFormat();
        pTag = 0;
    }
    else if( pElement->IsContainer() ){
        ED_Alignment alignment = pElement->Container()->GetAlignment();
        if ( alignment < ED_ALIGN_DEFAULT || alignment > ED_ALIGN_ABSTOP ) {
            alignment = ED_ALIGN_ABSTOP;
        }
        pData = lo_alignStrings[alignment];
        pData1 = "";
        pTag = 0;
    }
    else {
        pData1 = "";
        pTag = pElement->TagOpen(0);
        if( pTag ){
            // no multiple tag fetches.
            //XP_ASSERT( pTag->next == 0 );
            PA_LOCK( pData, char*, pTag->data );
        }
        else {
            pData = 0;
        }
    }

    printState.m_pOut->Printf("\n");
    ElementIndex baseIndex = pElement->GetElementIndex();
    ElementIndex elementCount = pElement->GetPersistentCount();
    printState.m_pOut->Printf("0x%08x %6ld-%6ld", pElement, (long)baseIndex, 
                (long)baseIndex + elementCount);
    printState.m_pOut->Printf("%s %s: %s%c", 
            indent, 
            EDT_TagString(pElement->GetType()), 
            pData1,
            (pData ? '"': ' ') );
    if( pData ){
        printState.m_pOut->Write( pData, XP_STRLEN(pData));
        printState.m_pOut->Write( "\"", 1 );
    }

    if( pTag ){
        PA_UNLOCK( pTag->data );
        PA_FreeTag( pTag );
    }

    while( pNext ){
        printState.m_iLevel++;
        DebugPrintTree( pNext );
        printState.m_iLevel--;
        pNext = pNext->GetNextSibling();
    }
}

void CEditBuffer::PrintTree( CEditElement* pElement ){
    CEditElement* pNext = pElement->GetChild();

    pElement->PrintOpen( &printState );
    
    while( pNext ){
        PrintTree( pNext );
        pNext = pNext->GetNextSibling();
    }
    pElement->PrintEnd( &printState );
}

void CEditBuffer::AppendTitle( char *pTitle ){
    if( m_pTitle == 0 ){
        m_pTitle = XP_STRDUP( pTitle );
    }
    else {
        int32 titleLen = XP_STRLEN(m_pTitle)+XP_STRLEN(pTitle)+1;
        m_pTitle = (char*)XP_REALLOC( m_pTitle,
                 titleLen);
        strcat( m_pTitle, pTitle );
    }
}

void CEditBuffer::PrintMetaData( CPrintState *pPrintState ){
    int i = 0;
    EDT_MetaData *pData;
    while( i < m_metaData.Size() ){
        if( (pData = m_metaData[i]) != 0  ){

            // LTNOTE:
            // You can't merge these into a single printf.  edt_MakeParamString
            //  uses a single static buffer.
            //
            pPrintState->m_pOut->Printf( "   <META %s=%s ", 
                    (pData->bHttpEquiv ? "HTTP-EQUIV" : "NAME"),
                    edt_MakeParamString( pData->pName ) );

            pPrintState->m_pOut->Printf( "CONTENT=%s>\n",
                    edt_MakeParamString( pData->pContent ) );
        }
        i++;
    }
}

void CEditBuffer::PrintDocumentHead( CPrintState *pPrintState ){
    pPrintState->m_pOut->Printf(
            "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n"
            "<HTML>\n"
            "<HEAD>\n"
            "   <TITLE>");
    edt_PrintWithEscapes( pPrintState, m_pTitle, FALSE );
    pPrintState->m_pOut->Printf( 
                  "</TITLE>\n");

    PrintMetaData( pPrintState );
    if( GetParseState()->m_pJavaScript ){
        int32 iLen = GetParseState()->m_pJavaScript->GetLen();
        XP_HUGE_CHAR_PTR pScript = GetParseState()->m_pJavaScript->GetText();

        // The <SCRIPT> tag, and its parameters, are in the saved text.
        // pPrintState->m_pOut->Printf("<SCRIPT>");
        while( iLen ){
            int16 iToWrite = (int16) min( iLen, 0x4000 );
            pPrintState->m_pOut->Write( pScript, iToWrite );
            iLen -= iToWrite;
            pScript += iToWrite;
        }
        pPrintState->m_pOut->Printf("</SCRIPT>\n");
    }
    pPrintState->m_pOut->Printf( 
            "</HEAD>\n"
            "<BODY");

    // print the contents of the body tag.
    if( m_colorText.IsDefined() ) {
        pPrintState->m_pOut->Printf(" TEXT=\"#%06lX\"", m_colorText.GetAsLong() );
    }
    if( m_colorBackground.IsDefined() ) {
        pPrintState->m_pOut->Printf(" BGCOLOR=\"#%06lX\"", m_colorBackground.GetAsLong() );
    }
    if( m_colorLink.IsDefined() ) {
        pPrintState->m_pOut->Printf(" LINK=\"#%06lX\"", m_colorLink.GetAsLong() );
    }
    if( m_colorFollowedLink.IsDefined() ) {
        pPrintState->m_pOut->Printf(" VLINK=\"#%06lX\"", m_colorFollowedLink.GetAsLong() );
    }
    if( m_colorActiveLink.IsDefined() ) {
        pPrintState->m_pOut->Printf(" ALINK=\"#%06lX\"", m_colorActiveLink.GetAsLong() );
    }
    if( m_pBackgroundImage ) {
        pPrintState->m_pOut->Printf(" BACKGROUND=%s", edt_MakeParamString(m_pBackgroundImage) );
    }

    if( m_pBodyExtra ) {
        pPrintState->m_pOut->Printf(" %s", m_pBodyExtra );
    }

    pPrintState->m_pOut->Printf(">\n");
}

void CEditBuffer::PrintDocumentEnd( CPrintState *pPrintState ){
    pPrintState->m_pOut->Printf( "\n"
            "</BODY>\n"
            "</HTML>\n");
}


// Setup to save file
ED_FileError CEditBuffer::SaveFile( char* pSourceURL,
                                    char* pDestURL,
                                    XP_Bool bSaveAs,
                                    XP_Bool bKeepImagesWithDoc,
                                    XP_Bool bAutoAdjustLinks ){
    DoneTyping();
    CEditSaveObject *pSaveObject;
    if( m_pSaveObject != 0 ){
        // This could happen if user is in the middle of saving a file
        //  (such as at an overwrite or error dialog when inserting image)
        //  and then AutoSave triggers saving the file. So don't ASSERT
        // XP_ASSERT(0);           // shouldn't be returning a value...
        return ED_ERROR_BLOCKED;
    }
    // Set write time to 0 so we don't trigger reload before file is saved
    // (doesn't completely solve this problem)
    if( bSaveAs) m_iFileWriteTime = 0;
    m_pSaveObject = pSaveObject = new CEditSaveObject( this, pSourceURL, 
            pDestURL, bSaveAs, bKeepImagesWithDoc, bAutoAdjustLinks );
    pSaveObject->FindAllDocumentImages();
    // This may save 1 or more files and delete itself before
    //  returning here. Error status (m_status) is set to pSaveObject->m_status
    //  via CEditBuffer::FileFetchComplete(m_status)
    m_status = ED_ERROR_NONE;
    pSaveObject->SaveFiles();
#if 0
    ED_FileError result = pSaveObject->SaveFiles();
    if ( result == ED_ERROR_NONE ) {
        m_bDirty = FALSE;
    }
    return result;
#endif
    return m_status;
}

ED_FileError CEditBuffer::WriteBufferToFile( char *pDestURL ){

    XP_File      hFile = NULL;
    ED_FileError nError = ED_ERROR_NONE;
    CEditFileBackup fb;
    
    nError = fb.BeginTransaction( pDestURL );
    if( nError != ED_ERROR_NONE ){
        return nError;
    }    


    // Note: xpTemporary gives us maximum private write permission
    //       but xpURL must be used (at least in Window platform)
    //       to prevent WH_FileName() from changing our directory or extension
#ifdef XP_MAC
    hFile = XP_FileOpen(fb.FileName(), xpURL, XP_FILE_TRUNCATE );
#else
    hFile = XP_FileOpen(fb.FileName(), xpTemporary, XP_FILE_TRUNCATE );
#endif
    if ( hFile == NULL ) {
        fb.Rollback();
        return ED_ERROR_FILE_OPEN;
    }

    if ( 0 != WriteToFile( hFile ) ) {
        fb.Rollback();
        return ED_ERROR_FILE_WRITE;
    }
    fb.Commit();
    return nError;
}

// CM: Return 0 if OK, or -1 if device is full.
int CEditBuffer::WriteToFile( XP_File hFile ){ //char *pFileName ){
    int iResult = 0;
    CStreamOutFile *pOutFile;
    pOutFile = new CStreamOutFile(hFile, FALSE);
    edt_InitEscapes(m_pContext);
    printState.Reset( pOutFile, this );
    Reduce( m_pRoot );
    PrintTree( m_pRoot );
    SetDirtyFlag( FALSE );
    printState.Reset( 0, this );
    if( pOutFile->Status() != IStreamOut::EOS_NoError ){
        iResult = -1;
    }
    else {
        iResult = 0;
    }
    delete pOutFile;
    // To Jack: this is a problem
    // When saving to a new file, this does nothing
    // 7/2/96 
    //   edit_saving_url flag is set/cleared in CEditSaveObject
    //   and GetFileWriteTime() is done in that object's destructor 
    // GetFileWriteTime();
    m_autoSaveTimer.Restart();
    return iResult;
}

void CEditBuffer::WriteToBuffer( char **ppBuffer ){
    CStreamOutMemory *pOutMemory = new CStreamOutMemory();
    edt_InitEscapes(m_pContext);
    printState.Reset( pOutMemory, this );
    DebugPrintTree( m_pRoot );
    printState.Reset( 0, this );
    *ppBuffer = pOutMemory->GetText();
    delete pOutMemory;
}

#ifdef DEBUG
void CEditBuffer::DebugPrintState(IStreamOut& stream) {
    {
        CEditSelection selection;
        GetSelection(selection);
        stream.Printf("\nCurrent selection: ");
        selection.Print(stream);
        stream.Printf("\n");
    }
    {
        CPersistentEditSelection selection;
        GetSelection(selection);
        stream.Printf("\nCurrent selection: ");
        selection.Print(stream);
        stream.Printf("\n");
    }
    if ( IsPhantomInsertPoint() ) {
        stream.Printf("   IsPhantomInsertPoint() = TRUE\n");
    }
    stream.Printf("Current typing command: ");
    if ( m_pTypingCommand ) {
        stream.Printf("0x%08lx (should be first on Undo)", m_pTypingCommand);
    }
    else {
        stream.Printf("none");
    }
    stream.Printf("\n");
    m_commandLog.Print(stream);

    {
        stream.Printf("\n\nFiles asociated with the current Buffer:");
        char*p = GetAllDocumentFiles();
        char *p1 = p;
        while( p && *p ){
            stream.Printf("\n  %s", p );
            p += XP_STRLEN( p );
        }
        if( p1) XP_FREE( p1 );
    }

    {
        stream.Printf("\n\nNamed Anchors in the current Buffer:");
        char*p = GetAllDocumentTargets();
        char *p1 = p;
        while( p && *p ){
            stream.Printf("\n  %s", p );
            p += XP_STRLEN( p );
        }
        if( p1 ) XP_FREE( p1 );
    }
    // lo_PrintLayout(m_pContext);
}
#endif

#ifdef DEBUG
void CEditBuffer::ValidateTree(){
    if ( m_bSkipValidation ) return;
    // Have the tree validate itself.
    m_pRoot->ValidateTree();
    if ( ! m_bSuppressPhantomInsertPointCheck ) {
        // Check that there are no phantom insert points other than at m_pCurrent
        CEditLeafElement* pElement;
        for ( pElement = m_pRoot->GetFirstMostChild()->Leaf();
            pElement;
            pElement = pElement->NextLeafAll() ){
            CEditInsertPoint ip(pElement, 0);
            if ( IsPhantomInsertPoint(ip) ){
                if ( pElement != m_pCurrent ) {
                    XP_ASSERT(FALSE);
                }
            }
        }
    }
}

void CEditBuffer::SuppressPhantomInsertPointCheck(XP_Bool bSuppress){
    if ( bSuppress )
        m_bSuppressPhantomInsertPointCheck++;
    else
        m_bSuppressPhantomInsertPointCheck--;

}
#endif

void CEditBuffer::DisplaySource( ){
    CStreamOutNet *pOutNet = new CStreamOutNet( m_pContext );
    edt_InitEscapes(m_pContext);
    printState.Reset( pOutNet, this );
#ifdef DEBUG
    DebugPrintTree( m_pRoot );
    DebugPrintState(*pOutNet);
#else
    PrintTree( m_pRoot );
#endif
    printState.Reset( 0, this );
    delete pOutNet;
}


char* CEditBuffer::NormalizeText( char* pSrc ){
    char *pStr = pSrc;
    char *pDest;
    char lastChar = 1;
    pDest = pStr;
    
    if( GetParseState()->bLastWasSpace ){
        while( *pStr != '\0' && XP_IS_SPACE( *pStr ) ){
            lastChar = ' ';
            pStr++;
        }
    }

    //
    // Loop through convert all white space to a single white space.
    //
    while( *pStr != '\0'){
        if( XP_IS_SPACE( *pStr ) ){
            lastChar = *pDest++ = ' ';
            while( *pStr != '\0' && XP_IS_SPACE( *pStr )){
                pStr++;
            }
        }
        else {
            lastChar = *pDest++ = *pStr++;
        }
    }
    *pDest = '\0';

    // if we actually coppied something, check the last character.
    if( pSrc != pDest ){
        GetParseState()->bLastWasSpace = XP_IS_SPACE( lastChar );
    }
    return pSrc;
}


void CEditBuffer::RepairAndSet(CEditSelection& selection)
{
    // force layout stop displaying the current selection.
    LO_StartSelectionFromElement( m_pContext, 0, 0);
    CPersistentEditSelection persel = EphemeralToPersistent(selection);
    Reduce(selection.GetCommonAncestor());
    selection = PersistentToEphemeral(persel);
    Relayout( selection.m_start.m_pElement, 0, selection.m_end.m_pElement, RELAYOUT_NOCARET );
    // Need to force selection.
    SetSelection(selection);
}

XP_Bool CEditBuffer::Reduce( CEditElement *pRoot ){
    // work backwards so when children go away, we don't have 
    //  lossage.
    CEditElement *pChild, *pNext;
    pChild = pRoot->GetChild();

    while( pChild ) {
        pNext = pChild->GetNextSibling();
        if( Reduce( pChild )){
            pChild->Unlink();       // may already be unlinked
            delete pChild;
        }
        pChild = pNext;
    }
    return pRoot->Reduce( this );
}

//
//  
//
void CEditBuffer::NormalizeTree(){
    CEditElement* pElement;
    
    // Make sure all text is in a container
    pElement = m_pRoot;
    while( (pElement = pElement->FindNextElement( &CEditElement::FindTextAll,0 )) 
            != 0 ) {
        // this block of text does not have a container.
        if( pElement->FindContainer() == 0 ){
        }
    }
    
}

// Finish load timer.

CFinishLoadTimer::CFinishLoadTimer(){
}

void CFinishLoadTimer::OnCallback() {
    m_pBuffer->FinishedLoad2();
}

void CFinishLoadTimer::FinishedLoad(CEditBuffer* pBuffer)
{
    m_pBuffer = pBuffer;
	const uint32 kRelayoutDelay = 1;
    SetTimeout(kRelayoutDelay);
}

void CEditBuffer::FinishedLoad(){
    // To avoid race conditions, set up a timer.
    // An example of a race condition is when the
    // save dialog box goes up when the load is
    // finished, but the image
    // attributes haven't been set yet.
    m_finishLoadTimer.FinishedLoad(this);
}

// Relayout the whole document.
void CEditBuffer::FinishedLoad2(){
    m_pCreationCursor = NULL;

    // Get rid of empty items.
    Reduce( m_pRoot );

    // Give everyone a chance to clean themselves up

    m_pRoot->FinishedLoad(this);

    // May have cleared out some more items.
    Reduce( m_pRoot );

#ifdef DEBUG
    m_pRoot->ValidateTree();
#endif

    m_pCurrent = m_pRoot->NextLeafAll();
    m_iCurrentOffset = 0;

    // Set page properties (color, background, etc.)
    //  for a new document:
    if( EDT_IS_NEW_DOCUMENT(m_pContext) ){
        // This will call EDT_SetPageData(),
        // which calls RefreshLayout()
        FE_SetNewDocumentProperties(m_pContext);
        m_bDummyCharacterAddedDuringLoad = TRUE; /* Sometimes it's a no-break space. */
    }
    else {
        RefreshLayout();
    }

    if ( m_bDummyCharacterAddedDuringLoad ) {
        // Get rid of that damn non-breaking space character
        EDT_ClipboardResult deleteResult = DeleteNextChar();
        XP_ASSERT(deleteResult == EDT_COP_OK);

        // EDT_SetPageData() sets this TRUE,
        //  but we are just a new doc, so clear it
       SetDirtyFlag( FALSE );
        Trim(); // Get rid of undo log in case FE_SetNewDocumentProperties called back.
    }


#if 0
        // Stamp charset info to newly created document
        char buf[100];
        XP_STRCPY(buf,  "text/html;charset=");
        INTL_CharSetIDToName(m_pContext->win_csid,  buf + strlen(buf));
        EDT_MetaData *pData = MakeMetaData( TRUE, "Content-Type", buf);
        SetMetaData( pData );
        FreeMetaData(pData);
#endif

 	char generatorValue[300];
	XP_SPRINTF(generatorValue, "%.90s/%.100s [%.100s]", XP_AppCodeName, XP_AppVersion, XP_AppName);

    EDT_MetaData *pData = MakeMetaData( FALSE, "GENERATOR", generatorValue);

    SetMetaData( pData );
    FreeMetaData( pData );

    m_bBlocked = FALSE;

    // Notify front end that user interaction can resume
    FE_EditorDocumentLoaded( m_pContext );
}

void CEditBuffer::DummyCharacterAddedDuringLoad(){
    m_bDummyCharacterAddedDuringLoad = TRUE;
}

XP_Bool CEditBuffer::IsFileModified(){
    int32 last_write_time = m_iFileWriteTime;

    //Get current time and save in m_iFileWriteTime,
    // but NOT if in the process of saving a file
    if( m_pContext->edit_saving_url ){
        return FALSE;
    }

    GetFileWriteTime();
    // Skip if current_time is 0 -- first time through
    return( last_write_time != 0 && last_write_time != m_iFileWriteTime );
}

void CEditBuffer::GetFileWriteTime(){
    char *pDocURL = LO_GetBaseURL(m_pContext);
    char *pFilename = NULL;

    // Don't set the time if we are in the process of saving the file
    if( m_pContext->edit_saving_url ){
        return;
    }

    // Must be a local file type
    if( NET_IsLocalFileURL(pDocURL) &&
        XP_ConvertUrlToLocalFile(pDocURL, &pFilename) ){
        XP_StatStruct statinfo;
        if( -1 != XP_Stat(pFilename, &statinfo, xpURL) && 
            statinfo.st_mode & S_IFREG ){
            m_iFileWriteTime = statinfo.st_mtime;
        }
    }
    // pFilename is allocated even if XP_ConvertUrlToLocalFile returns FALSE
    if( pFilename ){
        XP_FREE(pFilename);
    }
}

static void SetDocColor(MWContext* pContext, int type, ED_Color& color)
{
    LO_Color *pColor = edt_MakeLoColor(color);
    // If pColor is NULL, this will use the "MASTER",
    //  (default Browser) color, and that's OK!
    LO_SetDocumentColor( pContext, type, pColor );
    if( pColor) XP_FREE(pColor);
}

void CEditBuffer::RefreshLayout(){
    VALIDATE_TREE(this);

    SetDocColor(m_pContext, LO_COLOR_BG, m_colorBackground);
    SetDocColor(m_pContext, LO_COLOR_FG, m_colorText);
    SetDocColor(m_pContext, LO_COLOR_LINK, m_colorLink);
    SetDocColor(m_pContext, LO_COLOR_VLINK, m_colorFollowedLink);
    SetDocColor(m_pContext, LO_COLOR_ALINK, m_colorActiveLink);

    LO_SetBackgroundImage( m_pContext, m_pBackgroundImage );

    Relayout(m_pRoot, 0, m_pRoot->GetLastMostChild() );
}

void CEditBuffer::SetDisplayParagraphMarks(XP_Bool bDisplay) {
    m_pContext->display_paragraph_marks = bDisplay;
    RefreshLayout();
}

XP_Bool CEditBuffer::GetDisplayParagraphMarks() {
    return m_pContext->display_paragraph_marks;
}


void CEditBuffer::SetDisplayTables(XP_Bool bDisplay) {
    m_bDisplayTables = bDisplay;
    RefreshLayout();
}

XP_Bool CEditBuffer::GetDisplayTables() {
    return m_bDisplayTables;
}

ED_TextFormat CEditBuffer::GetCharacterFormatting( ){
    CEditLeafElement *pElement = m_pCurrent;
    
    // 
    // While selecting, we may not have a valid region
    //
    if( IsSelecting() ){
        return TF_NONE;
    }

    if( IsSelected() ){
        ElementOffset i,i1;
        CEditLeafElement *pEnd;
        XP_Bool bFromStart;
        GetSelection( pElement, i, pEnd, i1, bFromStart );
    }

    if( pElement && pElement->IsA(P_TEXT )){
        return pElement->Text()->m_tf;
    }

    return TF_NONE;
}

int CEditBuffer::GetFontSize( ){
    CEditLeafElement *pElement = m_pCurrent;

    if( IsSelecting() ){
        return 0;
    }

    if( IsSelected() ){
        ElementOffset i,i1;
        CEditLeafElement *pEnd;
        XP_Bool bFromStart;
        GetSelection( pElement, i, pEnd, i1, bFromStart );
    }
    if( pElement && pElement->IsA(P_TEXT )){
        return pElement->Text()->GetFontSize();
    }

	//
	// The following logic is similar to CEditBuffer::GetCharacterData().
	//

	// Try to find the previous text element and use its font size.
	if (pElement) {
		CEditTextElement *tElement = pElement->PreviousTextInContainer();
		if ( tElement){
			return tElement->GetFontSize();
		}

		// Get font size from parent of pElement.
		return pElement->GetDefaultFontSize();
	}
	
    return 0;
}

ED_Color CEditBuffer::GetFontColor( ){
    CEditLeafElement *pElement = m_pCurrent;

    if( IsSelecting() ){
        return ED_Color::GetUndefined();
    }

    if( IsSelected() ){
        ElementOffset i,i1;
        CEditLeafElement *pEnd;
        XP_Bool bFromStart;
        GetSelection( pElement, i, pEnd, i1, bFromStart );
    }
    if( pElement && pElement->IsA(P_TEXT )){
        return pElement->Text()->GetColor();
    }
	return ED_Color::GetUndefined();
}

TagType CEditBuffer::GetParagraphFormatting(){
    CEditContainerElement *pCont;

#if 0
    CEditLeafElement *pInsertPoint;
    // The old way - returns paragraph style at beginning of selection
    //  even if it contains multiple styles
    ElementOffset iOffset;
    GetPropertyPoint( &pInsertPoint, &iOffset );
    if( pInsertPoint && (pCont = pInsertPoint->FindContainer()) != 0){
        return pCont->GetType();
    } 
    else {
        return P_UNKNOWN;
    }
#endif

    TagType type = P_UNKNOWN;
    
    if( IsSelected() ){
        // Grab the current selection.
        CEditSelection selection;
        GetSelection(selection);
        CEditContainerElement* pStart = selection.m_start.m_pElement->FindContainer();
        CEditContainerElement* pEnd = selection.m_end.m_pElement->FindContainer();
        XP_Bool bUseEndContainer = !selection.EndsAtStartOfContainer();

        // Scan for all text elements and save first container type found
        // return that type only if all selected text elements have the same type
        while( pStart ){
            if( type == P_UNKNOWN ){
                // First time through
                type = pStart->GetType();
            } else if( type != pStart->GetType() ) {
                // Type is different - get out now
                return P_UNKNOWN;
            }
            // Start = end, we're done
            if( pStart == pEnd ){
                break;
            }
            CEditElement* pLastChild = pStart->GetLastMostChild();
            if ( ! pLastChild ) break;
            CEditElement* pNextChild = pLastChild->NextLeaf();
            if ( ! pNextChild ) break;
            pStart = pNextChild->FindContainer();
            if ( pStart == pEnd && ! bUseEndContainer ) break;
        }
    }
    else {
        if( m_pCurrent && (pCont = m_pCurrent->FindContainer()) != 0){
            type = pCont->GetType();
        } 
        else {
            type = P_UNKNOWN;
        }
    }
    return type;        
}

void CEditBuffer::PositionCaret(int32 x, int32 y) {
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();    
    DoneTyping();
    ClearMove();
    LO_PositionCaret( m_pContext, x, y );
}

// this routine is called when the mouse goes down.
//
void CEditBuffer::StartSelection( int32 x, int32 y, XP_Bool doubleClick ){
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();    
    {
        // This is a hack to avoid auto-scrolling to the old selection.
        XP_Bool scrolling = m_inScroll;
        m_inScroll = TRUE;
        ClearSelection();
        m_inScroll = scrolling;
    }

    FE_DestroyCaret(m_pContext);
    ClearMove();
    DoneTyping();
    if ( doubleClick ) {
        LO_DoubleClick( m_pContext, x, y );
    }
    else {
        LO_Click( m_pContext, x, y, FALSE );
    }
}

// this routine is called when the right mouse goes down.
//
void CEditBuffer::SelectObject( int32 x, int32 y ){
    PositionCaret(x,y);
    ClearSelection();
    FE_DestroyCaret(m_pContext);
    LO_SelectObject(m_pContext, x, y);
}

//
//
// this routine is called when the mouse is moved while down.
//  at this point we actually decide that we are inside a selection.
//
void CEditBuffer::ExtendSelection( int32 x, int32 y ){
    BeginSelection();
    LO_ExtendSelection( m_pContext, x, y );
    // may not result in an actual selection.
    //  if this is the case, just position the caret.
    if( ! IsSelected() ) {
        // If there's no selection, it is still possible that a
        // selection has been started.
        if ( ! LO_IsSelectionStarted( m_pContext ) ) {
            StartSelection( x, y, FALSE );
        }
        else
        {
            // The selection is the empty selection.
            //ClearSelection();
            //FE_DestroyCaret(m_pContext);
            //ClearMove();
        }
    }
    else {
        m_pCurrent = 0;
        m_iCurrentOffset = 0;
     }
} 

void CEditBuffer::ExtendSelectionElement( CEditLeafElement *pEle, int iOffset, XP_Bool bStickyAfter ){
    int iLayoutOffset;
    LO_Element* pLayoutElement;
    BeginSelection();

    if( IsSelected() ){
        LO_Element *pStart, *pEnd;
        intn iStartPos, iEndPos;
        XP_Bool bFromStart;
        LO_GetSelectionEndPoints( m_pContext,
                    &pStart, &iStartPos,
                    &pEnd, &iEndPos,
                    &bFromStart, 0);
        if( !(pStart == pEnd && iStartPos == iEndPos) ){
            if( iOffset ) iOffset--;
        }
    }
    else {
        if( iOffset ) iOffset--;
    }
        
    
    // we need to convert a selection edit iOffset to a Selection 
    //  offset.  This will only work when selecting to the right...

    pEle->GetLOElementAndOffset( iOffset, bStickyAfter,
               pLayoutElement, iLayoutOffset );

    LO_ExtendSelectionFromElement( m_pContext, pLayoutElement, 
                iLayoutOffset, FALSE );
}

void CEditBuffer::SelectAll(){
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();
    ClearMove();
    DoneTyping();
	// Clear selection (don't resync insertion point to avoid scrolling.)
	ClearSelection(FALSE);
    // Destroy the cursor. Perhaps this is something ClearSelection should do.
    FE_DestroyCaret(m_pContext);
	// Delegate actual selection to the layout engine
    if( LO_SelectAll(m_pContext) ) {
	    // Clear insertion point
	    m_pCurrent = 0;
	    m_iCurrentOffset = 0;
	}
    RevealSelection();
}


void CEditBuffer::SelectTable(){
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();
    ClearMove();
    DoneTyping();
    CEditSelection selection;
    GetSelection(selection);
    CEditTableElement* pStartTable = selection.m_start.m_pElement->GetTableIgnoreSubdoc();
    CEditSelection startAll;
    if ( pStartTable ) {
        pStartTable->GetAll(startAll);
    }
    CEditTableElement* pEndTable = selection.m_end.m_pElement->GetTableIgnoreSubdoc();
    CEditSelection endAll;
    if ( pEndTable ) {
        pEndTable->GetAll(endAll);
    }
    if ( pStartTable ) {
        if ( pEndTable ) {
            // Both a start table and an end table.
            selection.m_start = startAll.m_start;
            selection.m_end = endAll.m_end;
        }
        else {
            // Just a start table.
            selection = startAll;
        }
    } else {
        if ( pEndTable ) {
            // Just an end table.
            selection = endAll;
        }
        else {
            // No table. Don't change selection.
            return;
        }
    }

    SetSelection(selection);
}

void CEditBuffer::SelectTableCell(){
    VALIDATE_TREE(this);
    ClearPhantomInsertPoint();
    ClearMove();
    DoneTyping();
    CEditSelection selection;
    GetSelection(selection);
    CEditTableCellElement* pStartTableCell =
        selection.m_start.m_pElement->GetTableCellIgnoreSubdoc();
    CEditSelection startAll;
    if ( pStartTableCell ) {
        pStartTableCell->GetAll(startAll);
        // KLUDGE ALERT: Without this, attempts to set cell data
        //  go to next cell. As it is, there is a bug in which
        //  caret is in next cell after unselecting this cell
        startAll.m_bFromStart = TRUE;
        AdoptAndDo(new CSetSelectionCommand(this, startAll));
    }
}


void CEditBuffer::BeginSelection( XP_Bool bExtend, XP_Bool bFromStart ){
    ClearMove(); // For side effect of flushing table formatting timer.
    if( !IsSelected() ){
        int iLayoutOffset;
        CEditLeafElement *pNext;
        LO_Element* pLayoutElement;

        m_bSelecting = TRUE;

        FE_DestroyCaret( m_pContext );

        ClearPhantomInsertPoint();
#if 0
        if( m_pCurrent->IsA( P_TEXT )) {        
            if( m_pCurrent->Text()->GetLen() == m_iCurrentOffset 
                    && (pNext = m_pCurrent->TextInContainerAfter())
                        != 0){
                pNext->Text()->GetLOElementAndOffset( 0, FALSE,
                        pLayoutElement, iLayoutOffset );

            }
            else {
                m_pCurrent->Text()->GetLOElementAndOffset( m_iCurrentOffset, m_bCurrentStickyAfter,
                        pLayoutElement, iLayoutOffset );
            }
        }
#endif
        // If we're at the end of an element, and the next element isn't a break,
        // then move the cursor to the start of the next element.
        if ( m_pCurrent->GetLen() == m_iCurrentOffset
                && (pNext = m_pCurrent->LeafInContainerAfter()) != 0
                && ! m_pCurrent->CausesBreakAfter() && ! pNext->CausesBreakBefore()){
            pNext->GetLOElementAndOffset( 0, FALSE,
                    pLayoutElement, iLayoutOffset );
        }
        else {
            XP_Bool good = m_pCurrent->Leaf()->GetLOElementAndOffset( m_iCurrentOffset, m_bCurrentStickyAfter,
                        pLayoutElement, iLayoutOffset );
            if ( ! good ) {
                XP_ASSERT(FALSE);
                return;
            }
        }

        // if we're selecting forward, and
        // if we are before the space that cause the wrap, we should be 
        //  starting selection from the line feed.
        if( ! bFromStart && pLayoutElement->type == LO_TEXT 
                    && iLayoutOffset == pLayoutElement->lo_text.text_len ){
            if( pLayoutElement->lo_any.next->type == LO_LINEFEED ){
                pLayoutElement = pLayoutElement->lo_any.next;
                iLayoutOffset = 0;
            }
            else{
                // The layout element is shorter than we think it should
                //  be.
                XP_ASSERT( FALSE );
            }
        }

        XP_ASSERT( pLayoutElement );

        if ( bExtend ) {
            if ( ! LO_SelectElement( m_pContext, pLayoutElement, 
                    iLayoutOffset, bFromStart ) ) {
                SetCaret(); // If the selection moved off the end of the document, set the caret again.
            }
        }
        else {
            LO_StartSelectionFromElement( m_pContext, pLayoutElement, 
                    iLayoutOffset);
        }
    }
}

void CEditBuffer::EndSelection(int32 /* x */, int32 /* y */){
    EndSelection();
}

void CEditBuffer::EndSelection( )
{
    // Clear the move just in case EndSelection is called out of order
    // (this seems to be happening in the WinFE when you click a lot.)
    // ClearMove has a side-effect of flushing the relayout timer.
    ClearMove();

    // Fix for the mouse move that does not move enough
    //   to show selection (kills caret)

    if( LO_IsSelectionStarted(m_pContext) ){
        SetCaret();
    }
    m_bSelecting = FALSE;

    // Make the active end visible
    RevealSelection();
}

void CEditBuffer::RevealSelection()
{
    if ( IsSelected() ) {
        CEditLeafElement* pStartElement;
        ElementOffset iStartOffset;
        CEditLeafElement* pEndElement;
        ElementOffset iEndOffset;
        XP_Bool bFromStart;
        GetSelection( pStartElement, iStartOffset, pEndElement, iEndOffset, bFromStart );
        if ( bFromStart ) {
            RevealPosition(pStartElement, iStartOffset, FALSE);
        }
        else {
            RevealPosition(pEndElement, iEndOffset, FALSE);
        }
    }
    else
    {
        if(m_pCurrent != NULL){
            RevealPosition(m_pCurrent, m_iCurrentOffset, m_bCurrentStickyAfter);
        }
    }
}

void CEditBuffer::SelectCurrentElement(){
    FE_DestroyCaret(m_pContext);
    SelectRegion( m_pCurrent, 0, m_pCurrent, 1, 
        TRUE, TRUE );
    m_pCurrent = 0;
    m_iCurrentOffset = 0;
}


void CEditBuffer::ClearSelection( XP_Bool bResyncInsertPoint, XP_Bool bKeepLeft ){

    if( !bResyncInsertPoint ){
       LO_StartSelectionFromElement( m_pContext, 0, 0); 
       return;
    }

    CEditSelection selection;
    GetSelection(selection);
    selection.ExcludeLastDocumentContainerEnd();
    SetInsertPoint(*selection.GetEdge( ! bKeepLeft ));
#if 0
    LO_Element *pStart, *pEnd;
    intn iStartPos, iEndPos;
    CEditLeafElement *pEle;
    intn iOffset;
    XP_Bool bFromStart;
    if( IsSelected() ){

        //
        // Grab the current selection.
        //            
        LO_GetSelectionEndPoints( m_pContext,
                    &pStart, &iStartPos,
                    &pEnd, &iEndPos,
                    &bFromStart, 0 );

        XP_ASSERT( pStart );
        XP_ASSERT( pEnd );

        if( pStart == pEnd && iEndPos == iStartPos ){
            bFromStart = TRUE;
        }

        LO_StartSelectionFromElement( m_pContext, 0, 0); 
        m_bSelecting = FALSE;

        if( bFromStart || bKeepLeft ){
            while( pStart && pStart->lo_any.edit_element == 0 ){
                pStart = pStart->lo_any.next;
                if( pStart &&pStart->type == LO_TEXT ){
                    iStartPos = pStart->lo_text.text_len;
                    if( iStartPos ) iStartPos--;
                }
                else {
                    iStartPos = 0;
                }
            }
            if( pStart ){
                pEle = pStart->lo_any.edit_element->Leaf(),
                iOffset = pStart->lo_any.edit_offset+iStartPos;            
                SetInsertPoint( pEle, iOffset,  iStartPos == 0);
            }
            else{
                XP_ASSERT(FALSE);
                // go to the end of document
                NavigateChunk( FALSE, LO_NA_DOCUMENT, TRUE );
                //EndOfDocument( FALSE );
            }
        }
        else {
            while( pEnd && pEnd->lo_any.edit_element == 0 ){
                pEnd = pEnd->lo_any.prev;
                if( pEnd && pEnd->type == LO_TEXT ){
                    iEndPos = pEnd->lo_text.text_len;
                    if( iEndPos ) iEndPos--;
                }
                else {
                    iEndPos = 0;
                }
            }
            if( pEnd ){
                pEle = pEnd->lo_any.edit_element->Leaf(),
                iOffset = pEnd->lo_any.edit_offset+iEndPos;            
                SetInsertPoint( pEle, iOffset,  iEndPos == 0 );
            }
            else {
                XP_ASSERT(FALSE);
                // go to the end of document
                NavigateChunk( FALSE, LO_NA_DOCUMENT, TRUE );
            }
        }
    }
#endif
}


void CEditBuffer::GetInsertPoint( CEditLeafElement** ppEle, ElementOffset *pOffset, XP_Bool *pbStickyAfter){
    LO_Element *pStart, *pEnd;
    intn iStartPos, iEndPos;
    XP_Bool bFromStart;

    if( IsSelected() ){
        //
        // Grab the current selection.
        //            
        LO_GetSelectionEndPoints( m_pContext,
                    &pStart, &iStartPos,
                    &pEnd, &iEndPos,
                    &bFromStart, 0 );

        XP_ASSERT( pStart );
        XP_ASSERT( pEnd );

        if( pStart == pEnd && iEndPos == iStartPos ){
            bFromStart = TRUE;
        }

        if( bFromStart ){
            *pbStickyAfter = iStartPos != 0;
            while( pStart && pStart->lo_any.edit_element == 0 ){
                pStart = pStart->lo_any.next;
                if( pStart &&pStart->type == LO_TEXT ){
                    iStartPos = pStart->lo_text.text_len;
                    if( iStartPos ) iStartPos--;
                }
                else {
                    iStartPos = 0;
                }
            }
            if( pStart ){
                *ppEle = pStart->lo_any.edit_element->Leaf(),
                *pOffset = pStart->lo_any.edit_offset+iStartPos;            
            }
            else{
                // LTNOTE: NEED to implement end of document:
                //Insert point is at end of document
                //EndOfDocument( FALSE );
                XP_ASSERT(FALSE);
            }
       }
        else {
            *pbStickyAfter = iEndPos != 0;
            while( pEnd && pEnd->lo_any.edit_element == 0 ){
                pEnd = pEnd->lo_any.prev;
                if( pEnd && pEnd->type == LO_TEXT ){
                    iEndPos = pEnd->lo_text.text_len;
                    if( iEndPos ) iEndPos--;
                }
                else {
                    iEndPos = 0;
                }
            }
            if( pEnd ){
                *ppEle = pEnd->lo_any.edit_element->Leaf(),
                *pOffset = pEnd->lo_any.edit_offset+iEndPos;            
            }
            else {
                // LTNOTE: NEED to implement end of document:
                //Insert point is at end of document.
                //EndOfDocument( FALSE );
                XP_ASSERT(FALSE);
            }
        }

    }
    else {
        *ppEle = m_pCurrent->Leaf();
        *pOffset = m_iCurrentOffset;
        *pbStickyAfter = m_bCurrentStickyAfter;
    }
    if (ppEle && *ppEle && (*ppEle)->IsEndOfDocument()) {
        // End of document. Move back one edit position
        CEditInsertPoint ip(*ppEle, *pOffset, *pbStickyAfter);
        ip = ip.PreviousPosition();
        *ppEle = ip.m_pElement;
        *pOffset = ip.m_iPos;
        *pbStickyAfter = ip.m_bStickyAfter;
    }
}


//
// The 'PropertyPoint' is the beginning of a selection or current insert
//  point.
//
// Returns 
//  TRUE if a single element is selected like an image or hrule 
//  FALSE no selection or an extended selection. 
//
XP_Bool CEditBuffer::GetPropertyPoint( CEditLeafElement**ppElement, 
            ElementOffset* pOffset ){
    LO_Element *pStart, *pEnd;
    intn iStartPos, iEndPos;
    XP_Bool bFromStart;
    XP_Bool bSingleElementSelection;


    if( IsSelected() ){
        //
        // Grab the current selection.
        //            
        LO_GetSelectionEndPoints( m_pContext,
                    &pStart, &iStartPos,
                    &pEnd, &iEndPos,
                    &bFromStart, &bSingleElementSelection );

        XP_ASSERT( pStart->lo_any.edit_element );

        //------------------------- Begin Removable code ------------------------
        //
        // LTNOTE:  We are doing this in GetSelectionEndpoints, but I
        //  don't think we need to be doing it.  If the above assert never
        //  happens remove this code
        //
        while( pStart && pStart->lo_any.edit_element == 0 ){
            pStart = pStart->lo_any.next;
            if( pStart &&pStart->type == LO_TEXT ){
                iStartPos = pStart->lo_text.text_len;
                if( iStartPos ) iStartPos--;
            }
            else {
                iStartPos = 0;
            }
        }
        //------------------------- End Removable code ------------------------
        
        *ppElement = pStart->lo_any.edit_element->Leaf();
        *pOffset = pStart->lo_any.edit_offset+iStartPos;            
        XP_ASSERT( *ppElement );
    }
    else {
        *ppElement = m_pCurrent;
        *pOffset = m_iCurrentOffset;
        bSingleElementSelection = FALSE;
    }
    return bSingleElementSelection;
}

void CEditBuffer::SelectRegion(CEditLeafElement *pBegin, intn iBeginPos, 
            CEditLeafElement* pEnd, intn iEndPos, XP_Bool bFromStart, XP_Bool bForward  ){
    int iBeginLayoutOffset;
    LO_Element* pBeginLayoutElement;
    int iEndLayoutOffset;
    LO_Element* pEndLayoutElement;

    // If the start is an empty element, grab previous item.
    if( iBeginPos == 0 && pBegin->Leaf()->GetLen() == iBeginPos ){
        CEditLeafElement *pPrev = pBegin->PreviousLeafInContainer();
        if( pPrev ){
            pBegin = pPrev;
            iBeginPos = pPrev->GetLen();
        }
    }

    // if the end is a phantom insert point, grab the previous
    // item.

    if( iEndPos == 0 ){
        CEditLeafElement* pPrev = pEnd->PreviousLeafInContainer();
        if( pPrev ){
            pEnd = pPrev;
            iEndPos = pPrev->GetLen();
        }
    }

    XP_Bool startOK = pBegin->GetLOElementAndOffset( iBeginPos, FALSE,
            pBeginLayoutElement, iBeginLayoutOffset );
    XP_Bool endOK = pEnd->GetLOElementAndOffset( iEndPos, FALSE,
               pEndLayoutElement, iEndLayoutOffset );

    XP_ASSERT(startOK);
    XP_ASSERT(endOK);
    if (startOK && endOK ) {
        if ( ! IsSelected() ) {
            FE_DestroyCaret( m_pContext );
        }
        // Hack for end-of-document
        if ( iEndLayoutOffset < 0 ) iEndLayoutOffset = 0;

        LO_SelectRegion( m_pContext, pBeginLayoutElement, iBeginLayoutOffset,
            pEndLayoutElement, iEndLayoutOffset, bFromStart, bForward );
#ifdef DEBUG_EDITOR_LAYOUT
        // Verify that the selection is reversible.
        CEditSelection selection;
        GetSelection( selection );
        CEditInsertPoint a(pBegin, iBeginPos);
        CEditInsertPoint b(pEnd, iEndPos);
        CEditSelection original(a, b, bFromStart);
        XP_ASSERT(original == selection);
#endif
    }

//    EndSelection();
}

void CEditBuffer::SetSelection(CEditSelection& selection)
{
    if ( selection.IsInsertPoint() ) {
        SetInsertPoint(selection.m_start.m_pElement, selection.m_start.m_iPos, selection.m_start.m_bStickyAfter);
    }
    else {
        SelectRegion(selection.m_start.m_pElement, selection.m_start.m_iPos,
            selection.m_end.m_pElement, selection.m_end.m_iPos,
            selection.m_bFromStart);
    }
}

void CEditBuffer::SetInsertPoint(CEditInsertPoint& insertPoint) {
    CEditSelection selection(insertPoint, insertPoint, FALSE);
    SetSelection(selection);
}

void CEditBuffer::SetInsertPoint(CPersistentEditInsertPoint& insertPoint) {
    CEditInsertPoint p = PersistentToEphemeral(insertPoint);
    SetInsertPoint(p);
}

void CEditBuffer::GetSelection( CEditLeafElement*& pStartElement, ElementOffset& iStartOffset,
                CEditLeafElement*& pEndElement, ElementOffset& iEndOffset, XP_Bool& bFromStart ){

    if( m_bLayoutBackpointersDirty ) {
        XP_ASSERT(FALSE);
        return;
    }
    LO_Element *pStart, *pEnd;
    intn iStartPos, iEndPos;


    XP_ASSERT( IsSelected());

    //
    // Grab the current selection.
    //            
    LO_GetSelectionEndPoints( m_pContext,
                &pStart, &iStartPos,
                &pEnd, &iEndPos,
                &bFromStart, 0 );

     if ( ! pStart->lo_any.edit_element ) {
        XP_ASSERT(FALSE);
        // We should always be handed back an editable element.
        // LTNOTE: if we blow up here it is because we have searched to the end
        //  of document.  
        while( pStart->lo_any.edit_element == 0 ){
            pStart = pStart->lo_any.next;
            iStartPos = 0;
        }
    }
    
    pStartElement = pStart->lo_any.edit_element->Leaf();
    iStartOffset = pStart->lo_any.edit_offset+iStartPos;            

#if 0
    // LTNOTE: Don't fixup the insert point.  Layout metrics point to the 
    //  right container.
    if( iStartOffset == 0 ){
        CEditElement* pPrev = pStartElement->PreviousLeafInContainer();
        if( pPrev ){
            pStartElement = pPrev;
            iStartOffset = pPrev->Leaf()->GetLen();
        }
    }
#endif

    if ( ! pEnd->lo_any.edit_element ) {
        XP_ASSERT(FALSE); // We should always be handed back an editable element.
        while( pEnd->lo_any.edit_element == 0 ){
            pEnd = pEnd->lo_any.prev;
            if( pEnd->type == LO_TEXT ){
                iEndPos = pEnd->lo_text.text_len;
                if( iEndPos ) iEndPos--;
            }
            else {
                iEndPos = 0;
            }
        }
    }

    pEndElement = pEnd->lo_any.edit_element->Leaf();
#if 0
    if( pEndElement && pEndElement->IsA( P_TEXT ) && pEndElement->Text()->GetLen() ){
        iEndOffset = pEnd->lo_any.edit_offset+iEndPos+1;
    }
    else if ( pEndElement && pEndElement->IsA( P_TEXT ) ) {
        // we are dealing with an empty paragraph
        iEndOffset = 0;
    }
    else {
        // an image or hrule.
        iEndOffset = iEndPos + 1;
    }
 #else
    // jhp The simple way seems to work better.
    iEndOffset = pEnd->lo_any.edit_offset+iEndPos;
	
    if ( iEndOffset < 0 ) {
        iEndOffset = 0; // End-of-document flummery
    }


	// Normalize multibyte position, 
	// make it always start from beginning of char, end of the beginning of next char
	if (pStart->type == LO_TEXT && pStart->lo_text.text
	    && INTL_NthByteOfChar(m_pContext->win_csid, (char *) pStart->lo_text.text, iStartOffset+1) > 1) {
		XP_TRACE(("iStartOffset = %d which is wrong for multibyte", (int)iStartOffset));
		iStartOffset = INTL_NextCharIdx(m_pContext->win_csid, (unsigned char *) pStart->lo_text.text,iStartOffset); 
	}
	if (pEnd->type == LO_TEXT && pEnd->lo_text.text
	    && INTL_NthByteOfChar(m_pContext->win_csid, (char *) pEnd->lo_text.text, iEndOffset+1) > 1) {
		XP_TRACE(("iEndOffset = %d which is wrong for multibyte", (int)iEndOffset));
		iEndOffset = INTL_NextCharIdx(m_pContext->win_csid, (unsigned char *) pStart->lo_text.text,iEndOffset); 
	}
#endif
}

void CEditBuffer::MakeSelectionEndPoints( CEditSelection& selection,
    CEditLeafElement*& pBegin, CEditLeafElement*& pEnd ){
#ifdef DEBUG
    // If the selection is busted, we're in big trouble.
    CPersistentEditSelection persel = EphemeralToPersistent(selection);
    XP_ASSERT ( persel.m_start.m_index <=  persel.m_end.m_index);
#endif
    pEnd = selection.m_end.m_pElement->Divide( selection.m_end.m_iPos )->Leaf();
    pBegin = selection.m_start.m_pElement->Divide( selection.m_start.m_iPos )->Leaf();
    m_bLayoutBackpointersDirty = TRUE;
}

void CEditBuffer::MakeSelectionEndPoints( CEditLeafElement*& pBegin, 
        CEditLeafElement*& pEnd ){
    CEditSelection selection;
    GetSelection(selection);
    MakeSelectionEndPoints(selection, pBegin, pEnd);
}

int CEditBuffer::Compare( CEditElement *p1, int i1Offset,
                           CEditElement *p2, int i2Offset ) {

    CEditPositionComparable *pPos1, *pPos2;
    int iRetVal;

    pPos1 = new CEditPositionComparable( p1, i1Offset);
    pPos2 = new CEditPositionComparable( p2, i2Offset);

    // find which one is really the beginning.
    iRetVal = pPos1->Compare( pPos2 );
    delete pPos1;
    delete pPos2;
    return iRetVal;
}

// Prevent recursive calls for autosaving
XP_Bool CEditBuffer::m_bAutoSaving = FALSE;

void CEditBuffer::AutoSaveCallback() {
    if ( m_bAutoSaving || IsBlocked() ) {
        XP_TRACE(("Auto Save ignored -- busy."));
        return;
    }
    if ( ! m_bDirty ) {
        XP_TRACE(("Skipping AutoSave because we're not dirty."));
        return;
    }
    // Skip trying to save if we are already in the process of saving a file
    if( m_pContext->edit_saving_url ) {
        XP_TRACE(("Skipping AutoSave because we're not already saving a file."));
        return;
    }

    XP_TRACE(("Auto Save...."));
    
    m_bAutoSaving = TRUE;
    if( EDT_IS_NEW_DOCUMENT(m_pContext) ){
        // New document -- prompt front end to get filename and "save file as"
         if( !FE_CheckAndAutoSaveDocument(m_pContext) ){
            // User canceled the Autosave prompt, turn off autosave
            SetAutoSavePeriod(0);
         }
    } else {
        History_entry * hist_entry = SHIST_GetCurrent(&(m_pContext->hist));

	    if(hist_entry && hist_entry->address)
	    {
            char *szLocalFile = NULL;
            if ( XP_ConvertUrlToLocalFile( hist_entry->address, &szLocalFile ) )
            {
                char buf[300];

	            PR_snprintf(buf, sizeof(buf)-1, XP_GetString(XP_EDITOR_AUTO_SAVE), szLocalFile);
	            FE_Progress(m_pContext, buf);

                ED_FileError nResult = EDT_SaveFile(m_pContext,hist_entry->address,  // Source
                                           hist_entry->address,  // Destination
                                           FALSE,  // Not doing "SaveAs"
                                           FALSE,
                                           FALSE );
            }
            if (szLocalFile){
                XP_FREE(szLocalFile);
            }
        }
    }
    m_bAutoSaving = FALSE;
}

void CEditBuffer::FileFetchComplete(ED_FileError status) {
    m_status = status;
    FE_Progress(m_pContext, NULL);
    RefreshLayout();
}

void CEditBuffer::SetAutoSavePeriod(int32 minutes) {
    m_autoSaveTimer.SetPeriod(minutes);
}

int32 CEditBuffer::GetAutoSavePeriod() {
    return m_autoSaveTimer.GetPeriod();
}

void CEditBuffer::ClearMove(XP_Bool bFlushRelayout){
    if ( bFlushRelayout ) {
        m_relayoutTimer.Flush();
    }
    m_iDesiredX = -1;
}

void CEditBuffer::FormatCharacterSelection( ED_TextFormat tf ){
    VALIDATE_TREE(this);
    CEditLeafElement *pCurrent, *pEnd, *pBegin, *pNext;
    XP_Bool bSet;
    XP_Bool bHaveSet = FALSE;

    SetDirtyFlag( TRUE );

    //
    // Guarantee that the text blocks of the beginning and end of selection
    //  are atomic.
    //
    MakeSelectionEndPoints( pBegin, pEnd );

    pCurrent = pBegin;    

    while( pCurrent != pEnd && pCurrent ){
        pNext = pCurrent->NextLeafAll();
        XP_ASSERT(pNext);

        if( pCurrent->IsA( P_TEXT ) ){
            // figure out weather we are setting or clearing...
            if( !bHaveSet ) {
                bSet = !(pCurrent->Text()->m_tf & tf);
                bHaveSet = TRUE;
            }

            if( tf == TF_NONE ){
                pCurrent->Text()->ClearFormatting();
            }
            else if (tf == TF_SERVER || tf == TF_SCRIPT ){
                pCurrent->Text()->ClearFormatting();
                pCurrent->Text()->m_tf = tf;
            }
            else {
                if( bSet && pCurrent->IsA( P_TEXT ) ){
                    // If we are in either Java Script type, IGNORE change in format
                    if( ED_IS_NOT_SCRIPT(pCurrent) ){
                        // Make superscript and subscript mutually-exclusive
                        if( tf == TF_SUPER && (pCurrent->Text()->m_tf & TF_SUB) ){
                            pCurrent->Text()->m_tf &= ~TF_SUB;
                        } else if( tf == TF_SUB && (pCurrent->Text()->m_tf & TF_SUPER) ){
                            pCurrent->Text()->m_tf &= ~TF_SUPER;
                        }
                        pCurrent->Text()->m_tf |= tf;
                    }
                }
                else {
                    pCurrent->Text()->m_tf &= ~tf;
                }
            }
        }
        else if( pCurrent->IsImage() && tf == TF_NONE ){
            // Clear the link for the image within the selection
            pCurrent->Image()->SetHREF( ED_LINK_ID_NONE );
        }
        pCurrent = pNext;
    }
	CEditSelection tmp(pBegin, 0, pEnd, 0);
    RepairAndSet(tmp);
}

void CEditBuffer::SetFontSizeSelection( int iSize){
    VALIDATE_TREE(this);
    CEditLeafElement *pCurrent, *pEnd, *pBegin, *pNext;

    SetDirtyFlag( TRUE );

    //
    // Guarantee that the text blocks of the beginning and end of selection
    //  are atomic.
    //
    MakeSelectionEndPoints( pBegin, pEnd );
    pCurrent = pBegin;    

    while( pCurrent != pEnd ){
        pNext = pCurrent->NextLeafAll();
        if( pCurrent->IsA( P_TEXT ) && 
            ED_IS_NOT_SCRIPT(pCurrent) ){
            pCurrent->Text()->SetFontSize(iSize);
        }
        pCurrent = pNext;
    }

    // force layout stop displaying the current selection.
    LO_StartSelectionFromElement( m_pContext, 0, 0); 
    // probably needs to be the common ancestor.
    Relayout( pBegin, 0, pEnd, RELAYOUT_NOCARET );
    // Need to force selection.
    SelectRegion(pBegin, 0, pEnd, 0);
    Reduce(pBegin->GetCommonAncestor(pEnd));
}

void CEditBuffer::SetFontColorSelection( ED_Color iColor ){
    VALIDATE_TREE(this);
    CEditLeafElement *pCurrent, *pEnd, *pBegin, *pNext;

    SetDirtyFlag( TRUE );

    //
    // Guarantee that the text blocks of the beginning and end of selection
    //  are atomic.
    //
    MakeSelectionEndPoints( pBegin, pEnd );
    pCurrent = pBegin;    

    while( pCurrent != pEnd ){
        pNext = pCurrent->NextLeafAll();
        if( pCurrent->IsA( P_TEXT ) ){
            pCurrent->Text()->SetColor( iColor );
        }
        pCurrent = pNext;
    }

    // force layout stop displaying the current selection.
    LO_StartSelectionFromElement( m_pContext, 0, 0); 
    // probably needs to be the common ancestor.
    Relayout( pBegin, 0, pEnd, RELAYOUT_NOCARET );
    // Need to force selection.
    SelectRegion(pBegin, 0, pEnd, 0);
    Reduce(pBegin->GetCommonAncestor(pEnd));
}

void CEditBuffer::SetHREFSelection( ED_LinkId id ){
    VALIDATE_TREE(this);
    CEditLeafElement *pCurrent, *pEnd, *pBegin, *pNext;

    SetDirtyFlag( TRUE );

    //
    // Guarantee that the text blocks of the beginning and end of selection
    //  are atomic.
    //
    MakeSelectionEndPoints( pBegin, pEnd );
    pCurrent = pBegin;    

    while( pCurrent != pEnd ){
        pNext = pCurrent->NextLeafAll();
        pCurrent->Leaf()->SetHREF( id );
        pCurrent = pNext;
    }

	CEditSelection tmp(pBegin, 0, pEnd, 0);
    RepairAndSet(tmp);
}

EDT_ClipboardResult CEditBuffer::DeleteSelection(CEditSelection& selection, XP_Bool bCopyAppendAttributes){
    SetSelection(selection);
    return DeleteSelection(bCopyAppendAttributes);
}

EDT_ClipboardResult CEditBuffer::DeleteSelection(XP_Bool bCopyAppendAttributes){
    VALIDATE_TREE(this);
    EDT_ClipboardResult result = CanCut(TRUE);
    if ( result == EDT_COP_OK ){
        if ( IsSelected() ) {
            CEditLeafElement *pBegin;
            CEditLeafElement *pEnd;

            CEditSelection selection;
            GetSelection(selection);
            if ( selection.ContainsLastDocumentContainerEnd() ) {
                selection.ExcludeLastDocumentContainerEnd();
                SetSelection(selection);
            }
            if ( selection.IsInsertPoint() ) {
                return result;
            }
            MakeSelectionEndPoints( selection, pBegin, pEnd );
            SetDirtyFlag( TRUE );
            DeleteBetweenPoints( pBegin, pEnd, bCopyAppendAttributes );
        }
    }
    return result;
}    

CPersistentEditSelection CEditBuffer::GetEffectiveDeleteSelection(){
    // If the selection includes the end of the document, then the effective selection is
    // less than that.
    CEditSelection selection;
    GetSelection(selection);
    selection.ExcludeLastDocumentContainerEnd();
    selection.ExpandToIncludeFragileSpaces();
    return EphemeralToPersistent(selection);
}

//
// Get comparable positions to the 
//
void CEditBuffer::DeleteBetweenPoints( CEditLeafElement* pBegin, CEditLeafElement* pEnd,
     XP_Bool bCopyAppendAttributes){
    CEditLeafElement *pCurrent;
    CEditLeafElement *pNext;

    SetDirtyFlag( TRUE );

    pCurrent = pBegin;    

    XP_ASSERT( pEnd != 0 );

    CEditLeafElement* pPrev = pCurrent->PreviousLeafInContainer();
    CEditElement* pCommonAncestor = pBegin->GetCommonAncestor(pEnd);
    CEditElement* pContainer = pCurrent->FindContainer();
    CEditElement* pCurrentContainer;
    while( pCurrent != pEnd ){
        pNext = pCurrent->NextLeafAll();

        // if we've entered a new container, merge it.
        pCurrentContainer = pCurrent->FindContainer();

        // DeleteElement removes empty paragraphs so we don't have to do the merges.
        //
        //if( pContainer != pCurrentContainer ){
        //    pContainer->Merge( pCurrentContainer );
        //}   

#ifdef DEBUG
         m_pRoot->ValidateTree();
#endif

        if( pCurrent->DeleteElement( pContainer )) {
            pContainer = pNext->FindContainer();
        }
#ifdef DEBUG
        m_pRoot->ValidateTree();
#endif
        pCurrent = pNext;
    }

    //
    // if the selection spans partial paragraphs, merge the remaining paragraphs
    //
    if( pPrev && pPrev->FindContainer() != pEnd->FindContainer() ){
        pPrev->FindContainer()->Merge( pEnd->FindContainer(), bCopyAppendAttributes );
    }

    m_pCurrent = pEnd->PreviousLeafInContainer();
    if( m_pCurrent != 0 ){
        m_iCurrentOffset = m_pCurrent->Leaf()->GetLen();
    }
    else {
        m_pCurrent = pEnd;
        m_iCurrentOffset = 0;
    }

    FixupSpace();

    // probably needs to be the common ancestor.
    ClearSelection(FALSE);
    CEditInsertPoint tmp(pEnd,0);
    CPersistentEditInsertPoint end2 = EphemeralToPersistent(tmp);
    Reduce( pCommonAncestor );
    SetCaret();
    if ( pPrev && pPrev->FindContainer() != pContainer ) {
        Reduce( pPrev->FindContainer() );
    }
    CEditInsertPoint end3 = PersistentToEphemeral(end2);
    Relayout( pContainer, 0,  end3.m_pElement);
}

//
// LTNOTE: this routine needs to be broken into PasteText and PasteFormattedText
//  It didn't happen because we were too close to ship.
// 
EDT_ClipboardResult CEditBuffer::PasteText( char *pText ){
    VALIDATE_TREE(this);
    SUPPRESS_PHANTOMINSERTPOINTCHECK(this);
    EDT_ClipboardResult result = CanPaste(TRUE);
    if ( result != EDT_COP_OK ) return result;

    SetDirtyFlag( TRUE );

    m_bNoRelayout = TRUE;
    int iCharsOnLine = 0;

    if( IsSelected() ){
        DeleteSelection();
    }
    FixupInsertPoint();

    // If the pasted text starts with returns, the start element can end up
    // being moved down the document as paragraphs are inserted above it.
    // So we hold onto the start as a persistent insert point.

    CPersistentEditInsertPoint persistentStart;
    GetInsertPoint(persistentStart);

    CEditElement *pEnd = m_pCurrent;

    // If we're in preformatted text, every return is a break.
    // If we're in normal text, single returns are spaces,
    // double returns are paragraph marks.

    //XP_Bool bInFormattedText = m_pCurrent->FindContainer()->GetType() == P_PREFORMAT;
    XP_Bool bInFormattedText = m_pCurrent->InFormattedText();

    XP_Bool bLastCharWasReturn = FALSE;
    while( *pText ){
        XP_Bool bReturn = FALSE;
        if( *pText == 0x0d ){
            if( *(pText+1) == 0xa ){
                pText++;
            }
            bReturn = TRUE;
        }
        else if ( *pText == 0xa ){
            bReturn = TRUE;
        }

        if( bReturn ){
            if ( bInFormattedText ) {
                InsertBreak(ED_BREAK_NORMAL, FALSE);
                iCharsOnLine = 0;
            }
            else {
                if ( bLastCharWasReturn ){
                    ReturnKey(FALSE);
                    iCharsOnLine = 0;
                    bLastCharWasReturn = FALSE;
                }
                else {
                    // Remember this return for later. It will
                    // become either a space or a return.
                    bLastCharWasReturn = TRUE;
                }
            }
        }
        else {
            if ( bLastCharWasReturn ) {
                InsertChar(' ', FALSE);
                bLastCharWasReturn = FALSE;
            }

            if ( *pText == '\t' ){
                if ( bInFormattedText ) {
                    do {
                        InsertChar(' ', FALSE);
                        iCharsOnLine++;
                    } while( iCharsOnLine % DEF_TAB_WIDTH != 0 );
                }
                else {
                    InsertChar(' ', FALSE);
                    iCharsOnLine++;
                }
            }
            else {
                InsertChar( *pText, FALSE );
                iCharsOnLine++;
            }
            bLastCharWasReturn = FALSE;
        }
        pText++;
    }

    if ( bLastCharWasReturn ) {
        InsertChar(' ', FALSE);
    }

    m_bNoRelayout = FALSE;

    Reduce(m_pRoot);

    CEditInsertPoint start = PersistentToEphemeral(persistentStart);

    Relayout( start.m_pElement, start.m_iPos, pEnd );
    return result;
}

EDT_ClipboardResult CEditBuffer::PasteExternalHTML( char* /* pText */ ){
    VALIDATE_TREE(this);
    EDT_ClipboardResult result = CanPaste(TRUE);
    if ( result != EDT_COP_OK ) return result;

    // Create a new EDT buffer

    return result;
}


EDT_ClipboardResult CEditBuffer::PasteHTML( char *pBuffer, XP_Bool bUndoRedo ){

    CStreamInMemory stream(pBuffer);
    return PasteHTML(stream, bUndoRedo);
}

EDT_ClipboardResult CEditBuffer::PasteHTML( IStreamIn& stream, XP_Bool /* bUndoRedo */ ){
    VALIDATE_TREE(this);
    EDT_ClipboardResult result = CanPaste(TRUE);
    if ( result != EDT_COP_OK ) return result;
    m_bNoRelayout = TRUE;

    SetDirtyFlag( TRUE );

    if( IsSelected() ){
        DeleteSelection();
    }
    FixupInsertPoint();

#ifdef DEBUG
    m_pRoot->ValidateTree();
#endif

    CEditElement* pStart = m_pCurrent;
    int iStartOffset = m_iCurrentOffset;
    CEditLeafElement* pRight = m_pCurrent->Divide(m_iCurrentOffset)->Leaf();
    XP_ASSERT(pRight->FindContainer());
    XP_Bool bAtStartOfParagraph = pRight->PreviousLeafInContainer() == NULL;
    CEditLeafElement* pLeft = pRight->PreviousLeaf(); // Will be NULL at start of document

    // The first thing in the buffer is a flag that tells us if we need to merge the end.
    int32 bMergeEnd = stream.ReadInt();
    // The buffer has zero or more elements. (The case where there's more than one is
    // the one where there are multiple paragraphs.)
    CEditElement* pFirstInserted = NULL;
    CEditElement* pLastInserted = NULL;
    {
        CEditElement* pElement;
        while ( NULL != (pElement = CEditElement::StreamCtor(&stream, this)) ){
            if ( pElement->IsLeaf() ) {
                XP_ASSERT(FALSE);
                delete pElement;
                continue;
            }
#ifdef DEBUG
            pElement->ValidateTree();
#endif
            if ( ! pFirstInserted ) {
                pFirstInserted = pElement;
                if ( ! bAtStartOfParagraph ){
    XP_ASSERT(pRight->FindContainer());
                    InternalReturnKey(FALSE);
    XP_ASSERT(pRight->FindContainer());
               }
            }
            pLastInserted = pElement;
            pElement->InsertBefore(pRight->FindContainer());

#ifdef DEBUG
            m_pRoot->ValidateTree();
#endif
        }
    }

#ifdef DEBUG
    m_pRoot->ValidateTree();
#endif

    if ( pFirstInserted ) {
        CEditElement* pLast = pLastInserted->GetLastMostChild()->NextLeafAll();
        // Get the children early, because if we paste a single container,
        // it will be deleted when we merge the left edge
        CEditLeafElement* pFirstMostNewChild = pFirstInserted->GetFirstMostChild()->Leaf();
        CEditLeafElement* pLastMostNewChild = pLastInserted->GetLastMostChild()->Leaf();
        m_pCurrent = pLastMostNewChild;
        m_iCurrentOffset = pLastMostNewChild->GetLen();
        ClearPhantomInsertPoint();
        if ( ! bAtStartOfParagraph ){
            if ( pLeft ) {
                CEditContainerElement* pLeftContainer = pLeft->FindContainer();
                CEditContainerElement* pFirstNewContainer =
                    pFirstMostNewChild->FindContainer();
                if ( pLeftContainer != pFirstNewContainer ){
                    XP_Bool bCopyNewAttributes = FALSE;
                    pLeftContainer->Merge(pFirstNewContainer);
                    // Now deleted in Merge delete pFirstNewContainer;
                    FixupInsertPoint();
                }
            }
        }
        else {
            // The insert went into the container before us. Adjust the start.
            pStart = pFirstInserted->GetFirstMostChild();
            iStartOffset = 0;
        }

        if ( bMergeEnd ) {
            CEditContainerElement* pRightContainer = pRight->FindContainer();
            CEditContainerElement* pLastNewContainer = pLastMostNewChild->FindContainer();
            if ( pRightContainer != pLastNewContainer ){
                pLastNewContainer->Merge(pRightContainer);
                // Now deleted in Merge delete pRightContainer;
                FixupInsertPoint();
            }
        }
        else {
            // Move to beginning of next container
            m_pCurrent = pLastMostNewChild->NextContainer()->GetFirstMostChild()->Leaf();
            m_iCurrentOffset = 0;
        }

        // Have to FinishedLoad after merging because FinishedLoad is eager to
        // remove spaces from text element at the end of containers. If we let
        // FinishedLoad run earlier, we couldn't paste text that ended in white space.

        m_pRoot->FinishedLoad(this);

#ifdef DEBUG
        m_pRoot->ValidateTree();
#endif

        m_bNoRelayout = FALSE;
        FixupSpace( );
        // We need to reduce before we relayout because otherwise
        // null insert points mess up the setting of end-of-paragraph
        // marks.
		CEditInsertPoint tmp(pLast,0);
		CPersistentEditInsertPoint end2 = EphemeralToPersistent(tmp);
        Reduce( m_pRoot );
        CEditInsertPoint end3 = PersistentToEphemeral(end2);
        Relayout( pStart, iStartOffset, end3.m_pElement );
    }
    else {
        m_bNoRelayout = FALSE;
    }

   return result;
}

EDT_ClipboardResult CEditBuffer::PasteHREF( char **ppHref, char **ppTitle, int iCount){
    VALIDATE_TREE(this);
    SUPPRESS_PHANTOMINSERTPOINTCHECK(this);
    EDT_ClipboardResult result = CanPaste(TRUE);
    if ( result != EDT_COP_OK ) return result;
    m_bNoRelayout = TRUE;
    XP_Bool bFirst = TRUE;

    SetDirtyFlag( TRUE );

    if( IsSelected() ){
        DeleteSelection();
    }
    FixupInsertPoint();

    FormatCharacter( TF_NONE );
    InsertChar(' ', FALSE);

    CEditElement *pStart = m_pCurrent;
    int iStartOffset = m_iCurrentOffset;

    m_pCurrent->Divide( m_iCurrentOffset );

    int i = 0;
    while( i < iCount ){
        char *pTitle = ppTitle[i];

        if( pTitle == 0 ){
            pTitle = ppHref[i];
        }
        else {
            // LTNOTE:
            // probably shouldn't be doing this to a buffer we were passed
            //  but what the hell.
            NormalizeText( pTitle );

            // kill any trailing spaces..
            int iLen = XP_STRLEN( pTitle )-1;
            while( iLen >= 0 && pTitle[iLen] == ' ' ){
                pTitle[iLen] = 0;
                iLen--;
            }
            if( *pTitle == 0 ){
                pTitle = ppHref[i];
            }
        }

        CEditTextElement *pElement = new CEditTextElement( 0, pTitle );
        pElement->SetHREF( linkManager.Add( ppHref[i], 0 ));

        if( bFirst && m_iCurrentOffset == 0 ){
            pElement->InsertBefore( m_pCurrent );
            pStart = pElement;
        }
        else {
            pElement->InsertAfter( m_pCurrent );
        }
        if( bFirst ){
            FixupSpace();
            bFirst = FALSE;
        }
        m_pCurrent = pElement;
        m_iCurrentOffset = pElement->Text()->GetLen();
        i++;
    }

    m_bNoRelayout = FALSE;
    FixupSpace( );
    // Fixes bug 21920 - Crash when dropping link into blank document.
    // We might have an empty text container just after m_pCurrent.
    Reduce(m_pRoot);
    FormatCharacter( TF_NONE );
    InsertChar(' ', FALSE);
    Relayout( pStart, iStartOffset, m_pCurrent );
    Reduce( m_pRoot );
    return result;
}


//
// Make sure that there are not spaces next to each other after a delete, 
//  or paste
//
void CEditBuffer::FixupSpace( XP_Bool bTyping){
    if( m_pCurrent->InFormattedText() ){
        return;
    }

    if( m_pCurrent->IsBreak() && m_iCurrentOffset == 1 ){
        // Can't have a space after a break.
        CEditLeafElement *pNext = m_pCurrent->TextInContainerAfter();
        if( pNext
                && pNext->IsA(P_TEXT)
                && pNext->Text()->GetLen() != 0
                && pNext->Text()->GetText()[0] == ' '){
            if ( bTyping ) {
                GetTypingCommand()->AddCharDeleteSpace();
            }
            pNext->Text()->DeleteChar(m_pContext, 0);        
            return;
        }
    }

    if( !m_pCurrent->IsA(P_TEXT) ){
        return;
    }

    CEditTextElement *pText = m_pCurrent->Text();

    if( pText->GetLen() == 0 ){
        return;        
    }
    if( m_iCurrentOffset > 0 && pText->GetText()[ m_iCurrentOffset-1 ] == ' ' ){
        if( m_iCurrentOffset == pText->GetLen() ){
            CEditLeafElement *pNext = pText->TextInContainerAfter();
            if( pNext
                    && pNext->IsA(P_TEXT)
                    && pNext->Text()->GetLen() != 0
                    && pNext->Text()->GetText()[0] == ' '){
                if ( bTyping ) {
                    GetTypingCommand()->AddCharDeleteSpace();
                }
                pNext->Text()->DeleteChar(m_pContext, 0);        
                //m_iCurrentOffset--;  // WHy???
                return;
            }
        }
        else if( m_iCurrentOffset < pText->GetLen() && pText->GetText()[m_iCurrentOffset] == ' ' ){
            pText->DeleteChar(m_pContext, m_iCurrentOffset);
            return;
        }
    }
    // check for beginning of paragraph with a space.
    else if( m_iCurrentOffset == 0 && pText->GetText()[0] == ' ' ){
        pText->DeleteChar(m_pContext, 0);
        CEditLeafElement *pNext = m_pCurrent->TextInContainerAfter();
        if( m_pCurrent->Text()->GetLen() == 0 && pNext ){
            m_pCurrent = pNext;
        }
    }
    // Can't reduce here because it might smash objects pointers we know about
}

EDT_ClipboardResult CEditBuffer::CutSelection( char **ppText, int32* pTextLen,
                    char **ppHtml, int32* pHtmlLen){
    VALIDATE_TREE(this);
    EDT_ClipboardResult result = CanCut(TRUE);
    if ( result != EDT_COP_OK ) return result;

    result = CopySelection( ppText, pTextLen, ppHtml, pHtmlLen );
    if ( result != EDT_COP_OK ) return result;

    CPersistentEditSelection selection = GetEffectiveDeleteSelection();
    CChangeAttributesCommand* pCommand =
        new CChangeAttributesCommand(this, selection, kCutCommandID);
    
    result = DeleteSelection();
#if 0
    // Old voo doo
    ClearPhantomInsertPoint();
    FixupInsertPoint();
#endif
    AdoptAndDo(pCommand);

    // Check to see if we trashed the document
    XP_ASSERT ( m_pCurrent && m_pCurrent->GetElementType() != eEndElement );
    return result;
}

XP_Bool CEditBuffer::CutSelectionContents( CEditSelection& selection,
                    char **ppHtml, int32* pHtmlLen ){
    XP_Bool result = CopySelectionContents( selection, ppHtml, pHtmlLen );
    if ( result ) {
        CEditLeafElement *pBegin;
        CEditLeafElement *pEnd;
        MakeSelectionEndPoints( selection, pBegin, pEnd );
    
        DeleteBetweenPoints( pBegin, pEnd );
    }
    return TRUE;
}


EDT_ClipboardResult CEditBuffer::CopySelection( char **ppText, int32* pTextLen,
                    char **ppHtml, int32* pHtmlLen){
    EDT_ClipboardResult result = CanCopy(TRUE);
    if ( result != EDT_COP_OK ) return result;

    if ( ppText ) *ppText = (char*) LO_GetSelectionText( m_pContext );
    if ( pTextLen ) *pTextLen = XP_STRLEN( *ppText );

    CEditSelection selection;
    GetSelection(selection);
    CopySelectionContents( selection, ppHtml, pHtmlLen );

    return result;
}

XP_Bool CEditBuffer::CopyBetweenPoints( CEditElement *pBegin,
                    CEditElement *pEnd, char **ppText, int32* pTextLen,
                    char **ppHtml, int32* pHtmlLen ){
    if ( ppText ) *ppText = (char*) LO_GetSelectionText( m_pContext );
    if ( pTextLen ) *pTextLen = XP_STRLEN( *ppText );
    CEditInsertPoint a(pBegin, 0);
    CEditInsertPoint b(pEnd, 0);
    CEditSelection s(a,b);
    return CopySelectionContents(s, ppHtml, pHtmlLen);
}

XP_Bool CEditBuffer::CopySelectionContents( CEditSelection& selection,
                    char **ppHtml, int32* pHtmlLen ){
    CStreamOutMemory stream;

    XP_Bool result = CopySelectionContents(selection, stream);

    *ppHtml = stream.GetText();
    *pHtmlLen = stream.GetLen();

    return result;
}

XP_Bool CEditBuffer::CopySelectionContents( CEditSelection& selection,
                    IStreamOut& stream ){
    int32 bMergeEnd = ! selection.EndsAtStartOfContainer();
    stream.WriteInt(bMergeEnd);
    m_pRoot->PartialStreamOut(&stream, selection);
    stream.WriteInt( (int32)eElementNone );
    return TRUE;

#if 0
    CEditElement *pCurrent;
    CEditElement *pNext;

    pCurrent = selection.m_start.m_pElement;

    if( selection.m_end.m_pElement == 0 ) {
        XP_ASSERT(FALSE);
        return FALSE;
    }

    CEditElement* pContainer = pCurrent->FindContainer();
    CEditElement* pCurrentContainer;

    // The containers are copied out in postfix style.

    while( pCurrent != selection.m_end.m_pElement ){
        pNext = pCurrent->NextLeafAll();

        if ( pCurrent == selection.m_start.m_pElement && selection.m_start.m_iPos > 0 ) {
            pCurrent->PartialStreamOut( &stream, selection );
        }
        else {
            pCurrent->StreamOut( &stream );
        }

        pCurrent = pNext;

       // if we've entered a new container, copy the old container.
        pCurrentContainer = pCurrent->FindContainer();
        if( pContainer != pCurrentContainer ){
            pContainer->StreamOut( &stream );
            pContainer = pCurrentContainer;
        }   
    }

    // Write last element
    if ( pCurrent == selection.m_start.m_pElement ) {
        // Only one element
        pCurrent->PartialStreamOut( &stream, selection );
    }
    else if ( selection.m_end.m_iPos > 0 ) {
        // if we've entered a new container, merge it.
        pCurrentContainer = pCurrent->FindContainer();
        if( pContainer != pCurrentContainer ){
            pCurrentContainer->StreamOut( &stream );
            pContainer = pCurrentContainer;
        }   

        if ( selection.m_end.m_iPos < pCurrent->Leaf()->GetLen() ) {
            pCurrent->PartialStreamOut( &stream, selection );
        }
        else {
            pCurrent->StreamOut( &stream );
        }
    }

    stream.WriteInt( eElementNone );

    return TRUE;
#endif
}

//
// Used during parse phase.
//
ED_Alignment CEditBuffer::GetCurrentAlignment(){
    if( GetParseState()->m_formatAlignStack.IsEmpty()){
        return m_pCreationCursor->GetDefaultAlignment();
    }
    else {
        return GetParseState()->m_formatAlignStack.Top();
    }
}

void CEditBuffer::GetSelection( CEditSelection& selection ){
    if ( IsSelected() ) {
        GetSelection( selection.m_start.m_pElement, selection.m_start.m_iPos,
            selection.m_end.m_pElement, selection.m_end.m_iPos,
            selection.m_bFromStart);
    }
    else {
        GetInsertPoint( &selection.m_start.m_pElement, &selection.m_start.m_iPos, &selection.m_start.m_bStickyAfter);
        selection.m_end = selection.m_start;
        selection.m_bFromStart = FALSE;
    }
}

void CEditBuffer::GetSelection( CPersistentEditSelection& persistentSelection ){
    CEditSelection selection;
    GetSelection(selection);
    persistentSelection = EphemeralToPersistent(selection);
}

void CEditBuffer::GetInsertPoint(CEditInsertPoint& insertPoint){
    if ( ! IsSelected() )
    {
        GetInsertPoint( & insertPoint.m_pElement, & insertPoint.m_iPos, & insertPoint.m_bStickyAfter );
    }
    else
    {
        CEditSelection selection;
        GetSelection(selection);
        selection.ExcludeLastDocumentContainerEnd();
        insertPoint = *selection.GetActiveEdge();
    }
}

void CEditBuffer::GetInsertPoint(CPersistentEditInsertPoint& insertPoint){
    CEditInsertPoint pt;
    GetInsertPoint(pt);
    insertPoint = EphemeralToPersistent(pt);
}

void CEditBuffer::SetSelection(CPersistentEditSelection& persistentSelection){
    CEditSelection selection = PersistentToEphemeral(persistentSelection);
    SetSelection(selection);
}

void CEditBuffer::CopyEditText(CEditText& text){
    text.Clear();
    if ( IsSelected() ) {
        XP_Bool copyOK = CopySelection(NULL, NULL, text.GetPChars(), text.GetPLength());
        XP_ASSERT(copyOK);
    }
}

void CEditBuffer::CopyEditText(CPersistentEditSelection& persistentSelection, CEditText& text){
    text.Clear();
    if ( ! persistentSelection.IsInsertPoint() ) {
        CEditSelection selection = PersistentToEphemeral(persistentSelection);
        CopySelectionContents(selection, text.GetPChars(), text.GetPLength());
    }
}

void CEditBuffer::CutEditText(CEditText& text){
    text.Clear();
    CEditSelection selection;
    GetSelection(selection);
    if ( ! selection.IsInsertPoint() ) {
        CutSelectionContents(selection, text.GetPChars(), text.GetPLength());
    }
}

void CEditBuffer::PasteEditText(CEditText& text){
    if ( text.Length() > 0 )
        PasteHTML( text.GetChars(), TRUE);
}

// Persistent to regular selection conversion routines
CEditInsertPoint CEditBuffer::PersistentToEphemeral(CPersistentEditInsertPoint& persistentInsertPoint){
    CEditInsertPoint result = m_pRoot->IndexToInsertPoint(
        persistentInsertPoint.m_index, persistentInsertPoint.m_bStickyAfter);
#ifdef DEBUG_EDITOR_LAYOUT
    // Check for reversability
    CPersistentEditInsertPoint p2 = result.m_pElement->GetPersistentInsertPoint(result.m_iPos);
    XP_ASSERT(persistentInsertPoint == p2);
    // Check for legality.
    if ( result.m_pElement->IsEndOfDocument() &&
        result.m_iPos != 0){
        XP_ASSERT(FALSE);
        result.m_iPos = 0;
    }

#endif
    return result;
}

CPersistentEditInsertPoint CEditBuffer::EphemeralToPersistent(CEditInsertPoint& insertPoint){
    CPersistentEditInsertPoint result = insertPoint.m_pElement->GetPersistentInsertPoint(insertPoint.m_iPos);
#ifdef DEBUG_EDITOR_LAYOUT
    // Check for reversability
    CEditInsertPoint p2 = m_pRoot->IndexToInsertPoint(result.m_index);
    if ( ! ( p2 == insertPoint ||
        insertPoint.IsDenormalizedVersionOf(p2) ) ) {
        // One or the other is empty.
        if ( insertPoint.m_pElement->Leaf()->GetLen() == 0 ||
            p2.m_pElement->Leaf()->GetLen() == 0 ) {
            // OK, do they at least have the same persistent point?
            CPersistentEditInsertPoint pp2 = p2.m_pElement->GetPersistentInsertPoint(p2.m_iPos);
            XP_ASSERT(result == pp2);
        }
        else
        {
            // Just wrong.
            XP_ASSERT(FALSE);
        }
    }
#endif
    return result;
}

CEditSelection CEditBuffer::PersistentToEphemeral(CPersistentEditSelection& persistentSelection){
    CEditSelection selection(PersistentToEphemeral(persistentSelection.m_start),
        PersistentToEphemeral(persistentSelection.m_end),
        persistentSelection.m_bFromStart);
    XP_ASSERT(!selection.m_start.IsEndOfDocument());
    return selection;
}

CPersistentEditSelection CEditBuffer::EphemeralToPersistent(CEditSelection& selection){
    CPersistentEditSelection persistentSelection;
    XP_ASSERT(!selection.m_start.IsEndOfDocument());
    persistentSelection.m_start = EphemeralToPersistent(selection.m_start);
    persistentSelection.m_end = EphemeralToPersistent(selection.m_end);
    persistentSelection.m_bFromStart = selection.m_bFromStart;
    return persistentSelection;
}

void CEditBuffer::AdoptAndDo(CEditCommand* command) {
    DoneTyping();
    m_commandLog.AdoptAndDo(command);
    SetDirtyFlag( TRUE );
}

void CEditBuffer::Undo() {
    DoneTyping();
    m_commandLog.Undo();
    SetDirtyFlag( TRUE );
}

void CEditBuffer::Redo() {
    DoneTyping();
    m_commandLog.Redo();
    SetDirtyFlag( TRUE );
}

void CEditBuffer::Trim() {
    DoneTyping();
    m_commandLog.Trim();
}

intn CEditBuffer::GetCommandHistoryLimit() {
    return m_commandLog.GetCommandHistoryLimit();
}

void CEditBuffer::SetCommandHistoryLimit(intn newLimit) {
    m_commandLog.SetCommandHistoryLimit(newLimit);
}

// Returns NULL if out of range
CEditCommand* CEditBuffer::GetUndoCommand(intn n) {
    return m_commandLog.GetUndoCommand(n);
}

CEditCommand* CEditBuffer::GetRedoCommand(intn n) {
    return m_commandLog.GetRedoCommand(n);
}

void CEditBuffer::BeginBatchChanges(intn id) {
    m_commandLog.BeginBatchChanges(new CEditCommandGroup(this, id));
}

void CEditBuffer::EndBatchChanges() {
    m_commandLog.EndBatchChanges();
}

void CEditBuffer::DoneTyping() {
#ifdef DEBUG
    if ( m_pTypingCommand )
        XP_TRACE(("Done typing."));
#endif
    m_pTypingCommand = 0;
}

void CEditBuffer::TypingDeleted(CTypingCommand* deletedCommand) {
    if ( deletedCommand == m_pTypingCommand ) {
        m_pTypingCommand = 0;
        XP_TRACE(("Typing deleted."));
    }
}

CTypingCommand* CEditBuffer::GetTypingCommand() {
    // Creates it if needed
    if ( ! m_pTypingCommand ) {
        m_pTypingCommand = new CTypingCommand(this, kTypingCommandID);
        m_commandLog.AdoptAndDo(m_pTypingCommand);
    }
    return m_pTypingCommand;
}

//-----------------------------------------------------------------------------
// CEditTagCursor
//-----------------------------------------------------------------------------
CEditTagCursor::CEditTagCursor( CEditBuffer* pEditBuffer, 
            CEditElement *pElement, int iEditOffset, CEditElement* pEndElement ):
            m_pEditBuffer(pEditBuffer), 
            m_pCurrentElement(pElement), 
            m_endPos(pEndElement,0),
            m_stateDepth(0),
            m_currentStateDepth(0),
            m_iEditOffset( iEditOffset ),
            m_pContext( pEditBuffer->GetContext() ),
            m_pStateTags(0),
            m_bClearRelayoutState(0),
            m_tagPosition(tagOpen){

    CEditElement *pContainer = m_pCurrentElement->FindContainer();
    if( (m_pCurrentElement->PreviousLeafInContainer() == 0 )
            && iEditOffset == 0
            && pContainer
            && pContainer->IsA( P_LIST_ITEM ) ){
        m_pCurrentElement = pContainer;
    }

    //m_bClearRelayoutState = (m_pCurrentElement->PreviousLeafInContainer() != 0 );

    //
    // If the element has a parent, setup the depth
    //
    pElement = m_pCurrentElement->GetParent();

    while( pElement ){
        PA_Tag *pTag = pElement->TagOpen( 0 );
        PA_Tag *pTagEnd = pTag;

        while( pTagEnd->next != 0 ) pTagEnd = pTag->next;

        pTagEnd->next = m_pStateTags;  
        m_pStateTags = pTag;
        pElement = pElement->GetParent();
    }
}

CEditTagCursor::~CEditTagCursor(){
    PA_Tag *pTag =  m_pStateTags;
    while ( pTag ) {
        PA_Tag* pNext = pTag->next;
        PA_FreeTag( pTag );
        pTag = pNext;
    }
}

CEditTagCursor* CEditTagCursor::Clone(){
    CEditTagCursor *pCursor = new CEditTagCursor( m_pEditBuffer, 
                                                  m_pCurrentElement,
                                                  m_iEditOffset, 
                                                  m_endPos.Element());
    return pCursor;
}


//
// We iterate through the edit tree generating tags.  The the goto code here
//  insures that after we deliver a tag, the m_pCurrentElement is always pointing
//  to the next element to be output
//
PA_Tag* CEditTagCursor::GetNextTag(){
    XP_Bool bDone = FALSE;
    PA_Tag* pTag;

    // Check for end
    if( m_pCurrentElement == 0 ){
        return 0;
    }


    while( !bDone && m_pCurrentElement != 0 ){
        // we are either generating the beginning tag, content or end tags
        switch( m_tagPosition ){

        case tagOpen:
            pTag = m_pCurrentElement->TagOpen( m_iEditOffset );
            bDone = (pTag != 0);
            m_iEditOffset = 0;  // only counts on the first tag.
            if( m_pCurrentElement->GetChild() ){
                m_pCurrentElement = m_pCurrentElement->GetChild();
                m_tagPosition = tagOpen;
            }
            else {
                m_tagPosition = tagEnd;
                if( !WriteTagClose(m_pCurrentElement->GetType()) ){
                    goto seek_next;
                }
            }
            break;

        case tagEnd:
            pTag = m_pCurrentElement->TagEnd();
            bDone = (pTag != 0);
        seek_next:
            if( m_pCurrentElement->GetNextSibling()){
                m_pCurrentElement = m_pCurrentElement->GetNextSibling();
                m_tagPosition = tagOpen;
            }
            else {
                //
                // We've exausted all the elements at this level. Set the 
                //  current element to the end of our parent.  m_tagPosition is
                //  already set to tagEnd, but lets be explicit
                //
                m_pCurrentElement = m_pCurrentElement->GetParent();
                m_tagPosition = tagEnd; 
                if( m_pCurrentElement && !WriteTagClose(m_pCurrentElement->GetType()) ){
                    goto seek_next;
                }
            }
            break;
        }
    }

    if( bDone ){
        return pTag;
    }
    else {
        XP_DELETE(pTag);
        return 0;
    }
}

PA_Tag* CEditTagCursor::GetNextTagState(){
    PA_Tag *pRet = m_pStateTags;
    if( m_pStateTags ){
        m_pStateTags = 0;
    }
    return pRet;    
}

XP_Bool CEditTagCursor::AtBreak( XP_Bool* pEndTag ){
    XP_Bool bAtPara;
    XP_Bool bEndTag; 

    if( m_pCurrentElement == 0 ){
        return FALSE;
    }

    if(  m_tagPosition == tagEnd ){
        bEndTag = TRUE;
        bAtPara = BitSet( edt_setTextContainer,  m_pCurrentElement->GetType()  );
    }
    else {
        bEndTag = FALSE;
        bAtPara = BitSet( edt_setParagraphBreak,  m_pCurrentElement->GetType()  );
    }

    if( bAtPara ){
        // force the layout engine to process this tag before computing line
        //  position.
        //if( m_pCurrentElement->IsA( P_LINEBREAK ){
        //    bEndTag = TRUE;
        //}

        // if there is an end position and the current position is before it
        //  ignore this break.
        CEditPosition p(m_pCurrentElement);

        if( m_endPos.IsPositioned() &&
                 m_endPos.Compare(&p) <= 0  ){
            return FALSE;
        }
        if( CurrentLine() != -1 ){
            *pEndTag = bEndTag;
            return TRUE;
        }
        else {
            //XP_ASSERT(FALSE)
        }
    }
    return FALSE;
}


int32 CEditTagCursor::CurrentLine(){ 
    CEditElement *pElement;
    LO_Element * pLayoutElement, *pLoStartLine;
    int32 iLineNum;

    //
    // Pop to the proper state
    //
    CEditElement *pSave = m_pCurrentElement;
    while( m_pCurrentElement && m_tagPosition == tagEnd ){
        // some convelouded shit, so do it this way.
        PA_Tag* pTag = GetNextTag();
        PA_FreeTag( pTag );
        m_tagPosition = tagEnd;     // restore this.
    }
    

    // if we fell of the end of the document, then we are done.
    if( m_pCurrentElement == 0 ){
        m_pCurrentElement = pSave;
        return -1;
    }

    // LTNOTE: need to check to see if we are currently at a TextElement..
    pElement = m_pCurrentElement->NextLeaf();
    m_pCurrentElement = pSave;

    if( pElement == 0 ){
        return -1;
    }

    XP_ASSERT( pElement->IsLeaf() );
    pLayoutElement = pElement->Leaf()->GetLayoutElement();
    if( pLayoutElement == 0 ){
        return -1;
    }

    // Find the first element on this line.
    pLoStartLine = LO_FirstElementOnLine( m_pContext, pLayoutElement->lo_any.x, 
                        pLayoutElement->lo_any.y, &iLineNum );

    // LTNOTE: this isn't true in delete cases.
    //XP_ASSERT( pLoStartLine == pLayoutElement );

    return iLineNum;
}

#ifdef XP_WIN16
// code segment is full, switch to a new segment
#pragma code_seg("EDTSAVE3_TEXT","CODE")
#endif

#endif

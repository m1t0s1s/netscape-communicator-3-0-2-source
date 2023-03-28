#include "awt_textarea.h"
#include "awt_toolkit.h"
#include "awt_defs.h"
#ifdef INTLUNICODE
#include "awt_i18n.h"
#endif

// All java header files are extern "C"
extern "C" {
#include "java_awt_TextArea.h"

#include "sun_awt_windows_WTextComponentPeer.h"
#include "sun_awt_windows_WTextAreaPeer.h"
};


AwtTextArea::AwtTextArea(HWTextAreaPeer *peer) : AwtTextComponent((JHandle*)peer)
{
    DEFINE_AWT_SENTINAL("TXA");
}


// -----------------------------------------------------------------------
//
// Return the window class of an AwtTextArea window.
//
// -----------------------------------------------------------------------
const char * AwtTextArea::WindowClass()
{
    return "EDIT";
}


// -----------------------------------------------------------------------
//
// Return the window style used to create an AwtButton window
//
// -----------------------------------------------------------------------
DWORD AwtTextArea::WindowStyle()
{
    return WS_CHILD | WS_VSCROLL | WS_BORDER | WS_CLIPSIBLINGS |
           ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL;
}


// -----------------------------------------------------------------------
//
// Set the edit buffer to the contents of a Java String.
//
// This method does not require locking because no member variables
// are accessed!.  The edit buffer is changed via ReplaceText(...)
// which is synchronized.
//
// -----------------------------------------------------------------------
void AwtTextArea::SetText(Hjava_lang_String *string)
{
    ReplaceText(string, 0, -1);
}


// -----------------------------------------------------------------------
//
// Replace the text within the specified range in the edit control with
// the contents of the Java String...
//
// This method does not require locking because no member variables
// are accessed!.  The edit buffer is changed via ReplaceText(...)
// which is synchronized.
//
// -----------------------------------------------------------------------
void AwtTextArea::ReplaceText(Hjava_lang_String *string, long start, long end)
{
    char *buffer;
#ifdef INTLUNICODE
	buffer = libi18n::intl_allocCString(this->GetPrimaryEncoding() , string);
	if( buffer ) {
		// Be careful, although we pass in text in non-Unicode here, we 
		// still pass in start, end as CharLen instead of Byte Count
		buffer = ReplaceText(buffer, start, end);
		free(buffer);
	}
#else // INTLUNICODE
   long len;

   len    = javaStringLength(string);

   buffer = (char *)malloc(len+1);
    if (buffer) {
        javaString2CString(string, buffer, len+1);

		// the following function may realloc buffer 
        buffer = ReplaceText(buffer, start, end);

        free(buffer);
    }
#endif // INTLUNICODE
}


// -----------------------------------------------------------------------
//
// Replace the text within the specified range in the edit control with
// the contents of the character buffer...
//
// Accomodation for CRs is centralized in this method.
// Assume the buffer passed in is without CRs (which should always be the case)
// Return the buffer reallocated if any CRs or the same buffer passed in
//
// -----------------------------------------------------------------------
char * AwtTextArea::ReplaceText(char *buffer, long start, long end)
{
    ASSERT( ::IsWindow(m_hWnd) );

    // adjust for CRs
    char* newBuffer   = AwtTextArea::AddCRtoBuffer(buffer);

    //
    // Lock the object to prevent other threads from changing the
    // contents of the Edit control..
    //
    Lock();

    SelectText(start, end);

    //
    // On Win16 the WPARAM must be 0, however, on Win32 it specifies
    // whether the insertion can be undone (0 is no undo).
    //
    ::SendMessage(m_hWnd, EM_REPLACESEL, 0, (LPARAM)newBuffer);

    Unlock();

	return newBuffer;
}


// -----------------------------------------------------------------------
//
// Select the specified range of text in the edit control...
//
// -----------------------------------------------------------------------
void AwtTextArea::SelectText(long start, long end)
{
    ASSERT( ::IsWindow(m_hWnd) );

    Lock();
    
#ifdef INTLUNICODE
	// To support unicode, we need to convert start and end from CharLen to ByteCount 
	// Here
	if((start > 0 ) || (end > 0))
	{
		int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
		unsigned char *buffer = NULL;
		if(len > 0)
			buffer = (unsigned char *)malloc(len+1);
		if (buffer) {
			int charLen;
			::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
			charLen = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
								buffer, len);
			if((start > 0) && (start <= charLen))
				start = libi18n::INTL_TextCharLenToByteCount(this->GetPrimaryEncoding(), 
								buffer, start);
			if((end > 0) && (end <= charLen))
				end = libi18n::INTL_TextCharLenToByteCount(this->GetPrimaryEncoding(), 
								buffer, end);
			free(buffer);
		}
	}

#endif

    //
    // Adjust the start and end values to compensate for the extra '\r'
    // at the end of each line...
    //
    start = AddCRtoOffset(start);
    end   = AddCRtoOffset(end);

    //
    // Select the specified range of text in the Edit control
    //
#ifdef _WIN32
    ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
#else
    ::SendMessage(m_hWnd, EM_SETSEL, (WPARAM)1, MAKELONG((int)start, (int)end));
#endif

    Unlock();
}


// -----------------------------------------------------------------------
//
// Return the starting position of the selected text in the Edit buffer...
//
// This method does not require locking because no member variables
// are accessed!.  The SendMessage(...) synchronizes the access to the 
// edit buffer.
//
// REMIND: Should the selection be adjusted for the number of CR's
//         in the buffer??
// -----------------------------------------------------------------------
long AwtTextArea::GetSelectionStart(void)
{
    long selStart;

    ASSERT( ::IsWindow(m_hWnd) );

#ifdef _WIN32
    /*selStart =*/ ::SendMessage(m_hWnd, EM_GETSEL, (WPARAM)&selStart, NULL);
#else
    selStart = ::SendMessage(m_hWnd, EM_GETSEL, 0, 0L);
    selStart = LOWORD(selStart);
#endif

#ifdef INTLUNICODE
	// To support unicode, we need to convert selStart from byteCount back to charLen 
	// Here
	if(selStart > 0)
	{
		int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
		unsigned char *buffer = NULL;
		if(len > 0)
			buffer = (unsigned char *)malloc(len+1);
		if (buffer) {
			::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
			if(selStart <= len)
				selStart = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
								buffer, selStart);
			free(buffer);
		}
	}

#endif
    selStart -= ::SendMessage(m_hWnd, EM_LINEFROMCHAR, (WPARAM)selStart, 0L);


    return selStart;
}


// -----------------------------------------------------------------------
//
// Return the ending position of the selected text in the Edit buffer...
//
// This method does not require locking because no member variables
// are accessed!.  The SendMessage(...) synchronizes the access to the 
// edit buffer.
//
// REMIND: Should the selection be adjusted for the number of CR's
//         in the buffer??
// -----------------------------------------------------------------------
long AwtTextArea::GetSelectionEnd(void)
{
    long selEnd;

    ASSERT( ::IsWindow(m_hWnd) );

#ifdef _WIN32
    /*selEnd =*/ ::SendMessage(m_hWnd, EM_GETSEL, NULL, (LPARAM)&selEnd);
#else
    selEnd = ::SendMessage(m_hWnd, EM_GETSEL, 0, 0L);
    selEnd = HIWORD(selEnd);
#endif

#ifdef INTLUNICODE
	// To support unicode, we need to convert selStart from byteCount back to charLen 
	// Here
	if(selEnd > 0)
	{
		int len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
		unsigned char *buffer = NULL;
		if(len > 0)
			buffer = (unsigned char *)malloc(len+1);
		if (buffer) {
			::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);
			if(selEnd <= len)
				selEnd = libi18n::INTL_TextByteCountToCharLen(this->GetPrimaryEncoding(), 
								buffer, selEnd);
			free(buffer);
		}
	}

#endif
    selEnd -= ::SendMessage(m_hWnd, EM_LINEFROMCHAR, (WPARAM)selEnd, 0L);

    return selEnd;
}


// -----------------------------------------------------------------------
//
// Return a copy of the contents of the edit control.
//
// -----------------------------------------------------------------------
Hjava_lang_String* AwtTextArea::GetText(void)
{
    int len;
    char *buffer = NULL;
    Hjava_lang_String *string = NULL;

    ASSERT( ::IsWindow(m_hWnd) );

    //
    // Lock the object to prevent other threads from changing the
    // contents of the Edit control..
    //
    Lock();

    len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);
    buffer = (char *)malloc(len+1);
    if (buffer) {
        ::SendMessage(m_hWnd, WM_GETTEXT, len+1, (LPARAM)buffer);

        buffer = RemoveCRfromBuffer(buffer);
        len = strlen(buffer);

#ifdef INTLUNICODE
		string = (Hjava_lang_String*)libi18n::intl_makeJavaString(GetPrimaryEncoding(), buffer, len);
#else	// INTLUNICODE
		string = makeJavaString(buffer, len);
#endif	// INTLUNICODE
	free(buffer);
	}

    Unlock();
    return string;
}


// -----------------------------------------------------------------------
//
// Returns a copy of null-terminated buffer where each '\n' has been 
// converted to '\r\n'.  
//
// The buffer is realloc()'ed to accomodate the extra '\r' characters.
//
// -----------------------------------------------------------------------
char * AwtTextArea::AddCRtoBuffer(char *buffer) 
{
    int nNewlines, nLen;
    char *pStr;
    char *result;

    //
    // Count the # of newlines in buffer
    //
    nNewlines = 0;
    for (nLen=0; buffer[nLen]; nLen++) {
        if (buffer[nLen] == '\n') {
            nNewlines++;
        }
    }

    //
    // If the buffer has no newlines, then return the buffer as-is
    //
    if (nNewlines == 0) {
        return buffer;
    }

    //
    // Realloc the buffer to accomodate the extra '\r' required for
    // each newline...
    //
    result = (char *)realloc(buffer, nLen + nNewlines + 1);


    //
    // Walk back through the buffer adding the '\r' characters 
    // along the way...
    //
    buffer = result + nLen;
    pStr   = result + nLen + nNewlines;

    while (buffer >= result) {
        *pStr-- = *buffer;
        if (*buffer == '\n') {
            *pStr-- = '\r';
        }
        buffer--;
    }

    return result;
}


// -----------------------------------------------------------------------
//
// Returns a copy of null-terminated buffer where each '\r\n' has been 
// converted to '\n'.  
//
// -----------------------------------------------------------------------
char * AwtTextArea::RemoveCRfromBuffer(char *buffer) 
{
    char *pStr;
    char *result;

    pStr = result = buffer;
    if (result) {
        //
        // Start at the front of the buffer converting any '\r\n' to '\n'
        //
        while (*pStr && *result) {
            if ((*pStr == '\r') && (*(pStr+1) == '\n')) {
                pStr++;
            }
            *buffer++ = *pStr++;
        }
    }
    *buffer = '\0';
    return result;
}


// -----------------------------------------------------------------------
//
// Adjust the position to reflect the number of '\r' characters that 
// have been added to the edit buffer.
//
// This method can ONLY be called when the AwtTextArea is locked!
//
// -----------------------------------------------------------------------
long AwtTextArea::AddCRtoOffset(long pos)
{
    char* pEditBuffer;
    int lEditBufferSize = 0, lBufferPos = pos;

    ASSERT( ::IsWindow(m_hWnd) );
    ASSERT( this->IsLocked() );

    // allocate a buffer to hold the edit field content
    if (pos > 0) {
        lEditBufferSize = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0L);
        if (lEditBufferSize && pos < lEditBufferSize) {
            pEditBuffer = (char*) malloc(lEditBufferSize + 1);
            if (pEditBuffer) {
                ::SendMessage(m_hWnd, WM_GETTEXT, (WPARAM)lEditBufferSize, (LPARAM)pEditBuffer);
                // loop through the edit to find the real position asked
                // The problem here is that java does not have any idea about the \r 
                // required by window to move to the next line.
                // From a java prospective a \n is enough.
                // So if someone is keeping a counter on the java side that counter is
                // totally screwed up since it does not account for \r
                lBufferPos = 0;
                while (pos && lBufferPos <= lEditBufferSize) {

                    // decrement only if not \r\n
                    if (pEditBuffer[lBufferPos] != '\r' /*|| pEditBuffer[lBufferPos + 1] != '\n'*/) 
                        pos--;

                    // increment no matter what because this is the real position
                    lBufferPos++;
                }

                // if we are in between a /r/n move to the end of line
                //if (pEditBuffer[lBufferPos] == '\r' /*&& pEditBuffer[lBufferPos + 1] == '\n'*/) 
                //    lBufferPos++;

                free(pEditBuffer);

            }
        }
        else
            lBufferPos = lEditBufferSize;
    }

    return lBufferPos;
    //
    // Only adjust the offset if pos is positive... Otherwise the end of the buffer
    // is being referenced... So, let the edit control figure it out...
    //
#if 0
    long nLines1, nChar;

    if (pos > 0) {
        nLines1 = ::SendMessage(m_hWnd, EM_LINEFROMCHAR, (WPARAM)pos, 0L);
        while (nLines1 != pos) {
            pos += nLines1;
            nChar = ::SendMessage(m_hWnd, EM_LINEINDEX, (WPARAM)nLines1, 0L);
            if (nChar > 
        }

        nLines2 = ::SendMessage(m_hWnd, EM_LINEFROMCHAR, (WPARAM)(pos + 1), 0L);
        if (nLines1 != nLines2)
            pos++;
    }

    return pos;
#endif

}





//-------------------------------------------------------------------------
//
// Native method implementations for sun.awt.WTextAreaPeer
//
//-------------------------------------------------------------------------

extern "C" {

void
sun_awt_windows_WTextAreaPeer_create(HWTextAreaPeer *self, 
                                     HWComponentPeer *hParent)
{
    AwtTextArea *pNewTextArea;

    CHECK_NULL(self,    "TextAreaPeer is null.");
    CHECK_NULL(hParent, "ComponentPeer is null.");

    pNewTextArea = new AwtTextArea(self);
    CHECK_ALLOCATION(pNewTextArea);

    // REMIND: What about locking the new object??
    pNewTextArea->CreateWidget( pNewTextArea->GetParentWindowHandle(hParent) );
}

#if 0
void
sun_awt_windows_WTextAreaPeer_widget_setEditable(HWTextAreaPeer *self, 
                                                 long flag)
{
    CHECK_PEER(self);

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    obj->SetEditable((BOOL)flag);
}


void
sun_awt_windows_WTextAreaPeer_select(HWTextAreaPeer *self, 
                                     long start, long end)
{
    CHECK_PEER(self);

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);
    
    obj->SelectText(start, end);
}


long
sun_awt_windows_WTextAreaPeer_getSelectionStart(HWTextAreaPeer *self)
{
    CHECK_PEER_AND_RETURN(self, 0);

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);
    
    return obj->GetSelectionStart();
}


long
sun_awt_windows_WTextAreaPeer_getSelectionEnd(HWTextAreaPeer *self)
{
    CHECK_PEER(self);

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    return obj->GetSelectionEnd();
}


void
sun_awt_windows_WTextAreaPeer_setText(HWTextAreaPeer *self, 
                                      Hjava_lang_String *string)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    obj->SetText(string);
}


Hjava_lang_String *
sun_awt_windows_WTextAreaPeer_getText(HWTextAreaPeer *self)
{
    CHECK_PEER(self, NULL);

    Hjava_lang_String *string;
    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    string = obj->GetText();
    return string;
}
#endif

void
sun_awt_windows_WTextAreaPeer_insertText(HWTextAreaPeer *self, 
                                         Hjava_lang_String *string, long pos)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    obj->ReplaceText(string, pos, pos);
}


void
sun_awt_windows_WTextAreaPeer_replaceText(HWTextAreaPeer *self, 
                                          Hjava_lang_String *string, 
                                          long start, long end)
{
    CHECK_PEER(self);
    CHECK_NULL(string, "null text");

    AwtTextArea *obj = GET_OBJ_FROM_PEER(AwtTextArea, self);

    obj->ReplaceText(string, start, end);
}


};  // end of extern "C"

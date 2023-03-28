#ifndef AWT_TEXTAREA_H
#define AWT_TEXTAREA_H

#include "awt_defs.h"
#include "awt_textcomponent.h"

DECLARE_SUN_AWT_WINDOWS_PEER(WTextAreaPeer);

class AwtTextArea : public AwtTextComponent 
{
public:
                            AwtTextArea(HWTextAreaPeer *peer);

    virtual void            SetText(Hjava_lang_String *string);
            void            ReplaceText(Hjava_lang_String *string, 
                                        long start, long end);
            char *          ReplaceText(char *buffer, 
                                        long start, long end);

            long            GetSelectionStart(void);
            long            GetSelectionEnd(void);
            void            SelectText(long start, long end);

    virtual Hjava_lang_String * GetText(void);

protected:
    //
    // Window class and style of an AwtTextArea window
    //
    virtual const char *    WindowClass();
    virtual DWORD           WindowStyle();

private:
            char *          AddCRtoBuffer(char *buffer);
            long            AddCRtoOffset(long offset);

            char *          RemoveCRfromBuffer(char *buffer);

};


#endif  // AWT_TEXTAREA_H

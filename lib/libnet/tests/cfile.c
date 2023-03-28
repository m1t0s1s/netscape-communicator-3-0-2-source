
#include "memory.h"
#include "net.h"
#include "structs.h"

/* #include "ctxtfunc.h" */

#include "xp_core.h"
#include "fe_proto.h"

#include "netwrap.h"
#include "private.h"

char *
XP_GetString(int16 i)
{
    extern char * XP_GetBuiltinString(int16 i);

    return XP_GetBuiltinString(i);
}

print_msg(const char *msg)
{
	printf(msg);
	printf("\n");
	fflush(stdout);
}

void FE_Alert(MWContext * context, const char * Msg)
{
	print_msg(Msg);
}

void LWFE_Progress (MWContext *cx, const char *msg)
{
	print_msg(msg);
}

XP_Bool LWFE_Confirm(MWContext * cx, const char * msg)
{
	print_msg(msg);
    return 1;
}

char *LWFE_Prompt(MWContext * cx, const char * msg, const char * dflt)
{
	print_msg(msg);
	return NULL;
}

char *LWFE_PromptPassword(MWContext * cx, const char * msg)
{
	print_msg(msg);
	return NULL;
}

XP_Bool LWFE_PromptUsernameAndPassword(MWContext *cx,const char *msg,
        char **username, char **password)
{
	print_msg(msg);
	return NULL;
}

/*  additions for link errors */

int FE_StrColl(const char* s1, const  char* s2) 
{
    return(strcoll(s1, s2));
}

size_t FE_StrfTime(MWContext* context, 
                    char *result, 
                    size_t maxsize, 
                    int format,
                    const struct tm *timeptr)
{
	XP_ASSERT(0);
    return(strftime(result, maxsize, (char*)format, timeptr));
}

#ifdef XP_WIN
/* this needs to work sometime for ssl to 
 * be secure
 */
size_t SEC_GetNoise(void *buf, size_t max_bytes)
{
	XP_ASSERT(0);
	return(0);
}
#endif

/* needed for ssl lib.  It's alright to
 * always fail
 */
int dupsocket(int foo)
{
	return(-1);
}

#ifndef XP_MAC
void XP_Trace (const char *msg, ...)
{
 	char vbuffer[2000];
	va_list stack;

	va_start(stack, msg);
	vsprintf(vbuffer, msg, stack);

	/* write trace output here */

#ifdef XP_WIN
    NW_Trace(vbuffer);
    NW_Trace("\r\n");
#endif  /* XP_WIN */

	printf(vbuffer);
	printf("\n");

	va_end(stack);
}
#endif

void XP_AssertAtLine (char *filename, int line)
{
#ifdef XP_WIN
    char buffer[2000];
    int result;
    sprintf(buffer, "%s at line %d", filename, line);
    result = MessageBox(0, buffer, "Assert", MB_ABORTRETRYIGNORE);
    if (IDABORT == result) exit(0);
    if (IDRETRY == result) __asm int 3;
#else
    assert(0);
#endif
}

void *FE_AboutData(const char *which,
                         char **data_ret,
                         int32 *length_ret,
                         char **content_type_ret)
{
    return 0;
}

void FE_FreeAboutData(void * data, const char* which)
{
	XP_ASSERT(FALSE);
}

/*
 *      Also need to know when the URL is being deleted by the
 *              netlib so can clean up extra structures hanging off
 *              the url structure.
 */
void FE_DeleteUrlData(URL_Struct *pUrl)
{
}
 
void FE_ClearDNSSelect(MWContext * win_id, int fd)
{
}

int32 FE_GetContextID(MWContext * window_id)
{
    return (int32)window_id;
}

extern Bool FE_SecurityDialog(MWContext * context, int message)
{
    return TRUE;
}

void *FE_SetTimeout(TimeoutCallbackFunction func, void * closure, uint32 msecs)
{
    return 0;
}

void FE_URLEcho(URL_Struct *pURL, int iStatus, MWContext *pContext)
{
}

/*
 *      Windows needs a way to know when a URL switches contexts,
 *              so that it can keep the NCAPI progress messages
 *              specific to a URL loading and not specific to the
 *              context attempting to load.
 */
void FE_UrlChangedContext(URL_Struct *pUrl, MWContext *pOldContext, MWContext *pNewContext)
{
}

/* end of additions for link errors */

void FE_SetReadSelect(
    MWContext *context,
    int sd)
{
}

void FE_ClearReadSelect(
    MWContext *context,
    int sd)
{
}

void FE_SetConnectSelect(
    MWContext *context,
    int sd)
{
}

void FE_ClearConnectSelect(
    MWContext *context,
    int sd)
{
}

void FE_SetFileReadSelect(
    MWContext *context,
    int sd)
{
}

void FE_ClearFileReadSelect(
    MWContext *context,
    int sd)
{
}

void xl_alldone() { }
void LWFE_Alert () {}
void LWFE_SetDocPosition() {}
void LWFE_GetDocPosition() {}
void LWFE_FreeFormElement() { }
void LWFE_ClearView() { }
void LWFE_CreateNewDocWindow() { }
void LWFE_SetFormElementToggle() {}
void LWFE_GetFormElementInfo() { }
void LWFE_SetProgressBarPercent() { }
void LWFE_FreeImageElement() { }
void LWFE_FormTextIsSubmit() {}
void LWFE_GetFormElementValue() {}
void LWFE_ResetFormElement() {}
void LWFE_DisplayFormElement() { }
void LWFE_SetDocDimension() { }
void LWFE_SetDocTitle() { }
void LWFE_BeginPreSection() { }
void LWFE_EndPreSection() { }
void LWFE_DisplaySubDoc() { }
void LWFE_UseFancyFTP () {}
void LWFE_FileSortMethod () {}
void LWFE_EnableClicking () {}
void LWFE_ShowAllNewsArticles () {}
void LWFE_GraphProgressDestroy () {}
void LWFE_ImageData () {}
void LWFE_ImageOnScreen () {}
void LWFE_SetColormap () {}
void LWFE_GraphProgressInit () {}
void LWFE_ImageSize () {}
void LWFE_UseFancyNewsgroupListing () {}
void LWFE_GraphProgress () {}
void LWFE_ImageIcon() {}
void LWFE_SetBackgroundColor() {}
void LWFE_GetEmbedSize(){}
void LWFE_FreeEmbedElement(){}
void LWFE_DisplayEmbed(){}
void LWFE_GetJavaAppSize(){}
void LWFE_FreeJavaAppElement(){}
void LWFE_HideJavaAppElement(){}
void LWFE_DisplayJavaApp(){}
void LWFE_FreeEdgeElement(){}
void LWFE_DisplayCell(){}
void LWFE_DisplayEdge(){}


void LWFE_AllConnectionsComplete(MWContext *cx)
{
    /* stop spinning the icon */
}

void LWFE_SetCallNetlibAllTheTime(MWContext *cx)
{
}

void LWFE_ClearCallNetlibAllTheTime(MWContext *cx)
{
}

int LWFE_GetTextInfo(MWContext *cx, LO_TextStruct *text, LO_TextInfo *text_info)
{
    return 0;
}

void LWFE_LayoutNewDocument(MWContext *cx, URL_Struct *url, int32 *w, int32 *h,
    int32* mw, int32* mh)
{
}

void LWFE_DisplaySubtext(MWContext *cx, int iLocation, LO_TextStruct *text,
    int32 start_pos, int32 end_pos, XP_Bool notused)
{
}

void LWFE_DisplayText(MWContext *cx, int iLocation, LO_TextStruct *text, XP_Bool
    needbg)
{
}

void LWFE_DisplayTable(MWContext *cx, int iLoc, LO_TableStruct *table)
{
}

void LWFE_DisplayLineFeed(MWContext *cx, int iLocation, LO_LinefeedStruct
    *line_feed, XP_Bool notused)
{
}

void LWFE_DisplayHR(MWContext *cx, int iLocation , LO_HorizRuleStruct *HR)
{
}

char * LWFE_TranslateISOText(MWContext *cx, int charset, char *ISO_Text)
{
    return ISO_Text;
}

void LWFE_DisplaySubImage(MWContext *cx, int iLoc, LO_ImageStruct *image_struct,
    int32 x, int32 y, uint32 width, uint32 height)
{
}

void LWFE_DisplayImage(MWContext *cx, int iLocation, LO_ImageStruct *img)
{
}

void LWFE_GetImageInfo(MWContext *cx, LO_ImageStruct *image, NET_ReloadMethod reload)
{
}

void LWFE_DisplayBullet(MWContext *cx, int iLocation, LO_BullettStruct *bullet)
{
}

void LWFE_FinishedLayout(MWContext *cx)
{
}

void LWFE_UpdateStopState(MWContext *cx)
{
}


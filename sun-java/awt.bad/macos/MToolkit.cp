//##############################################################################
//##############################################################################
//
//	File:		MToolkit.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################
#include "MToolkit.h"
#include <LPane.h>
#include <PP_Messages.h>
#include "MTextAreaPeer.h"
#include <UDesktop.h>

extern "C" {

#include "mdmacmem.h"
#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "sun_awt_macos_MToolkit.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "java_awt_Component.h"
#include "interpreter.h"
#include "exceptions.h"
#include "prlog.h"
#include "prgc.h"
#include "swkern.h"
#undef __cplusplus
#define IMPLEMENT_netscape_applet_MozillaAppletContext
#include "n_applet_MozillaAppletContext.h"
#include "java.h"
#include "jri.h"
#include "lj.h"

extern void FreeClassArenas(void);

};

#include "fredmem.h"

enum {
	kTimeBetweenStationaryMouseMovedEvents	= 20,
	kMacJavaAppletMaxHistorySize			= 1
};


#ifndef CAFFEINE
#include "earlmgr.h"
#endif

extern "C" {
#include "uapp.h"
#include "lj.h"
}

#include <Quickdraw.h>
#include <UDesktop.h>

#include "MCanvasPeer.h"
#include "MFramePeer.h"

enum {
	kDefaultColorTableResID				 = 2048
};

Boolean							gHasScreenResolution = false;
Rect							gMainScreenBounds;

long							gTotalClickCount = 0;
Hsun_awt_macos_MComponentPeer 	*gCurrentJavaTarget;



void GetScreenResolution(void)
{
	if (!gHasScreenResolution) {
	
		GDHandle		mainScreenDevice;
	
		mainScreenDevice = GetMainDevice();
		gMainScreenBounds = (**mainScreenDevice).gdRect;
	
		gHasScreenResolution = true;
		
	}
}

UInt32 MToolkitExecutJavaDynamicMethod(void *obj, char *method_name, char *signature, ...)
{
	UInt32 returnValue;
	va_list args;
	va_start(args, signature);
	returnValue = (UInt32)do_execute_java_method_vararg(EE(), obj, method_name, signature,
					      0, (bool_t)FALSE, args, 0, (bool_t)FALSE);
	va_end(args);

	if (exceptionOccurred(EE())) {
	    exceptionDescribe(EE());
    }
	return returnValue;
}

UInt32 MToolkitExecutJavaStaticMethod(ClassClass *classObject, char *method_name, char *signature, ...)
{
	UInt32 returnValue;
    	struct 	methodblock *mb = NULL;
    	Int32	i;

	va_list args;
	va_start(args, signature);

    /* XXX Why are we doing this?? do_execute_java_method_vararg will throw an exception
     * if the method is not found. */
    /* Lookup the method */
    mb = cbMethods(classObject);
    for (i = classObject->methods_count; --i >= 0; mb++) {
		if ((strcmp(fieldname(&mb->fb), method_name) == 0) &&
		    (strcmp(fieldsig(&mb->fb), signature) == 0)) {
		    break;
		}
    }

	if (mb == NULL) {
#if DEBUG
		DebugStr("\pJava: MToolkitExecutJavaStaticMethod called with illegal function");
#endif
		return 0;	
	}

	returnValue = (UInt32)do_execute_java_method_vararg(EE(), classObject, method_name, signature, 0, (bool_t)TRUE, args, 0, (bool_t)FALSE);

	va_end(args);

	if (exceptionOccurred(EE())) {
	    exceptionDescribe(EE());
    }
    
    return returnValue;
}

LWindow *
GetContainerWindow(LPane *currentPane)
{
	LView		*currentContainer,
				*parentView;
	
	currentContainer = currentPane->GetSuperView();
	
	if (currentContainer == NULL) {
		if (currentPane->GetPaneID() == MFramePeer::class_ID)
			return (LWindow *)currentPane;
		else
			return NULL;
	}
	
	while (1) {
		
		parentView = currentContainer->GetSuperView();
		if (parentView == NULL)
			break;
		currentContainer = parentView;
	
	}
	
	if (currentContainer->GetPaneID() == 'java')
		return NULL;
	else
		return (LWindow *)currentContainer;
}

enum {
	kEventHistoryLength	= 32
};

EventRecord								gEventRecordHistory[kEventHistoryLength];
UInt32									gEventRecordHistoryNextItem = 0;

Hsun_awt_macos_MComponentPeer			*gCurrentMouseTargetPeer = NULL;

extern "C" jint
LJ_GetTotalApplets(); // FIX ME

unsigned char JavaAppletHistoryFlusher(size_t bytesNeeded)
{
	SInt32			totalApplets,
					appletsToTrim,
					appletsWeDidTrim;
	static Boolean	alreadyInFlusher = false;	

	DebugStr("\pLJ_GetTotalApplets"); // FIX ME

	if (alreadyInFlusher)
	{
#if DEBUG
		DebugStr("\pJavaAppletHistoryFlusher has been called re-intrantly");
#endif
		return false;
	}
		
	alreadyInFlusher = true;
	
	//	Make sure that we can't pre-empt during this cache flusher
	DeactivateTimer();
	
	if ((EE())->exceptionKind == EXCKIND_YIELD)
	    (EE())->exceptionKind = EXCKIND_NONE;	
	
	//	Trim half of the applets that we have.
	totalApplets = LJ_GetTotalApplets();
	appletsToTrim = totalApplets / 2;
	
	//	First try to trim the inactive applets.  If that doesn't
	//	get us anywhere, kill the LRU active applet.
	appletsWeDidTrim = LJ_TrimApplets(appletsToTrim, true);
	
	if (appletsWeDidTrim == 0)
		appletsWeDidTrim = LJ_TrimApplets(1, false);
	
	ActivateTimer();

	alreadyInFlusher = false;
	
	return (appletsWeDidTrim != 0);
}

MToolkit::
MToolkit():LAttachment(msg_Event, true)
{
	mLastNullPoint.h = 0;
	mLastNullPoint.v = 0;
	mFrameList = new LList(sizeof(LPane *));
	if (mFrameList == NULL)
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);		
	
}

void
MToolkit::AddFrame(LPane *frame)
{
	if (mFrameList->FetchIndexOf(&frame) == 0)
		mFrameList->InsertItemsAt(1, 1, &frame);
}
		
void
MToolkit::RemoveFrame(LPane *frame)
{
	mFrameList->Remove(&frame);
}

Hsun_awt_macos_MComponentPeer *
MToolkit::LocatePointInFrame(LPane *currentPane, Point &incomingPoint)
{
	Hsun_awt_macos_MComponentPeer			*currentPeer = NULL;
	Rect									localRect;
	Point									incomingPointCopy = incomingPoint;

	if (!currentPane->IsVisible())
		return NULL;

	currentPane->CalcLocalFrameRect(localRect);
	currentPane->GlobalToPortPoint(incomingPointCopy);
	currentPane->PortToLocalPoint(incomingPointCopy);

	if (!PtInRect(incomingPointCopy, &localRect))
		return NULL;
		
	currentPeer = PaneToPeer(currentPane);
	if (currentPeer == NULL)
		return NULL;
	
	//	The point must not be in the obscured region as well.
	
	if (unhand(currentPeer)->mObscuredRgn == NULL)
		return NULL;
	
	if (PtInRgn(incomingPointCopy, (RgnHandle)(unhand(currentPeer)->mObscuredRgn)))
		return NULL;
		
	//	If the pane is a container, check all of the sub
	//	views to see if they contain the point.
	
	if (unhand(currentPeer)->mIsContainer) {
	
		LView								*currentPaneAsView = (LView *)currentPane;
		LListIterator						iterator(currentPaneAsView->GetSubPanes(), iterate_FromStart);
		LPane								*theSub;
		Hsun_awt_macos_MComponentPeer		*childPeer;
		
		while (iterator.Next(&theSub)) {

			childPeer = LocatePointInFrame(theSub, incomingPoint);

			if (childPeer != NULL)
				return childPeer;

		}	
	
	}
	
	//	Check to make sure that the point is actually visible. 
	
	currentPane->FocusDraw();
	
	static	RgnHandle tempRegion = NULL;
	if (tempRegion == NULL)
		tempRegion = ::NewRgn();

	if (tempRegion == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return NULL;
	}

	::GetClip(tempRegion);
	
	if (::PtInRgn(incomingPointCopy, tempRegion)) {
		incomingPoint = incomingPointCopy;
		return currentPeer;
	}
	
	else {
		return NULL;
	}
	
}

Hsun_awt_macos_MComponentPeer *
MToolkit::LocatePoint(Point &incomingPoint)
{
	Hsun_awt_macos_MComponentPeer			*currentPeer = NULL;
	LListIterator							iterator(*mFrameList, iterate_FromStart);
	LPane									*theSub;
	LWindow									*frontRegularWindow = LWindow::FetchWindowObject(FrontWindow());
	
	while (iterator.Next(&theSub)) {

		LWindow		*ourWindowObject = GetContainerWindow(theSub);
		
		if (ourWindowObject != frontRegularWindow)
			continue;
		
		currentPeer = LocatePointInFrame(theSub, incomingPoint);

		if (currentPeer != NULL)
			return currentPeer;

	}
	
	return NULL;

}

void
MToolkit::ExecuteSelf(MessageT inMessage, void *ioParam)
{
	Hsun_awt_macos_MComponentPeer	*targetPeer;
	EventRecord						*incomingEvent = (EventRecord *)ioParam;
	Point							translatedPoint = incomingEvent->where;
	Boolean							executeInHost = true;
	static UInt32					gNextTimeToSendStationaryEvent = 0;
	UInt32							currentTime = TickCount();
	Boolean							memoryIsReallyLow = false;
	Boolean							timeToTrimApplet = false;
	Boolean							eventToRemember = false;

	gEventRecordHistory[gEventRecordHistoryNextItem] = *incomingEvent;

	FreeClassArenas();

	//	Only send events to the front window.	
	WindowRef		eventWindow;
	::FindWindow(translatedPoint, &eventWindow);
	
	if (eventWindow == ::FrontWindow()) {
	
		switch (incomingEvent->what) {

			case nullEvent:
			
				//	Only send mouse moved events when the mouse actually moves.

				if ((incomingEvent->where.h != mLastNullPoint.h) ||
					(incomingEvent->where.v != mLastNullPoint.v) ||
					(gNextTimeToSendStationaryEvent < TickCount())) {
					
					targetPeer = LocatePoint(translatedPoint);
					
	//				if (PeerToPane(targetPeer)->GetPaneID() != MCanvasPeer::class_ID)
	//					break;				

					gNextTimeToSendStationaryEvent = TickCount() + kTimeBetweenStationaryMouseMovedEvents;
					
					mLastNullPoint = incomingEvent->where;

					//	Take care of the mouse moved event.
					
					if (targetPeer != NULL) {

						//	If the target is not a canvas, then don't send
						//	mouse events...  This is a really terrible
						//	inconsistency in AWT.
						
						if (StillDown()) {
							MToolkitExecutJavaDynamicMethod(targetPeer, "handleMouseMoved", 
								"(IIIZ)V", translatedPoint.h, translatedPoint.v, 
								incomingEvent->modifiers, true);
						} else
							MToolkitExecutJavaDynamicMethod(targetPeer, "handleMouseMoved", 
								"(IIIZ)V", translatedPoint.h, translatedPoint.v, 
								incomingEvent->modifiers, false);					
						executeInHost = false;

					}
					
					//	And the exit/enter events as well.
					
					if (targetPeer != gCurrentMouseTargetPeer) {
						
						if (gCurrentMouseTargetPeer != NULL)
							MToolkitExecutJavaDynamicMethod(gCurrentMouseTargetPeer, "handleMouseExit", 
								"(III)V", translatedPoint.h, translatedPoint.v, incomingEvent->modifiers);
					
						if (targetPeer != NULL)
							MToolkitExecutJavaDynamicMethod(targetPeer, "handleMouseEnter", 
								"(III)V", translatedPoint.h, translatedPoint.v, incomingEvent->modifiers);
					
						gCurrentMouseTargetPeer = targetPeer;
					
					}

				}
				
				break;
			
			case mouseDown:

				targetPeer = LocatePoint(translatedPoint);
				LPane		*targetPane = PeerToPane(targetPeer);

				if (targetPeer != NULL) {

					//	If the target is not a canvas, then don't send
					//	mouse events...  This is a really terrible
					//	inconsistency in AWT.
					
					if (targetPane->GetPaneID() == MCanvasPeer::class_ID) {
						LCommander::SwitchTarget((MCanvasPeer *)targetPane);
					} else
						break;
						
					if (targetPeer != gCurrentJavaTarget)
						gTotalClickCount = 0;
						
					MToolkitExecutJavaDynamicMethod(targetPeer, "handleMouseChanged", 
						"(IIIIZI)V", translatedPoint.h, translatedPoint.v, 
						incomingEvent->modifiers, ++gTotalClickCount, true, gEventRecordHistoryNextItem);
					executeInHost = false;

				}
				
				eventToRemember = true;

				break;
			
			case mouseUp:
				targetPeer = LocatePoint(translatedPoint);
				if (targetPeer != NULL) {
					MToolkitExecutJavaDynamicMethod(targetPeer, "handleMouseChanged", 
						"(IIIIZI)V", translatedPoint.h, translatedPoint.v, 
						incomingEvent->modifiers, 0, false, gEventRecordHistoryNextItem);
					executeInHost = false;
				}

				eventToRemember = true;

				break;
			
			case keyDown:
			case autoKey:
				//	If there is a modal dialog up, then don't
				//	grab key strokes... they are meant for the dialog.
				
				if (UDesktop::FrontWindowIsModal())
					break;

				if ((GetJavaTarget() != NULL) && ((incomingEvent->modifiers & cmdKey) == 0)) {
					Int16		theKey = incomingEvent->message & charCodeMask;
					if (theKey == '\r')
						theKey = '\n';
					MToolkitExecutJavaDynamicMethod(GetJavaTarget(), "handleKeyEvent", 
						"(IIIIZI)V", incomingEvent->where.h, incomingEvent->where.v, theKey, 
						incomingEvent->modifiers, true, gEventRecordHistoryNextItem);
					executeInHost = false;
				}

				eventToRemember = true;

				break;
			
			case keyUp:
				if (GetJavaTarget() != NULL) {
					Int16		theKey = incomingEvent->message & charCodeMask;
					MToolkitExecutJavaDynamicMethod(GetJavaTarget(), "handleKeyEvent", 
						"(IIIIZI)V", incomingEvent->where.h, incomingEvent->where.v, theKey, 
						incomingEvent->modifiers, false, gEventRecordHistoryNextItem);
					executeInHost = false;
				} 

				eventToRemember = true;

				break;
				
			default:
				break;

		}

	}


	if (eventToRemember) {
		gEventRecordHistoryNextItem++;
		if (gEventRecordHistoryNextItem >= kEventHistoryLength)
			gEventRecordHistoryNextItem = 0;
	}
	
	PR_SetThreadPriority(PR_CurrentThread(), 0);

	mExecuteHost = executeInHost;

}

MToolkit		*gMToolkit = NULL;

MToolkit *
MToolkit::GetMToolkit()
{
	//	If there is no toolkit already, then call java AWT to 
	//	fetch the Toolkit, which will force our MToolkit to 
	//	initialize
	
	if (gMToolkit == NULL) {
		
		ClassClass		*toolkitClass;
	
		toolkitClass = FindClass(EE(), "java/awt/Toolkit", TRUE);
		
		if (toolkitClass != NULL)
			MToolkitExecutJavaStaticMethod(toolkitClass, "getDefaultToolkit", "()Ljava/awt/Toolkit;");
	
	}

	return gMToolkit;
}


void sun_awt_macos_MToolkit_init(struct Hsun_awt_macos_MToolkit *toolkit)
{
	gMToolkit = new MToolkit();
	if (gMToolkit == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
	(CFrontApp::GetApplication())->AddAttachment(gMToolkit);
	URegistrar::RegisterClass(MTextAreaPeer::class_ID, (ClassCreatorFunc)MTextAreaPeer::CreateMTextAreaPeerStream);
	URegistrar::RegisterClass(MTextEditPeer::class_ID, (ClassCreatorFunc)MTextEditPeer::CreateMTextEditPeerStream);
}

Hjava_awt_image_ColorModel 	*gStandard8BitColorModel = NULL;

struct Hjava_awt_image_ColorModel *sun_awt_macos_MToolkit_makeColorModel(struct Hsun_awt_macos_MToolkit *toolkit)
{
	struct Hjava_awt_image_ColorModel 			*awt_colormodel;
	CTabHandle									defaultColorTable;
	ColorSpec									*currentColorSpec;
	
    defaultColorTable = (CTabHandle)GetResource('clut', kDefaultColorTableResID);
    if (defaultColorTable == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return NULL;
    }
    
	//	Get the system 8-bit color lookup table.  This will be the 
	//	the colors we use to create the default color model.  
	//	By making the arrays volatile, we ensure that the GC won’t
	//	get rid of our arrays.
	
	volatile HArrayOfByte 	*redComponent 		= (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	volatile HArrayOfByte	*greenComponent 	= (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	volatile HArrayOfByte 	*blueComponent 		= (HArrayOfByte *) ArrayAlloc(T_BYTE, 256);
	char					*currentRed 		= unhand(redComponent)->body;
	char					*currentBlue 		= unhand(blueComponent)->body;
	char					*currentGreen 		= unhand(greenComponent)->body;
    
	if ((redComponent == NULL) || (greenComponent == NULL) || (blueComponent == NULL)) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return NULL;
	}
	
	//	Fill in the red, green and blue component arrays.

	currentColorSpec = (**defaultColorTable).ctTable;
	
	for (UInt32 currentItem = 0; currentItem < 256; currentItem++) {
	
		*currentRed++ = currentColorSpec->rgb.red >> 8;
		*currentBlue++ = currentColorSpec->rgb.blue >> 8;
		*currentGreen++ = currentColorSpec->rgb.green >> 8;
	
	}
	
	//	We are finished with the color table.  Reselase it.

	ReleaseResource((Handle)defaultColorTable);
	
	//	Execute the constructor for the color model object.
	
	awt_colormodel = (Hjava_awt_image_ColorModel *)execute_java_constructor(EE(), "java/awt/image/IndexColorModel", 0, 
			"(II[B[B[B)", 8, 256, redComponent, greenComponent, greenComponent);

	gStandard8BitColorModel = awt_colormodel;

    return awt_colormodel;

}

long sun_awt_macos_MToolkit_getScreenResolution(struct Hsun_awt_macos_MToolkit *toolkit)
{

	//	Even though multi-sync monitors make this untrue, for now 
	//	return the classic Macintosh 72 dpi.

	return 72;
	
}

long sun_awt_macos_MToolkit_getScreenWidth(struct Hsun_awt_macos_MToolkit *toolkit)
{
	GetScreenResolution();
	return gMainScreenBounds.right - gMainScreenBounds.left;	
}

long sun_awt_macos_MToolkit_getScreenHeight(struct Hsun_awt_macos_MToolkit *toolkit)
{
	GetScreenResolution();
	return gMainScreenBounds.bottom - gMainScreenBounds.top;
}

void sun_awt_macos_MToolkit_sync(struct Hsun_awt_macos_MToolkit *toolkit)
{
	//	On the macintosh, this does nothing (up to now…)
}


extern "C" {
#include "native.h"
#include "sun_awt_macos_MComponentPeer.h"
};

#pragma once

#include <LAttachment.h>

enum {
	msg_ButtonPressed	= 'btpr'
};

class MToolkit: public LAttachment {
	
	public:
	
										MToolkit();
		virtual void					ExecuteSelf(MessageT inMessage, void *ioParam);

		Hsun_awt_macos_MComponentPeer 	*LocatePoint(Point &incomingPoint);
		Hsun_awt_macos_MComponentPeer	*LocatePointInFrame(LPane *frame, Point &incomingPoint);

		void							AddFrame(LPane *frame);
		void							RemoveFrame(LPane *frame);

		static MToolkit					*GetMToolkit();

	private:
	
		Point						 	mLastNullPoint;
	
		LList							*mFrameList;
		

};

UInt32 MToolkitExecutJavaDynamicMethod(void *obj, char *method_name, char *signature, ...);
UInt32 MToolkitExecutJavaStaticMethod(ClassClass *classObject, char *method_name, char *signature, ...);

extern EventRecord		gLastEventRecordCopy;

LWindow *GetContainerWindow(LPane *pane);

extern long gTotalClickCount;
extern Hsun_awt_macos_MComponentPeer *gCurrentJavaTarget;
extern Hsun_awt_macos_MComponentPeer *gCurrentMouseTargetPeer;

#pragma once

#include <Quickdraw.h>
#include <LPane.h>
#include "sun_awt_macos_MComponentPeer.h"

Hsun_awt_macos_MComponentPeer *GetComponentParent(Hsun_awt_macos_MComponentPeer *componentPeerHandle);
Hsun_awt_macos_MComponentPeer *PaneToPeer(const LPane *pane);
LPane *PeerToPane(Hsun_awt_macos_MComponentPeer *componentPeerHandle);
RgnHandle GetComponentRegion(Hsun_awt_macos_MComponentPeer *componentPeerHandle);

void RemoveAllSubpanes(LView *containerPane);
void CalculateComponentClipRgn(Hsun_awt_macos_MComponentPeer *componentPeerHandle);
void PreparePeerGrafPort(Hsun_awt_macos_MComponentPeer *componentPeerHandle, ControlHandle peerControlHandle);
void InvalidateParentClip(Hsun_awt_macos_MComponentPeer *componentPeerHandle);

void SetJavaTarget(Hsun_awt_macos_MComponentPeer *currentTarget);
Hsun_awt_macos_MComponentPeer *GetJavaTarget(void);

void NewLinesToReturns(char *textPtr, long textSize);
void ReturnsToNewLines(char *textPtr, long textSize);

void OverlapBindToTopLevelFrame(Hsun_awt_macos_MComponentPeer *componentPeerHandle, Hsun_awt_macos_MComponentPeer *parentPeerHandle, long *x, long *y, long *width, long *height);

extern Boolean 	gInPaneFocusDraw;

void InvalidateComponent(LPane *componentPane, Boolean hasFrame);
void DisposeJavaDataForPane(LPane *deadPane);

RGBColor SwitchInControlColors(LStdControl *theControl);
void SwitchOutControlColors(LStdControl *theControl, RGBColor savedColor);
Boolean ShouldRefocusAWTPane(LPane *newFocusPane);
void ClearCachedAWTRefocusPane(void);

#ifdef UNICODE_FONTLIST
short intl_GetWindowEncoding(Hsun_awt_macos_MComponentPeer *componentPeerHandle);
short intl_GetSystemUIEncoding();
short intl_GetDefaultEncoding();

void intl_javaString2CString(int , Hjava_lang_String *, unsigned char *, int);
int intl_javaString2CEncodingString(Hjava_lang_String *, unsigned char *, int);
char * intl_allocCString(int  , Hjava_lang_String *);
void intl_javaString2UnicodePStr(Hjava_lang_String *, Str255);

extern "C" {
Hjava_lang_String *intl_makeJavaString(int16 , char *str, int len);
};
#endif

enum {
	kJavaComponentPeerHasBeenDisposed 	= -1
};

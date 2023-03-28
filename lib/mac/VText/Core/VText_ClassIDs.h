//	===========================================================================
//	VText_ClassIDs
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

//	This file is processable by rez and C/C++ compilers
#ifdef	rez
#else
	#pragma	once
#endif

#ifdef	__VText_ClassIDs__
#else

	#define	__VText_ClassIDs__

	#include	<PP_ClassIDs_DM.h>
	
	//	VText class IDs:
	#define	kVTCID_VStyleSet 			'VCss'
	#define	kVTCID_VRulerSet 			'VCrs'
	#define	kVTCID_VTextEngine			'VCte'
	#define	kVTCID_VTextView			'VCtv'
	#define	kVTCID_VOTextModel			'VCto'
	#define kVTCID_VTextHandler			'VCth'
	
	#define	kVTCID_VSimpleTextView		'VCsv'
	#define	kVTCID_VEditField			'VCef'

	#define	kVTCID_VFontMenuAttachment	'VCfa'
	#define	kVTCID_VSizeMenuAttachment	'VCsz'
	#define	kVTCID_VStyleMenuAttachment	'VCsy'
#endif

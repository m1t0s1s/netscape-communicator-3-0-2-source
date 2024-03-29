//	===========================================================================
//	UCursor.h						�1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	Values for cursors for CursorSet:
/*
		Note these correspond to cursor resources.  If the
		indicated resource doesn't exist CursorUtils will make
		an approximation with an existing or system cursor.
*/

#pragma	once

#include <PP_Prefix.h>

enum {
	cu_Base			= 0x400,	//	1024
	cu_Mask			= 0xfff0,	//	Mask to find family of a given cursor.
	
	cu_Default		= cu_Base,
	cu_Arrow		= cu_Default,
	cu_IBeam		= cu_Base+0x10,
	cu_Cross		= cu_Base+0x20,
	cu_Plus			= cu_Base+0x30,
	cu_Watch		= cu_Base+0x40,
	cu_Hand			= cu_Base+0x50,
	cu_HSlide		= cu_Base+0x60,
	cu_VSlide		= cu_Base+0x70,
	cu_EmptyArrow	= cu_Base+0x80,
	cu_HVSlide		= cu_Base+0x90
};

class UCursor {
public:
	static	void	Reset(void);
	static	void	Set(Int16 inID);
	static	Int16	Get(void);
	static	void	Tick(Int16 inID = 0);
	static	void	Restore(void);

protected:
	static	Int16		sCurrentCursorID;
	static	CursHandle	sBWCursor;
	static	CCrsrHandle	sColorCursor;
};

class StCursor {
public:
					StCursor(Int16 inID = cu_Watch);
					~StCursor();
					
	virtual void	Tick(Int16 inID = 0);

protected:
	Int16			mOldCursorID;
	Int16			mFamilyID;
};
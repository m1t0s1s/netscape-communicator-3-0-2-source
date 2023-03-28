//	===========================================================================
//	WRegistry
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include <PP_Prefix.h>

class	LStream;
class	LWindow;
class	LCommander;
class	LModelObject;
class	LView;

/*
	It would make more sense if URegistrar declared
	
		WObjectFactory,
		WStreamedObjectFactory, and
		WStreamableFactory
*/

template <class T>
class WObjectFactory
{
public:
	static	T*	Create(void) {return new T;}
};

template <class T>
class WStreamedObjectFactory
{
public:
	static	T*	Create(LStream *inStream) {return new T(inStream);}
};

template <class T>
class WStreamableFactory
{
public:
	static	LStreamable*	Create(void) {return new T;}
};

class WRegistry
{
public:
	static	void		RegisterDMClasses(void);
	static	void		RegisterTextClasses(void);
	static	void		RegisterTableClasses(void);
	
					//	This belongs in LWindow?
#ifndef	PP_No_ObjectStreaming
	static	LWindow *	ReadVCMWindow(
							ResIDT			inResID,
							LCommander		*inSuperCommander,
							LModelObject	*inSuperModel);
#endif
};

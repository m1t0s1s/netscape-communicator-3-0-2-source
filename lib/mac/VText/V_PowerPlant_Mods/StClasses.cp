//	===========================================================================
//	StClasses.cp					©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"StClasses.h"

//	===========================================================================
//	StChange
//
//	Useful explicit StChange template instantiations...

#include	"StClasses.t"

template class	StChange<Boolean>;
template class	StChange<EDebugAction>;
template class	StChange<Int32>;

//	===========================================================================

StPenMove::StPenMove(
	Int16	inXDelta,
	Int16	inYDelta)
{
	mXDelta = inXDelta;
	mYDelta = inYDelta;
	
	::Move(mXDelta, mYDelta);
}

StPenMove::~StPenMove()
{
	Move(-mXDelta, -mYDelta);
}

StIndResource::StIndResource()
{
	mResourceH = NULL;
}

StIndResource::StIndResource(
	ResType	inType,
	ResIDT	inIndex,
	Boolean	inThrow,
	Boolean	in1Deep)
{
	if (in1Deep)
		mResourceH = Get1IndResource(inType, inIndex);
	else
		mResourceH = GetIndResource(inType, inIndex);

	if (inThrow)
		ThrowIfResFail_(mResourceH);
}

StIndResource::~StIndResource()
{
	if (mResourceH)
		ReleaseResource(mResourceH);
}


//	===========================================================================
//	StProfile

#undef	DO_IT

#if	defined(__MWERKS__) && defined(__profile__)
	#if	__profile__
		#define	DO_IT
	#endif
#endif

#ifdef	DO_IT
	#include	<LDebugApp.h>

	StProfile::StProfile()
	{
		LDebugApp::SetProfiler(true);
	}
	
	StProfile::~StProfile()
	{
		LDebugApp::SetProfiler(false);
	}
#else
	StProfile::StProfile() {}
	StProfile::~StProfile() {}
#endif


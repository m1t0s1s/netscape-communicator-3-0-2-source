//	===========================================================================
//	VTabAEOM.cp
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#include	"VTabAEOM.h"
#include	<AEVTextSuite.h>
#include	<LRulerSet.h>
#include	<LStream.h>
#include	<UAppleEventsMgr.h>

#pragma	warn_unusedarg off

VTabAEOM::VTabAEOM(
	LModelObject	*inSuperModel,
	LRulerSet		*inRuler,
	TabRecord		&inTabRecord
)
:	LModelObject(inSuperModel, cRulerTab)
,	mTabRecord(inTabRecord)
,	mRuler(inRuler)
{
	SetLaziness(true);
}

TabRecord &	VTabAEOM::GetTabRecord(void) const
{
	return mTabRecord;
}

void	VTabAEOM::HandleDelete(
	AppleEvent&		outAEReply,
	AEDesc&			outResult)
{
	if (mRuler)
		mRuler->DeleteTab(&mTabRecord);
}

void	VTabAEOM::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	OSErr	err = noErr;
	
	switch (inProperty) {

		case pVTextTabOrientation:
			err = AECreateDesc(typeEnumeration, &mTabRecord.orientation, sizeof(mTabRecord.orientation), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
			
		case pLocation:
		{
			Fixed	fixed = mTabRecord.location.ToFixed();
			err = AECreateDesc(typeFixed, &fixed, sizeof(fixed), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}

		case pRepeating:
		{
			Fixed	fixed = mTabRecord.repeat.ToFixed();
			err = AECreateDesc(typeFixed, &fixed, sizeof(fixed), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}

		case pDelimiter:
		{
			Byte		*data = (Byte *)&mTabRecord.delimiter;
			LAEStream	stream;
			stream.OpenDesc(typeChar);
			if (*data)
				stream.WriteData(data, 1);
			stream.WriteData(data +1, 1);
			stream.CloseDesc();
			stream.Close(&outPropertyDesc);
			break;
		}
		
		default:
			inherited::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	VTabAEOM::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc			&outAEReply)
{
	LAESubDesc	value(inValue);
	
	switch (inProperty) {

		case pVTextTabOrientation:
			mTabRecord.orientation = value.ToEnum();
			if (mRuler)
				mRuler->Changed();
			break;
		
		
		case pLocation:
		{
			Fixed	fixed;
			value.ToPtr(typeFixed, &fixed, sizeof(fixed));
			mTabRecord.location.SetFromFixed(fixed);
			if (mRuler)
				mRuler->Changed();
			break;
		}

		case pRepeating:
		{
			Fixed	fixed;
			value.ToPtr(typeFixed, &fixed, sizeof(fixed));
			mTabRecord.repeat.SetFromFixed(fixed);
			if (mRuler)
				mRuler->Changed();
			break;
		}

		case pDelimiter:
		{
			Int16	newDelimiter;
			Size	length = value.GetDataLength();
			length = min(length, 2L);
			if (length) {
				value.ToPtr(typeChar, &newDelimiter, length);
				if (length == 1)
					newDelimiter = (newDelimiter >> 8) & 0x0ff;
				mTabRecord.delimiter = newDelimiter;
				if (mRuler)
					mRuler->Changed();
			}
			break;
		}
		
		default:
			inherited::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}



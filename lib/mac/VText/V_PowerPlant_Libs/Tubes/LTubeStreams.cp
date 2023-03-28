//	===========================================================================
//	LTubeStreams
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTubeStreams.h"
#include	<UAEGizmos.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	LAEStream write

LAEStreamWriteStream::LAEStreamWriteStream(LAEStream *inStream)
:	mAEStream(inStream)
{
}

LAEStreamWriteStream::~LAEStreamWriteStream()
{
}

ExceptionCode	LAEStreamWriteStream::GetBytes(
	void	*outBuffer,
	Int32	&ioByteCount)
{
	Throw_(paramErr);	//	this is a write stream!
	
	return paramErr;
}

ExceptionCode	LAEStreamWriteStream::PutBytes(
	const void	*inBuffer,
	Int32		&ioByteCount)
{
	ExceptionCode	err = noErr;
	
	try {
		ThrowIfNULL_(mAEStream);
		
		mAEStream->WriteData(inBuffer, ioByteCount);

		SetLength(GetLength() + ioByteCount);
		SetMarker(GetMarker() + ioByteCount, streamFrom_Start);
	} catch(ExceptionCode inErr) {
		err = inErr;
	} catch(...) {
		err = paramErr;
		Assert_(false);
	}

	return err;
}

//	===========================================================================
//	LAESubDesc read

LAESubDescReadStream::LAESubDescReadStream(const LAESubDesc &inSubDesc)
:	mSubDesc(inSubDesc)
{
	SetLength(mSubDesc.GetDataLength());
}

LAESubDescReadStream::~LAESubDescReadStream()
{
}

ExceptionCode	LAESubDescReadStream::GetBytes(
	void	*outBuffer,
	Int32	&ioByteCount)
{
	ExceptionCode	err;
	
	try {
		Byte	*source = (Byte *)mSubDesc.GetDataPtr() + GetMarker();
		Int32	dataRemaining = GetLength() - GetMarker(),
				dataGot = ioByteCount;
		
		if (dataGot > dataRemaining)
			dataGot = dataRemaining;
		if (dataGot < 0)
			dataGot = 0;
		
		if (dataGot)
			BlockMoveData(source, outBuffer, dataGot);
		
		SetMarker(GetMarker() + dataGot, streamFrom_Start);
		
		if (dataGot != ioByteCount) {
			ioByteCount = dataGot;
			return eofErr;
		} else {
			return noErr;
		}
	} catch(ExceptionCode inErr) {
		err = inErr;
	} catch(...) {
		err = paramErr;
		Assert_(false);
	}

	return err;
}

ExceptionCode	LAESubDescReadStream::PutBytes(
	const void	*inBuffer,
	Int32		&ioByteCount)
{
	Throw_(paramErr);	//	this is a read stream!
	
	return paramErr;
}

//	===========================================================================
//	Null write

LNullWriteStream::LNullWriteStream()
{
}

LNullWriteStream::~LNullWriteStream()
{
}

ExceptionCode	LNullWriteStream::GetBytes(
	void	*outBuffer,
	Int32	&ioByteCount)
{
	Throw_(paramErr);	//	this is a write stream!
	
	return paramErr;
}

ExceptionCode	LNullWriteStream::PutBytes(
	const void	*inBuffer,
	Int32		&ioByteCount)
{
	ExceptionCode	err = noErr;
	
	SetLength(GetLength() + ioByteCount);
	SetMarker(GetMarker() + ioByteCount, streamFrom_Start);

	return err;
}

//	===========================================================================
//	

/*-
LTubeFlavorStream::LTubeFlavorStream()
{
	mTube = NULL;
	mFlavor = typeNull;
	mForWrite = false;
}

LTubeFlavorStream::~LTubeFlavorStream()
{
}

const LDataTube *	LTubeFlavorStream::GetTube(void) const
{
	return mTube;
}

LDataTube * LTubeFlavorStream::GetTube(void)
{
	return mTube;
}

void	LTubeFlavorStream::SetTube(const LDataTube *inTube)
{
	mTube = (LDataTube *)inTube;
	mForWrite = false;
}

void	LTubeFlavorStream::SetTube(LDataTube *inTube)
{
	mTube = inTube;
	mForWrite = true;
}

FlavorType	LTubeFlavorStream::GetFlavor(void) const
{
	return mFlavor;
}

void	LTubeFlavorStream::SetFlavor(FlavorType inFlavor)
{
	mFlavor = inFlavor;
}

ExceptionCode	LTubeFlavorStream::PutBytes(
	const void	*inBuffer,
	Int32		&ioByteCount)
{
	ExceptionCode	err = noErr;
	
	ThrowIfNULL_(mTube);
	ThrowIf_(!mForWrite);
	
	try {
		mTube->SetFlavorData(mFlavor, inBuffer, GetLength() + ioByteCount, GetMarker(), ioByteCount);
		SetLength(GetLength() + ioByteCount);
		SetMarker(GetLength());
	} catch (ExceptionCode inErr) {
		ioByteCount = 0;
		err = inErr;
	} catch (...) {
		ioByteCount = 0;
		err = writErr;
	}
	
	return err;
}

ExceptionCode	LTubeFlavorStream::GetBytes(
	void	*outBuffer,
	Int32	&ioByteCount)
{
	ExceptionCode	err = noErr;
	
	ThrowIfNULL_(mTube);
	
	try {
		mTube->GetFlavorData(mFlavor, outBuffer, GetMarker(), ioByteCount);
		SetMarker(GetMarker() + ioByteCount);
	} catch (ExceptionCode inErr) {
		ioByteCount = 0;
		err = inErr;
	} catch (...) {
		ioByteCount = 0;
		err = readErr;
	}
	
	return err;
}
*/
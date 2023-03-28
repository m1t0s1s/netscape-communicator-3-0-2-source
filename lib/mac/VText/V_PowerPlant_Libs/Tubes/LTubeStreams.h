//	===========================================================================
//	LTubeStreams
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#include	<LDataTube.h>
#include	<LStream.h>
#include	<UAEGizmos.h>

class	LAEStream;

class	LAEStreamWriteStream
			:	public LStream
{
public:
							LAEStreamWriteStream(LAEStream *inStream);
							~LAEStreamWriteStream();

	virtual	ExceptionCode	GetBytes(
								void	*outBuffer,
								Int32	&ioByteCount);
	virtual ExceptionCode	PutBytes(
								const void	*inBuffer,
								Int32		&ioByteCount);

protected:
	LAEStream				*mAEStream;
};
							
class	LAESubDescReadStream
			:	public LStream
{
public:
							LAESubDescReadStream(const LAESubDesc &inSubDesc);
							~LAESubDescReadStream();

	virtual	ExceptionCode	GetBytes(
								void	*outBuffer,
								Int32	&ioByteCount);
	virtual ExceptionCode	PutBytes(
								const void	*inBuffer,
								Int32		&ioByteCount);

protected:
	LAESubDesc				mSubDesc;
};

class	LNullWriteStream
			:	public LStream
{
public:
							LNullWriteStream();
							~LNullWriteStream();

	virtual	ExceptionCode	GetBytes(
								void	*outBuffer,
								Int32	&ioByteCount);
	virtual ExceptionCode	PutBytes(
								const void	*inBuffer,
								Int32		&ioByteCount);
};
	
/*-							
class LTubeFlavorStream
		:	public LStream
{
public:
					
							LTubeFlavorStream();
	virtual					~LTubeFlavorStream();

	const LDataTube *		GetTube(void) const;
	LDataTube *				GetTube(void);
	void					SetTube(const LDataTube *inTube);
	void					SetTube(LDataTube *inTube);
	FlavorType				GetFlavor(void) const;
	void					SetFlavor(FlavorType inFlavor);
	
	virtual	ExceptionCode	GetBytes(
								void	*outBuffer,
								Int32	&ioByteCount);
	virtual ExceptionCode	PutBytes(
								const void	*inBuffer,
								Int32		&ioByteCount);

protected:
	LDataTube				*mTube;
	FlavorType				mFlavor;
	Boolean					mForWrite;
};
*/
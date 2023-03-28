// BestFitHeap.h 
// Copyright й 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef __BESTFITHEAP__
#define __BESTFITHEAP__

#ifndef __MEMORYHEAP__
#include "MemoryHeap.h" // CHANGE <>
#endif

#ifdef DEBUG
	extern void AssertPointer (const void * p, char *condition);
#	define ASSERT_POINTER(X)	AssertPointer(X,#X)
#else
#	define ASSERT_POINTER(X)
#endif

//========================================================================================
// Forward class declarations
//========================================================================================

class BestFitBlock;
class BestFitHeap;


//========================================================================================
// STRUCT BestFitBlockFreeListLinks
//
// The following fields are only present in free blocks. They are used to link the free
// block into the free block tree. The address of a free block is also stored at the end
// of the block and must be accounted for in the minimum block size.
//========================================================================================

struct BestFitBlockFreeListLinks
{
#ifdef DEBUG
	inline BestFitBlockFreeListLinks ();
#endif
	BestFitBlock* fParent;
	BestFitBlock* fLeft;
	BestFitBlock* fRight;
};

#ifdef DEBUG

BestFitBlockFreeListLinks::BestFitBlockFreeListLinks ()
{
	fParent = fLeft = fRight = nil;
}

#endif

//========================================================================================
// STRUCT BestFitBlockHeader
//========================================================================================

// The fBits field of BestFitBlockHeader contains five fields. The following masks are
// used to get and set the fields. The block type field is used to distinguish best fit
// blocks from chunky blocks and must be the first four bits of the last byte in fBits.

#ifdef BUILD_WIN
// Bytes are in reverse order in a long.

const unsigned long BestFitBlockHeader_kSizeMask = 0x00FFFFFF;
const unsigned long BestFitBlockHeader_kSizeShift = 0;

const unsigned long BestFitBlockHeader_kBlockTypeMask = 0xF0000000;
const unsigned long BestFitBlockHeader_kBlockTypeShift = 28;

const unsigned long BestFitBlockHeader_kBusyMask = 0x08000000;
const unsigned long BestFitBlockHeader_kBusyShift = 27;

const unsigned long BestFitBlockHeader_kPreviousBusyMask = 0x04000000;
const unsigned long BestFitBlockHeader_kPreviousBusyShift = 26;

const unsigned long BestFitBlockHeader_kMagicNumberMask = 0x03000000;
const unsigned long BestFitBlockHeader_kMagicNumberShift = 24;
#else
const unsigned long BestFitBlockHeader_kSizeMask = 0xFFFFFF00;
const unsigned long BestFitBlockHeader_kSizeShift = 8;

const unsigned long BestFitBlockHeader_kBlockTypeMask = 0x000000F0;
const unsigned long BestFitBlockHeader_kBlockTypeShift = 4;

const unsigned long BestFitBlockHeader_kBusyMask = 0x00000008;
const unsigned long BestFitBlockHeader_kBusyShift = 3;

const unsigned long BestFitBlockHeader_kPreviousBusyMask = 0x00000004;
const unsigned long BestFitBlockHeader_kPreviousBusyShift = 2;

const unsigned long BestFitBlockHeader_kMagicNumberMask = 0x00000003;
const unsigned long BestFitBlockHeader_kMagicNumberShift = 0;
#endif

struct BestFitBlockHeader
{
	unsigned long fBits;
	BestFitBlockFreeListLinks fFreeLinks;
};


//========================================================================================
// CLASS BestFitBlock
//========================================================================================

// еее JS NEW еее MW
#define SIZE_T size_t

class BestFitBlock
{
public:
	static const FW_BlockSize kMaxBlockSize;
	
	enum
	{
		kBusyOverhead = sizeof(unsigned long), 
		kMinBlockSize = sizeof(BestFitBlockHeader) + sizeof(void *), 
		kBlockTypeId = MemoryHeap::kBlockTypeId + 1, 
		kMagicNumber = 0x3
	};


	Boolean operator>(const BestFitBlock& blk) const;
	Boolean operator<(const BestFitBlock& blk) const;
	Boolean operator>=(const BestFitBlock& blk) const;
	Boolean operator<=(const BestFitBlock& blk) const;
	Boolean operator==(const BestFitBlock& blk) const;
	Boolean operator!=(const BestFitBlock& blk) const;
	BestFitBlock& operator=(const BestFitBlock& blk);

	// еее JS NEW еее MW
	inline void operator delete(void *) { }
	void* operator new(SIZE_T, void* ptr);
	void* operator new(SIZE_T);

	BestFitBlock(short busy,
					 short prevBusy,
					 long size);
	BestFitBlock(const BestFitBlock& otherBlock);

	Boolean GetBusy() const;
	BestFitBlock* GetLeft() const;
	unsigned short GetMagicNumber() const;
	BestFitBlock* GetNext() const;
	BestFitBlock* GetParent() const;
	Boolean GetPreviousBusy() const;
	BestFitBlock* GetRight() const;
	FW_BlockSize GetSize() const;
	unsigned short GetBlockType() const;

	void SetBusy(Boolean busy);
	void SetLeft(BestFitBlock* left);
	void SetNext(BestFitBlock* fNext);
	void SetParent(BestFitBlock* parent);
	void SetPrevBusy(Boolean busy);
	void SetRight(BestFitBlock* right);
	void SetSize(FW_BlockSize size);
	void SetBlockType(unsigned long blockType);
	void SetMagicNumber(unsigned long magicNumber);

	void StuffAddressAtEnd();

protected:

private:
	BestFitBlockHeader fHeader;
};

//----------------------------------------------------------------------------------------
// BestFitBlock::operator delete
//----------------------------------------------------------------------------------------
/*
inline void BestFitBlock::operator delete(void*)
{
}
*/
//----------------------------------------------------------------------------------------
// BestFitBlock::operator new
//----------------------------------------------------------------------------------------

inline void* BestFitBlock::operator new(SIZE_T, void* ptr)
{
	return ptr;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator new
//----------------------------------------------------------------------------------------

inline void* BestFitBlock::operator new(SIZE_T)
{
	return NULL;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator>=
//----------------------------------------------------------------------------------------

inline Boolean BestFitBlock::operator>=(const BestFitBlock& blk) const
{
	return *this > blk || *this == blk;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator<=
//----------------------------------------------------------------------------------------

inline Boolean BestFitBlock::operator<=(const BestFitBlock& blk) const
{
	return *this < blk || *this == blk;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator!=
//----------------------------------------------------------------------------------------

inline Boolean BestFitBlock::operator!=(const BestFitBlock& blk) const
{
	return !(*this == blk);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator=
//----------------------------------------------------------------------------------------

inline BestFitBlock& BestFitBlock::operator=(const BestFitBlock& blk)
{
	fHeader = blk.fHeader;
	return (*this);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::BestFitBlock
//----------------------------------------------------------------------------------------

inline BestFitBlock::BestFitBlock(const BestFitBlock& blk) :
	fHeader(blk.fHeader)
{
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetBusy
//----------------------------------------------------------------------------------------

inline Boolean BestFitBlock::GetBusy() const
{
	return (fHeader.fBits & BestFitBlockHeader_kBusyMask) != 0 ? true : false;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetLeft
//----------------------------------------------------------------------------------------

inline BestFitBlock* BestFitBlock::GetLeft() const
{
	ASSERT_POINTER (fHeader.fFreeLinks.fLeft);
	return fHeader.fFreeLinks.fLeft;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetMagicNumber
//----------------------------------------------------------------------------------------

inline unsigned short BestFitBlock::GetMagicNumber() const
{
	return (fHeader.fBits & BestFitBlockHeader_kMagicNumberMask)
				>> BestFitBlockHeader_kMagicNumberShift;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetNext
//----------------------------------------------------------------------------------------

inline BestFitBlock* BestFitBlock::GetNext() const
{
	ASSERT_POINTER (fHeader.fFreeLinks.fParent);
	return fHeader.fFreeLinks.fParent;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetParent
//----------------------------------------------------------------------------------------

inline BestFitBlock* BestFitBlock::GetParent() const
{
	ASSERT_POINTER (fHeader.fFreeLinks.fParent);
	return fHeader.fFreeLinks.fParent;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetPreviousBusy
//----------------------------------------------------------------------------------------

inline Boolean BestFitBlock::GetPreviousBusy() const
{
	return (fHeader.fBits & BestFitBlockHeader_kPreviousBusyMask) != 0 ? true : false;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetRight
//----------------------------------------------------------------------------------------

inline BestFitBlock* BestFitBlock::GetRight() const
{
	ASSERT_POINTER (fHeader.fFreeLinks.fRight);
	return fHeader.fFreeLinks.fRight;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetSize
//----------------------------------------------------------------------------------------

inline FW_BlockSize BestFitBlock::GetSize() const
{
	return (fHeader.fBits & BestFitBlockHeader_kSizeMask)
				>> BestFitBlockHeader_kSizeShift;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::GetBlockType
//----------------------------------------------------------------------------------------

inline unsigned short BestFitBlock::GetBlockType() const
{
	return (fHeader.fBits & BestFitBlockHeader_kBlockTypeMask)
				>> BestFitBlockHeader_kBlockTypeShift;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetBusy
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetBusy(Boolean busy)
{
	if (busy)
		fHeader.fBits |= BestFitBlockHeader_kBusyMask;
	else
		fHeader.fBits &= ~BestFitBlockHeader_kBusyMask;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetLeft
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetLeft(BestFitBlock* left)
{
	fHeader.fFreeLinks.fLeft = left;
	ASSERT_POINTER (fHeader.fFreeLinks.fLeft);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetNext
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetNext(BestFitBlock* fNext)
{
	// The fParent field is used both for a parent and a next pointer on different
	// occasions.
	
	fHeader.fFreeLinks.fParent = fNext;
	ASSERT_POINTER (fHeader.fFreeLinks.fParent);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetParent
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetParent(BestFitBlock* parent)
{
	fHeader.fFreeLinks.fParent = parent;
	ASSERT_POINTER (fHeader.fFreeLinks.fParent);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetPrevBusy
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetPrevBusy(Boolean busy)
{
	if (busy)
		fHeader.fBits |= BestFitBlockHeader_kPreviousBusyMask;
	else
		fHeader.fBits &= ~BestFitBlockHeader_kPreviousBusyMask;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetRight
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetRight(BestFitBlock* right)
{
	fHeader.fFreeLinks.fRight = right;
	ASSERT_POINTER (fHeader.fFreeLinks.fRight);
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetSize
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetSize(FW_BlockSize size)
{
	fHeader.fBits &= ~BestFitBlockHeader_kSizeMask;
	fHeader.fBits |= (size << BestFitBlockHeader_kSizeShift)
						& BestFitBlockHeader_kSizeMask;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetBlockType
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetBlockType(unsigned long blockType)
{
	fHeader.fBits &= ~BestFitBlockHeader_kBlockTypeMask;
	fHeader.fBits |= (blockType << BestFitBlockHeader_kBlockTypeShift)
						& BestFitBlockHeader_kBlockTypeMask;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::SetMagicNumber
//----------------------------------------------------------------------------------------

inline void BestFitBlock::SetMagicNumber(unsigned long magicNumber)
{
	fHeader.fBits &= ~BestFitBlockHeader_kMagicNumberMask;
	fHeader.fBits |= (magicNumber << BestFitBlockHeader_kMagicNumberShift)
						& BestFitBlockHeader_kMagicNumberMask;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::BestFitBlock
//----------------------------------------------------------------------------------------

inline BestFitBlock::BestFitBlock(short busy,
										  short previousBusy,
										  long size)
{
	SetParent(NULL);
	SetRight(NULL);
	SetLeft(NULL);
	SetBlockType(kBlockTypeId);
	SetBusy(busy);
	SetPrevBusy(previousBusy);
	SetSize(size);
	SetMagicNumber(kMagicNumber);

	if (!busy)
		this->StuffAddressAtEnd();
}


//========================================================================================
// CLASS BestFitSegment
//
//		The BestFitHeap allocates memory from the system in segments. The segments are
//		linked together in a list so that When the heap is destroyed all segments can be
//		freed.
//
//========================================================================================

class BestFitSegment
{
public:

	friend BestFitHeap;

	enum
	{
		kSegmentPrefixSize = 12,
		kSegmentSuffixSize = 4,
		kSegmentOverhead = kSegmentPrefixSize + kSegmentSuffixSize
	};

	Boolean AddressInSegment(void* ptr);

private:
	void *fSegmentSpace;
	unsigned long fSegmentSize;
	BestFitSegment *fNextSegment;
	
	BestFitSegment(const BestFitSegment& blk);
	BestFitSegment& operator=(const BestFitSegment& blk);
		// This class shouldn't be copied.
};

//----------------------------------------------------------------------------------------
// INLINES BestFitSegment
//----------------------------------------------------------------------------------------

inline Boolean BestFitSegment::AddressInSegment(void* ptr)
{
	return ptr >= fSegmentSpace &&
			ptr <= (void*) ((FW_BytePtr) fSegmentSpace + fSegmentSize);
}


//========================================================================================
// CLASS FreeBlockTree
//
//		Binary tree for storing free blocks. Dependent on the structure and
//		implementation of BestFitBlock.
//
//========================================================================================

class FreeBlockTree
{
public:
	FreeBlockTree();
	FreeBlockTree(const FreeBlockTree& blk);

	FreeBlockTree& operator=(const FreeBlockTree& blk);

	void AddBlock(BestFitBlock* blk);
	void TreeInfo(unsigned long& bytesFree,
				  unsigned long& numberOfNodes) const;
	void RemoveBlock(BestFitBlock* blk);
	BestFitBlock* SearchForBlock(FW_BlockSize size,
									 void* blk,
									 BestFitBlock** insertLeaf = NULL);

#ifdef DEBUG
	void CheckTree() const;
	void PrintTree() const;
#endif

protected:
	BestFitBlock* GetSuccessorBlk(BestFitBlock* blk);
	int TreeInfoHelper(BestFitBlock* blk,
						unsigned long& bytesFree,
						unsigned long& numberOfNodes) const;

#ifdef DEBUG
	void CheckTreeHelper(BestFitBlock* blk) const;
	void PrintTreeHelper(BestFitBlock* blk,
						 short level = 0) const;
#endif

private:
	BestFitBlock fRoot;

};


//========================================================================================
// CLASS BestFitHeap
//
//		Memory allocation heap using the best fit allocation strategy.
//
//========================================================================================

class BestFitHeap: public MemoryHeap
{
public:
	enum FreeBlockDisposal { kDisposeFreeBlockSegments, kIsAboutToBeAllocated}; // MARK CHANGE
	virtual unsigned long	BytesFree() const;
	virtual unsigned long	HeapSize() const;

							BestFitHeap( unsigned long sizeInitial, unsigned long sizeIncrement = 0 );
	virtual					~BestFitHeap();

	void					IBestFitHeap(); // MEB
	void					ExpandHeap( unsigned long sizeInitial, unsigned long sizeIncrement ); // MEB
	
	void					SetLimitFromMax( size_t limit );
							// CHANGE don't allocate last LIMIT bytes of memory
							// will need a new_handler function!
#ifdef DEBUG
	virtual void			Check() const;
	virtual Boolean			IsMyBlock( void* blk ) const;
	virtual void			Print(char* msg = "" ) const;
#endif

protected:
	void					AddToFreeBlocks( BestFitBlock* blk,
								FreeBlockDisposal doDispose = kDisposeFreeBlockSegments);
	BestFitBlock*			Coalesce( BestFitBlock* blk );
	void					CreateNewSegment( unsigned long size );
	void					DeleteSegments();
	void					DeleteFreeSegments();
	void					GrowHeap( unsigned long sizeIncrement );
	void					RemoveFromFreeBlocks( BestFitBlock* blk );
	BestFitBlock*			SearchFreeBlocks( FW_BlockSize size );

	virtual void*			DoAllocate( FW_BlockSize size, FW_BlockSize& allocatedSize );
	virtual FW_BlockSize	DoBlockSize( const void* block ) const;
	virtual void			DoFree( void* );
	virtual void			DoReset();

#ifdef DEBUG
	virtual void			CompilerCheck();
	virtual Boolean			DoIsValidBlock( void* blk ) const;
#endif

private:
	size_t					fDontUse; // CHANGE amount *not* to allocate from system, ever
	
	BestFitSegment*			fSegments;
	unsigned long			fSizeIncrement;
	unsigned long			fSizeInitial;
	FreeBlockTree			fFreeTree;
	

	BestFitHeap(const BestFitHeap& blk);
	BestFitHeap& operator=(const BestFitHeap& blk);
		// This class shouldn't be copied.
};

#endif // __BESTFITHEAP__



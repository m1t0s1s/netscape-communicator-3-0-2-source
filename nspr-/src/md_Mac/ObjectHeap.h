// ObjectHeap.h 
// Copyright © 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef __OBJECTHEAP__
#define __OBJECTHEAP__

#ifndef PLATFORMMEMORY_H
#include "PlatformMemory.h" // CHANGE <>
#endif

#ifndef __MEMORYHEAP__
#include "MemoryHeap.h" // CHANGE <>
#endif

#ifndef __BESTFITHEAP__
#include "BestFitHeap.h" // CHANGE <>
#endif

//========================================================================================
// Forward class declarations
//========================================================================================

class Chunk;
class ChunkyBlock;
class ObjectHeap;
class ChunkyBlockStack;
 

//========================================================================================
// CLASS ChunkyBlock
//========================================================================================

#ifdef BUILD_WIN
// Bytes are in reverse order in a word.

const unsigned short ChunkyBlock_kSizeIndexMask = 0x00F0;
const unsigned short ChunkyBlock_kSizeIndexShift = 4;

const unsigned short ChunkyBlock_kBlockIndexMask = 0x000F;
const unsigned short ChunkyBlock_kBlockIndexShift = 0;

const unsigned short ChunkyBlock_kBlockTypeMask = 0xF000;
const unsigned short ChunkyBlock_kBlockTypeShift = 12;

const unsigned short ChunkyBlock_kMagicNumberMask = 0x0F00;
const unsigned short ChunkyBlock_kMagicNumberShift = 8;
#else
const unsigned short ChunkyBlock_kSizeIndexMask = 0xF000;
const unsigned short ChunkyBlock_kSizeIndexShift = 12;

const unsigned short ChunkyBlock_kBlockIndexMask = 0x0F00;
const unsigned short ChunkyBlock_kBlockIndexShift = 8;

const unsigned short ChunkyBlock_kBlockTypeMask = 0x00F0;
const unsigned short ChunkyBlock_kBlockTypeShift = 4;

const unsigned short ChunkyBlock_kMagicNumberMask = 0x000F;
const unsigned short ChunkyBlock_kMagicNumberShift = 0;
#endif

struct SChunkHeader
{
	unsigned short fBlockBusyBits;
};

class ChunkyBlock
{
public:
	enum
	{
		kBusyOverhead = sizeof( unsigned short ), 
		kBlockTypeId = BestFitBlock::kBlockTypeId + 1, 
		kMagicNumber = 0xA
	};

	void *operator			new( SIZE_T, void *ptr );
	void *operator			new( SIZE_T );
	void operator delete	(void*) { };

							ChunkyBlock();
							ChunkyBlock( unsigned short sizeIndex, unsigned short blockIndex );
							ChunkyBlock( const ChunkyBlock& blk );
							ChunkyBlock& operator=( const ChunkyBlock& blk );
	
	unsigned short			GetSizeIndex() const
							{
								ASSERT_POINTER( this );
								return ( fBits & ChunkyBlock_kSizeIndexMask ) >> ChunkyBlock_kSizeIndexShift;
							}

	void					SetSizeIndex( unsigned short index )
							{
								ASSERT_POINTER( this );
								fBits &= ~ChunkyBlock_kSizeIndexMask;
								fBits |= ( index << ChunkyBlock_kSizeIndexShift ) & ChunkyBlock_kSizeIndexMask;
							}

	unsigned short			GetBlockIndex() const
							{
								ASSERT_POINTER( this );
								return ( fBits & ChunkyBlock_kBlockIndexMask ) >> ChunkyBlock_kBlockIndexShift;
							}
							
	void					SetBlockIndex( unsigned short index )
							{
								ASSERT_POINTER( this );
								fBits &= ~ChunkyBlock_kBlockIndexMask;
								fBits |= ( index << ChunkyBlock_kBlockIndexShift ) & ChunkyBlock_kBlockIndexMask;
							}
	
	unsigned short			GetBlockType() const
							{
								ASSERT_POINTER( this );
								return ( fBits & ChunkyBlock_kBlockTypeMask ) >> ChunkyBlock_kBlockTypeShift;
							}

	void					SetBlockType( unsigned short type )
							{
								ASSERT_POINTER( this );
								fBits &= ~ChunkyBlock_kBlockTypeMask;
								fBits |= ( type << ChunkyBlock_kBlockTypeShift ) & ChunkyBlock_kBlockTypeMask;
							}

	Chunk*					GetChunk( FW_BlockSize blkSize )
							{
								Chunk *chk 
									= (Chunk *) ((FW_BytePtr) this - 
										sizeof( SChunkHeader ) -
										blkSize * GetBlockIndex() );
								return chk;
							}

	ChunkyBlock*			GetNext()
							{
								ASSERT_POINTER( fNext );
								return fNext;
							}

	void					SetNext( ChunkyBlock* blk )
							{
								ASSERT_POINTER( blk );
								fNext = blk;
							}

	unsigned short			GetMagicNumber() const;
	void					SetMagicNumber( unsigned short magic );	

	Boolean					IsBusy( FW_BlockSize blockSize );
	void					SetBusy( FW_BlockSize blockSize, Boolean busy );
							
	// Fields present in both free and busy blocks. Several bit fields are stored in
	// the following fields. They are accessed using get and set methods.

	unsigned short			fBits;

	// Fields present in only free blocks.

	ChunkyBlock*			fNext;
};


//========================================================================================
// CLASS ChunkyBlockStack
//========================================================================================

class ChunkyBlockStack
{
public:
							ChunkyBlockStack();
							ChunkyBlockStack( const ChunkyBlockStack& blk );
							~ChunkyBlockStack();
							ChunkyBlockStack& operator=( const ChunkyBlockStack& blk );

	ChunkyBlock*			Pop()
							{
								ChunkyBlock* blk = fHead.GetNext();
								fHead.SetNext( blk ? blk->GetNext(): NULL ); // CHANGE handle nil fNext (MARK 9/5)
								return blk;
							}

	void					Push( ChunkyBlock* blk )
							{
								ASSERT_POINTER( blk );
								blk->SetNext( fHead.GetNext() );
								fHead.SetNext( blk );
							}

	void					RemoveRange( void* begAddr, void* endAddr );
	ChunkyBlock*			Top()
							{
								return fHead.GetNext();
							}

	ChunkyBlock				fHead;
};


//========================================================================================
// CLASS Chunk
//========================================================================================

class Chunk
{
public:
							Chunk( short blocksPerChunk, unsigned short sizeIndex,
			  					FW_BlockSize blockSize );

	void *operator			new( SIZE_T, void* ptr );
	void *operator			new( SIZE_T );
	void operator			delete(void *) { };

	ChunkyBlock*			GetBlock( unsigned short blkIndex, FW_BlockSize blkSize )
							{
								ChunkyBlock * blk 
									= (ChunkyBlock *) ((FW_BytePtr) this + sizeof(SChunkHeader) + blkIndex * blkSize);
								return blk;
							}

	unsigned short			GetSizeIndex()
							{
								ChunkyBlock *block 
									= (ChunkyBlock*) ((FW_BytePtr) this + sizeof( SChunkHeader ) );
								return block->GetSizeIndex();
							}

	Boolean					IsBlockBusy( unsigned short whichBlock )
							{
								return (fHeader.fBlockBusyBits & ( 0x0001 << whichBlock ) ) != 0;
							}

	Boolean					IsBusy()
							{
								return fHeader.fBlockBusyBits != 0;
							}

	void					SetBlockBusy( unsigned short whichBlock, Boolean busy )
							{
							if ( busy )
								fHeader.fBlockBusyBits |= ( 0x0001 << whichBlock );
							else
								fHeader.fBlockBusyBits &= ~( 0x0001 << whichBlock );
							}

	SChunkHeader			fHeader;
	
							Chunk( const Chunk& blk );
							Chunk& operator=( const Chunk& blk );
							// This class shouldn't be copied.
};


//========================================================================================
// CLASS ObjectHeap
//========================================================================================

class ObjectHeap: public BestFitHeap
{
private:
	static const FW_BlockSize kDefaultBlockSizes[];

public:
	enum
	{
		kMaxNumberOfBlockSizes = 16, 
		kDefaultBlocksPerChunk = 4,
		kDefaultInitialSize = 10240,
		kDefaultIncrementSize = 4096
	};

					ObjectHeap(	unsigned long initialSize,
			 			unsigned long incrementSize = 0,
			 			short blocksPerChunk = kDefaultBlocksPerChunk);

					ObjectHeap(	const FW_BlockSize* blockSizes = kDefaultBlockSizes,
					 unsigned long initialSize = kDefaultInitialSize,
					 unsigned long incrementSize = kDefaultIncrementSize,
					 short blocksPerChunk = kDefaultBlocksPerChunk );

	void			IObjectHeap();

	virtual 		~ObjectHeap();

protected:
	virtual void*	DoAllocate( FW_BlockSize size, FW_BlockSize& allocatedSize );
	virtual FW_BlockSize DoBlockSize(const void* block) const;
	virtual void	DoFree( void* );
	virtual void	DoReset();

	void*			AllocateBlock( unsigned short sizeIndex );
	void			CreateNewChunk( unsigned short sizeIndex );
	void			FreeBlock( ChunkyBlock* blk );
	unsigned short	SizeIndex( FW_BlockSize size );

#ifdef DEBUG
	virtual void	CompilerCheck();
	virtual Boolean	DoIsValidBlock( void* blk ) const;
#endif

private:
	
	short				fNumberOfBlockSizes;
	const FW_BlockSize* fBlockSizes;
	ChunkyBlockStack	fFreeLists[ kMaxNumberOfBlockSizes ];
	short				fBlocksPerChunk;
	FW_BlockSize		fLargestBlockSize;
	

					ObjectHeap( const ObjectHeap& blk );
					ObjectHeap& operator=( const ObjectHeap& blk );
					// This class shouldn't be copied.
};


//========================================================================================
// CLASS ChunkyBlock
//========================================================================================

//----------------------------------------------------------------------------------------
// ChunkyBlock::operator new
//----------------------------------------------------------------------------------------

inline void *ChunkyBlock::operator new( SIZE_T, void *ptr )
{
	return ptr;
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::operator new
//----------------------------------------------------------------------------------------

inline void *ChunkyBlock::operator new( SIZE_T )
{
	return NULL;
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::ChunkyBlock
//----------------------------------------------------------------------------------------

inline ChunkyBlock::ChunkyBlock( const ChunkyBlock& blk ) :
	fBits( blk.fBits ),
	fNext( blk.fNext )
{
	ASSERT_POINTER( blk.fNext );
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::operator=
//----------------------------------------------------------------------------------------

inline ChunkyBlock& ChunkyBlock::operator=( const ChunkyBlock& blk )
{
	fBits = blk.fBits;
	fNext = blk.fNext;
	ASSERT_POINTER( blk.fNext );
	return *this;
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::GetMagicNumber
//----------------------------------------------------------------------------------------

inline unsigned short ChunkyBlock::GetMagicNumber() const
{
	ASSERT_POINTER( this );
	return ( fBits & ChunkyBlock_kMagicNumberMask ) >> ChunkyBlock_kMagicNumberShift;
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::SetMagicNumber
//----------------------------------------------------------------------------------------

inline void ChunkyBlock::SetMagicNumber( unsigned short magic )
{
	ASSERT_POINTER( this );
	fBits &= ~ChunkyBlock_kMagicNumberMask;
	fBits |= ( magic << ChunkyBlock_kMagicNumberShift ) & ChunkyBlock_kMagicNumberMask;
}	

//========================================================================================
// CLASS Chunk
//========================================================================================

//----------------------------------------------------------------------------------------
// Chunk::operator new
//----------------------------------------------------------------------------------------
inline void* Chunk::operator new( SIZE_T, void *ptr )
{
	return ptr;
}

//----------------------------------------------------------------------------------------
// Chunk::operator new
//----------------------------------------------------------------------------------------
inline void* Chunk::operator new( SIZE_T )
{
	return NULL;
}

inline Boolean ChunkyBlock::IsBusy( FW_BlockSize blockSize )
{
	return this->GetChunk( blockSize )->IsBlockBusy( GetBlockIndex() );
}

inline void	ChunkyBlock::SetBusy( FW_BlockSize blockSize, Boolean busy )
{
	this->GetChunk( blockSize )->SetBlockBusy( GetBlockIndex(), busy );
}



#define SET_CHK_BLOCK_BUSY( chk, blockIndex ) \
	(chk)->fHeader.fBlockBusyBits |= ( 0x0001 << (blockIndex) )

#define CLEAR_CHK_BLOCK_BUSY( chk, blockIndex ) \
	(chk)->fHeader.fBlockBusyBits &= ~( 0x0001 << (blockIndex) );

#define PUSH_BLK( blk, freeList ) \
	(blk)->fNext = (freeList)->fHead.fNext; \
	(freeList)->fHead.fNext = (blk);

#define POP_BLK( blk, freeList ) \
	(blk) = (freeList)->fHead.fNext; \
	(freeList)->fHead.fNext = ( (blk)? (blk)->fNext : NULL );

#define CHUNK_PTR( blk, blockSize, blockIndex ) \
	(Chunk*)( (FW_BytePtr)(blk) - sizeof(SChunkHeader) - ( (blockSize)*(blockIndex) ) )
	
#define BLOCK_INDEX( blk ) \
	( ( (blk)->fBits & ChunkyBlock_kBlockIndexMask ) >> ChunkyBlock_kBlockIndexShift )
#define SIZE_INDEX( blk ) \
	( ( (blk)->fBits & ChunkyBlock_kSizeIndexMask ) >> ChunkyBlock_kSizeIndexShift )
	
#define CHUNKY_BLOCK_PTR( chk, blkIndex, blkSize ) \
	(ChunkyBlock*)( ((FW_BytePtr)(chk)) + sizeof(SChunkHeader) + ((blkIndex) * (blkSize)) )


#endif

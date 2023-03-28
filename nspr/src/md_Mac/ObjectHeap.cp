// ObjectHeap.cp 
// Copyright © 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef PLATFORMMEMORY_H
#include "PlatformMemory.h"
#endif

#ifndef __OBJECTHEAP__
#include "ObjectHeap.h"
#endif

#ifdef PROFILE
#pragma profile on
#endif

#ifdef DEBUG

char * keepConditionAlive; // dummy

void AssertPointer (const void * p, char *condition)
{
	keepConditionAlive = condition; // dummy to keep this around in the debugger
	long pp = (long) p;
	PR_ASSERT (
		pp == 0 
		|| (0x00200000 <= pp && pp <= 0x08000000)
	);
}

#endif

//========================================================================================
// CLASS ChunkyBlock
//========================================================================================

#define SET_SIZE_INDEX( blk, sizeIndex ) \
	(blk)->fBits |= ( (sizeIndex) << ChunkyBlock_kSizeIndexShift ) & ChunkyBlock_kSizeIndexMask;

#define SET_BLOCK_INDEX( blk, blockIndex ) \
	(blk)->fBits |= ( (blockIndex) << ChunkyBlock_kBlockIndexShift ) & ChunkyBlock_kBlockIndexMask;

#define SET_BLOCK_TYPE( blk, blockType ) \
	(blk)->fBits |= ( (blockType) << ChunkyBlock_kBlockTypeShift ) & ChunkyBlock_kBlockTypeMask;

#define SET_MAGIC_NUMBER( blk, magic ) \
	(blk)->fBits |= ( (magic) << ChunkyBlock_kMagicNumberShift ) & ChunkyBlock_kMagicNumberMask;

//----------------------------------------------------------------------------------------
// ChunkyBlock::ChunkyBlock
//----------------------------------------------------------------------------------------

ChunkyBlock::ChunkyBlock()
{
	SET_BLOCK_TYPE( this, kBlockTypeId );
	SET_MAGIC_NUMBER( this, kMagicNumber );
	fNext = NULL;
}

//----------------------------------------------------------------------------------------
// ChunkyBlock::ChunkyBlock
//----------------------------------------------------------------------------------------

ChunkyBlock::ChunkyBlock( unsigned short sizeIndex, unsigned short blockIndex )
{
	fBits = 0;
	SET_SIZE_INDEX( this, sizeIndex );
	SET_BLOCK_INDEX( this, blockIndex );
	SET_BLOCK_TYPE( this, kBlockTypeId );
	SET_MAGIC_NUMBER( this, kMagicNumber );
	fNext = NULL;
}

//========================================================================================
// Chunk
//========================================================================================

//----------------------------------------------------------------------------------------
// Chunk::Chunk
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Chunk::Chunk( short blocksPerChunk, unsigned short sizeIndex, FW_BlockSize blockSize )
{
	fHeader.fBlockBusyBits = 0;
	void* blkPtr = (void*) ((FW_BytePtr)this + sizeof( SChunkHeader ));
	for ( int i = 0; i < blocksPerChunk; i++ )
	{
		ChunkyBlock* blk = new ( blkPtr ) ChunkyBlock( sizeIndex, i );
		blkPtr = (void*) ( (FW_BytePtr)blkPtr + blockSize );
	}
}

//========================================================================================
// ObjectHeap
//========================================================================================

const FW_BlockSize ObjectHeap::kDefaultBlockSizes[] = {sizeof(ChunkyBlock), 10, 14, 18, 0};

//----------------------------------------------------------------------------------------
// ObjectHeap::ObjectHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ObjectHeap::ObjectHeap( unsigned long initialSize,
				   unsigned long incrementSize,
				   short blocksPerChunk ) :
	BestFitHeap( initialSize, incrementSize ),
	fBlockSizes( kDefaultBlockSizes )
{
#ifdef DEBUG
	CompilerCheck();
#endif

	fBlocksPerChunk = blocksPerChunk;

	for ( fNumberOfBlockSizes = 0; fBlockSizes[ fNumberOfBlockSizes ]; fNumberOfBlockSizes++ )
		;
		
	fLargestBlockSize = fBlockSizes[ fNumberOfBlockSizes - 1 ];
}

//----------------------------------------------------------------------------------------
// ObjectHeap::ObjectHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ObjectHeap::ObjectHeap(const FW_BlockSize* blockSizes,
				   unsigned long initialSize,
				   unsigned long incrementSize,
				   short blocksPerChunk) :
	BestFitHeap( initialSize, incrementSize ),
	fBlockSizes( blockSizes )
{
#ifdef DEBUG
	CompilerCheck();
#endif

	fBlocksPerChunk = blocksPerChunk;

	for ( fNumberOfBlockSizes = 0; fBlockSizes[ fNumberOfBlockSizes ]; fNumberOfBlockSizes++ )
		;
		
	fLargestBlockSize = fBlockSizes[ fNumberOfBlockSizes - 1 ];
}

//----------------------------------------------------------------------------------------
// ObjectHeap::IObjectHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::IObjectHeap()
{
	this->IBestFitHeap();
}

//----------------------------------------------------------------------------------------
// ObjectHeap::~ObjectHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ObjectHeap::~ObjectHeap()
{
}

//----------------------------------------------------------------------------------------
// ObjectHeap::DoAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

#define ALLOCATE_SLOP

extern void TraceAllocate (size_t size);

void* ObjectHeap::DoAllocate( FW_BlockSize size, FW_BlockSize& allocatedSize )
{
#ifdef ALLOCATE_SLOP
	FW_BlockSize	realSize = size + ChunkyBlock::kBusyOverhead;
	
	if ( realSize > fLargestBlockSize )
		return BestFitHeap::DoAllocate( size, allocatedSize );

	register unsigned short		sizeIndex;
		
	allocatedSize = fBlockSizes[ sizeIndex = this->SizeIndex( size ) ] -
					ChunkyBlock::kBusyOverhead;
	return this->AllocateBlock( sizeIndex );
#else

	unsigned short sizeIndex = fNumberOfBlockSizes;
	// always allocate minimum a [0] byte block.
//	if (size + ChunkyBlock::kBusyOverhead < fBlockSizes[0])
//		sizeIndex = 0;
//	else

	for ( unsigned short i = 0; i < fNumberOfBlockSizes; i++ )
		// don't use bins for odd sizes to avoid wasted slop
		if ( size + ChunkyBlock::kBusyOverhead == fBlockSizes[i] )
		{
			sizeIndex = i;
			break;
		}
		
	if ( sizeIndex == fNumberOfBlockSizes )
		return BestFitHeap::DoAllocate( size, allocatedSize );
	else
	{
		allocatedSize = fBlockSizes[ sizeIndex ] - ChunkyBlock::kBusyOverhead;
		return this->AllocateBlock( sizeIndex );
	}
#endif
}

//----------------------------------------------------------------------------------------
// ObjectHeap::DoBlockSize
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FW_BlockSize ObjectHeap::DoBlockSize(const void* ptr) const
{
	ChunkyBlock *block
		= (ChunkyBlock *) ((FW_BytePtr) ptr - ChunkyBlock::kBusyOverhead);
		
	if (block->GetBlockType() == BestFitBlock::kBlockTypeId)
		return BestFitHeap::DoBlockSize(ptr);
	else
		return fBlockSizes[block->GetSizeIndex()] - ChunkyBlock::kBusyOverhead;
}

//----------------------------------------------------------------------------------------
// ObjectHeap::DoFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::DoFree(void* ptr)
{
	ChunkyBlock *block 
		= (ChunkyBlock *) ((FW_BytePtr) ptr - ChunkyBlock::kBusyOverhead);
		
	if (block->GetBlockType() == BestFitBlock::kBlockTypeId)
		BestFitHeap::DoFree(ptr);
	else
		this->FreeBlock(block);
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// ObjectHeap::DoIsValidBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean ObjectHeap::DoIsValidBlock(void* ptr) const
{
	Boolean isBlockValid = false;
	
	ChunkyBlock *block
		= (ChunkyBlock *) ((FW_BytePtr) ptr - ChunkyBlock::kBusyOverhead);

	if (block->GetBlockType() == BestFitBlock::kBlockTypeId)
		isBlockValid = BestFitHeap::DoIsValidBlock(ptr);
	else
		isBlockValid
			= block->GetSizeIndex() <= fNumberOfBlockSizes &&
			  block->GetBlockIndex() <= fBlocksPerChunk &&
			  block->GetMagicNumber() == (unsigned short) ChunkyBlock::kMagicNumber;

	return isBlockValid;
}
#endif

//----------------------------------------------------------------------------------------
// ObjectHeap::DoReset
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::DoReset()
{
	ChunkyBlockStack clr;

	for (int i = 0; i < fNumberOfBlockSizes; i++)
		fFreeLists[i] = clr;

	BestFitHeap::DoReset();
}

//----------------------------------------------------------------------------------------
// ObjectHeap::AllocateBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void* ObjectHeap::AllocateBlock( unsigned short sizeIndex )
{
	ChunkyBlockStack*	freeList = &fFreeLists[ sizeIndex ];
	ChunkyBlock*		blk = NULL;
	
	// ¥ pull the first block off the free list
	POP_BLK( blk, freeList );
	
	// ¥Êif we have one, set it busy and return it
	if ( blk )
		goto done;
		
	// ¥ else do a lot of extra work to create a new chunk, get a block out of it
	//		and return that one
	this->CreateNewChunk( sizeIndex );
	POP_BLK( blk, freeList );

	// ¥ we're flat out empty now
	if ( !blk )
		return NULL;

done:
	// ¥Êget our block size
	FW_BlockSize		blockSize = fBlockSizes[ sizeIndex ];
	// ¥ and our block index
	unsigned short		blockIndex = BLOCK_INDEX( blk );
	
	// ¥ get the chunk ptr so we can get to it's fBlockBusyBits
	Chunk*				chk = CHUNK_PTR( blk, blockSize, blockIndex );
	
	// ¥Êmark the block busy
	SET_CHK_BLOCK_BUSY( chk, blockIndex );

	return (void *) ((FW_BytePtr) blk + ChunkyBlock::kBusyOverhead );
}	


/*	if ( ! ( fFreeLists[ sizeIndex ].Top() ) )
		this->CreateNewChunk( sizeIndex );

	ChunkyBlock* blk = fFreeLists[ sizeIndex ].Pop();
	if ( blk )
	{
		blk->SetBusy( fBlockSizes[ sizeIndex ], true );
		return (void *) ((FW_BytePtr) blk + ChunkyBlock::kBusyOverhead );
	}
	else
		return NULL;
*/

//----------------------------------------------------------------------------------------
// ObjectHeap::FreeBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::FreeBlock( ChunkyBlock* blk )
{
/*	blk->SetBusy(fBlockSizes[blk->GetSizeIndex()], false);
	fFreeLists[blk->GetSizeIndex()].Push(blk);

	// Check to see if all blocks in this block's Chunk are free, if so then free the
	// Chunk.

	Chunk *chk = blk->GetChunk(fBlockSizes[blk->GetSizeIndex()]);
	if (!chk->IsBusy())
	{
		// Remove blocks in this Chunk from the free list. This is the achililles hill
		// of the Heap. Its difficult to remove blocks from a singly linked list
		// rapidly.

		void *begAddr = chk;
		void *endAddr = (void *) ((FW_BytePtr) chk + 
									sizeof(SChunkHeader) +
									fBlocksPerChunk * fBlockSizes[chk->GetSizeIndex()]);
		fFreeLists[chk->GetSizeIndex()].RemoveRange(begAddr, endAddr);
		BestFitHeap::DoFree(chk);
	}
*/
	unsigned short		sizeIndex = SIZE_INDEX( blk );
	unsigned short		blockIndex = BLOCK_INDEX( blk );
	unsigned long		blockSize = fBlockSizes[ sizeIndex ];
	ChunkyBlockStack*	freeList = &fFreeLists[ sizeIndex ];

	// ¥ get the chunk ptr so we can get to it's fBlockBusyBits
	Chunk*				chk = CHUNK_PTR( blk, blockSize, blockIndex );

	CLEAR_CHK_BLOCK_BUSY( chk, blockIndex );
	
	// ¥ push the block onto the free list
	PUSH_BLK( blk, freeList );
	
	// ¥ check to see if all blocks in this block's Chunk are free, if so then free the
	//		chunk.

	if ( !chk->fHeader.fBlockBusyBits )
	{
		// ¥ remove blocks in this Chunk from the free list. This is the achililles hill of the Heap.
		//		Its difficult to remove blocks from a singly linked list rapidly.

		void*	begAddr = chk;
		void*	endAddr = (void *) ((FW_BytePtr)chk + 
								sizeof( SChunkHeader ) +
								fBlocksPerChunk * blockSize );
		freeList->RemoveRange( begAddr, endAddr );
		BestFitHeap::DoFree( chk );
	}
}

//----------------------------------------------------------------------------------------
// ObjectHeap::CreateNewChunk
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::CreateNewChunk( unsigned short sizeIndex )
{
/*	FW_BlockSize allocatedSize;
	FW_BlockSize allocSize = sizeof(SChunkHeader) + fBlocksPerChunk * fBlockSizes[sizeIndex];
	void * memory = BestFitHeap::DoAllocate(allocSize, allocatedSize);
	Chunk * chk = new(memory)Chunk(fBlocksPerChunk, sizeIndex, fBlockSizes[sizeIndex]);
	if (chk != NULL)
	{
		for (int i = 0; i < fBlocksPerChunk; i++)
		{
			ChunkyBlock* blk = CHUNKY_BLOCK_PTR( chk, i, fBlockSizes[ sizeIndex ] );
			//ChunkyBlock * blk = chk->GetBlock(i, fBlockSizes[sizeIndex]);
			//blk->SetBusy(fBlockSizes[sizeIndex], false);
			fFreeLists[sizeIndex].Push(blk);
		}
	}

*/
	FW_BlockSize		blockSize = fBlockSizes[ sizeIndex ];
	FW_BlockSize		allocatedSize;
	FW_BlockSize		allocSize = sizeof( SChunkHeader ) + ( fBlocksPerChunk * blockSize );
	
	void* memory = BestFitHeap::DoAllocate( allocSize, allocatedSize );

	Chunk* chk = new ( memory ) Chunk( fBlocksPerChunk, sizeIndex, blockSize );

	if ( chk )
	{
		ChunkyBlockStack*	freeList = &fFreeLists[ sizeIndex ];
		ChunkyBlock*		blk;
		
		for ( int blockIndex = 0; blockIndex < fBlocksPerChunk; blockIndex++ )
		{
			blk = CHUNKY_BLOCK_PTR( chk, blockIndex, blockSize );
			PUSH_BLK( blk, freeList );
		}
	}
}

//----------------------------------------------------------------------------------------
// ObjectHeap::SizeIndex
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

unsigned short ObjectHeap::SizeIndex( FW_BlockSize size )
{
	register FW_BlockSize		realSize = size + ChunkyBlock::kBusyOverhead;
	register unsigned short		count = 0;
	
	while ( count < fNumberOfBlockSizes )
	{
#ifdef ALLOCATE_SLOP
		if ( realSize <= fBlockSizes[ count ] )
#else
		if ( realSize == fBlockSizes[ count ] )
#endif
			return count;
		count++;
	}

	// Uh oh! An internal error:

#ifdef XP_MAC
	DebugStr("\pObjectHeap::SizeIndex internal error");
#else
#ifdef ALLOCATE_SLOP
	BreakToSourceDebugger_();
#endif
#endif

	return 0xFFFFFFFF;	// the former way of handling the internal error
						// we leave it in to defeat an xlC warning
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// ObjectHeap::CompilerCheck
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ObjectHeap::CompilerCheck()
{
	BestFitHeap::CompilerCheck();
	
	ChunkyBlock block;
	
	block.SetSizeIndex(0xF);
	block.SetBlockIndex(0xE);
	block.SetBlockType(ChunkyBlock::kBlockTypeId);
	block.SetMagicNumber(0xC);
	
	PLATFORM_ASSERT(block.GetSizeIndex() == 0xF);
	PLATFORM_ASSERT(block.GetBlockIndex() == 0xE);
	PLATFORM_ASSERT(block.GetBlockType() == ChunkyBlock::kBlockTypeId);
	PLATFORM_ASSERT(block.GetBlockType() != BestFitBlock::kBlockTypeId);
	PLATFORM_ASSERT(block.GetMagicNumber() == 0xC);
	
	block.SetSizeIndex(0x7);
	block.SetBlockIndex(0x6);
	block.SetBlockType(BestFitBlock::kBlockTypeId);
	block.SetMagicNumber(0x4);
	
	PLATFORM_ASSERT(block.GetSizeIndex() == 0x7);
	PLATFORM_ASSERT(block.GetBlockIndex() == 0x6);
	PLATFORM_ASSERT(block.GetBlockType() == BestFitBlock::kBlockTypeId);
	PLATFORM_ASSERT(block.GetBlockType() != ChunkyBlock::kBlockTypeId);
	PLATFORM_ASSERT(block.GetMagicNumber() == 0x4);
}
#endif

//========================================================================================
// ChunkyBlockStack
//========================================================================================

//----------------------------------------------------------------------------------------
// ChunkyBlockStack::ChunkyBlockStack
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ChunkyBlockStack::ChunkyBlockStack()
{
	fHead.SetNext(NULL);
}

//----------------------------------------------------------------------------------------
// ChunkyBlockStack::ChunkyBlockStack
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ChunkyBlockStack::ChunkyBlockStack(const ChunkyBlockStack& blk) :
	fHead(blk.fHead)
{
}

//----------------------------------------------------------------------------------------
// ChunkyBlockStack::operator=
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ChunkyBlockStack& ChunkyBlockStack::operator=(const ChunkyBlockStack& blk)
{
	fHead = blk.fHead;
	return *this;
}

//----------------------------------------------------------------------------------------
// ChunkyBlockStack::RemoveRange
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void ChunkyBlockStack::RemoveRange( void* begAddr, void* endAddr )
{
	ASSERT_POINTER( begAddr );
	ASSERT_POINTER( endAddr );
	ChunkyBlock*	prevBlk = &fHead;
	ChunkyBlock*	curBlk = fHead.GetNext();

	while ( curBlk )
	{
		void*	curAddr = curBlk;

		if ( curAddr >= begAddr && curAddr <= endAddr )
		{
			prevBlk->SetNext( curBlk->GetNext() );
			curBlk = curBlk->GetNext();
		}
		else
		{
			prevBlk = curBlk;
			curBlk = curBlk->GetNext();
		}
	}
}

//----------------------------------------------------------------------------------------
// ChunkyBlockStack::~ChunkyBlockStack
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

ChunkyBlockStack::~ChunkyBlockStack()
{
}

#ifdef PROFILE
#pragma profile off
#endif


// BestFitHeap.cp 
// Copyright © 1985-1994 by Apple Computer, Inc.  All rights reserved.

#ifndef PLATFORMMEMORY_H
#include "PlatformMemory.h" // CHANGE <>
#endif

#ifndef __BESTFITHEAP__
#include "BestFitHeap.h" // CHANGE <>
#endif

#ifndef __MEMORY__
#include <Memory.h>
#endif

#ifndef __LIMITS__
#include <limits.h>
#endif

#ifndef __STRING__
#include <string.h>
#endif

#ifndef __STDIO__
#include <stdio.h>
#endif

#ifdef PROFILE
#pragma profile on
#endif

#ifdef XP_MAC
#include "prlog.h"
#include "xpassert.h"
#endif

PR_LOG_DEFINE(NSPR);

// CHANGE #define gIntenseDebugging 0 - we're not in MacApp
#define gIntenseDebugging 0

#if defined(DEBUG) && defined(powerc) && !defined(__profile)
//#define DEBUG_MEMORY
#endif

inline long Max (long a, long b) {
	return a > b? a: b;
}

//========================================================================================
// BestFitBlock
//========================================================================================

#ifdef BUILD_WIN16
const FW_BlockSize BestFitBlock::kMaxBlockSize = 60L * 1024L;
	// Block size limited by 64K limit of far pointers. Allow 4K for overhead in the
	// various layers.
#else
const FW_BlockSize BestFitBlock::kMaxBlockSize = 0x00FFFFFF; // 16 megabytes (see bitfields)
#endif

//----------------------------------------------------------------------------------------
// BestFitBlock::operator >
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean BestFitBlock::operator>(const BestFitBlock& blk) const
{
	if (GetSize() == blk.GetSize())
		return this > &blk;
	else
		return GetSize() > blk.GetSize();
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator <
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean BestFitBlock::operator<(const BestFitBlock& blk) const
{
	if (GetSize() == blk.GetSize())
		return this < &blk;
	else
		return GetSize() < blk.GetSize();
}

//----------------------------------------------------------------------------------------
// BestFitBlock::operator ==
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean BestFitBlock::operator==(const BestFitBlock& blk) const
{
	return GetSize() == blk.GetSize() && this == &blk;
}

//----------------------------------------------------------------------------------------
// BestFitBlock::StuffAddressAtEnd
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitBlock::StuffAddressAtEnd()
{
	// coalescence possible in constant time.

	if (!this->GetBusy())
	{
		void** addr= (void**) ((FW_BytePtr) this + this->GetSize() - sizeof(BestFitBlock *));
		*addr = this;
	}
#ifdef DEBUG
	else
		DebugStr("\pBestFitBlock::StuffAddressAtEnd: corrupt heap!");
#endif
}


//========================================================================================
// BestFitHeap
//========================================================================================

//----------------------------------------------------------------------------------------
// BestFitHeap::BestFitHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitHeap::BestFitHeap(unsigned long sizeInitial,
								 unsigned long sizeIncrement) :
	MemoryHeap(false, false, false) // MARK don't zap on allocate or free
{
#ifdef DEBUG
	CompilerCheck();
#endif

	fSizeIncrement = sizeIncrement;
	fSizeInitial = sizeInitial;
	fSegments = NULL;
	fDontUse = 0; // CHANGE warning this allows Bedrock to allocate the very last byte of memory!

//	this->GrowHeap(fSizeInitial);	* MEB *
}

//----------------------------------------------------------------------------------------
// BestFitHeap::~BestFitHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitHeap::~BestFitHeap()
{
	this->DeleteSegments();
}

//----------------------------------------------------------------------------------------
// BestFitHeap::IBestFitHeap	* MEB *
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::IBestFitHeap()
{
	this->GrowHeap(fSizeInitial);
}

//----------------------------------------------------------------------------------------
// BestFitHeap::ExpandHeap	* MEB *
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::ExpandHeap(unsigned long sizeInitial, unsigned long sizeIncrement)
{
	if (sizeInitial > fSizeInitial)
		fSizeInitial = sizeInitial;
	if (sizeIncrement > fSizeIncrement)
		fSizeIncrement = sizeIncrement;
	
	if (sizeInitial > this->HeapSize())
		this->GrowHeap(sizeInitial);
	else
		this->GrowHeap(sizeIncrement);
}

void
BestFitHeap::SetLimitFromMax (size_t limit) // CHANGE don't allocate last LIMIT bytes of memory
{
	fDontUse = limit;
}

//----------------------------------------------------------------------------------------
// BestFitHeap::BytesFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

unsigned long BestFitHeap::BytesFree() const
{
	unsigned long bytesFree, numberOfNodes;

	fFreeTree.TreeInfo(bytesFree, numberOfNodes);
	bytesFree -= numberOfNodes * BestFitBlock::kBusyOverhead;

	return bytesFree;
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// BestFitHeap::Check
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::Check() const
{
#ifdef DEBUG_MEMORY
	fFreeTree.CheckTree();
#endif
}
#endif

//----------------------------------------------------------------------------------------
// BestFitHeap::DoAllocate
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void* BestFitHeap::DoAllocate(FW_BlockSize dataSize, FW_BlockSize& allocatedSize)
{
	FW_BlockSize blockSize = dataSize + BestFitBlock::kBusyOverhead;

	if (blockSize < BestFitBlock::kMinBlockSize)
		blockSize = BestFitBlock::kMinBlockSize;

	// Make sure blockSize is rounded to the nearest 32-bit long word.

	blockSize = ((blockSize + 3U) & ~3U);

#ifdef DEBUG
	PLATFORM_ASSERT(blockSize <= BestFitBlock::kMaxBlockSize);
#endif

	BestFitBlock * blk = this->SearchFreeBlocks(blockSize);

	// Make an attempt to expand the heap

	if (blk == NULL && fSizeIncrement > 0)
	{
		// Expand heap so that allocation request can be filled
		unsigned long neededSegSize = fSizeIncrement; // *includes* segment overhead
		FW_BlockSize segOverhead = BestFitSegment::kSegmentOverhead + BestFitBlock::kMinBlockSize;
		if (blockSize + segOverhead > fSizeIncrement)
		{
			neededSegSize = blockSize + segOverhead;
			PR_LOG(NSJAVA, error, ("BestFitHeap::DoAllocate: %i > fSizeIncrement!", blockSize));
		}

		this->GrowHeap(neededSegSize);
		blk = this->SearchFreeBlocks(blockSize);
	}

	allocatedSize = 0;
	void* newPtr = NULL;

	if (blk != NULL)
	{
		// We found a block, so mark it busy and remove it from the free list.

		blk->SetBusy(true);
		this->RemoveFromFreeBlocks(blk);

		if (blk->GetSize() - blockSize >= BestFitBlock::kMinBlockSize)
		{
			// The block is big enough to split, so here we do the split.

			BestFitBlock *splitBlock
				= new((void *) ((FW_BytePtr) blk + blockSize))
					BestFitBlock(false, true, blk->GetSize() - blockSize);
			this->AddToFreeBlocks(splitBlock, kDisposeFreeBlockSegments);
			blk->SetSize(blockSize);
		}

		BestFitBlock * nextBlk
			= (BestFitBlock *) ((FW_BytePtr) blk + blk->GetSize());
		nextBlk->SetPrevBusy(true);

		newPtr = (void *) ((FW_BytePtr) blk + BestFitBlock::kBusyOverhead);
		allocatedSize = blk->GetSize() - BestFitBlock::kBusyOverhead;
	}

	return newPtr;
}

//----------------------------------------------------------------------------------------
// BestFitHeap::DoBlockSize
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FW_BlockSize BestFitHeap::DoBlockSize(const void* block) const
{
	BestFitBlock * blk
		= (BestFitBlock *) ((FW_BytePtr) block - BestFitBlock::kBusyOverhead);

#ifdef DEBUG
	PLATFORM_ASSERT(blk->GetBusy());
#endif
	
	return blk->GetSize() - BestFitBlock::kBusyOverhead;
}

//----------------------------------------------------------------------------------------
// BestFitHeap::DoFree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::DoFree(void* ptr)
{
	BestFitBlock *blk
		= (BestFitBlock *) ((FW_BytePtr) ptr - BestFitBlock::kBusyOverhead);
	
	// CHANGE mark lanett, Bedrock isn't being paranoid enough
	if (blk->GetMagicNumber() != 3) {
#ifdef DEBUG
		PR_ASSERT (blk->GetMagicNumber() == 3);
		return;
#endif
	}
	if (!blk->GetBusy()) {
#ifdef DEBUG
		PLATFORM_ASSERT(blk->GetBusy());
#endif
		return;
	}
	
	blk->SetBusy(false);
	blk->StuffAddressAtEnd();

	BestFitBlock *nextBlk
		= (BestFitBlock *) ((FW_BytePtr) blk + blk->GetSize());
	nextBlk->SetPrevBusy(false);

	if (this->Coalesce(nextBlk))
		this->RemoveFromFreeBlocks(nextBlk);
/*
	Coalesce (blk) will corrupt the Free Block Tree if it succeeds, because
	it changes the size of "blk" in place and the tree is size-sorted.
	We can't remove this block from the tree after it returns because it
	will assert. Therefore we have to remove it beforehand. Since this would
	suck if coalesce was going to fail often, we'll cheat and check whether
	it would succeed first.
	//BestFitBlock * prevBlk = this->Coalesce(blk);
	//if (prevBlk)
*/
	if (!blk->GetBusy() && !blk->GetPreviousBusy()) {
		// we have to remove the prevblock before coalescing because 
		// if prevblk's size changes it invalidates its position in
		// the free block tree
		BestFitBlock *prevBlk = *(BestFitBlock **) ((FW_BytePtr) blk - sizeof(FW_BlockSize));
		this->RemoveFromFreeBlocks(prevBlk);
		this->Coalesce(blk);
		this->AddToFreeBlocks(prevBlk, kDisposeFreeBlockSegments);
	}
	else
		this->AddToFreeBlocks(blk, kDisposeFreeBlockSegments);
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// BestFitHeap::DoIsValidBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean BestFitHeap::DoIsValidBlock(void* ptr) const
{
	BestFitBlock *blk 
		= (BestFitBlock *) ((FW_BytePtr) ptr - BestFitBlock::kBusyOverhead);

	FW_BlockSize blkSize = blk->GetSize();

	return (blkSize <= fSizeIncrement || blkSize <= fSizeInitial) && blk->GetBusy() && blk->GetMagicNumber() == (unsigned short)BestFitBlock::kMagicNumber;
}
#endif

//----------------------------------------------------------------------------------------
// BestFitHeap::DoReset
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::DoReset()
{
	this->DeleteSegments();
	fSegments = NULL;
	fFreeTree = FreeBlockTree();

	this->GrowHeap(fSizeInitial);
}

//----------------------------------------------------------------------------------------
// BestFitHeap::HeapSize
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

unsigned long BestFitHeap::HeapSize() const
{
	BestFitSegment * segment = fSegments;
	unsigned long heapSize = 0;

	while (segment != NULL)
	{
		heapSize += segment->fSegmentSize;
		segment = segment->fNextSegment;
	}

	return heapSize;
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// BestFitHeap::IsMyBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

Boolean BestFitHeap::IsMyBlock(void* blk) const
{
	Boolean myBlock = false;

	BestFitSegment * segment = fSegments;
	while (segment != NULL && myBlock == false)
	{
		myBlock = segment->AddressInSegment(blk);
		segment = segment->fNextSegment;
	}

	return myBlock;
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// BestFitHeap::Print
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::Print(char* msg) const
{
	PLATFORM_PRINT_STRING(NSPR, warn, ("********** BestFitHeap **********"));
	fFreeTree.PrintTree();
}
#endif

//----------------------------------------------------------------------------------------
// BestFitHeap::AddToFreeBlocks
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::AddToFreeBlocks(BestFitBlock* blk, FreeBlockDisposal doDispose)
{
/*
	Find the segment containing this block.
	if the address of this block is the segment space...
	
	If this size of this block matches the segment's block space size,
	then this block is the entire segment and can be freed.
*/
	Boolean freed = false;
	if (doDispose == kDisposeFreeBlockSegments)
	{
		// find the segment of this block
		Boolean found = false;
		FW_BlockSize blkSize = blk->GetSize();
		BestFitSegment** pseg = &fSegments;
		BestFitSegment* seg = fSegments;
		while (seg && !found)
		{
			long tempSizeG = seg->fSegmentSize + BestFitSegment::kSegmentPrefixSize;
			// if it's the first segment in the block, try to free if it's the only one
			if (seg->fSegmentSpace == blk)
			{
				long tempSize = seg->fSegmentSize - BestFitSegment::kSegmentOverhead;
				if (blkSize == tempSize)
				{
					*pseg = seg->fNextSegment;
					FreeRawMemory (seg);
					return;
				}
				else
					break;
			}
			// or it could be a different segment in the block; skip out
			else if ( (long)seg <= (long)blk && (long)blk 
				< (long)seg + tempSizeG)
				break;
			// or we haven't found the segment yet
			pseg = &seg->fNextSegment;
			seg = *pseg;
		}
		// we have to have found a segment or we have a memory problem
		if (!seg)
			PR_LOG(NSJAVA, warn, ("seg != 0"));

	}
	if (freed)
		PR_LOG(NSJAVA, warn, ("freed == 0"));

	fFreeTree.AddBlock(blk);
}

//----------------------------------------------------------------------------------------
// BestFitHeap::Coalesce: Coalesce blk with the previous block if both are free.
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitBlock* BestFitHeap::Coalesce(BestFitBlock* blk)
{
/*
	This function tries to combine this block into the PREVIOUS block if
	both are free. It should be called CoalesceWithPrevious. If it succeeds
	then it returns the pointer to the previous block, else null.
*/
	BestFitBlock * prevBlk = NULL;

	if (!blk->GetBusy() && !blk->GetPreviousBusy())
	{
#ifdef DEBUG
		if (blk->GetSize() < BestFitBlock::kMinBlockSize)
			DebugStr("\pBestFitHeap::Coalesce: corrupt heap!");
#endif
		prevBlk = *(BestFitBlock **) ((FW_BytePtr) blk - sizeof(FW_BlockSize));
		prevBlk->SetSize(prevBlk->GetSize() + blk->GetSize());
		prevBlk->StuffAddressAtEnd();
	}

	return prevBlk;
}

//----------------------------------------------------------------------------------------
// BestFitHeap::CreateNewSegment
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::CreateNewSegment(unsigned long size)
{
#ifdef BUILD_WIN16
	PLATFORM_ASSERT(size < 64L * 1024L);
#endif
	
	if (size < BestFitSegment::kSegmentOverhead) // CHANGE don't allocate 0-byte segment!
		return;
	
	if (size & 0x01)
		size++;

	BestFitSegment * segment
		= (BestFitSegment *)this->AllocateRawMemory((FW_BlockSize) size);
	if (!segment)
		return; // CHANGE MARK
	// The actual usable space is offset by the size of the fields
	// of a BestFitSegment.

	segment->fSegmentSpace 
		= (void *) ((FW_BytePtr) segment + BestFitSegment::kSegmentPrefixSize);
	segment->fSegmentSize = size - BestFitSegment::kSegmentPrefixSize;

	// Add the segment to the list of segments in this heap.

	segment->fNextSegment = fSegments;
	fSegments = segment;

	// Suffix the segment with a busy block of zero length so that the last free
	// block is not coalesced with a block past the fEnd of the segment.

	void * endOfSegment 
		= (void *) ((FW_BytePtr) segment->fSegmentSpace + segment->fSegmentSize);
	BestFitBlock *blk
		= new((void *) ((FW_BytePtr) endOfSegment - sizeof(BestFitBlock)))
			BestFitBlock(true, false, 0);

	// Create one free block out of this entire segment and Add it to the
	// free block list.

	blk = new(segment->fSegmentSpace)BestFitBlock(false, true, segment->fSegmentSize - sizeof(BestFitBlock));

	this->AddToFreeBlocks (blk, kIsAboutToBeAllocated);
}

//----------------------------------------------------------------------------------------
// BestFitHeap::DeleteSegments
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::DeleteSegments()
{
	BestFitSegment * segment = fSegments;
	while (segment != NULL)
	{
		BestFitSegment * nextSegment = segment->fNextSegment;
		this->FreeRawMemory(segment);
		segment = nextSegment;
	}
}

//----------------------------------------------------------------------------------------
// BestFitHeap::GrowHeap
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::GrowHeap(unsigned long sizeIncrement)
{
#ifdef BUILD_WIN16
	// To avoid crossing 64K boundries on pointer arithmatic, the size of each segment
	// must be limited to 64K. So a single grow request may result in more than one
	// segment being allocated.
	
	const unsigned long kSegmentSizeLimit = 1024L * 62L;     // Allow for 2K overhead
	
	while (sizeIncrement > 0)
	{
		unsigned long segmentSize = sizeIncrement;
		if (segmentSize > kSegmentSizeLimit)
			segmentSize = kSegmentSizeLimit;
		this->CreateNewSegment(segmentSize);
		sizeIncrement -= segmentSize;
	}
#else
	// CHANGE don't allocate the last fDontUse bytes of memory!
	if (fDontUse) { // if 0 don't do any checking (i.e. allow GrowZone to be called)
		
		size_t totalFreeMem = FreeMem();
		
		signed long howMuchBedrockIsAllowed = totalFreeMem - fDontUse;
		if (howMuchBedrockIsAllowed < 0) // it's possible someone else ate free memory into the Dead Zone
			howMuchBedrockIsAllowed = 0;
		
		size_t howMuchAnyoneIsAllowed = MaxBlock();
		if (howMuchBedrockIsAllowed > howMuchAnyoneIsAllowed) // if we try to allocate more it'll trigger GrowZone
			howMuchBedrockIsAllowed = howMuchAnyoneIsAllowed;
		
		PR_ASSERT (howMuchBedrockIsAllowed < 500 * 1024 * 1024); // completely bad number
		
		if (sizeIncrement > howMuchBedrockIsAllowed) // we're trying to allocate into the Dead Zone
			return;
	}
	PR_ASSERT (0 <= sizeIncrement && sizeIncrement <= 100 * 1024 * 1024);
	this->CreateNewSegment(sizeIncrement);
#endif
}

//----------------------------------------------------------------------------------------
// BestFitHeap::RemoveFromFreeBlocks
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::RemoveFromFreeBlocks(BestFitBlock* blk)
{
	fFreeTree.RemoveBlock(blk);
}

//----------------------------------------------------------------------------------------
// BestFitHeap::SearchFreeBlocks
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitBlock* BestFitHeap::SearchFreeBlocks(FW_BlockSize size)
{
	//all blocks are of even length, so round up to fNext even number

	if (size & 1)
		size++;

	return fFreeTree.SearchForBlock(size, NULL, NULL);
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// BestFitHeap::CompilerCheck
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void BestFitHeap::CompilerCheck()
{
	MemoryHeap::CompilerCheck();
	
	char buffer[sizeof(BestFitBlock) + sizeof(void *)];
	BestFitBlock &block = *((BestFitBlock *) buffer);
		
	// Check to make sure the bit field setters and getters work.
	
	block.SetBlockType(3);
	block.SetBusy(true);
	block.SetPrevBusy(false);
#ifndef BUILD_WIN16
	block.SetSize(100201);
#endif
	block.SetMagicNumber(3);

	PLATFORM_ASSERT(block.GetBlockType() == 3);
	PLATFORM_ASSERT(block.GetBusy() == true);
	PLATFORM_ASSERT(block.GetPreviousBusy() == false);
#ifndef BUILD_WIN16
	PLATFORM_ASSERT(block.GetSize() == 100201);
#endif
	PLATFORM_ASSERT(block.GetMagicNumber() == 3);
	
	block.SetBlockType(2);
	block.SetBusy(false);
	block.SetPrevBusy(true);
	block.SetSize(150);
	block.SetMagicNumber(2);

	PLATFORM_ASSERT(block.GetBlockType() == 2);
	PLATFORM_ASSERT(block.GetBusy() == false);
	PLATFORM_ASSERT(block.GetPreviousBusy() == true);
	PLATFORM_ASSERT(block.GetSize() == 150);
	PLATFORM_ASSERT(block.GetMagicNumber() == 2);
}
#endif


//========================================================================================
// CLASS FreeBlockTree
//========================================================================================

//----------------------------------------------------------------------------------------
// FreeBlockTree::FreeBlockTree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FreeBlockTree::FreeBlockTree() :
	fRoot(true, true, 0)
{
	fRoot.SetRight(NULL);
	fRoot.SetLeft(NULL);
}

//----------------------------------------------------------------------------------------
// FreeBlockTree::FreeBlockTree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FreeBlockTree::FreeBlockTree(const FreeBlockTree& blk) :
	fRoot(blk.fRoot)
{
}

//----------------------------------------------------------------------------------------
// FreeBlockTree::operator=
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

FreeBlockTree& FreeBlockTree::operator=(const FreeBlockTree& blk)
{
	fRoot = blk.fRoot;
	return (*this);
}

//----------------------------------------------------------------------------------------
// FreeBlockTree::AddBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::AddBlock(BestFitBlock* blk)
{	
#ifdef DEBUG_MEMORY
	this->CheckTree();

	if (gIntenseDebugging)
	{
		PLATFORM_PRINT_STRING(NSPR, warn, ("FreeBlockTree::AddBlock Entry-------------------------------------"));
		this->PrintTree();
	}
#endif

	BestFitBlock * parent;

	this->SearchForBlock(blk->GetSize(), blk, &parent);

	blk->SetRight(NULL);
	blk->SetLeft(NULL);
	blk->SetParent(parent);

	if (*blk > *parent)
		parent->SetRight(blk);
	else
		parent->SetLeft(blk);

#ifdef DEBUG_MEMORY
	this->CheckTree();

	if (gIntenseDebugging)
	{
		PLATFORM_PRINT_STRING(NSPR, warn, ("FreeBlockTree::AddBlock Exit"));
		this->PrintTree();
	}
#endif
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// FreeBlockTree::CheckTree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::CheckTree() const
{
	this->CheckTreeHelper(&((FreeBlockTree *)this)->fRoot);
}
#endif

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// FreeBlockTree::PrintTree
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::PrintTree() const
{
	this->PrintTreeHelper(&((FreeBlockTree *)this)->fRoot);
}
#endif

//----------------------------------------------------------------------------------------
// FreeBlockTree::RemoveBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::RemoveBlock(BestFitBlock* blk)
{
#ifdef DEBUG_MEMORY
	this->CheckTree();

	if (gIntenseDebugging)
	{
		PLATFORM_PRINT_STRING(NSPR, warn, ("FreeBlockTree::RemoveBlock Entry----------------------------------"));
		this->PrintTree();
	}
#endif

	BestFitBlock * spliceOutBlk;

	// Decide which block to splice out of the tree. Either the block being
	// removed, or its successor block in the tree.

	if (blk->GetLeft() == NULL || blk->GetRight() == NULL)
		spliceOutBlk = blk;
	else
		spliceOutBlk = this->GetSuccessorBlk(blk);

	// Splice the node out of the tree

	BestFitBlock * tmp;
	BestFitBlock * parent = spliceOutBlk->GetParent();
	if (spliceOutBlk->GetLeft())
		tmp = spliceOutBlk->GetLeft();
	else
		tmp = spliceOutBlk->GetRight();
	if (tmp)
		tmp->SetParent(parent);
	if (spliceOutBlk == parent->GetLeft())
		parent->SetLeft(tmp);
	else
		parent->SetRight(tmp);

	// If we spliced out the successor to blk above then we need to patch the successor
	// back in, in Place of blk.

	if (spliceOutBlk != blk)
	{
		BestFitBlock * parent = blk->GetParent();

		// Fix up the parent's child pointer.

		if (parent->GetLeft() == blk)
			parent->SetLeft(spliceOutBlk);
		else
			parent->SetRight(spliceOutBlk);

		// Fix up the splice in block's pointers.

		spliceOutBlk->SetParent(parent);
		spliceOutBlk->SetLeft(blk->GetLeft());
		spliceOutBlk->SetRight(blk->GetRight());

		// Fix up the children of the splice in block's parent pointers.

		if (spliceOutBlk->GetLeft())
			(spliceOutBlk->GetLeft())->SetParent(spliceOutBlk);
		if (spliceOutBlk->GetRight())
			(spliceOutBlk->GetRight())->SetParent(spliceOutBlk);
	}

#ifdef DEBUG_MEMORY
	this->CheckTree();

	if (gIntenseDebugging)
	{
		PLATFORM_PRINT_STRING(NSPR, warn, ("FreeBlockTree::RemoveBlock Exit"));
		this->PrintTree();
	}
#endif
}

//----------------------------------------------------------------------------------------
// FreeBlockTree::SearchForBlock
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitBlock* FreeBlockTree::SearchForBlock(FW_BlockSize size,
											void* blk,
											BestFitBlock** insertLeaf)
{
#ifdef DEBUG_MEMORY
	this->CheckTree();

	if (gIntenseDebugging)
	{
		char str[80];
		sprintf(str, "FreeBlockTree::SearchForBlock(%d)---------------------------------\n", size);
		PLATFORM_PRINT_STRING(NSPR, warn, (str));
		this->PrintTree();
	}
#endif

	long minDiff = LONG_MAX;
	BestFitBlock * minDiffBlock = NULL;
	BestFitBlock * currentBlock = &fRoot;
	do
	{
		long diff = currentBlock->GetSize() - size;
		if (diff >= 0 && diff < minDiff)
		{
			minDiffBlock = currentBlock;
			minDiff = diff;
		}

		if (insertLeaf)
			*insertLeaf = currentBlock;

		// Determine which branch of the tree to continue down. Since duplicate keys are
		// difficult to deal with in binary trees, we use the address of blocks to break
		// ties, in the case of two blocks whose size are equal.

		if (size < currentBlock->GetSize())
			currentBlock = currentBlock->GetLeft();
		else if (size > currentBlock->GetSize())
			currentBlock = currentBlock->GetRight();
		else if (blk)
		{
			if (blk <= currentBlock)
				currentBlock = currentBlock->GetLeft();
			else
				currentBlock = currentBlock->GetRight();
		}
		else
			currentBlock = currentBlock->GetLeft();

	} while (currentBlock != NULL);

	return minDiffBlock;
}

//----------------------------------------------------------------------------------------
// FreeBlockTree::TreeInfo
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::TreeInfo(unsigned long& bytesFree,
							 unsigned long& numberOfNodes) const
{
	bytesFree = numberOfNodes = 0;
	int maxdepth =
	this->TreeInfoHelper(&((FreeBlockTree *)this)->fRoot, bytesFree, numberOfNodes);

	// Subtract one from the number of nodes to account for the header node which
	// is always empty.

	numberOfNodes--;

	PR_LOG(NSJAVA, warn, ("TreeInfo: %i nodes %i depth",numberOfNodes,maxdepth));
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// FreeBlockTree::CheckTreeHelper
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::CheckTreeHelper(BestFitBlock* blk) const
{
	if (blk == NULL)
		return;

	ASSERT_POINTER(blk);
	
	if (blk->GetLeft())
	this->CheckTreeHelper(blk->GetLeft());

	BestFitBlock * parent = blk->GetParent();
	if (parent != NULL)
	{
		if (parent->GetRight() == blk)
		{
			if (*blk < *parent)
			{
				this->PrintTree();
//				PLATFORM_PRINT_STRING (PR_LOG_NSPR, PR_LOG_notice, "%p (%i) < %p (%i)\n", blk, blk->GetSize(), parent, parent->GetSize());
//				XP_ABORT(("FreeBlockTree::CheckTreeHelper"
//											 "Corrupt free list tree: "
//											 "right child less than parent."));
			}
		}
		else if (parent->GetLeft() == blk)
		{
			if (*blk > *parent)
			{
				this->PrintTree();
//				PLATFORM_PRINT_STRING ("%p (%i) > %p (%i)\n", blk, blk->GetSize(), parent, parent->GetSize());
//				XP_ABORT(("FreeBlockTree::CheckTreeHelper"
//											 "Corrupt free list tree: "
//											 "left child greater than parent."));
			}
		}
		else
		{
				this->PrintTree();
//				PLATFORM_PRINT_STRING ("%p ~= %p\n", blk, parent);
//				XP_ABORT(("FreeBlockTree::CheckTreeHelper"
//											 "Corrupt free list tree: "
//											 "I am not my parent's child."));
		}
	}

	// AMB_TODO: Recursing down the right side of the tree trashes the tree. This is
	//			 very strange, and as of yet I don't understand what the problem is. I
	//			 had suspected stack over-run.
	if (blk->GetRight())
	this->CheckTreeHelper(blk->GetRight());
}
#endif

//----------------------------------------------------------------------------------------
// FreeBlockTree::GetSuccessorBlk
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

BestFitBlock* FreeBlockTree::GetSuccessorBlk(BestFitBlock* blk)
{
	if (blk->GetRight())
	{
		BestFitBlock * current = blk->GetRight();
		while (current->GetLeft())
			current = current->GetLeft();
		return current;
	}
	else
	{
		BestFitBlock * parent = blk->GetParent();
		BestFitBlock * current = NULL;
		while (parent != NULL && current == parent->GetRight())
		{
			current = parent;
			parent = parent->GetParent();
		}
		return parent;
	}
}

#ifdef DEBUG
//----------------------------------------------------------------------------------------
// FreeBlockTree::PrintTreeHelper
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

void FreeBlockTree::PrintTreeHelper(BestFitBlock* blk,
										short level) const
{
	for (int i = 0; i < level; i++)
		PLATFORM_PRINT_STRING(NSPR, warn, ("\t"));

	if (blk == NULL)
	{
		PLATFORM_PRINT_STRING(NSPR, warn, ("NULL"));
		return;
	}

	char str[80];
	sprintf(str, "Block=%lx fSize=%ld\n", blk, blk->GetSize());
	PLATFORM_PRINT_STRING(NSPR, warn, (str));

	this->PrintTreeHelper(blk->GetLeft(), level + 1);
	this->PrintTreeHelper(blk->GetRight(), level + 1);
}
#endif

//----------------------------------------------------------------------------------------
// FreeBlockTree::TreeInfoHelper
//----------------------------------------------------------------------------------------
#pragma segment HeapSeg

int FreeBlockTree::TreeInfoHelper(BestFitBlock* blk,
									   unsigned long& bytesFree,
									   unsigned long& numberOfNodes) const
{
	if (blk == NULL)
		return 0;

	int ldepth = 
	this->TreeInfoHelper(blk->GetLeft(), bytesFree, numberOfNodes);

	numberOfNodes++;
	bytesFree += blk->GetSize();
	
	int rdepth = 
	this->TreeInfoHelper(blk->GetRight(), bytesFree, numberOfNodes);
	
	return 1 + (ldepth > rdepth? ldepth: rdepth);
}

#ifdef PROFILE
#pragma profile off
#endif


//	===========================================================================
//	LStack.t						©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<PP_Prefix.h>
#include	"LStack.h"

template <class T>
LStack<T>::LStack(Boolean inKeepStackLocked)
:	LArray(sizeof(T))
/*
	WARNING!

	If inKeepStackLocked is false, references returned are valid only until
	the next memory move.
*/
{
	if (inKeepStackLocked)
		HLock(mItemsH);
}

template <class T>
LStack<T>::~LStack()
{
	while (Depth())
		Pop();
}

template <class T>
void	LStack<T>::Push(const T &inItem)
{
	Boolean	locked = (HGetState(mItemsH) & 0x80) != 0;

	Try_ {
		if (locked)
			HUnlock(mItemsH);

		InsertItemsAt(1, Depth()+1, NULL);

		if (locked)
			HLock(mItemsH);
			
	} Catch_(inErr) {

		if (locked)
			HLock(mItemsH);
			
		Throw_(inErr);
			
	} EndCatch_;

	Top() = inItem;
}

template <class T>
void	LStack<T>::Push(void)
/*
	Push a new object on that stack that was constructed
	with the default constructor.
*/
{
	Push(T());
}

template <class T>
void	LStack<T>::Pop(void)
{
	if (!Depth())
		Throw_(paramErr);
	
	Top().~T();			//	Call destructor
	
	RemoveItemsAt(1, Depth());
}

template <class T>
T&	LStack<T>::Top(void) const
/*
	If !inKeepStackLocked, reference is valid only until next memory move!
*/
{
	return PeekBack(0);
}

template <class T>
T&	LStack<T>::PeekBack(Int32 inDistance) const
/*
	If !inKeepStackLocked reference is valid only until next memory move!
*/
{
	Int32	index = Depth() - inDistance;
	
	if (index <= 0)
		Throw_(paramErr);	//	Stack isn't that deep!
	
	return *((T *)GetItemPtr(index));	
}


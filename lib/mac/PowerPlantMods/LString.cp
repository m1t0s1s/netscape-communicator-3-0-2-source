// ===========================================================================
//	LString.cp						©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Classes for handling Pascal Strings
//
//	LString		- Abstract base class
//	LStr255		- Wrapper class for Str255
//	TString<T>	- Template class for any string type (array of unsigned char)

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LString.h>


#pragma mark === LString Class ===

// ===========================================================================
//	¥ LString
// ===========================================================================
//	Abstract base class for a Pascal-style string

#pragma mark --- Conversions ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator Int32
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Conversion to a long integer

LString::operator Int32() const
{
	Int32	number;
	::StringToNum(mStringPtr, &number);
	return number;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator double_t
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Conversion to a floating point number
/*
LString::operator double_t() const
{
	double_t	theDouble = 0;
	decimal		decNumber;
	Int16		ix = 1;
	Int16		vp;
	
		// The str2dec routine expects a C string, but LString holds
		// a Pascal string. If there is room, we null terminate
		// the Pascal string. If the string is the max length, we
		// copy the string into a local array and then terminate it.
	
	if ((mStringPtr[0] + 1) < mMaxBytes) {	
		mStringPtr[mStringPtr[0] + 1] = 0;
		::str2dec((char*) mStringPtr, &ix, &decNumber, &vp);
		
	} else {						// String is max size
		char	cstr[256];
		::BlockMoveData(mStringPtr+1, cstr, mStringPtr[0]);
		cstr[mStringPtr[0]] = 0;
		ix = 0;
		::str2dec(cstr, &ix, &decNumber, &vp);
	}

	if (vp) {						// Conversion was successful
		theDouble = ::dec2num(&decNumber);
	}
	
	return theDouble;
}
*/

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator FourCharCode
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Conversion to a four character code

LString::operator FourCharCode() const
{
	FourCharCode	theCode;
	::BlockMoveData(mStringPtr + 1, &theCode, sizeof(FourCharCode));
	return theCode;
}


#pragma mark --- Assignment ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( LString&, Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assigment from a substring of another String object
//
//	Use inStartPos and inCount to specify the substring of inString to
//	copy into this String. The default values of these parameters
//	will copy all of inString.
//
//	If inStartPos is beyond the end of inString or inCount is zero,
//	this String is made empty. If inCount extends beyong the end of
//	inString, only bytes from inStartPos to the end of inString
//	are copied.

LString&
LString::Assign(
	const LString&	inString,
	Uint8			inStartPos,
	Uint8			inCount)
{
	if ((inStartPos > inString.Length()) || (inCount == 0)) {
										// Substring to copy is empty
		mStringPtr[0] = 0;				//   so make this String empty
		
	} else {
										// If count is too big, limit it
										//   to the end of the String
		if ((inStartPos + inCount - 1) > inString.Length()) {
			inCount = inString.Length() - inStartPos + 1;
		}
		
		LoadFromPtr(&inString[inStartPos], inCount);
	}
	
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( Uchar )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assigment from a character

LString&
LString::Assign(
	Uchar	inChar)
{
	mStringPtr[0] = 1;					// 1-character string
	mStringPtr[1] = inChar;
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Set String contents from a pointer to characters and a length

LString&
LString::Assign(
	const void*		inPtr,
	Uint8			inLength)
{
	LoadFromPtr(inPtr, inLength);
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( Handle )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Set String contents from the data in a Handle

LString&
LString::Assign(
	Handle		inHandle)
{
	if (inHandle != nil) {				// LoadFromPtr doesn't move memory
		LoadFromPtr((Uint8*)*inHandle, GetHandleSize(inHandle));
	
	} else {
		mStringPtr[0] = 0;
	}
	
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( ResIDT, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Set String contents from a 'STR ' or 'STR#' resource
//
//	The value of inIndex determines whether to use a 'STR ' or 'STR#'.
//	Uses 'STR ' for inIndex <= 0, and 'STR#' for inIndex >= 1. 

LString&
LString::Assign(
	ResIDT		inResID,
	Int16		inIndex)
{
	if (inIndex <= 0) {
		LoadFromSTRResource(inResID);
	} else {
		LoadFromSTRListResource(inResID, inIndex);
	}
	
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( Int32 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assignment from a long integer

LString&
LString::Assign(
	Int32		inNumber)
{
	Str255	numStr;
	::NumToString(inNumber, numStr);
	Assign(numStr + 1, numStr[0]);
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( double_t, Int8, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assignment from floating point number
//
//	inStyle must be FLOATDECIMAL or FIXEDDECIMAL
//
//	For FLOATDECIMAL, inDigits is the number of significant digits
//		(should be > 0)
//	For FIXEDDECIMAL, inDigits is the number of digits to the right
//		of the decimal point
/*
LString&
LString::Assign(
	double_t		inNumber,
	Int8			inStyle,
	Int16			inDigits)
{
		// In order to convert a floating point number to a string,
		// we first have to convert the number to a "decimal", which
		// is a struct used to describe a number.
		
	decform	theDecform;
	theDecform.style = inStyle;
	theDecform.digits = inDigits;
	decimal	decNumber;
	
	::num2dec(&theDecform, inNumber, &decNumber);
	
		// The Toolbox dec2str() routine takes a "decimal" number
		// and converts it into a C string that can be at most
		// DECSTROUTLEN characters (we add 1 for the null terminator).
	
	char	numStr[DECSTROUTLEN + 1];
	::dec2str(&theDecform, &decNumber, numStr);
	
	Assign(numStr, CStringLength(numStr));		// Assign from a C string
	return *this;
}
*/

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Assign ( FourCharCode )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assignment from a four character code

LString&
LString::Assign(
	FourCharCode	inCode)
{
	LoadFromPtr(&inCode, sizeof(FourCharCode));
	return *this;
}

#pragma mark --- Append ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Append ( Uchar )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Append a character to this String

LString&
LString::Append(
	Uchar	inChar)
{
	if (mStringPtr[0] < (mMaxBytes - 1)) {
		mStringPtr[++mStringPtr[0]] = inChar;
	}
	
	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Append ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Append characters to the end of this String

LString&
LString::Append(
	const void*		inPtr,
	Uint8			inLength)
{
	Int16	charsToCopy = inLength;			// Limit to max size of String
	if ((mStringPtr[0] + charsToCopy + 1) > mMaxBytes) {
		charsToCopy = mMaxBytes - mStringPtr[0] - 1;
	}
	
	::BlockMoveData(inPtr, mStringPtr + mStringPtr[0] + 1, charsToCopy );
	
	mStringPtr[0] += charsToCopy;			// Adjust length byte
	
	return *this;
}

#pragma mark --- Search for Substring ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Find ( const void*, Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return the index of where a substring starts within this String
//
//	Returns 0 if the substring is not in the String
//	inStartPos is the index within the String at which to begin searching.
//		The default value is 1. Value 0 is not valid.
//
//	To find the next occurrence, set inStartPos to one plus the index
//	returned by the last Find.

Uint8
LString::Find(
	const void*		inPtr,
	Uint8			inLength,
	Uint8			inStartPos) const
{
	Uint8	startIndex = 0;
										// Last possible index depends on
										//   relative lengths of the strings
	Int16	lastChance = Length() - inLength + 1;
	
										// Search forward from starting pos
	for (Int16 i = inStartPos; i <= lastChance; i++) {
		if ((*mCompareFunc)(mStringPtr + i, inPtr, inLength, inLength) == 0) {
			startIndex = i;
			break;
		}
	}
	
	return startIndex;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ ReverseFind ( const void*, Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return the index of where a substring starts within this String, searching
//	from the end of the String
//
//	Returns 0 if the substring is not in the String
//	inStartPos is the index within the String at which to begin searching.
//		The default value is 255. Value 0 is not valid.
//
//	To find the previous occurrence, set inStartPos to the index returned
//	by the last ReverseFind minus 1.

Uint8
LString::ReverseFind(
	const void*		inPtr,
	Uint8			inLength,
	Uint8			inStartPos) const
{
	Uint8	startIndex = 0;
										// Highest possible index depends on
										//   relative lengths of the strings
	Int16	lastChance = Length() - inLength + 1;
	if (lastChance > 0) {				// Substring can't be longer
		if (inStartPos > lastChance) {	// Don't bother looking after lastChance
			inStartPos = lastChance;	
		}
										// Search backward from starting pos
		for (Int16 i = inStartPos; i >= 1; i--) {
			if ((*mCompareFunc)(mStringPtr + i, inPtr, inLength, inLength) == 0) {
				startIndex = i;
				break;
			}
		}
	}
			
	return startIndex;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ BeginsWith ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return whether this String begins with a specified String

Boolean
LString::BeginsWith(
	const void*		inPtr,
	Uint8			inLength) const
{
										// Compare first "inLength" bytes
										//   of String with input
	return (inLength <= Length()) &&
		   ((*mCompareFunc)(mStringPtr + 1, inPtr, inLength, inLength) == 0);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ EndsWith ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return whether this String ends with a specified String

Boolean
LString::EndsWith(
	const void*		inPtr,
	Uint8			inLength) const
{
										// Compare last "inLength" bytes
										//   of String with input
	return (inLength <= Length()) &&
		   ((*mCompareFunc)(mStringPtr + Length() - inLength + 1, inPtr,
		   					inLength, inLength) == 0);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ FindWithin ( const void*, Uint32, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return the index of where this String starts within another String
//
//	Returns 0 if the substring is not in the String
//	inStartPos is the index within inPtr at which to begin searching.
//		The default value is 1. Value 0 is not valid.
//
//	To find the next occurrence, set inStartPos to one plus the index
//	returned by the last FindWithin.

Uint32
LString::FindWithin(
	const void*		inPtr,
	Uint32			inLength,
	Uint32			inStartPos) const
{
	Uint32	startIndex = 0;
	
										// inStartPos of zero is not valid and
										//   substring can't be longer
	if ( (inStartPos > 0) && (inLength >= Length()) ) {
	
										// Highest possible offset depends on
										//   relative lengths of the strings
		Uint32	lastOffset = inLength - Length();
		
										// Search forward after starting pos
		for (Uint32 offset = inStartPos - 1; offset <= lastOffset; offset++) {
			if ((*mCompareFunc)(mStringPtr + 1, (Uint8*) inPtr + offset, Length(), Length()) == 0) {
				startIndex = offset + 1;
				break;
			}
		}
	}

	return startIndex;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ ReverseFindWithin ( const void*, Uint32, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return the index of where this String starts within another String,
//	searching from the end of the String
//
//	Returns 0 if this String is not within the input string
//	inStartPos is the index within inPtr at which to begin searching.
//		Value 0 is not valid.
//
//	To find the previous occurrence, set inStartPos to the index returned
//	by the last ReverseFindWithin minus 1.

Uint32
LString::ReverseFindWithin(
	const void*		inPtr,
	Uint32			inLength,
	Uint32			inStartPos) const
{
	Uint32	startIndex = 0;
	
										// inStartPos of zero is not valid and
										//   substring can't be longer
	if ( (inStartPos > 0) && (inLength >= Length()) ) {
										// Highest possible offset depends on
										//   relative lengths of the strings
		Uint32	lastOffset = inLength - Length();
		
										// We start from the lesser of lastOffset
		if (inStartPos <= lastOffset) {	//   and inStartPos - 1
			lastOffset = inStartPos - 1;
		}
	
										// Search backward from last offset
		Uint32	offset = lastOffset + 1;
		do {
			if ((*mCompareFunc)(mStringPtr + 1, (Uint8*) inPtr + (--offset), Length(), Length()) == 0) {
				startIndex = offset + 1;
				break;
			}
		} while (offset > 0);
	}
			
	return startIndex;
}

#pragma mark --- Copy Substring ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator () ( Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return a LStr255 String object containing the specified substring
//	of this String

LStr255
LString::operator()(
	Uint8	inStartPos,
	Uint8	inCount) const
{
	LStr255	subString;
	
	if (inStartPos <= Length()) {
		if (inStartPos == 0) {			// Interpret 0 or 1 as the first
			inStartPos = 1;				//    character
		}
		
										// If count is too big, limit it
										//   to the end of the String
		if ((inStartPos + inCount - 1) > Length()) {
			inCount = Length() - inStartPos + 1;
		}
		
		subString.Assign(mStringPtr + inStartPos, inCount);
	}
		
	return subString;
}

#pragma mark --- Changing Substrings ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Insert ( const void*, Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Insert a string into this String starting at the specified index

LString&
LString::Insert(
	const void*		inPtr,
	Uint8			inLength,
	Uint8			inAtIndex)
{
	if (inAtIndex == 0) {				// Interpret 0 or 1 as the start
		inAtIndex = 1;					//   of the String
	} else if (inAtIndex > Length() + 1) {
		inAtIndex = Length() + 1;		// Interpret any index past end
										//   to mean to append the string
	}

										// Don't insert more than bytes
										//    than this String can hold
	if ((inLength + Length() + 1) > mMaxBytes) {
		inLength = mMaxBytes - Length() - 1;
	}
	
	if (inAtIndex <= Length()) {		// Shift up bytes after insertion
		::BlockMoveData(mStringPtr + inAtIndex,
						mStringPtr + inAtIndex + inLength,
						Length() - inAtIndex + 1);
	}
	
										// Copy input string
	::BlockMoveData(inPtr, mStringPtr + inAtIndex, inLength);
	
	mStringPtr[0] += inLength;			// Adjust length byte

	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Remove ( Uint8, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Remove a given number of bytes from this String starting at the
//	specified index

LString&
LString::Remove(
	Uint8	inStartPos,
	Uint8	inCount)
{
	if (inStartPos <= Length()) {
		if (inStartPos == 0) {			// Interpret 0 or 1 as the first
			inStartPos = 1;				//    character
		}
		
										// If count is too big, limit it
										//   to the end of the String
		if ((inStartPos + inCount - 1) > Length()) {
			inCount = Length() - inStartPos + 1;
		}
		
		if ((inStartPos + inCount) <= Length()) {
										// Shift down bytes after removed one
			::BlockMoveData(mStringPtr + inStartPos + inCount,
							mStringPtr + inStartPos,
							Length() - inStartPos - inCount + 1);
		}
		
		mStringPtr[0] -= inCount;		// Adjust length byte
	}

	return *this;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ Replace ( Uint8, Uint8, const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Replace a given number of bytes in this String starting at the
//	specified index with the another string, specified a pointer and length

LString&
LString::Replace(
	Uint8			inStartPos,			// First byte to replace
	Uint8			inCount,			// Number of bytes to replace
	const void*		inPtr,				// Ptr to replacement bytes
	Uint8			inLength)			// Number of replacement bytes
{
	Remove(inStartPos, inCount);		// Replace same as Remove & Insert
	Insert(inPtr, inLength, inStartPos);

	return *this;
}

#pragma mark --- Constructor ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LString
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a max length and a string pointer
//
//	Subclasses must call this "protected" constructor to specify the
//	maximum string size (including the length byte) and a pointer to
//	the storage for the string

LString::LString(
	Int16		inMaxBytes,
	StringPtr	inStringPtr)
		:	mStringPtr(inStringPtr),
			mMaxBytes(inMaxBytes)
{
	mCompareFunc = ToolboxCompareText;
}

#pragma mark --- Comparison Functions ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ ToolboxCompareText [static]
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	String comparison function that uses the Toolbox CompareText() routine

Int16
LString::ToolboxCompareText(
	const void*		inLeft,
	const void*		inRight,
	Uint8			inLeftLength,
	Uint8			inRightLength)
{
	return ::CompareText(inLeft, inRight, inLeftLength, inRightLength, nil);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CompareBytes [static]
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Simple byte-by-byte value comparison of two strings

Int16
LString::CompareBytes(
	const void*		inLeft,
	const void*		inRight,
	Uint8			inLeftLength,
	Uint8			inRightLength)
{
	Uint8	n = inLeftLength;				// Determine shortest length
	if (inRightLength < inLeftLength) {
		n = inRightLength;
	}
	
	const Uint8	*leftP = (Uint8*) inLeft;
	const Uint8 *rightP = (Uint8*) inRight;
	
	while (n > 0) { 						// Compare up to shortest length
		if (*leftP != *rightP) {
			if (*leftP > *rightP) {	
				return 1;					// Left is greater
			}
			return -1;						// Left is smaller
		}
		
		leftP++;
		rightP++;
		n--;
	}
	
		// All bytes up to shortest length match. Therefore, the
		// longer of the two string is the greater one. If they
		// have the same length, then they are equal.
		
	if (inLeftLength > inRightLength) {
		return 1;
	
	} else if (inLeftLength < inRightLength) {
		return -1;
	}
				
	return 0;								// String are equal
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CompareIgnoringCase [static]
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Case-insensitve string comparison

Int16
LString::CompareIgnoringCase(
	const void*		inLeft,
	const void*		inRight,
	Uint8			inLeftLength,
	Uint8			inRightLength)
{
		// Copy both left and right strings and convert all characters
		// to uppercase before performing comparison.

	LStr255		leftStr(inLeft, inLeftLength);
	::UppercaseText((Ptr) &leftStr[1], leftStr[0], smSystemScript);
	
	LStr255		rightStr(inRight, inRightLength);
	::UppercaseText((Ptr) &rightStr[1], rightStr[0], smSystemScript);

	return ::CompareText(&leftStr[1], &rightStr[1],
						 leftStr[0], rightStr[0], nil);
}

#pragma mark --- Public Utilities ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ CStringLength
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Return the length of a C string, but with an upper limit of 255

Uint8
LString::CStringLength(
	const char*		inCString)
{
		// Find length of C string by searching for the terminating
		// null character. However, don't bother to look past 255
		// characters, since that's all that a Pascal string can hold.
		
	Uint8	strLength = 255;
	for (Uint8 i = 0; i < 255; i++) {
		if (inCString[i] == '\0') {
			strLength = i;
			break;
		}
	}
	
	return strLength;
}


// ---------------------------------------------------------------------------
//		¥ CopyPStr
// ---------------------------------------------------------------------------
//	Copy a Pascal string
//
//	Returns a pointer to the copied string
//
//	inDestSize specifies the maximum size of the destination (INCLUDING
//		the length byte). The default is sizeof Str255.
//
//	Call this function as follows:
//
//		CopyPStr(source, dest, sizeof(dest));

StringPtr
LString::CopyPStr(
	ConstStringPtr	inSourceString,
	StringPtr		outDestString,
	Int16			inDestSize)
{
	Int16	dataLen = inSourceString[0] + 1;
	if (dataLen > inDestSize) {
		dataLen = inDestSize;
	}
	
	::BlockMoveData(inSourceString, outDestString, dataLen);
	outDestString[0] = dataLen - 1;
	return outDestString;
}


// ---------------------------------------------------------------------------
//		¥ AppendPStr
// ---------------------------------------------------------------------------
//	Append two Pascal strings. The first string becomes the combination
//	of the first and second strings.
//
//	Returns a pointer to the resulting string
//
//	inDestSize specifies the maximum size of the base string
//		(INCLUDING the length byte). The default is sizeof Str255.
//
//	Call this function as follows:
//
//		AppendPStr(ioString, appendMe, sizeof(ioString));

StringPtr
LString::AppendPStr(
	Str255				ioBaseString,
	ConstStr255Param	inAppendString,
	Int16				inDestSize)
{
								// Limit combined string to inDestSize chars
    Int16	charsToCopy = inAppendString[0];
    if (ioBaseString[0] + charsToCopy > inDestSize - 1) {
    	charsToCopy = inDestSize - 1 - ioBaseString[0];
    }

								// Copy second to end of first string
    ::BlockMoveData(inAppendString + 1,  ioBaseString + ioBaseString[0] + 1,
    					charsToCopy);
    							// Set length of combined string
    ioBaseString[0] += charsToCopy;
    
    return ioBaseString;
}


// ---------------------------------------------------------------------------
//		¥ FourCharCodeToPStr
// ---------------------------------------------------------------------------
//	Convert a four character code to a Pascal string and return a pointer
//	to the string
//
//	outString must point to a buffer (usually a string) that can hold
//	the resulting string, including the length byte. Therefore, the buffer
//	must be at least 5-bytes large. Usually, you will allocate an array
//	of 5 unsigned char's.

StringPtr
LString::FourCharCodeToPStr(
	FourCharCode	inCode,
	StringPtr		outString)
{
	::BlockMoveData(&inCode, outString + 1, sizeof(FourCharCode));
	outString[0] = sizeof(FourCharCode);
	return outString;
}


// ---------------------------------------------------------------------------
//		¥ PStrToFourCharCode
// ---------------------------------------------------------------------------
//	Convert an Pascal string to a four character code

void
LString::PStrToFourCharCode(
	ConstStringPtr	inString,
	FourCharCode	&outCode)
{
	::BlockMoveData(inString + 1, &outCode, sizeof(OSType));
}

#pragma mark --- Protected Utilities ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LoadFromPtr
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Fill in the String given a pointer and a byte count

void
LString::LoadFromPtr(
	const void*		inPtr,
	Uint8			inLength)
{
	if (inLength > mMaxBytes - 1) {		// mMaxBytes includes a length byte
		inLength = mMaxBytes - 1;		// Enforce maximum string length
	}
	
	::BlockMoveData(inPtr, mStringPtr + 1, inLength);
	mStringPtr[0] = inLength;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LoadFromSTRListResource
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Fill in the String from a STR# resource

void
LString::LoadFromSTRListResource(
	ResIDT	inResID,
	Int16	inIndex)
{
										// A string from an STR# resource
										//   can have up to 255 characters.
	if (mMaxBytes >= sizeof(Str255)) {
										// String is big enough to hold it
		::GetIndString(mStringPtr, inResID, inIndex);
		
	} else {							// String may be too small
		Str255 theTempString;			// Read chars from resource into
										//   a buffer, then load the String
										//   from that buffer (which will
										//   truncate if the String is
										//   too small
		::GetIndString(theTempString, inResID, inIndex);
		LoadFromStringPtr(theTempString);
	}
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LoadFromSTRResource
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Fill in the String from a STR resource

void
LString::LoadFromSTRResource(
	ResIDT	inResID)
{
	StringHandle	strH = ::GetString(inResID);
	if (strH != nil) {
		LoadFromStringPtr(*strH);
		
	} else {
		SignalPStr_("\pSTR resource not found");
		
		mStringPtr[0] = 0;				// Empty String
	}
}

#pragma mark --- operator + ---

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator+ (LString&, LString&)
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Concatenate two String objects
//		leftString + rightString

LStr255
operator+(
	const LString&	inLeftString,
	const LString&	inRightString)
{
	LStr255	resultString = inLeftString;
	return resultString += inRightString;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator+ (LString&, ConstStringPtr)
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Concatenate a String object and a string pointer
//		leftString + "\pRight";

LStr255
operator+(
	const LString&	inLeftString,
	ConstStringPtr	inRightStringPtr)
{
	LStr255	resultString = inLeftString;
	return resultString += inRightStringPtr;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator+ (ConstStringPtr, LString&)
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Concatenate a string pointer and a String object
//		"\pLeft" + rightString;

LStr255
operator+(
	ConstStringPtr	inLeftStringPtr,
	const LString&	inRightString)
{
	LStr255	resultString = inLeftStringPtr;
	return resultString += inRightString;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator+ (LString&, Uchar)
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Concatenate a String object and a character
//		leftString + 'a';

LStr255
operator+(
	const LString&	inLeftString,
	Uchar			inRightChar)
{
	LStr255	resultString = inLeftString;
	return resultString += inRightChar;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ operator+ (Uchar, LString&)
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Concatenate a character and a String object
//		'a' + rightString;

LStr255
operator+(
	Uchar			inLeftChar,
	const LString&	inRightString)
{
	LStr255	resultString = inLeftChar;
	return resultString += inRightString;
}


#pragma mark === LStr255 Class ===
// ===========================================================================
//	¥ LStr255
// ===========================================================================
//	A String with a maximum of 255 characters

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Default Constructor

LStr255::LStr255()
		: LString(sizeof(mString), mString)
{
	mString[0] = 0;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( LStr255& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Copy Constructor

LStr255::LStr255(
	const LStr255&	inOriginal)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inOriginal);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( LString& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Copy Constructor

LStr255::LStr255(
	const LString&	inOriginal)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inOriginal);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( ConstStringPtr& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a pointer to a string

LStr255::LStr255(
	ConstStringPtr	inStringPtr)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inStringPtr);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( Uchar )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a single character

LStr255::LStr255(
	Uchar	inChar)
		: LString(sizeof(mString), mString)
{
	mString[0] = 1;
	mString[1] = inChar;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( char* )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a C string (null terminated)

LStr255::LStr255(
	const char*	inCString)
		: LString(sizeof(mString), mString)
{
	Assign(inCString, CStringLength(inCString));
}	


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a pointer and length

LStr255::LStr255(
	const void*		inPtr,
	Uint8			inLength)
		: LString(sizeof(mString), mString)
{
	LoadFromPtr(inPtr, inLength);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( Handle )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from the data in a Handle

LStr255::LStr255(
	Handle	inHandle)
		: LString(sizeof(mString), mString)
{
	Assign(inHandle);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( ResIDT, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a 'STR ' or 'STR#' resource
//
//	The value of inIndex determines whether to use a 'STR ' or 'STR#'.
//	Uses 'STR ' for inIndex <= 0, and 'STR#' for inIndex >= 1. 

LStr255::LStr255(
	ResIDT	inResID,
	Int16	inIndex)
		: LString(sizeof(mString), mString)
{
	Assign(inResID, inIndex);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( Int32 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a long integer

LStr255::LStr255(
	Int32	inNumber)
		: LString(sizeof(mString), mString)
{
	Assign(inNumber);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( double_t, Int8, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assignment from floating point number
//
//	inStyle must be FLOATDECIMAL or FIXEDDECIMAL
//
//	For FLOATDECIMAL, inDigits is the number of significant digits
//		(should be > 0)
//	For FIXEDDECIMAL, inDigits is the number of digits to the right
//		of the decimal point

LStr255::LStr255(
	double_t		inNumber,
	Int8			inStyle,
	Int16			inDigits)
		: LString(sizeof(mString), mString)
{
	Assign(inNumber, inStyle, inDigits);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ LStr255 ( FourCharCode )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a four character code, which is an unsigned long,
//	the same as ResType and OSType

LStr255::LStr255(
	FourCharCode	inCode)
		: LString(sizeof(mString), mString)
{
	Assign(inCode);
}

#pragma mark === TString Class ===
// ===========================================================================
//	¥ TString
// ===========================================================================
//	A template String class

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Default Constructor

template <class T>
TString<T>::TString()
		: LString(sizeof(mString), mString)
{
	mString[0] = 0;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( TString& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Copy Constructor

template <class T>
TString<T>::TString(
	const TString&	inOriginal)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inOriginal);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( LString& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Copy Constructor

template <class T>
TString<T>::TString(
	const LString&	inOriginal)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inOriginal);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( ConstStringPtr& )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a pointer to a string

template <class T>
TString<T>::TString(
	ConstStringPtr	inStringPtr)
		: LString(sizeof(mString), mString)
{
	LoadFromStringPtr(inStringPtr);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( Uchar )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a single character

template <class T>
TString<T>::TString(
	Uchar	inChar)
		: LString(sizeof(mString), mString)
{
	mString[0] = 1;
	mString[1] = inChar;
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( char* )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a C string (null terminated)

template <class T>
TString<T>::TString(
	const char*	inCString)
		: LString(sizeof(mString), mString)
{
	Assign(inCString, CStringLength(inCString));
}	


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( const void*, Uint8 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a pointer and length

template <class T>
TString<T>::TString(
	const void*		inPtr,
	Uint8			inLength)
		: LString(sizeof(mString), mString)
{
	LoadFromPtr(inPtr, inLength);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( Handle )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from the data in a Handle

template <class T>
TString<T>::TString(
	Handle	inHandle)
		: LString(sizeof(mString), mString)
{
	Assign(inHandle);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( ResIDT, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a 'STR ' or 'STR#' resource
//
//	The value of inIndex determines whether to use a 'STR ' or 'STR#'.
//	Uses 'STR ' for inIndex <= 0, and 'STR#' for inIndex >= 1. 

template <class T>
TString<T>::TString(
	ResIDT	inResID,
	Int16	inIndex)
		: LString(sizeof(mString), mString)
{
	Assign(inResID, inIndex);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( Int32 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a long integer

template <class T>
TString<T>::TString(
	Int32	inNumber)
		: LString(sizeof(mString), mString)
{
	Assign(inNumber);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( double_t, Int8, Int16 )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Assignment from floating point number
//
//	inStyle must be FLOATDECIMAL or FIXEDDECIMAL
//
//	For FLOATDECIMAL, inDigits is the number of significant digits
//		(should be > 0)
//	For FIXEDDECIMAL, inDigits is the number of digits to the right
//		of the decimal point

template <class T>
TString<T>::TString(
	double_t		inNumber,
	Int8			inStyle,
	Int16			inDigits)
		: LString(sizeof(mString), mString)
{
	Assign(inNumber, inStyle, inDigits);
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//		¥ TString ( FourCharCode )
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constructor from a four character code, which is an unsigned long,
//	the same as ResType and OSType

template <class T>
TString<T>::TString(
	FourCharCode	inCode)
		: LString(sizeof(mString), mString)
{
	Assign(inCode);
}


#pragma mark === Template Instantiations ===
// ===========================================================================
//	¥ Template Instantiations
// ===========================================================================
//	Instantiate Template classes for all String types declared in <Types.h>

template class	TString<Str255>;
template class	TString<Str63>;
template class	TString<Str32>;
template class	TString<Str31>;
template class	TString<Str27>;
template class	TString<Str15>;

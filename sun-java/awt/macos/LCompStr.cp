// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "LCompStr.h"
extern "C" {
#include "jdk_java_lang_string.h"
};
LCompStr::LCompStr()
{
	fCompStr = NULL;
}
LCompStr::~LCompStr()
{
	if(fCompStr)
		INTL_CompoundStrDestroy(fCompStr);
	fCompStr = NULL;	
}
void LCompStr::InitCommon(unicode* ustr, int32 len)
{
	fCompStr = INTL_CompoundStrFromUnicode((INTL_Unicode* )ustr, len);
}
LCompStr::LCompStr(unicode* ustr, int32 len)
{
	InitCommon(ustr, len);
}
// This create fumction is similar to the makeXmString() which defined
// in ns/sun-java/awt/x/ustring.c
LCompStr::LCompStr(struct Hjava_lang_String *javaStringHandle)
{
	long        length;
	unicode*    uniChars;
	unicode     emptyString = 0;

	// Many awt routines seem to use the convention that NULL pointers to Java strings
	// should be treated as empty strings.  This routine preserves that convention.
	if( javaStringHandle == NULL )
	{
		uniChars = &emptyString;
		length = 0;
	}
	else
	{
		Classjava_lang_String* javaStringClass = unhand( javaStringHandle );
		HArrayOfChar* unicodeArrayHandle = (HArrayOfChar *)javaStringClass->value;
		uniChars =  unhand(unicodeArrayHandle)->body + javaStringClass->offset;
		length = javaStringClass->count;
	}
	InitCommon(uniChars, length);
}
LCompStr::LCompStr(ConstStr255Param unicodePStr)
{
	InitCommon((unicode*)&unicodePStr[2], (unicodePStr[0]-1)/2);	
}
LCompStr::LCompStr(HArrayOfChar *text, long offset, long length)
{
	InitCommon((unhand(text)->body + offset), length);
}

LCompStrIterator::LCompStrIterator(LCompStr *inStr)
{
	fState = 0;
	fIterator = getIterator(inStr);
}
LCompStrIterator::~LCompStrIterator()
{
	// empty
}
INTL_CompoundStrIterator getIterator(LCompStr* inStr)
{
	return (INTL_CompoundStrIterator) inStr->fCompStr;
}

Boolean LCompStrIterator::Next(int16 *outencoding, unsigned char** outtext)
{
	if(fState == 0)
	{
		fState++;
		fIterator = INTL_CompoundStrFirstStr(	fIterator, 
												(INTL_Encoding_ID *)outencoding,
												outtext);
	}
	else
	{
		fIterator = INTL_CompoundStrNextStr(	fIterator, 
												(INTL_Encoding_ID *)outencoding,
												outtext);
	}
	return (fIterator != 0);
}


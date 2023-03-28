

#include "libi18n.h"

/* 	Private Function	*/
static void				
INTL_CompoundStrAddSeg_p(
	INTL_CompoundStr* This, 
	INTL_Encoding_ID inencoding, 
	unsigned char* intext);
static INTL_CompoundStr* 	INTL_CompoundStrNewSeg_p(INTL_Encoding_ID inencoding, unsigned char* intext);


/*	
	Not Support Function - Too Complex to Implement!!! Believe it or not.
	INTL_CompoundStrComp	: We currently dont implement this funciton. If we decide to implement it. We should use the following algorithm
	(1) Compare each segment. If all match, return 0.
	(2) Convert both string to Unicode and them compare. If it equal in Unicode, return 0
	(3) If they are not the same in Unicode, we have to pick one sorting sequence to decide wheather it should return 1 or -1.
		The problem is which sorting order should we follow ? We need to decide :
			(a) the sorting order in one script,
			(b) the sorting order between scripts
*/

/*	Implementation */
INTL_CompoundStr* 	INTL_CompoundStrFromStr(INTL_Encoding_ID inencoding, unsigned char* intext)
{
	return INTL_CompoundStrNewSeg_p(inencoding, intext);
}

#define TMPLEN	256
INTL_CompoundStr*
INTL_CompoundStrFromUnicode(INTL_Unicode* inunicode, uint32 inunicodelen)
{	
	INTL_Encoding_ID	encoding;
	unsigned char TMP[TMPLEN];	
	INTL_UnicodeToStrIterator iterator;
	if((iterator = INTL_UnicodeToStrIteratorCreate(inunicode, 
					inunicodelen,
					&encoding, 
					&TMP[0], TMPLEN))!=NULL)
	{
		INTL_CompoundStr *This;
		This = INTL_CompoundStrNewSeg_p(encoding, &TMP[0]);
		if(This != NULL)
		{
			while(INTL_UnicodeToStrIterate(iterator, &encoding, &TMP[0], TMPLEN))
				INTL_CompoundStrAddSeg_p(This, encoding, &TMP[0]);
		}
		INTL_UnicodeToStrIteratorDestroy(iterator);
		return This;
	}
	return NULL;
}

INTL_CompoundStrIterator 
INTL_CompoundStrFirstStr(INTL_CompoundStr* This, INTL_Encoding_ID *outencoding, unsigned char** outtext)
{
	if(This == NULL)
		return NULL;
	else
	{
		*outencoding = This->encoding;
		*outtext = This->text;
		return ((INTL_CompoundStrIterator)This);
	}
}
INTL_CompoundStrIterator 
INTL_CompoundStrNextStr(INTL_CompoundStrIterator iterator, INTL_Encoding_ID *outencoding, unsigned char** outtext)
{
	INTL_CompoundStr* This = (INTL_CompoundStr*)iterator;
	return INTL_CompoundStrFirstStr(
			This->next, 
			outencoding, outtext);
}

void 			
INTL_CompoundStrDestroy(INTL_CompoundStr* This)
{
	INTL_CompoundStr* Next;
	for(; (This != NULL); This=Next)
	{
		if(This->next)
			Next=This->next;
		else
			Next=NULL;
		XP_FREE(This->text);
		XP_FREE(This);	
	}
}

INTL_CompoundStr* 	
INTL_CompoundStrClone(INTL_CompoundStr* s2)
{
	if(s2 != NULL)
	{
		INTL_CompoundStr* This;	
		This=INTL_CompoundStrNewSeg_p(s2->encoding, s2->text);
		This->next=INTL_CompoundStrClone(s2->next);  
		return This;
	}
	return NULL;
}	


static
void
INTL_CompoundStrCat_p(INTL_CompoundStr* s1, INTL_CompoundStr* s2)
{
	if(s1->next != NULL)
		INTL_CompoundStrCat_p(s1->next, s2);
	else
		s1->next = INTL_CompoundStrClone(s2);
}
void
INTL_CompoundStrCat(INTL_CompoundStr* s1, INTL_CompoundStr* s2)
{
	if((s2 != NULL) && (s2->text[0])) 
		INTL_CompoundStrCat_p(s1, s2);
}

static
INTL_CompoundStr* 	
INTL_CompoundStrNewSeg_p(INTL_Encoding_ID inencoding, unsigned char* intext)
{
	INTL_CompoundStr *This;
	
	This = XP_ALLOC(sizeof(INTL_CompoundStr));
	if(This != NULL)
	{
		char *p_text=0;
		StrAllocCopy(p_text, (char*)intext);
		This->text = (unsigned char*)p_text;
		
		This->next = NULL;
		This->encoding = inencoding;
	}
	return This;
}

static 
void
INTL_CompoundStrAddSeg_p(
	INTL_CompoundStr* This, 
	INTL_Encoding_ID inencoding, 
	unsigned char* intext)
{
	if(*intext)
	{
		for(;This->next;This=This->next)
			;
		This->next = INTL_CompoundStrNewSeg_p(inencoding,intext);
	}
}

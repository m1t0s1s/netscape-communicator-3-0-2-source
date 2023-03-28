// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include <LListIterator.h>
#include <LList.h>
#include "LFontList.h"
#include "xp_trace.h"
typedef struct id_font {
	int16 id;
	short font;
};
LFontList::LFontList(const LFontList& infont)
{
	// unfortunately, the LList class we contain do not support Copy constructor
	// we have to do the copy by ourslef.
	fFont= new LList(sizeof(id_font));
	ThrowIfNil_( fFont );
	fID= new LList(sizeof(int16));
	ThrowIfNil_( fID );
	int count = infont.fFont->GetCount();
	for(int i = 1 ; i <= count; i++)
	{
		id_font font;
		infont.fFont->FetchItemAt(i, &font);
		this->fFont->InsertItemsAt(1,arrayIndex_Last,&font);
	}
	count = infont.fID->GetCount();
	for(int i = 1; i <= count; i++)
	{
		int16 id;
		infont.fID->FetchItemAt(i, &id);
		this->fID->InsertItemsAt(1,arrayIndex_Last,&id);
	}
}
LFontList::LFontList()
{
	fFont= new LList(sizeof(id_font));
	ThrowIfNil_( fFont );
	fID= new LList(sizeof(int16));
	ThrowIfNil_( fID );
}
LFontList::~LFontList()
{
	delete fFont;
	delete fID;
}
void LFontList::resetFonts()
{
	delete fFont;
	delete fID;
	fFont= new LList(sizeof(id_font));
	ThrowIfNil_( fFont );
	fID= new LList(sizeof(int16));
	ThrowIfNil_( fID );	
}
void LFontList::addFont(int16 id, short font)
{
	id_font fontitem;
	fontitem.id=id;
	fontitem.font=font;
	fFont->InsertItemsAt( 1, arrayIndex_Last, &fontitem);
	fID->InsertItemsAt( 1, arrayIndex_Last, &id);
}

short LFontList::getFont(int16 id)
{
	id_font fontitem;
	LListIterator	iterID(*fFont, iterate_FromStart);
	while( iterID.Next(&fontitem))
	{
		if(fontitem.id == id)
			return fontitem.font;
	}
	XP_ASSERT(1);
	return 0;
}

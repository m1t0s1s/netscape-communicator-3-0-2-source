// =================================================================================
// Copyright Netscape Communication 1996-1997
// Author: Frank Tang <ftang@netscape.com>
// =================================================================================
#include "LJavaFontList.h"
LJavaFontList::LJavaFontList() : LFontList()
{
	fSize = 12;
	fStyle = normal;
	this->setFontName("\pDialog");	
}
LJavaFontList::LJavaFontList(const LJavaFontList& infont)
	: LFontList(infont)
{
	fSize = infont.fSize;
	fStyle = infont.fStyle;
}
LJavaFontList::LJavaFontList(Classjava_awt_Font	*font)
	: LFontList()
{
	this->SetFont(font);
}

LJavaFontList::~LJavaFontList()
{
	// Empty
}
void	LJavaFontList::SetFont(Classjava_awt_Font	*font)
{
	Str255	fontPString;
	javaString2CString(font->name, (char *)(fontPString + 1), sizeof(fontPString) - 1);
	fontPString[0] = strlen((char *)(fontPString + 1));
	// 	put SetFontSize & SetFontStyle before setFontName because 
	//	"Dialog" will over write size and style
	this->setFontSize(font);	
	this->setFontStyle(font);
	this->setFontName(fontPString);
}
void	LJavaFontList::setFontStyle(Classjava_awt_Font	*infont)
{
	fStyle = 0;

	if ((infont->style & java_awt_Font_BOLD) != 0)
		fStyle |= bold;

	if ((infont->style & java_awt_Font_ITALIC) != 0)
		fStyle |= italic;
		
}
void LJavaFontList::InitFontMetrics(Classsun_awt_macos_MacFontMetrics *macFontMetrics)
{
	macFontMetrics->ascent = 0;
  	macFontMetrics->descent = 0;
    macFontMetrics->leading = 0;
    macFontMetrics->maxAscent = 0;
    macFontMetrics->maxDescent = 0;
    macFontMetrics->maxAdvance = 0;
    
    this->SetTextSize();
    this->SetTextStyle();
	LListIterator	iterID(*(this->getIDList()), iterate_FromStart);
	int16 curid;
	while( iterID.Next(&curid))
		this->fillOverallFontMetrics(curid, macFontMetrics);
    macFontMetrics->height = macFontMetrics->ascent + macFontMetrics->descent;
    macFontMetrics->maxHeight = macFontMetrics->ascent + macFontMetrics->descent;
    this->fillPerCharFontMetrics(macFontMetrics);
}		
#define _GETMAX(a,b)	{if((a) < (b)) (a) = (b);}
void LJavaFontList::fillOverallFontMetrics(int16 encoding, Classsun_awt_macos_MacFontMetrics *macFontMetrics)
{
	FontInfo fontinfo;
	this->getFontInfo(encoding, fontinfo);
	_GETMAX( macFontMetrics->ascent , 	fontinfo.ascent);
	_GETMAX( macFontMetrics->descent , 	fontinfo.descent);
	_GETMAX( macFontMetrics->leading , 	fontinfo.leading);
	_GETMAX( macFontMetrics->maxAscent,	fontinfo.ascent);
	_GETMAX( macFontMetrics->maxDescent, fontinfo.descent);
	_GETMAX( macFontMetrics->maxAdvance, fontinfo.widMax);
}		
#define ISO8859_1_TO_MACROMAN_XLAT 128
void LJavaFontList::fillPerCharFontMetrics(Classsun_awt_macos_MacFontMetrics *macFontMetrics)
{
	SInt32									*widthArray;
	this->SetTextLatin1Font();	
	char** table= INTL_GetSingleByteTable(CS_LATIN1, CS_MAC_ROMAN,ISO8859_1_TO_MACROMAN_XLAT);
	unsigned char *isomap = (unsigned char*) *table;
	//  WARNING:
	//  I do not lock this resource since I don't call any toolbox that move memory
	//  from here till the end of function 
	//	Please LOCK the resource if adding new code to this function that may
	//  move memory.
	
	//	Fill the array with the font info.	
	widthArray = unhand(macFontMetrics->widths)->body;
	for (UInt32 count = 0; count < 256; count++) 
		widthArray[count] = ::CharWidth(isomap[count]);
	INTL_FreeSingleByteTable(table);
}

void LJavaFontList::setFontName(Str255 fontName)
{
	int16 itemcount, *csidlist;
	
	this->resetFonts();
	
	csidlist = INTL_GetUnicodeCSIDList(&itemcount);
	
	if(EqualString(fontName, "\pDialog", false, false))	{
		fSize = 9;
		fStyle = normal;		
	}
	
	LVFont vfont(fontName);
	
	for(int i=0;i<itemcount;i++)
		this->addFont(csidlist[i],vfont.GetVF(csidlist[i]));
}

void LJavaFontList::SetTextLatin1Font()
{
	this->SetTextFont(CS_MAC_ROMAN);
	this->SetTextSize();
	this->SetTextStyle();
}
void LJavaFontList::getFontInfo(int16 encoding, FontInfo& info)
{
	this->SetTextFont(encoding);
	::GetFontInfo(&info);
}


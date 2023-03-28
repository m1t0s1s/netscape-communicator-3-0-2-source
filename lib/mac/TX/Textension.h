// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


#pragma once

#include "StyledText.h"
#include "CTextRenderer.h"
#include "RunObject.h"
#include "RunsRanges.h"

#ifdef txtnRulers
#include "RulerObject.h"
#include "RulersRanges.h"
#endif

#include "CLineHeights.h"
#include "Formatter.h"


#include "CPageGeometry.h"


const short kMaxRegisteredObjects = 6;

class CRegisteredObjects
{
	public :
		inline void IRegisteredObjects() {fCount = 0;}
		void Free();
		inline short GetCountObjects() const {return fCount;}
		CAttrObject* GetIndObject(short index) const;
		void Add(CAttrObject* theObject);
		CAttrObject* Search(ClassId objClassId);
		
	private:
		short fCount;
		CAttrObject* fTable[kMaxRegisteredObjects];
};

extern CRegisteredObjects gRegisteredRuns;

#ifdef txtnRulers
extern CRegisteredObjects gRegisteredRulers;
#endif

extern unsigned char gGraphicsRunAssocChar;
//************************************************************************************************
typedef unsigned char TIOFlags;

const TIOFlags kNoIOFlags = 0;

const TIOFlags kIOText = 1;
const TIOFlags kIORuns = 2;

#ifdef txtnRulers
const TIOFlags kIORulers = 4;
#endif

#ifdef txtnRulers
const TIOFlags kIOTypesMask = kIOText+kIORuns+kIORulers;

const TIOFlags kIOAll = kIOText+kIORuns+kIORulers;
#else
const TIOFlags kIOTypesMask = kIOText+kIORuns;

const TIOFlags kIOAll = kIOText+kIORuns;
#endif
//***************************************************************************************************

//constants for descendents
const unsigned long kTextIOSuiteType = 'TEXT';
const unsigned long kRunsIOSuiteType = 'runs';

#ifdef txtnRulers
const unsigned long kRulersIOSuiteType = 'rlrs';
#endif


typedef unsigned short TKeyDownFlags; //"Ctextension::KeyDown" returns a value of this type.


/*
following are the constants for TKeyDownFlags, note that those are bit values, they will be used as:
TKeyDownFlags keyFlags = textension->KeyDown(...);
if (keyFlags & kTextensionModif) {...}
*/

const TKeyDownFlags kNoKeyDownFlags = 0; //the key down had no effect (esc, function keys, ..)
const TKeyDownFlags kKeyDownHandled = 1 << 0; //the keydown is handled
const TKeyDownFlags kTextensionModif = 1 << 1; //the key has modified the content of the document

const TKeyDownFlags kContinuousObjectsModif = 1 << 2;
/*
added to help caller avoiding set up the font-size-style menus or ruler icons with each key down.
caller should do the setup only when keydown returns a result with kContinuousObjectsModif on. 
*/

const TKeyDownFlags kArrowKey = 1 << 3; //the key is an arrow key
//***************************************************************************************************


const short kKeyDownBufferSize = 50;
//successive (up to "kKeyDownBufferSize") key downs are buffered until no keydown events and then flushed to the document

struct TKeyDownParams {//Output from CalcKeyDownParams
	TOffsetRange rangeToReplace;
	/*
	the range to be replaced by this key down event. note that this is not always equal to the current
	selected range (back space when empty selection, for instance)
	*/

	long countSelectedChars;
	//how many chars are currently selected. note that this is not always equal to rangeToReplace.Len()
	
	short countNewChars;
	//count chars to replace the "rangeToReplace"
	
	unsigned char newChars[kKeyDownBufferSize]; //the actual chars to replace the "rangeToReplace"
	
	TKeyDownFlags flags; //see TKeyDownFlags notes
};
//***************************************************************************************************

struct TTextDescriptor {
	TTextDescriptor() {
//		fTextStream = nil;
		fTextPtr = nil;
		fTextSize = 0;
	}
	
	
	TTextDescriptor(const unsigned char* textPtr, long textSize) {
		fTextPtr = textPtr;
		fTextSize = textSize;
//		fTextStream = nil;
	}
	
//	CStream* fTextStream;
	const unsigned char* fTextPtr;
	long fTextSize;
};

/*
TTextDescriptor is used as input to some of the "TReplaceParams" constructors (declared later).
The text may be described in one of the following ways:
1) using a stream: in this case you'll use the constructor "TTextDescriptor(CStream* stream)".
The stream may be any descendent of the abstract "CStream" (CHandleStream, CFileStream are already implemented)

2) by a ptr to the text and its size: you'll use the constructor "TTextDescriptor(unsigned char* textPtr, long textSize)"
*/
//***************************************************************************************************

struct TReplaceParams {
	TReplaceParams(); //initialized to zeros

//	TReplaceParams(CTextensionIOSuite* ioSuite);

	TReplaceParams(CRunObject* runObj);
	//non text run obj (pict, sound, ..)

	TReplaceParams(const TTextDescriptor& textDesc);


	TReplaceParams(const TTextDescriptor& textDesc, CRunObject* replaceObj, Boolean takeObjCopy);
	
	TReplaceParams(const TTextDescriptor& textDesc, CObjectsRanges* runs, Boolean plugNewRuns);


TTextDescriptor fTextDesc;

Boolean fCalcScriptRuns;

//CTextensionIOSuite* fIOSuite;

CObjectsRanges* fNewRuns;
Boolean fPlugNewRuns;

CRunObject* fReplaceObj;
Boolean fTakeObjCopy;

Boolean fTrailEdge;
Boolean fInvalPendingRun;
};

/*
TReplaceParams is used as input to "CTextension::ReplaceRange". It describes the new data that will replace
an old range of text. depending on the data you have, you'll use a different TReplaceParams' constructor:

1) TReplaceParams();
	all data descriptors are initialized to zero. Could be used to clear certain range.
	
2) TReplaceParams(CTextensionIOSuite* ioSuite);
	usually "ioSuite" is filled by "CTextension::OutputRangeIOSuite". This is the engine rich format
	(ioSuite may contain text, runs, rulers info)
	
3) TReplaceParams(CRunObject* runObj);
	used to replace a range with a non text run (pict, sound, ..).
	-runObj : any descendent of the abstract "CRunObject".
	Note that the object itself will be used in the replace and the the engine will free it.
	
	Below is how you  can, for instance, inserts a sound run:
			CSoundRun* soundRun = new CSoundRun;
			soundRun->ISoundRun();
			
			if (soundObj->Record() == noErr) {
				TOffsetRange selRange;
				textension->GetSelectionRange(&selRange);
				
				TReplaceParams replaceParams(soundRun);
				textension->ReplaceRange(selRange.Start(), selRange.End(), replaceParams);
			}
			else
				soundRun->Free();
	

4) TReplaceParams(const TTextDescriptor& textDesc);
	used when you have only raw text (no runs, rulers, .. info).
	-textDesc : a "TTextDescriptor" which describes the raw text that will be used in the replace operation
		if the current system has more than 1 script installed , "ReplaceRange" will calculate the script runs 
		(by calling the script manager's routine "FindScriptRun).


5) TReplaceParams(const TTextDescriptor& textDesc, CRunObject* replaceObj, Boolean takeObjCopy);
	used when you have some text and an associated text run object. Usually you won't need to use this way

6) TReplaceParams(const TTextDescriptor& textDesc, CObjectsRanges* runs, Boolean plugNewRuns);
	used when you have some text and their associated runs objects.
	-textDesc: describes the text parameter. see "TTextDescriptor" remarks
	-runs: you'll have to create a "CObjectsRanges" instance and fill it
	-plugNewRuns: you'll normally pass this param as "false"
*/
//***************************************************************************************************



class CTextension : public CStyledText
{

  public:
	static OSErr TextensionStart();
	/*
	should be called before any other CTextension members. better to call it as early as possible because some pointers
	might be created (prevent heap fragmentation)
	returns an OSErr. the engine can not be used if this routine returns with an error
	*/
	
static void TextensionTerminate();
	//should be called at the end to release any global storage
	
static void RegisterRun(CRunObject* run);
static CAttrObject* GetDefaultObject(unsigned long ioSuiteType);

	/*
		parameters:
		-run: any descendent of CRunObject. You should create and initialize the run object. The engine will, however,
		free it at termination time.
		
		Any run object supported by your application should be registered with the engine by calling "RegisterRun"
		at the app's initialization time (after calling "TextensionStart").
	
		The order in which you register the runs is important:
			*the first one should be the default run object (usually a text run)
			*the order of other runs will affect the conversion from the system s. While converting from the system scrap,
				each registered run obj (in the order you registered theme) is asked to check if it can reads data from
				the system scrap. If a run object can read from the system scrap, the runs following it are not asked.
				This means, for instance, that you should register a movie run obj before registering a picture run obj,
				so that if the scrap contains both types, the movie is pasted and not the pict.
		
		Suppose that you support text and sounds in your app, this is how you might register theme:
			CQDTextRun* textRun = new CQDTextRun;
			textRun->IQDTextRun();
			CTextension::RegisterRun(textRun);
			
			CSoundRun* sound = new CSoundRun;
			sound->ISoundRun();
			CTextension::RegisterRun(sound);
	*/
	
#ifdef txtnRulers
static void RegisterRuler(CRulerObject* ruler);
	/*
		parameters:
		-ruler: any descendent of CRulerObject. You should create and initialize the ruler object.
		The engine will, however, free it at termination time.
		
		Any ruler object supported by your application should be registered with the engine by calling "RegisterRuler"
		at the app's initialization time (after calling "TextensionStart").
		
		usually you'll have only one ruler object. The engine supplies 2 types of rulers:
		CBasicRuler: supports justification(left, center, right, full) and automatic tabs (like MPW tabs)
		CAdvancedRuler: supports justification + indent, left and right marges, line spacing and user defined tabs
	*/
#endif
	
static CRunObject* GetNewRunObject();
	/*
		returns a new initialized run object which has the same class as the one that has been 
		registered first by calling RegisterRun.
			
		caller should free the object when done with it.
	*/

#ifdef txtnRulers
static CRulerObject* GetNewRulerObject();
	/*
		returns a new initialized ruler object which has the same class as the one that has been 
		registered first by calling RegisterRuler.
			
		caller should free the object when done with it.
	*/
#endif


inline static void SetDefaultRun(CRunObject* defaultRun) {fDefaultRun = defaultRun;}
	/*
		-defaultRun: this run object will be used any time the engine needs to use a default run
		(typing when no text, for instance).
		It Might be used to implement the preferences for text runs.
		
		Note : the engine will free and delete the passed "defaultRun" at termination time.
	*/
	
static CRunObject* GetDefaultRun() {return fDefaultRun;}
	//returns the object you passed to "SetDefaultRun" (nil if you didn't call SetDefaultRun)
	
	

inline static void SetImaging(Boolean on_off) {gTextensionImaging = on_off;}
	/*
	This informs the engine if you are currently printing. normally will be used as:
	
		CTextension::SetImaging(true);
		Print();
		CTextension::SetImaging(false);
	
	this information is kept in a global var "gTextensionImaging" that can be cheked by run objects which have
	different drawing method depending whether the drawing is to a printer (like the movie object).
	It is used as well to avoid some clipping and erasing operations not necessary when printing
	*/
	
inline static Boolean IsImaging() {return gTextensionImaging;}
	//returns the current status of the variable "gTextensionImaging"


//inline static void SetMinAppMemory(long minMem) {gMinAppMemory = minMem;}
	/*
		the minimum app memory is the amount guaranteed after any edit operation
		(no edit operation will leave the memory less than this amount).
		by default its 32k
	*/
	
//																non static members
//																------------------
	
CTextension(); //default constructor
	
void ITextension(GrafPtr textPort, TSize sizeInfo = kLargeSize);
	/*
		should be called just after creating a new "CTextension" object.

		-textPort: quickDraw port which will be used for drawing. if you pass nil in this param, the drawing will
			always take place in the current port.
		
		-sizeInfo: can be one of 3 values : kSmallSize, kMediumSize or kLargeSize.
			*kSmallSize : text is expected to be in the range : 0..5k bytes
			*kMediumSize : text is expected to be in the range : 5k..32k bytes
			*kLargeSize : text is expected to be more than 32k bytes
			
			the effect of "sizeInfo" is just for performance. the text could, actually, exceeds its expected size
	*/

virtual void Free();
	//call it when the CTextension object is no longer needed
	
//																				handlers
//																				--------

#ifdef txtnRulers
inline CRulersRanges* GetRulersRanges() const {return fRulersRanges;}
#endif

inline CTextRenderer* GetDisplayHandler() const {return mRenderer;}
inline CPageGeometry* GetPageGeometry() const { return mGeometry;		}
inline CFormatter* GetFormatter() const {return mFormatter;}
inline CLineHeights* GetLineHeights(void) const {return mLineHeights;}
inline CRunsRanges* GetRunsRanges() const {return mRunRanges;}
	
//																				frames	
//																				------
virtual short DisplayChanged(const CDisplayChanges& displayChanges);
	/*
	should be called after calling any "CTextension" or "CFrames" member which returns a 
	"CDisplayChanges" result (SetDirection, SetTextBoundsSize, ..).
	"DisplayChanged" will translate those display changes to their visual equivalent (formatting, invalidating some frames, ..)
	
	-displayChanges : usually this is the output of one of CTextension or CFrames members
	
	-result: set of bit values, could be one of the following:
		*kRedrawAll ==> all frames should be invalidated by the caller.
		*kCheckSize ==> some frames or lines have been added or deleted, could be used to adjust scroll bars.
		*kCheckUpdate ==> some frames have been invalidated, caller might process the update event immediately for fast response.
	
	ex:
		Rect margins;
		margins.top = 2;
		margins.left = 2;
		margins.bottom = 2;
		margins.right = 2;
		
		CDisplayChanges displayChanges;
		frames->SetFramesMargins(margins, &displayChanges);
	
		short action = fTextension->DisplayChanged(displayChanges);
		
		if (action & kRedrawAll)
			{Redraw();}
			
		if (action & kCheckSize)
			{AdjustScrollBars();}
		
		.......
	*/

//																				drawing
//																				-------
	
		virtual	void			DisableDrawing(void); 
		virtual void			EnableDrawing(void);

		virtual void			Draw(
										LView*			inViewContext,
										const Rect& 	inLocalUpdateRect,
										TDrawFlags 		drawFlags = kEraseBeforeDraw);
	/*
		called in response of an update event.
		
		-updateRect : rectangle to be updated
		-drawFlags : combination of the bit values : kNoDrawFlags, kEraseBeforeDraw, kDrawAllOffScreen
	*/
		
	
	
//																				formatting
//																				----------
virtual OSErr Format(Boolean temporary = false, long firstChar =-1, long count =-1);
	/*
		to be called to format (calculates the line ends) certain char range
		
		-temporary: if true, the line ends will be calculated but lines will not redraw.
			It is assumed that "Format" will be called to restore the original line ends.
		
		-firstChar: starts formatting from this offset. A value < 0 means format all chars
		-count : calculates line ends falling in the range firstChar..firstChar+count. A value < 0 means format all chars
	*/

void SetCRFlag(Boolean newFlag, CDisplayChanges* displayChanges);
	/*
		sets the carriage return formatting to "newFlag" and returns the display changes into "displayChanges". 
		"SetCRFlag" does not, actually, have any visible effect, caller should call "DisplayChanged" after calling 
		it to update the display.

		if "newFlag" is true, line ends will always end at a carriage return (like MPW editor).
		the default is, of course, false.
	*/
	
inline Boolean GetCRFlag() const {return mFormatter->GetCRFlag();}
	//returns the current setting of the carriage return flag.
	
inline long	CountLines() const {return mFormatter->CountLines();}
	//returns the total number of lines
	
inline void GetLineRange(long lineNo, TOffsetRange* range) const
	{mFormatter->GetLineRange(lineNo, range);}
	

//																				selection
//																				---------
virtual void Activate(Boolean activ, Boolean turnSelOn = true);
		
	


//																				chars
//																				-----
//inline Point CharToPoint(TOffset theChar, short* height =nil)
//{return mRenderer->CharToPoint(theChar, height);}
	/*
		given a char offset, CharToPoint returns the point corresponding to that char.
		
		-theChar: the char offset.
		-height: if not nil, *height will hold the char's line height.
		-result: returns the point in draw coord. result.v is the char's line top, result.h is the horiz. pix up to the char start
	*/
	
long CharToLine(TOffset charOffset, TOffsetRange* lineRange = nil) const;
	/*
	given a char offset, CharToLine returns the line containing this char offset and optionally returns
	the line range in "lineRange".
	*/
	
void CharToParagraph(long offset, TOffsetRange* paragRange);
	//given a char offset, CharToParagraph returns in "paragRange" the parag. range corresponding to this char offset.



	
//virtual OSErr ReplaceRange(long rangeStart, long rangeEnd, const TReplaceParams& replaceParams, Boolean invisible = false,
virtual OSErr ReplaceRange(long rangeStart, long rangeEnd, const TReplaceParams& replaceParams, Boolean invisible,
TOffsetPair* ioLineFormatPair);

	/*
		replaces the char range rangeStart..rangeEnd by the content of "replaceParams".
		
		see "TReplaceParams" notes for more information on the different ways to construct "replaceParams"

		if "invisible" is true, no formatting or drawing will take place.
	*/



	
//																				runs
//																				----
virtual OSErr UpdateRangeRuns(const TOffsetRange& theRange, const AttrObjModifier& modifier
															, Boolean invisible=false);
	/*
		Modify the run objects in the char range "theRange" depending on "modifier".
		see AttrObjModifier notes for more information.
		
		if "invisible" is true, no formatting or drawing will take place.
		
		Note: if "theRange" is empty, the pending run object is updated
			(the pending run object is the object that will be used in subsequent typed chars).
		
		the following examples show how you can use "UpdateRangeRuns"
		
		ex1: change the font in the current selection range to helvetica
			short helveticaFontNumber = helvetica; //helvetica const is declared in Fonts.h
			AttrObjModifier runModifier(kFontAttr, &helveticaFontNumber);
			
			TOffsetRange currSelRange;
			textension->GetSelectionRange(&currSelRange);
			textension->UpdateRangeRuns(currSelRange, runModifier);
		
		ex2: change the font in the current selection range to helvetica and the font size to 18
			short helveticaFontNumber = helvetica; //helvetica const is declared in Fonts.h
			short newFontSize = 18;
			
			CRunObject* modifierObj = textension->GetNewRunObject();
			modifierObj->SetAttributeValue(kFontAttr, &helveticaFontNumber);
			modifierObj->SetAttributeValue(kFontSizeAttr, &newFontSize);

			AttrObjModifier runModifier(modifierObj);
			
			TOffsetRange currSelRange;
			textension->GetSelectionRange(&currSelRange);
			textension->UpdateRangeRuns(currSelRange, runModifier);
			
			modifierObj->Free();
	*/
	
const CRunObject* GetContinuousRun(const TOffsetRange& inRange);

//for other run operations see also StyledText.h


#ifdef txtnRulers
//																				rulers
//																				------
inline const CRulerObject* CharToRuler(TOffset charOffset) const
{return (CRulerObject*)(fRulersRanges->OffsetToObject(charOffset));}
	/*
		given a char offset in "charOffset", CharToRuler returns the ruler object corresponding to that offset.
		Note: CharToRuler returns always a valid ruler (ruler with all attributes set)
	*/

inline const CRulerObject* GetNextRuler(long offset, long* runLen) const;

OSErr UpdateRangeRulers(const TOffsetRange& theRange, const AttrObjModifier& modifier, Boolean invisible=false);
	//as "UpdateRangeRuns" but for rulers
	
#endif //txtnRulers


//																				misc.
//																				-----
virtual void SetDirection(char newDirection, CDisplayChanges* displayChanges);
	/*
		sets the direction to "newDirection" and returns the display changes into "displayChanges".
		"SetDirection" does not, actually, have any visible effect, caller should call "DisplayChanged" after calling 
		it to update the display (format, ..).
		
		by default, the direction is equal to the global sysJust, but you can change it by calling this member.
		
		the direction will affect the following:
		-runs order inside a line (in case the line has left to right and right to left runs).
		-the columnar frames handler use the direction to decide the order of the columns (right to left direction will
			draw the first column in the right side).
		-the pages frames handler uses the direction to decide the order of the pages (if you have multiple pages across).
	*/
	
inline char GetDirection() const {return mRenderer->GetDirection();}
	//returns the current direction
	

	protected:



		Int32 		ClearKeyDown(unsigned char key, TOffsetRange* rangeToClear);
	
		OSErr 		UpdateFormatter(long updateFlags, const TOffsetRange& range
											, long* firstFormatLine, long* lastFormatLine);
	
		CRunObject* IsRangeGraphicsObj(const TOffsetRange& charRange);

		void Compact();
	
		void UpdateKeyboardScript();

		virtual CRunObject* UpdatePendingRun();
		
		virtual void BeginEdit(TEditInfo* editInfo);
		virtual void EndEdit(const TEditInfo& editInfo, long firstFormatLine, long lastFormatLine
													, TOffset* selOffset=nil, Boolean updateKeyScript=true);



		CPageGeometry*			mGeometry;
		CTextRenderer* 			mRenderer;
		CLineHeights* 			mLineHeights;
		CFormatter*				mFormatter;

		#ifdef txtnRulers
		CRulersRanges*			mRulerRanges;
		#endif
	
		CRunObject*				mPendingRun;

		
	
		Int16 					mDrawVisLevel;

	
		static CRunObject* 			fDefaultRun;

};





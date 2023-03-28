// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#pragma once

#include "RunsRanges.h"



struct TTextEnv {
	TTextEnv(char nesting = 0) {allowNesting = nesting;}
	
	char allowNesting;
	GrafPtr savedPort;
	short savedSysJust;
	long	savedFontForce;
};



class	CStyledText
{
	private:
								CStyledText();			// Parameters Required
	public:
								CStyledText(
										GrafPtr 		inPort,
										CRunObject* 	inProtoObj,
										TSize 			inSizeInfo);
		
		virtual					~CStyledText();
			
		CRunsRanges* 			GetRunsRanges(void);
		
		virtual void SetTextPort(GrafPtr newPort);
		GrafPtr GetTextPort() const;
		
		virtual void 			SetTextEnv(TTextEnv* savedTextEnv);
		virtual void 			RestoreTextEnv(const TTextEnv& savedTextEnv);
		
		Uint8* 					LockChars(Boolean moveHi = false);
		void 					UnlockChars(void);
		
		Uint8* 					GetCharPtr(long charOffset) const;
			
		Int32					CountCharsBytes() const;
		
		virtual void 			CharToWord(TOffset theChar, TOffsetRange* wordRange);
		
		short 					AdvanceOffset(long charOffset, Boolean forward);
		
		const CRunObject* 		CharToRun(TOffset charOffset); //returns nil if text is empty
		const CRunObject* 		CharToTextRun(TOffset charOffset) const;
		const CRunObject* 		GetNextRun(long offset, long* runLen) const;
		
		#ifdef txtnDebug
		inline char TextEnvLevel()
			{return fTextEnvLevel;}
		#endif
	
	protected:

		GrafPtr				mTextPort;
		
		CArray*				mTextChars;
		CRunsRanges* 		mRunRanges;
	
		
		char CalcNextScriptRun(long offset, long len, long* scriptRunLen); //call FindScriptRun
		OSErr RunToScriptRuns(long runOffset, long runLen, const CRunObject* defaultRun);
		
		OSErr	CalcScriptRuns(const CRunObject* defaultRun, char* majorDirection);
		//defaultRun may be nil
		
		short GetByteOrder(unsigned char* chars, short offset, short charScript);
		//returns the same result as CharByte
		
			
	private:

		CRunObject*	fSavedObj;
		char 		fTextEnvLevel; //allow nested calls to SaveTextEnv and RestoreTextEnv

	public:

		virtual	CArray*		GetArray(void);
		virtual	void		SetArray(CArray *inArray);
};


inline CRunsRanges* CStyledText::GetRunsRanges(void)
	{	return mRunRanges;				}
inline Uint8* CStyledText::LockChars(Boolean moveHi)
	{	return (Uint8*)mTextChars->LockArray(moveHi);	}
inline void CStyledText::UnlockChars(void)
	{	mTextChars->UnlockArray();		}
inline Uint8* CStyledText::GetCharPtr(long charOffset) const
	{	return ((unsigned char*)mTextChars->GetElementPtr(charOffset));	}
inline Int32 CStyledText::CountCharsBytes() const
	{	return mTextChars->CountElements();				}
inline const CRunObject* CStyledText::CharToRun(TOffset charOffset)
	{	return (CRunObject*) mRunRanges->OffsetToObject(charOffset);	}
inline const CRunObject* CStyledText::CharToTextRun(TOffset charOffset) const
	{	return mRunRanges->CharToTextRun(charOffset);	}


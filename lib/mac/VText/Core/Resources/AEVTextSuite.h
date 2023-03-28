//	===========================================================================
//	AEVTextSuite.h
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef	rez
#else
	#pragma	once
#endif

#ifdef	__AEVTextSuite__
#else

	#define	__AEVTextSuite__
	
	#include	<AESuiteMacros.h>
	
	#define		ae_Modify					12500

	#define		kAEVTextSuite				'VTxt'
	#define		kAEModify					'Vmmy'
	#define		kEnumVTextStyleModifiers	'VEMm'
	#define		keyAEModifyProperty			'Vmmp'
	#define		keyAEModifyToValue			'Vmmt'
	#define		keyAEModifyByValue			'Vmmb'
	
	#define		typeVRulerSets				'VTrs'
	#define		typeVRulerSetRuns			'VTrr'
	#define		typeVStyleSets				'VTss'
	#define		typeVStyleSetRuns			'VTsr'
	
//	#define		pParent						'Vpar'
	#define		pFirstIndent				'Vfid'
	#define		pBegin						'Vpbg'
	#define		pEnd						'Vpen'

	#define		pLocation					'Vplc'
	#define		pRepeating					'Vprp'
	#define		pDelimiter					'Vpdl'
	
			//	Stylesets
	#define		cStyleSet					'stys'
	#define		pStyleSet					'VPss'

			//	Rulersets
	#define		cRulerSet					'Vcrs'
	#define		pRulerSet					'Vprs'
	
	#define		pVTextDirection				'Vpwd'
	#define		pVTextJustification			'Vpjs'
	
	#define		kEnumVTextJustification		'VEJk'
	#define		kVTextRulerLeading			'VEJl'
	#define		kVTextRulerTrailing			'VEJt'
	#define		kVTextRulerCentered			'VEJc'
	#define		kVTextRulerFilled			'VEJf'
//?	#define		kVTextRulerProgrammer		'VEJp'
/*
				Note:	trailing, centered, and filled rulers make sense iff:

					there are no tabs on the line, or

					the last tab on the line has a "leading" orientation.
					
						In this case only the text after that tab is trailed,
						centered, or filled with respect to the last tab position.
				
					For cases when the last tab does not have a "leading" orientation,
					the text after the tab is positioned according to the orientation
					of the last tab.
*/
	
	#define		kEnumVTextWritingDirection	'VEDk'
	#define		kVTextLeftToRight			'VEDl'
	#define		kVTextRightToLeft			'VEDr'
	
			//	Tabs
	#define		cRulerTab					'Vctb'
	
	#define		pVTextTabOrientation		'Vpto'
		
	#define		kEnumVTextTabOrientation	'VETE'
	#define		kVTextTabLeading			'VETl'
	#define		kVTextTabTrailing			'VETt'
	#define		kVTextTabCentered			'VETc'
	#define		kVTextTabAligned			'VETa'

#endif


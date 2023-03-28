//	===========================================================================
//	AESuiteMacros.h
//	===========================================================================
//	This file is processable by rez and C/C++ compilers


#ifdef	rez
#else
	#pragma	once
#endif

#ifdef	__AESuiteMacros__
#else
	#define	__AESuiteMacros__
	
	//	Macros used by many suites...
	#define	Reserved8_		reserved, reserved, reserved, reserved, reserved,	\
							reserved, reserved, reserved
	#define	Reserved12_		Reserved8_, reserved, reserved, reserved, reserved
	#define	ReservedVerb_	Reserved8_, verbEvent, reserved, reserved, reserved
	#define ReservedPrep_	Reserved8_, reserved, prepositionParam, notFeminine, \
							notMasculine, singular
	#define	ReservedApost_	Reserved8_, noApostrophe, notFeminine, notMasculine, singular
#endif



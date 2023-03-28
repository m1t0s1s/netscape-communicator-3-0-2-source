//	===========================================================================
//	PP_ClassIDs_DM
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

//	This file is processable by rez and C/C++ compilers
#ifdef	rez
#else
	#pragma	once
#endif

#ifdef	__PP_ClassIDs_DM__
#else
	#define	__PP_ClassIDs_DM__

	//	------------------------------------------------------------
	//	еее	Class IDs:
	/*
		ID's commented out and labeled "virtual base class" are at least
		conceptually virtual base classes.
	*/
	
	//	е	Direct Manipulation
	#define	kPPCID_SelectionModelAEOM	'seom'	//	virtual base class
	#define	kPPCID_Selection			'sele'
	#define	kPPCID_EventMachine			'evmc'
	#define	kPPCID_EventHandler			'evhl'
	#define	kPPCID_DataDragEventHandler	'ddhl'
	#define	kPPCID_HandlerView			'hdlv'	//	virtual base class
	#define kPPCID_SelectHandlerView	'shv '	//	virtual base class
	
	//	е	Miscellaneous classes
	#define	kPPCID_Targeter				'tgtr'
	#define	kPPCID_2dDynamicArray		'2dar'

	//	е	DM Tables
	#define	kPPCID_NTableView			'tblv'	//	virtual base class
	#define	kPPCID_NTable				'ntbl'	//	virtual base class
	#define	kPPCID_TableModel			'tblm'
	#define	kPPCID_ActionTableArray		'actd'
	#define	kPPCID_ActionTableView		'actv'
	#define	kPPCID_ActionTableHandler	'acth'
	
	//	е	DM Text
//	#define	kPPCID_StyleSet 			'stys'	//	virtual base class
	#define	kPPCID_TextEngine			'txte'	//	virtual base class
	#define	kPPCID_TextView				'txtv'
	#define kPPCID_TextSelection		'txts'
	#define	kPPCID_TextModel			'txmd'
	#define kPPCID_TextHandler			'txth'
	#define	kPPCID_NTextEdit			'ntxt'
	
	//	------------------------------------------------------------
	//	еее	specially defined stream object "keys"
	
	#define	kPPSK_Commander		0xffffffff
	#define kPPSK_View			0xfffffffe
	#define	kPPSK_ModelObject	0xfffffffd

#endif

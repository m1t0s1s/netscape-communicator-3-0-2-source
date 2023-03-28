//	===========================================================================
//	WTextMenuAttachments
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include	<LAttachment.h>
#include	<VText_ClassIDs.h>
#include	<VText_Resources.h>

class	LTextView;
class	LAEAction;
class	LAEStream;
class	LMenu;

//	===========================================================================

class	VTextMenuAttachment
			:	public	LAttachment
{
public:
						VTextMenuAttachment(LStream *inStream);
						VTextMenuAttachment(
							LTextView		*inTextView,
							Int16			inMenuID);

protected:

	virtual	void		CheckMenu(void);
	virtual	void		EnableEveryItem(Boolean inEnable) const;
	virtual	void		CheckEveryItem(Boolean inApplyCheck) const;

	virtual void		ExecuteSelf(
							MessageT		inMessage,
							void			*ioParam);


	virtual Boolean		ObeyCommand(
							CommandT		inCommand,
							void			*ioParam = nil) = 0;
	virtual Boolean		FindCommandStatus(
							CommandT		inCommand,
							Boolean			&outEnabled,
							Boolean			&outUsesMark,
							Char16			&outMark,
							Str255			outName) = 0;
	virtual	LAEAction *	MakeModifyAction(Int32 inForValue);
	virtual	void		WriteModifyParameters(
							LAEStream		&inStream,
							Int32			inForValue) const = 0;

	LTextView			*mTextView;
	Int16				mMenuID;

	MenuHandle			mMenuH;
	LMenu				*mMenu;
};

//	===========================================================================

class	VFontMenuAttachment
			:	public	VTextMenuAttachment
{
public:
	enum { class_ID = kVTCID_VFontMenuAttachment };

						VFontMenuAttachment(LStream *inStream);
						VFontMenuAttachment(
							LTextView		*inTextView,
							Int16			inMenuID = kVText_MENU_ID_Font);

protected:
	virtual	void		CheckMenu(void);
	virtual	Int32		FontNumberToItem(Int16 inFontNumber) const;

	virtual Boolean		ObeyCommand(
							CommandT		inCommand,
							void			*ioParam = nil);
	virtual Boolean		FindCommandStatus(
							CommandT		inCommand,
							Boolean			&outEnabled,
							Boolean			&outUsesMark,
							Char16			&outMark,
							Str255			outName);
	virtual	void		WriteModifyParameters(
							LAEStream		&inStream,
							Int32			inForValue) const;
};

//	===========================================================================

class VSizeMenuAttachment
			:	public	VTextMenuAttachment
{
public:
	enum { class_ID = kVTCID_VSizeMenuAttachment };

						VSizeMenuAttachment(LStream *inStream);
						VSizeMenuAttachment(
							LTextView		*inTextView,
							Int16			inMenuID = kVText_MENU_ID_Size);

protected:
	virtual Boolean		ObeyCommand(
							CommandT		inCommand,
							void			*ioParam = nil);
	virtual Boolean		FindCommandStatus(
							CommandT		inCommand,
							Boolean			&outEnabled,
							Boolean			&outUsesMark,
							Char16			&outMark,
							Str255			outName);
	virtual	void		WriteModifyParameters(
							LAEStream		&inStream,
							Int32			inForValue) const;
};

//	===========================================================================

class VStyleMenuAttachment
			:	public	VTextMenuAttachment
{
public:
	enum { class_ID = kVTCID_VStyleMenuAttachment };

						VStyleMenuAttachment(LStream *inStream);
						VStyleMenuAttachment(
							LTextView		*inTextView,
							Int16			inMenuID = kVText_MENU_ID_Style);

protected:
	virtual Boolean		ObeyCommand(
							CommandT		inCommand,
							void			*ioParam = nil);
	virtual Boolean		FindCommandStatus(
							CommandT		inCommand,
							Boolean			&outEnabled,
							Boolean			&outUsesMark,
							Char16			&outMark,
							Str255			outName);
	virtual	void		WriteModifyParameters(
							LAEStream		&inStream,
							Int32			inForValue) const;
};
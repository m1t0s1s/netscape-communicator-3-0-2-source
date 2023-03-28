//	===========================================================================
//	UEventUtils.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"UEventUtils.h"

Boolean	UEventUtils::KeyDownInMap(Uint16 inKeyScanCode, const KeyMap inKeys)
{
	Uint8	*p = (Uint8 *) inKeys;
	Uint8	index = inKeyScanCode >> 3;
			Assert_((16 > index) && (index >= 0));
	Uint8	subvalue = p[index];
	Uint8	subposition = inKeyScanCode & 0x07;
	Uint8	mask = 0x01 << subposition;
	
	if ((subvalue & mask) != 0)
		return true;
	else
		return false;
}

short	UEventUtils::GetModifiers(void)
{
	KeyMap		keys;
	short		modifiers = Button() ? btnState : 0;

	GetKeys(keys);
	if (KeyDownInMap(cmdKeyCode, keys))
		modifiers |= cmdKey;
	if (KeyDownInMap(shiftKeyCode, keys))
		modifiers |= shiftKey;
	if (KeyDownInMap(alphaLockKeyCode, keys))
		modifiers |= alphaLock;
	if (KeyDownInMap(optionKeyCode, keys))
		modifiers |= optionKey;
	if (KeyDownInMap(controlKeyCode, keys))
		modifiers |= controlKey;
	
	return modifiers;
}

void	UEventUtils::FleshOutEvent(Boolean inOldButtonWasDown, EventRecord *ioEvent)
/*
	The event manager leaves most fields of an event record undefined unless
	they're specifically related to a given event.
	
	This can cause problems with state machine event handlers and this routine
	helps stuff values into the undefined fields.
*/
//	(There might still be some important stuffing missing.)
{
	ThrowIfNULL_(ioEvent);
	switch (ioEvent->what) {
	
		case osEvt:	//	Hail to Patrick Doane for this case...
		{
			Uint8	osEvtFlag = ((Uint32) ioEvent->message) >> 24;

			if (osEvtFlag == mouseMovedMessage) {
				ioEvent->what = nullEvent;
			} else {
				break;
			}
				
			//	Fall through!
		}

		case keyDown:
		case keyUp:
		case autoKey:	
		case mouseDown:
		case mouseUp:
		case nullEvent:
		{
			//	where
			if ( !((ioEvent->what == mouseDown) || (ioEvent->what == mouseUp))) {
				GetMouse(&ioEvent->where);
				LocalToGlobal(&ioEvent->where);
			}
	
			//	modifiers
			ioEvent->modifiers = UEventUtils::GetModifiers();
		
			//	fix btnState
			if (ioEvent->what == mouseDown) {
				ioEvent->modifiers |= btnState;
			} else if (ioEvent->what == mouseUp) {
				ioEvent->modifiers &= ~btnState;
			} else {
				ioEvent->modifiers &= ~btnState;
				if (inOldButtonWasDown)
					ioEvent->modifiers |= btnState;
			}
			break;
		}
		
		default:
			break;
	}
}	


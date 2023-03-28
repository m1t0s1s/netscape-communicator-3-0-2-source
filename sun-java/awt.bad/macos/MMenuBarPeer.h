#pragma once

#include <LPane.h>

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_MenuBar.h"
#include "java_awt_Menu.h"
#include "java_awt_Frame.h"
#include "sun_awt_macos_MMenuPeer.h"
};

struct MenuListItem {
	Hjava_awt_Menu			*menu;
	Boolean					isSelectionSet;
	UInt16					menuLeft;
	UInt16					menuRight;
};

typedef struct MenuListItem MenuListItem;

struct ActiveMenuRecord {
	Hjava_awt_Menu			*javaMenu;
	MenuHandle				macMenu;
};

typedef struct ActiveMenuRecord ActiveMenuRecord;

class MMenuBarPeer : public LPane {

	public:
		
		LList					*mMenuList;
		LList					*mHelpMenuList;
		LList					*mActiveMenuList;
		struct Hjava_awt_Frame  *mContainerFrameHandle;
				
		Hjava_awt_MenuBar		*mMenuBarObjectHandle;
		
		enum {
			class_ID 			= 'mbpr',
			kMenuBarHeight		= 14
		};
		
		MMenuBarPeer(struct SPaneInfo &newScrollbarInfo);
		~MMenuBarPeer();
		
		Boolean 			FocusDraw();
		virtual void 		DrawSelf();
		virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);
		void				RebuildMenuListGeometry();

		MenuHandle			BuildMenuHandle(Hjava_awt_Menu *javaMenu);

		void				RemoveAndDisposeMenus();
		
		Hjava_awt_MenuItem	*DispatchMenuSelection(UInt32 selectionResult);
		
};


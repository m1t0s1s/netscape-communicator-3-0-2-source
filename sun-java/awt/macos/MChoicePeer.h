 #pragma once

#include <LPane.h>
#include <Menus.h>
#include "MComponentPeer.h"

enum {
	kChoicePeerDefaultHeight			= 15,
	kChoicePeerItemTextIndent			= 10,
	kChoicePeerItemTextIndentUp			= 4,
	kChoicePeerArrowRightIndent			= 13,
	kChoicePeerArrowDownIndent			= 5,
	kChoicePeerDefaultExtraWidth		= 2
};

class MChoicePeer : public LPane {

	public:
		
		enum {
			class_ID 	= 'pmpr'
		};
		
		MChoicePeer(struct SPaneInfo &newScrollbarInfo);
		~MChoicePeer();

		Boolean 			FocusDraw();
		
		virtual Int32		GetValue() const;
		
		virtual void		DrawSelf();
		virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);
		
		virtual void		CalculateMenuDimensions(short &width);

};


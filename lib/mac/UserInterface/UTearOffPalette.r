/* UFloatPalette.r */
/* The standard window resource for palette windows */

#define SystemSevenOrLater 1

#include <Types.r>

#include <PowerPlant.r>

#define FLOAT_ID -19844

resource 'WIND' (FLOAT_ID , "", purgeable) {
	{0, 0, 50, 300},
	1985, invisible, goAway,
	0x0,	// refCon
	"Floater",
	noAutoCenter
};

resource 'PPob' (FLOAT_ID, "Floater") { {
	ClassAlias { 'DRFW' },
	ObjectData { Window {
		FLOAT_ID, floating,
		hasCloseBox, hasTitleBar, noResize, noSizeBox, noZoom, noShowNew,
		enabled, noTarget, hasGetSelectClick, hasHideOnSuspend, noDelaySelect,
		hasEraseOnUpdate,
		0, 0,						//	Minimum width, height
		screenSize, screenSize,		//	Maximum width, height
		screenSize, screenSize,		//	Standard width, height
		0							//	UserCon
	} }
} };

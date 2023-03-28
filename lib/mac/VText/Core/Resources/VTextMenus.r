//	===========================================================================
//	VTexMenus.r	
//	===========================================================================

#include	"VText_Resources.h"
#include	<Types.r>
#include	<PowerPlant.r>

resource 'MENU' (kVText_MENU_ID_Font, "Font") {
	kVText_MENU_ID_Font,
	textMenuProc,
	allEnabled,
	enabled,
	"Font",
	{}
};

resource 'MENU' (kVText_MENU_ID_Size, "Size") {
	kVText_MENU_ID_Size,
	textMenuProc,
	0x7FFFFEFF,
	enabled,
	"Size",
	{
		"6", noIcon, noKey, noMark, plain,
		"8", noIcon, noKey, noMark, plain,
		"9", noIcon, noKey, noMark, plain,
		"10", noIcon, noKey, noMark, plain,
		"12", noIcon, noKey, noMark, plain,
		"18", noIcon, noKey, noMark, plain,
		"24", noIcon, noKey, noMark, plain,
		"36", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Larger", noIcon, "+", noMark, plain,
		"Smaller", noIcon, "-", noMark, plain,
		"Other ()...", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (kVText_MENU_ID_Style, "Style") {
	kVText_MENU_ID_Style,
	textMenuProc,
	allEnabled,
	enabled,
	"Style",
	{
		"Plain", noIcon, "T", noMark, plain,
		"Bold", noIcon, "B", noMark, 1,
		"Italic", noIcon, "I", noMark, 2,
		"Underline", noIcon, "U", noMark, 4,
		"Outline", noIcon, noKey, noMark, 8,
		"Shadow", noIcon, noKey, noMark, 16,
		"Condense", noIcon, noKey, noMark, 32,
		"Extend", noIcon, noKey, noMark, 64
	}
};


resource 'Mcmd' (kVText_MENU_ID_Font, "Font") {{
}};

resource 'Mcmd' (kVText_MENU_ID_Size, "Size") {{
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	cmd_UseMenuItem,
	msg_Nothing,
	cmd_FontLarger,
	cmd_FontSmaller,
	cmd_FontOther
}};

resource 'Mcmd' (kVText_MENU_ID_Style, "Style") {{
	cmd_Plain,
	cmd_Bold,
	cmd_Italic,
	cmd_Underline,
	cmd_Outline,
	cmd_Shadow,
	cmd_Condense,
	cmd_Extend
}};

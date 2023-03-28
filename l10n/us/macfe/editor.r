// A lot of these resources overwrite resource defined in macfe.r for the browser.
// No doubt, someone will change a resource in one place and not the other and
// thus, the two will be out of synch.
// I guess that is what QA is for....

#include "Types.r"
#include "SysTypes.r"
#include "Browser_Defines.h"
#include "resae.h"					// Apple event constants
#include "resgui.h"					// main window constants
#include "mversion.h"				// version numbers, application name, etc
#include "csid.h"

#include "PowerPlant.r"
// Use the Mercutio MDEF
#define	nsMenuProc	19999

resource 'MBAR' ( EDIT_MENUBAR ) {{
	APPLE_MENU_ID,
	EDITOR_FILE_MENU_ID,
	EDITOR_EDIT_MENU_ID,
	EDITOR_VIEW_MENU_ID,			/* Keep the same for now. Robin doesn't seem sure about this yet. */
	EDITOR_INSERT_MENU_ID,
	EDITOR_FORMAT_MENU_ID,
	EDITOR_OPTIONS_MENU_ID,
	WINDOW_MENU_ID			/* We can't add items to this menu for the editor until we change some code...*/
}};

resource 'MENU' ( FILE_MENU_ID, "File-Main" ) {
	FILE_MENU_ID, nsMenuProc, allEnabled, enabled, "File", {
		"New Web Browser",		noIcon, "N",	noMark,	plain,
		"New Document",			noIcon, hierarchicalMenu,	"\0xED",	plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Edit Document",		noIcon,	noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Open Location…",		noIcon, "L",	noMark, plain,
		"Open File in Browser…",noIcon, "O",	noMark, plain,
		"Open File in Editor…",	noIcon, noKey,	noMark, plain,
		"Save As…",				noIcon, noKey,	noMark, plain,
		"Upload File…",			noIcon, noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"New Mail Message",		noIcon,	"M",	noMark,	plain,
		"Mail Location…",		noIcon, noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Page Setup…",			noIcon, noKey,	noMark, plain,
		"Print…",				noIcon, "P",	noMark, plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Close",				noIcon, "W",	noMark, plain,
		"Quit",					noIcon, "Q",	noMark, plain
	}
};

resource 'Mcmd' ( FILE_MENU_ID, "File-Main" ) {{
	cmd_New,
	cmd_New_Document_Heirarchical_Menu,
	msg_Nothing,
	cmd_EditDocument,
	msg_Nothing,
	cmd_OpenURL,
	cmd_Open,
	cmd_OpenFileEditor,
	cmd_SaveAs,
	cmd_FTPUpload,
	msg_Nothing,
	cmd_MailTo,
	cmd_MailDocument,
	msg_Nothing,
	cmd_PageSetup,
	cmd_Print,
	msg_Nothing,
	cmd_Close,
	cmd_Quit
}};

resource 'MENU' ( OPTIONS_MENU_ID, "Options-Main" ) {
	OPTIONS_MENU_ID, nsMenuProc, 0x7FFFFFFD, enabled, "Options", {
		"General Preferences…",	noIcon,	noKey,	noMark, plain,
		"Editor Preferences…",	noIcon,	noKey,	noMark, plain,
		"Mail and News Preferences…", noIcon,	noKey,	noMark,	plain,
		"Network Preferences…",	noIcon,	noKey,	noMark,	plain,
		"Security Preferences…",	noIcon,	noKey,	noMark,	plain,
		"-",					noIcon,	noKey,	noMark, plain,
		"Show Toolbar",			noIcon, noKey,	noMark, plain,
		"Show Location",		noIcon,	noKey,	noMark, plain,
		"Show Directory Buttons",	noIcon,	noKey,	noMark, plain,
		"Show Java Console",	noIcon,	noKey,	noMark, plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Auto Load Images",		noIcon,	noKey,	noMark, plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Document Encoding",	noIcon,	hierarchicalMenu, "\0xE9", plain,
	}
};

resource 'Mcmd' ( OPTIONS_MENU_ID, "Options-Main" ) {{
	cmd_EditGeneral,
	cmd_EditEditor,
	cmd_EditMailNews,
	cmd_EditNetwork,
	cmd_EditSecurity,
	msg_Nothing,
	cmd_ToggleToolbar,
	cmd_ToggleURLField,
	cmd_ShowDirectory,
	cmd_ShowJavaConsole,
	msg_Nothing,
	cmd_DelayImages,
	msg_Nothing,
	cmd_DocumentEncoding,
}};

/*-----------------------------------------------------------------------------
	Editor Menus
-----------------------------------------------------------------------------*/


resource 'MENU' ( EDITOR_FILE_MENU_ID, "File-Editor" ) {
	EDITOR_FILE_MENU_ID, nsMenuProc, allEnabled, enabled, "File", {
		"New Web Browser",		noIcon, "N",	noMark,	plain,
		"New Document",			noIcon, hierarchicalMenu,	"\0xED",	plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Browse Document",		noIcon,	noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Open Location…",		noIcon, "L",	noMark, plain,
		"Open File…",			noIcon, "O",	noMark, plain,
		"Save",					noIcon, "S",	noMark, plain,
		"Save As…",				noIcon, noKey,	noMark, plain,
		"Publish…",				noIcon, noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"New Mail Message",		noIcon,	"M",	noMark,	plain,
		"Mail Document…",		noIcon, noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Page Setup…",			noIcon, noKey,	noMark, plain,
		"Print…",				noIcon, "P",	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Close",				noIcon, "W",	noMark, plain,
		"Quit",					noIcon, "Q",	noMark, plain
	}
};

resource 'Mcmd' ( EDITOR_FILE_MENU_ID, "File-Editor" ) {{
	cmd_New,
	cmd_New_Document_Heirarchical_Menu,
	msg_Nothing,
	cmd_BrowseDocument,
	msg_Nothing,
	cmd_OpenURL,
	cmd_OpenFileEditor,
	cmd_Save,
	cmd_SaveAs,
	cmd_Publish,
	msg_Nothing,
	cmd_MailTo,
	cmd_MailDocument,
	msg_Nothing,
	cmd_PageSetup,
	cmd_Print,
	msg_Nothing,
	cmd_Close,
	cmd_Quit
}};

resource 'MENU' ( EDITOR_NEW_DOCUMENT_MENU_ID, "New Document-Editor") {
	EDITOR_NEW_DOCUMENT_MENU_ID, nsMenuProc, allEnabled, enabled, "", {
		"Blank", 				noIcon, "N", noMark, extend,
		"From Template", 		noIcon, noKey, noMark, plain,
		"From Wizard…", 		noIcon, noKey, noMark, plain	}
};

resource 'Mcmd' ( EDITOR_NEW_DOCUMENT_MENU_ID, "New Document-Editor" ) {{
	cmd_NewWindowEditor,
	cmd_New_Document_Template,
	cmd_New_Document_Wizard	
}};


resource 'MENU' ( EDITOR_EDIT_MENU_ID, "Edit-Editor") {
	EDITOR_EDIT_MENU_ID, nsMenuProc, allEnabled, enabled, "Edit", {
		"Undo",					noIcon, "Z",	noMark, plain,
		"Redo",					noIcon, noKey,	noMark, plain,		/* the only difference */
		"-",					noIcon, noKey,	noMark, plain,
		"Cut",					noIcon, "X",	noMark, plain,
		"Copy",					noIcon, "C",	noMark, plain,
		"Paste",				noIcon, "V",	noMark, plain,
		"Clear",				noIcon, noKey,	noMark, plain,
		"Select All",			noIcon, "A",	noMark, plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Select Table",			noIcon,	noKey,	noMark,	plain,
		"Delete Table",			noIcon,	hierarchicalMenu, "\0xEF",	plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Find…",				noIcon, "F", 	noMark, plain,
		"Find Again",			noIcon, "G", 	noMark, plain,
	}
};

resource 'Mcmd' ( EDITOR_EDIT_MENU_ID, "Edit-Editor" ) {{
	cmd_Undo,
	cmd_Redo,														/* the only difference */
	msg_Nothing,
	cmd_Cut,
	cmd_Copy,
	cmd_Paste,
	cmd_Clear,
	cmd_SelectAll,
	msg_Nothing,
	cmd_Select_Table,
	cmd_Edit_Table_Heirarchical_Menu,
	msg_Nothing,
	cmd_Find,
	cmd_FindAgain
}};

resource 'MENU' ( EDITOR_EDIT_TABLE_MENU_ID, "Edit-Table-Editor") {
	EDITOR_EDIT_TABLE_MENU_ID, nsMenuProc, allEnabled, enabled, "Table", {
		"Table",				noIcon, noKey,	noMark, plain,
		"Row",					noIcon, noKey,	noMark, plain,
		"Column",				noIcon, noKey,	noMark, plain,
		"Cell",					noIcon, noKey,	noMark, plain,
	}
};

resource 'Mcmd' ( EDITOR_EDIT_TABLE_MENU_ID, "Edit-Table-Editor" ) {{
	cmd_Delete_Table,
	cmd_Delete_Row,												
	cmd_Delete_Col,
	cmd_Delete_Cell
}};

resource 'MENU' ( EDITOR_VIEW_MENU_ID, "View-Editor" ) {
	EDITOR_VIEW_MENU_ID, nsMenuProc, allEnabled, enabled, "View", {
		"Reload",					noIcon, "R", 	noMark, plain,
		"Load Images",				noIcon, noKey,	noMark, plain,
		"Refresh",					noIcon, noKey,	noMark, plain,
		"-",						noIcon,	noKey,	noMark,	plain,
		"View Document Source", 			noIcon, noKey, 	noMark, plain,
		"Edit Document Source", 			noIcon, noKey, 	noMark, plain,
		"Document Info",			noIcon,	noKey,	noMark,	plain,
/*		"-",						noIcon,	noKey,	noMark,	plain,
		"Display Paragraph Marks",	noIcon, noKey,	noMark, plain,
		"Display Table Boundaries",	noIcon, noKey,	noMark, plain, */
	}
};

resource 'Mcmd' ( EDITOR_VIEW_MENU_ID, "View-Editor" ) {{
	cmd_Reload,
	cmd_LoadImages,
	cmd_Refresh,
	msg_Nothing,
	cmd_ViewSource,
	cmd_EditSource,
	cmd_DocumentInfo,
/*	msg_Nothing,
	cmd_DisplayParagraphMarks,
	cmd_DisplayTableBoundaries, */
}};

resource 'MENU' ( EDITOR_INSERT_MENU_ID, "Insert-Editor") {
	EDITOR_INSERT_MENU_ID, nsMenuProc, allEnabled, enabled, "Insert", {
		"Link…",					noIcon,	noKey,	noMark, plain,
		"Target [Named Anchor]…",	noIcon,	noKey,	noMark, plain,
		"Image…",					noIcon,	noKey,	noMark, plain,
		"Table",					noIcon, hierarchicalMenu, "\0xF0", plain,
		"Horizontal Line…",			noIcon,	noKey,	noMark, plain,
		"HTML Tag…",				noIcon,	noKey,	noMark, plain,
		"-",						noIcon, noKey,	noMark, plain,
		"New Line Break",			noIcon,	noKey,	noMark, plain,
		"Break below Image(s)",		noIcon,	noKey,	noMark, plain,
		"Nonbreaking Space",		noIcon,	noKey,	noMark, plain,
	}
};

resource 'Mcmd' ( EDITOR_INSERT_MENU_ID, "Insert-Editor" ) {{
	cmd_Insert_Link,
	cmd_Insert_Target,
	cmd_Insert_Image,
	cmd_Insert_Table_Heirarchical_Menu,
	cmd_Insert_Line,
	cmd_Insert_Unknown_Tag,
	msg_Nothing,
	cmd_Insert_LineBreak,
	cmd_Insert_BreakBelowImage,
	cmd_Insert_NonbreakingSpace,
}};

resource 'MENU' ( EDITOR_INSERT_TABLE_MENU_ID, "Insert-Table-Editor") {
	EDITOR_INSERT_TABLE_MENU_ID, nsMenuProc, allEnabled, enabled, "Insert Table", {
		"Table…",			noIcon,	noKey,	noMark, plain,
		"Row",				noIcon,	"R",	noMark, extend,
		"Column",			noIcon,	"C",	noMark, extend,
		"Cell",				noIcon, noKey,	noMark, plain,
	}
};

resource 'Mcmd' ( EDITOR_INSERT_TABLE_MENU_ID, "Insert-Table-Editor" ) {{
	cmd_Insert_Table,
	cmd_Insert_Row,
	cmd_Insert_Col,
	cmd_Insert_Cell

}};




resource 'MENU' ( EDITOR_FORMAT_MENU_ID, "Format-Editor") {
	EDITOR_FORMAT_MENU_ID, nsMenuProc, allEnabled, enabled, "Format", {
		"Text…",				noIcon,	"T",	noMark, plain,
		"Link…",				noIcon,	"L",	noMark, extend,
		"Target [Named Anchor]…",				noIcon,	noKey,	noMark, plain,
		"Image…",				noIcon,	noKey,	noMark, plain,
		"Table…",					noIcon, noKey,	noMark, plain,
		"Horizontal Line…",		noIcon,	noKey,	noMark, plain,
		"HTML Tag…",		noIcon,	noKey,	noMark, plain,
		"Document…",			noIcon,	noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Remove Links",					noIcon, noKey,	noMark, plain,
		"-",					noIcon, noKey,	noMark, plain,
		"Character",			noIcon,	hierarchicalMenu, "\0xEA", plain,
		"Font Size",			noIcon,	hierarchicalMenu, "\0xEC", plain,
		"Paragraph",			noIcon,	hierarchicalMenu, "\0xEB", plain,
	}
};

resource 'Mcmd' ( EDITOR_FORMAT_MENU_ID, "Format-Editor" ) {{
	cmd_Format_Text,
	cmd_Format_Link,
	cmd_Format_Target,
	cmd_Format_Image,
	cmd_Format_Table,
	cmd_Format_Line,
	cmd_Format_Unknown_Tag,
	cmd_Format_Document,
	msg_Nothing,
	cmd_Remove_Links,
	msg_Nothing,
	cmd_Character_Hierarchical_Menu,
	cmd_Font_Size_Hierarchical_Menu,
	cmd_Paragraph_Hierarchical_Menu,
}};

resource 'MENU' ( EDITOR_FORMAT_CHARACTER_MENU_ID, "Format Character-Editor") {
	EDITOR_FORMAT_CHARACTER_MENU_ID, nsMenuProc, allEnabled, enabled, "", {
		"Bold", 							noIcon, "B", noMark, extend,
		"Italic", 							noIcon, "I", noMark, extend,
		"Fixed Width", 						noIcon, "T", noMark, extend,
		"Superscript", 						noIcon, noKey, noMark, plain,
		"Subscript", 						noIcon, noKey, noMark, plain,
		"Strikethrough", 					noIcon, noKey, noMark, plain,
		"Blink", 							noIcon, noKey, noMark, plain,
		"-", 								noIcon, noKey, noMark, plain,
		"Text Color…", 							noIcon, noKey, noMark, plain,
		"Default Color", 							noIcon, noKey, noMark, plain,
		"-", 								noIcon, noKey, noMark, plain,
		"JavaScript (Server)", 							noIcon, noKey, noMark, plain,
		"JavaScript (Client)", 							noIcon, noKey, noMark, plain,
		"-", 								noIcon, noKey, noMark, plain,
		"Clear all character styles", 		noIcon, "K", noMark, plain	}
};

resource 'Mcmd' ( EDITOR_FORMAT_CHARACTER_MENU_ID, "Format Character-Editor" ) {{
	cmd_Format_Character_Bold,
	cmd_Format_Character_Italic,
	cmd_Format_Character_Fixed_Width,
	cmd_Format_Character_Superscript,
	cmd_Format_Character_Subscript,
	cmd_Format_Character_Strikeout,
	cmd_Format_Character_Blink,
	msg_Nothing,
	cmd_Format_FontColor,
	cmd_Format_DefaultFontColor,
	msg_Nothing,
	cmd_Format_FontJavaScriptServer,
	cmd_Format_FontJavaScriptClient,
	msg_Nothing,
	cmd_Format_Character_ClearAll,	
}};

resource 'MENU' ( EDITOR_FORMAT_PARAGRAPH_MENU_ID, "Format Paragraph-Editor") {
	EDITOR_FORMAT_PARAGRAPH_MENU_ID, nsMenuProc, allEnabled, enabled, "", {
		"Normal", 					noIcon, noKey, noMark, plain,
		"Heading 1", 				noIcon, noKey, noMark, plain,
		"Heading 2", 				noIcon, noKey, noMark, plain,
		"Heading 3", 				noIcon, noKey, noMark, plain,
		"Heading 4", 				noIcon, noKey, noMark, plain,
		"Heading 5", 				noIcon, noKey, noMark, plain,
		"Heading 6", 				noIcon, noKey, noMark, plain,
		"Address", 					noIcon, noKey, noMark, plain,
		"Formatted", 				noIcon, noKey, noMark, plain,
		"List Item, LI", 			noIcon, noKey, noMark, plain,
		"Description Title, DT", 	noIcon, noKey, noMark, plain,
		"Description Text, DD", 	noIcon, noKey, noMark, plain,
		"-", 						noIcon, noKey, noMark, plain,
		"Indent one level", 		noIcon, noKey, noMark, plain,
		"Remove one indent level", 	noIcon, noKey, noMark, plain	}
};

resource 'Mcmd' ( EDITOR_FORMAT_PARAGRAPH_MENU_ID, "Format Paragraph-Editor" ) {{
	cmd_Format_Paragraph_Normal,
	cmd_Format_Paragraph_Head1,
	cmd_Format_Paragraph_Head2,
	cmd_Format_Paragraph_Head3,
	cmd_Format_Paragraph_Head4,
	cmd_Format_Paragraph_Head5,
	cmd_Format_Paragraph_Head6,
	cmd_Format_Paragraph_Address,
	cmd_Format_Paragraph_Formatted,
	cmd_Format_Paragraph_List_Item,
	cmd_Format_Paragraph_Desc_Title,
	cmd_Format_Paragraph_Desc_Text,
	msg_Nothing,
	cmd_Format_Paragraph_Indent,
	cmd_Format_Paragraph_UnIndent	
}};

resource 'MENU' ( EDITOR_FORMAT_FONT_SIZE_MENU_ID, "Format Font Size-Editor") {
	EDITOR_FORMAT_FONT_SIZE_MENU_ID, nsMenuProc, allEnabled, enabled, "", {
		" -2", 		noIcon, noKey, noMark, plain,
		" -1", 		noIcon, noKey, noMark, plain,
		" 0", 		noIcon, noKey, noMark, plain,
		" +1", 		noIcon, noKey, noMark, plain,
		" +2", 		noIcon, noKey, noMark, plain,
		" +3", 		noIcon, noKey, noMark, plain,
		" +4", 		noIcon, noKey, noMark, plain	}
};

resource 'Mcmd' ( EDITOR_FORMAT_FONT_SIZE_MENU_ID, "Format Font Size-Editor" ) {{
	cmd_Format_Font_Size_N2,
	cmd_Format_Font_Size_N1,
	cmd_Format_Font_Size_0,
	cmd_Format_Font_Size_P1,
	cmd_Format_Font_Size_P2,
	cmd_Format_Font_Size_P3,
	cmd_Format_Font_Size_P4	
}};

resource 'MENU' ( EDITOR_OPTIONS_MENU_ID, "Options-Editor") {
	EDITOR_OPTIONS_MENU_ID, nsMenuProc, allEnabled, enabled, "Options", {
		"General Preferences…",			noIcon,	noKey,	noMark, plain,
		"Editor Preferences…",			noIcon,	noKey,	noMark, plain,
		"Mail and News Preferences…",	noIcon,	noKey,	noMark,	plain,
		"Network Preferences…",			noIcon,	noKey,	noMark,	plain,
		"Security Preferences…",		noIcon,	noKey,	noMark,	plain,
		"-",							noIcon,	noKey,	noMark, plain,
		"Show File/Edit Toolbar",		noIcon, noKey,	noMark, plain,
		"Show Character Format Toolbar",		noIcon,	noKey,	noMark, plain,
		"Show Paragraph Format Toolbar",		noIcon,	noKey,	noMark, plain,
		"Show Java Console",				noIcon,	noKey,	noMark, plain,
		"Auto Load Images",				noIcon,	noKey,	noMark, plain,
		"Show FTP File Information",	noIcon,	noKey,	noMark, plain,
		"-",							noIcon,	noKey,	noMark, plain,
		"Document Encoding",			noIcon,	hierarchicalMenu, "\0xE9", plain,
	}
};

resource 'Mcmd' ( EDITOR_OPTIONS_MENU_ID, "Options-Editor" ) {{
	cmd_EditGeneral,
	cmd_EditEditor,
	cmd_EditMailNews,
	cmd_EditNetwork,
	cmd_EditSecurity,
	msg_Nothing,
	cmd_Toggle_FileEdit_Toolbar,
	cmd_Toggle_Character_Toolbar,
	cmd_Toggle_Paragraph_Toolbar,
	cmd_ShowJavaConsole,
	cmd_DelayImages,
	cmd_Show_FTP_File_Information,
	msg_Nothing,
	cmd_DocumentEncoding,
}};

resource 'STR '	( NO_SRC_EDITOR_PREF_SET + 0, "", purgeable ) {
"The HTML editor preference is not set. Please choose an editor to use."
};


// • Other stuff
resource 'STR#' ( HELP_URLS_MENU_STRINGS, "Help URL Menu Entries", purgeable ) {{
	"About Netscape",
	"About Plugins",
	"Registration Information",
	"Software",
	"Web Page Starter",
	"(-",
	"Handbook",
	"Release Notes",
	"Frequently Asked Questions",
	"On Security",
	"(-",
	"How to Give Feedback",
	"How to Get Support",
	"How to Create Web Services",
}};
#include "custom.r"
resource 'STR#' ( HELP_URLS_RESID, "Help URLs", purgeable ) {{
	ABOUT_LOCATION,
	ABOUT_PLUGINS_LOCATION,
	REGISTRATION_LOCATION,
	ABOUT_SOFTWARE_LOCATION,
	WEB_PAGE_STARTER_LOCATION,
	"",
	MANUAL_LOCATION,			// 
	VERSION_LOCATION,			// release notes
	FAQ_LOCATION,				// FAQ
	HELPONSEC_LOCATION,
	"",
	FEEDBACK_LOCATION,	// FEEDBACK
	SUPPORT_LOCATION,
	WEBSERVICES_LOCATION,
}};


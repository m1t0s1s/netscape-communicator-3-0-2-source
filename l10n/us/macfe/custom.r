#define		APPLICATION_NAME		"Netscape"
#define		TRADEMARK_NAME			APPLICATION_NAME "ª "

#define		VERSION_CORP_STR		"Netscape Communications Corp."

#undef BETA
#undef ALPHA

#define		VERSION_MAJOR_STR		"3.0"
#define		VERSION_MINOR_STR		"b5"
#define		VERSION_STRING			VERSION_MAJOR_STR VERSION_MINOR_STR
#define 	VERSION_LANG			"en"	// e.g. en, ja, de, fr
#define 	VERSION_COUNTRY			"_US"		// e.g.,  _JP, _DE, _FR, _US
#define 	VERSION_LOCALE			VERSION_LANG VERSION_COUNTRY
#define 	VERSION_MAJOR			3
#define		VERSION_MINOR			0
#define 	VERSION_MICRO			0
#define		VERSION_KIND			beta
#define		COPYRIGHT_STRING		", © Netscape Communications Corporation 1995-6"


#define		GETINFO_DESC			VERSION_CORP_STR

#define		GETINFO_VERSION			VERSION_STRING COPYRIGHT_STRING


#define		USER_AGENT_PPC_STRING		"(Macintosh; %s; PPC)"
#define		USER_AGENT_68K_STRING		"(Macintosh; %s; 68K)"

#define		USER_AGENT_NAME			"Mozilla"

#ifdef	powerc
#define		USER_AGENT_VERSION		VERSION_STRING " [" VERSION_LOCALE "] " USER_AGENT_PPC_STRING
#else
#define		USER_AGENT_VERSION		VERSION_STRING " [" VERSION_LOCALE "] " USER_AGENT_68K_STRING
#endif

/*-----------------------------------------------------------------------------
	Version Numbers
-----------------------------------------------------------------------------*/

resource 'vers' (1, "Program") {
	VERSION_MAJOR,
	VERSION_MINOR,
	VERSION_KIND,
	VERSION_MICRO,
	verUS,
	USER_AGENT_VERSION,
	GETINFO_VERSION
};

resource 'vers' (2, "Suite") {
	VERSION_MAJOR,
	VERSION_MINOR,
	VERSION_KIND,
	VERSION_MICRO,
	verUS,
	USER_AGENT_VERSION,
	GETINFO_DESC
};

resource 'STR#' ( ID_STRINGS, "IDs", purgeable) {{
	APPLICATION_NAME,
	VERSION_STRING,
	USER_AGENT_NAME,
	USER_AGENT_68K_STRING,
	USER_AGENT_PPC_STRING,
}};

resource 'MENU' ( DIRECTORY_MENU_ID, "Directory" ) {
	DIRECTORY_MENU_ID, textMenuProc, 0x7FFFFFFD, enabled, "Directory", {
		"Netscape's Home",				noIcon,	noKey,	noMark,	plain,
		"WhatÕs New?",			noIcon,	noKey,	noMark,	plain,
		"WhatÕs Cool?",			noIcon,	noKey,	noMark,	plain,
		"-",					noIcon,	noKey,	noMark,	plain,
		"Netscape Galleria",	noIcon,	noKey,	noMark,	plain,
		"Netscape Destinations",	noIcon,	noKey,	noMark,	plain,
		"Internet Search",		noIcon,	noKey,	noMark,	plain,
		"People",	noIcon,	noKey,	noMark,	plain,
		"About the Internet",	noIcon,	noKey,	noMark,	plain,
	}
};

resource 'Mcmd' ( DIRECTORY_MENU_ID, "Directory" ) { {
	DIR_MENU_BASE,
	DIR_MENU_BASE + 1,
	DIR_MENU_BASE + 2,
	DIR_MENU_BASE + 3,
	DIR_MENU_BASE + 4,
	DIR_MENU_BASE + 5,
	DIR_MENU_BASE + 6,
	DIR_MENU_BASE + 7,
	DIR_MENU_BASE + 8,
	DIR_MENU_BASE + 9,
	DIR_MENU_BASE + 10,
	DIR_MENU_BASE + 11,
	DIR_MENU_BASE + 12,
	DIR_MENU_BASE + 13,
	DIR_MENU_BASE + 14,
	DIR_MENU_BASE + 15,
	DIR_MENU_BASE + 16,
	DIR_MENU_BASE + 17,
	DIR_MENU_BASE + 18,
	DIR_MENU_BASE + 19,
	DIR_MENU_BASE + 20,
	DIR_MENU_BASE + 21,
	DIR_MENU_BASE + 22,
	DIR_MENU_BASE + 23,
	DIR_MENU_BASE + 24,
} };

#define		NETSCAPE_LOCATION		"http://home.netscape.com/"
#define		ENG_LOCATION			NETSCAPE_LOCATION "eng/mozilla/" VERSION_MAJOR_STR "/"
#define		HOME_LOCATION			NETSCAPE_LOCATION "home/"

#define		WELCOME_LOCATION		"welcome.html"

#define		WHATSNEW_LOCATION		HOME_LOCATION "whats-new.html"
#define		WHATSCOOL_LOCATION		HOME_LOCATION "whats-cool.html"
#define		NEWSRC_LOCATION			"news:"
#define		MARKETPLACE_LOCATION	HOME_LOCATION "netscape-galleria.html"
#define		DIRECTORY_LOCATION		HOME_LOCATION "internet-directory.html"
#define		SEARCH_LOCATION			HOME_LOCATION "internet-search.html"
#define		WHITEPAGES_LOCATION		HOME_LOCATION "internet-white-pages.html"
#define		ABOUTINTERNET_LOCATION	HOME_LOCATION "about-the-internet.html"

#define		ABOUT_LOCATION			"about:"
#define		ABOUT_PLUGINS_LOCATIONS	"about:plugins"
#define		REGISTRATION_LOCATION	NETSCAPE_LOCATION "cgi-bin/reginfo.cgi"
#define     MANUAL_LOCATION         ENG_LOCATION "handbook/"		
#define		ESCAPES_LOCATION		NETSCAPE_LOCATION "escapes/index.html"
#define 	VERSION_LOCATION		ENG_LOCATION "relnotes/mac-" VERSION_STRING ".html"
#define		FAQ_LOCATION			ENG_LOCATION "faq.html"
#define		HELPONSEC_LOCATION		NETSCAPE_LOCATION "info/security-doc.html"
#define		FEEDBACK_LOCATION		"http://cgi.netscape.com/cgi-bin/auto_bug.cgi"
#define		SUPPORT_LOCATION		HOME_LOCATION "how-to-get-support.html"
#define		WEBSERVICES_LOCATION	HOME_LOCATION "how-to-create-web-services.html"
#define		SOFTWARE_LOCATION		NETSCAPE_LOCATION "comprod/upgrades/index.html"

#define		NEWDOC_TEMPLATE_LOCATION	NETSCAPE_LOCATION "assist/net_sites/starter/samples/index.html"
#define		NEWDOC_WIZARD_LOCATION		HOME_LOCATION "gold2.0_wizard.html"

#define		ABOUT_PLUGINS_LOCATION		"about:plugins"
#define		ABOUT_SOFTWARE_LOCATION		NETSCAPE_LOCATION	"comprod/upgrades/index.html"
#define		WEB_PAGE_STARTER_LOCATION	HOME_LOCATION 		"starter.html"

resource 'STR#'	( DIR_BUTTON_BASE, "Directory URLs", purgeable ) {{
	WHATSNEW_LOCATION;
	WHATSCOOL_LOCATION;
	ESCAPES_LOCATION;
	SEARCH_LOCATION;
	WHITEPAGES_LOCATION;
	SOFTWARE_LOCATION;
}};

resource 'STR#'	( DIR_MENU_BASE, "Directory menu URLs", purgeable ) {{
	NETSCAPE_LOCATION;
	WHATSNEW_LOCATION;
	WHATSCOOL_LOCATION;
	NEWSRC_LOCATION;
	MARKETPLACE_LOCATION;
	ESCAPES_LOCATION;
	SEARCH_LOCATION;
	WHITEPAGES_LOCATION;
	ABOUTINTERNET_LOCATION;
}};

resource 'STR ' ( LOGO_BUTTON_URL_RESID, "Logo Button URL", purgeable ) {
	NETSCAPE_LOCATION
};

resource 'STR ' ( CO_BRAND_BUTTON_URL_RESID, "Co-brand button URL", purgeable ) {
	NETSCAPE_LOCATION
};

resource 'STR#' ( 300, "Pref file names", purgeable ) {{
	PREFS_FOLDER_NAME;						// 1
	PREFS_FILE_NAME;						// 2
	"Global History";						// 3 Global history
	"Cache Ä";								// 4 Cache folder name
	"CCache log";							// 5 Cache file allocation table name
	"newsrc";								// 6 Subscribed Newsgroups
	"Bookmarks.html";						// 7 Bookmarks
	"";										// 8 MIME types, not used
	"MagicCookie";							// 9 Magic cookie file
	"socks.conf";							// 10 SOCKS file, inside preferences
	"Newsgroups";							// 11 Newsgroup listings
	"NewsFAT";								// 12 Mappings of newsrc files
	"Certificates";							// 13 Certificates
	"Certificate Index";					// 14 Certificate index
	"Mail";									// 15 Mail folder
	"News";									// 16 News folder
	"Security";								// 17 Security folder
	".snm";									// 18 Mail index extension
	"Mail Filters";							// 19 Mail sort file
	"Pop State";							// 20 Mail pop state
	"Proxy Config";							// 21 Proxy configuration
	"Key Database";							// 22 Key db
	"Headers cache";						// 23 Headers xover cache
	"AddressBook.html";						// 24 Address Book
	"Sent Mail";							// 25 Sent mail
	"Posted Articles";						// 26 Posted news articles
}};

resource 'STR#' ( BUTTON_STRINGS_RESID, "Button Names", purgeable ) {{
	"Back";					// 1 Main
	"Forward";				// 2 Main
	"Home";					// 3 Main
	"Reload";				// 4 Main
	"Images";				// 5 Main
	"Open";					// 6 Main
	"Print";				// 7 Main
	"Find";					// 8 Main
	"Stop";					// 9 Main
	"WhatÕs New?";			// 10 Directory button
	"WhatÕs Cool?";			// 11 Directory button
	"Destinations";			// 12 Directory button
	"Net Search";			// 13 Directory button
	"People";		// 14 Directory button
	"Software";				// 15 Directory button
	"Get Mail";				// 16 Mail: Get New Mail
	"To: Mail";				// 17 Mail: Mail New
	"Delete";				// 18 Mail: Delete Message
	"Re: Mail";				// 19 Mail: Reply to Sender
	"Re: All";				// 20 Mail: Reply to All
	"Forward";				// 21 Mail and News: Forward Message
	"Previous";				// 22 Mail and News: Previous Unread Message
	"Next";					// 23 Mail and News: Next Unread Message
	"New";					// 24 (not used)
	"Read";					// 25 (not used)
	"Send Now";				// 26 Compose: Send Now
	"Attach";				// 27 Compose: Attach
	"Address";				// 28 Compose: Bring up Address Book
	"Send Later";			// 29 Compose: Send Later
	"Quote";				// 30 Compose: Quote Original Message
	"To: News";				// 31 News: Post to News
	"To: Mail";				// 32 News: Send Mail
	"Re: Mail";				// 33 News: Reply via Mail
	"Re: News";				// 34 News: Reply via News
	"Re: Both";				// 35 News: Reply via Mail and News
	"Thread";				// 36 News: Mark Thread Read
	"Group";				// 37 News: Mark Group Read
	"Netscape";				// 38 Main: Co-brand button
	"New";					// 39 Editor; File/Edit: New Document
	"Open";					// 40 Editor; File/Edit: Open
	"Save";					// 41 Editor; File/Edit: Save
	"Browse";				// 42 Editor; File/Edit: Browse Document
	"Cut";					// 43 Editor; File/Edit: Cut
	"Copy";					// 44 Editor; File/Edit: Copy
	"Paste";				// 45 Editor; File/Edit: Paste
	"Print";				// 46 Editor; File/Edit: Print
	"Find";					// 47 Editor; File/Edit: Find
	"Edit";					// 48 Edit
	"Publish";				// 49 Editor; File/Edit: Publish
	"";						// 50 Editor; Decrease font size
	"";						// 51 Editor; Increase font size
	"";						// 52 Editor; Bold
	"";						// 53 Editor; Italic
	"";						// 54 Editor; Fixed Width
	"";						// 55 Editor; Font Color
	"";						// 56 Editor; Make Link
	"";						// 57 Editor; Clear All Styles
	"";						// 58 Editor; Insert Target (Named Anchor)
	"";						// 59 Editor; Insert Image
	"";						// 60 Editor; Insert Horiz. Line
	"";						// 61 Editor; Table
	"";						// 62 Editor; Object Properties
	"";						// 63 Editor; Bullet List
	"";						// 64 Editor; Numbered List
	"";						// 65 Editor; Decrease Indent
	"";						// 66 Editor; Increase Indent
	"";						// 67 Editor; Align Left
	"";						// 68 Editor; Center
	"";						// 69 Editor; Align Right
}};

resource 'STR#' ( BUTTON_TOOLTIPS_RESID, "", purgeable ) {{
	"Back";					// 1 Main
	"Forward";				// 2 Main
	"Go To Home Page";		// 3 Main
	"Reload Page";			// 4 Main
	"Load Images";			// 5 Main
	"Open Location";		// 6 Main
	"Print";				// 7 Main
	"Find Text On Page";	// 8 Main
	"Stop Loading";			// 9 Main
	"";						// 10 What's New ?
	"";						// 11 Directory button
	"";						// 12 Handbook
	"";						// 13 Net Search
	"";						// 14 Net Directory
	"";						// 15 Software
	"Get New Mail";			// 16 Mail: Get New Mail
	"Compose New Mail Message";		// 17 Mail: Mail New
	"Delete Selected Messages";		// 18 Mail: Delete Message
	"Reply To Sender";				// 19 Mail: Reply to Sender
	"Reply To All";					// 20 Mail: Reply to All
	"Forward Message";				// 21 Mail and News: Forward Message
	"Previous Unread Message";		// 22 Mail and News: Previous Unread Message
	"Next Unread Message";			// 23 Mail and News: Next Unread Message
	"";								// 24 (not used)
	"";								// 25 (not used)
	"Send Mail Now";			// 26 Compose: Send Now
	"Attach";					// 27 Compose: Attach
	"Open Address Book";		// 28 Compose: Bring up Address Book
	"Place Message In Outbox";	// 29 Compose: Send Later
	"Quote Original Message";	// 30 Compose: Quote Original Message
	"Post News Message";		// 31 News: Post to News
	"Compose New Mail Message";	// 32 News: Send Mail
	"Reply via Mail";			// 33 News: Reply via Mail
	"Reply via News";			// 34 News: Reply via News
	"Reply via Mail and News";	// 35 News: Reply via Mail and News
	"Mark Thread Read";			// 36 News: Mark Thread Read
	"Mark Group Read";			// 37 News: Mark Group Read
	"";							// 38 Main: Co-brand button
	"New Document";				// 39 Editor; File/Edit: New Document
	"Open File To Edit";		// 40 Editor; File/Edit: Open
	"Save";						// 41 Editor; File/Edit: Save
	"View in Browser";			// 42 Editor; File/Edit: Browse Document
	"Cut";						// 43 Editor; File/Edit: Cut
	"Copy";						// 44 Editor; File/Edit: Copy
	"Paste";					// 45 Editor; File/Edit: Paste
	"Print";					// 46 Editor; File/Edit: Print
	"Find";						// 47 Editor; File/Edit: Find
	"Edit";						// 48 Edit
	"Publish";					// 49 Editor; File/Edit: Publish
	"Decrease font size";		// 50 Editor; Decrease font size
	"Increase font size";		// 51 Editor; Increase font size
	"Bold";						// 52 Editor; Bold
	"Italic";					// 53 Editor; Italic
	"Fixed Width";				// 54 Editor; Fixed Width
	"Font Color";				// 55 Editor; Font Color
	"Make Link";				// 56 Editor; Make Link
	"Clear All Styles";			// 57 Editor; Clear All Styles
	"Insert Target (Named Anchor)";		// 58 Editor; Insert Target (Named Anchor)
	"Insert Image";				// 59 Editor; Insert Image
	"Insert Horiz. Line";		// 60 Editor; Insert Horiz. Line
	"Table";					// 61 Editor; Table
	"Object Properties";		// 62 Editor; Object Properties
	"Bullet List";				// 63 Editor; Bullet List
	"Numbered List";			// 64 Editor; Numbered List
	"Decrease Indent";			// 65 Editor; Decrease Indent
	"Increase Indent";			// 66 Editor; Increase Indent
	"Align Left";				// 67 Editor; Align Left
	"Center";					// 68 Editor; Center
	"Align Right";				// 69 Editor; Align Right
}};


/******************************************************************
 * PREFERENCES
 ******************************************************************/
/* Check out uprefd.cp for description of the strings */
resource 'STR#' ( res_Strings1, "Default Strings", purgeable ) {{
/* 1  */	NETSCAPE_LOCATION;
/* 2  */	"news";
/* 3  */	"";
/* 4  */	"";
/* 5  */	"";
/* 6  */	"";
/* 7  */	"";
/* 8  */	"";
/* 9  */	"";
/* 10 */	"";
/* 11 */	"";
/* 12 */	"";
/* 13 */	"";
/* 14 */	"";
/* 15 */	"";
/* 16 */	"mail"
}};

resource 'STR#' ( res_Strings2, "Default Strings", purgeable ) {{
/* 1  */	"";
/* 2  */	"";
/* 3  */	"";
/* 4  */	"";
/* 5  */	"";
/* 6  */	"pop";
/* 7  */	"";			/* AcceptLanguage */
/* 8  */	"";
/* 9  */	"";
/* 10 */	"";
/* 11 */	"";
/* 12 */	"";
/* 13 */	"";			/* Mail password */
/* 14 */	"";			/* Ciphers */
/* 15 */	"";			/* EditorAuthor */
/* 16 */	NEWDOC_TEMPLATE_LOCATION;			/* EditorNewDocTemplateLocation */
/* 17 */	"";			/* EditorBackgroundImage */
/* 18 */	"";			/* PublishLocation */
/* 19 */	"";			/* PublishBrowseLocation */
/* 20 */	"";			/* PublishHistory0 */
/* 21 */	"";			/* PublishHistory1 */
/* 22 */	"";			/* PublishHistory2 */
/* 23 */	"";			/* PublishHistory3 */
/* 24 */	"";			/* PublishHistory4 */
/* 25 */	"";			/* PublishHistory5 */
/* 26 */	"";			/* PublishHistory6 */
/* 27 */	"";			/* PublishHistory7 */
/* 28 */	"";			/* PublishHistory8 */
/* 29 */	"";			/* PublishHistory9 */
}};

resource 'STR#' ( res_StringsBoolean, "Default Booleans", purgeable ) {{
/* 1  */	"FALSE";
/* 2  */	"TRUE";
/* 3  */	"FALSE";
/* 4  */	"TRUE";
/* 5  */	"FALSE";
/* 6  */	"TRUE";
/* 7  */	"TRUE";
/* 8  */	"TRUE";
/* 9  */	"TRUE";
/* 10 */	"FALSE";
/* 11 */	"TRUE";
/* 12 */	"FALSE";
/* 13 */	"TRUE";
/* 14 */	"FALSE";
/* 15 */	"TRUE";
/* 16 */	"TRUE";
/* 17 */	"TRUE";
/* 18 */	"TRUE";
/* 19 */	"TRUE";
/* 20 */	"FALSE";
/* 21 */	"FALSE";
/* 22 */	"TRUE";
/* 23 */	"TRUE";
/* 24 */	"FALSE";
/* 25 */	"FALSE";
/* 26 */	"FALSE";
/* 27 */	"TRUE";
/* 28 */	"FALSE";
/* 29 */	"FALSE";
/* 30 */	"FALSE";
/* 31 */	"FALSE";
/* 32 */	"FALSE";
/* 33 */	"FALSE";
/* 34 */	"FALSE";
/* 35 */ 	"FALSE";
/* 36 */ 	"FALSE";	/* Thread mail */
/* 37 */ 	"TRUE";		/* Thread news */
/* 38 */	"FALSE";	/* Use inline view source */
/* 39 */	"FALSE";	/* Auto-quote on reply */
/* 40 */	"FALSE";	/* Remember mail password */
/* 41 */	"TRUE";		/* Immediate mail delivery */
/* 42 */	"FALSE";		/* Disable Javascript */
/* 43 */	"TRUE";		/* Show ToolTips */
/* 44 */ 	"FALSE";		/* Use internet config */
/* 45 */	"FALSE";		/* Disable Java */
/* 46 */	"FALSE";		/* warn before accepting Cookies */
/* 47 */	"FALSE";		/* warn before using email address as anonymous FTP pw */
/* 48 */	"TRUE";			/* warn before submitting a form by email */
/* 49 */	"FALSE";		/* allow SSL disk caching */
/* 50 */	"TRUE";			/* enable SSLv2 */
/* 51 */	"TRUE";			/* enable SSLv3 */
/* 52 */	"TRUE";			/* enable active scrolling */
/* 53 */	"FALSE";		/* Should new editor documents be set up to use custom colors */
/* 54 */	"TRUE";			/* Should new editor documents be set up to use a solid background color */
/* 55 */	"TRUE";			/* Maintain links when publishing a document */
/* 56 */	"TRUE";			/* Keep images with document when publishing it */
/* 57 */	"TRUE";			/* Should we should the Copyright notice next time we "steal" something off the web */
}};

resource 'STR#' ( res_StringsLong, "Default longs", purgeable ) {{
/* 1  */	"5242880";
/* 2  */	"0";	/* NAME 0, TYPE 1, SIZE 2, DATE 3 */
/* 3  */	$$Format("%d", TOOLBAR_TEXT_AND_ICONS);
/* 4  */	"9";
/* 5  */	"4";
/* 6  */	"8";
/* 7  */	$$Format("%d", PRINT_CROPPED);
/* 8  */	"500";
/* 9  */	$$Format("%d", 0);	/*CU_CHECK_PER_SESSION */
/* 10 */	$$Format("%d", CS_LATIN1);
/* 11 */	$$Format("%d",CS_MAC_ROMAN);
/* 1uprefd2 */	$$Format("%d",DEFAULT_BACKGROUND);
/* 13 */	$$Format("%d",0);
/* 14 */	"0";	/* MSG_PlainFont */
/* 15 */	"0";	/* MSG_NormalSize */
/* 16 */	"40";	/* Maximum message size in K */
/* 17 */	"0";	/* Top left print header */
/* 18 */	"0";	/* Top right print header */
/* 19 */	"0";	/* Bottom left print footer */
/* 20 */	"0";	/* Bottom mid print footer */
/* 21 */	"0";	/* Bottom right print footer */
/* 22 */	"0";	/* Print background */
/* 23 */	"1";	/* Proxy config PROXY_STYLE_MANUAL*/
/* 24 */	$$Format("%d",STARTUP_BROWSER);
/* 25 */	$$Format("%d",BROWSER_STARTUP_ID);
/* 26 */	"10";	/* Check for mail how often? */
/* 27 */	"0"; 	/* Should we ask for Mozilla password. default is once per session */
/* 28 */	"10";	/* How often should we ask for the password */
/* 29 */	$$Format("%d",MAIL_SORT_BY_DATE);	/* Sort mail */
/* 30 */	$$Format("%d",MAIL_SORT_BY_DATE);	/* Sort news */
/* 31 */	"0";	/* Top middle print header */
/* 32 */    "0";
/* 33 */	"0";
/* 34 */	"2";	/* show SOME mail headers */
/* 35 */	"2";	/* show SOME news headers */
}};

resource 'STR#' ( res_StringsColor, "Default Colors", purgeable ) {{
/* $$Format("%d %d %d", red, green, blue); */
/* 1  */	$$Format("%d %d %d", 0xC0C0, 0xC0C0, 0xC0C0); 		/* WindowBkgnd */
/* 2  */	$$Format("%d %d %d", 0x0000, 0x0000, 0xFFFF); 		/* Anchor */
/* 3  */	$$Format("%d %d %d", 0xFFFF, 0x0000, 0x0000); 		/* Visited */
/* 4  */	$$Format("%d %d %d", 0x0000, 0x0000, 0x0000); 		/* TextFore */
/* 5  */	$$Format("%d %d %d", 0xC0C0, 0xC0C0, 0xC0C0); 		/* TextBkgnd */
/* 6  */	$$Format("%d %d %d", 0x0000, 0x0000, 0x0000);	/* EditorText */
/* 7  */	$$Format("%d %d %d", 0x0000, 0x0000, 0xFFFF);	/* EditorLink */
/* 8  */	$$Format("%d %d %d", 0x0000, 0x0000, 0x8888);	/* EditorActiveLink */
/* 9  */	$$Format("%d %d %d", 0xFFFF, 0x0000, 0x0000);	/* EditorFollowedLink */
/* 10  */	$$Format("%d %d %d", 0xC0C0, 0xC0C0, 0xC0C0);	/* EditorBackground */
/* 11  */	$$Format("%d %d %d", 0x0000, 0x0000, 0x0000); 		/* Caption text color */
}};


/*
 * Enterprise Kit:
 * The Lock resource indicates the application is customizable,
 * whether to show the co-brand button and what preferences to lock.
 * The version number needs to be incremented any time a
 * customizable resource ID is changed (menus, buttons, preferences).
 */

type 'Lock' {
	byte;			/* version of E-Kit data */
	byte;			/* bool: app has been modified by E-Kit */
	byte;			/* bool: show the co-brand button */
	array [14] {
		byte;		/* bool: lock the specified preference */
	};
};

resource 'Lock' (256, "E-Kit Data") {
	3,
	0,
	0,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
};


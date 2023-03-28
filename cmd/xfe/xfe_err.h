#ifndef __XFE_XFE_ERR_H_
#define __XFE_XFE_ERR_H_

#include <resdef.h>

#define XFE_ERR_OFFSET 1000

RES_START
BEGIN_STR(mcom_cmd_xfe_xfe_err_h_strings)
ResDef(DEFAULT_ADD_BOOKMARK_ROOT, XFE_ERR_OFFSET + 0, "End of List")
ResDef(DEFAULT_MENU_BOOKMARK_ROOT, XFE_ERR_OFFSET + 1, "Entire List")
ResDef(XFE_SAVE_AS_TYPE_ENCODING, XFE_ERR_OFFSET + 3, "Save As... (type %.90s, encoding %.90s)")
ResDef(XFE_SAVE_AS_TYPE, XFE_ERR_OFFSET + 4, "Save As... (type %.90s)")
ResDef(XFE_SAVE_AS_ENCODING, XFE_ERR_OFFSET + 5, "Save As... (encoding %.90s)")
ResDef(XFE_SAVE_AS, XFE_ERR_OFFSET + 6, "Save As...")
ResDef(XFE_ERROR_OPENING_FILE, XFE_ERR_OFFSET + 7, "Error opening %.900s:")
ResDef(XFE_ERROR_DELETING_FILE, XFE_ERR_OFFSET + 8, "Error deleting %.900s:")
ResDef(XFE_LOG_IN_AS, XFE_ERR_OFFSET + 9, "When connected, log in as user `%.900s'")
ResDef(XFE_OUT_OF_MEMORY_URL, XFE_ERR_OFFSET + 10, "Out Of Memory -- Can't Open URL")
ResDef(XFE_COULD_NOT_LOAD_FONT, XFE_ERR_OFFSET + 11, "couldn't load:\n%s")
ResDef(XFE_USING_FALLBACK_FONT, XFE_ERR_OFFSET + 12, "%s\nNo other resources were reasonable!\nUsing fallback font \"%s\" instead.")
ResDef(XFE_NO_FALLBACK_FONT, XFE_ERR_OFFSET + 13, "%s\nNo other resources were reasonable!\nThe fallback font \"%s\" could not be loaded!\nGiving up.")
ResDef(XFE_DISCARD_BOOKMARKS, XFE_ERR_OFFSET + 14, "Bookmarks file has changed on disk: discard your changes?")
ResDef(XFE_RELOAD_BOOKMARKS, XFE_ERR_OFFSET + 15, "Bookmarks file has changed on disk: reload it?")
ResDef(XFE_NEW_ITEM, XFE_ERR_OFFSET + 16, "New Item")
ResDef(XFE_NEW_HEADER, XFE_ERR_OFFSET + 17, "New Header")
ResDef(XFE_REMOVE_SINGLE_ENTRY, XFE_ERR_OFFSET + 18, "Remove category \"%.900s\" and its %d entry?")
ResDef(XFE_REMOVE_MULTIPLE_ENTRIES, XFE_ERR_OFFSET + 19, "Remove category \"%.900s\" and its %d entries?")
ResDef(XFE_EXPORT_BOOKMARK, XFE_ERR_OFFSET + 20, "Export Bookmark")
ResDef(XFE_IMPORT_BOOKMARK, XFE_ERR_OFFSET + 21, "Import Bookmark")
ResDef(XFE_SECURITY_WITH, XFE_ERR_OFFSET + 22, "This version supports %s security with %s.")
ResDef(XFE_SECURITY_DISABLED, XFE_ERR_OFFSET + 23, "Security disabled")
ResDef(XFE_WELCOME_HTML, XFE_ERR_OFFSET + 24, "file:/usr/local/lib/netscape/docs/Welcome.html")
ResDef(XFE_DOCUMENT_DONE, XFE_ERR_OFFSET + 25, "Document: Done.")
ResDef(XFE_OPEN_FILE, XFE_ERR_OFFSET + 26, "Open File")
ResDef(XFE_ERROR_OPENING_PIPE, XFE_ERR_OFFSET + 27, "Error opening pipe to %.900s")
ResDef(XFE_WARNING, XFE_ERR_OFFSET + 28, "Warning:\n\n")
ResDef(XFE_FILE_DOES_NOT_EXIST, XFE_ERR_OFFSET + 29, "%s \"%.255s\" does not exist.\n")
ResDef(XFE_HOST_IS_UNKNOWN, XFE_ERR_OFFSET + 30, "%s \"%.255s\" is unknown.\n")
ResDef(XFE_NO_PORT_NUMBER, XFE_ERR_OFFSET + 31, "No port number specified for %s.\n")
ResDef(XFE_MAIL_HOST, XFE_ERR_OFFSET + 32, "Mail host")
ResDef(XFE_NEWS_HOST, XFE_ERR_OFFSET + 33, "News host")
ResDef(XFE_NEWS_RC_DIRECTORY, XFE_ERR_OFFSET + 34, "News RC directory")
ResDef(XFE_TEMP_DIRECTORY, XFE_ERR_OFFSET + 35, "Temporary directory")
ResDef(XFE_FTP_PROXY_HOST, XFE_ERR_OFFSET + 36, "FTP proxy host")
ResDef(XFE_GOPHER_PROXY_HOST, XFE_ERR_OFFSET + 37, "Gopher proxy host")
ResDef(XFE_HTTP_PROXY_HOST, XFE_ERR_OFFSET + 38, "HTTP proxy host")
ResDef(XFE_HTTPS_PROXY_HOST, XFE_ERR_OFFSET + 39, "HTTPS proxy host")
ResDef(XFE_WAIS_PROXY_HOST, XFE_ERR_OFFSET + 40, "WAIS proxy host")
ResDef(XFE_SOCKS_HOST, XFE_ERR_OFFSET + 41, "SOCKS host")
ResDef(XFE_GLOBAL_MIME_FILE, XFE_ERR_OFFSET + 42, "Global MIME types file")
ResDef(XFE_PRIVATE_MIME_FILE, XFE_ERR_OFFSET + 43, "Private MIME types file")
ResDef(XFE_GLOBAL_MAILCAP_FILE, XFE_ERR_OFFSET + 44, "Global mailcap file")
ResDef(XFE_PRIVATE_MAILCAP_FILE, XFE_ERR_OFFSET + 45, "Private mailcap file")
ResDef(XFE_BM_CANT_DEL_ROOT, XFE_ERR_OFFSET + 46, "Cannot delete toplevel bookmark")
ResDef(XFE_BM_CANT_CUT_ROOT, XFE_ERR_OFFSET + 47, "Cannot cut toplevel bookmark")
ResDef(XFE_BM_ALIAS, XFE_ERR_OFFSET + 48, "This is an alias to the following Bookmark:")
/* NPS i18n stuff */
ResDef(XFE_FILE_OPEN, XFE_ERR_OFFSET + 49, "File Open...")
ResDef(XFE_PRINTING_OF_FRAMES_IS_CURRENTLY_NOT_SUPPORTED, XFE_ERR_OFFSET + 50, "Printing of frames is currently not supported.")
ResDef(XFE_ERROR_SAVING_OPTIONS, XFE_ERR_OFFSET + 51, "error saving options")
ResDef(XFE_UNKNOWN_ESCAPE_CODE, XFE_ERR_OFFSET + 52,
"unknown %s escape code %%%c:\n%%h = host, %%p = port, %%u = user")
ResDef(XFE_COULDNT_FORK, XFE_ERR_OFFSET + 53, "couldn't fork():")
ResDef(XFE_EXECVP_FAILED, XFE_ERR_OFFSET + 54, "%s: execvp(%s) failed")
ResDef(XFE_SAVE_FRAME_AS, XFE_ERR_OFFSET + 55, "Save Frame as...")
ResDef(XFE_PRINT_FRAME, XFE_ERR_OFFSET + 57, "Print Frame...")
ResDef(XFE_PRINT, XFE_ERR_OFFSET + 58 , "Print...")
ResDef(XFE_DOWNLOAD_FILE, XFE_ERR_OFFSET + 59, "Download File: %s")
ResDef(XFE_COMPOSE_NO_SUBJECT, XFE_ERR_OFFSET + 60, "Compose: (No Subject)")
ResDef(XFE_COMPOSE, XFE_ERR_OFFSET + 61, "Compose: %s")
ResDef(XFE_NETSCAPE_UNTITLED, XFE_ERR_OFFSET + 62, "Netscape: <untitled>")
ResDef(XFE_NETSCAPE, XFE_ERR_OFFSET + 63, "Netscape: %s")
ResDef(XFE_NO_SUBJECT, XFE_ERR_OFFSET + 64, "(no subject)")
ResDef(XFE_UNKNOWN_ERROR_CODE, XFE_ERR_OFFSET + 65, "unknown error code %d")
ResDef(XFE_INVALID_FILE_ATTACHMENT_DOESNT_EXIST, XFE_ERR_OFFSET + 66,
"Invalid File attachment.\n%s: doesn't exist.\n")
ResDef(XFE_INVALID_FILE_ATTACHMENT_NOT_READABLE, XFE_ERR_OFFSET + 67,
"Invalid File attachment.\n%s: not readable.\n")
ResDef(XFE_INVALID_FILE_ATTACHMENT_IS_A_DIRECTORY, XFE_ERR_OFFSET + 68,
"Invalid File attachment.\n%s: is a directory.\n")
ResDef(XFE_COULDNT_FORK_FOR_MOVEMAIL, XFE_ERR_OFFSET + 69,
"couldn't fork() for movemail")
ResDef(XFE_PROBLEMS_EXECUTING, XFE_ERR_OFFSET + 70, "problems executing %s:")
ResDef(XFE_TERMINATED_ABNORMALLY, XFE_ERR_OFFSET + 71, "%s terminated abnormally:")
ResDef(XFE_UNABLE_TO_OPEN, XFE_ERR_OFFSET + 72, "Unable to open %.900s")
ResDef(XFE_PLEASE_ENTER_NEWS_HOST, XFE_ERR_OFFSET + 73,
"Please enter your news host in one\n\
of the following formats:\n\n\
    news://HOST,\n\
    news://HOST:PORT,\n\
    snews://HOST, or\n\
    snews://HOST:PORT\n\n" )

ResDef(XFE_MOVEMAIL_FAILURE_SUFFIX, XFE_ERR_OFFSET + 74,
       "For the internal movemail method to work, we must be able to create\n\
lock files in the mail spool directory.  On many systems, this is best\n\
accomplished by making that directory be mode 01777.  If that is not\n\
possible, then an external setgid/setuid movemail program must be used.\n\
Please see the Release Notes for more information.")
ResDef(XFE_CANT_MOVE_MAIL, XFE_ERR_OFFSET + 75, "Can't move mail from %.200s")
ResDef(XFE_CANT_GET_NEW_MAIL_LOCK_FILE_EXISTS, XFE_ERR_OFFSET + 76, "Can't get new mail; a lock file %.200s exists.")
ResDef(XFE_CANT_GET_NEW_MAIL_UNABLE_TO_CREATE_LOCK_FILE, XFE_ERR_OFFSET + 77, "Can't get new mail; unable to create lock file %.200s")
ResDef(XFE_CANT_GET_NEW_MAIL_SYSTEM_ERROR, XFE_ERR_OFFSET + 78, "Can't get new mail; a system error occurred.")
ResDef(XFE_CANT_MOVE_MAIL_UNABLE_TO_OPEN, XFE_ERR_OFFSET + 79, "Can't move mail; unable to open %.200s")
ResDef(XFE_CANT_MOVE_MAIL_UNABLE_TO_READ, XFE_ERR_OFFSET + 80, "Can't move mail; unable to read %.200s")
ResDef(XFE_CANT_MOVE_MAIL_UNABLE_TO_WRITE, XFE_ERR_OFFSET + 81, "Can't move mail; unable to write to %.200s")
ResDef(XFE_THERE_WERE_PROBLEMS_MOVING_MAIL, XFE_ERR_OFFSET + 82, "There were problems moving mail")
ResDef(XFE_THERE_WERE_PROBLEMS_MOVING_MAIL_EXIT_STATUS, XFE_ERR_OFFSET + 83, "There were problems moving mail: exit status %d" )
ResDef(XFE_THERE_WERE_PROBLEMS_CLEANING_UP, XFE_ERR_OFFSET + 134, "There were problems cleaning up %s")
ResDef(XFE_USAGE_MSG1, XFE_ERR_OFFSET + 85, "%s\n\
usage: %s [ options ... ]\n\
       where options include:\n\
\n\
       -help                     to show this message.\n\
       -version                  to show the version number and build date.\n\
       -display <dpy>            to specify the X server to use.\n\
       -geometry =WxH+X+Y        to position and size the window.\n\
       -visual <id-or-number>    to use a specific server visual.\n\
       -install                  to install a private colormap.\n\
       -no-install               to use the default colormap.\n" )
ResDef(XFE_USAGE_MSG2, XFE_ERR_OFFSET + 154, "\
       -share                    when -install, cause each window to use the\n\
                                 same colormap instead of each window using\n\
                                 a new one.\n\
       -no-share                 cause each window to use the same colormap.\n")
ResDef(XFE_USAGE_MSG3, XFE_ERR_OFFSET + 86, "\
       -ncols <N>                when not using -install, set the maximum\n\
                                 number of colors to allocate for images.\n\
       -mono                     to force 1-bit-deep image display.\n\
       -iconic                   to start up iconified.\n\
       -xrm <resource-spec>      to set a specific X resource.\n\
\n\
       -remote <remote-command>  to execute a command in an already-running\n\
                                 Netscape process.  For more info, see\n\
			  http://home.netscape.com/newsref/std/x-remote.html\n\
       -id <window-id>           the id of an X window to which the -remote\n\
                                 commands should be sent; if unspecified,\n\
                                 the first window found will be used.\n\
       -raise                    whether following -remote commands should\n\
                                 cause the window to raise itself to the top\n\
                                 (this is the default.)\n\
       -noraise                  the opposite of -raise: following -remote\n\
                                 commands will not auto-raise the window.\n\
\n\
       Arguments which are not switches are interpreted as either files or\n\
       URLs to be loaded.\n\
\n\
       Most customizations can be performed through the Options menu.\n\n" )
ResDef(XFE_VERSION_COMPLAINT_FORMAT, XFE_ERR_OFFSET + 87, "%s: program is version %s, but resources are version %s.\n\n\
\tThis means that there is an inappropriate `%s' file in\n\
\the system-wide app-defaults directory, or possibly in your\n\
\thome directory.  Check these environment variables and the\n\
\tdirectories to which they point:\n\n\
  $XAPPLRESDIR\n\
  $XFILESEARCHPATH\n\
  $XUSERFILESEARCHPATH\n\n\
\tAlso check for this file in your home directory, or in the\n\
\tdirectory called `app-defaults' somewhere under /usr/lib/." )
ResDef(XFE_INAPPROPRIATE_APPDEFAULT_FILE, XFE_ERR_OFFSET + 88, "%s: couldn't find our resources?\n\n\
\tThis could mean that there is an inappropriate `%s' file\n\
\tinstalled in the app-defaults directory.  Check these environment\n\
\tvariables and the directories to which they point:\n\n\
  $XAPPLRESDIR\n\
  $XFILESEARCHPATH\n\
  $XUSERFILESEARCHPATH\n\n\
\tAlso check for this file in your home directory, or in the\n\
\tdirectory called `app-defaults' somewhere under /usr/lib/." )
ResDef(XFE_INVALID_GEOMETRY_SPEC, XFE_ERR_OFFSET + 89, "%s: invalid geometry specification.\n\n\
 Apparently \"%s*geometry: %s\" or \"%s*geometry: %s\"\n\
 was specified in the resource database.  Specifying \"*geometry\"\n\
 will make %s (and most other X programs) malfunction in obscure\n\
 ways.  You should always use \".geometry\" instead.\n" )
ResDef(XFE_UNRECOGNISED_OPTION, XFE_ERR_OFFSET + 90, "%s: unrecognized option \"%s\"\n")
ResDef(XFE_APP_HAS_DETECTED_LOCK, XFE_ERR_OFFSET + 91, "%s has detected a %s\nfile.\n")
ResDef(XFE_ANOTHER_USER_IS_RUNNING_APP, XFE_ERR_OFFSET + 92, "\nThis may indicate that another user is running\n%s using your %s files.\n" )
ResDef(XFE_APPEARS_TO_BE_RUNNING_ON_HOST_UNDER_PID, XFE_ERR_OFFSET + 93,"It appears to be running on host %s\nunder process-ID %u.\n" )
ResDef(XFE_YOU_MAY_CONTINUE_TO_USE, XFE_ERR_OFFSET + 94, "\nYou may continue to use %s, but you will\n\
be unable to use the disk cache, global history,\n\
or your personal certificates.\n" )
ResDef(XFE_OTHERWISE_CHOOSE_CANCEL, XFE_ERR_OFFSET + 95, "\nOtherwise, you may choose Cancel, make sure that\n\
you are not running another %s Navigator,\n\
delete the %s file, and\n\
restart %s." )
ResDef(XFE_EXISTED_BUT_WAS_NOT_A_DIRECTORY, XFE_ERR_OFFSET + 96, "%s: %s existed, but was not a directory.\n\
The old file has been renamed to %s\n\
and a directory has been created in its place.\n\n" )
ResDef(XFE_EXISTS_BUT_UNABLE_TO_RENAME, XFE_ERR_OFFSET + 97, "%s: %s exists but is not a directory,\n\
and we were unable to rename it!\n\
Please remove this file: it is in the way.\n\n" )
ResDef(XFE_UNABLE_TO_CREATE_DIRECTORY, XFE_ERR_OFFSET + 98,
"%s: unable to create the directory `%s'.\n%s\n\
Please create this directory.\n\n" )
ResDef(XFE_UNKNOWN_ERROR, XFE_ERR_OFFSET + 99, "unknown error")
ResDef(XFE_ERROR_CREATING, XFE_ERR_OFFSET + 100, "error creating %s")
ResDef(XFE_ERROR_WRITING, XFE_ERR_OFFSET + 101, "error writing %s")
ResDef(XFE_CREATE_CONFIG_FILES, XFE_ERR_OFFSET + 102,
"This version of %s uses different names for its config files\n\
than previous versions did.  Those config files which still use\n\
the same file formats have been copied to their new names, and\n\
those which don't will be recreated as necessary.\n%s\n\n\
Would you like us to delete the old files now?" )
ResDef(XFE_OLD_FILES_AND_CACHE, XFE_ERR_OFFSET + 103,
"\nThe old files still exist, including a disk cache directory\n\
(which might be large.)" )
ResDef(XFE_OLD_FILES, XFE_ERR_OFFSET + 104, "The old files still exist.")
ResDef(XFE_GENERAL, XFE_ERR_OFFSET + 105, "General")
ResDef(XFE_PASSWORDS, XFE_ERR_OFFSET + 106, "Passwords")
ResDef(XFE_PERSONAL_CERTIFICATES, XFE_ERR_OFFSET + 107, "Personal Certificates")
ResDef(XFE_SITE_CERTIFICATES, XFE_ERR_OFFSET + 108, "Site Certificates")
ResDef(XFE_ESTIMATED_TIME_REMAINING_CHECKED, XFE_ERR_OFFSET + 109,
"Checked %s (%d left)\n%d%% completed)\n\n\
Estimated time remaining: %s\n\
(Remaining time depends on the sites selected and\n\
the network traffic.)" )
ResDef(XFE_ESTIMATED_TIME_REMAINING_CHECKING, XFE_ERR_OFFSET + 110,
"Checking ... (%d left)\n%d%% completed)\n\n\
Estimated time remaining: %s\n\
(Remaining time depends on the sites selected and\n\
the network traffic.)" )
ResDef(XFE_RE, XFE_ERR_OFFSET + 111, "Re: ")
ResDef(XFE_DONE_CHECKING_ETC, XFE_ERR_OFFSET + 112,
"Done checking %d bookmarks.\n\
%d documents were reached.\n\
%d documents have changed and are marked as new." )
ResDef(XFE_APP_EXITED_WITH_STATUS, XFE_ERR_OFFSET + 115,"\"%s\" exited with status %d" )
ResDef(XFE_THE_MOTIF_KEYSYMS_NOT_DEFINED, XFE_ERR_OFFSET + 116,
"%s: The Motif keysyms seem not to be defined.\n\n\
 This is usually because the proper XKeysymDB file was not found.\n\
 You can set the $XKEYSYMDB environment variable to the location\n\
 of a file which contains the right keysyms.\n\n\
 Without the right XKeysymDB, many warnings will be generated,\n\
 and most keyboard accelerators will not work.\n\n\
 (An appropriate XKeysymDB file was included with the %s\n\
 distribution.)\n\n" )
ResDef(XFE_SOME_MOTIF_KEYSYMS_NOT_DEFINED, XFE_ERR_OFFSET + 117,
 "%s: Some of the Motif keysyms seem not to be defined.\n\n\
 This is usually because the proper XKeysymDB file was not found.\n\
 You can set the $XKEYSYMDB environment variable to the location\n\
 of a file which contains the right keysyms.\n\n\
 Without the right XKeysymDB, many warnings will be generated,\n\
 and some keyboard accelerators will not work.\n\n\
 (An appropriate XKeysymDB file was included with the %s\n\
 distribution.)\n\n" )
ResDef(XFE_VISUAL_GRAY_DIRECTCOLOR, XFE_ERR_OFFSET + 118,
"Visual 0x%02x is a%s %d bit %s visual.\n\
This is not a supported visual; images %s.\n\n\
Currently supported visuals are:\n\n\
       StaticGray, all depths\n\
        GrayScale, all depths\n\
        TrueColor, depth 8 or greater\n\
        DirectColor, depth 8 or greater\n\
        StaticColor, depth 8 or greater\n\
        PseudoColor, depth 8 only\n\n\
If you have any of the above visuals (see `xdpyinfo'),\n\
it is recommended that you start %s with\n\
the `-visual' command-line option to specify one.\n\n\
More visuals may be directly supported in the future;\n\
your feedback is welcome." )
ResDef(XFE_VISUAL_GRAY, XFE_ERR_OFFSET + 119,
"Visual 0x%02x is a%s %d bit %s visual.\n\
This is not a supported visual; images %s.\n\n\
Currently supported visuals are:\n\n\
       StaticGray, all depths\n\
       GrayScale, all depths\n\
       TrueColor, depth 8 or greater\n\
        StaticColor, depth 8 or greater\n\
        PseudoColor, depth 8 only\n\n\
If you have any of the above visuals (see `xdpyinfo'),\n\
it is recommended that you start %s with\n\
the `-visual' command-line option to specify one.\n\n\
More visuals may be directly supported in the future;\n\
your feedback is welcome." )
ResDef(XFE_VISUAL_DIRECTCOLOR, XFE_ERR_OFFSET + 120,
"Visual 0x%02x is a%s %d bit %s visual.\n\
This is not a supported visual; images %s.\n\n\
Currently supported visuals are:\n\n\
       StaticGray, all depths\n\
        TrueColor, depth 8 or greater\n\
        DirectColor, depth 8 or greater\n\
        StaticColor, depth 8 or greater\n\
        PseudoColor, depth 8 only\n\n\
If you have any of the above visuals (see `xdpyinfo'),\n\
it is recommended that you start %s with\n\
the `-visual' command-line option to specify one.\n\n\
More visuals may be directly supported in the future;\n\
your feedback is welcome." )
ResDef(XFE_VISUAL_NORMAL, XFE_ERR_OFFSET + 121,
"Visual 0x%02x is a%s %d bit %s visual.\n\
This is not a supported visual; images %s.\n\n\
Currently supported visuals are:\n\n\
       StaticGray, all depths\n\
        TrueColor, depth 8 or greater\n\
        StaticColor, depth 8 or greater\n\
        PseudoColor, depth 8 only\n\n\
If you have any of the above visuals (see `xdpyinfo'),\n\
it is recommended that you start %s with\n\
the `-visual' command-line option to specify one.\n\n\
More visuals may be directly supported in the future;\n\
your feedback is welcome." )
ResDef(XFE_WILL_BE_DISPLAYED_IN_MONOCHROME, XFE_ERR_OFFSET + 122,
"will be\ndisplayed in monochrome" )
ResDef(XFE_WILL_LOOK_BAD, XFE_ERR_OFFSET + 123, "will look bad")
ResDef(XFE_APPEARANCE, XFE_ERR_OFFSET + 124, "Appearance")
ResDef(XFE_BOOKMARKS, XFE_ERR_OFFSET + 125, "Bookmarks")
ResDef(XFE_COLORS, XFE_ERR_OFFSET + 126, "Colors")
ResDef(XFE_FONTS, XFE_ERR_OFFSET + 127, "Fonts" )
ResDef(XFE_APPLICATIONS, XFE_ERR_OFFSET + 128, "Applications")
ResDef(XFE_HELPERS, XFE_ERR_OFFSET + 155, "Helpers")
ResDef(XFE_IMAGES, XFE_ERR_OFFSET + 129, "Images")
ResDef(XFE_LANGUAGES, XFE_ERR_OFFSET + 130, "Languages")
ResDef(XFE_CACHE, XFE_ERR_OFFSET + 131, "Cache")
ResDef(XFE_CONNECTIONS, XFE_ERR_OFFSET + 132, "Connections")
ResDef(XFE_PROXIES, XFE_ERR_OFFSET + 133, "Proxies")
ResDef(XFE_COMPOSE_DLG, XFE_ERR_OFFSET + 135, "Compose")
ResDef(XFE_SERVERS, XFE_ERR_OFFSET + 136, "Servers")
ResDef(XFE_IDENTITY, XFE_ERR_OFFSET + 137, "Identity")
ResDef(XFE_ORGANIZATION, XFE_ERR_OFFSET + 138, "Organization")
ResDef(XFE_MAIL_FRAME, XFE_ERR_OFFSET + 139, "Mail Frame" )
ResDef(XFE_MAIL_DOCUMENT, XFE_ERR_OFFSET + 140, "Mail Document" )
ResDef(XFE_NETSCAPE_MAIL, XFE_ERR_OFFSET + 141, "Netscape Mail" )
ResDef(XFE_NETSCAPE_NEWS, XFE_ERR_OFFSET + 142, "Netscape News" )
ResDef(XFE_ADDRESS_BOOK, XFE_ERR_OFFSET + 143, "Address Book" )
ResDef(XFE_X_RESOURCES_NOT_INSTALLED_CORRECTLY, XFE_ERR_OFFSET + 144, 
"X resources not installed correctly!" )
ResDef(XFE_GG_EMPTY_LL, XFE_ERR_OFFSET + 145, "<< Empty >>")
ResDef(XFE_ERROR_SAVING_PASSWORD, XFE_ERR_OFFSET + 146, "error saving password")
ResDef(XFE_UNIMPLEMENTED, XFE_ERR_OFFSET + 147, "Unimplemented.")
ResDef(XFE_TILDE_USER_SYNTAX_DISALLOWED, XFE_ERR_OFFSET + 148, 
"%s: ~user/ syntax is not allowed in preferences file, only ~/\n" )
ResDef(XFE_UNRECOGNISED_VISUAL, XFE_ERR_OFFSET + 149, 
"%s: unrecognized visual \"%s\".\n" )
ResDef(XFE_NO_VISUAL_WITH_ID, XFE_ERR_OFFSET + 150,
"%s: no visual with id 0x%x.\n" )
ResDef(XFE_NO_VISUAL_OF_CLASS, XFE_ERR_OFFSET + 151,
"%s: no visual of class %s.\n" )
ResDef(XFE_STDERR_DIAGNOSTICS_HAVE_BEEN_TRUNCATED, XFE_ERR_OFFSET + 152, 
"\n\n<< stderr diagnostics have been truncated >>" )
ResDef(XFE_ERROR_CREATING_PIPE, XFE_ERR_OFFSET + 153, "error creating pipe:")
ResDef(XFE_OUTBOX_CONTAINS_MSG, XFE_ERR_OFFSET + 156,
"Outbox folder contains an unsent\nmessage. Send it now?\n")
ResDef(XFE_OUTBOX_CONTAINS_MSGS, XFE_ERR_OFFSET + 157,
"Outbox folder contains %d unsent\nmessages. Send them now?\n")
ResDef(XFE_NO_KEEP_ON_SERVER_WITH_MOVEMAIL, XFE_ERR_OFFSET + 158,
"The ``Leave on Server'' option only works when\n\
using a POP3 server, not when using a local\n\
mail spool directory.  To retrieve your mail,\n\
first turn off that option in the Servers pane\n\
of the Mail and News Preferences.")
ResDef(XFE_BACK, XFE_ERR_OFFSET + 159, "Back")
ResDef(XFE_BACK_IN_FRAME, XFE_ERR_OFFSET + 160, "Back in Frame")
ResDef(XFE_FORWARD, XFE_ERR_OFFSET + 161, "Forward")
ResDef(XFE_FORWARD_IN_FRAME, XFE_ERR_OFFSET + 162, "Forward in Frame")
ResDef(XFE_MAIL_SPOOL_UNKNOWN, XFE_ERR_OFFSET + 163,
"Please set the $MAIL environment variable to the\n\
location of your mail-spool file.")
ResDef(XFE_MOVEMAIL_NO_MESSAGES, XFE_ERR_OFFSET + 164,
"No new messages.")
ResDef(XFE_USER_DEFINED, XFE_ERR_OFFSET + 165,
"User-Defined")
ResDef(XFE_OTHER_LANGUAGE, XFE_ERR_OFFSET + 166,
"Other")
ResDef(XFE_COULDNT_FORK_FOR_DELIVERY, XFE_ERR_OFFSET + 167,
"couldn't fork() for external message delivery")
ResDef(XFE_CANNOT_READ_FILE, XFE_ERR_OFFSET + 168,
"Cannot read file %s.")
ResDef(XFE_CANNOT_SEE_FILE, XFE_ERR_OFFSET + 169,
"%s does not exist.")
ResDef(XFE_IS_A_DIRECTORY, XFE_ERR_OFFSET + 170,
"%s is a directory.")
ResDef(XFE_EKIT_LOCK_FILE_NOT_FOUND, XFE_ERR_OFFSET + 171,
"Lock file not found.")
ResDef(XFE_EKIT_CANT_OPEN, XFE_ERR_OFFSET + 172,
"Can't open Netscape.lock file.")
ResDef(XFE_EKIT_MODIFIED, XFE_ERR_OFFSET + 173,
"Netscape.lock file has been modified.")
ResDef(XFE_EKIT_FILESIZE, XFE_ERR_OFFSET + 174,
"Could not determine lock file size.")
ResDef(XFE_EKIT_CANT_READ, XFE_ERR_OFFSET + 175,
"Could not read Netscape.lock data.")
ResDef(XFE_ANIM_CANT_OPEN, XFE_ERR_OFFSET + 176,
"Couldn't open animation file.")
ResDef(XFE_ANIM_MODIFIED, XFE_ERR_OFFSET + 177,
"Animation file modified.\nUsing default animation.")
ResDef(XFE_ANIM_READING_SIZES, XFE_ERR_OFFSET + 178,
"Couldn't read animation file size.\nUsing default animation.")
ResDef(XFE_ANIM_READING_NUM_COLORS, XFE_ERR_OFFSET + 179,
"Couldn't read number of animation colors.\nUsing default animation.")
ResDef(XFE_ANIM_READING_COLORS, XFE_ERR_OFFSET + 180,
"Couldn't read animation colors.\nUsing default animation.")
ResDef(XFE_ANIM_READING_FRAMES, XFE_ERR_OFFSET + 181,
"Couldn't read animation frames.\nUsing default animation.")
ResDef(XFE_ANIM_NOT_AT_EOF, XFE_ERR_OFFSET + 182,
"Ignoring extra bytes at end of animation file.")
ResDef(XFE_LOOK_FOR_DOCUMENTS_THAT_HAVE_CHANGED_ON, XFE_ERR_OFFSET + 183,
"Look for documents that have changed on:")
/* I'm not quite sure why these are resources?? ..djw */
ResDef(XFE_CHARACTER, XFE_ERR_OFFSET + 184, "Character") /* 26FEB96RCJ */
ResDef(XFE_LINK, XFE_ERR_OFFSET + 185, "Link")           /* 26FEB96RCJ */
ResDef(XFE_PARAGRAPH, XFE_ERR_OFFSET + 186, "Paragraph") /* 26FEB96RCJ */
ResDef(XFE_IMAGE,     XFE_ERR_OFFSET + 187, "Image")     /*  5MAR96RCJ */
ResDef(XFE_REFRESH_FRAME, XFE_ERR_OFFSET + 188, "Refresh Frame")
ResDef(XFE_REFRESH, XFE_ERR_OFFSET + 189, "Refresh")
ResDef(XFE_MAIL_TITLE_FMT, XFE_ERR_OFFSET + 190, "Netscape Mail: %.900s")
ResDef(XFE_NEWS_TITLE_FMT, XFE_ERR_OFFSET + 191, "Netscape News: %.900s")
ResDef(XFE_TITLE_FMT, XFE_ERR_OFFSET + 192, "Netscape: %.900s")

ResDef(XFE_PROTOCOLS, XFE_ERR_OFFSET + 193, "Protocols")
ResDef(XFE_LANG, XFE_ERR_OFFSET + 194, "Languages")
ResDef(XFE_CHANGE_PASSWORD, XFE_ERR_OFFSET + 195, "Change Password")
ResDef(XFE_SET_PASSWORD, XFE_ERR_OFFSET + 196, "Set Password")
ResDef(XFE_NO_PLUGINS, XFE_ERR_OFFSET + 197, "No Plugins")

/*
 * Messages for the dialog that warns before doing movemail.
 * DEM 4/30/96
 */
ResDef(XFE_CONTINUE_MOVEMAIL, XFE_ERR_OFFSET + 198, "Continue Movemail")
ResDef(XFE_CANCEL_MOVEMAIL, XFE_ERR_OFFSET + 199, "Cancel Movemail")
ResDef(XFE_MOVEMAIL_EXPLANATION, XFE_ERR_OFFSET + 200,
"Netscape will move your mail from %s\n\
to %s/Inbox.\n\
\n\
Moving the mail will interfere with other mailers\n\
that expect already-read mail to remain in\n\
%s." )
ResDef(XFE_SHOW_NEXT_TIME, XFE_ERR_OFFSET + 201, "Show this Alert Next Time" )
ResDef(XFE_EDITOR_TITLE_FMT, XFE_ERR_OFFSET + 202, "Netscape Editor: %.900s")

ResDef(XFE_HELPERS_NETSCAPE, XFE_ERR_OFFSET + 203, "Netscape")
ResDef(XFE_HELPERS_UNKNOWN, XFE_ERR_OFFSET + 204, "Unknown:Prompt User")
ResDef(XFE_HELPERS_SAVE, XFE_ERR_OFFSET + 205, "Save To Disk")
ResDef(XFE_HELPERS_PLUGIN, XFE_ERR_OFFSET + 206, "Plug In : %s")
ResDef(XFE_HELPERS_EMPTY_MIMETYPE, XFE_ERR_OFFSET + 207,
"Mime type cannot be emtpy.")
ResDef(XFE_HELPERS_LIST_HEADER, XFE_ERR_OFFSET + 208, "Description|Handle By")
ResDef(XFE_MOVEMAIL_CANT_DELETE_LOCK, XFE_ERR_OFFSET + 209,
"Can't get new mail; a lock file %s exists.")
ResDef(XFE_PLUGIN_NO_PLUGIN, XFE_ERR_OFFSET + 210,
"No plugin %s. Reverting to save-to-disk for type %s.\n")
ResDef(XFE_PLUGIN_CANT_LOAD, XFE_ERR_OFFSET + 211,
"ERROR: %s\nCant load plugin %s. Ignored.\n")
ResDef(XFE_HELPERS_PLUGIN_DESC_CHANGE, XFE_ERR_OFFSET + 212,
"Plugin specifies different Description and/or Suffixes for mimetype %s.\n\
\n\
        Description = \"%s\"\n\
        Suffixes = \"%s\"\n\
\n\
Use plugin specified Description and Suffixes ?")
ResDef(XFE_CANT_SAVE_PREFS, XFE_ERR_OFFSET + 213, "error Saving options.")
ResDef(XFE_VALUES_OUT_OF_RANGE, XFE_ERR_OFFSET + 214,
	   "Some values are out of range:")
ResDef(XFE_VALUE_OUT_OF_RANGE, XFE_ERR_OFFSET + 215,
	   "The following value is out of range:")
ResDef(XFE_EDITOR_TABLE_ROW_RANGE, XFE_ERR_OFFSET + 216,
	   "You can have between 1 and 100 rows.")
ResDef(XFE_EDITOR_TABLE_COLUMN_RANGE, XFE_ERR_OFFSET + 217,
	   "You can have between 1 and 100 columns.")
ResDef(XFE_EDITOR_TABLE_BORDER_RANGE, XFE_ERR_OFFSET + 218,
	   "For border width, you can have 0 to 10000 pixels.")
ResDef(XFE_EDITOR_TABLE_SPACING_RANGE, XFE_ERR_OFFSET + 219,
	   "For cell spacing, you can have 0 to 10000 pixels.")
ResDef(XFE_EDITOR_TABLE_PADDING_RANGE, XFE_ERR_OFFSET + 220,
	   "For cell padding, you can have 0 to 10000 pixels.")
ResDef(XFE_EDITOR_TABLE_WIDTH_RANGE, XFE_ERR_OFFSET + 221,
	   "For width, you can have between 1 and 10000 pixels,\n"
	   "or between 1 and 100%.")
ResDef(XFE_EDITOR_TABLE_HEIGHT_RANGE, XFE_ERR_OFFSET + 222,
	   "For height, you can have between 1 and 10000 pixels,\n"
	   "or between 1 and 100%.")
ResDef(XFE_EDITOR_TABLE_IMAGE_WIDTH_RANGE, XFE_ERR_OFFSET + 223,
	   "For width, you can have between 1 and 10000 pixels.")
ResDef(XFE_EDITOR_TABLE_IMAGE_HEIGHT_RANGE, XFE_ERR_OFFSET + 224,
	   "For height, you can have between 1 and 10000 pixels.")
ResDef(XFE_EDITOR_TABLE_IMAGE_SPACE_RANGE, XFE_ERR_OFFSET + 225,
	   "For space, you can have between 1 and 10000 pixels.")
ResDef(XFE_ENTER_NEW_VALUE, XFE_ERR_OFFSET + 226,
	   "Please enter a new value and try again.")
ResDef(XFE_ENTER_NEW_VALUES, XFE_ERR_OFFSET + 227,
	   "Please enter new values and try again.")
ResDef(XFE_EDITOR_LINK_TEXT_LABEL_NEW, XFE_ERR_OFFSET + 228,
	   "Enter the text of the link:")
ResDef(XFE_EDITOR_LINK_TEXT_LABEL_IMAGE, XFE_ERR_OFFSET + 229,
	   "Linked image:")
ResDef(XFE_EDITOR_LINK_TEXT_LABEL_TEXT, XFE_ERR_OFFSET + 230,
	   "Linked text:")
ResDef(XFE_EDITOR_LINK_TARGET_LABEL_NO_TARGETS, XFE_ERR_OFFSET + 231,
	   "No targets in the selected document")
ResDef(XFE_EDITOR_LINK_TARGET_LABEL_SPECIFIED, XFE_ERR_OFFSET + 232,
	   "Link to a named target in the specified document (optional).")
ResDef(XFE_EDITOR_LINK_TARGET_LABEL_CURRENT, XFE_ERR_OFFSET + 233,
	   "Link to a named target in the current document (optional).")
ResDef(XFE_EDITOR_WARNING_REMOVE_LINK, XFE_ERR_OFFSET + 234,
	   "Do you want to remove the link?")
ResDef(XFE_UNKNOWN, XFE_ERR_OFFSET + 235,
	   "<unknown>")
ResDef(XFE_EDITOR_TAG_UNOPENED, XFE_ERR_OFFSET + 236,
	   "Unopened Tag: '<' was expected")
ResDef(XFE_EDITOR_TAG_UNCLOSED, XFE_ERR_OFFSET + 237,
	   "Unopened Tag:  '>' was expected")
ResDef(XFE_EDITOR_TAG_UNTERMINATED_STRING, XFE_ERR_OFFSET + 238,
	   "Unterminated String in tag: closing quote expected")
ResDef(XFE_EDITOR_TAG_PREMATURE_CLOSE, XFE_ERR_OFFSET + 239,
	   "Premature close of tag")
ResDef(XFE_EDITOR_TAG_TAGNAME_EXPECTED, XFE_ERR_OFFSET + 240,
	   "Tagname was expected")
ResDef(XFE_EDITOR_TAG_UNKNOWN, XFE_ERR_OFFSET + 241,
	   "Unknown tag error")
ResDef(XFE_EDITOR_TAG_OK, XFE_ERR_OFFSET + 242,
	   "Tag seems ok")
ResDef(XFE_EDITOR_ALERT_FRAME_DOCUMENT, XFE_ERR_OFFSET + 243,
	   "This document contains frames. Currently the editor\n"
	   "cannot edit documents which contain frames.")
ResDef(XFE_EDITOR_ALERT_ABOUT_DOCUMENT, XFE_ERR_OFFSET + 244,
	   "This document is an about document. The editor\n"
	   "cannot edit about documents.")
ResDef(XFE_ALERT_SAVE_CHANGES, XFE_ERR_OFFSET + 245,
	   "You must save this document locally before\n"
	   "continuing with the requested action.")
ResDef(XFE_WARNING_SAVE_CHANGES, XFE_ERR_OFFSET + 246,
	   "Do you want to save changes to:\n%.900s?")
ResDef(XFE_ERROR_GENERIC_ERROR_CODE, XFE_ERR_OFFSET + 247,
	   "The error code = (%d).")
ResDef(XFE_EDITOR_COPY_DOCUMENT_BUSY, XFE_ERR_OFFSET + 248,
	   "Cannot copy or cut at this time, try again later.")
ResDef(XFE_EDITOR_COPY_SELECTION_EMPTY, XFE_ERR_OFFSET + 249,
	   "Nothing is selected.")
ResDef(XFE_EDITOR_COPY_SELECTION_CROSSES_TABLE_DATA_CELL, XFE_ERR_OFFSET + 250,
	   "The selection includes a table cell boundary.\n"
	   "Deleting and copying are not allowed.")
ResDef(XFE_COMMAND_EMPTY, XFE_ERR_OFFSET + 251,
	   "Empty command specified!")
ResDef(XFE_EDITOR_HTML_EDITOR_COMMAND_EMPTY, XFE_ERR_OFFSET + 252,
	   "No html editor command has been specified in Editor Preferences.\n"
	   "Specify the file argument with %f. Netscape will replace %f with\n"
	   "the correct file name. Example:\n"
	   "             xterm -e vi %f\n"
	   "Would you like to enter a value in Editor Preferences now?")
ResDef(XFE_ACTION_SYNTAX_ERROR, XFE_ERR_OFFSET + 253,
	   "Syntax error in action handler: %s")
ResDef(XFE_ACTION_WRONG_CONTEXT, XFE_ERR_OFFSET + 254,
	   "Invalid window type for action handler: %s")
ResDef(XFE_EKIT_ABOUT_MESSAGE, XFE_ERR_OFFSET + 255,
	   "Modified by the Netscape Navigator Administration Kit.\n"
           "Version: %s\n"
           "User agent: C")

ResDef(XFE_PRIVATE_MIMETYPE_RELOAD, XFE_ERR_OFFSET + 256,
 "Private Mimetype File (%s) has changed on disk.  Discard your unsaved changes\n\
and reload?")
ResDef(XFE_PRIVATE_MAILCAP_RELOAD, XFE_ERR_OFFSET + 257,
 "Private Mailcap File (%s) has changed on disk.  Discard your unsaved changes\n\
and reload?")
ResDef(XFE_PRIVATE_RELOADED_MIMETYPE, XFE_ERR_OFFSET + 258,
 "Private Mimetype File (%s) has changed on disk and is being reloaded.")
ResDef(XFE_PRIVATE_RELOADED_MAILCAP, XFE_ERR_OFFSET + 259,
 "Private Mailcap File (%s) has changed on disk and is being reloaded.")
ResDef(XFE_EDITOR_IMAGE_EDITOR_COMMAND_EMPTY, XFE_ERR_OFFSET + 260,
	   "No image editor command has been specified in Editor Preferences.\n"
	   "Specify the file argument with %f. Netscape will replace %f with\n"
	   "the correct file name. Example:\n"
	   "             xgifedit %f\n"
	   "Would you like to enter a value in Editor Preferences now?")
ResDef(XFE_ERROR_COPYRIGHT_HINT, XFE_ERR_OFFSET + 261,
 "You are about to download a remote\n"
 "document or image.\n"
 "You should get permission to use any\n"
 "copyrighted images or documents.")
ResDef(XFE_ERROR_READ_ONLY, XFE_ERR_OFFSET + 262,
	   "The file is marked read-only.")
ResDef(XFE_ERROR_BLOCKED, XFE_ERR_OFFSET + 263,
	   "The file is blocked at this time. Try again later.")
ResDef(XFE_ERROR_BAD_URL, XFE_ERR_OFFSET + 264,
	   "The file URL is badly formed.")
ResDef(XFE_ERROR_FILE_OPEN, XFE_ERR_OFFSET + 265,
	   "Error opening file for writing.")
ResDef(XFE_ERROR_FILE_WRITE, XFE_ERR_OFFSET + 266,
	   "Error writing to the file.")
ResDef(XFE_ERROR_CREATE_BAKNAME, XFE_ERR_OFFSET + 267,
	   "Error creating the temporary backup file.")
ResDef(XFE_ERROR_DELETE_BAKFILE, XFE_ERR_OFFSET + 268,
	   "Error deleting the temporary backup file.")
ResDef(XFE_WARNING_SAVE_CONTINUE, XFE_ERR_OFFSET + 269,
	   "Continue saving document?")
ResDef(XFE_ERROR_WRITING_FILE, XFE_ERR_OFFSET + 270,
	   "There was an error while saving the file:\n%.900s")
ResDef(XFE_EDITOR_DOCUMENT_TEMPLATE_EMPTY, XFE_ERR_OFFSET + 271,
	   "The new document template location is not set.\n"
	   "Would you like to enter a value in Editor Preferences now?")
ResDef(XFE_EDITOR_AUTOSAVE_PERIOD_RANGE, XFE_ERR_OFFSET + 272,
	   "Please enter an autosave period between 0 and 600 minutes.")
ResDef(XFE_EDITOR_BROWSE_LOCATION_EMPTY, XFE_ERR_OFFSET + 273,
	   "The default browse location is not set.\n"
	   "Would you like to enter a value in Editor Preferences now?")
ResDef(XFE_EDITOR_PUBLISH_LOCATION_INVALID, XFE_ERR_OFFSET + 274,
	   "Publish destination must begin with \"ftp://\" or \"http://\".\n"
	   "Please enter new values and try again.")
ResDef(XFE_EDITOR_IMAGE_IS_REMOTE, XFE_ERR_OFFSET + 275,
	   "Image is at a remote location.\n"
	   "Please save image locally before editing.")
ResDef(XFE_COLORMAP_WARNING_TO_IGNORE, XFE_ERR_OFFSET + 276,
	   "cannot allocate colormap")
ResDef(XFE_UPLOADING_FILE, XFE_ERR_OFFSET + 277,
	   "Uploading file to remote server:\n%.900s")
ResDef(XFE_SAVING_FILE, XFE_ERR_OFFSET + 278,
	   "Saving file to local disk:\n%.900s")
ResDef(XFE_LOADING_IMAGE_FILE, XFE_ERR_OFFSET + 279,
	   "Loading image file:\n%.900s")
ResDef(XFE_FILE_N_OF_N, XFE_ERR_OFFSET + 280,
	   "File %d of %d")
ResDef(XFE_ERROR_SRC_NOT_FOUND, XFE_ERR_OFFSET + 281,
	   "Source not found.")
ResDef(XFE_WARNING_AUTO_SAVE_NEW_MSG, XFE_ERR_OFFSET + 282,
	   "Press Cancel to turn off AutoSave until you\n"
	   "save this document later.")

END_STR(mcom_cmd_xfe_xfe_err_h_strings)

#endif /* __XFE_XFE_ERR_H_ */

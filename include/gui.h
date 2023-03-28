#ifndef _GUI_H_
#define _GUI_H_

/* These are defined in libnet/mkhttp.c.

  XP_AppName      The name of the client program - usually "Netscape", but
                  possibly something else for bundled versions, like the
                  MCI client.

  XP_AppCodeName  The name sent at the HTTP vendor ID string; regardless of 
                  the value of XP_AppName, this must be "Mozilla" or 
                  everything will break.

  XP_AppVersion   The version number of the client as a string.  This is the
                  string sent along with the vendor ID string, so it should be
                  of the form "1.1N (Windows)" or "1.1N (X11; SunOS 4.1.3)".
 */

XP_BEGIN_PROTOS
#ifdef XP_WIN
extern char *XP_AppName, *XP_AppCodeName, *XP_AppVersion;
#else
extern const char *XP_AppName, *XP_AppCodeName, *XP_AppVersion;
#endif
XP_END_PROTOS

/* this define is needed for error message efficiency
 *
 * please don't comment it out for UNIX - LJM
 */
/* this is constant across languages - do NOT localize it */
#define XP_CANONICAL_CLIENT_NAME "Netscape"

/* name of the program */
/* XP_LOCAL_CLIENT_NAME was never used consistently: use XP_AppName instead. */

#endif /* _GUI_H_ */

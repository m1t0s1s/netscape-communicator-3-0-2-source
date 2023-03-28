
#ifndef GLHIST_H
#define GLHIST_H

#include "ntypes.h"

XP_BEGIN_PROTOS

/* if the url was found in the global history then the then number of seconds since
 * the last access is returned.  if the url is not found -1 is returned
 */
extern int GH_CheckGlobalHistory(char * url);

/* add or update the url in the global history
 */
extern void GH_UpdateGlobalHistory(URL_Struct * URL_s);

/* save the global history to a file and remove the list from memory
 */
/*extern void GH_CleanupGlobalHistory(void);*/

/* save the global history to a file and remove the list from memory
 */
extern void GH_SaveGlobalHistory(void);

/* free the global history list
 */
extern void GH_FreeGlobalHistory(void);

/* clear the entire global history list
 */
extern void GH_ClearGlobalHistory(void);

/* set the maximum time for an object in the Global history in
 * number of seconds
 */
extern void GH_SetGlobalHistoryTimeout(int32 timeout_interval);

/* start global history tracking
 */
extern void GH_InitGlobalHistory(void);

/* create an HTML stream and push a bunch of HTML about
 * the global history
 *
 * returns -1
 */
extern int NET_DisplayGlobalHistoryInfoAsHTML(MWContext *context,
                                   				URL_Struct *URL_s,
                                   				int format_out);


XP_END_PROTOS

#endif /* GLHIST_H */

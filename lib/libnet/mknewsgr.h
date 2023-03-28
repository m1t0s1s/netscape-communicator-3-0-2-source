
/* functions to manage large numbers of USENET newsgroups
 * store them on disk, free them, search them, etc.
 *
 * Lou Montulli
 */

#ifndef MKNEWSGR_H
#define MKNEWSGR_H

/* save a newsgroup name
 *
 * will munge the newsgroup name passed in for efficiency
 *
 */
extern void NET_StoreNewsGroup(char *hostname, XP_Bool is_secure, char * newsgroup);

/* free up the list of newsgroups
 */
extern void NET_FreeNewsgroups(char *hostname, XP_Bool is_secure);

/* sort the newsgroups
 */
extern void NET_SortNewsgroups(char *hostname, XP_Bool is_secure);

/* Search and display newsgroups
 */
extern int NET_DisplayNewsgroups(MWContext *context,
								 char *hostname,
								 Bool is_secure, 
								 char *search_string);

/* Save them out to disk
 */
extern void NET_SaveNewsgroupsToDisk(char *hostname, XP_Bool is_secure);

/* read them from disk
 */
extern time_t NET_ReadNewsgroupsFromDisk(char *hostname, XP_Bool is_secure);

/* Get the last access date, load the newsgroups from
 * disk if they are not loaded
 */
extern time_t NET_NewsgroupsLastUpdatedTime(char *hostname, XP_Bool is_secure);

#endif


#ifndef MKSMTP_H
#define MKSMTP_H

extern int NET_MailtoLoad (ActiveEntry * cur_entry);
extern int NET_ProcessMailto (ActiveEntry *cur_entry);
/* abort the connection in progress
 */
extern int NET_InterruptMailto(ActiveEntry * cur_entry);
/* Free any memory that might be used in caching etc.
 */
extern void NET_CleanupMailto(void);

#endif /* MKSMTP_H */

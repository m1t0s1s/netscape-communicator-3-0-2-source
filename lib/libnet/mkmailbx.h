
#ifndef MK_MAILBX_H
#define MK_MAILBX_H

MODULE_PRIVATE int 
NET_MailboxLoad (ActiveEntry * ce);

MODULE_PRIVATE int 
NET_ProcessMailbox (ActiveEntry *ce);

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptMailbox(ActiveEntry * ce);

/* Free any memory that might be used in caching etc.
 */
MODULE_PRIVATE void
NET_CleanupMailbox(void);

#endif /* MK_MAILBX_H */

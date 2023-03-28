
#ifndef MKIMAP4_H
#define MKIMAP4_H

XP_BEGIN_PROTOS

/* start a imap4 load
 */
MODULE_PRIVATE int
NET_IMAP4Load (ActiveEntry * ce);

/* NET_ProcessIMAP4  will control the state machine that
 * loads messages from a iamp4 server
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int
NET_ProcessIMAP4 (ActiveEntry *ce);

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptIMAP4(ActiveEntry * ce);


XP_END_PROTOS


#endif /* MKIMAP4_H */

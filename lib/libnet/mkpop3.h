
#ifndef MKPOP3_H
#define MKPOP3_H

/* start a pop3 load
 */
MODULE_PRIVATE int
NET_Pop3Load (ActiveEntry * ce);

/* NET_process_Pop3  will control the state machine that
 * loads messages from a pop3 server
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int
NET_ProcessPop3 (ActiveEntry *ce);

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptPop3(ActiveEntry * ce);


#endif /* not MKPOP3_H */

#ifndef MKLDAP_H
#define MKLDAP_H

XP_BEGIN_PROTOS

/* start an LDAP load
 */
int NET_LoadLdap (ActiveEntry *ce);

/* NET_ProcessLdap  will control the state machine that
 * loads messages from an LDAP server
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
int NET_ProcessLdap (ActiveEntry *ce);

/* abort the connection in progress
 */
int NET_InterruptLdap (ActiveEntry *ce);

XP_END_PROTOS

#endif 

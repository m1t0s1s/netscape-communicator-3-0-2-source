#ifndef _SECITEM_H_
#define _SECITEM_H_
/*
 * secitem.h - public data structures and prototypes for handling
 *	       SECItems
 *
 * $Id: secitem.h,v 1.2 1996/10/14 06:03:29 jsw Exp $
 */

#include "prarena.h"
#include "seccomon.h"

SEC_BEGIN_PROTOS

/*
** Compare two item's returning the difference between them.
*/
extern SECComparison SECITEM_CompareItem(SECItem *a, SECItem *b);

/*
** Copy "from" to "to"
*/
extern SECStatus SECITEM_CopyItem(PRArenaPool *arena, SECItem *to, SECItem *from);

extern SECItem *SECITEM_DupItem(SECItem *from);

/*
** Free "zap". If freeit is PR_TRUE then "zap" itself is freed.
*/
extern void SECITEM_FreeItem(SECItem *zap, PRBool freeit);

/*
** Zero and then free "zap". If freeit is PR_TRUE then "zap" itself is freed.
*/
extern void SECITEM_ZfreeItem(SECItem *zap, PRBool freeit);

SEC_END_PROTOS

#endif /* _SECITEM_H_ */

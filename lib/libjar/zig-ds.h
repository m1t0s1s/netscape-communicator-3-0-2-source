#ifndef __ZIG_DS_h_
#define __ZIG_DS_h_
#ifdef XP_WIN32
#include <windows.h>
#endif /* XP_WIN32 */

#include "xp_mcom.h"

XP_BEGIN_PROTOS

/* Typedefs */
typedef struct ZSLinkStr ZSLink;
typedef struct ZSListStr ZSList;

/*
** Circular linked list. Each link contains a pointer to the object that
** is actually in the list.
*/ 
struct ZSLinkStr {
    ZSLink *next;
    ZSLink *prev;
    void *thing;
};

struct ZSListStr {
    ZSLink link;
};

#define ZS_InitList(lst)	     \
{				     \
    (lst)->link.next = &(lst)->link; \
    (lst)->link.prev = &(lst)->link; \
    (lst)->link.thing = 0;	     \
}

#define ZS_ListEmpty(lst) \
    ((lst)->link.next == &(lst)->link)

#define ZS_ListHead(lst) \
    ((lst)->link.next)

#define ZS_ListTail(lst) \
    ((lst)->link.prev)

#define ZS_ListIterDone(lst,lnk) \
    ((lnk) == &(lst)->link)

#define ZS_AppendLink(lst,lnk)	    \
{				    \
    (lnk)->next = &(lst)->link;	    \
    (lnk)->prev = (lst)->link.prev; \
    (lst)->link.prev->next = (lnk); \
    (lst)->link.prev = (lnk);	    \
}

#define ZS_InsertLink(lst,lnk)	    \
{				    \
    (lnk)->next = (lst)->link.next; \
    (lnk)->prev = &(lst)->link;	    \
    (lst)->link.next->prev = (lnk); \
    (lst)->link.next = (lnk);	    \
}

#define ZS_RemoveLink(lnk)	     \
{				     \
    (lnk)->next->prev = (lnk)->prev; \
    (lnk)->prev->next = (lnk)->next; \
    (lnk)->next = 0;		     \
    (lnk)->prev = 0;		     \
}

extern ZSLink *ZS_NewLink(void *thing);
extern ZSLink *ZS_FindLink(ZSList *lst, void *thing);
extern void ZS_DestroyLink(ZSLink *lnk, int freeit);

XP_END_PROTOS

#endif /* __ZIG_DS_h_ */

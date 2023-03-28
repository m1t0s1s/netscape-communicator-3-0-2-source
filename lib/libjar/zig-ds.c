#include "zig-ds.h"

/* Added by dell for libjar 6-Jan-97 */
/* These are old DS_* routines renamed to ZS_* */

ZSLink *ZS_NewLink(void *thing)
{
    ZSLink *lnk = (ZSLink*) XP_CALLOC (1, sizeof(ZSLink));
    if (lnk) {
        lnk->thing = thing;
    }
    return lnk;
}

ZSLink *ZS_FindLink(ZSList *lst, void *thing)
{
    ZSLink *lnk;

    lnk = ZS_ListHead(lst);
    while (!ZS_ListIterDone(lst,lnk)) {
        if (lnk->thing == thing) {
            return lnk;
        }
        lnk = lnk->next;
    }
    return 0;
}

void ZS_DestroyLink(ZSLink *lnk, int freeit)
{
    if (freeit) {
        XP_FREE (lnk);
    }
}

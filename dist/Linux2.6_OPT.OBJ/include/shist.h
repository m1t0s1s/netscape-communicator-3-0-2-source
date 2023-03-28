
#ifndef SHIST_H
#define SHIST_H

#include "xp_list.h"
#include "ntypes.h"
#include "hotlist.h"


/* see shistele.h for the structure definitions. :(
 */

/* --------------------------------------------------------------------------
 * Session history module routines
 */

struct MWContext_;
struct URL_Struct_;

#define HIST_NEW_OBJECT   0
#define HIST_MOVE_FORWARD 1
#define HIST_MOVE_BACK    2

XP_BEGIN_PROTOS

/* Front-End Specialized Functions */
extern void             SHIST_InitSession(struct MWContext_ * ctxt);
extern void SHIST_EndSession(MWContext * ctxt);

/* copys all the session data from the old context into the
 * new context.  Does not effect data in old_context session history
 *
 * if new_context has not had SHIST_InitSession called for it
 * it will be called to initalize it.
 */
extern void
SHIST_CopySession(MWContext * new_context, MWContext * old_context);

                        /* these update position and buttons! */
extern void             SHIST_AddDocument(struct MWContext_ * ctxt, History_entry * entry);
extern History_entry *  SHIST_GetPrevious(struct MWContext_ * ctxt);
extern History_entry *  SHIST_GetNext(struct MWContext_ * ctxt);
                        /* convenience functions */
extern History_entry *  SHIST_CreateHistoryEntry (struct URL_Struct_ * URL_s, char * title);
extern URL_Struct *     SHIST_CreateURLStructFromHistoryEntry(struct MWContext_ * ctxt, 
															  History_entry * entry);
extern URL_Struct *     SHIST_CreateWysiwygURLStruct(struct MWContext_ * ctxt,
                                                     History_entry * entry);
extern BM_Entry*	SHIST_CreateHotlistStructFromHistoryEntry(History_entry * h);

extern void SHIST_FreeHistoryEntry (MWContext * ctxt, History_entry * entry);


/* Standard History Functions */
#ifndef NEW_FRAME_HIST
#define NEW_FRAME_HIST 1
#endif
#ifdef NEW_FRAME_HIST
extern int              SHIST_CanGoBack(MWContext * ctxt);
extern int              SHIST_CanGoForward(MWContext * ctxt);
#else
extern int              SHIST_CanGoBack(History * hist);
extern int              SHIST_CanGoForward(History * hist);
#endif /* NEW_FRAME_HIST */
extern History_entry *  SHIST_GetEntry(History * hist, int entry_number);
extern History_entry *  SHIST_GetCurrent(History * hist);
extern XP_List *        SHIST_GetList(MWContext * ctxt);

/* sets the current doc pointer to the index specified in the call
 *
 * entry numbering begins at one.
 */
extern void SHIST_SetCurrent(History * hist, int entry_number);

/* set the title of the current document
 */
extern void SHIST_SetTitleOfCurrentDoc(History * hist, char * title);


/* set the layout specific form data neccessary to recreate the user settable
 * entries within a form
 */
PUBLIC void
SHIST_SetCurrentDocFormListData(MWContext * context, void * form_data);

/* set the layout-plugin specific embed data neccessary to recreate the
 * last session state within the embedded object.
 */
PUBLIC void
SHIST_SetCurrentDocEmbedListData(MWContext * context, void * embed_data);

/* set the layout grid data neccessary to recreate the
 * grid when revisited through history.
 */
PUBLIC void
SHIST_SetCurrentDocGridData(MWContext * context, void * grid_data);

/* set the window object for a grid being resized.
 */
PUBLIC void
SHIST_SetCurrentDocWindowData(MWContext * context, void * window_data);

/* set the layout applet data neccessary to recreate the
 * applet when revisited through history.
 */
PUBLIC void
SHIST_SetCurrentDocAppletData(MWContext * context, void * applet_data);

/* set the position of the current document
 */
extern void SHIST_SetPositionOfCurrentDoc(History * hist, int32 position_tag);

/* gets an index associated with the entry
 *
 * entry numbering begins at one.
 *
 * zero is returned if object not found
 */
extern int SHIST_GetIndex(History * hist, History_entry * entry);

/* return the n'th object
 */
extern History_entry * SHIST_GetObjectNum(History * hist, int n);

XP_END_PROTOS


#endif /* SHIST_H */

/* -*- Mode: C; tab-width: 8 -*-
   motifdnd.h --- atoms and other stuff defined for use with the
                  new motif drag and drop.
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 15-Jun-96.
 */

#ifndef MOTIFDND_H
#define MOTIFDND_H

#include <Xm/VendorS.h>
#include <Xm/DragDrop.h>

/* Atoms used for drag/drop targets -- these are initialized in
   the DND_Initialize routine. */


extern Atom NS_DND_URL;
extern Atom NS_DND_BOOKMARK;
extern Atom NS_DND_ADDRESS;
extern Atom NS_DND_HTMLTEXT;
extern Atom NS_DND_FOLDER;
extern Atom NS_DND_MESSAGE;
extern Atom NS_DND_COLUMN;
extern Atom NS_DND_COMPOUNDTEXT;

/* for dropping webjumpers in browser contexts */
extern Atom _SGI_ICON;

/* initialize target atoms and new drag start actions */
void DND_initialize_atoms(Widget toplevel);

/* used for installing the drag start action on the location icon */
void DND_install_location_drag(Widget location_label);

/* used for installing the drop site on the shell associated with
   a browser context, so you can drag bookmarks/urls/webjumpers. */
void DND_install_browser_drop(MWContext *context);

#endif /* MOTIFDND_H */

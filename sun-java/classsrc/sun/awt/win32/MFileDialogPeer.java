/*
 * @(#)MFileDialogPeer.java	1.14 95/12/09 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
package sun.awt.win32;

import java.awt.*;
import java.awt.peer.*;
import java.io.FilenameFilter;

public class MFileDialogPeer extends MDialogPeer implements FileDialogPeer {
    native void pCreate(MComponentPeer parent,
	int mode, String title, String file);

    void create(MComponentPeer parent) {
        FileDialog target = (FileDialog)this.target;

        pCreate(parent, target.getMode(), target.getTitle(), target.getFile());
    }

    public MFileDialogPeer(FileDialog target) {
	super(target);

	FileDialog	fdialog = (FileDialog)target;
	String		dir = fdialog.getDirectory();

	if (dir != null) {
	    setDirectory(dir);
	}
    }
    native void		pShow();
    native void		pHide();
    public void		handleSelected(String file) {
	int	index = file.lastIndexOf('\\');

	String dir;
	if (index == -1) {
	    dir = ".\\";
	    ((FileDialog)target).setFile(file);
	} else {
	    dir = file.substring(0, index+1);
	    ((FileDialog)target).setFile(file.substring(index+1));
	}
	((FileDialog)target).setDirectory(dir);
    }
    public void		handleCancel() {
	((FileDialog)target).setFile(null);
    }
    public void		handleQuit() {
	handleCancel();
	hide();
    }
    public native void	setDirectory(String dir);
    public native void	setFile(String file);
    public void		setFilenameFilter(FilenameFilter filter) {
        // Not implemented for 1.0
    }

    public void setInsets(Insets i) {
        insets = i;
    }
}

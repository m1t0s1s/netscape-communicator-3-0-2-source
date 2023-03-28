/*
 * @(#)MFileDialogPeer.java	1.10 95/12/04 Sami Shaio
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
package sun.awt.motif;

import java.awt.*;
import java.awt.peer.*;
import java.io.FilenameFilter;

public class MFileDialogPeer extends MDialogPeer implements FileDialogPeer {
    native void create(MComponentPeer parent);
    void create(MComponentPeer parent, Object arg) {
	create(parent);
    }
    public MFileDialogPeer(FileDialog target) {
	super(target);
	FileDialog	fdialog = (FileDialog)target;
	String		dir = fdialog.getDirectory();

	insets = new Insets(0, 0, 0, 0);
	if (dir != null) {
	    setDirectory(dir);
	} else if ((dir = fdialog.getFile()) != null) {
	    setFile(dir);
	}
    }
    native void		pReshape(int x, int y, int width, int height);
    native void		pShow();
    native void		pHide();
    native void		setFileEntry(String dir, String file);

    public void		handleSelected(String file) {
	int	index = file.lastIndexOf('/');

	String dir;

	if (index == -1) {
	    dir = "./";
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

    public  void setDirectory(String dir) {
	String f = ((FileDialog)target).getFile();

	setFileEntry(dir, (f != null) ? f : "");
    }


    public  void setFile(String file) {
	String d = ((FileDialog)target).getDirectory();

	setFileEntry((d != null) ? d : "", file);
    }


    public void		setFilenameFilter(FilenameFilter filter) {
    }
}

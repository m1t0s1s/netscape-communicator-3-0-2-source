package sun.awt.macos;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageRepresentation;

class MFramePeer extends MPanelPeer implements FramePeer {

    boolean		mFrameIsSecure;
    boolean		mFrameIsEmbedded;
    boolean		mIsWindow;

	////////////////////////////////////////////////////////////////////////
	//
	//	Constructors, Destructors and Initialization 
	//
	////////////////////////////////////////////////////////////////////////

    MFramePeer(Frame target) {
   
		super(target);

		mIsWindow = false;

		if (target.getTitle() != null) {
		    setTitle(target.getTitle());
		}

		Font f = target.getFont();
		if (f == null) {
		    f = new Font("Dialog", Font.PLAIN, 12);
		    target.setFont(f);
		    setFont(f);
		}

		Color c = target.getBackground();
		if (c == null) {
		    target.setBackground(Color.lightGray);
		    setBackground(Color.lightGray);
		}

		c = target.getForeground();
		if (c == null) {
		    target.setForeground(Color.black);
		    setForeground(Color.black);
		}

		Image icon = target.getIconImage();
		if (icon != null) {
		    setIconImage(icon);
		}

		setResizable(target.isResizable());

		Rectangle r = target.bounds();
		reshape(r.x, r.y, r.width, r.height);
 
    }
  
    MFramePeer(Window target) {

		super(target);
    
		mIsWindow = true;
	
		Font f = target.getFont();
		if (f == null) {
		    f = new Font("Dialog", Font.PLAIN, 12);
		    target.setFont(f);
		    setFont(f);
		}

		Color c = target.getBackground();
		if (c == null) {
		    target.setBackground(Color.lightGray);
		    setBackground(Color.lightGray);
		}

		c = target.getForeground();
		if (c == null) {
		    target.setForeground(Color.black);
		    setForeground(Color.black);
		}

		setResizable(false);

		Rectangle r = target.bounds();
		reshape(r.x, r.y, r.width, r.height);

	}
	
    native void create(MComponentPeer parent);

	////////////////////////////////////////////////////////////////////////
	//
	//	Setting attributes of the frame
	//
	////////////////////////////////////////////////////////////////////////

	native public void setTitle(String title);
   
    public void show() { super.show(); pShow(); }
   	public void reshape(int x, int y, int width, int height) { pReshape(x, y, width, height); }
	public void setIconImage(Image im) { }
	public void setMenuBar(MenuBar mb) { }
	public void setResizable(boolean resizeable) { }
	public void setCursor(Image img) { }
    public void dispose() { pDispose(); }
	
	native private void pReshape(int x, int y, int width, int height);
	native private void pDispose();
    native private void pShow();
	
	protected void handleFrameReshape(int x, int y, int width, int height) { 
		target.reshape(x, y, width, height); 
		target.invalidate();
		target.validate();
		target.repaint();
	}

	protected void handleWindowClose() {
		target.postEvent(new Event(target, Event.WINDOW_DESTROY, null));	
	}
	
	////////////////////////////////////////////////////////////////////////
	//
	//	Front/Back movement notification
	//
	////////////////////////////////////////////////////////////////////////

	native public void toFront();
	native public void toBack();

 	////////////////////////////////////////////////////////////////////////
	//
	//	Cursor control
	//
	////////////////////////////////////////////////////////////////////////

  	native public void setCursor(int cursorType);

}


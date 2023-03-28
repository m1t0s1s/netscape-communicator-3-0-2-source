package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;
import sun.awt.image.ImageRepresentation;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;

public abstract class MComponentPeer implements ComponentPeer {

    Component			target;
    
    int 				pData;
	int					pInternationalData;

	////////////////////////////////////////////////////////////////////////
	//
	//	Mac-specific component information 
	//
	////////////////////////////////////////////////////////////////////////
	
	int					mOwnerPane;

	int					mFont;
	int 				mSize;
	int					mStyle;
	
	Color				mForeColor;
	Color				mBackColor;
	
	boolean				mRecalculateClip;
	boolean				mIsContainer;
	boolean				mIsButton;
	boolean				mIsChoice;
	boolean				mIsResizableFrame;
	
	int					mObscuredRgn;
	int					mClipRgn;

	////////////////////////////////////////////////////////////////////////
	//
	//	Constructors, Destructors and Initialization 
	//
	////////////////////////////////////////////////////////////////////////

    MComponentPeer(Component target) {
    	
    	//	Remember our target, and get our parent peer
    	//	(its our target’s parent’s peer)
    
		this.target = target;

		Component parent = target.getParent();
		
		setup();

		create((MComponentPeer)((parent != null) ? parent.getPeer() : null));

		//	Initialize the component 
	
		initialize();

    }

	//	MComponentPeer cannot instantiate any concrete AWT peers
	//	itself, only the subclasses can.

    abstract void create(MComponentPeer parent);
	
    void create(MComponentPeer parent, Object arg) {
		create(parent);
    }

	void setup() {

		mForeColor = new Color(0, 0, 0);
		mBackColor = new Color(192, 192, 192);
		
		pSetup();		

	}

    void initialize() {
    	
  		//	Resize the peer to the dimensions of the 
		//	target.

		Rectangle r = target.bounds();
		reshape(r.x, r.y, r.width, r.height);
		
	  	//	If the target is visible, then so should
    	//	be the peer.
    
		if (target.isVisible()) {
		    show();
		} else {
		    hide();
		}
		
		//	Get the foreground and background color
		//	color charateristics of the target, as well
		//	as the font information, and make them our
		//	own.
		
		Color c;
		Font  f;
	
		if ((c = target.getForeground()) != null) {
		    setForeground(c);
		}
		if ((c = target.getBackground()) != null) {
		    setBackground(c);
		}
		if ((f = target.getFont()) != null) {
		    setFont(f);
		}

	}
	
    public void dispose() { pDispose(); }


	////////////////////////////////////////////////////////////////////////
	//
	//	Common Accessors 
	//
	////////////////////////////////////////////////////////////////////////

	public ColorModel getColorModel() { return MToolkit.getStaticColorModel(); }

	public java.awt.Toolkit getToolkit() { return Toolkit.getDefaultToolkit(); }

    public Graphics getGraphics() {
		Graphics g = new MacGraphics(this);
		g.setColor(target.getForeground());
		g.setFont(target.getFont());
		return g;
	}

	public FontMetrics getFontMetrics(Font font) {
		return MacFontMetrics.getFontMetrics(font);
	}


	////////////////////////////////////////////////////////////////////////
	//
	//	Methods to control appearance
	//
	////////////////////////////////////////////////////////////////////////

    public void show() { pShow(); }
    public void hide() { pHide(); }
	
    public void enable() { pEnable(); }
    public void disable() { pDisable(); }

    public void paint(Graphics g) {
		g.setColor(target.getForeground());
		g.setFont(target.getFont());
		target.paint(g);
    }

	public void repaint(long tm, int x, int y, int width, int height) { pRepaint(tm, x, y, width, height); }
	public void print(Graphics g) { pPrint(g); }
	public void reshape(int x, int y, int width, int height) { pReshape(x, y, width, height); }
	

	////////////////////////////////////////////////////////////////////////
	//
	//	Methods to control color and font
	//
	////////////////////////////////////////////////////////////////////////

	public void setForeground(Color foreColor) {
		mForeColor = foreColor;
	}
	
	public void setBackground(Color backColor) {
		mBackColor = backColor;
	}
	
	native public void setFont(Font newFont);
	
	
	////////////////////////////////////////////////////////////////////////
	//
	//	Methods to handle events
	//
	////////////////////////////////////////////////////////////////////////

    native public boolean handleEvent(Event e);

	////////////////////////////////////////////////////////////////////////
	//
	//	Imaging methods
	//
	////////////////////////////////////////////////////////////////////////

    public Image createImage(ImageProducer producer) {
	return new MacImage(producer);
    }
    public Image createImage(int width, int height) {
	return new MacImage(width, height);
    }
    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return MToolkit.prepareScrImage(img, w, h, o);
    }
    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return MToolkit.checkScrImage(img, w, h, o);
	}

 	////////////////////////////////////////////////////////////////////////
	//
	//	Methods to handle sizing
	//
	////////////////////////////////////////////////////////////////////////

	public Dimension minimumSize() { return target.size(); }
	public Dimension preferredSize() { return minimumSize(); }
	
 	////////////////////////////////////////////////////////////////////////
	//
	//	Methods to handle focus arbitration
	//
	////////////////////////////////////////////////////////////////////////

    public native void requestFocus();

    public native void nextFocus();

    protected void gotFocus() {
		target.postEvent(new Event(target, Event.GOT_FOCUS, null));
    }

    protected void lostFocus() {
		target.postEvent(new Event(target, Event.LOST_FOCUS, null));
    }

 	////////////////////////////////////////////////////////////////////////
	//
	//	NATIVE STUBS
	//
	////////////////////////////////////////////////////////////////////////

    private native void pSetup();
    private native void pShow();
    private native void pHide();
    private native void pEnable();
    private native void pDisable();
    private native void pPrint(Graphics g);
    private native void pReshape(int x, int y, int width, int height);
    private native void pRepaint(long tm, int x, int y, int width, int height);
    private native void pDispose();
	public void finalize() { dispose(); }

	protected void handleWindowClosing() {
		target.postEvent(new Event(target, Event.WINDOW_DESTROY, null));
	}

    void handleExpose() {
    	InterfaceThread.postInterfaceEvent(InterfaceEvent.EXPOSE, target, 0, 0, 0, 0, 0, 0, 0);
    }
    
    void handleRepaint(int x, int y, int width, int height) {
    	InterfaceThread.postInterfaceEvent(InterfaceEvent.REPAINT, target, x, y, width, height, 0, 0, 0);
    }

	////////////////////////////////////////////////////////////////////////
	//
	//	Key up/down (including ACTION keys)
	//
	////////////////////////////////////////////////////////////////////////

	private int modifiersToJavaFlags(int modifiers) {
	
		int	flags = 0;
	
   		if ((modifiers & 0x0100) != 0)		// cmdKey
   			flags |= Event.META_MASK;

   		if ((modifiers & 0x0200) != 0)		// shiftKey
   			flags |= Event.SHIFT_MASK;
   			
   		if ((modifiers & 0x0800) != 0)		// optionKey
   			flags |= Event.ALT_MASK;
   			
   		if ((modifiers & 0x1000) != 0)		// controlKey
   			flags |= Event.CTRL_MASK;
   		
   		return flags;
   		 
	}

    protected void handleKeyEvent(int x, int y, int key, int modifiers, boolean down, int eventNumber) {
   	
   		long 		when = 0;
   		int 		flags = modifiersToJavaFlags(modifiers);
   		boolean		action = true;

		//	This is not exactly correct, but it will have to do for now.
   		
   		when = System.currentTimeMillis();
   		
   		//	Map special keys from their native Macintosh values to their
   		//	AWT counterparts.
   		
		switch (key) {

			case 0x01:
   				key = Event.HOME;
				break;

			case 0x04:
   				key = Event.END;
				break;

			case 0x0B:
   				key = Event.PGUP;
				break;

			case 0x0C:
   				key = Event.PGDN;
				break;
   		
   			case 0x1E:
   				key = Event.UP;
   				break;
   		
   			case 0x1C:
   				key = Event.LEFT;
   				break;

   			case 0x1D:
   				key = Event.RIGHT;
   				break;

   			case 0x1F:
   				key = Event.DOWN;
   				break;
   		
   			default:
   				action = false;
   				break;
   		
   		}
   		
   		//	Finally, dispatch the event.
   		 
   		if (action)
		   	if (down)
				InterfaceThread.postInterfaceEvent(InterfaceEvent.KEY_ACTION, target, x, y, 0, 0, when, flags, key, 0, eventNumber);
			else
				InterfaceThread.postInterfaceEvent(InterfaceEvent.KEY_ACTION_RELEASE, target, x, y, 0, 0, when, flags, key, 0, eventNumber);
   		else 
		   	if (down)
				InterfaceThread.postInterfaceEvent(InterfaceEvent.KEY_PRESS, target, x, y, 0, 0, when, flags, key, 0, eventNumber);
			else
				InterfaceThread.postInterfaceEvent(InterfaceEvent.KEY_RELEASE, target, x, y, 0, 0, when, flags, key, 0, eventNumber);
  	
  	}


	////////////////////////////////////////////////////////////////////////
	//
	//	Mouse Up/Down
	//
	////////////////////////////////////////////////////////////////////////

	protected void handleMouseChanged(int x, int y, int modifiers, int clickCount, boolean down, int eventNumber) {
	
   		long 		when = System.currentTimeMillis();
   		int 		flags = modifiersToJavaFlags(modifiers);

	   	if (down)
			InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_DOWN, target, x, y, 0, 0, when, flags, 0, clickCount, eventNumber);
		else
			InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_UP, target, x, y, 0, 0, when, flags, 0, clickCount, eventNumber);
    
    }

	////////////////////////////////////////////////////////////////////////
	//
	//	Mouse movement
	//
	////////////////////////////////////////////////////////////////////////

	protected void handleMouseMoved(int x, int y, int modifiers, boolean down) {
	
   		long 		when = System.currentTimeMillis();
   		int 		flags = modifiersToJavaFlags(modifiers);
	
	   	if (down)
			InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_DRAGGED, target, x, y, 0, 0, when, flags, 0);
		else
			InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_MOVED, target, x, y, 0, 0, when, flags, 0);
		
	}


	protected void handleMouseEnter(int x, int y, int modifiers) {
    
  		long 		when = System.currentTimeMillis();
   		int 		flags = modifiersToJavaFlags(modifiers);
		
		InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_ENTER, target, x, y, 0, 0, when, flags, 0);

	}

    protected void handleMouseExit(int x, int y, int modifiers) {
    
  		long 		when = System.currentTimeMillis();
   		int 		flags = modifiersToJavaFlags(modifiers);
		
		InterfaceThread.postInterfaceEvent(InterfaceEvent.MOUSE_EXIT, target, x, y, 0, 0, when, flags, 0);

    }
}


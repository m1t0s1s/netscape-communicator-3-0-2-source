package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

public class MTextFieldPeer extends MComponentPeer implements TextFieldPeer, ActionComponent {

	public MTextFieldPeer(TextField target) {
		super(target);
	}

    native void create(MComponentPeer parent);

	void initialize() {
	
		TextField txt = (TextField)target;
		
		setText(txt.getText());
		if (txt.echoCharIsSet()) {
		    setEchoCharacter(txt.getEchoChar());
		}

		select(txt.getSelectionStart(), txt.getSelectionEnd());

		setEditable(txt.isEditable());
		super.initialize();
	
	}

	public void setBackground(Color c) {
		TextField t = (TextField)target;
		if (t.isEditable()) {
			c = c.brighter();
		}
		super.setBackground(c);
	}

    public native void setEditable(boolean editable);
    public native void select(int selStart, int selEnd);
    public native int getSelectionStart();
    public native int getSelectionEnd();
    public native void setText(String l);
    public native String getText();
    public native void setEchoCharacter(char c);

    public Dimension minimumSize() {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.stringWidth(((TextField)target).getText()) + 20, 
					    fm.getHeight() + 6);
    }
    public Dimension preferredSize(int cols) {
		return minimumSize(cols);
    }

	public Dimension minimumSize(int cols) {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.charWidth('0') * cols + 20,fm.getHeight() + 6);
    }

	public void action() {
		target.postEvent(new Event(target, Event.ACTION_EVENT, ((TextField)target).getText()));
	}

    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}

    public void action(int refcon) {
		System.err.println("Internal AWT error");
	}

	public void handleAction() {
    	InterfaceThread.postInterfaceEvent(target);
	}
    
}

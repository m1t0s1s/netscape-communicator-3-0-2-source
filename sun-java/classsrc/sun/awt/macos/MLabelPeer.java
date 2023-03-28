package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MLabelPeer extends MComponentPeer implements LabelPeer {

	int		mAlignment;

    MLabelPeer(Label target) {
		super(target);
    }

    native void create(MComponentPeer parent);

    public void initialize() {

		Label		l = (Label)target;
		String		txt;
		int			align;

		if ((txt = l.getText()) != null) {
			setText(l.getText());
		}
		
		if ((align = l.getAlignment()) != Label.LEFT) {
			setAlignment(align);
		}

		super.initialize();
   
	}

	public Dimension minimumSize() {
	
		FontMetrics fm = getFontMetrics(target.getFont());
		String label = ((Label)target).getText();
		if (label == null) label = "";
		return new Dimension(fm.stringWidth(label) + 14, 
		fm.getHeight() + 8);
	
	}

	public native void setText(String label);
	public native void setAlignment(int alignment);

}

package sun.awt.macos;

import java.awt.*;

public interface ActionComponent {

	public void action();
	
	public void action(int refcon);

	public void action(boolean refcon);
	
}


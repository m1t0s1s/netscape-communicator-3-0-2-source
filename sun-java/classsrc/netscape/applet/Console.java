package netscape.applet;

import java.awt.*;
import java.io.*;

class ConsoleInputStream extends InputStream {
    Console console;

    ConsoleInputStream(Console console) {
	this.console = console;
    }

    public int read() {
	return -1;
    }
    public int read(byte b[]) {
	return -1;
    }
    public int read(byte b[], int off, int len) throws IOException {
	return -1;
    }
}

class ConsoleOutputStream extends OutputStream {
    Console console;
    byte doh[] = new byte[1];

    public static int MAXIMUM_CHARACTERS_IN_CONSOLE = 0;
    public static final int CONSOLE_TRIM_SIZE = 2 * 1024;			

    ConsoleOutputStream(Console console) {
	this.console = console;
    }

    public void limitConsoleText(int newCharacterCount) {
	/* XXX take this out for now because it stops delivering output after the limit
	int currentTextLength = console.frame.text.getText().length();

 	// If we don't know the max number of characters that 
	// can be written to the console, then get it from a 
	// property... it is platform specific.
	if (MAXIMUM_CHARACTERS_IN_CONSOLE == 0)
	{
	    String consoleMaxCharString;
	    consoleMaxCharString = System.getProperty("console.bufferlength", "32768");
	    MAXIMUM_CHARACTERS_IN_CONSOLE = Integer.parseInt(consoleMaxCharString);
	}	

 	if (currentTextLength + newCharacterCount > MAXIMUM_CHARACTERS_IN_CONSOLE)	
	{
	    console.frame.text.replaceText("", 0, CONSOLE_TRIM_SIZE); 
	}
	*/
    }

    public void write(int b) {
	doh[0] = (byte) b;
	String s = new String(doh, 0, 0, 1);
	limitConsoleText(1);
	console.frame.text.insertText(s, 99999);
    }

    public void write(byte b[]) {
	String s = new String(b, 0, 0, b.length);
	limitConsoleText(b.length);
	console.frame.text.insertText(s, 99999);
    }

    public void write(byte b[], int off, int len) {
	String s = new String(b, 0, off, len);
	limitConsoleText(len);
	console.frame.text.insertText(s, 99999);
    }
}

class HorizontalRule extends Canvas {
    Color dark, light;

    HorizontalRule() {
	dark = new Color(0x666666);
	light = new Color(0xeeeeee);
    }

    public Dimension preferredSize() {
	Dimension d = getParent().size();
	d.height = 2;
	return d;
    }

    public void paint(Graphics g) {
	Dimension d = size();
	g.setColor(dark);
	g.drawLine(0, 0, d.width - 1, 0);
	g.setColor(light);
	g.drawLine(0, 1, d.width - 1, 1);
    }
}

class ConsoleFrame extends Frame {
    Console console;
    TextArea text;
    Button reset, hide;

    ConsoleFrame(Console console) {
	this.console = console;
	setTitle("Java Console");

	GridBagLayout gb = new GridBagLayout();
	setLayout(gb);

	text = new TextArea(20, 60);
	text.setEditable(false);
	GridBagConstraints c1 = new GridBagConstraints();
	c1.fill = GridBagConstraints.BOTH;
	c1.weightx = 1.0;
	c1.weighty = 1.0;
	c1.gridwidth = GridBagConstraints.REMAINDER;
	gb.setConstraints(text, c1);
	add(text);

	HorizontalRule hr = new HorizontalRule();
	GridBagConstraints c2 = new GridBagConstraints();
	c2.fill = GridBagConstraints.HORIZONTAL;
	c2.weightx = 1.0;
	c2.insets = new Insets(1, 1, 0, 0);
	c2.gridwidth = GridBagConstraints.REMAINDER;
	gb.setConstraints(hr, c2);
	add(hr);

	reset = new Button("Clear");
	GridBagConstraints c3 = new GridBagConstraints();
	c3.insets = new Insets(4, 0, 2, 4);
	c3.anchor = GridBagConstraints.EAST;
	gb.setConstraints(reset, c3);
	add(reset);

	hide = new Button("Close");
	GridBagConstraints c4 = new GridBagConstraints();
	c4.insets = new Insets(4, 0, 2, 4);
	c4.anchor = GridBagConstraints.EAST;
	gb.setConstraints(hide, c4);
	add(hide);

	pack();
    }

    public boolean handleEvent(Event evt) {
	if ((evt.id == Event.WINDOW_DESTROY) || 
	     ((evt.target == hide) && (evt.id == Event.ACTION_EVENT))) {
	    console.hide();
	    return true;
	}
	if ((evt.target == reset) && (evt.id == Event.ACTION_EVENT)) {
	    console.reset();
	    return true;
	}
	return super.handleEvent(evt);
    }

    public boolean keyDown(Event evt, int key) {
	if ((key >= '0') && (key <= '9')) {
	    MozillaAppletContext.debug = key - '0';
	    System.err.println("# Applet debug level set to " + MozillaAppletContext.debug);
	    return true;
	} else if ((key == 'd') || (key == 'D')) {
	    MozillaAppletContext.dumpState(System.err);
	    return true;
	} else if ((key == 'g') || (key == 'G')) {
	    System.err.println("# Performing a garbage collection...");
	    System.gc();
	    printMemoryStats("# GC complete: memory");
	    return true;
	} else if ((key == 'f') || (key == 'F')) {
	    System.err.println("# Performing finalization...");
	    System.runFinalization();
	    printMemoryStats("# Finalization complete: memory");
	    return true;
	} else if ((key == 'm') || (key == 'M')) {
	    printMemoryStats("Memory");
	    return true;
	} else if (key == 'x') {
	    System.err.print("# Dumping memory to 'memory.out'...");
            Console.dumpMemory(false);
	    System.err.println("done.");
	    return true;
	} else if (key == 'X') {
	    System.err.print("# Dumping memory to 'memory.out'...");
            Console.dumpMemory(true);
	    System.err.println("done.");
	    return true;
	} else if ((key == 's') || (key == 'S')) {
	    System.err.print("# Dumping memory summary to 'memory.out'...");
	    Console.dumpMemorySummary();
	    System.err.println("done.");
	    return true;
	} else if ((key == 't') || (key == 'T')) {
	    System.err.print("# Dumping thread info to 'memory.out'...");
	    Console.dumpNSPRInfo();
	    System.err.println("done.");
	    return true;
	}
	else if ((key == 'k') || (key == 'K')) {
	    System.err.print("# Checkpointing memory...");
	    Console.checkpointMemory();
	    System.err.println("done.");
	    return true;
	}
	else if ((key == 'c') || (key == 'C')) {
	    reset();
	    return true;
	}
	else if ((key == 'q') || (key == 'Q')) {
	    console.hide();
	    return true;
	}
	else if ((key == 'h') || (key == 'H')) {
	    printConsoleHelp();
	    return true;
	}
	else if ((key == 'w') || (key == 'W')) {
	    System.err.print("# Dumping application heaps...");
	    console.dumpApplicationHeaps();
	    System.err.println("done.");
	    return true;
	}
	else {
	    return super.keyDown(evt, key);
	}
    }

    void printMemoryStats(String label) {
	Runtime rt = Runtime.getRuntime();
	System.err.println("# " + label + ": " +
			   rt.totalMemory() + " free: " + rt.freeMemory() +
			   " (" + (rt.freeMemory() * 100 / rt.totalMemory()) + "%)");
    }

    void printConsoleHelp() {
	System.out.println("Netscape Java Console Commands:");
	System.out.println("  c:   clear console window");
	System.out.println("  d:   dump applet context state to console");
	System.out.println("  f:   finalize objects on finalization queue");
	System.out.println("  g:   garbage collect");
	System.out.println("  h:   print this help message");
	System.out.println("  m:   print current memory use to console");
	System.out.println("  q:   hide console");
	System.out.println("  s:   dump memory summary to \"memory.out\"");
	System.out.println("  t:   dump thread info to \"memory.out\"");
	System.out.println("  x:   dump memory to \"memory.out\"");
	System.out.println("  X:   dump detailed dump memory to \"memory.out\"");
	System.out.println("  k:   checkpoint memory to \"mem.out\" (Windows only)");
	System.out.println("  w:   dump GlobalAlloc blocks to \"heaps.out\" (Windows only)");
	System.out.println("  0-9: set applet debug level to <n>");
	System.out.println();
    }

    void reset() {
	text.setText("");
    }
}

class Console {
    ConsoleFrame frame;

    Console() {
	frame = new ConsoleFrame(this);
        InputStream in =  new ConsoleInputStream(this);
        PrintStream out = new PrintStream(new BufferedOutputStream(
	    new ConsoleOutputStream(this), 128), true);
        setSystemIO(in, out, out);
    }

    /* hack: Unless we have "mutable" in/out/err in System.java,
	we need to force the setting into "final static" members.
	For now, we use a native method.  Me *might* some day
	switch to fancy mutable streams that can be properly
	retargeted with Java accessCheck*() calls as protection.
	This native method forwards to a native in system.c, 
        bypassing all our inter-class-security. */
    private native static void setSystemIO(InputStream in, 
					   PrintStream out, 
					   PrintStream err);

    static native void dumpNSPRInfo();
    static native void dumpMemory(boolean detailed);
    static native void dumpMemorySummary();
    static native void checkpointMemory();
    native static void dumpApplicationHeaps();

    void reset() {
	frame.reset();
    }

    void show() {
	if (!frame.isVisible()) {
	    frame.show();
	    MozillaAppletContext.setConsoleState(1);
	}
    }

    void hide() {
	if (frame.isVisible()) {
	    frame.hide();
	    MozillaAppletContext.setConsoleState(0);
	}
    }
} 

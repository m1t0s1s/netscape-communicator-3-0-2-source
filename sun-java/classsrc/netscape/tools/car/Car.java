/*
 * @(#)
 *
 * CAR file reader/writer code
 *
 * @author Kipp E.B. Hickman
 */
package netscape.tools.car;

import java.util.*;
import java.io.*;

public class Car {
    Vector contents;
    Hashtable excludes;
    long tocsize;

    /**
     * Make a brand new empty car
     */
    public Car(Hashtable excludes) {
	contents = new Vector();
	this.excludes = excludes;
    }

    /*
     * Add a file/directory to the car. When given a directory add it's
     * entire contents recursively.
     */
    void add(String name) throws FileNotFoundException, IOException {
	try {
	    File file = new File(name);
	    if (file.isDirectory()) {
		String files[] = file.list();
		if (files != null) {
		    for (int i = 0; i < files.length; i++) {
			add(name + File.separator + files[i]);
		    }
		} else {
		    System.err.println(file.getPath() + " is empty");
		}
	    } else if (file.isFile()) {
		if (!file.canRead()) {
		    System.err.println(file.getPath() + " is unreadable");
		}
		add(file);
	    } else {
		System.err.println(file.getPath() + " is not a file or a directory");
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	    throw new FileNotFoundException(name);
	}
    }

    /*
     * Add a file to the car. Setup an Entry for it.
     */
    void add(File file) throws IOException {
	String name = file.getPath();
	if (excludes.containsKey(name)) {
	    return;
	}
	if (name.endsWith(".class")) {
	    CarTOCEntry e = new CarTOCEntry();
	    e.source = file;
	    e.name = name.startsWith("./") ? name.substring(2) : name;
	    if (excludes.containsKey(e.name)) {
		return;
	    }
	    e.lastModified = file.lastModified();
	    e.length = file.length();
	    e.offset = 0;
	    contents.addElement(e);
	    tocsize = 0;
	}
    }

    void buildTOC() {
	long off = headersize() + tocsize();
	Enumeration e = contents.elements();
	while (e.hasMoreElements()) {
	    CarTOCEntry entry = (CarTOCEntry) e.nextElement();
	    entry.offset = off;
	    off += entry.length;
	}
    }

    int count() { return contents.size(); }

    long tocsize() {
	if (tocsize == 0) {
	    Enumeration e = contents.elements();
	    while (e.hasMoreElements()) {
		CarTOCEntry entry = (CarTOCEntry) e.nextElement();
		tocsize = tocsize + entry.tocsize();
	    }
	}
	return tocsize;
    }

    long headersize() {
	return 4 + 4 + 4 + 8;
    }

    public final int MAGIC = 0xBEBACAFF;
    public final int VERSION = 0x00000002;

    /**
     * Write the car file out
     */
    void write(DataOutputStream out) throws FileNotFoundException, IOException {
	buildTOC();

	// Write out the header
	out.writeInt(MAGIC);
	out.writeInt(VERSION);
	out.writeInt(count());
	out.writeLong(tocsize());

	// Write out the TOC
	writeTOC(out);

	// Write out the contents
	Enumeration e = contents.elements();
	while (e.hasMoreElements()) {
	    CarTOCEntry entry = (CarTOCEntry) e.nextElement();
	    BufferedInputStream in = new BufferedInputStream(
		new FileInputStream(entry.source), 4096);
	    byte buf[] = new byte[4096];
	    int nb;
	    while ((nb = in.read(buf, 0, 4096)) >= 0) {
		out.write(buf, 0, nb);
	    }
	    in.close();
	}
	out.close();
    }

    void writeTOC(DataOutputStream out) throws IOException {
	Enumeration e = contents.elements();
	while (e.hasMoreElements()) {
	    CarTOCEntry entry = (CarTOCEntry) e.nextElement();
	    entry.writeTOC(out);
	}
    }

    void printTOC(PrintStream out, boolean verbose) throws IOException {
	Enumeration e = contents.elements();
	while (e.hasMoreElements()) {
	    CarTOCEntry entry = (CarTOCEntry) e.nextElement();
	    entry.printTOC(out);
	}
    }

    public String toString() {
	return "<car>";
    }

    /**
     * Read in an existing car
     */
    public Car(DataInputStream in) throws IOException {
	int magic = in.readInt();
	if (magic != MAGIC) {
	    throw new IOException("bad magic number " + magic + " (expected "
		+ MAGIC + ")");
	}
	int version = in.readInt();
	if (version != VERSION) {
	    throw new IOException("bad version number " + version + " (expected "
		+ VERSION + ")");
	}
	int entries = in.readInt();
	if (entries < 0) {
	    throw new IOException("bad entries " + entries);
	}
	long length = in.readLong();
	readTOC(in, entries);	// XXX: should be able to check length
    }

    void readTOC(DataInputStream in, int n) throws IOException, UTFDataFormatException {
	contents = new Vector();
	for (int i = 0; i < n; i++) {
	    contents.addElement(new CarTOCEntry(in));
	}
    }

    /**
     * Check a car out (kick the tires)
     */
    boolean check(RandomAccessFile in, String root, boolean verbose) {/* XXX root not used */
	Enumeration e = contents.elements();
	while (e.hasMoreElements()) {
	    CarTOCEntry entry = (CarTOCEntry) e.nextElement();
	    entry.source = new File(entry.name);
	    try {
		if (verbose) {
		    System.out.println("checking " + entry.name);
		}
		entry.check(in);
	    } catch (IOException x) {
		x.printStackTrace();
		System.err.println("IO exception processing " + entry.source.getPath());
		return false;
	    } catch (CarCheckException x) {
		x.printStackTrace();
		System.err.println(entry.source.getPath() + " " + x.getMessage());
		return false;
	    }
	}
	return true;
    }
}

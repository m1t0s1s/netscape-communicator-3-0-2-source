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

public class Main {
    public static void Usage() {
	System.out.println(
	    "Usage: car -[ct] achive [-r root] files/directories ..."
	    );
	Runtime.getRuntime().exit(0);
    }

    public static void main(String argv[]) {
	Vector dirs = new Vector();
	Hashtable excludes = new Hashtable();
	boolean creating = false;
	boolean listing = false;
	boolean checking = false;
	boolean verbose = false;
	String archive = "";
	for (int i = 0; i < argv.length; i++) {
	    if (argv[i].charAt(0) == '-') {
		boolean error = false;
		boolean picked = false;
		if ("-verbose".startsWith(argv[i])) {
		    if (picked) error = true;
		    picked = true;
		    verbose = true;
		}
		if ("-create".startsWith(argv[i])) {
		    if (picked) error = true;
		    picked = true;
		    creating = true;
		    if (++i < argv.length) {
			archive = argv[i];
		    } else {
			error = true;
		    }
		}
		if ("-check".startsWith(argv[i])) {
		    if (picked) error = true;
		    picked = true;
		    checking = true;
		    if (++i < argv.length) {
			archive = argv[i];
		    } else {
			error = true;
		    }
		}
		if ("-toc".startsWith(argv[i])) {
		    if (picked) error = true;
		    picked = true;
		    listing = true;
		    if (++i < argv.length) {
			archive = argv[i];
		    } else {
			error = true;
		    }
		}
		if ("-exclude".startsWith(argv[i])) {
		    if (picked) error = true;
		    picked = true;
		    if (++i < argv.length) {
			excludes.put(argv[i], argv[i]);
		    } else {
			error = true;
		    }
		}
		if (error || !picked)
		    Usage();
	    } else
		dirs.addElement(argv[i]);
	}
	if (!listing && !creating && !checking)
	    Usage();
	if (checking && dirs.size() != 1)
	    Usage();

	try {
	    if (creating) {
		Car car = new Car(excludes);
		Enumeration e = dirs.elements();
		while (e.hasMoreElements()) {
		    String s = (String) e.nextElement();
		    try {
			car.add(s);
		    } catch (FileNotFoundException x) {
			System.out.println("File " + x.getMessage()
					   + " not found");
		    }
		}
		DataOutputStream out = new DataOutputStream(
		    new BufferedOutputStream(new FileOutputStream(archive)));
		car.write(out);
		out.close();
	    } else if (listing || checking) {
		DataInputStream in = new DataInputStream(
		    new BufferedInputStream(new FileInputStream(archive)));
		Car car = new Car(in);
		if (checking) {
		    car.check(new RandomAccessFile(archive, "r"),
			      (String) dirs.elementAt(0), verbose);
		} else {
		    car.printTOC(System.out, verbose);
		}
	    }
	} catch (IOException x) {
	    System.out.println("IO error: " + x);
	}
    }
}

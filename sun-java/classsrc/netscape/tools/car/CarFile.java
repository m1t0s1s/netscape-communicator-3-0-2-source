package netscape.tools.car;

import java.io.*;
import java.util.Hashtable;

class CarInputStream extends InputStream {
    RandomAccessFile in;
    long offset;
    int limit;

    CarInputStream(RandomAccessFile in, long offset, int length) {
	this.in = in;
	this.offset = offset;
	this.limit = length;
    }

    public int read() throws IOException {
	if (limit == 0) {
	    return -1;
	}
	in.seek(offset);
	int c = in.read();
	if (c == -1) {
	    limit = 0;
	} else {
	    offset++;
	    limit--;
	}
	return c;
    }

    public int read(byte b[]) throws IOException {
	return read(b, 0, b.length);
    }

    public int read(byte b[], int off, int len) throws IOException {
	if (len > limit) {
	    len = limit;
	}
	in.seek(offset);
	int rv = in.read(b, off, len);
	if (rv == -1) {
	    limit = 0;
	} else {
	    offset += rv;
	    limit -= rv;
	}
	return rv;
    }

    public long skip(long n) throws IOException {
	if (n > limit) {
	    limit = 0;
	    return 0;
	} else {
	    limit -= n;
	    offset += n;
	    return n;
	}
    }

    public int available() throws IOException {
	return limit;
    }

    public void close() {
    }

    public synchronized void mark(int readlimit) {
    }

    public synchronized void reset() throws IOException {
	throw new IOException("mark not supported");
    }

    public boolean markSupported() {
	return false;
    }
}

public class CarFile {
    RandomAccessFile in;
    Hashtable toc;

    public CarFile(File file) throws IOException {
	in = new RandomAccessFile(file, "r");
	readHeader();
    }

    public final int MAGIC = 0xBEBACAFF;
    public final int VERSION = 0x00000002;

    /**
     * Read in the header and the table of contents
     */
    void readHeader() throws IOException {
	int magic = in.readInt();
	if (magic != MAGIC) {
	    throw new IOException("bad magic number " + magic
				  + " (expected " + MAGIC + ")");
	}
	int version = in.readInt();
	if (version != VERSION) {
	    throw new IOException("bad version number " + version
				  + " (expected " + VERSION + ")");
	}
	int entries = in.readInt();
	if (entries < 0) {
	    throw new IOException("bad entries " + entries);
	}
	long length = in.readLong();
	readTOC(in, entries);	// XXX: should be able to check length
    }

    void readTOC(DataInput in, int n) throws IOException, UTFDataFormatException {
	toc = new Hashtable();
	for (int i = 0; i < n; i++) {
	    CarTOCEntry e = new CarTOCEntry(in);
	    toc.put(e.name, e);
	}
    }

    /**
     * Lookup the name in the car file and if found return an
     * input stream which can read the file in. If not found
     * return null.
     */
    public InputStream findFile(String name) {
	if (name.startsWith("./")) {
	    name = name.substring(2);
	}
	CarTOCEntry entry = (CarTOCEntry) toc.get(name);
	if (entry != null) {
	    return new CarInputStream(in, entry.offset, (int) entry.length);
	}
	return null;
    }
}

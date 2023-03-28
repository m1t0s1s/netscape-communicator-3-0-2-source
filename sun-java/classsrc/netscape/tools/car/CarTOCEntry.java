/*
 * @(#)
 *
 * CAR file reader/writer code
 *
 * @author Kipp E.B. Hickman
 */
package netscape.tools.car;

import java.io.*;
import java.util.Date;

class CarTOCEntry {
    File source;
    String name;
    long lastModified;
    long offset;
    long length;

    CarTOCEntry() {
    }

    static int utfLength(String str) {
	int strlen = str.length();
	int utflen = 0;
	for (int i = 0 ; i < strlen ; i++) {
	    int c = str.charAt(i);
	    if ((c >= 0x0001) && (c <= 0x007F)) {
		utflen++;
	    } else if (c > 0x07FF) {
		utflen += 3;
	    } else {
		utflen += 2;
	    }
	}
	return utflen;
    }

    static void writeUTF(OutputStream out, String str) throws IOException {
	int strlen = str.length();
	for (int i = 0 ; i < strlen ; i++) {
	    int c = str.charAt(i);
	    if ((c >= 0x0001) && (c <= 0x007F)) {
		out.write(c);
	    } else if (c > 0x07FF) {
		out.write(0xE0 | ((c >> 12) & 0x0F));
		out.write(0x80 | ((c >>  6) & 0x3F));
		out.write(0x80 | ((c >>  0) & 0x3F));
	    } else {
		out.write(0xC0 | ((c >>  6) & 0x1F));
		out.write(0x80 | ((c >>  0) & 0x3F));
	    }
	}
    }

    static String readUTF(InputStream in) throws IOException, UTFDataFormatException {
	// Read utf encoded bytes until we get a \0
	char str[] = new char[32];
	int ix = 0;
	for (;;) {
	    int c = in.read();
	    if (c == 0)
		break;
	    if (ix == str.length) {
		char newstr[] = new char[str.length * 2];
		System.arraycopy(str, 0, newstr, 0, str.length);
		str = newstr;
	    }
	    if ((c & 0x80) == 0) {
		// one byte encoding
		str[ix++] = (char) c;
	    } else if ((c & 0xE0) == 0xC0) {
		// two byte encoding
		int c1 = in.read();
		if ((c1 & 0xc0) != 0x80) {
		    throw new UTFDataFormatException();		  
		}
		str[ix++] = (char) (((c & 0x1f) << 6) | (c1 & 0x3f));
	    } else if ((c & 0xF0) == 0xE0) {
		// three byte encoding
		int c1 = in.read();
		int c2 = in.read();
		if (((c1 & 0xc0) != 0x80) || ((c2 & 0xc0) != 0x80)) {
		    throw new UTFDataFormatException();		  
		}
		str[ix++] = (char) (((c & 0x1f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f));
	    } else
		throw new UTFDataFormatException();		  
	}
	return new String(str, 0, ix);
    }

    static String readUTF(DataInput in) throws IOException, EOFException, UTFDataFormatException {
	// Read utf encoded bytes until we get a \0
	char str[] = new char[32];
	int ix = 0;
	for (;;) {
	    int c = in.readByte();
	    if (c == 0)
		break;
	    if (ix == str.length) {
		char newstr[] = new char[str.length * 2];
		System.arraycopy(str, 0, newstr, 0, str.length);
		str = newstr;
	    }
	    if ((c & 0x80) == 0) {
		// one byte encoding
		str[ix++] = (char) c;
	    } else if ((c & 0xE0) == 0xC0) {
		// two byte encoding
		int c1 = in.readByte();
		if ((c1 & 0xc0) != 0x80) {
		    throw new UTFDataFormatException();		  
		}
		str[ix++] = (char) (((c & 0x1f) << 6) | (c1 & 0x3f));
	    } else if ((c & 0xF0) == 0xE0) {
		// three byte encoding
		int c1 = in.readByte();
		int c2 = in.readByte();
		if (((c1 & 0xc0) != 0x80) || ((c2 & 0xc0) != 0x80)) {
		    throw new UTFDataFormatException();		  
		}
		str[ix++] = (char) (((c & 0x1f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f));
	    } else
		throw new UTFDataFormatException();		  
	}
	return new String(str, 0, ix);
    }

    int tocsize() {
	return 3*8 + utfLength(name) + 1;
    }

    /**
     * Write out the TOC entry
     */
    void writeTOC(DataOutputStream out) throws IOException {
	out.writeLong(lastModified);
	out.writeLong(offset);
	out.writeLong(length);
	writeUTF(out, name);
	out.writeByte(0);
    }

    /**
     * Read in a TOC entry
     */
    CarTOCEntry(DataInput in) throws IOException, UTFDataFormatException {
	lastModified = in.readLong();
	offset = in.readLong();
	length = in.readLong();
	name = readUTF(in);
    }

    public String toString() {
	return source.getPath() + ": "
	    + " modified=" + lastModified
	    + " length=" + length
	    + " offset=" + offset;
    }

    void printTOC(PrintStream out) throws IOException {
	out.println((new Date(lastModified)).toString()
		    + " " + length
		    + " " + offset
		    + " " + name);
    }

    void check(RandomAccessFile in) throws IOException, CarCheckException {
	byte buf1[] = new byte[4096];
	byte buf2[] = new byte[4096];
	BufferedInputStream in2 = new BufferedInputStream(
	    new FileInputStream(source.getPath()));
	long fileLength = source.length();
	if (fileLength != length) {
	    throw new CarCheckException("length mismatch: car has " + length +
					" while file has " + fileLength);
	}

	in.seek(offset);
	long off = 0;
	long rem = fileLength;
	while (rem != 0) {
	    long amount = rem;
	    if (amount > 4096) {
		amount = 4096;
	    }
	    int nb1 = in.read(buf1, 0, (int)amount);
	    int nb2 = in2.read(buf2, 0, nb1);
	    if ((nb1 != (int) amount) || (nb2 != nb1)) {
		throw new IOException("nb1 = " + nb1 + ", nb2 = " + nb2);
	    }
	    for (int i = 0; i < nb1; i++) {
		if (buf1[i] != buf2[i]) {
		    throw new CarCheckException("data misatch: offset="
						+ (off + i) +
						" read " + ((int)buf1[i]) +
						" expected " + ((int)buf2[i]));
		}
	    }
	    rem -= amount;
	    off += amount;
	}
    }
}

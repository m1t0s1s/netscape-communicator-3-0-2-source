package netscape.server.base;

import java.io.FileNotFoundException;

/**
 * filestat objects represent the attributes of a given path. What type of 
 * file is it (regular file, directory, or link to another file), its size,
 * the last time it was accessed or modified, and the last time its 
 * attributes were changed are all available.
 *
 * @author	Rob McCool
 */

public class filestat {
    private long size, acc, lmd, chm;
    private boolean dir, file, link;

    private native void create(String file) throws FileNotFoundException;

    /**
      Get the attributes of the pathname which file points to. Throws
      FileNotFoundException if the given file does not exist or can't be 
      read.
     */
    public filestat(String file) throws FileNotFoundException {
        create(file);
    }
    /**
      Return the last modification date of the file. This long is the
      number of milliseconds after January 1, 1970 UTC that the file was 
      last accessed. You may use this date to construct a Date object.
     */
    public long whenLastAccessed() { return acc; }
    /**
      Return the last modification date of the file. This long is the
      number of milliseconds after January 1, 1970 UTC that the file was 
      last modified. You may use this date to construct a Date object.
     */
    public long whenLastModified() { return lmd; }
    /**
      Return the last status change of the file. This long is the
      number of milliseconds after January 1, 1970 UTC that the file's 
      access mode was changed. You may use this date to construct a Date 
      object.
     */
    public long whenStatusChanged() { return chm; }
    /**
      Returns the size of the file.
     */
    public long getSize() { return size; }

    /**
      True if the file is a directory.
     */
    public boolean isDirectory() { return dir; }
    /**
      True if the file is a regular file.
     */
    public boolean isFile() { return file; }
    /**
      True if the file is a link to another file. Under UNIX, only symbolic
      links are considered links.
     */
    public boolean isLink() { return link; }
}

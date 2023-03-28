
package netscape.server.base;

import netscape.server.base.sem;

/**
 * shmem is a class which supports server-wide shared memory. This is 
 * generally implemented via memory mapped file I/O. By creating a shared
 * memory object during an Init function, your NSAPI programs can share data.
 *
 * @author	Rob McCool
 */

public class shmem {
    private int Cshmem;
    private sem mutex;

    /**
      Construct a new shared memory object using the given name as the
      full path to a file name in which to store size bytes of shared
      data. If expose is true, then the file will be visible to other
      processes as a file of size bytes at the named path. If needMutex
      is true, then the server will make sure that a server-wide semaphore
      prevents multiple server threads or processes from modifying the same
      area at the same time.
     */
    private native void create(String name, int size, boolean expose);
    public shmem(String name, int size, boolean expose, boolean needMutex) {
        int namepos;
        String mutexName;

        create(name, size, expose);
        if(needMutex) {
            namepos = name.lastIndexOf(System.getProperty("file.separator"));
            if(namepos < 0)
                mutexName = name;
            else
                mutexName = name.substring(namepos + 1);
            mutex = new sem(mutexName, size);
        }
        else
            mutex = null;
    }


    /**
      This constructor is for use by native methods only. Using it from
      Java code is a sure-fire path toward NullPointerExceptions.
     */
    public shmem() {
        Cshmem = 0;
    }


    private native void free();
    public void finalize() {
        free();
    }


    /**
      getByte returns the byte at the given offset within the shared memory
      area.
     */
    public byte getByte(int offset) {
        byte[] b = new byte[1];

        getBytes(offset, b, 1);
        return b[0];
    }


    /**
      Set the byte at the given offset to b.
     */
    public void setByte(int offset, byte b) {
        byte[] ba = new byte[1];

        ba[0] = b;
        setBytes(offset, ba);
    }


    /**
      Transfer length bytes from the given offset within the shared memory
      to the array b.
     */
    public void getBytes(int offset, byte[] b, int length) {
        if(mutex != null)
            mutex.grab();
        CgetBytes(offset, b, length);
        if(mutex != null)
            mutex.release();
    }


    /**
      Fill the array b with bytes from the given offset within the shared
      memory.
     */
    public void getBytes(int offset, byte[] b) {
        getBytes(offset, b, b.length);
    }


    /**
      Transfer length bytes from the array b to the given offset within the
      shared memory.
     */
    public void setBytes(int offset, byte b[], int length) {
        if(mutex != null)
            mutex.grab();
        CsetBytes(offset, b, length);
        if(mutex != null)
            mutex.release();
    }
    /**
      Transfer all bytes from the array b to the given offset within the 
      shared memory.
     */
    public void setBytes(int offset, byte b[]) {
        setBytes(offset, b, b.length);
    }

    private native void CgetBytes(int offset, byte[] b, int length);
    private native void CsetBytes(int offset, byte[] b, int length);
}

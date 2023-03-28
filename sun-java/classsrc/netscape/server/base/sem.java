

package netscape.server.base;

/**
 * A wrapper for the Netscape server's concept of server-wide semaphores.
 * These semaphores are used as a mutual exclusion tool, to make sure that
 * of all of the server's running processes and threads, only one is 
 * executing the section of code you protect at a time.
 *
 * @author	Rob McCool
 */

public class sem {
    private int Csem;

    /**
      Create a new server-wide semaphore, using the given identifier string
      and number to ensure that it is unique.
     */
    private native void create(String name, int number);
    public sem(String name, int number) {
        create(name, number);
    }
    /**
      Using this constructor from anything but a native method guarantees
      NullPointerExceptions.
     */
    public sem() {
        Csem = 0;
    }
    /**
      When you are finished with a sempahore, this de-allocates any 
      resources it may be holding.
     */
    private native void free();
    public void finalize() {
        free();
    }
    /**
      Grab the semaphore and make sure no other thread or process grabs
      it until you release it. If the semaphore is not available, the
      calling thread will block until it is.
     */
    public native void grab();
    /**
      Try to grab the semaphore. Returns true if the thread was able
      to grab the semaphore, false if the semaphore is not currently 
      available.
     */
    public native boolean tgrab();
    /**
      Release the semaphore after a previous grab.
     */
    public native void release();
}

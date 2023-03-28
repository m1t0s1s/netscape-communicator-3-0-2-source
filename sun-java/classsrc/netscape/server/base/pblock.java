
package netscape.server.base;

import netscape.server.base.pb_param;


/**
 * Parameter blocks, or pblocks, represent one of the fundamental hash tables
 * of the NSAPI. They are simple hash tables which use a String name to
 * look up String values. 
 *
 * @author	Rob McCool
 */

public class pblock {
    private int Cpblock;

    private native void create(int size);
    private native void free();


    /**
      Create a new, empty pblock using the given hash table size. Using
      smaller hash table sizes will decrease memory use, but increase
      search times for pblocks with a lot of pairs. Using a larger hash
      table might waste memory.

      Using a zero size guarantees NullPointerExceptions.
     */
    public pblock(int size) {
        // If size is zero, object is being constructed from C code that 
        // already has a Cpblock for us.
        if(size != 0)
            create(size);
        else
            Cpblock = 0;
    }
    public void finalize() {
        free();
    }

    private native pb_param fr(String name, boolean remove);

    /**
      Find a parameter structure with the given name. If you modify this
      pb_param, changes are <strong>not</strong> reflected in the pblock's
      version of that structure. To change it, remove the pb_param first,
      change it, and re-insert it.

      Returns null if there is no entry with the given name.
     */
    public pb_param find(String name) {
        return fr(name, false);
    }
    /**
      If there is a pb_param with the given name in the pblock, return
      its value. Otherwise return null.
     */
    public String findval(String name) {
        pb_param ret = fr(name, false);
        if(ret == null)
            return null;
        return ret.getValue();
    }
    /**
      Find a parameter structure with the given name, and remove it from
      the pblock. You may modify and re-insert this parameter structure, or
      simply remove and discard it. Returns null if there is no entry in
      the pblock with the given name.
     */
    public pb_param remove(String name) {
        return fr(name, true);
    }
    /**
      Insert a new name value pair into the pblock, using a pb_param object.
      Duplicates are not transparently avoided.
     */
    public native void insert(pb_param pb);
    /**
      Insert a new name value pair into the pblock, using the supplied
      strings. Duplicates are not transparently avoided.
     */
    public void insert(String n, String v) {
        pb_param param = new pb_param(n, v);
        insert(param);
    }
    /**
      Insert a new name value pair into the pblock, using the given name 
      string, and converting the given value into a string representing
      the decimal number for its value. Duplicates are not transparently
      avoided.
     */
    public void insert(String n, int v) {
        String vstr = (new Integer(v)).toString();
        insert(n, vstr);
    }
    /**
      Translate this pblock into a string, consisting of multiple 
      name="value" strings separated by spaces.
     */
    public native String toString();

    /**
      Take a string consisting of multiple name="value" strings separated
      by spaces, and add each to the pblock. Duplicates are not necessarily
      handled; if a name is already in the pblock, then the existing 
      entry may either be shadowed or it may shadow the new one. If a
      duplicate is removed, the copy will then be visible.
     */
    public native int addString(String toadd);
}

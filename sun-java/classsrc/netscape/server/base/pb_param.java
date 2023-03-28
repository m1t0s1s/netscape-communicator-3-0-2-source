package netscape.server.base;
import java.lang.String;

/**
 * pb_param is a parameter for pblocks. They are simple name value pairs,
 * with only Strings allowed. You are not allowed direct access to the name
 * and value Strings, but can use methods to read and modify them.
 *
 * @author	Rob McCool
 */


public class pb_param {
    private String name;
    private String value;

    /**
      Construct a new pb_param with the given name n and value v.
     */
    public pb_param(String n, String v) { 
        name = new String(n); 
        value = new String(v); 
    }
    /**
      Set this pb_param's name to n. 
     */
    public void setName(String n) { name = new String(n); }
    /**
      Set this pb_param's value to v.
     */
    public void setValue(String v) { value = new String(v); }

    /**
      Get this pb_param's name.
     */
    public String getName() { return name; }
    /**
      Set this pb_param's name.
     */
    public String getValue() { return value; }
}



package netscape.server.base;

import java.lang.String;
import java.lang.IllegalArgumentException;

/**
 * shexp handles what the Netscape server calls shell expressions. They are
 * wildcard patterns similar to those used by most shell programs. 
 *
 * <pre>
 * o * matches anything
 * o ? matches one character
 * o \ will escape a special character
 * o $ matches the end of the string
 * o [abc] matches one occurence of a, b, or c. The only character that needs
 *   to be escaped in this is ], all others are not special.
 * o [a-z] matches any character between a and z
 * o [^az] matches any character except a or z
 * o ~ followed by another shell expression will remove any pattern
 *     matching the shell expression from the match list
 * o (foo|bar) will match either the substring foo, or the substring bar.
 *             These can be shell expressions as well.
 * o Nested or expressions such as (a|(b|c)) are not allowed.
 * </pre>
 *
 * @author	Rob McCool
 */

public class shexp {
    private shexp() {}

    /**
      valid returns true if the given shell expression is valid, and doesn't
      contain any syntax errors such as missing closing brackets or 
      parenthesis.
     */
    public static native boolean valid(String exp);

    /**
      cmp compares the given string of data against the given shell
      expression, and returns true if the string matches the expression.
      caseless is a boolean describing whether or not alphabetic characters
      should be caseless (i.e. A matches a) or not.
     */
    public static native boolean cmp(String data, String exp, boolean caseless)
        throws IllegalArgumentException;

    /**
      cmp performs a case-sensitive match of the given string of data
      with the given shell expression, and returns true if the string
      matches the expression.
     */
    public static boolean cmp(String data, String exp) 
        throws IllegalArgumentException
    {
        return cmp(data, exp, false);
    }

    /**
      casecmp performs a case-insensitive match of the given string of data
      with the given shell expression, and returns true if the string
      matches the expression.
     */
    public static boolean casecmp(String data, String exp)
        throws IllegalArgumentException
    {
        return cmp(data, exp, true);
    }
}

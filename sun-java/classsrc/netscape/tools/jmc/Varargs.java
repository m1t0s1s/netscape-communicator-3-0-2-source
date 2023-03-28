/*
This interface is a dummy, provided for jric. Using it in a method
declaration will cause 3 versions of the method to be put in the
native interface, e.g.:

interface Foo {
	void format(String fmt, varargs args);
}

struct Foo {
	void (*format)(java_lang_String* fmt, ...);
	void (*formatV)(java_lang_String* fmt, va_list args);
	void (*formatA)(java_lang_String* fmt, JRIValue* args);
};

It is only legal to use Varargs as the last argument in an argument
list.
*/

package netscape.tools.jmc;

import java.util.Vector;

public class Varargs extends Vector {
}

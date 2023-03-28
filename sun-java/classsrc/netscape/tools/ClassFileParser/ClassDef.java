// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class ClassDef {
    public short	flags;
    public String	name;
    public ClassDef	superclass;
    public ClassDef[]	interfaces;
    public FieldDef[]	fields;
    public MethodDef[]	methods;
    public AttrDef[]	attributes;
    public long		modDate; 

    public boolean inheritsFrom(String superName) {
	for (ClassDef clazz = this; clazz != null; clazz = clazz.superclass) {
	    if (clazz.name.equals(superName))
		return true;
	}
	return false;
    }

    String prefix;
    boolean macMode;

    public void setPrintInfo(String prefix, boolean macMode) {
	this.prefix = prefix;
	this.macMode = macMode;
    }

    public String getPrefix() {
	if (prefix != null)
	    return prefix;
	return getCName();
    }

    static final String CTYPE_CLASSNAME = "netscape/tools/jmc/CType";

    public String getCType() {
        return getConstantField("ctype");
    }

    public String getInclude() {
        return getConstantField("include");
    }

    public String getConstantField(String namestring) {
	if (inheritsFrom(CTYPE_CLASSNAME)) {
	    for (ClassDef clazz = this; clazz != null; clazz = clazz.superclass) {
		for (int i = 0; i < clazz.fields.length; i++) {
		    FieldDef field = clazz.fields[i];
		    if (field.name.equals(namestring)) {
			for (int j = 0; j < field.attributes.length; j++) {
			    AttrDef attr = field.attributes[j];
			    if (attr instanceof ConstantValueAttrDef) {
				Object value = ((ConstantValueAttrDef)attr).value;
				if (value instanceof StringCPValue)
				    value = ((StringCPValue)value).valueOf();
				return value.toString();
			    }
			}
		    }
		}
	    }
	}
	return null;
    }
    
    String cName;

    public String getCName() {
	if (cName == null)
	    cName = getFileName(name);
	return cName;
    }

    static final int MAC_FILE_LENGTH = 29;	/* 32 - 2 (for the ".h" or ".c") - 1 (for the count) */

    String getFileName(String src) {
	String result;
	if (prefix != null) {
	    int index = src.lastIndexOf('/');
	    String baseName = (index == -1 ? src : src.substring(index + 1));
	    result = baseName;
	}
	else 
	    result = src.replace('/', '_');
/*	if (macMode) {
	    int len = result.length;
	    if (len > MAC_FILE_LENGTH) {
		int i = 0, j = 0;
		char c;

	      loop: while (true) {
		  // at least write the first character of each word
		  hfname[j++] = src[i++]; 
		
		  if (len - i + j > MAC_FILE_LENGTH) {
		      // if there's not enough room for all the rest, strip the
		      // rest of the first package name and try again
		      int k = i;
		      while (k < MAC_FILE_LENGTH) {
			  if (src[k] == DIR_DELIM) {
			      hfname[j++] = F_DELIM;
			      i = k+1;
			      if (i < MAC_FILE_LENGTH)
				  continue loop;
			      else
				  break;
			  }
			  k++;
		      }
		  }
		  // if we get here there are no more DIR_DELIMs, or the rest
		  // fits -- so just copy the rest of whatever's left
		  while (j < MAC_FILE_LENGTH && (c = src[i++]) != 0) {
		      if (c == DIR_DELIM)
			  hfname[j++] = F_DELIM;
		      else
			  hfname[j++] = c;
		  }
		  hfname[j++] = '\0';
		  break;
	      }
	    }
	}
*/
	return result;
    }

    public void describe(PrintStream out, int depth) {
	out.print("Class: " + name);
    }

}

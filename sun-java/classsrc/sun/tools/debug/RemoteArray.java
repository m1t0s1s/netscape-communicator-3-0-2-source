/*
 * @(#)RemoteArray.java	1.7 95/09/14
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.tools.debug;

import java.io.IOException;

/**
 * The RemoteArray class allows remote debugging of arrays.
 *
 * @see RemoteValue
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteArray extends RemoteObject {
    private int size;

    RemoteArray(RemoteAgent agent, int id, RemoteClass clazz,
		       int size) {
	super(agent, TC_ARRAY, id, clazz);
	this.size = size;
    }

    /** Return the number of elements in the array. */
    public final int getSize() {
	return size;
    }

    /** Return this RemoteValue's type ("array"). */
    public String typeName() {
	return new String("array");
    }

    /** Return the element type as a string. */
    public String arrayTypeName(int type) {
 	switch (type) {
 	  case TC_BOOLEAN:
 	    return new String("boolean");
 	  case TC_BYTE:
 	    return new String("byte");
 	  case TC_CHAR:
 	    return new String("char");
 	  case TC_SHORT:
 	    return new String("short");
 	  case TC_INT:
 	    return new String("int");
 	  case TC_LONG:
 	    return new String("long");
 	  case TC_FLOAT:
 	    return new String("float");
 	  case TC_DOUBLE:
 	    return new String("double");
 	  case TC_ARRAY:
 	    return new String("array");
 	  case TP_CLASS:
 	    return new String("Class");
 	  case TP_OBJECT:
	  case TC_CLASS:
 	    return new String("Object");
 	  case TP_STRING:
 	    return new String("String");
 	  case TP_THREAD:
 	    return new String("Thread");
 	  case TC_VOID:
 	    return new String("void");
 	}
 	return new String("unknown type");
    }

    /** Return the element type as a "TC_" constant, such as "TC_CHAR". */
    public final int getElementType() throws Exception {
	String signature = getClazz().getName();
	switch (signature.charAt(1)) {
	  case 'C' :
	    return TC_CHAR;
	  case 'B' :
	    return TC_BYTE;
	  case 'L' :
	    return TC_CLASS;
	  case 'F' :
	    return TC_FLOAT;
	  case 'D' :
	    return TC_DOUBLE;
	  case 'I' :
	    return TC_INT;
	  case 'J' :
	    return TC_LONG;
	  case 'S' :
	    return TC_SHORT;
	  case 'V' :
	    return TC_VOID;
	  case 'Z' :
	    return TC_BOOLEAN;
	  default:
	    agent.error("Invalid array type: '" +
		  	new Character(signature.charAt(1)).toString() + "'");
	    return -1;
	}
    }

    /**
     * Return an array element.
     *
     * @param index	the index of the element
     * @return the element as a RemoteValue
     * @exception ArrayIndexOutOfBoundsException when the index is greater than the size of the array
     */
    public final RemoteValue getElement(int index) throws Exception {
	if (index < 0 || index >= size) {
	    throw new ArrayIndexOutOfBoundsException(index);
	}
	RemoteValue array[] = agent.getElements(id, getElementType(),
						index, index);
	return array[0];
    }

    /** Returns a copy of the array as instances of RemoteValue. */
    public final RemoteValue[] getElements() throws Exception {
	return agent.getElements(id, getElementType(), 0, size - 1);
    }

    /**
     * Returns a copy of a portion of the array as instances of RemoteValue.
     *
     * @param beginIndex	the beginning array index
     * @param endIndex	the final array index
     * @exception ArrayIndexOutOfBoundsException when the index is greater than the size of the array
     */
    public final RemoteValue[] getElements(int beginIndex, int endIndex) throws Exception {
	if (beginIndex < 0 || beginIndex >= size) {
	    throw new ArrayIndexOutOfBoundsException(beginIndex);
	}
	if (endIndex < 0 || endIndex >= size || beginIndex > endIndex) {
	    throw new ArrayIndexOutOfBoundsException(endIndex);
	}
	return agent.getElements(id, getElementType(), beginIndex, endIndex);
    }

    /** Return a description of the array. */
    public String description() {
	return toString();
    }

    /** Return a string version of the array. */
    public String toString() {
	try {
	    String s = new String();
	    if (size > 0) {
	        if (getElementType() == TC_CHAR) {
		    s = s.concat("\"");
		    RemoteValue a[] = getElements();
		    for (int i = 0; i < size; i++) {
			s = s.concat(a[i].toString());
		    }
		    s = s.concat("\"");
	        } else {
		    s = toHex(id) + " " + arrayTypeName(getElementType()) + "[" +
		        new Integer(size).toString() + "]";
		    int len = (size < 3) ? size : 3;
		    RemoteValue sub[] = getElements(0, len - 1);
		    s = s.concat(" = { ");
		    for (int i = 0; i < len; i++) {
		        s = s.concat((sub[i] == null) ? "null"
				     : sub[i].toString());
		        s = s.concat(((i < len-1) ? ", " :
				      (len < size) ? ", ... }" : " }"));
		    }
		}
	    }
   	    return s;
	} catch (Exception ex) {
	    return ("<communication error>");
	}
    }
}

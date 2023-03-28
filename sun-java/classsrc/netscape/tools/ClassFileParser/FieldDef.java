// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;

public class FieldDef extends MemberDef {
    String cName = null;

    public String getCName() {
	if (cName != null)
	    return cName;
	String clazzCName = clazz.getCName();
	cName = clazzCName + "_" + name;
	return cName;
    }
}

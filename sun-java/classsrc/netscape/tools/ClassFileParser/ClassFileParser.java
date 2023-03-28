// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.*;
import java.util.Hashtable;

public class ClassFileParser {
    // access_flags
    public static final int ACC_PUBLIC		= 0x0001;
    public static final int ACC_PRIVATE		= 0x0002;
    public static final int ACC_PROTECTED	= 0x0004;
    public static final int ACC_STATIC		= 0x0008;
    public static final int ACC_FINAL		= 0x0010;
    public static final int ACC_SYNCHRONIZED	= 0x0020;
    public static final int ACC_THREADSAFE	= 0x0040;
    public static final int ACC_TRANSIENT	= 0x0080;
    public static final int ACC_NATIVE		= 0x0100;
    public static final int ACC_INTERFACE	= 0x0200;
    public static final int ACC_ABSTRACT	= 0x0400;

    // signatures
    public static final char SIG_BOOLEAN	= 'Z';
    public static final char SIG_BYTE		= 'B';
    public static final char SIG_CHAR		= 'C';
    public static final char SIG_SHORT		= 'S';
    public static final char SIG_INT		= 'I';
    public static final char SIG_LONG		= 'J';
    public static final char SIG_FLOAT		= 'F';
    public static final char SIG_DOUBLE		= 'D';
    public static final char SIG_VOID		= 'V';
    public static final char SIG_CLASS		= 'L';
    public static final char SIG_ENDCLASS	= ';';
    public static final char SIG_ARRAY		= '[';
    public static final char SIG_DUMMY		= ']';	// happy emacs
    public static final char SIG_METHOD		= '(';
    public static final char SIG_ENDMETHOD	= ')';

    // internal class file constants
    static final int MAGIC			= 0xCAFEBABE;
    static final int MAJOR_VERSION		= 45;
    static final int MINOR_VERSION		= 3;

    // constant types
    static final int CONSTANT_Class			= 7;
    static final int CONSTANT_Fieldref			= 9;
    static final int CONSTANT_Methodref			= 10;
    static final int CONSTANT_String			= 8;
    static final int CONSTANT_Integer			= 3;
    static final int CONSTANT_Float			= 4;
    static final int CONSTANT_Long			= 5;
    static final int CONSTANT_Double			= 6;
    static final int CONSTANT_InterfaceMethodref	= 11;
    static final int CONSTANT_NameAndType		= 12;
    static final int CONSTANT_Asciz			= 1;

    ////////////////////////////////////////////////////////////////////////////
    // Global stuff:

    public static String[] classpath;
    static {
	classpath = new String[1];
	classpath[0] = ".";
    }

    public static boolean trace = false;

    static Hashtable classes = new Hashtable();

    public static ClassDef getClassDef(String classname)
		throws ClassNotFoundException, IOException {
	ClassDef c = (ClassDef)classes.get(classname);
	if (c != null) return c;
	c = loadClassDef(classname);
	classes.put(classname, c);
	return c;
    }

    static File getInputFile(String classname) throws ClassNotFoundException {
	for (int i = 0; i < classpath.length; i++) {
	    String path = classpath[i] + "/" + classname + ".class";
	    if (trace) 
		System.err.println("looking for " + path);
	    File file = new File(path);
	    if (file.exists()) return file;
	}
	throw new ClassNotFoundException(classname);
    }

    static ClassDef loadClassDef(String classname) throws IOException, ClassNotFoundException {
	File file = getInputFile(classname);
//	long startTime = System.currentTimeMillis();
	ClassDef clazz = new ClassFileParser().parse(classname, file);
//	long stopTime = System.currentTimeMillis();
//	System.err.println("Time to load " + classname + " = " + (stopTime - startTime));
	return clazz;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Instance stuff:

    ClassDef classDef;
    Object[] cpool;

    ClassFileParser() {
    }

    ClassDef parse(String classname, File file)
		throws ClassNotFoundException, IOException {
	InputStream in = new BufferedInputStream(new FileInputStream(file), 8*1024);
	LoggingInputStream s = new LoggingInputStream(in, trace);

	int magic = s.readInt("magic number");
	if (magic != MAGIC)
	    throw new ClassFormatError("The file " + classname + " isn't a Java .class file.");

	int minorVersion = s.readShort("minorVersion");
	int majorVersion = s.readShort("majorVersion");
	if (majorVersion != MAJOR_VERSION || minorVersion != MINOR_VERSION) 
	    throw new ClassFormatError("The file " + classname +
				       " has the wrong version (" +
				       majorVersion + "." + minorVersion +
				       " rather than " + 
				       MAJOR_VERSION + "." + MINOR_VERSION + ")");
	
	short constant_pool_count = s.readShort("constant_pool_count");
	cpool = new Object[constant_pool_count];

	// suck in the constant pool for future use
	for (int i = 1; i < constant_pool_count; i++) { // constant_pool[0] always unused
	    byte type = s.readByte("constant pool [" + i + "] item type");
	    switch (type) {
	      case CONSTANT_Class: {
		  short name_index = s.readShort("Class name_index");
		  cpool[i] = new ClassCPValue(name_index);
		  break;
	      }
	      case CONSTANT_Fieldref: {
		  short class_index = s.readShort("Field class_index");
		  short name_and_type_index = s.readShort("Fieldref name_and_type_index");
		  cpool[i] = new FieldCPValue(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_Methodref: {
		  short class_index = s.readShort("Method class_index");
		  short name_and_type_index = s.readShort("Methodref name_and_type_index");
		  cpool[i] = new MethodCPValue(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_InterfaceMethodref: {
		  short class_index = s.readShort("InterfaceMethod class_index");
		  short name_and_type_index = s.readShort("InterfaceMethodref name_and_type_index");
		  cpool[i] = new InterfaceMethodCPValue(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_String: {
		  short string_index = s.readShort("String string_index");
		  cpool[i] = new StringCPValue(cpool, string_index);
		  break;
	      }
	      case CONSTANT_Integer: {
		  int value = s.readInt("Integer value");
		  cpool[i] = new Integer(value);
		  break;
	      }
	      case CONSTANT_Float: {
		  float value = s.readFloat("Float value");
		  cpool[i] = new Float(value);
		  break;
	      }
	      case CONSTANT_Long: {
		  long value = s.readLong("Long value");
		  i++;	/* Argh!!!!! */
		  cpool[i] = new Long(value);
		  break;
	      }
	      case CONSTANT_Double: {
		  double value = s.readDouble("Double value");
		  i++;	/* Argh!!!!! */
		  cpool[i] = new Double(value);
		  break;
	      }
	      case CONSTANT_NameAndType: {
		  short name_index = s.readShort("NameAndType name_index");
		  short signature_index = s.readShort("NameAndType signature_index");
		  cpool[i] = new NameAndTypeCPValue(name_index, signature_index);
		  break;
	      }
	      case CONSTANT_Asciz: {
		  cpool[i] = s.readUTF("Asciz value");
		  break;
	      }
	      default:
		  throw new ClassFormatError("Unrecognized constant pool type: " + type);
	    }
	}

	short access_flags = s.readShort("access_flags");
	short this_class = s.readShort("this_class");

	classDef = new ClassDef();
	classDef.modDate = file.lastModified();

	ClassCPValue myClass = (ClassCPValue)cpool[this_class];
	classDef.name = (myClass != null) ? getName(myClass.name_index) : "<unknown>";

	short super_class = s.readShort("super_class");
	ClassCPValue mySuper = (ClassCPValue)cpool[super_class];
	classDef.superclass = (mySuper != null) ? getClassDef(getName(mySuper.name_index)) : null;

	short interfaces_count = s.readShort("interfaces_count");
	classDef.interfaces = new ClassDef[interfaces_count];
	for (int i = 0; i < interfaces_count; i++) {
	    classDef.interfaces[i] = readInterface(s);
	}

	short fields_count = s.readShort("fields_count");
	classDef.fields = new FieldDef[fields_count];
	for (int i = 0; i < fields_count; i++) {
	    classDef.fields[i] = readField(s);
	}

	short methods_count = s.readShort("methods_count");
	classDef.methods = new MethodDef[methods_count];
	for (int i = 0; i < methods_count; i++) {
	    classDef.methods[i] = readMethod(s);
	}

	short attributes_count = s.readShort("attributes_count");
	classDef.attributes = new AttrDef[attributes_count];
	for (int i = 0; i < attributes_count; i++) {
	    classDef.attributes[i] = readAttribute(s);
	}

	// success:
	return classDef;
    }

    String getName(int index) {
	Object obj = cpool[index];
	if (obj == null) return "null";
	return obj.toString();
    }

    ClassDef readInterface(LoggingInputStream s)
		throws ClassNotFoundException, IOException {
	short index = s.readShort("Interface index");
	return getClassDef(getName(((ClassCPValue)cpool[index]).name_index));
    }

    FieldDef readField(LoggingInputStream s)
		throws ClassNotFoundException, IOException {
	FieldDef fd = new FieldDef();
	fd.clazz = classDef;
	fd.flags = s.readShort("Field access_flags");
	fd.name = getName(s.readShort("Field name_index"));
	fd.signature = getName(s.readShort("Field signature_index"));

	short attributes_count = s.readShort("Field attributes_count");
	fd.attributes = new AttrDef[attributes_count];
	for (int i = 0; i < attributes_count; i++) {
	    fd.attributes[i] = readAttribute(s);
	}
	return fd;
    }

    MethodDef readMethod(LoggingInputStream s)
		throws ClassNotFoundException, IOException {
	MethodDef md = new MethodDef();
	md.clazz = classDef;
	md.flags = s.readShort("Method access_flags");
	md.name = getName(s.readShort("Method name_index"));
	md.signature = getName(s.readShort("Method signature_index"));

	short attributes_count = s.readShort("Method attributes_count");
	md.attributes = new AttrDef[attributes_count];
	for (int i = 0; i < attributes_count; i++) {
	    md.attributes[i] = readAttribute(s);
	}
	return md;
    }

    AttrDef readAttribute(LoggingInputStream s)
		throws ClassNotFoundException, IOException {
	short attribute_name_index = s.readShort("Attribute  attribute_name_index");
	int attribute_length = s.readInt("Attribute attribute_length");
	String name = (String)cpool[attribute_name_index];

	if ("SourceFile".equalsIgnoreCase(name)) {
	    SourceFileAttrDef ad = new SourceFileAttrDef();
	    short sourcefile_index = s.readShort("Attribute SourceFile sourcefile_index");
	    ad.sourceFile = getName(sourcefile_index);
	    return ad;
	}
	else if ("ConstantValue".equalsIgnoreCase(name)) {
	    ConstantValueAttrDef ad = new ConstantValueAttrDef();
	    short constantvalue_index = s.readShort("Attribute ConstantValue constantvalue_index");
	    Object cpvalue = cpool[constantvalue_index];
	    if (cpvalue instanceof CPValue)
		cpvalue = ((CPValue)cpvalue).valueOf();
	    ad.value = cpvalue;
	    return ad;
	}
	else if ("Code".equalsIgnoreCase(name)) {
	    CodeAttrDef ad = new CodeAttrDef();
	    ad.maxStack = s.readShort("Attribute Code max_stack");
	    ad.maxLocals = s.readShort("Attribute Code max_locals");

	    int code_length = s.readInt("Attribute Code code_length");
	    ad.code = new byte[code_length];
	    s.read(ad.code, "Attribute Code bytecodes");

	    short exception_table_length = s.readShort("Attribute Code exception_table_length");
	    ad.exceptions = new ExceptionAttrDef[exception_table_length];
	    for (int i = 0; i < exception_table_length; i++) {
		ExceptionAttrDef e = new ExceptionAttrDef();
		e.startPC = s.readShort("Attribute Code ExceptionTable start_pc");
		e.endPC = s.readShort("Attribute Code ExceptionTable end_pc");
		e.handlerPC = s.readShort("Attribute Code ExceptionTable handler_pc");
		e.catchType = getName(s.readShort("Attribute Code ExceptionTable catch_type"));
		ad.exceptions[i] = e;
	    }

	    short attributes_count = s.readShort("Attribute Code attributes_count");
	    ad.attributes = new AttrDef[attributes_count];
	    for (int i = 0; i < attributes_count; i++) {
		ad.attributes[i] = readAttribute(s);
	    }
	    return ad;
	}
	else if ("LineNumberTable".equalsIgnoreCase(name)) {
	    LineNumberTableAttrDef ad = new LineNumberTableAttrDef();
	    short line_number_table_length = s.readShort("Attribute LineNumberTable line_number_table_length");
	    ad.lineNumbers = new LineNumberAttrDef[line_number_table_length];
	    for (int i = 0; i < line_number_table_length; i++) {
		LineNumberAttrDef ln = new LineNumberAttrDef();
		ln.startPC = s.readShort("Attribute LineNumberTable start_pc");
		ln.lineNumber = s.readShort("Attribute LineNumberTable line_number");
		ad.lineNumbers[i] = ln;
	    }
	    return ad;
	}
	else if ("LocalVariableTable".equalsIgnoreCase(name)) {
	    LocalVariableTableAttrDef ad = new LocalVariableTableAttrDef();
	    short local_variable_table_length = s.readShort("Attribute LocalVariableNumberTable local_variable_table_length");
	    ad.localVariables = new LocalVariableAttrDef[local_variable_table_length];
	    for (int i = 0; i < local_variable_table_length; i++) {
		LocalVariableAttrDef lv = new LocalVariableAttrDef();
		lv.startPC = s.readShort("Attribute LocalVariableNumberTable start_pc");
		lv.length = s.readShort("Attribute LocalVariableNumberTable length");
		lv.name = getName(s.readShort("Attribute LocalVariableNumberTable name_index"));
		lv.signature = getName(s.readShort("Attribute LocalVariableNumberTable signature_index"));
		lv.slot = s.readShort("Attribute LocalVariableNumberTable slot");
		ad.localVariables[i] = lv;
	    }
	    return ad;
	}
	else if ("Exceptions".equalsIgnoreCase(name)) {
	    ExceptionsTableAttrDef ad = new ExceptionsTableAttrDef();
	    short number_of_exceptions = s.readShort("Attribute Exceptions number_of_exceptions");
	    ad.exceptions = new String[number_of_exceptions];
	    for (int i = 0; i < number_of_exceptions; i++) {
		short index = s.readShort("Attribute Exceptions exception_table_index[i]");
		if (index != 0) {
		    String classname = getName(((ClassCPValue)cpool[index]).name_index);
		    ad.exceptions[i] = classname;
		}
	    }
	    return ad;
	}
	else {
	    UnknownAttrDef ad = new UnknownAttrDef();
	    ad.name = getName(attribute_name_index);
	    ad.length = attribute_length;
	    s.skipBytes(attribute_length, "Unrecognized attribute");
	    return ad;
	}
    }

}

class CPValue {
    public Object valueOf() {
	return this;
    }
}

class ClassCPValue extends CPValue {
    public short name_index;
    ClassCPValue(short name_index) {
	this.name_index = name_index;
    }
}

class FieldCPValue extends CPValue {
    public short class_index;
    public short name_and_type_index;
    FieldCPValue(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class MethodCPValue extends CPValue {
    public short class_index;
    public short name_and_type_index;
    MethodCPValue(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class InterfaceMethodCPValue extends CPValue {
    public short class_index;
    public short name_and_type_index;
    InterfaceMethodCPValue(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class StringCPValue extends CPValue {
    Object[] cpool;
    public short string_index;
    StringCPValue(Object[] cpool, short string_index) {
	this.cpool = cpool;
	this.string_index = string_index;
    }
    
    public Object valueOf() {
	return cpool[string_index];
    }
}

class NameAndTypeCPValue extends CPValue {
    public short name_index;
    public short signature_index;
    NameAndTypeCPValue(short name_index, short signature_index) {
	this.name_index = name_index;
	this.signature_index = signature_index;
    }
}

class LoggingInputStream {
    public boolean trace;
    DataInputStream s;

    LoggingInputStream(InputStream s, boolean trace) {
	super();
	this.s = new DataInputStream(s);
	this.trace = trace;
    }

    public int readInt(String reason) throws IOException {
	if (trace) System.err.print("# reading int for " + reason);
	int result = s.readInt();
	if (trace) System.err.println(" => " + result);
	return result;
    }
 
    public short readShort(String reason) throws IOException {
	if (trace) System.err.print("# reading short for " + reason);
	short result = s.readShort();
	if (trace) System.err.println(" => " + result);
	return result;
    }

    public byte readByte(String reason) throws IOException {
	if (trace) System.err.print("# reading byte for " + reason);
	byte result = s.readByte();
	if (trace) System.err.println(" => " + result);
	return result;
    }

    public float readFloat(String reason) throws IOException {
	if (trace) System.err.print("# reading float for " + reason);
	float result = s.readFloat();
	if (trace) System.err.println(" => " + result);
	return result;
    }

    public long readLong(String reason) throws IOException {
	if (trace) System.err.print("# reading long for " + reason);
	long result = s.readLong();
	if (trace) System.err.println(" => " + result);
	return result;
    }

    public double readDouble(String reason) throws IOException {
	if (trace) System.err.print("# reading double for " + reason);
	double result = s.readDouble();
	if (trace) System.err.println(" => " + result);
	return result;
    }

    public void read(byte[] b, String reason) throws IOException {
	if (trace) System.err.println("# reading byte[" + b.length + "] for " + reason);
	s.readFully(b);
    }

    public void skipBytes(int amount, String reason) throws IOException {
	if (trace) System.err.println("# skipping " + amount + " bytes for " + reason);
	s.skipBytes(amount);
    }

    public String readUTF(String reason) throws IOException {
	if (trace) System.err.print("# reading utf for " + reason);
	String result = DataInputStream.readUTF(s);
	if (trace) System.err.println(" => " + result);
	return result;
    }
}

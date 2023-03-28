// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.DumpClass;

import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.DataInputStream;
import java.io.IOException;

public class DumpClass {

    public static void main(String[] argv) throws Exception {
	if (argv.length != 1) {
	    System.out.println("usage: java DumpClass <file.class>");
	    return;
	}
	new DumpClass().dump(argv[0]);
    }

    static final int MAGIC = 0xCAFEBABE;
    static final int MAJOR_VERSION = 45;
    static final int MINOR_VERSION = 3;

    // access_flags
    static final int ACC_PUBLIC		= 0x0001;
    static final int ACC_PRIVATE	= 0x0002;
    static final int ACC_PROTECTED	= 0x0004;
    static final int ACC_STATIC		= 0x0008;
    static final int ACC_FINAL		= 0x0010;
    static final int ACC_SYNCHRONIZED	= 0x0020;
    static final int ACC_THREADSAFE	= 0x0040;
    static final int ACC_TRANSIENT	= 0x0080;
    static final int ACC_NATIVE		= 0x0100;
    static final int ACC_INTERFACE	= 0x0200;
    static final int ACC_ABSTRACT	= 0x0400;

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

    Indenter out = new Indenter();
    Object[] cpool;

    public void dump(String filename) throws Exception {
	LoggingInputStream s = new LoggingInputStream(new FileInputStream(filename));

	int magic = s.readInt("magic number");
	if (magic != MAGIC)
	    throw new Exception("The file " + filename + " isn't a Java .class file.");

	int minorVersion = s.readShort("minorVersion");
	int majorVersion = s.readShort("majorVersion");
	if (majorVersion != MAJOR_VERSION || minorVersion != MINOR_VERSION) 
	    throw new Exception("The file " + filename +
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
		  cpool[i] = new ClassStruct(name_index);
		  break;
	      }
	      case CONSTANT_Fieldref: {
		  short class_index = s.readShort("Fieldref class_index");
		  short name_and_type_index = s.readShort("Fieldref name_and_type_index");
		  cpool[i] = new FieldrefStruct(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_Methodref: {
		  short class_index = s.readShort("Methodref class_index");
		  short name_and_type_index = s.readShort("Methodref name_and_type_index");
		  cpool[i] = new MethodrefStruct(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_InterfaceMethodref: {
		  short class_index = s.readShort("InterfaceMethodref class_index");
		  short name_and_type_index = s.readShort("InterfaceMethodref name_and_type_index");
		  cpool[i] = new InterfaceMethodrefStruct(class_index, name_and_type_index);
		  break;
	      }
	      case CONSTANT_String: {
		  short string_index = s.readShort("String string_index");
		  cpool[i] = new StringStruct(string_index);
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
		  cpool[i] = new NameAndTypeStruct(name_index, signature_index);
		  break;
	      }
	      case CONSTANT_Asciz: {
		  cpool[i] = s.readUTF("Asciz value");
		  break;
	      }
	      default:
		throw new Exception("Unrecognized constant pool type: " + type);
	    }
	}

	out.println("// DumpClass: " + filename);

	short access_flags = s.readShort("access_flags");
	if ((access_flags & ACC_PUBLIC) != 0)
	    out.print("public ");
	if ((access_flags & ACC_FINAL) != 0)
	    out.print("final ");
	if ((access_flags & ACC_INTERFACE) != 0)
	    out.print("interface ");
	out.nl();

	out.print("class ");
	short this_class = s.readShort("this_class");
	ClassStruct myClass = (ClassStruct)cpool[this_class];
	if (myClass != null) printName(myClass.name_index);
	else out.print("<unknown>");
	out.print(" extends ");

	short super_class = s.readShort("super_class");
	ClassStruct mySuper = (ClassStruct)cpool[super_class];
	if (mySuper != null) printName(mySuper.name_index);
	else out.print("<unknown>");

	short interfaces_count = s.readShort("interfaces_count");
	if (interfaces_count != 0) {
	    out.nl();
	    out.print("\timplements ");
	    readInterface(s);
	    for (int i = 1; i < interfaces_count; i++) {
		out.print(", ");
		readInterface(s);
	    }
	}
	out.println(" {");
	out.incr();

	short fields_count = s.readShort("fields_count");
	for (int i = 0; i < fields_count; i++) {
	    readField(s);
	}

	short methods_count = s.readShort("methods_count");
	for (int i = 0; i < methods_count; i++) {
	    readMethod(s);
	}

	short attributes_count = s.readShort("attributes_count");
	for (int i = 0; i < attributes_count; i++) {
	    readAttribute(s);
	}

	out.decr();
	out.println("}");
    }

    void readInterface(LoggingInputStream s) throws IOException {
	short index = s.readShort("Interface index");
	printName(((ClassStruct)cpool[index]).name_index);
    }

    void readField(LoggingInputStream s) throws Exception {
	short access_flags = s.readShort("Field access_flags");
	short name_index = s.readShort("Field name_index");
	short signature_index = s.readShort("Field signature_index");
	short attributes_count = s.readShort("Field attributes_count");
	
	if ((access_flags & ACC_PUBLIC) != 0)		out.print("public ");
	if ((access_flags & ACC_PRIVATE) != 0)		out.print("private ");
	if ((access_flags & ACC_PROTECTED) != 0)	out.print("protected ");
	if ((access_flags & ACC_STATIC) != 0)		out.print("static ");
	if ((access_flags & ACC_FINAL) != 0)		out.print("final ");
	if ((access_flags & ACC_THREADSAFE) != 0)	out.print("threadsafe ");
	if ((access_flags & ACC_TRANSIENT) != 0)	out.print("transient ");

	printName(name_index);
	out.print(" : ");
	printName(signature_index);

	out.incr();
	for (int i = 0; i < attributes_count; i++) {
	    readAttribute(s);
	}
	out.decr();
	out.nl();
    }

    void readMethod(LoggingInputStream s) throws Exception {
	short access_flags = s.readShort("Method access_flags");
	short name_index = s.readShort("Method name_index");
	short signature_index = s.readShort("Method signature_index");
	short attributes_count = s.readShort("Method attributes_count");

	if ((access_flags & ACC_PUBLIC) != 0)		out.print("public ");
	if ((access_flags & ACC_PRIVATE) != 0)		out.print("private ");
	if ((access_flags & ACC_PROTECTED) != 0)	out.print("protected ");
	if ((access_flags & ACC_STATIC) != 0)		out.print("static ");
	if ((access_flags & ACC_FINAL) != 0)		out.print("final ");
	if ((access_flags & ACC_SYNCHRONIZED) != 0)	out.print("synchronized ");
	if ((access_flags & ACC_NATIVE) != 0)		out.print("native ");
	if ((access_flags & ACC_ABSTRACT) != 0)		out.print("abstract ");

	printName(name_index);
	printName(signature_index);
	out.nl();

	out.incr();
	for (int i = 0; i < attributes_count; i++) {
	    readAttribute(s);
	}
	out.decr();
    }

    void readAttribute(LoggingInputStream s) throws Exception {
	short attribute_name_index = s.readShort("Attribute  attribute_name_index");
	int attribute_length = s.readInt("Attribute attribute_length");
	String name = (String)cpool[attribute_name_index];

	if ("SourceFile".equals(name)) {
	    short sourcefile_index = s.readShort("Attribute SourceFile sourcefile_index");
	    out.print("// Source file: ");
	    printName(sourcefile_index);
	    out.nl();
	}
	else if ("ConstantValue".equals(name)) {
	    short constantvalue_index = s.readShort("Attribute ConstantValue constantvalue_index");
	    out.print(" = " + cpool[constantvalue_index]);
	}
	else if ("Code".equals(name)) {
	    short max_stack = s.readShort("Attribute Code max_stack");
	    short max_locals = s.readShort("Attribute Code max_locals");
	    int code_length = s.readInt("Attribute Code code_length");
	    out.println("// Code: length: " + code_length + 
			", max_stack: " + max_stack +
			", max_locals: " + max_locals);

	    byte[] code = new byte[code_length];
	    s.read(code, "Attribute Code bytecodes");
	    printCode(code);
	    short exception_table_length = s.readShort("Attribute Code exception_table_length");

	    if (exception_table_length != 0) {
		out.println("// Exception handlers:");
		for (int i = 0; i < exception_table_length; i++) {
		    short start_pc = s.readShort("Attribute Code ExceptionTable start_pc");
		    short end_pc = s.readShort("Attribute Code ExceptionTable end_pc");
		    short handler_pc = s.readShort("Attribute Code ExceptionTable handler_pc");
		    short catch_type = s.readShort("Attribute Code ExceptionTable catch_type");
		    out.print("//   start_pc: " + start_pc +
			      ", end_pc: " + end_pc +
			      ", handler_pc: " + handler_pc +
			      ", catch_type: ");
		    printName(catch_type);
		    out.nl();
		}
	    }

	    short attributes_count = s.readShort("Attribute Code attributes_count");
	    for (int i = 0; i < attributes_count; i++) {
		readAttribute(s);
	    }
	}
	else if ("LineNumberTable".equals(name)) {
	    short line_number_table_length = s.readShort("Attribute LineNumberTable line_number_table_length");

	    if (line_number_table_length != 0) {
		out.println("// Line numbers:");
		for (int i = 0; i < line_number_table_length; i++) {
		    short start_pc = s.readShort("Attribute LineNumberTable start_pc");
		    short line_number = s.readShort("Attribute LineNumberTable line_number");
		    out.println("//   line_number: " + line_number +
				" start_pc: " + start_pc);
		}
	    }
	}
	else if ("LocalVariableTable".equals(name)) {
	    short local_variable_table_length = s.readShort("Attribute LocalVariableNumberTable local_variable_table_length");
	    if (local_variable_table_length != 0) {
		out.println("// Local variables:");
		for (int i = 0; i < local_variable_table_length; i++) {
		    short start_pc = s.readShort("Attribute LocalVariableNumberTable start_pc");
		    short length = s.readShort("Attribute LocalVariableNumberTable length");
		    short name_index = s.readShort("Attribute LocalVariableNumberTable name_index");
		    short signature_index = s.readShort("Attribute LocalVariableNumberTable signature_index");
		    short slot = s.readShort("Attribute LocalVariableNumberTable slot");
		    out.print("// ");
		    printName(name_index);
		    out.print(" : ");
		    printName(signature_index);
		    out.println("\t(start_pc: " + start_pc +
				", length: " + length +
				", slot: " + slot +
				")");
		}
	    }
	}
	else {
	    out.print("// Unrecognized attribute ");
	    printName(attribute_name_index);
	    out.println(", length: " + attribute_length);
	    s.skipBytes(attribute_length, "Unrecognized attribute");
	}
    }

    void printName(int index) {
	Object obj = cpool[index];
	if (obj == null) out.print("null");
	out.print(obj.toString());
    }

    void printCode(byte[] bytes) {
	int i = 0;
	int length = bytes.length;
	for (i = 0; i < length; i++) {
	    byte b = bytes[i];
	    out.println("//   " + i + ": " + b);
	}
/*
	int i = 0, j = 0, index = 0;
	int length = bytes.length;
	char[] hex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	while (true) {
	    out.print("//   " + index + ":\t");
	    while (j++ < 4) {
		byte b = bytes[index++];
		System.out.print(hex[b >> 16]);
		System.out.print(hex[b & 0xF]);
		if (index == length) {
		    out.nl();
		    return;
		}
	    }
	    j = 0;
	    i++;
	    out.nl();
	}
*/
    }
}

class Indenter {
    int depth = 0;
    boolean atStart = true;
    
    public void incr() {
	depth++;
    }

    public void decr() {
	depth--;
    }

    public void nl() {
	System.out.println("");
	atStart = true;
    }

    public void print(String s) {
	if (atStart) {
	    for (int i = 0; i < depth; i++)
		System.out.print("\t");
	    atStart = false;
	}
	System.out.print(s);
    }

    public void println(String s) {
	print(s);
	nl();
    }
}

class ConstantPoolStruct {
}

class ClassStruct extends ConstantPoolStruct {
    public short name_index;
    ClassStruct(short name_index) {
	this.name_index = name_index;
    }
}

class FieldrefStruct extends ConstantPoolStruct {
    public short class_index;
    public short name_and_type_index;
    FieldrefStruct(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class MethodrefStruct extends ConstantPoolStruct {
    public short class_index;
    public short name_and_type_index;
    MethodrefStruct(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class InterfaceMethodrefStruct extends ConstantPoolStruct {
    public short class_index;
    public short name_and_type_index;
    InterfaceMethodrefStruct(short class_index, short name_and_type_index) {
	this.class_index = class_index;
	this.name_and_type_index = name_and_type_index;
    }
}

class StringStruct extends ConstantPoolStruct {
    public short string_index;
    StringStruct(short string_index) {
	this.string_index = string_index;
    }
}

class NameAndTypeStruct extends ConstantPoolStruct {
    public short name_index;
    public short signature_index;
    NameAndTypeStruct(short name_index, short signature_index) {
	this.name_index = name_index;
	this.signature_index = signature_index;
    }
}

class LoggingInputStream {
    public boolean trace = false;
    DataInputStream s;

    LoggingInputStream(InputStream s) {
	super();
	this.s = new DataInputStream(s);
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

    public int read(byte[] b, String reason) throws IOException {
	if (trace) System.err.print("# reading byte[" + b.length + "] for " + reason);
	int result = s.read(b);
	if (trace) System.err.println(" => " + result);
	return result;
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

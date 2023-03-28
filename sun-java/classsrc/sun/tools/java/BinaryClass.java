/*
 * @(#)BinaryClass.java	1.20 95/09/14 Arthur van Hoff
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.java;

import java.io.IOException;
import java.io.DataInputStream;
import java.io.OutputStream;
import java.io.DataOutputStream;
import java.io.ByteArrayInputStream;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;


public final
class BinaryClass extends ClassDefinition implements Constants {
    BinaryConstantPool cpool;
    BinaryAttribute atts;
    Vector dependencies;
    
    /**
     * Constructor
     */
    public BinaryClass(Object source, ClassDeclaration declaration, int modifiers,
			   ClassDeclaration superClass, ClassDeclaration interfaces[],
		           Vector dependencies) {
	super(source, 0, declaration, modifiers, superClass, interfaces);
	this.dependencies = dependencies;
    }

    /**
     * Load a binary class
     */
    public static BinaryClass load(Environment env, DataInputStream in) throws IOException {
	return load(env, in, ~ATT_CODE);
    }
    public static BinaryClass load(Environment env, DataInputStream in, int mask) throws IOException {
	// Read the header
	int magic = in.readInt();
	if (magic != JAVA_MAGIC) {
	    throw new ClassFormatError("wrong magic: " + magic + ", expected " + JAVA_MAGIC);
	}
	int minor_version = in.readShort();
	int version = in.readShort();
	if (version != JAVA_VERSION) {
	    throw new ClassFormatError("wrong version: " + version + ", expected " + JAVA_VERSION);
	}

	// Read the constant pool
	BinaryConstantPool cpool = new BinaryConstantPool(in);

	// The dependencies of this class
	Vector dependencies = cpool.getDependencies(env);

	// Read modifiers
	int classMod = in.readShort() & MM_CLASS;

	// Read the class name
	ClassDeclaration classDecl = cpool.getDeclaration(env, in.readShort());

	// Read the super class name (may be null)
	ClassDeclaration superClassDecl = cpool.getDeclaration(env, in.readShort());

	// Read the interface names
	ClassDeclaration interfaces[] = new ClassDeclaration[in.readShort()];
	for (int i = 0 ; i < interfaces.length ; i++) {
	    interfaces[i] = cpool.getDeclaration(env, in.readShort());
	}

	// Allocate the class
	BinaryClass c = new BinaryClass(null, classDecl, classMod, superClassDecl,
					interfaces, dependencies);
	c.cpool = cpool;

	// Add any additional dependencies
	c.addDependency(superClassDecl);

	// Read the fields
	int nfields = in.readShort();
	for (int i = 0 ; i < nfields ; i++) {
	    int fieldMod = in.readShort() & MM_FIELD;

	    Identifier fieldName = cpool.getIdentifier(in.readShort());
	    Type fieldType = cpool.getType(in.readShort());
	    BinaryAttribute atts = BinaryAttribute.load(in, cpool, mask);
	    c.addField(new BinaryField(c, fieldMod, fieldType, fieldName, atts));
	}

	// Read the methods
	int nmethods = in.readShort();
	for (int i = 0 ; i < nmethods ; i++) {
	    int methMod = in.readShort() & MM_METHOD;
	    Identifier methName = cpool.getIdentifier(in.readShort());
	    Type methType = cpool.getType(in.readShort());
	    BinaryAttribute atts = BinaryAttribute.load(in, cpool, mask);
	    c.addField(new BinaryField(c, methMod, methType, methName, atts));
	}

	// Read the class attributes
	c.atts = BinaryAttribute.load(in, cpool, mask);

	// See if the SourceFile is known
	byte data[] = c.getAttribute(idSourceFile);
	if (data != null) {
	    c.source = cpool.getString(new DataInputStream(new ByteArrayInputStream(data)).readShort());
	}

	// See if the Documentation is know
	data = c.getAttribute(idDocumentation);
	if (data != null) {
	    c.documentation = new DataInputStream(new ByteArrayInputStream(data)).readUTF();
	}
	return c;
    }

    /**
     * Write the class out to a given stream.  This function mirrors the loader.
     */
    public void write(Environment env, OutputStream out) throws IOException {
	DataOutputStream data = new DataOutputStream(out);
	
	// write out the header
	data.writeInt(JAVA_MAGIC);
	data.writeShort(JAVA_MINOR_VERSION);
	data.writeShort(JAVA_VERSION);

	// Write out the constant pool
	cpool.write(data, env);

	// Write class information
	data.writeShort(getModifiers() & MM_CLASS);
	data.writeShort(cpool.indexObject(getClassDeclaration(), env));
	data.writeShort((getSuperClass() != null) 
			? cpool.indexObject(getSuperClass(), env) : 0);
	data.writeShort(interfaces.length);
	for (int i = 0 ; i < interfaces.length ; i++) {
	    data.writeShort(cpool.indexObject(interfaces[i], env));
	}

	// count the fields and the methods
	int fieldCount = 0, methodCount = 0;
	for (FieldDefinition f = firstField; f != null; f = f.getNextField()) 
	    if (f.isMethod()) methodCount++; else fieldCount++;
	
	// write out each the field count, and then each field
	data.writeShort(fieldCount);
	for (FieldDefinition f = firstField; f != null; f = f.getNextField()) {
	    if (!f.isMethod()) {
		data.writeShort(f.getModifiers() & MM_FIELD);
		String name = f.getName().toString();
		String signature = f.getType().getTypeSignature();
		data.writeShort(cpool.indexString(name, env));
		data.writeShort(cpool.indexString(signature, env));
		BinaryAttribute.write(((BinaryField)f).atts, data, cpool, env);
	    }
	}

	// write out each method count, and then each method
	data.writeShort(methodCount);
	for (FieldDefinition f = firstField; f != null; f = f.getNextField()) {
	    if (f.isMethod()) {
		data.writeShort(f.getModifiers() & MM_METHOD);
		String name = f.getName().toString();
		String signature = f.getType().getTypeSignature();
		data.writeShort(cpool.indexString(name, env));
		data.writeShort(cpool.indexString(signature, env));
		BinaryAttribute.write(((BinaryField)f).atts, data, cpool, env);
	    }
	}
	
	// write out the class attributes
	BinaryAttribute.write(atts, data, cpool, env);
	data.flush();
    }

    /**
     * Get the dependencies
     */
    public Enumeration getDependencies() {
	return dependencies.elements();
    }

    /**
     * Add a dependency
     */
    public void addDependency(ClassDeclaration c) {
	if ((c != null) && !dependencies.contains(c)) {
	    dependencies.addElement(c);
	}
    }

    /**
     * Get the constant pool
     */
    public BinaryConstantPool getConstants() {
	return cpool;
    }

    /**
     * Get a class attribute
     */
    public byte getAttribute(Identifier name)[] {
	for (BinaryAttribute att = atts ; att != null ; att = att.next) {
	    if (att.name.equals(name)) {
		return att.data;
	    }
	}
	return null;
    }
}

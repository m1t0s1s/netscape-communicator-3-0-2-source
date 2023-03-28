/*
 * @(#)Environment.java	1.34 95/11/15 Arthur van Hoff
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

import java.util.Stack;
import java.io.IOException;

/**
 * This class defines the environment for a compilation.
 * It is used to load classes, resolve class names and
 * report errors. It is an abstract class, a subclass
 * must define implementations for some of the functions.<p>
 *
 * An environment has a source object associated with it.
 * This is the thing against which errors are reported, it
 * is usually a file name, a field or a class.<p>
 *
 * Environments can be nested to change the source object.<p>
 *
 * @author 	Arthur van Hoff
 * @version 	1.34, 15 Nov 1995
 */

import java.lang.SecurityManager;

public class Environment implements Constants {
    /**
     * The actual environment to which everything is forwarded.
     */
    Environment env;
    
    /**
     * The object that is currently being parsed/compiled.
     * It is either a file name (String) or a field (FieldDefinition)
     * or a class (ClassDeclaration or ClassDefinition).
     */
    Object source;

    public Environment(Environment env, Object source) {
	this.env = ((env != null) && (env.env != null)) ? env.env : env;
	this.source = source;
    }
    public Environment() {
	this(null, null);
    }
    
    /**
     * Return a class declaration given a fully qualified class name.
     */
    public ClassDeclaration getClassDeclaration(Identifier nm) {
	return env.getClassDeclaration(nm);
    }
    
    /**
     * Return a class definition given a fully qualified class name.
     */
    public final ClassDefinition getClassDefinition(Identifier nm) throws ClassNotFound {
	return getClassDeclaration(nm).getClassDefinition(this);
    }

    /**
     * Return a class declaration given a type. Only works for
     * class types.
     */
    public final ClassDeclaration getClassDeclaration(Type t) {
	return getClassDeclaration(t.getClassName());
    }

    /**
     * Return a class definition given a type. Only works for
     * class types.
     */
    public final ClassDefinition getClassDefinition(Type t) throws ClassNotFound {
	return getClassDefinition(t.getClassName());
    }

    /**
     * Check if a class exists (without actually loading it).
     */
    public boolean classExists(Identifier nm) {
	return env.classExists(nm);
    }

    /**
     * Get the package path for a package
     */
    public Package getPackage(Identifier pkg) throws IOException {
	return env.getPackage(pkg);
    }

    /**
     * Load the definition of a class.
     */
    public void loadDefinition(ClassDeclaration c) {
	env.loadDefinition(c);
    }

    /**
     * Return the source of the environment (ie: the thing being compiled/parsed).
     */
    public final Object getSource() {
	return source;
    }

    /**
     * Resolve a type. Make sure that all the classes referred to by
     * the type have a definition.
     */
    public void resolve(int where, ClassDefinition c, Type t) throws ClassNotFound {
	switch (t.getTypeCode()) {
	  case TC_CLASS: {
	    ClassDefinition def;
	    try {
		def = getClassDefinition(t);
	    } catch (ClassNotFound e) {
		error(where, "class.not.found", e.name, "type declaration");
		break;
	    }
	    if (!c.canAccess(this, def.getClassDeclaration())) {
		error(where, "cant.access.class", def);
	    }
	    break;
	  }

	  case TC_ARRAY:
	    resolve(where, c, t.getElementType());
	    break;

	  case TC_METHOD:
	    resolve(where, c, t.getReturnType());
	    Type args[] = t.getArgumentTypes();
	    for (int i = args.length ; i-- > 0 ; ) {
		resolve(where, c, args[i]);
	    }
	    break;
	}
    }


    /**
     * Returns true if the given method is applicable to the given arguments
     */

    public boolean isApplicable(FieldDefinition m, Type args[]) throws ClassNotFound { 
	Type mType = m.getType();
	if (!mType.isType(TC_METHOD))
	    return false;
	Type mArgs[] = mType.getArgumentTypes();
	if (args.length != mArgs.length) 
	    return false;
	for (int i = args.length ; --i >= 0 ;)
	    if (!isMoreSpecific(args[i], mArgs[i]))
		return false;
	return true;
    }

    
    /** 
     * Returns true if "best" is in every argument at least as good as "other"
     */
    public boolean isMoreSpecific(FieldDefinition best, FieldDefinition other) 
	   throws ClassNotFound {	       
	Type bestType = best.getClassDeclaration().getType();
	Type otherType = other.getClassDeclaration().getType();
	boolean result = isMoreSpecific(bestType, otherType)
	              && isApplicable(other, best.getType().getArgumentTypes());
	// System.out.println("isMoreSpecific: " + best + "/" + other 
	//                      + " => " + result);
	return result;
    }

    /**
     * Returns true if "from" is a more specific type than "to"
     */

    public boolean isMoreSpecific(Type from, Type to) throws ClassNotFound {
	return implicitCast(from, to);
    }

    /**  
     * Return true if an implicit cast from this type to
     * the given type is allowed.
     */  
    public boolean implicitCast(Type from, Type to) throws ClassNotFound {
	if (from == to)
	    return true;

	int toTypeCode = to.getTypeCode();

	switch(from.getTypeCode()) {
	case TC_BYTE:
	    if (toTypeCode == TC_SHORT) 
		return true;
	case TC_SHORT:
	case TC_CHAR:
	    if (toTypeCode == TC_INT) return true;
	case TC_INT:
	    if (toTypeCode == TC_LONG) return true;
	case TC_LONG:
	    if (toTypeCode == TC_FLOAT) return true;
	case TC_FLOAT:
	    if (toTypeCode == TC_DOUBLE) return true;
	case TC_DOUBLE:
	default:
	    return false;

	case TC_NULL:
	    return to.inMask(TM_REFERENCE);

	case TC_ARRAY:
	    if (!to.isType(TC_ARRAY)) {
		return (to == Type.tObject || to == Type.tCloneable);
	    } else {
		// both are arrays.  recurse down both until one isn't an array
		do { 
		    from = from.getElementType();
		    to = to.getElementType();
		} while (from.isType(TC_ARRAY) && to.isType(TC_ARRAY));
		if (  from.inMask(TM_ARRAY|TM_CLASS) 
		      && to.inMask(TM_ARRAY|TM_CLASS)) {
		    return isMoreSpecific(from, to);
		} else { 
		    return (from.getTypeCode() == to.getTypeCode());
		}
	    } 

	case TC_CLASS:
	    if (toTypeCode == TC_CLASS) { 
		ClassDefinition fromDef = env.getClassDefinition(from);
		ClassDefinition toDef = env.getClassDefinition(to);
		return toDef.implementedBy(env, 
					   fromDef.getClassDeclaration());
	    } else {
		return false;
	    }
	}
    }


    /**
     * Return true if an explicit cast from this type to
     * the given type is allowed.
     */
    public boolean explicitCast(Type from, Type to) throws ClassNotFound {
        if (implicitCast(from, to)) {
	    return true;
	}
	if (from.inMask(TM_NUMBER)) {
	    return to.inMask(TM_NUMBER);
	}
	if (from.isType(TC_CLASS) && to.isType(TC_CLASS)) {
	    ClassDefinition fromClass = getClassDefinition(from);
	    ClassDefinition toClass = getClassDefinition(to);
	    if (toClass.isFinal()) {
	        return fromClass.implementedBy(env, 
					       toClass.getClassDeclaration());
	    }
	    if (fromClass.isFinal()) {
	        return toClass.implementedBy(env, 
					     fromClass.getClassDeclaration());
		}
	    return toClass.isInterface() ||
	           fromClass.isInterface() ||
		   fromClass.superClassOf(env, toClass.getClassDeclaration());
	}
	if (to.isType(TC_ARRAY)) { 
	    if (from.isType(TC_ARRAY))  {
		Type t1 = from.getElementType();
		Type t2 = to.getElementType();
		while ((t1.getTypeCode() == TC_ARRAY) 
		       && (t2.getTypeCode() == TC_ARRAY)) {
		    t1 = t1.getElementType();
		    t2 = t2.getElementType();
		}
		if (t1.inMask(TM_ARRAY|TM_CLASS) && 
		    t2.inMask(TM_ARRAY|TM_CLASS)) {
		    return explicitCast(t1, t2);
		}
	    } else if (from == Type.tObject || from == Type.tCloneable)
		return true;
	}
	return false;
    }

    /**
     * Flags.
     */
    public int getFlags() {
        return env.getFlags();
    }

    /**
     * Generate debugging information
     */
    public final boolean debug() {
        return (getFlags() & F_DEBUG) != 0;
    }

    /**
     * Optimize the code
     */
    public final boolean optimize() {
        return (getFlags() & F_OPTIMIZE) != 0;
    }

    /**
     * Verbose
     */
    public final boolean verbose() {
        return (getFlags() & F_VERBOSE) != 0;
    }

    /**
     * Dump debugging stuff
     */
    public final boolean dump() {
        return (getFlags() & F_DUMP) != 0;
    }

    /**
     * Verbose
     */
    public final boolean warnings() {
        return (getFlags() & F_WARNINGS) != 0;
    }

    /**
     * Dependencies
     */
    public final boolean dependencies() {
        return (getFlags() & F_DEPENDENCIES) != 0;
    }

    /**
     * Issue an error.
     *  source	 - the input source, usually a file name string
     *  offset   - the offset in the source of the error
     *  err      - the error number (as defined in this interface)
     *  arg1     - an optional argument to the error (null if not applicable)
     *  arg2     - a second optional argument to the error (null if not applicable)
     *  arg3     - a third optional argument to the error (null if not applicable)
     */
    public void error(Object source, int where, String err, Object arg1, Object arg2, Object arg3) {
	env.error(source, where, err, arg1, arg2, arg3);
    }
    public final void error(int where, String err, Object arg1, Object arg2, Object arg3) {
	error(source, where, err, arg1, arg2, arg3);
    }
    public final void error(int where, String err, Object arg1, Object arg2) {
	error(source, where, err, arg1, arg2, null);
    }
    public final void error(int where, String err, Object arg1) {
	error(source, where, err, arg1, null, null);
    }
    public final void error(int where, String err) {
	error(source, where, err, null, null, null);
    }

    /**
     * Output a string. This can either be an error message or something
     * for debugging. This should be used instead of println.
     */
    public void output(String msg) {
	env.output(msg);
    }

    private static boolean debugging = (getSystemPropertyDebug() != null);

    public static void debugOutput(Object msg) { 
        if (Environment.debugging) 
	    System.out.println(msg.toString());
    }

    private static String getSystemPropertyDebug() {
	SecurityManager.setScopePermission();
	return(System.getProperty("debug"));
    }
}


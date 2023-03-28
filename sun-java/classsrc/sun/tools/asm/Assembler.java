/*
 * @(#)Assembler.java	1.30 95/10/24 Arthur van Hoff
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

package sun.tools.asm;

import sun.tools.java.*;
import java.util.Enumeration;
import java.io.IOException;
import java.io.DataOutputStream;
import java.io.PrintStream;
import java.util.Vector;

/**
 * This class is used to assemble the bytecode instructions for a method.
 *
 * @author Arthur van Hoff
 * @version 	1.30, 24 Oct 1995
 */
public final
class Assembler implements Constants {
    static final int NOTREACHED		= 0;
    static final int REACHED		= 1;
    static final int NEEDED		= 2;

    Label first = new Label();
    Instruction last = first;
    int maxdepth;
    int maxvar;
    int maxpc;

    /**
     * Add an instruction
     */
    public void add(Instruction inst) {
	if (inst != null) {
	    last.next = inst;
	    last = inst;
	}
    }
    public void add(int where, int opc) {
	add(new Instruction(where, opc, null));
    }
    public void add(int where, int opc, Object obj) {
	add(new Instruction(where, opc, obj));
    }
    
    /**
     * Optimize instructions and mark those that can be reached
     */
    void optimize(Environment env, Label lbl) {
	lbl.pc = REACHED;
	
	for (Instruction inst = lbl.next ; inst != null ; inst = inst.next)  {
	    switch (inst.pc) {
	      case NOTREACHED:
		inst.optimize(env);
		inst.pc = REACHED;
		break;
	      case REACHED:
		return;
	      case NEEDED:
		break;
	    }

	    switch (inst.opc) {
	      case opc_label:
	      case opc_dead:
		if (inst.pc == REACHED) {
		    inst.pc = NOTREACHED;
		}
		break;

	      case opc_ifeq:
	      case opc_ifne:
	      case opc_ifgt:
	      case opc_ifge:
	      case opc_iflt:
	      case opc_ifle:
	      case opc_if_icmpeq:
	      case opc_if_icmpne:
	      case opc_if_icmpgt:
	      case opc_if_icmpge:
	      case opc_if_icmplt:
	      case opc_if_icmple:
	      case opc_if_acmpeq:
	      case opc_if_acmpne:
	      case opc_ifnull:
	      case opc_ifnonnull:
		optimize(env, (Label)inst.value);
		break;
		
	      case opc_goto:
		optimize(env, (Label)inst.value);
		return;

	      case opc_jsr:
		optimize(env, (Label)inst.value);
		break;

	      case opc_ret:
	      case opc_return:
	      case opc_ireturn:
	      case opc_lreturn:
	      case opc_freturn:
	      case opc_dreturn:
	      case opc_areturn:
	      case opc_athrow:
		return;

	      case opc_tableswitch:
	      case opc_lookupswitch: {
		SwitchData sw = (SwitchData)inst.value;
		optimize(env, sw.defaultLabel);
		for (Enumeration e = sw.tab.elements() ; e.hasMoreElements();) {
		    optimize(env, (Label)e.nextElement());
		}
		return;
	      }

	      case opc_try: {
		TryData td = (TryData)inst.value;
		td.getEndLabel().pc = NEEDED;
		for (Enumeration e = td.catches.elements() ; e.hasMoreElements();) {
		    CatchData cd = (CatchData)e.nextElement();
		    optimize(env, cd.getLabel());
		}
		break;
	      }
	    }
	}
    }

    /**
     * Eliminate instructions that are not reached
     */
    boolean eliminate() {
	boolean change = false;
	Instruction prev = first;
	
	for (Instruction inst = first.next ; inst != null ; inst = inst.next) {
	    if (inst.pc != NOTREACHED) {
		prev.next = inst;
		prev = inst;
		inst.pc = NOTREACHED;
	    } else {
		change = true;
	    }
	}
	first.pc = NOTREACHED;
	prev.next = null;
	return change;
    }

    /**
     * Optimize the byte codes
     */
    public void optimize(Environment env) {
	//listing(System.out);
	do {
	    // Figure out which instructions are reached
	    optimize(env, first);

	    // Eliminate instructions that are not reached
	} while (eliminate() && env.optimize());
    }

    /**
     * Collect all constants into the constant table
     */
    public void collect(Environment env, FieldDefinition field, ConstantPool tab) {
	// Collect constants for arguments (if a local variable table is generated)
	if ((field != null) && env.debug()) {
	    if (field.getArguments() != null) {
		for (Enumeration e = field.getArguments().elements() ; e.hasMoreElements() ;) {
		    FieldDefinition f = (FieldDefinition)e.nextElement();
		    tab.put(f.getName().toString());
		    tab.put(f.getType().getTypeSignature());
		}
	    }
	}
	
	// Collect constants from the instructions
	for (Instruction inst = first ; inst != null ; inst = inst.next) {
	    inst.collect(tab);
	}
    }

    /**
     * Determine stack size, count local variables
     */
    void balance(Label lbl, int depth) {
	for (Instruction inst = lbl ; inst != null ; inst = inst.next)  {
	    //Environment.debugOutput(inst.toString() + ": " + depth + " => " + 
	    //                                 (depth + inst.balance()));
	    depth += inst.balance();
	    if (depth < 0) {
	       throw new CompilerError("stack under flow: " + inst.toString() + " = " + depth);
	    }
	    if (depth > maxdepth) {
		maxdepth = depth;
	    }
	    switch (inst.opc) {
	      case opc_label:
		lbl = (Label)inst;
		if (inst.pc == REACHED) {
		    if (lbl.depth != depth) {
		        throw new CompilerError("stack depth error " +
						depth + "/" + lbl.depth);
		    }
		    return;
		}
		lbl.pc = REACHED;
		lbl.depth = depth;
		break;

	      case opc_ifeq:
	      case opc_ifne:
	      case opc_ifgt:
	      case opc_ifge:
	      case opc_iflt:
	      case opc_ifle:
	      case opc_if_icmpeq:
	      case opc_if_icmpne:
	      case opc_if_icmpgt:
	      case opc_if_icmpge:
	      case opc_if_icmplt:
	      case opc_if_icmple:
	      case opc_if_acmpeq:
	      case opc_if_acmpne:
	      case opc_ifnull:
	      case opc_ifnonnull:
		balance((Label)inst.value, depth);
		break;

	      case opc_goto:
		balance((Label)inst.value, depth);
		return;

	      case opc_jsr:
		balance((Label)inst.value, depth + 1);
		break;

	      case opc_ret:
	      case opc_return:
	      case opc_ireturn:
	      case opc_lreturn:
	      case opc_freturn:
	      case opc_dreturn:
	      case opc_areturn:
	      case opc_athrow:
		return;

	      case opc_iload:
	      case opc_fload:
	      case opc_aload:
	      case opc_istore:
	      case opc_fstore:
	      case opc_astore: {
		int v = ((inst.value instanceof Number) 
		            ? ((Number)inst.value).intValue() 
		            : ((LocalVariable)inst.value).slot) + 1;
		if (v > maxvar)
		    maxvar = v;
		break;
	      }

	      case opc_lload:
	      case opc_dload:
	      case opc_lstore:
	      case opc_dstore: {
		int v = ((inst.value instanceof Number) 
		            ? ((Number)inst.value).intValue() 
		            : ((LocalVariable)inst.value).slot) + 2;
		if (v  > maxvar) 
		    maxvar = v;
		break;
	      }

	      case opc_iinc: { 
		  int v = ((int[])inst.value)[0] + 1;
		  if (v  > maxvar) 
		      maxvar = v + 1;
		  break;
	      }

	      case opc_tableswitch:
	      case opc_lookupswitch: {
		SwitchData sw = (SwitchData)inst.value;
		balance(sw.defaultLabel, depth);
		for (Enumeration e = sw.tab.elements() ; e.hasMoreElements();) {
		    balance((Label)e.nextElement(), depth);
		}
		return;
	      }

	      case opc_try: {
		TryData td = (TryData)inst.value;
		for (Enumeration e = td.catches.elements() ; e.hasMoreElements();) {
		    CatchData cd = (CatchData)e.nextElement();
		    balance(cd.getLabel(), depth + 1);
		}
		break;
	      }
	    }
	}
    }

    /**
     * Generate code
     */
    public void write(Environment env, DataOutputStream out, 
		      FieldDefinition field, ConstantPool tab) 
                 throws IOException {
	//listing(System.out);

	if ((field != null) && field.getArguments() != null) { 
	      int sum = 0;
	      Vector v = field.getArguments();
	      for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
		  FieldDefinition f = ((FieldDefinition)e.nextElement());
		  sum += f.getType().stackSize();
	      } 
	      maxvar = sum;
	}

	// Make sure the stack balances.  Also calculate maxvar and maxstack
	try {
	    balance(first, 0);
	} catch (CompilerError e) {
	    System.out.println("ERROR: " + e);
	    listing(System.out);
	    throw e;
	}
	
	// Assign PCs
	int pc = 0, nexceptions = 0;
	for (Instruction inst = first ; inst != null ; inst = inst.next) {
	    inst.pc = pc;
	    pc += inst.size(tab);

	    if (inst.opc == opc_try) {
		nexceptions += ((TryData)inst.value).catches.size();
	    }
	}

	// Write header
	out.writeShort(maxdepth);
	out.writeShort(maxvar);
	out.writeInt(maxpc = pc);

	// Generate code
	for (Instruction inst = first.next ; inst != null ; inst = inst.next) {
	    inst.write(out, tab);
	}

	// write exceptions
	out.writeShort(nexceptions);
	if (nexceptions > 0) {
	    //listing(System.out);
	    writeExceptions(env, out, tab, first, last);
	}
    }

    /**
     * Write the exceptions table
     */
    void writeExceptions(Environment env, DataOutputStream out, ConstantPool tab, Instruction first, Instruction last) throws IOException {
	for (Instruction inst = first ; inst != last.next ; inst = inst.next) {
	    if (inst.opc == opc_try) {
		TryData td = (TryData)inst.value;
		writeExceptions(env, out, tab, inst.next, td.getEndLabel());
		for (Enumeration e = td.catches.elements() ; e.hasMoreElements();) {
		    CatchData cd = (CatchData)e.nextElement();
		    //System.out.println("EXCEPTION: " + env.getSource() + ", pc=" + inst.pc + ", end=" + td.getEndLabel().pc + ", hdl=" + cd.getLabel().pc + ", tp=" + cd.getType());
		    out.writeShort(inst.pc);
		    out.writeShort(td.getEndLabel().pc);
		    out.writeShort(cd.getLabel().pc);
		    if (cd.getType() != null) {
			out.writeShort(tab.index(cd.getType()));
		    } else {
			out.writeShort(0);
		    }
		}
		inst = td.getEndLabel();
	    }
	}
    }

    /**
     * Write the linenumber table
     */
    public void writeLineNumberTable(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	int ln = -1;
	int count = 0;

	for (Instruction inst = first ; inst != null ; inst = inst.next) {
	    int n = (inst.where >> OFFSETBITS);
	    if ((n > 0) && (ln != n)) {
		ln = n;
		count++;
	    }
	}

	ln = -1;
	out.writeShort(count);
	for (Instruction inst = first ; inst != null ; inst = inst.next) {
	    int n = (inst.where >> OFFSETBITS);
	    if ((n > 0) && (ln != n)) {
		ln = n;
		out.writeShort(inst.pc);
		out.writeShort(ln);
		//System.out.println("pc = " + inst.pc + ", ln = " + ln);
	    }
	}
    }
    
    /**
     * Figure out when registers contain a legal value. This is done
     * using a simple data flow algorithm. This information is later used
     * to generate the local variable table.
     */
    void flowFields(Environment env, Label lbl, FieldDefinition locals[]) {
	if (lbl.locals != null) {
	    // Been here before. Erase any conflicts.
	    FieldDefinition f[] = lbl.locals;
	    for (int i = 0 ; i < maxvar ; i++) {
		if (f[i] != locals[i]) {
		    f[i] = null;
		}
	    }
	    return;
	}

	// Remember the set of active registers at this point
	lbl.locals = new FieldDefinition[maxvar];
	System.arraycopy(locals, 0, lbl.locals, 0, maxvar);

	FieldDefinition newlocals[] = new FieldDefinition[maxvar];
	System.arraycopy(locals, 0, newlocals, 0, maxvar);
	locals = newlocals;
	
	for (Instruction inst = lbl.next ; inst != null ; inst = inst.next)  {
	    switch (inst.opc) {
	      case opc_istore:   case opc_istore_0: case opc_istore_1:
	      case opc_istore_2: case opc_istore_3:
	      case opc_fstore:   case opc_fstore_0: case opc_fstore_1:
	      case opc_fstore_2: case opc_fstore_3:
	      case opc_astore:   case opc_astore_0: case opc_astore_1:
	      case opc_astore_2: case opc_astore_3:
	      case opc_lstore:   case opc_lstore_0: case opc_lstore_1:
	      case opc_lstore_2: case opc_lstore_3:
	      case opc_dstore:   case opc_dstore_0: case opc_dstore_1:
	      case opc_dstore_2: case opc_dstore_3:
		if (inst.value instanceof LocalVariable) {
		    LocalVariable v = (LocalVariable)inst.value;
		    locals[v.slot] = v.field;
		}
		break;

	      case opc_label:
		flowFields(env, (Label)inst, locals);
		return;

	      case opc_ifeq: case opc_ifne: case opc_ifgt:
	      case opc_ifge: case opc_iflt: case opc_ifle:
	      case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpgt:
	      case opc_if_icmpge: case opc_if_icmplt: case opc_if_icmple:
	      case opc_if_acmpeq: case opc_if_acmpne: 
	      case opc_ifnull: case opc_ifnonnull:
	      case opc_jsr:
		flowFields(env, (Label)inst.value, locals);
		break;
		
	      case opc_goto:
		flowFields(env, (Label)inst.value, locals);
		return;

	      case opc_return:   case opc_ireturn:  case opc_lreturn:
	      case opc_freturn:  case opc_dreturn:  case opc_areturn:
	      case opc_athrow:   case opc_ret:
		return;

	      case opc_tableswitch:
	      case opc_lookupswitch: {
		SwitchData sw = (SwitchData)inst.value;
		flowFields(env, sw.defaultLabel, locals);
		for (Enumeration e = sw.tab.elements() ; e.hasMoreElements();) {
		    flowFields(env, (Label)e.nextElement(), locals);
		}
		return;
	      }

	      case opc_try: {
		Vector catches = ((TryData)inst.value).catches;
		for (Enumeration e = catches.elements(); e.hasMoreElements();) {
		    CatchData cd = (CatchData)e.nextElement();
		    flowFields(env, cd.getLabel(), locals);
		}
		break;
	      }
	    }
	}
    }

    /**
     * Write the local variable table. The necessary constants have already been
     * added to the constant table by the collect() method. The flowFields method
     * is used to determine which variables are alive at each pc.
     */
    public void writeLocalVariableTable(Environment env, FieldDefinition field, DataOutputStream out, ConstantPool tab) throws IOException {
	FieldDefinition locals[] = new FieldDefinition[maxvar];
	int i = 0;

	// Initialize arguments
	if ((field != null) && (field.getArguments() != null)) {
	    int reg = 0;
	    Vector v = field.getArguments();
	    for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
	        FieldDefinition f = ((FieldDefinition)e.nextElement());
		locals[reg] = f;
		reg += f.getType().stackSize();
	    } 
	}

	flowFields(env, first, locals);
	LocalVariableTable lvtab = new LocalVariableTable();

	// Initialize arguments again
	for (i = 0; i < maxvar; i++) 
	    locals[i] = null;
	if ((field != null) && (field.getArguments() != null)) {
	    int reg = 0;
	    Vector v = field.getArguments();
	    for (Enumeration e = v.elements(); e.hasMoreElements(); ) {
	        FieldDefinition f = ((FieldDefinition)e.nextElement());
		locals[reg] = f;
		lvtab.define(f, reg, 0, maxpc);
		reg += f.getType().stackSize();
	    } 
	}

	int pcs[] = new int[maxvar];

	for (Instruction inst = first ; inst != null ; inst = inst.next)  {
	    switch (inst.opc) {
	      case opc_istore:   case opc_istore_0: case opc_istore_1:
	      case opc_istore_2: case opc_istore_3: case opc_fstore:
	      case opc_fstore_0: case opc_fstore_1: case opc_fstore_2:
	      case opc_fstore_3:
	      case opc_astore:   case opc_astore_0: case opc_astore_1:
	      case opc_astore_2: case opc_astore_3:
	      case opc_lstore:   case opc_lstore_0: case opc_lstore_1:
	      case opc_lstore_2: case opc_lstore_3:
	      case opc_dstore:   case opc_dstore_0: case opc_dstore_1:
	      case opc_dstore_2: case opc_dstore_3:
		if (inst.value instanceof LocalVariable) {
		    LocalVariable v = (LocalVariable)inst.value;
		    int pc = (inst.next != null) ? inst.next.pc : inst.pc;
		    if (locals[v.slot] != null) {
			lvtab.define(locals[v.slot], v.slot, pcs[v.slot], pc);
		    }
		    pcs[v.slot] = pc;
		    locals[v.slot] = v.field;
		}
		break;

	      case opc_label: {
		// flush  previous labels
		for (i = 0 ; i < maxvar ; i++) {
		    if (locals[i] != null) {
			lvtab.define(locals[i], i, pcs[i], inst.pc);
		    }
		}
		// init new labels
		int pc = inst.pc;
		FieldDefinition[] labelLocals = ((Label)inst).locals;
		if (labelLocals == null) { // unreachable code??
		    for (i = 0; i < maxvar; i++) 
			locals[i] = null;
		} else { 
		    System.arraycopy(labelLocals, 0, locals, 0, maxvar);
		}
		for (i = 0 ; i < maxvar ; i++) {
		    pcs[i] = pc;
		}
		break;
	      }
	    }
	}

	// flush  remaining labels
	for (i = 0 ; i < maxvar ; i++) {
	    if (locals[i] != null) {
		lvtab.define(locals[i], i, pcs[i], maxpc);
	    }
	}

	// write the local variable table
	lvtab.write(env, out, tab);
    }

    /**
     * Return true if empty
     */
    public boolean empty() {
	return first == last;
    }

    /**
     * Print the byte codes
     */
    public void listing(PrintStream out) {
	out.println("-- listing --");
	for (Instruction inst = first ; inst != null ; inst = inst.next) {
	    out.println(inst.toString());
	}
    }
}


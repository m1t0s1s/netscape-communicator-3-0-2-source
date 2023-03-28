 /*
 * @(#)Parser.java	1.53 95/11/08 Arthur van Hoff
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

import sun.tools.tree.*;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Vector;

/**
 * This class is used to parse Java statements and expressions.
 * The result is a parse tree.<p>
 *
 * This class implements an operator precedence parser. Errors are
 * reported to the Environment object, if the error can't be
 * resolved immediatly, a SyntaxError exception is thrown.<p>
 *
 * Error recovery is implemented by catching SyntaxError exceptions
 * and discarding input tokens until an input token is reached that
 * is possibly a legal continuation.<p>
 *
 * The parse tree that is constructed represents the input
 * exactly (no rewrites to simpler forms). This is important
 * if the resulting tree is to be used for code formatting in
 * a programming environment. Currently only documentation comments
 * are retained.<p>
 *
 * The parsing algorithm does NOT use any type information. Changes
 * in the type system do not affect the structure of the parse tree.
 * This restriction does introduce an ambiguity an expression of the
 * form: (e1) e2 is assumed to be a cast if e2 does not start with
 * an operator. That means that (a) - b is interpreted as subtract
 * b from a and not cast negative b to type a. However, if a is a
 * simple type (byte, int, ...) then it is assumed to be a cast.<p>
 *
 * @author 	Arthur van Hoff
 * @version 	1.53, 08 Nov 1995
 */

public abstract class Parser extends Scanner implements Constants {
    /**
     * Create a parser
     */
    protected Parser(Environment env, InputStream in) throws IOException {
	super(env, in);
    }

    /**
     * Return the type name associated with a type identifier
     */
    protected abstract Identifier resolveClass(Identifier id);

    /**
     * package declaration
     */
    protected abstract void packageDeclaration(int off, Identifier nm);

    /**
     * import class
     */
    protected abstract void importClass(int off, Identifier nm);

    /**
     * import package
     */
    protected abstract void importPackage(int off, Identifier nm);

    /**
     * Define class
     */
    protected abstract void beginClass(int off, String doc, int mod, Identifier nm,
				       Identifier sup, Identifier impl[]);

    /**
     * End class
     */
    protected abstract void endClass(int off, Identifier nm);

    /**
     * Define a field
     */
    protected abstract void defineField(int where, String doc, int mod, Type t,
				        Identifier nm, Identifier args[], 
					Identifier exp[], Node val);

    /*
     * A growable array of nodes. It is used as a growable
     * buffer to hold argument lists and expression lists.
     * I'm not using Vector to make it more efficient.
     */
    private Node args[] = new Node[32];
    private int argIndex = 0;

    protected final void addArgument(Node n) {
	if (argIndex == args.length) {
	    Node newArgs[] = new Node[args.length * 2];
	    System.arraycopy(args, 0, newArgs, 0, args.length);
	    args = newArgs;
	}
	args[argIndex++] = n;
    }
    protected final Expression exprArgs(int index)[] {
	Expression e[] = new Expression[argIndex - index];
	System.arraycopy(args, index, e, 0, argIndex - index);
	argIndex = index;
	return e;
    }
    protected final Statement statArgs(int index)[] {
	Statement s[] = new Statement[argIndex - index];
	System.arraycopy(args, index, s, 0, argIndex - index);
	argIndex = index;
	return s;
    }

    /**
     * Expect a token, return its value, scan the next token or
     * throw an exception.
     */
    protected final void expect(int t) throws SyntaxError, IOException {
	if (token != t) {
	    switch (t) {
	      case IDENT:
		env.error(prevPos, "identifier.expected");
		break;
	      default:
		env.error(prevPos, "token.expected", opNames[t]);
		break;
	    }
		throw new SyntaxError();
	}
	scan();
    }

    /**
     * Parse a type expression. Does not parse the []'s.
     */
    protected Expression parseTypeExpression() throws SyntaxError, IOException {
	switch (token) {
	  case VOID:
	    return new TypeExpression(scan(), Type.tVoid);
	  case BOOLEAN:
	    return new TypeExpression(scan(), Type.tBoolean);
	  case BYTE:
	    return new TypeExpression(scan(), Type.tByte);
	  case CHAR:
	    return new TypeExpression(scan(), Type.tChar);
	  case SHORT:
	    return new TypeExpression(scan(), Type.tShort);
	  case INT:
	    return new TypeExpression(scan(), Type.tInt);
	  case LONG:
	    return new TypeExpression(scan(), Type.tLong);
	  case FLOAT:
	    return new TypeExpression(scan(), Type.tFloat);
	  case DOUBLE:
	    return new TypeExpression(scan(), Type.tDouble);
	  case IDENT:
	    Expression e = new IdentifierExpression(pos, idValue);
	    scan();
	    while (token == FIELD) {
		e = new FieldExpression(scan(), e, idValue);
		expect(IDENT);
	    }
	    return e;
	}
	env.error(pos, "type.expected");
	throw new SyntaxError();
    }

    /**
     * Parse a method invocation. Should be called when the current
     * then is the '(' of the argument list.
     */
    protected Expression parseMethodExpression(Expression e, Identifier id) throws SyntaxError, IOException {
       int p = scan();
       int i = argIndex;
       if (token != RPAREN) {
	   addArgument(parseExpression());
	   while (token == COMMA) {
	       scan();
	       addArgument(parseExpression());
	   }
       }
       expect(RPAREN);
       return new MethodExpression(p, e, id, exprArgs(i));
    }

    /**
     * Parse a primary expression.
     */
    protected Expression parseTerm() throws SyntaxError, IOException {
	switch (token) {
	  case CHARVAL: {
	    char v = charValue;
	    return new CharExpression(scan(), v);
	  }
	  case INTVAL: {
	    int v = intValue;
	    int q = scan();
	    if (v < 0 && radix == 10) env.error(q, "overflow");
	    return new IntExpression(q, v);
	  }
	  case LONGVAL: {
	    long v = longValue;
	    int q = scan();
	    if (v < 0 && radix == 10) env.error(q, "overflow");
	    return new LongExpression(q, v);
	  }
	  case FLOATVAL: {
	    float v = floatValue;
	    return new FloatExpression(scan(), v);
	  }
	  case DOUBLEVAL: {
	    double v = doubleValue;
	    return new DoubleExpression(scan(), v);
	  }
	  case STRINGVAL: {
	    String v = stringValue;
	    return new StringExpression(scan(), v);
	  }
	  case IDENT: {
	    Identifier v = idValue;
	    int p = scan();
	    return (token == LPAREN) ? 
			parseMethodExpression(null, v) : new IdentifierExpression(p, v);
	  }

	  case TRUE:
	    return new BooleanExpression(scan(), true);
	  case FALSE:
	    return new BooleanExpression(scan(), false);
	  case NULL:
	    return new NullExpression(scan());

	  case THIS: {
	    Expression e = new ThisExpression(scan());
	    return (token == LPAREN) ? parseMethodExpression(e, idInit) : e;
	  }
	  case SUPER: {
	    Expression e = new SuperExpression(scan());
	    return (token == LPAREN) ? parseMethodExpression(e, idInit) : e;
	  }

	  case VOID:
	  case BOOLEAN:
	  case BYTE:
	  case CHAR:
	  case SHORT:
	  case INT:
	  case LONG:
	  case FLOAT:
	  case DOUBLE:
	    return parseTypeExpression();

	  case ADD: {
	    int p = scan();
	    switch (token) {
	      case INTVAL: {
		int v = intValue;
		int q = scan();
		if (v < 0 && radix == 10) env.error(q, "overflow");
		return new IntExpression(q, v);
	      }
	      case LONGVAL: {
		long v = longValue;
		int q = scan();
		if (v < 0 && radix == 10) env.error(q, "overflow");
		return new LongExpression(q, v);
	      }
	      case FLOATVAL: {
		float v = floatValue;
		return new FloatExpression(scan(), v);
	      }
	      case DOUBLEVAL: {
		double v = doubleValue;
		return new DoubleExpression(scan(), v);
	      }
	    }
	    return new PositiveExpression(p, parseTerm());
	  }
	  case SUB: {
	    int p = scan();
	    switch (token) {
	      case INTVAL: {
		int v = -intValue;
		return new IntExpression(scan(), v);
	      }
	      case LONGVAL: {
		long v = -longValue;
		return new LongExpression(scan(), v);
	      }
	      case FLOATVAL: {
		float v = -floatValue;
		return new FloatExpression(scan(), v);
	      }
	      case DOUBLEVAL: {
		double v = -doubleValue;
		return new DoubleExpression(scan(), v);
	      }
	    }
	    return new NegativeExpression(p, parseTerm());
	  }
	  case NOT:
	    return new NotExpression(scan(), parseTerm());
	  case BITNOT:
	    return new BitNotExpression(scan(), parseTerm());
	  case INC:
	    return new PreIncExpression(scan(), parseTerm());
	  case DEC:
	    return new PreDecExpression(scan(), parseTerm());
	
	  case LPAREN: {
	    // bracketed-expr: (expr)
	    int p = scan();
	    Expression e = parseExpression();
	    expect(RPAREN);

	    if (e.getOp() == TYPE) {
		// cast-expr: (simple-type) expr
		return new CastExpression(p, e, parseTerm());
	    }

	    switch (token) {
	      case LPAREN:
	      case CHARVAL:
	      case INTVAL:
	      case LONGVAL:
	      case FLOATVAL:
	      case DOUBLEVAL:
	      case STRINGVAL:
	      case IDENT:
	      case TRUE:
	      case FALSE:
	      case NOT:
	      case BITNOT:
	      case INC:
	      case DEC:
	      case THIS:
	      case SUPER:
	      case NULL:
	      case NEW:
		// cast-expr: (expr) expr
		return new CastExpression(p, e, parseTerm());
	    }
	    return new ExprExpression(p, e);
	  }

	  case LBRACE: {
	    // array initializer: {expr1, expr2, ... exprn}
	    int p = scan();
	    int i = argIndex;
	    if (token != RBRACE) {
		addArgument(parseExpression());
		while (token == COMMA) {
		    scan();
		    if (token == RBRACE) {
			break;
		    }
		    addArgument(parseExpression());
		}
	    }
	    expect(RBRACE);
	    return new ArrayExpression(p, exprArgs(i));
	  }

	  case NEW: {
	    int p = scan();
	    int i = argIndex;

	    if (token == LPAREN) {
		scan();
		Expression e = parseExpression();
		expect(RPAREN);
		env.error(p, "not.supported", "new(...)");
		return new NullExpression(p);
	    }

	    Expression e = parseTypeExpression();

	    if (token == LSQBRACKET) {
		while (token == LSQBRACKET) {
		    scan();
		    addArgument((token != RSQBRACKET) ? parseExpression() : null);
		    expect(RSQBRACKET);
		}
		return new NewArrayExpression(p, e, exprArgs(i));
	    } else {
		expect(LPAREN);
		if (token != RPAREN) {
		    addArgument(parseExpression());
		    while (token == COMMA) {
			scan();
			addArgument(parseExpression());
		    }
		}
		expect(RPAREN);
		return new NewInstanceExpression(p, e, exprArgs(i));
	    }
	  }
	}
	
	// System.err.println("NEAR: " + opNames[token]);
	env.error(prevPos, "missing.term");
	return new IntExpression(pos, 0);
    }

    /**
     * Parse an expression.
     */
    protected Expression parseExpression() throws SyntaxError, IOException {
	for (Expression e = parseTerm() ; e != null ; e = e.order()) {
	    switch (token) {
	      case LSQBRACKET: {
		// index: expr1[expr2]
		int p = scan();
		Expression index = (token != RSQBRACKET) ? parseExpression() : null;
		expect(RSQBRACKET);
		e = new ArrayAccessExpression(p, e, index);
		break;
	      }

              case INC: 
                e = new PostIncExpression(scan(), e);
                break;
              case DEC:
                e = new PostDecExpression(scan(), e);
                break;
	      case FIELD: {
		int p = scan();
		Identifier id = idValue;
		expect(IDENT);
		if (token == LPAREN) {
		    e = parseMethodExpression(e, id);
		} else {
		    e = new FieldExpression(p, e, id);
		}
		break;
	      }
	      case INSTANCEOF:
		e = new InstanceOfExpression(scan(), e, parseTerm());
		break;
	      case ADD:
		e = new AddExpression(scan(), e, parseTerm());
		break;
	      case SUB:
		e = new SubtractExpression(scan(), e, parseTerm());
		break;
	      case MUL:
		e = new MultiplyExpression(scan(), e, parseTerm());
		break;
	      case DIV:
		e = new DivideExpression(scan(), e, parseTerm());
		break;
	      case REM:
		e = new RemainderExpression(scan(), e, parseTerm());
		break;
	      case LSHIFT:
		e = new ShiftLeftExpression(scan(), e, parseTerm());
		break;
	      case RSHIFT:
		e = new ShiftRightExpression(scan(), e, parseTerm());
		break;
	      case URSHIFT:
		e = new UnsignedShiftRightExpression(scan(), e, parseTerm());
		break;
	      case LT:
		e = new LessExpression(scan(), e, parseTerm());
		break;
	      case LE:
		e = new LessOrEqualExpression(scan(), e, parseTerm());
		break;
	      case GT:
		e = new GreaterExpression(scan(), e, parseTerm());
		break;
	      case GE:
		e = new GreaterOrEqualExpression(scan(), e, parseTerm());
		break;
	      case EQ:
		e = new EqualExpression(scan(), e, parseTerm());
		break;
	      case NE:
		e = new NotEqualExpression(scan(), e, parseTerm());
		break;
	      case BITAND:
		e = new BitAndExpression(scan(), e, parseTerm());
		break;
	      case BITXOR:
		e = new BitXorExpression(scan(), e, parseTerm());
		break;
	      case BITOR:
		e = new BitOrExpression(scan(), e, parseTerm());
		break;
	      case AND:
		e = new AndExpression(scan(), e, parseTerm());
		break;
	      case OR:
		e = new OrExpression(scan(), e, parseTerm());
		break;
	      case ASSIGN:
		e = new AssignExpression(scan(), e, parseTerm());
		break;
	      case ASGMUL:
		e = new AssignMultiplyExpression(scan(), e, parseTerm());
		break;
	      case ASGDIV:
		e = new AssignDivideExpression(scan(), e, parseTerm());
		break;
	      case ASGREM:
		e = new AssignRemainderExpression(scan(), e, parseTerm());
		break;
	      case ASGADD:
		e = new AssignAddExpression(scan(), e, parseTerm());
		break;
	      case ASGSUB:
		e = new AssignSubtractExpression(scan(), e, parseTerm());
		break;
	      case ASGLSHIFT:
		e = new AssignShiftLeftExpression(scan(), e, parseTerm());
		break;
	      case ASGRSHIFT:
		e = new AssignShiftRightExpression(scan(), e, parseTerm());
		break;
	      case ASGURSHIFT:
		e = new AssignUnsignedShiftRightExpression(scan(), e, parseTerm());
		break;
	      case ASGBITAND:
		e = new AssignBitAndExpression(scan(), e, parseTerm());
		break;
	      case ASGBITOR:
		e = new AssignBitOrExpression(scan(), e, parseTerm());
		break;
	      case ASGBITXOR:
		e = new AssignBitXorExpression(scan(), e, parseTerm());
		break;
	      case QUESTIONMARK: {
		int p = scan();
		Expression t = parseExpression();
		expect(COLON);
		e = new ConditionalExpression(p, e, t, parseExpression());
		break;
	      }

	      default:
		return e;
	    }
	}
	// this return is bogus
	return null;
    }

    /**
     * Recover after a syntax error in a statement. This involves
     * discarding tokens until EOF or a possible continuation is
     * encountered.
     */
    protected boolean recoverStatement() throws SyntaxError, IOException {
	while (true) {
	    switch (token) {
	      case EOF:
	      case RBRACE:
	      case LBRACE:
	      case IF:
	      case FOR:
	      case WHILE:
	      case DO:
	      case TRY:
	      case CATCH:
	      case FINALLY:
	      case BREAK:
	      case CONTINUE:
	      case RETURN:
		// begin of a statement, return
		return true;

	      case VOID:
	      case STATIC:
	      case PUBLIC:
	      case PRIVATE:
	      case SYNCHRONIZED:
	      case INTERFACE:
	      case CLASS:
	      case TRANSIENT:
		// begin of something outside a statement, panic some more
		expect(RBRACE);
		return false;

	      case LPAREN:
		match(LPAREN, RPAREN);
		scan();
		break;

	      case LSQBRACKET:
	        match(LSQBRACKET, RSQBRACKET);
	        scan();
	        break;

	      default:
	        // don't know what to do, skip
	        scan();
		break;
	    }
	}
    }

    /**
     * Parse declaration, called after the type expression
     * has been parsed and the current token is IDENT.
     */
    Statement parseDeclaration(int p, int mod, Expression type) throws SyntaxError, IOException {
	int i = argIndex;
	if (token == IDENT) {
	    addArgument(new VarDeclarationStatement(pos, parseExpression()));
	    while (token == COMMA) {
		scan();
		addArgument(new VarDeclarationStatement(pos, parseExpression()));
	    }
	}
	return new DeclarationStatement(p, mod, type, statArgs(i));
    }

    /**
     * Check if an expression is a legal toplevel expression.
     * Only method, inc, dec, and new expression are allowed.
     */
    void topLevelExpression(Expression e) {
	switch (e.getOp()) {
	  case ASSIGN:
	  case ASGMUL:
	  case ASGDIV:
	  case ASGREM:
	  case ASGADD:
	  case ASGSUB:
	  case ASGLSHIFT:
	  case ASGRSHIFT:
	  case ASGURSHIFT:
	  case ASGBITAND:
	  case ASGBITOR:
	  case ASGBITXOR:
	  case PREINC:
	  case PREDEC:
	  case POSTINC:
	  case POSTDEC:
	  case METHOD:
	  case NEWINSTANCE:
	    return;
	}
	env.error(e.getWhere(), "invalid.expr");
    }

    /**
     * Parse a statement.
     */
    protected Statement parseStatement() throws SyntaxError, IOException {
	switch (token) {
	  case SEMICOLON:
	    return new CompoundStatement(scan(), new Statement[0]);
	    
	  case LBRACE: {
	    // compound statement: { stat1 stat2 ... statn }
	    int p = scan();
	    int i = argIndex;
	    while ((token != EOF) && (token != RBRACE)) {
		int j = argIndex;
		try {
		    addArgument(parseStatement());
		} catch (SyntaxError e) {
		    argIndex = j;
		    if (!recoverStatement()) {
			throw e;
		    }
		}
	    }

	    expect(RBRACE);
	    return new CompoundStatement(p, statArgs(i));
	  }

	  case IF: {
	    // if-statement: if (expr) stat
	    // if-statement: if (expr) stat else stat
	    int p = scan();
	      
	    expect(LPAREN);
	    Expression c = parseExpression();
	    expect(RPAREN);
	    Statement t = parseStatement();
	    if (token == ELSE) {
	 	scan();
		return new IfStatement(p, c, t, parseStatement());
	    } else {
		return new IfStatement(p, c, t, null);
	    }
	  }

	  case ELSE: {
	    // else-statement: else stat
	    env.error(scan(), "else.without.if");
	    return parseStatement();
	  }

	  case FOR: {
	    // for-statement: for (decl-expr? ; expr? ; expr?) stat
	    int p = scan();
	    Statement init = null;
	    Expression cond = null, inc = null;
	      
	    expect(LPAREN);
	    if (token != SEMICOLON) {
		int p2 = pos;
		Expression e = parseExpression();

		if (token == IDENT) {
		    init = parseDeclaration(p2, 0, e);
		} else {
		    topLevelExpression(e);
		    while (token == COMMA) {
			int p3 = scan();
			Expression e2 = parseExpression();
			topLevelExpression(e2);
			e = new CommaExpression(p3, e, e2);
		    }
		    init = new ExpressionStatement(p2, e);
		}
	    }
	    expect(SEMICOLON);
	    if (token != SEMICOLON) {
		cond = parseExpression();
	    }
	    expect(SEMICOLON);
	    if (token != RPAREN) {
		inc = parseExpression();
		topLevelExpression(inc);
		while (token == COMMA) {
		    int p2 = scan();
		    Expression e2 = parseExpression();
		    topLevelExpression(e2);
		    inc = new CommaExpression(p2, inc, e2);
		}
	    }
	    expect(RPAREN);
	    return new ForStatement(p, init, cond, inc, parseStatement());
	  }

	  case WHILE: {
	    // while-statement: while (expr) stat
	    int p = scan();
	      
	    expect(LPAREN);
	    Expression cond = parseExpression();
	    expect(RPAREN);
	    return new WhileStatement(p, cond, parseStatement());
	  }

	  case DO: {
	    // do-statement: do stat while (expr)
	    int p = scan();
	      
	    Statement body = parseStatement();
	    expect(WHILE);
	    expect(LPAREN);
	    Expression cond = parseExpression();
	    expect(RPAREN);
	    expect(SEMICOLON);
	    return new DoStatement(p, body, cond);
	  }

	  case BREAK: {
	    // break-statement: break ;
	    int p = scan();
	    Identifier label = null;

	    if (token == IDENT) {
		label = idValue;
		scan();
	    } 
	    expect(SEMICOLON);
	    return new BreakStatement(p, label);
	  }

	  case CONTINUE: {
	    // continue-statement: continue ;
	    int p = scan();
	    Identifier label = null;

	    if (token == IDENT) {
		label = idValue;
		scan();
	    }
	    expect(SEMICOLON);
	    return new ContinueStatement(p, label);
	  }

	  case RETURN: {
	    // return-statement: return ;
	    // return-statement: return expr ;
	    int p = scan();
	    Expression e = null;

	    if (token != SEMICOLON) {
		e = parseExpression();
	    }
	    expect(SEMICOLON);
	    return new ReturnStatement(p, e);
	  }

	  case SWITCH: {
	    // switch statement: switch ( expr ) stat
	    int p = scan();
	    int i = argIndex;

	    expect(LPAREN);
	    Expression e = parseExpression();
	    expect(RPAREN);
	    expect(LBRACE);
	    
	    while ((token != EOF) && (token != RBRACE)) {
		int j = argIndex;
		try {
		    switch (token) {
		      case CASE:
			// case-statement: case expr:
			addArgument(new CaseStatement(scan(), parseExpression()));
			expect(COLON);
			break;

		      case DEFAULT:
			// default-statement: default:
			addArgument(new CaseStatement(scan(), null));
			expect(COLON);
			break;
			
		      default:
			addArgument(parseStatement());
			break;
		    }
		} catch (SyntaxError ee) {
		    argIndex = j;
		    if (!recoverStatement()) {
			throw ee;
		    }
		}
	    }
	    expect(RBRACE);
	    return new SwitchStatement(p, e, statArgs(i));
	  }

	  case CASE: {
	    // case-statement: case expr : stat
	    env.error(pos, "case.without.switch");
	    while (token == CASE) {
		scan();
		parseExpression();
		expect(COLON);
	    }
	    return parseStatement();
	  }

	  case DEFAULT: {
	    // default-statement: default : stat
	    env.error(pos, "default.without.switch");
	    scan();
	    expect(COLON);
	    return parseStatement();
	  }

	  case TRY: {
	    // try-statement: try stat catch (type-expr ident) stat finally stat
	    int p = scan();
	    int i = argIndex;
	    boolean catches = false;
	    Statement s = parseStatement();

	    while (token == CATCH) {
		int pp = pos;
		expect(CATCH);
		expect(LPAREN);
		Expression t = parseExpression();
		Identifier id = (token == IDENT) ? idValue : null;
		expect(IDENT);
		// We only catch Throwable's, so this is no longer required
		// while (token == LSQBRACKET) {
		//    t = new ArrayAccessExpression(scan(), t, null);
		//    expect(RSQBRACKET);
		// }
		expect(RPAREN);
		addArgument(new CatchStatement(pp, t, id, parseStatement()));
		catches = true;
	    } 
	    
	    if (catches) 
	        s = new TryStatement(p, s, statArgs(i));
	    
	    if (token == FINALLY) {
		scan();
		return new FinallyStatement(p, s, parseStatement());
	    } else if (catches) { 
	        return s;
	    } else {
	        env.error(pos, "try.without.catch.finally");
	        return new TryStatement(p, s, null);
	    }
	  }
	    
	  case CATCH: {
	    // catch-statement: catch (expr ident) stat finally stat
	    env.error(pos, "catch.without.try");

	    Statement s;
	    do {
		scan();
		expect(LPAREN);
		parseExpression();
		expect(IDENT);
		expect(RPAREN);
		s = parseStatement();
	    } while (token == CATCH);

	    if (token == FINALLY) {
		scan();
		s = parseStatement();
	    }
	    return s;
	  }

	  case FINALLY: {
	    // finally-statement: finally stat
	    env.error(pos, "finally.without.try");
	    scan();
	    return parseStatement();
	  }

	  case THROW: {
	    // throw-statement: throw expr;
	    int p = scan();
	    Expression e = parseExpression();
	    expect(SEMICOLON);
	    return new ThrowStatement(p, e);
	  }

	  case GOTO: {
	    int p = scan();
	    expect(IDENT);
	    expect(SEMICOLON);
	    env.error(p, "not.supported", "goto");
	    return new CompoundStatement(p, new Statement[0]);
	  }

	  case SYNCHRONIZED: {
	    // synchronized-statement: synchronized (expr) stat
	    int p = scan();
	    expect(LPAREN);
	    Expression e = parseExpression();
	    expect(RPAREN);
	    return new SynchronizedStatement(p, e, parseStatement());
	  }

	  case VOID:
	  case STATIC:
	  case PUBLIC:
	  case PRIVATE:
	  case INTERFACE:
	  case CLASS:
	  case TRANSIENT:
	    // This is the start of something outside a statement
	    env.error(pos, "statement.expected");
	    throw new SyntaxError();
	}

	int p = pos;
	Expression e = parseExpression();

	if (token == IDENT) {
	    // declaration: expr expr
	    Statement s = parseDeclaration(p, 0, e);
	    expect(SEMICOLON);
	    return s;
	}
	if (token == COLON) {
	    // label: id: stat
	    scan();
	    Statement s = parseStatement();
	    s.setLabel(env, e);
	    return s;
	}

	// it was just an expression...
	topLevelExpression(e);
	expect(SEMICOLON);
	return new ExpressionStatement(p, e);
    }

    /**
     * Parse an identifier. ie: a.b.c returns "a.b.c"
     * If star is true then "a.b.*" is allowed.
     */
    protected Identifier parseIdentifier(boolean star) throws SyntaxError, IOException {
	StringBuffer buf = new StringBuffer(32);
	if (token == IDENT) {
	    buf.append(idValue);
	}
	expect(IDENT);

	while (token == FIELD) {
	    scan();
	    if ((token == MUL) && star) {
		scan();
		buf.append(".*");
		break;
	    }

	    buf.append('.');
	    if (token == IDENT) {
		buf.append(idValue);
	    }
	    expect(IDENT);
	}

	return Identifier.lookup(buf.toString());
    }

    /**
     * Parse a type expression, this results in a Type.
     * It does not parse []'s.
     */
    protected Type parseType() throws SyntaxError, IOException {
	Type t;

	switch (token) {
	  case IDENT:
	    t = Type.tClass(resolveClass(parseIdentifier(false)));
	    break;
	  case VOID:
	    scan();
	    t = Type.tVoid;
	    break;
	  case BOOLEAN:
	    scan();
	    t = Type.tBoolean;
	    break;
	  case BYTE:
	    scan();
	    t = Type.tByte;
	    break;
	  case CHAR:
	    scan();
	    t = Type.tChar;
	    break;
	  case SHORT:
	    scan();
	    t = Type.tShort;
	    break;
	  case INT:
	    scan();
	    t = Type.tInt;
	    break;
	  case FLOAT:
	    scan();
	    t = Type.tFloat;
	    break;
	  case LONG:
	    scan();
	    t = Type.tLong;
	    break;
	  case DOUBLE:
	    scan();
	    t = Type.tDouble;
	    break;
	  default:
	    env.error(pos, "type.expected");
	    throw new SyntaxError();
	}

	// Parse []'s
	while (token == LSQBRACKET) {
	    scan();
	    expect(RSQBRACKET);
	    t = Type.tArray(t);
	}
	return t;
    }

    /*
     * Dealing with argument lists, I'm not using
     * Vector for efficiency.
     */

    private int aCount = 0;
    private Type aTypes[] = new Type[8];
    private Identifier aNames[] = new Identifier[aTypes.length];

    private void addArgument(Type t, Identifier nm) {
	if (aCount >= aTypes.length) {
	    Type newATypes[] = new Type[aCount * 2];
	    System.arraycopy(aTypes, 0, newATypes, 0, aCount);
	    aTypes = newATypes;
	    Identifier newANames[] = new Identifier[aCount * 2];
	    System.arraycopy(aNames, 0, newANames, 0, aCount);
	    aNames = newANames;
	}
	aTypes[aCount] = t;
	aNames[aCount++] = nm;
    }

    /**
     * Parse a field.
     */
    protected void parseField() throws SyntaxError, IOException {
	// Empty fields are allowed
	if (token == SEMICOLON) {
	    scan();
	    return;
	}

	// Optional doc comment
	String doc = docComment;

	// The start of the field
	int p = pos;

	// Parse the modifiers
	int mod = 0;

	while (true) {
	    int nextmod = 0;
	    switch (token) {
	      case PRIVATE: 	nextmod = M_PRIVATE; 		break;
	      case PUBLIC: 	nextmod = M_PUBLIC; 		break;
	      case PROTECTED: 	nextmod = M_PROTECTED; 		break;
	      case STATIC: 	nextmod = M_STATIC; 		break;
	      case TRANSIENT: 	nextmod = M_TRANSIENT; 		break;
	      case FINAL: 	nextmod = M_FINAL; 		break;
	      case ABSTRACT: 	nextmod = M_ABSTRACT; 		break;
	      case NATIVE: 	nextmod = M_NATIVE; 		break;
	      case VOLATILE: 	nextmod = M_VOLATILE;		break;
	      case SYNCHRONIZED:nextmod = M_SYNCHRONIZED;	break;
	    }
	    if (nextmod == 0) {
		break;
	    }
	    if ((mod & nextmod) != 0) {
		env.error(pos, "repeated.modifier");
	    }
	    mod |= nextmod;
	    scan();
	}

	// Check for static initializer
	// ie: static { ... }
	if ((mod == M_STATIC) && (token == LBRACE)) {
	    // static initializer
	    defineField(p, doc, M_STATIC,
			Type.tMethod(Type.tVoid),
		        idClassInit, null, null,
			parseStatement());
	    return;
	}

	// Parse the type
	p = pos;
	Type t = parseType();
	Identifier id = null;

	// Check that the type is followed by an Identifier
	// (the name of the method or the first variable),
	// otherwise it is a constructor.
	switch (token) {
	  case IDENT:
	    id = idValue;
	    p = scan();
	    break;

	  case LPAREN:
	    // It is a constructor
	    id = idInit;
	    break;

	  default:
	    expect(IDENT);
	}

	// If the next token is a left-bracket then we
	// are dealing with a method, otherwise it is
	// a list of variables
	if (token == LPAREN) {
	    // It is a method declaration
	    scan();
	    aCount = 0;

	    if (token != RPAREN) {
		// Parse argument type and identifier
		Type at = parseType();
		Identifier an = idValue;
		expect(IDENT);

		// Parse optional array specifier, ie: a[][]
		while (token == LSQBRACKET) {
		    scan();
		    if (token != RSQBRACKET) {
			env.error(pos, "array.dim.in.decl");
			parseExpression();
		    }
		    expect(RSQBRACKET);
		    at = Type.tArray(at);
		}
		addArgument(at, an);

		// If the next token is a comma then there are
		// more arguments
		while (token == COMMA) {
		    // Parse argument type and identifier
		    scan();
		    at = parseType();
		    an = idValue;
		    expect(IDENT);

		    // Parse optional array specifier, ie: a[][]
		    while (token == LSQBRACKET) {
			scan();
			if (token != RSQBRACKET) {
			    env.error(pos, "array.dim.in.decl");
			    parseExpression();
			}
			expect(RSQBRACKET);
			at = Type.tArray(at);
		    }
		    addArgument(at, an);
		}
	    }
	    expect(RPAREN);

	    // Parse optional array sepecifier, ie: foo()[][]
	    while (token == LSQBRACKET) {
		scan();
		if (token != RSQBRACKET) {
		    env.error(pos, "array.dim.in.decl");
		    parseExpression();
		}
		expect(RSQBRACKET);
		t = Type.tArray(t);
	    }

	    // copy arguments
	    Type atypes[] = new Type[aCount];
	    System.arraycopy(aTypes, 0, atypes, 0, aCount);

	    Identifier anames[] = new Identifier[aCount];
	    System.arraycopy(aNames, 0, anames, 0, aCount);

	    // Construct the type signature
	    t = Type.tMethod(t, atypes);

            // Parse and ignore throws clause
	    Identifier exp[] = null;
            if (token == THROWS) {
		Vector v = new Vector();
                scan();
		v.addElement(resolveClass(parseIdentifier(false)));
		while (token == COMMA) {
		    scan();
		    v.addElement(resolveClass(parseIdentifier(false)));
		}

		exp = new Identifier[v.size()];
		v.copyInto(exp);
	    }

	    // Check if it is a method definition or a method declaration
	    // ie: foo() {...} or foo();
	    switch (token) {
	      case LBRACE:
		// It is a method definition
		defineField(p, doc, mod, t, id, anames, exp, parseStatement());
		break;

	      case SEMICOLON:
		scan();
		defineField(p, doc, mod, t, id, anames, exp, null);
		break;

	      default:
		// really expected a statement body here
		if ((mod & (M_NATIVE | M_ABSTRACT)) == 0) {
		    expect(LBRACE);
		} else {
		    expect(SEMICOLON);
		}
	    }
	    return;
	}

	  // It is a list of instance variables
	while (true) {
	    p = pos;		// get the current position
	    // parse the array brackets (if any)
	    // ie: var[][][]
	    Type vt = t;
	    while (token == LSQBRACKET) {
		scan();
		if (token != RSQBRACKET) {
		    env.error(pos, "array.dim.in.decl");
		    parseExpression();
		}
		expect(RSQBRACKET);
		vt = Type.tArray(vt);
	    }

	    // Parse the optional initializer
	    Node init = null;
	    if (token == ASSIGN) {
		scan();
		init = parseExpression();
	    }

	    // Define the variable
	    defineField(p, doc, mod, vt, id, null, null, init);

	    // If the next token is a comma, then there is more
	    if (token != COMMA) {
		expect(SEMICOLON);
		return;
	    }
	    scan();

	    // The next token must be an identifier
	    id = idValue;
	    expect(IDENT);
	}
    }

    /**
     * Recover after a syntax error in a field. This involves
     * discarding tokens until an EOF or a possible legal
     * continutation is encountered.
     */
    protected void recoverField(Identifier nm) throws SyntaxError, IOException {
	while (true) {
	    switch (token) {
	      case EOF:
	      case STATIC:
	      case FINAL:
	      case PUBLIC:
	      case PRIVATE:
	      case SYNCHRONIZED:
	      case TRANSIENT:

	      case VOID:
	      case BOOLEAN:
	      case BYTE:
	      case CHAR:
	      case SHORT:
	      case INT:
	      case FLOAT:
	      case LONG:
	      case DOUBLE:
		// possible begin of a field, continue
		return;

	      case LBRACE:
		match(LBRACE, RBRACE);
		scan();
		break;

	      case LPAREN:
		match(LPAREN, RPAREN);
		scan();
		break;

	      case LSQBRACKET:
		match(LSQBRACKET, RSQBRACKET);
		scan();
		break;

	      case RBRACE:
	      case INTERFACE:
	      case CLASS:
	      case IMPORT:
	      case PACKAGE:
		// begin of something outside a class, panic more
		endClass(pos, nm);
		throw new SyntaxError();

	      default:
		// don't know what to do, skip
		scan();
		break;
	    }
	}
    }

    /**
     * Parse a class or interface declaration.
     */
    protected void parseClass() throws SyntaxError, IOException {
	String doc = docComment;

	// Parse the modifiers
	int mod = 0;

	while (true) {
	    int nextmod = 0;
	    switch (token) {
	      case PRIVATE: 	env.error(pos, "private.class"); scan(); continue;
	      case PUBLIC: 	nextmod = M_PUBLIC; 		break;
	      case FINAL: 	nextmod = M_FINAL; 		break;
	      case ABSTRACT: 	nextmod = M_ABSTRACT; 		break;
	    }
	    if (nextmod == 0) {
		break;
	    }
	    if ((mod & nextmod) != 0) {
		env.error(pos, "repeated.modifier");
	    }
	    mod |= nextmod;
	    scan();
	}

	// Parse class/interface
	switch (token) {
	  case INTERFACE:
	    scan();
	    mod |= M_INTERFACE;
	    break;

	  case CLASS:
	    scan();
	    break;

	  default:
	    env.error(pos, "class.expected");
	    break;
	}

	// Parse the class name
	Identifier nm = idValue;
	Vector ext = new Vector();
	Vector impl = new Vector();
	int p = pos;
	expect(IDENT);

	// Parse extends clause
	if (token == EXTENDS) {
	    scan();
	    ext.addElement(resolveClass(parseIdentifier(false)));
	    while (token == COMMA) {
		scan();
		ext.addElement(resolveClass(parseIdentifier(false)));
	    }
	}

	// Parse implements clause
	if (token == IMPLEMENTS) {
	    scan();
	    impl.addElement(resolveClass(parseIdentifier(false)));
	    while (token == COMMA) {
		scan();
		Identifier intf = resolveClass(parseIdentifier(false));
		if (impl.contains(intf)) {
		    env.error(p, "intf.repeated", intf);
		} else {
		    impl.addElement(intf);
		}
	    }
	}

	// Decide which is the super class
	Identifier sup = null;
	if ((mod & M_INTERFACE) != 0) {
	    if (impl.size() > 0) {
		env.error(p, "intf.impl.intf");
	    }
	    impl = ext;
	} else {
	    if (ext.size() > 0) {
		if (ext.size() > 1) {
		    env.error(p, "multiple.inherit");
		}
		sup = (Identifier)ext.elementAt(0);
	    } else {
		sup = idJavaLangObject;
	    }
	}

	// Begin a new class
	Identifier implids[] = new Identifier[impl.size()];
	impl.copyInto(implids);
	beginClass(p, doc, mod, nm, sup, implids);

	// Parse fields
	expect(LBRACE);
	while ((token != EOF) && (token != RBRACE)) {
	    try {
		parseField();
	    } catch (SyntaxError e) {
		recoverField(nm);
	    }
	}
	expect(RBRACE);

	// End the class
	endClass(prevPos, nm);
    }

    /**
     * Recover after a syntax error in the file.
     * This involves discarding tokens until an EOF
     * or a possible legal continuation is encountered.
     */
    protected void recoverFile() throws IOException {
	while (true) {
	    switch (token) {
	      case CLASS:
	      case INTERFACE:
		// Start of a new source file statement, continue
		return;
		
	      case LBRACE:
		match(LBRACE, RBRACE);
		scan();
		break;

	      case LPAREN:
		match(LPAREN, RPAREN);
		scan();
		break;

	      case LSQBRACKET:
		match(LSQBRACKET, RSQBRACKET);
		scan();
		break;

	      case EOF:
		return;
		
	      default:
		// Don't know what to do, skip
		scan();
		break;
	    }
	}
    }

    /**
     * Parse an Java file.
     */
    public void parseFile() {
	try {
	    try {
		if (token == PACKAGE) {
		    // Package statement
		    int p = scan();
		    Identifier id = parseIdentifier(false);
		    expect(SEMICOLON);
		    packageDeclaration(p, id);
		}
	    } catch (SyntaxError e) {
		recoverFile();
	    }
	    while (token == IMPORT) {
		try{
		    // Import statement
		    int p = scan();
		    Identifier id = parseIdentifier(true);
		    expect(SEMICOLON);
		    if (id.getName().equals(idStar)) {
			importPackage(p, id.getQualifier());
		    } else {
			importClass(p, id);
		    }
		} catch (SyntaxError e) {
		    recoverFile();
		}
	    }

	    while (token != EOF) {
		try {
		    switch (token) {
		      case FINAL:
		      case PUBLIC:
		      case PRIVATE:
		      case ABSTRACT:
		      case CLASS:
		      case INTERFACE:
			// Start of a class
			parseClass();
			break;

		      case SEMICOLON:
			// Bogus semi colon
			scan();
			break;

		      case EOF:
			// The end
			return;

		      default:
			// Oops
			env.error(pos, "toplevel.expected");
			throw new SyntaxError();
		    }
		} catch (SyntaxError e) {
		    recoverFile();
		}
	    }
	} catch (IOException e) {
	    env.error(pos, "io.exception", env.getSource());
	    return;
	}
    }
}


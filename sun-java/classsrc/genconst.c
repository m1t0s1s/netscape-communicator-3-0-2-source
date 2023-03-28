
#include <stdio.h>
#include <sys/types.h>
#include <oobj.h>
#include <tree.h>
#include <signature.h>
#include <typecodes.h>
#include <opcodes.init>
#include <opcodes.length>
#include <string.h>
#include <ctype.h>

typedef struct fieldblock fieldblock;
typedef struct methodblock methodblock;
typedef struct CatchFrame CatchFrame;

#define off(s, f)	((int)&(((s *)0)->f))
#define siz(s, f)	sizeof(((s *)0)->f)

#define field_off(nm, s, f)	\
    constant(nm "_off", off(s, f));

#define field_siz(nm, s, f)	\
    constant(nm "_siz", siz(s, f));

#define field0(nm, s, f)		\
    field_off(nm, s, f);	\
    field_siz(nm, s, f);

#define field(nm, s, f)	\
    field0(nm "_" # f, s, f)

#define cfield(nm, s, f)	\
    constant(nm "_" # f "_off", off(s, f) - off(s, magic)); \
    constant(nm "_" # f "_siz", siz(s, f))

void dconstant(char *name, char *val) {
    char buf[128], *p = buf;
    while ((*p++ = toupper(*name++)));
    printf("    public static final int %-28s = %s;\n", buf, val);
}

void hexconstant(char *name, int val) {
    char buf[128], *p = buf;
    while ((*p++ = toupper(*name++)));
    printf("    public static final int %-28s = 0x%08x;\n", buf, val);
}

void cconstant(char *name, int val) {
    char buf[128], *p = buf;
    while ((*p++ = toupper(*name++)));
    printf("    public static final char %-28s = '%c';\n", buf, val);
}

void constant(char *name, int val) {
    char buf[128], *p = buf;
    while ((*p++ = toupper(*name++)));
    printf("    public static final int %-28s = %d;\n", buf, val);
}

void sigconstant(char *name, int val) {
    char buf[128], *p = buf;
    while ((*p++ = toupper(*name++)));
    printf("    public static final char   SIGC_%-21s = '%c';\n", buf, val);
    printf("    public static final String SIG_%-22s = \"%c\";\n", buf, val);
}

int main(int argc, char *argv[]) {
    int i, last_opcode;

    printf("\n");
    printf("    /* Signature Characters */\n");
    sigconstant("VOID",		SIGNATURE_VOID);
    sigconstant("BOOLEAN",	SIGNATURE_BOOLEAN);
    sigconstant("BYTE",		SIGNATURE_BYTE);
    sigconstant("CHAR",		SIGNATURE_CHAR);
    sigconstant("SHORT",	SIGNATURE_SHORT);
    sigconstant("INT",		SIGNATURE_INT);
    sigconstant("LONG",		SIGNATURE_LONG);
    sigconstant("FLOAT",	SIGNATURE_FLOAT);
    sigconstant("DOUBLE",	SIGNATURE_DOUBLE);
    sigconstant("ARRAY",	SIGNATURE_ARRAY);
    sigconstant("CLASS",	SIGNATURE_CLASS);
    sigconstant("METHOD",	SIGNATURE_FUNC);
    sigconstant("ENDCLASS",	SIGNATURE_ENDCLASS);
    sigconstant("ENDMETHOD",	SIGNATURE_ENDFUNC);
    sigconstant("PACKAGE",	'/');

    printf("\n");
    printf("    /* Class File Constants */\n");
    hexconstant("JAVA_MAGIC", JAVA_CLASSFILE_MAGIC);
    constant("JAVA_VERSION", JAVA_VERSION);
    constant("JAVA_MINOR_VERSION", JAVA_MINOR_VERSION);

    printf("\n");
    printf("    /* Constant table */\n");
    constant("CONSTANT_UTF8", 			CONSTANT_Utf8);
    constant("CONSTANT_UNICODE", 		CONSTANT_Unicode);
    constant("CONSTANT_INTEGER",		CONSTANT_Integer);
    constant("CONSTANT_FLOAT", 			CONSTANT_Float);
    constant("CONSTANT_LONG", 			CONSTANT_Long);
    constant("CONSTANT_DOUBLE", 		CONSTANT_Double);
    constant("CONSTANT_CLASS", 			CONSTANT_Class);
    constant("CONSTANT_STRING", 		CONSTANT_String);
    constant("CONSTANT_FIELD", 			CONSTANT_Fieldref);
    constant("CONSTANT_METHOD",			CONSTANT_Methodref);
    constant("CONSTANT_INTERFACEMETHOD",	CONSTANT_InterfaceMethodref);
    constant("CONSTANT_NAMEANDTYPE",		CONSTANT_NameAndType);

    printf("\n");
    printf("    /* Access Flags */\n");
    hexconstant("ACC_PUBLIC", 		ACC_PUBLIC);
    hexconstant("ACC_PRIVATE", 		ACC_PRIVATE);
    hexconstant("ACC_PROTECTED",	ACC_PROTECTED);
    hexconstant("ACC_STATIC", 		ACC_STATIC);
    hexconstant("ACC_FINAL", 		ACC_FINAL);
    hexconstant("ACC_SYNCHRONIZED", 	ACC_SYNCHRONIZED);
    hexconstant("ACC_VOLATILE", 	ACC_THREADSAFE);
    hexconstant("ACC_TRANSIENT", 	ACC_TRANSIENT);
    hexconstant("ACC_NATIVE", 		ACC_NATIVE);
    hexconstant("ACC_INTERFACE", 	ACC_INTERFACE);
    hexconstant("ACC_ABSTRACT", 	ACC_ABSTRACT);

    printf("\n");
    printf("    /* Type codes */\n");
    hexconstant("T_CLASS", 	T_CLASS);
    hexconstant("T_BOOLEAN", 	T_BOOLEAN);
    hexconstant("T_CHAR", 	T_CHAR);
    hexconstant("T_FLOAT", 	T_FLOAT);
    hexconstant("T_DOUBLE", 	T_DOUBLE);
    hexconstant("T_BYTE", 	T_BYTE);
    hexconstant("T_SHORT", 	T_SHORT);
    hexconstant("T_INT", 	T_INT);
    hexconstant("T_LONG", 	T_LONG);

    for (i = 0; i < 256; i++) {
	char *str = opnames[i];
	if (strcmp(str, "??") == 0)
	    break;
	if (  ((int)(strlen(str)) > 6) && 
	       (strcmp(str + (strlen(str) - 6), "_quick") == 0))
	    break;
    }
    last_opcode = i;

    printf("\n");
    printf("    /* Opcodes */\n");
    printf("    public static final int opc_%-24s = %d;\n", "try", -3);
    printf("    public static final int opc_%-24s = %d;\n", "dead", -2);
    printf("    public static final int opc_%-24s = %d;\n", "label", -1);
    for (i = 0 ; i < last_opcode ; i++) {
	char *str = opnames[i];
	printf("    public static final int opc_%-24s = %d;\n", str, i);
    }
    printf("\n");

    printf("    /* Opcode Names */\n");
    printf("    public static final String opcNames[] = {");
    for (i = 0 ; i < last_opcode ; i++) 
	printf((i > 0) ? ",\n\t\"%s\"" : "\n\t\"%s\"", opnames[i]);
    printf("\n    };\n");
    printf("\n");

    printf("    /* Opcode Lengths */\n");
    printf("    public static final int opcLengths[] = {");
    for (i = 0 ; i < last_opcode; i++)
	printf((i > 0) ? ",\n\t%d" : "\n\t%d", opcode_length[i]);
    printf("\n    };\n");
    printf("\n");


    return 0;
}

package netscape.jri;

import netscape.jri.*;

interface JRIEnv {
    // Environments
    Class	LoadClass(byte[] buf);
    Class	FindClass(ConstCString name);
    void	Throw(Throwable obj);
    void	ThrowNew(Class clazz, ConstCString message);
    Throwable	ExceptionOccurred();
    void	ExceptionDescribe();
    void	ExceptionClear();

    // References
    jglobal	NewGlobalRef(Object obj);
    void	DisposeGlobalRef(jglobal ref);
    Object	GetGlobalRef(jglobal ref);
    void	SetGlobalRef(jglobal ref, Object value);
    boolean	IsSameObject(Object o1, Object o2);

    // Object Operations
    Object	NewObject(Class clazz, int methodID, Varargs args);
    Class	GetObjectClass(Object obj);
    boolean	IsInstanceOf(Object obj, Class clazz);

    // Calling Public Dynamic Methods of Objects
    int		GetMethodID(Class clazz, ConstCString name, ConstCString sig);

    Object	CallMethod(Object obj, int methodID, Varargs args);
    boolean	CallMethodBoolean(Object obj, int methodID, Varargs args);
    byte	CallMethodByte(Object obj, int methodID, Varargs args);
    char	CallMethodChar(Object obj, int methodID, Varargs args);
    short	CallMethodShort(Object obj, int methodID, Varargs args);
    int		CallMethodInt(Object obj, int methodID, Varargs args);
    long	CallMethodLong(Object obj, int methodID, Varargs args);
    float	CallMethodFloat(Object obj, int methodID, Varargs args);
    double	CallMethodDouble(Object obj, int methodID, Varargs args);

    // Accessing Public Fields of Objects
    int		GetFieldID(Class clazz, ConstCString name, ConstCString sig);

    Object	GetField(Object obj, int fieldID);
    boolean	GetFieldBoolean(Object obj, int fieldID);
    byte	GetFieldByte(Object obj, int fieldID);
    char	GetFieldChar(Object obj, int fieldID);
    short	GetFieldShort(Object obj, int fieldID);
    int		GetFieldInt(Object obj, int fieldID);
    long	GetFieldLong(Object obj, int fieldID);
    float	GetFieldFloat(Object obj, int fieldID);
    double	GetFieldDouble(Object obj, int fieldID);

    void	SetField(Object obj, int fieldID, Object value);
    void	SetFieldBoolean(Object obj, int fieldID, boolean value);
    void	SetFieldByte(Object obj, int fieldID, byte value);
    void	SetFieldChar(Object obj, int fieldID, char value);
    void	SetFieldShort(Object obj, int fieldID, short value);
    void	SetFieldInt(Object obj, int fieldID, int value);
    void	SetFieldLong(Object obj, int fieldID, long value);
    void	SetFieldFloat(Object obj, int fieldID, float value);
    void	SetFieldDouble(Object obj, int fieldID, double value);

    // Class Operations
    boolean	IsSubclassOf(Class clazz, Class superclass);

    // Calling Public Dynamic Methods of Objects
    int		GetStaticMethodID(Class clazz, ConstCString name, ConstCString sig);

    Object	CallStaticMethod(Class clazz, int methodID, Varargs args);
    boolean	CallStaticMethodBoolean(Class clazz, int methodID, Varargs args);
    byte	CallStaticMethodByte(Class clazz, int methodID, Varargs args);
    char	CallStaticMethodChar(Class clazz, int methodID, Varargs args);
    short	CallStaticMethodShort(Class clazz, int methodID, Varargs args);
    int		CallStaticMethodInt(Class clazz, int methodID, Varargs args);
    long	CallStaticMethodLong(Class clazz, int methodID, Varargs args);
    float	CallStaticMethodFloat(Class clazz, int methodID, Varargs args);
    double	CallStaticMethodDouble(Class clazz, int methodID, Varargs args);

    // Accessing Public Static Fields of Classes
    int		GetStaticFieldID(Class clazz, ConstCString name, ConstCString sig);

    Object	GetStaticField(Class clazz, int fieldID);
    boolean	GetStaticFieldBoolean(Class clazz, int fieldID);
    byte	GetStaticFieldByte(Class clazz, int fieldID);
    char	GetStaticFieldChar(Class clazz, int fieldID);
    short	GetStaticFieldShort(Class clazz, int fieldID);
    int		GetStaticFieldInt(Class clazz, int fieldID);
    long	GetStaticFieldLong(Class clazz, int fieldID);
    float	GetStaticFieldFloat(Class clazz, int fieldID);
    double	GetStaticFieldDouble(Class clazz, int fieldID);

    void	SetStaticField(Class clazz, int fieldID, Object value);
    void	SetStaticFieldBoolean(Class clazz, int fieldID, boolean value);
    void	SetStaticFieldByte(Class clazz, int fieldID, byte value);
    void	SetStaticFieldChar(Class clazz, int fieldID, char value);
    void	SetStaticFieldShort(Class clazz, int fieldID, short value);
    void	SetStaticFieldInt(Class clazz, int fieldID, int value);
    void	SetStaticFieldLong(Class clazz, int fieldID, long value);
    void	SetStaticFieldFloat(Class clazz, int fieldID, float value);
    void	SetStaticFieldDouble(Class clazz, int fieldID, double value);

    // String Operations
    // Unicode Interface
    String	NewString(const_jchar_star unicode, int length);
    int		GetStringLength(String string);
    const_jchar_star	GetStringChars(String string);
    // UTF Interface
    String	NewStringUTF(const_jbyte_star utf, int length);
    int		GetStringUTFLength(String string);
    const_jbyte_star	GetStringUTFChars(String string);
    
    // Scalar Array Operations
    Object	NewScalarArray(int length, ConstCString elementSig,
			       const_jbyte_star initialElement);
    int		GetScalarArrayLength(Object array);
    jbyte_star	GetScalarArrayElements(Object array);

    // Object Array Operations
    Object	NewObjectArray(int length, Class elementClass, 
			       Object initialElement);
    int		GetObjectArrayLength(Object array);
    Object	GetObjectArrayElement(Object array, int index);
    void	SetObjectArrayElement(Object array, int index, Object value);

    // Native Bootstrap
    void	RegisterNatives(Class clazz, 
				null_terminated_cstring_array nameAndSigArray,
				null_terminated_pointer_array nativeProcArray);
    void	UnregisterNatives(Class clazz);
}

class jglobal extends CType {
    public static final String ctype = "jglobal";
}

class const_jchar_star extends CType {
    public static final String ctype = "const jchar*";
}

class const_jbyte_star extends CType {
    public static final String ctype = "const jbyte*";
}

class jbyte_star extends CType {
    public static final String ctype = "jbyte*";
}

class null_terminated_cstring_array extends CType {
    public static final String ctype = "char**";
}

class null_terminated_pointer_array extends CType {
    public static final String ctype = "void**";
}


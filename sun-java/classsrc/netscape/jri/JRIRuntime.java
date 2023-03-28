package netscape.jri;

import netscape.jri.*;

interface JRIRuntime {
    void	Dispose();
    void	SetIOMode(int flags);
    void	SetFSMode(int flags);
    void	SetRTMode(int flags);
    JRIEnv	NewEnv(CType thread);
}

interface JRIEnvControl extends JRIEnv {
    void	DisposeEnv();
    JRIRuntime	GetRuntime();
    CType	GetThread();
    void	SetClassLoader(ClassLoader classLoader);
}

interface JRIReflection {
    // enumerate all classes:
    int		GetClassCount();
    Class	GetClass(int i);

    // reflective class operations:
    String	GetClassName(Class clazz);
    boolean	VerifyClass(Class clazz);
    Class	GetClassSuperclass(Class clazz);
    int		GetClassInterfaceCount(Class clazz);
    Class	GetClassInterface(Class clazz, int i);
    int		GetClassFieldCount(Class clazz);
    // void	GetClassFieldInfo(Class clazz, ???);
    int		GetClassMethodCount(Class clazz);
    // void	GetClassMethodInfo(Class clazz, ???);
}

interface JRIDebugger {
    
}


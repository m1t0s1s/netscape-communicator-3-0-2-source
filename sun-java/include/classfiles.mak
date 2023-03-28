IOFILES =					   \
	java.io.BufferedInputStream		   \
	java.io.BufferedOutputStream		   \
	java.io.DataInputStream			   \
	java.io.DataOutputStream		   \
	java.io.File				   \
	java.io.FileInputStream			   \
	java.io.FileOutputStream		   \
	java.io.FilenameFilter			   \
	java.io.FilterInputStream		   \
	java.io.FilterOutputStream		   \
	java.io.IOException			   \
	java.io.InputStream			   \
	java.io.InputStreamBuffer		   \
	java.io.InputStreamSequence		   \
	java.io.InputStreamStringBuffer		   \
	java.io.LineNumberInputStream		   \
	java.io.OutputStream			   \
	java.io.OutputStreamBuffer		   \
	java.io.PrintStream			   \
	java.io.PushbackInputStream		   \
	java.io.RandomAccessFile		   \
	java.io.StreamTokenizer			   

LANGFILES = 					   \
	java.lang.AbstractMethodException	   \
	java.lang.ArithmeticException		   \
	java.lang.ArrayIndexOutOfBoundsException   \
	java.lang.ArrayStoreException		   \
	java.lang.Boolean			   \
	java.lang.Character			   \
	java.lang.Class				   \
	java.lang.ClassCastException		   \
	java.lang.ClassCircularityException	   \
	java.lang.ClassFormatException		   \
	java.lang.ClassLoader			   \
	java.lang.DataFormatException		   \
	java.lang.Double			   \
	java.lang.Exception			   \
	java.lang.FileNotFoundException		   \
	java.lang.Float				   \
	java.lang.IllegalAccessException	   \
	java.lang.IllegalArgumentException	   \
	java.lang.IllegalThreadStateException	   \
	java.lang.IncompatibleClassChangeException \
	java.lang.IndexOutOfBoundsException	   \
	java.lang.InstantiationException	   \
	java.lang.Integer			   \
	java.lang.InternalException		   \
	java.lang.LinkageException		   \
	java.lang.Long				   \
	java.lang.Math				   \
	java.lang.NegativeArraySizeException	   \
	java.lang.NoClassDefFoundException	   \
	java.lang.NoSuchMethodException		   \
	java.lang.NullPointerException		   \
	java.lang.Number			   \
	java.lang.NumberFormatException		   \
	java.lang.Object			   \
	java.lang.OutOfMemoryException		   \
	java.lang.Process			   \
	java.lang.Runnable			   \
	java.lang.RuntimeException		   \
	java.lang.Runtime			   \
	java.lang.SecurityException		   \
	java.lang.SecurityManager		   \
	java.lang.SeriousException		   \
	java.lang.StackOverflowException	   \
	java.lang.String			   \
	java.lang.StringBuffer			   \
	java.lang.StringIndexOutOfRangeException   \
	java.lang.System			   \
	java.lang.Thread			   \
	java.lang.ThreadDeath			   \
	java.lang.ThreadGroup			   \
	java.lang.UnknownException		   \
	java.lang.UnsatisfiedLinkException	   \
	java.lang.VirtualMachineException

NETFILES = 					   \
	java.net.InetAddress			   \
	java.net.MalformedURLException		   \
	java.net.PlainSocketImpl		   \
	java.net.ProtocolException		   \
	java.net.ServerSocket			   \
	java.net.Socket				   \
	java.net.SocketException		   \
	java.net.SocketImpl			   \
	java.net.SocketImplFactory		   \
	java.net.SocketInputStream		   \
	java.net.SocketOutputStream		   \
	java.net.URL				   \
	java.net.URLConnection			   \
	java.net.URLStreamHandler		   \
	java.net.URLStreamHandlerFactory	   \
	java.net.UnknownHostException		   \
	java.net.UnknownServiceException

UTILFILES = 					   \
	java.util.BitSet			   \
	java.util.Date				   \
	java.util.EmptyStackException		   \
	java.util.Enumeration			   \
	java.util.Hashtable			   \
	java.util.NoSuchElementException	   \
	java.util.Observable			   \
	java.util.Observer			   \
	java.util.Properties			   \
	java.util.Stack				   \
	java.util.StringTokenizer		   \
	java.util.Vector			   \
	java.tools.debug.BreakpointQueue

CLASSFILES = $(IOFILES) $(LANGFILES) $(NETFILES) $(UTILFILES)

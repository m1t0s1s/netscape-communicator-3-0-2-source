/* XXX should put this into libjava and into javai.c instead */
#include "_stubs/java_lang_Compiler.c"
#include "_stubs/java_lang_Double.c"
#include "_stubs/java_lang_Throwable.c"
#include "_stubs/java_lang_Float.c"
#include "_stubs/java_lang_Integer.c"
#include "_stubs/java_lang_Long.c"
#include "_stubs/java_lang_Math.c"
#include "_stubs/java_lang_Object.c"
#include "_stubs/java_lang_String.c"
#include "_stubs/java_lang_Thread.c"
#include "_stubs/java_util_Date.c"
#include "_stubs/sun_awt_image_GifImageDecoder.c"

#if !defined(XP_PC)
/* 
** Since the JPEG decoder requires the jpeg library, these functions
** must be linked into the navigator...
*/
#include "_stubs/sun_awt_image_JPEGImageDecoder.c"
#endif /* XP_PC */

/* Contentious natives */
#include "_stubs/java_lang_Class.c"
#include "_stubs/java_lang_ClassLoader.c"
#include "_stubs/java_lang_Runtime.c"
#include "_stubs/java_lang_SecurityManager.c"
#include "_stubs/java_lang_System.c"
#include "_stubs/java_io_FileOutputStream.c"
#include "_stubs/java_net_DatagramSocket.c"
#include "_stubs/java_net_InetAddress.c"

#include "_stubs/java_io_File.c"
#include "_stubs/java_io_FileDescriptor.c"
#include "_stubs/java_io_FileInputStream.c"
#include "_stubs/java_io_RandomAccessFile.c"
#include "_stubs/java_net_PlainSocketImpl.c"
#include "_stubs/java_net_SocketInputStream.c"
#include "_stubs/java_net_SocketOutputStream.c"

#ifdef BREAKPTS
#include "_stubs/sun_tools_debug_Agent.c"
#include "_stubs/sun_tools_debug_BreakpointHandler.c"
#endif

void _java_lang_init(void) { }

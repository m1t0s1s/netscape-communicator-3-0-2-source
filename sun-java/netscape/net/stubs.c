#ifndef XP_MAC
#include "_stubs/netscape_net_URLStreamHandlerFactory.c"
#include "_stubs/netscape_net_URLConnection.c"
#include "_stubs/netscape_net_URLInputStream.c"
#include "_stubs/netscape_net_URLOutputStream.c"
#else
#include "n_net_URLStreamHandlerFactory.c"
#include "netscape_net_URLConnection.c"
#include "netscape_net_URLInputStream.c"
#include "netscape_net_URLOutputStream.c"
#endif

void _java_net_init(void) { }

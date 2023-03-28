#!make
#
# Sample text plugin makefile
#
# Platform: SCO UnixWare 2.01 and above
#
# The output of the make process will be libtextplugin.so
# Install this file either in
#	/usr/lib/netscape/plugins/
#	(or)
#	~/.netscape/plugins/
#	(or) in any convenient directory and point environment variable
#	     NPX_PLUGIN_PATH to point to the directory. It is advisable
#	     that the plugins (.so) are the only files in that directory.
#
# This makefile contains some of our defines for the compiler:
#
# XP_UNIX	This needs to get defined for npapi.h on unix platforms.
# PLUGIN_TRACE	Enable this define to get debug prints whenever the plugin
#		api gets control.
#		
# - dp Suresh <dp@netscape.com>
# Wed May 15 23:03:36 PDT 1996
#
# Makefile ported to SCO 
# - Roger Oberholtzer <roger@seaotter.opq.se.netscape.com>
# Wed, 26 Jun 1996 13:44:31 +0200 (MET DST)
#

# PLUGIN_DEFINES= -DXP_UNIX -DPLUGIN_TRACE
PLUGIN_DEFINES= -DXP_UNIX

CC= cc
OPTIMIZER= -O
CFLAGS= -KPIC $(OPTIMIZER) $(PLUGIN_DEFINES) -I. -I/usr/include -I/usr/X/include

SRC= npunix.c npshell.c 
OBJ= npunix.o npshell.o
SHAREDTARGET=libtextplugin.so

.c.o:
	$(CC) -c $(CFLAGS) $<

default all: $(SHAREDTARGET)

$(SHAREDTARGET): $(OBJ)
	@cc -G -o $(SHAREDTARGET) $(OBJ)

clean:
	$(RM) $(OBJ) $(SHAREDTARGET)


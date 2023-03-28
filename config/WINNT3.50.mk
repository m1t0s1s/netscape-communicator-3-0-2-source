#
# Config stuff for WINNT 3.51
#

CC=cl
RANLIB = echo

OTHER_DIRT = 
GARBAGE = vc20.pdb

CPU_ARCH = x386
OS_CFLAGS = -MT -W3 -nologo -D_X86_ -Dx386 -D_WINDOWS -DWIN32 -DHW_THREADS
OS_LIBS = libcmt.lib kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib

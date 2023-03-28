#
# Config stuff for WINNT 4.0
#

CC=cl
LINK = link
RANLIB = echo
BSDECHO = echo

OTHER_DIRT = 
GARBAGE = vc20.pdb

PROCESSOR := $(shell uname -p)
ifeq ($(PROCESSOR), I386)
CPU_ARCH = x386
OS_CFLAGS = $(OPTIMIZER) -MD -W3 -nologo -D_X86_ -Dx386 -D_WINDOWS -DWIN32 -DHW_THREADS
else 
ifeq ($(PROCESSOR), MIPS)
CPU_ARCH = MIPS
#OS_CFLAGS = $(OPTIMIZER) -MD -W3 -nologo -D_MIPS_ -D_WINDOWS -DWIN32 -DHW_THREADS
OS_CFLAGS = $(OPTIMIZER) -MD -W3 -nologo -D_WINDOWS -DWIN32 -DHW_THREADS
else 
ifeq ($(PROCESSOR), ALPHA)
CPU_ARCH = ALPHA
OS_CFLAGS = $(OPTIMIZER) -MD -W3 -nologo -D_ALPHA_=1 -D_WINDOWS -DWIN32 -DHW_THREADS
else 
CPU_ARCH = processor_is_undefined
endif
endif
endif

OS_DLLFLAGS = -nologo -DLL -SUBSYSTEM:WINDOWS
OS_LFLAGS = -nologo -PDB:NONE -INCREMENT:NO -SUBSYSTEM:console
OS_LIBS = kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib

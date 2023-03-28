.SILENT:
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the rules necessary to build 16 and 32 bit
#// object files.
#//
#//------------------------------------------------------------------------
!if !defined(CONFIG_OBJ_INC)
CONFIG_OBJ_INC=1

#//------------------------------------------------------------------------
#//
#// Rules for compiling 16/32 bit object files from either c or c++ source
#//
#//------------------------------------------------------------------------
.c{.\$(OBJDIR)\}.obj:
    $(CC) @<<$(CFGFILE)
        -c $(CFLAGS)
        -Fd$(PDBFILE)
        -Fo.\$(OBJDIR)\
        $(*B).c
<<KEEP

.cpp{.\$(OBJDIR)\}.obj:
    $(CC) @<<$(CFGFILE)
        -c $(CFLAGS)
        -Fd$(PDBFILE)
        -Fo.\$(OBJDIR)\
        $(*B).cpp
<<

#//------------------------------------------------------------------------
#//
#// Rule for compiling resource files
#//
#//------------------------------------------------------------------------
.rc{.\$(OBJDIR)\}.res:
#   //
#   // execute the command
#   //
    echo Creating Resource file: $*.res
    $(RC) $(RCFLAGS) -r -Fo.\$(OBJDIR)\$(*B).res $(*B).rc


!if "$(OS_RELEASE)" == "16"
#//------------------------------------------------------------------------
#//
#// Rule for building simple 16 bit executables
#//
#//------------------------------------------------------------------------
.c{.\$(OBJDIR)\}.exe:
    $(CC) @<<$(CFGFILE)
        -c $(CFLAGS)
        -Fd$(PBDFILE)
        -Fo.\$(OBJDIR)\
        $(*B).c
<<
    $(LD) @<<$(CFGFILE)
        $(LFLAGS)
        $(OBJDIR)\$(*B).obj,
        $(OBJDIR)\$(*B).exe,
        $(MAPFILE),
        $(LLIBS) $(OS_LIBS),
        $(DEFFILE),
        $(RESFILE),
<<

!else
#//------------------------------------------------------------------------
#//
#// Rule for building simple 32 bit executables
#//
#//------------------------------------------------------------------------
.c{.\$(OBJDIR)\}.exe:
    $(CC) @<<$(CFGFILE)
        $(CFLAGS)
        -Fd$(PBDFILE)
        -Fe.\$(OBJDIR)\
        $(*B).c
<<
!endif

!endif # CONFIG_OBJ_INC

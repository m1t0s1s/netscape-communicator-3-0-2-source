#.SILENT:
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the rules necessary to build 16 and 32 bit
#// DLLs.
#//
#//------------------------------------------------------------------------
!if defined(DLL) && !defined(CONFIG_DLL_INC)
CONFIG_DLL_INC=1


!if "$(OS_RELEASE)" == "16"
#//------------------------------------------------------------------------
#//
#// Rule to build a 16-bit DLL using the DLL target
#//
#//------------------------------------------------------------------------
$(DLL): $(OBJDIR) $(OBJS) $(RESFILE) $(DEFFILE)
	echo +++ MAKE: Creating DLL: $@
#   //
#   // create response file for the command.  The format is:
#   //     Object files, Output file, Map file, Libraries, DEF file, RES file
#   //
        echo $(LFLAGS)                        > $(CFGFILE)
        for %%d in ($(OBJS)) do echo %%d  +  >> $(CFGFILE)
        echo.                                >> $(CFGFILE)
        echo $(OBJDIR)\$(*B).dll,            >> $(CFGFILE)
!ifdef MAPFILE
        echo $(MAPFILE),                     >> $(CFGFILE)
!endif
        echo $(LLIBS) $(OS_LIBS)             >> $(CFGFILE)
!ifdef DEFFILE
        echo $(DEFFILE),                     >> $(CFGFILE)
!else
        echo.                                >> $(CFGFILE)
!endif
!ifdef RESFILE
        echo $(RESFILE),                     >> $(CFGFILE)
!else
        echo.                                >> $(CFGFILE)
!endif
#   //
#   // execute the commands
#   //
    $(LD) @$(CFGFILE)
    implib /noignorecase /nologo $(OBJDIR)\$(*B).lib $(OBJDIR)\$(*B).dll

!else
#//------------------------------------------------------------------------
#//
#// Rule to build a 32-bit DLL using the DLL target
#//
#//------------------------------------------------------------------------
$(DLL): $(OBJDIR) $(OBJS) $(RESFILE) $(DEFFILE)
	echo +++ MAKE: Creating DLL: $@
    $(LD) @<<$(CFGFILE)
        /NOLOGO /DLL /OUT:$@ 
		/PDB:$(PDBFILE)
!ifdef DEFFILE
        /DEF:$(DEFFILE)
!endif
!ifdef MAPFILE
		/MAP:$(MAPFILE)
!endif

        $(LFLAGS)
        $(OBJS) 
        $(RESFILE)
        $(LLIBS) $(OS_LIBS)
<<KEEP
!endif

!endif # DLL && ! CONFIG_DLL_INC

.SILENT:
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the rules necessary to build 16 and 32 bit
#// executables.
#//
#//------------------------------------------------------------------------
!if defined(PROGRAM) && !defined(CONFIG_EXE_INC)
CONFIG_EXE_INC=1


!if "$(OS_RELEASE)" == "16"
#//------------------------------------------------------------------------
#//
#// Rule to build a 16-bit executable using the PROGRAM target
#//
#//------------------------------------------------------------------------
$(PROGRAM):: $(OBJDIR) $(OBJS) $(RESFILE) $(DEFFILE)
	echo +++ MAKE: Creating EXE: $@
#   //
#   // create response file for the command.  The format is:
#   //     Object files, Output file, Map file, Libraries, DEF file, RES file
#   //
        echo $(LFLAGS)                       > $(CFGFILE)
        for %%d in ($(OBJS)) do echo %%d  + >> $(CFGFILE)
        echo.                               >> $(CFGFILE)
        echo $(OBJDIR)\$(*B).exe,           >> $(CFGFILE)
        echo $(MAPFILE),                    >> $(CFGFILE)
        echo $(LLIBS) $(OS_LIBS)            >> $(CFGFILE)
!ifdef DEFFILE
        echo $(DEFFILE),                    >> $(CFGFILE)
!else
        echo.                               >> $(CFGFILE)
!endif
!ifdef RESFILE
        echo $(RESFILE),                    >> $(CFGFILE)
!else
        echo.                               >> $(CFGFILE)
!endif
#   //
#   // execute the command
#   //
    $(LD) /ST:8192 @$(CFGFILE)

!else
#//------------------------------------------------------------------------
#//
#// Rule to build a 32-bit executable using the PROGRAM target
#//
#//------------------------------------------------------------------------
$(PROGRAM):: $(OBJDIR) $(OBJS) $(RESFILE) $(DEFFILE)
	echo +++ MAKE: Creating EXE: $@
    $(LD) @<<$(CFGFILE)
        /NOLOGO /OUT:$@ 
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
<<
!endif

!endif # PROGRAM && ! CONFIG_EXE_INC

.SILENT:
!CMDSWITCHES -S
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the rules necessary to build 16 and 32 bit
#// libraries.
#//
#//------------------------------------------------------------------------
!if defined(LIBRARY) && !defined(CONFIG_LIB_INC)
CONFIG_LIB_INC=1

!if "$(OS_RELEASE)" == "16"
#//------------------------------------------------------------------------
#//
#// Rule to build a 16-bit Library
#//
#//------------------------------------------------------------------------
$(LIBRARY):: $(OBJDIR) $(OBJS)
#   //
#   // create response file for the command.  The format is:
#   //     LIBNAME, -+foo.obj -+bar.obj , LISTFILE, NEWLIB
#   //
	rm -f $(CFGFILE)
!if "$(OS)" == "Windows_NT"
        for %%d in ($(OBJS)) do echo -+%%d ^& >> $(CFGFILE)
!else
        for %%d in ($(OBJS)) do echo -+%%d  & >> $(CFGFILE)
!endif
        echo *                                >> $(CFGFILE)
#   //
#   // execute the commands
#   //
    $(RM) $@
    $(AR) $@ @$(CFGFILE)
    $(RANLIB) $@
!else
#//------------------------------------------------------------------------
#//
#// Rule to build a 32-bit Library
#//
#//------------------------------------------------------------------------
$(LIBRARY):: $(OBJDIR) $(OBJS)
    $(RM) $@ 2> NUL
    $(AR) @<<$(CFGFILE)
        -NOLOGO -OUT:$@
        $(OBJS)
<<
    $(RANLIB) $@
!endif

!endif # LIBRARY && ! CONFIG_LIB_INC

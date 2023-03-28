#.SILENT:
#//------------------------------------------------------------------------
#//
#// This makefile contains all of the common rules shared by all other
#// makefiles.
#//
#//------------------------------------------------------------------------

!if !defined(CONFIG_RULES_MAK)
CONFIG_RULES_MAK=1

include <$(DEPTH)/config/config.mak>

#//------------------------------------------------------------------------
#//
#// Specify a default target if non was set...
#//
#//------------------------------------------------------------------------
!ifndef TARGETS
TARGETS=$(PROGRAM) $(LIBRARY) $(DLL)
!endif

!ifndef MAKE_ARGS
MAKE_ARGS=export
!endif

all:: $(MAKE_ARGS)


#//------------------------------------------------------------------------
#//
#// Setup tool flags for the appropriate type of objects being built
#// (either DLL or EXE)
#//
#//------------------------------------------------------------------------
!if "$(MAKE_OBJ_TYPE)" == "EXE"
CFLAGS=$(EXE_CFLAGS) $(CFLAGS)
LFLAGS=$(EXE_LFLAGS) $(LFLAGS)
OS_LIBS=$(EXE_LIBS) $(OS_LIBS)
!endif

!if "$(MAKE_OBJ_TYPE)" == "DLL"
CFLAGS=$(DLL_CFLAGS) $(CFLAGS)
LFLAGS=$(DLL_LFLAGS) $(LFLAGS)
OS_LIBS=$(DLL_LIBS) $(OS_LIBS)
!endif

#//------------------------------------------------------------------------
#//
#// Prepend the "object directory" to any public make variables.
#//    PDBFILE - File containing debug info
#//    RESFILE - Compiled resource file
#//    MAPFILE - MAP file for an executable
#//
#//------------------------------------------------------------------------
!ifdef PBDFILE
PDBFILE=.\$(OBJDIR)\$(PDBFILE)
!else
PDBFILE=NONE
!endif
!ifdef RESFILE
RESFILE=.\$(OBJDIR)\$(RESFILE)
!endif
!ifdef MAPFILE
MAPFILE=.\$(OBJDIR)\$(MAPFILE)
!endif

!ifdef DIRS
#//------------------------------------------------------------------------
#//
#// Rule to recursively make all subdirectories specified by the DIRS target
#//
#//------------------------------------------------------------------------
$(DIRS)::
    echo +++ MAKE: Entering directory: $@
	cd $@
	nmake /nologo /f makefile.win
    cd ..

!endif # DIRS

!if defined(INSTALL_FILE_LIST) && defined(INSTALL_DIR)
#//------------------------------------------------------------------------
#//
#// Rule to install the files specified by the INSTALL_FILE_LIST variable
#// into the directory specified by the INSTALL_DIR variable
#//
#//------------------------------------------------------------------------
INSTALL_FILES: $(INSTALL_FILE_LIST)
    !$(MAKE_INSTALL) $** $(INSTALL_DIR)

!endif # INSTALL_FILES

#//------------------------------------------------------------------------
#//
#// Global rules...
#//
#//------------------------------------------------------------------------

#//
#// Set the MAKE_ARGS variable to indicate the target being built...  This is used
#// when processing subdirectories via the $(DIRS) rule
#//
clean:: 
	echo +++ MAKE: clean
	set MAKE_ARGS=$@

clobber:: 
	echo +++ MAKE: clobber
	set MAKE_ARGS=$@

clobber_all:: 
	echo +++ MAKE: clobber_all
	set MAKE_ARGS=$@

export:: 
	echo +++ MAKE: export
	set MAKE_ARGS=$@

install:: 
	echo +++ MAKE: install
	set MAKE_ARGS=$@

mangle:: 
	echo +++ MAKE: mangle
	set MAKE_ARGS=$@

unmangle:: 
	echo +++ MAKE: unmangle
	set MAKE_ARGS=$@

depend:: 
	echo +++ MAKE: depend
	set MAKE_ARGS=$@



clean:: $(DIRS)
    -$(RM) $(OBJS) $(NOSUCHFILE) NUL 2> NUL

clobber:: $(DIRS)
    -$(RM_R) $(GARBAGE) $(OBJDIR) 2> NUL

clobber_all:: $(DIRS)
    -$(RM_R) *.OBJ $(OBJS) $(TARGETS) $(GARBAGE) 2> NUL

export:: $(DIRS)

install:: $(DIRS)

mangle:: $(DIRS)
    $(MAKE_MANGLE)

unmangle:: $(DIRS)
    -$(MAKE_UNMANGLE)

depend:: $(OBJS)
    echo +++ MAKE: Creating dependancy file...  
    echo.
    -del $(DEPFILE)
    !@$(CC) -E $(CFLAGS) $(**B).c | $(AWK) -f $(DEPTH)\config\mkdepend.awk -v FILE=$(**B) >> $(DEPFILE)



#//------------------------------------------------------------------------
#//
#// Rule to create the object directory (if necessary)
#//
#//------------------------------------------------------------------------
$(OBJDIR):
	echo +++ MAKE: Creating directory: $(OBJDIR)
	echo.
    -mkdir $(OBJDIR)


#//------------------------------------------------------------------------
#//
#// Include the makefile for building the various targets...
#//
#//------------------------------------------------------------------------
include <$(DEPTH)/config/obj.inc>
include <$(DEPTH)/config/exe.inc>
include <$(DEPTH)/config/dll.inc>
include <$(DEPTH)/config/lib.inc>
include <$(DEPTH)/config/java.inc>

!endif # CONFIG_RULES_MAK

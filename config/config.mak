!if !defined(CONFIG_CONFIG_MAK)
CONFIG_CONFIG_MAK=1

#//------------------------------------------------------------------------
#//
#// If the type of build has not been specified, (ie. 16 or 32 bit) then
#// set a default.  Currently, the default is to perform a Win32 build.
#//
#//------------------------------------------------------------------------
!ifndef OS_RELEASE
OS_RELEASE=32
!endif

#//------------------------------------------------------------------------
#//
#// Define public make variables:
#//
#//    OBJDIR  - Specifies the location of intermediate files (ie. objs...)
#//              Currently, the names are WINxx_O.OBJ or WINxx_D.OBJ for
#//              optimized and debug builds respectively.
#//
#//    DIST    - Specifies the location of the distribution directory where
#//              all targets are delivered.
#//
#//    CFGFILE - Specifies the name of the temporary configuration file 
#//              containing the arguments to the current command.
#//
#//    INCS    - Default include paths.
#//
#//    CFLAGS  - Default compiler options.
#//
#//    LFLAGS  - Default linker options.
#//
#//------------------------------------------------------------------------
!if "$(OS_RELEASE)" == "16"
!if "$(MAKE_OBJ_TYPE)" == "DLL"
OBJTYPE=D
!else
OBJTYPE=E
!endif

!else
OBJTYPE=
!endif

!ifdef BUILD_OPT
OBJDIR=WIN$(OS_RELEASE)$(OBJTYPE)_O.OBJ
DIST=$(DEPTH)\dist\WIN$(OS_RELEASE)_O.OBJ
!else
OBJDIR=WIN$(OS_RELEASE)$(OBJTYPE)_D.OBJ
DIST=$(DEPTH)\dist\WIN$(OS_RELEASE)_D.OBJ
!endif

CFGFILE=$(OBJDIR)\cmd.cfg
DEPFILE=depend$(OS_RELEASE).mk
INCS=$(LINCS) $(INCS) -I$(DIST)\include -I$(DIST)\include\nspr -I\ns\include -I$(DEPTH)\include
!if "$(OS_RELEASE)" == "16"
CFLAGS=-DMOCHA $(OS_CFLAGS) $(LCFLAGS) $(INCS)
!else
CFLAGS=-DJAVA -DMOCHA $(OS_CFLAGS) $(LCFLAGS) $(INCS)
!endif
LFLAGS=$(OS_LFLAGS) $(LLFLAGS)

#//------------------------------------------------------------------------
#//
#// Include the OS dependent configuration information
#//
#//------------------------------------------------------------------------
include <$(DEPTH)/config/WIN$(OS_RELEASE)>

#//------------------------------------------------------------------------
#//
#// Define the global make commands.
#//
#//    MAKE_INSTALL  - Copy a target to the distribution directory.
#//
#//    MAKE_OBJDIRS  - Create an object directory (if necessary).
#//
#//    MAKE_JAVA_CLASSES - Compile all .java files
#//
#//    MAKE_JAVA_HEADERS - Create header files for java classes
#//
#//    MAKE_JAVA_STUBS   - Create stub .c files for java classes
#//
#//     MAKE_MANGLE   - Convert all long filenames into 8.3 names
#//
#//     MAKE_UNMANGLE - Restore all long filenames
#//
#//------------------------------------------------------------------------
!if !defined(MOZ_SRC)
#enable builds on any drive if defined.
MOZ_SRC=y:
!endif
MAKE_INSTALL=$(DEPTH)\config\makecopy.exe
MAKE_MANGLE=$(DEPTH)\config\mangle.exe
MAKE_UNMANGLE=if exist unmangle.bat call unmangle.bat
MAKE_JAVA_CLASSES=$(JAVA) -classpath $(MOZ_SRC)\javac.zip -ms8m sun.tools.javac.Main
MAKE_JAVA_HEADERS=$(JAVAH) -classpath $(BOOT_CLASSPATH)
MAKE_JAVA_STUBS=$(JAVAH) -classpath $(BOOT_CLASSPATH) -stubs
MAKE_JAVA_LIBSTUBS=$(JAVAH) -o libstubs.c -classpath $(BOOT_CLASSPATH) -stubs

#//------------------------------------------------------------------------
#//
#// Common Libraries
#//
#//------------------------------------------------------------------------
LIBNSPR=$(DIST)\lib\pr$(OS_RELEASE)301.lib

!endif # CONFIG_CONFIG_MAK

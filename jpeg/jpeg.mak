# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=jpeg - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to jpeg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "jpeg - Win32 Debug" && "$(CFG)" != "jpeg - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "jpeg.mak" CFG="jpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "jpeg - Win32 Release"
CPP=cl.exe

!IF  "$(CFG)" == "jpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\jpeg.lib"

CLEAN : 
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdatadst.obj"
	-@erase "$(INTDIR)\jdatasrc.obj"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jmemansi.obj"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\jpeg.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /Gy /D "_DEBUG" /D "DEBUG" /D "JAVA" /D "JAVA_WIN32" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "NEW_BOOKMARKS" /D "MOCHA" /D "_AFXDLL" /D "_MBCS" /D "MSVC4" /YX /c
# SUBTRACT CPP /Gf /Fr
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /Gy /D "_DEBUG" /D "DEBUG" /D\
 "JAVA" /D "JAVA_WIN32" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D\
 "_WINDOWS" /D "_X86_" /D "NEW_BOOKMARKS" /D "MOCHA" /D "_AFXDLL" /D "_MBCS" /D\
 "MSVC4" /Fp"$(INTDIR)/jpeg.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/jpeg.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/jpeg.lib" 
LIB32_OBJS= \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdatadst.obj" \
	"$(INTDIR)\jdatasrc.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemansi.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj"

"$(OUTDIR)\jpeg.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "jpeg - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "jpeg___W"
# PROP BASE Intermediate_Dir "jpeg___W"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\jpeg.lib"

CLEAN : 
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdatadst.obj"
	-@erase "$(INTDIR)\jdatasrc.obj"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jmemansi.obj"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(OUTDIR)\jpeg.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /Gy /D "_DEBUG" /D "DEBUG" /D "EDITOR" /D "JAVA" /D "JAVA_WIN32" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "EXPORT_VERSION" /D "_X86_" /D "NEW_BOOKMARKS" /D "MOCHA" /D "_AFXDLL" /D "_MBCS" /YX /c
# SUBTRACT BASE CPP /Gf /Fr
# ADD CPP /nologo /MD /W3 /GX /Ox /Og /Oi /Gy /D "NDEBUG" /D "JAVA" /D "JAVA_WIN32" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D "NEW_BOOKMARKS" /D "MOCHA" /D "_AFXDLL" /D "_MBCS" /D "MSVC4" /YX /c
# SUBTRACT CPP /Gf /Fr
CPP_PROJ=/nologo /MD /W3 /GX /Ox /Og /Oi /Gy /D "NDEBUG" /D "JAVA" /D\
 "JAVA_WIN32" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D\
 "_X86_" /D "NEW_BOOKMARKS" /D "MOCHA" /D "_AFXDLL" /D "_MBCS" /D "MSVC4"\
 /Fp"$(INTDIR)/jpeg.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/jpeg.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/jpeg.lib" 
LIB32_OBJS= \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdatadst.obj" \
	"$(INTDIR)\jdatasrc.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemansi.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj"

"$(OUTDIR)\jpeg.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "jpeg - Win32 Debug"
# Name "jpeg - Win32 Release"

!IF  "$(CFG)" == "jpeg - Win32 Debug"

!ELSEIF  "$(CFG)" == "jpeg - Win32 Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\jutils.c
DEP_CPP_JUTIL=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jutils.obj" : $(SOURCE) $(DEP_CPP_JUTIL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jcomapi.c
DEP_CPP_JCOMA=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jcomapi.obj" : $(SOURCE) $(DEP_CPP_JCOMA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdapimin.c
DEP_CPP_JDAPI=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdapimin.obj" : $(SOURCE) $(DEP_CPP_JDAPI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdapistd.c
DEP_CPP_JDAPIS=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdapistd.obj" : $(SOURCE) $(DEP_CPP_JDAPIS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdatadst.c
DEP_CPP_JDATA=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdatadst.obj" : $(SOURCE) $(DEP_CPP_JDATA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdatasrc.c
DEP_CPP_JDATAS=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdatasrc.obj" : $(SOURCE) $(DEP_CPP_JDATAS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdcoefct.c
DEP_CPP_JDCOE=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdcoefct.obj" : $(SOURCE) $(DEP_CPP_JDCOE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdcolor.c
DEP_CPP_JDCOL=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdcolor.obj" : $(SOURCE) $(DEP_CPP_JDCOL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jddctmgr.c
DEP_CPP_JDDCT=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jddctmgr.obj" : $(SOURCE) $(DEP_CPP_JDDCT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdhuff.c
DEP_CPP_JDHUF=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdhuff.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdhuff.obj" : $(SOURCE) $(DEP_CPP_JDHUF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdinput.c
DEP_CPP_JDINP=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdinput.obj" : $(SOURCE) $(DEP_CPP_JDINP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdmainct.c
DEP_CPP_JDMAI=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdmainct.obj" : $(SOURCE) $(DEP_CPP_JDMAI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdmarker.c
DEP_CPP_JDMAR=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdmarker.obj" : $(SOURCE) $(DEP_CPP_JDMAR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdmaster.c
DEP_CPP_JDMAS=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdmaster.obj" : $(SOURCE) $(DEP_CPP_JDMAS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdmerge.c
DEP_CPP_JDMER=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdmerge.obj" : $(SOURCE) $(DEP_CPP_JDMER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdphuff.c
DEP_CPP_JDPHU=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdhuff.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdphuff.obj" : $(SOURCE) $(DEP_CPP_JDPHU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdpostct.c
DEP_CPP_JDPOS=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdpostct.obj" : $(SOURCE) $(DEP_CPP_JDPOS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jdsample.c
DEP_CPP_JDSAM=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jdsample.obj" : $(SOURCE) $(DEP_CPP_JDSAM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jerror.c
DEP_CPP_JERRO=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jversion.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	
NODEP_CPP_JERRO=\
	".\xp_trace.h"\
	

"$(INTDIR)\jerror.obj" : $(SOURCE) $(DEP_CPP_JERRO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jfdctflt.c
DEP_CPP_JFDCT=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jfdctflt.obj" : $(SOURCE) $(DEP_CPP_JFDCT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jfdctfst.c
DEP_CPP_JFDCTF=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jfdctfst.obj" : $(SOURCE) $(DEP_CPP_JFDCTF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jfdctint.c
DEP_CPP_JFDCTI=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jfdctint.obj" : $(SOURCE) $(DEP_CPP_JFDCTI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jidctflt.c
DEP_CPP_JIDCT=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jidctflt.obj" : $(SOURCE) $(DEP_CPP_JIDCT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jidctfst.c
DEP_CPP_JIDCTF=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jidctfst.obj" : $(SOURCE) $(DEP_CPP_JIDCTF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jidctint.c
DEP_CPP_JIDCTI=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jidctint.obj" : $(SOURCE) $(DEP_CPP_JIDCTI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jidctred.c
DEP_CPP_JIDCTR=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jdct.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jidctred.obj" : $(SOURCE) $(DEP_CPP_JIDCTR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jmemansi.c
DEP_CPP_JMEMA=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmemsys.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jmemansi.obj" : $(SOURCE) $(DEP_CPP_JMEMA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jmemmgr.c
DEP_CPP_JMEMM=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmemsys.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jmemmgr.obj" : $(SOURCE) $(DEP_CPP_JMEMM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jquant1.c
DEP_CPP_JQUAN=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jquant1.obj" : $(SOURCE) $(DEP_CPP_JQUAN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\jquant2.c
DEP_CPP_JQUANT=\
	".\jconfig-mac-cw.h"\
	".\jconfig.h"\
	".\jerror.h"\
	".\jinclude.h"\
	".\jmorecfg.h"\
	".\jpegint.h"\
	".\jpeglib.h"\
	".\jwinfig.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\jquant2.obj" : $(SOURCE) $(DEP_CPP_JQUANT) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################

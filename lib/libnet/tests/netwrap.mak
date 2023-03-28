# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Debug" && "$(CFG)" != "Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "netwrap.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
CPP=cl.exe

!IF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Lib32_De"
# PROP BASE Intermediate_Dir "Lib32_De"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WINNT3.51_DBG.OBJ"
# PROP Intermediate_Dir "WINNT3.51_DBG.OBJ"
OUTDIR=.\WINNT3.51_DBG.OBJ
INTDIR=.\WINNT3.51_DBG.OBJ

ALL : $(OUTDIR)/netwrap.lib $(OUTDIR)/netwrap.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /Z7 /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Z7 /YX /Od /I "..\include" /I "..\..\..\include" /I "..\..\..\dist\WINNT3.51_DBG.OBJ\include" /I "..\..\..\dist\WINNT3.51_DBG.OBJ\include\nspr" /D "_DEBUG" /D "DEBUG" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "EXPORT_VERSION" /D "US_VERSION" /D "_X86_" /D "_CONSOLE" /D "_MBCS" /D "LW_WRAPPER" /D "LIVEWIRE" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Z7 /YX /Od /I "..\include" /I "..\..\..\include"\
 /I "..\..\..\dist\WINNT3.51_DBG.OBJ\include" /I\
 "..\..\..\dist\WINNT3.51_DBG.OBJ\include\nspr" /D "_DEBUG" /D "DEBUG" /D\
 "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "EXPORT_VERSION"\
 /D "US_VERSION" /D "_X86_" /D "_CONSOLE" /D "_MBCS" /D "LW_WRAPPER" /D\
 "LIVEWIRE" /FR$(INTDIR)/ /Fp$(OUTDIR)/"netwrap.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WINNT3.51_DBG.OBJ/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"netwrap.bsc" 
BSC32_SBRS= \
	$(INTDIR)/cfile.sbr \
	$(INTDIR)/xpfile.sbr \
	$(INTDIR)/testnet.sbr

$(OUTDIR)/netwrap.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=lib.exe
# ADD BASE LIB32 /NOLOGO
# ADD LIB32 /NOLOGO
LIB32_FLAGS=/NOLOGO /OUT:$(OUTDIR)\"netwrap.lib" 
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
	$(INTDIR)/cfile.obj \
	$(INTDIR)/xpfile.obj \
	$(INTDIR)/testnet.obj

$(OUTDIR)/netwrap.lib : $(OUTDIR)  $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Lib32_Re"
# PROP BASE Intermediate_Dir "Lib32_Re"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WINNT3.51_OPT.OBJ"
# PROP Intermediate_Dir "WINNT3.51_OPT.OBJ"
OUTDIR=.\WINNT3.51_OPT.OBJ
INTDIR=.\WINNT3.51_OPT.OBJ

ALL : $(OUTDIR)/netwrap.lib $(OUTDIR)/netwrap.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /YX /O2 /I "..\include" /I "..\..\..\include" /I "..\..\..\dist\WINNT3.51_OPT.OBJ\include" /I "..\..\..\dist\WINNT3.51_OPT.OBJ\include\nspr" /D "NDEBUG" /D "_DEBUG" /D "DEBUG" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D "EXPORT_VERSION" /D "US_VERSION" /D "_X86_" /D "_CONSOLE" /D "_MBCS" /D "LW_WRAPPER" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /YX /O2 /I "..\include" /I "..\..\..\include" /I\
 "..\..\..\dist\WINNT3.51_OPT.OBJ\include" /I\
 "..\..\..\dist\WINNT3.51_OPT.OBJ\include\nspr" /D "NDEBUG" /D "_DEBUG" /D\
 "DEBUG" /D "HW_THREADS" /D "XP_PC" /D "x386" /D "WIN32" /D "_WINDOWS" /D\
 "EXPORT_VERSION" /D "US_VERSION" /D "_X86_" /D "_CONSOLE" /D "_MBCS" /D\
 "LW_WRAPPER" /FR$(INTDIR)/ /Fp$(OUTDIR)/"netwrap.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WINNT3.51_OPT.OBJ/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"netwrap.bsc" 
BSC32_SBRS= \
	$(INTDIR)/cfile.sbr \
	$(INTDIR)/xpfile.sbr \
	$(INTDIR)/testnet.sbr

$(OUTDIR)/netwrap.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=lib.exe
# ADD BASE LIB32 /NOLOGO
# ADD LIB32 /NOLOGO
LIB32_FLAGS=/NOLOGO /OUT:$(OUTDIR)\"netwrap.lib" 
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
	$(INTDIR)/cfile.obj \
	$(INTDIR)/xpfile.obj \
	$(INTDIR)/testnet.obj

$(OUTDIR)/netwrap.lib : $(OUTDIR)  $(DEF_FILE) $(LIB32_OBJS)
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

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\cfile.c
DEP_CFILE=\
	.\private.h

$(INTDIR)/cfile.obj :  $(SOURCE)  $(DEP_CFILE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\xpfile.c

$(INTDIR)/xpfile.obj :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\testnet.c
DEP_TESTN=\
	\ns\include\xp.h

$(INTDIR)/testnet.obj :  $(SOURCE)  $(DEP_TESTN) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################

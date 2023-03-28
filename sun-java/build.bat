@echo off
rem --------------------------------------------------------------------------
rem 
rem This batch file is used to build the NS tree for WIN32.
rem
rem --------------------------------------------------------------------------
rem
rem Usage:
rem     build (16 | 32) {all | 'component'} {export | clean} 
rem     build (16 | 32) release
rem
rem --------------------------------------------------------------------------
rem
rem Set up global variables
rem
rem --------------------------------------------------------------------------
rem Allow builds outside of y: drive only.
@if "%MOZ_SRC%"=="" set MOZ_SRC=y:
@set ROOTDIR=%MOZ_SRC%\ns\sun-java
@set ITEM=%2
@set OS_RELEASE=%1
rem --------------------------------------------------------------------------
rem
rem Make sure user specified which version to build
rem
rem --------------------------------------------------------------------------
@if %1==16 goto prepare
@if %1==32 goto prepare
@echo You must specify whether you want a 16 or 32 bit build.
@goto done

:prepare

rem --------------------------------------------------------------------------
rem
rem Prepare for a release build...
rem
rem --------------------------------------------------------------------------
@if not %ITEM%==release goto debug
set DIST=%MOZ_SRC%\ns\dist\WIN%1_O.OBJ
set MAKE_OPTS="BUILD_OPT=TRUE OS_RELEASE=%1" export
set ITEM=all
goto create_dirs

rem --------------------------------------------------------------------------
rem
rem Prepare for a debug build...
rem
rem --------------------------------------------------------------------------
:debug
set DIST=%MOZ_SRC%\ns\dist\WIN%1_D.OBJ
set MAKE_OPTS="OS_RELEASE=%1"

@if %3==clobber goto start
rem --------------------------------------------------------------------------
rem
rem Create the distribution directories
rem
rem --------------------------------------------------------------------------
:create_dirs
@if not exist \ns\dist                  @mkdir \ns\dist
@if not exist %DIST%                    @mkdir %DIST%
@if not exist %DIST%\bin                @mkdir %DIST%\bin
@if not exist %DIST%\lib                @mkdir %DIST%\lib
@if not exist %DIST%\classes            @mkdir %DIST%\classes
@if not exist %DIST%\include            @mkdir %DIST%\include
@if not exist %DIST%\include\nspr       @mkdir %DIST%\include\nspr
@if not exist %DIST%\include\sun-java   @mkdir %DIST%\include\sun-java

rem --------------------------------------------------------------------------
rem
rem Start the build process...
rem
rem --------------------------------------------------------------------------
:start
@goto %ITEM%

:all

rem --------------------------------------------------------------------------
rem 
rem Build the NSPR library
rem
rem --------------------------------------------------------------------------
:nspr
@set CURRENT=nspr
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %ROOTDIR%\..\nspr\include
@set CURRENT=nspr-include
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%\..\nspr\src
@set CURRENT=nspr-src
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done
@if %1==16 goto mocha

rem --------------------------------------------------------------------------
rem
rem Build the JAVAH executable
rem
rem --------------------------------------------------------------------------
:javah
@set CURRENT=JAVAH
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Generate the INCLUDE files for java native methods and deliver ALL
rem header files to $(DIST)\include\sun-java\
rem
rem --------------------------------------------------------------------------
:include
@set CURRENT=INCLUDE
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error

@set CURRENT=MD-INCLUDE
@cd %ROOTDIR%\md-include
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the java RUNTIME library
rem
rem --------------------------------------------------------------------------
:runtime
@set CURRENT=RUNTIME
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the Machine Dependent java library
rem
rem --------------------------------------------------------------------------
:md
@set CURRENT=MD
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the Netscape JAVA DLL
rem
rem --------------------------------------------------------------------------
:nsjava
@set CURRENT=NSJAVA
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the command-line JAVA executable
rem
rem --------------------------------------------------------------------------
:java
@set CURRENT=JAVA
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %CURRENT%
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the Netscape NET and APPLET libraries
rem
rem --------------------------------------------------------------------------
:netscape
@set CURRENT=NETSCAPE
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd netscape
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the MOCHA library
rem
rem --------------------------------------------------------------------------
:mocha
@set CURRENT=mocha
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd %ROOTDIR%\..\mocha
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done
@if %1==16 goto done

rem --------------------------------------------------------------------------
rem
rem Build the Sun AWT windowing DLL
rem
rem --------------------------------------------------------------------------
:awt
@set CURRENT=AWT
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd awt\windows
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@cd awt\win
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

rem --------------------------------------------------------------------------
rem
rem Build the Sun MMEDIA audio DLL
rem
rem --------------------------------------------------------------------------
:mmedia
@set CURRENT=MMEDIA
@echo ++++++++++ Building %CURRENT% +++++++++++
@echo.
@cd mmedia\windows
@nmake -nologo -f makefile.win %MAKE_OPTS% %3 %4 %5 %6
@if errorlevel 1 goto error
@cd %ROOTDIR%
@if not %ITEM%==all goto done

@goto done

:error
@echo =============== Build error in %CURRENT% ====================

:done

@echo off
REM     04-02-96 GAB
REM     Goals:
REM             written to work under 95
REM             minimalist effort for others to build a dist
REM             not a finely tuned working tool unless you just want a complete dist

REM
REM     Validate arguments.
REM     Give usage if invalid.
REM
:arg1
if "%1"=="16" goto arg2
if "%1"=="32" goto arg2
goto usage
:arg2
if "%2"=="all" goto start
if "%2"=="debug" goto start
if "%2"=="release" goto start
goto usage
:usage
echo Usage:
echo build95 32 or 16  debug or release  makefileargs
goto end

REM
REM     Argument validation is complete.
REM     Expand all builds
REM
:start
if "%2"=="all" call build95 %1 debug %3 %4 %5 %6 %7 %8 %9
if "%2"=="all" call build95 %1 release %3 %4 %5 %6 %7 %8 %9
if "%2"=="all" goto end

REM
REM     Set up environment vairables.
REM
if "%MOZ_SRC%"=="" set MOZ_SRC=y:
set ROOTDIR=%MOZ_SRC%\ns\sun-java
set OS_RELEASE=%1
set VERSION=%2
if "%VERSION%"=="release" set DIST=%MOZ_SRC%\ns\dist\win%OS_RELEASE%_o.obj
if "%VERSION%"=="debug" set DIST=%MOZ_SRC%\ns\dist\win%OS_RELEASE%_d.obj
if "%VERSION%"=="release" set BUILD_OPT=TRUE
if "%VERSION%"=="debug" set BUILD_OPT=
set WINDOWS95=
if NOT "%OS%"=="Windows_NT" set WINDOWS95=YES

REM
REM     Set up the directory structure.
REM
mkdir %MOZ_SRC%\ns\dist
mkdir %DIST%
mkdir %DIST%\bin
mkdir %DIST%\lib
mkdir %DIST%\classes
mkdir %DIST%\include
mkdir %DIST%\include\nspr
mkdir %DIST%\include\sun-java

REM
REM     Get the drive and directory right.
REM
%MOZ_SRC%
cd %ROOTDIR%

REM
REM     Build NSPR
REM
cd %ROOTDIR%\..\nspr\include
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

cd %ROOTDIR%\..\nspr\src
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Win16, skip to mocha.
REM
if "%OS_RELEASE%"=="16" goto mocha

REM
REM     Build JAVAH
REM
cd %ROOTDIR%\javah
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build INCLUDE stuff
REM
cd %ROOTDIR%\include
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

cd %ROOTDIR%\md-include
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build the java RUNTIME
REM
cd %ROOTDIR%\runtime
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build machine dependent java stuff
REM
cd %ROOTDIR%\md
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build java DLL
REM
cd %ROOTDIR%\nsjava
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build command line java exe
REM
cd %ROOTDIR%\java
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build netscape net and applet libs
REM
cd %ROOTDIR%\netscape
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build MOCHA
REM
:mocha
cd %ROOTDIR%\..\mocha
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Win16, that's it.
REM
if "%OS_RELEASE%"=="16" goto end

REM
REM     Build sun awt dll
REM
cd %ROOTDIR%\awt\windows
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build Netscape awt dll
REM
cd %ROOTDIR%\awt\win
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     Build sun mmedia audio dll
REM
cd %ROOTDIR%\mmedia\windows
nmake -nologo -f makefile.win %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 goto err

REM
REM     We're finished.
REM
goto end

REM
REM     We've erred.
REM
:err
echo Build failure.  Get expert help soon.

REM
REM     We're done.
REM
:end
cd %ROOTDIR%

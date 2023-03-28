echo off
REM This file invokes awk with the supplied arguments
REM It is provided on NT just to overcome some gmake/awk
REM problems.

del %3
awk -f %1 %2 > %3


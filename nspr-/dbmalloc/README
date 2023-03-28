/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (uunet!virtech!cpcahil).  
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */

This package is a collection of routines which are a drop-in replacement
for the malloc(3), memory(3), string(3), and bstring(3) library functions.
These replacement modules are different from the original modules in that
they provide a full set of debugging features which detect malloc 
memory overruns and other types of misuse.

The software has been developed under ISC UNIX and ported to several other
UNIX architectures including HP-UX, SunOS, and AIX.  You should be able 
to get it to run out of the box on most UNIX systems.  If not, see the PROBLEMS
file for some help.  

Virtual Technologies, Inc also supports a commercial version of the library 
which includes more features, better error reporting, better performance,
no need to recompile the code to be verified, professional documentation,
and support.  The commercial version is available on a growing range of
architectures and is reasonably priced.

If you would like more info on the commercial version, you can send email
to info@virtech.vti.com.

--------------------------------------------------------------------------
PORTING NOTES:

0. Short cut:  just run "make runtests".  This will configure and compile
   the library and compile and run all of the tests.  Otherwise, you can
   follow the following steps:

1. Edit the makefile and set the appropriate flags for the compilations
   See note about configure and setting special flags in the next step.

2. Type make malloc.h  - which will run the Configure script to generate
   the appropriate entries in malloc.h file for this compiler/OS.
   If the configure script has problems, then you will probably have to 
   configure the malloc.h file by hand.  Look in the PROBLEMS file
   for more info.

   NOTE: Configure looks around for all of the compilers you have on the
   system and attempts to setup a single malloc.h that can be included by
   any of them.  Configure assumes that the compilers can be called without
   any special flags for a basic compile.  If this is not the case you
   will have to modify the Config.flags file and add a line with the
   compiler name and special flags. 

   For example, if you wanted to use the -Aa flag on HP/UX cc (strict 
   ANSI mode - which also would require -D_POSIX_SOURCE to compile the
   malloc library), you would have to add the line: "cc -Aa -D_POSIX_SOURCE"
   to Config.flags (and add -Aa -D_POSIX_SOURCE to the CFLAGS definition
   in the Makefile).

   REMEMBER, if you do this, you may get error message if the malloc.h
   file is included in compiles that do not include the specified flags.
   If you expect different compilation environments with the same compiler,
   you might want to have different versions of the debugging malloc.h file
   for each environment.

   NOTE: Configure will automatically generate two configurations for
   gcc if it finds it (one for normal gcc and one for gcc -traditional)
   so that the library can be used in either mode.

   NOTE: real programmers don't need any fancy Configure script, they can 
   configure the utility by hand (and so bypass this step). :-}

   PROBLEMS: if you find a problem with the configure script, you can do
   one of the following:

	1. give up
	2. report problem and give up
	3. fix problem, send fix to me and continue
	4. bypass the automatic configuration (see the PROBLEMS file for
	   info on how to do this).

   Since I have ensured that this works on all the environments that I 
   have access to, I will not be able to fix a problem that you have 
   on your OS/compiler combination unless you give me extremely good 
   details about the problem including:
	
	1. Hardware & OS
	2. compiler
	3. compilation environment (i.e. on suns you can run in a Sys V
	   environment or a BSD environment - this is important)
	4. any flags used for compiling
	5. output from "make runtests"
	6. any pertinent files - which *should* include the following:
	
		/usr/include/memory.h (if it exists)
		/usr/include/malloc.h
		/usr/include/string.h
		malloc.h (if it got generated)
		.configure
		.configure.s
		

   With this info I will probably be able to *guess* at the source of
   the problem and send you back a potential fix.  However, without access
   to such an architecture, I may not be able to fix the problem. 

3. Type make (See the PROBLEMS file if you have a problem compiling)

4. Type make runtests to build and runt the tests (some of which will take
   a long time and some which will abort).  See the notes below for 
   more info on the tests and the expected output

5. Install libdbmalloc.a and malloc.h somewhere where users can get
   to them by using make install (make sure INSTDIR is set correctly
   in the makefile)

6. If the system memory routines are not compatible with the ones expected
   by the malloclib (this is the case if the configure script doesn't create
   a datam[cs].O file) and if you are good with assembly, and if you have
   nothing better to do, you could put together an asm module for either, or
   both of the functions (as I have done for the DataMC routine on 386 
   systems).

   These two functions are typically the bottle neck when running the debugging
   library and I have found that a few moments spent on carefully implementing
   these routines will vastly increase the performance of the library.

   If you do start upon this mission, make sure that you maintain the 
   functionality present in the portable versions of the routines.  This is
   especially important with the DataMC routine, which must function like
   memcpy plus handle overlapping moves (like memmove).

   Once you have it working (and it passes the tests) send it in and I will 
   include it in the baseline.

--------------------------------------------------------------------------
USAGE:

To use the library, include the malloc.h header included with this
library ( this is not entirely necessary, but the identification of where
the problems originated will be much better if it is), compile the 
object files, and link with the libdbmalloc.a (make sure it is before
the libc.a on the link command - this is the default if you don't specifically
include libc.a on your link).

For more info on how to use the library, see the man page in
malloc.3 ([nt]roff source) or malloc.man (already nroffed).

--------------------------------------------------------------------------
TEST PROGRAMS

The library comes with several test programs that should be run before
it is installed on the system.  The tests are run automatically by the
Runtests shell (which is invoked by make runtests).  Runtests will attempt
to automatically determine if the library passes all of the tests and
will save all of its output in the file Runtests.out.  If a failure is 
reported, you should look at Runtests.out for more info on where the failure
occurred.

The following test are run by the script:

testmalloc - this program runs through zillions of mallocs/reallocs and frees
in order to exercise the malloc system.  The output from this program will
look something like the following (NOTE that if run from the Runtests script,
it will only run for 50000 iterations):

	MAXITER 1000000 MAXOBJS 1000 BIGOBJ 90000, TINYOBJ 80, nbig/ntiny 1/100
	Memory use at start: 20140 bytes
	Starting the test...
	0 iterations
	10000 iterations
	20000 iterations
	....(lots of iterations skipped)....
	980000 iterations
	990000 iterations

	Did 1000000 iterations, 999 objects, 515733 mallocs, 484267 reallocs
	Memory use at end: 1617580 bytes
	Memory use after free: 1617580 bytes

	************************************************************...
				     FREE     FREE                 ...
	  PTR      NEXT     PREV     NEXT     PREV      FLAGS     ...
	-------- -------- -------- -------- -------- ---------- -...
	0x403C30 0x407510 0x000000 0x000000 0x000000 0x00000000  ...
	0x407510 0x407640 0x403C30 0x000000 0x000000 0x03156111  ...
	0x407640 0x000000 0x407510 0x000000 0x000000 0x03156100  ...
	Malloc start:      0x403C30
	Malloc end:        0x407640
	Malloc data start: 0x407510
	Malloc data end:   0x58F510
	Malloc free list:  0x407640

	startsize 20140 != endsize 1617580

This is normal output.  If it doesn't get to the expected number of 
iterations (50000 if run by runtest, or 1000000 if run from the command
line with no other arguments) or if there are any allocations still in
effect that were initiated by testmalloc.c, something went wrong (the
library didn't pass the test).  Don't worry about having exact matches with
the numbers listed above and don't worry that startsize != endsize.

Note that there may be several entries in the malloc chain if the C startup
function has allocated data before the main gets called.  The key is that
there are no allocated entries (those with the 1 bit on in the flags field)
left that are identified as being allocated within the testmalloc.c file.
In the example above, the middle entry does in fact have the 1 bit on, but
in the info that was cut-off, there was no indication that it was allocated
within testmalloc.c, therefore it must have been allocated somewhere else.
The last line in the chain will usually say it was allocated within
testmalloc.c; however, as in this case, it will also indicate that it is
not in use (the 1 bit will be off in the flags field).

Most likely, if the program ran to completion (processed 1000000 iterations),
the test was successful.

Also note that this program performs many small and large allocations and
may take a long time to run.  The run time for various architectures (all
done under fairly quiet system utilization):

	486/33 with cc under ISC UNIX 3.0:		1:17 (1 minute, 17 secs)
	HP9000/857 with cc under HP/UX 8.0:		0:59
	RS6000/320 with cc under AIX ?3.1?:		3:47
	ATT 3B2/600 with cc under SVR4.1ES:	       16:59

If this test takes too long for you, you can speed it up by setting 
MALLOC_FILLAREA equal to zero.  This has the following effects on the 
time it takes to run the tests:

	486/33 with cc under ISC UNIX 3.0:		0:45.8 (45 seconds)
	HP9000/857 with cc under HP/UX 8.0:		0:33.4
	RS6000/320 with cc under AIX ?3.1?:		3:47
	ATT 3B2/600 with cc under SVR4.1ES:	        7:04.2

If it is still too slow, you can add an argument to the program specifying
the number of iterations.  The default is 1,000,000. 

One thing to think about before making a change to increase the performance
of this test is that maintaining dynamic memory (along wit the additional
debugging code that has been added) is a somewhat complex piece of software.
It should be well tested before you place it into a user environment.

testmem - this program tests many of the string and memory functions to verify
that they work as they are supposed to.  You should get no output from this
test.  If you do, something is wrong.

testerr - this program tests several malloc related program errors (to make
sure that the library is correctly identifying the problems).  The output
from the test goes to stderr and should be similar to the following (although
because of machine architectures the error messages may be slightly different):

	-------------------------------------
	Error from strcpy() - out of bounds
	MALLOC Warning from strncpy() (called from testerr.c line 33):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 29.
		This was the 2nd call to malloc.

	-------------------------------------
	Error from memset() - out of bounds
	MALLOC Warning from memset() (called from testerr.c line 37):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 20 bytes in testerr.c on line 27.
		This was the 1st call to malloc.

	-------------------------------------
	Error from free() - overrun
	MALLOC Warning from free() (called from testerr.c line 41):
	Data has overrun beyond requested number of bytes
	This error is *probably* associated with the following allocation:

		A call to malloc for 20 bytes in testerr.c on line 27.
		This was the 1st call to malloc.

	MALLOC Warning from free() (called from testerr.c line 41):
	Data has overrun beyond requested number of bytes
	This error is *probably* associated with the following allocation:

		A call to malloc for 20 bytes in testerr.c on line 27.
		This was the 1st call to malloc.

	-------------------------------------
	Error from free() - double free
	MALLOC Warning from free() (called from testerr.c line 45):
	Data area is not in use (can't be freed or realloced)
	This error is *probably* associated with the following allocation:

		A call to malloc for 20 bytes in testerr.c on line 27.
		This was the 1st call to malloc.

		This block was freed on the 1st call to free()
		in testerr.c on line 41.

	-------------------------------------
	NO error from bzero
	-------------------------------------
	Error from bzero() - out of bounds
	MALLOC Warning from bzero() (called from testerr.c line 53):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 29.
		This was the 2nd call to malloc.

	-------------------------------------
	Error from free() - overrun
	MALLOC Warning from free() (called from testerr.c line 57):
	Data has overrun beyond requested number of bytes
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 29.
		This was the 2nd call to malloc.

	MALLOC Warning from free() (called from testerr.c line 57):
	Data has overrun beyond requested number of bytes
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 29.
		This was the 2nd call to malloc.

	-------------------------------------
	Error from free() - invalid magic
	MALLOC Warning from free() (called from testerr.c line 61):
	Malloc region does not have valid magic number in header
	-------------------------------------
	Error from memset() - out of bounds
	MALLOC Warning from memset() (called from testerr.c line 68):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 63.
		This was the 3rd call to malloc.

	-------------------------------------
	Error from malloc() - chain broken
	MALLOC Warning from malloc() (called from testerr.c line 72):
	Pointers between this segment and adjoining segments are invalid
	This error is *probably* associated with the following allocation:

		A call to malloc for 10 bytes in testerr.c on line 63.
		This was the 3rd call to malloc.

	MALLOC Warning from malloc() (called from testerr.c line 72):
	Malloc region does not have valid magic number in header
	MALLOC Fatal error from malloc() (called from testerr.c line 72):
	Pointer is not within malloc area

abort-core dumped (or something about SIGIOT) can be expected at the end
of the test because of a purposeful overwrite of memory.  Depending upon the
architecture, the purposeful overwrite may actually cause some other form
of core dump.  If it dumps somewhere in the last error, you can *probably*
consider it normal.

teststack - this test verifies that the stack tracking mechanism still works.
The program just uses the malloc_enter and malloc_leave macros to record a
stack while allocating variables and then dumping the list.  The output
should look something like the following:

	MALLOC Warning from strcpy() (called from teststack.c line 84):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 1 bytes in teststack.c on line 82.
		This was the 5th call to malloc.
		Stack from where allocated:
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)

	MALLOC Warning from strcpy() (called from teststack.c line 84):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 1 bytes in teststack.c on line 82.
		This was the 7th call to malloc.
		Stack from where allocated:
		 -> sub3() in teststack.c(80)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)

	MALLOC Warning from strcpy() (called from teststack.c line 84):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 1 bytes in teststack.c on line 82.
		This was the 9th call to malloc.
		Stack from where allocated:
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)

	MALLOC Warning from strcpy() (called from teststack.c line 84):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 1 bytes in teststack.c on line 82.
		This was the 13th call to malloc.
		Stack from where allocated:
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> main() in teststack.c(27)

	MALLOC Warning from strcpy() (called from teststack.c line 84):
	Pointer within malloc region, but outside of malloc data bounds
	This error is *probably* associated with the following allocation:

		A call to malloc for 1 bytes in teststack.c on line 82.
		This was the 15th call to malloc.
		Stack from where allocated:
		 -> sub3() in teststack.c(80)
		 -> main() in teststack.c(27)

	************************** Dump of Malloc Chain ******************...
	POINTER     FILE  WHERE         LINE      ALLOC        DATA     HE...
	TO DATA      ALLOCATED         NUMBER     FUNCT       LENGTH  OF B...
	-------- -------------------- ------- -------------- ------- -----...
	0x404D94 teststack.c               29 malloc(2)           10 01010...
		 -> main() in teststack.c(27)
	0x404DE8 teststack.c               48 malloc(3)            0        
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404E34 teststack.c               67 malloc(4)            0       
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404E80 teststack.c               82 malloc(5)            1 31   
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404ECC teststack.c               71 malloc(6)           10 01010...
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404F20 teststack.c               82 malloc(7)            1 31  
		 -> sub3() in teststack.c(80)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404F6C teststack.c               67 malloc(8)            0    
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x404FB8 teststack.c               82 malloc(9)            1 31
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x405004 teststack.c               71 malloc(10)          10 01010...
		 -> sub2() in teststack.c(65)
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x405058 teststack.c               56 malloc(11)          10 01010...
		 -> sub1() in teststack.c(46)
		 -> main() in teststack.c(27)
	0x4050AC teststack.c               67 malloc(12)           0   
		 -> sub2() in teststack.c(65)
		 -> main() in teststack.c(27)
	0x4050F8 teststack.c               82 malloc(13)           1 31 
		 -> sub3() in teststack.c(80)
		 -> sub2() in teststack.c(65)
		 -> main() in teststack.c(27)
	0x405144 teststack.c               71 malloc(14)          10 01010...
		 -> sub2() in teststack.c(65)
		 -> main() in teststack.c(27)
	0x405198 teststack.c               82 malloc(15)           1 31
		 -> sub3() in teststack.c(80)
		 -> main() in teststack.c(27)


The item of importance is the presence of the stack information on the
lines with the arrows (->).


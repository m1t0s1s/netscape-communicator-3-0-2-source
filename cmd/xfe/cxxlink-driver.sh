#!/bin/sh
#-----------------------------------------------------------------------------
#    cxxlink-driver.sh
#
#    Copyright © 1996 Netscape Communications Corporation, all rights reserved.
#    Created: David Williams <djw@netscape.com>, 18-Jul-1996
#
#    C++ Link driver. This guy is a replacement for a broken C++ link
#    command. It will fix the options for the link so that no unwanted
#    shared libraries get linked in, and other stuff. It may use it's
#    pal cxxlink-filter.sh (for Cfront based loser linking) to do this.
#    
#-----------------------------------------------------------------------------
OS_NAME=OSF1
CC_NAME=cxx

ARGS=$*
OUT_ARGS=
VERBOSE=

while [ X$1 != X ]
do
  case X$1 in
	# This must be stripped out first, because it would match 'X-O*' as well,
	# and that doesn't strip out the limit arg.
	X-Olimit)
			shift
			shift
			;;
	# Include, define, undefine, optimize (also optimization level), ignore
	X-I*|X-D*|X-U*|X-O*)
			shift
			;;
	# Verbose
	X-v)
			VERBOSE=TRUE
			shift
			;;
	# HP-UX, OSF1, IRIX5.2.....
	X-cxx_platform)
			OS_NAME=$2
			shift 2
			;;
	# CC, cxx, gcc, .....
	X-cxx_command)
			CC_NAME=$2
			shift 2
			;;
	# Pass through
	*)
			OUT_ARGS="$OUT_ARGS $1"
			shift
			;;
	esac
done

case X${OS_NAME}Y${CC_NAME} in
  XOSF1Ycxx)	# Losing Dec cxx driver
    LD_DIR=/usr/lib/cmplrs/cc
    CXX_DIR=/usr/lib/cmplrs/cxx
	HEAD="$LD_DIR/ld -G 8 -g2 -call_shared -nocount $CXX_DIR/crt0.o $CXX_DIR/_main.o -count -taso"
	TAIL="-nocount $CXX_DIR/libcxx.a $CXX_DIR/libexc.a -lc"
	;;
  XHP-UXYCC)	# Losing HP CC driver
    HEAD=CC -tl,cxxlink-filter
	TAIL=
	;;
  XIRIX5.2YCC)	# Losing IRIX 5.2 CC driver
    HEAD=CC -tl,cxxlink-filter
	TAIL=
	;;
  *)		    # Unknown OS/Compiler
	echo "Unknown OS/Compiler: '$OS_NAME/$CC_NAME'" >$2
	exit 2
	;;
esac

case X$VERBOSE in
	XTRUE)	echo $HEAD $OUT_ARGS $TAIL
			;;
esac

exec $HEAD $OUT_ARGS $TAIL 

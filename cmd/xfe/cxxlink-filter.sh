#!/bin/sh
#-----------------------------------------------------------------------------
#    cxxlink-filter.sh
#
#    Copyright © 1996 Netscape Communications Corporation, all rights reserved.
#    Created: David Williams <djw@netscape.com>, 18-Jul-1996
#
#    C++ Link line filter. This guy attempts to "fix" the arguments that
#    are passed to the linker. It should probably know what platform it
#    is on, but it doesn't need to (yet) because we only use it for
#    Cfront drivers, and the names are the same on all the Cfront platforms.
#    
#-----------------------------------------------------------------------------
ARGS_IN=$*
ARGS_OUT=
for arg in $ARGS_IN
do
	case X$arg in
		X-lC)	# convert to .a
				arg=/usr/lib/libC.a
				;;
		X-lC.ansi)	# convert to .a
				arg=/usr/lib/libC.ansi.a
				;;
		*)		;;
	esac
	ARGS_OUT="$ARGS_OUT $arg"
done
exec ld $ARGS_OUT

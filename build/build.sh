#!/bin/sh

# This does both a debug and optimized build, and then installs the -O version.
# If anything goes wrong, everything stops.
#
# hosts to run it on:
#
# server2	hpux
# server9	aix 3.2
# server3,lemon	dec osf1
# neon		irix 5.2
# lunker	irix 4
# flop		solaris 2.3
# diva		solaris 2.4
# openwound	sun4
# linux		linux
# bsdi		bsdi/386 1.1
#
# Options:
#
# -clean	delete the whole build tree and recreate it (default)
# -update	do a "cvs update" first (default if not -clean)
# -debug	build in the debug tree (default)
# -optimize	build in the optimized tree (default)
# -install	do a "make install" of optimized tree (default if -optimize)
# -makefiles	do a "make Makefiles" and "make depend" (default)
#		makefiles will still be rebuilt if the Imakefiles are newer,
#		but forcing a rebuild ensures that all dependencies are right.
# -recursive	internal hack - do not use this
#
# options like -no-clean have the obvious meanings.

RELEASE=
#RELEASE="-r REL_1_1_UNIX"

CHECKOUT_DIRS="$RELEASE
               mcom/Imakefile
	       mcom/config.guess
	       mcom/debug+both.def
	       mcom/opt+both.def
	       mcom/mozilla-options.def
	       mcom/dbmlib.def
	       mcom/cmd/xfe
	       mcom/include
	       mcom/lib/layout
	       mcom/lib/libi18n
	       mcom/lib/libimg
	       mcom/lib/libmisc
	       mcom/lib/libmsg
	       mcom/lib/libnet
	       mcom/lib/libparse
	       mcom/lib/libsec
	       mcom/lib/plugin
	       mcom/lib/xlate
	       mcom/lib/xp
	       mcom/l10n
	       jpeg
	       dbm"

clean_p=true
update_p=true
debug_p=true
opt_p=true
install_p=true
makefiles_p=true
cvs_works=true
have_purify=false
recursive_p=false

orig_args=$*

while [ $# -ge 1 ]
do
  if [ "x$1" = "x-clean" ]; then
    clean_p=true
  elif [ "x$1" = "x-no-clean" ]; then
    clean_p=false
  elif [ "x$1" = "x-update" ]; then
    update_p=true
  elif [ "x$1" = "x-no-update" ]; then
    update_p=false
  elif [ "x$1" = "x-debug" ]; then
    debug_p=true
  elif [ "x$1" = "x-no-debug" ]; then
    debug_p=false
  elif [ "x$1" = "x-optimize" ]; then
    opt_p=true
  elif [ "x$1" = "x-no-optimize" ]; then
    opt_p=false
  elif [ "x$1" = "x-install" ]; then
    install_p=true
  elif [ "x$1" = "x-no-install" ]; then
    install_p=false
  elif [ "x$1" = "x-makefiles" ]; then
    makefiles_p=true
  elif [ "x$1" = "x-no-makefiles" ]; then
    makefiles_p=false
  elif [ "x$1" = "x-recursive" ]; then
    recursive_p=true
  else
    echo "usage: $0 [ -clean|-no-clean|-no-update|-no-makefiles|-no-debug|-no-optimize|-no-install ]"
    exit 1
  fi
  shift
done

CVSROOT=/m/src
ETAGS=etags
PURIFYOPTIONS=

# On BSDI, setting $MAKE in the environment breaks everything.  Fuckers.
REAL_MAKE=make
MAKEFILE_MAKE=make

BASE=/share/builds/a/client

CONFIG=`$BASE/config.guess`

RSH=rsh

ccopt=-O

case $CONFIG in
 *sunos4* )
   ARCH=sun4
   # Be sure to use R5 (in /usr/bin/X11/) and not R6, because of Motif lossage.
   #PATH=/usr/X11R6/bin:/usr/local/sun4/bin:$PATH
   PATH=/usr/bin/X11:/usr/local/sun4/bin:$PATH
   ccopt=-O3
   have_purify=true
   PURIFYOPTIONS="-windows=no -optimize-save-o7=no -cache-dir=/h/openwound/home/purify-cache"
   # actually it does work, but it's way faster to run it on neon, since
   # that's where the disks are
   cvs_works=false
 ;;
 *solaris* )
   if [ "`uname -r`" = 5.4 ]; then
     ARCH=sol24
   else
     ARCH=sol2
   fi
   OPENWINHOME=/usr/openwin
   PATH=/usr/openwin/bin:/usr/ucb:/opt/gnu/bin:/usr/ccs/bin:/usr/local/bin:$PATH
   ccopt=-O3
   cvs_works=false
   have_purify=true
   PURIFYOPTIONS="-windows=no"
 ;;
 *dec-osf* )
   ARCH=osf1
   PATH=/usr/bin/X11:$PATH
   cvs_works=false
   ETAGS=true
 ;;
 *irix5* )
   if [ "`uname -r`" = 5.3 ]; then
     ARCH=irix53
   else
     ARCH=irix5
   fi
   PATH=/usr/bin/X11:/usr/local/bin:$PATH
#   MKDEPFILE=/dev/null
#   export MKDEPFILE
#   REAL_MAKE="make MKDEPFILE=/dev/null"
 ;;
 *irix4* )
   ARCH=irix4
   PATH=/usr/bin/X11:$PATH
   RSH=/usr/bsd/rsh
   ETAGS=true
   cvs_works=false
 ;;
 *hpux* )
   ARCH=hpux
   PATH=/usr/bin/X11:/usr/bin:/usr/local/hpux/bin:$PATH
   ETAGS=true
   RSH=remsh
   REAL_MAKE="gmake MAKE=gmake"
   MAKE=${REAL_MAKE}
   export MAKE
   cvs_works=false
 ;;
 *aix* )
   ARCH=aix
   PATH=/usr/bin/X11:/usr/bin:/usr/ucb:/usr/local/aix32/bin:$PATH
   ETAGS=true
   cvs_works=false
 ;;
 *linux* )
   ARCH=linux
   PATH=/usr/bin/X11:/usr/bin:/usr/ucb:/usr/local/bin:$PATH
   ETAGS=true
   cvs_works=false
 ;;
 *i386-*-bsd* )
   ARCH=bsdi
   PATH=/usr/X11/bin:/usr/bin:/usr/ucb:/usr/local/bin:$PATH
   ETAGS=true
   cvs_works=false

   # This fucker doesn't have enough space in /tmp to ranlib libsecurity.a!!!
   TMPDIR=/u/jwz/tmp/
   export TMPDIR
 ;;
 * )
   echo 'ERROR, what arch is this? ' "($CONFIG)"
   exit 1
 ;;
esac

export CVSROOT PATH ETAGS OPENWINHOME PURIFYOPTIONS

CVS()
{
 if [ $cvs_works = true ]; then
   cvs $*
 else
   dir=`pwd | sed 's@/tmp_mnt@@' | sed 's@^/j/client@/share/builds/a/client@'`
   $RSH neon "cd $dir ; cvs $*"
 fi
}

cleanit()
{
  # Much faster to do deletion on server3, since that's where the disks are.
  # We should check to see if we're on s3 and not rsh, but life is too short.
  cd /tmp
  dir=$1
  $RSH server3 "rm -rf $dir ; mkdir $1"
  cd $dir
  CVS checkout $CHECKOUT_DIRS

  # some versions of make blow up if these directories don't already exist.
  if [ ! -d $dir/mcom/cmd/xfe/config ]; then
    mkdir $dir/mcom/cmd/xfe/config
  fi
}


umask 022


# First, update this script (build.sh) and then re-invoke it.
# But re-invoke it with an additional argument to prevent a loop.
if [ $recursive_p = false ]; then
  echo "(updating and re-invoking self...)"
  cd $BASE
  CVS update -l .
  exec $0 -recursive $orig_args
fi

if [ $clean_p = true ]; then
  # no need to do an update if we're deleting and recreating the dirs.
  update_p=false
fi

echo "======================================================================="
echo ""
echo "building on `uname -n` ($CONFIG) in $BASE/$ARCH"
echo ""
echo "======================================================================="
echo ""

set -e
set -x


dotree()
{
  which=$1
  dir=$BASE/$ARCH/$which

  config_which=$which
  if [ "$config_which" = "optimized" ]; then config_which=opt ; fi

  if [ $clean_p = true ]; then
    cleanit $dir
  fi

  cd $dir

  if [ $update_p = true ]; then
    CVS update .
  fi

  cd $dir/mcom
  rm -f mozilla.def
  ln -s $config_which+both.def mozilla.def
  pwd
  MAKE=${MAKEFILE_MAKE} xmkmf
  if [ $makefiles_p = true ]; then
    if [ $ARCH = bsdi ]; then
      make Makefiles
    else
      MAKE=${MAKEFILE_MAKE} ${MAKEFILE_MAKE} Makefiles
    fi
  fi
  ${REAL_MAKE}

  if [ $which = optimized ]; then
    cd $dir/mcom/cmd/xfe
    echo "releasing in $dir/mcom/cmd/xfe ($CONFIG)"
    ${REAL_MAKE} release
    if [ $install_p = true ]; then
      echo "installing in $dir/mcom/cmd/xfe ($CONFIG)"
      ${REAL_MAKE} install-us
      echo "install done from $dir/mcom/cmd/xfe ($CONFIG)"
    fi
  fi

  if [ $which = debug -a $have_purify = true ]; then
    cd $dir/mcom/cmd/xfe
    ${REAL_MAKE} netscape-us.pure netscape-export.pure
  fi
}

if [ $debug_p = true ]; then
  dotree debug
fi

if [ $opt_p = true ]; then
  dotree optimized
fi

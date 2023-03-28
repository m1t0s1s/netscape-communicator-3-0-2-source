#!/bin/sh

BASE=/share/builds/a/client
DEST=/h/neon/d/releases
#DEST=/h/wwwww/d/releases
NAME=netscape

CP="cp -p"
LN="ln -s"
RM="rm"
MKDIR="mkdir"

if [ "x$1" = "x-testing" ]; then
  DEST=$BASE/testing
  shift
# set -x
# set -e
else
  set +x
  set -x
#  set -e
fi

if [ $# -ne 1 ]; then
  echo usage: $0 revision
  exit 1
fi

REV=$1

cd $BASE

if [ ! -d $DEST/$REV ]; then
   ${MKDIR} $DEST/$REV
fi

for DIR in `find [a-z]*[a-z0-9] -type d -prune \( -name testing -o -print \)`
do
  ${RM} -rf $DEST/$REV/$DIR
  ${MKDIR} $DEST/$REV/$DIR

#  ( cd $BASE/$DIR/optimized/mcom/cmd/xfe/release ; tar -cf - . ) |
  ( cd $BASE/$DIR/optimized/mcom/cmd/xfe/release ; tar -cf - `ls | sed 's@.*-net.*@@'` ) |
  ( cd $DEST/$REV/$DIR ; tar -vxf - )

  for EXE in $BASE/$DIR/debug/mcom/cmd/xfe/netscape-* \
             $BASE/$DIR/debug/mcom/cmd/xfe/netscape_dns-*
  do
    if [ -f $EXE -a x"`echo $EXE | sed -n 's/.*pure.*/True/p'" != 'xTrue' ]
    then
      SUFFIX1=`echo $EXE | sed -n 's|.*/netscape\(_[^/]*\)-.*$|\1|p'`
      SUFFIX2=`echo $EXE | sed -n 's|.*/netscape-\([^/]*\)$|\1|p'`
      if [ "x" != x`echo $SUFFIX2 | sed 's@.*-net.*@@'` ]; then
        ${CP} $EXE $DEST/$REV/$DIR/netscape-$SUFFIX2/netscape$SUFFIX1.debug
      fi
    fi
  done

  for EXE in $BASE/$DIR/debug/mcom/cmd/xfe/netscape-*.pure \
             $BASE/$DIR/debug/mcom/cmd/xfe/netscape_dns-*.pure
  do
    if [ -f $EXE ]; then
      SUFFIX1=`echo $EXE | sed -n 's|.*/netscape\(_[^/.]*\)-.*$|\1|p'`
      SUFFIX2=`echo $EXE | sed -n 's|.*/netscape-\([^/.]*\)\..*$|\1|p'`
      if [ "x" != x`echo $SUFFIX2 | sed 's@.*-net.*@@'` ]; then
        ${CP} $EXE $DEST/$REV/$DIR/netscape-$SUFFIX2/netscape$SUFFIX1.pure
      fi
    fi
  done

  gzip --best --verbose $DEST/$REV/$DIR/*/*

done

${RM} -rf $DEST/$REV/DIST
${MKDIR} $DEST/$REV/DIST

for SUFFIX in     us     export \
	      net-us net-export \
	      sgi-us sgi-export
do
  ${MKDIR} $DEST/$REV/DIST/netscape-$SUFFIX
  cd $DEST/$REV/DIST/netscape-$SUFFIX
  ${CP} ../../irix5/netscape-$SUFFIX/README* .
  ${CP} ../../irix5/netscape-$SUFFIX/LICENSE* .
  ${LN} ../../*/netscape-$SUFFIX/*.tar.Z .
done

cd $DEST/$REV
ls -lLFr */*/netscape */*/netscape.gz

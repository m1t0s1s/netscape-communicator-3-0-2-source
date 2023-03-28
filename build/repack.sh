#!/bin/sh

# extracts and repacks a tar file with the same contents, but gives
# us the chance to do other junk like replace files or change owners
# or permissions in the meantime.  This is for emergencies...

if [ $# -ne 1 ]; then
  echo usage: $0 file-name
  exit 1
fi

FILE=$1

#set -x
set -e

rm -rf tmp
mkdir tmp
cd tmp

case $FILE in
  *Z ) compressed=true ;;
  *  ) compressed=false ;;
esac

CAT()
{
 if [ $compressed = true ]; then
   zcat $1
 else
   cat $1
 fi
}

COMPRESS()
{
 if [ $compressed = true ]; then
   compress -v
 else
   cat
 fi
}


FILES=`CAT ../$FILE | tar -tf -`

echo extracting files $FILES ...
CAT ../$FILE | tar -xf -

#############################################################################

cp -p ../../README-B10N README
#chmod -R og-w *
cmon chown -R root.sys *

#############################################################################

echo repacking...

tar -vcf - $FILES | COMPRESS > ../$FILE-2
cd ..

if [ ! -f $FILE-old ]; then
  echo saving $FILE as $FILE-old...
  mv $FILE $FILE-old
fi
echo replacing $FILE...
mv $FILE-2 $FILE

rm -rf tmp

CAT $FILE | tar -vtf -

#!/bin/sh

root=/h/neon/d/builds

file="$1"
arch="$2"

tmpdir=$root/TMP

if [ x"$file" = "x" -o x"$arch" = "x" ]; then
  echo "usage: $0 TARFILE ARCH"
  exit 1
fi

file=`pwd`/$file

rm -rf $tmpdir
mkdir $tmpdir
cd $tmpdir

case $file in
  *.tar.Z|*.tar.gz )
     zcat $file | tar -xf -
     ;;
  *.tar )
     tar -xf $file
     ;;
  * )
     echo not a tar file - $file
     exit 1
     ;;
esac

for file in * ; do
  ls -lF $file
  diff $file $root/$arch/optimized/mcom/cmd/xfe/$file
done

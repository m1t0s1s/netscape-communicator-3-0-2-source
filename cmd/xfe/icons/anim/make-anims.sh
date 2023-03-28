#!/bin/sh

# creates anim-gifs (*-large.gif and *-small.gif) from the Anim*gif files
# in the subdirectories of the current directory.

TMP=/tmp
PATH=$PATH:/usr/local/src/gifanim:/usr/local/bin
set -e

make_anim() {
 final=$1
 shift
 files=$*
 rgbs=""

# echo "$final ..."

 if ( pwd | grep '/sgi$' > /dev/null ) || \
    ( pwd | grep '/throbber$' > /dev/null )
 then
   frob="ppmchange #000000 #C0C0C0"
 else
   frob="cat"
 fi

# echo "  generating rgbs ..."
 for file in $files
 do
   rgb=`echo $file | sed s@gif@rgb@`
   rgb=$TMP/$rgb
   rgbs="$rgbs $rgb"
   giftopnm < $file | $frob | pnmtosgi > $rgb
 done

 map=$TMP/$final.map
 mkpal $map 255 $rgbs

# echo "  generating gif ..."
 togifanim $final $map -r 0 -26 $rgbs

 rm -f $map $rgbs
}


make_all() {
  tmp=tmp$$.gif

  rm allanims.html
  echo "<table border=0 cellspacing=10>"	>> allanims.html

  for dir in `find [a-z]* -type d -prune -print`
  do
    echo $dir...
    ( cd $dir; make_anim $tmp AnimSm??.gif )
    mv $dir/$tmp $dir-small.gif
    ls -lF $dir-small.gif

    (cd $dir; make_anim $tmp AnimHuge??.gif )
    mv $dir/$tmp $dir-large.gif
    ls -lF $dir-large.gif

    echo " <tr align=center>"			>> allanims.html
    echo "  <th align=right>$dir</th>"		>> allanims.html
    echo "  <td><img src=$dir-small.gif></td>"	>> allanims.html
    echo "  <td><img src=$dir-large.gif></td>"	>> allanims.html
    echo " </tr>"				>> allanims.html

  done
  echo "</table>"				>> allanims.html
}

make_all

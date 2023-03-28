#!/bin/sh
#
# generate various files from secprefs.html
#

INPUT=secprefs.html
TMP=/tmp/convhtml.$$
rm -f $TMP

echo "/* This file is generated from $INPUT.  Do not edit.
   (see ns/security/lib/nav/convhtml.sh) */"	\
 > secprefs.h

cat $INPUT	 				\
 | sed 's@[ 	][ 	]*@ @g'			\
 | awk '/^<HTML>/,/^var sa_handle/;
	/^.. BODY-BEGIN/,/^_juego_terminado_/;'	\
 | sed 's@//.*@@'				\
 | sed 's@<!--.*-->@@'				\
 | grep .					\
 > $TMP

# generate secprefs.h for unix/mac
#
sed -f convhtml < $TMP >> secprefs.h
echo generated secprefs.h from $INPUT


# generate secprefs.rc for windows
#
echo "secprefs RCDATA" > secprefs.rc
echo "BEGIN" >> secprefs.rc
cat $TMP						\
 | awk '/^var sa_handle/,/_juego_terminado_/;'		\
 | tail +2						\
 | sed -f convwrc					\
 >> secprefs.rc
echo "\"\\\\0\"" >> secprefs.rc
echo END >> secprefs.rc
echo generated secprefs.rc from $INPUT

rm -f $TMP

egregious_terminology_filter()
{
  if [ $AKBAR = 0 ]; then
   cat
  else
    sed \
     's@"Messenger"@"Mail/News"@g;
      s@Messenger Message@Netscape Mail@g;
      s@Discussion Message@Netscape News@g;
      s@Messenger@Netscape@g;
      s@Discussion [Gg]roup@Newsgroup@g;
      s@discussion (news)@news@g;
     '
  fi
}


# now generate the xp strings
#
cat $INPUT	 				\
 | awk '/^.. L10N-BEGIN/,/^.. BODY-BEGIN/'	\
 | sed 's@[ 	][ 	]*@ @g'			\
 | sed 's@//.*@@g;s@^ *$@@g'			\
 | grep '.'					\
 | egregious_terminology_filter			\
 > $TMP

cat $TMP					\
 | sed -f convquot				\
 | perl5 mkxpstrs.pl				\
 > allxpstr.ins
echo generated allxpstr.ins from $INPUT

rm -f $TMP

#!/bin/sh
# Script for converting NCSA xmosaic hotlist files to Netscape's format.
# Copyright © 1995 Netscape Communications Corp., all rights reserved.
# Created: Jamie Zawinski <jwz@netscape.com>, 17-Oct-94.
# Bugs and commentary to x_cbug@netscape.com.

usage()
{
  echo "usage: $0 [ input-file [ output-file ]]" >&2
  exit 1
}

#set -x
set -e

input=__DEFAULT__
output=$HOME/.netscape-bookmarks.html
tmp=/tmp/hc$$
tmp2=/tmp/hc$$2

create_p=false

if [ $# = 2 ]; then
  input="$1"
  output="$2"
elif [ $# = 1 ]; then
  input="$1"
elif [ $# != 0 ]; then
  usage
fi

if [ "$input" = __DEFAULT__ ]; then
  if [ -f $HOME/.mosaic-hotlist-default.html ]; then
    input=$HOME/.mosaic-hotlist-default.html
  else
    input=$HOME/.mosaic-hotlist-default
  fi
fi

if [ ! -f $input ]; then
  echo $0: input file $input does not exist.
  usage
fi

if [ ! -f $output ]; then
  create_p=true
fi

rm -f $tmp $tmp2

firstline="`head -1 $input`"

format=0;
if [ "$firstline" = "ncsa-xmosaic-hotlist-format-1" ]; then
  format=1
elif [ "$firstline" = "<HTML>" ]; then
  secondline="`head -2 $input | tail -1 | sed 's/>.*/>/'`"
  if [ "$secondline" = "<!-- ncsa-xmosaic-hotlist-format-2 -->" ]; then
    format=2
  elif [ "$secondline" = "<TITLE>" ]; then
    thirdline="`head -3 $input | tail -1`"
    if [ "$thirdline" = "Default" ]; then
      format=2
    fi
  fi
fi

if [ "$format" = 0 ]; then
  echo "$0: input file $input is not an NCSA xmosaic hotlist file"
  usage
fi

#if [ $create_p != true ]; then
#  if [ "`head -1 $output`" != "<!DOCTYPE MCOM-Bookmark-file-1>" ]; then
#   if [ "`head -1 $output`" != "<!DOCTYPE NETSCAPE-Bookmark-file-1>" ]; then
#    echo "$0: output file $output exists but is not a Netscape bookmarks file"
#    usage
#   fi
#  fi
#fi

size="`wc -l $input | awk '{print $1}'`"

if [ "$size" -le 2 ]; then
  echo "$0: input file $input contains no hotlist items"
  usage
fi

# Take off the first two lines of the input
if [ $format = 1 ]; then

  tail +3 $input |

  awk 'BEGIN { state=0; }
  {
    if ( state == 0 )
      {
        printf ("    <DT><A HREF=\"%s\">", $1);
        state = 1;
      }
    else
      {
        printf ("%s</A>\n", $0);
        state = 0;
      }
  }
  ' > $tmp

else

  # new format
  tail +5 $input | sed '
	s@^<LI> *@    <DT>@;
	s@^</UL> *$@@;
	s@^</HTML> *$@@;
   ' > $tmp

fi


if [ $create_p = true ]; then

  echo "<!DOCTYPE NETSCAPE-Bookmark-file-1>"		  >> $tmp2
  echo "<!-- This is an automatically generated file.>"   >> $tmp2
  echo "     It will be read and overwritten."            >> $tmp2
  echo "     Do Not Edit! -->"                            >> $tmp2
  echo "<TITLE>$USER's Bookmarks</TITLE>"                 >> $tmp2
  echo "<H1>$USER's Bookmarks</H1>"                       >> $tmp2
  echo "<DL><p>"                                          >> $tmp2
  cat $tmp >> $tmp2
  echo "</DL><p>"                                         >> $tmp2
  cat $tmp2 > $output

  echo "$0: created bookmarks file $output" >&2

else

  # take off the last closing /DL (the flushleft one) so that we can put
  # the new items before it.
  sed 's@^</DL><p>@@' < $output > $tmp2
  echo "<HR>" >> $tmp2
  cat $tmp >> $tmp2
  echo "</DL><p>" >> $tmp2
  cat $tmp2 > $output

  echo "$0: appended to bookmarks file $output" >&2

fi

rm -f $tmp $tmp2

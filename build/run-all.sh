#!/bin/sh

url=http://islay:8008/test.cgi

echo DISPLAY is $DISPLAY
sleep 2

root=/h/neon/d/builds

launch()
{
  host=$1
  arch=$2
  suffix=$3

  echo ""
  echo "$host ($arch$suffix debug):"
  echo ""
  rsh $host "
    $root/$arch/debug/mcom/cmd/xfe/netscape$suffix -version ; \
    $root/$arch/debug/mcom/cmd/xfe/netscape$suffix \
      -display $DISPLAY $url"

  echo ""
  echo "$host ($arch$suffix optimized):"
  echo ""
  rsh $host "
    $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix -version ; \
    $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix \
      -display $DISPLAY $url"

  echo ""
  echo "$host ($arch$suffix optimized, unstripped):"
  echo ""
  rsh $host "
    $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix.unstripped -version ; \
    $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix.unstripped \
      -display $DISPLAY $url"
}

launch neon irix5
launch openwound sun4
launch openwound sun4 _dns
launch flop sol2
launch server2 hpux
launch server3 osf1
launch server9 aix
launch linux linux
launch bsdi bsdi

#!/bin/sh

root=/h/neon/d/builds

check()
{
  host=$1
  arch=$2
  suffix=$3

  echo ""
  echo "===== $host ($arch$suffix) ======"
  # this expects rsh to run csh

#  rsh $host netscape$suffix -version

  ( rsh $host "
    echo -n 'debug: ' ;
    ( $root/$arch/debug/mcom/cmd/xfe/netscape$suffix -version |& cat) ;
    echo -n 'opt:   ' ;
    ( $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix -version |& cat) ;
    echo -n 'opt+d: ' ;
    ( $root/$arch/optimized/mcom/cmd/xfe/netscape$suffix.unstripped -version |& cat) ;
   " 2>&1 ) | sed 's@(c) 1994 Netscape Communications Corp. *@@'
}

check neon irix5
check openwound sun4
check openwound sun4 _dns
check flop sol2
check server2 hpux
check server3 osf1
check server9 aix
check linux linux
check bsdi bsdi

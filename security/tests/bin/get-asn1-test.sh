#!/bin/sh
#
#    get-smail-code.sh
#

CVSROOT=/m/src
export CVSROOT

NAME=asn1-test
DIR=$HOME/$NAME

if [ -d "$DIR" ] 
then
	rm -rf $DIR
fi

mkdir $DIR
cd $DIR

cvs co ns/security/tests/libsec-smime

exit 0

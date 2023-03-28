#!/bin/csh
#
# This script is a test driver for the usdate test program.  Usdate tests the
# function PR_FormatDateUSEnglish().
#
#

# This one generally fails - because the system library generally writes off the
# end of its buffer and produces junk.  Our routine did not duplicate this
# behaviour.
usdate '%a %A %'

usdate '%a %A %$'
usdate '%Z'
usdate '%Y'
usdate '%y'
usdate '%X'
usdate '%x'
usdate '%W'
usdate '%w'
usdate '%U'
usdate '%S'
usdate '%p'
usdate '%M'
usdate '%m'
usdate '%j'
usdate '%I'
usdate '%H'
usdate '%d'
usdate '%c'
usdate '%B'
usdate '%b'
usdate '%A'
usdate '%a'
usdate '%%'

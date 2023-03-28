#!/usr/local/bin/tcl -f

# This takes a date string (fairly arbitrarily formatted), and spews out
# a number suitable for pasting into the timebomb code.

foreach i $argv {
    set num [convertclock $i]
    puts "$num          [fmtclock $num]"
}

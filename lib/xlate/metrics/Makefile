#
# This directory contains the source used to generate the font metric files
# It is not built, the result is checked into the tree.  This is just
# here for reference.
#

makefm: makefm.c
	$(CC) -I.. -I../../../include makefm.c parseAFM.c -o makefm

../font_metrics.c: makefm
	makefm > ../font_metrics.c

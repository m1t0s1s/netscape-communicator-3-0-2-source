.SUFFIXES:	.c .o

E1 = tfolder1 tfolder2 tgrid1 tgrid2 tgrid3 tprog1
E2 = tutil1 tutil2 tutil3

LDLIBS = -lXmL -lMrm -lXm -lXt -lX11

.c:
	$(CC) -I.. $(CFLAGS) -L../XmL $(LDFLAGS) -o $@ $< $(LDLIBS)

.uil.uid :
	uil $*.uil -o $*.uid

all:	$(E1) $(E2)

clean:
	rm -f $(E1) $(E2)

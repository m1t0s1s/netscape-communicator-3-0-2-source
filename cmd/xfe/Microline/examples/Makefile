.SUFFIXES:	.c .o .uil .uid

E1 = demo
E2 = grid1 grid2 grid3 grid4 grid5
E3 = folder1 folder2 folder3
E4 = prog1 prog2 util1 uil1.uid uil1

LDLIBS = -lXmL -lMrm -lXm -lXt -lX11

.c:
	$(CC) -I.. $(CFLAGS) -L../XmL $(LDFLAGS) -o $@ $< $(LDLIBS)

.uil.uid :
	uil $*.uil -o $*.uid

all:	$(E1) $(E2) $(E3) $(E4)

clean:
	rm -f $(E1) $(E2) $(E3) $(E4) uil1.uid

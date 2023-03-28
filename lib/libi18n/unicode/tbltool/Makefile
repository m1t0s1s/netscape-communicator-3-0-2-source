INCLUDES = -I../..
.c.o:
	cc -c $(CFLAGS)  $(INCLUDES) $*.c

all:	fromu tou fromu0208 tou0208 xlatgen xlatgenmac

xlatgenmac:	xlatgenmac.o xlatgenutil.o

xlatgen:	xlatgen.c 

fromu:	fromu.o utblutil.o

tou:	tou.o utblutil.o

fromu0208:	fromu0208.o utblutil.o

tou0208:	tou0208.o utblutil.o

utblutil.o: ../../umap.h


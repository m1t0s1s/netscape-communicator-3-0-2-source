OBJS= lots.o

CC=cc

LIBS= ../IRIX5.3_DBG.OBJ/libdbm.a
#LIBS= ../BSD_3861.1_DBG.OBJ/libdbm.a
#LIBS= ../SunOS4.1.3_U1_DBG.OBJ/libdbm.a

CFLAGS= -g -DDEBUG -I../../../include -DSTDARG
#CFLAGS= -g -I../../../include -DXP_UNIX -g -DBSDI -DHAVE_STRERROR -D__386BSD__ -DDEBUG -DMEMMOVE -D__DBINTERFACE_PRIVATE 
#CFLAGS= -g -I../../../include -D_sun_

all: lots

lots.pure: lots
	purify $(CC) -o lots.pure $(CFLAGS) $(OBJS) $(LIBS)

lots: $(OBJS) $(LIBS)
	$(CC) -o lots $(CFLAGS) $(OBJS) $(LIBS)

crash: crash.o $(LIBS)
	$(CC) -o crash $(CFLAGS) crash.o $(LIBS)

crash.pure: crash.o $(LIBS)
	purify $(CC) -o crash.pure $(CFLAGS) crash.o $(LIBS)

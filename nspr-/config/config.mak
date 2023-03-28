# Configuration information for building in the NSPR source module

!ifdef BUILD_DEBUG_GC
LCFLAGS=$(LCFLAGS) -DDEBUG_GC
!endif

LCFLAGS=$(LCFLAGS) -D_NSPR
LINCS=$(LINCS) -I$(DEPTH)\nspr\include

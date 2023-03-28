macsock
Macintosh socket compatibility layer
Implements a subset of UNIX socket library - DNS and socket streams

/***********************************************
 * REQUIRED FILES
 * If you are using netlib, you need navsock.cp
 ***********************************************/
// PPC & 68K
CNetwork.cp			-- abstract driver
sockets.cp			-- sockets, mediates between sockets model and streams
mactcpdriver.cp		-- mactcp driver
otdriver.cp			-- open transport driver
dnr.c				-- inside MacTCP folder in CodeWarrior:Libraries
					-- MacTCP DNR implementation
macsock.rsrc		-- dialog resources

// PPC -- import these as 'weak'
LibraryManagerPPC.o	-- Needed by OpenTransport
OpenTransportLib	-- OpenTransport libraries
OpenTptInternetLib
OpenTptInetPPC.o
OpenTransportAppPPC.o

// 68K
OpenTptInet.o	(.n.o) for model far
OpenTransport.o	(.n.o) for model far
OpenTransportApp.o	(.n.o) for model far

/***********************************************
 * USAGE
 ***********************************************/
#include "macsock.h" // for standard socket calls
#include "CNetwork.h" // For headers that interact with the driver

At startup call
CNetworkDriver::CreateNetworkDriver();
At quit call
delete gNetDriver;

socket library calls that are still not implemented, or not implemented fully:
dup - there is no similar concept for streams. We might have to implement reference 
	counting. OT accept has potential, but for MacTCP we are dead
listen - only works for a special case 
select - timeout parameter is ignored, easy to implement, we do not use it yet
shutdown
getsockopt
setsockopt

/***********************************************
 * DEFINES
 ***********************************************/
#define CMD_PERIOD // If you want CmdPeriod to abort synchonous tasks
#define WAIT_TO_QUIT
When application quits, we need to make sure that all async calls have
been completed. We'll call WaitToQuit. Use this define if you want to call
build-in wait-to-quit routine. This routine puts up a dialog box.
#define TCP_DEBUG
You can define these inside otdriver and mactcpdriver for informative trace messages

/***********************************************
 * TO BE DONE
 ***********************************************/
Unimplemented socket calls - do it when some XP library starts using them
Test Listen/Accept. We have no local test cases

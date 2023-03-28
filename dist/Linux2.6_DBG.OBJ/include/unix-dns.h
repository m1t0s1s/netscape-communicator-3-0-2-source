/* -*- Mode:C; tab-width: 8 -*-
 * dns.h --- portable nonblocking DNS for Unix
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 19-Dec-96.
 */

#ifndef __UNIX_DNS_H__
#define __UNIX_DNS_H__

/* Kick off an async DNS lookup;
   The returned value is an id representing this transaction;
    the result_callback will be run (in the main process) when we
    have a result.  Returns negative if something went wrong.
   If `status' is negative,`result' is an error message.
   If `status' is positive, `result' is a 4-character string of
   the IP address.
   If `status' is 0, then the lookup was prematurely aborted
    via a call to DNS_AbortHostLookup().
 */
extern int DNS_AsyncLookupHost(const char *name,
			       int (*result_callback) (void *id,
						       void *closure,
						       int status,
						       const char *result),
			       void *closure,
			       void **id_return);

/* Prematurely give up on the given host-lookup transaction.
   The `id' is what was returned by DNS_AsyncLookupHost.
   This causes the result_callback to be called with a negative
   status.
 */
extern int DNS_AbortHostLookup(void *id);

/* Call this from main() to initialize the async DNS library.
   Returns a file descriptor that should be selected for, or
   negative if something went wrong.  Pass it the argc/argv
   that your `main' was called with (it needs these pointers
   in order to give its forked subprocesses sensible names.)
 */
extern int DNS_SpawnProcess(int argc, char **argv);

/* The main select() loop of your program should call this when the fd
   that was returned by DNS_SpawnProcess comes active.  This may cause
   some of the result_callback functions to run.

   If this returns negative, then a fatal error has happened, and you
   should close `fd' and not select() for it again.  Call gethostbyname()
   in the foreground or something.
 */
extern int DNS_ServiceProcess(int fd);

#endif /* __UNIX_DNS_H__ */

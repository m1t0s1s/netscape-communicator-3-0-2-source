#include "secutil.h"

#define SSL_ENCRYPT_MASK        (1<<0)
#define SSL_ENCRYPT             (1<<0)  /* encrypt the connection */
#define SSL_DONT_ENCRYPT        (0)     /* dont encrypt the connection */

#define SSL_PROXY_MASK          (3<<1)
#define SSL_SECURE_PROXY        (3<<1)  /* use ssld as proxy */
#define SSL_PROXY               (1<<1)  /* use sockd as proxy */
#define SSL_NO_PROXY            (0)     /* direct connection */

/* parse flags and return createflags
**
*/
extern int
ssl_ParseFlags(char *argstring, int *pencrypt, int *pproxy);

/* use ssl to issue rcmd , takes same args as rcmd plus two more:
**    createflags, which uses bits defined above
**    certDir, which if not null sets the dir for cert.db for server auth
**    nickname, which is used for client auth to access key by name
*/
extern int
SSL_Rcmd(char **ahost, int rport, char *locuser, char *remuser, char *cmd,
	 int *fd2p, int createflags, char *certDir, char *nickname);


#include "secutil.h"
#include "sechash.h"

#ifdef XP_UNIX
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif
#ifdef XP_WIN
#include <windows.h>
#include <winsock.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef XP_MAC
#include <unistd.h>
#endif

#ifdef XP_UNIX
#include "prnetdb.h"
#else
#define PRHostEnt struct hostent
#define PR_NETDB_BUF_SIZE 5
#endif

static RC4Context *
sec_MakeDongleKey(void)
{
    RC4Context *rc4;
    char hostname[64];
    PRHostEnt hpbuf, *hent;
    char dbbuf[PR_NETDB_BUF_SIZE];
    int ipaddr;
    struct stat s;
    int found = 1; /* Whether /vmunix, etc. found */
    unsigned char *buf, *totalbuf;
    MD5Context *md5 = 0;
    unsigned char digest[16];
    unsigned int digestLen;

    /* XXX Fix MAC and WIN to stat something comparable to kernel on UNIX */
#ifdef XP_MAC
    return NULL;
#elif defined(XP_WIN)
    return NULL;
#endif

    /* gather system data */
    if( gethostname( hostname, 64 ) < 0 ) {
	return(NULL);
    }

#ifdef XP_UNIX
    hent = PR_gethostbyname(hostname, &hpbuf, dbbuf, sizeof(dbbuf), 0);
#else
    hent = gethostbyname(hostname);
#endif
    if (hent == NULL) {
	return(NULL);
    }

    ipaddr = htonl(((struct in_addr *)hent->h_addr)->s_addr);

    /*
    ** /unix         IRIX & AIX & SCO
    ** /vmunix       SunOS & OSF
    ** /kernel/unix  Solaris
    ** /vmlinuz      Linux
    ** /hp-ux        HP-UX
    ** /bsd          BSD
    */
    if ( (stat("/unix", &s) == -1 ) && (stat("/vmunix", &s) == -1) &&
	 (stat("/bsd", &s) == -1) && (stat("kernel/unix", &s)== -1) &&
	 (stat("/hp-ux", &s) == -1) && (stat("/vmlinuz", &s) == -1) ) {
	found = 0;
    }
								      
    buf = totalbuf = (unsigned char *)
	XP_CALLOC(1, 1000 + sizeof(ipaddr) + sizeof(s) + PORT_Strlen(hostname));
    
    if ( buf == 0 ) {
	return(NULL);
    }

    PORT_Memcpy(buf, hostname, PORT_Strlen(hostname));
    buf += PORT_Strlen(hostname);
    
    PORT_Memcpy(buf, &ipaddr, sizeof(ipaddr));
    buf += sizeof(ipaddr);
   
    if (found) {
	PORT_Memcpy(buf, &s.st_mode, sizeof(s.st_mode));
	buf += sizeof(s.st_mode);
    
	PORT_Memcpy(buf, &s.st_ino, sizeof(s.st_ino));
	buf += sizeof(s.st_ino);
	
	PORT_Memcpy(buf, &s.st_dev, sizeof(s.st_dev));
	buf += sizeof(s.st_dev);
	
	PORT_Memcpy(buf, &s.st_mtime, sizeof(s.st_mtime));
	buf += sizeof(s.st_mtime);
    }

    /* Digest the system info using MD5 */
    md5 = MD5_NewContext();
    if (md5 == NULL) {
	return (NULL);
    }

    MD5_Begin(md5);
    MD5_Update(md5, totalbuf, PORT_Strlen((char *)totalbuf));
    MD5_End(md5, digest, &digestLen, MD5_LENGTH);

    /* Make an RC4 key using the digest */
    rc4 = RC4_CreateContext(digest, digestLen);

    /* Zero out information */
    MD5_DestroyContext(md5, PR_TRUE);
    PORT_Memset(digest, 0 , sizeof(digest));

   
    return(rc4);
}

extern unsigned char *
SEC_ReadDongleFile(int fd)
{
    RC4Context *rc4;
    struct stat s;
    unsigned char *inbuf, *pw;
    int nb;
    unsigned int pwlen, inlen;
    SECStatus rv;
    
    rc4 = sec_MakeDongleKey();
    if (rc4 == NULL) {
	return(0);
    }

    /* get size of file */
    if ( fstat(fd, &s) < 0 ) {
	return(0);
    }

    inlen = s.st_size;
    
    inbuf = (unsigned char *) PORT_Alloc(inlen);
    if (!inbuf) {
	return(0);
    }

    nb = read(fd, (char *)inbuf, inlen);
    if (nb != inlen) {
	return(0);
    }

    pw = (unsigned char *) PORT_Alloc(inlen);
    if (pw == 0) {
	return(0);
    }

    rv = RC4_Decrypt(rc4, pw, &pwlen, inlen, inbuf, inlen);
    if (rv) {
	return(0);
    }

    PORT_Free(inbuf);

    return(pw);
}

extern SECStatus
SEC_WriteDongleFile(int fd, unsigned char *pw)
{
    RC4Context *rc4;
    unsigned char *outbuf;
    unsigned int outlen;
    SECStatus rv;
    int nb;
    
    rc4 = sec_MakeDongleKey();
    if (rc4 == NULL) {
	return(SECFailure);
    }

    outbuf = (unsigned char *) PORT_Alloc(PORT_Strlen((char *)pw) + 1);
    if (!outbuf) {
	return(SECFailure);
    }

    rv = RC4_Encrypt(rc4, outbuf, &outlen, PORT_Strlen((char *)pw), pw,
		     PORT_Strlen((char *)pw));
    if (rv) {
	return(SECFailure);
    }

    RC4_DestroyContext(rc4, PR_TRUE);

    nb = write(fd, (char *)outbuf, outlen);
    if (nb != outlen) {
	return(SECFailure);
    }

    PORT_Free(outbuf);

    return(SECSuccess);
}


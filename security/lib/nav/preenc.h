/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */

/*
 * header file to define pre-encrypted field and magic numbers
 */

#include "sslimpl.h"

typedef struct PEHeaderStr PEHeader;
typedef struct PECryptedHeaderStr PECryptedHeader;

#define PE_MIME_TYPE "application/pre-encrypted"


/*
 * unencrypted header. The 'top' half of this header is generic. The union
 * is type specific, and may include bulk cipher type information
 * (Fortezza supports only Fortezza Bulk encryption). Only fortezza 
 * pre-encrypted is defined.
 */
typedef struct PEFortezzaHeaderStr PEFortezzaHeader;
typedef struct PEFortezzaGeneratedHeaderStr PEFortezzaGeneratedHeader;

struct PEFortezzaHeaderStr {
    unsigned char key[12];      /* Ks wrapped MEK */
    unsigned char iv[24];       /* iv for this MEK */
    unsigned char hash[20];     /* SHA hash of file */
    unsigned char serial[8];    /* serial number of the card that owns
                                     * Ks */
};

struct PEFortezzaGeneratedHeaderStr {
    unsigned char key[12];      /* TEK wrapped MEK */
    unsigned char iv[24];       /* iv for this MEK */
    unsigned char hash[20];     /* SHA hash of file */
    unsigned char Ra[128];      /* RA to generate TEK */
    unsigned char Y[128];       /* Y to generate TEK */
};


struct PEHeaderStr {
    unsigned char magic[2];		/* always 0xC0DE */
    unsigned char len[2];		/* length of PEHeader */
    unsigned char type[2];		/* FORTEZZA, DIFFIE-HELMAN, RSA */
    unsigned char version[2];	/* version number: 1.0 */
    union {
        PEFortezzaHeader fortezza;
        PEFortezzaGeneratedHeader g_fortezza;
    } u;
};

#define PEFortezzaHeaderSz	        (6+sizeof(PEFortezzaHeader))
#define PEFortezzaGeneratedHeaderSz	(6+sizeof(PEFortezzaGeneratedHeader))

#define PE_CRYPT_INTRO_LEN 8
#define PE_INTRO_LEN 4

#define PRE_BLOCK_SIZE 8         /* for decryption blocks */


/*
 * Encrypted header. This header is placed directly in front of data for the
 * file. The block size is rounded to 8 byte boundary. Pre-encrypting programs
 * should pad this area to prevent unwanted data leakage from other files in
 * the pre-encrypted stream.
 */
struct PECryptedHeaderStr {
    unsigned char header_len[2]; /* length of this header including mime_type*/
    unsigned char mime_len[2];   /* length of the mime_type including NULL */
    unsigned char version[2];    /* version number: 1.0 */
    unsigned char padding[2];    /* round it off to 8 bytes */
    unsigned char data_len[4];   /* real data length of the file (not 
                                      * including pad data) */
    unsigned char mime_type[1];  /* mime-type including NULL */
};

/*
 * Platform neutral encode/decode macros.
 */
#define GetInt2(c) ((c[0] << 8) | c[1])
#define GetInt4(c) (((unsigned long)c[0] << 24)|((unsigned long)c[1] << 16)\
			|((unsigned long)c[2] << 8)| ((unsigned long)c[3]))
#define PutInt2(c,i) ((c[1] = (i) & 0xff), (c[0] = ((i) >> 8) & 0xff))
#define PutInt4(c,i) ((c[0]=((i) >> 24) & 0xff),(c[1]=((i) >> 16) & 0xff),\
			(c[2] = ((i) >> 8) & 0xff), (c[3] = (i) & 0xff))

/*
 * magic numbers.
 */
#define PRE_MAGIC	0xc0de
#define PRE_VERSION	0x1000
#define PRE_FORTEZZA_FILE	0x00ff      /* pre-encrypted file on disk */
#define PRE_FORTEZZA_STREAM	0x00f5      /* pre-encrypted file in stream */
#define PRE_FORTEZZA_GEN_STREAM	0x00f6  /* Generated pre-encrypted file */

/*
 * internal implementation info
 */

typedef struct PEContextStr PEContext;

struct PEContextStr {
    enum { pec_none, pec_fortezza } type;
    void *base_context;
    SSLCipher decrypt;
    SSLDestroy destroy;
};

/* convert an existing stream header to a version with local parameters */
PEHeader *SSL_PreencryptedStreamToFile(int fd, PEHeader *);

/* convert an existing file header to one suitable for streaming out */
PEHeader *SSL_PreencryptedFileToStream(int fd, PEHeader *);

/* create a preenc context for decrypting */
PEContext *PE_CreateContext(PEHeader *, void *window_id);

/* destroy a preenc context */
void PE_DestroyContext(PEContext *, PRBool);

/* decrypt a block of data */
SECStatus PE_Decrypt(PEContext *, unsigned char *out, unsigned *part,
                    unsigned int maxOut, unsigned char *in, unsigned inLen);



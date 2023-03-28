#include "crypto.h"
#include "cert.h"
#include "secrng.h"
#include "secutil.h"
#include "prglobal.h"
#include "prlong.h"
#include "prtime.h"

#define MAX_RSA_MODULUS_BYTES (1024/8)
#define DEFAULT_ITERS 10

typedef struct TimingContextStr TimingContext;

struct TimingContextStr {
    int64 start;
    int64 end;
    int64 interval;
    long days;     
    int hours;    
    int minutes;  
    int seconds;  
    int millisecs;
};

TimingContext *CreateTimingContext(void) {
    return XP_ALLOC(sizeof(TimingContext));
}

void DestroyTimingContext(TimingContext *ctx) {
    XP_FREE(ctx);
}

void TimingBegin(TimingContext *ctx) {
    ctx->start = PR_Now();
}

static void timingUpdate(TimingContext *ctx) {
    int64 tmp, remaining;
    int64 L1000,L60,L24;

    LL_I2L(L1000,1000);
    LL_I2L(L60,60);
    LL_I2L(L24,24);

    LL_DIV(remaining, ctx->interval, L1000);
    LL_MOD(tmp, remaining, L1000);
    LL_L2I(ctx->millisecs, tmp);
    LL_DIV(remaining, remaining, L1000);
    LL_MOD(tmp, remaining, L60);
    LL_L2I(ctx->seconds, tmp);
    LL_DIV(remaining, remaining, L60);
    LL_MOD(tmp, remaining, L60);
    LL_L2I(ctx->minutes, tmp);
    LL_DIV(remaining, remaining, L60);
    LL_MOD(tmp, remaining, L24);
    LL_L2I(ctx->hours, tmp);
    LL_DIV(remaining, remaining, L24);
    LL_L2I(ctx->days, remaining);
}

void TimingEnd(TimingContext *ctx) {
    ctx->end = PR_Now();
    LL_SUB(ctx->interval, ctx->end, ctx->start);
    XP_ASSERT(LL_GE_ZERO(ctx->interval));
    timingUpdate(ctx);
}

void TimingDivide(TimingContext *ctx, int divisor) {
    int64 tmp;

    LL_I2L(tmp, divisor);
    LL_DIV(ctx->interval, ctx->interval, tmp);

    timingUpdate(ctx);
}

char *TimingGenerateString(TimingContext *ctx) {
    char *buf = NULL;

    if (ctx->days != 0) {
	buf = PR_sprintf_append(buf, "%d days", ctx->days);
    }
    if (ctx->hours != 0) {
	if (buf != NULL) buf = PR_sprintf_append(buf, ", ");
	buf = PR_sprintf_append(buf, "%d hours", ctx->hours);
    }
    if (ctx->minutes != 0) {
	if (buf != NULL) buf = PR_sprintf_append(buf, ", ");
	buf = PR_sprintf_append(buf, "%d minutes", ctx->minutes);
    }
    if (buf != NULL) buf = PR_sprintf_append(buf, ", and ");
    if (ctx->millisecs == 0) {
	buf = PR_sprintf_append(buf, "%d seconds", ctx->seconds);
    } else {
	buf = PR_sprintf_append(buf, "%d.%03d seconds",
				ctx->seconds, ctx->millisecs);
    }
    return buf;
}

void
Usage(char *progName)
{
    fprintf(stderr, "Usage: %s [-d certdir] [-i itterations] [-s | -e]"
	            " -n nickname\n",
	    progName);
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "-d certdir");
    fprintf(stderr, "%-20s How many operations to perform\n", "-i itterations");
    fprintf(stderr, "%-20s Perform signing (private key) operations\n", "-s");
    fprintf(stderr, "%-20s Perform encryption (public key) operations\n", "-e");
    fprintf(stderr, "%-20s Nickname of certificate or key\n", "-n nickname");
    exit(-1);
}

SECStatus
DoRSAPublicOps(SECKEYLowPublicKey *key, unsigned char *data, int iters)
{
    int modulusLen;
    unsigned char input[MAX_RSA_MODULUS_BYTES];
    unsigned char output[MAX_RSA_MODULUS_BYTES];
    SECStatus rv;
    
    modulusLen = SECKEY_LowPublicModulusLen(key);

    XP_MEMCPY(input, data, modulusLen);

    while(iters--) {
	rv = RSA_PublicKeyOp(key, output, input, modulusLen);
	if (rv != SECSuccess) return rv;
	XP_MEMCPY(&output, &input, modulusLen);
    }
    return rv;
}

SECStatus
DoRSAPrivateOps(SECKEYLowPrivateKey *key, unsigned char *data, int iters)
{
    int modulusLen;
    unsigned char input[MAX_RSA_MODULUS_BYTES];
    unsigned char output[MAX_RSA_MODULUS_BYTES];
    SECStatus rv;
    
    modulusLen = SECKEY_LowPrivateModulusLen(key);

    XP_MEMCPY(input, data, modulusLen);

    while(iters--) {
	rv = RSA_PrivateKeyOp(key, output, input, modulusLen);
	if (rv != SECSuccess) return rv;
	XP_MEMCPY(&output, &input, modulusLen);
    }
    return rv;
}

int
main(int argc, char **argv)
{
    TimingContext *timeCtx;
    SECKEYLowPrivateKey *privKey;
    SECKEYPublicKey *pubHighKey;
    SECKEYLowPublicKey *pubKey;
    CERTCertificate *cert;
    int opt;
    char *secDir = NULL;
    char *nickname = NULL;
    long iters = DEFAULT_ITERS;
    PRBool doPriv = PR_FALSE, doPub = PR_FALSE;
    SECKEYKeyDBHandle *keydb;
    CERTCertDBHandle certdb;
    int rv;
    unsigned char buf[1024];
    unsigned int modulus_len;
    int i;
    
    while ((opt = getopt(argc, argv, "d:i:sen:")) != EOF) {
	switch (opt) {
	  case '?':
	    Usage(argv[0]);
	    break;
          case 'd':
	    secDir = XP_STRDUP(optarg);
	    break;
	  case 'i':
	    iters = atol(optarg);
	    break;
	  case 's':
	    doPriv = PR_TRUE;
	    break;
	  case 'e':
	    doPub = PR_TRUE;
	    break;
	  case 'n':
	    nickname = XP_STRDUP(optarg);
	    break;
	}
    }

    if ((doPriv && doPub) || (nickname == NULL)) Usage(argv[0]);

    if (!doPriv && !doPub) doPriv = PR_TRUE;

    PR_Init(argv[0], 1, 1, 0);
    SEC_Init();

    rv = CERT_OpenCertDBFilename(&certdb, NULL, PR_TRUE);
    if (rv != SECSuccess) {
	fprintf(stderr, "Can't open certificate database.\n");
	exit(1);
    }

    if (doPub) {
	cert = CERT_FindCertByNickname(&certdb, nickname);
	if (cert == NULL) {
	    fprintf(stderr,
		    "Can't find certificate by name \"%s\"\n", nickname);
	    exit(1);
	}
	pubHighKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
	pubKey = SECU_ConvHighToLow(pubHighKey);
	if (pubKey == NULL) {
	    fprintf(stderr, "Can't extract public key from certificate");
	    exit(1);
	}
	modulus_len = SECKEY_LowPublicModulusLen(pubKey);
    }

    if (doPriv) {
        keydb = SECU_OpenKeyDB();
      
	if (keydb == NULL) {
	    fprintf(stderr, "Can't open key database.\n");
	    exit(1);
	}
	privKey = SECU_FindLowPrivateKeyFromNickname(nickname);
	if (privKey == NULL) {
	    fprintf(stderr,
		    "Can't find private key by name \"%s\"\n", nickname);
	    exit(1);
	}
	modulus_len = SECKEY_LowPrivateModulusLen(privKey);
    }

    RNG_GenerateGlobalRandomBytes(buf, sizeof(buf));

    timeCtx = CreateTimingContext();
    TimingBegin(timeCtx);
    i = iters;
    while(i--) {
	buf[0] &= 0x3f;
	if (doPub) {
	    rv = RSA_PublicKeyOp(pubKey, buf, buf, modulus_len);
	} else if (doPriv) {
	    rv = RSA_PrivateKeyOp(privKey, buf, buf, modulus_len);
	}
	if (rv != SECSuccess) {
	    fprintf(stderr, "Error in RSA operation\n");
	    exit(1);
	}
    }
    TimingEnd(timeCtx);
    printf("%d iterations in %s\n",
	   iters, TimingGenerateString(timeCtx));
    TimingDivide(timeCtx, iters);
    printf("one operation every %s\n", TimingGenerateString(timeCtx));
    exit(0);
}

/*
** Example:
**
** CV bytes:	ac cb 21 10 e5
** Salt bytes:	e5 38 b0 00 00 00 00 00 00 00 00 
** Plaintext: 	15 f8 c1 f3 80 ee 7e 68 00 00 00 00 00 00 00 00
**
** Ciphertext:	55 12 a8 06 41 3c 66 61 e1 2b cb 73 a5 38 5a 0d
**
*/
#include "sec.h"
#include <stdio.h>

static void Convert(SECItem *d, char *data)
{
    unsigned char tmp[100];
    unsigned char *tp = tmp;
    char ch;

    d->data = 0;
    d->len = 0;
    while ((ch = *data++) != 0) {
	if (ch == ' ') continue;
	if ((ch >= '0') && (ch <= '9')) *tp = (ch - '0') << 4;
	else if ((ch >= 'a') && (ch <= 'f')) *tp = (ch - 'a' + 10) << 4;
	else if ((ch >= 'A') && (ch <= 'F')) *tp = (ch - 'A' + 10) << 4;
	ch = *data++;
	if ((ch >= '0') && (ch <= '9')) *tp |= ch - '0';
	else if ((ch >= 'a') && (ch <= 'f')) *tp |= ch - 'a' + 10;
	else if ((ch >= 'A') && (ch <= 'F')) *tp = ch - 'A' + 10;
	tp++;
    }
    d->len = tp - tmp;
    d->data = (unsigned char*) malloc(d->len);
    memcpy(d->data, tmp, d->len);
}

static void PrintHex(unsigned char *d, unsigned len)
{
    unsigned char *cp = d;
    unsigned char *end = cp + len;
    while (cp < end) {
	printf("%02x ", *cp++);
    }
    printf("\n");
}

void main(int argc, char **argv)
{
    char cvb[1000], sb[1000], pt[1000];
    unsigned char rc2enc[500], rc4enc[500];
    unsigned char key[500];
    unsigned keyLen, rc2len, rc4len;
    SECItem cv, s, p;
    RC2Context *rc2;
    RC4Context *rc4;

    printf("CV bytes: ");
    gets(cvb);
    printf("Salt bytes: ");
    gets(sb);
    printf("Plain text: ");
    gets(pt);
    Convert(&cv, cvb);
    Convert(&s, sb);
    Convert(&p, pt);

    memcpy(key, cv.data, cv.len);
    memcpy(key+cv.len, s.data, s.len);
    keyLen = cv.len + s.len;

    rc2 = RC2_CreateContext(key, keyLen, 0, SEC_RC2);
    RC2_Encrypt(rc2, rc2enc, &rc2len, sizeof(rc2enc), p.data, p.len);
    rc4 = RC4_CreateContext(key, keyLen);
    RC4_Encrypt(rc4, rc4enc, &rc4len, sizeof(rc4enc), p.data, p.len);

    printf("-------------------------------\n");
    printf("CV bytes: ");
    PrintHex(cv.data, cv.len);
    printf("Salt bytes: ");
    PrintHex(s.data, s.len);
    printf("Plain text: ");
    PrintHex(p.data, p.len);
    printf("RC2 Cipher text: ");
    PrintHex(rc2enc, rc2len);
    printf("RC4 Cipher text: ");
    PrintHex(rc4enc, rc4len);
}

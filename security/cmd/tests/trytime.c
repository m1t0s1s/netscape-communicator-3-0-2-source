#include "sec.h"
#include <stdio.h>

void main(int argc, char **argv)
{
    for (;;) {
	char b1[1000], b2[1000];
	SECValidity *v;
	time_t now, time1, time2;
	SECItem n, t1, t2;
	int rv;

	printf("Time1: "); gets(b1);
	printf("Time2: "); gets(b2);
	now = time(0);

	rv = DER_TimeToUTCTime(&n, now);
	if (rv) { printf("Bad utc conversion of now\n"); continue; }
	rv = DER_AsciiToTime(&time1, b1);
	if (rv) { printf("Bad utc in time1\n"); continue; }
	rv = DER_AsciiToTime(&time2, b2);
	if (rv) { printf("Bad utc in time2\n"); continue; }
	rv = DER_TimeToUTCTime(&t1, time1);
	if (rv) { printf("Bad utc conversion of time1\n"); continue; }
	rv = DER_TimeToUTCTime(&t2, time2);
	if (rv) { printf("Bad utc conversion of time2\n"); continue; }

	printf("Now = %*s\n", n.len, n.data);
	printf("Time1 = %*s\n", t1.len, t1.data);
	printf("Time2 = %*s\n", t2.len, t2.data);
	v = SEC_CreateValidity(time1, time2);
	if (!v) {
	    printf("Create validity failed, error=%d\n", XP_GetError());
	    continue;
	}
	rv = SEC_ValidTime(now, v);
	if (rv == SEC_ERROR_INVALID_TIME) {
	    printf("Now is not in time1 && time2\n");
	} else {
	    printf("Now is in range of time1 && time2\n");
	}
	SEC_DestroyValidity(v, 1);
    }
}

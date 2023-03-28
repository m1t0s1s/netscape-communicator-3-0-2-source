#include "prlong.h"
#ifndef NSPR20
#include "prmjtime.h"
#endif /* NSPR20 */
#include "prtime.h"
#include "secder.h"
#include "cert.h"
#include "secitem.h"

DERTemplate CERTValidityTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTValidity) },
    { DER_UTC_TIME,
	  offsetof(CERTValidity,notBefore), },
    { DER_UTC_TIME,
	  offsetof(CERTValidity,notAfter), },
    { 0, }
};

static char *DecodeUTCTime2FormattedAscii (SECItem *utcTimeDER, char *format);

/* convert DER utc time to ascii time string */
char *
DER_UTCTimeToAscii(SECItem *utcTime)
{
    return (DecodeUTCTime2FormattedAscii (utcTime, "%a %b %d %T %Y"));
}

/* convert DER utc time to ascii time string, only include day, not time */
char *
DER_UTCDayToAscii(SECItem *utctime)
{
    return (DecodeUTCTime2FormattedAscii (utctime, "%a %b %d, %Y"));
}

CERTValidity *
CERT_CreateValidity(int64 notBefore, int64 notAfter)
{
    CERTValidity *v;
    int rv;
    PRArenaPool *arena;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return(0);
    }
    
    v = (CERTValidity*) PORT_ArenaZAlloc(arena, sizeof(CERTValidity));
    if (v) {
	v->arena = arena;
	rv = DER_TimeToUTCTime(&v->notBefore, notBefore);
	if (rv) goto loser;
	rv = DER_TimeToUTCTime(&v->notAfter, notAfter);
	if (rv) goto loser;
    }
    return v;

  loser:
    CERT_DestroyValidity(v);
    return 0;
}

SECStatus
CERT_CopyValidity(PRArenaPool *arena, CERTValidity *to, CERTValidity *from)
{
    SECStatus rv;

    CERT_DestroyValidity(to);
    to->arena = arena;
    
    rv = SECITEM_CopyItem(arena, &to->notBefore, &from->notBefore);
    if (rv) return rv;
    rv = SECITEM_CopyItem(arena, &to->notAfter, &from->notAfter);
    return rv;
}

void
CERT_DestroyValidity(CERTValidity *v)
{
    if (v && v->arena) {
	PORT_FreeArena(v->arena, PR_FALSE);
    }
    return;
}

char *
CERT_UTCTime2FormattedAscii (int64 utcTime, char *format)
{
#ifndef NSPR20
    PRTime printableTime; 
#else
    PRExplodedTime printableTime; 
#endif /* NSPR20 */
    char *timeString;
   
    /* Converse time to local time and decompose it into components */
#ifndef NSPR20
    PR_ExplodeTime(&printableTime, utcTime);
#else
    PR_ExplodeTime(utcTime, PR_GMTParameters, &printableTime);
#endif /* NSPR20 */
    
    timeString = (char *)PORT_Alloc(100);

    if ( timeString ) {
        PR_FormatTime( timeString, 100, format, &printableTime );
    }
    
    return (timeString);
}

char *CERT_GenTime2FormattedAscii (int64 genTime, char *format)
{
#ifndef NSPR20
    PRTime printableTime; 
#else
    PRExplodedTime printableTime; 
#endif /* NSPR20 */
    char *timeString;
   
    /* Converse time to local time and decompose it into components */
#ifndef NSPR20
    PR_ExplodeGMTTime(&printableTime, genTime);
#else
    PR_ExplodeTime(genTime, PR_GMTParameters, &printableTime);
#endif /* NSPR20 */
    
    timeString = (char *)PORT_Alloc(100);

    if ( timeString ) {
        PR_FormatTime( timeString, 100, format, &printableTime );
    }
    
    return (timeString);
}


/* convert DER utc time to ascii time string, The format of the time string
   depends on the input "format"
 */
static char *
DecodeUTCTime2FormattedAscii (SECItem *utcTimeDER,  char *format)
{
    int64 utcTime;
    int rv;
   
    rv = DER_UTCTimeToTime(&utcTime, utcTimeDER);
    if (rv) {
        return(NULL);
    }
    return (CERT_UTCTime2FormattedAscii (utcTime, format));
}

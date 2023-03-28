#ifndef _RESDEF_H_
#define _RESDEF_H_


#include "xp_core.h"


#define RES_OFFSET 7000


#ifndef RESOURCE_STR

#ifdef WANT_ENUM_STRING_IDS

#define RES_START
#define BEGIN_STR(arg)   enum {
#define ResDef(name,id,msg)	name=id,
#ifdef XP_WIN
#define END_STR(arg)	};
#else
#define END_STR(arg)     arg=0 };
#endif

#else /* WANT_ENUM_STRING_IDS */

#define RES_START
#define BEGIN_STR(arg)
#define ResDef(name,id,msg)	int name = (id);
#define END_STR(arg)

#endif /* WANT_ENUM_STRING_IDS */

#else  /* RESOURCE_STR, the definition here is for building resources */
#ifdef   XP_WIN

#ifndef MOZILLA_CLIENT
#define RES_START
#define BEGIN_STR(arg) static char * (arg) (int16 i) { switch (i) {
#define ResDef(name,id,msg)	case (id)+RES_OFFSET: return (msg);
#define END_STR(arg) } return NULL; }
#else /* MOZILLA_CLIENT */
#define RES_START STRINGTABLE DISCARDABLE
#define BEGIN_STR(arg)  BEGIN
#define ResDef(name,id,msg)	id+RES_OFFSET msg
#define END_STR(arg)   END
#endif /* not MOZILLA_CLIENT */

#elif defined(XP_MAC)
	/* Do nothing -- leave ResDef() to be perl'ized via MPW */
#define ResDef(name,id,msg)	ResDef(name,id,msg)

#elif defined(XP_UNIX)
#ifdef RESOURCE_STR_X
#define RES_START
#define BEGIN_STR(arg) static char *(arg)(void) {
#define ResDef(name,id,msg)	output((id)+RES_OFFSET, (msg));
#define END_STR(arg) }
#else
#define RES_START
#define BEGIN_STR(arg) static char *(arg)(int16 i) { switch (i) {
#define ResDef(name,id,msg)	case (id)+RES_OFFSET: return (msg);
#define END_STR(arg) } return NULL; }
#endif /* RESOURCE_STR_X */
#endif   /*  XP_WIN  */
#endif /* RESOURCE_STR   */


#endif /* _RESDEF_H_ */

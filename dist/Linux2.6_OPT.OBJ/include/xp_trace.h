#ifndef _XP_Trace_
#define _XP_Trace_

#ifdef DEBUG

#ifdef __cplusplus
extern "C" void XP_Trace (const char * format, ...);
#else
void XP_Trace (const char * format, ...);
#endif

#define XP_TRACE(MESSAGE) \
    XP_Trace MESSAGE
#define XP_LTRACE(FLAG,LEVEL,MESSAGE) \
    do { if (FLAG >= (LEVEL)) XP_Trace MESSAGE; } while (0)

#else

#ifdef __cplusplus
inline void XP_Trace (char *, ...) {}
#else
void XP_Trace (const char * format, ...);
#endif

#define	XP_TRACE(MESSAGE) \
    ((void) (MESSAGE))
#define XP_LTRACE(FLAG,LEVEL,MESSAGE) \
    ((void) (MESSAGE))

#endif

#endif /* _XP_Trace_ */

#ifndef MKMOCHA_H
#define MKMOCHA_H
/*
 * Mocha netlib-specific stuff.
 */
#ifdef MOCHA

extern int  net_OpenMochaURL(ActiveEntry *ae);
extern void net_AbortMochaURL(ActiveEntry *ae);
extern int  net_InterruptMocha(ActiveEntry *ae);

#endif /* MOCHA */
#endif /* MKMOCHA_H */

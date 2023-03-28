#ifndef libjava_lj_h___
#define libjava_lj_h___

#include "java.h"
#include "jri.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Internal (to the libjava library) procedures and variables */

extern int LJ_CurrentDocID(MWContext* context);

extern jref LJ_InvokeMethod(JRIMethodID method, ...);
extern jref LJ_MakeArrayOfString(int size);
extern int LJ_SetStringArraySlot(jref hs, int slot, char *name, char *value,
                                 int16 encoding);

extern JRIEnv* mozenv;

extern int lj_java_enabled;
extern int lj_init_failed;
extern int lj_bad_version;
extern int lj_correct_version;
extern int lj_is_started;
extern char *lj_zip_name;

/****	****	****	****	****	****	****	****	****
 *   mozilla/java interlock stuff
 */

/* initialize the mozilla/java interlock */
extern void lj_InitThread(void);

/* from java env "env", call the given callback within the mozilla thread */
extern void LJ_CallMozilla(JRIEnv *env, LJCallback doit, void *data);

/* from mozilla, call the given callback with the given java env. */
extern void LJ_CallJava(JRIEnv *env, LJCallback doit, void *data);

/* return PR_TRUE if the mozilla thread is running and able to handle
 * requests.  if it returns PR_FALSE, then any synchronous network
 * activity (i.e. java stuf) will deadlock the browser.
 * see also sun-java/netscape/net/inStr.c */
PRBool LJ_MozillaRunning(JRIEnv *env);

#ifdef __cplusplus
};
#endif

#endif /* libjava_lj_h___ */

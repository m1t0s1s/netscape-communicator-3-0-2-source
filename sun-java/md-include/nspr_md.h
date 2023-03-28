#ifndef sun_java_nspr_md_h___
#define sun_java_nspr_md_h___

#include "prtypes.h"
#undef SET_BIT
#undef TEST_BIT
#undef CLEAR_BIT

#include "prosdep.h"
#include "prmacros.h"

NSPR_BEGIN_EXTERN_C

extern void intrInit(void);

extern void InitializeAsyncIO(void);

extern void InitializeMem(void);

NSPR_END_EXTERN_C

#endif /* sun_java_nspr_md_h___ */

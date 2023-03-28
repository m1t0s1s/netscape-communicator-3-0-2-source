/* the context function table
 *
 * This was typedef'd to ContextFuncs in structs.h
 */
#ifndef _ContextFunctions_
#define _ContextFunctions_

struct _ContextFuncs {
#define MAKE_FE_FUNCS_STRUCT
#include "mk_cx_fn.h"
};

#endif /* _ContextFunctions_ */


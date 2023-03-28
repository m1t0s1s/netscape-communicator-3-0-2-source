#ifdef TRACING
#include "oobj.h"
#include "interpreter.h"
#include "javaThreads.h"
#include "prlog.h"
#include "prprf.h"

PR_LOG_DEFINE(Method);

void trace_method(struct execenv* ee, struct methodblock* mb, 
		  int args_size, int type) 
{
#if 0	/* XXX We should be using this: */
    HArrayOfChar* threadName = threadSelf() ? getThreadName() : NULL;
#endif
    JavaFrame *frame;
    int depth;
    int i;
    char *s;

    for (frame = ee->current_frame, depth = 0;
	   frame != NULL; 
	   frame = frame->prev)
	if (frame->current_method != 0) depth++;
    if (type != TRACE_METHOD_ENTER) 
	depth--;

    s = PR_sprintf_append(0, "# [%2d] ", depth);
    for (i = 0; i < depth; i++) 
	s = PR_sprintf_append(s, "| ");

    s = PR_sprintf_append(s, "%c %s.%s%s ",
			  ((type == TRACE_METHOD_ENTER) ? '>' : '<'), 
			  classname(fieldclass(&mb->fb)), 
			  fieldname(&mb->fb),
			  fieldsig(&mb->fb));

    if (type == TRACE_METHOD_ENTER) 
	s = PR_sprintf_append(s, "(%d) entered", args_size);
    else if (exceptionOccurred(ee))
	s = PR_sprintf_append(s, "throwing %s",
			  ee->exception.exc->methods->classdescriptor->name);
    else
	s = PR_sprintf_append(s, "returning");
    if (s) {
	PR_LOG(Method, out, ("%s", s));
	free(s);
    }
}
#endif

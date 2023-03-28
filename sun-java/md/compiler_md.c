#include "oobj.h"
#include "interpreter.h"

void ErrorUnwind(void)
{
}

void InitializeForCompiler_md(void)
{
}

void InvokeCompiledMethodVoid(struct methodblock *mb, void *a,
			      stack_item *b, int c)
{
    abort();
}

int InvokeCompiledMethodInt(struct methodblock *mb, void *a,
			    stack_item *b, int c)
{
    abort();
    return 0;
}

int64_t InvokeCompiledMethodLong(struct methodblock *mb, void *a,
				 stack_item *b, int c)
{
    int64 rv;
    abort();
    return rv;
}

float InvokeCompiledMethodFloat(struct methodblock *mb, void *a,
				stack_item *b, int c)
{
    abort();
    return 0;
}

double InvokeCompiledMethodDouble(struct methodblock *mb, void *a,
				  stack_item *b, int c)
{
    abort();
    return 0;
}

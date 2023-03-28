#include "prlog.h"

PR_LOG_DEFINE(Foo);
PR_LOG_DEFINE(Fact);

int fact(int n) {
    int result;
    PR_LOG_BEGIN(Fact, debug, ("calling fact(%d)", n));
    if (n == 1)
	result = 1;
    else
	result = n * fact(n - 1);
    PR_LOG_END(Fact, debug, ("fact(%d) returning %d", n, result));
    return result;
}

int main() {
    int err = -1;
    PR_Init("main", 10, 10, 0);
    PR_LOG(Foo, warn, ("doit got %d", err));

    PR_LOG_BEGIN(Foo, error, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, warn, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));

    PR_LOG(Foo, warn, ("doit got %d", err));

    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_BEGIN(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, debug, ("doit got %d", err));
    PR_LOG_END(Foo, warn, ("doit got %d", err));
    PR_LOG_END(Foo, error, ("doit got %d", err));

    fact(3);

    PR_LOG_FLUSH();
    return 0;
}

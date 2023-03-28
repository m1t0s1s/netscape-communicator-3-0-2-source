/* -*- Mode: C; tab-width: 4; -*- */

#include "prlog.h"
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "prthread.h"
#include "prfile.h"
#include "prmem.h"
#include "prprf.h"

#ifdef XP_MAC
#include <Types.h>
#include <unistd.h>
#include <fcntl.h>
#endif /* XP_MAC */

#ifdef SW_THREADS
#include "swkern.h"
#else
#include "hwkern.h"
#endif

#ifdef XP_PC
#define strcasecmp(a,b)		strcmpi(a,b)
#define STDERR_FILENO		2
#endif /* XP_PC */

#define OUT_BUF_SIZE	16384

static PROSFD pr_log = STDERR_FILENO;	/* log to stderr by default */
int pr_logSync = 1;						/* synchronous logging by default */
static char* pr_outbuf = NULL;
static int pr_outbuflen;

static PRLogLevel
pr_name2level(const char* name)
{
    int value = atoi(name);

    if (value > PRLogLevel_none && value <= PRLogLevel_debug)
		return (PRLogLevel)value;
    else if (strcasecmp(name, "out") == 0
			 || strcasecmp(name, "stdout") == 0
			 || value == PRLogLevel_out)
		return PRLogLevel_out;
    else if (strcasecmp(name, "err") == 0
			 || strcasecmp(name, "error") == 0
			 || value == PRLogLevel_error)
		return PRLogLevel_error;
    else if (strcasecmp(name, "wrn") == 0
			 || strcasecmp(name, "warn") == 0
			 || value == PRLogLevel_warn)
		return PRLogLevel_warn;
    else if (strcasecmp(name, "dbg") == 0
			 || strcasecmp(name, "debug") == 0
			 || value == PRLogLevel_debug)
		return PRLogLevel_debug;

    return PRLogLevel_none;
}

static const char*
pr_level2name(PRLogLevel level)
{
    switch (level) {
      case PRLogLevel_none:		return "none";
      case PRLogLevel_out:		return "stdout";
      case PRLogLevel_error:	return "error";
      case PRLogLevel_warn:		return "warn";
      case PRLogLevel_debug:	return "debug";
      default:					return "?";
    }
}

static char*
pr_ParseNameValue(char* ev, int *len, char* name, char* value)
{
    int evlen = *len;
    char dummy[64];
    int count = 0, delta = 0;

    if (!(ev && ev[0])) return ev;
    evlen = strlen(ev);
    name[0] = value[0] = '\0';

    count = sscanf(ev, "%64[,; ]%n", dummy, &delta); /* skip noise */
    if (count) {
		ev += delta;
		evlen -= delta;
    }

    count = sscanf(ev, "%64[A-Za-z0-9] %n", name, &delta);
    if (count == 0) {
		fprintf(stderr, "PRLOG: Can't parse name %s\n", ev);
		return ev;
    }
    ev += delta;
    evlen -= delta;

    count = sscanf(ev, "%64[:= ]%64[A-Za-z0-9/\\:.]%n%64[,; ]%n",
				   dummy, value, &delta, dummy, &delta);
    if (count == 0) {
		fprintf(stderr, "PRLOG: Can't parse value for %s: %s\n",
				name, ev);
		return ev;
    }
    ev += delta;
    evlen -= delta;

    /*fprintf(stderr, ">>> %s : %s [%s] %d\n", name, value, ev, count);*/
    *len = evlen;
    return ev;
}

static void
pr_LogSystemInit(char* ev)
{
    char name[64], value[64];
    char line[400];
    int evlen;

    if (!(ev && ev[0])) return;

    evlen = strlen(ev);
    while (evlen > 0) {
		ev = pr_ParseNameValue(ev, &evlen, name, value);
		if (strcasecmp(name, "file") == 0) {
#if defined(XP_UNIX)
			pr_log = open(value, O_WRONLY|O_CREAT|O_TRUNC, 0644);
#elif defined(XP_PC)
			pr_log = _open(value, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
#elif defined(XP_MAC)
			pr_log = PR_OpenFile(value, O_WRONLY|O_CREAT|O_TRUNC, 0644);
#endif
			if (pr_log < 0) {
				PR_snprintf(line, sizeof(line),
							"PRLOG: unable to create log file \"%s\"\n",
							value);
				write(2, line, strlen(line));
				return;
			}
			fprintf(stderr, "PRLOG: Logging to \"%s\"\n", value);
		}
		else if (strcasecmp(name, "sync") == 0) {
			int v = atoi(value);
			pr_logSync = (v ? 1 : 0);
			fprintf(stderr, "PRLOG: Logging is %ssynchronous\n", 
					(v ? "" : "a"));
		}
    }
    /*
    ** Get a big buffer to reduce i/o overhead
    */
    if (!pr_logSync && pr_outbuf == NULL) {
		pr_outbuf = (char*)malloc(OUT_BUF_SIZE);
    }
}

PR_PUBLIC_API(void)
PR_LogInit(PRLogModule* module)
{
    char* ev;
    int evlen;
    static int pr_logSystemInitialized = 0;

    if (module->depth >= 0) {
		/* then already initialized */
		return;
    }

    ev = getenv("PRLOG");

    if (!pr_logSystemInitialized) {
		pr_LogSystemInit(ev);
		pr_logSystemInitialized = 1;
    }

	module->level = PRLogLevel_none;	/* set default */
    if (!(ev && ev[0])) return;
    evlen = strlen(ev);
    while (evlen > 0) {
		char name[64], value[64];
		ev = pr_ParseNameValue(ev, &evlen, name, value);
		if (strcasecmp(name, module->name) == 0
			|| strcasecmp(name, "*") == 0) {
			/* found our module */
			PRLogLevel level = pr_name2level(value);
			module->level = level;
			if (level == PRLogLevel_none)
				fprintf(stderr, "PRLOG: Disabling %s logs\n",
						module->name);
			else
				fprintf(stderr, "PRLOG: Enabling %s %s logs\n",
						module->name, pr_level2name(level));
			break;
		}
    }
    module->depth = 0;	/* mark as initialized */
}

PR_PUBLIC_API(int)
PR_LogEnter(PRLogModule* module, PRLogLevel level, int direction)
{
    PRThread* thread = PR_CurrentThread();
    char* name = (thread ? thread->name : "main");
    int depth = module->depth;
    char line[400];
    int len, i, s;

    if (depth < 0)
		PR_LogInit(module);

    if (module->level < level)
		return PR_LOG_SKIP;

    len = PR_snprintf(line, sizeof(line), "%s %s %s", name,
					  module->name, pr_level2name(level));
    if (direction == 0) {
		len += PR_snprintf(&line[len], sizeof(line)-len, ":\t");
    }
    else if (direction > 0) {
		len += PR_snprintf(&line[len], sizeof(line)-len, ">\t");
		module->depth += direction;
		depth += direction;
    }
    else {
		len += PR_snprintf(&line[len], sizeof(line)-len, "<\t");
		module->depth += direction;
    }

    if (depth > 1) {
		len += PR_snprintf(&line[len], sizeof(line)-len,
						   "%-2d", depth);
		for (i = 0; i < depth-2; i++) {
			len += PR_snprintf(&line[len], sizeof(line)-len,
							   "| ");
		}
    }

    if (pr_logSync) {
		_OS_WRITE(pr_log, line, len);
		return 0;
    }
    else {
		_PR_BEGIN_CRITICAL_SECTION(s);	/* end in PR_LogExit */
		if (len + pr_outbuflen > OUT_BUF_SIZE) {
			_OS_WRITE(pr_log, pr_outbuf, pr_outbuflen);
			pr_outbuflen = 0;
		}
		memcpy(pr_outbuf + pr_outbuflen, line, len);
		pr_outbuflen += len;
		return s;
    }
}

PR_PUBLIC_API(void)
PR_LogPrint(const char* format, ...)
{
    char line[400];
    int len;
    va_list args;

    va_start(args, format);
    len = PR_vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    if (pr_logSync) {
		_OS_WRITE(pr_log, line, len);
		return;
    }
    else {
		if (len + pr_outbuflen > OUT_BUF_SIZE) {
			_OS_WRITE(pr_log, pr_outbuf, pr_outbuflen);
			pr_outbuflen = 0;
		}
		memcpy(pr_outbuf + pr_outbuflen, line, len);
		pr_outbuflen += len;
    }
}

PR_PUBLIC_API(void)
PR_LogExit(int interruptStatus, const char* file, int lineno)
{
    char line[400];
    int len;
    len = PR_snprintf(line, sizeof(line), 
					  "\t[%s:%d]\n", file, lineno);

    if (pr_logSync) {
		_OS_WRITE(pr_log, line, len);
		return;
    }
    else {
		if (len + pr_outbuflen > OUT_BUF_SIZE) {
			_OS_WRITE(pr_log, pr_outbuf, pr_outbuflen);
			pr_outbuflen = 0;
		}
		memcpy(pr_outbuf + pr_outbuflen, line, len);
		pr_outbuflen += len;
		/* began in PR_LogEnter: */
		_PR_END_CRITICAL_SECTION(interruptStatus);
    }
}

PR_PUBLIC_API(void)
PR_LogFlush(void)
{
    int s;

    _PR_BEGIN_CRITICAL_SECTION(s);
    if (pr_outbuflen) {
		_OS_WRITE(pr_log, pr_outbuf, pr_outbuflen);
		pr_outbuflen = 0;
    }
    _PR_END_CRITICAL_SECTION(s);
}


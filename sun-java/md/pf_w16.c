#if defined(XP_PC) && !defined(_WIN32)

#include <nspr.h>
#include <prlog.h>
#include <prprf.h>


static FILE *logFile;
static char *logName;

static void
LogString(char *str)
{

	OutputDebugString(str);

	if (logFile == NULL) {
		if (logName==NULL) {
			logName = getenv("LOGPATH");
			if (logName==NULL)
				logName= "c:\\java.log";
		}
		logFile = fopen(logName,"w");
	}

	if (logFile != NULL) {
		fwrite(str,1,strlen(str),logFile);
		fflush(logFile);
	}

}

int
printf(const char *fmt, ...)
{
    char buffer[1024];
    int ret = 0;

    va_list args;
    va_start(args, fmt);

    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    LogString(buffer);

    va_end(args);
    return ret;
}

int
fprintf(void *dummy,const char *fmt, ...)
{
    char buffer[1024];
    int ret = 0;

    va_list args;
    va_start(args, fmt);

    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    LogString(buffer);

    va_end(args);
    return ret;
}

#endif

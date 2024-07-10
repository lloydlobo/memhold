/*
**
** memhold - Simple timeouts for processes that hog memory
**
**
*/

#include <assert.h> // Required for: assert()
#include <errno.h>  // Required for: errno
#include <signal.h> //Required for: SIGTERM, kill(), pid_t
#include <stdio.h>  // Required for: printf(), fprintf(), sprintf(), stderr, stdout, popen() [with compiler option `-pthread`]
#include <stdlib.h> // Required for: atoi(), exit()
#include <string.h> // Required for: strcmp(), NULL
#include <sys/wait.h>
#include <time.h>   // Required for: clock(), [time() ~ not used]
#include <unistd.h> // Required for: fork(), getpid(), sleep(),... [UNIX only lib]

#include "memhold.h"  // Declares module functions
#include "unittest.h" // Declares custom unit tests functions

#define MAX_HOT_LOOP_COUNT         64
#define MAX_RETRIES_FILE_NOT_FOUND 3

static const TraceLogLevelTuple TRACE_LOG_LEVELS[MAX_LOG_COUNT] = {
    TRACELEVELTOTUPLES(LOG_ALL, STRINGIFY(log)),     TRACELEVELTOTUPLES(LOG_TRACE, STRINGIFY(trace)),  TRACELEVELTOTUPLES(LOG_DEBUG, STRINGIFY(debug)),
    TRACELEVELTOTUPLES(LOG_INFO, STRINGIFY(info)),   TRACELEVELTOTUPLES(LOG_WARNING, STRINGIFY(warn)), TRACELEVELTOTUPLES(LOG_ERROR, STRINGIFY(error)),
    TRACELEVELTOTUPLES(LOG_FATAL, STRINGIFY(fatal)), TRACELEVELTOTUPLES(LOG_NONE, STRINGIFY(none)),
};

#define LOGSTREAM(level, format, ...)                                                                                                                          \
        do {                                                                                                                                                   \
                if (level >= LOG_TRACE) {                                                                                                                      \
                        fprintf(((level == LOG_WARNING) || (level == LOG_ERROR) || (level == LOG_FATAL)) ? stderr : stdout, "%ld [%s] " format "\n", clock(),  \
                                TRACE_LOG_LEVELS[level].label, __VA_ARGS__);                                                                                   \
                }                                                                                                                                              \
        } while (0); /* Write formatted output to STREAM.*/

struct Memhold {
        bool        log, verbose;
        const char *apiID;
        f32         refreshSec, cpuThresh;
        i64         memThresh;
        pid_t       userPID, mainPID;
} Memhold;

static struct Memhold mh = {
    .log        = true,
    .verbose    = true,
    .apiID      = MEMHOLD_ID,
    .refreshSec = 2.0f,
    .cpuThresh  = 50.0f,
    .memThresh  = 60000, // 100000 -- 100Mb
    .userPID    = 0,
    .mainPID    = 0,
};

static i32 fopenRetries = 0; // Count fopen retries

static u64 ReadFileToBuffer(const char *path, char *buf, u64 bufSize)
{
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
                fprintf(stderr, "error opening file %s: %s\n", path, strerror(errno));
                return (u64)(-1);
        }

        u64 bytesRead = fread(buf, 1, bufSize - 1, fp);
        if ((bytesRead == 0) && !feof(fp)) {
                fprintf(stderr, "error reading file %s: %s\n", path, strerror(errno));
                fclose(fp);
                return (u64)(-1);
        }

        fclose(fp);
        buf[bytesRead] = '\0'; // Null-terminate the buffer
        return bytesRead;
}

static i32 ParseProcStatusMem(i64 *mem)
{
        char buf[356], *line; // NOTE: 256 is not enough to hold data of /proc/PID/status
        snprintf(buf, sizeof(buf), "/proc/%d/status", mh.userPID);

        LOGSTREAM(LOG_DEBUG, "reading: %s", buf);
        u64 ret = ReadFileToBuffer(buf, buf, sizeof(buf)); //> -1 is error
        if (ret == 0 || ret == -1)                         //> 0 means bytes read(fread returned 0)
                return -1;

        (*mem) = -1;
        for (line = strtok(buf, "\n"); line; line = strtok(NULL, "\n")) {
                i32 cmpS1S2 = strncmp(line, "VmRSS:", 6);
                assert(cmpS1S2 == -1);
                if (!cmpS1S2) {
                        sscanf(line + 6, "%ld", mem);
                        break;
                }
        }

        return ((*mem) == -1) ? -1 : 0;
}

static i32 RunMain(void)
{
        i64    mem; // long
        ullong cpuTime1, cpuTime2;
        f64    cpuPercent;
        // Sleep USECONDS microseconds, or until a signal arrives that is not blocked or ignored.
        // This function is a cancellation point and therefore not marked with \_\_THROW.
        __useconds_t refreshMicroSeconds = 2 * 1000000; //> 2 seconds

        i32 i = 0;
        do {
                LOGSTREAM(LOG_DEBUG, "PID: %d", mh.userPID);

                if (ParseProcStatusMem(&mem)) {
                        perror("something went wrong while parsing MEM");
                        break;
                }

                LOGSTREAM(LOG_INFO, "PID: %d MEM: %.1LfMb", mh.userPID, (mem * 0.001L));

                if (mem > mh.memThresh) {
                        LOGSTREAM(LOG_WARNING, "*KILL* %d Memory usage: %ld (kB)", mh.userPID, mem);
                        // TODO: Only send signal 9(SIGKILL) if presence of memory overhead, exceeds or overloads the whole OS memory quota
                        //       Else we suspend this till things settle down
                        kill(mh.userPID, SIGTERM);
                        break;
                }
                usleep(refreshMicroSeconds);

                i++;
        } while (i < MAX_HOT_LOOP_COUNT);

        return 0;
}


int main(int argc, char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
                exit(1);
        }

        fprintf(stdout, "%s %s\n", MEMHOLD_ID, MEMHOLD_VERSION);
        if (argc == 3) {
                LOGSTREAM(LOG_INFO, "%s %s %s", argv[0], argv[1], argv[2]);
        }
        mh.mainPID = getpid();
        mh.userPID = atoi(argv[1]);

        return RunMain();
}

// file: main.c

// TOP

#include "memhold.h" // Declares module functions

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Required for: [Unix only] fork, wait, waitpid - basic process management
/* SYNOPSIS @load "fork" pid = fork() ret = waitpid(pid) ret = wait(); */

#if defined(MEMHOLD_SLOW)
    // Debugging: ~
    //   + MEMHOLD_SLOW = 0 --> fast code
    //   + MEMHOLD_SLOW = 1 --> slow code
    // See in "./Makefile": ~
    //   + DFLAGS = -DMEMHOLD_SLOW=0
    #define MEMHOLD_SLOW = 0
#endif

#if defined(MEMHOLD_YAGNI)
    // For when we want to enjoy dabbling with xy problems,
    // code golfing, busy work, and procrastination.
    #define MEMHOLD_YAGNI = 0
#endif

// clang-format off
#define COLOR_INFO    CLITERAL(Color) { 102, 191, 255, 255 }   // Sky Blue
#define COLOR_WARN    CLITERAL(Color) { 255, 161, 0, 255 }     // Orange
#define COLOR_ERROR   CLITERAL(Color) { 230, 41, 55, 255 }     // Red
#define COLOR_SUCCESS CLITERAL(Color) { 0, 228, 48, 255 }      // Green
// clang-format on

// memhold.c:174:30: error: format specifies type 'char *' but the argument has
// type '__pid_t' (aka 'int') [-Werror,-Wformat]
//
typedef __pid_t MH_PID_TYPE;

typedef struct Memhold
{
    bool flagLog;
    bool flagVerbose;

    const char *apiID;
    char       *apiVersion;

    float refreshSeconds;

    float  cpuThreshold;
    size_t memThreshold;

    __pid_t userProcessPID;
    __pid_t memholdMainProcessPID;

} Memhold;

MHAPI Memhold InitMemhold(void);
MHAPI Memhold InitMemhold(void)
{
    Memhold result = {};

    result = (Memhold){
        .flagLog     = true, // TODO(Lloyd): Override via CLI args
        .flagVerbose = true, // TODO(Lloyd): Override via CLI args

        .apiID      = "memhold",
        .apiVersion = MEMHOLD_VERSION,

        .refreshSeconds = 2.0f,

        .cpuThreshold = 50.0f,
        .memThreshold = 500000,

        .userProcessPID        = 0,
        .memholdMainProcessPID = 0,
    };

    return result;
}

MHAPI double GetCpuUsage(MH_PID_TYPE pid);
MHAPI long   GetMemUsage(MH_PID_TYPE pid);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        exit(1);
    }

    // Declare main function variables
    //-------------------------------------------------------------------------
    Memhold     memhold = {};
    MH_PID_TYPE procPID;
    // NOTE(Lloyd): Set this later to `memhold.flagVerbose`
    bool tmpArgVerbose = true;
    //------------------------------------------------------------------------

    // Write stdout program name and version
    memhold.apiVersion = MEMHOLD_VERSION;
    fprintf(stdout, "memhold %s\n", memhold.apiVersion);

    if (tmpArgVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 1: Initialize program >>>\n\n");

    // Parse args and ensure a valid process PID is passed.
    //-------------------------------------------------------------------------
    {                                       // $ pgrep waybar | xargs -I _ ./memhold _
        char *pid = argv[1];                // argv[1] is stdout of `$ pgrep lua`
        procPID   = (MH_PID_TYPE)atoi(pid); // This will panic either way.

        if (!(procPID >= 0)) // If not a number|integer...
        {
            fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
            fprintf(stderr, "expected valid PID. For example: 105815\n. got: %i", procPID);
            exit(1);
        }
    }
    //-------------------------------------------------------------------------

    // Initialize module
    //-------------------------------------------------------------------------
    memhold                       = InitMemhold();
    memhold.userProcessPID        = procPID;
    memhold.memholdMainProcessPID = getpid();
    //-------------------------------------------------------------------------

    // Log module information to stdout
    //-------------------------------------------------------------------------
    if (memhold.flagVerbose) fprintf(stdout, "[  OK  ]  <PID> %d\n", memhold.userProcessPID);
    if (memhold.flagLog)
    {
        { // Log user stats
            fprintf(stdout, "[ INFO ]  [ user ]\n");
            fprintf(stdout, "[ INFO ]  PID: %d\n", memhold.userProcessPID);

            // Opts: constants like
            fprintf(stdout, "[ INFO ]  Threshold CPU: %f\n", memhold.cpuThreshold);
            fprintf(stdout, "[ INFO ]  Threshold MEM: %zu\n", memhold.memThreshold);

            // Opts: loop stats
            fprintf(stdout, "[ INFO ]  Refresh: %.2fs (%s)\n", memhold.refreshSeconds, memhold.apiID);
        }

        { // Log memhold stats
            fprintf(stdout, "[ INFO ]  [ %s ]\n", memhold.apiID);
            fprintf(stdout, "[ INFO ]  PID: %d\n", memhold.memholdMainProcessPID);

            // Memhold: stats
            fprintf(stdout, "[ INFO ]  Version: %d.%d.%d\n", MEMHOLD_VERSION_MAJOR, MEMHOLD_VERSION_MINOR, MEMHOLD_VERSION_PATCH);
        }
    }
    //-------------------------------------------------------------------------

#if MEMHOLD_YAGNI
    __pid_t id = fork(); // Fork fun!!

    if (id == 0) printf("child process id = %d\n", id);
    else printf("not child process id = %d\n", id);
#endif /* if MEMHOLD_YAGNI */

    // Start main loop
    //-------------------------------------------------------------------------
    int loopCounter = 0, maxLoopCount = 4;

#if MEMHOLD_SLOW
    int sleepResultThisFrame = -1;
#endif

    if (memhold.flagVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 2: Monitor processes >>>\n\n");

    size_t cpuUsage[64];
    size_t memUsage[64];
    int    cpuUsageCounter   = 0;
    int    memUsageCounter   = 0;
    size_t cpuUsageThisFrame = 0;
    size_t memUsageThisFrame = 0;
    char   cmdCPU[256];
    char   cmdMEM[256];

    // Prepare command statements
    snprintf(cmdCPU, sizeof(cmdCPU), "ps -p %d -o %%cpu --no-headers", memhold.userProcessPID);
    snprintf(cmdMEM, sizeof(cmdMEM), "ps -p %d -o rss --no-headers", memhold.userProcessPID);

#if MEMHOLD_YAGNI
    if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  %s[cpu]: Preparing command: $ %s\n", memhold.apiID, cmdCPU);
    if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  %s[mem]: Preparing command: $ %s\n", memhold.apiID, cmdMEM);
#endif /* if MEMHOLD_YAGNI */

    //
    //
    //
    //
    //
    // Run main loop
    while (1)
    {
        { // Get CPU Usage.
#if MEMHOLD_YAGNI
            cpuUsageThisFrame         = 1.0f + loopCounter; // TEMPORARY PSEUDOCODE!!!!
            cpuUsage[cpuUsageCounter] = cpuUsageThisFrame;
            cpuUsageCounter += 1;
            cpuUsageCounter %= 64; // Avoid overflowing buffer!
#endif                             /* if MEMHOLD_YAGNI */

            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  cpu: %zu\n", cpuUsageThisFrame);
        }

        { // Get Memory Usage.
#if MEMHOLD_YAGNI
            memUsageThisFrame         = 1.0f + loopCounter; // TEMPORARY PSEUDOCODE!!!!
            memUsage[memUsageCounter] = memUsageThisFrame;
            memUsageCounter += 1;
            memUsageCounter %= 64; // Avoid overflowing buffer!
#endif                             /* if MEMHOLD_YAGNI */
            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  mem: %zu\n", memUsageThisFrame);
#if 0
            FILE *fp = popen(command, "r");
            if (!fp) { perror("popen"); exit(1); }
#endif /* if 0 */
        }

        { // Sleep/Pause this frame.
#if MEMHOLD_SLOW
            sleepResultThisFrame = sleep(memhold.refreshSeconds);

            assert(sleepResultThisFrame == 0 && "failed to assert 0 code from sleep signal result");
#else
            sleep(memhold.refreshSeconds); // Default: 2s per frame
#endif /* if MEMHOLD_SLOW */
        }

#if 1 || MEMHOLD_SLOW
        // TODO(Lloyd): Remove the overide `1` after prototyping - 20240703114505UTC
        loopCounter += 1;

        if (loopCounter >= maxLoopCount)
        {
            fprintf(stdout, "[ WARN ]  *break* main loop on iteration: %d\n", loopCounter);
            break;
        };
#endif /* if 1 || MEMHOLD_SLOW */

    } // end while (1)
      //
      //
      //
      //
      //
    //-------------------------------------------------------------------------

    // Unload program
    //-------------------------------------------------------------------------
    if (memhold.flagVerbose)
    {
        fprintf(stdout, "\n[ INFO ]  <<< Stage 3: Cleanup and Exit >>>\n\n");
        fprintf(stdout, "[ INFO ]  took %.2fs\n", loopCounter * memhold.refreshSeconds);
    }

    // TODO(Lloyd): Unload more data or free memory here...
    // (e.g. ML_FREE(...))
    // ...
    // ...
    //-------------------------------------------------------------------------

    return 0; // EXIT_SUCCESS
}

// BOT

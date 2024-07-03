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
#include <unistd.h>

#if defined(MEMHOLD_SLOW)
    // Debugging: ~
    //   + MEMHOLD_SLOW = 0 --> fast code
    //   + MEMHOLD_SLOW = 1 --> slow code
    //
    // See in "./Makefile": ~
    //   + DFLAGS = -DMEMHOLD_SLOW=0
    #define MEMHOLD_SLOW = 0
#endif

// clang-format off
#define COLOR_INFO    CLITERAL(Color) { 102, 191, 255, 255 }   // Sky Blue
#define COLOR_WARN    CLITERAL(Color) { 255, 161, 0, 255 }     // Orange
#define COLOR_ERROR   CLITERAL(Color) { 230, 41, 55, 255 }     // Red
#define COLOR_SUCCESS CLITERAL(Color) { 0, 228, 48, 255 }      // Green
// clang-format on

typedef int MH_PID_TYPE;

typedef struct Memhold
{
    char       *apiVersion;
    const char *apiID;

    float  cpuThreshold;
    size_t memThreshold;

    MH_PID_TYPE currentPID;

    float refreshSeconds;

    bool flagVerbose;
    bool flagLog;
} Memhold;

MHAPI Memhold InitMemhold(void);
MHAPI Memhold InitMemhold(void)
{
    Memhold result = {};

    result = (Memhold){
        .apiID      = "memhold",
        .apiVersion = MEMHOLD_VERSION,

        .cpuThreshold = 50.0f,
        .memThreshold = 500000,

        .refreshSeconds = 2.0f,

        // .currentPID = -1;

        .flagVerbose = true, // TODO(Lloyd): Use cli args
        .flagLog     = true, // TODO(Lloyd): Use cli args
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

    bool tmpArgVerbose = true; // NOTE(Lloyd): Set this later to `memhold.flagVerbose`
    //-------------------------------------------------------------------------

    if (tmpArgVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 1: Initialize program >>>\n\n");

    // Parse args and ensure a valid process PID is passed.
    //-------------------------------------------------------------------------
    {
        // $ pgrep waybar | xargs -I _ ./memhold _
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
    memhold = InitMemhold();
    {
        memhold.currentPID = procPID;
    }
    //-------------------------------------------------------------------------

    // Log module information to stdout
    //-------------------------------------------------------------------------

    fprintf(stdout, "memhold %s\n", memhold.apiVersion);

    if (memhold.flagVerbose) fprintf(stdout, "[  OK  ]  <PID> %d\n", memhold.currentPID);

    if (memhold.flagLog)
    {
        // Opts: user initiated or process
        fprintf(stdout, "[ INFO ]  PID: %d\n", memhold.currentPID);

        // Opts: constants like
        fprintf(stdout, "[ INFO ]  Threshold CPU: %f\n", memhold.cpuThreshold);
        fprintf(stdout, "[ INFO ]  Threshold MEM: %zu\n", memhold.memThreshold);

        // Opts: loop stats
        fprintf(stdout, "[ INFO ]  Refresh: %.2fs (%s)\n", memhold.refreshSeconds, memhold.apiID);

        // Misc
        fprintf(stdout, "[ INFO ]  Version: %d.%d.%d (%s)\n", MEMHOLD_VERSION_MAJOR, MEMHOLD_VERSION_MINOR, MEMHOLD_VERSION_PATCH, memhold.apiID);
    }
    //-------------------------------------------------------------------------

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
    {
        snprintf(cmdCPU, sizeof(cmdCPU), "ps -p %d -o %%cpu --no-headers", memhold.currentPID);
        if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  command = %s\n", cmdCPU);
    }
    char cmdMEM[256];
    {
        snprintf(cmdMEM, sizeof(cmdMEM), "ps -p %d -o rss --no-headers", memhold.currentPID);
        if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  command = %s\n", cmdMEM);
    }

    while (1)
    {
        // Get CPU Usage
        //-------------------------------------------------------------------------
        {
            { // TEMPORARY PSEUDOCODE!!!!
                cpuUsageThisFrame         = 1.0f + loopCounter;
                cpuUsage[cpuUsageCounter] = cpuUsageThisFrame;
                cpuUsageCounter += 1;
            }

            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  cpu: %zu\n", cpuUsageThisFrame);
        }
        //-------------------------------------------------------------------------

        // Get Memory Usage
        //-------------------------------------------------------------------------
        {
            { // TEMPORARY PSEUDOCODE!!!!
                memUsageThisFrame         = 1.0f + loopCounter;
                memUsage[memUsageCounter] = memUsageThisFrame;
                memUsageCounter += 1;
            }

            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  mem: %zu\n", memUsageThisFrame);
#if 0
            {
                FILE *fp = popen(command, "r");
                if (!fp)
                {
                    perror("popen");
                    exit(1);
                }
            }
#endif
        }
        //-------------------------------------------------------------------------

#if MEMHOLD_SLOW
        sleepResultThisFrame = sleep(memhold.refreshSeconds);
        assert(sleepResultThisFrame == 0 && "failed to assert 0 code from sleep signal result");
#else
        sleep(memhold.refreshSeconds); // Default: 2s per frame
#endif

#if 1 || MEMHOLD_SLOW
        loopCounter += 1;

        if (loopCounter >= maxLoopCount)
        {
            fprintf(stdout, "[ WARN ]  *break* main loop on iteration: %d\n", loopCounter);
            break;
        };
#endif
    } // end while(1)
    //-------------------------------------------------------------------------

    // Unload program
    //-------------------------------------------------------------------------
    // TODO(Lloyd): free memory here... (e.g. ML_FREE(...))
    // ...
    if (memhold.flagVerbose)
    {
        fprintf(stdout, "\n[ INFO ]  <<< Stage 3: Cleanup and Exit >>>\n\n");
        fprintf(stdout, "[  OK  ]  took %.2fs\n", loopCounter * memhold.refreshSeconds);
    }
    //-------------------------------------------------------------------------

    return 0;
}

// BOT

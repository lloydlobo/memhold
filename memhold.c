// file: main.c

// TOP

#include "memhold.h" // Declares module functions

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

    bool fverbose;
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

        // .currentPID = -1;

        // TODO(Lloyd): Use cli args
        .fverbose = false,
    };

    return result;
}

MHAPI double GetCpuUsage(MH_PID_TYPE pid);
MHAPI long   GetMemUsage(MH_PID_TYPE pid);

int main(int argc, char *argv[])
{
    MH_PID_TYPE procPID;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        exit(1);
    }

    {
        // $ pgrep 'lua' | xargs -I _ ./memhold _
        // argv[1] --> $ pgrep 'lua'
        char *pid = argv[1];
        procPID   = (MH_PID_TYPE)atoi(pid); // This will panic either way.

        if (!(procPID >= 0)) // If not a number|integer...
        {
            fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
            fprintf(stderr, "expected valid PID. For example: 105815\n. got: %i", procPID);
            exit(1);
        }
        else
            fprintf(stdout, "[  OK  ]  <PID> %d\n", procPID);
    }

    // Init
    //-------------------------------------------------------------------------
    Memhold memhold    = InitMemhold();
    memhold.currentPID = procPID;
    //-------------------------------------------------------------------------

    // Log to stdout
    //-------------------------------------------------------------------------
    {
#if MEMHOLD_SLOW
        printf("memhold.apiID = %s\n", memhold.apiID);
        printf("memhold.apiVersion = %s\n", memhold.apiVersion);
#endif
        printf("memhold.cpuThreshold = %f\n", memhold.cpuThreshold);
        printf("memhold.memThreshold = %zu\n", memhold.memThreshold);
        printf("memhold.currentPID = %d\n", memhold.currentPID);
    }
    //-------------------------------------------------------------------------

    // Start main loop
    //-------------------------------------------------------------------------
    int maxLoopCount = 4;
    int loopCounter  = 0;
    while (1)
    {
        loopCounter += 1;
        if (loopCounter >= maxLoopCount)
        {
            fprintf(stdout, "[  OK  ]  main loop break %d\n", __LINE__);
            break;
        };

        sleep(5);
    }
    //-------------------------------------------------------------------------

    return 0;
}

// BOT


/*file: main.c*************************************************************************************
 *
 *
 *  memhold 0.1
 *
 *
 *  20240704144247UTC
 *      ==48754== Command: ./memhold 2007
 *      [ INFO ]  took 8.00s
 *      ==48754== HEAP SUMMARY:
 *      ==48754==     in use at exit: 0 bytes in 0 blocks
 *      ==48754==   total heap usage: 9 allocs, 9 frees, 7,008 bytes allocated
 *      ==48754== All heap blocks were freed -- no leaks are possible
 *      ==48754== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
 *
 *      ==46954== Command: ./memhold 2007
 *      [ INFO ]  took 8.00s
 *      ==46954== I refs:        285,880
 *
 *
 *************************************************************************************************/

// TOP

#include "memhold.h" // Declares module functions

#include <assert.h> // Required for: assert()
#include <stdio.h>  // Required for: printf(), fprintf(), sprintf(), stderr, stdout
#include <stdlib.h> // Required for: atoi(), exit()
#include <string.h> // Required for: strcmp(), NULL
#include <sys/wait.h>
#include <unistd.h> // Required for: fork(), getpid(), sleep(),... [UNIX only lib]

//-----------------------------------------------------------------------------
// Debug Flags (set in build step)
//-----------------------------------------------------------------------------
// Debugging: ~ + MEMHOLD_SLOW = 0 --> fast code + MEMHOLD_SLOW = 1 --> slow code: See in "./Makefile": ~ + DFLAGS = -DMEMHOLD_SLOW=0
#if defined(MEMHOLD_SLOW)
    #define MEMHOLD_SLOW = 0
#endif

// For when we want to enjoy dabbling with xy problems, code golfing, busy work, and procrastination.
#if defined(MEMHOLD_YAGNI)
    #define MEMHOLD_YAGNI = 0
#endif

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
// clang-format off
#define COLOR_INFO    CLITERAL(Color) { 102, 191, 255, 255 }   // Sky Blue
#define COLOR_WARN    CLITERAL(Color) { 255, 161, 0, 255 }     // Orange
#define COLOR_ERROR   CLITERAL(Color) { 230, 41, 55, 255 }     // Red
#define COLOR_SUCCESS CLITERAL(Color) { 0, 228, 48, 255 }      // Green
// clang-format on

//-----------------------------------------------------------------------------
// Some constants
//-----------------------------------------------------------------------------
static const int MAX_MAIN_LOOP_COUNT = 4;

//-----------------------------------------------------------------------------
// DATA STRUCTURESSSSS
//-----------------------------------------------------------------------------

// memhold.c:174:30: error: format specifies type 'char *' but the argument has
// type '__pid_t' (aka 'int') [-Werror,-Wformat]
// typedef __pid_t MH_PID_TYPE;
typedef pid_t MH_PID_TYPE;

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

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

Memhold     memhold = {0};
MH_PID_TYPE procPID;
bool        tmpArgVerbose = true; // NOTE(Lloyd): Set this later to `memhold.flagVerbose`

//-----------------------------------------------------------------------------
// FUNCTIONSSSS
//-----------------------------------------------------------------------------

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
        .memThreshold = MH_MEMORY_THRESHOLD, //>10240kb Max: 500000kb

        .userProcessPID        = 0,
        .memholdMainProcessPID = 0,
    };

    return result;
}

//<<<<<<<<<<<<<<<<<<TODO>>>>>>>>>>>>>>>>>
MHAPI double GetCpuUsage(MH_PID_TYPE pid);

MHAPI double GetCpuUsage(MH_PID_TYPE pid)
{
    fprintf(stderr, "[  ERR  ]  unimplemented\n");
    exit(1);
};

//<<<<<<<<<<<<<<<<<<TODO>>>>>>>>>>>>>>>>>
MHAPI long GetMemUsage(MH_PID_TYPE pid);

MHAPI long GetMemUsage(MH_PID_TYPE pid)
{
    long status = -1;

    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");

    if (!file)
    {
        perror("[ ERR! ]  failed to open status file");
        status = -1;
        goto cleanupError;
    }

    char line[356];
    long memoryUsage = -1;

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line + 6, "%ld", &memoryUsage);
            break;
        }
    }
    status = 0; // Success

    // Cleanup
    status = fclose(file);

    if (status != 0)
    {
        perror("[ ERR! ]  failed to close status file");
        goto ioError;
    }

    return memoryUsage;

cleanupError:
    status = fclose(file);

    if (status != 0) perror("[ ERR! ]  failed to close status file");

ioError:
    return status;
};

//-----------------------------------------------------------------------------
// IT'S SHOWTIME                                                       ^_^
//-----------------------------------------------------------------------------

int RunMain()
{
    int success = 0; // EXIT_SUCCESS

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

    // Prepare main loop
    //-------------------------------------------------------------------------
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
    //-------------------------------------------------------------------------

    // Run main loop
    //-------------------------------------------------------------------------
    int loopCounter = 0;

    while (1)
    {
#if 1 /* <<<<<<<<<<< Remove this after prototyping >>>>>>>>>> */
        if (loopCounter >= MAX_MAIN_LOOP_COUNT)
        {
            fprintf(stdout, "[ WARN ]  *break* main loop on iteration: %d\n", loopCounter);
            break;
        };

        loopCounter += 1;
#endif

        { // Get CPU Usage.
            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  cpu: %zu\n", cpuUsageThisFrame);
        }

        { // Get Memory Usage.
            memUsageThisFrame = GetMemUsage(memhold.userProcessPID);

            if (memhold.flagVerbose) fprintf(stdout, "[ INFO ]  mem: %zu\n", memUsageThisFrame);
        }

        // Pause this frame (2s per frame by default.)
        sleep(memhold.refreshSeconds);
    }
    // end while (1)
    //-------------------------------------------------------------------------

    // Unload program
    //-------------------------------------------------------------------------
    // NOTE(Lloyd): Unload more data or free memory here... (e.g. ML_FREE(...))
    // ...

    if (memhold.flagVerbose)
    {
        fprintf(stdout, "\n[ INFO ]  <<< Stage 3: Cleanup and Exit >>>\n\n");
        fprintf(stdout, "[ INFO ]  took %.2fs\n", loopCounter * memhold.refreshSeconds);
    }
    //-------------------------------------------------------------------------

    return success;
};

// Main entry point of the program.
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        exit(1);
    }

    { // Write stdout program name and version
        memhold.apiVersion = MEMHOLD_VERSION;
        fprintf(stdout, "memhold %s\n", memhold.apiVersion);
    }

    // Parse args and ensure a valid process PID is passed.
    //-------------------------------------------------------------------------
    if (tmpArgVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 1: Initialize program >>>\n\n");

    char *pid = argv[1];                // argv[1] is stdout of `$ pgrep lua`
    procPID   = (MH_PID_TYPE)atoi(pid); // This will panic either way.
                                        //
    if (!(procPID >= 0))                // If is invalid (not a number or integer.)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        fprintf(stderr, "expected valid PID. For example: 105815\n. got: %i", procPID);
        exit(1);
    }
    //-------------------------------------------------------------------------

    // Begin memhold hot loop.
    int success = RunMain(); // 0 is success.
    return success;          // EXIT_SUCCESS
}

// BOT

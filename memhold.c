
/*file: main.c*************************************************************************************
 *
 *
 *  memhold 0.1
 *
 *
 *
 *           ✦ ❯ ./memhold $(pgrep clang)
 *           memhold 0.1
 *
 *           [ INFO ]  <<< Stage 1: Initialize program >>>
 *
 *           [  OK  ]  <PID> 77100
 *           [ INFO ]  [ user ]
 *           [ INFO ]  PID: 77100
 *           [ INFO ]  Threshold CPU: 50.000000
 *           [ INFO ]  Threshold MEM: 10240
 *           [ INFO ]  Refresh: 2.00s (memhold)
 *           [ INFO ]  [ memhold ]
 *           [ INFO ]  PID: 77183
 *           [ INFO ]  Version: 0.1.0
 *
 *           [ INFO ]  <<< Stage 2: Monitor processes >>>
 *
 *           [  OK  ] [0]: lloyd      77100  1.3  0.8 435860 34876 ?        Ssl  11:46   0:00 /home/lloyd/.local/share/nvim/mason/bin/clangd
 *           [  OK  ] [1]: lloyd      77183  0.0  0.0   3400  1932 pts/6    S+   11:46   0:00 ./memhold 77100
 *           [  OK  ] [2]: lloyd      77184  0.0  0.0 223700  3548 pts/6    S+   11:46   0:00 sh -c -- ps aux | grep 77100
 *           [  OK  ] [3]: lloyd      77186  0.0  0.0 222724  2696 pts/6    S+   11:46   0:00 grep 77100
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       1302
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       1532
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       1773
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       1985
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       2185
 *           *file = 0x5607e7b0c8d0
 *           [ INFO ]  PID: 77100  MEM: 34876K       2435
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        2591
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        2760
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        2926
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        3003
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        3179
 *           *file = (nil)
 *           [ ERR! ]  failed to open status file. file: (nil)
 *           [ INFO ]  PID: 77100  MEM: 18446744073709551615K        3351
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
#include <signal.h>
#include <stdio.h>  // Required for: printf(), fprintf(), sprintf(), stderr, stdout, popen() [with compiler option `-pthread`]
#include <stdlib.h> // Required for: atoi(), exit()
#include <string.h> // Required for: strcmp(), NULL
#include <sys/wait.h>
#include <time.h>   // Required for: clock(), [time() ~ not used]
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

// 2s refresh cycle per loop
static const int MAX_HOT_LOOP_COUNT = (1 << 8); //> `256 (0x100)` (1 << 8)

// Limit fopen for file at path: `char path[256]; snprintf(path, sizeof(path), "/proc/%d/status", pid);`
static const int MAX_RETRIES_FILE_NOT_FOUND = (1 << 3); //> `8 (0x100)` (1 << 3)

//-----------------------------------------------------------------------------
// DATA STRUCTURESSSSS
//-----------------------------------------------------------------------------

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
// TODO: All declared global variables, must be initialized. Even if it is 0 again.

Memhold memhold = {0};

bool       tmpArgVerbose    = false;
static int cntrFopenRetries = 0;

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

int Init(void);

int Init(void)
{
    int status = -1; // SUCCESS

    cntrFopenRetries = 0;
    tmpArgVerbose    = true; // NOTE(Lloyd): Set this later to `memhold.flagVerbose`

    memhold = InitMemhold();
    {
        memhold.memholdMainProcessPID = getpid();
    }
    status = 0;

    return status;
};

//<<<<<<<<<<<<<<<<<<TODO>>>>>>>>>>>>>>>>>
MHAPI double GetCpuUsage(MH_PID_TYPE pid);

MHAPI double GetCpuUsage(MH_PID_TYPE pid)
{
    fprintf(stderr, "[  ERR  ]  unimplemented\n");
    exit(1);
};

MHAPI long GetMemUsage(MH_PID_TYPE pid);

MHAPI long GetMemUsage(MH_PID_TYPE pid)
{
    long status = -1;

    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r"); //> stream or NULL
    /* EXIT STATUS
           file  will  exit  with 0 if the operation was successful or >0 if an error was encountered.  The
           following errors cause diagnostic messages, but don't affect the program exit code (as POSIX re‐
           quires), unless -E is specified:
                 •   A file cannot be found
                 •   There is no permission to read a file
                 •   The file type cannot be determined
    */
    // printf("*file = %p\n", file);

    if (!file)
    {
        // [ ERR! ]  failed to open status file. file: (nil)
        // zsh: segmentation fault (core dumped)  ./memhold $(pgrep clang)

        fprintf(stderr, "[ ERR! ]  failed to open status file. file: %p\n", file);
        // perror("[ ERR! ]  failed to open status file");
        status = -1;
        goto ioError;
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

    // NOTE(Lloyd): @label NOT USED YET
#if 0

cleanupError:
    status = fclose(file);

    if (status != 0) perror("[ ERR! ]  failed to close status file");
#endif /* if 0 */

ioError:
    return status;
};

//-----------------------------------------------------------------------------
// IT'S SHOWTIME                                                       ^_^
//-----------------------------------------------------------------------------

int RunMain()
{
    int status = 0; // EXIT_SUCCESS

    // Log module information to stdout
    //-------------------------------------------------------------------------
    if (memhold.flagVerbose) fprintf(stdout, "[  OK  ]  <PID> %d\n", memhold.userProcessPID);

    if (memhold.flagLog)
    {
        // Log user stats
        fprintf(stdout, "[ INFO ]  [ user ]\n");
        fprintf(stdout, "[ INFO ]  PID: %d\n", memhold.userProcessPID);
        // Opts: constants like
        fprintf(stdout, "[ INFO ]  Threshold CPU: %f\n", memhold.cpuThreshold);
        fprintf(stdout, "[ INFO ]  Threshold MEM: %zu\n", memhold.memThreshold);
        // Opts: loop stats
        fprintf(stdout, "[ INFO ]  Refresh: %.2fs (%s)\n", memhold.refreshSeconds, memhold.apiID);

        // Log memhold stats
        fprintf(stdout, "[ INFO ]  [ %s ]\n", memhold.apiID);
        fprintf(stdout, "[ INFO ]  PID: %d\n", memhold.memholdMainProcessPID);
        // Memhold: stats
        fprintf(stdout, "[ INFO ]  Version: %d.%d.%d\n", MEMHOLD_VERSION_MAJOR, MEMHOLD_VERSION_MINOR, MEMHOLD_VERSION_PATCH);
    }
    //-------------------------------------------------------------------------

    // Prepare main loop
    //-------------------------------------------------------------------------
    if (memhold.flagVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 2: Monitor processes >>>\n\n");

    char cmdGetProcName[256];

    char   cmdCPU[256];
    char   cmdMEM[256];
    size_t cpuUsage[64];
    size_t memUsage[64];

    int    cpuUsageCounter   = 0;
    int    memUsageCounter   = 0;
    size_t cpuUsageThisFrame = 0;
    size_t memUsageThisFrame = 0;

    // Prepare command statements
    {
        int stackAllocCmd = (sizeof(cmdCPU) + sizeof(cmdMEM) + sizeof(cmdGetProcName)); //> 768
        int bytesSoFar    = 0;
        bytesSoFar += snprintf(cmdCPU, sizeof(cmdCPU), "ps -p %d -o %%cpu --no-headers", memhold.userProcessPID);
        bytesSoFar += snprintf(cmdMEM, sizeof(cmdMEM), "ps -p %d -o rss --no-headers", memhold.userProcessPID);
        bytesSoFar += snprintf(cmdGetProcName, sizeof(cmdGetProcName), "ps aux | grep %d", memhold.userProcessPID);
        assert(bytesSoFar >= 64 && bytesSoFar <= stackAllocCmd); //> 79 >= 64

        //
        // TEMP LOG to stdout Process Name
        //

#if 0

        {
            int ret = system(cmdGetProcName);

            if (ret != 0)
            {
                char msg[256]; // *note:* 'system' declared in stdlib.h
                snprintf(msg, sizeof(msg), "[ ERR! ]  failed to execute command. system call returned: %d", ret);
                perror(msg);
                exit(1);
            };
        }

#else

        {
            const int CMD_MAX = 1035;
            int       pstatus;
            FILE     *pipefp;
            char      cmd[CMD_MAX];

            pipefp = popen(cmdGetProcName, "r");

            if (pipefp == NULL)
            {
                perror("[ ERR! ]  failed to execute popen for getting process name via its PID");
                exit(1);
            }

            int cntr = 0;

            /* 
            clang-format off
                [  OK  ] [0]: user       2007  0.0  2.4 808912 96252 ?        Ssl  Jul04   0:06 /nix/store/r813aa9qb8rh3jhq6089hfcgqqj3zps9-rsibreak-0.12.13/bin/rsibreak
                [  OK  ] [1]: user     113341  0.0  0.0   3416  1856 pts/0    S    08:57   0:00 ./memhold 2007
                [  OK  ] [2]: user     113342  0.0  0.0 223700  3480 pts/0    S    08:57   0:00 sh -c -- ps aux | grep 2007
                [  OK  ] [3]: user     113344  0.0  0.0 222724  2572 pts/0    S    08:57   0:00 grep 2007
                pstatus = 0
            clang-format on
            */
            while (fgets(cmd, CMD_MAX, pipefp) != NULL)
            {
                fprintf(stdout, "[  OK  ] [%d]: %s", cntr, cmd);
                cntr++;
            }

            pstatus = pclose(pipefp);

            if (pstatus == -1)
            {
                // todo
                perror("[ ERR! ] pclose");
            }
            else
            {

    #if MEMHOLD_SLOW

                // TODO: use macros (From examples in popen() are marvelous articles?)

                fprintf(stdout, "[ INFO ] pclose returned status for: %s\n", cmdGetProcName);

    #endif /* if MEMHOLD_SLOW */
            }
        }

#endif /* if 0 */
    }
    //-------------------------------------------------------------------------

    // Run main loop
    //-------------------------------------------------------------------------
    int loopCounter = 0;

    while (1)
    {
#if 1 /* <<<<<<<<<<< Remove this after prototyping >>>>>>>>>> */

        if (loopCounter >= MAX_HOT_LOOP_COUNT)
        {
            fprintf(stdout, "[ WARN ]  *break* main loop on iteration: %d\n", loopCounter);
            break;
        };

        loopCounter += 1;

#endif

        // Log process usage.
        //---------------------------------------------------------------------
        {
            memUsageThisFrame = GetMemUsage(memhold.userProcessPID);

            if (memhold.flagVerbose)
            { // Similar to procs, top, htop, btop
                fprintf(stdout, "[ INFO ]  PID: %d  MEM: %zuK  \t%ld\n", memhold.userProcessPID, memUsageThisFrame, clock());
            }
        }
        //---------------------------------------------------------------------

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

    return status;
};

// Main entry point of the program.
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        exit(1);
    }

    // Declare main functions scoped variables
    int         status;
    MH_PID_TYPE procPID;

    { // Write stdout program name and version
        memhold.apiVersion = MEMHOLD_VERSION;
        fprintf(stdout, "memhold %s\n", memhold.apiVersion);
    }

    { // Parse args and ensure a valid process PID is passed.
        if (tmpArgVerbose) fprintf(stdout, "\n[ INFO ]  <<< Stage 1: Initialize program >>>\n\n");

        char *pid = argv[1];                // argv[1] is stdout of `$ pgrep lua`
        procPID   = (MH_PID_TYPE)atoi(pid); // This will panic either way.
                                            //
        if (!(procPID >= 0))                // If is invalid (not a number or integer.)
        {
            fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
            fprintf(stderr, "expected valid PID. For example: 105815\n. got: %i", procPID);
            status = 1;
            goto cleanupError;
        }
    }

    { // Initialize module
        status = Init();

        if (status != 0)
        {
            fprintf(stderr, "[ ERR! ]  failed to initialize module\n");
            goto cleanupError;
        }

        memhold.userProcessPID = procPID;
    }

    // Begin memhold hot loop.
    status = RunMain();

cleanupError:

    return status; // EXIT_SUCCESS
}

// BOT

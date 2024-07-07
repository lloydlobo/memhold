
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

typedef struct Memhold
{
    bool flagLog;
    bool flagVerbose;

    const char *apiID;
    char       *apiVersion;

    float refreshSeconds;

    float  cpuThreshold;
    size_t memThreshold;

    pid_t userProcessPID;
    pid_t memholdMainProcessPID;

} Memhold;

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
// TODO: All declared global variables, must be initialized. Even if it is 0 again.

Memhold memhold = {0};

bool  gVerbose; //@Temp
pid_t gProcPID;

static int cntrFopenRetries = 0;

//-----------------------------------------------------------------------------
// FUNCTIONSSSS
//-----------------------------------------------------------------------------

MHAPI Memhold InitMemhold(void);

MHAPI Memhold InitMemhold(void)
{
    Memhold result = {};

    result = (Memhold){
        .flagLog     = true,     // TODO(Lloyd): Override via CLI args
        .flagVerbose = gVerbose, // TODO(Lloyd): Override via CLI args

        .apiID      = MEMHOLD_ID,
        .apiVersion = MEMHOLD_VERSION,

        .refreshSeconds = 2.0f,

        .cpuThreshold = 50.0f,
        .memThreshold = MH_MEMORY_THRESHOLD, //>10240kb Max: 500000kb

        .userProcessPID        = gProcPID,
        .memholdMainProcessPID = 0,
    };

    return result;
}

int Init(void);

int Init(void)
{
    int status = -1; // SUCCESS

    cntrFopenRetries = 0;
    memhold          = InitMemhold();
    {
        memhold.memholdMainProcessPID = getpid();
    }
    status = 0;

    return status;
};

/* 1.14. /proc

Linux Filesystem Hierarchy:
Chapter 1. Linux Filesystem Hierarchy
See https://tldp.org/LDP/Linux-Filesystem-Hierarchy/html/proc.html

The purpose and contents of each of these files is explained below:

/proc/PID/cmdline   Command line arguments.
/proc/PID/cpu       Current and last cpu in which it was executed.
/proc/PID/cwd       Link to the current working directory.
/proc/PID/environ   Values of environment variables.
/proc/PID/exe       Link to the executable of this process.
/proc/PID/fd        Directory, which contains all file descriptors.
/proc/PID/maps      Memory maps to executables and library files.
/proc/PID/mem       Memory held by this process.
/proc/PID/root      Link to the root directory of this process.
/proc/PID/stat      Process status.
/proc/PID/statm     Process memory status information.
/proc/PID/status    Process status in human readable form.

Should you wish to know more, the man page for proc describes each of the files
associated with a running process ID in far greater detail.
Even though files appear to be of size 0, examining their contents reveals
otherwise:
# cat status
*/

/*
 attr/
 cwd/
 fd/
 fdinfo/
 map_files/
 net/
 ns/
 root/
 task/
 arch_status
 autogroup
 auxv
 cgroup
 clear_refs
 cmdline
 comm
 coredump_filter
 cpuset
 environ
 exe@
 gid_map
 io
 ksm_merging_pages
 ksm_stat
 limits
 loginuid
 maps
 mem
 mountinfo
 mounts
 mountstats
 numa_maps
 oom_adj
 oom_score
v
1/54 H 2024-07-07 12:12 dr-xr-xr-x 0B
*/

//-----------------------------------------------------------------------------
// Module specific function declarations
//-----------------------------------------------------------------------------

int RunMain(void);

MHAPI double GetCpuUsage(pid_t pid);
MHAPI long   GetMemUsage(pid_t pid);

MHAPI void LogProcLimits(pid_t pid);
MHAPI void NoOp(void); // Placeholder function that does nothing.


#if MEMHOLD_YAGNI
MHAPI void PanicUnimplemented(void);
#endif /* if MEMHOLD_YAGNI */


//-----------------------------------------------------------------------------
// Module specific function implementations
//-----------------------------------------------------------------------------

MHAPI void NoOp(void) {}


// See also: ~
//   - snprintf(path, sizeof(path), "/proc/%d/status", pid);
//     /proc/[pid]/status:
//       While primarily used for memory information, it does contain some CPU-related fields:
//
//       Threads: Number of threads in the process
//       voluntary_ctxt_switches and nonvoluntary_ctxt_switches: Context switch counts
//
MHAPI void LogProcLimits(pid_t pid)
{
    long status = -1;

    // /proc
    // NOTE: The file doesn't actually contain any data; it just acts as a
    // pointer to where the actual process information resides.
    // See https://tldp.org/LDP/Linux-Filesystem-Hierarchy/html/proc.html
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/limits", pid);  // Choices: cpuset
                                                           //
    fprintf(stdout, "[ INFO ]  PID: %d  %s\n", pid, path); //> path = /proc/2014/cpuset

    FILE *file = fopen(path, "r"); //> stream or NULL

    if (!file)
    {
        fprintf(stderr, "[ ERR! ]  failed to open status file. file: %p\n", file);
        status = -1;
        goto ioError;
    }

    char line[356];
    int  lineCount = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (memhold.flagVerbose)
        {
            lineCount += 1;
            fprintf(stdout, "[ INFO ]  PID: %d  \t| %2d ~ %s", pid, lineCount, line);
        }
    }

    status = fclose(file);

    if ((status != 0))
    {
        perror("[ !ERR ]  failed to close status file");
        goto ioError;
    }

    return;

ioError:

    return;
}



// For processes using popen use: ~ "ps -p %d -o %%cpu --no-headers"
//
// /proc/[pid]/stat:
// This file contains more detailed CPU usage data. The relevant fields are:
//
// utime: User mode CPU time
// stime: Kernel mode CPU time
MHAPI double GetCpuUsage(pid_t pid)
{
    long status = -1;
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid); // Choices: cpuset


    FILE *file = fopen(path, "r"); //> stream or NULL

    if (!file)
    {
        fprintf(stderr, "[ ERR! ]  failed to open status file. file: %p\n", file);
        status = -1;
        goto ioError;
    }

    long cpuUsage      = -1; // Result
    long cpuUsageUtime = -1;
    long cpuUsageStime = -1;

    char line[356];
    int  lineCount = 0;

#if 0
   // Skip the first 13 fields
    for (int i = 0; i < 13; i++) {
        fscanf(file, "%*s");
    }

    fscanf(file, "%llu %llu", &utime, &stime);
    fclose(file);
#endif /* if 0 */

    // Split fields in output of /proc/<pid>/stat that are presented as a series
    // of numbers and values separated by spaces.
    //----------------------------------------------------------------------------------
    const int MAX_TOKENS_CONSUMED_COUNT = 30; // Prevent infinite loops in inner while loop

    while (fgets(line, sizeof(line), file))
    {
        char *token                = strtok(line, " "); // Divide S into tokens separated by characters in DELIM.
        int   tokenConsumedCounter = 1;

        while (token != NULL)
        {
            if (tokenConsumedCounter >= MAX_TOKENS_CONSUMED_COUNT) break;

            if (tokenConsumedCounter == MH_UTIME_FIELD_INDEX) cpuUsageUtime = strtol(token, NULL, 10);
            if (tokenConsumedCounter == MH_STIME_FIELD_INDEX) cpuUsageStime = strtol(token, NULL, 10);

// DEBUG
#if 0
            printf("tokens[%d] = %s\n", tokenConsumedCounter, token);
#endif /* if 0 */

            token = strtok(NULL, " ");

            tokenConsumedCounter += 1;
        }
    }

    if ((cpuUsageUtime != -1) && (cpuUsageStime != -1)) cpuUsage = (cpuUsageUtime + cpuUsageStime);
    //----------------------------------------------------------------------------------

    status = fclose(file); // Cleanup

    if ((status != 0))
    {
        perror("[ !ERR ]  failed to close status file");
        goto ioError;
    }

    return cpuUsage;

ioError:

    return status;
};


MHAPI long GetMemUsage(pid_t pid)
{
    long status = -1;

    char path[256];

    // For processes using popen use: ~ "ps -p %d -o rss --no-headers"
    snprintf(path, sizeof(path), "/proc/%d/status", pid); // Choices: status, mem

    FILE *file = fopen(path, "r"); //> stream or NULL

    if (!file) // [ ERR! ]  failed to open status file. file: (nil)
    {          // zsh: segmentation fault (core dumped)  ./memhold $(pgrep clang)
        fprintf(stderr, "[ ERR! ]  failed to open status file. file: %p\n", file);
        status = -1;
        goto ioError;
    }


    char line[356];
    long memoryUsage = -1;

    /*
    VmRSS stands for Virtual Memory Resident Set Size.

    VmRSS shows the amount of physical memory (RAM) that a process is
    currently using. This includes:

      - The process's code
      - Its data
      - Shared libraries that are currently loaded into RAM
    */
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line + 6, "%ld", &memoryUsage);
            status = 0; // Success
            break;
        }
    }

    status = fclose(file); // Cleanup

    if (status != 0)
    {
        perror("[ ERR! ]  failed to close status file");
        goto ioError;
    }

    return memoryUsage;

ioError:

    return status;
};

#if MEMHOLD_YAGNI
MHAPI void PanicUnimplemented(void) { UNIMPLEMENTED; }
#endif /* if MEMHOLD_YAGNI */

//-----------------------------------------------------------------------------
// IT'S SHOWTIME                                                       ^_^
//-----------------------------------------------------------------------------

int RunMain(void)
{
    int status = 0; // EXIT_SUCCESS

    // Log module information to stdout
    //----------------------------------------------------------------------------------
    if (memhold.flagVerbose)
    {
        fprintf(stdout, "[  OK  ]  <PID> %d\n", memhold.userProcessPID);
        LogProcLimits(memhold.userProcessPID);
    }

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
    //----------------------------------------------------------------------------------

    // Prepare main loop
    //----------------------------------------------------------------------------------
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

#if 0 && MEMHOLD_YAGNI
    //
    // Prepare command statements
    //

    int stackAllocCmd = (sizeof(cmdCPU) + sizeof(cmdMEM) + sizeof(cmdGetProcName)); //> 768
    int bytesSoFar    = 0;

    bytesSoFar += snprintf(cmdCPU, sizeof(cmdCPU), "ps -p %d -o %%cpu --no-headers", memhold.userProcessPID);
    bytesSoFar += snprintf(cmdMEM, sizeof(cmdMEM), "ps -p %d -o rss --no-headers", memhold.userProcessPID);
    bytesSoFar += snprintf(cmdGetProcName, sizeof(cmdGetProcName), "ps aux | grep %d", memhold.userProcessPID);
    assert(bytesSoFar >= 64 && bytesSoFar <= stackAllocCmd); //> 79 >= 64

    #if 0            // TEMP LOG to stdout Process Name
        int ret = system(cmdGetProcName);
        if (ret != 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "[ ERR! ]  failed to execute command. system call returned: %d", ret);
            perror(msg);
            exit(1);
        };
    #endif           /* if 0 */

    const int CMD_MAX = 1035;
    int       pstatus;
    FILE     *pipefp;
    char      cmd[CMD_MAX];
    pipefp = popen(cmdGetProcName, "r");
    if (pipefp == NULL) {
        perror("[ ERR! ]  failed to execute popen for getting process name via its PID");
        exit(1);
    }

    int cntr = 0;
    while (fgets(cmd, CMD_MAX, pipefp) != NULL) {
        cntr++;
        fprintf(stdout, "[  OK  ] [%d]: %s", cntr, cmd);
    }

    pstatus = pclose(pipefp);
    if (pstatus == -1) perror("[ ERR! ] pclose"); // todo
    #if MEMHOLD_SLOW // TODO: use macros (From examples in popen() are marvelous articles?)
    else fprintf(stdout, "[ INFO ] pclose returned status for: %s\n", cmdGetProcName);
    #endif           /* if MEMHOLD_SLOW */

#endif /* if MEMHOLD_YAGNI */
    //----------------------------------------------------------------------------------

    // Run main loop
    //----------------------------------------------------------------------------------
    int                loopCounter = 0;
    double             cpuPercent  = 0;
    unsigned long long cpuTime1, cpuTime2;
    unsigned int       cpuWaitASecond = 1;


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
        // Similar to procs. And somewhat like top, htop, btop (but not ncurses like)
        //----------------------------------------------------------------------------------
        //
        // cpuUsage_         = GetCpuUsage(memhold.userProcessPID);
        memUsageThisFrame = GetMemUsage(memhold.userProcessPID);

        if (memhold.flagVerbose)
        {
            fprintf(stdout, "[ INFO ]  PID: %d  CPU: %3.6f%%  \t%ld\n", memhold.userProcessPID, cpuPercent, clock());
            fprintf(stdout, "[ INFO ]  PID: %d  MEM: %8zuK  \t%ld\n", memhold.userProcessPID, memUsageThisFrame, clock());
        }
        //----------------------------------------------------------------------------------

        // Pause this frame (2s per frame by default.)
        //----------------------------------------------------------------------------------
        { // Wait for 1 second
            cpuTime1 = GetCpuUsage(memhold.userProcessPID);
            sleep(cpuWaitASecond);
            cpuTime2 = GetCpuUsage(memhold.userProcessPID);
        }

        // Calculate CPU usage
        //
        // We use sysconf(_SC_CLK_TCK) to get the number of clock ticks per
        // second, which allows us to convert from clock ticks to seconds.
        // @sysconf: Get the value of the system variable NAME. _SC_CLK_TCK:
        // Type: `int`  Value = `2`
        //
        cpuPercent = (1.0 * (cpuTime2 - cpuTime1)) / sysconf(_SC_CLK_TCK); // Multiply by 1.0 to avoid precision loss
                                                                           // while dividing in next instruction

#if 0
        // Disabled as the value is too small and seems to be double of CPU% in
        // btop for the running user process.
        cpuPercent /= 100.0;
#endif /* if 0 */

        // Ensure the loop waited for total `memhold.refreshSeconds` seconds
        if (cpuWaitASecond < memhold.refreshSeconds) { sleep(memhold.refreshSeconds - cpuWaitASecond); }
        //----------------------------------------------------------------------------------
    }
    // end while (1)
    //----------------------------------------------------------------------------------

    // Unload program
    //----------------------------------------------------------------------------------
    // NOTE(Lloyd): Unload more data or free memory here... (e.g. ML_FREE(...))
    // ...

    if (memhold.flagVerbose)
    {
        fprintf(stdout, "\n[ INFO ]  <<< Stage 3: Cleanup and Exit >>>\n\n");
        fprintf(stdout, "[ INFO ]  took %.2fs\n", loopCounter * memhold.refreshSeconds);
    }
    //----------------------------------------------------------------------------------

    return status;
};

// Main entry point of the program.
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        exit(1);
    }


    // Declare main functions scoped variables
    //----------------------------------------------------------------------------------
    int status;
    //----------------------------------------------------------------------------------


    // Parse args and ensure a valid process PID is passed.
    //----------------------------------------------------------------------------------
    if (!(argc < 2))
    {
        switch (argc - 1)
        {

        case 2:
            if (strcmp(argv[2], "--verbose") == 0) { gVerbose = true; }
            else gVerbose = false;
            break;

        default: break;
        }
    }

    // Convert argv[1] (<PID>: stdout of `$ pgrep lua`) to a long integer.
    gProcPID = (pid_t)(strtol(argv[1], NULL, 10));

    // <<<<<<< HOW TO FIGURE OUT WHAT A VALID PID IS???? >>>>>>
    // If is invalid (not a number or integer.) then
    if (!(gProcPID >= 0))
    {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        fprintf(stderr, "expected valid PID. For example: 105815\n. got: %i", gProcPID);
        status = 1;

        goto cleanupError;
    }
    //----------------------------------------------------------------------------------


    // Write stdout program name and version
    //----------------------------------------------------------------------------------
    fprintf(stdout, "%s %s\n", MEMHOLD_ID, MEMHOLD_VERSION);

    if (gVerbose)
    {
        fprintf(stdout, "\n[ INFO ]  <<< Stage 1: Initialize program >>>\n\n");
        {
            fprintf(stdout, "[ INFO ]  ");
            for (int i = 0; i < argc; i++)
                fprintf(stdout, "%s ", argv[i]);
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "[ INFO ]  Verbose: %s\n", gVerbose ? "true" : "false");
    }
    //----------------------------------------------------------------------------------


    // Initialize module
    //----------------------------------------------------------------------------------
    status = Init();

    if (status != 0)
    {
        fprintf(stderr, "[ ERR! ]  failed to initialize module\n");

        goto cleanupError;
    }
    //----------------------------------------------------------------------------------

    // Begin memhold hot loop.
    //----------------------------------------------------------------------------------
    status = RunMain();
    //----------------------------------------------------------------------------------

cleanupError:

    return status; // EXIT_SUCCESS
}

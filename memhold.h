#ifndef MEMHOLD_H
    #define MEMHOLD_H

    #include <stdarg.h> // Required for: va_list - Only used by TraceLogCallback

//// NOTE(Lloyd): The following is ported from raylib.h

    #define MEMHOLD_VERSION_MAJOR 0
    #define MEMHOLD_VERSION_MINOR 1
    #define MEMHOLD_VERSION_PATCH 0
    #define MEMHOLD_VERSION       "0.1"

    // Function specifiers in case library is build/used as a shared library (Windows)
    // NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
    #if defined(_WIN32)
        #if defined(BUILD_LIBTYPE_SHARED)
            #if defined(__TINYC__)
                #define __declspec(x) __attribute__((x))
            #endif
            #define MHAPI __declspec(dllexport) // We are building the library as a Win32 shared library (.dll)
        #elif defined(USE_LIBTYPE_SHARED)
            #define MHAPI __declspec(dllimport) // We are using the library as a Win32 shared library (.dll)
        #endif
    #endif

    #ifndef MHAPI
        #define MHAPI // Functions defined as 'extern' by default (implicit specifiers)
    #endif

    //----------------------------------------------------------------------------------
    // Some basic Defines
    //----------------------------------------------------------------------------------

    // NOTE(Lloyd): The following is ported from raylib.h

    // Allow custom memory allocators
    // NOTE: Require recompiling raylib sources
    #ifndef MH_MALLOC
        #define MH_MALLOC(sz) malloc(sz)
    #endif
    #ifndef MH_CALLOC
        #define MH_CALLOC(n, sz) calloc(n, sz)
    #endif
    #ifndef MH_REALLOC
        #define MH_REALLOC(ptr, sz) realloc(ptr, sz)
    #endif
    #ifndef MH_FREE
        #define MH_FREE(ptr) free(ptr)
    #endif

    // NOTE: MSVC C++ compiler does not support compound literals (C99 feature)
    // Plain structures in C++ (without constructors) can be initialized with { }
    // This is called aggregate initialization (C++11 feature)
    #if defined(__cplusplus)
        #define CLITERAL(type) type
    #else
        #define CLITERAL(type) (type)
    #endif

    // Some compilers (mostly macos clang) default to C++98,
    // where aggregate initialization can't be used
    // So, give a more clear error stating how to fix this
    #if !defined(_MSC_VER) && (defined(__cplusplus) && __cplusplus < 201103L)
        #error "C++11 or later is required. Add -std=c++11"
    #endif

/*
    // NOTE: We set some defines with some data types declared by raylib
    // Other modules (raymath, rlgl) also require some of those types, so,
    // to be able to use those other modules as standalone (not depending on raylib)
    // this defines are very useful for internal check and avoid type (re)definitions
    #define RL_COLOR_TYPE
    #define RL_RECTANGLE_TYPE
    #define RL_VECTOR2_TYPE
    #define RL_VECTOR3_TYPE
    #define RL_VECTOR4_TYPE
    #define RL_QUATERNION_TYPE
    #define RL_MATRIX_TYPE
*/

//----------------------------------------------------------------------------------
// Structures Definition
//----------------------------------------------------------------------------------

// Boolean type
    #if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
        #include <stdbool.h>
    #elif !defined(__cplusplus) && !defined(bool)
typedef enum bool
{
    false = 0,
    true  = !false
} bool;
        #define RL_BOOL_TYPE
    #endif

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color
{
    unsigned char r; // Color red value
    unsigned char g; // Color green value
    unsigned char b; // Color blue value
    unsigned char a; // Color alpha value
} Color;

//----------------------------------------------------------------------------------
// Enumerators Definition
//----------------------------------------------------------------------------------

// System/Window config flags
// NOTE: Every bit registers one state (use it with bit masks)
// By default all flags are set to 0
typedef enum
{
    FLAG_VSYNC_HINT               = 0x00000040, // Set to try enabling V-Sync on GPU
    FLAG_FULLSCREEN_MODE          = 0x00000002, // Set to run program in fullscreen
    FLAG_WINDOW_RESIZABLE         = 0x00000004, // Set to allow resizable window
    FLAG_WINDOW_UNDECORATED       = 0x00000008, // Set to disable window decoration (frame and buttons)
    FLAG_WINDOW_HIDDEN            = 0x00000080, // Set to hide window
    FLAG_WINDOW_MINIMIZED         = 0x00000200, // Set to minimize window (iconify)
    FLAG_WINDOW_MAXIMIZED         = 0x00000400, // Set to maximize window (expanded to monitor)
    FLAG_WINDOW_UNFOCUSED         = 0x00000800, // Set to window non focused
    FLAG_WINDOW_TOPMOST           = 0x00001000, // Set to window always on top
    FLAG_WINDOW_ALWAYS_RUN        = 0x00000100, // Set to allow windows running while minimized
    FLAG_WINDOW_TRANSPARENT       = 0x00000010, // Set to allow transparent framebuffer
    FLAG_WINDOW_HIGHDPI           = 0x00002000, // Set to support HighDPI
    FLAG_WINDOW_MOUSE_PASSTHROUGH = 0x00004000, // Set to support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
    FLAG_BORDERLESS_WINDOWED_MODE = 0x00008000, // Set to run program in borderless windowed mode
    FLAG_MSAA_4X_HINT             = 0x00000020, // Set to try enabling MSAA 4X
    FLAG_INTERLACED_HINT          = 0x00010000  // Set to try enabling interlaced video format (for V3D)
} ConfigFlags;

// Trace log level
// NOTE: Organized by priority level
typedef enum
{
    LOG_ALL = 0, // Display all logs
    LOG_TRACE,   // Trace logging, intended for internal use only
    LOG_DEBUG,   // Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO,    // Info logging, used for program execution info
    LOG_WARNING, // Warning logging, used on recoverable failures
    LOG_ERROR,   // Error logging, used on unrecoverable failures
    LOG_FATAL,   // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE     // Disable logging
} TraceLogLevel;

// Callbacks to hook some internal functions
// WARNING: These callbacks are intended for advance users
typedef void (*TraceLogCallback)(int logLevel, const char *text, va_list args);       // Logging: Redirect trace log messages
typedef unsigned char *(*LoadFileDataCallback)(const char *fileName, int *dataSize);  // FileIO: Load binary data
typedef bool (*SaveFileDataCallback)(const char *fileName, void *data, int dataSize); // FileIO: Save binary data
typedef char *(*LoadFileTextCallback)(const char *fileName);                          // FileIO: Load text data
typedef bool (*SaveFileTextCallback)(const char *fileName, char *text);               // FileIO: Save text data

//------------------------------------------------------------------------------------
// Global Variables Definition
//------------------------------------------------------------------------------------
// It's lonely here...

//------------------------------------------------------------------------------------
// Window and Graphics Device Functions (Module: core)
//------------------------------------------------------------------------------------

    #if defined(__cplusplus)
extern "C"
{ // Prevents name mangling of functions
    #endif

    // Window-related functions
    MHAPI void InitWindow(int width, int height, const char *title); // Initialize window and OpenGL context
    MHAPI void CloseWindow(void);                                    // Close window and unload OpenGL context
    MHAPI bool WindowShouldClose(void);                              // Check if application should close (KEY_ESCAPE pressed or windows close icon clicked)
                                                                     //
    // Timing-related functions
    MHAPI void   SetTargetFPS(int fps); // Set target FPS (maximum)
    MHAPI float  GetFrameTime(void);    // Get time in seconds for last frame drawn (delta time)
    MHAPI double GetTime(void);         // Get elapsed time in seconds since InitWindow()
    MHAPI int    GetFPS(void);          // Get current FPS

    // Custom frame control functions
    // NOTE: Those functions are intended for advance users that want full control over the frame processing
    // By default EndDrawing() does this job: draws everything + SwapScreenBuffer() + manage frame timing + PollInputEvents()
    // To avoid that behaviour and control frame processes manually, enable in config.h: SUPPORT_CUSTOM_FRAME_CONTROL
    MHAPI void SwapScreenBuffer(void);   // Swap back buffer with front buffer (screen drawing)
    MHAPI void PollInputEvents(void);    // Register all input events
    MHAPI void WaitTime(double seconds); // Wait for some time (halt program execution)

    // Random values generation functions
    MHAPI void SetRandomSeed(unsigned int seed);                         // Set the seed for the random number generator
    MHAPI int  GetRandomValue(int min, int max);                         // Get a random value between min and max (both included)
    MHAPI int *LoadRandomSequence(unsigned int count, int min, int max); // Load random values sequence, no values repeated
    MHAPI void UnloadRandomSequence(int *sequence);                      // Unload random values sequence

    // Misc. functions
    MHAPI void TakeScreenshot(const char *fileName); // Takes a screenshot of current screen (filename extension defines format)
    MHAPI void SetConfigFlags(unsigned int flags);   // Setup init configuration flags (view FLAGS)
    MHAPI void OpenURL(const char *url);             // Open URL with default system browser (if available)

    // NOTE: Following functions implemented in module [utils]
    //------------------------------------------------------------------
    MHAPI void  TraceLog(int logLevel, const char *text, ...); // Show trace log messages (LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR...)
    MHAPI void  SetTraceLogLevel(int logLevel);                // Set the current threshold (minimum) log level
    MHAPI void *MemAlloc(unsigned int size);                   // Internal memory allocator
    MHAPI void *MemRealloc(void *ptr, unsigned int size);      // Internal memory reallocator
    MHAPI void  MemFree(void *ptr);                            // Internal memory free

    // Set custom callbacks
    // WARNING: Callbacks setup is intended for advance users
    MHAPI void SetTraceLogCallback(TraceLogCallback callback);         // Set custom trace log
    MHAPI void SetLoadFileDataCallback(LoadFileDataCallback callback); // Set custom file binary data loader
    MHAPI void SetSaveFileDataCallback(SaveFileDataCallback callback); // Set custom file binary data saver
    MHAPI void SetLoadFileTextCallback(LoadFileTextCallback callback); // Set custom file text data loader
    MHAPI void SetSaveFileTextCallback(SaveFileTextCallback callback); // Set custom file text data saver

    // Files management functions
    MHAPI unsigned char *LoadFileData(const char *fileName, int *dataSize);            // Load file data as byte array (read)
    MHAPI void           UnloadFileData(unsigned char *data);                          // Unload file data allocated by LoadFileData()
    MHAPI bool           SaveFileData(const char *fileName, void *data, int dataSize); // Save data to file from byte array (write), returns true on success
    MHAPI bool           ExportDataAsCode(const unsigned char *data, int dataSize, const char *fileName); // Export data to code (.h), returns true on success
    MHAPI char          *LoadFileText(const char *fileName);   // Load text data from file (read), returns a '\0' terminated string
    MHAPI void           UnloadFileText(char *text);           // Unload file text data allocated by LoadFileText()
    MHAPI bool SaveFileText(const char *fileName, char *text); // Save text data to file (write), string must be '\0' terminated, returns true on success
    //------------------------------------------------------------------

    #if defined(__cplusplus)
}
    #endif

#endif // !MEMHOLD_H

// EPILOGUE
//
// Alterations, modifications, and ports: ~
//   + RLAPI -> MHAPI
//      + Functions and Data structures naming used as a good starter base
//        reference for library API.
//   + ConfigFlags
//   + TraceLogLevel
//   + ...Callback() (*function pointers*)

// raylib
//
// LICENSE
/*
Copyright (c) 2013-2023 Ramon Santamaria (@raysan5)

This software is provided "as-is", without any express or implied warranty. In no event
will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial
applications, and to alter it and redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you
  wrote the original software. If you use this software in a product, an acknowledgment
  in the product documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be misrepresented
  as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

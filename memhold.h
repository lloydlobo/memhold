#ifndef MEMHOLD_H
#define MEMHOLD_H

#include <stdarg.h> // Required for: va_list - Only used by TraceLogCallback

// NOTE(Lloyd): The following is ported from raylib.h

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

#endif // !MEMHOLD_H

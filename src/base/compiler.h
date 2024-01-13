#pragma once

//NOTE: Contains definitions to help identify the compiler used

#include "system.h"

#ifdef __clang__
    #define COMP_CLANG 1
    #define COMP_GCC   0
#elif defined(__GNUC__)
    #define COMP_CLANG 0
    #define COMP_GCC   1
#else
    #error "Compiler not supported"
#endif

#include <assert.h>

#if COMP_CLANG
    #define STATIC_ASSERT(a, m) static_assert(a, m)

    #if __has_builtin(__builtin_debugtrap)
        #define DEBUG_BRK()     __builtin_debugtrap()
    #else
        #include <stdlib.h>
        #define DEBUG_BRK()     abort()
    #endif
#elif COMP_GCC
    #define STATIC_ASSERT(a, m) _Static_assert(a, m)

    #if defined(SIGTRAP)
        #if SYSTEM_POSIX
            #define DEBUG_BRK() raise(SIGTRAP)
        #endif
    #else
        #include <stdlib.h>
        #define DEBUG_BRK()     abort()
    #endif
#endif


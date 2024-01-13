#pragma once

//NOTE: This header file contains defines used to determine the system on which this program is being compiled on

// Is a posix system ?
#if defined(__unix__) || (defined (__APPLE__) && defined ( __MACH__) )
    #define SYSTEM_POSIX    1
#else
    #define SYSTEM_POSIX    0
#endif

#if defined(_WIN32) && defined (_WIN64)
    #define SYSTEM_WINDOWS  1
    #define SYSTEM_NAME     "Windows 64-bit"
#elif defined(_WIN32) && !defined(_WIN64)
    #error "Only Windows 64bit is supported"
#else
    #define SYSTEM_WINDOWS  0
#endif

#if defined(__APPLE__) && defined (__MACH__)
    #define SYSTEM_MAC      1
    #define SYSTEM_NAME     "MacOS"
#else
    #define SYSTEM_MAC      0
#endif

#if !defined(__ANDROID__) && defined (__gnu_linux__)
    #define SYSTEM_LINUX    1
    #define SYSTEM_GNULINUX 1
    #define SYSTEM_NAME     "GNU/Linux"
#else
    #define SYSTEM_LINUX    0
    #define SYSTEM_GNULINUX 0
#endif

#if !defined(SYSTEM_NAME)
    #error "System identification failed (or not supported) !"
#endif


/**
 * @file libgossip_api.h
 * @brief C-compatible API export/import macros
 *
 * This header provides the LIBGOSSIP_API macro that works with both C and C++ compilers.
 * It can be safely included from C code.
 */

#ifndef LIBGOSSIP_API_H
#define LIBGOSSIP_API_H

// API Export/Import Macros
#ifdef LIBGOSSIP_BUILD
    #ifdef _WIN32
        #define LIBGOSSIP_API __declspec(dllexport)
    #else
        #define LIBGOSSIP_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define LIBGOSSIP_API __declspec(dllimport)
    #else
        #define LIBGOSSIP_API
    #endif
#endif

// Platform Detection
#if defined(_WIN32) || defined(_WIN64)
    #define LIBGOSSIP_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define LIBGOSSIP_PLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define LIBGOSSIP_PLATFORM_MACOS 1
#endif

#endif // LIBGOSSIP_API_H

/**
 * @file config.hpp
 * @brief Library configuration and feature detection
 * 
 * This header provides compile-time configuration options and feature detection
 * for libgossip. It can be customized via CMake options.
 */

#pragma once

#include <cstdint>
#include <cstddef>

// ============================================
// Library Identification
// ============================================
#define LIBGOSSIP_NAME "libgossip"
#define LIBGOSSIP_VERSION "1.1.2"
#define LIBGOSSIP_VERSION_MAJOR 1
#define LIBGOSSIP_VERSION_MINOR 1
#define LIBGOSSIP_VERSION_PATCH 2
#define LIBGOSSIP_VERSION_NUMBER 10102

// ============================================
// Feature Flags
// ============================================

// Enable thread safety (always on for production)
#ifndef LIBGOSSIP_ENABLE_THREAD_SAFETY
#define LIBGOSSIP_ENABLE_THREAD_SAFETY 1
#endif

// Enable verbose logging (controlled by CMake)
#ifndef LIBGOSSIP_ENABLE_LOGGING
#define LIBGOSSIP_ENABLE_LOGGING 1
#endif

// Enable debug checks (controlled by CMake)
#ifndef LIBGOSSIP_ENABLE_DEBUG_CHECKS
#ifdef NDEBUG
#define LIBGOSSIP_ENABLE_DEBUG_CHECKS 0
#else
#define LIBGOSSIP_ENABLE_DEBUG_CHECKS 1
#endif
#endif

// Enable assertions (always enabled in debug builds)
#ifndef LIBGOSSIP_ENABLE_ASSERTIONS
#ifdef NDEBUG
#define LIBGOSSIP_ENABLE_ASSERTIONS 0
#else
#define LIBGOSSIP_ENABLE_ASSERTIONS 1
#endif
#endif

// Enable network statistics
#ifndef LIBGOSSIP_ENABLE_NETWORK_STATS
#define LIBGOSSIP_ENABLE_NETWORK_STATS 1
#endif

// ============================================
// Configuration Constants
// ============================================

// Default configuration values (can be overridden at runtime)
namespace libgossip::config {

// ============================================
// Gossip Protocol Configuration
// ============================================

/// Default heartbeat interval in milliseconds
constexpr uint32_t DEFAULT_HEARTBEAT_INTERVAL_MS = 1000;

/// Default suspicion timeout multiplier
constexpr uint32_t DEFAULT_SUSPICION_TIMEOUT_MULTIPLIER = 5;

/// Default ping timeout in milliseconds
constexpr uint32_t DEFAULT_PING_TIMEOUT_MS = 500;

/// Default ping request timeout in milliseconds
constexpr uint32_t DEFAULT_PING_REQUEST_TIMEOUT_MS = 1000;

/// Default indirect ping retries
constexpr uint32_t DEFAULT_INDIRECT_PING_RETRIES = 3;

/// Default failure detection timeout in milliseconds
constexpr uint32_t DEFAULT_FAILURE_TIMEOUT_MS = 3000;

/// Default sync interval in milliseconds
constexpr uint32_t DEFAULT_SYNC_INTERVAL_MS = 10000;

/// Default gossip fanout (number of nodes to gossip to)
constexpr uint32_t DEFAULT_GOSSIP_FANOUT = 3;

/// Default message serialization format
enum class serialization_format {
    json,      // JSON (default, readable)
    protobuf,  // Protocol Buffers (fast, compact) - not yet implemented
    msgpack,   // MessagePack (fast, compact) - not yet implemented
};

constexpr serialization_format DEFAULT_SERIALIZATION_FORMAT = serialization_format::json;

// ============================================
// Network Configuration
// ============================================

/// Default TCP receive buffer size in bytes
constexpr size_t DEFAULT_TCP_RECV_BUFFER_SIZE = 65536;

/// Default UDP receive buffer size in bytes
constexpr size_t DEFAULT_UDP_RECV_BUFFER_SIZE = 65536;

/// Default maximum message size in bytes
constexpr size_t DEFAULT_MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB

/// Default connection timeout in milliseconds
constexpr uint32_t DEFAULT_CONNECTION_TIMEOUT_MS = 5000;

/// Default socket send timeout in milliseconds
constexpr uint32_t DEFAULT_SOCKET_SEND_TIMEOUT_MS = 3000;

/// Default socket receive timeout in milliseconds
constexpr uint32_t DEFAULT_SOCKET_RECV_TIMEOUT_MS = 3000;

/// Default maximum concurrent connections
constexpr size_t DEFAULT_MAX_CONCURRENT_CONNECTIONS = 1000;

// ============================================
// Node Configuration
// ============================================

/// Default node status refresh interval in milliseconds
constexpr uint32_t DEFAULT_NODE_REFRESH_INTERVAL_MS = 1000;

/// Default maximum number of nodes in cluster
constexpr size_t DEFAULT_MAX_NODES = 1000;

/// Default node metadata size limit in bytes
constexpr size_t DEFAULT_NODE_METADATA_SIZE_LIMIT = 65536;  // 64KB

/// Default number of indirect nodes to use for ping requests
constexpr uint32_t DEFAULT_INDIRECT_NODES_COUNT = 3;

// ============================================
// Performance Tuning
// ============================================

/// Default number of IO threads for network operations
constexpr uint32_t DEFAULT_IO_THREADS = 1;

/// Enable message batching (experimental)
#ifndef LIBGOSSIP_ENABLE_MESSAGE_BATCHING
#define LIBGOSSIP_ENABLE_MESSAGE_BATCHING 0
#endif

/// Message batch size (when batching is enabled)
constexpr size_t DEFAULT_MESSAGE_BATCH_SIZE = 10;

/// Message batch timeout in milliseconds
constexpr uint32_t DEFAULT_MESSAGE_BATCH_TIMEOUT_MS = 50;

// ============================================
// Security Configuration (Future)
// ============================================

/// Enable TLS encryption (not yet implemented)
#ifndef LIBGOSSIP_ENABLE_TLS
#define LIBGOSSIP_ENABLE_TLS 0
#endif

/// Enable node authentication (not yet implemented)
#ifndef LIBGOSSIP_ENABLE_AUTHENTICATION
#define LIBGOSSIP_ENABLE_AUTHENTICATION 0
#endif

/// Enable message signing (not yet implemented)
#ifndef LIBGOSSIP_ENABLE_MESSAGE_SIGNING
#define LIBGOSSIP_ENABLE_MESSAGE_SIGNING 0
#endif

/// Default TLS version (when TLS is enabled)
constexpr uint32_t DEFAULT_TLS_VERSION = 0x303;  // TLS 1.2

// ============================================
// Debugging and Monitoring
// ============================================

/// Enable detailed logging (very verbose)
#ifndef LIBGOSSIP_ENABLE_VERBOSE_LOGGING
#define LIBGOSSIP_ENABLE_VERBOSE_LOGGING 0
#endif

/// Enable performance metrics collection
#ifndef LIBGOSSIP_ENABLE_PERFORMANCE_METRICS
#define LIBGOSSIP_ENABLE_PERFORMANCE_METRICS 1
#endif

/// Enable memory usage tracking
#ifndef LIBGOSSIP_ENABLE_MEMORY_TRACKING
#define LIBGOSSIP_ENABLE_MEMORY_TRACKING 0
#endif

/// Enable call stack tracing (debug only)
#ifndef LIBGOSSIP_ENABLE_CALL_STACK_TRACING
#define LIBGOSSIP_ENABLE_CALL_STACK_TRACING 0
#endif

// ============================================
// Platform Detection
// ============================================

#if defined(_WIN32) || defined(_WIN64)
#define LIBGOSSIP_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define LIBGOSSIP_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#define LIBGOSSIP_PLATFORM_MACOS 1
#elif defined(__FreeBSD__)
#define LIBGOSSIP_PLATFORM_FREEBSD 1
#else
#define LIBGOSSIP_PLATFORM_UNKNOWN 1
#endif

// ============================================
// Compiler Detection
// ============================================

#if defined(_MSC_VER)
#define LIBGOSSIP_COMPILER_MSVC _MSC_VER
#elif defined(__clang__)
#define LIBGOSSIP_COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#define LIBGOSSIP_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#define LIBGOSSIP_COMPILER_UNKNOWN 1
#endif

// ============================================
// Architecture Detection
// ============================================

#if defined(__x86_64__) || defined(_M_X64)
#define LIBGOSSIP_ARCH_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define LIBGOSSIP_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
#define LIBGOSSIP_ARCH_ARM 1
#else
#define LIBGOSSIP_ARCH_UNKNOWN 1
#endif

// ============================================
// Endianness Detection
// ============================================

#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LIBGOSSIP_LITTLE_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LIBGOSSIP_BIG_ENDIAN 1
#endif
#elif defined(_WIN32)
#define LIBGOSSIP_LITTLE_ENDIAN 1
#endif

// ============================================
// Utility Macros
// ============================================

/// Mark a function as noexcept if exceptions are enabled
#if LIBGOSSIP_ENABLE_DEBUG_CHECKS
#define LIBGOSSIP_NOEXCEPT noexcept(false)
#else
#define LIBGOSSIP_NOEXCEPT noexcept
#endif

/// Force inline (when optimization is enabled)
#if defined(NDEBUG)
#define LIBGOSSIP_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define LIBGOSSIP_FORCE_INLINE __forceinline
#else
#define LIBGOSSIP_FORCE_INLINE inline
#endif

/// Mark a variable as unused
#define LIBGOSSIP_UNUSED(x) (void)(x)

/// Compile-time assertion
#define LIBGOSSIP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)

/// Runtime assertion (only in debug builds)
#if LIBGOSSIP_ENABLE_ASSERTIONS
#define LIBGOSSIP_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "Assertion failed: %s at %s:%d\n", \
                        #cond, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while(0)
#else
#define LIBGOSSIP_ASSERT(cond) ((void)0)
#endif

/// Debug-only check
#if LIBGOSSIP_ENABLE_DEBUG_CHECKS
#define LIBGOSSIP_DEBUG_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::fprintf(stderr, "Debug check failed: %s at %s:%d: %s\n", \
                        #cond, __FILE__, __LINE__, msg); \
        } \
    } while(0)
#else
#define LIBGOSSIP_DEBUG_CHECK(cond, msg) ((void)0)
#endif

} // namespace libgossip::config

// ============================================
// API Export/Import Macros
// ============================================

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
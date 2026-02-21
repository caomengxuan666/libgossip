/**
 * @file version_check.hpp
 * @brief Version compatibility checks for libgossip
 * 
 * This header provides compile-time and runtime version checks
 * to ensure compatibility between different versions of libgossip.
 */

#pragma once

#include <cstdint>
#include <string>
#include <sstream>

namespace libgossip {

/**
 * @brief Version information structure
 */
struct version_info {
    uint8_t major;      // Major version (breaking changes)
    uint8_t minor;      // Minor version (new features, backward compatible)
    uint8_t patch;      // Patch version (bug fixes, backward compatible)
    uint8_t build;      // Build number

    /**
     * @brief Default constructor (sets to 0.0.0.0)
     */
    constexpr version_info() 
        : major(0), minor(0), patch(0), build(0) {}

    /**
     * @brief Constructor with version components
     */
    constexpr version_info(uint8_t maj, uint8_t min, uint8_t pat, uint8_t bld = 0)
        : major(maj), minor(min), patch(pat), build(bld) {}

    /**
     * @brief Convert to string representation
     */
    std::string to_string() const {
        std::ostringstream oss;
        oss << static_cast<int>(major) << "." 
            << static_cast<int>(minor) << "." 
            << static_cast<int>(patch);
        if (build > 0) {
            oss << "." << static_cast<int>(build);
        }
        return oss.str();
    }

    /**
     * @brief Compare versions (returns -1 if < this, 0 if equal, 1 if > this)
     */
    int compare(const version_info& other) const {
        if (major != other.major) return (major > other.major) ? 1 : -1;
        if (minor != other.minor) return (minor > other.minor) ? 1 : -1;
        if (patch != other.patch) return (patch > other.patch) ? 1 : -1;
        if (build != other.build) return (build > other.build) ? 1 : -1;
        return 0;
    }

    /**
     * @brief Equality operators
     */
    constexpr bool operator==(const version_info& other) const {
        return major == other.major && minor == other.minor && 
               patch == other.patch && build == other.build;
    }

    constexpr bool operator!=(const version_info& other) const {
        return !(*this == other);
    }

    /**
     * @brief Comparison operators
     */
    constexpr bool operator<(const version_info& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (patch != other.patch) return patch < other.patch;
        return build < other.build;
    }

    constexpr bool operator>(const version_info& other) const {
        return other < *this;
    }

    constexpr bool operator<=(const version_info& other) const {
        return !(other < *this);
    }

    constexpr bool operator>=(const version_info& other) const {
        return !(*this < other);
    }
};

/**
 * @brief Current library version
 */
constexpr version_info LIBGOSSIP_VERSION = {1, 1, 2, 0};

/**
 * @brief Minimum compatible library version
 */
constexpr version_info LIBGOSSIP_MIN_COMPATIBLE_VERSION = {1, 0, 0, 0};

/**
 * @brief Check if a version is compatible with the current library version
 * 
 * A version is compatible if:
 * - Major version matches exactly
 * - Minor version is not older than the minimum compatible version
 * 
 * @param version Version to check
 * @return true if compatible, false otherwise
 */
constexpr bool is_version_compatible(const version_info& version) {
    return (version.major == LIBGOSSIP_VERSION.major) &&
           (version >= LIBGOSSIP_MIN_COMPATIBLE_VERSION);
}

/**
 * @brief Check protocol version compatibility
 * 
 * Different major versions have incompatible protocols.
 * 
 * @param protocol_version Protocol version to check
 * @return true if protocol is compatible
 */
constexpr bool is_protocol_compatible(uint8_t protocol_version) {
    // Currently only protocol version 1 is supported
    return protocol_version == 1;
}

/**
 * @brief Get current protocol version
 */
constexpr uint8_t get_protocol_version() {
    return 1;
}

/**
 * @brief Version checking result
 */
enum class version_check_result {
    compatible,           // Versions are compatible
    incompatible_major,   // Major version mismatch
    too_old,              // Version is too old
    too_new,              // Version is too new (future)
    invalid               // Invalid version data
};

/**
 * @brief Check version compatibility at runtime
 * 
 * @param version Version to check
 * @return Version check result
 */
constexpr version_check_result check_version_compatibility(const version_info& version) {
    // Check for invalid version
    if (version.major == 0 && version.minor == 0 && version.patch == 0) {
        return version_check_result::invalid;
    }

    // Check major version
    if (version.major != LIBGOSSIP_VERSION.major) {
        return version_check_result::incompatible_major;
    }

    // Check if too old
    if (version < LIBGOSSIP_MIN_COMPATIBLE_VERSION) {
        return version_check_result::too_old;
    }

    // Check if too new (future version)
    if (version > LIBGOSSIP_VERSION) {
        return version_check_result::too_new;
    }

    return version_check_result::compatible;
}

/**
 * @brief Get version check result as string
 */
inline const char* version_check_result_to_string(version_check_result result) {
    switch (result) {
        case version_check_result::compatible:
            return "Compatible";
        case version_check_result::incompatible_major:
            return "Incompatible major version";
        case version_check_result::too_old:
            return "Version too old";
        case version_check_result::too_new:
            return "Version too new (future)";
        case version_check_result::invalid:
            return "Invalid version";
        default:
            return "Unknown";
    }
}

} // namespace libgossip

/**
 * @brief Compile-time version check
 * 
 * Ensures that the library is compiled with a compatible API version.
 */
#define LIBGOSSIP_VERSION_CHECK() \
    static_assert(libgossip::LIBGOSSIP_VERSION.major == 1, \
                  "libgossip major version mismatch")

/**
 * @brief Ensure the library version is at least a certain version
 */
#define LIBGOSSIP_REQUIRE_VERSION(major, minor, patch) \
    static_assert( \
        (libgossip::LIBGOSSIP_VERSION.major > (major)) || \
        (libgossip::LIBGOSSIP_VERSION.major == (major) && \
         libgossip::LIBGOSSIP_VERSION.minor > (minor)) || \
        (libgossip::LIBGOSSIP_VERSION.major == (major) && \
         libgossip::LIBGOSSIP_VERSION.minor == (minor) && \
         libgossip::LIBGOSSIP_VERSION.patch >= (patch)), \
        "libgossip version requirement not met")

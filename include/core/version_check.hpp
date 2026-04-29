/**
 * @file version_check.hpp
 * @brief Version compatibility checks for libgossip
 *
 * This header provides compile-time and runtime version checks
 * to ensure compatibility between different versions of libgossip.
 */

#pragma once

#include "config.hpp"
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

    constexpr version_info()
        : major(0), minor(0), patch(0) {}

    constexpr version_info(uint8_t maj, uint8_t min, uint8_t pat)
        : major(maj), minor(min), patch(pat) {}

    std::string to_string() const {
        std::ostringstream oss;
        oss << static_cast<int>(major) << "."
            << static_cast<int>(minor) << "."
            << static_cast<int>(patch);
        return oss.str();
    }

    int compare(const version_info& other) const {
        if (major != other.major) return (major > other.major) ? 1 : -1;
        if (minor != other.minor) return (minor > other.minor) ? 1 : -1;
        if (patch != other.patch) return (patch > other.patch) ? 1 : -1;
        return 0;
    }

    constexpr bool operator==(const version_info& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    constexpr bool operator!=(const version_info& other) const {
        return !(*this == other);
    }

    constexpr bool operator<(const version_info& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
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
constexpr version_info LIBGOSSIP_VERSION_INFO = {
    LIBGOSSIP_VERSION_MAJOR,
    LIBGOSSIP_VERSION_MINOR,
    LIBGOSSIP_VERSION_PATCH
};

/**
 * @brief Minimum compatible library version
 */
constexpr version_info LIBGOSSIP_MIN_COMPATIBLE_VERSION = {1, 0, 0};

/**
 * @brief Check if a version is compatible with the current library version
 */
constexpr bool is_version_compatible(const version_info& version) {
    return (version.major == LIBGOSSIP_VERSION_INFO.major) &&
           (version >= LIBGOSSIP_MIN_COMPATIBLE_VERSION);
}

/**
 * @brief Version checking result
 */
enum class version_check_result {
    compatible,
    incompatible_major,
    too_old,
    too_new,
    invalid
};

/**
 * @brief Check version compatibility at runtime
 */
constexpr version_check_result check_version_compatibility(const version_info& version) {
    if (version.major == 0 && version.minor == 0 && version.patch == 0) {
        return version_check_result::invalid;
    }
    if (version.major != LIBGOSSIP_VERSION_INFO.major) {
        return version_check_result::incompatible_major;
    }
    if (version < LIBGOSSIP_MIN_COMPATIBLE_VERSION) {
        return version_check_result::too_old;
    }
    if (version > LIBGOSSIP_VERSION_INFO) {
        return version_check_result::too_new;
    }
    return version_check_result::compatible;
}

/**
 * @brief Get version check result as string
 */
inline const char* version_check_result_to_string(version_check_result result) {
    switch (result) {
        case version_check_result::compatible:          return "Compatible";
        case version_check_result::incompatible_major:  return "Incompatible major version";
        case version_check_result::too_old:             return "Version too old";
        case version_check_result::too_new:             return "Version too new (future)";
        case version_check_result::invalid:             return "Invalid version";
        default:                                        return "Unknown";
    }
}

} // namespace libgossip

/**
 * @brief Compile-time version check
 */
#define LIBGOSSIP_VERSION_CHECK() \
    static_assert(libgossip::LIBGOSSIP_VERSION_INFO.major == 1, \
                  "libgossip major version mismatch")

/**
 * @brief Ensure the library version is at least a certain version
 */
#define LIBGOSSIP_REQUIRE_VERSION(major, minor, patch) \
    static_assert( \
        (libgossip::LIBGOSSIP_VERSION_INFO.major > (major)) || \
        (libgossip::LIBGOSSIP_VERSION_INFO.major == (major) && \
         libgossip::LIBGOSSIP_VERSION_INFO.minor > (minor)) || \
        (libgossip::LIBGOSSIP_VERSION_INFO.major == (major) && \
         libgossip::LIBGOSSIP_VERSION_INFO.minor == (minor) && \
         libgossip::LIBGOSSIP_VERSION_INFO.patch >= (patch)), \
        "libgossip version requirement not met")

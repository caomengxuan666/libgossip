/**
 * @file enum_reflection.inl
 * @brief Compiling-time reflection for enums
 *
 * This file is included by gossip_core.hpp,and it supports compiling-time reflection for enums.
 * Instead of dynamically querying the enum values at runtime, this file provides a way to query them at compile time.
 * Which means that you can use the enum values in compile time,save memory and improve performance.
 * @author caomengxuan666
 * @date 2025-8-30
 */
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <string_view>
#include <type_traits>// For std::enable_if

namespace libgossip {
    /**
     * @brief Converts an enum value to its corresponding string name
     * @tparam T Enum type (must be an enum)
     * @param value The enum value to convert
     * @return String view containing the enum's name
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr std::string_view enum_to_string(T value) {
        return magic_enum::enum_name(value);
    }

    /**
     * @brief Parses a string to get the corresponding enum value
     * @tparam T Enum type (must be an enum)
     * @param str The string to parse
     * @param case_sensitive Whether the parsing should be case-sensitive
     * @return Optional containing the parsed enum value, or empty if parsing failed
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr std::optional<T> string_to_enum(std::string_view str, bool case_sensitive = true) {
        if (case_sensitive) {
            return magic_enum::enum_cast<T>(str);
        } else {
            return magic_enum::enum_cast<T>(str, magic_enum::case_insensitive);
        }
    }

    /**
     * @brief Retrieves all values of the enum type
     * @tparam T Enum type (must be an enum)
     * @return Array containing all enum values in declaration order
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr auto get_all_enum_values() {
        return magic_enum::enum_values<T>();
    }

    /**
     * @brief Retrieves all names of the enum values
     * @tparam T Enum type (must be an enum)
     * @return Array containing all enum names in declaration order
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr auto get_all_enum_names() {
        return magic_enum::enum_names<T>();
    }

    /**
     * @brief Retrieves all (value, name) pairs of the enum type
     * @tparam T Enum type (must be an enum)
     * @return Array of pairs containing each enum value and its corresponding name
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr auto get_all_enum_entries() {
        return magic_enum::enum_entries<T>();
    }

    /**
     * @brief Gets the number of values in the enum type
     * @tparam T Enum type (must be an enum)
     * @return The count of enum values
     */
    template<typename T,
             typename = std::enable_if_t<magic_enum::detail::is_enum_v<T>>>
    constexpr size_t get_enum_count() {
        return magic_enum::enum_count<T>();
    }
}// namespace libgossip
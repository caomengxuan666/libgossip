/**
 * @file node_id_utils.hpp
 * @brief Utility functions for node ID manipulation
 *
 * Provides functions for converting, parsing, and generating node IDs.
 * Node IDs are 16-byte arrays that can represent UUIDs or other identifiers.
 */

#pragma once

#include "config.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace libgossip {

/// Node ID type (16 bytes)
using node_id_t = std::array<uint8_t, 16>;

/**
 * @brief Convert a node ID to a hex string
 *
 * @param id The node ID to convert
 * @return 32-character lowercase hex string (no dashes)
 */
LIBGOSSIP_API std::string node_id_to_string(const node_id_t& id);

/**
 * @brief Parse a node ID from a hex string
 *
 * @param hex 32-character hex string (with or without dashes)
 * @return The parsed node ID, or std::nullopt if parsing fails
 */
LIBGOSSIP_API std::optional<node_id_t> parse_node_id(std::string_view hex);

/**
 * @brief Generate a random node ID
 *
 * Uses std::random_device for entropy, combined with timestamp
 * to ensure uniqueness even across rapid successive calls.
 *
 * @return A randomly generated node ID
 */
LIBGOSSIP_API node_id_t generate_node_id();

/**
 * @brief Create a node ID from raw bytes
 *
 * @param data Pointer to at least 16 bytes
 * @return The node ID
 */
LIBGOSSIP_API node_id_t node_id_from_bytes(const uint8_t* data);

/**
 * @brief Copy node ID to a byte buffer
 *
 * @param id The node ID
 * @param data Pointer to at least 16 bytes of output buffer
 */
LIBGOSSIP_API void node_id_to_bytes(const node_id_t& id, uint8_t* data);

/**
 * @brief Create a node ID from a 64-bit hash value
 *
 * The hash is placed in the first 8 bytes, remaining bytes are zeroed.
 * Useful for creating temporary IDs from hash values.
 *
 * @param hash The hash value
 * @return The node ID
 */
LIBGOSSIP_API node_id_t node_id_from_hash(uint64_t hash);

/**
 * @brief Get a "null" node ID (all zeros)
 *
 * @return A node ID with all bytes set to zero
 */
constexpr node_id_t null_node_id() {
    return node_id_t{};
}

/**
 * @brief Check if a node ID is null (all zeros)
 *
 * @param id The node ID to check
 * @return true if the ID is null, false otherwise
 */
constexpr bool is_null_node_id(const node_id_t& id) {
    for (uint8_t byte : id) {
        if (byte != 0) return false;
    }
    return true;
}

} // namespace libgossip

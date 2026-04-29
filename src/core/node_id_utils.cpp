/**
 * @file node_id_utils.cpp
 * @brief Implementation of node ID utility functions
 */

#include "core/node_id_utils.hpp"
#include <chrono>
#include <cstring>
#include <random>
#include <sstream>

namespace libgossip {

std::string node_id_to_string(const node_id_t& id) {
    constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.reserve(32);

    for (uint8_t byte : id) {
        result.push_back(hex_chars[byte >> 4]);
        result.push_back(hex_chars[byte & 0x0f]);
    }

    return result;
}

std::optional<node_id_t> parse_node_id(std::string_view hex) {
    // Remove dashes if present (UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)
    std::string clean_hex;
    clean_hex.reserve(32);

    for (char c : hex) {
        if (c != '-') {
            clean_hex.push_back(c);
        }
    }

    if (clean_hex.size() != 32) {
        return std::nullopt;
    }

    node_id_t id{};
    for (size_t i = 0; i < 16; ++i) {
        uint8_t byte = 0;

        for (int j = 0; j < 2; ++j) {
            char c = clean_hex[i * 2 + j];
            byte <<= 4;

            if (c >= '0' && c <= '9') {
                byte |= static_cast<uint8_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                byte |= static_cast<uint8_t>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                byte |= static_cast<uint8_t>(c - 'A' + 10);
            } else {
                return std::nullopt;
            }
        }

        id[i] = byte;
    }

    return id;
}

node_id_t generate_node_id() {
    node_id_t id{};

    // Use random_device for entropy
    std::random_device rd;

    // Combine time-based entropy with random device
    auto now = std::chrono::steady_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();

    // Fill with random bytes
    std::mt19937_64 gen(rd());
    uint64_t rand1 = gen();
    uint64_t rand2 = gen();

    // Mix in timestamp for uniqueness across rapid calls
    rand1 ^= static_cast<uint64_t>(nanos);

    std::memcpy(id.data(), &rand1, 8);
    std::memcpy(id.data() + 8, &rand2, 8);

    return id;
}

node_id_t node_id_from_bytes(const uint8_t* data) {
    node_id_t id{};
    std::memcpy(id.data(), data, 16);
    return id;
}

void node_id_to_bytes(const node_id_t& id, uint8_t* data) {
    std::memcpy(data, id.data(), 16);
}

node_id_t node_id_from_hash(uint64_t hash) {
    node_id_t id{};
    std::memcpy(id.data(), &hash, 8);
    return id;
}

} // namespace libgossip

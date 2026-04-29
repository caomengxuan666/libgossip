/**
 * @file message_serializer.hpp
 * @brief Abstract interface for message serialization
 *
 * This header defines the message_serializer interface that allows users
 * to plug in their own serialization format (JSON, Protobuf, FlatBuffers,
 * MessagePack, etc.) without modifying the library.
 *
 * The library ships with a default JSON serializer in the network module,
 * but users can register custom serializers via the serializer_factory.
 */

#pragma once

#include "config.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Forward declare to avoid circular dependency with gossip_core.hpp
namespace libgossip {
    struct gossip_message;
}

namespace libgossip {

/**
 * @brief Error codes for serialization operations
 */
enum class serialization_error {
    success = 0,
    invalid_input,
    serialization_failed,
    deserialization_failed,
    unsupported_format
};

/**
 * @brief Abstract interface for message serialization
 *
 * Implement this interface to provide custom serialization formats.
 * The serializer is used by the network transport layer to convert
 * gossip messages to/from byte arrays for network transmission.
 *
 * @note Implementations must be thread-safe if the serializer will be
 *       shared across multiple transport instances.
 */
class LIBGOSSIP_API message_serializer {
public:
    virtual ~message_serializer() = default;

    /**
     * @brief Get the name of this serializer
     *
     * Used for registration and lookup in the serializer_factory.
     * Examples: "json", "protobuf", "flatbuffers", "msgpack"
     */
    virtual std::string name() const = 0;

    /**
     * @brief Serialize a gossip message to byte array
     *
     * @param msg The message to serialize
     * @param data Output byte array containing serialized data
     * @return Error code indicating success or failure
     */
    virtual serialization_error serialize(const gossip_message& msg,
                                          std::vector<uint8_t>& data) const = 0;

    /**
     * @brief Deserialize a byte array to gossip message
     *
     * @param data Input byte array containing serialized data
     * @param msg Output message
     * @return Error code indicating success or failure
     */
    virtual serialization_error deserialize(const std::vector<uint8_t>& data,
                                            gossip_message& msg) const = 0;
};

} // namespace libgossip

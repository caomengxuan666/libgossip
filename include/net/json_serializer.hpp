/**
 * @file json_serializer.hpp
 * @brief JSON serializer implementation for gossip messages using nlohmann/json
 *
 * This file contains the JSON serializer implementation for gossip messages.
 * It provides serialization and deserialization of gossip messages to/from
 * JSON format for network transmission using the nlohmann/json library.
 *
 * @author caomengxuan666
 * @date 2025-08-30
 */

#pragma once

#include "core/gossip_core.hpp"
#include "core/message_serializer.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>

namespace libgossip {

    /**
     * @brief JSON serializer implementation using nlohmann/json
     *
     * This class implements the message_serializer interface using JSON as
     * the serialization format. It provides methods to serialize gossip
     * messages to JSON byte arrays and deserialize JSON byte arrays back
     * to gossip messages.
     */
    class LIBGOSSIP_API json_serializer : public message_serializer {
    public:
        /**
         * @brief Get the name of this serializer
         * @return "json"
         */
        std::string name() const override { return "json"; }

        /**
         * @brief Serialize a gossip message to JSON byte array
         * @param msg The message to serialize
         * @param data Output byte array containing JSON representation
         * @return Error code indicating success or failure
         */
        serialization_error serialize(const gossip_message &msg, std::vector<uint8_t> &data) const override;

        /**
         * @brief Deserialize a JSON byte array to gossip message
         * @param data Input byte array containing JSON representation
         * @param msg Output message
         * @return Error code indicating success or failure
         */
        serialization_error deserialize(const std::vector<uint8_t> &data, gossip_message &msg) const override;

    private:
        nlohmann::json serialize_node_to_json(const node_view &node) const;
        node_view deserialize_node_from_json(const nlohmann::json &j) const;
        void parse_node_id_from_hex(const std::string &hex_str, node_id_t &node_id) const;
    };

} // namespace libgossip

// Keep backward compatibility alias in gossip::net namespace
namespace gossip {
    namespace net {
        using json_serializer = libgossip::json_serializer;
        using message_serializer = libgossip::message_serializer;
    } // namespace net
} // namespace gossip

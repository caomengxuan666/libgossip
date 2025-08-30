/**
 * @file json_serializer.hpp
 * @brief JSON serializer implementation for gossip messages
 * 
 * This file contains the JSON serializer implementation for gossip messages.
 * It provides serialization and deserialization of gossip messages to/from
 * JSON format for network transmission.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#ifndef LIBGOSSIP_JSON_SERIALIZER_HPP
#define LIBGOSSIP_JSON_SERIALIZER_HPP

#include "udp_transport.hpp"
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace gossip {
    namespace net {

        /**
         * @brief JSON serializer implementation
         * 
         * This class implements the message_serializer interface using JSON as
         * the serialization format. It provides methods to serialize gossip
         * messages to JSON byte arrays and deserialize JSON byte arrays back
         * to gossip messages.
         */
        class LIBGOSSIP_API json_serializer : public message_serializer {
        public:
            /**
             * @brief Serialize a gossip message to JSON byte array
             * @param msg The message to serialize
             * @param data Output byte array containing JSON representation
             * @return Error code indicating success or failure
             */
            error_code serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const override;

            /**
             * @brief Deserialize a JSON byte array to gossip message
             * @param data Input byte array containing JSON representation
             * @param msg Output message
             * @return Error code indicating success or failure
             */
            error_code deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const override;

        private:
            /**
             * @brief Serialize a node view to JSON
             * @param oss Output string stream
             * @param node The node to serialize
             */
            void serialize_node(std::ostringstream &oss, const libgossip::node_view &node) const;

            /**
             * @brief Parse entries from JSON string
             * @param entries_str Input JSON string containing entries
             * @param entries Output vector of node views
             */
            void parse_entries(const std::string &entries_str, std::vector<libgossip::node_view> &entries) const;

            /**
             * @brief Find matching closing brace for an opening brace
             * @param str Input string
             * @param start_pos Position of opening brace
             * @return Position of matching closing brace, or std::string::npos if not found
             */
            size_t find_matching_brace(const std::string &str, size_t start_pos) const;

            /**
             * @brief Parse a node view from JSON string
             * @param node_str Input JSON string
             * @param node Output node view
             */
            void parse_node(const std::string &node_str, libgossip::node_view &node) const;

            /**
             * @brief Parse metadata from JSON string
             * @param metadata_str Input JSON string containing metadata
             * @param metadata Output metadata map
             */
            void parse_metadata(const std::string &metadata_str, std::map<std::string, std::string> &metadata) const;

            /**
             * @brief Parse a node ID from hex string
             * @param hex_str Input hex string
             * @param node_id Output node ID
             */
            void parse_node_id(const std::string &hex_str, libgossip::node_id_t &node_id) const;
            
            /**
             * @brief Escape special characters in a string for JSON
             * @param str Input string
             * @return Escaped string
             */
            std::string escape_json_string(const std::string& str) const;
            
            /**
             * @brief Unescape special characters in a JSON string
             * @param str Input escaped string
             * @return Unescaped string
             */
            std::string unescape_json_string(const std::string& str) const;
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_JSON_SERIALIZER_HPP
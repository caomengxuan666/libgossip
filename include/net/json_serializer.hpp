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
#include <iomanip>
#include <sstream>

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
            error_code serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const override {
                try {
                    std::ostringstream oss;
                    oss << "{";

                    // Serialize sender
                    oss << "\"sender\":\"";
                    for (size_t i = 0; i < msg.sender.size(); ++i) {
                        if (i > 0) {
                            oss << ",";
                        }
                        oss << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(msg.sender[i]);
                    }
                    oss << "\",";

                    // Serialize type and timestamp
                    oss << "\"type\":" << static_cast<int>(msg.type) << ",";
                    oss << "\"timestamp\":" << msg.timestamp << ",";

                    // Serialize entries
                    oss << "\"entries\":[";
                    for (size_t i = 0; i < msg.entries.size(); ++i) {
                        if (i > 0) {
                            oss << ",";
                        }
                        serialize_node(oss, msg.entries[i]);
                    }
                    oss << "]";

                    oss << "}";

                    std::string json_str = oss.str();
                    data.assign(json_str.begin(), json_str.end());
                    return error_code::success;
                } catch (...) {
                    return error_code::serialization_error;
                }
            }

            /**
         * @brief Deserialize a JSON byte array to gossip message
         * @param data Input byte array containing JSON representation
         * @param msg Output message
         * @return Error code indicating success or failure
         */
            error_code deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const override {
                try {
                    if (data.empty()) {
                        return error_code::success;// Nothing to deserialize
                    }

                    std::string json_str(data.begin(), data.end());

                    // Simple JSON parsing - in a production implementation,
                    // you would use a proper JSON library like nlohmann/json
                    // This is a basic implementation for demonstration purposes

                    // Parse sender
                    size_t sender_pos = json_str.find("\"sender\":\"");
                    if (sender_pos != std::string::npos) {
                        sender_pos += 10;// Length of "\"sender\":\""
                        size_t end_pos = json_str.find("\"", sender_pos);
                        if (end_pos != std::string::npos) {
                            std::string sender_str = json_str.substr(sender_pos, end_pos - sender_pos);
                            parse_node_id(sender_str, msg.sender);
                        }
                    }

                    // Parse type
                    size_t type_pos = json_str.find("\"type\":");
                    if (type_pos != std::string::npos) {
                        type_pos += 7;// Length of "\"type\":"
                        size_t end_pos = json_str.find(",", type_pos);
                        if (end_pos == std::string::npos) {
                            end_pos = json_str.find("}", type_pos);
                        }
                        if (end_pos != std::string::npos) {
                            std::string type_str = json_str.substr(type_pos, end_pos - type_pos);
                            msg.type = static_cast<libgossip::message_type>(std::stoi(type_str));
                        }
                    }

                    // Parse timestamp
                    size_t timestamp_pos = json_str.find("\"timestamp\":");
                    if (timestamp_pos != std::string::npos) {
                        timestamp_pos += 12;// Length of "\"timestamp\":"
                        size_t end_pos = json_str.find(",", timestamp_pos);
                        if (end_pos == std::string::npos) {
                            end_pos = json_str.find("}", timestamp_pos);
                        }
                        if (end_pos != std::string::npos) {
                            std::string timestamp_str = json_str.substr(timestamp_pos, end_pos - timestamp_pos);
                            msg.timestamp = std::stoull(timestamp_str);
                        }
                    }

                    // Parse entries (simplified)
                    size_t entries_pos = json_str.find("\"entries\":[");
                    if (entries_pos != std::string::npos) {
                        entries_pos += 11;// Length of "\"entries\":["
                        // In a full implementation, we would parse all entries here
                        // For now, we'll leave the entries vector empty
                        msg.entries.clear();
                    }

                    return error_code::success;
                } catch (...) {
                    return error_code::serialization_error;
                }
            }

        private:
            /**
         * @brief Serialize a node view to JSON
         * @param oss Output string stream
         * @param node The node to serialize
         */
            void serialize_node(std::ostringstream &oss, const libgossip::node_view &node) const {
                oss << "{";

                // Serialize ID
                oss << "\"id\":\"";
                for (size_t i = 0; i < node.id.size(); ++i) {
                    if (i > 0) {
                        oss << ",";
                    }
                    oss << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(node.id[i]);
                }
                oss << "\",";

                // Serialize basic fields
                oss << "\"ip\":\"" << node.ip << "\",";
                oss << "\"port\":" << node.port << ",";
                oss << "\"config_epoch\":" << node.config_epoch << ",";
                oss << "\"heartbeat\":" << node.heartbeat << ",";
                oss << "\"version\":" << node.version << ",";
                oss << "\"status\":" << static_cast<int>(node.status) << ",";

                // Serialize role and region
                oss << "\"role\":\"" << node.role << "\",";
                oss << "\"region\":\"" << node.region << "\",";

                // Serialize metadata
                oss << "\"metadata\":{";
                size_t meta_count = 0;
                for (const auto &pair: node.metadata) {
                    if (meta_count > 0) {
                        oss << ",";
                    }
                    oss << "\"" << pair.first << "\":\"" << pair.second << "\"";
                    meta_count++;
                }
                oss << "}";

                oss << "}";
            }

            /**
         * @brief Parse a node ID from hex string
         * @param hex_str Input hex string
         * @param node_id Output node ID
         */
            void parse_node_id(const std::string &hex_str, libgossip::node_id_t &node_id) const {
                std::vector<std::string> bytes;
                size_t start = 0;
                size_t pos = 0;

                // Split by comma
                while ((pos = hex_str.find(",", start)) != std::string::npos) {
                    bytes.push_back(hex_str.substr(start, pos - start));
                    start = pos + 1;
                }
                bytes.push_back(hex_str.substr(start));

                // Convert to bytes
                for (size_t i = 0; i < bytes.size() && i < node_id.size(); ++i) {
                    node_id[i] = static_cast<uint8_t>(std::stoi(bytes[i], nullptr, 16));
                }
            }
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_JSON_SERIALIZER_HPP
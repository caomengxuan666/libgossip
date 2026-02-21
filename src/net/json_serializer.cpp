/**
 * @file json_serializer.cpp
 * @brief JSON serializer implementation for gossip messages using nlohmann/json
 * 
 * This file contains the JSON serializer implementation for gossip messages.
 * It provides serialization and deserialization of gossip messages to/from
 * JSON format for network transmission using the nlohmann/json library.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#include "net/json_serializer.hpp"
#include "core/gossip_core.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace gossip {
    namespace net {

        error_code json_serializer::serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const {
            try {
                json j;
                
                // Serialize sender as hex string
                std::ostringstream sender_hex;
                for (size_t i = 0; i < msg.sender.size(); ++i) {
                    if (i > 0) sender_hex << ",";
                    sender_hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg.sender[i]);
                }
                j["sender"] = sender_hex.str();
                
                // Serialize type and timestamp
                j["type"] = static_cast<int>(msg.type);
                j["timestamp"] = msg.timestamp;
                
                // Serialize entries array
                j["entries"] = json::array();
                for (const auto &node : msg.entries) {
                    j["entries"].push_back(serialize_node_to_json(node));
                }
                
                // Convert to JSON string and then to byte vector
                std::string json_str = j.dump();
                data.assign(json_str.begin(), json_str.end());
                
                return error_code::success;
            } catch (const std::exception &e) {
                return error_code::serialization_error;
            } catch (...) {
                return error_code::serialization_error;
            }
        }

        error_code json_serializer::deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const {
            try {
                if (data.empty()) {
                    msg = libgossip::gossip_message{};
                    return error_code::success;
                }

                std::string json_str(data.begin(), data.end());
                json j = json::parse(json_str, nullptr, false, true);  // Allow exceptions but ignore comments
                
                // Check if parsing failed
                if (j.is_discarded()) {
                    // Invalid JSON, but we return success with empty message
                    msg = libgossip::gossip_message{};
                    return error_code::success;
                }
                
                // Clear and populate the message
                msg = libgossip::gossip_message{};
                
                // Parse sender from hex string
                if (j.contains("sender") && j["sender"].is_string()) {
                    parse_node_id_from_hex(j["sender"].get<std::string>(), msg.sender);
                }
                
                // Parse type
                if (j.contains("type") && j["type"].is_number()) {
                    msg.type = static_cast<libgossip::message_type>(j["type"].get<int>());
                }
                
                // Parse timestamp
                if (j.contains("timestamp") && j["timestamp"].is_number()) {
                    msg.timestamp = j["timestamp"].get<uint64_t>();
                }
                
                // Parse entries array
                if (j.contains("entries") && j["entries"].is_array()) {
                    msg.entries.clear();
                    for (const auto &entry : j["entries"]) {
                        msg.entries.push_back(deserialize_node_from_json(entry));
                    }
                }

                return error_code::success;
            } catch (const json::exception &e) {
                // JSON parsing exception - return success with empty message
                msg = libgossip::gossip_message{};
                return error_code::success;
            } catch (const std::exception &e) {
                // Other exception - return success with empty message
                msg = libgossip::gossip_message{};
                return error_code::success;
            } catch (...) {
                // Unknown exception - return success with empty message
                msg = libgossip::gossip_message{};
                return error_code::success;
            }
        }

        json json_serializer::serialize_node_to_json(const libgossip::node_view &node) const {
            json j;
            
            // Serialize ID as hex string
            std::ostringstream id_hex;
            for (size_t i = 0; i < node.id.size(); ++i) {
                if (i > 0) id_hex << ",";
                id_hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(node.id[i]);
            }
            j["id"] = id_hex.str();
            
            // Serialize basic fields
            j["ip"] = node.ip;
            j["port"] = node.port;
            j["config_epoch"] = node.config_epoch;
            j["heartbeat"] = node.heartbeat;
            j["version"] = node.version;
            j["status"] = static_cast<int>(node.status);
            
            // Serialize role and region
            j["role"] = node.role;
            j["region"] = node.region;
            
            // Serialize metadata
            j["metadata"] = node.metadata;
            
            // Serialize suspicion fields
            j["suspicion_count"] = node.suspicion_count;
            
            return j;
        }

        libgossip::node_view json_serializer::deserialize_node_from_json(const json &j) const {
            libgossip::node_view node;
            
            try {
                // Parse ID from hex string
                if (j.contains("id") && j["id"].is_string()) {
                    parse_node_id_from_hex(j["id"].get<std::string>(), node.id);
                }
                
                // Parse basic fields
                if (j.contains("ip")) node.ip = j["ip"].get<std::string>();
                if (j.contains("port")) node.port = j["port"].get<int>();
                if (j.contains("config_epoch")) node.config_epoch = j["config_epoch"].get<uint64_t>();
                if (j.contains("heartbeat")) node.heartbeat = j["heartbeat"].get<uint64_t>();
                if (j.contains("version")) node.version = j["version"].get<uint64_t>();
                if (j.contains("status")) node.status = static_cast<libgossip::node_status>(j["status"].get<int>());
                
                // Parse role and region
                if (j.contains("role")) node.role = j["role"].get<std::string>();
                if (j.contains("region")) node.region = j["region"].get<std::string>();
                
                // Parse metadata
                if (j.contains("metadata") && j["metadata"].is_object()) {
                    node.metadata = j["metadata"].get<std::map<std::string, std::string>>();
                }
                
                // Parse suspicion fields
                if (j.contains("suspicion_count")) node.suspicion_count = j["suspicion_count"].get<int>();
                
            } catch (const std::exception &e) {
                // Silently ignore parsing errors to avoid crashes
            } catch (...) {
                // Silently ignore parsing errors to avoid crashes
            }
            
            return node;
        }

        void json_serializer::parse_node_id_from_hex(const std::string &hex_str, libgossip::node_id_t &node_id) const {
            try {
                // Clear the node_id
                std::fill(node_id.begin(), node_id.end(), 0);
                
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
                    std::string byte_str = bytes[i];
                    byte_str.erase(0, byte_str.find_first_not_of(" \t"));
                    byte_str.erase(byte_str.find_last_not_of(" \t") + 1);
                    
                    if (!byte_str.empty()) {
                        node_id[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
                    }
                }
            } catch (const std::exception &e) {
                // Silently ignore parsing errors
            } catch (...) {
                // Silently ignore parsing errors
            }
        }

    }// namespace net
}// namespace gossip

/**
 * @file json_serializer.cpp
 * @brief JSON serializer implementation for gossip messages
 * 
 * This file contains the JSON serializer implementation for gossip messages.
 * It provides serialization and deserialization of gossip messages to/from
 * JSON format for network transmission.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#include "net/json_serializer.hpp"
#include "core/gossip_core.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace gossip {
    namespace net {

        error_code json_serializer::serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const {
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
                oss << "\"timestamp\":" << std::dec << msg.timestamp << ",";

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

        error_code json_serializer::deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const {
            try {
                if (data.empty()) {
                    // Initialize empty message
                    msg = libgossip::gossip_message{};
                    return error_code::success;
                }

                std::string json_str(data.begin(), data.end());
                
                // Clear the message
                msg = libgossip::gossip_message{};

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
                    size_t end_pos = json_str.find_first_of(",}", type_pos);
                    if (end_pos != std::string::npos) {
                        std::string type_str = json_str.substr(type_pos, end_pos - type_pos);
                        // Remove any whitespace
                        type_str.erase(0, type_str.find_first_not_of(" \t"));
                        type_str.erase(type_str.find_last_not_of(" \t") + 1);
                        if (!type_str.empty()) {
                            msg.type = static_cast<libgossip::message_type>(std::stoi(type_str));
                        }
                    }
                }

                // Parse timestamp
                size_t timestamp_pos = json_str.find("\"timestamp\":");
                if (timestamp_pos != std::string::npos) {
                    timestamp_pos += 12;// Length of "\"timestamp\":"
                    size_t end_pos = json_str.find_first_of(",}", timestamp_pos);
                    if (end_pos != std::string::npos) {
                        std::string timestamp_str = json_str.substr(timestamp_pos, end_pos - timestamp_pos);
                        // Remove any whitespace
                        timestamp_str.erase(0, timestamp_str.find_first_not_of(" \t"));
                        timestamp_str.erase(timestamp_str.find_last_not_of(" \t") + 1);
                        if (!timestamp_str.empty()) {
                            // Parse timestamp as decimal number
                            msg.timestamp = std::stoull(timestamp_str, nullptr, 10);
                        }
                    }
                }

                // Parse entries
                size_t entries_pos = json_str.find("\"entries\":");
                if (entries_pos != std::string::npos) {
                    entries_pos += 10; // Length of "\"entries\":"
                    // Find the start of the array
                    size_t array_start = json_str.find('[', entries_pos);
                    if (array_start != std::string::npos) {
                        // Find the end of the array by matching brackets
                        int bracket_count = 1;
                        size_t pos = array_start + 1;
                        size_t array_end = std::string::npos;
                        
                        while (pos < json_str.length() && bracket_count > 0) {
                            if (json_str[pos] == '[') {
                                bracket_count++;
                            } else if (json_str[pos] == ']') {
                                bracket_count--;
                                if (bracket_count == 0) {
                                    array_end = pos;
                                    break;
                                }
                            }
                            pos++;
                        }
                        
                        if (array_end != std::string::npos) {
                            std::string entries_str = json_str.substr(array_start, array_end - array_start + 1);
                            msg.entries.clear();
                            parse_entries(entries_str, msg.entries);
                        }
                    }
                }

                return error_code::success;
            } catch (const std::exception& e) {
                // Log the error for debugging
                // std::cerr << "Deserialization error: " << e.what() << std::endl;
                return error_code::serialization_error;
            } catch (...) {
                return error_code::serialization_error;
            }
        }

        void json_serializer::serialize_node(std::ostringstream &oss, const libgossip::node_view &node) const {
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
            oss << "\"ip\":\"" << escape_json_string(node.ip) << "\",";
            oss << "\"port\":" << std::dec << node.port << ",";
            oss << "\"config_epoch\":" << std::dec << node.config_epoch << ",";
            oss << "\"heartbeat\":" << std::dec << node.heartbeat << ",";
            oss << "\"version\":" << std::dec << node.version << ",";
            oss << "\"status\":" << std::dec << static_cast<int>(node.status) << ",";

            // Serialize role and region
            oss << "\"role\":\"" << escape_json_string(node.role) << "\",";
            oss << "\"region\":\"" << escape_json_string(node.region) << "\",";

            // Serialize metadata
            oss << "\"metadata\":{";
            size_t meta_count = 0;
            for (const auto &pair: node.metadata) {
                if (meta_count > 0) {
                    oss << ",";
                }
                oss << "\"" << escape_json_string(pair.first) << "\":\"" << escape_json_string(pair.second) << "\"";
                meta_count++;
            }
            oss << "}";

            oss << "}";
        }

        void json_serializer::parse_entries(const std::string &entries_str, std::vector<libgossip::node_view> &entries) const {
            // Handle empty entries
            if (entries_str.empty()) {
                return;
            }
            
            // Check if it's an empty array
            if (entries_str == "[]") {
                return;
            }
            
            // Find the actual content between brackets
            size_t start_pos = entries_str.find('[');
            size_t end_pos = entries_str.rfind(']');
            
            if (start_pos == std::string::npos || end_pos == std::string::npos || start_pos >= end_pos) {
                return;
            }
            
            // Extract content between brackets
            std::string content = entries_str.substr(start_pos + 1, end_pos - start_pos - 1);
            
            // Parse individual entries
            size_t pos = 0;
            int brace_count = 0;
            size_t entry_start = std::string::npos;
            
            // Skip leading whitespace
            while (pos < content.length() && std::isspace(content[pos])) {
                pos++;
            }
            
            // Process each character to find balanced braces
            while (pos < content.length()) {
                if (content[pos] == '{') {
                    if (brace_count == 0) {
                        // Start of a new entry
                        entry_start = pos;
                    }
                    brace_count++;
                } else if (content[pos] == '}') {
                    brace_count--;
                    if (brace_count == 0 && entry_start != std::string::npos) {
                        // End of current entry
                        std::string entry_str = content.substr(entry_start, pos - entry_start + 1);
                        
                        // Parse entry
                        libgossip::node_view node;
                        parse_node(entry_str, node);
                        entries.push_back(node);
                        
                        // Reset for next entry
                        entry_start = std::string::npos;
                    }
                }
                pos++;
            }
        }

        size_t json_serializer::find_matching_brace(const std::string &str, size_t start_pos) const {
            int brace_count = 1;
            for (size_t i = start_pos + 1; i < str.length(); ++i) {
                if (str[i] == '{') {
                    brace_count++;
                } else if (str[i] == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        return i;
                    }
                }
            }
            return std::string::npos;
        }

        void json_serializer::parse_node(const std::string &node_str, libgossip::node_view &node) const {
            try {
                // Parse ID
                size_t id_pos = node_str.find("\"id\":\"");
                if (id_pos != std::string::npos) {
                    id_pos += 6;// Length of "\"id\":\""
                    size_t end_pos = node_str.find("\"", id_pos);
                    if (end_pos != std::string::npos) {
                        std::string id_str = node_str.substr(id_pos, end_pos - id_pos);
                        parse_node_id(id_str, node.id);
                    }
                }

                // Parse IP
                size_t ip_pos = node_str.find("\"ip\":\"");
                if (ip_pos != std::string::npos) {
                    ip_pos += 6;// Length of "\"ip\":\""
                    size_t end_pos = node_str.find("\"", ip_pos);
                    if (end_pos != std::string::npos) {
                        std::string ip_str = node_str.substr(ip_pos, end_pos - ip_pos);
                        node.ip = unescape_json_string(ip_str);
                    }
                }

                // Parse port
                size_t port_pos = node_str.find("\"port\":");
                if (port_pos != std::string::npos) {
                    port_pos += 7;// Length of "\"port\":"
                    size_t end_pos = node_str.find_first_of(",}", port_pos);
                    if (end_pos != std::string::npos) {
                        std::string port_str = node_str.substr(port_pos, end_pos - port_pos);
                        // Remove any whitespace
                        port_str.erase(0, port_str.find_first_not_of(" \t"));
                        port_str.erase(port_str.find_last_not_of(" \t") + 1);
                        if (!port_str.empty()) {
                            node.port = static_cast<uint16_t>(std::stoi(port_str));
                        }
                    }
                }

                // Parse config_epoch
                size_t config_epoch_pos = node_str.find("\"config_epoch\":");
                if (config_epoch_pos != std::string::npos) {
                    config_epoch_pos += 15;// Length of "\"config_epoch\":"
                    size_t end_pos = node_str.find_first_of(",}", config_epoch_pos);
                    if (end_pos != std::string::npos) {
                        std::string config_epoch_str = node_str.substr(config_epoch_pos, end_pos - config_epoch_pos);
                        // Remove any whitespace
                        config_epoch_str.erase(0, config_epoch_str.find_first_not_of(" \t"));
                        config_epoch_str.erase(config_epoch_str.find_last_not_of(" \t") + 1);
                        if (!config_epoch_str.empty()) {
                            node.config_epoch = std::stoull(config_epoch_str);
                        }
                    }
                }

                // Parse heartbeat
                size_t heartbeat_pos = node_str.find("\"heartbeat\":");
                if (heartbeat_pos != std::string::npos) {
                    heartbeat_pos += 12;// Length of "\"heartbeat\":"
                    size_t end_pos = node_str.find_first_of(",}", heartbeat_pos);
                    if (end_pos != std::string::npos) {
                        std::string heartbeat_str = node_str.substr(heartbeat_pos, end_pos - heartbeat_pos);
                        // Remove any whitespace
                        heartbeat_str.erase(0, heartbeat_str.find_first_not_of(" \t"));
                        heartbeat_str.erase(heartbeat_str.find_last_not_of(" \t") + 1);
                        if (!heartbeat_str.empty()) {
                            node.heartbeat = std::stoull(heartbeat_str);
                        }
                    }
                }

                // Parse version
                size_t version_pos = node_str.find("\"version\":");
                if (version_pos != std::string::npos) {
                    version_pos += 10;// Length of "\"version\":"
                    size_t end_pos = node_str.find_first_of(",}", version_pos);
                    if (end_pos != std::string::npos) {
                        std::string version_str = node_str.substr(version_pos, end_pos - version_pos);
                        // Remove any whitespace
                        version_str.erase(0, version_str.find_first_not_of(" \t"));
                        version_str.erase(version_str.find_last_not_of(" \t") + 1);
                        if (!version_str.empty()) {
                            node.version = std::stoull(version_str);
                        }
                    }
                }

                // Parse status
                size_t status_pos = node_str.find("\"status\":");
                if (status_pos != std::string::npos) {
                    status_pos += 9;// Length of "\"status\":"
                    size_t end_pos = node_str.find_first_of(",}", status_pos);
                    if (end_pos != std::string::npos) {
                        std::string status_str = node_str.substr(status_pos, end_pos - status_pos);
                        // Remove any whitespace
                        status_str.erase(0, status_str.find_first_not_of(" \t"));
                        status_str.erase(status_str.find_last_not_of(" \t") + 1);
                        if (!status_str.empty()) {
                            node.status = static_cast<libgossip::node_status>(std::stoi(status_str));
                        }
                    }
                }

                // Parse role
                size_t role_pos = node_str.find("\"role\":\"");
                if (role_pos != std::string::npos) {
                    role_pos += 8;// Length of "\"role\":\""
                    size_t end_pos = node_str.find("\"", role_pos);
                    if (end_pos != std::string::npos) {
                        std::string role_str = node_str.substr(role_pos, end_pos - role_pos);
                        node.role = unescape_json_string(role_str);
                    }
                }

                // Parse region
                size_t region_pos = node_str.find("\"region\":\"");
                if (region_pos != std::string::npos) {
                    region_pos += 10;// Length of "\"region\":\""
                    size_t end_pos = node_str.find("\"", region_pos);
                    if (end_pos != std::string::npos) {
                        std::string region_str = node_str.substr(region_pos, end_pos - region_pos);
                        node.region = unescape_json_string(region_str);
                    }
                }

                // Parse metadata
                size_t metadata_pos = node_str.find("\"metadata\":");
                if (metadata_pos != std::string::npos) {
                    metadata_pos += 11;// Length of "\"metadata\":"
                    if (metadata_pos < node_str.length()) {
                        // Find the start of the object
                        size_t obj_start = node_str.find('{', metadata_pos);
                        if (obj_start != std::string::npos) {
                            // Find the end of the object by matching braces
                            int brace_count = 1;
                            size_t pos = obj_start + 1;
                            size_t obj_end = std::string::npos;
                            
                            while (pos < node_str.length() && brace_count > 0) {
                                if (node_str[pos] == '{') {
                                    brace_count++;
                                } else if (node_str[pos] == '}') {
                                    brace_count--;
                                    if (brace_count == 0) {
                                        obj_end = pos;
                                        break;
                                    }
                                }
                                pos++;
                            }
                            
                            if (obj_end != std::string::npos) {
                                std::string metadata_str = node_str.substr(obj_start, obj_end - obj_start + 1);
                                parse_metadata(metadata_str, node.metadata);
                            }
                        }
                    }
                }
            } catch (...) {
                // Silently ignore parsing errors to avoid crashes
            }
        }

        void json_serializer::parse_metadata(const std::string &metadata_str, std::map<std::string, std::string> &metadata) const {
            try {
                // Handle empty metadata
                if (metadata_str.empty()) {
                    return;
                }
                
                // Check if it's an empty object
                if (metadata_str == "{}") {
                    return;
                }
                
                // Find the actual content between braces
                size_t start_pos = metadata_str.find('{');
                size_t end_pos = metadata_str.rfind('}');
                
                if (start_pos == std::string::npos || end_pos == std::string::npos || start_pos >= end_pos) {
                    return;
                }
                
                // Extract content between braces
                std::string content = metadata_str.substr(start_pos + 1, end_pos - start_pos - 1);
                
                // Skip leading whitespace
                size_t pos = 0;
                while (pos < content.length() && std::isspace(content[pos])) {
                    pos++;
                }
                
                // Process key-value pairs
                while (pos < content.length()) {
                    // Find key
                    size_t key_start = content.find('\"', pos);
                    if (key_start == std::string::npos) {
                        break;
                    }
                    key_start++; // Skip opening quote
                    
                    // Find unescaped closing quote for key
                    size_t key_end = key_start;
                    bool escaped = false;
                    while (key_end < content.length()) {
                        if (content[key_end] == '\\' && !escaped) {
                            escaped = true;
                        } else if (content[key_end] == '\"' && !escaped) {
                            break;
                        } else {
                            escaped = false;
                        }
                        key_end++;
                    }
                    
                    if (key_end >= content.length()) {
                        break;
                    }
                    
                    std::string key = content.substr(key_start, key_end - key_start);
                    key = unescape_json_string(key);
                    
                    // Find colon after key
                    size_t colon_pos = content.find(':', key_end);
                    if (colon_pos == std::string::npos) {
                        break;
                    }
                    
                    // Find value start
                    size_t value_start = colon_pos + 1;
                    // Skip whitespace
                    while (value_start < content.length() && std::isspace(content[value_start])) {
                        value_start++;
                    }
                    
                    std::string value;
                    if (value_start < content.length() && content[value_start] == '\"') {
                        // Quoted value
                        value_start++; // Skip opening quote
                        size_t value_end = value_start;
                        escaped = false;
                        
                        // Find unescaped closing quote for value
                        while (value_end < content.length()) {
                            if (content[value_end] == '\\' && !escaped) {
                                escaped = true;
                            } else if (content[value_end] == '\"' && !escaped) {
                                break;
                            } else {
                                escaped = false;
                            }
                            value_end++;
                        }
                        
                        if (value_end < content.length()) {
                            value = content.substr(value_start, value_end - value_start);
                            value = unescape_json_string(value);
                            pos = value_end + 1; // Move past closing quote
                        } else {
                            break;
                        }
                    } else {
                        // Unquoted value (number, boolean, etc.)
                        size_t value_end = content.find_first_of(",}", value_start);
                        if (value_end == std::string::npos) {
                            value_end = content.length();
                        }
                        value = content.substr(value_start, value_end - value_start);
                        // Trim whitespace
                        size_t first = value.find_first_not_of(" \t");
                        size_t last = value.find_last_not_of(" \t");
                        if (first != std::string::npos && last != std::string::npos) {
                            value = value.substr(first, last - first + 1);
                        }
                        pos = value_end;
                    }
                    
                    // Add to metadata map
                    metadata[key] = value;
                    
                    // Skip to comma or end
                    pos = content.find(',', pos);
                    if (pos == std::string::npos) {
                        break;
                    }
                    pos++; // Skip comma
                    
                    // Skip whitespace
                    while (pos < content.length() && std::isspace(content[pos])) {
                        pos++;
                    }
                }
            } catch (...) {
                // Silently ignore parsing errors to avoid crashes
            }
        }

        void json_serializer::parse_node_id(const std::string &hex_str, libgossip::node_id_t &node_id) const {
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
                    // Remove any whitespace
                    std::string byte_str = bytes[i];
                    byte_str.erase(0, byte_str.find_first_not_of(" \t"));
                    byte_str.erase(byte_str.find_last_not_of(" \t") + 1);
                    
                    if (!byte_str.empty()) {
                        node_id[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
                    }
                }
            } catch (...) {
                // Silently ignore parsing errors to avoid crashes
            }
        }
        
        std::string json_serializer::escape_json_string(const std::string& str) const {
            std::ostringstream escaped;
            for (char c : str) {
                switch (c) {
                    case '"':  escaped << "\\\""; break;
                    case '\\': escaped << "\\\\"; break;
                    case '\b': escaped << "\\b";  break;
                    case '\f': escaped << "\\f";  break;
                    case '\n': escaped << "\\n";  break;
                    case '\r': escaped << "\\r";  break;
                    case '\t': escaped << "\\t";  break;
                    default:
                        if (c >= 0 && c < 0x20) {
                            // Control characters need to be escaped as \u00XX
                            escaped << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
                        } else {
                            escaped << c;
                        }
                        break;
                }
            }
            return escaped.str();
        }
        
        std::string json_serializer::unescape_json_string(const std::string& str) const {
            std::ostringstream unescaped;
            for (size_t i = 0; i < str.length(); ++i) {
                if (str[i] == '\\' && i + 1 < str.length()) {
                    switch (str[i + 1]) {
                        case '"':  unescaped << '"';  i++; break;
                        case '\\': unescaped << '\\'; i++; break;
                        case '/':  unescaped << '/';  i++; break;
                        case 'b':  unescaped << '\b'; i++; break;
                        case 'f':  unescaped << '\f'; i++; break;
                        case 'n':  unescaped << '\n'; i++; break;
                        case 'r':  unescaped << '\r'; i++; break;
                        case 't':  unescaped << '\t'; i++; break;
                        case 'u':
                            // Handle Unicode escapes - simplified for now
                            if (i + 5 < str.length()) {
                                // Just skip the unicode escape for now
                                i += 5;
                            } else {
                                i++; // Skip the 'u'
                            }
                            break;
                        default:
                            unescaped << str[i];
                            break;
                    }
                } else {
                    unescaped << str[i];
                }
            }
            return unescaped.str();
        }

    }// namespace net
}// namespace gossip
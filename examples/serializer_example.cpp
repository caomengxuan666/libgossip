/**
 * @file serializer_example.cpp
 * @brief Example demonstrating serializer usage
 * 
 * This example demonstrates how to use the JSON serializer to serialize
 * and deserialize gossip messages.
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include <iomanip>
#include <iostream>
#include <vector>

// ========================================================================
// Helper functions
// ========================================================================

/**
 * @brief Print a byte array as hex string
 * 
 * @param data The byte array to print
 */
void print_hex(const std::vector<uint8_t> &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0 && i % 16 == 0) {
            std::cout << std::endl;
        }
        std::cout << std::setfill('0') << std::setw(2) << std::hex
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::endl
              << std::dec;
}

/**
 * @brief Create a test node
 * 
 * @param last_byte The last byte of the node ID
 * @return The created node
 */
libgossip::node_view create_test_node(uint8_t last_byte) {
    libgossip::node_view node;
    node.id = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, last_byte}};
    node.ip = "127.0.0." + std::to_string(last_byte);
    node.port = 8000 + last_byte;
    node.config_epoch = last_byte * 10;
    node.heartbeat = last_byte * 100;
    node.version = last_byte * 1000;
    node.status = static_cast<libgossip::node_status>(last_byte % 5);
    node.role = "role_" + std::to_string(last_byte);
    node.region = "region_" + std::to_string(last_byte);
    node.metadata["key1"] = "value1_" + std::to_string(last_byte);
    node.metadata["key2"] = "value2_" + std::to_string(last_byte);
    return node;
}

// ========================================================================
// Main function
// ========================================================================

/**
 * @brief Main function demonstrating serializer usage
 * 
 * This function demonstrates:
 * 1. Creating a JSON serializer
 * 2. Serializing different types of gossip messages
 * 3. Deserializing gossip messages
 */
int main() {
    std::cout << "libgossip Serializer Example" << std::endl;
    std::cout << "===========================" << std::endl;

    try {
        // --------------------------------------------------------------------
        // 1. Create JSON serializer
        // --------------------------------------------------------------------
        gossip::net::json_serializer serializer;
        std::cout << "Created JSON serializer" << std::endl;

        // --------------------------------------------------------------------
        // 2. Test serializing different message types
        // --------------------------------------------------------------------
        std::vector<libgossip::message_type> types = {
                libgossip::message_type::ping,
                libgossip::message_type::pong,
                libgossip::message_type::meet,
                libgossip::message_type::join,
                libgossip::message_type::leave,
                libgossip::message_type::update};

        for (const auto &type: types) {
            std::cout << "\n--- Testing " << static_cast<int>(type) << " Message Type ---" << std::endl;

            // Create test message
            libgossip::gossip_message msg;
            msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
            msg.type = type;
            msg.timestamp = 1000 + static_cast<int>(type);

            // Add some nodes for certain message types
            if (type == libgossip::message_type::meet ||
                type == libgossip::message_type::update) {
                msg.entries.push_back(create_test_node(1));
                msg.entries.push_back(create_test_node(2));
            }

            // Serialize message
            std::vector<uint8_t> data;
            gossip::net::error_code ec = serializer.serialize(msg, data);

            if (ec != gossip::net::error_code::success) {
                std::cerr << "Failed to serialize message, error code: "
                          << static_cast<int>(ec) << std::endl;
                continue;
            }

            std::cout << "Serialized message to " << data.size() << " bytes" << std::endl;

            // Print first 100 bytes of serialized data
            std::cout << "First 100 bytes of serialized data:" << std::endl;
            size_t print_size = std::min(static_cast<size_t>(100), data.size());
            std::vector<uint8_t> print_data(data.begin(), data.begin() + static_cast<long>(print_size));
            print_hex(print_data);

            // Show as string if possible
            std::string json_str(data.begin(), data.end());
            std::cout << "As JSON string (first 200 chars):" << std::endl;
            if (json_str.length() > 200) {
                std::cout << json_str.substr(0, 200) << "..." << std::endl;
            } else {
                std::cout << json_str << std::endl;
            }

            // Deserialize message
            libgossip::gossip_message deserialized_msg;
            ec = serializer.deserialize(data, deserialized_msg);

            if (ec != gossip::net::error_code::success) {
                std::cerr << "Failed to deserialize message, error code: "
                          << static_cast<int>(ec) << std::endl;
                continue;
            }

            std::cout << "Deserialized message successfully" << std::endl;
            std::cout << "  Sender: ";
            for (size_t i = 0; i < deserialized_msg.sender.size(); ++i) {
                std::cout << std::hex << static_cast<int>(deserialized_msg.sender[i]);
                if (i < deserialized_msg.sender.size() - 1) {
                    std::cout << ",";
                }
            }
            std::cout << std::dec << std::endl;
            std::cout << "  Type: " << static_cast<int>(deserialized_msg.type) << std::endl;
            std::cout << "  Timestamp: " << deserialized_msg.timestamp << std::endl;
            std::cout << "  Entries: " << deserialized_msg.entries.size() << std::endl;
        }

        // --------------------------------------------------------------------
        // 3. Test with complex node data
        // --------------------------------------------------------------------
        std::cout << "\n--- Testing Complex Node Data ---" << std::endl;

        libgossip::gossip_message complex_msg;
        complex_msg.sender = {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                               0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
        complex_msg.type = libgossip::message_type::update;
        complex_msg.timestamp = 9876543210ULL;

        // Add nodes with complex metadata
        auto node1 = create_test_node(10);
        node1.metadata["description"] = "This is a test node with complex metadata";
        node1.metadata["version"] = "1.2.3";
        node1.metadata["build"] = "2025-08-30";
        complex_msg.entries.push_back(node1);

        auto node2 = create_test_node(20);
        node2.metadata["service"] = "user-service";
        node2.metadata["environment"] = "production";
        node2.metadata["datacenter"] = "us-east-1";
        complex_msg.entries.push_back(node2);

        // Serialize complex message
        std::vector<uint8_t> complex_data;
        gossip::net::error_code ec = serializer.serialize(complex_msg, complex_data);

        if (ec != gossip::net::error_code::success) {
            std::cerr << "Failed to serialize complex message, error code: "
                      << static_cast<int>(ec) << std::endl;
        } else {
            std::cout << "Serialized complex message to " << complex_data.size() << " bytes" << std::endl;

            // Show as string
            std::string json_str(complex_data.begin(), complex_data.end());
            std::cout << "As JSON string (first 300 chars):" << std::endl;
            if (json_str.length() > 300) {
                std::cout << json_str.substr(0, 300) << "..." << std::endl;
            } else {
                std::cout << json_str << std::endl;
            }
        }

        // --------------------------------------------------------------------
        // 4. Test error handling
        // --------------------------------------------------------------------
        std::cout << "\n--- Testing Error Handling ---" << std::endl;

        // Test deserializing empty data
        std::vector<uint8_t> empty_data;
        libgossip::gossip_message empty_msg;
        ec = serializer.deserialize(empty_data, empty_msg);

        if (ec != gossip::net::error_code::success) {
            std::cout << "Handled empty data correctly, error code: "
                      << static_cast<int>(ec) << std::endl;
        } else {
            std::cout << "Deserialized empty data successfully" << std::endl;
        }

        std::cout << "\nSerializer Example Completed!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
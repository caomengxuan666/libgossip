/**
 * @file serializer_example.cpp
 * @brief Example demonstrating serializer usage
 *
 * This example demonstrates how to use the JSON serializer to serialize
 * and deserialize gossip messages.
 */

#include "core/gossip_core.hpp"
#include "core/message_serializer.hpp"
#include "net/json_serializer.hpp"
#include "net/serializer_factory.hpp"
#include <iomanip>
#include <iostream>

void print_hex(const std::vector<uint8_t> &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0 && i % 16 == 0) {
            std::cout << std::endl;
        }
        std::cout << std::setfill('0') << std::setw(2) << std::hex
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::endl << std::dec;
}

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

int main() {
    std::cout << "libgossip Serializer Example" << std::endl;
    std::cout << "===========================" << std::endl;

    try {
        // 1. Create JSON serializer using factory
        auto serializer = libgossip::serializer_factory::create("json");
        if (!serializer) {
            std::cerr << "Failed to create JSON serializer" << std::endl;
            return 1;
        }
        std::cout << "Created JSON serializer: " << serializer->name() << std::endl;

        // 2. List registered serializers
        auto names = libgossip::serializer_factory::registered_names();
        std::cout << "Registered serializers:";
        for (const auto& name : names) {
            std::cout << " " << name;
        }
        std::cout << std::endl;

        // 3. Test serializing different message types
        std::vector<libgossip::message_type> types = {
                libgossip::message_type::ping,
                libgossip::message_type::pong,
                libgossip::message_type::meet,
                libgossip::message_type::join,
                libgossip::message_type::leave,
                libgossip::message_type::update};

        for (const auto &type: types) {
            std::cout << "\n--- Testing " << static_cast<int>(type) << " Message Type ---" << std::endl;

            libgossip::gossip_message msg;
            msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
            msg.type = type;
            msg.timestamp = 1000 + static_cast<int>(type);

            if (type == libgossip::message_type::meet ||
                type == libgossip::message_type::update) {
                msg.entries.push_back(create_test_node(1));
                msg.entries.push_back(create_test_node(2));
            }

            // Serialize
            std::vector<uint8_t> data;
            auto ec = serializer->serialize(msg, data);

            if (ec != libgossip::serialization_error::success) {
                std::cerr << "Failed to serialize message" << std::endl;
                continue;
            }

            std::cout << "Serialized message to " << data.size() << " bytes" << std::endl;

            size_t print_size = std::min(static_cast<size_t>(100), data.size());
            std::vector<uint8_t> print_data(data.begin(), data.begin() + static_cast<long>(print_size));
            print_hex(print_data);

            std::string json_str(data.begin(), data.end());
            std::cout << "As JSON string (first 200 chars):" << std::endl;
            if (json_str.length() > 200) {
                std::cout << json_str.substr(0, 200) << "..." << std::endl;
            } else {
                std::cout << json_str << std::endl;
            }

            // Deserialize
            libgossip::gossip_message deserialized_msg;
            ec = serializer->deserialize(data, deserialized_msg);

            if (ec != libgossip::serialization_error::success) {
                std::cerr << "Failed to deserialize message" << std::endl;
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

            for (size_t i = 0; i < deserialized_msg.entries.size(); ++i) {
                const auto &entry = deserialized_msg.entries[i];
                std::cout << "    Entry " << i << ": "
                          << entry.ip << ":" << entry.port
                          << " status=" << static_cast<int>(entry.status) << std::endl;
            }
        }

        // 4. Test complex message
        std::cout << "\n--- Testing Complex Message ---" << std::endl;
        libgossip::gossip_message complex_msg;
        complex_msg.sender = {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                               0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
        complex_msg.type = libgossip::message_type::update;
        complex_msg.timestamp = 999999;

        for (int i = 0; i < 5; ++i) {
            complex_msg.entries.push_back(create_test_node(static_cast<uint8_t>(i + 10)));
        }

        std::vector<uint8_t> complex_data;
        auto ec = serializer->serialize(complex_msg, complex_data);

        if (ec == libgossip::serialization_error::success) {
            std::cout << "Serialized complex message to " << complex_data.size() << " bytes" << std::endl;

            libgossip::gossip_message deserialized_complex;
            ec = serializer->deserialize(complex_data, deserialized_complex);

            if (ec == libgossip::serialization_error::success) {
                std::cout << "Deserialized complex message successfully" << std::endl;
                std::cout << "  Entries: " << deserialized_complex.entries.size() << std::endl;
            }
        }

        std::cout << "\nSerializer example completed successfully!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

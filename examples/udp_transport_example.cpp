/**
 * @file udp_transport_example.cpp
 * @brief Example demonstrating basic UDP transport usage
 * 
 * This example demonstrates how to use the UDP transport implementation
 * for sending and receiving gossip messages.
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

// ========================================================================
// Callback functions
// ========================================================================

/**
 * @brief Send callback function
 * 
 * This function is called by the gossip core when it needs to send a message.
 * 
 * @param msg The message to send
 * @param target The target node
 */
void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "[Core] Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

/**
 * @brief Event callback function
 * 
 * This function is called by the gossip core when a node's status changes.
 * 
 * @param node The node that changed status
 * @param old_status The previous status of the node
 */
void event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Core] Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

// ========================================================================
// Main function
// ========================================================================

/**
 * @brief Main function demonstrating UDP transport usage
 * 
 * This function demonstrates:
 * 1. Creating a gossip core instance
 * 2. Creating and configuring a UDP transport
 * 3. Starting the transport
 * 4. Sending messages synchronously and asynchronously
 * 5. Stopping the transport
 */
int main() {
    std::cout << "libgossip UDP Transport Example" << std::endl;
    std::cout << "=============================" << std::endl;

    try {
        // --------------------------------------------------------------------
        // 1. Create local node view
        // --------------------------------------------------------------------
        libgossip::node_view self_node;
        self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};// Simple ID
        self_node.ip = "127.0.0.1";
        self_node.port = 8000;
        self_node.status = libgossip::node_status::online;

        std::cout << "Created local node: " << self_node.ip << ":" << self_node.port << std::endl;

        // --------------------------------------------------------------------
        // 2. Create gossip core
        // --------------------------------------------------------------------
        auto core = std::make_shared<libgossip::gossip_core>(self_node, send_callback, event_callback);
        std::cout << "Created gossip core" << std::endl;

        // --------------------------------------------------------------------
        // 3. Create UDP transport layer
        // --------------------------------------------------------------------
        auto udp_transport = gossip::net::transport_factory::create_transport(
                gossip::net::transport_type::udp, "127.0.0.1", 8000);

        if (!udp_transport) {
            std::cerr << "Failed to create UDP transport" << std::endl;
            return 1;
        }

        std::cout << "Created UDP transport on 127.0.0.1:8000" << std::endl;

        // --------------------------------------------------------------------
        // 4. Create and set serializer
        // --------------------------------------------------------------------
        auto serializer = std::make_unique<gossip::net::json_serializer>();
        udp_transport->set_serializer(std::move(serializer));
        std::cout << "Set JSON serializer" << std::endl;

        // --------------------------------------------------------------------
        // 5. Set gossip core and start transport
        // --------------------------------------------------------------------
        udp_transport->set_gossip_core(core);

        gossip::net::error_code ec = udp_transport->start();
        if (ec != gossip::net::error_code::success) {
            std::cerr << "Failed to start UDP transport: " << static_cast<int>(ec) << std::endl;
            return 1;
        }

        std::cout << "Started UDP transport successfully" << std::endl;

        // --------------------------------------------------------------------
        // 6. Create a target node for testing message sending
        // --------------------------------------------------------------------
        libgossip::node_view target_node;
        target_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};// Another simple ID
        target_node.ip = "127.0.0.1";
        target_node.port = 8001;
        target_node.status = libgossip::node_status::online;

        std::cout << "Created target node: " << target_node.ip << ":" << target_node.port << std::endl;

        // --------------------------------------------------------------------
        // 7. Create a test message
        // --------------------------------------------------------------------
        libgossip::gossip_message test_msg;
        test_msg.sender = self_node.id;
        test_msg.type = libgossip::message_type::ping;
        test_msg.timestamp = 12345;
        test_msg.entries.push_back(self_node);
        test_msg.entries.push_back(target_node);

        std::cout << "Created test message of type: " << static_cast<int>(test_msg.type) << std::endl;

        // --------------------------------------------------------------------
        // 8. Send test message synchronously
        // --------------------------------------------------------------------
        std::cout << "\n--- Synchronous Send ---" << std::endl;
        ec = udp_transport->send_message(test_msg, target_node);
        if (ec != gossip::net::error_code::success) {
            std::cerr << "Failed to send message synchronously, error code: " << static_cast<int>(ec) << std::endl;
        } else {
            std::cout << "Message sent synchronously successfully" << std::endl;
        }

        // --------------------------------------------------------------------
        // 9. Send test message asynchronously
        // --------------------------------------------------------------------
        std::cout << "\n--- Asynchronous Send ---" << std::endl;
        udp_transport->send_message_async(test_msg, target_node, [](gossip::net::error_code ec) {
            if (ec != gossip::net::error_code::success) {
                std::cerr << "Failed to send message asynchronously, error code: " << static_cast<int>(ec) << std::endl;
            } else {
                std::cout << "Message sent asynchronously successfully" << std::endl;
            }
        });

        // --------------------------------------------------------------------
        // 10. Wait for some time to observe results
        // --------------------------------------------------------------------
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // --------------------------------------------------------------------
        // 11. Stop transport layer
        // --------------------------------------------------------------------
        std::cout << "\n--- Stopping Transport ---" << std::endl;
        ec = udp_transport->stop();
        if (ec != gossip::net::error_code::success) {
            std::cerr << "Failed to stop UDP transport: " << static_cast<int>(ec) << std::endl;
            return 1;
        }

        std::cout << "UDP transport stopped successfully" << std::endl;
        std::cout << "\nUDP Transport Example Completed!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

// Send callback function
void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "Core send callback: Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

// Event callback function
void event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "Core event callback: Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

int main() {
    using namespace gossip::net;

    std::cout << "libgossip Network Layer Example" << std::endl;
    std::cout << "===============================" << std::endl;

    // 1. Create local node view
    libgossip::node_view self_node;
    self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};// Simple ID
    self_node.ip = "127.0.0.1";
    self_node.port = 8000;
    self_node.status = libgossip::node_status::online;

    // 2. Create gossip core
    auto core = std::make_shared<libgossip::gossip_core>(self_node, send_callback, event_callback);

    // 3. Create UDP transport layer
    auto udp_transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8000);

    // 4. Create serializer
    auto serializer = std::make_unique<json_serializer>();

    // 5. Configure transport layer
    udp_transport->set_gossip_core(core);
    udp_transport->set_serializer(std::move(serializer));

    // 6. Start transport layer
    gossip::net::error_code ec = udp_transport->start();
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to start UDP transport" << std::endl;
        return 1;
    }

    std::cout << "UDP transport started successfully" << std::endl;

    // 7. Create a target node for testing message sending
    libgossip::node_view target_node;
    target_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};// Another simple ID
    target_node.ip = "127.0.0.1";
    target_node.port = 8001;
    target_node.status = libgossip::node_status::online;

    // 8. Create a test message
    libgossip::gossip_message test_msg;
    test_msg.sender = self_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(self_node);
    test_msg.entries.push_back(target_node);

    // 9. Send test message synchronously
    std::cout << "\nSending test message synchronously..." << std::endl;
    ec = udp_transport->send_message(test_msg, target_node);
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to send message synchronously, error code: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "Message sent synchronously successfully" << std::endl;
    }

    // 10. Send test message asynchronously
    std::cout << "\nSending test message asynchronously..." << std::endl;
    std::promise<gossip::net::error_code> promise;
    std::future<gossip::net::error_code> future = promise.get_future();

    udp_transport->send_message_async(test_msg, target_node, [&promise](gossip::net::error_code ec) {
        std::cout << "Async send callback executed" << std::endl;
        if (ec != gossip::net::error_code::success) {
            std::cerr << "Failed to send message asynchronously, error code: " << static_cast<int>(ec) << std::endl;
        } else {
            std::cout << "Message sent asynchronously successfully" << std::endl;
        }
        promise.set_value(ec);
    });

    // Wait for the async operation to complete with timeout
    auto result = future.wait_for(std::chrono::seconds(2));
    if (result == std::future_status::ready) {
        gossip::net::error_code async_ec = future.get();
        std::cout << "Async operation completed with error code: " << static_cast<int>(async_ec) << std::endl;
    } else {
        std::cerr << "Async operation timed out" << std::endl;
    }

    // 11. Wait for some time to observe results
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 12. Stop transport layer
    std::cout << "\nStopping transport..." << std::endl;
    ec = udp_transport->stop();
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to stop UDP transport" << std::endl;
        return 1;
    }

    std::cout << "UDP transport stopped successfully" << std::endl;

    // 13. Demonstrate TCP transport creation
    std::cout << "\nCreating TCP transport..." << std::endl;
    auto tcp_transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9000);

    if (tcp_transport) {
        tcp_transport->set_gossip_core(core);
        tcp_transport->set_serializer(std::make_unique<json_serializer>());
        std::cout << "TCP transport created successfully" << std::endl;

        // Try to start TCP transport
        ec = tcp_transport->start();
        if (ec == gossip::net::error_code::success) {
            std::cout << "TCP transport started successfully" << std::endl;

            // Send test message synchronously via TCP
            std::cout << "\nSending TCP message synchronously..." << std::endl;
            ec = tcp_transport->send_message(test_msg, target_node);
            if (ec == gossip::net::error_code::success) {
                std::cout << "TCP message sent synchronously successfully" << std::endl;
            } else {
                std::cerr << "Failed to send TCP message synchronously, error code: " << static_cast<int>(ec) << std::endl;
            }

            // Send test message asynchronously via TCP
            std::cout << "\nSending TCP message asynchronously..." << std::endl;
            std::promise<gossip::net::error_code> tcp_promise;
            std::future<gossip::net::error_code> tcp_future = tcp_promise.get_future();

            tcp_transport->send_message_async(test_msg, target_node, [&tcp_promise](gossip::net::error_code ec) {
                std::cout << "TCP async send callback executed" << std::endl;
                if (ec != gossip::net::error_code::success) {
                    std::cerr << "Failed to send TCP message asynchronously, error code: " << static_cast<int>(ec) << std::endl;
                } else {
                    std::cout << "TCP message sent asynchronously successfully" << std::endl;
                }
                tcp_promise.set_value(ec);
            });

            // Wait for the TCP async operation to complete with timeout
            auto tcp_result = tcp_future.wait_for(std::chrono::seconds(2));
            if (tcp_result == std::future_status::ready) {
                gossip::net::error_code tcp_async_ec = tcp_future.get();
                std::cout << "TCP async operation completed with error code: " << static_cast<int>(tcp_async_ec) << std::endl;
            } else {
                std::cerr << "TCP async operation timed out" << std::endl;
            }

            // Stop TCP transport
            std::this_thread::sleep_for(std::chrono::seconds(1));
            tcp_transport->stop();
            std::cout << "TCP transport stopped" << std::endl;
        } else {
            std::cout << "Failed to start TCP transport (might be in use)" << std::endl;
        }
    }

    std::cout << "\nNetwork example completed!" << std::endl;
    return 0;
}
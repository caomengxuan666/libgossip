/**
 * @file gossip_manager_example.cpp
 * @brief Example demonstrating the high-level GossipManager API
 *
 * This example shows how to use the GossipManager class to easily
 * integrate libgossip into your application.
 */

#include "core/gossip_manager.hpp"
#include "core/node_id_utils.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace libgossip;
using namespace std::chrono_literals;

int main() {
    // Create gossip manager
    gossip_manager manager;

    // Configure
    gossip_config config;
    config.bind_ip = "127.0.0.1";
    config.gossip_port = 7946;
    config.role = "master";
    config.region = "us-east-1";
    config.serializer = "json";

    // Initialize
    if (!manager.init(config)) {
        std::cerr << "Failed to initialize gossip manager" << std::endl;
        return 1;
    }

    std::cout << "Node ID: " << node_id_to_string(manager.get_self().id) << std::endl;

    // Set event callback
    manager.set_event_callback([](const node_view& node, node_status old_status, node_status new_status) {
        std::cout << "[EVENT] Node " << node_id_to_string(node.id)
                  << " (" << node.ip << ":" << node.port << "): "
                  << static_cast<int>(old_status) << " -> "
                  << static_cast<int>(new_status) << std::endl;
    });

    // Start
    if (!manager.start()) {
        std::cerr << "Failed to start gossip manager" << std::endl;
        return 1;
    }

    std::cout << "Gossip manager started on port " << config.gossip_port << std::endl;

    // Meet another node (if available)
    // manager.meet_node("127.0.0.1", 7947);

    // Update metadata
    std::map<std::string, std::string> metadata;
    metadata["slots"] = "0-5000";
    metadata["config_epoch"] = "1";
    manager.update_metadata(metadata);

    // Run gossip loop
    std::cout << "Running gossip loop (press Ctrl+C to stop)..." << std::endl;

    for (int i = 0; i < 100; ++i) {
        manager.tick();

        // Print stats every 10 ticks
        if (i % 10 == 0) {
            auto stats = manager.get_stats();
            std::cout << "[Stats] Nodes: " << stats.known_nodes
                      << ", Sent: " << stats.sent_messages
                      << ", Received: " << stats.received_messages
                      << ", Tick duration: " << stats.last_tick_duration_ms << "ms"
                      << std::endl;
        }

        std::this_thread::sleep_for(100ms);
    }

    // Stop
    manager.stop();
    std::cout << "Gossip manager stopped" << std::endl;

    return 0;
}

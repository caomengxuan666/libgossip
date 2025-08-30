/**
 * @file simple_cluster_stats_test.cpp
 * @brief Simple test to demonstrate correct statistics counting
 */

#include "core/gossip_core.hpp"
#include <iostream>
#include <chrono>
#include <memory>

// Global statistics counters
size_t g_total_messages_sent = 0;
size_t g_total_messages_received = 0;
size_t g_node_join_events = 0;
size_t g_node_leave_events = 0;
size_t g_node_failures = 0;
size_t g_node_recoveries = 0;

// Global node cores for message routing
std::shared_ptr<libgossip::gossip_core> g_node1_core, g_node2_core;

// Send callback function
void send_callback(const libgossip::gossip_message& msg, const libgossip::node_view& target) {
    g_total_messages_sent++;
    auto now = libgossip::clock::now();
    
    std::cout << "[Send] Type: " << static_cast<int>(msg.type) 
              << " to node " << target.port
              << " (Sender: " << static_cast<int>(msg.sender.at(0)) << ")" << std::endl;

    // Route message to the target node
    if (target.port == 8001) {
        g_node1_core->handle_message(msg, now);
    }
    else if (target.port == 8002) {
        g_node2_core->handle_message(msg, now);
    }
    else {
        std::cerr << "[Warning] Unknown target port: " << target.port << std::endl;
    }
}

// Event callback function
void event_callback(const libgossip::node_view& node, libgossip::node_status old_status) {
    std::cout << "[Event] Node " << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
              
    // Note: node_status enum values are: unknown=0, joining=1, online=2, suspect=3, failed=4
    // Count node join when transitioning from joining to online
    if (node.status == libgossip::node_status::online && 
        old_status == libgossip::node_status::joining) {
        g_node_join_events++;
        std::cout << "[Stat] Node join event incremented, total: " << g_node_join_events << std::endl;
    } 
    // Node failure
    else if (node.status == libgossip::node_status::failed && 
             old_status != libgossip::node_status::failed) {
        g_node_failures++;
        std::cout << "[Stat] Node failure event incremented, total: " << g_node_failures << std::endl;
    }
    // Node recovery (from failed to online)
    else if (node.status == libgossip::node_status::online && 
             old_status == libgossip::node_status::failed) {
        g_node_recoveries++;
        std::cout << "[Stat] Node recovery event incremented, total: " << g_node_recoveries << std::endl;
    }
    // Node leave (from online to unknown)
    else if (node.status == libgossip::node_status::unknown && 
             old_status == libgossip::node_status::online) {
        g_node_leave_events++;
        std::cout << "[Stat] Node leave event incremented, total: " << g_node_leave_events << std::endl;
    }
}


int main() {
    std::cout << "libgossip Simple Cluster Statistics Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create two nodes
    libgossip::node_view node1_view;
    node1_view.id = {{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
    node1_view.ip = "127.0.0.1";
    node1_view.port = 8001;
    node1_view.status = libgossip::node_status::online;
    
    libgossip::node_view node2_view;
    node2_view.id = {{2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
    node2_view.ip = "127.0.0.1";
    node2_view.port = 8002;
    node2_view.status = libgossip::node_status::online;
    
    // Create gossip cores
    auto node1_core = std::make_shared<libgossip::gossip_core>(node1_view, send_callback, event_callback);
    auto node2_core = std::make_shared<libgossip::gossip_core>(node2_view, send_callback, event_callback);
    
    // Set global pointers for message routing
    g_node1_core = node1_core;
    g_node2_core = node2_core;
    
    // Make nodes aware of each other
    node1_core->meet(node2_view);
    node2_core->meet(node1_view);
    
    std::cout << "\nInitial state:" << std::endl;
    std::cout << "Total Messages Sent:     " << g_total_messages_sent << std::endl;
    std::cout << "Total Messages Received: " << g_total_messages_received << std::endl;
    std::cout << "Node Join Events:        " << g_node_join_events << std::endl;
    std::cout << "Node Failures:           " << g_node_failures << std::endl;
    
    // Run a few gossip cycles
    std::cout << "\nRunning gossip cycles..." << std::endl;
    for (int i = 0; i < 3; i++) {
        node1_core->tick();
        node2_core->tick();
        
        // Get individual node stats
        auto stats1 = node1_core->get_stats();
        auto stats2 = node2_core->get_stats();
        
        std::cout << "Cycle " << (i+1) << " - Node1 knows " << stats1.known_nodes 
                  << " nodes, sent " << stats1.sent_messages 
                  << " messages, received " << stats1.received_messages << std::endl;
        std::cout << "Cycle " << (i+1) << " - Node2 knows " << stats2.known_nodes 
                  << " nodes, sent " << stats2.sent_messages 
                  << " messages, received " << stats2.received_messages << std::endl;
    }
    
    // Update global received message count from node cores
    g_total_messages_received = node1_core->get_stats().received_messages + 
                                node2_core->get_stats().received_messages;
    
    std::cout << "\nAfter gossip cycles:" << std::endl;
    std::cout << "Total Messages Sent:     " << g_total_messages_sent << std::endl;
    std::cout << "Total Messages Received: " << g_total_messages_received << std::endl;
    std::cout << "Node Join Events:        " << g_node_join_events << std::endl;
    std::cout << "Node Failures:           " << g_node_failures << std::endl;
    
    // Simulate a node failure and recovery
    std::cout << "\nSimulating node failure and recovery..." << std::endl;
    libgossip::node_view failed_node = node2_view;
    failed_node.status = libgossip::node_status::failed;
    failed_node.heartbeat = 1000;  // Must be greater than current heartbeat
    failed_node.config_epoch = 1;  // Increase config epoch to ensure update
    auto time_point = std::chrono::steady_clock::now();
    
    // Create a message to simulate node failure
    libgossip::gossip_message failure_msg;
    failure_msg.sender = node2_view.id;
    failure_msg.type = libgossip::message_type::update;
    failure_msg.timestamp = 1000;
    failure_msg.entries = {failed_node};
    
    g_node1_core->handle_message(failure_msg, time_point);
    
    std::cout << "\nAfter simulating failure:" << std::endl;
    std::cout << "Node Failures:           " << g_node_failures << std::endl;
    
    // Simulate recovery
    libgossip::node_view recovered_node = node2_view;
    recovered_node.status = libgossip::node_status::online;
    recovered_node.heartbeat = 1001;  // Must be greater than previous heartbeat
    recovered_node.config_epoch = 2;  // Increase config epoch to ensure update
    
    // Create a message to simulate node recovery
    libgossip::gossip_message recovery_msg;
    recovery_msg.sender = node2_view.id;
    recovery_msg.type = libgossip::message_type::update;
    recovery_msg.timestamp = 1001;
    recovery_msg.entries = {recovered_node};
    
    g_node1_core->handle_message(recovery_msg, time_point);
    
    std::cout << "\nAfter simulating recovery:" << std::endl;
    std::cout << "Node Recoveries:         " << g_node_recoveries << std::endl;
    
    // Final statistics
    std::cout << "\nFinal statistics:" << std::endl;
    std::cout << "Total Messages Sent:     " << g_total_messages_sent << std::endl;
    std::cout << "Total Messages Received: " << g_total_messages_received << std::endl;
    std::cout << "Node Join Events:        " << g_node_join_events << std::endl;
    std::cout << "Node Leave Events:       " << g_node_leave_events << std::endl;
    std::cout << "Node Failures:           " << g_node_failures << std::endl;
    std::cout << "Node Recoveries:         " << g_node_recoveries << std::endl;
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}
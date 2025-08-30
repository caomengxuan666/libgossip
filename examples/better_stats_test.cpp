/**
 * @file better_stats_test.cpp
 * @brief Better test to demonstrate correct statistics counting
 */

#include "core/gossip_core.hpp"
#include <iostream>
#include <chrono>
#include <thread>

// Per-node statistics
struct node_stats {
    size_t messages_sent = 0;
    size_t messages_received = 0;
};

// Global statistics
node_stats g_node1_stats;
node_stats g_node2_stats;
size_t g_global_join_events = 0;
size_t g_global_failures = 0;
size_t g_global_recoveries = 0;

// Send callback function for node 1
void node1_send_callback(const libgossip::gossip_message& msg, const libgossip::node_view& target) {
    g_node1_stats.messages_sent++;
    std::cout << "[Node1 Send] Type: " << static_cast<int>(msg.type) 
              << " to node " << target.port
              << " (Sender: " << static_cast<int>(msg.sender.at(0)) << ")" << std::endl;
}

// Send callback function for node 2
void node2_send_callback(const libgossip::gossip_message& msg, const libgossip::node_view& target) {
    g_node2_stats.messages_sent++;
    std::cout << "[Node2 Send] Type: " << static_cast<int>(msg.type) 
              << " to node " << target.port
              << " (Sender: " << static_cast<int>(msg.sender.at(0)) << ")" << std::endl;
}

// Event callback function
void global_event_callback(const libgossip::node_view& node, libgossip::node_status old_status) {
    std::cout << "[Event] Node " << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
              
    switch (node.status) {
        case libgossip::node_status::online:
            if (old_status == libgossip::node_status::joining || 
                old_status == libgossip::node_status::unknown) {
                g_global_join_events++;
            } else if (old_status == libgossip::node_status::failed) {
                g_global_recoveries++;
            }
            break;
            
        case libgossip::node_status::failed:
            g_global_failures++;
            break;
            
        default:
            break;
    }
}

// Simulate sending a message from one node to another
void simulate_message_passing(
    const std::shared_ptr<libgossip::gossip_core>& sender_core,
    const std::shared_ptr<libgossip::gossip_core>& receiver_core,
    const libgossip::node_view& receiver_node,
    libgossip::message_type type) {
    
    // Create a simple ping message
    libgossip::gossip_message msg;
    msg.sender = sender_core->self().id;
    msg.type = type;
    msg.timestamp = sender_core->self().heartbeat;
    
    // Process the message on the receiver
    auto time_point = std::chrono::steady_clock::now();
    receiver_core->handle_message(msg, time_point);
}

int main() {
    std::cout << "libgossip Better Statistics Test" << std::endl;
    std::cout << "===============================" << std::endl;
    
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
    auto node1_core = std::make_shared<libgossip::gossip_core>(node1_view, node1_send_callback, global_event_callback);
    auto node2_core = std::make_shared<libgossip::gossip_core>(node2_view, node2_send_callback, global_event_callback);
    
    std::cout << "\n=== Initial State ===" << std::endl;
    auto stats1 = node1_core->get_stats();
    auto stats2 = node2_core->get_stats();
    std::cout << "Node 1 - Known nodes: " << stats1.known_nodes 
              << ", Sent: " << stats1.sent_messages
              << ", Received: " << stats1.received_messages << std::endl;
    std::cout << "Node 2 - Known nodes: " << stats2.known_nodes 
              << ", Sent: " << stats2.sent_messages
              << ", Received: " << stats2.received_messages << std::endl;
    std::cout << "Global Join Events: " << g_global_join_events << std::endl;
    
    // Make nodes aware of each other (this will trigger join events)
    std::cout << "\n=== Making nodes aware of each other ===" << std::endl;
    node1_core->meet(node2_view);
    node2_core->meet(node1_view);
    
    stats1 = node1_core->get_stats();
    stats2 = node2_core->get_stats();
    std::cout << "Node 1 - Known nodes: " << stats1.known_nodes 
              << ", Sent: " << stats1.sent_messages
              << ", Received: " << stats1.received_messages << std::endl;
    std::cout << "Node 2 - Known nodes: " << stats2.known_nodes 
              << ", Sent: " << stats2.sent_messages
              << ", Received: " << stats2.received_messages << std::endl;
    std::cout << "Global Join Events: " << g_global_join_events << std::endl;
    
    // Run gossip cycles
    std::cout << "\n=== Running gossip cycles ===" << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "\n--- Cycle " << (i+1) << " ---" << std::endl;
        node1_core->tick();
        node2_core->tick();
        
        // Simulate message passing to increase received count
        simulate_message_passing(node1_core, node2_core, node2_view, libgossip::message_type::ping);
        simulate_message_passing(node2_core, node1_core, node1_view, libgossip::message_type::pong);
        
        stats1 = node1_core->get_stats();
        stats2 = node2_core->get_stats();
        std::cout << "Node 1 - Known nodes: " << stats1.known_nodes 
                  << ", Sent: " << stats1.sent_messages
                  << ", Received: " << stats1.received_messages << std::endl;
        std::cout << "Node 2 - Known nodes: " << stats2.known_nodes 
                  << ", Sent: " << stats2.sent_messages
                  << ", Received: " << stats2.received_messages << std::endl;
    }
    
    std::cout << "\n=== Final Statistics ===" << std::endl;
    stats1 = node1_core->get_stats();
    stats2 = node2_core->get_stats();
    std::cout << "Node 1 - Known nodes: " << stats1.known_nodes 
              << ", Sent: " << stats1.sent_messages
              << ", Received: " << stats1.received_messages << std::endl;
    std::cout << "Node 2 - Known nodes: " << stats2.known_nodes 
              << ", Sent: " << stats2.sent_messages
              << ", Received: " << stats2.received_messages << std::endl;
    std::cout << "Global messages sent (Node 1): " << g_node1_stats.messages_sent << std::endl;
    std::cout << "Global messages sent (Node 2): " << g_node2_stats.messages_sent << std::endl;
    std::cout << "Global Join Events: " << g_global_join_events << std::endl;
    std::cout << "Global Failures: " << g_global_failures << std::endl;
    std::cout << "Global Recoveries: " << g_global_recoveries << std::endl;
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}
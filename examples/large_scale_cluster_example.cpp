/**
 * @file large_scale_cluster_example.cpp
 * @brief Large scale cluster example demonstrating complex communication patterns
 * 
 * This example demonstrates a large scale cluster with multiple nodes communicating
 * through various message types and patterns. It showcases the full capabilities
 * of libgossip in a realistic distributed system scenario.
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include "net/udp_transport.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

// ========================================================================
// Configuration constants
// ========================================================================

/// Number of nodes in the cluster
constexpr size_t CLUSTER_SIZE = 10;

/// Base port for the cluster nodes
constexpr uint16_t BASE_PORT = 8000;

/// Simulation duration in seconds
constexpr int SIMULATION_DURATION = 30;

/// Interval between periodic operations in milliseconds
constexpr int OPERATION_INTERVAL = 1000;

/// Probability of node failure (0.0 to 1.0)
constexpr double NODE_FAILURE_PROBABILITY = 0.05;

/// Probability of node recovery (0.0 to 1.0)
constexpr double NODE_RECOVERY_PROBABILITY = 0.1;

// ========================================================================
// Type definitions
// ========================================================================

/// Node operation types
enum class node_operation {
    join_cluster,
    leave_cluster,
    send_update,
    query_status,
    broadcast_message
};

/// Statistics structure for tracking cluster metrics
struct cluster_statistics {
    std::atomic<size_t> total_messages_sent{0};
    std::atomic<size_t> total_messages_received{0};
    std::atomic<size_t> total_gossip_rounds{0};
    std::atomic<size_t> node_join_events{0};
    std::atomic<size_t> node_leave_events{0};
    std::atomic<size_t> node_failures{0};
    std::atomic<size_t> node_recoveries{0};

    /// Reset all statistics
    void reset() {
        total_messages_sent = 0;
        total_messages_received = 0;
        total_gossip_rounds = 0;
        node_join_events = 0;
        node_leave_events = 0;
        node_failures = 0;
        node_recoveries = 0;
    }
};

// ========================================================================
// Global variables
// ========================================================================

/// Shared cluster statistics
cluster_statistics g_stats;

/// Flag to control simulation execution
std::atomic<bool> g_running{true};

/// Random number generator
std::random_device g_rd;
std::mt19937 g_gen(g_rd());

// ========================================================================
// Forward declarations
// ========================================================================

class cluster_node;

// ========================================================================
// Helper functions
// ========================================================================

/**
 * @brief Generate a random node ID
 * 
 * @return Randomly generated node ID
 */
libgossip::node_id_t generate_random_node_id() {
    libgossip::node_id_t id;
    std::uniform_int_distribution<uint16_t> dis(0, 255);

    for (auto &byte: id) {
        byte = static_cast<unsigned char>(dis(g_gen));
    }
    return id;
}

/**
 * @brief Convert node ID to string representation
 * 
 * @param id Node ID to convert
 * @return String representation of the node ID
 */
std::string node_id_to_string(const libgossip::node_id_t &id) {
    std::ostringstream oss;
    for (size_t i = 0; i < id.size(); ++i) {
        if (i > 0) {
            oss << ":";
        }
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(id[i]);
    }
    return oss.str();
}

/**
 * @brief Create a test node
 * 
 * @param node_index Index of the node
 * @return Created node view
 */
libgossip::node_view create_test_node(size_t node_index) {
    libgossip::node_view node;
    node.id = generate_random_node_id();
    node.ip = "127.0.0.1";
    node.port = static_cast<uint16_t>(BASE_PORT + node_index);
    node.config_epoch = 1;
    node.heartbeat = 0;
    node.version = 1;
    node.status = libgossip::node_status::joining;
    node.role = "worker";
    node.region = "datacenter-1";
    node.metadata["created_at"] = std::to_string(std::time(nullptr));
    node.metadata["node_index"] = std::to_string(node_index);
    return node;
}

/**
 * @brief Print cluster statistics
 */
void print_statistics() {
    std::cout << "\n=== Cluster Statistics ===" << std::endl;
    std::cout << "Total Messages Sent:     " << g_stats.total_messages_sent.load() << std::endl;
    std::cout << "Total Messages Received: " << g_stats.total_messages_received.load() << std::endl;
    std::cout << "Total Gossip Rounds:     " << g_stats.total_gossip_rounds.load() << std::endl;
    std::cout << "Node Join Events:        " << g_stats.node_join_events.load() << std::endl;
    std::cout << "Node Leave Events:       " << g_stats.node_leave_events.load() << std::endl;
    std::cout << "Node Failures:           " << g_stats.node_failures.load() << std::endl;
    std::cout << "Node Recoveries:         " << g_stats.node_recoveries.load() << std::endl;
    std::cout << "=========================" << std::endl;
}

/**
 * @brief Print node status
 * 
 * @param node Node to print status for
 */
void print_node_status(const libgossip::node_view &node) {
    std::cout << "Node " << node_id_to_string(node.id)
              << " (" << node.ip << ":" << node.port << ")"
              << " Status: " << static_cast<int>(node.status)
              << " Heartbeat: " << node.heartbeat
              << std::endl;
}

// ========================================================================
// Node class
// ========================================================================

// Global node registry for message routing
std::map<uint16_t, cluster_node *> g_node_registry;

/**
 * @brief Event callback function
 * 
 * This function is called by the gossip core when a node's status changes.
 * 
 * @param node The node that changed status
 * @param old_status The previous status of the node
 */
void node_event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Event] Node " << node_id_to_string(node.id)
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;

    switch (node.status) {
        case libgossip::node_status::online:
            if (old_status == libgossip::node_status::joining ||
                old_status == libgossip::node_status::unknown) {
                g_stats.node_join_events++;
            } else if (old_status == libgossip::node_status::failed) {
                g_stats.node_recoveries++;
            }
            break;

        case libgossip::node_status::failed:
            g_stats.node_failures++;
            break;

        case libgossip::node_status::unknown:
            if (old_status == libgossip::node_status::online) {
                g_stats.node_leave_events++;
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Cluster node class
 * 
 * This class represents a single node in the cluster, encapsulating
 * the gossip core, transport, and node-specific functionality.
 */
class cluster_node {
public:
    /**
     * @brief Constructor
     * 
     * @param index Node index
     */
    explicit cluster_node(size_t index)
        : m_index(index), m_node_info(create_test_node(index)), m_core(std::make_shared<libgossip::gossip_core>(m_node_info, [this](const libgossip::gossip_message &msg, const libgossip::node_view &target) { this->send_callback(msg, target); }, node_event_callback)), m_serializer(std::make_unique<gossip::net::json_serializer>()) {

        // Register node in global registry for message routing
        g_node_registry[m_node_info.port] = this;

        // Create transport (alternating between UDP and TCP)
        gossip::net::transport_type type = (index % 2 == 0) ? gossip::net::transport_type::udp : gossip::net::transport_type::tcp;

        m_transport = gossip::net::transport_factory::create_transport(
                type, m_node_info.ip, m_node_info.port);

        if (!m_transport) {
            throw std::runtime_error("Failed to create transport");
        }

        // Configure transport
        m_transport->set_gossip_core(m_core);
        m_transport->set_serializer(std::move(m_serializer));

        std::cout << "Created node " << m_index
                  << " with " << ((type == gossip::net::transport_type::udp) ? "UDP" : "TCP")
                  << " transport at " << m_node_info.ip << ":" << m_node_info.port
                  << std::endl;
    }

    /**
     * @brief Destructor
     */
    ~cluster_node() {
        // Remove node from registry
        g_node_registry.erase(m_node_info.port);

        if (m_transport_thread.joinable()) {
            m_transport_thread.join();
        }
    }

    /**
     * @brief Start the node
     * 
     * @return Error code indicating success or failure
     */
    gossip::net::error_code start() {
        // Start transport
        gossip::net::error_code ec = m_transport->start();
        if (ec != gossip::net::error_code::success) {
            return ec;
        }

        // Start transport thread
        m_transport_thread = std::thread([this]() {
            while (g_running) {
                // In a real implementation, this would handle incoming messages
                // For this example, we just sleep to simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        // Broadcast a join message to announce our presence
        // This will allow other nodes to learn about us through the gossip protocol
        broadcast_join_message();

        std::cout << "Started node " << m_index << std::endl;
        return gossip::net::error_code::success;
    }

    /**
     * @brief Stop the node
     * 
     * @return Error code indicating success or failure
     */
    gossip::net::error_code stop() {
        g_running = false;

        // Stop transport
        gossip::net::error_code ec = m_transport->stop();
        if (ec != gossip::net::error_code::success) {
            return ec;
        }

        // Wait for transport thread to finish
        if (m_transport_thread.joinable()) {
            m_transport_thread.join();
        }

        std::cout << "Stopped node " << m_index << std::endl;
        return gossip::net::error_code::success;
    }

    /**
     * @brief Perform periodic operations
     */
    void perform_periodic_operations() {
        // Update heartbeat
        m_node_info.heartbeat++;

        // Perform gossip round
        m_core->tick();
        g_stats.total_gossip_rounds++;

        // Randomly send updates or perform other operations
        std::uniform_real_distribution<double> op_dis(0.0, 1.0);
        double op_choice = op_dis(g_gen);

        if (op_choice < 0.3) {
            // Send update message
            send_update_message();
        } else if (op_choice < 0.5) {
            // Broadcast message
            broadcast_message();
        }

        // Randomly simulate node failure or recovery
        simulate_node_issues();
    }

    /**
     * @brief Get node information
     * 
     * @return Node information
     */
    const libgossip::node_view &get_node_info() const {
        return m_node_info;
    }

    /**
     * @brief Get node index
     * 
     * @return Node index
     */
    size_t get_index() const {
        return m_index;
    }

    /**
     * @brief Get gossip core
     * 
     * @return Shared pointer to gossip core
     */
    std::shared_ptr<libgossip::gossip_core> get_core() const {
        return m_core;
    }

    /**
     * @brief Meet another node
     * 
     * @param other The other node to meet
     */
    void meet(const libgossip::node_view &other) {
        m_core->meet(other);
    }

    /**
     * @brief Set manager nodes for message sending
     * 
     * @param nodes Vector of all nodes in the cluster
     */
    void set_manager_nodes(const std::vector<std::unique_ptr<cluster_node>> &nodes) {
        // Store pointer references to all nodes in the cluster
        m_manager_nodes.reserve(nodes.size());
        for (const auto &node: nodes) {
            m_manager_nodes.push_back(node.get());
        }
    }

    /**
     * @brief Receive a message from another node
     * 
     * @param msg The message to receive
     */
    void receive_message(const libgossip::gossip_message &msg) {
        // Convert timestamp to time_point
        ++g_stats.total_messages_received;
        auto time_point = std::chrono::time_point<std::chrono::steady_clock>(
                std::chrono::milliseconds(msg.timestamp));
        m_core->handle_message(msg, time_point);
    }

private:
    /**
     * @brief Send callback function
     * 
     * This function is called by the gossip core when it needs to send a message.
     * 
     * @param msg The message to send
     * @param target The target node
     */
    void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
        g_stats.total_messages_sent++;

        // Route message to the target node
        auto it = g_node_registry.find(target.port);
        if (it != g_node_registry.end()) {
            auto now = libgossip::clock::now();
            it->second->get_core()->handle_message(msg, now);
        } else {
            std::cerr << "[Warning] Unknown target port: " << target.port << std::endl;
        }
    }

    /**
     * @brief Send an update message
     */
    void send_update_message() {
        libgossip::gossip_message msg;
        msg.sender = m_node_info.id;
        msg.type = libgossip::message_type::update;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

        // Add some metadata
        libgossip::node_view self_node = m_node_info;
        self_node.metadata["last_update"] = std::to_string(msg.timestamp);
        self_node.metadata["operation"] = "periodic_update";
        msg.entries.push_back(self_node);

        // Send message to a random node to simulate real network communication
        // This will properly increment the received_messages counter
        if (!m_manager_nodes.empty()) {
            // Pick a random node to send to
            std::uniform_int_distribution<size_t> node_dis(0, m_manager_nodes.size() - 1);
            size_t target_idx = node_dis(g_gen);

            // Don't send to self
            if (m_manager_nodes[target_idx] != this) {
                m_manager_nodes[target_idx]->receive_message(msg);
                g_stats.total_messages_sent++;
                std::cout << "[Node " << m_index << "] Sent update message to node " << target_idx << std::endl;
            }
        } else {
            // Fallback to original behavior if manager nodes not set
            // In a real implementation, this would be sent to other nodes
            // For this example, we just process it locally
            // Convert timestamp to time_point
            auto time_point = std::chrono::time_point<std::chrono::steady_clock>(
                    std::chrono::milliseconds(msg.timestamp));
            m_core->handle_message(msg, time_point);
            g_stats.total_messages_sent++;
            std::cout << "[Node " << m_index << "] Sent update message (local)" << std::endl;
        }
    }

    /**
     * @brief Broadcast a message to all nodes
     */
    void broadcast_message() {
        libgossip::gossip_message msg;
        msg.sender = m_node_info.id;
        msg.type = libgossip::message_type::join;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

        // Add node info
        msg.entries.push_back(m_node_info);

        // Send message to all other nodes to simulate real network communication
        bool sent = false;
        for (auto *node: m_manager_nodes) {
            // Don't send to self
            if (node != this) {
                node->receive_message(msg);
                g_stats.total_messages_sent++;
                sent = true;
            }
        }

        if (!sent && m_manager_nodes.empty()) {
            // Fallback to original behavior if manager nodes not set
            // In a real implementation, this would be broadcast to all nodes
            // For this example, we just process it locally
            // Convert timestamp to time_point
            auto time_point = std::chrono::time_point<std::chrono::steady_clock>(
                    std::chrono::milliseconds(msg.timestamp));
            m_core->handle_message(msg, time_point);
            g_stats.total_messages_sent++;
            std::cout << "[Node " << m_index << "] Broadcast join message (local)" << std::endl;
        } else if (sent) {
            std::cout << "[Node " << m_index << "] Broadcast join message to "
                      << (m_manager_nodes.size() - 1) << " nodes" << std::endl;
        }
    }

    /**
     * @brief Broadcast a join message to announce our presence
     */
    void broadcast_join_message() {
        libgossip::gossip_message msg;
        msg.sender = m_node_info.id;
        msg.type = libgossip::message_type::join;
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

        // Add node info
        msg.entries.push_back(m_node_info);

        // In a real implementation, this would be broadcast to all nodes
        // For this example, we just process it locally
        // Convert timestamp to time_point
        auto time_point = std::chrono::time_point<std::chrono::steady_clock>(
                std::chrono::milliseconds(msg.timestamp));
        m_core->handle_message(msg, time_point);
        g_stats.total_messages_sent++;

        std::cout << "[Node " << m_index << "] Broadcast join message" << std::endl;
    }

    /**
     * @brief Simulate node issues (failures/recoveries)
     */
    void simulate_node_issues() {
        std::uniform_real_distribution<double> fail_dis(0.0, 1.0);
        double fail_chance = fail_dis(g_gen);

        libgossip::node_status old_status = m_node_info.status;

        if (m_node_info.status == libgossip::node_status::online &&
            fail_chance < NODE_FAILURE_PROBABILITY) {
            // Simulate node failure
            m_node_info.status = libgossip::node_status::failed;
            std::cout << "[Node " << m_index << "] Simulated failure" << std::endl;
            // Call event callback manually since we're changing status directly
            node_event_callback(m_node_info, old_status);
        } else if (m_node_info.status == libgossip::node_status::failed &&
                   fail_chance < NODE_RECOVERY_PROBABILITY) {
            // Simulate node recovery
            m_node_info.status = libgossip::node_status::online;
            std::cout << "[Node " << m_index << "] Simulated recovery" << std::endl;
            // Call event callback manually since we're changing status directly
            node_event_callback(m_node_info, old_status);
        }
    }

    /// Node index
    size_t m_index;

    /// Node information
    libgossip::node_view m_node_info;

    /// Gossip core
    std::shared_ptr<libgossip::gossip_core> m_core;

    /// Transport
    std::unique_ptr<gossip::net::transport> m_transport;

    /// Serializer
    std::unique_ptr<gossip::net::message_serializer> m_serializer;

    /// Transport thread
    std::thread m_transport_thread;

    /// References to all nodes in the cluster for message sending
    std::vector<cluster_node *> m_manager_nodes;
};

// ========================================================================
// Cluster manager
// ========================================================================

/**
 * @brief Cluster manager class
 * 
 * This class manages the entire cluster, including node creation,
 * inter-node communication, and cluster-wide operations.
 */
class cluster_manager {
public:
    /**
     * @brief Constructor
     * 
     * @param size Number of nodes in the cluster
     */
    explicit cluster_manager(size_t size) : m_size(size) {
        m_nodes.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            m_nodes.emplace_back(std::make_unique<cluster_node>(i));
        }
    }

    /**
     * @brief Initialize the cluster
     * 
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize() {
        std::cout << "Initializing cluster with " << m_size << " nodes..." << std::endl;

        // Start all nodes
        for (auto &node: m_nodes) {
            gossip::net::error_code ec = node->start();
            if (ec != gossip::net::error_code::success) {
                std::cerr << "Failed to start node " << node->get_index() << std::endl;
                return false;
            }
        }

        // Allow some time for nodes to start
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Set manager nodes for all nodes to enable proper message sending
        for (auto &node: m_nodes) {
            node->set_manager_nodes(m_nodes);
        }

        // Make nodes aware of each other through meet operations
        // For simplicity, just make all nodes meet the first node
        if (!m_nodes.empty()) {
            auto &first_node = m_nodes[0];
            for (size_t i = 1; i < m_nodes.size(); ++i) {
                auto &node = m_nodes[i];
                first_node->meet(node->get_node_info());
                node->meet(first_node->get_node_info());
            }
        }

        std::cout << "Cluster initialized successfully" << std::endl;
        return true;
    }

    /**
     * @brief Run the cluster simulation
     * 
     * @param duration Duration to run the simulation in seconds
     */
    void run_simulation(int duration) {
        std::cout << "Running cluster simulation for " << duration << " seconds..." << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration);

        while (g_running && std::chrono::steady_clock::now() < end_time) {
            // Perform periodic operations on all nodes
            for (auto &node: m_nodes) {
                node->perform_periodic_operations();
            }

            // Print statistics periodically
            static int counter = 0;
            if (++counter % 5 == 0) {
                print_statistics();
            }

            // Sleep for a bit before next iteration
            std::this_thread::sleep_for(std::chrono::milliseconds(OPERATION_INTERVAL));
        }

        std::cout << "Simulation completed" << std::endl;
    }

    /**
     * @brief Shutdown the cluster
     */
    void shutdown() {
        std::cout << "Shutting down cluster..." << std::endl;

        g_running = false;

        // Stop all nodes
        for (auto &node: m_nodes) {
            node->stop();
        }

        std::cout << "Cluster shutdown completed" << std::endl;
    }

    /**
     * @brief Print current cluster status
     */
    void print_cluster_status() {
        std::cout << "\n=== Cluster Status ===" << std::endl;
        for (const auto &node: m_nodes) {
            print_node_status(node->get_node_info());
        }
        std::cout << "=====================" << std::endl;
    }

private:
    /// Cluster size
    size_t m_size;

    /// Nodes in the cluster
    std::vector<std::unique_ptr<cluster_node>> m_nodes;
};

// ========================================================================
// Signal handler
// ========================================================================

/**
 * @brief Signal handler for graceful shutdown
 * 
 * @param signal Signal number
 */
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

// ========================================================================
// Main function
// ========================================================================

/**
 * @brief Main function demonstrating large scale cluster operations
 * 
 * This function demonstrates:
 * 1. Creating a large cluster of nodes
 * 2. Using both UDP and TCP transports
 * 3. Simulating various node operations and failures
 * 4. Monitoring cluster statistics
 * 5. Graceful shutdown
 */
int main() {
    std::cout << "libgossip Large Scale Cluster Example" << std::endl;
    std::cout << "=====================================" << std::endl;

    try {
        // Create cluster manager
        cluster_manager manager(CLUSTER_SIZE);

        // Initialize cluster
        if (!manager.initialize()) {
            std::cerr << "Failed to initialize cluster" << std::endl;
            return 1;
        }

        // Print initial status
        manager.print_cluster_status();

        // Run simulation
        manager.run_simulation(SIMULATION_DURATION);

        // Print final status
        manager.print_cluster_status();

        // Print final statistics
        print_statistics();

        // Shutdown cluster
        manager.shutdown();

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Large scale cluster example completed successfully" << std::endl;
    return 0;
}
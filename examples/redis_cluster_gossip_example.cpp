/**
 * @file redis_cluster_gossip_example.cpp
 * @brief Redis cluster style gossip example demonstrating node discovery and message passing
 * 
 * This example demonstrates a Redis cluster style gossip implementation with:
 * 1. Node discovery using gossip protocol
 * 2. Cluster state synchronization
 * 3. Failure detection and recovery
 * 4. Message routing between nodes
 * 5. Cluster configuration management
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/tcp_transport.hpp"
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

/// Number of nodes in the Redis cluster
constexpr size_t CLUSTER_SIZE = 6;

/// Base port for the cluster nodes
constexpr uint16_t BASE_PORT = 7000;

/// Simulation duration in seconds
constexpr int SIMULATION_DURATION = 60;

/// Interval between gossip cycles in milliseconds
constexpr int GOSSIP_INTERVAL = 100;

/// Interval between periodic operations in milliseconds
constexpr int OPERATION_INTERVAL = 2000;

/// Probability of node failure (0.0 to 1.0)
constexpr double NODE_FAILURE_PROBABILITY = 0.05;

/// Probability of node recovery (0.0 to 1.0)
constexpr double NODE_RECOVERY_PROBABILITY = 0.15;

/// Redis cluster hash slots
constexpr size_t HASH_SLOTS = 16384;

// ========================================================================
// Type definitions
// ========================================================================

/// Redis node roles
enum class node_role {
    master,
    slave
};

/// Redis cluster node information
struct redis_node_info {
    libgossip::node_id_t id;
    std::string ip;
    uint16_t port;
    node_role role;
    std::vector<size_t> slots;// Hash slots managed by this node
    std::string replication_id;
    size_t replication_offset;
};

/// Cluster statistics
struct cluster_statistics {
    std::atomic<size_t> total_messages_sent{0};
    std::atomic<size_t> total_messages_received{0};
    std::atomic<size_t> total_gossip_cycles{0};
    std::atomic<size_t> node_join_events{0};
    std::atomic<size_t> node_leave_events{0};
    std::atomic<size_t> node_failures{0};
    std::atomic<size_t> node_recoveries{0};
    std::atomic<size_t> slot_migrations{0};

    void reset() {
        total_messages_sent = 0;
        total_messages_received = 0;
        total_gossip_cycles = 0;
        node_join_events = 0;
        node_leave_events = 0;
        node_failures = 0;
        node_recoveries = 0;
        slot_migrations = 0;
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

class redis_cluster_node;

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
        if (i > 0) { oss << ":";
}
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(id[i]);
    }
    return oss.str();
}

/**
 * @brief Create a Redis cluster node
 * 
 * @param node_index Index of the node
 * @return Created node view
 */
libgossip::node_view create_redis_node(size_t node_index) {
    libgossip::node_view node;
    node.id = generate_random_node_id();
    node.ip = "127.0.0.1";
    node.port = static_cast<uint16_t>(BASE_PORT + node_index);
    node.config_epoch = 1;
    node.heartbeat = 0;
    node.version = 1;
    node.status = libgossip::node_status::joining;
    node.role = (node_index % 2 == 0) ? "master" : "slave";
    node.region = "datacenter-1";

    // Add Redis-specific metadata
    node.metadata["redis_version"] = "6.2.0";
    node.metadata["role"] = node.role;
    node.metadata["created_at"] = std::to_string(std::time(nullptr));
    node.metadata["node_index"] = std::to_string(node_index);

    return node;
}

/**
 * @brief Print cluster statistics
 */
void print_statistics() {
    std::cout << "\n=== Redis Cluster Statistics ===" << std::endl;
    std::cout << "Total Messages Sent:     " << g_stats.total_messages_sent.load() << std::endl;
    std::cout << "Total Messages Received: " << g_stats.total_messages_received.load() << std::endl;
    std::cout << "Total Gossip Cycles:     " << g_stats.total_gossip_cycles.load() << std::endl;
    std::cout << "Node Join Events:        " << g_stats.node_join_events.load() << std::endl;
    std::cout << "Node Leave Events:       " << g_stats.node_leave_events.load() << std::endl;
    std::cout << "Node Failures:           " << g_stats.node_failures.load() << std::endl;
    std::cout << "Node Recoveries:         " << g_stats.node_recoveries.load() << std::endl;
    std::cout << "Slot Migrations:         " << g_stats.slot_migrations.load() << std::endl;
    std::cout << "================================" << std::endl;
}

/**
 * @brief Print node status
 * 
 * @param node Node to print status for
 */
void print_node_status(const libgossip::node_view &node) {
    std::cout << "Node " << node_id_to_string(node.id)
              << " (" << node.ip << ":" << node.port << ")"
              << " Role: " << node.role
              << " Status: " << libgossip::enum_to_string(node.status).data()
              << " Heartbeat: " << node.heartbeat
              << std::endl;
}

// ========================================================================
// Redis Cluster Node
// ========================================================================

// Global node registry for message routing
std::map<uint16_t, redis_cluster_node *> g_node_registry;

/**
 * @brief Event callback function for Redis cluster nodes
 * 
 * @param node The node that changed status
 * @param old_status The previous status of the node
 */
void redis_node_event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Redis Event] Node " << node_id_to_string(node.id)
              << " changed from status " << libgossip::enum_to_string(old_status).data()
              << " to " << libgossip::enum_to_string(node.status).data() << std::endl;

    switch (node.status) {
        case libgossip::node_status::online:
            if (old_status == libgossip::node_status::joining ||
                old_status == libgossip::node_status::unknown) {
                g_stats.node_join_events++;
                std::cout << "[Stat] Node join event incremented, total: "
                          << g_stats.node_join_events.load() << std::endl;
            } else if (old_status == libgossip::node_status::failed) {
                g_stats.node_recoveries++;
                std::cout << "[Stat] Node recovery event incremented, total: "
                          << g_stats.node_recoveries.load() << std::endl;
            }
            break;

        case libgossip::node_status::failed:
            g_stats.node_failures++;
            std::cout << "[Stat] Node failure event incremented, total: "
                      << g_stats.node_failures.load() << std::endl;
            break;

        case libgossip::node_status::unknown:
            if (old_status == libgossip::node_status::online) {
                g_stats.node_leave_events++;
                std::cout << "[Stat] Node leave event incremented, total: "
                          << g_stats.node_leave_events.load() << std::endl;
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Redis cluster node class
 * 
 * This class represents a single node in a Redis cluster, implementing
 * the gossip-based discovery and failure detection mechanisms.
 */
class redis_cluster_node {
public:
    /**
     * @brief Constructor
     * 
     * @param index Node index
     */
    explicit redis_cluster_node(size_t index)
        : m_index(index), m_node_info(create_redis_node(index)), m_core(std::make_shared<libgossip::gossip_core>(m_node_info, [this](const libgossip::gossip_message &msg, const libgossip::node_view &target) { this->send_callback(msg, target); }, redis_node_event_callback)), m_serializer(std::make_unique<gossip::net::json_serializer>()), m_transport(std::make_unique<gossip::net::tcp_transport>(m_node_info.ip, m_node_info.port)) {

        // Register node in global registry for message routing
        g_node_registry[m_node_info.port] = this;

        // Configure transport
        m_transport->set_gossip_core(m_core);
        m_transport->set_serializer(std::move(m_serializer));

        // Assign hash slots if this is a master node
        if (m_node_info.role == "master") {
            assign_hash_slots();
        }

        std::cout << "Created Redis node " << m_index
                  << " at " << m_node_info.ip << ":" << m_node_info.port
                  << " Role: " << m_node_info.role
                  << std::endl;
    }

    /**
     * @brief Destructor
     */
    ~redis_cluster_node() {
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
                // Sleep to simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        // Broadcast a join message to announce our presence
        broadcast_join_message();

        std::cout << "Started Redis node " << m_index << std::endl;
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

        std::cout << "Stopped Redis node " << m_index << std::endl;
        return gossip::net::error_code::success;
    }

    /**
     * @brief Perform periodic operations
     */
    void perform_periodic_operations() {
        // Perform gossip cycle
        m_core->tick();
        g_stats.total_gossip_cycles++;

        // Randomly send updates or perform other operations
        std::uniform_real_distribution<double> op_dis(0.0, 1.0);
        double op_choice = op_dis(g_gen);

        if (op_choice < 0.2) {
            // Send update message
            send_update_message();
        } else if (op_choice < 0.3) {
            // Broadcast message
            broadcast_message();
        } else if (op_choice < 0.4) {
            // Simulate slot migration
            simulate_slot_migration();
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
    void set_manager_nodes(const std::vector<std::unique_ptr<redis_cluster_node>> &nodes) {
        // Store pointer references to all nodes in the cluster
        m_manager_nodes.reserve(nodes.size());
        for (const auto &node: nodes) {
            m_manager_nodes.push_back(node.get());
        }
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
            // Use receive_message to ensure proper message counting
            it->second->receive_message(msg);
        } else {
            std::cerr << "[Warning] Unknown target port: " << target.port << std::endl;
        }
    }

    /**
     * @brief Receive a message from another node
     * 
     * @param msg The message to receive
     */
    void receive_message(const libgossip::gossip_message &msg) {
        // Increment global received messages counter
        g_stats.total_messages_received++;
        
        // Use current time as seen_time instead of converting timestamp
        // The timestamp in the message is not a steady_clock time_point
        auto now = libgossip::clock::now();
        m_core->handle_message(msg, now);
        // Note: received_messages counter is incremented inside handle_message
    }

    /**
     * @brief Assign hash slots to master nodes
     */
    void assign_hash_slots() {
        if (m_node_info.role != "master") {
            return;
        }

        // Simple slot assignment - divide slots equally among master nodes
        size_t master_count = CLUSTER_SIZE / 2;// Assuming half are masters
        size_t slots_per_node = HASH_SLOTS / master_count;
        size_t start_slot = (m_index / 2) * slots_per_node;
        size_t end_slot = std::min(start_slot + slots_per_node, HASH_SLOTS);

        // Add Redis-specific metadata about slots
        m_node_info.metadata["slots"] = std::to_string(start_slot) + "-" + std::to_string(end_slot);

        std::cout << "Assigned slots " << start_slot << "-" << end_slot
                  << " to master node " << m_index << std::endl;
    }

    /**
     * @brief Send an update message
     */
    void send_update_message() {
        libgossip::gossip_message msg;
        msg.sender = m_node_info.id;
        msg.type = libgossip::message_type::update;

        // Use steady clock for consistency
        auto now = libgossip::clock::now();
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now.time_since_epoch())
                                .count();
        libgossip::node_view self = m_core->self();
        // Add some metadata
        libgossip::node_view self_node = m_node_info;
        self_node.metadata["last_update"] = std::to_string(msg.timestamp);
        self_node.metadata["operation"] = "periodic_update";
        msg.entries.push_back(self_node);

        // Send message to a random node to simulate real network communication
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
            m_core->handle_message(msg, now);
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

        // Use steady clock for consistency
        auto now = libgossip::clock::now();
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now.time_since_epoch())
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
            m_core->handle_message(msg, now);
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
        // 使用 gossip_core 内部的最新状态
        auto self = m_core->self();
        msg.sender = self.id;
        msg.type = libgossip::message_type::join;
        
        auto now = libgossip::clock::now();
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()).count();

        msg.entries.push_back(self);

        for (auto* node : m_manager_nodes) {
            if (node != this) {
                g_node_registry[node->get_node_info().port]->receive_message(msg);
                g_stats.total_messages_sent++;
            }
        }

        std::cout << "[Node " << m_index << "] Broadcast join message to " 
                  << (m_manager_nodes.size() - 1) << " nodes" << std::endl;
    }

    /**
     * @brief Broadcast a failure message for this node
     */
    void broadcast_failure() {
        libgossip::gossip_message msg;
        auto self = m_core->self();
        msg.sender = self.id;
        msg.type = libgossip::message_type::leave;  // Changed from update to leave for proper handling
        
        auto now = libgossip::clock::now();
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()).count();

        libgossip::node_view failure_view = self;
        failure_view.status = libgossip::node_status::failed;
        failure_view.heartbeat = self.heartbeat + 1;  
        failure_view.config_epoch = self.config_epoch + 1;  

        msg.entries.push_back(failure_view);

        for (auto* node : m_manager_nodes) {
            if (node != this) {
                node->receive_message(msg);
                g_stats.total_messages_sent++;
            }
        }

        std::cout << "[Node " << m_index << "] Broadcasted failure message (hb=" 
                  << failure_view.heartbeat << ", epoch=" << failure_view.config_epoch << ")" << std::endl;
    }

    /**
     * @brief Broadcast a recovery message for this node
     */
    void broadcast_recovery() {
        libgossip::gossip_message msg;
        auto self = m_core->self();
        msg.sender = self.id;
        msg.type = libgossip::message_type::join;  // Changed from update to join for proper handling
        
        auto now = libgossip::clock::now();
        msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()).count();

        libgossip::node_view recovery_view = self;
        recovery_view.status = libgossip::node_status::online;
        recovery_view.heartbeat = self.heartbeat + 1;  
        recovery_view.config_epoch = self.config_epoch + 1; 

        msg.entries.push_back(recovery_view);

        for (auto* node : m_manager_nodes) {
            if (node != this) {
                node->receive_message(msg);
                g_stats.total_messages_sent++;
            }
        }

        std::cout << "[Node " << m_index << "] Broadcasted recovery message (hb=" 
                  << recovery_view.heartbeat << ", epoch=" << recovery_view.config_epoch << ")" << std::endl;
    }

    /**
     * @brief Simulate slot migration
     */
    void simulate_slot_migration() {
        if (m_node_info.role != "master") {
            return;// Only masters can migrate slots
        }

        // Simulate slot migration by updating metadata
        m_node_info.metadata["last_migration"] = std::to_string(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());

        g_stats.slot_migrations++;
        std::cout << "[Node " << m_index << "] Simulated slot migration" << std::endl;
    }

    /**
     * @brief Simulate node issues (failures/recoveries)
     */
    void simulate_node_issues() {
        std::uniform_real_distribution<double> fail_dis(0.0, 1.0);
        double fail_chance = fail_dis(g_gen);

        auto self_status = m_core->self().status;
        
        std::cout << "[DEBUG] Node " << m_index << " status: " 
                  << libgossip::enum_to_string(self_status).data()
                  << " (heartbeat: " << m_core->self().heartbeat 
                  << ", epoch: " << m_core->self().config_epoch << ")" << std::endl;

        if (self_status == libgossip::node_status::online && fail_chance < NODE_FAILURE_PROBABILITY) {
            std::cout << "[Node " << m_index << "] Simulated failure" << std::endl;
            broadcast_failure();
        } 
        else if (self_status == libgossip::node_status::failed && fail_chance < NODE_RECOVERY_PROBABILITY) {
            std::cout << "[Node " << m_index << "] Simulated recovery" << std::endl;
            broadcast_recovery();
        }
    }

    /// Node index
    size_t m_index;

    /// Node information
    libgossip::node_view m_node_info;

    /// Gossip core
    std::shared_ptr<libgossip::gossip_core> m_core;

    /// Transport
    std::unique_ptr<gossip::net::tcp_transport> m_transport;

    /// Serializer
    std::unique_ptr<gossip::net::message_serializer> m_serializer;

    /// Transport thread
    std::thread m_transport_thread;

    /// References to all nodes in the cluster for message sending
    std::vector<redis_cluster_node *> m_manager_nodes;
};

// ========================================================================
// Redis Cluster Manager
// ========================================================================

/**
 * @brief Redis cluster manager class
 * 
 * This class manages the entire Redis cluster, including node creation,
 * inter-node communication, and cluster-wide operations.
 */
class redis_cluster_manager {
public:
    /**
     * @brief Constructor
     * 
     * @param size Number of nodes in the cluster
     */
    explicit redis_cluster_manager(size_t size) : m_size(size) {
        m_nodes.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            m_nodes.emplace_back(std::make_unique<redis_cluster_node>(i));
        }
    }

    /**
     * @brief Initialize the cluster
     * 
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize() {
        std::cout << "Initializing Redis cluster with " << m_size << " nodes..." << std::endl;

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
        // Create a more connected network topology
        if (!m_nodes.empty()) {
            // Connect each node to a few random nodes to create a mesh-like topology
            std::uniform_int_distribution<size_t> node_dis(0, m_nodes.size() - 1);

            for (size_t i = 0; i < m_nodes.size(); ++i) {
                auto &node = m_nodes[i];

                // Meet with 2-3 random nodes
                size_t connections = 2 + (i % 2);
                for (size_t j = 0; j < connections; ++j) {
                    size_t target_idx = node_dis(g_gen);
                    // Don't meet with self
                    if (target_idx != i) {
                        auto &target_node = m_nodes[target_idx];
                        node->meet(target_node->get_node_info());
                        target_node->meet(node->get_node_info());
                    }
                }
            }

            // Additionally, create a fully connected topology for better initial convergence
            for (size_t i = 0; i < m_nodes.size(); ++i) {
                for (size_t j = i + 1; j < m_nodes.size(); ++j) {
                    m_nodes[i]->meet(m_nodes[j]->get_node_info());
                    m_nodes[j]->meet(m_nodes[i]->get_node_info());
                }
            }
        }

        std::cout << "Redis cluster initialized successfully" << std::endl;
        return true;
    }

    /**
     * @brief Run the cluster simulation
     * 
     * @param duration Duration to run the simulation in seconds
     */
    void run_simulation(int duration) {
        std::cout << "Running Redis cluster simulation for " << duration << " seconds..." << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration);

        while (g_running && std::chrono::steady_clock::now() < end_time) {
            // Perform periodic operations on all nodes
            for (auto &node: m_nodes) {
                node->perform_periodic_operations();
            }

            // Print statistics periodically
            static int counter = 0;
            if (++counter % 10 == 0) {
                print_statistics();

                // Print cluster status
                std::cout << "\n=== Cluster Status ===" << std::endl;
                for (const auto &node: m_nodes) {
                    print_node_status(node->get_node_info());
                }
                std::cout << "=====================" << std::endl;
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
        std::cout << "Shutting down Redis cluster..." << std::endl;

        g_running = false;

        // Stop all nodes
        for (auto &node: m_nodes) {
            node->stop();
        }

        std::cout << "Redis cluster shutdown completed" << std::endl;
    }

    /**
     * @brief Print current cluster status
     */
    void print_cluster_status() {
        std::cout << "\n=== Redis Cluster Status ===" << std::endl;
        for (const auto &node: m_nodes) {
            print_node_status(node->get_node_info());
        }
        std::cout << "===========================" << std::endl;
    }

private:
    /// Cluster size
    size_t m_size;

    /// Nodes in the cluster
    std::vector<std::unique_ptr<redis_cluster_node>> m_nodes;
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
 * @brief Main function demonstrating Redis cluster gossip operations
 * 
 * This function demonstrates:
 * 1. Creating a Redis-style cluster of nodes
 * 2. Using TCP transport for communication
 * 3. Simulating Redis cluster operations like slot migration
 * 4. Handling node failures and recoveries
 * 5. Monitoring cluster statistics
 * 6. Graceful shutdown
 */
int main() {
    std::cout << "libgossip Redis Cluster Gossip Example" << std::endl;
    std::cout << "=====================================" << std::endl;

    try {
        // Create cluster manager
        redis_cluster_manager manager(CLUSTER_SIZE);

        // Initialize cluster
        if (!manager.initialize()) {
            std::cerr << "Failed to initialize Redis cluster" << std::endl;
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

    std::cout << "Redis cluster gossip example completed successfully" << std::endl;
    return 0;
}
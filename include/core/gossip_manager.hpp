/**
 * @file gossip_manager.hpp
 * @brief High-level GossipManager class for easy libgossip integration
 *
 * This header provides a convenient wrapper around libgossip's core functionality,
 * handling lifecycle management, network transport, and event callbacks.
 *
 * Usage:
 * @code
 *   gossip_manager manager;
 *
 *   gossip_config config;
 *   config.bind_ip = "0.0.0.0";
 *   config.gossip_port = 7946;
 *   config.role = "master";
 *
 *   if (!manager.init(config)) {
 *       // Handle error
 *   }
 *
 *   manager.set_event_callback([](const node_view& node,
 *                                  node_status old_status,
 *                                  node_status new_status) {
 *       std::cout << "Node " << node_id_to_string(node.id)
 *                 << " changed from " << static_cast<int>(old_status)
 *                 << " to " << static_cast<int>(new_status) << std::endl;
 *   });
 *
 *   manager.start();
 *
 *   // In your main loop:
 *   while (running) {
 *       manager.tick();
 *       std::this_thread::sleep_for(std::chrono::milliseconds(100));
 *   }
 *
 *   manager.stop();
 * @endcode
 */

#pragma once

#include "config.hpp"
#include "gossip_config.hpp"
#include "gossip_core.hpp"
#include "net/udp_transport.hpp"
#include "node_id_utils.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace libgossip {

/**
 * @brief Event types for cluster events
 */
enum class cluster_event : uint8_t {
    node_joined,     ///< New node joined the cluster
    node_left,       ///< Node left gracefully
    node_failed,     ///< Node failure detected
    node_recovered,  ///< Node recovered from suspect state
    metadata_changed ///< Node metadata changed
};

/**
 * @brief Event callback type
 *
 * Parameters:
 * - const node_view&: The node that changed
 * - node_status: Previous status
 * - node_status: New status
 */
using cluster_event_callback = std::function<void(const node_view&, node_status, node_status)>;

/**
 * @brief High-level wrapper for libgossip
 *
 * GossipManager simplifies the usage of libgossip by handling:
 * - Lifecycle management (init, start, stop)
 * - Network transport setup
 * - Event callback dispatch
 * - Node management (meet, join, leave)
 * - Metadata propagation
 * - Statistics collection
 */
class LIBGOSSIP_API gossip_manager {
public:
    gossip_manager() noexcept = default;
    ~gossip_manager() noexcept;

    // Non-copyable, non-movable
    gossip_manager(const gossip_manager&) = delete;
    gossip_manager& operator=(const gossip_manager&) = delete;
    gossip_manager(gossip_manager&&) noexcept = delete;
    gossip_manager& operator=(gossip_manager&&) noexcept = delete;

    /**
     * @brief Initialize the gossip manager
     *
     * @param config Configuration parameters
     * @return true if initialization succeeded, false otherwise
     */
    bool init(const gossip_config& config) noexcept;

    /**
     * @brief Start the gossip service
     *
     * @return true if started successfully, false otherwise
     */
    bool start() noexcept;

    /**
     * @brief Stop the gossip service
     */
    void stop() noexcept;

    /**
     * @brief Drive one gossip tick
     *
     * Should be called periodically (e.g., every 100ms).
     */
    void tick() noexcept;

    /**
     * @brief Full broadcast for critical config changes
     *
     * Sends gossip messages to all known nodes immediately.
     */
    void broadcast_config() noexcept;

    // ========== Node Management ==========

    /**
     * @brief Meet a new node (add to cluster)
     *
     * @param ip Node IP address
     * @param port Node gossip port
     * @return true if meet message sent successfully
     */
    bool meet_node(std::string_view ip, uint16_t port) noexcept;

    /**
     * @brief Join an existing cluster through a known node
     *
     * @param ip Known node IP address
     * @param port Known node gossip port
     * @return true if join message sent successfully
     */
    bool join_cluster(std::string_view ip, uint16_t port) noexcept;

    /**
     * @brief Leave the cluster gracefully
     */
    void leave_cluster() noexcept;

    // ========== Query ==========

    /**
     * @brief Get all known nodes
     */
    std::vector<node_view> get_nodes() const noexcept;

    /**
     * @brief Get the number of known nodes
     */
    size_t get_node_count() const noexcept;

    /**
     * @brief Find a node by ID
     *
     * @param id Node ID to find
     * @return The node view if found, std::nullopt otherwise
     */
    std::optional<node_view> find_node(const node_id_t& id) const noexcept;

    /**
     * @brief Get self node view
     */
    node_view get_self() const noexcept;

    // ========== Metadata ==========

    /**
     * @brief Update self metadata
     *
     * Thread-safe: can be called from any thread.
     *
     * @param metadata Key-value pairs to update
     */
    void update_metadata(const std::map<std::string, std::string>& metadata) noexcept;

    // ========== Events ==========

    /**
     * @brief Set the event callback
     *
     * @param callback Function to call on cluster events
     */
    void set_event_callback(cluster_event_callback callback) noexcept;

    // ========== Statistics ==========

    /**
     * @brief Gossip statistics
     */
    struct stats {
        size_t known_nodes = 0;
        size_t sent_messages = 0;
        size_t received_messages = 0;
        int64_t last_tick_duration_ms = 0;
    };

    /**
     * @brief Get current statistics
     */
    stats get_stats() const noexcept;

    /**
     * @brief Check if the manager is initialized
     */
    bool is_initialized() const noexcept { return initialized_.load(std::memory_order_acquire); }

    /**
     * @brief Check if the manager is running
     */
    bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }

private:
    // Internal callbacks
    void on_send_message(const gossip_message& msg, const node_view& target) noexcept;
    void on_node_event(const node_view& node, node_status old_status) noexcept;

    // Configuration
    gossip_config config_;
    node_id_t self_id_{};

    // Core components
    std::shared_ptr<gossip_core> gossip_core_;
    std::unique_ptr<net::transport> transport_;

    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};

    // Event callback
    cluster_event_callback event_callback_;
    std::atomic<bool> event_callback_set_{false};
    mutable std::mutex event_callback_mutex_;
};

} // namespace libgossip

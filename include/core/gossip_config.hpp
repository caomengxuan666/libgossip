/**
 * @file gossip_config.hpp
 * @brief Configuration structure for GossipManager
 *
 * Provides a convenient configuration structure for initializing
 * the GossipManager high-level API.
 */

#pragma once

#include "config.hpp"
#include "node_id_utils.hpp"
#include <chrono>
#include <string>

namespace libgossip {

/**
 * @brief Configuration for GossipManager initialization
 *
 * This structure contains all the configuration parameters needed
 * to initialize a GossipManager instance.
 */
struct gossip_config {
    // Node identification
    std::string node_id;            ///< Hex string node ID (auto-generated if empty)
    std::string bind_ip = "0.0.0.0"; ///< IP address to bind to
    uint16_t gossip_port = 7946;     ///< Gossip protocol port

    // Timing configuration
    uint32_t heartbeat_interval_ms = config::DEFAULT_HEARTBEAT_INTERVAL_MS;
    uint32_t failure_timeout_ms = config::DEFAULT_FAILURE_TIMEOUT_MS;

    // Gossip configuration
    int gossip_nodes = config::DEFAULT_GOSSIP_NODES;  ///< Number of nodes to gossip with per tick
    int sync_nodes = config::DEFAULT_SYNC_NODES;      ///< Number of nodes to sync per message

    // Transport configuration
    bool use_tcp = false;              ///< false = UDP, true = TCP
    std::string serializer = "json";   ///< Serializer name (default: "json")

    // Node metadata
    std::string role = "master";       ///< Node role ("master", "replica")
    std::string region;                ///< Geographic region (e.g., "us-east-1")
};

} // namespace libgossip

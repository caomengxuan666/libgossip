//​
//  _ _ _                         _       
// | (_) |__   __ _  ___  ___ ___(_)_ __  
// | | | '_ \ / _` |/ _ \/ __/ __| | '_ \_
// | | | |_) | (_| | (_) \__ \__ \ | |_) |
// |_|_|_.__/ \__, |\___/|___/___/_| .__/ 
//            |___/                |_|    
// Project: libgossip
// Repository: https://github.com/caomengxuan666/libgossip
// Version: 1.1.2
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Caomengxuan.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//​END

/**
 * @file gossip_core.hpp
 * @brief Core implementation of the gossip protocol
 *
 * This file contains the core implementation of the gossip protocol based on SWIM.
 * It provides node membership management, failure detection, and metadata propagation.
 *
 * @author caomengxuan666
 * @date 2025-05-13
 */

#ifndef LIBGOSSIP_CORE_HPP
#define LIBGOSSIP_CORE_HPP

#include "libgossip_api.h"
#include "magic_enum/magic_enum.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace libgossip {

    // ---------------------------------------------------------
    // Basic type definitions
    // ---------------------------------------------------------

    /// Node unique ID, 16 bytes (e.g. UUID or MD5)
    using node_id_t = std::array<uint8_t, 16>;

    /// Time point type (using steady_clock)
    using time_point = std::chrono::steady_clock::time_point;

    /// Millisecond duration
    using duration_ms = std::chrono::milliseconds;

    // ---------------------------------------------------------
    // Node status flags
    // ---------------------------------------------------------

    enum class node_status {
        unknown = 0,
        joining,// Joining
        online, // Online
        suspect,// Suspected offline (timeout)
        failed  // Confirmed offline
    };
    template<typename E>
    auto to_string(E value) -> std::enable_if_t<std::is_enum_v<E>, std::string> {
        return std::string{magic_enum::enum_name(value)};
    }

    // ---------------------------------------------------------
    // Node view: summary information of each node in the cluster
    // ---------------------------------------------------------

    struct node_view {
        node_id_t id{};
        std::string ip;
        int port = 0;
        uint64_t config_epoch = 0;// Configuration version (for master-slave election)
        uint64_t heartbeat = 0;   // Logical heartbeat (incremental sequence number)
        uint64_t version = 0;     // Version number (version++ on each local update)
        time_point seen_time{};   // The last time this node's message was received
        node_status status = node_status::unknown;

        // Business extension fields
        std::string role;  // "master", "replica"
        std::string region;// "us-east-1"
        std::map<std::string, std::string> metadata;

        // Suspicion mechanism fields
        int suspicion_count = 0;
        time_point last_suspected{};

        // Comparison operators
        bool operator==(const node_view &other) const noexcept {
            return id == other.id &&
                   ip == other.ip &&
                   port == other.port &&
                   config_epoch == other.config_epoch &&
                   heartbeat == other.heartbeat &&
                   version == other.version &&
                   seen_time == other.seen_time &&
                   status == other.status &&
                   role == other.role &&
                   region == other.region &&
                   metadata == other.metadata &&
                   suspicion_count == other.suspicion_count &&
                   last_suspected == other.last_suspected;
        }

        bool operator!=(const node_view &other) const noexcept {
            return !(*this == other);
        }

        // Comparison: used to determine if an update is needed
        bool newer_than(const node_view &other) const noexcept;

        // For master-slave failover
        bool can_replace(const node_view &other) const noexcept;
    };

    // ---------------------------------------------------------
    // Gossip message types
    // ---------------------------------------------------------

    enum class message_type : uint8_t {
        ping = 0,
        pong,
        meet,
        join, // Explicit join
        leave,// Explicit leave
        update
    };


    // ---------------------------------------------------------
    // Gossip message: used for information exchange between nodes
    // ---------------------------------------------------------

    struct gossip_message {
        node_id_t sender{};
        message_type type = message_type::ping;
        uint64_t timestamp = 0;        // Usually the sender's heartbeat
        std::vector<node_view> entries;// Carried node information (0~N nodes)

        // Comparison operators
        bool operator==(const gossip_message &other) const noexcept {
            return sender == other.sender &&
                   type == other.type &&
                   timestamp == other.timestamp &&
                   entries == other.entries;
        }

        bool operator!=(const gossip_message &other) const noexcept {
            return !(*this == other);
        }
    };

    // ---------------------------------------------------------
    // Callback function types
    // ---------------------------------------------------------

    /// Send message callback: core requests to send a message to target node
    using send_callback = std::function<void(const gossip_message &, const node_view &target)>;

    /// Event notification callback: node status changes
    using event_callback = std::function<void(const node_view &, node_status old_status)>;

    // ---------------------------------------------------------
    // Statistics
    // ---------------------------------------------------------

    struct gossip_stats {
        size_t known_nodes = 0;
        size_t sent_messages = 0;
        size_t received_messages = 0;
        duration_ms last_tick_duration = duration_ms(0);
    };

    // ---------------------------------------------------------
    // Clock policy (replaceable, for testing)
    // ---------------------------------------------------------

    struct clock {
        static time_point now() {
            return std::chrono::steady_clock::now();
        }
    };

    // ---------------------------------------------------------
    // Gossip core class
    // Thread safety must be guaranteed by upper layer (single-threaded driver model)
    // ---------------------------------------------------------

    class LIBGOSSIP_API gossip_core {
    public:
        /// Constructor
        explicit gossip_core(node_view self,
                             send_callback sender,
                             event_callback event_handler);

        /// Destructor
        ~gossip_core() = default;

        // Disable copy, enable move
        gossip_core(const gossip_core &) = delete;
        gossip_core &operator=(const gossip_core &) = delete;
        gossip_core(gossip_core &&) noexcept = default;
        gossip_core &operator=(gossip_core &&) noexcept = default;

    public:
        // ---------------------------------------------------------
        // Interface API
        // ---------------------------------------------------------

        /// Drive one gossip cycle (recommended to call every 100ms)
        void tick();

        /// Drive a complete broadcast gossip cycle (for rapid propagation of critical configurations)
        void tick_full_broadcast();

        /// Process a received gossip message
        /// @param msg Received message
        /// @param recv_time Local time when the message arrived (for failure detection)
        /// @note Thread unsafe, upper layer must guarantee single-threaded call
        void handle_message(const gossip_message &msg, time_point recv_time);

        /// Actively initiate join: introduce a new node (equivalent to MEET command)
        void meet(const node_view &node);

        /// Explicitly join the cluster
        void join(const node_view &node);

        /// Explicitly leave the cluster (graceful exit)
        void leave(const node_id_t &node_id);

        /// Get self node view
        const node_view &self() const noexcept { return self_; }

        /// Get all currently known nodes (excluding self)
        std::vector<node_view> get_nodes() const;

        /// Find node by ID
        std::optional<node_view> find_node(const node_id_t &id) const;

        /// Get node count
        size_t size() const noexcept { return nodes_.size(); }

        /// Clean up expired nodes (optional call)
        void cleanup_expired(duration_ms timeout);

        /// Reset core state (for testing or restart)
        void reset();

        /// Get statistics
        gossip_stats get_stats() const;

    private:
        // ---------------------------------------------------------
        // Private methods
        // ---------------------------------------------------------

        /// Randomly select up to k nodes (excluding self and optional exclude)
        std::vector<node_view> select_random_peers(int k, const node_id_t *exclude = nullptr) const;

        /// Update local perception of a node
        node_view &update_node(const node_view &remote, time_point seen_time);

        /// Trigger event
        void notify(const node_view &node, node_status old_status);

        node_status old_status_of(node_view &current, const gossip_message &msg);

    private:
        node_view self_;
        std::list<node_view> nodes_;// All known nodes
        send_callback send_fn_;
        event_callback event_fn_;

        duration_ms heartbeat_interval_ = std::chrono::milliseconds(100);
        duration_ms failure_timeout_ = std::chrono::milliseconds(2000);// 2s without update → suspect
        int gossip_nodes_ = 3;                                         // Send gossip to 3 random nodes each time
        int sync_nodes_ = 2;                                           // Carry 2 other node information each time

        // Statistics
        size_t sent_messages_ = 0;
        size_t received_messages_ = 0;
    };

}// namespace libgossip

#include "enum_reflection.inl"

#endif

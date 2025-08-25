// src/core/gossip.cpp
#include "core/gossip_core.hpp"
#include <algorithm>
#include <random>
#include <thread>


namespace libgossip {

    // ---------------------------------------------------------
    // node_view member functions
    // ---------------------------------------------------------

    bool node_view::newer_than(const node_view &other) const noexcept {
        if (heartbeat > other.heartbeat) return true;
        if (heartbeat < other.heartbeat) return false;
        return config_epoch > other.config_epoch;
    }

    bool node_view::can_replace(const node_view &other) const noexcept {
        if (config_epoch > other.config_epoch) return true;
        if (config_epoch < other.config_epoch) return false;
        return heartbeat > other.heartbeat;// epoch is the same, higher heartbeat wins
    }

    // ---------------------------------------------------------
    // gossip_core member function implementations
    // ---------------------------------------------------------

    gossip_core::gossip_core(node_view self, send_callback sender, event_callback event_handler)
        : self_(std::move(self)), send_fn_(std::move(sender)), event_fn_(std::move(event_handler)) {
        if (!send_fn_) {
            throw std::invalid_argument("send_callback cannot be null");
        }
        self_.status = node_status::online;
        self_.seen_time = clock::now();// Initialize
    }

    void gossip_core::tick() {
        auto start_time = clock::now();
        self_.seen_time = start_time;

        // Step 1: Randomly select target nodes and send PING
        auto targets = select_random_peers(gossip_nodes_, &self_.id);
        for (const auto &target: targets) {
            gossip_message msg;
            msg.sender = self_.id;
            msg.type = message_type::ping;
            msg.timestamp = self_.heartbeat;

            // Carry self + additional nodes (anti-entropy)
            msg.entries.clear();
            msg.entries.push_back(self_);
            auto extras = select_random_peers(sync_nodes_, &target.id);
            msg.entries.insert(msg.entries.end(), extras.begin(), extras.end());

            send_fn_(msg, target);
            sent_messages_++;
        }

        // Step 2: Increment heartbeat
        self_.heartbeat++;
        self_.version++;

        // Step 3: Failure detection
        for (auto &node: nodes_) {
            if (node.status == node_status::online) {
                auto elapsed = std::chrono::duration_cast<duration_ms>(clock::now() - node.seen_time);
                if (elapsed >= failure_timeout_) {
                    auto old = node.status;
                    node.status = node_status::suspect;
                    node.suspicion_count++;
                    node.last_suspected = clock::now();
                    notify(node, old);
                }
            } else if (node.status == node_status::suspect) {
                // Add suspicion count logic
                auto elapsed = std::chrono::duration_cast<duration_ms>(clock::now() - node.last_suspected);
                if (elapsed >= failure_timeout_) {
                    node.suspicion_count++;
                    node.last_suspected = clock::now();

                    // If suspicion count exceeds threshold, mark as failed
                    if (node.suspicion_count > 3) {
                        auto old = node.status;
                        node.status = node_status::failed;
                        notify(node, old);
                    }
                }
            }
        }

        // Record tick duration
        auto end_time = clock::now();
        // Here we just recorded the time, but didn't store it, because we need to return it in get_stats
    }

    void gossip_core::tick_full_broadcast() {
        self_.seen_time = clock::now();

        // Send ping message to all online nodes
        for (const auto &node: nodes_) {
            if (node.status == node_status::online) {
                gossip_message msg;
                msg.sender = self_.id;
                msg.type = message_type::ping;
                msg.timestamp = self_.heartbeat;

                // Carry self + additional nodes (anti-entropy)
                msg.entries.clear();
                msg.entries.push_back(self_);
                auto extras = select_random_peers(sync_nodes_, &node.id);
                msg.entries.insert(msg.entries.end(), extras.begin(), extras.end());

                send_fn_(msg, node);
                sent_messages_++;
            }
        }

        // Increment heartbeat
        self_.heartbeat++;
        self_.version++;
    }

    void gossip_core::handle_message(const gossip_message &msg, time_point recv_time) {
        received_messages_++;
        node_view *sender = nullptr;

        // First find sender in locally known nodes
        for (auto &node: nodes_) {
            if (node.id == msg.sender) {
                sender = &node;
                break;
            }
        }

        // If sender is unknown, try to find from entries (used for MEET/JOIN)
        if (!sender && (msg.type == message_type::meet || msg.type == message_type::join) && !msg.entries.empty()) {
            for (const auto &entry: msg.entries) {
                if (entry.id == msg.sender) {
                    sender = &update_node(entry, recv_time);
                    break;
                }
            }
        }

        if (!sender && msg.type != message_type::meet && msg.type != message_type::join) {
            return;// Not MEET/JOIN and not recognized, discard
        }

        // Update sender's status
        if (sender) {
            auto old_status = sender->status;
            if (msg.timestamp > sender->heartbeat) {
                sender->heartbeat = msg.timestamp;
            }
            sender->seen_time = recv_time;
            sender->version++;

            // Reset suspicion count, because we received a message from the node
            if (sender->status == node_status::suspect) {
                sender->suspicion_count = 0;
            }

            if (sender->status == node_status::joining) {
                sender->status = node_status::online;
                notify(*sender, old_status);
            }

            // Handle leave message
            if (msg.type == message_type::leave) {
                if (sender->status != node_status::failed) {
                    sender->status = node_status::failed;
                    notify(*sender, old_status);
                }
            }
        }

        // Handle entries (containing node information carried by the other party)
        for (const auto &remote: msg.entries) {
            update_node(remote, recv_time);
        }

        // Reply PONG
        if ((msg.type == message_type::ping || msg.type == message_type::meet || msg.type == message_type::join) && sender) {
            gossip_message pong;
            pong.sender = self_.id;
            pong.type = message_type::pong;
            pong.timestamp = self_.heartbeat;

            pong.entries.clear();
            pong.entries.push_back(self_);// Bring yourself
            auto extras = select_random_peers(sync_nodes_, &msg.sender);
            pong.entries.insert(pong.entries.end(), extras.begin(), extras.end());

            send_fn_(pong, *sender);
            sent_messages_++;
        }
    }


    void gossip_core::meet(const node_view &node) {
        if (node.id == self_.id) return;

        // Record locally
        auto it = std::find_if(nodes_.begin(), nodes_.end(),
                               [&node](const node_view &n) { return n.id == node.id; });
        if (it == nodes_.end()) {
            node_view nv = node;
            nv.status = node_status::joining;
            nv.seen_time = clock::now();
            nodes_.push_back(nv);
            notify(nv, node_status::unknown);
        }

        // Proactively send MEET message to tell the other party about yourself
        gossip_message msg;
        msg.sender = self_.id;
        msg.type = message_type::meet;
        msg.timestamp = self_.heartbeat;
        msg.entries.push_back(self_);// Bring yourself
        send_fn_(msg, node);
        sent_messages_++;
    }

    void gossip_core::join(const node_view &node) {
        if (node.id == self_.id) return;

        // Record locally
        auto it = std::find_if(nodes_.begin(), nodes_.end(),
                               [&node](const node_view &n) { return n.id == node.id; });
        if (it == nodes_.end()) {
            node_view nv = node;
            nv.status = node_status::joining;
            nv.seen_time = clock::now();
            nodes_.push_back(nv);
            notify(nv, node_status::unknown);
        }

        // Proactively send JOIN message to tell the other party about yourself
        gossip_message msg;
        msg.sender = self_.id;
        msg.type = message_type::join;
        msg.timestamp = self_.heartbeat;
        msg.entries.push_back(self_);// Bring yourself
        send_fn_(msg, node);
        sent_messages_++;
    }

    void gossip_core::leave(const node_id_t &node_id) {
        auto it = std::find_if(nodes_.begin(), nodes_.end(),
                               [&node_id](const node_view &n) { return n.id == node_id; });
        if (it != nodes_.end()) {
            // Notify other nodes that this node has left
            gossip_message msg;
            msg.sender = self_.id;
            msg.type = message_type::leave;
            msg.timestamp = self_.heartbeat;
            msg.entries.push_back(*it);// Bring information of the leaving node

            // Send to all online nodes
            for (const auto &node: nodes_) {
                if (node.status == node_status::online && node.id != node_id) {
                    send_fn_(msg, node);
                    sent_messages_++;
                }
            }

            // Update local status
            auto old_status = it->status;
            it->status = node_status::failed;
            notify(*it, old_status);
        }
    }


    std::vector<node_view> gossip_core::get_nodes() const {
        std::vector<node_view> result;
        result.reserve(nodes_.size());
        for (const auto &node: nodes_) {
            result.push_back(node);
        }
        return result;
    }

    std::optional<node_view> gossip_core::find_node(const node_id_t &id) const {
        if (id == self_.id) {
            return self_;
        }
        for (const auto &node: nodes_) {
            if (node.id == id) {
                return node;
            }
        }
        return std::nullopt;
    }

    std::vector<node_view> gossip_core::select_random_peers(int k, const node_id_t *exclude) const {
        std::vector<node_view> candidates;
        std::copy_if(nodes_.begin(), nodes_.end(), std::back_inserter(candidates),
                     [exclude](const node_view &n) {
                         return exclude ? (n.id != *exclude) : true;
                     });

        if (candidates.empty()) return {};

        // High entropy random source
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::generate(seed_data.begin(), seed_data.end(), [&rd]() { return rd(); });
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        std::mt19937 gen(seq);

        // Perturbation
        std::hash<std::thread::id> hash_thread;
        gen.discard(1 + (hash_thread(std::this_thread::get_id()) ^
                         static_cast<size_t>(clock::now().time_since_epoch().count())) %
                                100);

        std::shuffle(candidates.begin(), candidates.end(), gen);

        int n = std::min(k, static_cast<int>(candidates.size()));
        return std::vector<node_view>(candidates.begin(), candidates.begin() + n);
    }

    node_view &gossip_core::update_node(const node_view &remote, time_point seen_time) {
        auto it = std::find_if(nodes_.begin(), nodes_.end(),
                               [&remote](const node_view &n) { return n.id == remote.id; });

        if (it == nodes_.end()) {
            node_view nv = remote;
            nv.seen_time = seen_time;

            // Avoid UNKNOWN → UNKNOWN
            if (nv.status == node_status::unknown) {
                nv.status = node_status::joining;
            }

            nodes_.push_back(nv);
            auto &ref = nodes_.back();
            notify(ref, node_status::unknown);
            return ref;
        } else {
            auto old_status = it->status;
            // Use can_replace instead of newer_than for more explicit decision
            if (remote.can_replace(*it)) {
                *it = remote;
                it->seen_time = seen_time;
                if (it->status == node_status::unknown) {
                    it->status = node_status::joining;
                }
            }

            if (old_status != it->status) {
                notify(*it, old_status);
            }
            return *it;
        }
    }


    void gossip_core::notify(const node_view &node, node_status old_status) {
        if (event_fn_ && node.status != old_status) {
            event_fn_(node, old_status);
        }
    }

    void gossip_core::cleanup_expired(duration_ms timeout) {
        auto now = clock::now();
        auto expired = [timeout, now](const node_view &n) {
            return (n.status != node_status::online) &&
                   (std::chrono::duration_cast<duration_ms>(now - n.seen_time) > timeout);
        };
        nodes_.remove_if(expired);
    }

    void gossip_core::reset() {
        nodes_.clear();
        self_.heartbeat = 1;
        self_.version = 0;
        self_.seen_time = clock::now();
        sent_messages_ = 0;
        received_messages_ = 0;
    }

    gossip_stats gossip_core::get_stats() const {
        gossip_stats stats;
        stats.known_nodes = nodes_.size();
        stats.sent_messages = sent_messages_;
        stats.received_messages = received_messages_;
        // last_tick_duration cannot be accurately obtained, because we didn't record it in tick
        return stats;
    }

}// namespace libgossip
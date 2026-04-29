/**
 * @file gossip_manager.cpp
 * @brief Implementation of the GossipManager class
 */

#include "core/gossip_manager.hpp"
#include "net/transport_factory.hpp"

#include <iostream>

namespace libgossip {

gossip_manager::~gossip_manager() noexcept {
    stop();
}

bool gossip_manager::init(const gossip_config& config) noexcept {
    if (initialized_.load(std::memory_order_acquire)) {
        return false; // Already initialized
    }

    config_ = config;

    // Parse or generate node ID
    if (!config.node_id.empty()) {
        auto parsed = parse_node_id(config.node_id);
        if (parsed) {
            self_id_ = *parsed;
        } else {
            self_id_ = generate_node_id();
        }
    } else {
        self_id_ = generate_node_id();
    }

    // Create self node view
    node_view self_view;
    self_view.id = self_id_;
    self_view.ip = config.bind_ip;
    self_view.port = config.gossip_port;
    self_view.role = config.role;
    self_view.region = config.region;
    self_view.config_epoch = 1;
    self_view.heartbeat = 1;
    self_view.version = 1;
    self_view.status = node_status::joining;

    // Create gossip core with callbacks
    try {
        gossip_core_ = std::make_shared<gossip_core>(
            self_view,
            [this](const gossip_message& msg, const node_view& target) {
                on_send_message(msg, target);
            },
            [this](const node_view& node, node_status old_status) {
                on_node_event(node, old_status);
            });
    } catch (const std::exception&) {
        return false;
    }

    // Create transport
    auto transport_type = config.use_tcp ? net::transport_type::tcp
                                         : net::transport_type::udp;

    transport_ = net::transport_factory::create_transport(
        transport_type, config.bind_ip, config.gossip_port, config.serializer);

    if (!transport_) {
        gossip_core_.reset();
        return false;
    }

    // Connect transport to gossip core
    transport_->set_gossip_core(gossip_core_);

    initialized_.store(true, std::memory_order_release);
    return true;
}

bool gossip_manager::start() noexcept {
    if (!initialized_.load(std::memory_order_acquire)) {
        return false;
    }

    if (running_.load(std::memory_order_acquire)) {
        return false; // Already running
    }

    auto result = transport_->start();
    if (result != net::error_code::success) {
        return false;
    }

    running_.store(true, std::memory_order_release);
    return true;
}

void gossip_manager::stop() noexcept {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return; // Already stopped
    }

    // Clear event callback before stopping
    {
        std::lock_guard<std::mutex> lock(event_callback_mutex_);
        event_callback_set_.store(false, std::memory_order_release);
        event_callback_ = nullptr;
    }

    if (transport_) {
        transport_->stop();
    }

    gossip_core_.reset();
    transport_.reset();
}

void gossip_manager::tick() noexcept {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    if (gossip_core_) {
        gossip_core_->tick();
    }
}

void gossip_manager::broadcast_config() noexcept {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    if (gossip_core_) {
        gossip_core_->tick_full_broadcast();
    }
}

bool gossip_manager::meet_node(std::string_view ip, uint16_t port) noexcept {
    if (!running_.load(std::memory_order_acquire)) {
        return false;
    }

    node_view node;
    node.ip = std::string(ip);
    node.port = port;
    node.status = node_status::unknown;

    gossip_core_->meet(node);
    return true;
}

bool gossip_manager::join_cluster(std::string_view ip, uint16_t port) noexcept {
    if (!running_.load(std::memory_order_acquire)) {
        return false;
    }

    node_view node;
    node.ip = std::string(ip);
    node.port = port;

    gossip_core_->join(node);
    return true;
}

void gossip_manager::leave_cluster() noexcept {
    if (gossip_core_) {
        gossip_core_->leave(self_id_);
    }
}

std::vector<node_view> gossip_manager::get_nodes() const noexcept {
    if (!gossip_core_) {
        return {};
    }
    return gossip_core_->get_nodes();
}

size_t gossip_manager::get_node_count() const noexcept {
    return gossip_core_ ? gossip_core_->size() : 0;
}

std::optional<node_view> gossip_manager::find_node(const node_id_t& id) const noexcept {
    if (!gossip_core_) {
        return std::nullopt;
    }
    return gossip_core_->find_node(id);
}

node_view gossip_manager::get_self() const noexcept {
    if (!gossip_core_) {
        return {};
    }
    return gossip_core_->self();
}

void gossip_manager::update_metadata(const std::map<std::string, std::string>& metadata) noexcept {
    if (gossip_core_) {
        gossip_core_->update_self_metadata(metadata);
    }
}

void gossip_manager::set_event_callback(cluster_event_callback callback) noexcept {
    std::lock_guard<std::mutex> lock(event_callback_mutex_);
    event_callback_ = std::move(callback);
    event_callback_set_.store(true, std::memory_order_release);
}

gossip_manager::stats gossip_manager::get_stats() const noexcept {
    stats result;

    if (gossip_core_) {
        auto core_stats = gossip_core_->get_stats();
        result.known_nodes = core_stats.known_nodes;
        result.sent_messages = core_stats.sent_messages;
        result.received_messages = core_stats.received_messages;
        result.last_tick_duration_ms = core_stats.last_tick_duration.count();
    }

    return result;
}

void gossip_manager::on_send_message(const gossip_message& msg, const node_view& target) noexcept {
    if (!transport_) {
        return;
    }

    transport_->send_message(msg, target);
}

void gossip_manager::on_node_event(const node_view& node, node_status old_status) noexcept {
    if (!event_callback_set_.load(std::memory_order_acquire)) {
        return;
    }

    // Determine cluster event type (for future extensibility)
    // Currently we just forward the raw status change

    // Lock and call callback
    cluster_event_callback callback;
    {
        std::lock_guard<std::mutex> lock(event_callback_mutex_);
        if (event_callback_set_.load(std::memory_order_acquire)) {
            callback = event_callback_;
        }
    }

    if (callback) {
        callback(node, old_status, node.status);
    }
}

} // namespace libgossip

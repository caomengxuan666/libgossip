/**
 * @file real_cluster_example.cpp
 * @brief Real multi-node cluster example using actual network communication
 * 
 * This example creates multiple nodes that communicate over the network using TCP.
 * It demonstrates:
 * - Real network communication (not simulation)
 * - Multi-process architecture
 * - Node discovery via gossip protocol
 * - Failure detection
 * - Dynamic node joining
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace gossip::net;
using namespace libgossip;

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

/**
 * @brief Cluster node that communicates over real network
 */
class ClusterNode {
public:
    ClusterNode(uint16_t port, const std::vector<node_view>& seed_nodes = {})
        : port_(port) {
        // Create node view
        self_node_.id = generate_node_id(port);
        self_node_.ip = "127.0.0.1";
        self_node_.port = port;
        self_node_.status = node_status::online;
        self_node_.role = "worker";

        // Create gossip core
        core_ = std::make_shared<gossip_core>(
            self_node_,
            [this](const gossip_message& msg, const node_view& target) {
                this->send_callback(msg, target);
            },
            [this](const node_view& node, node_status old_status) {
                this->event_callback(node, old_status);
            }
        );

        // Create TCP transport
        transport_ = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", port);

        if (!transport_) {
            throw std::runtime_error("Failed to create transport");
        }

        // Configure transport
        transport_->set_gossip_core(core_);
        transport_->set_serializer(std::make_unique<json_serializer>());

        // Store seed nodes
        seed_nodes_ = seed_nodes;
    }

    ~ClusterNode() {
        stop();
    }

    /**
     * @brief Start the node
     */
    bool start() {
        // Start transport
        auto ec = transport_->start();
        if (ec != error_code::success) {
            std::cerr << "Failed to start transport: " << static_cast<int>(ec) << std::endl;
            return false;
        }

        std::cout << "[Node:" << port_ << "] Started successfully" << std::endl;

        // Meet with seed nodes
        for (const auto& seed : seed_nodes_) {
            if (seed.port != port_) {  // Don't meet with self
                std::cout << "[Node:" << port_ << "] Meeting seed node " 
                          << seed.ip << ":" << seed.port << std::endl;
                core_->meet(seed);
            }
        }

        return true;
    }

    /**
     * @brief Stop the node
     */
    void stop() {
        if (transport_) {
            transport_->stop();
        }
    }

    /**
     * @brief Run the node (main loop)
     */
    void run() {
        while (g_running) {
            // Drive gossip protocol
            core_->tick();

            // Print status periodically
            static int counter = 0;
            if (++counter >= 50) {  // Every 5 seconds
                print_status();
                counter = 0;
            }

            std::this_thread::sleep_for(100ms);
        }
    }

    /**
     * @brief Get current cluster size
     */
    size_t get_cluster_size() const {
        return core_->size();
    }

    /**
     * @brief Get known nodes
     */
    std::vector<node_view> get_known_nodes() const {
        return core_->get_nodes();
    }

    /**
     * @brief Generate a unique node ID based on port
     */
    static node_id_t generate_node_id(uint16_t port) {
        node_id_t id{};
        id[0] = static_cast<uint8_t>((port >> 8) & 0xFF);
        id[1] = static_cast<uint8_t>(port & 0xFF);
        id[15] = static_cast<uint8_t>(port);
        return id;
    }

private:
    /**
     * @brief Send callback - called when core wants to send a message
     */
    void send_callback(const gossip_message& msg, const node_view& target) {
        if (!transport_) {
            return;
        }

        // Send message via transport
        auto ec = transport_->send_message(msg, target);
        if (ec != error_code::success) {
            std::cerr << "[Node:" << port_ << "] Failed to send message to "
                      << target.ip << ":" << target.port << std::endl;
        }
    }

    /**
     * @brief Event callback - called when node status changes
     */
    void event_callback(const node_view& node, node_status old_status) {
        auto status_str = [](node_status s) -> const char* {
            switch (s) {
                case node_status::unknown: return "UNKNOWN";
                case node_status::joining: return "JOINING";
                case node_status::online: return "ONLINE";
                case node_status::suspect: return "SUSPECT";
                case node_status::failed: return "FAILED";
                default: return "UNKNOWN";
            }
        };

        std::cout << "[Node:" << port_ << "] Event: " 
                  << node.ip << ":" << node.port << " "
                  << status_str(old_status) << " -> " << status_str(node.status) << std::endl;
    }

    /**
     * @brief Print node status
     */
    void print_status() const {
        auto nodes = core_->get_nodes();
        std::cout << "[Node:" << port_ << "] Cluster size: " << nodes.size() << std::endl;

        for (const auto& node : nodes) {
            auto status_str = [](node_status s) -> const char* {
                switch (s) {
                    case node_status::unknown: return "UNKNOWN";
                    case node_status::joining: return "JOINING";
                    case node_status::online: return "ONLINE";
                    case node_status::suspect: return "SUSPECT";
                    case node_status::failed: return "FAILED";
                    default: return "UNKNOWN";
                }
            };
            std::cout << "  - " << node.ip << ":" << node.port
                      << " [" << status_str(node.status) << "]" << std::endl;
        }
    }

private:
    uint16_t port_;
    node_view self_node_;
    std::shared_ptr<gossip_core> core_;
    std::unique_ptr<transport> transport_;
    std::vector<node_view> seed_nodes_;
};

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "==========================================" << std::endl;
    std::cout << "Real Cluster Example" << std::endl;
    std::cout << "==========================================" << std::endl;

    // Parse command line arguments
    int node_count = 3;
    uint16_t base_port = 9000;

    if (argc > 1) {
        node_count = std::atoi(argv[1]);
        if (node_count < 1 || node_count > 10) {
            std::cerr << "Node count must be between 1 and 10" << std::endl;
            return 1;
        }
    }

    if (argc > 2) {
        base_port = static_cast<uint16_t>(std::atoi(argv[2]));
    }

    std::cout << "Creating " << node_count << " nodes starting from port " 
              << base_port << std::endl;
    std::cout << "Press Ctrl+C to shutdown..." << std::endl;
    std::cout << std::endl;

    // Create seed nodes (first node is the seed)
    std::vector<node_view> seed_nodes;
    node_view seed_node;
    seed_node.id = ClusterNode::generate_node_id(base_port);
    seed_node.ip = "127.0.0.1";
    seed_node.port = base_port;
    seed_node.status = node_status::online;
    seed_node.role = "seed";
    seed_nodes.push_back(seed_node);

    // Create nodes
    std::vector<std::unique_ptr<ClusterNode>> nodes;
    std::vector<std::thread> node_threads;

    try {
        // Create all nodes
        for (int i = 0; i < node_count; ++i) {
            uint16_t port = base_port + i;
            nodes.push_back(std::make_unique<ClusterNode>(port, seed_nodes));
        }

        // Start all nodes
        for (int i = 0; i < node_count; ++i) {
            if (!nodes[i]->start()) {
                std::cerr << "Failed to start node " << i << std::endl;
                return 1;
            }
            std::this_thread::sleep_for(100ms);  // Small delay between starts
        }

        std::cout << std::endl;
        std::cout << "All nodes started successfully!" << std::endl;
        std::cout << "Waiting for cluster to converge..." << std::endl;
        std::cout << std::endl;

        // Start node threads
        for (int i = 0; i < node_count; ++i) {
            node_threads.emplace_back([&nodes, i]() {
                nodes[i]->run();
            });
        }

        // Wait for all threads
        for (auto& thread : node_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Cleanup is automatic via destructors
    std::cout << "Cluster shutdown complete" << std::endl;
    return 0;
}

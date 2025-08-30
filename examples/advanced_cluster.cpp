// examples/advanced_cluster.cpp
#include "core/gossip_core.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

int main() {
    using namespace libgossip;
    using namespace std::chrono;

    auto status_str = [](node_status s) -> const char * {
        switch (s) {
            case node_status::unknown:
                return "UNKNOWN";
            case node_status::joining:
                return "JOINING";
            case node_status::online:
                return "ONLINE";
            case node_status::suspect:
                return "SUSPECT";
            case node_status::failed:
                return "FAILED";
            default:
                return "UNKNOWN";
        }
    };

    const int N = 4;
    std::vector<std::shared_ptr<gossip_core>> nodes(N);

    // Step 1: Create all self_view
    std::vector<node_view> self_views;
    for (int i = 0; i < N; ++i) {
        node_view self;
        self.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<uint8_t>(100 + i)}};
        self.ip = "127.0.0.1";
        self.port = 8000 + i;
        self.heartbeat = 1;
        self.config_epoch = 1;
        self.seen_time = clock::now();
        self.status = node_status::online;

        // Add metadata
        self.role = (i == 0) ? "master" : "replica";
        self.region = "us-east-1";
        self.metadata["datacenter"] = "dc1";
        self.metadata["rack"] = "rack" + std::to_string(i / 2);

        self_views.push_back(self);
    }

    // Step 2: Create all send_fn and event_fn
    auto send_fn_generator = [&nodes](int from_index) {
        return [&nodes, from_index](const gossip_message &msg, const node_view &target) {
            for (size_t i = 0; i < nodes.size(); ++i) {
                const auto &node = nodes[i];
                if (node && node->self().id == target.id) {
                    std::thread t([node, msg, i]() {
                        std::this_thread::sleep_for(10ms);
                        node->handle_message(msg, clock::now());
                    });
                    t.detach();
                    return;
                }
            }
        };
    };

    auto event_fn_generator = [](int node_id, const auto &status_str) {
        return [node_id, status_str](const node_view &node, node_status old) {
            std::cout << "[Node " << node_id << "] EVENT: "
                      << static_cast<int>(node.id[15]) << " " << status_str(old)
                      << " → " << status_str(node.status) << "\n";
        };
    };

    // Step 3: Create all gossip_core
    for (int i = 0; i < N; ++i) {
        auto send_fn = send_fn_generator(i);
        auto event_fn = event_fn_generator(100 + i, status_str);
        nodes[i] = std::make_shared<gossip_core>(self_views[i], send_fn, event_fn);
    }

    // Step 4: Establish connections
    std::cout << "=== Establishing node connections ===\n";
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i != j) {
                nodes[i]->join(nodes[j]->self());
            }
        }
    }

    // Step 5: Run for a period of time
    std::cout << "\n=== Running cluster ===\n";
    for (int step = 0; step < 10; ++step) {
        std::cout << "Step " << step << "\n";
        for (const auto &node: nodes) {
            if (node) {
                node->tick();
            }
        }
        std::this_thread::sleep_for(100ms);
    }

    // Step 6: Demonstrate node leaving
    std::cout << "\n=== Node 103 gracefully leaves ===\n";
    nodes[3]->leave(nodes[3]->self().id);

    // Run a few more rounds to propagate leave message
    for (int step = 0; step < 5; ++step) {
        for (const auto &node: nodes) {
            if (node) {
                node->tick();
            }
        }
        std::this_thread::sleep_for(100ms);
    }

    // Step 7: Display statistics
    std::cout << "\n=== Statistics ===\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]) {
            auto stats = nodes[i]->get_stats();
            std::cout << "Node " << (100 + i) << ": "
                      << stats.known_nodes << " known nodes, "
                      << stats.sent_messages << " sent, "
                      << stats.received_messages << " received\n";
        }
    }

    std::cout << "\nSimulation ended.\n";
    return 0;
}
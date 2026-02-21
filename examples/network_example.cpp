#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>

using namespace gossip::net;

// Pipes for server ready notification
int tcp_server_ready_pipe[2];

// Send callback function
void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "[Client] Core send callback: Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

// Event callback function
void event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Client] Core event callback: Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

// Server callbacks
void server_send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "[Server] Send callback: Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

void server_event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Server] Event callback: Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

// Server process - listens on given port
void run_tcp_server(int port) {
    std::cout << "[Server] Starting TCP server on port " << port << std::endl;

    // Create server node
    libgossip::node_view server_node;
    server_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
    server_node.ip = "127.0.0.1";
    server_node.port = port;
    server_node.status = libgossip::node_status::online;

    // Create gossip core for server
    auto server_core = std::make_shared<libgossip::gossip_core>(server_node, server_send_callback, server_event_callback);

    // Create TCP transport
    auto tcp_transport = transport_factory::create_transport(transport_type::tcp, "127.0.0.1", port);
    if (!tcp_transport) {
        std::cerr << "[Server] Failed to create TCP transport" << std::endl;
        return;
    }

    tcp_transport->set_gossip_core(server_core);
    tcp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = tcp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Server] Failed to start TCP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Server] TCP server started successfully" << std::endl;

    // Notify client that server is ready
    char ready = 1;
    write(tcp_server_ready_pipe[1], &ready, 1);

    // Keep server running
    std::cout << "[Server] Waiting for messages..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    tcp_transport->stop();
    std::cout << "[Server] TCP server stopped" << std::endl;
}

void run_udp_server(int port) {
    std::cout << "[Server] Starting UDP server on port " << port << std::endl;

    // Create server node
    libgossip::node_view server_node;
    server_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};
    server_node.ip = "127.0.0.1";
    server_node.port = port;
    server_node.status = libgossip::node_status::online;

    // Create gossip core for server
    auto server_core = std::make_shared<libgossip::gossip_core>(server_node, server_send_callback, server_event_callback);

    // Create UDP transport
    auto udp_transport = transport_factory::create_transport(transport_type::udp, "127.0.0.1", port);
    if (!udp_transport) {
        std::cerr << "[Server] Failed to create UDP transport" << std::endl;
        return;
    }

    udp_transport->set_gossip_core(server_core);
    udp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = udp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Server] Failed to start UDP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Server] UDP server started successfully" << std::endl;

    // Keep server running
    std::cout << "[Server] Waiting for messages..." << std::endl;
    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[Server] UDP server stopping..." << std::endl;
    udp_transport->stop();
    std::cout << "[Server] UDP server stopped" << std::endl;
}

// Client process - sends messages to server
void run_tcp_client(int server_port) {
    std::cout << "[Client] Waiting for server to be ready..." << std::endl;

    // Wait for server to be ready via pipe (up to 10 seconds)
    char ready = 0;
    for (int i = 0; i < 100; ++i) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(tcp_server_ready_pipe[0], &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms

        int ret = select(tcp_server_ready_pipe[0] + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ret > 0 && FD_ISSET(tcp_server_ready_pipe[0], &read_fds)) {
            read(tcp_server_ready_pipe[0], &ready, 1);
            if (ready) {
                std::cout << "[Client] Server ready after " << (i * 100) << "ms" << std::endl;
                break;
            }
        }
    }

    if (!ready) {
        std::cerr << "[Client] Server did not start in time" << std::endl;
        return;
    }

    std::cout << "[Client] Server is ready, starting TCP client" << std::endl;

    // Create client node
    libgossip::node_view client_node;
    client_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3}};
    client_node.ip = "127.0.0.1";
    client_node.port = 9001;
    client_node.status = libgossip::node_status::online;

    // Create gossip core for client
    auto core = std::make_shared<libgossip::gossip_core>(client_node, send_callback, event_callback);

    // Create TCP transport
    auto tcp_transport = transport_factory::create_transport(transport_type::tcp, "127.0.0.1", 9001);
    if (!tcp_transport) {
        std::cerr << "[Client] Failed to create TCP transport" << std::endl;
        return;
    }

    tcp_transport->set_gossip_core(core);
    tcp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = tcp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to start TCP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Client] TCP transport started successfully" << std::endl;

    // Create target node (server)
    libgossip::node_view target_node;
    target_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
    target_node.ip = "127.0.0.1";
    target_node.port = server_port;
    target_node.status = libgossip::node_status::online;

    // Create test message
    libgossip::gossip_message test_msg;
    test_msg.sender = client_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(client_node);
    test_msg.entries.push_back(target_node);

    // Send test message synchronously
    std::cout << "\n[Client] Sending TCP message synchronously..." << std::endl;
    ec = tcp_transport->send_message(test_msg, target_node);
    if (ec == error_code::success) {
        std::cout << "[Client] TCP message sent synchronously successfully" << std::endl;
    } else {
        std::cerr << "[Client] Failed to send TCP message synchronously, error code: "
                  << libgossip::enum_to_string(ec) << std::endl;
    }

    // Send test message asynchronously
    std::cout << "\n[Client] Sending TCP message asynchronously..." << std::endl;
    std::promise<error_code> tcp_promise;
    std::future<error_code> tcp_future = tcp_promise.get_future();

    tcp_transport->send_message_async(test_msg, target_node, [&tcp_promise](error_code ec) {
        std::cout << "[Client] TCP async send callback executed" << std::endl;
        if (ec == error_code::success) {
            std::cout << "[Client] TCP message sent asynchronously successfully" << std::endl;
        } else {
            std::cerr << "[Client] Failed to send TCP message asynchronously, error code: "
                      << libgossip::enum_to_string(ec) << std::endl;
        }
        tcp_promise.set_value(ec);
    });

    // Wait for the TCP async operation to complete
    auto tcp_result = tcp_future.wait_for(std::chrono::seconds(2));
    if (tcp_result == std::future_status::ready) {
        error_code tcp_async_ec = tcp_future.get();
        std::cout << "[Client] TCP async operation completed with error code: "
                  << libgossip::enum_to_string(tcp_async_ec) << std::endl;
    } else {
        std::cerr << "[Client] TCP async operation timed out" << std::endl;
    }

    // Wait a bit for server to process
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop transport
    tcp_transport->stop();
    std::cout << "[Client] TCP client stopped" << std::endl;
}

void run_udp_client(int server_port) {
    std::cout << "\n[Client] Starting UDP client" << std::endl;

    // Create client node
    libgossip::node_view client_node;
    client_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4}};
    client_node.ip = "127.0.0.1";
    client_node.port = 8002;
    client_node.status = libgossip::node_status::online;

    // Create gossip core for client
    auto core = std::make_shared<libgossip::gossip_core>(client_node, send_callback, event_callback);

    // Create UDP transport
    auto udp_transport = transport_factory::create_transport(transport_type::udp, "127.0.0.1", 8002);
    if (!udp_transport) {
        std::cerr << "[Client] Failed to create UDP transport" << std::endl;
        return;
    }

    udp_transport->set_gossip_core(core);
    udp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = udp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to start UDP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Client] UDP transport started successfully" << std::endl;

    // Create target node (server)
    libgossip::node_view target_node;
    target_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};
    target_node.ip = "127.0.0.1";
    target_node.port = server_port;
    target_node.status = libgossip::node_status::online;

    // Create test message
    libgossip::gossip_message test_msg;
    test_msg.sender = client_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(client_node);
    test_msg.entries.push_back(target_node);

    // Send test message synchronously
    std::cout << "\n[Client] Sending UDP message synchronously..." << std::endl;
    ec = udp_transport->send_message(test_msg, target_node);
    if (ec == error_code::success) {
        std::cout << "[Client] UDP message sent synchronously successfully" << std::endl;
    } else {
        std::cerr << "[Client] Failed to send UDP message synchronously, error code: "
                  << libgossip::enum_to_string(ec) << std::endl;
    }

    // Send test message asynchronously
    std::cout << "\n[Client] Sending UDP message asynchronously..." << std::endl;
    std::promise<error_code> promise;
    std::future<error_code> future = promise.get_future();

    udp_transport->send_message_async(test_msg, target_node, [&promise](error_code ec) {
        std::cout << "[Client] Async send callback executed" << std::endl;
        if (ec == error_code::success) {
            std::cout << "[Client] UDP message sent asynchronously successfully" << std::endl;
        } else {
            std::cerr << "[Client] Failed to send UDP message asynchronously, error code: "
                      << libgossip::enum_to_string(ec) << std::endl;
        }
        promise.set_value(ec);
    });

    // Wait for the async operation to complete
    auto result = future.wait_for(std::chrono::seconds(2));
    if (result == std::future_status::ready) {
        error_code async_ec = future.get();
        std::cout << "[Client] Async operation completed with error code: "
                  << libgossip::enum_to_string(async_ec) << std::endl;
    } else {
        std::cerr << "[Client] Async operation timed out" << std::endl;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop transport
    udp_transport->stop();
    std::cout << "[Client] UDP client stopped" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  libgossip Network Layer Example" << std::endl;
    std::cout << "  Testing TCP and UDP with Server/Client" << std::endl;
    std::cout << "========================================" << std::endl;

    // Create pipes for inter-process communication
    if (pipe(tcp_server_ready_pipe) < 0) {
        std::cerr << "Failed to create TCP server ready pipe" << std::endl;
        return 1;
    }

    pid_t tcp_server_pid = fork();
    if (tcp_server_pid == 0) {
        // Child process 1: TCP Server
        run_tcp_server(9000);
        exit(0);
    } else if (tcp_server_pid < 0) {
        std::cerr << "Failed to fork TCP server process" << std::endl;
        return 1;
    }

    pid_t udp_server_pid = fork();
    if (udp_server_pid == 0) {
        // Child process 2: UDP Server
        run_udp_server(8001);
        exit(0);
    } else if (udp_server_pid < 0) {
        std::cerr << "Failed to fork UDP server process" << std::endl;
        return 1;
    }

    // Parent process: Run TCP and UDP clients sequentially
    std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait for servers to initialize

    // Run TCP client first
    std::cout << "\n--- Starting TCP Test ---" << std::endl;
    run_tcp_client(9000);

    // Wait a bit before UDP test
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Run UDP client
    std::cout << "\n--- Starting UDP Test ---" << std::endl;
    run_udp_client(8001);

    // Wait a bit for servers to process messages
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Signal servers to stop by closing pipes
    close(tcp_server_ready_pipe[0]);
    close(tcp_server_ready_pipe[1]);

    // Wait for both servers to finish
    int status;
    waitpid(tcp_server_pid, &status, 0);
    waitpid(udp_server_pid, &status, 0);

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Network Test Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Network example completed!" << std::endl;

    return 0;
}
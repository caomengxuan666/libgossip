#include "net/tcp_transport.hpp"
#include <asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

namespace gossip {
    namespace net {

        // TCP transport implementation details
        class tcp_transport::impl {
        public:
            impl(const std::string &host, uint16_t port)
                : io_context_(),
                  acceptor_(io_context_),
                  endpoint_(asio::ip::make_address(host), port),
                  work_(asio::make_work_guard(io_context_)),
                  core_(nullptr),
                  serializer_(nullptr) {
            }

            ~impl() {
                stop();
            }

            error_code start() {
                try {
                    acceptor_.open(endpoint_.protocol());
                    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
                    acceptor_.bind(endpoint_);
                    acceptor_.listen();

                    // Start IO context thread
                    io_thread_ = std::thread([this]() {
                        io_context_.run();
                    });

                    // Start accepting connections
                    start_accept();
                    return error_code::success;
                } catch (const std::exception &e) {
                    std::cerr << "Failed to start TCP transport: " << e.what() << std::endl;
                    return error_code::network_error;
                }
            }

            error_code stop() {
                try {
                    if (acceptor_.is_open()) {
                        asio::post(io_context_, [this]() {
                            acceptor_.close();
                            // Close all connected sockets
                            for (auto &socket: sockets_) {
                                if (socket->is_open()) {
                                    socket->close();
                                }
                            }
                        });
                    }

                    io_context_.stop();

                    if (io_thread_.joinable()) {
                        io_thread_.join();
                    }
                    return error_code::success;
                } catch (const std::exception &e) {
                    std::cerr << "Error stopping TCP transport: " << e.what() << std::endl;
                    return error_code::network_error;
                }
            }

            void set_gossip_core(std::shared_ptr<libgossip::gossip_core> core) {
                core_ = core;
            }

            void set_serializer(std::unique_ptr<message_serializer> serializer) {
                serializer_ = std::move(serializer);
            }

            error_code send_message(const libgossip::gossip_message &msg,
                                    const libgossip::node_view &target) {
                // If no serializer is set, return error
                if (!serializer_) {
                    return error_code::serialization_error;
                }

                // Serialize message
                std::vector<uint8_t> data;
                error_code ec = serializer_->serialize(msg, data);
                if (ec != error_code::success) {
                    return ec;
                }

                // In a real implementation, we would send data over TCP to the target node
                // Here we just log and simulate
                std::cout << "Sending message of type " << static_cast<int>(msg.type)
                          << " to " << target.ip << ":" << target.port
                          << " via TCP (serialized to " << data.size() << " bytes)" << std::endl;

                // Simulate async send
                asio::post(io_context_, [this, data, target]() {
                    // In a real implementation, this would actually send data via TCP
                    // We just simulate the receive process
                    simulate_receive(data, target);
                });

                return error_code::success;
            }

            void send_message_async(const libgossip::gossip_message &msg,
                                    const libgossip::node_view &target,
                                    std::function<void(error_code)> callback) {
                // If no serializer is set, return error
                if (!serializer_) {
                    if (callback) {
                        callback(error_code::serialization_error);
                    }
                    return;
                }

                // Serialize message
                std::vector<uint8_t> data;
                error_code ec = serializer_->serialize(msg, data);
                if (ec != error_code::success) {
                    if (callback) {
                        callback(ec);
                    }
                    return;
                }

                // In a real implementation, we would send data over TCP
                // Here we just log and simulate
                std::cout << "Async sending message of type " << static_cast<int>(msg.type)
                          << " to " << target.ip << ":" << target.port
                          << " via TCP (serialized to " << data.size() << " bytes)" << std::endl;

                // Simulate async send with callback
                asio::post(io_context_, [this, data, target, callback]() {
                    // In a real implementation, this would actually send data via TCP
                    // We just simulate the receive process
                    simulate_receive(data, target);

                    // Call the callback
                    if (callback) {
                        callback(error_code::success);
                    }
                });
            }

        private:
            void start_accept() {
                auto new_socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
                acceptor_.async_accept(*new_socket, [this, new_socket](const asio::error_code &error) {
                    if (!error) {
                        handle_accept(new_socket);
                    }
                    if (!error && acceptor_.is_open()) {
                        start_accept();// Continue accepting new connections
                    }
                });
            }

            void handle_accept(std::shared_ptr<asio::ip::tcp::socket> socket) {
                std::cout << "New TCP connection accepted" << std::endl;
                sockets_.push_back(socket);

                // Start receiving data from this socket
                start_receive_from_socket(socket);
            }

            void start_receive_from_socket(std::shared_ptr<asio::ip::tcp::socket> socket) {
                auto buffer = std::make_shared<std::vector<char>>(65536);
                socket->async_read_some(asio::buffer(*buffer),
                                        [this, socket, buffer](const asio::error_code &error, std::size_t bytes_transferred) {
                                            if (!error && bytes_transferred > 0) {
                                                handle_socket_receive(*buffer, bytes_transferred);
                                                // Continue receiving data
                                                start_receive_from_socket(socket);
                                            } else if (error) {
                                                std::cerr << "Error receiving data: " << error.message() << std::endl;
                                                // Remove socket from list
                                                auto it = std::find(sockets_.begin(), sockets_.end(), socket);
                                                if (it != sockets_.end()) {
                                                    sockets_.erase(it);
                                                }
                                            }
                                        });
            }

            void handle_socket_receive(const std::vector<char> &buffer, std::size_t bytes_transferred) {
                // Process received data
                std::cout << "Received " << bytes_transferred << " bytes via TCP" << std::endl;

                // If we have gossip core and serializer, process the received message
                if (core_ && serializer_) {
                    // Convert received data to vector of uint8_t
                    std::vector<uint8_t> data(buffer.begin(), buffer.begin() + static_cast<std::vector<char>::difference_type>(bytes_transferred));

                    // Deserialize message
                    libgossip::gossip_message msg;
                    error_code ec = serializer_->deserialize(data, msg);
                    if (ec == error_code::success) {
                        // Pass message to gossip core
                        auto now = std::chrono::steady_clock::now();
                        core_->handle_message(msg, now);
                    } else {
                        std::cerr << "Failed to deserialize received message, error code: " << static_cast<int>(ec) << std::endl;
                    }
                }
            }

            void simulate_receive(const std::vector<uint8_t> &data,
                                  const libgossip::node_view &target) {
                // Simulate receive process
                std::cout << "Simulating TCP receipt of " << data.size() << " bytes" << std::endl;

                // If we have core instance and serializer, simulate message processing
                if (core_ && serializer_) {
                    // Deserialize message
                    libgossip::gossip_message msg;
                    error_code ec = serializer_->deserialize(data, msg);
                    if (ec == error_code::success) {
                        auto now = std::chrono::steady_clock::now();
                        core_->handle_message(msg, now);
                    } else {
                        std::cerr << "Failed to deserialize simulated message, error code: " << static_cast<int>(ec) << std::endl;
                    }
                }
            }

            asio::io_context io_context_;
            asio::ip::tcp::acceptor acceptor_;
            asio::ip::tcp::endpoint endpoint_;
            asio::executor_work_guard<asio::io_context::executor_type> work_;
            std::thread io_thread_;
            std::shared_ptr<libgossip::gossip_core> core_;
            std::unique_ptr<message_serializer> serializer_;
            std::vector<std::shared_ptr<asio::ip::tcp::socket>> sockets_;
        };

        // TCP transport public interface implementation
        tcp_transport::tcp_transport(const std::string &host, uint16_t port)
            : pimpl_(std::make_unique<impl>(host, port)) {
        }

        tcp_transport::~tcp_transport() = default;

        error_code tcp_transport::start() {
            return pimpl_->start();
        }

        error_code tcp_transport::stop() {
            return pimpl_->stop();
        }

        error_code tcp_transport::send_message(const libgossip::gossip_message &msg,
                                               const libgossip::node_view &target) {
            return pimpl_->send_message(msg, target);
        }

        void tcp_transport::send_message_async(const libgossip::gossip_message &msg,
                                               const libgossip::node_view &target,
                                               std::function<void(error_code)> callback) {
            pimpl_->send_message_async(msg, target, callback);
        }

        void tcp_transport::set_gossip_core(std::shared_ptr<libgossip::gossip_core> core) {
            pimpl_->set_gossip_core(core);
        }

        void tcp_transport::set_serializer(std::unique_ptr<message_serializer> serializer) {
            pimpl_->set_serializer(std::move(serializer));
        }

    }// namespace net
}// namespace gossip
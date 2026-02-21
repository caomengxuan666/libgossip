#include "net/tcp_transport.hpp"
#include "core/enum_reflection.inl"
#include <asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
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
                            std::lock_guard<std::mutex> lock(sockets_mutex_);
                            for (auto &socket: sockets_) {
                                if (socket->is_open()) {
                                    socket->close();
                                }
                            }
                            outbound_sockets_.clear();
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

                // Prepare message with 4-byte length prefix
                std::vector<uint8_t> packet;
                packet.reserve(4 + data.size());
                
                // Add 4-byte length prefix (big-endian)
                uint32_t length = static_cast<uint32_t>(data.size());
                packet.push_back((length >> 24) & 0xFF);
                packet.push_back((length >> 16) & 0xFF);
                packet.push_back((length >> 8) & 0xFF);
                packet.push_back(length & 0xFF);
                
                // Add serialized data
                packet.insert(packet.end(), data.begin(), data.end());

                // Find or create a connection to the target
                auto socket = get_or_create_socket(target.ip, target.port);
                if (!socket) {
                    return error_code::network_error;
                }

                // Send over TCP
                asio::error_code send_ec;
                size_t bytes_sent = socket->send(asio::buffer(packet), 0, send_ec);
                
                if (send_ec) {
                    std::cerr << "Failed to send TCP message to " << target.ip << ":" << target.port
                              << ": " << send_ec.message() << std::endl;
                    return error_code::network_error;
                }

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

                // Prepare message with 4-byte length prefix
                std::vector<uint8_t> packet;
                packet.reserve(4 + data.size());
                
                // Add 4-byte length prefix (big-endian)
                uint32_t length = static_cast<uint32_t>(data.size());
                packet.push_back((length >> 24) & 0xFF);
                packet.push_back((length >> 16) & 0xFF);
                packet.push_back((length >> 8) & 0xFF);
                packet.push_back(length & 0xFF);
                
                // Add serialized data
                packet.insert(packet.end(), data.begin(), data.end());

                // Find or create a connection to the target
                auto socket = get_or_create_socket(target.ip, target.port);
                if (!socket) {
                    if (callback) {
                        callback(error_code::network_error);
                    }
                    return;
                }

                // Send asynchronously over TCP
                auto packet_ptr = std::make_shared<std::vector<uint8_t>>(std::move(packet));
                
                asio::async_write(
                    *socket,
                    asio::buffer(*packet_ptr),
                    [callback](const asio::error_code &send_ec, size_t /*bytes_sent*/) {
                        if (send_ec) {
                            std::cerr << "Failed to async send TCP message: " << send_ec.message() << std::endl;
                            if (callback) {
                                callback(error_code::network_error);
                            }
                        } else {
                            if (callback) {
                                callback(error_code::success);
                            }
                        }
                    }
                );
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
                {
                    std::lock_guard<std::mutex> lock(sockets_mutex_);
                    sockets_.push_back(socket);
                }

                // Start receiving data from this socket
                start_receive_from_socket(socket);
            }

            std::shared_ptr<asio::ip::tcp::socket> get_or_create_socket(const std::string &ip, int port) {
                // Create connection key
                std::string key = ip + ":" + std::to_string(port);
                
                // Check if we already have a connection
                {
                    std::lock_guard<std::mutex> lock(sockets_mutex_);
                    auto it = outbound_sockets_.find(key);
                    if (it != outbound_sockets_.end() && it->second->is_open()) {
                        return it->second;
                    }
                }
                
                // Create new connection
                auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
                asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), static_cast<unsigned short>(port));
                
                asio::error_code connect_ec;
                socket->connect(endpoint, connect_ec);
                
                if (connect_ec) {
                    std::cerr << "Failed to connect to " << ip << ":" << port 
                              << ": " << connect_ec.message() << std::endl;
                    return nullptr;
                }
                
                // Store the socket and start receiving
                {
                    std::lock_guard<std::mutex> lock(sockets_mutex_);
                    outbound_sockets_[key] = socket;
                }
                start_receive_from_socket(socket);
                
                return socket;
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
                                                // Only log non-EOF errors (EOF is normal connection close)
                                                if (error != asio::error::eof) {
                                                    std::cerr << "Error receiving data: " << error.message() << std::endl;
                                                }
                                                // Remove socket from list
                                                std::lock_guard<std::mutex> lock(sockets_mutex_);
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

                    // Parse message with 4-byte length prefix
                    size_t offset = 0;
                    while (offset + 4 <= data.size()) {
                        // Read length prefix (big-endian)
                        uint32_t length = (static_cast<uint32_t>(data[offset]) << 24) |
                                         (static_cast<uint32_t>(data[offset + 1]) << 16) |
                                         (static_cast<uint32_t>(data[offset + 2]) << 8) |
                                         static_cast<uint32_t>(data[offset + 3]);
                        
                        offset += 4;

                        // Check if we have enough data for the message
                        if (offset + length > data.size()) {
                            std::cerr << "Incomplete message, expected " << length 
                                      << " bytes but only " << (data.size() - offset) << " available" << std::endl;
                            break;
                        }

                        // Extract message data
                        std::vector<uint8_t> message_data(data.begin() + offset, data.begin() + offset + length);
                        offset += length;

                        // Deserialize message
                        libgossip::gossip_message msg;
                        error_code ec = serializer_->deserialize(message_data, msg);
                        if (ec == error_code::success) {
                            std::cout << "[TCP Server] Successfully deserialized message, type: "
                                      << static_cast<int>(msg.type) << ", entries: " << msg.entries.size() << std::endl;
                            // Pass message to gossip core
                            auto now = std::chrono::steady_clock::now();
                            core_->handle_message(msg, now);
                        } else {
                            std::cerr << "[TCP Server] Failed to deserialize received message, error code: "
                                      << libgossip::enum_to_string(ec) << std::endl;
                        }
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
            std::map<std::string, std::shared_ptr<asio::ip::tcp::socket>> outbound_sockets_;
            std::mutex sockets_mutex_;
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
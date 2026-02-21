#include "net/udp_transport.hpp"
#include "core/enum_reflection.inl"
#include <asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

namespace gossip {
    namespace net {

        // UDP transport implementation details
        class udp_transport::impl {
        public:
            impl(const std::string &host, uint16_t port)
                : io_context_(),
                  socket_(io_context_),
                  endpoint_(asio::ip::make_address(host), port),
                  work_(asio::make_work_guard(io_context_)),
                  receive_buffer_(65536),
                  core_(nullptr),
                  serializer_(nullptr) {
            }

            ~impl() {
                stop();
            }

            error_code start() {
                try {
                    socket_.open(endpoint_.protocol());
                    socket_.bind(endpoint_);

                    // Start IO context thread
                    io_thread_ = std::thread([this]() {
                        io_context_.run();
                    });

                    // Start receiving messages
                    start_receive();
                    return error_code::success;
                } catch (const std::exception &e) {
                    std::cerr << "Failed to start UDP transport: " << e.what() << std::endl;
                    return error_code::network_error;
                }
            }

            error_code stop() {
                try {
                    if (socket_.is_open()) {
                        asio::post(io_context_, [this]() {
                            socket_.close();
                        });
                    }

                    io_context_.stop();

                    if (io_thread_.joinable()) {
                        io_thread_.join();
                    }
                    return error_code::success;
                } catch (const std::exception &e) {
                    std::cerr << "Error stopping UDP transport: " << e.what() << std::endl;
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

                // Send over UDP
                asio::ip::udp::endpoint target_endpoint(
                    asio::ip::make_address(target.ip), 
                    static_cast<unsigned short>(target.port)
                );

                asio::error_code send_ec;
                size_t bytes_sent = socket_.send_to(asio::buffer(packet), target_endpoint, 0, send_ec);
                
                if (send_ec) {
                    std::cerr << "Failed to send UDP message to " << target.ip << ":" << target.port
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

                // Send asynchronously over UDP
                asio::ip::udp::endpoint target_endpoint(
                    asio::ip::make_address(target.ip), 
                    static_cast<unsigned short>(target.port)
                );

                auto packet_ptr = std::make_shared<std::vector<uint8_t>>(std::move(packet));
                
                socket_.async_send_to(
                    asio::buffer(*packet_ptr),
                    target_endpoint,
                    [callback](const asio::error_code &send_ec, size_t /*bytes_sent*/) {
                        if (send_ec) {
                            std::cerr << "Failed to async send UDP message: " << send_ec.message() << std::endl;
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
            void start_receive() {
                socket_.async_receive_from(
                        asio::buffer(receive_buffer_),
                        remote_endpoint_,
                        [this](const asio::error_code &error, std::size_t bytes_transferred) {
                            if (!error && bytes_transferred > 0) {
                                handle_receive(bytes_transferred);
                            }
                            if (!error) {
                                start_receive();// Continue receiving
                            }
                        });
            }

            void handle_receive(std::size_t bytes_transferred) {
                // Process received data
                std::cout << "Received " << bytes_transferred << " bytes from "
                          << remote_endpoint_.address().to_string() << ":"
                          << remote_endpoint_.port() << std::endl;

                // If we have gossip core and serializer, process the received message
                if (core_ && serializer_) {
                    // Convert received data to vector of uint8_t
                    std::vector<uint8_t> data(receive_buffer_.begin(), receive_buffer_.begin() + static_cast<std::vector<char>::difference_type>(bytes_transferred));

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
                            std::cout << "[UDP Server] Successfully deserialized message, type: "
                                      << static_cast<int>(msg.type) << ", entries: " << msg.entries.size() << std::endl;
                            // Pass message to gossip core
                            auto now = std::chrono::steady_clock::now();
                            core_->handle_message(msg, now);
                        } else {
                            std::cerr << "[UDP Server] Failed to deserialize received message, error code: "
                                      << libgossip::enum_to_string(ec) << std::endl;
                        }
                    }
                }

                // Continue receiving
                start_receive();
            }

            asio::io_context io_context_;
            asio::ip::udp::socket socket_;
            asio::ip::udp::endpoint endpoint_;
            asio::executor_work_guard<asio::io_context::executor_type> work_;
            std::thread io_thread_;
            std::vector<char> receive_buffer_;
            asio::ip::udp::endpoint remote_endpoint_;
            std::shared_ptr<libgossip::gossip_core> core_;
            std::unique_ptr<message_serializer> serializer_;
        };

        // UDP transport public interface implementation
        udp_transport::udp_transport(const std::string &host, uint16_t port)
            : pimpl_(std::make_unique<impl>(host, port)) {
        }

        udp_transport::~udp_transport() = default;

        error_code udp_transport::start() {
            return pimpl_->start();
        }

        error_code udp_transport::stop() {
            return pimpl_->stop();
        }

        error_code udp_transport::send_message(const libgossip::gossip_message &msg,
                                               const libgossip::node_view &target) {
            return pimpl_->send_message(msg, target);
        }

        void udp_transport::send_message_async(const libgossip::gossip_message &msg,
                                               const libgossip::node_view &target,
                                               std::function<void(error_code)> callback) {
            pimpl_->send_message_async(msg, target, callback);
        }

        void udp_transport::set_gossip_core(std::shared_ptr<libgossip::gossip_core> core) {
            pimpl_->set_gossip_core(core);
        }

        void udp_transport::set_serializer(std::unique_ptr<message_serializer> serializer) {
            pimpl_->set_serializer(std::move(serializer));
        }

    }// namespace net
}// namespace gossip
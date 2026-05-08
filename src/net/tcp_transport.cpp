#include "net/tcp_transport.hpp"
#include "core/enum_reflection.inl"
#include <asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <thread>

namespace libgossip {
    namespace net {

        // Helper to convert serialization_error to error_code
        static error_code to_error_code(serialization_error se) {
            switch (se) {
                case serialization_error::success:
                    return error_code::success;
                case serialization_error::serialization_failed:
                case serialization_error::deserialization_failed:
                    return error_code::serialization_error;
                case serialization_error::invalid_input:
                case serialization_error::unsupported_format:
                    return error_code::invalid_argument;
                default:
                    return error_code::serialization_error;
            }
        }

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

                    io_thread_ = std::thread([this]() {
                        io_context_.run();
                    });

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

            void set_gossip_core(std::shared_ptr<gossip_core> core) {
                core_ = core;
            }

            void set_serializer(std::unique_ptr<message_serializer> serializer) {
                serializer_ = std::move(serializer);
            }

            error_code send_message(const gossip_message &msg,
                                    const node_view &target) {
                if (!serializer_) {
                    return error_code::serialization_error;
                }

                std::vector<uint8_t> data;
                auto se = serializer_->serialize(msg, data);
                if (se != serialization_error::success) {
                    return to_error_code(se);
                }

                std::vector<uint8_t> packet;
                packet.reserve(4 + data.size());

                uint32_t length = static_cast<uint32_t>(data.size());
                packet.push_back((length >> 24) & 0xFF);
                packet.push_back((length >> 16) & 0xFF);
                packet.push_back((length >> 8) & 0xFF);
                packet.push_back(length & 0xFF);

                packet.insert(packet.end(), data.begin(), data.end());

                auto socket = get_or_create_socket(target.ip, target.port);
                if (!socket) {
                    return error_code::network_error;
                }

                asio::error_code send_ec;
                socket->send(asio::buffer(packet), 0, send_ec);

                if (send_ec) {
                    std::cerr << "Failed to send TCP message to " << target.ip << ":" << target.port
                              << ": " << send_ec.message() << std::endl;
                    return error_code::network_error;
                }

                return error_code::success;
            }

            void send_message_async(const gossip_message &msg,
                                    const node_view &target,
                                    std::function<void(error_code)> callback) {
                if (!serializer_) {
                    if (callback) {
                        callback(error_code::serialization_error);
                    }
                    return;
                }

                std::vector<uint8_t> data;
                auto se = serializer_->serialize(msg, data);
                if (se != serialization_error::success) {
                    if (callback) {
                        callback(to_error_code(se));
                    }
                    return;
                }

                std::vector<uint8_t> packet;
                packet.reserve(4 + data.size());

                uint32_t length = static_cast<uint32_t>(data.size());
                packet.push_back((length >> 24) & 0xFF);
                packet.push_back((length >> 16) & 0xFF);
                packet.push_back((length >> 8) & 0xFF);
                packet.push_back(length & 0xFF);

                packet.insert(packet.end(), data.begin(), data.end());

                auto socket = get_or_create_socket(target.ip, target.port);
                if (!socket) {
                    if (callback) {
                        callback(error_code::network_error);
                    }
                    return;
                }

                auto packet_ptr = std::make_shared<std::vector<uint8_t>>(std::move(packet));

                asio::async_write(
                    *socket,
                    asio::buffer(packet_ptr->data(), packet_ptr->size()),
                    [packet_ptr, callback = std::move(callback)](const asio::error_code &send_ec, size_t /*bytes_sent*/) {
                        (void)packet_ptr;
                        if (send_ec) {
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
                        start_accept();
                    }
                });
            }

            void handle_accept(std::shared_ptr<asio::ip::tcp::socket> socket) {
                {
                    std::lock_guard<std::mutex> lock(sockets_mutex_);
                    sockets_.push_back(socket);
                }

                start_receive_from_socket(socket);
            }

            std::shared_ptr<asio::ip::tcp::socket> get_or_create_socket(const std::string &ip, int port) {
                std::string key = ip + ":" + std::to_string(port);

                {
                    std::lock_guard<std::mutex> lock(sockets_mutex_);
                    auto it = outbound_sockets_.find(key);
                    if (it != outbound_sockets_.end() && it->second->is_open()) {
                        return it->second;
                    }
                }

                auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
                asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), static_cast<unsigned short>(port));

                asio::error_code connect_ec;
                socket->connect(endpoint, connect_ec);

                if (connect_ec) {
                    std::cerr << "Failed to connect to " << ip << ":" << port
                              << ": " << connect_ec.message() << std::endl;
                    return nullptr;
                }

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
                                                start_receive_from_socket(socket);
                                            } else if (error) {
                                                if (error != asio::error::eof) {
                                                    std::cerr << "Error receiving data: " << error.message() << std::endl;
                                                }
                                                std::lock_guard<std::mutex> lock(sockets_mutex_);
                                                auto it = std::find(sockets_.begin(), sockets_.end(), socket);
                                                if (it != sockets_.end()) {
                                                    sockets_.erase(it);
                                                }
                                            }
                                        });
            }

            void handle_socket_receive(const std::vector<char> &buffer, std::size_t bytes_transferred) {
                if (core_ && serializer_) {
                    std::vector<uint8_t> data(buffer.begin(),
                                              buffer.begin() + static_cast<std::vector<char>::difference_type>(bytes_transferred));

                    size_t offset = 0;
                    while (offset + 4 <= data.size()) {
                        uint32_t length = (static_cast<uint32_t>(data[offset]) << 24) |
                                         (static_cast<uint32_t>(data[offset + 1]) << 16) |
                                         (static_cast<uint32_t>(data[offset + 2]) << 8) |
                                         static_cast<uint32_t>(data[offset + 3]);

                        offset += 4;

                        if (offset + length > data.size()) {
                            break;
                        }

                        std::vector<uint8_t> message_data(data.begin() + offset,
                                                          data.begin() + offset + length);
                        offset += length;

                        gossip_message msg;
                        auto se = serializer_->deserialize(message_data, msg);
                        if (se == serialization_error::success) {
                            auto now = std::chrono::steady_clock::now();
                            core_->handle_message(msg, now);
                        }
                    }
                }
            }

            asio::io_context io_context_;
            asio::ip::tcp::acceptor acceptor_;
            asio::ip::tcp::endpoint endpoint_;
            asio::executor_work_guard<asio::io_context::executor_type> work_;
            std::thread io_thread_;
            std::shared_ptr<gossip_core> core_;
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

        error_code tcp_transport::send_message(const gossip_message &msg,
                                               const node_view &target) {
            return pimpl_->send_message(msg, target);
        }

        void tcp_transport::send_message_async(const gossip_message &msg,
                                               const node_view &target,
                                               std::function<void(error_code)> callback) {
            pimpl_->send_message_async(msg, target, callback);
        }

        void tcp_transport::set_gossip_core(std::shared_ptr<gossip_core> core) {
            pimpl_->set_gossip_core(core);
        }

        void tcp_transport::set_serializer(std::unique_ptr<message_serializer> serializer) {
            pimpl_->set_serializer(std::move(serializer));
        }

    } // namespace net
} // namespace libgossip

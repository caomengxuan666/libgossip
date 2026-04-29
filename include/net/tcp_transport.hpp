/**
 * @file tcp_transport.hpp
 * @brief TCP transport implementation for the gossip protocol
 *
 * This file contains the TCP transport implementation for the gossip protocol.
 * It provides both synchronous and asynchronous message sending capabilities
 * over TCP network connections.
 *
 * @author caomengxuan666
 * @date 2025-08-30
 */

#pragma once

#include "udp_transport.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace libgossip {
    namespace net {

        /**
         * @brief TCP transport implementation
         *
         * @deprecated Use transport_factory or custom transport implementation instead.
         */
        class [[deprecated("Use transport_factory or custom transport implementation")]] LIBGOSSIP_API tcp_transport : public transport {
        public:
            tcp_transport(const std::string &host, uint16_t port);
            ~tcp_transport() override;

            error_code start() override;
            error_code stop() override;
            error_code send_message(const gossip_message &msg,
                                    const node_view &target) override;
            void send_message_async(const gossip_message &msg,
                                    const node_view &target,
                                    std::function<void(error_code)> callback) override;
            void set_gossip_core(std::shared_ptr<gossip_core> core) override;
            void set_serializer(std::unique_ptr<message_serializer> serializer) override;

        private:
            class impl;
            std::unique_ptr<impl> pimpl_;
        };

    } // namespace net
} // namespace libgossip

// Keep backward compatibility alias
namespace gossip {
    namespace net {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        using tcp_transport = libgossip::net::tcp_transport;
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
    } // namespace net
} // namespace gossip

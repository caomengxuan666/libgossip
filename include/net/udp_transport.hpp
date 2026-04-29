/**
 * @file udp_transport.hpp
 * @brief UDP transport implementation for the gossip protocol
 *
 * This file contains the UDP transport implementation for the gossip protocol.
 * It provides both synchronous and asynchronous message sending capabilities
 * over UDP network connections.
 *
 * @author caomengxuan666
 * @date 2025-08-30
 */

#pragma once

#include "core/gossip_core.hpp"
#include "core/message_serializer.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace libgossip {
    namespace net {

        /**
         * @brief Error code enumeration for network operations
         */
        enum class error_code {
            success = 0,
            network_error,
            serialization_error,
            invalid_argument,
            operation_not_permitted
        };

        /**
         * @brief Abstract transport layer interface
         *
         * This class defines the interface for network transport implementations.
         * It provides methods for starting/stopping the transport and sending messages.
         */
        class LIBGOSSIP_API transport {
        public:
            virtual ~transport() = default;

            /**
             * @brief Start the transport layer
             * @return Error code indicating success or failure
             */
            virtual error_code start() = 0;

            /**
             * @brief Stop the transport layer
             * @return Error code indicating success or failure
             */
            virtual error_code stop() = 0;

            /**
             * @brief Send a message synchronously
             * @param msg The message to send
             * @param target The target node
             * @return Error code indicating success or failure
             */
            virtual error_code send_message(const gossip_message &msg,
                                            const node_view &target) = 0;

            /**
             * @brief Send a message asynchronously
             * @param msg The message to send
             * @param target The target node
             * @param callback Completion callback function
             */
            virtual void send_message_async(const gossip_message &msg,
                                            const node_view &target,
                                            std::function<void(error_code)> callback) = 0;

            /**
             * @brief Set the gossip core instance
             * @param core Shared pointer to gossip core
             */
            virtual void set_gossip_core(std::shared_ptr<gossip_core> core) = 0;

            /**
             * @brief Set the message serializer
             * @param serializer Unique pointer to serializer
             */
            virtual void set_serializer(std::unique_ptr<message_serializer> serializer) = 0;
        };

        /**
         * @brief UDP transport implementation
         *
         * @deprecated Use transport_factory or custom transport implementation instead.
         */
        class [[deprecated("Use transport_factory or custom transport implementation")]] LIBGOSSIP_API udp_transport : public transport {
        public:
            udp_transport(const std::string &host, uint16_t port);
            ~udp_transport() override;

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

// Keep backward compatibility aliases
namespace gossip {
    namespace net {
        using transport = libgossip::net::transport;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        using udp_transport = libgossip::net::udp_transport;
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
        using error_code = libgossip::net::error_code;
    } // namespace net
} // namespace gossip

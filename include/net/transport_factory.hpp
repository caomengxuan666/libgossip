/**
 * @file transport_factory.hpp
 * @brief Factory for creating transport instances
 *
 * This file contains the transport factory implementation which provides
 * a way to create different types of transport instances (UDP, TCP, etc.)
 * based on the specified transport type.
 *
 * @author caomengxuan666
 * @date 2025-08-30
 */

#pragma once

#include "udp_transport.hpp"
#include "net/serializer_factory.hpp"
#include <memory>
#include <string>

namespace libgossip {
    namespace net {

        /**
         * @brief Transport type enumeration
         */
        enum class transport_type {
            udp = 0,
            tcp
        };

        /**
         * @brief Factory for creating transport instances
         *
         * Provides static factory methods for creating transport instances
         * with optional serializer configuration.
         */
        class LIBGOSSIP_API transport_factory {
        public:
            /**
             * @brief Create a transport instance with default JSON serializer
             *
             * @param type The type of transport to create
             * @param host The host address for the transport
             * @param port The port number for the transport
             * @return Unique pointer to the created transport, or nullptr if creation failed
             */
            static std::unique_ptr<transport> create_transport(transport_type type,
                                                               const std::string &host,
                                                               uint16_t port);

            /**
             * @brief Create a transport instance with a specific serializer
             *
             * @param type The type of transport to create
             * @param host The host address for the transport
             * @param port The port number for the transport
             * @param serializer_name Name of the registered serializer to use (e.g., "json")
             * @return Unique pointer to the created transport, or nullptr if creation failed
             */
            static std::unique_ptr<transport> create_transport(transport_type type,
                                                               const std::string &host,
                                                               uint16_t port,
                                                               const std::string &serializer_name);
        };

    } // namespace net
} // namespace libgossip

// Keep backward compatibility alias
namespace gossip {
    namespace net {
        using transport_factory = libgossip::net::transport_factory;
        using transport_type = libgossip::net::transport_type;
    } // namespace net
} // namespace gossip

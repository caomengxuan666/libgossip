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

#ifndef LIBGOSSIP_TRANSPORT_FACTORY_HPP
#define LIBGOSSIP_TRANSPORT_FACTORY_HPP

#include "udp_transport.hpp"
#include <memory>
#include <string>

namespace gossip {
    namespace net {

        /**
     * @brief Transport type enumeration
     * 
     * Defines the supported transport types that can be created by the factory.
     */
        enum class transport_type {
            udp = 0,///< UDP transport
            tcp     ///< TCP transport
        };

        /**
     * @brief Transport factory class
     * 
     * This class provides a factory method for creating transport instances
     * based on the specified transport type. It encapsulates the creation
     * logic and provides a clean interface for creating transports.
     */
        class LIBGOSSIP_API transport_factory {
        public:
            /**
         * @brief Create a transport instance
         * 
         * Factory method to create transport instances based on the specified
         * transport type, host, and port.
         * 
         * @param type The type of transport to create
         * @param host The host address for the transport
         * @param port The port number for the transport
         * @return Unique pointer to the created transport, or nullptr if creation failed
         */
            static std::unique_ptr<transport> create_transport(transport_type type,
                                                               const std::string &host,
                                                               uint16_t port);
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_TRANSPORT_FACTORY_HPP
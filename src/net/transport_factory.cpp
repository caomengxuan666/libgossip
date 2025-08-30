/**
 * @file transport_factory.cpp
 * @brief Implementation of the transport factory
 * 
 * This file contains the implementation of the transport factory which provides
 * a way to create different types of transport instances (UDP, TCP, etc.)
 * based on the specified transport type.
 */

#include "net/transport_factory.hpp"
#include "net/tcp_transport.hpp"
#include "net/udp_transport.hpp"
namespace gossip {
    namespace net {

        std::unique_ptr<transport> transport_factory::create_transport(transport_type type,
                                                                       const std::string &host,
                                                                       uint16_t port) {
            switch (type) {
                case transport_type::udp:
                    return std::make_unique<udp_transport>(host, port);
                case transport_type::tcp:
                    return std::make_unique<tcp_transport>(host, port);
                default:
                    return nullptr;
            }
        }

    }// namespace net
}// namespace gossip
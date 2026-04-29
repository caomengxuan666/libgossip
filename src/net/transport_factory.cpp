/**
 * @file transport_factory.cpp
 * @brief Implementation of the transport factory
 */

#include "net/transport_factory.hpp"
#include "net/tcp_transport.hpp"
#include "net/udp_transport.hpp"

namespace libgossip {
    namespace net {

        std::unique_ptr<transport> transport_factory::create_transport(transport_type type,
                                                                        const std::string &host,
                                                                        uint16_t port) {
            // Default to JSON serializer
            return create_transport(type, host, port, "json");
        }

        std::unique_ptr<transport> transport_factory::create_transport(transport_type type,
                                                                        const std::string &host,
                                                                        uint16_t port,
                                                                        const std::string &serializer_name) {
            std::unique_ptr<transport> transport_ptr;

            switch (type) {
                case transport_type::udp:
                    transport_ptr = std::make_unique<udp_transport>(host, port);
                    break;
                case transport_type::tcp:
                    transport_ptr = std::make_unique<tcp_transport>(host, port);
                    break;
                default:
                    return nullptr;
            }

            if (transport_ptr) {
                auto serializer = serializer_factory::create(serializer_name);
                if (serializer) {
                    transport_ptr->set_serializer(std::move(serializer));
                }
            }

            return transport_ptr;
        }

    } // namespace net
} // namespace libgossip

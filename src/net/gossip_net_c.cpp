#include "core/gossip_core.hpp"
#include "net/gossip_net.h"
#include "net/json_serializer.hpp"
#include "net/tcp_transport.hpp"
#include "net/transport_factory.hpp"
#include "net/udp_transport.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace gossip::net;
using namespace libgossip;

// Forward declaration of gossip_core_wrapper from gossip_c.cpp
struct gossip_core_wrapper;

// Forward declarations of conversion functions from gossip_c.cpp
libgossip::node_id_t to_cpp_node_id(const gossip_node_id_t *c_node_id);
libgossip::node_view to_cpp_node_view(const gossip_node_view_t *c_node_view);
gossip_node_id_t to_c_node_id(const libgossip::node_id_t &cpp_node_id);
gossip_node_view_t to_c_node_view(const libgossip::node_view &cpp_node_view);

// Internal wrapper for C serializer
struct gossip_serializer_wrapper {
    std::unique_ptr<message_serializer> serializer;
};

// Internal wrapper for C transport
struct gossip_transport_wrapper {
    std::unique_ptr<gossip::net::transport> transport_ptr;
};

// ---------------------------------------------------------
// Serializer API implementation
// ---------------------------------------------------------

gossip_serializer_t *gossip_json_serializer_create(void) {
    try {
        auto wrapper = new gossip_serializer_wrapper();
        wrapper->serializer = std::make_unique<json_serializer>();
        return reinterpret_cast<gossip_serializer_t *>(wrapper);
    } catch (...) {
        return nullptr;
    }
}

void gossip_serializer_destroy(gossip_serializer_t *serializer) {
    if (serializer) {
        auto wrapper = reinterpret_cast<gossip_serializer_wrapper *>(serializer);
        delete wrapper;
    }
}

gossip_error_code_t gossip_serializer_serialize(const gossip_serializer_t *serializer,
                                                const gossip_message_t *msg,
                                                uint8_t **data,
                                                size_t *data_size) {
    if (!serializer || !msg || !data || !data_size) {
        return GOSSIP_ERR_INVALID_ARGUMENT;
    }

    try {
        auto wrapper = reinterpret_cast<const gossip_serializer_wrapper *>(serializer);

        // Convert C message to C++ message
        libgossip::gossip_message cpp_msg;
        cpp_msg.sender = to_cpp_node_id(&msg->sender);
        cpp_msg.type = static_cast<libgossip::message_type>(msg->type);
        cpp_msg.timestamp = msg->timestamp;

        // Note: For simplicity, we're not converting entries in this example
        // A full implementation would need to convert the entries array

        std::vector<uint8_t> serialized_data;
        auto result = wrapper->serializer->serialize(cpp_msg, serialized_data);

        if (result == error_code::success) {
            // Allocate memory for the data and copy it
            *data_size = serialized_data.size();
            *data = static_cast<uint8_t *>(std::malloc(*data_size));
            if (*data) {
                std::memcpy(*data, serialized_data.data(), *data_size);
                return GOSSIP_ERR_SUCCESS;
            } else {
                return GOSSIP_ERR_NETWORK_ERROR;
            }
        } else {
            return static_cast<gossip_error_code_t>(result);
        }
    } catch (...) {
        return GOSSIP_ERR_SERIALIZATION_ERROR;
    }
}

gossip_error_code_t gossip_serializer_deserialize(const gossip_serializer_t *serializer,
                                                  const uint8_t *data,
                                                  size_t data_size,
                                                  gossip_message_t *msg) {
    if (!serializer || !data || !msg) {
        return GOSSIP_ERR_INVALID_ARGUMENT;
    }

    try {
        auto wrapper = reinterpret_cast<const gossip_serializer_wrapper *>(serializer);

        std::vector<uint8_t> serialized_data(data, data + data_size);
        libgossip::gossip_message cpp_msg;

        auto result = wrapper->serializer->deserialize(serialized_data, cpp_msg);

        if (result == error_code::success) {
            // Convert C++ message to C message
            msg->sender = to_c_node_id(cpp_msg.sender);
            msg->type = static_cast<gossip_message_type_t>(cpp_msg.type);
            msg->timestamp = cpp_msg.timestamp;

            // Note: For simplicity, we're not converting entries in this example
            // A full implementation would need to convert the entries array
            msg->entries = nullptr;
            msg->entries_count = 0;

            return GOSSIP_ERR_SUCCESS;
        } else {
            return static_cast<gossip_error_code_t>(result);
        }
    } catch (...) {
        return GOSSIP_ERR_SERIALIZATION_ERROR;
    }
}

// ---------------------------------------------------------
// Transport API implementation
// ---------------------------------------------------------

gossip_transport_t *gossip_transport_create(gossip_transport_type_t type,
                                            const char *host,
                                            uint16_t port) {
    try {
        auto wrapper = new gossip_transport_wrapper();

        switch (type) {
            case GOSSIP_TRANSPORT_UDP:
                wrapper->transport_ptr = std::make_unique<udp_transport>(std::string(host), port);
                break;
            case GOSSIP_TRANSPORT_TCP:
                wrapper->transport_ptr = std::make_unique<tcp_transport>(std::string(host), port);
                break;
            default:
                delete wrapper;
                return nullptr;
        }

        return reinterpret_cast<gossip_transport_t *>(wrapper);
    } catch (...) {
        return nullptr;
    }
}

void gossip_transport_destroy(gossip_transport_t *transport) {
    if (transport) {
        auto wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);
        delete wrapper;
    }
}

gossip_error_code_t gossip_transport_start(gossip_transport_t *transport) {
    if (!transport) {
        return GOSSIP_ERR_INVALID_ARGUMENT;
    }

    try {
        auto wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);
        auto result = wrapper->transport_ptr->start();
        return static_cast<gossip_error_code_t>(result);
    } catch (...) {
        return GOSSIP_ERR_NETWORK_ERROR;
    }
}

gossip_error_code_t gossip_transport_stop(gossip_transport_t *transport) {
    if (!transport) {
        return GOSSIP_ERR_INVALID_ARGUMENT;
    }

    try {
        auto wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);
        auto result = wrapper->transport_ptr->stop();
        return static_cast<gossip_error_code_t>(result);
    } catch (...) {
        return GOSSIP_ERR_NETWORK_ERROR;
    }
}

gossip_error_code_t gossip_transport_send_message(gossip_transport_t *transport,
                                                  const gossip_message_t *msg,
                                                  const gossip_node_view_t *target) {
    if (!transport || !msg || !target) {
        return GOSSIP_ERR_INVALID_ARGUMENT;
    }

    try {
        auto wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);

        // Convert C structures to C++ structures
        libgossip::gossip_message cpp_msg;
        cpp_msg.sender = to_cpp_node_id(&msg->sender);
        cpp_msg.type = static_cast<libgossip::message_type>(msg->type);
        cpp_msg.timestamp = msg->timestamp;
        // Note: For simplicity, we're not converting entries in this example

        libgossip::node_view cpp_target = to_cpp_node_view(target);

        auto result = wrapper->transport_ptr->send_message(cpp_msg, cpp_target);
        return static_cast<gossip_error_code_t>(result);
    } catch (...) {
        return GOSSIP_ERR_NETWORK_ERROR;
    }
}

void gossip_transport_set_core(gossip_transport_t *transport,
                               gossip_core_t *core) {
    if (!transport) {
        return;
    }

    try {
        auto wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);
        // Get the internal C++ gossip core instance from the C wrapper
        void *core_ptr = gossip_core_get_internal(core);
        if (core_ptr) {
            // Cast to shared_ptr as expected by the transport
            std::shared_ptr<libgossip::gossip_core> shared_core(
                    static_cast<libgossip::gossip_core *>(core_ptr),
                    [](libgossip::gossip_core *) { /* Do not delete, owned by C wrapper */ });
            wrapper->transport_ptr->set_gossip_core(shared_core);
        }
    } catch (...) {
        // Silent failure in C API
    }
}

void gossip_transport_set_serializer(gossip_transport_t *transport,
                                     gossip_serializer_t *serializer) {
    if (!transport) {
        return;
    }

    try {
        auto transport_wrapper = reinterpret_cast<gossip_transport_wrapper *>(transport);
        auto serializer_wrapper = reinterpret_cast<gossip_serializer_wrapper *>(serializer);

        if (serializer_wrapper && serializer_wrapper->serializer) {
            // We need to move the unique_ptr, so we create a new one
            auto new_serializer = std::make_unique<json_serializer>();
            transport_wrapper->transport_ptr->set_serializer(std::move(new_serializer));
        }
    } catch (...) {
        // Silent failure in C API
    }
}

// ---------------------------------------------------------
// Transport factory implementation
// ---------------------------------------------------------

gossip_transport_t *gossip_transport_factory_create(gossip_transport_type_t type,
                                                    const char *host,
                                                    uint16_t port) {
    try {
        transport_type cpp_type;
        switch (type) {
            case GOSSIP_TRANSPORT_UDP:
                cpp_type = transport_type::udp;
                break;
            case GOSSIP_TRANSPORT_TCP:
                cpp_type = transport_type::tcp;
                break;
            default:
                return nullptr;
        }

        auto cpp_transport = transport_factory::create_transport(cpp_type, std::string(host), port);
        if (!cpp_transport) {
            return nullptr;
        }

        auto wrapper = new gossip_transport_wrapper();
        wrapper->transport_ptr = std::move(cpp_transport);
        return reinterpret_cast<gossip_transport_t *>(wrapper);
    } catch (...) {
        return nullptr;
    }
}
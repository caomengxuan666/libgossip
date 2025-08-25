#include "core/gossip_c.h"
#include "core/gossip_core.hpp"
#include <cstring>
#include <cstdlib>
#include <memory>

// Internal wrapper for C callbacks
struct gossip_core_wrapper {
    std::unique_ptr<libgossip::gossip_core> core;
    gossip_send_callback_t send_callback;
    gossip_event_callback_t event_callback;
    void* user_data;
};

// Convert C node_id to C++ node_id
libgossip::node_id_t to_cpp_node_id(const gossip_node_id_t* c_node_id) {
    libgossip::node_id_t cpp_node_id;
    std::memcpy(cpp_node_id.data(), c_node_id->data, 16);
    return cpp_node_id;
}

// Convert C++ node_id to C node_id
gossip_node_id_t to_c_node_id(const libgossip::node_id_t& cpp_node_id) {
    gossip_node_id_t c_node_id;
    std::memcpy(c_node_id.data, cpp_node_id.data(), 16);
    return c_node_id;
}

// Convert C node_view to C++ node_view
libgossip::node_view to_cpp_node_view(const gossip_node_view_t* c_node_view) {
    libgossip::node_view cpp_node_view;
    cpp_node_view.id = to_cpp_node_id(&c_node_view->id);
    cpp_node_view.ip = std::string(c_node_view->ip);
    cpp_node_view.port = c_node_view->port;
    cpp_node_view.config_epoch = c_node_view->config_epoch;
    cpp_node_view.heartbeat = c_node_view->heartbeat;
    cpp_node_view.version = c_node_view->version;
    cpp_node_view.status = static_cast<libgossip::node_status>(c_node_view->status);
    cpp_node_view.role = std::string(c_node_view->role);
    cpp_node_view.region = std::string(c_node_view->region);
    // Note: metadata is not currently supported in C API
    return cpp_node_view;
}

// Convert C++ node_view to C node_view
gossip_node_view_t to_c_node_view(const libgossip::node_view& cpp_node_view) {
    gossip_node_view_t c_node_view = {0};
    c_node_view.id = to_c_node_id(cpp_node_view.id);
    std::strncpy(c_node_view.ip, cpp_node_view.ip.c_str(), sizeof(c_node_view.ip) - 1);
    c_node_view.port = cpp_node_view.port;
    c_node_view.config_epoch = cpp_node_view.config_epoch;
    c_node_view.heartbeat = cpp_node_view.heartbeat;
    c_node_view.version = cpp_node_view.version;
    c_node_view.status = static_cast<gossip_node_status_t>(cpp_node_view.status);
    std::strncpy(c_node_view.role, cpp_node_view.role.c_str(), sizeof(c_node_view.role) - 1);
    std::strncpy(c_node_view.region, cpp_node_view.region.c_str(), sizeof(c_node_view.region) - 1);
    // Stats are not part of node_view in C++, so we initialize them to 0
    c_node_view.sent_messages = 0;
    c_node_view.received_messages = 0;
    return c_node_view;
}

// Convert C message to C++ message
libgossip::gossip_message to_cpp_message(const gossip_message_t* c_msg) {
    libgossip::gossip_message cpp_msg;
    cpp_msg.sender = to_cpp_node_id(&c_msg->sender);
    cpp_msg.type = static_cast<libgossip::message_type>(c_msg->type);
    cpp_msg.timestamp = c_msg->timestamp;
    
    cpp_msg.entries.reserve(c_msg->entries_count);
    for (size_t i = 0; i < c_msg->entries_count; ++i) {
        cpp_msg.entries.push_back(to_cpp_node_view(&c_msg->entries[i]));
    }
    
    return cpp_msg;
}

// Convert C++ message to C message
gossip_message_t to_c_message(const libgossip::gossip_message& cpp_msg) {
    gossip_message_t c_msg = {0};
    c_msg.sender = to_c_node_id(cpp_msg.sender);
    c_msg.type = static_cast<gossip_message_type_t>(cpp_msg.type);
    c_msg.timestamp = cpp_msg.timestamp;
    
    c_msg.entries_count = cpp_msg.entries.size();
    if (c_msg.entries_count > 0) {
        c_msg.entries = static_cast<gossip_node_view_t*>(std::malloc(sizeof(gossip_node_view_t) * c_msg.entries_count));
        for (size_t i = 0; i < c_msg.entries_count; ++i) {
            c_msg.entries[i] = to_c_node_view(cpp_msg.entries[i]);
        }
    }
    
    return c_msg;
}

// Free C message resources
void free_c_message(gossip_message_t* c_msg) {
    if (c_msg->entries) {
        std::free(c_msg->entries);
        c_msg->entries = nullptr;
    }
    c_msg->entries_count = 0;
}

// C++ callback wrappers
void send_callback_wrapper(const libgossip::gossip_message& cpp_msg, 
                          const libgossip::node_view& cpp_target, 
                          gossip_core_wrapper* wrapper) {
    if (wrapper && wrapper->send_callback) {
        gossip_message_t c_msg = to_c_message(cpp_msg);
        gossip_node_view_t c_target = to_c_node_view(cpp_target);
        wrapper->send_callback(&c_msg, &c_target, wrapper->user_data);
        free_c_message(&c_msg);
    }
}

void event_callback_wrapper(const libgossip::node_view& cpp_node, 
                           libgossip::node_status cpp_old_status, 
                           gossip_core_wrapper* wrapper) {
    if (wrapper && wrapper->event_callback) {
        gossip_node_view_t c_node = to_c_node_view(cpp_node);
        gossip_node_status_t c_old_status = static_cast<gossip_node_status_t>(cpp_old_status);
        wrapper->event_callback(&c_node, c_old_status, wrapper->user_data);
    }
}

// C API implementation
extern "C" {

gossip_core_t* gossip_core_create(const gossip_node_view_t* self_node,
                                  gossip_send_callback_t send_callback,
                                  gossip_event_callback_t event_callback,
                                  void* user_data) {
    if (!self_node) {
        return nullptr;
    }
    
    try {
        auto wrapper = new gossip_core_wrapper;
        wrapper->send_callback = send_callback;
        wrapper->event_callback = event_callback;
        wrapper->user_data = user_data;
        
        libgossip::node_view cpp_self = to_cpp_node_view(self_node);
        
        // Create lambda wrappers for callbacks
        auto send_fn = [wrapper](const libgossip::gossip_message& msg, 
                                const libgossip::node_view& target) {
            send_callback_wrapper(msg, target, wrapper);
        };
        
        auto event_fn = [wrapper](const libgossip::node_view& node, 
                                 libgossip::node_status old_status) {
            event_callback_wrapper(node, old_status, wrapper);
        };
        
        wrapper->core = std::make_unique<libgossip::gossip_core>(cpp_self, send_fn, event_fn);
        return (gossip_core_t*)wrapper;
    } catch (...) {
        return nullptr;
    }
}

void gossip_core_destroy(gossip_core_t* core) {
    if (core) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        delete wrapper;
    }
}

void gossip_core_tick(gossip_core_t* core) {
    if (core) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        wrapper->core->tick();
    }
}

void gossip_core_tick_full_broadcast(gossip_core_t* core) {
    if (core) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        wrapper->core->tick_full_broadcast();
    }
}

void gossip_core_handle_message(gossip_core_t* core, const gossip_message_t* msg) {
    if (core && msg) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        libgossip::gossip_message cpp_msg = to_cpp_message(msg);
        wrapper->core->handle_message(cpp_msg, libgossip::clock::now());
    }
}

void gossip_core_meet(gossip_core_t* core, const gossip_node_view_t* node) {
    if (core && node) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        libgossip::node_view cpp_node = to_cpp_node_view(node);
        wrapper->core->meet(cpp_node);
    }
}

void gossip_core_join(gossip_core_t* core, const gossip_node_view_t* node) {
    if (core && node) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        libgossip::node_view cpp_node = to_cpp_node_view(node);
        wrapper->core->join(cpp_node);
    }
}

void gossip_core_leave(gossip_core_t* core, const gossip_node_id_t* node_id) {
    if (core && node_id) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        libgossip::node_id_t cpp_node_id = to_cpp_node_id(node_id);
        wrapper->core->leave(cpp_node_id);
    }
}

const gossip_node_view_t* gossip_core_self(const gossip_core_t* core) {
    if (!core) {
        return nullptr;
    }
    
    gossip_core_wrapper* wrapper = const_cast<gossip_core_wrapper*>(
        (const gossip_core_wrapper*)core);
    
    // We need to be careful here - we're returning a pointer to a temporary
    // In a real implementation, we'd want to manage this more carefully
    static gossip_node_view_t static_self_view;
    static_self_view = to_c_node_view(wrapper->core->self());
    return &static_self_view;
}

gossip_node_view_t* gossip_core_get_nodes(const gossip_core_t* core, size_t* count) {
    if (!core || !count) {
        return nullptr;
    }
    
    gossip_core_wrapper* wrapper = const_cast<gossip_core_wrapper*>(
        (const gossip_core_wrapper*)core);
    
    std::vector<libgossip::node_view> cpp_nodes = wrapper->core->get_nodes();
    *count = cpp_nodes.size();
    
    if (*count == 0) {
        return nullptr;
    }
    
    gossip_node_view_t* c_nodes = (gossip_node_view_t*)std::malloc(
        sizeof(gossip_node_view_t) * (*count));
    
    if (!c_nodes) {
        *count = 0;
        return nullptr;
    }
    
    for (size_t i = 0; i < *count; ++i) {
        c_nodes[i] = to_c_node_view(cpp_nodes[i]);
    }
    
    return c_nodes;
}

void gossip_core_free_nodes(gossip_node_view_t* nodes) {
    if (nodes) {
        std::free(nodes);
    }
}

int gossip_core_find_node(const gossip_core_t* core, 
                          const gossip_node_id_t* id, 
                          gossip_node_view_t* out_node) {
    if (!core || !id || !out_node) {
        return 0; // Not found
    }
    
    gossip_core_wrapper* wrapper = const_cast<gossip_core_wrapper*>(
        (const gossip_core_wrapper*)core);
    
    libgossip::node_id_t cpp_id = to_cpp_node_id(id);
    std::optional<libgossip::node_view> cpp_node = wrapper->core->find_node(cpp_id);
    
    if (cpp_node.has_value()) {
        *out_node = to_c_node_view(cpp_node.value());
        return 1; // Found
    }
    
    return 0; // Not found
}

size_t gossip_core_size(const gossip_core_t* core) {
    if (!core) {
        return 0;
    }
    
    gossip_core_wrapper* wrapper = const_cast<gossip_core_wrapper*>(
        (const gossip_core_wrapper*)core);
    
    return wrapper->core->size();
}

void gossip_core_reset(gossip_core_t* core) {
    if (core) {
        gossip_core_wrapper* wrapper = (gossip_core_wrapper*)core;
        wrapper->core->reset();
    }
}

} // extern "C"
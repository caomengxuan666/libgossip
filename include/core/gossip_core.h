//​
//  _ _ _                         _       
// | (_) |__   __ _  ___  ___ ___(_)_ __  
// | | | '_ \ / _` |/ _ \/ __/ __| | '_ \_
// | | | |_) | (_| | (_) \__ \__ \ | |_) |
// |_|_|_.__/ \__, |\___/|___/___/_| .__/ 
//            |___/                |_|    
// Project: libgossip
// Repository: https://github.com/caomengxuan666/libgossip
// Version: 1.1.2
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Caomengxuan.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//​END

#ifndef GOSSIP_C_H
#define GOSSIP_C_H

#include "libgossip_api.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------
// Basic type definitions
// ---------------------------------------------------------

/// Node unique ID, 16 bytes (e.g. UUID or MD5)
typedef struct gossip_node_id {
    uint8_t data[16];
} gossip_node_id_t;

/// Node status flags
typedef enum gossip_node_status {
    GOSSIP_NODE_UNKNOWN = 0,
    GOSSIP_NODE_JOINING,
    GOSSIP_NODE_ONLINE,
    GOSSIP_NODE_SUSPECT,
    GOSSIP_NODE_FAILED
} gossip_node_status_t;

/// Message types
typedef enum gossip_message_type {
    GOSSIP_MSG_PING = 0,
    GOSSIP_MSG_PONG,
    GOSSIP_MSG_MEET,
    GOSSIP_MSG_JOIN,
    GOSSIP_MSG_LEAVE,
    GOSSIP_MSG_UPDATE
} gossip_message_type_t;

// Forward declaration
typedef struct gossip_core gossip_core_t;

// ---------------------------------------------------------
// Node view structure
// ---------------------------------------------------------

typedef struct gossip_node_view {
    gossip_node_id_t id;
    char ip[64];
    int port;
    uint64_t config_epoch;
    uint64_t heartbeat;
    uint64_t version;
    gossip_node_status_t status;

    // Business extension fields
    char role[64];
    char region[64];

    // Statistics
    uint64_t sent_messages;
    uint64_t received_messages;
} gossip_node_view_t;

// ---------------------------------------------------------
// Message structure
// ---------------------------------------------------------

typedef struct gossip_message {
    gossip_node_id_t sender;
    gossip_message_type_t type;
    uint64_t timestamp;
    gossip_node_view_t *entries;
    size_t entries_count;
} gossip_message_t;

// ---------------------------------------------------------
// Callback function types
// ---------------------------------------------------------

/// Send message callback
typedef void (*gossip_send_callback_t)(const gossip_message_t *msg,
                                       const gossip_node_view_t *target,
                                       void *user_data);

/// Event notification callback
typedef void (*gossip_event_callback_t)(const gossip_node_view_t *node,
                                        gossip_node_status_t old_status,
                                        void *user_data);

// ---------------------------------------------------------
// Core API functions
// ---------------------------------------------------------

/// Create a new gossip core instance
LIBGOSSIP_API gossip_core_t *gossip_core_create(const gossip_node_view_t *self_node,
                                                gossip_send_callback_t send_callback,
                                                gossip_event_callback_t event_callback,
                                                void *user_data);

/// Destroy a gossip core instance
LIBGOSSIP_API void gossip_core_destroy(gossip_core_t *core);

/// Drive one gossip cycle
LIBGOSSIP_API void gossip_core_tick(gossip_core_t *core);

/// Drive a complete broadcast gossip cycle
LIBGOSSIP_API void gossip_core_tick_full_broadcast(gossip_core_t *core);

/// Process a received gossip message
LIBGOSSIP_API void gossip_core_handle_message(gossip_core_t *core,
                                              const gossip_message_t *msg);

/// Actively initiate join: introduce a new node
LIBGOSSIP_API void gossip_core_meet(gossip_core_t *core, const gossip_node_view_t *node);

/// Explicitly join the cluster
LIBGOSSIP_API void gossip_core_join(gossip_core_t *core, const gossip_node_view_t *node);

/// Explicitly leave the cluster
LIBGOSSIP_API void gossip_core_leave(gossip_core_t *core, const gossip_node_id_t *node_id);

/// Get self node view
LIBGOSSIP_API const gossip_node_view_t *gossip_core_self(const gossip_core_t *core);

/// Get all currently known nodes
LIBGOSSIP_API gossip_node_view_t *gossip_core_get_nodes(const gossip_core_t *core,
                                                        size_t *count);

/// Free node list returned by gossip_core_get_nodes
LIBGOSSIP_API void gossip_core_free_nodes(gossip_node_view_t *nodes);

/// Find node by ID
LIBGOSSIP_API int gossip_core_find_node(const gossip_core_t *core,
                                        const gossip_node_id_t *id,
                                        gossip_node_view_t *out_node);

/// Get node count
LIBGOSSIP_API size_t gossip_core_size(const gossip_core_t *core);

/// Reset core state
LIBGOSSIP_API void gossip_core_reset(gossip_core_t *core);

/// Get the internal C++ gossip core instance from the C wrapper
LIBGOSSIP_API void *gossip_core_get_internal(gossip_core_t *core);

#ifdef __cplusplus
}
#endif

#endif// GOSSIP_C_H
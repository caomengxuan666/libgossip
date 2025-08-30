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

#ifndef GOSSIP_NET_C_H
#define GOSSIP_NET_C_H

#include "../core/gossip_core.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------
// Basic type definitions for network layer
// ---------------------------------------------------------

/// Forward declarations
typedef struct gossip_transport gossip_transport_t;
typedef struct gossip_serializer gossip_serializer_t;

/// Error codes for network operations
typedef enum gossip_error_code {
    GOSSIP_ERR_SUCCESS = 0,           ///< Operation completed successfully
    GOSSIP_ERR_NETWORK_ERROR,         ///< Network error occurred
    GOSSIP_ERR_SERIALIZATION_ERROR,   ///< Serialization/deserialization error
    GOSSIP_ERR_INVALID_ARGUMENT,      ///< Invalid argument provided
    GOSSIP_ERR_OPERATION_NOT_PERMITTED///< Operation not permitted in current state
} gossip_error_code_t;

/// Transport types
typedef enum gossip_transport_type {
    GOSSIP_TRANSPORT_UDP = 0,///< UDP transport
    GOSSIP_TRANSPORT_TCP     ///< TCP transport
} gossip_transport_type_t;

// ---------------------------------------------------------
// Serializer API functions
// ---------------------------------------------------------

/// Create a JSON serializer instance
LIBGOSSIP_API gossip_serializer_t *gossip_json_serializer_create(void);

/// Destroy a serializer instance
LIBGOSSIP_API void gossip_serializer_destroy(gossip_serializer_t *serializer);

/// Serialize a message using the serializer
LIBGOSSIP_API gossip_error_code_t gossip_serializer_serialize(const gossip_serializer_t *serializer,
                                                              const gossip_message_t *msg,
                                                              uint8_t **data,
                                                              size_t *data_size);

/// Deserialize data to a message using the serializer
LIBGOSSIP_API gossip_error_code_t gossip_serializer_deserialize(const gossip_serializer_t *serializer,
                                                                const uint8_t *data,
                                                                size_t data_size,
                                                                gossip_message_t *msg);

// ---------------------------------------------------------
// Transport API functions
// ---------------------------------------------------------

/// Create a transport instance
LIBGOSSIP_API gossip_transport_t *gossip_transport_create(gossip_transport_type_t type,
                                                          const char *host,
                                                          uint16_t port);

/// Destroy a transport instance
LIBGOSSIP_API void gossip_transport_destroy(gossip_transport_t *transport);

/// Start the transport
LIBGOSSIP_API gossip_error_code_t gossip_transport_start(gossip_transport_t *transport);

/// Stop the transport
LIBGOSSIP_API gossip_error_code_t gossip_transport_stop(gossip_transport_t *transport);

/// Send a message synchronously
LIBGOSSIP_API gossip_error_code_t gossip_transport_send_message(gossip_transport_t *transport,
                                                                const gossip_message_t *msg,
                                                                const gossip_node_view_t *target);

/// Set the gossip core for the transport
LIBGOSSIP_API void gossip_transport_set_core(gossip_transport_t *transport,
                                             gossip_core_t *core);

/// Set the serializer for the transport
LIBGOSSIP_API void gossip_transport_set_serializer(gossip_transport_t *transport,
                                                   gossip_serializer_t *serializer);

// ---------------------------------------------------------
// Transport factory functions
// ---------------------------------------------------------

/// Create a transport using the factory
LIBGOSSIP_API gossip_transport_t *gossip_transport_factory_create(gossip_transport_type_t type,
                                                                  const char *host,
                                                                  uint16_t port);

#ifdef __cplusplus
}
#endif

#endif// GOSSIP_NET_C_H
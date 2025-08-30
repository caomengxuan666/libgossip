/**
 * @file net_c_api_example.c
 * @brief Example of using the libgossip C API with network layer
 * 
 * This example demonstrates how to use the libgossip C API with the network layer
 * to create a node that can communicate with other nodes in the cluster.
 */

#include "core/gossip_core.h"
#include "net/gossip_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP(seconds) Sleep((seconds) * 1000)
#else
#include <unistd.h>
#define SLEEP(seconds) sleep(seconds)
#endif

// Global transport pointer so we can use it in callbacks
gossip_transport_t *global_transport = NULL;

// Send callback - this is where we use the network layer to send messages
void send_callback(const gossip_message_t *msg,
                   const gossip_node_view_t *target,
                   void *user_data) {
    printf("Core wants to send message to node %s:%d\n", target->ip, target->port);
    
    // Use the network layer to send messages
    if (global_transport) {
        gossip_error_code_t err = gossip_transport_send_message(global_transport, msg, target);
        if (err != GOSSIP_ERR_SUCCESS) {
            printf("Failed to send message over network: %d\n", err);
        } else {
            printf("Message sent successfully over network\n");
        }
    }
}

// Simple event callback
void event_callback(const gossip_node_view_t *node,
                    gossip_node_status_t old_status,
                    void *user_data) {
    printf("Node %s:%d status changed from %d to %d\n", 
           node->ip, node->port, old_status, node->status);
}

int main() {
    printf("libgossip C API with network layer example\n");
    
    // Create self node view
    gossip_node_view_t self_node = {0};
    strcpy(self_node.ip, "127.0.0.1");
    self_node.port = 8000;
    self_node.status = GOSSIP_NODE_JOINING;
    strcpy(self_node.role, "example");
    strcpy(self_node.region, "local");
    
    // Create gossip core
    gossip_core_t *core = gossip_core_create(&self_node, send_callback, event_callback, NULL);
    if (!core) {
        fprintf(stderr, "Failed to create gossip core\n");
        return 1;
    }
    
    // Create a JSON serializer
    gossip_serializer_t *serializer = gossip_json_serializer_create();
    if (!serializer) {
        fprintf(stderr, "Failed to create JSON serializer\n");
        gossip_core_destroy(core);
        return 1;
    }
    
    // Create a UDP transport
    gossip_transport_t *transport = gossip_transport_factory_create(
        GOSSIP_TRANSPORT_UDP, "127.0.0.1", 8000);
    if (!transport) {
        fprintf(stderr, "Failed to create UDP transport\n");
        gossip_serializer_destroy(serializer);
        gossip_core_destroy(core);
        return 1;
    }
    
    // Store transport in global variable for use in callbacks
    global_transport = transport;
    
    // Configure the transport
    gossip_transport_set_core(transport, core);
    gossip_transport_set_serializer(transport, serializer);
    
    // Start the transport
    gossip_error_code_t err = gossip_transport_start(transport);
    if (err != GOSSIP_ERR_SUCCESS) {
        fprintf(stderr, "Failed to start transport: %d\n", err);
        gossip_transport_destroy(transport);
        gossip_serializer_destroy(serializer);
        gossip_core_destroy(core);
        return 1;
    }
    
    printf("libgossip C API with network layer example started\n");
    printf("Node listening on 127.0.0.1:8000\n");
    
    // Demonstrate serialization
    printf("Testing message serialization...\n");
    gossip_message_t test_msg = {0};
    // Create a node view entry for the message
    gossip_node_view_t test_node = {0};
    strcpy(test_node.ip, "127.0.0.1");
    test_node.port = 8000;
    test_node.status = GOSSIP_NODE_ONLINE;
    strcpy(test_node.role, "test");
    strcpy(test_node.region, "local");
    
    // Assign the node view to the message entries
    test_msg.entries = &test_node;
    test_msg.entries_count = 1;
    test_msg.type = GOSSIP_MSG_PING;
    test_msg.timestamp = 1234567890;
    // For sender, we just zero it out as it's a gossip_node_id_t (16-byte array)
    memset(test_msg.sender.data, 0, sizeof(test_msg.sender.data));
    
    uint8_t *serialized_data = NULL;
    size_t data_size = 0;
    gossip_error_code_t serialize_err = gossip_serializer_serialize(serializer, &test_msg, &serialized_data, &data_size);
    if (serialize_err == GOSSIP_ERR_SUCCESS) {
        printf("Message serialized successfully. Size: %zu bytes\n", data_size);
        
        // Try to deserialize
        gossip_message_t deserialized_msg = {0};
        gossip_error_code_t deserialize_err = gossip_serializer_deserialize(serializer, serialized_data, data_size, &deserialized_msg);
        if (deserialize_err == GOSSIP_ERR_SUCCESS) {
            printf("Message deserialized successfully\n");
            if (deserialized_msg.entries_count > 0 && deserialized_msg.entries) {
                printf("Deserialized node: %s:%d\n", 
                       deserialized_msg.entries[0].ip, 
                       deserialized_msg.entries[0].port);
            }
        } else {
            printf("Failed to deserialize message: %d\n", deserialize_err);
        }
        
        // Free the serialized data
        free(serialized_data);
    } else {
        printf("Failed to serialize message: %d\n", serialize_err);
    }
    
    // Run a few gossip cycles to demonstrate functionality
    printf("Running gossip protocol cycles...\n");
    for (int i = 0; i < 3; i++) {
        printf("Gossip cycle %d\n", i+1);
        gossip_core_tick(core);
        SLEEP(1); // Sleep for a second between cycles
    }
    
    // Demonstrate core functions
    size_t node_count = gossip_core_size(core);
    printf("Current node count: %zu\n", node_count);
    
    const gossip_node_view_t *self_view = gossip_core_self(core);
    if (self_view) {
        printf("Self node: %s:%d\n", self_view->ip, self_view->port);
    }
    
    // Cleanup
    gossip_transport_stop(transport);
    gossip_transport_destroy(transport);
    gossip_serializer_destroy(serializer);
    gossip_core_destroy(core);
    
    printf("Example completed successfully\n");
    return 0;
}
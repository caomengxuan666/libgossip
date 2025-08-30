#include "core/gossip_core.h"
#include <stdio.h>
#include <string.h>

// Simple network simulation
typedef struct {
    gossip_core_t* core;
    int node_id;
} node_context_t;

// Mock send function
void mock_send_callback(const gossip_message_t* msg, 
                       const gossip_node_view_t* target, 
                       void* user_data) {
    node_context_t* ctx = (node_context_t*)user_data;
    printf("[Node %d] Sending message type %d to node %.*s\n", 
           ctx->node_id, msg->type, 8, target->ip);
}

// Mock event callback
void mock_event_callback(const gossip_node_view_t* node, 
                        gossip_node_status_t old_status, 
                        void* user_data) {
    node_context_t* ctx = (node_context_t*)user_data;
    printf("[Node %d] Node %.*s status changed from %d to %d\n", 
           ctx->node_id, 8, node->ip, old_status, node->status);
}

int main() {
    printf("libgossip C API Example\n");
    printf("=======================\n");

    // Create node 1
    gossip_node_view_t node1_view = {0};
    memcpy(node1_view.id.data, "node1-node1-node1", 16);
    strncpy(node1_view.ip, "127.0.0.1", sizeof(node1_view.ip) - 1);
    node1_view.port = 8001;
    node1_view.status = GOSSIP_NODE_ONLINE;
    strncpy(node1_view.role, "master", sizeof(node1_view.role) - 1);

    node_context_t ctx1 = {0};
    ctx1.node_id = 1;
    
    gossip_core_t* core1 = gossip_core_create(&node1_view, 
                                              mock_send_callback, 
                                              mock_event_callback, 
                                              &ctx1);

    if (!core1) {
        printf("Failed to create gossip core 1\n");
        return 1;
    }

    // Create node 2
    gossip_node_view_t node2_view = {0};
    memcpy(node2_view.id.data, "node2-node2-node2", 16);
    strncpy(node2_view.ip, "127.0.0.2", sizeof(node2_view.ip) - 1);
    node2_view.port = 8002;
    node2_view.status = GOSSIP_NODE_ONLINE;
    strncpy(node2_view.role, "replica", sizeof(node2_view.role) - 1);

    node_context_t ctx2 = {0};
    ctx2.node_id = 2;
    
    gossip_core_t* core2 = gossip_core_create(&node2_view, 
                                              mock_send_callback, 
                                              mock_event_callback, 
                                              &ctx2);

    if (!core2) {
        printf("Failed to create gossip core 2\n");
        gossip_core_destroy(core1);
        return 1;
    }

    printf("Created two nodes\n");

    // Introduce nodes to each other (meet)
    gossip_core_meet(core1, &node2_view);
    gossip_core_meet(core2, &node1_view);

    printf("Nodes introduced to each other\n");

    // Run a few ticks
    for (int i = 0; i < 3; i++) {
        printf("\n--- Tick %d ---\n", i + 1);
        gossip_core_tick(core1);
        gossip_core_tick(core2);
    }

    // Get node information
    size_t node_count1, node_count2;
    gossip_node_view_t* nodes1 = gossip_core_get_nodes(core1, &node_count1);
    gossip_node_view_t* nodes2 = gossip_core_get_nodes(core2, &node_count2);

    printf("\nNode 1 knows about %zu nodes\n", node_count1);
    printf("Node 2 knows about %zu nodes\n", node_count2);

    // Clean up
    gossip_core_free_nodes(nodes1);
    gossip_core_free_nodes(nodes2);
    gossip_core_destroy(core1);
    gossip_core_destroy(core2);

    printf("\nExample completed successfully!\n");
    return 0;
}
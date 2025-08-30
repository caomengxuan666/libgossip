#include "core/gossip_core.hpp"
#include <gtest/gtest.h>

using namespace libgossip;

// Test fixtures
class GossipCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test node
        self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
        self_node.ip = "127.0.0.1";
        self_node.port = 8000;
        self_node.status = node_status::online;
    }

    node_view self_node;
};

// Mock callbacks
void mock_send_callback(const gossip_message &msg, const node_view &target) {
    // Do nothing for tests
}

void mock_event_callback(const node_view &node, node_status old_status) {
    // Do nothing for tests
}

// Tests
TEST_F(GossipCoreTest, ConstructorTest) {
    EXPECT_NO_THROW({
        gossip_core core(self_node, mock_send_callback, mock_event_callback);
    });
}

TEST_F(GossipCoreTest, SelfNodeTest) {
    gossip_core core(self_node, mock_send_callback, mock_event_callback);
    const node_view &self = core.self();
    EXPECT_EQ(self.id, self_node.id);
    EXPECT_EQ(self.ip, self_node.ip);
    EXPECT_EQ(self.port, self_node.port);
}

TEST_F(GossipCoreTest, SizeTest) {
    gossip_core core(self_node, mock_send_callback, mock_event_callback);
    EXPECT_EQ(core.size(), 0);
}

TEST_F(GossipCoreTest, MeetTest) {
    gossip_core core(self_node, mock_send_callback, mock_event_callback);

    node_view other_node;
    other_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};
    other_node.ip = "127.0.0.2";
    other_node.port = 8001;
    other_node.status = node_status::joining;

    core.meet(other_node);
    EXPECT_EQ(core.size(), 1);

    auto nodes = core.get_nodes();
    ASSERT_EQ(nodes.size(), 1);
    EXPECT_EQ(nodes[0].id, other_node.id);
    EXPECT_EQ(nodes[0].ip, other_node.ip);
    EXPECT_EQ(nodes[0].port, other_node.port);
}

TEST_F(GossipCoreTest, FindNodeTest) {
    gossip_core core(self_node, mock_send_callback, mock_event_callback);

    node_view other_node;
    other_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};
    other_node.ip = "127.0.0.2";
    other_node.port = 8001;
    other_node.status = node_status::joining;

    core.meet(other_node);

    // Find existing node
    auto found = core.find_node(other_node.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value().id, other_node.id);

    // Try to find non-existing node
    node_id_t non_existing_id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99}};
    auto not_found = core.find_node(non_existing_id);
    EXPECT_FALSE(not_found.has_value());

    // Find self node
    auto self_found = core.find_node(self_node.id);
    ASSERT_TRUE(self_found.has_value());
    EXPECT_EQ(self_found.value().id, self_node.id);
}

TEST_F(GossipCoreTest, NodeViewComparisonTest) {
    node_view node1, node2;

    // Initialize nodes
    node1.heartbeat = 10;
    node1.config_epoch = 5;

    node2.heartbeat = 5;
    node2.config_epoch = 5;

    // Test newer_than
    EXPECT_TRUE(node1.newer_than(node2));
    EXPECT_FALSE(node2.newer_than(node1));

    // Test with same heartbeat but different config_epoch
    node2.heartbeat = 10;
    node2.config_epoch = 3;
    EXPECT_TRUE(node1.newer_than(node2));
    EXPECT_FALSE(node2.newer_than(node1));

    // Test with same values
    node2.config_epoch = 5;
    EXPECT_FALSE(node1.newer_than(node2));
    EXPECT_FALSE(node2.newer_than(node1));
}

TEST_F(GossipCoreTest, NodeViewCanReplaceTest) {
    node_view node1, node2;

    // Initialize nodes
    node1.heartbeat = 10;
    node1.config_epoch = 5;

    node2.heartbeat = 5;
    node2.config_epoch = 5;

    // Test can_replace
    EXPECT_TRUE(node1.can_replace(node2));
    EXPECT_FALSE(node2.can_replace(node1));

    // Test with different config_epoch - higher config_epoch always wins
    node2.heartbeat = 15;                  // Higher heartbeat
    node2.config_epoch = 3;                // But lower config_epoch
    EXPECT_TRUE(node1.can_replace(node2)); // node1 wins because of higher config_epoch
    EXPECT_FALSE(node2.can_replace(node1));// node2 can't replace node1 due to lower config_epoch

    // Now test with higher config_epoch
    node2.heartbeat = 1;                   // Lower heartbeat
    node2.config_epoch = 7;                // But higher config_epoch
    EXPECT_FALSE(node1.can_replace(node2));// node1 can't replace node2 due to lower config_epoch
    EXPECT_TRUE(node2.can_replace(node1)); // node2 wins because of higher config_epoch

    // Test with same values
    node2.heartbeat = 10;
    node2.config_epoch = 5;
    EXPECT_FALSE(node1.can_replace(node2));
    EXPECT_FALSE(node2.can_replace(node1));

    // Test with same config_epoch but different heartbeat
    node2.heartbeat = 15;
    node2.config_epoch = 5;
    EXPECT_FALSE(node1.can_replace(node2));
    EXPECT_TRUE(node2.can_replace(node1));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#include "core/gossip_manager.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace libgossip;
using namespace std::chrono_literals;

class GossipManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a basic config
        config.bind_ip = "127.0.0.1";
        config.gossip_port = 17946; // Use high port to avoid conflicts
        config.serializer = "json";
    }

    gossip_config config;
};

TEST_F(GossipManagerTest, DefaultConstruction) {
    gossip_manager manager;
    EXPECT_FALSE(manager.is_initialized());
    EXPECT_FALSE(manager.is_running());
}

TEST_F(GossipManagerTest, InitSuccess) {
    gossip_manager manager;
    EXPECT_TRUE(manager.init(config));
    EXPECT_TRUE(manager.is_initialized());
    EXPECT_FALSE(manager.is_running());
}

TEST_F(GossipManagerTest, DoubleInitFails) {
    gossip_manager manager;
    EXPECT_TRUE(manager.init(config));
    EXPECT_FALSE(manager.init(config));
}

TEST_F(GossipManagerTest, StartWithoutInitFails) {
    gossip_manager manager;
    EXPECT_FALSE(manager.start());
}

TEST_F(GossipManagerTest, StartStop) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    EXPECT_TRUE(manager.start());
    EXPECT_TRUE(manager.is_running());

    manager.stop();
    EXPECT_FALSE(manager.is_running());
}

TEST_F(GossipManagerTest, TickWithoutStart) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    // Should not crash
    manager.tick();
}

TEST_F(GossipManagerTest, GetSelf) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    ASSERT_TRUE(manager.start());

    auto self = manager.get_self();
    EXPECT_EQ(self.ip, "127.0.0.1");
    EXPECT_EQ(self.port, 17946);
    EXPECT_EQ(self.role, "master");
    EXPECT_EQ(self.status, node_status::online);

    manager.stop();
}

TEST_F(GossipManagerTest, GetNodesEmpty) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    ASSERT_TRUE(manager.start());

    auto nodes = manager.get_nodes();
    EXPECT_TRUE(nodes.empty());
    EXPECT_EQ(manager.get_node_count(), 0);

    manager.stop();
}

TEST_F(GossipManagerTest, UpdateMetadata) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    ASSERT_TRUE(manager.start());

    std::map<std::string, std::string> metadata;
    metadata["key1"] = "value1";
    metadata["key2"] = "value2";

    // Should not crash
    manager.update_metadata(metadata);

    auto self = manager.get_self();
    EXPECT_EQ(self.metadata.at("key1"), "value1");
    EXPECT_EQ(self.metadata.at("key2"), "value2");

    manager.stop();
}

TEST_F(GossipManagerTest, GetStats) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));
    ASSERT_TRUE(manager.start());

    auto stats = manager.get_stats();
    EXPECT_EQ(stats.known_nodes, 0);
    EXPECT_EQ(stats.sent_messages, 0);
    EXPECT_EQ(stats.received_messages, 0);

    manager.stop();
}

TEST_F(GossipManagerTest, SetEventCallback) {
    gossip_manager manager;
    ASSERT_TRUE(manager.init(config));

    bool callback_called = false;
    manager.set_event_callback([&callback_called](const node_view&, node_status, node_status) {
        callback_called = true;
    });

    // Callback should be set but not called yet
    EXPECT_FALSE(callback_called);

    manager.stop();
}

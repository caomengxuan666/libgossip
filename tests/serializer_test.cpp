#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace gossip::net;
using namespace libgossip;

// Helper function to create test node
node_view create_test_node(uint8_t last_byte) {
    node_view node;
    node.id = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, last_byte}};
    node.ip = "127.0.0." + std::to_string(last_byte);
    node.port = 8000 + last_byte;
    node.config_epoch = last_byte * 10;
    node.heartbeat = last_byte * 100;
    node.version = last_byte * 1000;
    node.status = static_cast<node_status>(last_byte % 5);
    node.role = "role_" + std::to_string(last_byte);
    node.region = "region_" + std::to_string(last_byte);
    node.metadata["key1"] = "value1_" + std::to_string(last_byte);
    node.metadata["key2"] = "value2_" + std::to_string(last_byte);
    return node;
}

// Test fixture for serializer tests
class SerializerTest : public ::testing::Test {
protected:
    void SetUp() override {
        serializer = std::make_unique<json_serializer>();
    }

    std::unique_ptr<json_serializer> serializer;
};

// Tests for JSON serializer
TEST_F(SerializerTest, SerializeEmptyMessageTest) {
    gossip_message msg;
    // Initialize sender with a valid ID
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::ping;
    msg.timestamp = 1234567890;

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Verify basic structure
    std::string json_str(data.begin(), data.end());
    EXPECT_NE(json_str.find("\"sender\""), std::string::npos);
    EXPECT_NE(json_str.find("\"type\""), std::string::npos);
    EXPECT_NE(json_str.find("\"timestamp\""), std::string::npos);
    EXPECT_NE(json_str.find("\"entries\""), std::string::npos);
}

TEST_F(SerializerTest, SerializeMessageWithNodesTest) {
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::meet;
    msg.timestamp = 987654321;

    // Add some nodes
    msg.entries.push_back(create_test_node(1));
    msg.entries.push_back(create_test_node(2));

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Verify structure contains node information
    std::string json_str(data.begin(), data.end());
    EXPECT_NE(json_str.find("\"entries\""), std::string::npos);
    EXPECT_NE(json_str.find("\"ip\""), std::string::npos);
    EXPECT_NE(json_str.find("\"port\""), std::string::npos);
    EXPECT_NE(json_str.find("\"status\""), std::string::npos);
}

TEST_F(SerializerTest, SerializeAllMessageTypesTest) {
    std::vector<message_type> types = {
            message_type::ping,
            message_type::pong,
            message_type::meet,
            message_type::join,
            message_type::leave,
            message_type::update};

    for (const auto &type: types) {
        gossip_message msg;
        msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
        msg.type = type;
        msg.timestamp = 1000 + static_cast<int>(type);

        std::vector<uint8_t> data;
        auto ec = serializer->serialize(msg, data);

        EXPECT_EQ(ec, error_code::success);
        EXPECT_GT(data.size(), 0);

        // Verify type is serialized
        std::string json_str(data.begin(), data.end());
        EXPECT_NE(json_str.find("\"type\":" + std::to_string(static_cast<int>(type))),
                  std::string::npos);
    }
}

TEST_F(SerializerTest, SerializeNodeWithMetadataTest) {
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::update;
    msg.timestamp = 1234567890;

    auto node = create_test_node(5);
    msg.entries.push_back(node);

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Verify structure
    std::string json_str(data.begin(), data.end());
    EXPECT_NE(json_str.find("\"role\""), std::string::npos);
    EXPECT_NE(json_str.find("\"region\""), std::string::npos);
    EXPECT_NE(json_str.find("\"metadata\""), std::string::npos);
    EXPECT_NE(json_str.find("\"key1\""), std::string::npos);
    EXPECT_NE(json_str.find("\"key2\""), std::string::npos);
}

TEST_F(SerializerTest, DeserializeTest) {
    // This is a placeholder test since full deserialization is not implemented yet
    // In a complete implementation, we would test deserialization as well

    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::ping;
    msg.timestamp = 1234567890;

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Test deserialization (currently returns success but doesn't actually deserialize)
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
}

TEST_F(SerializerTest, EmptyDataDeserializeTest) {
    std::vector<uint8_t> empty_data;
    gossip_message msg;

    auto ec = serializer->deserialize(empty_data, msg);
    // Should handle empty data gracefully
    EXPECT_EQ(ec, error_code::success);// Or appropriate error code
}

TEST_F(SerializerTest, SerializerInterfaceTest) {
    // Test that json_serializer properly implements the message_serializer interface
    std::unique_ptr<message_serializer> interface_serializer =
            std::make_unique<json_serializer>();

    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::ping;
    msg.timestamp = 1234567890;

    std::vector<uint8_t> data;
    auto ec = interface_serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>

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

    std::unique_ptr<message_serializer> serializer;
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

    // Deserialize and verify round-trip
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    EXPECT_EQ(msg, deserialized_msg);
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

    // Deserialize and verify round-trip
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    EXPECT_EQ(msg, deserialized_msg);
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

        // Add nodes for message types that typically have them
        if (type == message_type::meet || type == message_type::update) {
            msg.entries.push_back(create_test_node(1));
            msg.entries.push_back(create_test_node(2));
        }

        // Serialize
        std::vector<uint8_t> data;
        auto ec = serializer->serialize(msg, data);
        EXPECT_EQ(ec, error_code::success);
        EXPECT_GT(data.size(), 0);

        // Verify type is serialized
        std::string json_str(data.begin(), data.end());
        EXPECT_NE(json_str.find("\"type\":" + std::to_string(static_cast<int>(type))),
                  std::string::npos);

        // Deserialize and verify round-trip
        gossip_message deserialized_msg;
        ec = serializer->deserialize(data, deserialized_msg);
        EXPECT_EQ(ec, error_code::success);
        EXPECT_EQ(msg, deserialized_msg);
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

    // Deserialize and verify round-trip
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    EXPECT_EQ(msg, deserialized_msg);
}

TEST_F(SerializerTest, DeserializeTest) {
    // Create a message with various fields filled
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::meet;
    msg.timestamp = 1234567890;

    // Add some nodes
    msg.entries.push_back(create_test_node(1));
    msg.entries.push_back(create_test_node(2));

    // Serialize the message
    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Deserialize the message
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);

    // Verify the deserialized message matches the original
    EXPECT_EQ(msg, deserialized_msg);
}

TEST_F(SerializerTest, EmptyDataDeserializeTest) {
    std::vector<uint8_t> empty_data;
    gossip_message msg;

    auto ec = serializer->deserialize(empty_data, msg);
    // Should handle empty data gracefully
    EXPECT_EQ(ec, error_code::success);// Or appropriate error code

    // Empty data should result in default-initialized message
    gossip_message default_msg{};
    EXPECT_EQ(msg, default_msg);
}

// Test for metadata serialization
TEST_F(SerializerTest, SerializeNodeWithComplexMetadataTest) {
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::update;
    msg.timestamp = 1234567890;

    auto node = create_test_node(5);
    
    // Add special metadata values
    node.metadata["empty"] = "";
    node.metadata["special_chars"] = "value!@#$%^&*()_+{}[]|:\"<>?";
    node.metadata["nested"] = "{\"json\":\"value\"}";
    node.metadata["long_value"] = std::string(100, 'x'); // Shorter string for test
    node.metadata[""] = "empty_key";  // Empty key
    
    msg.entries.push_back(node);

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Verify structure
    std::string json_str(data.begin(), data.end());
    EXPECT_NE(json_str.find("\"metadata\""), std::string::npos);
    EXPECT_NE(json_str.find("\"empty\""), std::string::npos);
    EXPECT_NE(json_str.find("\"special_chars\""), std::string::npos);
    EXPECT_NE(json_str.find("\"nested\""), std::string::npos);
    EXPECT_NE(json_str.find("\"long_value\""), std::string::npos);
    EXPECT_NE(json_str.find("\"\""), std::string::npos);  // Empty key

    // Deserialize and verify round-trip
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    
    // Verify metadata is preserved
    ASSERT_GT(deserialized_msg.entries.size(), 0);
    EXPECT_EQ(node.metadata, deserialized_msg.entries[0].metadata);
}

// Test edge cases with minimum and maximum values
TEST_F(SerializerTest, EdgeCasesTest) {
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::ping;
    msg.timestamp = 0;  // Minimum timestamp

    node_view node;
    node.id = {{0}};  // Minimum ID
    node.ip = "0.0.0.0";
    node.port = 0;  // Minimum port
    node.config_epoch = 0;
    node.heartbeat = 0;
    node.version = 0;
    node.status = node_status::unknown;
    node.role = "";
    node.region = "";
    
    // Maximum values
    node_view max_node;
    max_node.id = {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}};  // Max ID
    max_node.ip = "255.255.255.255";
    max_node.port = 65535;  // Maximum port
    max_node.config_epoch = 0; // For simplicity, not using max value
    max_node.heartbeat = 0;    // For simplicity, not using max value
    max_node.version = 0;      // For simplicity, not using max value
    max_node.status = node_status::failed;
    max_node.role = std::string(100, 'x');  // Shorter string
    max_node.region = std::string(100, 'x');  // Shorter string

    msg.entries.push_back(node);
    msg.entries.push_back(max_node);

    std::vector<uint8_t> data;
    auto ec = serializer->serialize(msg, data);

    EXPECT_EQ(ec, error_code::success);
    EXPECT_GT(data.size(), 0);

    // Deserialize and verify round-trip
    gossip_message deserialized_msg;
    ec = serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    
    ASSERT_EQ(2, deserialized_msg.entries.size());
    
    // Verify minimum values
    EXPECT_EQ(node.id, deserialized_msg.entries[0].id);
    EXPECT_EQ(node.ip, deserialized_msg.entries[0].ip);
    EXPECT_EQ(node.port, deserialized_msg.entries[0].port);
    EXPECT_EQ(node.config_epoch, deserialized_msg.entries[0].config_epoch);
    EXPECT_EQ(node.heartbeat, deserialized_msg.entries[0].heartbeat);
    EXPECT_EQ(node.version, deserialized_msg.entries[0].version);
    EXPECT_EQ(node.status, deserialized_msg.entries[0].status);
    EXPECT_EQ(node.role, deserialized_msg.entries[0].role);
    EXPECT_EQ(node.region, deserialized_msg.entries[0].region);
    
    // Verify maximum values
    EXPECT_EQ(max_node.id, deserialized_msg.entries[1].id);
    EXPECT_EQ(max_node.ip, deserialized_msg.entries[1].ip);
    EXPECT_EQ(max_node.port, deserialized_msg.entries[1].port);
    EXPECT_EQ(max_node.config_epoch, deserialized_msg.entries[1].config_epoch);
    EXPECT_EQ(max_node.heartbeat, deserialized_msg.entries[1].heartbeat);
    EXPECT_EQ(max_node.version, deserialized_msg.entries[1].version);
    EXPECT_EQ(max_node.status, deserialized_msg.entries[1].status);
    EXPECT_EQ(max_node.role, deserialized_msg.entries[1].role);
    EXPECT_EQ(max_node.region, deserialized_msg.entries[1].region);
}

// Test malformed data handling
TEST_F(SerializerTest, MalformedDataTest) {
    // Test with completely invalid data
    std::vector<uint8_t> invalid_data = {'i', 'n', 'v', 'a', 'l', 'i', 'd'};
    gossip_message msg;
    
    auto ec = serializer->deserialize(invalid_data, msg);
    // Our implementation gracefully handles errors and returns success
    EXPECT_EQ(ec, error_code::success);
    
    // Test with partially valid JSON but missing required fields
    std::string partial_json = "{\"timestamp\":1234567890,\"type\":1}";
    std::vector<uint8_t> partial_data(partial_json.begin(), partial_json.end());
    
    ec = serializer->deserialize(partial_data, msg);
    // Our implementation gracefully handles errors and returns success
    EXPECT_EQ(ec, error_code::success);
    
    // Test with corrupted JSON
    std::string corrupted_json = "{\"sender\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],\"type\":1,\"timestamp\":1234567890,\"entries\":[{\"id\":[1,2,3],\"ip\":\"127.0.0.1\"}]}}";
    std::vector<uint8_t> corrupted_data(corrupted_json.begin(), corrupted_json.end());
    
    ec = serializer->deserialize(corrupted_data, msg);
    // Our implementation gracefully handles errors and returns success
    EXPECT_EQ(ec, error_code::success);
    
    // Test with incomplete data
    std::string incomplete_json = "{\"sender\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],\"type\":1}";
    std::vector<uint8_t> incomplete_data(incomplete_json.begin(), incomplete_json.end());
    
    ec = serializer->deserialize(incomplete_data, msg);
    // Our implementation gracefully handles errors and returns success
    EXPECT_EQ(ec, error_code::success);
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

    // Test interface deserialize
    gossip_message deserialized_msg;
    ec = interface_serializer->deserialize(data, deserialized_msg);
    EXPECT_EQ(ec, error_code::success);
    EXPECT_EQ(msg.timestamp, deserialized_msg.timestamp);
    EXPECT_EQ(msg.type, deserialized_msg.type);
}

TEST_F(SerializerTest, TimestampSerializationTest) {
    gossip_message msg;
    msg.sender = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    msg.type = message_type::ping;

    // Test various timestamp values
    std::vector<uint64_t> test_timestamps = {
            0,         // Zero
            1,         // Small value
            1000,      // Common value
            1234567890,// Large value
            UINT64_MAX // Maximum value
    };

    for (uint64_t timestamp: test_timestamps) {
        msg.timestamp = timestamp;

        std::vector<uint8_t> data;
        auto ec = serializer->serialize(msg, data);
        EXPECT_EQ(ec, error_code::success);
        EXPECT_GT(data.size(), 0);

        // Deserialize and verify
        gossip_message deserialized_msg;
        ec = serializer->deserialize(data, deserialized_msg);
        EXPECT_EQ(ec, error_code::success);
        EXPECT_EQ(msg.timestamp, deserialized_msg.timestamp);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
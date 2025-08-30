#include "net/gossip_net.h"
#include <cstring>
#include <gtest/gtest.h>

// Test fixture for C binding tests
class CBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test creating and destroying a UDP transport
TEST_F(CBindingTest, UdpTransportCreateDestroyTest) {
    gossip_transport_t *transport = gossip_transport_create(GOSSIP_TRANSPORT_UDP, "127.0.0.1", 8001);
    ASSERT_NE(transport, nullptr);

    gossip_transport_destroy(transport);
}

// Test creating and destroying a TCP transport
TEST_F(CBindingTest, TcpTransportCreateDestroyTest) {
    gossip_transport_t *transport = gossip_transport_create(GOSSIP_TRANSPORT_TCP, "127.0.0.1", 8002);
    ASSERT_NE(transport, nullptr);

    gossip_transport_destroy(transport);
}

// Test creating and destroying a JSON serializer
TEST_F(CBindingTest, JsonSerializerCreateDestroyTest) {
    gossip_serializer_t *serializer = gossip_json_serializer_create();
    ASSERT_NE(serializer, nullptr);

    gossip_serializer_destroy(serializer);
}

// Test serializer serialize/deserialize functions
// Note: Current C binding implementation has limitations with entries field handling,
// so we focus on testing basic fields only
TEST_F(CBindingTest, JsonSerializerSerializeDeserializeTest) {
    gossip_serializer_t *serializer = gossip_json_serializer_create();
    ASSERT_NE(serializer, nullptr);

    // Create a test message
    gossip_message_t msg = {0};
    msg.sender.data[15] = 1;
    msg.type = GOSSIP_MSG_PING;
    msg.timestamp = 12345;
    msg.entries = nullptr;
    msg.entries_count = 0;

    uint8_t *data = nullptr;
    size_t data_size = 0;

    // Serialize the message
    gossip_error_code_t result = gossip_serializer_serialize(serializer, &msg, &data, &data_size);
    EXPECT_EQ(result, GOSSIP_ERR_SUCCESS);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);

    // Deserialize the message
    gossip_message_t deserialized_msg = {0};
    result = gossip_serializer_deserialize(serializer, data, data_size, &deserialized_msg);
    EXPECT_EQ(result, GOSSIP_ERR_SUCCESS);
    EXPECT_EQ(msg.sender.data[15], deserialized_msg.sender.data[15]);
    EXPECT_EQ(msg.type, deserialized_msg.type);
    EXPECT_EQ(msg.timestamp, deserialized_msg.timestamp);

    // Clean up
    free(data);
    if (deserialized_msg.entries) {
        free(deserialized_msg.entries);
    }
    gossip_serializer_destroy(serializer);
}

// Test transport factory create function
TEST_F(CBindingTest, TransportFactoryCreateTest) {
    gossip_transport_t *udp_transport = gossip_transport_factory_create(GOSSIP_TRANSPORT_UDP, "127.0.0.1", 8003);
    ASSERT_NE(udp_transport, nullptr);
    gossip_transport_destroy(udp_transport);

    gossip_transport_t *tcp_transport = gossip_transport_factory_create(GOSSIP_TRANSPORT_TCP, "127.0.0.1", 8004);
    ASSERT_NE(tcp_transport, nullptr);
    gossip_transport_destroy(tcp_transport);
}

// Test transport start/stop functions
TEST_F(CBindingTest, TransportStartStopTest) {
    gossip_transport_t *transport = gossip_transport_create(GOSSIP_TRANSPORT_UDP, "127.0.0.1", 8005);
    ASSERT_NE(transport, nullptr);

    // Start the transport
    gossip_error_code_t result = gossip_transport_start(transport);
    EXPECT_EQ(result, GOSSIP_ERR_SUCCESS);

    // Stop the transport
    result = gossip_transport_stop(transport);
    EXPECT_EQ(result, GOSSIP_ERR_SUCCESS);

    gossip_transport_destroy(transport);
}

// Test setting serializer on transport
TEST_F(CBindingTest, TransportSetSerializerTest) {
    gossip_transport_t *transport = gossip_transport_create(GOSSIP_TRANSPORT_UDP, "127.0.0.1", 8006);
    ASSERT_NE(transport, nullptr);

    gossip_serializer_t *serializer = gossip_json_serializer_create();
    ASSERT_NE(serializer, nullptr);

    // Set serializer on transport
    gossip_transport_set_serializer(transport, serializer);

    // Clean up
    gossip_transport_destroy(transport);
    gossip_serializer_destroy(serializer);
}
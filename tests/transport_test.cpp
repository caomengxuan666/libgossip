#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <memory>

using namespace gossip::net;

// Test node view for tests
libgossip::node_view create_test_node(uint8_t last_byte) {
    libgossip::node_view node;
    node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, last_byte}};
    node.ip = "127.0.0.1";
    node.port = 8000 + last_byte;
    node.status = libgossip::node_status::online;
    return node;
}

// Mock callbacks
void mock_send_callback(const libgossip::gossip_message &msg,
                        const libgossip::node_view &target) {
    // Do nothing for tests
}

void mock_event_callback(const libgossip::node_view &node,
                         libgossip::node_status old_status) {
    // Do nothing for tests
}

// Test fixture for transport tests
class TransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        self_node = create_test_node(1);
        other_node = create_test_node(2);
        core = std::make_shared<libgossip::gossip_core>(self_node, mock_send_callback, mock_event_callback);
    }

    libgossip::node_view self_node;
    libgossip::node_view other_node;
    std::shared_ptr<libgossip::gossip_core> core;
};

// Tests for UDP transport
TEST_F(TransportTest, UdpTransportCreateTest) {
    auto transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8001);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());
}

TEST_F(TransportTest, UdpTransportStartStopTest) {
    auto transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8002);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    EXPECT_EQ(ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

TEST_F(TransportTest, UdpTransportSyncSendTest) {
    auto transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8003);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    ASSERT_EQ(ec, error_code::success);

    // Create test message
    libgossip::gossip_message msg;
    msg.sender = self_node.id;
    msg.type = libgossip::message_type::ping;
    msg.timestamp = 12345;
    msg.entries.push_back(self_node);

    // Send message synchronously
    ec = transport->send_message(msg, other_node);
    EXPECT_EQ(ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

TEST_F(TransportTest, UdpTransportAsyncSendTest) {
    auto transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8004);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    ASSERT_EQ(ec, error_code::success);

    // Create test message
    libgossip::gossip_message msg;
    msg.sender = self_node.id;
    msg.type = libgossip::message_type::ping;
    msg.timestamp = 12345;
    msg.entries.push_back(self_node);

    // Send message asynchronously
    std::promise<error_code> promise;
    std::future<error_code> future = promise.get_future();

    transport->send_message_async(msg, other_node, [&promise](error_code ec) {
        promise.set_value(ec);
    });

    // Wait for the async operation to complete
    auto result = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(result, std::future_status::ready);

    error_code async_ec = future.get();
    EXPECT_EQ(async_ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

// Tests for TCP transport
TEST_F(TransportTest, TcpTransportCreateTest) {
    auto transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9001);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());
}

TEST_F(TransportTest, TcpTransportStartStopTest) {
    auto transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9002);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    EXPECT_EQ(ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

TEST_F(TransportTest, TcpTransportSyncSendTest) {
    auto transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9003);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    ASSERT_EQ(ec, error_code::success);

    // Create test message
    libgossip::gossip_message msg;
    msg.sender = self_node.id;
    msg.type = libgossip::message_type::ping;
    msg.timestamp = 12345;
    msg.entries.push_back(self_node);

    // Send message synchronously
    ec = transport->send_message(msg, other_node);
    EXPECT_EQ(ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

TEST_F(TransportTest, TcpTransportAsyncSendTest) {
    auto transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9004);
    ASSERT_NE(transport, nullptr);

    transport->set_gossip_core(core);
    transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    auto ec = transport->start();
    ASSERT_EQ(ec, error_code::success);

    // Create test message
    libgossip::gossip_message msg;
    msg.sender = self_node.id;
    msg.type = libgossip::message_type::ping;
    msg.timestamp = 12345;
    msg.entries.push_back(self_node);

    // Send message asynchronously
    std::promise<error_code> promise;
    std::future<error_code> future = promise.get_future();

    transport->send_message_async(msg, other_node, [&promise](error_code ec) {
        promise.set_value(ec);
    });

    // Wait for the async operation to complete
    auto result = future.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(result, std::future_status::ready);

    error_code async_ec = future.get();
    EXPECT_EQ(async_ec, error_code::success);

    // Stop transport
    ec = transport->stop();
    EXPECT_EQ(ec, error_code::success);
}

// Tests for transport factory
TEST_F(TransportTest, TransportFactoryTest) {
    // Test UDP transport creation
    auto udp_transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8001);
    ASSERT_NE(udp_transport, nullptr);

    // Test TCP transport creation
    auto tcp_transport = transport_factory::create_transport(
            transport_type::tcp, "127.0.0.1", 9001);
    ASSERT_NE(tcp_transport, nullptr);

    // Test invalid transport type
    auto invalid_transport = transport_factory::create_transport(
            static_cast<transport_type>(999), "127.0.0.1", 10000);
    EXPECT_EQ(invalid_transport, nullptr);
}

// Tests for error handling
TEST_F(TransportTest, TransportErrorHandlingTest) {
    auto transport = transport_factory::create_transport(
            transport_type::udp, "127.0.0.1", 8005);
    ASSERT_NE(transport, nullptr);

    // Try to send message without setting core and serializer
    libgossip::gossip_message msg;
    msg.sender = self_node.id;
    msg.type = libgossip::message_type::ping;
    msg.timestamp = 12345;

    auto ec = transport->send_message(msg, other_node);
    // Should fail because no serializer is set
    EXPECT_EQ(ec, error_code::serialization_error);

    // Set serializer but not core
    transport->set_serializer(std::make_unique<json_serializer>());

    ec = transport->send_message(msg, other_node);
    // Should succeed now
    EXPECT_EQ(ec, error_code::success);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
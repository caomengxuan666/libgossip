#include "core/node_id_utils.hpp"
#include <gtest/gtest.h>

using namespace libgossip;

TEST(NodeIdUtilsTest, ToStringBasic) {
    node_id_t id{};
    id[0] = 0xab;
    id[1] = 0xcd;
    id[15] = 0xef;

    auto str = node_id_to_string(id);
    EXPECT_EQ(str.size(), 32);
    EXPECT_EQ(str.substr(0, 2), "ab");
    EXPECT_EQ(str.substr(2, 2), "cd");
    EXPECT_EQ(str.substr(30, 2), "ef");
}

TEST(NodeIdUtilsTest, ToStringAllZeros) {
    node_id_t id{};
    auto str = node_id_to_string(id);
    EXPECT_EQ(str, "00000000000000000000000000000000");
}

TEST(NodeIdUtilsTest, ParseValidHex) {
    auto result = parse_node_id("00000000000000000000000000000001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()[15], 1);
}

TEST(NodeIdUtilsTest, ParseWithDashes) {
    auto result = parse_node_id("00000000-0000-0000-0000-000000000001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()[15], 1);
}

TEST(NodeIdUtilsTest, ParseInvalidLength) {
    auto result = parse_node_id("abc");
    EXPECT_FALSE(result.has_value());
}

TEST(NodeIdUtilsTest, ParseInvalidChars) {
    auto result = parse_node_id("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");
    EXPECT_FALSE(result.has_value());
}

TEST(NodeIdUtilsTest, RoundTrip) {
    node_id_t original{};
    original[0] = 0x12;
    original[5] = 0x34;
    original[10] = 0x56;
    original[15] = 0x78;

    auto str = node_id_to_string(original);
    auto parsed = parse_node_id(str);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed.value(), original);
}

TEST(NodeIdUtilsTest, GenerateNodeId) {
    auto id1 = generate_node_id();
    auto id2 = generate_node_id();

    // Two generated IDs should be different (extremely high probability)
    EXPECT_NE(id1, id2);

    // Should not be all zeros
    EXPECT_FALSE(is_null_node_id(id1));
    EXPECT_FALSE(is_null_node_id(id2));
}

TEST(NodeIdUtilsTest, FromBytes) {
    uint8_t bytes[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto id = node_id_from_bytes(bytes);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(id[i], bytes[i]);
    }
}

TEST(NodeIdUtilsTest, ToBytes) {
    node_id_t id{};
    for (size_t i = 0; i < 16; ++i) {
        id[i] = static_cast<uint8_t>(i + 1);
    }

    uint8_t bytes[16];
    node_id_to_bytes(id, bytes);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(bytes[i], i + 1);
    }
}

TEST(NodeIdUtilsTest, FromHash) {
    uint64_t hash = 0x1234567890abcdef;
    auto id = node_id_from_hash(hash);

    uint64_t extracted = 0;
    std::memcpy(&extracted, id.data(), 8);
    EXPECT_EQ(extracted, hash);

    // Remaining bytes should be zero
    for (size_t i = 8; i < 16; ++i) {
        EXPECT_EQ(id[i], 0);
    }
}

TEST(NodeIdUtilsTest, NullNodeId) {
    auto id = null_node_id();
    EXPECT_TRUE(is_null_node_id(id));
}

TEST(NodeIdUtilsTest, IsNullNodeId) {
    node_id_t id{};
    EXPECT_TRUE(is_null_node_id(id));

    id[0] = 1;
    EXPECT_FALSE(is_null_node_id(id));
}

/**
 * @file serializer_factory.hpp
 * @brief Factory for creating and registering message serializers
 *
 * Provides a registry for message_serializer implementations, allowing
 * users to register custom serializers and retrieve them by name.
 *
 * Usage:
 * @code
 *   // Register a custom serializer
 *   serializer_factory::register_serializer("my_format",
 *       []() -> std::unique_ptr<message_serializer> {
 *           return std::make_unique<my_serializer>();
 *       });
 *
 *   // Create a serializer by name
 *   auto serializer = serializer_factory::create("json");
 *
 *   // Use with transport
 *   transport->set_serializer(serializer_factory::create("my_format"));
 * @endcode
 */

#pragma once

#include "core/message_serializer.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace libgossip {

/**
 * @brief Factory for creating message serializer instances
 *
 * This factory allows registration and creation of message serializers
 * by name. It supports both built-in serializers (like JSON) and
 * user-provided custom implementations.
 */
class LIBGOSSIP_API serializer_factory {
public:
    /// Type alias for serializer creator functions
    using creator_func = std::function<std::unique_ptr<message_serializer>()>;

    /**
     * @brief Register a custom serializer
     *
     * @param name Unique name for the serializer (e.g., "protobuf", "msgpack")
     * @param creator Function that creates instances of the serializer
     * @return true if registration succeeded, false if name already exists
     */
    static bool register_serializer(const std::string& name, creator_func creator);

    /**
     * @brief Create a serializer instance by name
     *
     * @param name Name of the registered serializer
     * @return Unique pointer to the serializer, or nullptr if not found
     */
    static std::unique_ptr<message_serializer> create(const std::string& name);

    /**
     * @brief Check if a serializer with the given name is registered
     *
     * @param name Name to check
     * @return true if registered, false otherwise
     */
    static bool has_serializer(const std::string& name);

    /**
     * @brief Get a list of all registered serializer names
     *
     * @return Vector of registered serializer names
     */
    static std::vector<std::string> registered_names();

    /**
     * @brief Initialize built-in serializers
     *
     * Registers the default JSON serializer (if available).
     * Call this once at startup, or it will be called automatically
     * on first use.
     */
    static void init_defaults();

private:
    static std::unordered_map<std::string, creator_func>& get_registry();
    static std::mutex& get_mutex();
    static bool defaults_initialized_;
};

} // namespace libgossip

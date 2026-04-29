/**
 * @file serializer_factory.cpp
 * @brief Implementation of the serializer factory
 */

#include "net/serializer_factory.hpp"
#include <algorithm>
#include <mutex>

// Forward declare the JSON serializer creator
// It's defined in json_serializer.cpp
namespace libgossip {
    std::unique_ptr<message_serializer> create_json_serializer();
}

namespace libgossip {

// Static member initialization
bool serializer_factory::defaults_initialized_ = false;

std::unordered_map<std::string, serializer_factory::creator_func>&
serializer_factory::get_registry() {
    static std::unordered_map<std::string, creator_func> registry;
    return registry;
}

std::mutex& serializer_factory::get_mutex() {
    static std::mutex mutex;
    return mutex;
}

bool serializer_factory::register_serializer(const std::string& name,
                                              creator_func creator) {
    std::lock_guard<std::mutex> lock(get_mutex());

    auto& registry = get_registry();
    if (registry.find(name) != registry.end()) {
        return false; // Already registered
    }

    registry[name] = std::move(creator);
    return true;
}

std::unique_ptr<message_serializer> serializer_factory::create(const std::string& name) {
    init_defaults();

    std::lock_guard<std::mutex> lock(get_mutex());

    auto& registry = get_registry();
    auto it = registry.find(name);
    if (it == registry.end()) {
        return nullptr;
    }

    return it->second();
}

bool serializer_factory::has_serializer(const std::string& name) {
    init_defaults();

    std::lock_guard<std::mutex> lock(get_mutex());

    auto& registry = get_registry();
    return registry.find(name) != registry.end();
}

std::vector<std::string> serializer_factory::registered_names() {
    init_defaults();

    std::lock_guard<std::mutex> lock(get_mutex());

    std::vector<std::string> names;
    auto& registry = get_registry();
    names.reserve(registry.size());

    for (const auto& [name, _] : registry) {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

void serializer_factory::init_defaults() {
    if (defaults_initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(get_mutex());

    if (defaults_initialized_) {
        return;
    }

    // Register the default JSON serializer
    auto& registry = get_registry();
    if (registry.find("json") == registry.end()) {
        registry["json"] = []() -> std::unique_ptr<message_serializer> {
            return libgossip::create_json_serializer();
        };
    }

    defaults_initialized_ = true;
}

} // namespace libgossip

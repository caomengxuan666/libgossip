// When building from setup.py, we need to include the core source directly
#ifndef LIBGOSSIP_BUILD
// Include the core implementation directly when building the Python module
#include "../../src/core/gossip_c.cpp"
#include "../../src/core/gossip_core.cpp"
#include "../../src/core/node_id_utils.cpp"
#include "../../src/net/tcp_transport.cpp"
#include "../../src/net/transport_factory.cpp"
#include "../../src/net/udp_transport.cpp"
#include "../../src/net/json_serializer.cpp"
#include "../../src/net/serializer_factory.cpp"
#endif

#include <iomanip>
#include <memory>
#include <ostream>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// When building with CMake, use installed headers
#ifdef LIBGOSSIP_BUILD
#include "core/gossip_core.hpp"
#include "core/node_id_utils.hpp"
#include "net/json_serializer.hpp"
#include "net/serializer_factory.hpp"
#include "net/tcp_transport.hpp"
#include "net/transport_factory.hpp"
#include "net/udp_transport.hpp"
#else
// When building with setup.py, use relative paths
#include "../../include/core/gossip_core.hpp"
#include "../../include/core/node_id_utils.hpp"
#include "../../include/net/json_serializer.hpp"
#include "../../include/net/serializer_factory.hpp"
#include "../../include/net/tcp_transport.hpp"
#include "../../include/net/transport_factory.hpp"
#include "../../include/net/udp_transport.hpp"
#endif

namespace py = pybind11;
using namespace libgossip;

// Helper function to generate a random node ID
libgossip::node_id_t generate_random_node_id() {
    libgossip::node_id_t id;
    std::random_device rd;
    std::mt19937 gen(rd());
    // Use uint16_t instead of unsigned char to avoid MSVC compilation error
    std::uniform_int_distribution<uint16_t> dis(0, 255);

    for (auto &byte: id) {
        byte = static_cast<unsigned char>(dis(gen));
    }
    return id;
}

PYBIND11_MODULE(libgossip_py, m) {
    m.doc() = "Python bindings for libgossip";

    // Bindings for node_status enum
    py::enum_<libgossip::node_status>(m, "NodeStatus")
            .value("UNKNOWN", libgossip::node_status::unknown)
            .value("JOINING", libgossip::node_status::joining)
            .value("ONLINE", libgossip::node_status::online)
            .value("SUSPECT", libgossip::node_status::suspect)
            .value("FAILED", libgossip::node_status::failed)
            .export_values();

    // Bindings for message_type enum
    py::enum_<libgossip::message_type>(m, "MessageType")
            .value("PING", libgossip::message_type::ping)
            .value("PONG", libgossip::message_type::pong)
            .value("MEET", libgossip::message_type::meet)
            .value("JOIN", libgossip::message_type::join)
            .value("LEAVE", libgossip::message_type::leave)
            .value("UPDATE", libgossip::message_type::update)
            .export_values();

    // Bindings for node_id_t
    py::class_<libgossip::node_id_t>(m, "NodeId")
            .def(py::init<>())
            .def("__repr__", [](const libgossip::node_id_t &id) {
                std::stringstream ss;
                ss << "NodeId(";
                for (size_t i = 0; i < id.size(); ++i) {
                    if (i > 0) {
                        ss << ",";
                    }
                    ss << static_cast<int>(id[i]);
                }
                ss << ")";
                return ss.str();
            })
            .def("__str__", [](const libgossip::node_id_t &id) {
                std::stringstream ss;
                ss << std::hex << std::setfill('0');
                for (size_t i = 0; i < id.size(); ++i) {
                    if (i > 0) {
                        ss << "-";
                    }
                    ss << std::setw(2) << static_cast<int>(id[i]);
                }
                return ss.str();
            })
            .def_static("generate_random", &generate_random_node_id);

    // Bindings for node_view
    py::class_<libgossip::node_view>(m, "NodeView")
            .def(py::init<>())
            .def_readwrite("id", &libgossip::node_view::id)
            .def_readwrite("ip", &libgossip::node_view::ip)
            .def_readwrite("port", &libgossip::node_view::port)
            .def_readwrite("config_epoch", &libgossip::node_view::config_epoch)
            .def_readwrite("heartbeat", &libgossip::node_view::heartbeat)
            .def_readwrite("version", &libgossip::node_view::version)
            .def_readwrite("seen_time", &libgossip::node_view::seen_time)
            .def_readwrite("status", &libgossip::node_view::status)
            .def_readwrite("role", &libgossip::node_view::role)
            .def_readwrite("region", &libgossip::node_view::region)
            .def_readwrite("metadata", &libgossip::node_view::metadata)
            .def_readwrite("suspicion_count", &libgossip::node_view::suspicion_count)
            .def_readwrite("last_suspected", &libgossip::node_view::last_suspected)
            .def("newer_than", &libgossip::node_view::newer_than)
            .def("can_replace", &libgossip::node_view::can_replace);

    // Bindings for gossip_message
    py::class_<libgossip::gossip_message>(m, "GossipMessage")
            .def(py::init<>())
            .def_readwrite("sender", &libgossip::gossip_message::sender)
            .def_readwrite("type", &libgossip::gossip_message::type)
            .def_readwrite("timestamp", &libgossip::gossip_message::timestamp)
            .def_readwrite("entries", &libgossip::gossip_message::entries);

    // Bindings for gossip_stats
    py::class_<libgossip::gossip_stats>(m, "GossipStats")
            .def(py::init<>())
            .def_readwrite("known_nodes", &libgossip::gossip_stats::known_nodes)
            .def_readwrite("sent_messages", &libgossip::gossip_stats::sent_messages)
            .def_readwrite("received_messages", &libgossip::gossip_stats::received_messages)
            .def_readwrite("last_tick_duration", &libgossip::gossip_stats::last_tick_duration);

    // Bindings for gossip_core with shared_ptr for proper memory management
    py::class_<libgossip::gossip_core, std::shared_ptr<libgossip::gossip_core>>(m, "GossipCore")
            .def(py::init<const libgossip::node_view &, libgossip::send_callback, libgossip::event_callback>())
            .def("tick", &libgossip::gossip_core::tick)
            .def("tick_full_broadcast", &libgossip::gossip_core::tick_full_broadcast)
            .def("handle_message", &libgossip::gossip_core::handle_message,
                 "Handle a received gossip message",
                 py::arg("msg"), py::arg("recv_time"))
            .def("meet", &libgossip::gossip_core::meet)
            .def("join", &libgossip::gossip_core::join)
            .def("leave", &libgossip::gossip_core::leave)
            .def("self", &libgossip::gossip_core::self, py::return_value_policy::reference_internal)
            .def("get_nodes", &libgossip::gossip_core::get_nodes)
            .def("find_node", &libgossip::gossip_core::find_node)
            .def("size", &libgossip::gossip_core::size)
            .def("cleanup_expired", &libgossip::gossip_core::cleanup_expired)
            .def("reset", &libgossip::gossip_core::reset)
            .def("get_stats", &libgossip::gossip_core::get_stats)
            .def("update_self_metadata", &libgossip::gossip_core::update_self_metadata,
                 "Update self node metadata (thread-safe)",
                 py::arg("metadata"));

    // Network module bindings

    // Bindings for error_code enum
    py::enum_<libgossip::net::error_code>(m, "ErrorCode")
            .value("SUCCESS", libgossip::net::error_code::success)
            .value("NETWORK_ERROR", libgossip::net::error_code::network_error)
            .value("SERIALIZATION_ERROR", libgossip::net::error_code::serialization_error)
            .value("INVALID_ARGUMENT", libgossip::net::error_code::invalid_argument)
            .value("OPERATION_NOT_PERMITTED", libgossip::net::error_code::operation_not_permitted)
            .export_values();

    // Bindings for transport_type enum
    py::enum_<libgossip::net::transport_type>(m, "TransportType")
            .value("UDP", libgossip::net::transport_type::udp)
            .value("TCP", libgossip::net::transport_type::tcp)
            .export_values();

    // Bindings for message_serializer abstract class
    py::class_<libgossip::message_serializer, py::smart_holder>(m, "MessageSerializer");

    // Bindings for serialization_error enum
    py::enum_<libgossip::serialization_error>(m, "SerializationError")
            .value("SUCCESS", libgossip::serialization_error::success)
            .value("INVALID_INPUT", libgossip::serialization_error::invalid_input)
            .value("SERIALIZATION_FAILED", libgossip::serialization_error::serialization_failed)
            .value("DESERIALIZATION_FAILED", libgossip::serialization_error::deserialization_failed)
            .value("UNSUPPORTED_FORMAT", libgossip::serialization_error::unsupported_format)
            .export_values();

    // Bindings for json_serializer
    py::class_<libgossip::json_serializer, libgossip::message_serializer, py::smart_holder>(m, "JsonSerializer")
            .def(py::init<>())
            .def("name", &libgossip::json_serializer::name)
            .def("serialize", [](const libgossip::json_serializer &self, const libgossip::gossip_message &msg) {
                std::vector<uint8_t> data;
                libgossip::serialization_error ec = self.serialize(msg, data);
                return std::make_pair(ec, data);
            })
            .def("deserialize", [](const libgossip::json_serializer &self, const std::vector<uint8_t> &data) {
                libgossip::gossip_message msg;
                libgossip::serialization_error ec = self.deserialize(data, msg);
                return std::make_pair(ec, msg);
            });

    // Bindings for transport abstract class
    py::class_<libgossip::net::transport, std::shared_ptr<libgossip::net::transport>>(m, "Transport")
            .def("start", &libgossip::net::transport::start)
            .def("stop", &libgossip::net::transport::stop)
            .def("send_message", &libgossip::net::transport::send_message)
            .def("send_message_async", &libgossip::net::transport::send_message_async)
            .def("set_gossip_core", &libgossip::net::transport::set_gossip_core)
            .def("set_serializer", &libgossip::net::transport::set_serializer);

    // Bindings for udp_transport
    py::class_<libgossip::net::udp_transport, std::shared_ptr<libgossip::net::udp_transport>, libgossip::net::transport>(m, "UdpTransport")
            .def(py::init<const std::string &, uint16_t>())
            .def("start", &libgossip::net::udp_transport::start)
            .def("stop", &libgossip::net::udp_transport::stop)
            .def("send_message", &libgossip::net::udp_transport::send_message)
            .def("send_message_async", &libgossip::net::udp_transport::send_message_async)
            .def("set_gossip_core", &libgossip::net::udp_transport::set_gossip_core)
            .def("set_serializer", &libgossip::net::udp_transport::set_serializer);

    // Bindings for tcp_transport
    py::class_<libgossip::net::tcp_transport, std::shared_ptr<libgossip::net::tcp_transport>, libgossip::net::transport>(m, "TcpTransport")
            .def(py::init<const std::string &, uint16_t>())
            .def("start", &libgossip::net::tcp_transport::start)
            .def("stop", &libgossip::net::tcp_transport::stop)
            .def("send_message", &libgossip::net::tcp_transport::send_message)
            .def("send_message_async", &libgossip::net::tcp_transport::send_message_async)
            .def("set_gossip_core", &libgossip::net::tcp_transport::set_gossip_core)
            .def("set_serializer", &libgossip::net::tcp_transport::set_serializer);

    // Bindings for transport_factory
    py::class_<libgossip::net::transport_factory>(m, "TransportFactory")
            .def_static("create_transport", [](libgossip::net::transport_type type, const std::string &host, uint16_t port) { 
                std::unique_ptr<libgossip::net::transport> transport = libgossip::net::transport_factory::create_transport(type, host, port);
                std::shared_ptr<libgossip::net::transport> shared_transport(std::move(transport));
                return shared_transport; }, py::return_value_policy::take_ownership);
}
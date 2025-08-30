// When building from setup.py, we need to include the core source directly
#ifndef LIBGOSSIP_BUILD
// Include the core implementation directly when building the Python module
#include "../../src/core/gossip_c.cpp"
#include "../../src/core/gossip_core.cpp"
#endif

#include <ostream>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
// When building with CMake, use installed headers
#ifdef LIBGOSSIP_BUILD
#include "core/gossip_core.h"
#include "core/gossip_core.hpp"
#include <iomanip>
#include <random>
#else
// When building with setup.py, use relative paths
#include "../../include/core/gossip_core.h"
#include "../../include/core/gossip_core.hpp"
#endif

namespace py = pybind11;
using namespace libgossip;

// Helper function to generate a random node ID
libgossip::node_id_t generate_random_node_id() {
    libgossip::node_id_t id;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    for (auto &byte: id) {
        byte = static_cast<uint8_t>(dis(gen));
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
                    if (i > 0) ss << ",";
                    ss << static_cast<int>(id[i]);
                }
                ss << ")";
                return ss.str();
            })
            .def("__str__", [](const libgossip::node_id_t &id) {
                std::stringstream ss;
                ss << std::hex << std::setfill('0');
                for (size_t i = 0; i < id.size(); ++i) {
                    if (i > 0) ss << "-";
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

    // Bindings for gossip_core
    py::class_<libgossip::gossip_core>(m, "GossipCore")
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
            .def("get_stats", &libgossip::gossip_core::get_stats);
}
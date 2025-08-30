//​
//  _ _ _                         _       
// | (_) |__   __ _  ___  ___ ___(_)_ __  
// | | | '_ \ / _` |/ _ \/ __/ __| | '_ \_
// | | | |_) | (_| | (_) \__ \__ \ | |_) |
// |_|_|_.__/ \__, |\___/|___/___/_| .__/ 
//            |___/                |_|    
// Project: libgossip
// Repository: https://github.com/caomengxuan666/libgossip
// Version: 1.1.2
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Caomengxuan.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//​END

/**
 * @file udp_transport.hpp
 * @brief UDP transport implementation for the gossip protocol
 * 
 * This file contains the UDP transport implementation for the gossip protocol.
 * It provides both synchronous and asynchronous message sending capabilities
 * over UDP network connections.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#ifndef LIBGOSSIP_UDP_TRANSPORT_HPP
#define LIBGOSSIP_UDP_TRANSPORT_HPP

#include "core/gossip_core.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace gossip {
    namespace net {

        /**
     * @brief Error code enumeration for network operations
     * 
     * Defines the possible error codes that can be returned by network operations.
     */
        enum class error_code {
            success = 0,           ///< Operation completed successfully
            network_error,         ///< Network error occurred
            serialization_error,   ///< Serialization/deserialization error
            invalid_argument,      ///< Invalid argument provided
            operation_not_permitted///< Operation not permitted in current state
        };

        /**
     * @brief Abstract interface for message serialization
     * 
     * This class defines the interface for serializing and deserializing
     * gossip messages for network transmission.
     */
        class LIBGOSSIP_API message_serializer {
        public:
            /**
         * @brief Virtual destructor
         */
            virtual ~message_serializer() = default;

            /**
         * @brief Serialize a gossip message to byte array
         * @param msg The message to serialize
         * @param data Output byte array
         * @return Error code indicating success or failure
         */
            virtual error_code serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const = 0;

            /**
         * @brief Deserialize a byte array to gossip message
         * @param data Input byte array
         * @param msg Output message
         * @return Error code indicating success or failure
         */
            virtual error_code deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const = 0;
        };

        /**
     * @brief Abstract transport layer interface
     * 
     * This class defines the interface for network transport implementations.
     * It provides methods for starting/stopping the transport and sending messages.
     */
        class LIBGOSSIP_API transport {
        public:
            /**
         * @brief Virtual destructor
         */
            virtual ~transport() = default;

            /**
         * @brief Start the transport layer
         * @return Error code indicating success or failure
         */
            virtual error_code start() = 0;

            /**
         * @brief Stop the transport layer
         * @return Error code indicating success or failure
         */
            virtual error_code stop() = 0;

            /**
         * @brief Send a message synchronously
         * @param msg The message to send
         * @param target The target node
         * @return Error code indicating success or failure
         */
            virtual error_code send_message(const libgossip::gossip_message &msg,
                                            const libgossip::node_view &target) = 0;

            /**
         * @brief Send a message asynchronously
         * @param msg The message to send
         * @param target The target node
         * @param callback Completion callback function
         */
            virtual void send_message_async(const libgossip::gossip_message &msg,
                                            const libgossip::node_view &target,
                                            std::function<void(error_code)> callback) = 0;

            /**
         * @brief Set the gossip core instance
         * @param core Shared pointer to gossip core
         */
            virtual void set_gossip_core(std::shared_ptr<libgossip::gossip_core> core) = 0;

            /**
         * @brief Set the message serializer
         * @param serializer Unique pointer to serializer
         */
            virtual void set_serializer(std::unique_ptr<message_serializer> serializer) = 0;
        };

        /**
     * @brief UDP transport implementation
     * 
     * This class implements the transport interface using UDP as the underlying
     * network protocol. It provides both synchronous and asynchronous message
     * sending capabilities.
     */
        class LIBGOSSIP_API udp_transport : public transport {
        public:
            /**
         * @brief Constructor
         * @param host Host address
         * @param port Port number
         */
            udp_transport(const std::string &host, uint16_t port);

            /**
         * @brief Destructor
         */
            ~udp_transport() override;

            /**
         * @brief Start the UDP transport
         * @return Error code indicating success or failure
         */
            error_code start() override;

            /**
         * @brief Stop the UDP transport
         * @return Error code indicating success or failure
         */
            error_code stop() override;

            /**
         * @brief Send a message synchronously over UDP
         * @param msg The message to send
         * @param target The target node
         * @return Error code indicating success or failure
         */
            error_code send_message(const libgossip::gossip_message &msg,
                                    const libgossip::node_view &target) override;

            /**
         * @brief Send a message asynchronously over UDP
         * @param msg The message to send
         * @param target The target node
         * @param callback Completion callback function
         */
            void send_message_async(const libgossip::gossip_message &msg,
                                    const libgossip::node_view &target,
                                    std::function<void(error_code)> callback) override;

            /**
         * @brief Set the gossip core instance
         * @param core Shared pointer to gossip core
         */
            void set_gossip_core(std::shared_ptr<libgossip::gossip_core> core) override;

            /**
         * @brief Set the message serializer
         * @param serializer Unique pointer to serializer
         */
            void set_serializer(std::unique_ptr<message_serializer> serializer) override;

        private:
            class impl;
            std::unique_ptr<impl> pimpl_;
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_UDP_TRANSPORT_HPP
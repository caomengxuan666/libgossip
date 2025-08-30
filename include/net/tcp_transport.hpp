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
 * @file tcp_transport.hpp
 * @brief TCP transport implementation for the gossip protocol
 * 
 * This file contains the TCP transport implementation for the gossip protocol.
 * It provides both synchronous and asynchronous message sending capabilities
 * over TCP network connections.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#ifndef LIBGOSSIP_TCP_TRANSPORT_HPP
#define LIBGOSSIP_TCP_TRANSPORT_HPP

#include "udp_transport.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace gossip {
    namespace net {

        /**
     * @brief TCP transport implementation
     * 
     * This class implements the transport interface using TCP as the underlying
     * network protocol. It provides both synchronous and asynchronous message
     * sending capabilities with reliable delivery.
     */
        class LIBGOSSIP_API tcp_transport : public transport {
        public:
            /**
         * @brief Constructor
         * @param host Host address
         * @param port Port number
         */
            tcp_transport(const std::string &host, uint16_t port);

            /**
         * @brief Destructor
         */
            ~tcp_transport() override;

            /**
         * @brief Start the TCP transport
         * @return Error code indicating success or failure
         */
            error_code start() override;

            /**
         * @brief Stop the TCP transport
         * @return Error code indicating success or failure
         */
            error_code stop() override;

            /**
         * @brief Send a message synchronously over TCP
         * @param msg The message to send
         * @param target The target node
         * @return Error code indicating success or failure
         */
            error_code send_message(const libgossip::gossip_message &msg,
                                    const libgossip::node_view &target) override;

            /**
         * @brief Send a message asynchronously over TCP
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

#endif// LIBGOSSIP_TCP_TRANSPORT_HPP
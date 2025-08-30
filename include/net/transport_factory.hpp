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
 * @file transport_factory.hpp
 * @brief Factory for creating transport instances
 * 
 * This file contains the transport factory implementation which provides
 * a way to create different types of transport instances (UDP, TCP, etc.)
 * based on the specified transport type.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#ifndef LIBGOSSIP_TRANSPORT_FACTORY_HPP
#define LIBGOSSIP_TRANSPORT_FACTORY_HPP

#include "udp_transport.hpp"
#include <memory>
#include <string>

namespace gossip {
    namespace net {

        /**
     * @brief Transport type enumeration
     * 
     * Defines the supported transport types that can be created by the factory.
     */
        enum class transport_type {
            udp = 0,///< UDP transport
            tcp     ///< TCP transport
        };

        /**
     * @brief Transport factory class
     * 
     * This class provides a factory method for creating transport instances
     * based on the specified transport type. It encapsulates the creation
     * logic and provides a clean interface for creating transports.
     */
        class LIBGOSSIP_API transport_factory {
        public:
            /**
         * @brief Create a transport instance
         * 
         * Factory method to create transport instances based on the specified
         * transport type, host, and port.
         * 
         * @param type The type of transport to create
         * @param host The host address for the transport
         * @param port The port number for the transport
         * @return Unique pointer to the created transport, or nullptr if creation failed
         */
            static std::unique_ptr<transport> create_transport(transport_type type,
                                                               const std::string &host,
                                                               uint16_t port);
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_TRANSPORT_FACTORY_HPP
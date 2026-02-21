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
// Copyright (c) 2025 Caomengxuan666.
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
// FITNESS FOR A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//​END

/**
 * @file json_serializer.hpp
 * @brief JSON serializer implementation for gossip messages using nlohmann/json
 * 
 * This file contains the JSON serializer implementation for gossip messages.
 * It provides serialization and deserialization of gossip messages to/from
 * JSON format for network transmission using the nlohmann/json library.
 * 
 * @author caomengxuan666
 * @date 2025-08-30
 */

#ifndef LIBGOSSIP_JSON_SERIALIZER_HPP
#define LIBGOSSIP_JSON_SERIALIZER_HPP

#include "udp_transport.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>

namespace gossip {
    namespace net {

        /**
         * @brief JSON serializer implementation using nlohmann/json
         * 
         * This class implements the message_serializer interface using JSON as
         * the serialization format. It provides methods to serialize gossip
         * messages to JSON byte arrays and deserialize JSON byte arrays back
         * to gossip messages.
         */
        class LIBGOSSIP_API json_serializer : public message_serializer {
        public:
            /**
             * @brief Serialize a gossip message to JSON byte array
             * @param msg The message to serialize
             * @param data Output byte array containing JSON representation
             * @return Error code indicating success or failure
             */
            error_code serialize(const libgossip::gossip_message &msg, std::vector<uint8_t> &data) const override;

            /**
             * @brief Deserialize a JSON byte array to gossip message
             * @param data Input byte array containing JSON representation
             * @param msg Output message
             * @return Error code indicating success or failure
             */
            error_code deserialize(const std::vector<uint8_t> &data, libgossip::gossip_message &msg) const override;

        private:
            /**
             * @brief Serialize a node view to JSON object
             * @param node The node to serialize
             * @return JSON object
             */
            nlohmann::json serialize_node_to_json(const libgossip::node_view &node) const;

            /**
             * @brief Deserialize a node view from JSON object
             * @param j Input JSON object
             * @return Node view
             */
            libgossip::node_view deserialize_node_from_json(const nlohmann::json &j) const;

            /**
             * @brief Parse a node ID from hex string
             * @param hex_str Input hex string
             * @param node_id Output node ID
             */
            void parse_node_id_from_hex(const std::string &hex_str, libgossip::node_id_t &node_id) const;
        };

    }// namespace net
}// namespace gossip

#endif// LIBGOSSIP_JSON_SERIALIZER_HPP
// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include "opentxs/Proto.hpp"

namespace opentxs
{
namespace network
{
class DhtConfig;
class OpenDHT;
namespace zeromq
{
class Context;
class Frame;
class Message;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

namespace opentxs::factory
{
auto OpenDHT(const network::DhtConfig& config) noexcept
    -> std::unique_ptr<network::OpenDHT>;
auto ZMQContext() noexcept -> network::zeromq::Context*;
OPENTXS_EXPORT auto ZMQFrame() noexcept -> network::zeromq::Frame*;
OPENTXS_EXPORT auto ZMQFrame(std::size_t size) noexcept
    -> network::zeromq::Frame*;
OPENTXS_EXPORT auto ZMQFrame(const void* data, const std::size_t size) noexcept
    -> network::zeromq::Frame*;
OPENTXS_EXPORT auto ZMQFrame(const ProtobufType& data) noexcept
    -> network::zeromq::Frame*;
OPENTXS_EXPORT auto ZMQMessage() noexcept -> network::zeromq::Message*;
OPENTXS_EXPORT auto ZMQMessage(
    const void* data,
    const std::size_t size) noexcept -> network::zeromq::Message*;
OPENTXS_EXPORT auto ZMQMessage(const ProtobufType& data) noexcept
    -> network::zeromq::Message*;
}  // namespace opentxs::factory

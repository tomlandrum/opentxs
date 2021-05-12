// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/network/zeromq/Context.cpp"

#pragma once

#include <functional>
#include <iosfwd>
#include <string>

#include "internal/network/Factory.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/network/zeromq/Proxy.hpp"
#include "opentxs/network/zeromq/socket/Dealer.hpp"
#include "opentxs/network/zeromq/socket/Pair.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Pull.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Reply.hpp"
#include "opentxs/network/zeromq/socket/Request.hpp"
#include "opentxs/network/zeromq/socket/Router.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/network/zeromq/socket/Subscribe.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace network
{
namespace zeromq
{
class ListenCallback;
class PairEventCallback;
class ReplyCallback;
}  // namespace zeromq
}  // namespace network

class Factory;
}  // namespace opentxs

namespace opentxs::network::zeromq::implementation
{
class Context final : virtual public zeromq::Context
{
public:
    operator void*() const noexcept final;

    auto BuildEndpoint(
        const std::string& path,
        const int instance,
        const int version) const noexcept -> std::string final;
    auto BuildEndpoint(
        const std::string& path,
        const int instance,
        const int version,
        const std::string& suffix) const noexcept -> std::string final;
    auto DealerSocket(
        const ListenCallback& callback,
        const socket::Socket::Direction direction) const noexcept
        -> OTZMQDealerSocket final;
    auto Frame(const void* input, const std::size_t size) const noexcept
        -> OTZMQFrame final;
    auto Message() const noexcept -> OTZMQMessage final;
    auto Message(const ProtobufType& input) const noexcept
        -> OTZMQMessage final;
    auto Message(const network::zeromq::Message& input) const noexcept
        -> OTZMQMessage final
    {
        return OTZMQMessage(input);
    }
    auto Message(const void* input, const std::size_t size) const noexcept
        -> OTZMQMessage final;
    auto PairEventListener(
        const PairEventCallback& callback,
        const int instance) const noexcept -> OTZMQSubscribeSocket final;
    auto PairSocket(const opentxs::network::zeromq::ListenCallback& callback)
        const noexcept -> OTZMQPairSocket final;
    auto PairSocket(
        const opentxs::network::zeromq::ListenCallback& callback,
        const opentxs::network::zeromq::socket::Pair& peer) const noexcept
        -> OTZMQPairSocket final;
    auto PairSocket(
        const opentxs::network::zeromq::ListenCallback& callback,
        const std::string& endpoint) const noexcept -> OTZMQPairSocket final;
    auto Pipeline(
        const api::internal::Core& api,
        std::function<void(zeromq::Message&)> callback) const noexcept
        -> OTZMQPipeline final;
    auto Proxy(
        network::zeromq::socket::Socket& frontend,
        network::zeromq::socket::Socket& backend) const noexcept
        -> OTZMQProxy final;
    auto PublishSocket() const noexcept -> OTZMQPublishSocket final;
    auto PullSocket(const socket::Socket::Direction direction) const noexcept
        -> OTZMQPullSocket final;
    auto PullSocket(
        const ListenCallback& callback,
        const socket::Socket::Direction direction) const noexcept
        -> OTZMQPullSocket final;
    auto PushSocket(const socket::Socket::Direction direction) const noexcept
        -> OTZMQPushSocket final;
    auto ReplyMessage(const zeromq::Message& request) const noexcept
        -> OTZMQMessage final;
    auto ReplyMessage(const ReadView connectionID) const noexcept
        -> OTZMQMessage final;
    auto ReplySocket(
        const ReplyCallback& callback,
        const socket::Socket::Direction direction) const noexcept
        -> OTZMQReplySocket final;
    auto RequestSocket() const noexcept -> OTZMQRequestSocket final;
    auto RouterSocket(
        const ListenCallback& callback,
        const socket::Socket::Direction direction) const noexcept
        -> OTZMQRouterSocket final;
    auto SubscribeSocket(const ListenCallback& callback) const noexcept
        -> OTZMQSubscribeSocket final;
    auto TaggedMessage(const void* tag, const std::size_t size) const noexcept
        -> OTZMQMessage final;
    auto TaggedReply(
        const zeromq::Message& request,
        const void* tag,
        const std::size_t size) const noexcept -> OTZMQMessage final;
    auto TaggedReply(
        const ReadView connectionID,
        const void* tag,
        const std::size_t size) const noexcept -> OTZMQMessage final;

    ~Context() final;

private:
    friend network::zeromq::Context* opentxs::factory::ZMQContext() noexcept;

    void* context_{nullptr};

    auto clone() const noexcept -> Context* final { return new Context; }

    Context() noexcept;
    Context(const Context&) = delete;
    Context(Context&&) = delete;
    auto operator=(const Context&) -> Context& = delete;
    auto operator=(Context&&) -> Context& = delete;
};
}  // namespace opentxs::network::zeromq::implementation

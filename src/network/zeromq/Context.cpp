// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "network/zeromq/Context.hpp"  // IWYU pragma: associated

#include <zmq.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include "PairEventListener.hpp"
#include "internal/network/Factory.hpp"
#include "internal/network/zeromq/socket/Socket.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
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
#include "opentxs/network/zeromq/socket/Subscribe.hpp"

template class opentxs::Pimpl<opentxs::network::zeromq::Context>;

#define INPROC_PREFIX "inproc://opentxs/"
#define PATH_SEPERATOR "/"
#define OT_METHOD "opentxs::network::zeromq::Context::"

namespace opentxs::factory
{
auto ZMQContext() noexcept -> network::zeromq::Context*
{
    using ReturnType = network::zeromq::implementation::Context;

    return new ReturnType;
}
}  // namespace opentxs::factory

namespace opentxs::network::zeromq
{
auto Context::RawToZ85(
    const ReadView input,
    const AllocateOutput destination) noexcept -> bool
{
    if (0 != input.size() % 4) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid input size.").Flush();

        return false;
    }

    if (false == bool(destination)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator.")
            .Flush();

        return false;
    }

    const auto target = std::size_t{input.size() + input.size() / 4 + 1};
    auto out = destination(target);

    if (false == out.valid(target)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to allocate output")
            .Flush();

        return false;
    }

    return nullptr != ::zmq_z85_encode(
                          out.as<char>(),
                          reinterpret_cast<const std::uint8_t*>(input.data()),
                          input.size());
}

auto Context::Z85ToRaw(
    const ReadView input,
    const AllocateOutput destination) noexcept -> bool
{
    if (0 != input.size() % 5) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid input size.").Flush();

        return false;
    }

    if (false == bool(destination)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator.")
            .Flush();

        return false;
    }

    const auto target = std::size_t{input.size() * 4 / 5};
    auto out = destination(target);

    if (false == out.valid(target)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to allocate output")
            .Flush();

        return false;
    }

    return ::zmq_z85_decode(out.as<std::uint8_t>(), input.data());
}
}  // namespace opentxs::network::zeromq

namespace opentxs::network::zeromq::implementation
{
Context::Context() noexcept
    : context_(::zmq_ctx_new())
{
    assert(nullptr != context_);
    assert(1 == ::zmq_has("curve"));

    constexpr auto sockets
    {
#if defined _WIN32
        10240
#elif defined __APPLE__
        1024
#else
        16384
#endif
    };
    [[maybe_unused]] const auto init =
        ::zmq_ctx_set(context_, ZMQ_MAX_SOCKETS, sockets);

    assert(0 == init);
}

Context::operator void*() const noexcept
{
    OT_ASSERT(nullptr != context_)

    return context_;
}

auto Context::BuildEndpoint(
    const std::string& path,
    const int instance,
    const int version) const noexcept -> std::string
{
    return std::string(INPROC_PREFIX) + std::to_string(instance) +
           PATH_SEPERATOR + path + PATH_SEPERATOR + std::to_string(version);
}

auto Context::BuildEndpoint(
    const std::string& path,
    const int instance,
    const int version,
    const std::string& suffix) const noexcept -> std::string
{
    return BuildEndpoint(path, instance, version) + PATH_SEPERATOR + suffix;
}

auto Context::DealerSocket(
    const ListenCallback& callback,
    const socket::Socket::Direction direction) const noexcept
    -> OTZMQDealerSocket
{
    return OTZMQDealerSocket{
        factory::DealerSocket(*this, static_cast<bool>(direction), callback)};
}

auto Context::Frame(const void* input, const std::size_t size) const noexcept
    -> OTZMQFrame
{
    return OTZMQFrame{factory::ZMQFrame(input, size)};
}

auto Context::Message() const noexcept -> OTZMQMessage
{
    return OTZMQMessage{factory::ZMQMessage()};
}

auto Context::Message(const ProtobufType& input) const noexcept -> OTZMQMessage
{
    return OTZMQMessage{factory::ZMQMessage(input)};
}

auto Context::Message(const void* input, const std::size_t size) const noexcept
    -> OTZMQMessage
{
    return OTZMQMessage{factory::ZMQMessage(input, size)};
}

auto Context::PairEventListener(
    const PairEventCallback& callback,
    const int instance) const noexcept -> OTZMQSubscribeSocket
{
    return OTZMQSubscribeSocket(
        new class PairEventListener(*this, callback, instance));
}

auto Context::PairSocket(
    const opentxs::network::zeromq::ListenCallback& callback) const noexcept
    -> OTZMQPairSocket
{
    return OTZMQPairSocket{factory::PairSocket(*this, callback, true)};
}

auto Context::PairSocket(
    const opentxs::network::zeromq::ListenCallback& callback,
    const opentxs::network::zeromq::socket::Pair& peer) const noexcept
    -> OTZMQPairSocket
{
    return OTZMQPairSocket{factory::PairSocket(callback, peer, true)};
}

auto Context::PairSocket(
    const opentxs::network::zeromq::ListenCallback& callback,
    const std::string& endpoint) const noexcept -> OTZMQPairSocket
{
    return OTZMQPairSocket{factory::PairSocket(*this, callback, endpoint)};
}

auto Context::Pipeline(
    const api::internal::Core& api,
    std::function<void(zeromq::Message&)> callback) const noexcept
    -> OTZMQPipeline
{
    return OTZMQPipeline{opentxs::factory::Pipeline(api, *this, callback)};
}

auto Context::Proxy(
    network::zeromq::socket::Socket& frontend,
    network::zeromq::socket::Socket& backend) const noexcept -> OTZMQProxy
{
    return opentxs::network::zeromq::Proxy::Factory(*this, frontend, backend);
}

auto Context::PublishSocket() const noexcept -> OTZMQPublishSocket
{
    return OTZMQPublishSocket{factory::PublishSocket(*this)};
}

auto Context::PullSocket(
    const socket::Socket::Direction direction) const noexcept -> OTZMQPullSocket
{
    return OTZMQPullSocket{
        factory::PullSocket(*this, static_cast<bool>(direction))};
}

auto Context::PullSocket(
    const ListenCallback& callback,
    const socket::Socket::Direction direction) const noexcept -> OTZMQPullSocket
{
    return OTZMQPullSocket{
        factory::PullSocket(*this, static_cast<bool>(direction), callback)};
}

auto Context::PushSocket(
    const socket::Socket::Direction direction) const noexcept -> OTZMQPushSocket
{
    return OTZMQPushSocket{
        factory::PushSocket(*this, static_cast<bool>(direction))};
}

auto Context::ReplyMessage(const zeromq::Message& request) const noexcept
    -> OTZMQMessage
{
    auto output = Message();

    if (0 < request.Header().size()) {
        for (const auto& frame : request.Header()) { output->AddFrame(frame); }

        output->AddFrame();
    }

    OT_ASSERT(0 == output->Body().size());

    return output;
}

auto Context::ReplyMessage(const ReadView id) const noexcept -> OTZMQMessage
{
    auto output = Message(id.data(), id.size());
    output->StartBody();

    return output;
}

auto Context::ReplySocket(
    const ReplyCallback& callback,
    const socket::Socket::Direction direction) const noexcept
    -> OTZMQReplySocket
{
    return OTZMQReplySocket{
        factory::ReplySocket(*this, static_cast<bool>(direction), callback)};
}

auto Context::RequestSocket() const noexcept -> OTZMQRequestSocket
{
    return OTZMQRequestSocket{factory::RequestSocket(*this)};
}

auto Context::RouterSocket(
    const ListenCallback& callback,
    const socket::Socket::Direction direction) const noexcept
    -> OTZMQRouterSocket
{
    return OTZMQRouterSocket{
        factory::RouterSocket(*this, static_cast<bool>(direction), callback)};
}

auto Context::SubscribeSocket(const ListenCallback& callback) const noexcept
    -> OTZMQSubscribeSocket
{
    return OTZMQSubscribeSocket{factory::SubscribeSocket(*this, callback)};
}

auto Context::TaggedMessage(const void* tag, const std::size_t size)
    const noexcept -> OTZMQMessage
{
    auto output = Message();
    output->StartBody();
    output->AddFrame(tag, size);

    return output;
}

auto Context::TaggedReply(
    const zeromq::Message& request,
    const void* tag,
    const std::size_t size) const noexcept -> OTZMQMessage
{
    auto output = ReplyMessage(request);
    output->StartBody();
    output->AddFrame(tag, size);

    return output;
}

auto Context::TaggedReply(
    const ReadView connectionID,
    const void* tag,
    const std::size_t size) const noexcept -> OTZMQMessage
{
    auto output = ReplyMessage(connectionID);
    output->StartBody();
    output->AddFrame(tag, size);

    return output;
}

Context::~Context()
{
    if (nullptr != context_) {
        zmq_ctx_shutdown(context_);
        auto promise = std::promise<void>{};
        auto future = promise.get_future();
        auto thread = std::thread{[&] {
            zmq_ctx_term(context_);
            promise.set_value();
        }};
        using Status = std::future_status;
        constexpr auto limit = std::chrono::seconds{10};

        if (Status::ready != future.wait_for(limit)) {
            std::cout << "WARNING: zmq_ctx_term timed out.\n";
            thread.detach();
        }

        if (thread.joinable()) { thread.join(); }

        context_ = nullptr;
    }
}
}  // namespace opentxs::network::zeromq::implementation

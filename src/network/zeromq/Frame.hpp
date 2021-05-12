// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/network/zeromq/Frame.cpp"

#pragma once

#include <zmq.h>
#include <iosfwd>
#include <string>

#include "internal/network/Factory.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/network/zeromq/Frame.hpp"

namespace opentxs::network::zeromq::implementation
{
class Frame final : virtual public zeromq::Frame
{
public:
    operator std::string() const noexcept final;

    auto Bytes() const noexcept -> ReadView final;
    auto data() const noexcept -> const void* final
    {
        return zmq_msg_data(&message_);
    }
    auto size() const noexcept -> std::size_t final
    {
        return zmq_msg_size(&message_);
    }

    operator zmq_msg_t*() noexcept final { return &message_; }

    ~Frame() final;

private:
    friend network::zeromq::Frame* opentxs::factory::ZMQFrame() noexcept;
    friend network::zeromq::Frame* opentxs::factory::ZMQFrame(
        std::size_t) noexcept;
    friend network::zeromq::Frame* opentxs::factory::ZMQFrame(
        const void*,
        const std::size_t) noexcept;
    friend network::zeromq::Frame* opentxs::factory::ZMQFrame(
        const ProtobufType&) noexcept;
    friend network::zeromq::Frame;

    mutable zmq_msg_t message_;

    auto clone() const noexcept -> Frame* final;

    Frame() noexcept;
    explicit Frame(const ProtobufType& input) noexcept;
    explicit Frame(const std::size_t bytes) noexcept;
    Frame(const void* data, const std::size_t bytes) noexcept;
    Frame(const Frame&) = delete;
    Frame(Frame&&) = delete;
    auto operator=(Frame&&) -> Frame& = delete;
    auto operator=(const Frame&) -> Frame& = delete;
};
}  // namespace opentxs::network::zeromq::implementation

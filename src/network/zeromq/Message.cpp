// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "network/zeromq/Message.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <memory>
#include <utility>

#include "internal/network/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"

template class opentxs::Pimpl<opentxs::network::zeromq::Message>;

namespace opentxs::factory
{
auto ZMQMessage() noexcept -> network::zeromq::Message*
{
    using ReturnType = opentxs::network::zeromq::implementation::Message;

    return new ReturnType();
}

auto ZMQMessage(const void* data, const std::size_t size) noexcept
    -> network::zeromq::Message*
{
    using ReturnType = opentxs::network::zeromq::implementation::Message;
    auto output = new ReturnType();

    if (nullptr != output) { output->AddFrame(data, size); }

    return output;
}

auto ZMQMessage(const ProtobufType& data) noexcept -> network::zeromq::Message*
{
    using ReturnType = opentxs::network::zeromq::implementation::Message;
    auto output = new ReturnType();

    if (nullptr != output) { output->AddFrame(data); }

    return output;
}
}  // namespace opentxs::factory

namespace opentxs::network::zeromq
{
auto Message::Factory() -> OTZMQMessage
{
    return OTZMQMessage{new implementation::Message()};
}
}  // namespace opentxs::network::zeromq

namespace opentxs::network::zeromq::implementation
{
Message::Message()
    : messages_()
{
}

Message::Message(const Message& rhs)
    : zeromq::Message()
    , messages_()
{
    for (auto& message : rhs.messages_) { messages_.emplace_back(message); }
}

auto Message::AddFrame() -> Frame&
{
    messages_.emplace_back(factory::ZMQFrame());

    return messages_.back().get();
}

auto Message::AddFrame(const void* input, const std::size_t size) -> Frame&
{
    messages_.emplace_back(factory::ZMQFrame(input, size));

    return messages_.back().get();
}

auto Message::AddFrame(const ProtobufType& input) -> Frame&
{
    messages_.emplace_back(factory::ZMQFrame(input));

    return messages_.back().get();
}

auto Message::AppendBytes() noexcept -> AllocateOutput
{
    return [this](const std::size_t size) -> WritableView {
        auto& frame = messages_.emplace_back(factory::ZMQFrame(size));

        return {const_cast<void*>(frame->data()), frame->size()};
    };
}

auto Message::at(const std::size_t index) const -> const Frame&
{
    OT_ASSERT(messages_.size() > index);

    return const_cast<const Frame&>(messages_.at(index).get());
}

auto Message::at(const std::size_t index) -> Frame&
{
    OT_ASSERT(messages_.size() > index);

    return messages_.at(index).get();
}

auto Message::begin() const -> FrameIterator { return FrameIterator(this); }

auto Message::Body() const -> const FrameSection
{
    auto position = body_position();

    return FrameSection(this, position, messages_.size() - position);
}

auto Message::Body() -> FrameSection
{
    auto position = body_position();

    return FrameSection(this, position, messages_.size() - position);
}

auto Message::Body_at(const std::size_t index) const -> const Frame&
{
    return Body().at(index);
}

auto Message::Body_begin() const -> FrameIterator { return Body().begin(); }

auto Message::Body_end() const -> FrameIterator { return Body().end(); }

auto Message::body_position() const -> std::size_t
{
    std::size_t position{0};

    if (true == hasDivider()) { position = findDivider() + 1; }

    return position;
}

auto Message::end() const -> FrameIterator
{
    return FrameIterator(this, messages_.size());
}

// This function is only called by RouterSocket.  It makes sure that if a
// message has two or more frames, and no delimiter, then a delimiter is
// inserted after the first frame.
auto Message::EnsureDelimiter() -> void
{
    if (1 < messages_.size() && !hasDivider()) {
        auto it = messages_.begin();
        messages_.emplace(++it, factory::ZMQFrame());
    }
    // These cases should never happen.  When this function is called, there
    // should always be at least two frames.
    else if (0 < messages_.size() && !hasDivider()) {
        messages_.emplace(messages_.begin(), factory::ZMQFrame());
    } else if (!hasDivider()) {
        messages_.emplace_back(factory::ZMQFrame());
    }
}

auto Message::findDivider() const -> std::size_t
{
    std::size_t divider = 0;

    for (auto& message : messages_) {
        if (0 == message->size()) { break; }
        ++divider;
    }

    return divider;
}

auto Message::hasDivider() const -> bool
{
    return std::find_if(
               messages_.begin(),
               messages_.end(),
               [](const OTZMQFrame& msg) -> bool {
                   return 0 == msg->size();
               }) != messages_.end();
}

auto Message::Header_at(const std::size_t index) const -> const Frame&
{
    return Header().at(index);
}

auto Message::Header() const -> const FrameSection
{
    auto size = std::size_t{0};

    if (true == hasDivider()) { size = findDivider(); }

    return FrameSection(this, 0, size);
}

auto Message::Header() -> FrameSection
{
    auto size = std::size_t{0};

    if (true == hasDivider()) { size = findDivider(); }

    return FrameSection(this, 0, size);
}

auto Message::Header_begin() const -> FrameIterator { return Header().begin(); }

auto Message::Header_end() const -> FrameIterator { return Header().end(); }

auto Message::PrependEmptyFrame() -> void
{
    OTZMQFrame message{factory::ZMQFrame()};

    auto it = messages_.emplace(messages_.begin(), message);

    OT_ASSERT(messages_.end() != it);
}

auto Message::Replace(const std::size_t index, OTZMQFrame&& frame) -> Frame&
{
    auto& position = messages_.at(index);
    std::swap(position, frame);

    return position;
}

auto Message::set_field(const std::size_t position, const zeromq::Frame& input)
    -> bool
{
    const auto effectivePosition = body_position() + position;

    if (effectivePosition >= messages_.size()) { return false; }

    messages_[effectivePosition] = input;

    return true;
}

auto Message::StartBody() noexcept -> void
{
    if (0 == messages_.size()) {
        messages_.emplace_back(factory::ZMQFrame());
    } else if (0 < (*messages_.crbegin())->size()) {
        messages_.emplace_back(factory::ZMQFrame());
    }
}

auto Message::size() const -> std::size_t { return messages_.size(); }
}  // namespace opentxs::network::zeromq::implementation

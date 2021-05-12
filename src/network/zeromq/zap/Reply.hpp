// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/network/zeromq/zap/Reply.cpp"

#pragma once

#include <map>
#include <string>

#include "internal/network/Factory.hpp"
#include "network/zeromq/Message.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/zap/Reply.hpp"
#include "opentxs/network/zeromq/zap/ZAP.hpp"

namespace opentxs
{
namespace network
{
namespace zeromq
{
namespace zap
{
class Request;
}  // namespace zap
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

#define VERSION_POSITION 0
#define REQUEST_ID_POSITION 1
#define STATUS_CODE_POSITION 2
#define STATUS_TEXT_POSITION 3
#define USER_ID_POSITION 4
#define METADATA_POSITION 5

namespace opentxs::network::zeromq::zap::implementation
{
class Reply final : virtual zap::Reply, zeromq::implementation::Message
{
public:
    auto Code() const -> zap::Status override
    {
        return string_to_code(Body_at(STATUS_CODE_POSITION));
    }
    auto Debug() const -> std::string override;
    auto Metadata() const -> OTData override;
    auto RequestID() const -> OTData override;
    auto Status() const -> std::string override
    {
        return Body_at(STATUS_TEXT_POSITION);
    }
    auto UserID() const -> std::string override
    {
        return Body_at(USER_ID_POSITION);
    }
    auto Version() const -> std::string override
    {
        return Body_at(VERSION_POSITION);
    }

    auto SetCode(const zap::Status& code) -> bool override
    {
        const auto value = code_to_string(code);

        return set_field(
            STATUS_CODE_POSITION,
            OTZMQFrame{factory::ZMQFrame(value.data(), value.size())});
    }
    auto SetMetadata(const Data& metadata) -> bool override
    {
        return set_field(
            METADATA_POSITION,
            OTZMQFrame{factory::ZMQFrame(metadata.data(), metadata.size())});
    }
    auto SetStatus(const std::string& status) -> bool override
    {
        return set_field(
            STATUS_TEXT_POSITION,
            OTZMQFrame{factory::ZMQFrame(status.data(), status.size())});
    }
    auto SetUserID(const std::string& userID) -> bool override
    {
        return set_field(
            USER_ID_POSITION,
            OTZMQFrame{factory::ZMQFrame(userID.data(), userID.size())});
    }

    ~Reply() override = default;

private:
    friend network::zeromq::zap::Reply;

    using CodeMap = std::map<zap::Status, std::string>;
    using CodeReverseMap = std::map<std::string, zap::Status>;

    static const CodeMap code_map_;
    static const CodeReverseMap code_reverse_map_;

    static auto code_to_string(const zap::Status& code) -> std::string;
    static auto invert_code_map(const CodeMap& input) -> CodeReverseMap;
    static auto string_to_code(const std::string& string) -> zap::Status;

    auto clone() const -> Reply* override { return new Reply(*this); }

    Reply(
        const Request& request,
        const zap::Status& code,
        const std::string& status,
        const std::string& userID,
        const Data& metadata,
        const std::string& version);
    Reply() = delete;
    Reply(const Reply&);
    Reply(Reply&&) = delete;
    auto operator=(const Reply&) -> Reply& = delete;
    auto operator=(Reply&&) -> Reply& = delete;
};
}  // namespace opentxs::network::zeromq::zap::implementation

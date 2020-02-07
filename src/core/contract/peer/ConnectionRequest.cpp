// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/contract/peer/ConnectionRequest.hpp"
#include "opentxs/core/Log.hpp"

#include "core/contract/peer/PeerRequest.hpp"
#include "internal/api/Api.hpp"

#include "ConnectionRequest.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
using ParentType = contract::peer::implementation::Request;
using ReturnType = contract::peer::request::implementation::Connection;

auto Factory::ConnectionRequest(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipient,
    const proto::ConnectionInfoType type,
    const identifier::Server& server,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::request::Connection>
{
    try {
        auto output =
            std::make_shared<ReturnType>(api, nym, recipient, type, server);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::ConnectionRequest(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerRequest& serialized) noexcept
    -> std::shared_ptr<contract::peer::request::Connection>
{
    if (false == proto::Validate(serialized, VERBOSE)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Invalid serialized request.")
            .Flush();

        return {};
    }

    try {
        auto output = std::make_shared<ReturnType>(api, nym, serialized);

        OT_ASSERT(output);

        auto& contract = *output;
        Lock lock(contract.lock_);

        if (false == contract.validate(lock)) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid request.")
                .Flush();

            return {};
        }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
Connection::Connection(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const proto::ConnectionInfoType type,
    const identifier::Server& serverID)
    : Request(
          api,
          nym,
          CURRENT_VERSION,
          recipientID,
          serverID,
          proto::PEERREQUEST_CONNECTIONINFO)
    , connection_type_(type)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Connection::Connection(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Request(api, nym, serialized)
    , connection_type_(serialized.connectioninfo().type())
{
    Lock lock(lock_);
    init_serialized(lock);
}

Connection::Connection(const Connection& rhs)
    : Request(rhs)
    , connection_type_(rhs.connection_type_)
{
}

auto Connection::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Request::IDVersion(lock);
    auto& connectioninfo = *contract.mutable_connectioninfo();
    connectioninfo.set_version(version_);
    connectioninfo.set_type(connection_type_);

    return contract;
}
}  // namespace opentxs::contract::peer::request::implementation

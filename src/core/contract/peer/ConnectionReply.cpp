// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                            // IWYU pragma: associated
#include "1_Internal.hpp"                          // IWYU pragma: associated
#include "core/contract/peer/ConnectionReply.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/peer/PeerReply.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PeerRequestType.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ConnectionInfoReply.pb.h"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/verify/PeerReply.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
using ParentType = contract::peer::implementation::Reply;
using ReturnType = contract::peer::reply::implementation::Connection;

auto Factory::ConnectionReply(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& initiator,
    const Identifier& request,
    const identifier::Server& server,
    const bool ack,
    const std::string& url,
    const std::string& login,
    const std::string& password,
    const std::string& key,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::reply::Connection>
{
    auto pRequest = ParentType::LoadRequest(api, nym, request);

    if (false == bool(pRequest)) { return {}; }

    const auto peerRequest = *pRequest;

    try {
        auto output = std::make_shared<ReturnType>(
            api,
            nym,
            api.Factory().NymID(peerRequest.initiator()),
            request,
            server,
            ack,
            url,
            login,
            password,
            key);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::ConnectionReply(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerReply& serialized) noexcept
    -> std::shared_ptr<contract::peer::reply::Connection>
{
    if (false == proto::Validate(serialized, VERBOSE)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Invalid serialized reply.")
            .Flush();

        return {};
    }

    try {
        auto output = std::make_shared<ReturnType>(api, nym, serialized);

        OT_ASSERT(output);

        auto& contract = *output;
        Lock lock(contract.lock_);

        if (false == contract.validate(lock)) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid reply.")
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

namespace opentxs::contract::peer::reply::implementation
{
Connection::Connection(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& initiator,
    const Identifier& request,
    const identifier::Server& server,
    const bool ack,
    const std::string& url,
    const std::string& login,
    const std::string& password,
    const std::string& key)
    : Reply(
          api,
          nym,
          CURRENT_VERSION,
          initiator,
          server,
          core::PeerRequestType::ConnectionInfo,
          request)
    , success_(ack)
    , url_(url)
    , login_(login)
    , password_(password)
    , key_(key)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Connection::Connection(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Reply(api, nym, serialized)
    , success_(serialized.connectioninfo().success())
    , url_(serialized.connectioninfo().url())
    , login_(serialized.connectioninfo().login())
    , password_(serialized.connectioninfo().password())
    , key_(serialized.connectioninfo().key())
{
    Lock lock(lock_);
    init_serialized(lock);
}

Connection::Connection(const Connection& rhs)
    : Reply(rhs)
    , success_(rhs.success_)
    , url_(rhs.url_)
    , login_(rhs.login_)
    , password_(rhs.password_)
    , key_(rhs.key_)
{
}

auto Connection::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Reply::IDVersion(lock);
    auto& connectioninfo = *contract.mutable_connectioninfo();
    connectioninfo.set_version(version_);
    connectioninfo.set_success(success_);
    connectioninfo.set_url(url_);
    connectioninfo.set_login(login_);
    connectioninfo.set_password(password_);
    connectioninfo.set_key(key_);

    return contract;
}
}  // namespace opentxs::contract::peer::reply::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "core/contract/peer/NoticeAcknowledgement.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/peer/PeerReply.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/NoticeAcknowledgement.pb.h"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/verify/PeerReply.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
using ParentType = contract::peer::implementation::Reply;
using ReturnType = contract::peer::reply::implementation::Acknowledgement;

auto Factory::NoticeAcknowledgement(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& initiator,
    const Identifier& request,
    const identifier::Server& server,
    const core::PeerRequestType type,
    const bool& ack,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::reply::Acknowledgement>
{
    try {
        auto pRequest = ParentType::LoadRequest(api, nym, request);

        if (false == bool(pRequest)) { return {}; }

        const auto peerRequest = *pRequest;
        auto output = std::make_shared<ReturnType>(
            api,
            nym,
            api.Factory().NymID(peerRequest.initiator()),
            request,
            server,
            type,
            ack);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::NoticeAcknowledgement(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerReply& serialized) noexcept
    -> std::shared_ptr<contract::peer::reply::Acknowledgement>
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
Acknowledgement::Acknowledgement(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& initiator,
    const Identifier& request,
    const identifier::Server& server,
    const core::PeerRequestType type,
    const bool& ack)
    : Reply(api, nym, CURRENT_VERSION, initiator, server, type, request)
    , ack_(ack)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Acknowledgement::Acknowledgement(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Reply(api, nym, serialized)
    , ack_(serialized.notice().ack())
{
    Lock lock(lock_);
    init_serialized(lock);
}

Acknowledgement::Acknowledgement(const Acknowledgement& rhs)
    : Reply(rhs)
    , ack_(rhs.ack_)
{
}

auto Acknowledgement::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Reply::IDVersion(lock);
    auto& notice = *contract.mutable_notice();
    notice.set_version(version_);
    notice.set_ack(ack_);

    return contract;
}
}  // namespace opentxs::contract::peer::reply::implementation

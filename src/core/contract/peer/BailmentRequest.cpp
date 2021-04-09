// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                            // IWYU pragma: associated
#include "1_Internal.hpp"                          // IWYU pragma: associated
#include "core/contract/peer/BailmentRequest.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/peer/PeerRequest.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PeerRequestType.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/Bailment.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/verify/PeerRequest.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
using ParentType = contract::peer::implementation::Request;
using ReturnType = contract::peer::request::implementation::Bailment;

auto Factory::BailmentRequest(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipient,
    const identifier::UnitDefinition& unit,
    const identifier::Server& server,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::request::Bailment>
{
    try {
        api.Wallet().UnitDefinition(unit);

        auto output =
            std::make_shared<ReturnType>(api, nym, recipient, unit, server);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::BailmentRequest(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerRequest& serialized) noexcept
    -> std::shared_ptr<contract::peer::request::Bailment>
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
Bailment::Bailment(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const identifier::UnitDefinition& unitID,
    const identifier::Server& serverID)
    : Request(
          api,
          nym,
          CURRENT_VERSION,
          recipientID,
          serverID,
          core::PeerRequestType::Bailment)
    , unit_(unitID)
    , server_(serverID)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Bailment::Bailment(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Request(api, nym, serialized)
    , unit_(api_.Factory().UnitID(serialized.bailment().unitid()))
    , server_(api_.Factory().ServerID(serialized.bailment().serverid()))
{
    Lock lock(lock_);
    init_serialized(lock);
}

Bailment::Bailment(const Bailment& rhs)
    : Request(rhs)
    , unit_(rhs.unit_)
    , server_(rhs.server_)
{
}

auto Bailment::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Request::IDVersion(lock);
    auto& bailment = *contract.mutable_bailment();
    bailment.set_version(version_);
    bailment.set_unitid(unit_->str());
    bailment.set_serverid(server_->str());

    return contract;
}
}  // namespace opentxs::contract::peer::request::implementation

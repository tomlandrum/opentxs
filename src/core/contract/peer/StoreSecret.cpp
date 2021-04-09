// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "core/contract/peer/StoreSecret.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/peer/PeerRequest.hpp"
#include "internal/core/contract/peer/Peer.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/peer/PeerRequestType.hpp"
#include "opentxs/core/contract/peer/Types.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/StoreSecret.pb.h"
#include "opentxs/protobuf/verify/PeerRequest.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
using ParentType = contract::peer::implementation::Request;
using ReturnType = contract::peer::request::implementation::StoreSecret;

auto Factory::StoreSecret(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const contract::peer::SecretType type,
    const std::string& primary,
    const std::string& secondary,
    const identifier::Server& server,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::peer::request::StoreSecret>
{
    try {
        auto output = std::make_shared<ReturnType>(
            api, nym, recipientID, type, primary, secondary, server);

        OT_ASSERT(output);

        auto& reply = *output;

        if (false == ParentType::Finish(reply, reason)) { return {}; }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::StoreSecret(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::PeerRequest& serialized) noexcept
    -> std::shared_ptr<contract::peer::request::StoreSecret>
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
StoreSecret::StoreSecret(
    const api::internal::Core& api,
    const Nym_p& nym,
    const identifier::Nym& recipientID,
    const SecretType type,
    const std::string& primary,
    const std::string& secondary,
    const identifier::Server& serverID)
    : Request(
          api,
          nym,
          CURRENT_VERSION,
          recipientID,
          serverID,
          contract::peer::PeerRequestType::StoreSecret)
    , secret_type_(type)
    , primary_(primary)
    , secondary_(secondary)
{
    Lock lock(lock_);
    first_time_init(lock);
}

StoreSecret::StoreSecret(
    const api::internal::Core& api,
    const Nym_p& nym,
    const SerializedType& serialized)
    : Request(api, nym, serialized)
    , secret_type_(internal::translate(serialized.storesecret().type()))
    , primary_(serialized.storesecret().primary())
    , secondary_(serialized.storesecret().secondary())
{
    Lock lock(lock_);
    init_serialized(lock);
}

StoreSecret::StoreSecret(const StoreSecret& rhs)
    : Request(rhs)
    , secret_type_(rhs.secret_type_)
    , primary_(rhs.primary_)
    , secondary_(rhs.secondary_)
{
}

auto StoreSecret::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Request::IDVersion(lock);
    auto& storesecret = *contract.mutable_storesecret();
    storesecret.set_version(version_);
    storesecret.set_type(internal::translate(secret_type_));
    storesecret.set_primary(primary_);
    storesecret.set_secondary(secondary_);

    return contract;
}
}  // namespace opentxs::contract::peer::request::implementation

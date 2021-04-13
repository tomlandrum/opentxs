// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                             // IWYU pragma: associated
#include "1_Internal.hpp"                           // IWYU pragma: associated
#include "core/contract/basket/BasketContract.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/UnitDefinition.hpp"
#include "internal/core/contract/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/protobuf/BasketItem.pb.h"
#include "opentxs/protobuf/BasketParams.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/UnitDefinition.hpp"

using ReturnType = opentxs::contract::unit::implementation::Basket;

namespace opentxs
{
// Unlike the other factory functions, this one does not produce a complete,
// valid contract. This is used on the client side to produce a template for
// the server, which then finalizes the contract.
auto Factory::BasketContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const std::uint64_t weight,
    const contact::ContactItemType unitOfAccount,
    const VersionNumber version) noexcept
    -> std::shared_ptr<contract::unit::Basket>
{
    return std::make_shared<ReturnType>(
        api,
        nym,
        shortname,
        name,
        symbol,
        terms,
        weight,
        unitOfAccount,
        version);
}

auto Factory::BasketContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized) noexcept
    -> std::shared_ptr<contract::unit::Basket>
{
    if (false == proto::Validate<ReturnType::SerializedType>(
                     serialized, VERBOSE, true)) {

        return {};
    }

    auto output = std::make_shared<ReturnType>(api, nym, serialized);

    if (false == bool(output)) { return {}; }

    auto& contract = *output;
    Lock lock(contract.lock_);

    if (!contract.validate(lock)) { return {}; }

    return std::move(output);
}
}  // namespace opentxs

namespace opentxs::contract::unit
{
auto Basket::CalculateBasketID(
    const api::internal::Core& api,
    const proto::UnitDefinition& serialized) -> OTIdentifier
{
    auto contract(serialized);
    contract.clear_id();
    contract.clear_nymid();
    contract.clear_publicnym();

    for (auto& item : *contract.mutable_basket()->mutable_item()) {
        item.clear_account();
    }

    return contract::implementation::Unit::GetID(api, contract);
}

auto Basket::FinalizeTemplate(
    const api::internal::Core& api,
    const Nym_p& nym,
    proto::UnitDefinition& serialized,
    const PasswordPrompt& reason) -> bool
{
    auto contract = std::make_unique<ReturnType>(api, nym, serialized);

    if (!contract) { return false; }

    Lock lock(contract->lock_);

    try {
        contract->first_time_init(lock);
    } catch (const std::exception& e) {
        LogOutput("opentxs::contract::unit::Basket::")(__FUNCTION__)(": ")(
            e.what())
            .Flush();
    }

    if (contract->nym_) {
        proto::UnitDefinition basket = contract->SigVersion(lock);
        std::shared_ptr<proto::Signature> sig =
            std::make_shared<proto::Signature>();
        if (contract->update_signature(lock, reason)) {
            lock.unlock();
            serialized = contract->PublicContract();

            return proto::Validate(serialized, VERBOSE, false);
        }
    }

    return false;
}
}  // namespace opentxs::contract::unit

namespace opentxs::contract::unit::implementation
{
Basket::Basket(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const std::uint64_t weight,
    const contact::ContactItemType unitOfAccount,
    const VersionNumber version)
    : Unit(api, nym, shortname, name, symbol, terms, unitOfAccount, version)
    , subcontracts_()
    , weight_(weight)
{
}

Basket::Basket(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized)
    : Unit(api, nym, serialized)
    , subcontracts_()
    , weight_(0)
{
    if (serialized.has_basket()) {

        if (serialized.basket().has_weight()) {
            weight_ = serialized.basket().weight();
        }

        for (auto& item : serialized.basket().item()) {
            subcontracts_.insert(
                {item.unit(), {item.account(), item.weight()}});
        }
    }
}

Basket::Basket(const Basket& rhs)
    : Unit(rhs)
    , subcontracts_(rhs.subcontracts_)
    , weight_(rhs.weight_)
{
}

auto Basket::BasketID() const -> OTIdentifier
{
    Lock lock(lock_);

    return GetID(api_, BasketIDVersion(lock));
}

auto Basket::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Unit::IDVersion(lock);
    contract.set_type(
        contract::internal::translate(contract::UnitType::Basket));
    auto basket = contract.mutable_basket();
    basket->set_version(1);
    basket->set_weight(weight_);

    // determinism here depends on the defined ordering of std::map
    for (auto& item : subcontracts_) {
        auto serialized = basket->add_item();
        serialized->set_version(1);
        serialized->set_unit(item.first);
        serialized->set_account(item.second.first);
        serialized->set_weight(item.second.second);
    }

    return contract;
}

auto Basket::BasketIDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Unit::SigVersion(lock);

    for (auto& item : *(contract.mutable_basket()->mutable_item())) {
        item.clear_account();
    }

    return contract;
}
}  // namespace opentxs::contract::unit::implementation

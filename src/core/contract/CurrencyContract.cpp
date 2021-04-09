// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "core/contract/CurrencyContract.hpp"  // IWYU pragma: associated

#include <memory>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/UnitDefinition.hpp"
#include "internal/core/contract/Contract.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/CurrencyParams.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/UnitDefinition.hpp"

namespace opentxs
{
using ReturnType = contract::unit::implementation::Currency;

auto Factory::CurrencyContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const std::string& tla,
    const std::uint32_t power,
    const std::string& fraction,
    const proto::ContactItemType unitOfAccount,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::unit::Currency>
{
    auto output = std::make_shared<ReturnType>(
        api,
        nym,
        shortname,
        name,
        symbol,
        terms,
        tla,
        power,
        fraction,
        unitOfAccount,
        version);

    if (false == bool(output)) { return {}; }

    auto& contract = *output;
    Lock lock(contract.lock_);

    if (contract.nym_) {
        auto serialized = contract.SigVersion(lock);
        auto sig = std::make_shared<proto::Signature>();

        if (!contract.update_signature(lock, reason)) { return {}; }
    }

    if (!contract.validate(lock)) { return {}; }

    return std::move(output);
}

auto Factory::CurrencyContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized) noexcept
    -> std::shared_ptr<contract::unit::Currency>
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

namespace opentxs::contract::unit::implementation
{
Currency::Currency(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const std::string& tla,
    const std::uint32_t power,
    const std::string& fraction,
    const proto::ContactItemType unitOfAccount,
    const VersionNumber version)
    : Unit(api, nym, shortname, name, symbol, terms, unitOfAccount, version)
    , tla_(tla)
    , power_(power)
    , fractional_unit_name_(fraction)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Currency::Currency(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized)
    : Unit(api, nym, serialized)
    , tla_(serialized.currency().tla())
    , power_(serialized.currency().power())
    , fractional_unit_name_(serialized.currency().fraction())
{
    Lock lock(lock_);
    init_serialized(lock);
}

Currency::Currency(const Currency& rhs)
    : Unit(rhs)
    , tla_(rhs.tla_)
    , power_(rhs.power_)
    , fractional_unit_name_(rhs.fractional_unit_name_)
{
}

auto Currency::IDVersion(const Lock& lock) const -> SerializedType
{
    auto contract = Unit::IDVersion(lock);
    contract.set_type(
        contract::internal::translate(contract::UnitType::Currency));
    auto& currency = *contract.mutable_currency();
    currency.set_version(1);
    currency.set_tla(tla_);
    currency.set_power(power_);
    currency.set_fraction(fractional_unit_name_);

    return contract;
}
}  // namespace opentxs::contract::unit::implementation

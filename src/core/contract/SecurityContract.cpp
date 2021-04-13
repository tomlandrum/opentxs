// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "core/contract/SecurityContract.hpp"  // IWYU pragma: associated

#include <memory>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/UnitDefinition.hpp"
#include "internal/core/contract/Contract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/EquityParams.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/UnitDefinition.hpp"

namespace opentxs
{
using ReturnType = contract::unit::implementation::Security;

auto Factory::SecurityContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const contact::ContactItemType unitOfAccount,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::shared_ptr<contract::unit::Security>
{
    auto output = std::make_shared<ReturnType>(
        api, nym, shortname, name, symbol, terms, unitOfAccount, version);

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

auto Factory::SecurityContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized) noexcept
    -> std::shared_ptr<contract::unit::Security>
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
Security::Security(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const contact::ContactItemType unitOfAccount,
    const VersionNumber version)
    : Unit(api, nym, shortname, name, symbol, terms, unitOfAccount, version)
{
    Lock lock(lock_);
    first_time_init(lock);
}

Security::Security(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::UnitDefinition serialized)
    : Unit(api, nym, serialized)
{
    Lock lock(lock_);
    init_serialized(lock);
}

Security::Security(const Security& rhs)
    : Unit(rhs)
{
}

auto Security::IDVersion(const Lock& lock) const -> proto::UnitDefinition
{
    auto contract = Unit::IDVersion(lock);
    contract.set_type(contract::internal::translate(Type()));
    auto& security = *contract.mutable_security();
    security.set_version(1);
    security.set_type(proto::EQUITYTYPE_SHARES);

    return contract;
}
}  // namespace opentxs::contract::unit::implementation

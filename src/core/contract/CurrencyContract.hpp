// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/CurrencyContract.cpp"

#pragma once

#include <cstdint>
#include <string>

#include "core/contract/UnitDefinition.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/core/contract/CurrencyContract.hpp"
#include "opentxs/protobuf/UnitDefinition.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::unit::implementation
{
class Currency final : public unit::Currency,
                       public contract::implementation::Unit
{
public:
    auto DecimalPower() const -> std::int32_t final { return power_; }
    auto FractionalUnitName() const -> std::string final
    {
        return fractional_unit_name_;
    }
    auto TLA() const -> std::string final { return tla_; }
    auto Type() const -> contract::UnitType final
    {
        return contract::UnitType::Currency;
    }

    Currency(
        const api::internal::Core& api,
        const Nym_p& nym,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const std::string& tla,
        const std::uint32_t power,
        const std::string& fraction,
        const contact::ContactItemType unitOfAccount,
        const VersionNumber version);
    Currency(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::UnitDefinition serialized);

    ~Currency() final = default;

private:
    // ISO-4217. E.g., USD, AUG, PSE. Take as hint, not as contract.
    const std::string tla_;
    // If value is 103, decimal power of 0 displays 103 (actual value.) Whereas
    // decimal power of 2 displays 1.03 and 4 displays .0103
    const std::uint32_t power_;
    // "cents"
    const std::string fractional_unit_name_;

    auto clone() const noexcept -> Currency* final
    {
        return new Currency(*this);
    }
    auto IDVersion(const Lock& lock) const -> proto::UnitDefinition final;

    Currency(const Currency&);
    Currency(Currency&&) = delete;
    auto operator=(const Currency&) -> Currency& = delete;
    auto operator=(Currency&&) -> Currency& = delete;
};
}  // namespace opentxs::contract::unit::implementation

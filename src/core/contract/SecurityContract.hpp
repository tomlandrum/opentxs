// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/SecurityContract.cpp"

#pragma once

#include <string>

#include "core/contract/UnitDefinition.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/core/contract/SecurityContract.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
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

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::unit::implementation
{
class Security final : public unit::Security,
                       public contract::implementation::Unit
{
public:
    auto TLA() const -> std::string final { return primary_unit_symbol_; }
    auto Type() const -> contract::UnitType final
    {
        return contract::UnitType::Security;
    }

    Security(
        const api::internal::Core& api,
        const Nym_p& nym,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const proto::ContactItemType unitOfAccount,
        const VersionNumber version);
    Security(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::UnitDefinition serialized);

    ~Security() final = default;

private:
    friend opentxs::Factory;

    auto clone() const noexcept -> Security* final
    {
        return new Security(*this);
    }
    auto IDVersion(const Lock& lock) const -> proto::UnitDefinition final;

    Security(const Security&);
    Security(Security&&) = delete;
    auto operator=(const Security&) -> Security& = delete;
    auto operator=(Security&&) -> Security& = delete;
};
}  // namespace opentxs::contract::unit::implementation

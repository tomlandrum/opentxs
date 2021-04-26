// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/peer/BailmentRequest.cpp"

#pragma once

#include "core/contract/peer/PeerRequest.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/peer/BailmentRequest.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class PeerRequest;
}  // namespace proto

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
class Bailment final : public request::Bailment,
                       public peer::implementation::Request
{
public:
    Bailment(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& recipientID,
        const identifier::UnitDefinition& unitID,
        const identifier::Server& serverID);
    Bailment(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::PeerRequest& serialized);

    ~Bailment() final = default;

    auto ServerID() const -> const identifier::Server& final { return server_; }
    auto UnitID() const -> const identifier::UnitDefinition& final
    {
        return unit_;
    }

private:
    friend opentxs::Factory;

    const OTUnitID unit_;
    const OTServerID server_;

    auto clone() const noexcept -> Bailment* final
    {
        return new Bailment(*this);
    }
    auto IDVersion(const Lock& lock) const -> SerializedType final;

    Bailment() = delete;
    Bailment(const Bailment&);
    Bailment(Bailment&&) = delete;
    auto operator=(const Bailment&) -> Bailment& = delete;
    auto operator=(Bailment&&) -> Bailment& = delete;
};
}  // namespace opentxs::contract::peer::request::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

#include "core/contract/Signable.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/contract/peer/Types.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

class Core;
}  // namespace api

namespace identifier
{
class Server;
}  // namespace identifier

namespace proto
{
class Signature;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::implementation
{
class Request : virtual public peer::Request,
                public opentxs::contract::implementation::Signable
{
public:
    static auto Finish(Request& contract, const PasswordPrompt& reason) -> bool;

    auto Alias() const -> std::string final { return Name(); }
    auto Contract() const -> SerializedType final;
    auto Initiator() const -> const identifier::Nym& final
    {
        return initiator_;
    }
    auto Name() const -> std::string final { return id_->str(); }
    auto Recipient() const -> const identifier::Nym& final
    {
        return recipient_;
    }
    auto Serialize() const -> OTData final;
    auto Type() const -> PeerRequestType final { return type_; }
    void SetAlias(const std::string&) final {}

    ~Request() override = default;

protected:
    virtual auto IDVersion(const Lock& lock) const -> SerializedType;
    auto validate(const Lock& lock) const -> bool final;
    auto verify_signature(const Lock& lock, const proto::Signature& signature)
        const -> bool final;

    Request(
        const api::internal::Core& api,
        const Nym_p& nym,
        VersionNumber version,
        const identifier::Nym& recipient,
        const identifier::Server& serverID,
        const PeerRequestType& type,
        const std::string& conditions = {});
    Request(
        const api::internal::Core& api,
        const Nym_p& nym,
        const SerializedType& serialized,
        const std::string& conditions = {});
    Request(const Request&) noexcept;

private:
    using ot_super = Signable;

    const OTNymID initiator_;
    const OTNymID recipient_;
    const OTIdentifier server_;
    const OTIdentifier cookie_;
    const PeerRequestType type_;

    static auto GetID(
        const api::internal::Core& api,
        const SerializedType& contract) -> OTIdentifier;
    static auto FinalizeContract(
        Request& contract,
        const PasswordPrompt& reason) -> bool;

    auto contract(const Lock& lock) const -> SerializedType;
    auto GetID(const Lock& lock) const -> OTIdentifier final;
    auto SigVersion(const Lock& lock) const -> SerializedType;

    auto update_signature(const Lock& lock, const PasswordPrompt& reason)
        -> bool final;

    Request() = delete;
};
}  // namespace opentxs::contract::peer::implementation

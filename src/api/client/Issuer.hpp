// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/contract/peer/PeerRequestType.hpp"
#include "opentxs/core/contract/peer/Types.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/Issuer.pb.h"

namespace opentxs
{
namespace api
{
class Wallet;
}  // namespace api

class Factory;
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
class Issuer final : virtual public opentxs::api::client::Issuer, Lockable
{
public:
    auto toString() const -> std::string final;

    auto AccountList(
        const contact::ContactItemType type,
        const identifier::UnitDefinition& unitID) const
        -> std::set<OTIdentifier> final;
    auto BailmentInitiated(const identifier::UnitDefinition& unitID) const
        -> bool final;
    auto BailmentInstructions(
        const identifier::UnitDefinition& unitID,
        const bool onlyUnused = true) const
        -> std::vector<BailmentDetails> final;
    auto BailmentInstructionsSize(
        const identifier::UnitDefinition& unitID) const -> std::size_t final;
    auto ConnectionInfo(const contract::peer::ConnectionInfoType type) const
        -> std::vector<ConnectionDetails> final;
    auto ConnectionInfoInitiated(
        const contract::peer::ConnectionInfoType type) const -> bool final;
    auto ConnectionInfoSize(const contract::peer::ConnectionInfoType type) const
        -> std::size_t final;
    auto GetRequests(
        const contract::peer::PeerRequestType type,
        const RequestStatus state = RequestStatus::All) const
        -> std::set<std::tuple<OTIdentifier, OTIdentifier, bool>> final;
    auto IssuerID() const -> const identifier::Nym& final { return issuer_id_; }
    auto LocalNymID() const -> const identifier::Nym& final { return nym_id_; }
    auto Paired() const -> bool final;
    auto PairingCode() const -> const std::string& final;
    auto PrimaryServer() const -> OTServerID final;
    auto RequestTypes() const
        -> std::set<contract::peer::PeerRequestType> final;
    auto Serialize() const -> proto::Issuer final;
    auto StoreSecretComplete() const -> bool final;
    auto StoreSecretInitiated() const -> bool final;

    void AddAccount(
        const contact::ContactItemType type,
        const identifier::UnitDefinition& unitID,
        const Identifier& accountID) final;
    auto AddReply(
        const contract::peer::PeerRequestType type,
        const Identifier& requestID,
        const Identifier& replyID) -> bool final;
    auto AddRequest(
        const contract::peer::PeerRequestType type,
        const Identifier& requestID) -> bool final;
    auto RemoveAccount(
        const contact::ContactItemType type,
        const identifier::UnitDefinition& unitID,
        const Identifier& accountID) -> bool final;
    void SetPaired(const bool paired) final;
    void SetPairingCode(const std::string& code) final;
    auto SetUsed(
        const contract::peer::PeerRequestType type,
        const Identifier& requestID,
        const bool isUsed = true) -> bool final;

    Issuer(
        const api::Wallet& wallet,
        const identifier::Nym& nymID,
        const proto::Issuer& serialized);
    Issuer(
        const api::Wallet& wallet,
        const identifier::Nym& nymID,
        const identifier::Nym& issuerID);

    ~Issuer() final;

private:
    using Workflow = std::map<OTIdentifier, std::pair<OTIdentifier, bool>>;
    using WorkflowMap = std::map<contract::peer::PeerRequestType, Workflow>;
    using UnitAccountPair = std::pair<OTUnitID, OTIdentifier>;

    const api::Wallet& wallet_;
    VersionNumber version_{0};
    std::string pairing_code_{""};
    mutable OTFlag paired_;
    const OTNymID nym_id_;
    const OTNymID issuer_id_;
    std::map<contact::ContactItemType, std::set<UnitAccountPair>> account_map_;
    WorkflowMap peer_requests_;

    auto bailment_instructions(
        const Lock& lock,
        const identifier::UnitDefinition& unitID,
        const bool onlyUnused = true) const -> std::vector<BailmentDetails>;
    auto connection_info(
        const Lock& lock,
        const contract::peer::ConnectionInfoType type) const
        -> std::vector<Issuer::ConnectionDetails>;
    auto find_request(
        const Lock& lock,
        const contract::peer::PeerRequestType type,
        const Identifier& requestID) -> std::pair<bool, Workflow::iterator>;
    auto get_requests(
        const Lock& lock,
        const contract::peer::PeerRequestType type,
        const RequestStatus state = RequestStatus::All) const
        -> std::set<std::tuple<OTIdentifier, OTIdentifier, bool>>;

    auto add_request(
        const Lock& lock,
        const contract::peer::PeerRequestType type,
        const Identifier& requestID,
        const Identifier& replyID) -> bool;

    Issuer() = delete;
    Issuer(const Issuer&) = delete;
    Issuer(Issuer&&) = delete;
    auto operator=(const Issuer&) -> Issuer& = delete;
    auto operator=(Issuer&&) -> Issuer& = delete;
};
}  // namespace opentxs::api::client::implementation

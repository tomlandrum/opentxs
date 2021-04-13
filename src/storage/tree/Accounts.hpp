// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>
#include <set>
#include <string>
#include <tuple>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/StorageAccounts.pb.h"
#include "storage/tree/Node.hpp"

namespace opentxs
{
namespace api
{
namespace storage
{
class Driver;
}  // namespace storage
}  // namespace api

namespace storage
{
class Tree;
}  // namespace storage
}  // namespace opentxs

namespace opentxs::storage
{
class Accounts final : public Node
{
public:
    auto AccountContract(const Identifier& accountID) const -> OTUnitID;
    auto AccountIssuer(const Identifier& accountID) const -> OTNymID;
    auto AccountOwner(const Identifier& accountID) const -> OTNymID;
    auto AccountServer(const Identifier& accountID) const -> OTServerID;
    auto AccountSigner(const Identifier& accountID) const -> OTNymID;
    auto AccountUnit(const Identifier& accountID) const
        -> contact::ContactItemType;
    auto AccountsByContract(const identifier::UnitDefinition& unit) const
        -> std::set<OTIdentifier>;
    auto AccountsByIssuer(const identifier::Nym& issuerNym) const
        -> std::set<OTIdentifier>;
    auto AccountsByOwner(const identifier::Nym& ownerNym) const
        -> std::set<OTIdentifier>;
    auto AccountsByServer(const identifier::Server& server) const
        -> std::set<OTIdentifier>;
    auto AccountsByUnit(const contact::ContactItemType unit) const
        -> std::set<OTIdentifier>;
    auto Alias(const std::string& id) const -> std::string;
    auto Load(
        const std::string& id,
        std::string& output,
        std::string& alias,
        const bool checking) const -> bool;

    auto Delete(const std::string& id) -> bool;
    auto SetAlias(const std::string& id, const std::string& alias) -> bool;
    auto Store(
        const std::string& id,
        const std::string& data,
        const std::string& alias,
        const identifier::Nym& ownerNym,
        const identifier::Nym& signerNym,
        const identifier::Nym& issuerNym,
        const identifier::Server& server,
        const identifier::UnitDefinition& contract,
        const contact::ContactItemType unit) -> bool;

    ~Accounts() final = default;

private:
    friend Tree;

    using NymIndex = std::map<OTNymID, std::set<OTIdentifier>>;
    using ServerIndex = std::map<OTServerID, std::set<OTIdentifier>>;
    using ContractIndex = std::map<OTUnitID, std::set<OTIdentifier>>;
    using UnitIndex =
        std::map<contact::ContactItemType, std::set<OTIdentifier>>;
    /** owner, signer, issuer, server, contract, unit */
    using AccountData = std::tuple<
        OTNymID,
        OTNymID,
        OTNymID,
        OTServerID,
        OTUnitID,
        contact::ContactItemType>;
    using ReverseIndex = std::map<OTIdentifier, AccountData>;

    NymIndex owner_index_{};
    NymIndex signer_index_{};
    NymIndex issuer_index_{};
    ServerIndex server_index_{};
    ContractIndex contract_index_{};
    UnitIndex unit_index_{};
    mutable ReverseIndex account_data_{};

    template <typename A, typename M, typename I>
    static auto add_set_index(
        const Identifier& accountID,
        const A& argID,
        M& mapID,
        I& index) -> bool;

    template <typename K, typename I>
    static void erase(const Identifier& accountID, const K& key, I& index)
    {
        try {
            auto& set = index.at(key);
            set.erase(accountID);

            if (0 == set.size()) { index.erase(key); }
        } catch (...) {
        }
    }

    auto get_account_data(const Lock& lock, const OTIdentifier& accountID) const
        -> AccountData&;
    auto serialize() const -> proto::StorageAccounts;

    auto check_update_account(
        const Lock& lock,
        const OTIdentifier& accountID,
        const identifier::Nym& ownerNym,
        const identifier::Nym& signerNym,
        const identifier::Nym& issuerNym,
        const identifier::Server& server,
        const identifier::UnitDefinition& contract,
        const contact::ContactItemType unit) -> bool;
    void init(const std::string& hash) final;
    auto save(const Lock& lock) const -> bool final;

    Accounts(
        const opentxs::api::storage::Driver& storage,
        const std::string& key);
    Accounts() = delete;
    Accounts(const Accounts&) = delete;
    Accounts(Accounts&&) = delete;
    auto operator=(const Accounts&) -> Accounts = delete;
    auto operator=(Accounts&&) -> Accounts = delete;
};
}  // namespace opentxs::storage

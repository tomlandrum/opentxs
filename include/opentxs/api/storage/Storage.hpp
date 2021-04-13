// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_STORAGE_STORAGE_HPP
#define OPENTXS_API_STORAGE_STORAGE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Types.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"

namespace opentxs
{
namespace proto
{
class Bip47Channel;
class Ciphertext;
class Contact;
class Context;
class Credential;
class HDAccount;
class Issuer;
class Nym;
class PaymentWorkflow;
class PeerReply;
class PeerRequest;
class Purse;
class Seed;
class ServerContract;
class StorageThread;
class UnitDefinition;
}  // namespace proto

using NymLambda = std::function<void(const proto::Nym&)>;
using ServerLambda = std::function<void(const proto::ServerContract&)>;
using UnitLambda = std::function<void(const proto::UnitDefinition&)>;
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace storage
{
class Storage
{
public:
    using Bip47ChannelList = std::set<OTIdentifier>;

    OPENTXS_EXPORT virtual std::string AccountAlias(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual ObjectList AccountList() const = 0;
    OPENTXS_EXPORT virtual OTUnitID AccountContract(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual OTNymID AccountIssuer(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual OTNymID AccountOwner(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual OTServerID AccountServer(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual OTNymID AccountSigner(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType AccountUnit(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> AccountsByContract(
        const identifier::UnitDefinition& contract) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> AccountsByIssuer(
        const identifier::Nym& issuerNym) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> AccountsByOwner(
        const identifier::Nym& ownerNym) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> AccountsByServer(
        const identifier::Server& server) const = 0;
    OPENTXS_EXPORT virtual std::set<OTIdentifier> AccountsByUnit(
        const contact::ContactItemType unit) const = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType Bip47Chain(
        const identifier::Nym& nymID,
        const Identifier& channelID) const = 0;
    OPENTXS_EXPORT virtual Bip47ChannelList Bip47ChannelsByChain(
        const identifier::Nym& nymID,
        const contact::ContactItemType chain) const = 0;
    OPENTXS_EXPORT virtual std::set<std::string> BlockchainAccountList(
        const std::string& nymID,
        const contact::ContactItemType type) const = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType BlockchainAccountType(
        const std::string& nymID,
        const std::string& accountID) const = 0;
    OPENTXS_EXPORT virtual std::vector<OTIdentifier> BlockchainThreadMap(
        const identifier::Nym& nym,
        const Data& txid) const noexcept = 0;
    OPENTXS_EXPORT virtual std::vector<OTData> BlockchainTransactionList(
        const identifier::Nym& nym) const noexcept = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual bool CheckTokenSpent(
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit,
        const std::uint64_t series,
        const std::string& key) const = 0;
#endif
    OPENTXS_EXPORT virtual std::string ContactAlias(
        const std::string& id) const = 0;
    OPENTXS_EXPORT virtual ObjectList ContactList() const = 0;
    OPENTXS_EXPORT virtual ObjectList ContextList(
        const std::string& nymID) const = 0;
    OPENTXS_EXPORT virtual std::string ContactOwnerNym(
        const std::string& nymID) const = 0;
    OPENTXS_EXPORT virtual void ContactSaveIndices() const = 0;
    OPENTXS_EXPORT virtual VersionNumber ContactUpgradeLevel() const = 0;
    OPENTXS_EXPORT virtual bool CreateThread(
        const std::string& nymID,
        const std::string& threadID,
        const std::set<std::string>& participants) const = 0;
    OPENTXS_EXPORT virtual bool DeleteAccount(const std::string& id) const = 0;
    OPENTXS_EXPORT virtual std::string DefaultSeed() const = 0;
    OPENTXS_EXPORT virtual bool DeleteContact(const std::string& id) const = 0;
    OPENTXS_EXPORT virtual bool DeletePaymentWorkflow(
        const std::string& nymID,
        const std::string& workflowID) const = 0;
    OPENTXS_EXPORT virtual std::uint32_t HashType() const = 0;
    OPENTXS_EXPORT virtual ObjectList IssuerList(
        const std::string& nymID) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& accountID,
        std::string& output,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& accountID,
        std::shared_ptr<proto::HDAccount>& output,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const identifier::Nym& nymID,
        const Identifier& channelID,
        std::shared_ptr<proto::Bip47Channel>& output,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Contact>& contact,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Contact>& contact,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nym,
        const std::string& id,
        std::shared_ptr<proto::Context>& context,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Credential>& cred,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Nym>& nym,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Nym>& nym,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& id,
        std::shared_ptr<proto::Issuer>& issuer,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& workflowID,
        std::shared_ptr<proto::PaymentWorkflow>& workflow,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& id,
        const StorageBox box,
        std::string& output,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& id,
        const StorageBox box,
        std::shared_ptr<proto::PeerReply>& request,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymID,
        const std::string& id,
        const StorageBox box,
        std::shared_ptr<proto::PeerRequest>& request,
        std::time_t& time,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const identifier::Nym& nym,
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit,
        std::shared_ptr<proto::Purse>& output,
        const bool checking) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Seed>& seed,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::Seed>& seed,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::ServerContract>& contract,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::ServerContract>& contract,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& nymId,
        const std::string& threadId,
        std::shared_ptr<proto::StorageThread>& thread) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        std::shared_ptr<proto::Ciphertext>& output,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::UnitDefinition>& contract,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual bool Load(
        const std::string& id,
        std::shared_ptr<proto::UnitDefinition>& contract,
        std::string& alias,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual const std::set<std::string> LocalNyms() const = 0;
    OPENTXS_EXPORT virtual void MapPublicNyms(NymLambda& lambda) const = 0;
    OPENTXS_EXPORT virtual void MapServers(ServerLambda& lambda) const = 0;
    OPENTXS_EXPORT virtual void MapUnitDefinitions(
        UnitLambda& lambda) const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual bool MarkTokenSpent(
        const identifier::Server& notary,
        const identifier::UnitDefinition& unit,
        const std::uint64_t series,
        const std::string& key) const = 0;
#endif
    OPENTXS_EXPORT virtual bool MoveThreadItem(
        const std::string& nymId,
        const std::string& fromThreadID,
        const std::string& toThreadID,
        const std::string& itemID) const = 0;
    OPENTXS_EXPORT virtual ObjectList NymBoxList(
        const std::string& nymID,
        const StorageBox box) const = 0;
    OPENTXS_EXPORT virtual ObjectList NymList() const = 0;
    OPENTXS_EXPORT virtual ObjectList PaymentWorkflowList(
        const std::string& nymID) const = 0;
    OPENTXS_EXPORT virtual std::string PaymentWorkflowLookup(
        const std::string& nymID,
        const std::string& sourceID) const = 0;
    OPENTXS_EXPORT virtual std::set<std::string> PaymentWorkflowsByAccount(
        const std::string& nymID,
        const std::string& accountID) const = 0;
    OPENTXS_EXPORT virtual std::set<std::string> PaymentWorkflowsByState(
        const std::string& nymID,
        const api::client::PaymentWorkflowType type,
        const api::client::PaymentWorkflowState state) const = 0;
    OPENTXS_EXPORT virtual std::set<std::string> PaymentWorkflowsByUnit(
        const std::string& nymID,
        const std::string& unitID) const = 0;
    OPENTXS_EXPORT virtual std::pair<
        api::client::PaymentWorkflowType,
        api::client::PaymentWorkflowState>
    PaymentWorkflowState(
        const std::string& nymID,
        const std::string& workflowID) const = 0;
    OPENTXS_EXPORT virtual bool RelabelThread(
        const std::string& threadID,
        const std::string& label) const = 0;
    OPENTXS_EXPORT virtual bool RemoveBlockchainThreadItem(
        const identifier::Nym& nym,
        const Identifier& thread,
        const opentxs::blockchain::Type chain,
        const Data& txid) const noexcept = 0;
    OPENTXS_EXPORT virtual bool RemoveNymBoxItem(
        const std::string& nymID,
        const StorageBox box,
        const std::string& itemID) const = 0;
    OPENTXS_EXPORT virtual bool RemoveServer(const std::string& id) const = 0;
    OPENTXS_EXPORT virtual bool RemoveThreadItem(
        const identifier::Nym& nym,
        const Identifier& thread,
        const std::string& id) const = 0;
    OPENTXS_EXPORT virtual bool RemoveUnitDefinition(
        const std::string& id) const = 0;
    OPENTXS_EXPORT virtual bool RenameThread(
        const std::string& nymId,
        const std::string& threadId,
        const std::string& newID) const = 0;
    OPENTXS_EXPORT virtual void RunGC() const = 0;
    OPENTXS_EXPORT virtual std::string ServerAlias(
        const std::string& id) const = 0;
    OPENTXS_EXPORT virtual ObjectList ServerList() const = 0;
    OPENTXS_EXPORT virtual ObjectList SeedList() const = 0;
    OPENTXS_EXPORT virtual bool SetAccountAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetContactAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetDefaultSeed(const std::string& id) const = 0;
    OPENTXS_EXPORT virtual bool SetNymAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetPeerRequestTime(
        const std::string& nymID,
        const std::string& id,
        const StorageBox box) const = 0;
    OPENTXS_EXPORT virtual bool SetReadState(
        const std::string& nymId,
        const std::string& threadId,
        const std::string& itemId,
        const bool unread) const = 0;
    OPENTXS_EXPORT virtual bool SetSeedAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetServerAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetThreadAlias(
        const std::string& nymId,
        const std::string& threadId,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool SetUnitDefinitionAlias(
        const std::string& id,
        const std::string& alias) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const std::string& accountID,
        const std::string& data,
        const std::string& alias,
        const identifier::Nym& ownerNym,
        const identifier::Nym& signerNym,
        const identifier::Nym& issuerNym,
        const identifier::Server& server,
        const identifier::UnitDefinition& contract,
        const contact::ContactItemType unit) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const std::string& nymID,
        const contact::ContactItemType type,
        const proto::HDAccount& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const identifier::Nym& nymID,
        const Identifier& channelID,
        const proto::Bip47Channel& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(const proto::Contact& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(const proto::Context& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(const proto::Credential& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::Nym& data,
        const std::string& alias = std::string("")) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const std::string& nymID,
        const proto::Issuer& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const std::string& nymID,
        const proto::PaymentWorkflow& data) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const std::string& nymid,
        const std::string& threadid,
        const std::string& itemid,
        const std::uint64_t time,
        const std::string& alias,
        const std::string& data,
        const StorageBox box,
        const std::string& account = std::string("")) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const identifier::Nym& nym,
        const Identifier& thread,
        const opentxs::blockchain::Type chain,
        const Data& txid,
        const Time time) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::PeerReply& data,
        const std::string& nymid,
        const StorageBox box) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::PeerRequest& data,
        const std::string& nymid,
        const StorageBox box) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const identifier::Nym& nym,
        const proto::Purse& purse) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::Seed& data,
        const std::string& alias = std::string("")) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::ServerContract& data,
        const std::string& alias = std::string("")) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::Ciphertext& serialized) const = 0;
    OPENTXS_EXPORT virtual bool Store(
        const proto::UnitDefinition& data,
        const std::string& alias = std::string("")) const = 0;
    OPENTXS_EXPORT virtual ObjectList ThreadList(
        const std::string& nymID,
        const bool unreadOnly) const = 0;
    OPENTXS_EXPORT virtual std::string ThreadAlias(
        const std::string& nymID,
        const std::string& threadID) const = 0;
    OPENTXS_EXPORT virtual bool UnaffiliatedBlockchainTransaction(
        const identifier::Nym& recipient,
        const Data& txid) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string UnitDefinitionAlias(
        const std::string& id) const = 0;
    OPENTXS_EXPORT virtual ObjectList UnitDefinitionList() const = 0;
    OPENTXS_EXPORT virtual std::size_t UnreadCount(
        const std::string& nymId,
        const std::string& threadId) const = 0;
    OPENTXS_EXPORT virtual void UpgradeNyms() = 0;

    OPENTXS_EXPORT virtual ~Storage() = default;

protected:
    Storage() = default;

private:
    Storage(const Storage&) = delete;
    Storage(Storage&&) = delete;
    Storage& operator=(const Storage&) = delete;
    Storage& operator=(Storage&&) = delete;
};
}  // namespace storage
}  // namespace api
}  // namespace opentxs
#endif

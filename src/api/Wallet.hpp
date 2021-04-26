// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <utility>

#include "internal/identity/Identity.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/blind/CashType.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Request.hpp"
#include "opentxs/network/zeromq/socket/Request.tpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/protobuf/ContactEnums.pb.h"

namespace opentxs
{
namespace api
{
namespace client
{
class Issuer;
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace identity
{
namespace internal
{
struct Nym;
}  // namespace internal

class Nym;
}  // namespace identity

namespace internal
{
struct Context;
}  // namespace internal

namespace proto
{
class Context;
class Credential;
class Issuer;
class Nym;
class PeerReply;
class PeerRequest;
class ServerContract;
class UnitDefinition;
}  // namespace proto

namespace otx
{
namespace context
{
namespace internal
{
struct Base;
}  // namespace internal

class Base;
class Client;
class Server;
}  // namespace context
}  // namespace otx

class Context;
class NymFile;
class NymParameters;
class PasswordPrompt;
class PeerObject;
class String;
}  // namespace opentxs

namespace opentxs::api::implementation
{
class Wallet : virtual public opentxs::api::Wallet, public Lockable
{
public:
    auto Account(const Identifier& accountID) const -> SharedAccount final;
    auto AccountPartialMatch(const std::string& hint) const
        -> OTIdentifier final;
    auto CreateAccount(
        const identifier::Nym& ownerNymID,
        const identifier::Server& notaryID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const identity::Nym& signer,
        Account::AccountType type,
        TransactionNumber stash,
        const PasswordPrompt& reason) const -> ExclusiveAccount final;
    auto DeleteAccount(const Identifier& accountID) const -> bool final;
    auto IssuerAccount(const identifier::UnitDefinition& unitID) const
        -> SharedAccount final;
    auto mutable_Account(
        const Identifier& accountID,
        const PasswordPrompt& reason,
        const AccountCallback callback) const -> ExclusiveAccount final;
    auto UpdateAccount(
        const Identifier& accountID,
        const otx::context::Server&,
        const String& serialized,
        const PasswordPrompt& reason) const -> bool final;
    auto UpdateAccount(
        const Identifier& accountID,
        const otx::context::Server&,
        const String& serialized,
        const std::string& label,
        const PasswordPrompt& reason) const -> bool final;
    auto ImportAccount(std::unique_ptr<opentxs::Account>& imported) const
        -> bool final;
    auto ClientContext(const identifier::Nym& remoteNymID) const
        -> std::shared_ptr<const otx::context::Client> override;
    auto ServerContext(
        const identifier::Nym& localNymID,
        const Identifier& remoteID) const
        -> std::shared_ptr<const otx::context::Server> override;
    auto mutable_ClientContext(
        const identifier::Nym& remoteNymID,
        const PasswordPrompt& reason) const
        -> Editor<otx::context::Client> override;
    auto mutable_ServerContext(
        const identifier::Nym& localNymID,
        const Identifier& remoteID,
        const PasswordPrompt& reason) const
        -> Editor<otx::context::Server> override;
    auto IssuerList(const identifier::Nym& nymID) const
        -> std::set<OTNymID> final;
    auto Issuer(const identifier::Nym& nymID, const identifier::Nym& issuerID)
        const -> std::shared_ptr<const api::client::Issuer> final;
    auto mutable_Issuer(
        const identifier::Nym& nymID,
        const identifier::Nym& issuerID) const
        -> Editor<api::client::Issuer> final;
    auto IsLocalNym(const std::string& id) const -> bool final;
    auto LocalNymCount() const -> std::size_t final;
    auto LocalNyms() const -> std::set<OTNymID> final;
    auto Nym(
        const identifier::Nym& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const -> Nym_p final;
    auto Nym(const proto::Nym& nym) const -> Nym_p final;
    auto Nym(const Space& nym) const -> Nym_p final;
    auto Nym(
        const PasswordPrompt& reason,
        const std::string name,
        const NymParameters& parameters,
        const contact::ContactItemType type) const -> Nym_p final;
    auto mutable_Nym(const identifier::Nym& id, const PasswordPrompt& reason)
        const -> NymData final;
    auto Nymfile(const identifier::Nym& id, const PasswordPrompt& reason) const
        -> std::unique_ptr<const opentxs::NymFile> final;
    auto mutable_Nymfile(
        const identifier::Nym& id,
        const PasswordPrompt& reason) const -> Editor<opentxs::NymFile> final;
    auto NymByIDPartialMatch(const std::string& partialId) const -> Nym_p final;
    auto NymList() const -> ObjectList final;
    auto NymNameByIndex(const std::size_t index, String& name) const
        -> bool final;
    auto PeerReply(
        const identifier::Nym& nym,
        const Identifier& reply,
        const StorageBox& box) const -> std::shared_ptr<proto::PeerReply> final;
    auto PeerReplyComplete(
        const identifier::Nym& nym,
        const Identifier& replyOrRequest) const -> bool final;
    auto PeerReplyCreate(
        const identifier::Nym& nym,
        const proto::PeerRequest& request,
        const proto::PeerReply& reply) const -> bool final;
    auto PeerReplyCreateRollback(
        const identifier::Nym& nym,
        const Identifier& request,
        const Identifier& reply) const -> bool final;
    auto PeerReplySent(const identifier::Nym& nym) const -> ObjectList final;
    auto PeerReplyIncoming(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerReplyFinished(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerReplyProcessed(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerReplyReceive(const identifier::Nym& nym, const PeerObject& reply)
        const -> bool final;
    auto PeerRequest(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box,
        std::time_t& time) const -> std::shared_ptr<proto::PeerRequest> final;
    auto PeerRequestComplete(
        const identifier::Nym& nym,
        const Identifier& reply) const -> bool final;
    auto PeerRequestCreate(
        const identifier::Nym& nym,
        const proto::PeerRequest& request) const -> bool final;
    auto PeerRequestCreateRollback(
        const identifier::Nym& nym,
        const Identifier& request) const -> bool final;
    auto PeerRequestDelete(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box) const -> bool final;
    auto PeerRequestSent(const identifier::Nym& nym) const -> ObjectList final;
    auto PeerRequestIncoming(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerRequestFinished(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerRequestProcessed(const identifier::Nym& nym) const
        -> ObjectList final;
    auto PeerRequestReceive(
        const identifier::Nym& nym,
        const PeerObject& request) const -> bool final;
    auto PeerRequestUpdate(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box) const -> bool final;
#if OT_CASH
    auto Purse(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const bool checking) const -> std::unique_ptr<const blind::Purse> final;
    auto mutable_Purse(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const PasswordPrompt& reason,
        const blind::CashType type) const -> Editor<blind::Purse> final;
#endif
    auto RemoveServer(const identifier::Server& id) const -> bool final;
    auto RemoveUnitDefinition(const identifier::UnitDefinition& id) const
        -> bool final;
    auto Server(
        const identifier::Server& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const -> OTServerContract final;
    auto Server(const proto::ServerContract& contract) const
        -> OTServerContract final;
    auto Server(const Space& contract) const -> OTServerContract final;
    auto Server(
        const std::string& nymid,
        const std::string& name,
        const std::string& terms,
        const std::list<contract::Server::Endpoint>& endpoints,
        const PasswordPrompt& reason,
        const VersionNumber version) const -> OTServerContract final;
    auto ServerList() const -> ObjectList final;
    auto SetNymAlias(const identifier::Nym& id, const std::string& alias) const
        -> bool final;
    auto SetServerAlias(const identifier::Server& id, const std::string& alias)
        const -> bool final;
    auto SetUnitDefinitionAlias(
        const identifier::UnitDefinition& id,
        const std::string& alias) const -> bool final;
    auto UnitDefinitionList() const -> ObjectList final;
    auto UnitDefinition(
        const identifier::UnitDefinition& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const -> OTUnitDefinition final;
    auto BasketContract(
        const identifier::UnitDefinition& id,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(
            0)) const noexcept(false) -> OTBasketContract final;
    auto UnitDefinition(const proto::UnitDefinition& contract) const
        -> OTUnitDefinition final;
    auto UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const std::string& tla,
        const std::uint32_t power,
        const std::string& fraction,
        const contact::ContactItemType unitOfAccount,
        const PasswordPrompt& reason,
        const VersionNumber version = contract::Unit::DefaultVersion) const
        -> OTUnitDefinition final;
    auto UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const contact::ContactItemType unitOfAccount,
        const PasswordPrompt& reason,
        const VersionNumber version = contract::Unit::DefaultVersion) const
        -> OTUnitDefinition final;
    auto CurrencyTypeBasedOnUnitType(
        const identifier::UnitDefinition& contractID) const
        -> contact::ContactItemType final;

    auto LoadCredential(
        const std::string& id,
        std::shared_ptr<proto::Credential>& credential) const -> bool final;
    auto SaveCredential(const proto::Credential& credential) const
        -> bool final;

    ~Wallet() override = default;

protected:
    using AccountLock =
        std::pair<std::shared_mutex, std::unique_ptr<opentxs::Account>>;
    using ContextID = std::pair<std::string, std::string>;
    using ContextMap =
        std::map<ContextID, std::shared_ptr<otx::context::internal::Base>>;

    const api::internal::Core& api_;
    mutable ContextMap context_map_;
    mutable std::mutex context_map_lock_;

    auto context(
        const identifier::Nym& localNymID,
        const identifier::Nym& remoteNymID) const
        -> std::shared_ptr<otx::context::Base>;
    auto extract_unit(const identifier::UnitDefinition& contractID) const
        -> contact::ContactItemType;
    auto extract_unit(const contract::Unit& contract) const
        -> contact::ContactItemType;
    void save(
        const PasswordPrompt& reason,
        otx::context::internal::Base* context) const;
    auto server_to_nym(Identifier& nymOrNotaryID) const -> OTNymID;

    Wallet(const api::internal::Core& core);

private:
    using AccountMap = std::map<OTIdentifier, AccountLock>;
    using NymLock =
        std::pair<std::mutex, std::shared_ptr<identity::internal::Nym>>;
    using NymMap = std::map<std::string, NymLock>;
    using ServerMap = std::map<std::string, std::shared_ptr<contract::Server>>;
    using UnitMap = std::map<std::string, std::shared_ptr<contract::Unit>>;
    using IssuerID = std::pair<OTIdentifier, OTIdentifier>;
    using IssuerLock =
        std::pair<std::mutex, std::shared_ptr<api::client::Issuer>>;
    using IssuerMap = std::map<IssuerID, IssuerLock>;
    using PurseID = std::tuple<OTNymID, OTServerID, OTUnitID>;
    using UnitNameMap = std::map<std::string, proto::ContactItemType>;
    using UnitNameReverse = std::map<proto::ContactItemType, std::string>;

    static const UnitNameMap unit_of_account_;
    static const UnitNameReverse unit_lookup_;

    mutable AccountMap account_map_;
    mutable NymMap nym_map_;
    mutable ServerMap server_map_;
    mutable UnitMap unit_map_;
    mutable IssuerMap issuer_map_;
    mutable std::mutex account_map_lock_;
    mutable std::mutex nym_map_lock_;
    mutable std::mutex server_map_lock_;
    mutable std::mutex unit_map_lock_;
    mutable std::mutex issuer_map_lock_;
    mutable std::mutex peer_map_lock_;
    mutable std::map<std::string, std::mutex> peer_lock_;
    mutable std::mutex nymfile_map_lock_;
    mutable std::map<OTIdentifier, std::mutex> nymfile_lock_;
#if OT_CASH
    mutable std::mutex purse_lock_;
    mutable std::map<PurseID, std::mutex> purse_id_lock_;
#endif
    OTZMQPublishSocket account_publisher_;
    OTZMQPublishSocket issuer_publisher_;
    OTZMQPublishSocket nym_publisher_;
    OTZMQPublishSocket nym_created_publisher_;
    OTZMQPublishSocket server_publisher_;
    OTZMQPublishSocket unit_publisher_;
    OTZMQPublishSocket peer_reply_publisher_;
    OTZMQPublishSocket peer_request_publisher_;
    OTZMQRequestSocket dht_nym_requester_;
    OTZMQRequestSocket dht_server_requester_;
    OTZMQRequestSocket dht_unit_requester_;
    OTZMQPushSocket find_nym_;

    static auto reverse_unit_map(const UnitNameMap& map) -> UnitNameReverse;

    auto account_alias(const std::string& accountID, const std::string& hint)
        const -> std::string;
    auto account_factory(
        const Identifier& accountID,
        const std::string& alias,
        const std::string& serialized) const -> opentxs::Account*;
#if OT_CASH
    auto get_purse_lock(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit) const -> std::mutex&;
#endif
    virtual void instantiate_client_context(
        const proto::Context& serialized,
        const Nym_p& localNym,
        const Nym_p& remoteNym,
        std::shared_ptr<otx::context::internal::Base>& output) const
    {
    }
    virtual void instantiate_server_context(
        const proto::Context& serialized,
        const Nym_p& localNym,
        const Nym_p& remoteNym,
        std::shared_ptr<otx::context::internal::Base>& output) const
    {
    }
    virtual auto load_legacy_account(
        const Identifier& accountID,
        const eLock& lock,
        AccountLock& row) const -> bool
    {
        return false;
    }
    auto mutable_nymfile(
        const Nym_p& targetNym,
        const Nym_p& signerNym,
        const identifier::Nym& id,
        const PasswordPrompt& reason) const -> Editor<opentxs::NymFile>;
    virtual void nym_to_contact(
        [[maybe_unused]] const identity::Nym& nym,
        [[maybe_unused]] const std::string& name) const noexcept
    {
    }
    auto nymfile_lock(const identifier::Nym& nymID) const -> std::mutex&;
    auto peer_lock(const std::string& nymID) const -> std::mutex&;
    auto publish_server(const identifier::Server& id) const noexcept -> void;
    auto publish_unit(const identifier::UnitDefinition& id) const noexcept
        -> void;
#if OT_CASH
    auto purse(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const bool checking) const -> std::unique_ptr<blind::Purse>;
#endif
    void save(
        const PasswordPrompt& reason,
        const std::string id,
        std::unique_ptr<opentxs::Account>& in,
        eLock& lock,
        bool success) const;
    void save(const Lock& lock, api::client::Issuer* in) const;
#if OT_CASH
    void save(const Lock& lock, const OTNymID nym, blind::Purse* in) const;
#endif
    void save(NymData* nymData, const Lock& lock) const;
    void save(
        const PasswordPrompt& reason,
        opentxs::NymFile* nym,
        const Lock& lock) const;
    auto SaveCredentialIDs(const identity::Nym& nym) const -> bool;
    virtual auto signer_nym(const identifier::Nym& id) const -> Nym_p = 0;

    /* Throws std::out_of_range for missing accounts */
    auto account(
        const Lock& lock,
        const Identifier& accountID,
        const bool create) const -> AccountLock&;
    auto issuer(
        const identifier::Nym& nymID,
        const identifier::Nym& issuerID,
        const bool create) const -> IssuerLock&;

    auto server(std::unique_ptr<contract::Server> contract) const
        noexcept(false) -> OTServerContract;
    auto unit_definition(std::shared_ptr<contract::Unit>&& contract) const
        -> OTUnitDefinition;

    Wallet() = delete;
    Wallet(const Wallet&) = delete;
    Wallet(Wallet&&) = delete;
    auto operator=(const Wallet&) -> Wallet& = delete;
    auto operator=(Wallet&&) -> Wallet& = delete;
};
}  // namespace opentxs::api::implementation

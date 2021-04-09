// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "api/Wallet.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <type_traits>

#include "2_Factory.hpp"
#include "Exclusive.tpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Factory.hpp"
#include "internal/core/Core.hpp"
#include "internal/identity/Identity.hpp"
#include "internal/otx/OTX.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Exclusive.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/api/storage/Storage.hpp"
#if OT_CASH
#include "opentxs/blind/Purse.hpp"
#endif
#include "opentxs/client/NymData.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/NymFile.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/core/contract/peer/PeerObject.hpp"
#include "opentxs/core/contract/peer/PeerObjectType.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/otx/ConsensusType.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/Context.pb.h"
#include "opentxs/protobuf/Issuer.pb.h"  // IWYU pragma: keep
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/Purse.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "opentxs/protobuf/verify/Nym.hpp"
#include "opentxs/protobuf/verify/Purse.hpp"
#include "opentxs/util/WorkType.hpp"

template class opentxs::Exclusive<opentxs::Account>;
template class opentxs::Shared<opentxs::Account>;
template class opentxs::Pimpl<opentxs::network::zeromq::Message>;

#define OT_METHOD "opentxs::api::implementation::Wallet::"

namespace opentxs::api::implementation
{
const Wallet::UnitNameMap Wallet::unit_of_account_{
    {"BTC", proto::CITEMTYPE_BTC},   {"ETH", proto::CITEMTYPE_ETH},
    {"XRP", proto::CITEMTYPE_XRP},   {"LTC", proto::CITEMTYPE_LTC},
    {"DAO", proto::CITEMTYPE_DAO},   {"XEM", proto::CITEMTYPE_XEM},
    {"DAS", proto::CITEMTYPE_DASH},  {"LSK", proto::CITEMTYPE_LSK},
    {"DGD", proto::CITEMTYPE_DGD},   {"XMR", proto::CITEMTYPE_XMR},
    {"NXT", proto::CITEMTYPE_NXT},   {"AMP", proto::CITEMTYPE_AMP},
    {"XLM", proto::CITEMTYPE_XLM},   {"FCT", proto::CITEMTYPE_FCT},
    {"BTS", proto::CITEMTYPE_BTS},   {"USD", proto::CITEMTYPE_USD},
    {"EUR", proto::CITEMTYPE_EUR},   {"GBP", proto::CITEMTYPE_GBP},
    {"INR", proto::CITEMTYPE_INR},   {"AUD", proto::CITEMTYPE_AUD},
    {"CAD", proto::CITEMTYPE_CAD},   {"SGD", proto::CITEMTYPE_SGD},
    {"CHF", proto::CITEMTYPE_CHF},   {"MYR", proto::CITEMTYPE_MYR},
    {"JPY", proto::CITEMTYPE_JPY},   {"CNY", proto::CITEMTYPE_CNY},
    {"NZD", proto::CITEMTYPE_NZD},   {"THB", proto::CITEMTYPE_THB},
    {"HUF", proto::CITEMTYPE_HUF},   {"AED", proto::CITEMTYPE_AED},
    {"HKD", proto::CITEMTYPE_HKD},   {"MXN", proto::CITEMTYPE_MXN},
    {"ZAR", proto::CITEMTYPE_ZAR},   {"PHP", proto::CITEMTYPE_PHP},
    {"SEK", proto::CITEMTYPE_SEK},   {"BTT", proto::CITEMTYPE_TNBTC},
    {"LTT", proto::CITEMTYPE_TNLTC}, {"DAT", proto::CITEMTYPE_TNDASH},
    {"BCH", proto::CITEMTYPE_BCH},   {"BCT", proto::CITEMTYPE_TNBCH},
    {"PKT", proto::CITEMTYPE_PKT},   {"PTT", proto::CITEMTYPE_TNPKT},
};
const Wallet::UnitNameReverse Wallet::unit_lookup_{
    reverse_unit_map(unit_of_account_)};

Wallet::Wallet(const api::internal::Core& core)
    : api_(core)
    , context_map_()
    , context_map_lock_()
    , account_map_()
    , nym_map_()
    , server_map_()
    , unit_map_()
    , issuer_map_()
    , account_map_lock_()
    , nym_map_lock_()
    , server_map_lock_()
    , unit_map_lock_()
    , issuer_map_lock_()
    , peer_map_lock_()
    , peer_lock_()
    , nymfile_map_lock_()
    , nymfile_lock_()
#if OT_CASH
    , purse_lock_()
    , purse_id_lock_()
#endif
    , account_publisher_(api_.ZeroMQ().PublishSocket())
    , issuer_publisher_(api_.ZeroMQ().PublishSocket())
    , nym_publisher_(api_.ZeroMQ().PublishSocket())
    , nym_created_publisher_(api_.ZeroMQ().PublishSocket())
    , server_publisher_(api_.ZeroMQ().PublishSocket())
    , unit_publisher_(api_.ZeroMQ().PublishSocket())
    , peer_reply_publisher_(api_.ZeroMQ().PublishSocket())
    , peer_request_publisher_(api_.ZeroMQ().PublishSocket())
    , dht_nym_requester_{api_.ZeroMQ().RequestSocket()}
    , dht_server_requester_{api_.ZeroMQ().RequestSocket()}
    , dht_unit_requester_{api_.ZeroMQ().RequestSocket()}
    , find_nym_(api_.ZeroMQ().PushSocket(
          opentxs::network::zeromq::socket::Socket::Direction::Connect))
{
    account_publisher_->Start(api_.Endpoints().AccountUpdate());
    issuer_publisher_->Start(api_.Endpoints().IssuerUpdate());
    nym_publisher_->Start(api_.Endpoints().NymDownload());
    nym_created_publisher_->Start(api_.Endpoints().NymCreated());
    server_publisher_->Start(api_.Endpoints().ServerUpdate());
    unit_publisher_->Start(api_.Endpoints().UnitUpdate());
    peer_reply_publisher_->Start(api_.Endpoints().PeerReplyUpdate());
    peer_request_publisher_->Start(api_.Endpoints().PeerRequestUpdate());
    dht_nym_requester_->Start(api_.Endpoints().DhtRequestNym());
    dht_server_requester_->Start(api_.Endpoints().DhtRequestServer());
    dht_unit_requester_->Start(api_.Endpoints().DhtRequestUnit());
    find_nym_->Start(api_.Endpoints().FindNym());
}

auto Wallet::account(
    const Lock& lock,
    const Identifier& account,
    const bool create) const -> Wallet::AccountLock&
{
    OT_ASSERT(CheckLock(lock, account_map_lock_))

    auto& row = account_map_[account];
    auto& [rowMutex, pAccount] = row;

    if (pAccount) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Account ")(account)(
            " already exists in map.")
            .Flush();

        return row;
    }

    eLock rowLock(rowMutex);
    // What if more than one thread tries to create the same row at the same
    // time? One thread will construct the Account object and the other(s) will
    // block until the lock is obtained. Therefore this check is necessary to
    // avoid creating the same account twice.
    if (pAccount) { return row; }

    std::string serialized{""};
    std::string alias{""};
    const auto loaded =
        api_.Storage().Load(account.str(), serialized, alias, true);

    if (loaded) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Account ")(account)(
            " loaded from storage.")
            .Flush();
        pAccount.reset(account_factory(account, alias, serialized));

        OT_ASSERT(pAccount);
    } else {
        if (false == create) {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Trying to load account ")(
                account)(" via legacy method.")
                .Flush();
            const auto legacy = load_legacy_account(account, rowLock, row);

            if (legacy) { return row; }

            throw std::out_of_range("Unable to load account from storage");
        }
    }

    return row;
}

auto Wallet::Account(const Identifier& accountID) const -> SharedAccount
{
    Lock mapLock(account_map_lock_);

    try {
        auto& row = account(mapLock, accountID, false);
        // WTF clang? This is perfectly valid c++17. Fix your shit.
        // auto& [rowMutex, pAccount] = row;
        auto& rowMutex = std::get<0>(row);
        auto& pAccount = std::get<1>(row);

        if (pAccount) { return SharedAccount(pAccount.get(), rowMutex); }
    } catch (...) {

        return {};
    }

    return {};
}

auto Wallet::account_alias(
    const std::string& accountID,
    const std::string& hint) const -> std::string
{
    if (false == hint.empty()) { return hint; }

    return api_.Storage().AccountAlias(Identifier::Factory(accountID));
}

auto Wallet::account_factory(
    const Identifier& accountID,
    const std::string& alias,
    const std::string& serialized) const -> opentxs::Account*
{
    auto strContract = String::Factory(), strFirstLine = String::Factory();
    const bool bProcessed = Contract::DearmorAndTrim(
        String::Factory(serialized.c_str()), strContract, strFirstLine);

    if (false == bProcessed) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to dearmor serialized account.")
            .Flush();

        return nullptr;
    }

    const auto owner = api_.Storage().AccountOwner(accountID);
    const auto notary = api_.Storage().AccountServer(accountID);

    std::unique_ptr<opentxs::Account> pAccount{
        new opentxs::Account{api_, owner, accountID, notary}};

    if (false == bool(pAccount)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create account.")
            .Flush();

        return nullptr;
    }

    auto& account = *pAccount;

    if (account.GetNymID() != owner) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym id (")(account.GetNymID())(
            ") does not match expect value (")(owner)(")")
            .Flush();
        account.SetNymID(owner);
    }

    if (account.GetRealAccountID() != accountID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Account id (")(
            account.GetRealAccountID())(") does not match expect value (")(
            accountID)(")")
            .Flush();
        account.SetRealAccountID(accountID);
    }

    if (account.GetPurportedAccountID() != accountID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Purported account id (")(
            account.GetPurportedAccountID())(") does not match expect value (")(
            accountID)(")")
            .Flush();
        account.SetPurportedAccountID(accountID);
    }

    if (account.GetRealNotaryID() != notary) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Notary id (")(
            account.GetRealNotaryID())(") does not match expect value (")(
            notary)(")")
            .Flush();
        account.SetRealNotaryID(notary);
    }

    if (account.GetPurportedNotaryID() != notary) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Purported notary id (")(
            account.GetPurportedNotaryID())(") does not match expect value (")(
            notary)(")")
            .Flush();
        account.SetPurportedNotaryID(notary);
    }

    account.SetLoadInsecure();
    auto deserialized = account.LoadContractFromString(strContract);

    if (false == deserialized) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to deserialize account.")
            .Flush();

        return nullptr;
    }

    const auto signerID = api_.Storage().AccountSigner(accountID);

    if (signerID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unknown signer nym.").Flush();

        return nullptr;
    }

    const auto signerNym = Nym(signerID);

    if (false == bool(signerNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load signer nym.")
            .Flush();

        return nullptr;
    }

    if (false == account.VerifySignature(*signerNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature.").Flush();

        return nullptr;
    }

    account.SetAlias(alias.c_str());

    return pAccount.release();
}

auto Wallet::AccountPartialMatch(const std::string& hint) const -> OTIdentifier
{
    const auto list = api_.Storage().AccountList();

    for (const auto& [id, alias] : list) {
        if (0 == id.compare(0, hint.size(), hint)) {

            return Identifier::Factory(id);
        }

        if (0 == alias.compare(0, hint.size(), hint)) {

            return Identifier::Factory(alias);
        }
    }

    return Identifier::Factory();
}

auto Wallet::BasketContract(
    const identifier::UnitDefinition& id,
    const std::chrono::milliseconds& timeout) const noexcept(false)
    -> OTBasketContract
{
    UnitDefinition(id, timeout);

    Lock mapLock(unit_map_lock_);
    auto it = unit_map_.find(id.str());

    if (unit_map_.end() == it) {
        throw std::runtime_error("Basket contract ID not found");
    }

    auto output = std::dynamic_pointer_cast<contract::unit::Basket>(it->second);

    if (output) {

        return OTBasketContract{std::move(output)};
    } else {
        throw std::runtime_error("Unit definition is not a basket contract");
    }
}

auto Wallet::CreateAccount(
    const identifier::Nym& ownerNymID,
    const identifier::Server& notaryID,
    const identifier::UnitDefinition& instrumentDefinitionID,
    const identity::Nym& signer,
    Account::AccountType type,
    TransactionNumber stash,
    const PasswordPrompt& reason) const -> ExclusiveAccount
{
    Lock mapLock(account_map_lock_);

    try {
        const auto contract = UnitDefinition(instrumentDefinitionID);
        std::unique_ptr<opentxs::Account> newAccount(
            opentxs::Account::GenerateNewAccount(
                api_,
                signer.ID(),
                notaryID,
                signer,
                ownerNymID,
                instrumentDefinitionID,
                reason,
                type,
                stash));

        OT_ASSERT(newAccount)

        const auto& accountID = newAccount->GetRealAccountID();
        auto& row = account(mapLock, accountID, true);
        // WTF clang? This is perfectly valid c++17. Fix your shit.
        // auto& [rowMutex, pAccount] = row;
        auto& rowMutex = std::get<0>(row);
        auto& pAccount = std::get<1>(row);

        if (pAccount) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Account already exists.")
                .Flush();

            return {};
        } else {
            pAccount.reset(newAccount.release());

            OT_ASSERT(pAccount)

            const auto id = accountID.str();
            pAccount->SetNymID(ownerNymID);
            pAccount->SetPurportedAccountID(accountID);
            pAccount->SetRealNotaryID(notaryID);
            pAccount->SetPurportedNotaryID(notaryID);
            auto serialized = String::Factory();
            pAccount->SaveContractRaw(serialized);
            const auto saved = api_.Storage().Store(
                id,
                serialized->Get(),
                "",
                ownerNymID,
                signer.ID(),
                contract->Nym()->ID(),
                notaryID,
                instrumentDefinitionID,
                extract_unit(instrumentDefinitionID));

            OT_ASSERT(saved)

            std::function<void(
                std::unique_ptr<opentxs::Account>&, eLock&, bool)>
                callback = [this, id, &reason](
                               std::unique_ptr<opentxs::Account>& in,
                               eLock& lock,
                               bool success) -> void {
                this->save(reason, id, in, lock, success);
            };

            return ExclusiveAccount(&pAccount, rowMutex, callback);
        }
    } catch (...) {

        return {};
    }
}

auto Wallet::DeleteAccount(const Identifier& accountID) const -> bool
{
    Lock mapLock(account_map_lock_);

    try {
        auto& row = account(mapLock, accountID, false);
        // WTF clang? This is perfectly valid c++17. Fix your shit.
        // auto& [rowMutex, pAccount] = row;
        auto& rowMutex = std::get<0>(row);
        auto& pAccount = std::get<1>(row);
        eLock lock(rowMutex);

        if (pAccount) {
            const auto deleted = api_.Storage().DeleteAccount(accountID.str());

            if (deleted) {
                pAccount.reset();

                return true;
            }
        }
    } catch (...) {

        return false;
    }

    return false;
}

auto Wallet::IssuerAccount(const identifier::UnitDefinition& unitID) const
    -> SharedAccount
{
    const auto accounts = api_.Storage().AccountsByContract(unitID);
    Lock mapLock(account_map_lock_);

    try {
        for (const auto& accountID : accounts) {
            auto& row = account(mapLock, accountID, false);
            // WTF clang? This is perfectly valid c++17. Fix your shit.
            // auto& [rowMutex, pAccount] = row;
            auto& rowMutex = std::get<0>(row);
            auto& pAccount = std::get<1>(row);

            if (pAccount) {
                if (pAccount->IsIssuer()) {

                    return SharedAccount(pAccount.get(), rowMutex);
                }
            }
        }
    } catch (...) {

        return {};
    }

    return {};
}

auto Wallet::mutable_Account(
    const Identifier& accountID,
    const PasswordPrompt& reason,
    const AccountCallback callback) const -> ExclusiveAccount
{
    Lock mapLock(account_map_lock_);

    try {
        auto& [rowMutex, pAccount] = account(mapLock, accountID, false);
        const auto id = accountID.str();

        if (pAccount) {
            std::function<void(
                std::unique_ptr<opentxs::Account>&, eLock&, bool)>
                save = [this, id, &reason](
                           std::unique_ptr<opentxs::Account>& in,
                           eLock& lock,
                           bool success) -> void {
                this->save(reason, id, in, lock, success);
            };

            return ExclusiveAccount(&pAccount, rowMutex, save, callback);
        }
    } catch (...) {

        return {};
    }

    return {};
}

auto Wallet::UpdateAccount(
    const Identifier& accountID,
    const otx::context::Server& context,
    const String& serialized,
    const PasswordPrompt& reason) const -> bool
{
    return UpdateAccount(accountID, context, serialized, "", reason);
}

auto Wallet::UpdateAccount(
    const Identifier& accountID,
    const otx::context::Server& context,
    const String& serialized,
    const std::string& label,
    const PasswordPrompt& reason) const -> bool
{
    Lock mapLock(account_map_lock_);
    auto& row = account(mapLock, accountID, true);
    // WTF clang? This is perfectly valid c++17. Fix your shit.
    // auto& [rowMutex, pAccount] = row;
    auto& rowMutex = std::get<0>(row);
    auto& pAccount = std::get<1>(row);
    eLock rowLock(rowMutex);
    mapLock.unlock();
    const auto& localNym = *context.Nym();
    std::unique_ptr<opentxs::Account> newAccount{nullptr};
    newAccount.reset(
        new opentxs::Account(api_, localNym.ID(), accountID, context.Notary()));

    if (false == bool(newAccount)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to construct account.")
            .Flush();

        return false;
    }

    if (false == newAccount->LoadContractFromString(serialized)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to deserialize account.")
            .Flush();

        return false;
    }

    if (false == newAccount->VerifyAccount(context.RemoteNym())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to verify account.")
            .Flush();

        return false;
    }

    if (localNym.ID() != newAccount->GetNymID()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong nym on account.").Flush();

        return false;
    }

    if (context.Notary() != newAccount->GetRealNotaryID()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong server on account.")
            .Flush();

        return false;
    }

    newAccount->ReleaseSignatures();

    if (false == newAccount->SignContract(localNym, reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to sign account.").Flush();

        return false;
    }

    if (false == newAccount->SaveContract()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to serialize account.")
            .Flush();

        return false;
    }

    pAccount.reset(newAccount.release());

    OT_ASSERT(pAccount)

    const auto& unitID = pAccount->GetInstrumentDefinitionID();

    try {
        const auto contract = UnitDefinition(unitID);
        auto raw = String::Factory();
        auto saved = pAccount->SaveContractRaw(raw);

        if (false == saved) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unable to serialized account.")
                .Flush();

            return false;
        }

        const auto alias = account_alias(accountID.str(), label);
        saved = api_.Storage().Store(
            accountID.str(),
            raw->Get(),
            alias,
            localNym.ID(),
            localNym.ID(),
            contract->Nym()->ID(),
            context.Notary(),
            unitID,
            extract_unit(contract));

        if (false == saved) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to save account.")
                .Flush();

            return false;
        }

        pAccount->SetAlias(alias);
        const auto balance = pAccount->GetBalance();

        {
            auto work = api_.ZeroMQ().TaggedMessage(WorkType::AccountUpdated);
            work->AddFrame(accountID);
            work->AddFrame(balance);
            account_publisher_->Send(work);
        }

        return true;
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load unit definition contract ")(unitID)
            .Flush();

        return false;
    }
}

auto Wallet::CurrencyTypeBasedOnUnitType(
    const identifier::UnitDefinition& contractID) const
    -> proto::ContactItemType
{
    return extract_unit(contractID);
}

auto Wallet::extract_unit(const identifier::UnitDefinition& contractID) const
    -> proto::ContactItemType
{
    try {
        const auto contract = UnitDefinition(contractID);

        return extract_unit(contract);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to load unit definition contract ")(contractID)(".")
            .Flush();

        return proto::CITEMTYPE_UNKNOWN;
    }
}

auto Wallet::extract_unit(const contract::Unit& contract) const
    -> proto::ContactItemType
{
    try {
        if (contract.Version() < 2) {
            return unit_of_account_.at(contract.TLA());
        }

        return contract.UnitOfAccount();
    } catch (...) {

        return proto::CITEMTYPE_UNKNOWN;
    }
}

#if OT_CASH
auto Wallet::get_purse_lock(
    const identifier::Nym& nym,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit) const -> std::mutex&
{
    Lock lock(purse_lock_);
    const PurseID id{nym, server, unit};

    return purse_id_lock_[id];
}
#endif

auto Wallet::context(
    const identifier::Nym& localNymID,
    const identifier::Nym& remoteNymID) const
    -> std::shared_ptr<otx::context::Base>
{
    const std::string local = localNymID.str();
    const std::string remote = remoteNymID.str();
    const ContextID context = {local, remote};
    auto it = context_map_.find(context);
    const bool inMap = (it != context_map_.end());

    if (inMap) { return it->second; }

    // Load from storage, if it exists.
    std::shared_ptr<proto::Context> serialized;
    const bool loaded = api_.Storage().Load(
        localNymID.str(), remoteNymID.str(), serialized, true);

    if (!loaded) { return nullptr; }

    if (local != serialized->localnym()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect localnym in protobuf.")
            .Flush();

        return nullptr;
    }

    if (remote != serialized->remotenym()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect localnym in protobuf.")
            .Flush();

        return nullptr;
    }

    auto& entry = context_map_[context];

    // Obtain nyms.
    const auto localNym = Nym(localNymID);
    const auto remoteNym = Nym(remoteNymID);

    if (!localNym) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load local nym.")
            .Flush();

        return nullptr;
    }

    if (!remoteNym) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to load remote nym.")
            .Flush();

        return nullptr;
    }

    switch (otx::internal::translate(serialized->type())) {
        case otx::ConsensusType::Server: {
            instantiate_server_context(*serialized, localNym, remoteNym, entry);
        } break;
        case otx::ConsensusType::Client: {
            instantiate_client_context(*serialized, localNym, remoteNym, entry);
        } break;
        default: {
            return nullptr;
        }
    }

    OT_ASSERT(entry);

    const bool valid = entry->Validate();

    if (!valid) {
        context_map_.erase(context);

        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature on context.")
            .Flush();

        OT_FAIL
    }

    return entry;
}

auto Wallet::ClientContext(const identifier::Nym& remoteNymID) const
    -> std::shared_ptr<const otx::context::Client>
{
    // Overridden in appropriate child class.
    OT_FAIL;
}

auto Wallet::ServerContext(
    const identifier::Nym& localNymID,
    const Identifier& remoteID) const
    -> std::shared_ptr<const otx::context::Server>
{
    // Overridden in appropriate child class.
    OT_FAIL;
}

auto Wallet::mutable_ClientContext(
    const identifier::Nym& remoteNymID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Client>
{
    // Overridden in appropriate child class.
    OT_FAIL;
}

auto Wallet::mutable_ServerContext(
    const identifier::Nym& localNymID,
    const Identifier& remoteID,
    const PasswordPrompt& reason) const -> Editor<otx::context::Server>
{
    // Overridden in appropriate child class.
    OT_FAIL;
}

auto Wallet::ImportAccount(std::unique_ptr<opentxs::Account>& imported) const
    -> bool
{
    if (false == bool(imported)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return false;
    }

    const auto& accountID = imported->GetRealAccountID();
    Lock mapLock(account_map_lock_);

    try {
        auto& row = account(mapLock, accountID, true);
        // WTF clang? This is perfectly valid c++17. Fix your shit.
        // auto& [rowMutex, pAccount] = row;
        auto& rowMutex = std::get<0>(row);
        auto& pAccount = std::get<1>(row);
        eLock rowLock(rowMutex);
        mapLock.unlock();

        if (pAccount) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Account already exists.")
                .Flush();

            return false;
        }

        pAccount.reset(imported.release());

        OT_ASSERT(pAccount)

        const auto& contractID = pAccount->GetInstrumentDefinitionID();

        try {
            const auto contract = UnitDefinition(contractID);
            auto serialized = String::Factory();
            auto alias = String::Factory();
            pAccount->SaveContractRaw(serialized);
            pAccount->GetName(alias);
            const auto saved = api_.Storage().Store(
                accountID.str(),
                serialized->Get(),
                alias->Get(),
                pAccount->GetNymID(),
                pAccount->GetNymID(),
                contract->Nym()->ID(),
                pAccount->GetRealNotaryID(),
                contractID,
                extract_unit(contract));

            if (false == saved) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save account.")
                    .Flush();
                imported.reset(pAccount.release());

                return false;
            }

            return true;
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unable to load unit definition.")
                .Flush();
            imported.reset(pAccount.release());

            return false;
        }
    } catch (...) {
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to import account.").Flush();

    return false;
}

auto Wallet::IssuerList(const identifier::Nym& nymID) const -> std::set<OTNymID>
{
    std::set<OTNymID> output{};
    auto list = api_.Storage().IssuerList(nymID.str());

    for (const auto& it : list) {
        output.emplace(identifier::Nym::Factory(it.first));
    }

    return output;
}

auto Wallet::Issuer(
    const identifier::Nym& nymID,
    const identifier::Nym& issuerID) const
    -> std::shared_ptr<const api::client::Issuer>
{
    auto& [lock, pIssuer] = issuer(nymID, issuerID, false);
    const auto& notUsed [[maybe_unused]] = lock;

    return pIssuer;
}

auto Wallet::mutable_Issuer(
    const identifier::Nym& nymID,
    const identifier::Nym& issuerID) const -> Editor<api::client::Issuer>
{
    auto& [lock, pIssuer] = issuer(nymID, issuerID, true);

    OT_ASSERT(pIssuer);

    std::function<void(api::client::Issuer*, const Lock&)> callback =
        [=](api::client::Issuer* in, const Lock& lock) -> void {
        this->save(lock, in);
    };

    return Editor<api::client::Issuer>(lock, pIssuer.get(), callback);
}

auto Wallet::issuer(
    const identifier::Nym& nymID,
    const identifier::Nym& issuerID,
    const bool create) const -> Wallet::IssuerLock&
{
    Lock lock(issuer_map_lock_);
    auto& output = issuer_map_[{nymID, issuerID}];
    auto& [issuerMutex, pIssuer] = output;
    const auto& notUsed [[maybe_unused]] = issuerMutex;

    if (pIssuer) { return output; }

    std::shared_ptr<proto::Issuer> serialized{nullptr};
    const bool loaded =
        api_.Storage().Load(nymID.str(), issuerID.str(), serialized, true);

    if (loaded) {
        OT_ASSERT(serialized)

        pIssuer.reset(factory::Issuer(*this, nymID, *serialized));

        OT_ASSERT(pIssuer)

        return output;
    }

    if (create) {
        pIssuer.reset(factory::Issuer(*this, nymID, issuerID));

        OT_ASSERT(pIssuer);

        save(lock, pIssuer.get());
    }

    return output;
}

auto Wallet::IsLocalNym(const std::string& id) const -> bool
{
    return api_.Storage().LocalNyms().count(id);
}

auto Wallet::LocalNymCount() const -> std::size_t
{
    return api_.Storage().LocalNyms().size();
}

auto Wallet::LocalNyms() const -> std::set<OTNymID>
{
    const std::set<std::string> ids = api_.Storage().LocalNyms();

    std::set<OTNymID> nymIds;
    std::transform(
        ids.begin(),
        ids.end(),
        std::inserter(nymIds, nymIds.end()),
        [](std::string nym) -> OTNymID {
            return identifier::Nym::Factory(nym);
        });

    return nymIds;
}

auto Wallet::Nym(
    const identifier::Nym& id,
    const std::chrono::milliseconds& timeout) const -> Nym_p
{
    const std::string nym = id.str();
    Lock mapLock(nym_map_lock_);
    bool inMap = (nym_map_.find(nym) != nym_map_.end());
    bool valid = false;

    if (!inMap) {
        auto pSerialized = std::shared_ptr<proto::Nym>{};
        auto alias = std::string{};
        bool loaded = api_.Storage().Load(nym, pSerialized, alias, true);

        if (loaded) {
            OT_ASSERT(pSerialized)

            const auto& serialized = *pSerialized;
            auto& pNym = nym_map_[nym].second;
            pNym.reset(opentxs::Factory::Nym(api_, serialized, alias));

            if (pNym && pNym->CompareID(id)) {
                valid = pNym->VerifyPseudonym();
                pNym->SetAliasStartup(alias);
            } else {
                nym_map_.erase(nym);
            }
        } else {
            {
                auto work =
                    api_.ZeroMQ().TaggedMessage(WorkType::DHTRequestNym);
                work->AddFrame(id);
                dht_nym_requester_->Send(work);
            }

            if (timeout > std::chrono::milliseconds(0)) {
                mapLock.unlock();
                auto start = std::chrono::high_resolution_clock::now();
                auto end = start + timeout;
                const auto interval = std::chrono::milliseconds(100);

                while (std::chrono::high_resolution_clock::now() < end) {
                    std::this_thread::sleep_for(interval);
                    mapLock.lock();
                    bool found = (nym_map_.find(nym) != nym_map_.end());
                    mapLock.unlock();

                    if (found) { break; }
                }

                return Nym(id);  // timeout of zero prevents infinite
                                 // recursion
            }
        }
    } else {
        auto& pNym = nym_map_[nym].second;
        if (pNym) { valid = pNym->VerifyPseudonym(); }
    }

    if (valid) { return nym_map_[nym].second; }

    return nullptr;
}

auto Wallet::Nym(const proto::Nym& serialized) const -> Nym_p
{
    const auto& id = serialized.nymid();
    const auto nymID = identifier::Nym::Factory(id);

    if (nymID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid nym ID.").Flush();

        return {};
    }

    auto existing = Nym(nymID);

    if (existing && (existing->Revision() >= serialized.revision())) {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Incoming nym is not newer than existing nym.")
            .Flush();

        return existing;
    } else {
        auto pCandidate = std::unique_ptr<identity::internal::Nym>{
            opentxs::Factory::Nym(api_, serialized, "")};

        if (false == bool(pCandidate)) { return {}; }

        auto& candidate = *pCandidate;

        if (false == candidate.CompareID(nymID)) { return existing; }

        if (candidate.VerifyPseudonym()) {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Saving updated nym ")(id)
                .Flush();
            candidate.WriteCredentials();
            SaveCredentialIDs(candidate);
            Lock mapLock(nym_map_lock_);
            auto& mapNym = nym_map_[id].second;
            // TODO update existing nym rather than destroying it
            mapNym.reset(pCandidate.release());

            {
                auto work = api_.ZeroMQ().TaggedMessage(WorkType::NymUpdated);
                work->AddFrame(id);
                nym_publisher_->Send(work);
            }

            return mapNym;
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incoming nym is not valid.")
                .Flush();
        }
    }

    return existing;
}

auto Wallet::Nym(
    const PasswordPrompt& reason,
    const std::string name,
    const NymParameters& parameters,
    const proto::ContactItemType type) const -> Nym_p
{
    std::shared_ptr<identity::internal::Nym> pNym(
        opentxs::Factory::Nym(api_, parameters, type, name, reason));

    if (false == bool(pNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create nym").Flush();

        return {};
    }

    auto& nym = *pNym;

    if (nym.VerifyPseudonym()) {
        nym.SetAlias(name);

        {
            Lock mapLock(nym_map_lock_);
            auto it = nym_map_.find(nym.ID().str());

            if (nym_map_.end() != it) { return it->second.second; }
        }

        if (SaveCredentialIDs(nym)) {
            nym_to_contact(nym, name);

            {
                auto nymfile = mutable_nymfile(pNym, pNym, nym.ID(), reason);
            }

            Lock mapLock(nym_map_lock_);
            auto& pMapNym = nym_map_[nym.ID().str()].second;
            pMapNym = pNym;

            {
                auto work = api_.ZeroMQ().TaggedMessage(WorkType::NymCreated);
                work->AddFrame(pNym->ID());
                nym_created_publisher_->Send(work);
            }

            return std::move(pNym);
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save credentials")
                .Flush();

            return {};
        }
    } else {

        return {};
    }
}

auto Wallet::mutable_Nym(
    const identifier::Nym& id,
    const PasswordPrompt& reason) const -> NymData
{
    const std::string nym = id.str();
    auto exists = Nym(id);

    if (false == bool(exists)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" not found.")
            .Flush();
    }

    Lock mapLock(nym_map_lock_);
    auto it = nym_map_.find(nym);

    if (nym_map_.end() == it) { OT_FAIL }

    std::function<void(NymData*, Lock&)> callback = [&](NymData* nymData,
                                                        Lock& lock) -> void {
        this->save(nymData, lock);
    };

    return NymData(
        api_.Factory(), it->second.first, it->second.second, callback);
}

auto Wallet::Nymfile(const identifier::Nym& id, const PasswordPrompt& reason)
    const -> std::unique_ptr<const opentxs::NymFile>
{
    Lock lock(nymfile_lock(id));
    const auto targetNym = Nym(id);
    const auto signerNym = signer_nym(id);

    if (false == bool(targetNym)) { return {}; }
    if (false == bool(signerNym)) { return {}; }

    auto nymfile = std::unique_ptr<opentxs::internal::NymFile>(
        opentxs::Factory::NymFile(api_, targetNym, signerNym));

    OT_ASSERT(nymfile)

    if (false == nymfile->LoadSignedNymFile(reason)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failure calling load_signed_nymfile: ")(id)(".")
            .Flush();

        return {};
    }

    return std::move(nymfile);
}

auto Wallet::mutable_Nymfile(
    const identifier::Nym& id,
    const PasswordPrompt& reason) const -> Editor<opentxs::NymFile>
{
    const auto targetNym = Nym(id);
    const auto signerNym = signer_nym(id);

    return mutable_nymfile(targetNym, signerNym, id, reason);
}

auto Wallet::mutable_nymfile(
    const Nym_p& targetNym,
    const Nym_p& signerNym,
    const identifier::Nym& id,
    const PasswordPrompt& reason) const -> Editor<opentxs::NymFile>
{
    auto nymfile = std::unique_ptr<opentxs::internal::NymFile>(
        opentxs::Factory::NymFile(api_, targetNym, signerNym));

    OT_ASSERT(nymfile)

    if (false == nymfile->LoadSignedNymFile(reason)) {
        nymfile->SaveSignedNymFile(reason);
    }

    using EditorType = Editor<opentxs::NymFile>;
    EditorType::LockedSave callback = [&](opentxs::NymFile* in,
                                          Lock& lock) -> void {
        this->save(reason, in, lock);
    };
    EditorType::OptionalCallback deleter = [](const opentxs::NymFile& in) {
        auto* p = &const_cast<opentxs::NymFile&>(in);
        delete p;
    };

    return EditorType(nymfile_lock(id), nymfile.release(), callback, deleter);
}

auto Wallet::nymfile_lock(const identifier::Nym& nymID) const -> std::mutex&
{
    Lock map_lock(nymfile_map_lock_);
    auto& output = nymfile_lock_[Identifier::Factory(nymID)];
    map_lock.unlock();

    return output;
}

auto Wallet::NymByIDPartialMatch(const std::string& partialId) const -> Nym_p
{
    Lock mapLock(nym_map_lock_);
    bool inMap = (nym_map_.find(partialId) != nym_map_.end());
    bool valid = false;

    if (!inMap) {
        for (auto& it : nym_map_) {
            if (it.first.compare(0, partialId.length(), partialId) == 0)
                if (it.second.second->VerifyPseudonym())
                    return it.second.second;
        }
        for (auto& it : nym_map_) {
            if (it.second.second->Alias().compare(
                    0, partialId.length(), partialId) == 0)
                if (it.second.second->VerifyPseudonym())
                    return it.second.second;
        }
    } else {
        auto& pNym = nym_map_[partialId].second;
        if (pNym) { valid = pNym->VerifyPseudonym(); }
    }

    if (valid) { return nym_map_[partialId].second; }

    return nullptr;
}

auto Wallet::NymList() const -> ObjectList { return api_.Storage().NymList(); }

auto Wallet::NymNameByIndex(const std::size_t index, String& name) const -> bool
{
    std::set<std::string> nymNames = api_.Storage().LocalNyms();

    if (index < nymNames.size()) {
        std::size_t idx{0};
        for (auto& nymName : nymNames) {
            if (idx == index) {
                name.Set(String::Factory(nymName));

                return true;
            }

            ++idx;
        }
    }

    return false;
}

auto Wallet::peer_lock(const std::string& nymID) const -> std::mutex&
{
    Lock map_lock(peer_map_lock_);
    auto& output = peer_lock_[nymID];
    map_lock.unlock();

    return output;
}

auto Wallet::PeerReply(
    const identifier::Nym& nym,
    const Identifier& reply,
    const StorageBox& box) const -> std::shared_ptr<proto::PeerReply>
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    std::shared_ptr<proto::PeerReply> output;

    api_.Storage().Load(nymID, reply.str(), box, output, true);

    return output;
}

auto Wallet::PeerReplyComplete(
    const identifier::Nym& nym,
    const Identifier& replyID) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    std::shared_ptr<proto::PeerReply> reply;
    const bool haveReply = api_.Storage().Load(
        nymID, replyID.str(), StorageBox::SENTPEERREPLY, reply, false);

    if (!haveReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Sent reply not found.").Flush();

        return false;
    }

    // This reply may have been loaded by request id.
    const auto& realReplyID = reply->id();

    const bool savedReply =
        api_.Storage().Store(*reply, nymID, StorageBox::FINISHEDPEERREPLY);

    if (!savedReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save finished reply.")
            .Flush();

        return false;
    }

    const bool removedReply = api_.Storage().RemoveNymBoxItem(
        nymID, StorageBox::SENTPEERREPLY, realReplyID);

    if (!removedReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to delete finished reply from sent box.")
            .Flush();
    }

    return removedReply;
}

auto Wallet::PeerReplyCreate(
    const identifier::Nym& nym,
    const proto::PeerRequest& request,
    const proto::PeerReply& reply) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    if (reply.cookie() != request.id()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Reply cookie does not match request id.")
            .Flush();

        return false;
    }

    if (reply.type() != request.type()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Reply type does not match request type.")
            .Flush();

        return false;
    }

    const bool createdReply =
        api_.Storage().Store(reply, nymID, StorageBox::SENTPEERREPLY);

    if (!createdReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save sent reply.")
            .Flush();

        return false;
    }

    const bool processedRequest =
        api_.Storage().Store(request, nymID, StorageBox::PROCESSEDPEERREQUEST);

    if (!processedRequest) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to save processed request.")
            .Flush();

        return false;
    }

    const bool movedRequest = api_.Storage().RemoveNymBoxItem(
        nymID, StorageBox::INCOMINGPEERREQUEST, request.id());

    if (!processedRequest) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to delete processed request from incoming box.")
            .Flush();
    }

    return movedRequest;
}

auto Wallet::PeerReplyCreateRollback(
    const identifier::Nym& nym,
    const Identifier& request,
    const Identifier& reply) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    const std::string requestID = request.str();
    const std::string replyID = reply.str();
    std::shared_ptr<proto::PeerRequest> requestItem;
    bool output = true;
    time_t notUsed = 0;
    const bool loadedRequest = api_.Storage().Load(
        nymID,
        requestID,
        StorageBox::PROCESSEDPEERREQUEST,
        requestItem,
        notUsed);

    if (loadedRequest) {
        const bool requestRolledBack = api_.Storage().Store(
            *requestItem, nymID, StorageBox::INCOMINGPEERREQUEST);

        if (requestRolledBack) {
            const bool purgedRequest = api_.Storage().RemoveNymBoxItem(
                nymID, StorageBox::PROCESSEDPEERREQUEST, requestID);
            if (!purgedRequest) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to delete request from processed box.")
                    .Flush();
                output = false;
            }
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to save request to incoming box.")
                .Flush();
            output = false;
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Did not find the request in the processed box.")
            .Flush();
        output = false;
    }

    const bool removedReply = api_.Storage().RemoveNymBoxItem(
        nymID, StorageBox::SENTPEERREPLY, replyID);

    if (!removedReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to delete reply from sent box.")
            .Flush();
        output = false;
    }

    return output;
}

auto Wallet::PeerReplySent(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(nymID, StorageBox::SENTPEERREPLY);
}

auto Wallet::PeerReplyIncoming(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(nymID, StorageBox::INCOMINGPEERREPLY);
}

auto Wallet::PeerReplyFinished(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(nymID, StorageBox::FINISHEDPEERREPLY);
}

auto Wallet::PeerReplyProcessed(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(nymID, StorageBox::PROCESSEDPEERREPLY);
}

auto Wallet::PeerReplyReceive(
    const identifier::Nym& nym,
    const PeerObject& reply) const -> bool
{
    if (contract::peer::PeerObjectType::Response != reply.Type()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": This is not a peer reply.")
            .Flush();

        return false;
    }

    if (0 == reply.Request()->Version()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null request.").Flush();

        return false;
    }

    if (0 == reply.Reply()->Version()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null reply.").Flush();

        return false;
    }

    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    auto requestID = reply.Request()->ID();

    std::shared_ptr<proto::PeerRequest> request;
    std::time_t notUsed;
    const bool haveRequest = api_.Storage().Load(
        nymID,
        requestID->str(),
        StorageBox::SENTPEERREQUEST,
        request,
        notUsed,
        false);

    if (haveRequest) {
        OT_ASSERT(request);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": The request for this reply does not exist in the sent box.")
            .Flush();

        return false;
    }

    const proto::PeerReply serialized{reply.Reply()->Contract()};
    const bool receivedReply =
        api_.Storage().Store(serialized, nymID, StorageBox::INCOMINGPEERREPLY);

    if (receivedReply) {
        auto message = opentxs::network::zeromq::Message::Factory();
        message->AddFrame();
        message->AddFrame(nymID);
        message->AddFrame(serialized);
        peer_reply_publisher_->Send(message);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save incoming reply.")
            .Flush();

        return false;
    }

    const bool finishedRequest =
        api_.Storage().Store(*request, nymID, StorageBox::FINISHEDPEERREQUEST);

    if (!finishedRequest) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to save request to finished box.")
            .Flush();

        return false;
    }

    const bool removedRequest = api_.Storage().RemoveNymBoxItem(
        nymID, StorageBox::SENTPEERREQUEST, requestID->str());

    if (!finishedRequest) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to delete finished request from sent box.")
            .Flush();
    }

    return removedRequest;
}

auto Wallet::PeerRequest(
    const identifier::Nym& nym,
    const Identifier& request,
    const StorageBox& box,
    std::time_t& time) const -> std::shared_ptr<proto::PeerRequest>
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    std::shared_ptr<proto::PeerRequest> output;

    api_.Storage().Load(nymID, request.str(), box, output, time, true);

    return output;
}

auto Wallet::PeerRequestComplete(
    const identifier::Nym& nym,
    const Identifier& replyID) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    std::shared_ptr<proto::PeerReply> reply;
    const bool haveReply = api_.Storage().Load(
        nymID, replyID.str(), StorageBox::INCOMINGPEERREPLY, reply, false);

    if (!haveReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": The reply does not exist in the incoming box.")
            .Flush();

        return false;
    }

    // This reply may have been loaded by request id.
    const auto& realReplyID = reply->id();

    const bool storedReply =
        api_.Storage().Store(*reply, nymID, StorageBox::PROCESSEDPEERREPLY);

    if (!storedReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to save reply to processed box.")
            .Flush();

        return false;
    }

    const bool removedReply = api_.Storage().RemoveNymBoxItem(
        nymID, StorageBox::INCOMINGPEERREPLY, realReplyID);

    if (!removedReply) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to delete completed reply from incoming box.")
            .Flush();
    }

    return removedReply;
}

auto Wallet::PeerRequestCreate(
    const identifier::Nym& nym,
    const proto::PeerRequest& request) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().Store(
        request, nym.str(), StorageBox::SENTPEERREQUEST);
}

auto Wallet::PeerRequestCreateRollback(
    const identifier::Nym& nym,
    const Identifier& request) const -> bool
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().RemoveNymBoxItem(
        nym.str(), StorageBox::SENTPEERREQUEST, request.str());
}

auto Wallet::PeerRequestDelete(
    const identifier::Nym& nym,
    const Identifier& request,
    const StorageBox& box) const -> bool
{
    switch (box) {
        case StorageBox::SENTPEERREQUEST:
        case StorageBox::INCOMINGPEERREQUEST:
        case StorageBox::FINISHEDPEERREQUEST:
        case StorageBox::PROCESSEDPEERREQUEST: {
            return api_.Storage().RemoveNymBoxItem(
                nym.str(), box, request.str());
        }
        default: {
            return false;
        }
    }
}

auto Wallet::PeerRequestSent(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(nym.str(), StorageBox::SENTPEERREQUEST);
}

auto Wallet::PeerRequestIncoming(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(
        nym.str(), StorageBox::INCOMINGPEERREQUEST);
}

auto Wallet::PeerRequestFinished(const identifier::Nym& nym) const -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(
        nym.str(), StorageBox::FINISHEDPEERREQUEST);
}

auto Wallet::PeerRequestProcessed(const identifier::Nym& nym) const
    -> ObjectList
{
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));

    return api_.Storage().NymBoxList(
        nym.str(), StorageBox::PROCESSEDPEERREQUEST);
}

auto Wallet::PeerRequestReceive(
    const identifier::Nym& nym,
    const PeerObject& request) const -> bool
{
    if (contract::peer::PeerObjectType::Request != request.Type()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": This is not a peer request.")
            .Flush();

        return false;
    }

    if (0 == request.Request()->Version()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null request.").Flush();

        return false;
    }

    const proto::PeerRequest serialized{request.Request()->Contract()};
    const std::string nymID = nym.str();
    Lock lock(peer_lock(nymID));
    const auto saved = api_.Storage().Store(
        serialized, nymID, StorageBox::INCOMINGPEERREQUEST);

    if (saved) {
        auto message = opentxs::network::zeromq::Message::Factory();
        message->AddFrame();
        message->AddFrame(nymID);
        message->AddFrame(serialized);
        peer_request_publisher_->Send(message);
    }

    return saved;
}

auto Wallet::PeerRequestUpdate(
    const identifier::Nym& nym,
    const Identifier& request,
    const StorageBox& box) const -> bool
{
    switch (box) {
        case StorageBox::SENTPEERREQUEST:
        case StorageBox::INCOMINGPEERREQUEST:
        case StorageBox::FINISHEDPEERREQUEST:
        case StorageBox::PROCESSEDPEERREQUEST: {
            return api_.Storage().SetPeerRequestTime(
                nym.str(), request.str(), box);
        }
        default: {
            return false;
        }
    }
}

#if OT_CASH
auto Wallet::purse(
    const identifier::Nym& nym,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit,
    const bool checking) const -> std::unique_ptr<blind::Purse>
{
    auto serialized = std::make_shared<proto::Purse>();
    const auto loaded =
        api_.Storage().Load(nym, server, unit, serialized, checking);

    if (false == loaded) {
        if (false == checking) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Purse does not exist")
                .Flush();
        }

        return {};
    }

    OT_ASSERT(serialized);

    if (false == proto::Validate(*serialized, VERBOSE)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid purse").Flush();

        return {};
    }

    std::unique_ptr<blind::Purse> output{
        opentxs::Factory::Purse(api_, *serialized)};

    if (false == bool(output)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to instantiate purse")
            .Flush();

        return {};
    }

    return output;
}

auto Wallet::Purse(
    const identifier::Nym& nym,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit,
    const bool checking) const -> std::unique_ptr<const blind::Purse>
{
    return purse(nym, server, unit, checking);
}

auto Wallet::mutable_Purse(
    const identifier::Nym& nymID,
    const identifier::Server& server,
    const identifier::UnitDefinition& unit,
    const PasswordPrompt& reason,
    const blind::CashType type) const -> Editor<blind::Purse>
{
    auto pPurse = purse(nymID, server, unit, true);

    if (false == bool(pPurse)) {
        const auto nym = Nym(nymID);

        OT_ASSERT(nym);

        pPurse.reset(
            opentxs::Factory::Purse(api_, *nym, server, unit, type, reason));
    }

    OT_ASSERT(pPurse);

    const OTNymID otNymID{nymID};
    std::function<void(blind::Purse*, const Lock&)> callback =
        [=](blind::Purse* in, const Lock& lock) -> void {
        this->save(lock, otNymID, in);
    };

    return Editor<blind::Purse>(
        get_purse_lock(nymID, server, unit), pPurse.release(), callback);
}
#endif

auto Wallet::RemoveServer(const identifier::Server& id) const -> bool
{
    std::string server(id.str());
    Lock mapLock(server_map_lock_);
    auto deleted = server_map_.erase(server);

    if (0 != deleted) { return api_.Storage().RemoveServer(server); }

    return false;
}

auto Wallet::RemoveUnitDefinition(const identifier::UnitDefinition& id) const
    -> bool
{
    std::string unit(id.str());
    Lock mapLock(unit_map_lock_);
    auto deleted = unit_map_.erase(unit);

    if (0 != deleted) { return api_.Storage().RemoveUnitDefinition(unit); }

    return false;
}

auto Wallet::publish_server(const identifier::Server& id) const noexcept -> void
{
    const auto& socket = server_publisher_.get();
    auto work = socket.Context().TaggedMessage(WorkType::NotaryUpdated);
    work->AddFrame(id);
    socket.Send(work);
}

auto Wallet::publish_unit(const identifier::UnitDefinition& id) const noexcept
    -> void
{
    const auto& socket = unit_publisher_.get();
    auto work = socket.Context().TaggedMessage(WorkType::UnitDefinitionUpdated);
    work->AddFrame(id);
    socket.Send(work);
}

auto Wallet::reverse_unit_map(const UnitNameMap& map) -> Wallet::UnitNameReverse
{
    UnitNameReverse output{};

    for (const auto& [key, value] : map) { output.emplace(value, key); }

    return output;
}

void Wallet::save(
    const PasswordPrompt& reason,
    const std::string id,
    std::unique_ptr<opentxs::Account>& in,
    eLock&,
    bool success) const
{
    OT_ASSERT(in)

    auto& account = *in;
    const auto accountID = Identifier::Factory(id);

    if (false == success) {
        // Reload the last valid state for this Account.
        std::string serialized{""};
        std::string alias{""};
        const auto loaded = api_.Storage().Load(id, serialized, alias, false);

        OT_ASSERT(loaded)

        in.reset(account_factory(accountID, alias, serialized));

        OT_ASSERT(in);

        return;
    }

    const auto signerID = api_.Storage().AccountSigner(accountID);

    OT_ASSERT(false == signerID->empty())

    const auto signerNym = Nym(signerID);

    OT_ASSERT(signerNym)

    account.ReleaseSignatures();
    auto saved = account.SignContract(*signerNym, reason);

    OT_ASSERT(saved)

    saved = account.SaveContract();

    OT_ASSERT(saved)

    auto serialized = String::Factory();
    saved = in->SaveContractRaw(serialized);

    OT_ASSERT(saved)

    const auto contractID = api_.Storage().AccountContract(accountID);

    OT_ASSERT(false == contractID->empty())

    saved = api_.Storage().Store(
        accountID->str(),
        serialized->Get(),
        in->Alias(),
        api_.Storage().AccountOwner(accountID),
        api_.Storage().AccountSigner(accountID),
        api_.Storage().AccountIssuer(accountID),
        api_.Storage().AccountServer(accountID),
        contractID,
        extract_unit(contractID));

    OT_ASSERT(saved)
}

void Wallet::save(
    const PasswordPrompt& reason,
    otx::context::internal::Base* context) const
{
    if (nullptr == context) { return; }

    Lock lock(context->GetLock());
    const bool sig = context->UpdateSignature(lock, reason);

    OT_ASSERT(sig);
    OT_ASSERT(context->ValidateContext(lock));

    api_.Storage().Store(context->GetContract(lock));
}

void Wallet::save(const Lock& lock, api::client::Issuer* in) const
{
    OT_ASSERT(nullptr != in)
    OT_ASSERT(lock.owns_lock())

    const auto& nymID = in->LocalNymID();
    const auto& issuerID = in->IssuerID();
    api_.Storage().Store(nymID.str(), in->Serialize());

    {
        const auto& socket = issuer_publisher_.get();
        auto work = socket.Context().TaggedMessage(WorkType::IssuerUpdated);
        work->AddFrame(nymID);
        work->AddFrame(issuerID);
        socket.Send(work);
    }
}

#if OT_CASH
void Wallet::save(const Lock& lock, const OTNymID nym, blind::Purse* in) const
{
    OT_ASSERT(nullptr != in)
    OT_ASSERT(lock.owns_lock())

    std::unique_ptr<blind::Purse> pPurse{in};

    auto& purse = *pPurse;
    const auto serialized = purse.Serialize();

    OT_ASSERT(proto::Validate(serialized, VERBOSE));

    const auto stored = api_.Storage().Store(nym, serialized);

    OT_ASSERT(stored);
}
#endif

void Wallet::save(NymData* nymData, const Lock& lock) const
{
    OT_ASSERT(nullptr != nymData);
    OT_ASSERT(lock.owns_lock())

    SaveCredentialIDs(nymData->nym());
}

void Wallet::save(
    const PasswordPrompt& reason,
    opentxs::NymFile* nymfile,
    const Lock& lock) const
{
    OT_ASSERT(nullptr != nymfile);
    OT_ASSERT(lock.owns_lock())

    auto* internal = dynamic_cast<opentxs::internal::NymFile*>(nymfile);

    OT_ASSERT(nullptr != internal)

    const auto saved = internal->SaveSignedNymFile(reason);

    OT_ASSERT(saved);
}

auto Wallet::SaveCredentialIDs(const identity::Nym& nym) const -> bool
{
    const auto index = dynamic_cast<const identity::internal::Nym&>(nym)
                           .SerializeCredentialIndex(
                               identity::internal::Nym::Mode::Abbreviated);
    const bool valid = proto::Validate(index, VERBOSE);

    if (!valid) { return false; }

    if (!api_.Storage().Store(index, nym.Alias())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failure trying to store credential list for Nym: ")(nym.ID())
            .Flush();

        return false;
    }

    LogDetail(OT_METHOD)(__FUNCTION__)(": Credentials saved.").Flush();

    return true;
}

auto Wallet::SetNymAlias(const identifier::Nym& id, const std::string& alias)
    const -> bool
{
    Lock mapLock(nym_map_lock_);
    auto& nym = nym_map_[id.str()].second;

    nym->SetAlias(alias);

    return api_.Storage().SetNymAlias(id.str(), alias);
}

auto Wallet::Server(
    const identifier::Server& id,
    const std::chrono::milliseconds& timeout) const -> OTServerContract
{
    const std::string server = id.str();
    Lock mapLock(server_map_lock_);
    bool inMap = (server_map_.find(server) != server_map_.end());
    bool valid = false;

    if (!inMap) {
        std::shared_ptr<proto::ServerContract> serialized;

        std::string alias;
        bool loaded = api_.Storage().Load(server, serialized, alias, true);

        if (loaded) {
            auto nym = Nym(identifier::Nym::Factory(serialized->nymid()));

            if (!nym && serialized->has_publicnym()) {
                nym = Nym(serialized->publicnym());
            }

            if (nym) {
                auto& pServer = server_map_[server];
                pServer =
                    opentxs::Factory::ServerContract(api_, nym, *serialized);

                if (pServer) {
                    valid = true;  // Factory() performs validation
                    pServer->InitAlias(alias);
                } else {
                    server_map_.erase(server);
                }
            }
        } else {
            {
                auto work =
                    api_.ZeroMQ().TaggedMessage(WorkType::DHTRequestServer);
                work->AddFrame(id);
                dht_server_requester_->Send(work);
            }

            if (timeout > std::chrono::milliseconds(0)) {
                mapLock.unlock();
                auto start = std::chrono::high_resolution_clock::now();
                auto end = start + timeout;
                const auto interval = std::chrono::milliseconds(100);

                while (std::chrono::high_resolution_clock::now() < end) {
                    std::this_thread::sleep_for(interval);
                    mapLock.lock();
                    bool found =
                        (server_map_.find(server) != server_map_.end());
                    mapLock.unlock();

                    if (found) { break; }
                }

                return Server(id);  // timeout of zero prevents infinite
                                    // recursion
            }
        }
    } else {
        auto& pServer = server_map_[server];
        if (pServer) { valid = pServer->Validate(); }
    }

    if (valid) { return OTServerContract{server_map_[server]}; }

    throw std::runtime_error("Server contract not found");
}

auto Wallet::server(std::unique_ptr<contract::Server> contract) const
    noexcept(false) -> OTServerContract
{
    if (false == bool(contract)) {
        throw std::runtime_error("Null server contract");
    }

    if (false == contract->Validate()) {
        throw std::runtime_error("Invalid server contract");
    }

    const auto server = contract->ID()->str();
    const auto id = api_.Factory().ServerID(server);
    const auto serverNymName = contract->EffectiveName();

    if (serverNymName != contract->Name()) {
        contract->SetAlias(serverNymName);
    }

    if (api_.Storage().Store(contract->Contract(), contract->Alias())) {
        {
            Lock mapLock(server_map_lock_);
            server_map_[server].reset(contract.release());
        }

        publish_server(id);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save server contract.")
            .Flush();
    }

    return Server(identifier::Server::Factory(server));
}

auto Wallet::Server(const proto::ServerContract& contract) const
    -> OTServerContract
{
    const auto& server = contract.id();
    auto serverID = identifier::Server::Factory(server);

    if (serverID->empty()) {
        throw std::runtime_error("Invalid server contract");
    }

    const auto nymID = identifier::Nym::Factory(contract.nymid());

    if (nymID->empty()) { throw std::runtime_error("Invalid nym ID"); }

    auto nym = Nym(nymID);

    if (false == bool(nym) && contract.has_publicnym()) {
        nym = Nym(contract.publicnym());
    }

    if (nym) {
        auto candidate = std::unique_ptr<contract::Server>{
            opentxs::Factory::ServerContract(api_, nym, contract)};

        if (candidate) {
            if (candidate->Validate()) {
                if (serverID.get() != candidate->ID()) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong contract ID.")
                        .Flush();
                    serverID->Assign(candidate->ID());
                }

                const auto stored = api_.Storage().Store(
                    candidate->Contract(), candidate->EffectiveName());

                if (stored) {
                    {
                        Lock mapLock(server_map_lock_);
                        server_map_[server].reset(candidate.release());
                    }

                    publish_server(serverID);
                }
            }
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid nym.").Flush();
    }

    return Server(serverID);
}

auto Wallet::Server(
    const std::string& nymid,
    const std::string& name,
    const std::string& terms,
    const std::list<contract::Server::Endpoint>& endpoints,
    const opentxs::PasswordPrompt& reason,
    const VersionNumber version) const -> OTServerContract
{
    std::string server;
    auto nym = Nym(identifier::Nym::Factory(nymid));

    if (nym) {
        auto list = std::list<Endpoint>{};
        std::transform(
            std::begin(endpoints),
            std::end(endpoints),
            std::back_inserter(list),
            [](const auto& in) -> Endpoint {
                return {
                    static_cast<int>(std::get<0>(in)),
                    static_cast<int>(std::get<1>(in)),
                    std::get<2>(in),
                    std::get<3>(in),
                    std::get<4>(in)};
            });
        auto pContract =
            std::unique_ptr<contract::Server>{opentxs::Factory::ServerContract(
                api_, nym, list, terms, name, version, reason)};

        if (pContract) {

            return this->server(std::move(pContract));
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error: Failed to create contract.")
                .Flush();
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: Nym does not exist.")
            .Flush();
    }

    return Server(identifier::Server::Factory(server));
}

auto Wallet::ServerList() const -> ObjectList
{
    return api_.Storage().ServerList();
}

auto Wallet::server_to_nym(Identifier& input) const -> OTNymID
{
    auto output = identifier::Nym::Factory();
    auto nym = Nym(identifier::Nym::Factory(input.str()));  // TODO conversion
    const bool inputIsNymID = bool(nym);

    if (inputIsNymID) {
        output->Assign(input);
        const auto list = ServerList();
        std::size_t matches = 0;

        for (const auto& item : list) {
            const auto& serverID = item.first;

            try {
                auto server = Server(identifier::Server::Factory(serverID));

                if (server->Nym()->ID() == input) {
                    matches++;
                    // set input to the notary ID
                    input.Assign(server->ID());
                }
            } catch (...) {
            }
        }

        OT_ASSERT(2 > matches);
    } else {
        try {
            const auto contract = Server(
                identifier::Server::Factory(input.str()));  // TODO conversion
            output->SetString(contract->Contract().nymid());
        } catch (...) {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Non-existent server: ")(input)
                .Flush();
        }
    }

    return output;
}

auto Wallet::SetServerAlias(
    const identifier::Server& id,
    const std::string& alias) const -> bool
{
    const std::string server = id.str();
    const bool saved = api_.Storage().SetServerAlias(server, alias);

    if (saved) {
        {
            Lock mapLock(server_map_lock_);
            server_map_.erase(server);
        }

        publish_server(id);

        return true;
    }

    return false;
}

auto Wallet::SetUnitDefinitionAlias(
    const identifier::UnitDefinition& id,
    const std::string& alias) const -> bool
{
    const std::string unit = id.str();
    const bool saved = api_.Storage().SetUnitDefinitionAlias(unit, alias);

    if (saved) {
        {
            Lock mapLock(unit_map_lock_);
            unit_map_.erase(unit);
        }

        publish_unit(id);

        return true;
    }

    return false;
}

auto Wallet::UnitDefinitionList() const -> ObjectList
{
    return api_.Storage().UnitDefinitionList();
}

auto Wallet::UnitDefinition(
    const identifier::UnitDefinition& id,
    const std::chrono::milliseconds& timeout) const -> OTUnitDefinition
{
    const std::string unit = id.str();
    Lock mapLock(unit_map_lock_);
    bool inMap = (unit_map_.find(unit) != unit_map_.end());
    bool valid = false;

    if (!inMap) {
        std::shared_ptr<proto::UnitDefinition> serialized;

        std::string alias;
        bool loaded = api_.Storage().Load(unit, serialized, alias, true);

        if (loaded) {
            auto nym = Nym(identifier::Nym::Factory(serialized->nymid()));

            if (!nym && serialized->has_publicnym()) {
                nym = Nym(serialized->publicnym());
            }

            if (nym) {
                auto& pUnit = unit_map_[unit];
                pUnit =
                    opentxs::Factory::UnitDefinition(api_, nym, *serialized);

                if (pUnit) {
                    valid = true;  // Factory() performs validation
                    pUnit->InitAlias(alias);
                } else {
                    unit_map_.erase(unit);
                }
            }
        } else {
            {
                auto work =
                    api_.ZeroMQ().TaggedMessage(WorkType::DHTRequestUnit);
                work->AddFrame(id);
                dht_unit_requester_->Send(work);
            }

            if (timeout > std::chrono::milliseconds(0)) {
                mapLock.unlock();
                auto start = std::chrono::high_resolution_clock::now();
                auto end = start + timeout;
                const auto interval = std::chrono::milliseconds(100);

                while (std::chrono::high_resolution_clock::now() < end) {
                    std::this_thread::sleep_for(interval);
                    mapLock.lock();
                    bool found = (unit_map_.find(unit) != unit_map_.end());
                    mapLock.unlock();

                    if (found) { break; }
                }

                return UnitDefinition(id);  // timeout of zero prevents infinite
                                            // recursion
            }
        }
    } else {
        auto& pUnit = unit_map_[unit];
        if (pUnit) { valid = pUnit->Validate(); }
    }

    if (valid) { return OTUnitDefinition{unit_map_[unit]}; }

    throw std::runtime_error("Unit definition does not exist");
}

auto Wallet::unit_definition(std::shared_ptr<contract::Unit>&& contract) const
    -> OTUnitDefinition
{
    if (false == bool(contract)) {
        throw std::runtime_error("Null unit definition contract");
    }

    if (false == contract->Validate()) {
        throw std::runtime_error("Invalid unit definition contract");
    }

    const auto unit = contract->ID()->str();
    const auto id = api_.Factory().UnitID(unit);

    if (api_.Storage().Store(contract->Contract(), contract->Alias())) {
        {
            Lock mapLock(unit_map_lock_);
            auto it = unit_map_.find(unit);

            if (unit_map_.end() == it) {
                unit_map_.emplace(unit, std::move(contract));
            } else {
                it->second = std::move(contract);
            }
        }

        publish_unit(id);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save unit definition")
            .Flush();
    }

    return UnitDefinition(identifier::UnitDefinition::Factory(unit));
}

auto Wallet::UnitDefinition(const proto::UnitDefinition& contract) const
    -> OTUnitDefinition
{
    const auto unit = contract.id();
    auto unitID = api_.Factory().UnitID(unit);
    const auto nymID = api_.Factory().NymID(contract.nymid());

    {
        auto work = api_.ZeroMQ().TaggedMessage(WorkType::OTXSearchNym);
        work->AddFrame(nymID);
        find_nym_->Send(work);
    }

    auto nym = Nym(nymID);

    if (!nym && contract.has_publicnym()) { nym = Nym(contract.publicnym()); }

    if (nym) {
        auto candidate = opentxs::Factory::UnitDefinition(api_, nym, contract);

        if (candidate) {
            if (candidate->Validate()) {
                if (unitID.get() != candidate->ID()) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong contract ID.")
                        .Flush();
                    unitID->Assign(candidate->ID());
                }

                const auto stored = api_.Storage().Store(
                    candidate->Contract(), candidate->Alias());

                if (stored) {
                    {
                        Lock mapLock(unit_map_lock_);
                        auto it = unit_map_.find(unit);

                        if (unit_map_.end() == it) {
                            unit_map_.emplace(unit, std::move(candidate));
                        } else {
                            it->second = std::move(candidate);
                        }
                    }

                    publish_unit(unitID);
                }
            }
        }
    }

    return UnitDefinition(unitID);
}

auto Wallet::UnitDefinition(
    const std::string& nymid,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const std::string& tla,
    const std::uint32_t power,
    const std::string& fraction,
    const proto::ContactItemType unitOfAccount,
    const PasswordPrompt& reason,
    const VersionNumber version) const -> OTUnitDefinition
{
    auto unit = std::string{};
    auto nym = Nym(identifier::Nym::Factory(nymid));

    if (nym) {
        auto contract = opentxs::Factory::CurrencyContract(
            api_,
            nym,
            shortname,
            name,
            symbol,
            terms,
            tla,
            power,
            fraction,
            unitOfAccount,
            version,
            reason);

        if (contract) {

            return unit_definition(std::move(contract));
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error: Failed to create contract.")
                .Flush();
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: Nym does not exist.")
            .Flush();
    }

    return UnitDefinition(identifier::UnitDefinition::Factory(unit));
}

auto Wallet::UnitDefinition(
    const std::string& nymid,
    const std::string& shortname,
    const std::string& name,
    const std::string& symbol,
    const std::string& terms,
    const proto::ContactItemType unitOfAccount,
    const PasswordPrompt& reason,
    const VersionNumber version) const -> OTUnitDefinition
{
    std::string unit;
    auto nym = Nym(identifier::Nym::Factory(nymid));

    if (nym) {
        auto contract = opentxs::Factory::SecurityContract(
            api_,
            nym,
            shortname,
            name,
            symbol,
            terms,
            unitOfAccount,
            version,
            reason);

        if (contract) {

            return unit_definition(std::move(contract));
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error: Failed to create contract.")
                .Flush();
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: Nym does not exist.")
            .Flush();
    }

    return UnitDefinition(identifier::UnitDefinition::Factory(unit));
}

auto Wallet::LoadCredential(
    const std::string& id,
    std::shared_ptr<proto::Credential>& credential) const -> bool
{
    return api_.Storage().Load(id, credential);
}

auto Wallet::SaveCredential(const proto::Credential& credential) const -> bool
{
    return api_.Storage().Store(credential);
}
}  // namespace opentxs::api::implementation

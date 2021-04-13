// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"          // IWYU pragma: associated
#include "1_Internal.hpp"        // IWYU pragma: associated
#include "storage/tree/Nym.hpp"  // IWYU pragma: associated

#include <functional>
#include <type_traits>

#include "internal/contact/Contact.hpp"
#include "opentxs/api/storage/Driver.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/BlockchainAccountData.pb.h"
#include "opentxs/protobuf/BlockchainDeterministicAccountData.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/HDAccount.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/Purse.pb.h"
#include "opentxs/protobuf/StorageBlockchainAccountList.pb.h"
#include "opentxs/protobuf/StorageItemHash.pb.h"
#include "opentxs/protobuf/StorageNym.pb.h"
#include "opentxs/protobuf/StoragePurse.pb.h"
#include "opentxs/protobuf/verify/HDAccount.hpp"
#include "opentxs/protobuf/verify/Nym.hpp"
#include "opentxs/protobuf/verify/Purse.hpp"
#include "opentxs/protobuf/verify/StorageNym.hpp"
#include "storage/Plugin.hpp"
#include "storage/tree/Bip47Channels.hpp"
#include "storage/tree/Contexts.hpp"
#include "storage/tree/Issuers.hpp"
#include "storage/tree/Mailbox.hpp"
#include "storage/tree/Node.hpp"
#include "storage/tree/PaymentWorkflows.hpp"
#include "storage/tree/PeerReplies.hpp"
#include "storage/tree/PeerRequests.hpp"
#include "storage/tree/Thread.hpp"  // IWYU pragma: keep
#include "storage/tree/Threads.hpp"

#define CURRENT_VERSION 9
#define BLOCKCHAIN_INDEX_VERSION 1
#define STORAGE_PURSE_VERSION 1

#define OT_METHOD "opentxs::storage::Nym::"

namespace opentxs::storage
{
template <>
void Nym::_save(
    storage::Threads* input,
    const Lock& lock,
    std::mutex& mutex,
    std::string& root)
{
    OT_ASSERT(mail_inbox_);
    OT_ASSERT(mail_outbox_);

    _save(mail_inbox_.get(), lock, mail_inbox_lock_, mail_inbox_root_);
    _save(mail_outbox_.get(), lock, mail_outbox_lock_, mail_outbox_root_);

    if (nullptr == input) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null target.").Flush();
        OT_FAIL;
    }

    Lock rootLock(mutex);
    root = input->Root();
    rootLock.unlock();

    if (false == save(lock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Save error.").Flush();
        OT_FAIL;
    }
}

Nym::Nym(
    const opentxs::api::storage::Driver& storage,
    const std::string& id,
    const std::string& hash,
    const std::string& alias)
    : Node(storage, hash)
    , alias_(alias)
    , nymid_(id)
    , credentials_(Node::BLANK_HASH)
    , checked_(Flag::Factory(false))
    , private_(Flag::Factory(false))
    , revision_(0)
    , bip47_lock_{}
    , bip47_{nullptr}
    , bip47_root_{Node::BLANK_HASH}
    , sent_request_box_lock_()
    , sent_request_box_(nullptr)
    , sent_peer_request_(Node::BLANK_HASH)
    , incoming_request_box_lock_()
    , incoming_request_box_(nullptr)
    , incoming_peer_request_(Node::BLANK_HASH)
    , sent_reply_box_lock_()
    , sent_reply_box_(nullptr)
    , sent_peer_reply_(Node::BLANK_HASH)
    , incoming_reply_box_lock_()
    , incoming_reply_box_(nullptr)
    , incoming_peer_reply_(Node::BLANK_HASH)
    , finished_request_box_lock_()
    , finished_request_box_(nullptr)
    , finished_peer_request_(Node::BLANK_HASH)
    , finished_reply_box_lock_()
    , finished_reply_box_(nullptr)
    , finished_peer_reply_(Node::BLANK_HASH)
    , processed_request_box_lock_()
    , processed_request_box_(nullptr)
    , processed_peer_request_(Node::BLANK_HASH)
    , processed_reply_box_lock_()
    , processed_reply_box_(nullptr)
    , processed_peer_reply_(Node::BLANK_HASH)
    , mail_inbox_lock_()
    , mail_inbox_(nullptr)
    , mail_inbox_root_(Node::BLANK_HASH)
    , mail_outbox_lock_()
    , mail_outbox_(nullptr)
    , mail_outbox_root_(Node::BLANK_HASH)
    , threads_lock_()
    , threads_(nullptr)
    , threads_root_(Node::BLANK_HASH)
    , contexts_lock_()
    , contexts_(nullptr)
    , contexts_root_(Node::BLANK_HASH)
    , blockchain_lock_()
    , blockchain_account_types_()
    , blockchain_account_index_()
    , blockchain_accounts_()
    , issuers_root_(Node::BLANK_HASH)
    , issuers_lock_()
    , issuers_(nullptr)
    , workflows_root_(Node::BLANK_HASH)
    , workflows_lock_()
    , workflows_(nullptr)
    , purse_id_()
{
    if (check_hash(hash)) {
        init(hash);
    } else {
        blank(CURRENT_VERSION);
    }
}

auto Nym::Alias() const -> std::string { return alias_; }

auto Nym::bip47() const -> storage::Bip47Channels*
{
    return construct<storage::Bip47Channels>(bip47_lock_, bip47_, bip47_root_);
}

auto Nym::Bip47Channels() const -> const storage::Bip47Channels&
{
    return *bip47();
}

auto Nym::BlockchainAccountList(const contact::ContactItemType type) const
    -> std::set<std::string>
{
    Lock lock(blockchain_lock_);

    auto it = blockchain_account_types_.find(type);

    if (blockchain_account_types_.end() == it) { return {}; }

    return it->second;
}

auto Nym::BlockchainAccountType(const std::string& accountID) const
    -> contact::ContactItemType
{
    Lock lock(blockchain_lock_);

    try {

        return blockchain_account_index_.at(accountID);
    } catch (...) {

        return contact::ContactItemType::Error;
    }
}

template <typename T, typename... Args>
auto Nym::construct(
    std::mutex& mutex,
    std::unique_ptr<T>& pointer,
    const std::string& root,
    Args&&... params) const -> T*
{
    Lock lock(mutex);

    if (false == bool(pointer)) {
        pointer.reset(new T(driver_, root, params...));

        if (!pointer) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to instantiate.")
                .Flush();
            OT_FAIL;
        }
    }

    lock.unlock();

    return pointer.get();
}

auto Nym::contexts() const -> storage::Contexts*
{
    return construct<storage::Contexts>(
        contexts_lock_, contexts_, contexts_root_);
}

auto Nym::Contexts() const -> const storage::Contexts& { return *contexts(); }

template <typename T>
auto Nym::editor(std::string& root, std::mutex& mutex, T* (Nym::*get)() const)
    -> Editor<T>
{
    std::function<void(T*, Lock&)> callback = [&](T* in, Lock& lock) -> void {
        this->_save(in, lock, mutex, root);
    };

    return Editor<T>(write_lock_, (this->*get)(), callback);
}

auto Nym::finished_reply_box() const -> PeerReplies*
{
    return construct<storage::PeerReplies>(
        finished_reply_box_lock_, finished_reply_box_, finished_peer_reply_);
}

auto Nym::finished_request_box() const -> PeerRequests*
{
    return construct<storage::PeerRequests>(
        finished_request_box_lock_,
        finished_request_box_,
        finished_peer_request_);
}

auto Nym::FinishedRequestBox() const -> const PeerRequests&
{
    return *finished_request_box();
}

auto Nym::FinishedReplyBox() const -> const PeerReplies&
{
    return *finished_reply_box();
}

auto Nym::incoming_reply_box() const -> PeerReplies*
{
    return construct<storage::PeerReplies>(
        incoming_reply_box_lock_, incoming_reply_box_, incoming_peer_reply_);
}

auto Nym::incoming_request_box() const -> PeerRequests*
{
    return construct<storage::PeerRequests>(
        incoming_request_box_lock_,
        incoming_request_box_,
        incoming_peer_request_);
}

auto Nym::IncomingRequestBox() const -> const PeerRequests&
{
    return *incoming_request_box();
}

auto Nym::IncomingReplyBox() const -> const PeerReplies&
{
    return *incoming_reply_box();
}

void Nym::init(const std::string& hash)
{
    std::shared_ptr<proto::StorageNym> serialized;
    driver_.LoadProto(hash, serialized);

    if (!serialized) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load nym index file.")
            .Flush();
        OT_FAIL;
    }

    init_version(CURRENT_VERSION, *serialized);

    nymid_ = serialized->nymid();
    credentials_ = normalize_hash(serialized->credlist().hash());
    sent_peer_request_ = normalize_hash(serialized->sentpeerrequests().hash());
    incoming_peer_request_ =
        normalize_hash(serialized->incomingpeerrequests().hash());
    sent_peer_reply_ = normalize_hash(serialized->sentpeerreply().hash());
    incoming_peer_reply_ =
        normalize_hash(serialized->incomingpeerreply().hash());
    finished_peer_request_ =
        normalize_hash(serialized->finishedpeerrequest().hash());
    finished_peer_reply_ =
        normalize_hash(serialized->finishedpeerreply().hash());
    processed_peer_request_ =
        normalize_hash(serialized->processedpeerrequest().hash());
    processed_peer_reply_ =
        normalize_hash(serialized->processedpeerreply().hash());

    // Fields added in version 2
    if (serialized->has_mailinbox()) {
        mail_inbox_root_ = normalize_hash(serialized->mailinbox().hash());
    } else {
        mail_inbox_root_ = Node::BLANK_HASH;
    }

    if (serialized->has_mailoutbox()) {
        mail_outbox_root_ = normalize_hash(serialized->mailoutbox().hash());
    } else {
        mail_outbox_root_ = Node::BLANK_HASH;
    }

    if (serialized->has_threads()) {
        threads_root_ = normalize_hash(serialized->threads().hash());
    } else {
        threads_root_ = Node::BLANK_HASH;
    }

    // Fields added in version 3
    if (serialized->has_contexts()) {
        contexts_root_ = normalize_hash(serialized->contexts().hash());
    } else {
        contexts_root_ = Node::BLANK_HASH;
    }

    // Fields added in version 4
    for (const auto& it : serialized->blockchainaccountindex()) {
        const auto& id = it.id();
        auto& accountSet =
            blockchain_account_types_[contact::internal::translate(id)];

        for (const auto& accountID : it.list()) {
            accountSet.emplace(accountID);
            blockchain_account_index_.emplace(
                accountID, contact::internal::translate(id));
        }
    }

    for (const auto& account : serialized->hdaccount()) {
        const auto& id = account.deterministic().common().id();
        blockchain_accounts_.emplace(
            id, std::make_shared<proto::HDAccount>(account));
    }

    // Fields added in version 5
    issuers_root_ = normalize_hash(serialized->issuers());

    // Fields added in version 6
    workflows_root_ = normalize_hash(serialized->paymentworkflow());

    // Fields added in version 7
    bip47_root_ = normalize_hash(serialized->bip47());

    // Fields added in version 8
    for (const auto& purse : serialized->purse()) {
        auto server = identifier::Server::Factory(purse.notary());
        auto unit = identifier::UnitDefinition::Factory(purse.unit());
        const auto& pHash = purse.purse().hash();
        PurseID id{std::move(server), std::move(unit)};
        purse_id_.emplace(std::move(id), pHash);
    }

    // Fields added in version 9
    // NOTE txo field is no longer used
}

auto Nym::issuers() const -> storage::Issuers*
{
    return construct<storage::Issuers>(issuers_lock_, issuers_, issuers_root_);
}

auto Nym::Issuers() const -> const storage::Issuers& { return *issuers(); }

auto Nym::Load(
    const std::string& id,
    std::shared_ptr<proto::HDAccount>& output,
    const bool checking) const -> bool
{
    Lock lock(blockchain_lock_);

    const auto it = blockchain_accounts_.find(id);

    if (blockchain_accounts_.end() == it) {
        if (false == checking) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Account does not exist.")
                .Flush();
        }

        return false;
    }

    output = it->second;

    return bool(output);
}

auto Nym::Load(
    std::shared_ptr<proto::Nym>& output,
    std::string& alias,
    const bool checking) const -> bool
{
    Lock lock(write_lock_);

    if (!check_hash(credentials_)) {
        if (false == checking) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error: nym with id ")(nymid_)(
                " has no credentials.")
                .Flush();
        }

        return false;
    }

    alias = alias_;
    checked_->Set(driver_.LoadProto(credentials_, output, false));

    if (!checked_.get()) { return false; }

    private_->Set(proto::NYM_PRIVATE == output->mode());
    revision_.store(output->revision());

    return true;
}

auto Nym::Load(
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit,
    std::shared_ptr<proto::Purse>& output,
    const bool checking) const -> bool
{
    Lock lock(write_lock_);
    const PurseID id{notary, unit};
    const auto it = purse_id_.find(id);

    if (purse_id_.end() == it) {
        if (false == checking) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Purse not found ").Flush();
        }

        return false;
    }

    const auto& hash = it->second;

    return driver_.LoadProto(hash, output, false);
}

auto Nym::mail_inbox() const -> Mailbox*
{
    return construct<storage::Mailbox>(
        mail_inbox_lock_, mail_inbox_, mail_inbox_root_);
}

auto Nym::mail_outbox() const -> Mailbox*
{
    return construct<storage::Mailbox>(
        mail_outbox_lock_, mail_outbox_, mail_outbox_root_);
}

auto Nym::MailInbox() const -> const Mailbox& { return *mail_inbox(); }

auto Nym::MailOutbox() const -> const Mailbox& { return *mail_outbox(); }

auto Nym::Migrate(const opentxs::api::storage::Driver& to) const -> bool
{
    bool output{true};
    output &= migrate(credentials_, to);
    output &= sent_request_box()->Migrate(to);
    output &= incoming_request_box()->Migrate(to);
    output &= sent_reply_box()->Migrate(to);
    output &= incoming_reply_box()->Migrate(to);
    output &= finished_request_box()->Migrate(to);
    output &= finished_reply_box()->Migrate(to);
    output &= processed_request_box()->Migrate(to);
    output &= processed_reply_box()->Migrate(to);
    output &= mail_inbox()->Migrate(to);
    output &= mail_outbox()->Migrate(to);
    output &= threads()->Migrate(to);
    output &= contexts()->Migrate(to);
    output &= issuers()->Migrate(to);
    output &= workflows()->Migrate(to);
    output &= bip47()->Migrate(to);
    output &= migrate(root_, to);

    return output;
}

auto Nym::mutable_Bip47Channels() -> Editor<storage::Bip47Channels>
{
    return editor<storage::Bip47Channels>(
        bip47_root_, bip47_lock_, &Nym::bip47);
}

auto Nym::mutable_SentRequestBox() -> Editor<PeerRequests>
{
    return editor<storage::PeerRequests>(
        sent_peer_request_, sent_request_box_lock_, &Nym::sent_request_box);
}

auto Nym::mutable_IncomingRequestBox() -> Editor<PeerRequests>
{
    return editor<storage::PeerRequests>(
        incoming_peer_request_,
        incoming_request_box_lock_,
        &Nym::incoming_request_box);
}

auto Nym::mutable_SentReplyBox() -> Editor<PeerReplies>
{
    return editor<storage::PeerReplies>(
        sent_peer_reply_, sent_reply_box_lock_, &Nym::sent_reply_box);
}

auto Nym::mutable_IncomingReplyBox() -> Editor<PeerReplies>
{
    return editor<storage::PeerReplies>(
        incoming_peer_reply_,
        incoming_reply_box_lock_,
        &Nym::incoming_reply_box);
}

auto Nym::mutable_FinishedRequestBox() -> Editor<PeerRequests>
{
    return editor<storage::PeerRequests>(
        finished_peer_request_,
        finished_request_box_lock_,
        &Nym::finished_request_box);
}

auto Nym::mutable_FinishedReplyBox() -> Editor<PeerReplies>
{
    return editor<storage::PeerReplies>(
        finished_peer_reply_,
        finished_reply_box_lock_,
        &Nym::finished_reply_box);
}

auto Nym::mutable_ProcessedRequestBox() -> Editor<PeerRequests>
{
    return editor<storage::PeerRequests>(
        processed_peer_request_,
        processed_request_box_lock_,
        &Nym::processed_request_box);
}

auto Nym::mutable_ProcessedReplyBox() -> Editor<PeerReplies>
{
    return editor<storage::PeerReplies>(
        processed_peer_reply_,
        processed_reply_box_lock_,
        &Nym::processed_reply_box);
}

auto Nym::mutable_MailInbox() -> Editor<Mailbox>
{
    return editor<storage::Mailbox>(
        mail_inbox_root_, mail_inbox_lock_, &Nym::mail_inbox);
}

auto Nym::mutable_MailOutbox() -> Editor<Mailbox>
{
    return editor<storage::Mailbox>(
        mail_outbox_root_, mail_outbox_lock_, &Nym::mail_outbox);
}

auto Nym::mutable_Threads() -> Editor<storage::Threads>
{
    return editor<storage::Threads>(
        threads_root_, threads_lock_, &Nym::threads);
}

auto Nym::mutable_Threads(
    const Data& txid,
    const Identifier& contact,
    const bool add) -> Editor<storage::Threads>
{
    auto* threads = this->threads();

    OT_ASSERT(threads);

    if (add) {
        threads->AddIndex(txid, contact);
    } else {
        threads->RemoveIndex(txid, contact);
    }

    auto cb = [&](storage::Threads* in, Lock& lock) {
        _save(in, lock, threads_lock_, threads_root_);
    };

    return Editor<storage::Threads>(write_lock_, threads, cb);
}

auto Nym::mutable_Contexts() -> Editor<storage::Contexts>
{
    return editor<storage::Contexts>(
        contexts_root_, contexts_lock_, &Nym::contexts);
}

auto Nym::mutable_Issuers() -> Editor<storage::Issuers>
{
    return editor<storage::Issuers>(
        issuers_root_, issuers_lock_, &Nym::issuers);
}

auto Nym::mutable_PaymentWorkflows() -> Editor<storage::PaymentWorkflows>
{
    return editor<storage::PaymentWorkflows>(
        workflows_root_, workflows_lock_, &Nym::workflows);
}

auto Nym::PaymentWorkflows() const -> const storage::PaymentWorkflows&
{
    return *workflows();
}

auto Nym::processed_reply_box() const -> PeerReplies*
{
    return construct<storage::PeerReplies>(
        processed_reply_box_lock_, processed_reply_box_, processed_peer_reply_);
}

auto Nym::processed_request_box() const -> PeerRequests*
{
    return construct<storage::PeerRequests>(
        processed_request_box_lock_,
        processed_request_box_,
        processed_peer_request_);
}

auto Nym::ProcessedRequestBox() const -> const PeerRequests&
{
    return *processed_request_box();
}

auto Nym::ProcessedReplyBox() const -> const PeerReplies&
{
    return *processed_reply_box();
}

auto Nym::save(const Lock& lock) const -> bool
{
    if (!verify_write_lock(lock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Lock failure.").Flush();
        OT_FAIL;
    }

    auto serialized = serialize();

    if (!proto::Validate(serialized, VERBOSE)) { return false; }

    return driver_.StoreProto(serialized, root_);
}

template <typename O>
void Nym::_save(
    O* input,
    const Lock& lock,
    std::mutex& mutex,
    std::string& root)
{
    if (!verify_write_lock(lock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Lock failure.").Flush();
        OT_FAIL;
    }

    if (nullptr == input) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null target.").Flush();
        OT_FAIL;
    }

    Lock rootLock(mutex);
    root = input->Root();
    rootLock.unlock();

    if (false == save(lock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Save error.").Flush();
        OT_FAIL;
    }
}

auto Nym::sent_reply_box() const -> PeerReplies*
{
    return construct<storage::PeerReplies>(
        sent_reply_box_lock_, sent_reply_box_, sent_peer_reply_);
}

auto Nym::sent_request_box() const -> PeerRequests*
{
    return construct<storage::PeerRequests>(
        sent_request_box_lock_, sent_request_box_, sent_peer_request_);
}

auto Nym::SentRequestBox() const -> const PeerRequests&
{
    return *sent_request_box();
}

auto Nym::SentReplyBox() const -> const PeerReplies&
{
    return *sent_reply_box();
}

auto Nym::serialize() const -> proto::StorageNym
{
    proto::StorageNym serialized;
    serialized.set_version(version_);
    serialized.set_nymid(nymid_);

    set_hash(version_, nymid_, credentials_, *serialized.mutable_credlist());
    set_hash(
        version_,
        nymid_,
        sent_peer_request_,
        *serialized.mutable_sentpeerrequests());
    set_hash(
        version_,
        nymid_,
        incoming_peer_request_,
        *serialized.mutable_incomingpeerrequests());
    set_hash(
        version_,
        nymid_,
        sent_peer_reply_,
        *serialized.mutable_sentpeerreply());
    set_hash(
        version_,
        nymid_,
        incoming_peer_reply_,
        *serialized.mutable_incomingpeerreply());
    set_hash(
        version_,
        nymid_,
        finished_peer_request_,
        *serialized.mutable_finishedpeerrequest());
    set_hash(
        version_,
        nymid_,
        finished_peer_reply_,
        *serialized.mutable_finishedpeerreply());
    set_hash(
        version_,
        nymid_,
        processed_peer_request_,
        *serialized.mutable_processedpeerrequest());
    set_hash(
        version_,
        nymid_,
        processed_peer_reply_,
        *serialized.mutable_processedpeerreply());
    set_hash(
        version_, nymid_, mail_inbox_root_, *serialized.mutable_mailinbox());
    set_hash(
        version_, nymid_, mail_outbox_root_, *serialized.mutable_mailoutbox());
    set_hash(version_, nymid_, threads_root_, *serialized.mutable_threads());
    set_hash(version_, nymid_, contexts_root_, *serialized.mutable_contexts());

    for (const auto& it : blockchain_account_types_) {
        const auto& chainType = it.first;
        const auto& accountSet = it.second;
        auto& index = *serialized.add_blockchainaccountindex();
        index.set_version(BLOCKCHAIN_INDEX_VERSION);
        index.set_id(contact::internal::translate(chainType));

        for (const auto& accountID : accountSet) { index.add_list(accountID); }
    }

    for (const auto& it : blockchain_accounts_) {
        OT_ASSERT(it.second);

        const auto& account = *it.second;
        *serialized.add_hdaccount() = account;
    }

    serialized.set_issuers(issuers_root_);
    serialized.set_paymentworkflow(workflows_root_);
    serialized.set_bip47(bip47_root_);

    for (const auto& [key, hash] : purse_id_) {
        const auto& [server, unit] = key;
        auto& purse = *serialized.add_purse();
        purse.set_version(STORAGE_PURSE_VERSION);
        purse.set_notary(server->str());
        purse.set_unit(unit->str());
        set_hash(purse.version(), unit->str(), hash, *purse.mutable_purse());
    }

    return serialized;
}

auto Nym::SetAlias(const std::string& alias) -> bool
{
    Lock lock(write_lock_);

    alias_ = alias;

    return true;
}

auto Nym::Store(
    const contact::ContactItemType type,
    const proto::HDAccount& data) -> bool
{
    const auto& accountID = data.deterministic().common().id();

    if (accountID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account ID.").Flush();

        return false;
    }

    if (false == proto::Validate(data, VERBOSE)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid account.").Flush();

        return false;
    }

    Lock writeLock(write_lock_, std::defer_lock);
    Lock blockchainLock(blockchain_lock_, std::defer_lock);
    std::lock(writeLock, blockchainLock);
    auto accountItem = blockchain_accounts_.find(accountID);

    if (blockchain_accounts_.end() == accountItem) {
        blockchain_accounts_[accountID] =
            std::make_shared<proto::HDAccount>(data);
    } else {
        auto& existing = accountItem->second;

        if (existing->deterministic().common().revision() >
            data.deterministic().common().revision()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Not saving object with older revision.")
                .Flush();
        } else {
            existing = std::make_shared<proto::HDAccount>(data);
        }
    }

    blockchain_account_types_[type].insert(accountID);
    blockchain_account_index_.emplace(accountID, type);
    blockchainLock.unlock();

    return save(writeLock);
}

auto Nym::Store(
    const proto::Nym& data,
    const std::string& alias,
    std::string& plaintext) -> bool
{
    Lock lock(write_lock_);

    const std::uint64_t revision = data.revision();
    bool saveOk = false;
    const bool incomingPublic = (proto::NYM_PUBLIC == data.mode());
    const bool existing = check_hash(credentials_);

    if (existing) {
        if (incomingPublic) {
            if (checked_.get()) {
                saveOk = !private_.get();
            } else {
                std::shared_ptr<proto::Nym> serialized;
                driver_.LoadProto(credentials_, serialized, true);
                saveOk = !private_.get();
            }
        } else {
            saveOk = true;
        }
    } else {
        saveOk = true;
    }

    const bool keyUpgrade = (!incomingPublic) && (!private_.get());
    const bool revisionUpgrade = revision > revision_.load();
    const bool upgrade = keyUpgrade || revisionUpgrade;

    if (saveOk) {
        if (upgrade) {
            const bool saved =
                driver_.StoreProto<proto::Nym>(data, credentials_, plaintext);

            if (!saved) { return false; }

            revision_.store(revision);

            if (!alias.empty()) { alias_ = alias; }
        }
    }

    checked_->On();
    private_->Set(!incomingPublic);

    return save(lock);
}

auto Nym::Store(const proto::Purse& purse) -> bool
{
    Lock lock(write_lock_);
    const PurseID id{
        identifier::Server::Factory(purse.notary()),
        identifier::UnitDefinition::Factory(purse.mint())};
    std::string hash{};
    const auto output = driver_.StoreProto(purse, hash);

    if (false == output) { return output; }

    purse_id_[id] = hash;

    return output;
}

auto Nym::threads() const -> storage::Threads*
{
    return construct<storage::Threads>(
        threads_lock_, threads_, threads_root_, *mail_inbox(), *mail_outbox());
}

auto Nym::Threads() const -> const storage::Threads& { return *threads(); }

auto Nym::workflows() const -> storage::PaymentWorkflows*
{
    return construct<storage::PaymentWorkflows>(
        workflows_lock_, workflows_, workflows_root_);
}

Nym::~Nym() = default;
}  // namespace opentxs::storage

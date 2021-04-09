// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"             // IWYU pragma: associated
#include "1_Internal.hpp"           // IWYU pragma: associated
#include "api/storage/Storage.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "core/OTStorage.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/api/Legacy.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/storage/Multiplex.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/Contact.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/Context.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/StorageThread.pb.h"
#include "opentxs/protobuf/StorageThreadItem.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"
#include "storage/StorageConfig.hpp"
#include "storage/tree/Accounts.hpp"
#include "storage/tree/Bip47Channels.hpp"
#include "storage/tree/Contacts.hpp"
#include "storage/tree/Contexts.hpp"
#include "storage/tree/Credentials.hpp"
#include "storage/tree/Issuers.hpp"
#include "storage/tree/Mailbox.hpp"
#include "storage/tree/Notary.hpp"
#include "storage/tree/Nym.hpp"
#include "storage/tree/Nyms.hpp"
#include "storage/tree/PaymentWorkflows.hpp"
#include "storage/tree/PeerReplies.hpp"
#include "storage/tree/PeerRequests.hpp"
#include "storage/tree/Root.hpp"
#include "storage/tree/Seeds.hpp"
#include "storage/tree/Servers.hpp"
#include "storage/tree/Thread.hpp"
#include "storage/tree/Threads.hpp"
#include "storage/tree/Tree.hpp"
#include "storage/tree/Units.hpp"

#define STORAGE_CONFIG_KEY "storage"

#define OT_METHOD "opentxs::api::storage::implementation::Storage::"

namespace opentxs
{
auto Factory::Storage(
    const Flag& running,
    const api::Crypto& crypto,
    const api::Settings& config,
    const api::Legacy& legacy,
    const std::string& dataFolder,
    const String& defaultPluginCLI,
    const String& archiveDirectoryCLI,
    const std::chrono::seconds gcIntervalCLI,
    String& encryptedDirectoryCLI,
    StorageConfig& storageConfig) -> api::storage::StorageInternal*
{
    Digest hash = std::bind(
        static_cast<bool (api::crypto::Hash::*)(
            const std::uint32_t, const ReadView, const AllocateOutput) const>(
            &api::crypto::Hash::Digest),
        &(crypto.Hash()),
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3);
    Random random =
        std::bind(&api::crypto::Encode::RandomFilename, &(crypto.Encode()));
    std::shared_ptr<OTDB::StorageFS> storage(OTDB::StorageFS::Instantiate());

    {
        auto path = String::Factory();

        if (false == legacy.AppendFolder(
                         path,
                         String::Factory(dataFolder),
                         String::Factory(legacy.Common()))) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(
                "Failed to calculate storage path")
                .Flush();

            return nullptr;
        }

        if (false == legacy.BuildFolderPath(path)) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(
                "Failed to construct storage path")
                .Flush();

            return nullptr;
        }

        storageConfig.path_ = path->Get();
    }

    bool notUsed;
    bool migrate{false};
    auto old = String::Factory();
    OTString defaultPlugin = defaultPluginCLI;
    auto archiveDirectory = String::Factory();
    auto encryptedDirectory = String::Factory();

    LogDetail(OT_METHOD)(__FUNCTION__)(": Using ")(defaultPlugin)(
        " as primary storage plugin.")
        .Flush();

    if (archiveDirectoryCLI.empty()) {
        archiveDirectory =
            String::Factory(storageConfig.fs_backup_directory_.c_str());
    } else {
        archiveDirectory = archiveDirectoryCLI;
    }

    if (encryptedDirectoryCLI.empty()) {
        encryptedDirectory = String::Factory(
            storageConfig.fs_encrypted_backup_directory_.c_str());
    } else {
        encryptedDirectory = encryptedDirectoryCLI;
    }

    const bool haveGCInterval = (0 != gcIntervalCLI.count());
    std::int64_t defaultGcInterval{0};
    std::int64_t configGcInterval{0};

    if (haveGCInterval) {
        defaultGcInterval = gcIntervalCLI.count();
    } else {
        defaultGcInterval = storageConfig.gc_interval_;
    }

    encryptedDirectoryCLI.Set(encryptedDirectory);

    config.CheckSet_bool(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("auto_publish_nyms"),
        storageConfig.auto_publish_nyms_,
        storageConfig.auto_publish_nyms_,
        notUsed);
    config.CheckSet_bool(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("auto_publish_servers_"),
        storageConfig.auto_publish_servers_,
        storageConfig.auto_publish_servers_,
        notUsed);
    config.CheckSet_bool(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("auto_publish_units_"),
        storageConfig.auto_publish_units_,
        storageConfig.auto_publish_units_,
        notUsed);
    config.CheckSet_long(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("gc_interval"),
        defaultGcInterval,
        configGcInterval,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("path"),
        String::Factory(storageConfig.path_),
        storageConfig.path_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("fs_primary"),
        String::Factory(storageConfig.fs_primary_bucket_),
        storageConfig.fs_primary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("fs_secondary"),
        String::Factory(storageConfig.fs_secondary_bucket_),
        storageConfig.fs_secondary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("fs_root_file"),
        String::Factory(storageConfig.fs_root_file_),
        storageConfig.fs_root_file_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory(STORAGE_CONFIG_FS_BACKUP_DIRECTORY_KEY),
        archiveDirectory,
        storageConfig.fs_backup_directory_,
        notUsed);
    archiveDirectory =
        String::Factory(storageConfig.fs_backup_directory_.c_str());
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory(STORAGE_CONFIG_FS_ENCRYPTED_BACKUP_DIRECTORY_KEY),
        encryptedDirectory,
        storageConfig.fs_encrypted_backup_directory_,
        notUsed);
    encryptedDirectory =
        String::Factory(storageConfig.fs_encrypted_backup_directory_.c_str());
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("sqlite3_primary"),
        String::Factory(storageConfig.sqlite3_primary_bucket_),
        storageConfig.sqlite3_primary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("sqlite3_secondary"),
        String::Factory(storageConfig.sqlite3_secondary_bucket_),
        storageConfig.sqlite3_secondary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("sqlite3_control"),
        String::Factory(storageConfig.sqlite3_control_table_),
        storageConfig.sqlite3_control_table_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("sqlite3_root_key"),
        String::Factory(storageConfig.sqlite3_root_key_),
        storageConfig.sqlite3_root_key_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("sqlite3_db_file"),
        String::Factory(storageConfig.sqlite3_db_file_),
        storageConfig.sqlite3_db_file_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("lmdb_primary"),
        String::Factory(storageConfig.lmdb_primary_bucket_),
        storageConfig.lmdb_primary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("lmdb_secondary"),
        String::Factory(storageConfig.lmdb_secondary_bucket_),
        storageConfig.lmdb_secondary_bucket_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("lmdb_control"),
        String::Factory(storageConfig.lmdb_control_table_),
        storageConfig.lmdb_control_table_,
        notUsed);
    config.CheckSet_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory("lmdb_root_key"),
        String::Factory(storageConfig.lmdb_root_key_),
        storageConfig.lmdb_root_key_,
        notUsed);

    if (haveGCInterval) {
        storageConfig.gc_interval_ = defaultGcInterval;
        config.Set_long(
            String::Factory(STORAGE_CONFIG_KEY),
            String::Factory("gc_interval"),
            defaultGcInterval,
            notUsed);
    } else {
        storageConfig.gc_interval_ = configGcInterval;
    }

    const std::string defaultPluginName(defaultPlugin.get().Get());

    if (defaultPluginName == OT_STORAGE_PRIMARY_PLUGIN_LMDB) {
        {
            auto path = String::Factory();
            const auto subdir = std::string{legacy.Common()} + "_lmdb";

            if (false == legacy.AppendFolder(
                             path,
                             String::Factory(dataFolder),
                             String::Factory(subdir))) {
                LogOutput("opentxs::Factory::")(__FUNCTION__)(
                    "Failed to calculate lmdb storage path")
                    .Flush();

                return nullptr;
            }

            if (false == legacy.BuildFolderPath(path)) {
                LogOutput("opentxs::Factory::")(__FUNCTION__)(
                    "Failed to construct lmdb storage path")
                    .Flush();

                return nullptr;
            }

            storageConfig.path_ = path->Get();
        }
    }

    config.Set_str(
        String::Factory(STORAGE_CONFIG_KEY),
        String::Factory(STORAGE_CONFIG_PRIMARY_PLUGIN_KEY),
        defaultPlugin,
        notUsed);
    config.Save();

    return new api::storage::implementation::Storage(
        crypto,
        running,
        storageConfig,
        defaultPlugin,
        migrate,
        old,
        hash,
        random);
}
}  // namespace opentxs

namespace opentxs::api::storage::implementation
{
const std::uint32_t Storage::HASH_TYPE = 2;  // BTC160

Storage::Storage(
    const api::Crypto& crypto,
    const Flag& running,
    const StorageConfig& config,
    const String& primary,
    const bool migrate,
    const String& previous,
    const Digest& hash,
    const Random& random)
    : crypto_(crypto)
    , running_(running)
    , gc_interval_(config.gc_interval_)
    , write_lock_()
    , root_(nullptr)
    , primary_bucket_(Flag::Factory(false))
    , background_threads_()
    , config_(config)
    , multiplex_p_(opentxs::Factory::StorageMultiplex(
          *this,
          primary_bucket_,
          config_,
          primary,
          migrate,
          previous,
          hash,
          random))
    , multiplex_(*multiplex_p_)
{
    OT_ASSERT(multiplex_p_);
}

auto Storage::AccountAlias(const Identifier& accountID) const -> std::string
{
    return Root().Tree().Accounts().Alias(accountID.str());
}

auto Storage::AccountList() const -> ObjectList
{
    return Root().Tree().Accounts().List();
}

auto Storage::AccountContract(const Identifier& accountID) const -> OTUnitID
{
    return Root().Tree().Accounts().AccountContract(accountID);
}

auto Storage::AccountIssuer(const Identifier& accountID) const -> OTNymID
{
    return Root().Tree().Accounts().AccountIssuer(accountID);
}

auto Storage::AccountOwner(const Identifier& accountID) const -> OTNymID
{
    return Root().Tree().Accounts().AccountOwner(accountID);
}

auto Storage::AccountServer(const Identifier& accountID) const -> OTServerID
{
    return Root().Tree().Accounts().AccountServer(accountID);
}

auto Storage::AccountSigner(const Identifier& accountID) const -> OTNymID
{
    return Root().Tree().Accounts().AccountSigner(accountID);
}

auto Storage::AccountUnit(const Identifier& accountID) const
    -> proto::ContactItemType
{
    return Root().Tree().Accounts().AccountUnit(accountID);
}

auto Storage::AccountsByContract(
    const identifier::UnitDefinition& contract) const -> std::set<OTIdentifier>
{
    return Root().Tree().Accounts().AccountsByContract(contract);
}

auto Storage::AccountsByIssuer(const identifier::Nym& issuerNym) const
    -> std::set<OTIdentifier>
{
    return Root().Tree().Accounts().AccountsByIssuer(issuerNym);
}

auto Storage::AccountsByOwner(const identifier::Nym& ownerNym) const
    -> std::set<OTIdentifier>
{
    return Root().Tree().Accounts().AccountsByOwner(ownerNym);
}

auto Storage::AccountsByServer(const identifier::Server& server) const
    -> std::set<OTIdentifier>
{
    return Root().Tree().Accounts().AccountsByServer(server);
}

auto Storage::AccountsByUnit(const proto::ContactItemType unit) const
    -> std::set<OTIdentifier>
{
    return Root().Tree().Accounts().AccountsByUnit(unit);
}

auto Storage::Bip47Chain(
    const identifier::Nym& nymID,
    const Identifier& channelID) const -> proto::ContactItemType
{
    const bool exists = Root().Tree().Nyms().Exists(nymID.str());

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return proto::CITEMTYPE_ERROR;
    }

    return Root()
        .Tree()
        .Nyms()
        .Nym(nymID.str())
        .Bip47Channels()
        .Chain(channelID);
}

auto Storage::Bip47ChannelsByChain(
    const identifier::Nym& nymID,
    const proto::ContactItemType chain) const -> Storage::Bip47ChannelList
{
    const bool exists = Root().Tree().Nyms().Exists(nymID.str());

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root()
        .Tree()
        .Nyms()
        .Nym(nymID.str())
        .Bip47Channels()
        .ChannelsByChain(chain);
}

auto Storage::blockchain_thread_item_id(
    const opentxs::blockchain::Type chain,
    const Data& txid) const noexcept -> std::string
{
    return opentxs::blockchain_thread_item_id(crypto_, chain, txid)->str();
}

auto Storage::BlockchainAccountList(
    const std::string& nymID,
    const proto::ContactItemType type) const -> std::set<std::string>
{
    return Root().Tree().Nyms().Nym(nymID).BlockchainAccountList(type);
}

auto Storage::BlockchainAccountType(
    const std::string& nymID,
    const std::string& accountID) const -> proto::ContactItemType
{
    return Root().Tree().Nyms().Nym(nymID).BlockchainAccountType(accountID);
}

auto Storage::BlockchainThreadMap(const identifier::Nym& nym, const Data& txid)
    const noexcept -> std::vector<OTIdentifier>
{
    const auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" does not exist.")
            .Flush();

        return {};
    }

    return nyms.Nym(nym.str()).Threads().BlockchainThreadMap(txid);
}

auto Storage::BlockchainTransactionList(
    const identifier::Nym& nym) const noexcept -> std::vector<OTData>
{
    const auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" does not exist.")
            .Flush();

        return {};
    }

    return nyms.Nym(nym.str()).Threads().BlockchainTransactionList();
}

#if OT_CASH
auto Storage::CheckTokenSpent(
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit,
    const std::uint64_t series,
    const std::string& key) const -> bool
{
    return Root().Tree().Notary(notary.str()).CheckSpent(unit, series, key);
}
#endif

void Storage::Cleanup_Storage()
{
    for (auto& thread : background_threads_) {
        if (thread.joinable()) { thread.join(); }
    }

    if (root_) { root_->cleanup(); }
}

void Storage::Cleanup() { Cleanup_Storage(); }

void Storage::CollectGarbage() const { Root().Migrate(multiplex_.Primary()); }

auto Storage::ContactAlias(const std::string& id) const -> std::string
{
    return Root().Tree().Contacts().Alias(id);
}

auto Storage::ContactList() const -> ObjectList
{
    return Root().Tree().Contacts().List();
}

auto Storage::ContactOwnerNym(const std::string& nymID) const -> std::string
{
    return Root().Tree().Contacts().NymOwner(nymID);
}

void Storage::ContactSaveIndices() const
{
    mutable_Root().get().mutable_Tree().get().mutable_Contacts().get().Save();
}

auto Storage::ContactUpgradeLevel() const -> VersionNumber
{
    return Root().Tree().Contacts().UpgradeLevel();
}

auto Storage::ContextList(const std::string& nymID) const -> ObjectList
{
    return Root().Tree().Nyms().Nym(nymID).Contexts().List();
}

auto Storage::CreateThread(
    const std::string& nymID,
    const std::string& threadID,
    const std::set<std::string>& participants) const -> bool
{
    const auto id = mutable_Root()
                        .get()
                        .mutable_Tree()
                        .get()
                        .mutable_Nyms()
                        .get()
                        .mutable_Nym(nymID)
                        .get()
                        .mutable_Threads()
                        .get()
                        .Create(threadID, participants);

    return (false == id.empty());
}

auto Storage::DefaultSeed() const -> std::string
{
    return Root().Tree().Seeds().Default();
}

auto Storage::DeleteAccount(const std::string& id) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Accounts()
        .get()
        .Delete(id);
}

auto Storage::DeleteContact(const std::string& id) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Contacts()
        .get()
        .Delete(id);
}

auto Storage::DeletePaymentWorkflow(
    const std::string& nymID,
    const std::string& workflowID) const -> bool
{
    const bool exists = Root().Tree().Nyms().Exists(nymID);

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymID)
        .get()
        .mutable_PaymentWorkflows()
        .get()
        .Delete(workflowID);
}

auto Storage::HashType() const -> std::uint32_t { return HASH_TYPE; }

void Storage::InitBackup() { multiplex_.InitBackup(); }

void Storage::InitEncryptedBackup(opentxs::crypto::key::Symmetric& key)
{
    multiplex_.InitEncryptedBackup(key);
}

void Storage::InitPlugins()
{
    bool syncPrimary{false};
    const auto hash = multiplex_.BestRoot(syncPrimary);

    if (hash.empty()) { return; }

    std::unique_ptr<opentxs::storage::Root> root{new opentxs::storage::Root(
        multiplex_,
        hash,
        std::numeric_limits<std::int64_t>::max(),
        primary_bucket_)};

    OT_ASSERT(root);

    multiplex_.SynchronizePlugins(hash, *root, syncPrimary);
}

auto Storage::IssuerList(const std::string& nymID) const -> ObjectList
{
    const bool exists = Root().Tree().Nyms().Exists(nymID);

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).Issuers().List();
}

auto Storage::Load(
    const std::string& accountID,
    std::string& output,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Accounts().Load(accountID, output, alias, checking);
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& accountID,
    std::shared_ptr<proto::HDAccount>& output,
    const bool checking) const -> bool
{
    return Root().Tree().Nyms().Nym(nymID).Load(accountID, output, checking);
}

auto Storage::Load(
    const identifier::Nym& nymID,
    const Identifier& channelID,
    std::shared_ptr<proto::Bip47Channel>& output,
    const bool checking) const -> bool
{
    const bool exists = Root().Tree().Nyms().Exists(nymID.str());

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    return Root()
        .Tree()
        .Nyms()
        .Nym(nymID.str())
        .Bip47Channels()
        .Load(channelID, output, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Contact>& contact,
    const bool checking) const -> bool
{
    std::string notUsed{};

    return Load(id, contact, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Contact>& contact,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Contacts().Load(id, contact, alias, checking);
}

auto Storage::Load(
    const std::string& nym,
    const std::string& id,
    std::shared_ptr<proto::Context>& context,
    const bool checking) const -> bool
{
    std::string notUsed;

    return Root().Tree().Nyms().Nym(nym).Contexts().Load(
        id, context, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Credential>& cred,
    const bool checking) const -> bool
{
    return Root().Tree().Credentials().Load(id, cred, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Nym>& nym,
    const bool checking) const -> bool
{
    std::string notUsed;

    return Load(id, nym, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Nym>& nym,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Nyms().Nym(id).Load(nym, alias, checking);
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& id,
    std::shared_ptr<proto::Issuer>& issuer,
    const bool checking) const -> bool
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    std::string notUsed{""};

    return Root().Tree().Nyms().Nym(nymID).Issuers().Load(
        id, issuer, notUsed, checking);
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& workflowID,
    std::shared_ptr<proto::PaymentWorkflow>& workflow,
    const bool checking) const -> bool
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().Load(
        workflowID, workflow, checking);
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& id,
    const StorageBox box,
    std::string& output,
    std::string& alias,
    const bool checking) const -> bool
{
    switch (box) {
        case StorageBox::MAILINBOX: {
            return Root().Tree().Nyms().Nym(nymID).MailInbox().Load(
                id, output, alias, checking);
        }
        case StorageBox::MAILOUTBOX: {
            return Root().Tree().Nyms().Nym(nymID).MailOutbox().Load(
                id, output, alias, checking);
        }
        default: {
            return false;
        }
    }
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& id,
    const StorageBox box,
    std::shared_ptr<proto::PeerReply>& reply,
    const bool checking) const -> bool
{
    switch (box) {
        case StorageBox::SENTPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).SentReplyBox().Load(
                id, reply, checking);
        }
        case StorageBox::INCOMINGPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).IncomingReplyBox().Load(
                id, reply, checking);
        }
        case StorageBox::FINISHEDPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).FinishedReplyBox().Load(
                id, reply, checking);
        }
        case StorageBox::PROCESSEDPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).ProcessedReplyBox().Load(
                id, reply, checking);
        }
        default: {
            return false;
        }
    }
}

auto Storage::Load(
    const std::string& nymID,
    const std::string& id,
    const StorageBox box,
    std::shared_ptr<proto::PeerRequest>& request,
    std::time_t& time,
    const bool checking) const -> bool
{
    bool output = false;
    std::string alias;

    switch (box) {
        case StorageBox::SENTPEERREQUEST: {
            output = Root().Tree().Nyms().Nym(nymID).SentRequestBox().Load(
                id, request, alias, checking);
        } break;
        case StorageBox::INCOMINGPEERREQUEST: {
            output = Root().Tree().Nyms().Nym(nymID).IncomingRequestBox().Load(
                id, request, alias, checking);
        } break;
        case StorageBox::FINISHEDPEERREQUEST: {
            output = Root().Tree().Nyms().Nym(nymID).FinishedRequestBox().Load(
                id, request, alias, checking);
        } break;
        case StorageBox::PROCESSEDPEERREQUEST: {
            output = Root().Tree().Nyms().Nym(nymID).ProcessedRequestBox().Load(
                id, request, alias, checking);
        } break;
        default: {
        }
    }

    if (output) {
        try {
            time = std::stoi(alias);
        } catch (const std::invalid_argument&) {
            time = 0;
        } catch (const std::out_of_range&) {
            time = 0;
        }
    }

    return output;
}

auto Storage::Load(
    const identifier::Nym& nym,
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit,
    std::shared_ptr<proto::Purse>& output,
    const bool checking) const -> bool
{
    const auto& nymNode = Root().Tree().Nyms();

    if (false == nymNode.Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" doesn't exist.")
            .Flush();

        return false;
    }

    return nymNode.Nym(nym.str()).Load(notary, unit, output, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Seed>& seed,
    const bool checking) const -> bool
{
    std::string notUsed;

    return Load(id, seed, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::Seed>& seed,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Seeds().Load(id, seed, alias, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::ServerContract>& contract,
    const bool checking) const -> bool
{
    std::string notUsed;

    return Load(id, contract, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::ServerContract>& contract,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Servers().Load(id, contract, alias, checking);
}

auto Storage::Load(
    const std::string& nymId,
    const std::string& threadId,
    std::shared_ptr<proto::StorageThread>& thread) const -> bool
{
    const bool exists =
        Root().Tree().Nyms().Nym(nymId).Threads().Exists(threadId);

    if (!exists) { return false; }

    thread.reset(new proto::StorageThread);

    if (!thread) { return false; }

    *thread =
        Root().Tree().Nyms().Nym(nymId).Threads().Thread(threadId).Items();

    return bool(thread);
}

auto Storage::Load(
    std::shared_ptr<proto::Ciphertext>& output,
    const bool checking) const -> bool
{
    return Root().Tree().Load(output, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::UnitDefinition>& contract,
    const bool checking) const -> bool
{
    std::string notUsed;

    return Load(id, contract, notUsed, checking);
}

auto Storage::Load(
    const std::string& id,
    std::shared_ptr<proto::UnitDefinition>& contract,
    std::string& alias,
    const bool checking) const -> bool
{
    return Root().Tree().Units().Load(id, contract, alias, checking);
}

auto Storage::LocalNyms() const -> const std::set<std::string>
{
    return Root().Tree().Nyms().LocalNyms();
}

// Applies a lambda to all public nyms in the database in a detached thread.
void Storage::MapPublicNyms(NymLambda& lambda) const
{
    std::thread bgMap(&Storage::RunMapPublicNyms, this, lambda);
    bgMap.detach();
}

// Applies a lambda to all server contracts in the database in a detached
// thread.
void Storage::MapServers(ServerLambda& lambda) const
{
    std::thread bgMap(&Storage::RunMapServers, this, lambda);
    bgMap.detach();
}

// Applies a lambda to all unit definitions in the database in a detached
// thread.
void Storage::MapUnitDefinitions(UnitLambda& lambda) const
{
    std::thread bgMap(&Storage::RunMapUnits, this, lambda);
    bgMap.detach();
}

#if OT_CASH
auto Storage::MarkTokenSpent(
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit,
    const std::uint64_t series,
    const std::string& key) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Notary(notary.str())
        .get()
        .MarkSpent(unit, series, key);
}
#endif

auto Storage::MoveThreadItem(
    const std::string& nymId,
    const std::string& fromThreadID,
    const std::string& toThreadID,
    const std::string& itemID) const -> bool
{
    const auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nymId)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymId)(" does not exist.")
            .Flush();

        return false;
    }

    const auto& threads = nyms.Nym(nymId).Threads();

    if (false == threads.Exists(fromThreadID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Source thread ")(fromThreadID)(
            " does not exist.")
            .Flush();

        return false;
    }

    if (false == threads.Exists(toThreadID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Destination thread ")(toThreadID)(
            " does not exist.")
            .Flush();

        return false;
    }

    auto& fromThread = mutable_Root()
                           .get()
                           .mutable_Tree()
                           .get()
                           .mutable_Nyms()
                           .get()
                           .mutable_Nym(nymId)
                           .get()
                           .mutable_Threads()
                           .get()
                           .mutable_Thread(fromThreadID)
                           .get();
    const auto thread = fromThread.Items();
    auto found{false};
    auto time = std::uint64_t{};
    auto box = StorageBox{};
    const auto alias = std::string{};
    const auto contents = std::string{};
    auto index = std::uint64_t{};
    auto account = std::string{};

    for (const auto& item : thread.item()) {
        if (item.id() == itemID) {
            found = true;
            time = item.time();
            box = static_cast<StorageBox>(item.box());
            index = item.index();
            account = item.account();

            break;
        }
    }

    if (false == found) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Item does not exist.").Flush();

        return false;
    }

    if (false == fromThread.Remove(itemID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to remove item.").Flush();

        return false;
    }

    auto& toThread = mutable_Root()
                         .get()
                         .mutable_Tree()
                         .get()
                         .mutable_Nyms()
                         .get()
                         .mutable_Nym(nymId)
                         .get()
                         .mutable_Threads()
                         .get()
                         .mutable_Thread(toThreadID)
                         .get();
    const auto added =
        toThread.Add(itemID, time, box, alias, contents, index, account);

    if (false == added) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to insert item.").Flush();

        return false;
    }

    return true;
}

auto Storage::mutable_Root() const -> Editor<opentxs::storage::Root>
{
    std::function<void(opentxs::storage::Root*, Lock&)> callback =
        [&](opentxs::storage::Root* in, Lock& lock) -> void {
        this->save(in, lock);
    };

    return Editor<opentxs::storage::Root>(write_lock_, root(), callback);
}

auto Storage::NymBoxList(const std::string& nymID, const StorageBox box) const
    -> ObjectList
{
    switch (box) {
        case StorageBox::SENTPEERREQUEST: {
            return Root().Tree().Nyms().Nym(nymID).SentRequestBox().List();
        }
        case StorageBox::INCOMINGPEERREQUEST: {
            return Root().Tree().Nyms().Nym(nymID).IncomingRequestBox().List();
        }
        case StorageBox::SENTPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).SentReplyBox().List();
        }
        case StorageBox::INCOMINGPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).IncomingReplyBox().List();
        }
        case StorageBox::FINISHEDPEERREQUEST: {
            return Root().Tree().Nyms().Nym(nymID).FinishedRequestBox().List();
        }
        case StorageBox::FINISHEDPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).FinishedReplyBox().List();
        }
        case StorageBox::PROCESSEDPEERREQUEST: {
            return Root().Tree().Nyms().Nym(nymID).ProcessedRequestBox().List();
        }
        case StorageBox::PROCESSEDPEERREPLY: {
            return Root().Tree().Nyms().Nym(nymID).ProcessedReplyBox().List();
        }
        case StorageBox::MAILINBOX: {
            return Root().Tree().Nyms().Nym(nymID).MailInbox().List();
        }
        case StorageBox::MAILOUTBOX: {
            return Root().Tree().Nyms().Nym(nymID).MailOutbox().List();
        }
        default: {
            return {};
        }
    }
}

auto Storage::NymList() const -> ObjectList
{
    return Root().Tree().Nyms().List();
}

auto Storage::PaymentWorkflowList(const std::string& nymID) const -> ObjectList
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().List();
}

auto Storage::PaymentWorkflowLookup(
    const std::string& nymID,
    const std::string& sourceID) const -> std::string
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().LookupBySource(
        sourceID);
}

auto Storage::PaymentWorkflowsByAccount(
    const std::string& nymID,
    const std::string& accountID) const -> std::set<std::string>
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().ListByAccount(
        accountID);
}

auto Storage::PaymentWorkflowsByState(
    const std::string& nymID,
    const api::client::PaymentWorkflowType type,
    const api::client::PaymentWorkflowState state) const
    -> std::set<std::string>
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().ListByState(
        type, state);
}

auto Storage::PaymentWorkflowsByUnit(
    const std::string& nymID,
    const std::string& unitID) const -> std::set<std::string>
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().ListByUnit(
        unitID);
}

auto Storage::PaymentWorkflowState(
    const std::string& nymID,
    const std::string& workflowID) const -> std::
    pair<api::client::PaymentWorkflowType, api::client::PaymentWorkflowState>
{
    if (false == Root().Tree().Nyms().Exists(nymID)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return {};
    }

    return Root().Tree().Nyms().Nym(nymID).PaymentWorkflows().GetState(
        workflowID);
}

auto Storage::RelabelThread(
    const std::string& threadID,
    const std::string& label) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .RelabelThread(threadID, label);
}

auto Storage::RemoveBlockchainThreadItem(
    const identifier::Nym& nym,
    const Identifier& threadID,
    const opentxs::blockchain::Type chain,
    const Data& txid) const noexcept -> bool
{
    const auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" does not exist.")
            .Flush();

        return false;
    }

    const auto& threads = nyms.Nym(nym.str()).Threads();

    if (false == threads.Exists(threadID.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Thread ")(threadID)(
            " does not exist.")
            .Flush();

        return false;
    }

    auto& fromThread = mutable_Root()
                           .get()
                           .mutable_Tree()
                           .get()
                           .mutable_Nyms()
                           .get()
                           .mutable_Nym(nym.str())
                           .get()
                           .mutable_Threads(txid, threadID, false)
                           .get()
                           .mutable_Thread(threadID.str())
                           .get();
    const auto thread = fromThread.Items();
    auto found{false};
    const auto id = blockchain_thread_item_id(chain, txid);

    for (const auto& item : thread.item()) {
        if (item.id() == id) {
            found = true;

            break;
        }
    }

    if (false == found) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Item does not exist.").Flush();

        return false;
    }

    if (false == fromThread.Remove(id)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to remove item.").Flush();

        return false;
    }

    return true;
}

auto Storage::RemoveNymBoxItem(
    const std::string& nymID,
    const StorageBox box,
    const std::string& itemID) const -> bool
{
    switch (box) {
        case StorageBox::SENTPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_SentRequestBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::INCOMINGPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_IncomingRequestBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::SENTPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_SentReplyBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::INCOMINGPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_IncomingReplyBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::FINISHEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_FinishedRequestBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::FINISHEDPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_FinishedReplyBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::PROCESSEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_ProcessedRequestBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::PROCESSEDPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_ProcessedReplyBox()
                .get()
                .Delete(itemID);
        }
        case StorageBox::MAILINBOX: {
            const bool foundInThread = mutable_Root()
                                           .get()
                                           .mutable_Tree()
                                           .get()
                                           .mutable_Nyms()
                                           .get()
                                           .mutable_Nym(nymID)
                                           .get()
                                           .mutable_Threads()
                                           .get()
                                           .FindAndDeleteItem(itemID);
            bool foundInBox = false;

            if (!foundInThread) {
                foundInBox = mutable_Root()
                                 .get()
                                 .mutable_Tree()
                                 .get()
                                 .mutable_Nyms()
                                 .get()
                                 .mutable_Nym(nymID)
                                 .get()
                                 .mutable_MailInbox()
                                 .get()
                                 .Delete(itemID);
            }

            return foundInThread || foundInBox;
        }
        case StorageBox::MAILOUTBOX: {
            const bool foundInThread = mutable_Root()
                                           .get()
                                           .mutable_Tree()
                                           .get()
                                           .mutable_Nyms()
                                           .get()
                                           .mutable_Nym(nymID)
                                           .get()
                                           .mutable_Threads()
                                           .get()
                                           .FindAndDeleteItem(itemID);
            bool foundInBox = false;

            if (!foundInThread) {
                foundInBox = mutable_Root()
                                 .get()
                                 .mutable_Tree()
                                 .get()
                                 .mutable_Nyms()
                                 .get()
                                 .mutable_Nym(nymID)
                                 .get()
                                 .mutable_MailOutbox()
                                 .get()
                                 .Delete(itemID);
            }

            return foundInThread || foundInBox;
        }
        default: {
            return false;
        }
    }
}

auto Storage::RemoveServer(const std::string& id) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Servers()
        .get()
        .Delete(id);
}

auto Storage::RemoveThreadItem(
    const identifier::Nym& nym,
    const Identifier& threadID,
    const std::string& id) const -> bool
{
    const auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" does not exist.")
            .Flush();

        return false;
    }

    const auto& threads = nyms.Nym(nym.str()).Threads();

    if (false == threads.Exists(threadID.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Thread ")(threadID)(
            " does not exist.")
            .Flush();

        return false;
    }

    auto& fromThread = mutable_Root()
                           .get()
                           .mutable_Tree()
                           .get()
                           .mutable_Nyms()
                           .get()
                           .mutable_Nym(nym.str())
                           .get()
                           .mutable_Threads()
                           .get()
                           .mutable_Thread(threadID.str())
                           .get();
    const auto thread = fromThread.Items();
    auto found{false};

    for (const auto& item : thread.item()) {
        if (item.id() == id) {
            found = true;

            break;
        }
    }

    if (false == found) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Item does not exist.").Flush();

        return false;
    }

    if (false == fromThread.Remove(id)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to remove item.").Flush();

        return false;
    }

    return true;
}

auto Storage::RemoveUnitDefinition(const std::string& id) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Units()
        .get()
        .Delete(id);
}

auto Storage::RenameThread(
    const std::string& nymId,
    const std::string& threadId,
    const std::string& newID) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymId)
        .get()
        .mutable_Threads()
        .get()
        .Rename(threadId, newID);
}

auto Storage::root() const -> opentxs::storage::Root*
{
    Lock lock(write_lock_);

    if (!root_) {
        root_.reset(new opentxs::storage::Root(
            multiplex_, multiplex_.LoadRoot(), gc_interval_, primary_bucket_));
    }

    OT_ASSERT(root_);

    lock.unlock();

    return root_.get();
}

auto Storage::Root() const -> const opentxs::storage::Root& { return *root(); }

void Storage::RunGC() const
{
    if (!running_) { return; }

    CollectGarbage();
}

void Storage::RunMapPublicNyms(NymLambda lambda) const
{
    return Root().Tree().Nyms().Map(lambda);
}

void Storage::RunMapServers(ServerLambda lambda) const
{
    return Root().Tree().Servers().Map(lambda);
}

void Storage::RunMapUnits(UnitLambda lambda) const
{
    return Root().Tree().Units().Map(lambda);
}

void Storage::save(opentxs::storage::Root* in, const Lock& lock) const
{
    OT_ASSERT(verify_write_lock(lock));
    OT_ASSERT(nullptr != in);

    multiplex_.StoreRoot(true, in->root_);
}

auto Storage::SeedList() const -> ObjectList
{
    return Root().Tree().Seeds().List();
}

auto Storage::SetAccountAlias(const std::string& id, const std::string& alias)
    const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Accounts()
        .get()
        .SetAlias(id, alias);
}

auto Storage::SetContactAlias(const std::string& id, const std::string& alias)
    const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Contacts()
        .get()
        .SetAlias(id, alias);
}

auto Storage::SetDefaultSeed(const std::string& id) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Seeds()
        .get()
        .SetDefault(id);
}

auto Storage::SetNymAlias(const std::string& id, const std::string& alias) const
    -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(id)
        .get()
        .SetAlias(alias);
}

auto Storage::SetPeerRequestTime(
    const std::string& nymID,
    const std::string& id,
    const StorageBox box) const -> bool
{
    const std::string now = std::to_string(time(nullptr));

    switch (box) {
        case StorageBox::SENTPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_SentRequestBox()
                .get()
                .SetAlias(id, now);
        }
        case StorageBox::INCOMINGPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_IncomingRequestBox()
                .get()
                .SetAlias(id, now);
        }
        case StorageBox::FINISHEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_FinishedRequestBox()
                .get()
                .SetAlias(id, now);
        }
        case StorageBox::PROCESSEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_ProcessedRequestBox()
                .get()
                .SetAlias(id, now);
        }
        default: {
            return false;
        }
    }
}

auto Storage::SetReadState(
    const std::string& nymId,
    const std::string& threadId,
    const std::string& itemId,
    const bool unread) const -> bool
{
    auto& nyms = mutable_Root().get().mutable_Tree().get().mutable_Nyms().get();

    if (false == nyms.Exists(nymId)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymId)(" does not exist.")
            .Flush();

        return false;
    }

    auto& threads = nyms.mutable_Nym(nymId).get().mutable_Threads().get();

    if (false == threads.Exists(threadId)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Thread ")(threadId)(
            " does not exist.")
            .Flush();

        return false;
    }

    return threads.mutable_Thread(threadId).get().Read(itemId, unread);
}

auto Storage::SetSeedAlias(const std::string& id, const std::string& alias)
    const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Seeds()
        .get()
        .SetAlias(id, alias);
}

auto Storage::SetServerAlias(const std::string& id, const std::string& alias)
    const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Servers()
        .get()
        .SetAlias(id, alias);
}

auto Storage::SetThreadAlias(
    const std::string& nymId,
    const std::string& threadId,
    const std::string& alias) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymId)
        .get()
        .mutable_Threads()
        .get()
        .mutable_Thread(threadId)
        .get()
        .SetAlias(alias);
}

auto Storage::SetUnitDefinitionAlias(
    const std::string& id,
    const std::string& alias) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Units()
        .get()
        .SetAlias(id, alias);
}

auto Storage::ServerAlias(const std::string& id) const -> std::string
{
    return Root().Tree().Servers().Alias(id);
}

auto Storage::ServerList() const -> ObjectList
{
    return Root().Tree().Servers().List();
}

void Storage::start() { InitPlugins(); }

auto Storage::Store(
    const std::string& accountID,
    const std::string& data,
    const std::string& alias,
    const identifier::Nym& ownerNym,
    const identifier::Nym& signerNym,
    const identifier::Nym& issuerNym,
    const identifier::Server& server,
    const identifier::UnitDefinition& contract,
    const proto::ContactItemType unit) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Accounts()
        .get()
        .Store(
            accountID,
            data,
            alias,
            ownerNym,
            signerNym,
            issuerNym,
            server,
            contract,
            unit);
}

auto Storage::Store(
    const std::string& nymID,
    const proto::ContactItemType type,
    const proto::HDAccount& data) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymID)
        .get()
        .Store(type, data);
}

auto Storage::Store(
    const identifier::Nym& nymID,
    const Identifier& channelID,
    const proto::Bip47Channel& data) const -> bool
{
    const bool exists = Root().Tree().Nyms().Exists(nymID.str());

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymID.str())
        .get()
        .mutable_Bip47Channels()
        .get()
        .Store(channelID, data);
}

auto Storage::Store(const proto::Contact& data) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Contacts()
        .get()
        .Store(data, data.label());
}

auto Storage::Store(const proto::Context& data) const -> bool
{
    std::string notUsed;

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(data.localnym())
        .get()
        .mutable_Contexts()
        .get()
        .Store(data, notUsed);
}

auto Storage::Store(const proto::Credential& data) const -> bool
{
    std::string notUsed;

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Credentials()
        .get()
        .Store(data, notUsed);
}

auto Storage::Store(const proto::Nym& data, const std::string& alias) const
    -> bool
{
    std::string plaintext;
    const bool saved = mutable_Root()
                           .get()
                           .mutable_Tree()
                           .get()
                           .mutable_Nyms()
                           .get()
                           .mutable_Nym(data.nymid())
                           .get()
                           .Store(data, alias, plaintext);

    if (saved) {
        if (config_.auto_publish_nyms_ && config_.dht_callback_) {
            config_.dht_callback_(data.nymid(), plaintext);
        }

        return true;
    }

    return false;
}

auto Storage::Store(const std::string& nymID, const proto::Issuer& data) const
    -> bool
{
    const bool exists = Root().Tree().Nyms().Exists(nymID);

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymID)
        .get()
        .mutable_Issuers()
        .get()
        .Store(data, {""});
}

auto Storage::Store(
    const std::string& nymID,
    const proto::PaymentWorkflow& data) const -> bool
{
    const bool exists = Root().Tree().Nyms().Exists(nymID);

    if (false == exists) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(" doesn't exist.")
            .Flush();

        return false;
    }

    std::string notUsed{};

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymID)
        .get()
        .mutable_PaymentWorkflows()
        .get()
        .Store(data, notUsed);
}

auto Storage::Store(
    const std::string& nymid,
    const std::string& threadid,
    const std::string& itemid,
    const std::uint64_t time,
    const std::string& alias,
    const std::string& data,
    const StorageBox box,
    const std::string& account) const -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nymid)
        .get()
        .mutable_Threads()
        .get()
        .mutable_Thread(threadid)
        .get()
        .Add(itemid, time, box, alias, data, 0, account);
}

auto Storage::Store(
    const identifier::Nym& nym,
    const Identifier& thread,
    const opentxs::blockchain::Type chain,
    const Data& txid,
    const Time time) const noexcept -> bool
{
    const auto alias = std::string{};
    const auto account = std::string{};
    const auto id = blockchain_thread_item_id(chain, txid);

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nym.str())
        .get()
        .mutable_Threads(txid, thread, true)
        .get()
        .mutable_Thread(thread.str())
        .get()
        .Add(
            id,
            Clock::to_time_t(time),
            StorageBox::BLOCKCHAIN,
            alias,
            txid.str(),
            0,
            account,
            static_cast<std::uint32_t>(chain));
}

auto Storage::Store(
    const proto::PeerReply& data,
    const std::string& nymID,
    const StorageBox box) const -> bool
{
    switch (box) {
        case StorageBox::SENTPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_SentReplyBox()
                .get()
                .Store(data);
        }
        case StorageBox::INCOMINGPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_IncomingReplyBox()
                .get()
                .Store(data);
        }
        case StorageBox::FINISHEDPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_FinishedReplyBox()
                .get()
                .Store(data);
        }
        case StorageBox::PROCESSEDPEERREPLY: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_ProcessedReplyBox()
                .get()
                .Store(data);
        }
        default: {
            return false;
        }
    }
}

auto Storage::Store(
    const proto::PeerRequest& data,
    const std::string& nymID,
    const StorageBox box) const -> bool
{
    // Use the alias field to store the time at which the request was saved.
    // Useful for managing retry logic in the high level api
    const std::string now = std::to_string(time(nullptr));

    switch (box) {
        case StorageBox::SENTPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_SentRequestBox()
                .get()
                .Store(data, now);
        }
        case StorageBox::INCOMINGPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_IncomingRequestBox()
                .get()
                .Store(data, now);
        }
        case StorageBox::FINISHEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_FinishedRequestBox()
                .get()
                .Store(data, now);
        }
        case StorageBox::PROCESSEDPEERREQUEST: {
            return mutable_Root()
                .get()
                .mutable_Tree()
                .get()
                .mutable_Nyms()
                .get()
                .mutable_Nym(nymID)
                .get()
                .mutable_ProcessedRequestBox()
                .get()
                .Store(data, now);
        }
        default: {
            return false;
        }
    }
}

auto Storage::Store(const identifier::Nym& nym, const proto::Purse& purse) const
    -> bool
{
    auto nymNode = mutable_Root().get().mutable_Tree().get().mutable_Nyms();

    if (false == nymNode.get().Exists(nym.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nym)(" doesn't exist.")
            .Flush();

        return false;
    }

    return nymNode.get().mutable_Nym(nym.str()).get().Store(purse);
}

auto Storage::Store(const proto::Seed& data, const std::string& alias) const
    -> bool
{
    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Seeds()
        .get()
        .Store(data, alias);
}

auto Storage::Store(const proto::ServerContract& data, const std::string& alias)
    const -> bool
{
    auto storageVersion(data);
    storageVersion.clear_publicnym();
    std::string plaintext;
    const bool saved =
        mutable_Root().get().mutable_Tree().get().mutable_Servers().get().Store(
            data, alias, plaintext);

    if (saved) {
        if (config_.auto_publish_servers_ && config_.dht_callback_) {
            config_.dht_callback_(storageVersion.id(), plaintext);
        }

        return true;
    }

    return false;
}

auto Storage::Store(const proto::Ciphertext& serialized) const -> bool
{
    return mutable_Root().get().mutable_Tree().get().Store(serialized);
}

auto Storage::Store(const proto::UnitDefinition& data, const std::string& alias)
    const -> bool
{
    auto storageVersion(data);
    storageVersion.clear_publicnym();
    std::string plaintext;
    const bool saved =
        mutable_Root().get().mutable_Tree().get().mutable_Units().get().Store(
            data, alias, plaintext);

    if (saved) {
        if (config_.auto_publish_units_ && config_.dht_callback_) {
            config_.dht_callback_(storageVersion.id(), plaintext);
        }

        return true;
    }

    return false;
}

auto Storage::ThreadList(const std::string& nymID, const bool unreadOnly) const
    -> ObjectList
{
    return Root().Tree().Nyms().Nym(nymID).Threads().List(unreadOnly);
}

auto Storage::ThreadAlias(const std::string& nymID, const std::string& threadID)
    const -> std::string
{
    return Root().Tree().Nyms().Nym(nymID).Threads().Thread(threadID).Alias();
}

auto Storage::UnaffiliatedBlockchainTransaction(
    const identifier::Nym& nym,
    const Data& txid) const noexcept -> bool
{
    static const auto blank = Identifier::Factory();

    return mutable_Root()
        .get()
        .mutable_Tree()
        .get()
        .mutable_Nyms()
        .get()
        .mutable_Nym(nym.str())
        .get()
        .mutable_Threads()
        .get()
        .AddIndex(txid, blank);
}

auto Storage::UnitDefinitionAlias(const std::string& id) const -> std::string
{
    return Root().Tree().Units().Alias(id);
}

auto Storage::UnitDefinitionList() const -> ObjectList
{
    return Root().Tree().Units().List();
}

auto Storage::UnreadCount(const std::string& nymId, const std::string& threadId)
    const -> std::size_t
{
    auto& nyms = Root().Tree().Nyms();

    if (false == nyms.Exists(nymId)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym ")(nymId)(" does not exist.")
            .Flush();

        return 0;
    }

    auto& threads = nyms.Nym(nymId).Threads();

    if (false == threads.Exists(threadId)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Thread ")(threadId)(
            " does not exist.")
            .Flush();

        return 0;
    }

    return threads.Thread(threadId).UnreadCount();
}

void Storage::UpgradeNyms()
{
    const auto level = Root().Tree().Nyms().UpgradeLevel();

    if (3 > level) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Beginning upgrade for version ")(
            level)(".")
            .Flush();
        mutable_Root()
            .get()
            .mutable_Tree()
            .get()
            .mutable_Nyms()
            .get()
            .UpgradeLocalnym();
    } else {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": No need to upgrade version ")(
            level)
            .Flush();
    }
}

auto Storage::verify_write_lock(const Lock& lock) const -> bool
{
    if (lock.mutex() != &write_lock_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect mutex.").Flush();

        return false;
    }

    if (false == lock.owns_lock()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Lock not owned.").Flush();

        return false;
    }

    return true;
}

Storage::~Storage() { Cleanup_Storage(); }
}  // namespace opentxs::api::storage::implementation

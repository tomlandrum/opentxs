// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "api/client/blockchain/database/Database.hpp"
// IWYU pragma: no_include "internal/blockchain/client/Client.hpp"
// IWYU pragma: no_include "opentxs/api/client/Contacts.hpp"
// IWYU pragma: no_include "opentxs/api/client/blockchain/AddressStyle.hpp"
// IWYU pragma: no_include "opentxs/api/client/blockchain/BalanceTree.hpp"
// IWYU pragma: no_include "opentxs/api/client/blockchain/HD.hpp"
// IWYU pragma: no_include "opentxs/api/client/blockchain/PaymentCode.hpp"
// IWYU pragma: no_include "opentxs/api/client/blockchain/Subchain.hpp"
// IWYU pragma: no_include "opentxs/blockchain/BlockchainType.hpp"
// IWYU pragma: no_include "opentxs/blockchain/Network.hpp"
// IWYU pragma: no_include "opentxs/core/identifier/Nym.hpp"
// IWYU pragma: no_include "opentxs/network/zeromq/socket/Publish.hpp"

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "internal/api/client/Client.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/blockchain/BalanceList.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/protobuf/BlockchainP2PHello.pb.h"

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
namespace database
{
namespace implementation
{
class Database;
}  // namespace implementation
}  // namespace database

namespace internal
{
struct BalanceTree;
}  // namespace internal

class BalanceTree;
class HD;
class PaymentCode;
}  // namespace blockchain

class Activity;
class Contacts;
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal

class Legacy;
}  // namespace api

namespace blockchain
{
class Network;
}  // namespace blockchain

namespace identifier
{
class Nym;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

namespace proto
{
class HDPath;
}  // namespace proto

class Contact;
class PasswordPrompt;
class PaymentCode;
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
class Blockchain final : virtual public internal::Blockchain
{
public:
    struct Imp;

    enum class AccountType : std::uint8_t {
        Error = 0,
        HD,
        Imported,
        PaymentCode,
    };

    auto Account(const identifier::Nym& nymID, const Chain chain) const
        noexcept(false) -> const blockchain::BalanceTree& final;
    auto Accounts(const Chain chain) const noexcept(false)
        -> const blockchain::BalanceList& final;
    auto AccountList(const identifier::Nym& nymID) const noexcept
        -> std::set<OTIdentifier> final;
    auto AccountList(const Chain chain) const noexcept
        -> std::set<OTIdentifier> final;
    auto AccountList() const noexcept -> std::set<OTIdentifier> final;
    auto ActivityDescription(
        const identifier::Nym& nym,
        const Identifier& thread,
        const std::string& threadItemID) const noexcept -> std::string final;
    auto ActivityDescription(
        const identifier::Nym& nym,
        const Chain chain,
        const Tx& transaction) const noexcept -> std::string final;
    auto AssignContact(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const blockchain::Subchain subchain,
        const Bip32Index index,
        const Identifier& contactID) const noexcept -> bool final;
    auto AssignLabel(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const blockchain::Subchain subchain,
        const Bip32Index index,
        const std::string& label) const noexcept -> bool final;
    auto AssignTransactionMemo(const std::string& id, const std::string& label)
        const noexcept -> bool final;
    auto BalanceTree(const identifier::Nym& nymID, const Chain chain) const
        noexcept(false) -> const blockchain::internal::BalanceTree& final;
    auto BlockchainDB() const noexcept
        -> const blockchain::database::implementation::Database& final;
    auto CalculateAddress(
        const Chain chain,
        const blockchain::AddressStyle format,
        const Data& pubkey) const noexcept -> std::string final;
    auto Confirm(
        const blockchain::Key key,
        const opentxs::blockchain::block::Txid& tx) const noexcept
        -> bool final;
    auto Contacts() const noexcept -> const api::client::Contacts& final;
    auto DecodeAddress(const std::string& encoded) const noexcept
        -> DecodedAddress final;
    auto Disable(const Chain type) const noexcept -> bool final;
    auto Enable(const Chain type, const std::string& seednode) const noexcept
        -> bool final;
    auto EnabledChains() const noexcept -> std::set<Chain> final;
    auto EncodeAddress(const Style style, const Chain chain, const Data& data)
        const noexcept -> std::string final;
    auto GetChain(const Chain type) const noexcept(false)
        -> const opentxs::blockchain::Network& final;
    auto GetKey(const blockchain::Key& id) const noexcept(false)
        -> const blockchain::BalanceNode::Element& final;
    auto HDSubaccount(const identifier::Nym& nymID, const Identifier& accountID)
        const noexcept(false) -> const blockchain::HD& final;
    auto Hello() const noexcept -> proto::BlockchainP2PHello final;
    auto IndexItem(const ReadView bytes) const noexcept -> PatternID final;
    auto IsEnabled(const opentxs::blockchain::Type chain) const noexcept
        -> bool final;
    auto KeyEndpoint() const noexcept -> const std::string& final;
    auto KeyGenerated(const Chain chain) const noexcept -> void final;
    auto LoadTransactionBitcoin(const TxidHex& id) const noexcept
        -> std::unique_ptr<const Tx> final;
    auto LoadTransactionBitcoin(const Txid& id) const noexcept
        -> std::unique_ptr<const Tx> final;
    auto LookupAccount(const Identifier& id) const noexcept
        -> AccountData final;
    auto LookupContacts(const std::string& address) const noexcept
        -> ContactList final;
    auto LookupContacts(const Data& pubkeyHash) const noexcept
        -> ContactList final;
    auto NewHDSubaccount(
        const identifier::Nym& nymID,
        const BlockchainAccountType standard,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept -> OTIdentifier final;
    auto NewPaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const proto::HDPath path,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept -> OTIdentifier final;
    auto NewPaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const ReadView& view,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept -> OTIdentifier final;
    auto Owner(const Identifier& accountID) const noexcept
        -> const identifier::Nym& final;
    auto Owner(const blockchain::Key& key) const noexcept
        -> const identifier::Nym& final;
    auto PaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const Identifier& accountID) const noexcept(false)
        -> const blockchain::PaymentCode& final;
    auto PaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const proto::HDPath path,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept(false)
        -> const blockchain::PaymentCode& final;
    auto PubkeyHash(const Chain chain, const Data& pubkey) const noexcept(false)
        -> OTData final;
    auto ProcessContact(const Contact& contact) const noexcept -> bool final;
    auto ProcessMergedContact(const Contact& parent, const Contact& child)
        const noexcept -> bool final;
    auto ProcessSyncData(OTZMQMessage&& in) const noexcept -> void final;
    auto ProcessTransaction(
        const Chain chain,
        const Tx& transaction,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto RecipientContact(const blockchain::Key& key) const noexcept
        -> OTIdentifier final;
    auto Reorg() const noexcept
        -> const opentxs::network::zeromq::socket::Publish& final;
    auto Release(const blockchain::Key key) const noexcept -> bool final;
    auto ReportProgress(
        const Chain chain,
        const opentxs::blockchain::block::Height current,
        const opentxs::blockchain::block::Height target) const noexcept
        -> void final;
    auto ReportScan(
        const Chain chain,
        const identifier::Nym& owner,
        const Identifier& account,
        const blockchain::Subchain subchain,
        const opentxs::blockchain::block::Position& progress) const noexcept
        -> void final;
    auto RestoreNetworks() const noexcept -> void final;
    auto SenderContact(const blockchain::Key& key) const noexcept
        -> OTIdentifier final;
    auto Start(const Chain type, const std::string& seednode) const noexcept
        -> bool final;
    auto StartSyncServer(
        const std::string& syncEndpoint,
        const std::string& publicSyncEndpoint,
        const std::string& updateEndpoint,
        const std::string& publicUpdateEndpoint) const noexcept -> bool final;
    auto Stop(const Chain type) const noexcept -> bool final;
    auto SubaccountList(const identifier::Nym& nymID, const Chain chain)
        const noexcept -> std::set<OTIdentifier> final;
    auto UpdateBalance(
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::Balance balance) const noexcept
        -> void final;
    auto UpdateBalance(
        const identifier::Nym& owner,
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::Balance balance) const noexcept
        -> void final;
    auto UpdateElement(std::vector<ReadView>& pubkeyHashes) const noexcept
        -> void final;
    auto UpdatePeer(
        const opentxs::blockchain::Type chain,
        const std::string& address) const noexcept -> void final;
    auto Unconfirm(
        const blockchain::Key key,
        const opentxs::blockchain::block::Txid& tx,
        const Time time) const noexcept -> bool final;

    auto Init() noexcept -> void final;
    auto Shutdown() noexcept -> void final;

    Blockchain(
        const api::internal::Core& api,
        const api::client::Activity& activity,
        const api::client::Contacts& contacts,
        const api::Legacy& legacy,
        const std::string& dataFolder,
        const ArgList& args) noexcept;

    ~Blockchain() final;

private:
    std::unique_ptr<Imp> imp_;

    Blockchain() = delete;
    Blockchain(const Blockchain&) = delete;
    Blockchain(Blockchain&&) = delete;
    auto operator=(const Blockchain&) -> Blockchain& = delete;
    auto operator=(Blockchain&&) -> Blockchain& = delete;
};
}  // namespace opentxs::api::client::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/client/Blockchain.cpp"

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

#if OT_BLOCKCHAIN
#include "api/client/blockchain/database/Database.hpp"
#endif  // OT_BLOCKCHAIN
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/blockchain/Blockchain.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Network.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Bip44Type.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Pull.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Router.hpp"
#endif  // OT_BLOCKCHAIN

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
namespace internal
{
struct BalanceNode;
}  // namespace internal

class BalanceTree;
class HD;
struct SyncClient;
struct SyncServer;
}  // namespace blockchain

class Activity;
}  // namespace client

namespace internal
{
struct Core;
}  // namespace internal

class Core;
class Legacy;
}  // namespace api

namespace blockchain
{
namespace block
{
namespace bitcoin
{
namespace internal
{
struct Transaction;
}  // namespace internal
}  // namespace bitcoin
}  // namespace block
}  // namespace blockchain

namespace network
{
namespace zeromq
{
class Context;
class Message;
}  // namespace zeromq
}  // namespace network

namespace proto
{
class BlockchainP2PHello;
class BlockchainTransaction;
class HDPath;
}  // namespace proto

class Contact;
class PasswordPrompt;
class PaymentCode;
}  // namespace opentxs

namespace zmq = opentxs::network::zeromq;

namespace opentxs::api::client::implementation
{
class Blockchain final : virtual public internal::Blockchain
{
public:
    enum class Prefix : std::uint8_t {
        Unknown = 0,
        BitcoinP2PKH,
        BitcoinP2SH,
        BitcoinTestnetP2PKH,
        BitcoinTestnetP2SH,
        LitecoinP2PKH,
        LitecoinP2SH,
        LitecoinTestnetP2SH,
        PKTP2PKH,
        PKTP2SH,
    };

    using StylePair = std::pair<Style, Chain>;
    // Style, preferred prefix, additional prefixes
    using StyleMap = std::map<StylePair, std::pair<Prefix, std::set<Prefix>>>;
    using StyleReverseMap = std::map<Prefix, std::set<StylePair>>;
    using HrpMap = std::map<Chain, std::string>;
    using HrpReverseMap = std::map<std::string, Chain>;

    static auto reverse(const StyleMap& in) noexcept -> StyleReverseMap;

    auto Account(const identifier::Nym& nymID, const Chain chain) const
        noexcept(false) -> const blockchain::BalanceTree& final
    {
        return BalanceTree(nymID, chain);
    }
    auto AccountList(const identifier::Nym& nymID, const Chain chain)
        const noexcept -> std::set<OTIdentifier> final
    {
        return accounts_.List(nymID, chain);
    }
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
#if OT_BLOCKCHAIN
    auto BlockchainDB() const noexcept
        -> const blockchain::database::implementation::Database& final
    {
        return db_;
    }
#endif  // OT_BLOCKCHAIN
    auto CalculateAddress(
        const Chain chain,
        const blockchain::AddressStyle format,
        const Data& pubkey) const noexcept -> std::string final;
    auto Confirm(
        const blockchain::Key key,
        const opentxs::blockchain::block::Txid& tx) const noexcept
        -> bool final;
    auto Contacts() const noexcept -> const api::client::Contacts& final
    {
        return contacts_;
    }
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
#if OT_BLOCKCHAIN
    auto Hello() const noexcept -> proto::BlockchainP2PHello;
#endif  // OT_BLOCKCHAIN
    auto IndexItem(const ReadView bytes) const noexcept -> PatternID final;
#if OT_BLOCKCHAIN
    auto IO() const noexcept
        -> const opentxs::blockchain::client::internal::IO& final
    {
        return io_;
    }
    auto IsEnabled(const opentxs::blockchain::Type chain) const noexcept
        -> bool final;
    auto KeyEndpoint() const noexcept -> const std::string& final
    {
        return key_generated_endpoint_;
    }
    auto KeyGenerated(const Chain chain) const noexcept -> void final;
#endif  // OT_BLOCKCHAIN
    auto LoadTransactionBitcoin(const TxidHex& id) const noexcept
        -> std::unique_ptr<const Tx> final;
    auto LoadTransactionBitcoin(const Txid& id) const noexcept
        -> std::unique_ptr<const Tx> final;
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
    auto Owner(const Identifier& accountID) const noexcept
        -> const identifier::Nym& final
    {
        return accounts_.Owner(accountID);
    }
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
#if OT_BLOCKCHAIN
    auto ProcessContact(const Contact& contact) const noexcept -> bool final;
    auto ProcessMergedContact(const Contact& parent, const Contact& child)
        const noexcept -> bool final;
    auto ProcessSyncData(OTZMQMessage&& in) const noexcept -> void;
#endif  // OT_BLOCKCHAIN
    auto ProcessTransaction(
        const Chain chain,
        const Tx& transaction,
        const PasswordPrompt& reason) const noexcept -> bool final;
    auto RecipientContact(const blockchain::Key& key) const noexcept
        -> OTIdentifier final;
#if OT_BLOCKCHAIN
    auto Reorg() const noexcept -> const zmq::socket::Publish& final
    {
        return reorg_;
    }
#endif  // OT_BLOCKCHAIN
    auto Release(const blockchain::Key key) const noexcept -> bool final;
#if OT_BLOCKCHAIN
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
#endif  // OT_BLOCKCHAIN
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
#if OT_BLOCKCHAIN
    auto UpdateBalance(
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::Balance balance) const noexcept -> void final
    {
        balances_.UpdateBalance(chain, balance);
    }
    auto UpdateBalance(
        const identifier::Nym& owner,
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::Balance balance) const noexcept -> void final
    {
        balances_.UpdateBalance(owner, chain, balance);
    }
#endif  // OT_BLOCKCHAIN
    auto UpdateElement(std::vector<ReadView>& pubkeyHashes) const noexcept
        -> void final;
#if OT_BLOCKCHAIN
    auto UpdatePeer(
        const opentxs::blockchain::Type chain,
        const std::string& address) const noexcept -> void final;
#endif  // OT_BLOCKCHAIN
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
    enum class AccountType : std::uint8_t {
        Error = 0,
        HD,
        Imported,
        PaymentCode,
    };

    using IDLock = std::map<OTIdentifier, std::mutex>;
    using TXOs = std::vector<blockchain::Activity>;
    /// Unspent, spent
    using ParsedTransaction = std::pair<TXOs, TXOs>;
    using AddressMap = std::map<Prefix, std::string>;
    using AddressReverseMap = std::map<std::string, Prefix>;
#if OT_BLOCKCHAIN
    using Txid = opentxs::blockchain::block::Txid;
    using pTxid = opentxs::blockchain::block::pTxid;
    using LastHello = std::map<Chain, Time>;
    using Chains = std::vector<Chain>;
    using Config = opentxs::blockchain::client::internal::Config;
#endif  // OT_BLOCKCHAIN

    struct AccountCache {
        auto List(const identifier::Nym& nymID, const Chain chain)
            const noexcept -> std::set<OTIdentifier>;
        auto New(
            const AccountType type,
            const Chain chain,
            const Identifier& account,
            const identifier::Nym& owner) const noexcept -> void;
        auto Owner(const Identifier& accountID) const noexcept
            -> const identifier::Nym&;
        auto Type(const Identifier& accountID) const noexcept -> AccountType;

        auto Populate() noexcept -> void;

        AccountCache(const api::Core& api) noexcept;

    private:
        using NymAccountMap = std::map<OTNymID, std::set<OTIdentifier>>;
        using ChainAccountMap = std::map<Chain, std::optional<NymAccountMap>>;
        using AccountNymIndex = std::map<OTIdentifier, OTNymID>;
        using AccountTypeIndex = std::map<OTIdentifier, AccountType>;

        const api::Core& api_;
        mutable std::mutex lock_;
        mutable ChainAccountMap account_map_;
        mutable AccountNymIndex account_index_;
        mutable AccountTypeIndex account_type_;

        auto build_account_map(
            const Lock&,
            const Chain chain,
            std::optional<NymAccountMap>& map) const noexcept -> void;
        auto get_account_map(const Lock&, const Chain chain) const noexcept
            -> NymAccountMap&;
    };

    struct BalanceLists {
        auto Get(const Chain chain) noexcept
            -> client::blockchain::internal::BalanceList&;

        BalanceLists(
            const api::internal::Core& api,
            api::client::internal::Blockchain& parent) noexcept;

    private:
        const api::internal::Core& api_;
        api::client::internal::Blockchain& parent_;
        std::mutex lock_;
        std::map<
            Chain,
            std::unique_ptr<client::blockchain::internal::BalanceList>>
            lists_;
    };
#if OT_BLOCKCHAIN
    struct BalanceOracle {
        using Balance = opentxs::blockchain::Balance;
        using Chain = opentxs::blockchain::Type;

        auto RefreshBalance(const identifier::Nym& owner, const Chain chain)
            const noexcept -> void;
        auto UpdateBalance(const Chain chain, const Balance balance)
            const noexcept -> void;
        auto UpdateBalance(
            const identifier::Nym& owner,
            const Chain chain,
            const Balance balance) const noexcept -> void;

        BalanceOracle(
            const api::client::internal::Blockchain& parent,
            const api::Core& api) noexcept;

    private:
        using Subscribers = std::set<OTData>;

        const api::client::internal::Blockchain& parent_;
        const api::Core& api_;
        const zmq::Context& zmq_;
        OTZMQListenCallback cb_;
        OTZMQRouterSocket socket_;
        mutable std::mutex lock_;
        mutable std::map<Chain, Subscribers> subscribers_;
        mutable std::map<Chain, std::map<OTNymID, Subscribers>>
            nym_subscribers_;

        auto cb(zmq::Message& message) noexcept -> void;

        BalanceOracle() = delete;
    };
    struct EnableCallbacks {
        using EnabledCallback = std::function<bool(const bool)>;

        auto Add(const Chain type, EnabledCallback cb) noexcept -> std::size_t;
        auto Delete(const Chain type, const std::size_t index) noexcept -> void;
        auto Execute(const Chain type, const bool value) noexcept -> void;

        EnableCallbacks(const api::Core& api) noexcept;

    private:
        const zmq::Context& zmq_;
        mutable std::mutex lock_;
        std::map<Chain, std::vector<EnabledCallback>> map_;
        OTZMQPublishSocket socket_;

        EnableCallbacks(const EnableCallbacks&) = delete;
        EnableCallbacks(EnableCallbacks&&) = delete;
        auto operator=(const EnableCallbacks&) -> EnableCallbacks& = delete;
        auto operator=(EnableCallbacks&&) -> EnableCallbacks& = delete;
    };
#endif  // OT_BLOCKCHAIN

    static const AddressMap address_prefix_map_;
    static const AddressReverseMap address_prefix_reverse_map_;
    static const StyleMap address_style_map_;
    static const StyleReverseMap address_style_reverse_map_;
    static const HrpMap hrp_map_;
    static const HrpReverseMap hrp_reverse_map_;

    const api::internal::Core& api_;
#if OT_BLOCKCHAIN
    const api::client::Activity& activity_;
#endif  // OT_BLOCKCHAIN
    const api::client::Contacts& contacts_;
    const DecodedAddress blank_;
    mutable std::mutex lock_;
    mutable IDLock nym_lock_;
    mutable AccountCache accounts_;
    mutable BalanceLists balance_lists_;
#if OT_BLOCKCHAIN
    const std::string key_generated_endpoint_;
    opentxs::blockchain::client::internal::IO io_;
    blockchain::database::implementation::Database db_;
    OTZMQPublishSocket reorg_;
    OTZMQPublishSocket transaction_updates_;
    OTZMQPublishSocket peer_updates_;
    OTZMQPublishSocket key_updates_;
    OTZMQPublishSocket sync_updates_;
    OTZMQPublishSocket scan_updates_;
    OTZMQPublishSocket new_blockchain_accounts_;
    const Config base_config_;
    mutable std::map<Chain, Config> config_;
    mutable std::map<
        Chain,
        std::unique_ptr<opentxs::blockchain::client::internal::Network>>
        networks_;
    std::unique_ptr<blockchain::SyncClient> sync_client_;
    std::unique_ptr<blockchain::SyncServer> sync_server_;
    BalanceOracle balances_;
    OTZMQPublishSocket chain_state_publisher_;
    mutable LastHello last_hello_;
    std::atomic_bool running_;
    std::thread heartbeat_;
#endif  // OT_BLOCKCHAIN

    auto address_prefix(const Style style, const Chain chain) const
        noexcept(false) -> OTData;
    auto decode_bech23(const std::string& encoded) const noexcept
        -> std::optional<DecodedAddress>;
    auto decode_legacy(const std::string& encoded) const noexcept
        -> std::optional<DecodedAddress>;
    auto bip44_type(const contact::ContactItemType type) const noexcept
        -> Bip44Type;
    void init_path(
        const std::string& root,
        const contact::ContactItemType chain,
        const Bip32Index account,
        const BlockchainAccountType standard,
        proto::HDPath& path) const noexcept;
#if OT_BLOCKCHAIN
    auto broadcast_update_signal(const Txid& txid) const noexcept -> void
    {
        broadcast_update_signal(std::vector<pTxid>{txid});
    }
    auto broadcast_update_signal(
        const std::vector<pTxid>& transactions) const noexcept -> void;
    auto broadcast_update_signal(
        const opentxs::blockchain::block::bitcoin::internal::Transaction& tx)
        const noexcept -> void;
    auto check_hello(const Lock& lock) const noexcept -> Chains;
    auto disable(const Lock& lock, const Chain type) const noexcept -> bool;
    auto enable(const Lock& lock, const Chain type, const std::string& seednode)
        const noexcept -> bool;
#endif  // OT_BLOCKCHAIN
    auto get_node(const Identifier& accountID) const noexcept(false)
        -> blockchain::internal::BalanceNode&;
#if OT_BLOCKCHAIN
    auto heartbeat() const noexcept -> void;
    auto hello(const Lock&, const Chains& chains) const noexcept
        -> proto::BlockchainP2PHello;
    auto load_transaction(const Lock& lock, const Txid& id) const noexcept
        -> std::unique_ptr<
            opentxs::blockchain::block::bitcoin::internal::Transaction>;
    auto load_transaction(const Lock& lock, const TxidHex& id) const noexcept
        -> std::unique_ptr<
            opentxs::blockchain::block::bitcoin::internal::Transaction>;
#endif  // OT_BLOCKCHAIN
    auto new_payment_code(
        const Lock& lock,
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const proto::HDPath path,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept -> OTIdentifier;
    auto p2pkh(const Chain chain, const Data& pubkeyHash) const noexcept
        -> std::string;
    auto p2sh(const Chain chain, const Data& scriptHash) const noexcept
        -> std::string;
#if OT_BLOCKCHAIN
    auto publish_chain_state(Chain type, bool state) const -> void;
    auto reconcile_activity_threads(const Lock& lock, const Txid& txid)
        const noexcept -> bool;
    auto reconcile_activity_threads(
        const Lock& lock,
        const opentxs::blockchain::block::bitcoin::internal::Transaction& tx)
        const noexcept -> bool;
    auto start(const Lock& lock, const Chain type, const std::string& seednode)
        const noexcept -> bool;
    auto stop(const Lock& lock, const Chain type) const noexcept -> bool;
#endif  // OT_BLOCKCHAIN
    auto validate_nym(const identifier::Nym& nymID) const noexcept -> bool;

    Blockchain() = delete;
    Blockchain(const Blockchain&) = delete;
    Blockchain(Blockchain&&) = delete;
    auto operator=(const Blockchain&) -> Blockchain& = delete;
    auto operator=(Blockchain&&) -> Blockchain& = delete;
};
}  // namespace opentxs::api::client::implementation

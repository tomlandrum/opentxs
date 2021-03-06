// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_BLOCKCHAIN_HPP
#define OPENTXS_API_CLIENT_BLOCKCHAIN_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/Types.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/crypto/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Types.hpp"

namespace opentxs
{
namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Transaction;
}  // namespace bitcoin
}  // namespace block

namespace crypto
{
class Account;
class Element;
class HD;
class PaymentCode;
class Wallet;
}  // namespace crypto

namespace node
{
class Manager;
}  // namespace node
}  // namespace blockchain

namespace proto
{
class HDPath;
}  // namespace proto

class Contact;
class Identifier;
class PasswordPrompt;
class PaymentCode;
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace client
{
class OPENTXS_EXPORT Blockchain
{
public:
    using Chain = opentxs::blockchain::Type;
    using Key = opentxs::blockchain::crypto::Key;
    using Style = opentxs::blockchain::crypto::AddressStyle;
    using Subchain = opentxs::blockchain::crypto::Subchain;
    using DecodedAddress = std::tuple<OTData, Style, std::set<Chain>, bool>;
    using ContactList = std::set<OTIdentifier>;
    using Tx = opentxs::blockchain::block::bitcoin::Transaction;
    using Txid = opentxs::blockchain::block::Txid;
    using TxidHex = std::string;
    using PatternID = opentxs::blockchain::PatternID;
    using AccountData = std::pair<Chain, OTNymID>;

    // Throws std::out_of_range for invalid chains
    static Bip44Type Bip44(Chain chain) noexcept(false);
    static bool Bip44Path(
        Chain chain,
        const std::string& seed,
        AllocateOutput destination) noexcept(false);

    /// Throws std::runtime_error if chain is invalid
    virtual const opentxs::blockchain::crypto::Account& Account(
        const identifier::Nym& nymID,
        const Chain chain) const noexcept(false) = 0;
    virtual std::set<OTIdentifier> AccountList(
        const identifier::Nym& nymID) const noexcept = 0;
    virtual std::set<OTIdentifier> AccountList(
        const Chain chain) const noexcept = 0;
    virtual std::set<OTIdentifier> AccountList() const noexcept = 0;
    virtual std::string ActivityDescription(
        const identifier::Nym& nym,
        const Identifier& thread,
        const std::string& threadItemID) const noexcept = 0;
    virtual std::string ActivityDescription(
        const identifier::Nym& nym,
        const Chain chain,
        const Tx& transaction) const noexcept = 0;
    virtual bool AssignContact(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const Subchain subchain,
        const Bip32Index index,
        const Identifier& label) const noexcept = 0;
    virtual bool AssignLabel(
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const Subchain subchain,
        const Bip32Index index,
        const std::string& label) const noexcept = 0;
    virtual bool AssignTransactionMemo(
        const TxidHex& id,
        const std::string& label) const noexcept = 0;
    virtual std::string CalculateAddress(
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::crypto::AddressStyle format,
        const Data& pubkey) const noexcept = 0;
    virtual bool Confirm(
        const Key key,
        const opentxs::blockchain::block::Txid& tx) const noexcept = 0;
    virtual DecodedAddress DecodeAddress(
        const std::string& encoded) const noexcept = 0;
    virtual std::string EncodeAddress(
        const Style style,
        const Chain chain,
        const Data& data) const noexcept = 0;
    /// Throws std::out_of_range if the specified key does not exist
    virtual const opentxs::blockchain::crypto::Element& GetKey(
        const Key& id) const noexcept(false) = 0;
    /// Throws std::out_of_range if the specified account does not exist
    virtual const opentxs::blockchain::crypto::HD& HDSubaccount(
        const identifier::Nym& nymID,
        const Identifier& accountID) const noexcept(false) = 0;
    virtual PatternID IndexItem(const ReadView bytes) const noexcept = 0;
    virtual std::unique_ptr<const Tx> LoadTransactionBitcoin(
        const Txid& id) const noexcept = 0;
    virtual std::unique_ptr<const Tx> LoadTransactionBitcoin(
        const TxidHex& id) const noexcept = 0;
    virtual AccountData LookupAccount(const Identifier& id) const noexcept = 0;
    virtual ContactList LookupContacts(
        const std::string& address) const noexcept = 0;
    virtual ContactList LookupContacts(
        const Data& pubkeyHash) const noexcept = 0;
    virtual OTIdentifier NewHDSubaccount(
        const identifier::Nym& nymID,
        const BlockchainAccountType standard,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept = 0;
    OPENTXS_NO_EXPORT virtual OTIdentifier NewPaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const proto::HDPath& path,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual OTIdentifier NewPaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const ReadView& view,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual const identifier::Nym& Owner(
        const Identifier& accountID) const noexcept = 0;
    virtual const identifier::Nym& Owner(const Key& key) const noexcept = 0;
    /// Throws std::out_of_range if the specified account does not exist
    virtual const opentxs::blockchain::crypto::PaymentCode&
    PaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const Identifier& accountID) const noexcept(false) = 0;
    OPENTXS_NO_EXPORT virtual const opentxs::blockchain::crypto::PaymentCode&
    PaymentCodeSubaccount(
        const identifier::Nym& nymID,
        const opentxs::PaymentCode& local,
        const opentxs::PaymentCode& remote,
        const proto::HDPath& path,
        const Chain chain,
        const PasswordPrompt& reason) const noexcept(false) = 0;
    virtual bool ProcessTransaction(
        const Chain chain,
        const Tx& transaction,
        const PasswordPrompt& reason) const noexcept = 0;
    virtual OTIdentifier RecipientContact(const Key& key) const noexcept = 0;
    virtual bool Release(const Key key) const noexcept = 0;
    virtual OTIdentifier SenderContact(const Key& key) const noexcept = 0;
    virtual std::set<OTIdentifier> SubaccountList(
        const identifier::Nym& nymID,
        const Chain chain) const noexcept = 0;
    virtual bool Unconfirm(
        const Key key,
        const opentxs::blockchain::block::Txid& tx,
        const Time time = Clock::now()) const noexcept = 0;
    /// Throws std::runtime_error if chain is invalid
    virtual const opentxs::blockchain::crypto::Wallet& Wallet(
        const Chain chain) const noexcept(false) = 0;

    OPENTXS_NO_EXPORT virtual ~Blockchain() = default;

protected:
    Blockchain() noexcept = default;

private:
    Blockchain(const Blockchain&) = delete;
    Blockchain(Blockchain&&) = delete;
    Blockchain& operator=(const Blockchain&) = delete;
    Blockchain& operator=(Blockchain&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif  // OPENTXS_API_CLIENT_BLOCKCHAIN_HPP

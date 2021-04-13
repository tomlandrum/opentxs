// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "api/client/blockchain/BalanceNode.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/crypto/key/HD.hpp"  // IWYU pragma: keep
#include "opentxs/protobuf/BlockchainAccountData.pb.h"
#include "opentxs/protobuf/BlockchainActivity.pb.h"

// #define OT_METHOD
// "opentxs::api::client::blockchain::implementation::BalanceNode::"

namespace opentxs::api::client::blockchain::implementation
{
BalanceNode::BalanceNode(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const BalanceNodeType type,
    OTIdentifier&& id,
    const Revision revision,
    const std::vector<Activity>& unspent,
    const std::vector<Activity>& spent,
    Identifier& out) noexcept
    : api_(api)
    , parent_(parent)
    , chain_(parent_.Chain())
    , type_(type)
    , id_(std::move(id))
    , lock_()
    , revision_(revision)
    , unspent_(convert(unspent))
    , spent_(convert(spent))
{
    out.Assign(id_);
}

BalanceNode::BalanceNode(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const BalanceNodeType type,
    OTIdentifier&& id,
    Identifier& out) noexcept
    : BalanceNode(api, parent, type, std::move(id), 0, {}, {}, out)
{
}

BalanceNode::BalanceNode(
    const api::internal::Core& api,
    const internal::BalanceTree& parent,
    const BalanceNodeType type,
    const SerializedType& serialized,
    Identifier& out) noexcept(false)
    : BalanceNode(
          api,
          parent,
          type,
          api.Factory().Identifier(serialized.id()),
          serialized.revision(),
          convert(serialized.unspent()),
          convert(serialized.spent()),
          out)
{
    if (Translate(contact::internal::translate(serialized.chain())) != chain_) {
        throw std::runtime_error("Wrong account type");
    }
}

auto BalanceNode::UpdateElement(
    std::vector<ReadView>& pubkeyHashes) const noexcept -> void
{
    parent_.Parent().Parent().UpdateElement(pubkeyHashes);
}

auto BalanceNode::AssociateTransaction(
    const std::vector<Activity>& unspent,
    const std::vector<Activity>& spent,
    std::set<OTIdentifier>& contacts,
    const PasswordPrompt& reason) const noexcept -> bool
{
    auto lock = rLock{lock_};

    if (false == check_activity(lock, unspent, contacts, reason)) {

        return false;
    }

    for (const auto& [coin, key, value] : spent) {
        process_spent(lock, coin, key, value);
    }

    for (const auto& [coin, key, value] : unspent) {
        process_unspent(lock, coin, key, value);
    }

    return save(lock);
}

auto BalanceNode::Confirm(
    const Subchain type,
    const Bip32Index index,
    const Txid& tx) noexcept -> bool
{
    auto lock = rLock{lock_};

    try {
        auto& element = mutable_element(lock, type, index);

        if (element.Confirm(tx)) {
            confirm(lock, type, index);

            return save(lock);
        } else {

            return false;
        }
    } catch (...) {
        return false;
    }
}

auto BalanceNode::convert(Activity&& in) noexcept -> proto::BlockchainActivity
{
    const auto& [coin, key, value] = in;
    const auto& [txid, out] = coin;
    const auto& [account, chain, index] = key;
    proto::BlockchainActivity output{};
    output.set_version(ActivityVersion);
    output.set_txid(txid);
    output.set_output(out);
    output.set_amount(value);
    output.set_account(account);
    output.set_subchain(static_cast<std::uint32_t>(chain));
    output.set_index(index);

    return output;
}

auto BalanceNode::convert(const proto::BlockchainActivity& in) noexcept
    -> Activity
{
    Activity output{};
    auto& [coin, key, value] = output;
    auto& [txid, out] = coin;
    auto& [account, chain, index] = key;
    txid = in.txid();
    out = in.output();
    value = in.amount();
    account = in.account();
    chain = static_cast<Subchain>(in.subchain());
    index = in.index();

    return output;
}

auto BalanceNode::convert(const SerializedActivity& in) noexcept
    -> std::vector<Activity>
{
    auto output = std::vector<Activity>{};

    for (const auto& activity : in) { output.emplace_back(convert(activity)); }

    return output;
}

auto BalanceNode::convert(const std::vector<Activity>& in) noexcept
    -> internal::ActivityMap
{
    auto output = internal::ActivityMap{};

    for (const auto& [coin, key, value] : in) {
        output.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(coin),
            std::forward_as_tuple(key, value));
    }

    return output;
}

auto BalanceNode::IncomingTransactions(const Key& element) const noexcept
    -> std::set<std::string>
{
    auto lock = rLock{lock_};
    auto output = std::set<std::string>{};

    for (const auto& [coin, data] : unspent_) {
        const auto& [key, amount] = data;

        if (key == element) { output.emplace(coin.first); }
    }

    for (const auto& [coin, data] : spent_) {
        const auto& [key, amount] = data;

        if (key == element) { output.emplace(coin.first); }
    }

    return output;
}

void BalanceNode::init() noexcept { parent_.ClaimAccountID(id_->str(), this); }

// Due to asynchronous blockchain scanning, spends may be discovered out of
// order compared to receipts.
void BalanceNode::process_spent(
    const rLock& lock,
    const Coin& coin,
    const blockchain::Key key,
    const Amount value) const noexcept
{
    auto targetValue{value};

    if (0 < unspent_.count(coin)) {
        // Normal case
        targetValue = std::max(targetValue, unspent_.at(coin).second);
        unspent_.erase(coin);
    }

    // If the spend was found before the receipt, the value is not known
    spent_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(coin),
        std::forward_as_tuple(key, targetValue));
}

void BalanceNode::process_unspent(
    const rLock& lock,
    const Coin& coin,
    const blockchain::Key key,
    const Amount value) const noexcept
{
    if (0 == spent_.count(coin)) {
        // Normal case
        unspent_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(coin),
            std::forward_as_tuple(key, value));
    } else {
        // Spend was discovered out of order, so correct the value now
        auto& storedValue = spent_.at(coin).second;
        storedValue = std::max(storedValue, value);
    }
}

auto BalanceNode::serialize_common(
    const rLock&,
    proto::BlockchainAccountData& out) const noexcept -> void
{
    out.set_version(BlockchainAccountDataVersion);
    out.set_id(id_->str());
    out.set_revision(revision_.load());
    out.set_chain(contact::internal::translate(Translate(chain_)));

    for (const auto& [coin, data] : unspent_) {
        auto converted = Activity{coin, data.first, data.second};
        *out.add_unspent() = convert(std::move(converted));
    }

    for (const auto& [coin, data] : spent_) {
        auto converted = Activity{coin, data.first, data.second};
        *out.add_spent() = convert(std::move(converted));
    }
}

auto BalanceNode::SetContact(
    const Subchain type,
    const Bip32Index index,
    const Identifier& id) noexcept(false) -> bool
{
    auto lock = rLock{lock_};

    try {
        auto& element = mutable_element(lock, type, index);
        element.SetContact(id);

        return save(lock);
    } catch (...) {
        return false;
    }
}

auto BalanceNode::SetLabel(
    const Subchain type,
    const Bip32Index index,
    const std::string& label) noexcept(false) -> bool
{
    auto lock = rLock{lock_};

    try {
        auto& element = mutable_element(lock, type, index);
        element.SetLabel(label);

        return save(lock);
    } catch (...) {
        return false;
    }
}

auto BalanceNode::Unconfirm(
    const Subchain type,
    const Bip32Index index,
    const Txid& tx,
    const Time time) noexcept -> bool
{
    auto lock = rLock{lock_};

    try {
        auto& element = mutable_element(lock, type, index);

        if (element.Unconfirm(tx, time)) {
            unconfirm(lock, type, index);

            return save(lock);
        } else {

            return false;
        }
    } catch (...) {
        return false;
    }
}

auto BalanceNode::Unreserve(
    const Subchain type,
    const Bip32Index index) noexcept -> bool
{
    auto lock = rLock{lock_};

    try {
        auto& element = mutable_element(lock, type, index);

        if (element.Unreserve()) {

            return save(lock);
        } else {

            return false;
        }
    } catch (...) {
        return false;
    }
}
}  // namespace opentxs::api::client::blockchain::implementation

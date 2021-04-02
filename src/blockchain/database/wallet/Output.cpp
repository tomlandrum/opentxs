// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "blockchain/database/wallet/Output.hpp"  // IWYU pragma: associated

#include <robin_hood.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <map>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "blockchain/database/wallet/Proposal.hpp"
#include "blockchain/database/wallet/Subchain.hpp"
#include "blockchain/database/wallet/Transaction.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/blockchain/block/Outpoint.hpp"
#include "opentxs/blockchain/block/bitcoin/Input.hpp"
#include "opentxs/blockchain/block/bitcoin/Inputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/BlockchainTransactionOutput.pb.h"
#include "opentxs/protobuf/BlockchainWalletKey.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "util/Container.hpp"

#define OT_METHOD "opentxs::blockchain::database::Output::"

namespace std
{
using Outpoint = opentxs::blockchain::block::Outpoint;
using Position = opentxs::blockchain::block::Position;
using Identifier = opentxs::OTIdentifier;
using NymID = opentxs::OTNymID;

template <>
struct hash<Outpoint> {
    auto operator()(const Outpoint& data) const noexcept -> std::size_t
    {
        static const auto& api = opentxs::Context().Crypto().Hash();
        static const auto key = std::array<char, 16>{};
        auto out = std::size_t{};
        const auto bytes = data.Bytes();
        api.HMAC(
            opentxs::crypto::HashType::SipHash24,
            {key.data(), key.size()},
            {std::next(bytes.data(), 24), 12u},
            opentxs::preallocated(sizeof(out), &out));

        return out;
    }
};

template <>
struct hash<Position> {
    auto operator()(const Position& data) const noexcept -> std::size_t
    {
        auto out = std::size_t{};
        const auto& hash = data.second.get();
        std::memcpy(&out, hash.data(), std::min(sizeof(out), hash.size()));

        return out;
    }
};

template <>
struct hash<Identifier> {
    auto operator()(const opentxs::Identifier& data) const noexcept
        -> std::size_t
    {
        auto out = std::size_t{};
        std::memcpy(&out, data.data(), std::min(sizeof(out), data.size()));

        return out;
    }
};

template <>
struct hash<NymID> {
    auto operator()(const NymID& data) const noexcept -> std::size_t
    {
        static const auto hasher = hash<Identifier>{};

        return hasher(data.get());
    }
};
}  // namespace std

namespace opentxs::blockchain::database::wallet
{
struct Output::Imp {
    auto GetBalance() const noexcept -> Balance
    {
        auto lock = sLock{lock_};

        return get_balance(lock);
    }
    auto GetBalance(const identifier::Nym& owner) const noexcept -> Balance
    {
        auto lock = sLock{lock_};

        if (owner.empty()) { return {}; }

        return get_balance(lock, owner);
    }
    auto GetBalance(const identifier::Nym& owner, const NodeID& node)
        const noexcept -> Balance
    {
        auto lock = sLock{lock_};

        if (owner.empty() || node.empty()) { return {}; }

        return get_balance(lock, owner, node);
    }
    auto GetMutex() const noexcept -> std::shared_mutex& { return lock_; }
    auto GetOutputs(State type) const noexcept -> std::vector<UTXO>
    {
        auto lock = sLock{lock_};

        return get_outputs(lock, states(type), nullptr, nullptr, nullptr);
    }
    auto GetOutputs(const identifier::Nym& owner, State type) const noexcept
        -> std::vector<UTXO>
    {
        auto lock = sLock{lock_};

        if (owner.empty()) { return {}; }

        return get_outputs(lock, states(type), &owner, nullptr, nullptr);
    }
    auto GetOutputs(
        const identifier::Nym& owner,
        const Identifier& node,
        State type) const noexcept -> std::vector<UTXO>
    {
        auto lock = sLock{lock_};

        if (owner.empty() || node.empty()) { return {}; }

        return get_outputs(lock, states(type), &owner, &node, nullptr);
    }
    auto GetUnspentOutputs() const noexcept -> std::vector<UTXO>
    {
        static const auto blank = api_.Factory().Identifier();

        return GetUnspentOutputs(blank);
    }
    auto GetUnspentOutputs(const NodeID& id) const noexcept -> std::vector<UTXO>
    {
        auto lock = sLock{lock_};

        return get_unspent_outputs(lock, id);
    }

    auto AddConfirmedTransaction(
        const AccountID& account,
        const SubchainID& subchain,
        const block::Position& block,
        const std::size_t blockIndex,
        const std::vector<std::uint32_t> outputIndices,
        const block::bitcoin::Transaction& original) noexcept -> bool
    {
        auto lock = eLock{lock_};
        auto pCopy = original.clone();

        OT_ASSERT(pCopy);

        auto& copy = *pCopy;
        auto inputIndex = int{-1};

        for (const auto& input : copy.Inputs()) {
            const auto& outpoint = input.PreviousOutput();
            ++inputIndex;

            if (false == check_proposals(lock, outpoint, block, copy.ID())) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Error updating proposals")
                    .Flush();

                return false;
            }

            try {
                auto& serialized = find_output(lock, outpoint);
                const auto& [state, position, proto] = serialized;

                if (!copy.AssociatePreviousOutput(
                        blockchain_, inputIndex, proto)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error associating previous output to input")
                        .Flush();

                    return false;
                }

                if (false == change_state(
                                 lock,
                                 outpoint,
                                 serialized,
                                 TxoState::ConfirmedSpend,
                                 block)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error updating consumed output state")
                        .Flush();

                    return false;
                }
            } catch (...) {
            }

            // NOTE consider the case of parallel chain scanning where one
            // transaction spends inputs that belong to two different subchains.
            // The first subchain to find the transaction will recognize the
            // inputs belonging to itself but might miss the inputs belonging to
            // the other subchain if the other subchain's scanning process has
            // not yet discovered those outputs. This is fine. The other
            // scanning process will parse this transaction again and at that
            // point all inputs will be recognized. The only impact is that net
            // balance change of the transaction will underestimated temporarily
            // until scanning is complete for all subchains.
        }

        for (const auto& index : outputIndices) {
            const auto outpoint = Outpoint{copy.ID().Bytes(), index};
            const auto& output = copy.Outputs().at(index);

            OT_ASSERT((0 < output.Keys().size()));
            OT_ASSERT(outpoint.Index() == index);

            try {
                auto& serialized = find_output(lock, outpoint);

                if (false == change_state(
                                 lock,
                                 outpoint,
                                 serialized,
                                 TxoState::ConfirmedNew,
                                 block)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error updating created output state")
                        .Flush();

                    return false;
                }
            } catch (...) {
                if (false == create_state(
                                 lock,
                                 outpoint,
                                 TxoState::ConfirmedNew,
                                 block,
                                 output)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error created new output state")
                        .Flush();

                    return false;
                }
            }

            if (false == associate(lock, outpoint, account, subchain)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Error associating outpoint to subchain")
                    .Flush();

                return false;
            }

            for (const auto& key : output.Keys()) {
                const auto& owner = blockchain_.Owner(key);

                if (false == associate(lock, outpoint, owner)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error associating outpoint to nym")
                        .Flush();

                    return false;
                }
            }
        }

        const auto reason = api_.Factory().PasswordPrompt(
            "Save a received blockchain transaction");

        if (false == transactions_.Add(chain_, block, copy, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error adding transaction to database")
                .Flush();

            return false;
        }

        // NOTE do not call this function except for debugging: print(lock);
        blockchain_.UpdateBalance(chain_, get_balance(lock));

        for (const auto& [nym, balance] : get_balances(lock)) {
            blockchain_.UpdateBalance(nym, chain_, balance);
        }

        return true;
    }
    auto AddOutgoingTransaction(
        const Identifier& proposalID,
        const proto::BlockchainTransactionProposal& proposal,
        const block::bitcoin::Transaction& transaction) noexcept -> bool
    {
        auto lock = eLock{lock_};

        for (const auto& input : transaction.Inputs()) {
            const auto& outpoint = input.PreviousOutput();

            try {
                if (proposalID != proposal_reverse_index_.at(outpoint)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Incorrect proposal ID")
                        .Flush();

                    return false;
                }
            } catch (...) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Input spending")(
                    outpoint.str())(" not registered with a proposal")
                    .Flush();

                return false;
            }

            // NOTE it's not necessary to change the state of the spent outputs
            // because that was done when they were reserved for the proposal
        }

        auto index{-1};
        auto& pending = proposal_created_index_[proposalID];

        for (const auto& output : transaction.Outputs()) {
            ++index;

            if (0 == output.Keys().size()) {
                LogTrace(OT_METHOD)(__FUNCTION__)(": output ")(index)(
                    " belongs to someone else")
                    .Flush();

                continue;
            } else {
                LogTrace(OT_METHOD)(__FUNCTION__)(": output ")(index)(
                    " belongs to me")
                    .Flush();
            }

            const auto [it, added] = pending.emplace(
                transaction.ID().Bytes(), static_cast<std::uint32_t>(index));
            const auto& outpoint = *it;

            try {
                auto& serialized = find_output(lock, outpoint);

                if (false == change_state(
                                 lock,
                                 outpoint,
                                 serialized,
                                 TxoState::UnconfirmedNew,
                                 blank_)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error updating created output state")
                        .Flush();

                    return false;
                }
            } catch (...) {
                if (false == create_state(
                                 lock,
                                 outpoint,
                                 TxoState::UnconfirmedNew,
                                 blank_,
                                 output)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error creating new output state")
                        .Flush();

                    return false;
                }
            }

            for (const auto& key : output.Keys()) {
                const auto& owner = blockchain_.Owner(key);

                if (false == associate(lock, outpoint, key)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error associating output to subchain")
                        .Flush();

                    return false;
                }

                if (false == associate(lock, outpoint, owner)) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Error associating output to nym")
                        .Flush();

                    return false;
                }
            }
        }

        const auto reason = api_.Factory().PasswordPrompt(
            "Save an outgoing blockchain transaction");

        if (false == transactions_.Add(chain_, blank_, transaction, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error adding transaction to database")
                .Flush();

            return false;
        }

        print(lock);
        blockchain_.UpdateBalance(chain_, get_balance(lock));

        for (const auto& [nym, balance] : get_balances(lock)) {
            blockchain_.UpdateBalance(nym, chain_, balance);
        }

        return true;
    }
    auto CancelProposal(const Identifier& id) noexcept -> bool
    {
        auto lock = eLock{lock_};
        auto& reserved = proposal_spent_index_[id];
        auto& created = proposal_created_index_[id];

        for (const auto& id : reserved) {
            if (false == change_state(
                             lock,
                             id,
                             TxoState::UnconfirmedSpend,
                             TxoState::ConfirmedNew)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": failed to reclaim outpoint ")(id.str())
                    .Flush();

                return false;
            }

            proposal_reverse_index_.erase(id);
        }

        for (const auto& id : created) {
            if (false == change_state(
                             lock,
                             id,
                             TxoState::UnconfirmedNew,
                             TxoState::OrphanedNew)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": failed to orphan canceled outpoint ")(id.str())
                    .Flush();

                return false;
            }
        }

        proposal_spent_index_.erase(id);
        proposal_created_index_.erase(id);

        return proposals_.CancelProposal(id);
    }
    auto ReserveUTXO(
        const identifier::Nym& spender,
        const Identifier& id,
        const Spend policy) noexcept -> std::optional<UTXO>
    {
        // TODO implement smarter selection algorithms
        auto lock = eLock{lock_};
        auto output = std::optional<UTXO>{std::nullopt};
        const auto choose = [&](const auto outpoint) -> std::optional<UTXO> {
            auto& serialized = find_output(lock, outpoint);
            const auto& [state, position, data] = serialized;

            if (false == owns(spender, data)) { return std::nullopt; }

            auto output = std::make_optional<UTXO>(outpoint, data);
            const auto changed = change_state(
                lock, outpoint, serialized, TxoState::UnconfirmedSpend, blank_);

            OT_ASSERT(changed);

            proposal_spent_index_[id].emplace(outpoint);
            proposal_reverse_index_.emplace(outpoint, id);
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Reserving output ")(
                outpoint.str())
                .Flush();

            return output;
        };
        const auto select = [&](const auto& group) -> std::optional<UTXO> {
            for (const auto& outpoint : group) {
                auto utxo = choose(outpoint);

                if (utxo.has_value()) { return utxo; }
            }

            LogTrace(OT_METHOD)(__FUNCTION__)(
                ": No spendable outputs for this group")
                .Flush();

            return std::nullopt;
        };

        output = select(find_state(lock, TxoState::ConfirmedNew));

        if (output.has_value()) { return output; }

        if (Spend::UnconfirmedToo == policy) {
            output = select(find_state(lock, TxoState::UnconfirmedNew));

            if (output.has_value()) { return output; }
        }

        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": No spendable outputs for specified nym")
            .Flush();

        return output;
    }
    auto Rollback(
        const eLock& lock,
        const SubchainID& subchain,
        const block::Position& position) noexcept -> bool
    {
        // TODO rebroadcast transactions which have become unconfirmed
        const auto outpoints = [&] {
            auto out = std::vector<Outpoint>{};

            try {
                for (const auto& outpoint : position_index_.at(position)) {
                    if (belongs_to(lock, outpoint, subchain)) {
                        insert_sorted(out, outpoint);
                    }
                }
            } catch (...) {
            }

            return out;
        }();

        for (const auto& id : outpoints) {
            auto& serialized = find_output(lock, id);
            const auto state = [&]() -> std::optional<TxoState> {
                switch (std::get<0>(serialized)) {
                    case TxoState::ConfirmedNew:
                    case TxoState::OrphanedNew: {

                        return TxoState::UnconfirmedNew;
                    }
                    case TxoState::ConfirmedSpend:
                    case TxoState::OrphanedSpend: {

                        return TxoState::UnconfirmedSpend;
                    }
                    default: {

                        return std::nullopt;
                    }
                }
            }();

            if (state.has_value() &&
                (!change_state(
                    lock, id, serialized, state.value(), position))) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to update output state")
                    .Flush();

                return false;
            }

            const auto& txid = api_.Factory().Data(id.Txid());
            const auto& [opState, opPosition, data] = serialized;

            for (const auto& sKey : data.key()) {
                using Subchain = api::client::blockchain::Subchain;
                blockchain_.Unconfirm(
                    {sKey.subaccount(),
                     static_cast<Subchain>(sKey.subchain()),
                     sKey.index()},
                    txid);
            }

            if (false == transactions_.Rollback(position.first, txid)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to update transaction history")
                    .Flush();

                return false;
            }
        }

        return true;
    }

    Imp(const api::Core& api,
        const api::client::internal::Blockchain& blockchain,
        const blockchain::Type chain,
        const wallet::SubchainData& subchains,
        wallet::Proposal& proposals,
        wallet::Transaction& transactions) noexcept
        : api_(api)
        , blockchain_(blockchain)
        , chain_(chain)
        , subchains_(subchains)
        , proposals_(proposals)
        , transactions_(transactions)
        , blank_([&] {
            auto out = make_blank<block::Position>::value(api_);

            OT_ASSERT(0 < out.second->size());

            return out;
        }())
        , lock_()
        , outputs_()
        , account_index_()
        , nym_index_()
        , position_index_()
        , proposal_created_index_()
        , proposal_spent_index_()
        , proposal_reverse_index_()
        , state_index_()
        , subchain_index_()
    {
    }

private:
    using SubchainID = Identifier;
    using pSubchainID = OTIdentifier;
    using Outpoint = block::Outpoint;
    using Outpoints = std::set<Outpoint>;
    using TxoState = client::Wallet::TxoState;
    using Output = std::
        tuple<TxoState, block::Position, proto::BlockchainTransactionOutput>;
    using OutputMap =
        robin_hood::unordered_flat_map<Outpoint, std::unique_ptr<Output>>;
    using AccountIndex = std::map<OTIdentifier, Outpoints>;
    using NymIndex = std::map<OTNymID, Outpoints>;
    using PositionIndex = std::map<block::Position, Outpoints>;
    using ProposalIndex = std::map<OTIdentifier, Outpoints>;
    using ProposalReverseIndex = std::map<Outpoint, OTIdentifier>;
    using StateIndex = std::map<TxoState, Outpoints>;
    using SubchainIndex =
        robin_hood::unordered_flat_map<pSubchainID, Outpoints>;
    using NymBalances = std::map<OTNymID, Balance>;
    using KeyID = api::client::blockchain::Key;
    using States = std::vector<TxoState>;
    using Matches = std::vector<Outpoint>;

    const api::Core& api_;
    const api::client::internal::Blockchain& blockchain_;
    const blockchain::Type chain_;
    const wallet::SubchainData& subchains_;
    wallet::Proposal& proposals_;
    wallet::Transaction& transactions_;
    const block::Position blank_;
    mutable std::shared_mutex lock_;
    OutputMap outputs_;
    AccountIndex account_index_;
    NymIndex nym_index_;
    PositionIndex position_index_;
    ProposalIndex proposal_created_index_;
    ProposalIndex proposal_spent_index_;
    ProposalReverseIndex proposal_reverse_index_;
    StateIndex state_index_;
    SubchainIndex subchain_index_;

    static auto owns(
        const identifier::Nym& spender,
        const proto::BlockchainTransactionOutput& output) noexcept -> bool
    {
        const auto id = spender.str();

        for (const auto& key : output.key()) {
            if (key.nym() == id) { return true; }
        }

        if (0 == output.key_size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": No keys").Flush();
        }

        return false;
    }
    static auto states(TxoState in) noexcept -> States
    {
        static const auto all = States{
            TxoState::UnconfirmedNew,
            TxoState::UnconfirmedSpend,
            TxoState::ConfirmedNew,
            TxoState::ConfirmedSpend,
            TxoState::OrphanedNew,
            TxoState::OrphanedSpend,
        };

        if (TxoState::All == in) { return all; }

        return States{in};
    }

    auto belongs_to(
        const eLock& lock,
        const Outpoint& id,
        const SubchainID& subchain) const noexcept -> bool
    {
        const auto it1 = subchain_index_.find(subchain);

        if (subchain_index_.cend() == it1) { return false; }

        const auto& vector = it1->second;

        for (const auto& outpoint : vector) {
            if (id == outpoint) { return true; }
        }

        return false;
    }
    auto effective_position(
        const TxoState state,
        const block::Position& oldPos,
        const block::Position& newPos) const noexcept -> const block::Position&
    {
        switch (state) {
            case TxoState::UnconfirmedNew:
            case TxoState::UnconfirmedSpend: {

                return oldPos;
            }
            default: {

                return newPos;
            }
        }
    }
    template <typename LockType>
    auto find_account(const LockType& lock, const AccountID& id) const noexcept
        -> const Outpoints&
    {
        static const auto empty = Outpoints{};

        try {

            return account_index_.at(id);
        } catch (...) {

            return empty;
        }
    }
    template <typename LockType>
    auto find_nym(const LockType& lock, const identifier::Nym& id)
        const noexcept -> const Outpoints&
    {
        static const auto empty = Outpoints{};

        try {

            return nym_index_.at(id);
        } catch (...) {

            return empty;
        }
    }
    template <typename LockType>
    auto find_output(const LockType& lock, const Outpoint& id) const
        noexcept(false) -> const Output&
    {
        return *outputs_.at(id);
    }
    template <typename LockType>
    auto find_state(const LockType& lock, TxoState state) const noexcept
        -> const Outpoints&
    {
        static const auto empty = Outpoints{};

        try {

            return state_index_.at(state);
        } catch (...) {

            return empty;
        }
    }
    template <typename LockType>
    auto find_subchain(const LockType& lock, const NodeID& id) const noexcept
        -> const Outpoints&
    {
        static const auto empty = Outpoints{};

        try {

            return subchain_index_.at(id);
        } catch (...) {

            return empty;
        }
    }
    template <typename LockType>
    auto get_balance(const LockType& lock) const noexcept -> Balance
    {
        static const auto blank = api_.Factory().NymID();

        return get_balance<LockType>(lock, blank);
    }
    template <typename LockType>
    auto get_balance(const LockType& lock, const identifier::Nym& owner)
        const noexcept -> Balance
    {
        static const auto blank = api_.Factory().Identifier();

        return get_balance<LockType>(lock, owner, blank);
    }
    template <typename LockType>
    auto get_balance(
        const LockType& lock,
        const identifier::Nym& owner,
        const AccountID& account) const noexcept -> Balance
    {
        auto output = Balance{};
        auto& [confirmed, unconfirmed] = output;
        const auto* pNym = owner.empty() ? nullptr : &owner;
        const auto* pAcct = account.empty() ? nullptr : &account;
        auto cb = [&](const auto previous, const auto& outpoint) -> auto
        {
            const auto& [state, position, data] = find_output(lock, outpoint);

            return previous + data.value();
        };

        const auto unconfirmedSpendTotal = [&] {
            const auto txos =
                match(lock, {TxoState::UnconfirmedSpend}, pNym, pAcct, nullptr);

            return std::accumulate(
                txos.begin(), txos.end(), std::uint64_t{0}, cb);
        }();

        {
            const auto txos =
                match(lock, {TxoState::ConfirmedNew}, pNym, pAcct, nullptr);
            confirmed =
                unconfirmedSpendTotal +
                std::accumulate(txos.begin(), txos.end(), std::uint64_t{0}, cb);
        }

        {
            const auto txos =
                match(lock, {TxoState::UnconfirmedNew}, pNym, pAcct, nullptr);
            unconfirmed =
                std::accumulate(txos.begin(), txos.end(), confirmed, cb) -
                unconfirmedSpendTotal;
        }

        return output;
    }
    template <typename LockType>
    auto get_balances(const LockType& lock) const noexcept -> NymBalances
    {
        auto output = NymBalances{};

        for (const auto& [nym, outpoints] : nym_index_) {
            output[nym] = get_balance(lock, nym);
        }

        return output;
    }
    auto get_outputs(
        const sLock& lock,
        const States states,
        const identifier::Nym* owner,
        const AccountID* account,
        const NodeID* subchain) const noexcept -> std::vector<UTXO>
    {
        const auto matches = match(lock, states, owner, account, subchain);
        auto output = std::vector<UTXO>{};

        for (const auto& outpoint : matches) {
            const auto& [state, position, data] = find_output(lock, outpoint);
            output.emplace_back(outpoint, data);
        }

        return output;
    }
    auto get_unspent_outputs(const sLock& lock, const NodeID& id) const noexcept
        -> std::vector<UTXO>
    {
        const auto* pSub = id.empty() ? nullptr : &id;

        return get_outputs(
            lock,
            {TxoState::UnconfirmedNew,
             TxoState::ConfirmedNew,
             TxoState::UnconfirmedSpend},
            nullptr,
            nullptr,
            pSub);
    }
    template <typename LockType>
    auto has_account(
        const LockType& lock,
        const AccountID& id,
        const Outpoint& outpoint) const noexcept -> bool
    {
        return 0 < find_account<LockType>(lock, id).count(outpoint);
    }
    template <typename LockType>
    auto has_nym(
        const LockType& lock,
        const identifier::Nym& id,
        const Outpoint& outpoint) const noexcept -> bool
    {
        return 0 < find_nym<LockType>(lock, id).count(outpoint);
    }
    template <typename LockType>
    auto has_subchain(
        const LockType& lock,
        const NodeID& id,
        const Outpoint& outpoint) const noexcept -> bool
    {
        return 0 < find_subchain<LockType>(lock, id).count(outpoint);
    }
    template <typename LockType>
    auto match(
        const LockType& lock,
        const States states,
        const identifier::Nym* owner,
        const AccountID* account,
        const NodeID* subchain) const noexcept -> Matches
    {
        auto output = Matches{};
        const auto allSubs = (nullptr == subchain);
        const auto allAccts = (nullptr == account);
        const auto allNyms = (nullptr == owner);

        for (const auto state : states) {
            for (const auto& outpoint : find_state(lock, state)) {
                // NOTE if a more specific conditions is requested then it's
                // not necessary to test any more general conditions. A subchain
                // match implies an account match implies a nym match
                const auto goodSub =
                    allSubs || has_subchain(lock, *subchain, outpoint);
                const auto goodAcct = (!allSubs) || allAccts ||
                                      has_account(lock, *account, outpoint);
                const auto goodNym = (!allSubs) || (!allAccts) || allNyms ||
                                     has_nym(lock, *owner, outpoint);

                if (goodNym && goodAcct && goodSub) {
                    output.emplace_back(outpoint);
                }
            }
        }

        return output;
    }
    template <typename LockType>
    auto print(const LockType&) const noexcept -> void
    {
        struct Output {
            std::stringstream text_{};
            std::size_t total_{};
        };
        auto output = std::map<TxoState, Output>{};

        for (const auto& data : outputs_) {
            const auto& outpoint = data.first;
            const auto& [state, position, proto] = *data.second;
            auto& out = output[state];
            out.text_ << "\n * " << outpoint.str() << ' ';
            out.text_ << " value: " << std::to_string(proto.value());
            out.total_ += proto.value();
            using Position = block::bitcoin::Script::Position;
            const auto pScript = factory::BitcoinScript(
                chain_, proto.script(), Position::Output);

            OT_ASSERT(pScript);

            const auto& script = *pScript;
            out.text_ << ", type: ";
            using Pattern = block::bitcoin::Script::Pattern;

            switch (script.Type()) {
                case Pattern::PayToMultisig: {
                    out.text_ << "P2MS";
                } break;
                case Pattern::PayToPubkey: {
                    out.text_ << "P2PK";
                } break;
                case Pattern::PayToPubkeyHash: {
                    out.text_ << "P2PKH";
                } break;
                case Pattern::PayToScriptHash: {
                    out.text_ << "P2SH";
                } break;
                default: {
                    out.text_ << "unknown";
                }
            }
        }

        const auto& unconfirmed = output[TxoState::UnconfirmedNew];
        const auto& confirmed = output[TxoState::ConfirmedNew];
        const auto& pending = output[TxoState::UnconfirmedSpend];
        const auto& spent = output[TxoState::ConfirmedSpend];
        LogTrace(OT_METHOD)(__FUNCTION__)(": Instance ")(api_.Instance())(
            " TXO database contents:")
            .Flush();
        LogTrace(OT_METHOD)(__FUNCTION__)(": Unconfirmed available value: ")(
            unconfirmed.total_)(unconfirmed.text_.str())
            .Flush();
        LogTrace(OT_METHOD)(__FUNCTION__)(": Confirmed available value: ")(
            confirmed.total_)(confirmed.text_.str())
            .Flush();
        LogTrace(OT_METHOD)(__FUNCTION__)(": Unconfirmed spent value: ")(
            pending.total_)(pending.text_.str())
            .Flush();
        LogTrace(OT_METHOD)(__FUNCTION__)(": Confirmed spent value: ")(
            spent.total_)(spent.text_.str())
            .Flush();
    }

    auto associate(
        const eLock& lock,
        const Outpoint& outpoint,
        const KeyID& key) noexcept -> bool
    {
        const auto& [nodeID, subchain, index] = key;
        const auto accountID = api_.Factory().Identifier(nodeID);
        const auto subchainID = subchains_.GetSubchainID(accountID, subchain);

        return associate(lock, outpoint, accountID, subchainID);
    }
    auto associate(
        const eLock& lock,
        const Outpoint& outpoint,
        const AccountID& accountID,
        const SubchainID& subchainID) noexcept -> bool
    {
        OT_ASSERT(false == accountID.empty());
        OT_ASSERT(false == subchainID.empty());

        account_index_[accountID].emplace(outpoint);
        subchain_index_[subchainID].emplace(outpoint);

        return true;
    }
    auto associate(
        const eLock& lock,
        const Outpoint& outpoint,
        const identifier::Nym& nymID) noexcept -> bool
    {
        OT_ASSERT(false == nymID.empty());

        nym_index_[nymID].emplace(outpoint);

        return true;
    }
    // Only used by CancelProposal
    auto change_state(
        const eLock& lock,
        const Outpoint& id,
        const TxoState oldState,
        const TxoState newState) noexcept -> bool
    {
        try {
            auto& serialized = find_output(lock, id);
            const auto& [state, position, data] = serialized;

            if (state != oldState) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": incorrect state for outpoint ")(id.str())
                    .Flush();

                return false;
            }

            return change_state(lock, id, serialized, newState, blank_);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": outpoint ")(id.str())(
                " does not exist")
                .Flush();

            return false;
        }
    }
    auto change_state(
        const eLock& lock,
        const Outpoint& id,
        const TxoState newState,
        const block::Position newPosition) noexcept -> bool
    {
        try {
            auto& serialized = find_output(lock, id);

            return change_state(lock, id, serialized, newState, newPosition);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": outpoint ")(id.str())(
                " does not exist")
                .Flush();

            return false;
        }
    }
    auto change_state(
        const eLock& lock,
        const Outpoint& id,
        Output& serialized,
        const TxoState newState,
        const block::Position newPosition) noexcept -> bool
    {
        auto& [oldState, oldPosition, data] = serialized;
        const auto effective =
            effective_position(newState, oldPosition, newPosition);

        if (newState != oldState) {
            auto& from = state_index_[oldState];
            auto& to = state_index_[newState];
            to.insert(from.extract(id));
            oldState = newState;
        }

        if (effective != oldPosition) {
            auto& from = position_index_[oldPosition];
            auto& to = position_index_[effective];
            to.insert(from.extract(id));
            oldPosition = effective;
        }

        return true;
    }
    auto check_proposals(
        const eLock& lock,
        const Outpoint& outpoint,
        const block::Position& block,
        const block::Txid& txid) noexcept -> bool

    {
        if (0 == proposal_reverse_index_.count(outpoint)) { return true; }

        auto proposalID{proposal_reverse_index_.at(outpoint)};

        if (0 < proposal_created_index_.count(proposalID)) {
            auto& created = proposal_created_index_.at(proposalID);

            for (const auto& newOutpoint : created) {
                const auto rhs = api_.Factory().Data(newOutpoint.Txid());

                if (txid != rhs) {
                    const auto changed = change_state(
                        lock, outpoint, TxoState::OrphanedNew, block);

                    if (false == changed) {
                        LogOutput(OT_METHOD)(__FUNCTION__)(
                            ": Failed to update txo state")
                            .Flush();

                        return false;
                    }
                }
            }

            proposal_created_index_.erase(proposalID);
        }

        if (0 < proposal_spent_index_.count(proposalID)) {
            auto& spent = proposal_spent_index_.at(proposalID);

            for (const auto& spentOutpoint : spent) {
                proposal_reverse_index_.erase(spentOutpoint);
            }

            proposal_spent_index_.erase(proposalID);
        }

        return proposals_.FinishProposal(proposalID);
    }
    auto create_state(
        const eLock& lock,
        const Outpoint& id,
        const TxoState state,
        const block::Position position,
        const block::bitcoin::Output& output) noexcept -> bool
    {
        if (0 < outputs_.count(id)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Outpoint already exists in db")
                .Flush();

            return false;
        }

        const auto& effective = effective_position(state, blank_, position);

        {
            auto data = block::bitcoin::Output::SerializeType{};

            if (false == output.Serialize(blockchain_, data)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to serialize output")
                    .Flush();

                return false;
            }

            outputs_.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(std::make_unique<Output>(
                    state, effective, std::move(data))));
        }

        state_index_[state].emplace(id);
        position_index_[effective].emplace(id);

        return true;
    }
    auto find_output(const eLock& lock, const Outpoint& id) noexcept(false)
        -> Output&
    {
        return *outputs_.at(id);
    }
};

Output::Output(
    const api::Core& api,
    const api::client::internal::Blockchain& blockchain,
    const blockchain::Type chain,
    const wallet::SubchainData& subchains,
    wallet::Proposal& proposals,
    wallet::Transaction& transactions) noexcept
    : imp_(std::make_unique<
           Imp>(api, blockchain, chain, subchains, proposals, transactions))
{
    OT_ASSERT(imp_);
}

auto Output::AddConfirmedTransaction(
    const AccountID& account,
    const SubchainID& subchain,
    const block::Position& block,
    const std::size_t blockIndex,
    const std::vector<std::uint32_t> outputIndices,
    const block::bitcoin::Transaction& transaction) noexcept -> bool
{
    return imp_->AddConfirmedTransaction(
        account, subchain, block, blockIndex, outputIndices, transaction);
}

auto Output::AddOutgoingTransaction(
    const Identifier& proposalID,
    const proto::BlockchainTransactionProposal& proposal,
    const block::bitcoin::Transaction& transaction) noexcept -> bool
{
    return imp_->AddOutgoingTransaction(proposalID, proposal, transaction);
}

auto Output::CancelProposal(const Identifier& id) noexcept -> bool
{
    return imp_->CancelProposal(id);
}

auto Output::GetBalance() const noexcept -> Balance
{
    return imp_->GetBalance();
}

auto Output::GetBalance(const identifier::Nym& owner) const noexcept -> Balance
{
    return imp_->GetBalance(owner);
}

auto Output::GetBalance(const identifier::Nym& owner, const NodeID& node)
    const noexcept -> Balance
{
    return imp_->GetBalance(owner, node);
}

auto Output::GetOutputs(State type) const noexcept -> std::vector<UTXO>
{
    return imp_->GetOutputs(type);
}

auto Output::GetOutputs(const identifier::Nym& owner, State type) const noexcept
    -> std::vector<UTXO>
{
    return imp_->GetOutputs(owner, type);
}

auto Output::GetOutputs(
    const identifier::Nym& owner,
    const NodeID& node,
    State type) const noexcept -> std::vector<UTXO>
{
    return imp_->GetOutputs(owner, node, type);
}

auto Output::GetMutex() const noexcept -> std::shared_mutex&
{
    return imp_->GetMutex();
}

auto Output::GetUnspentOutputs() const noexcept -> std::vector<UTXO>
{
    return imp_->GetUnspentOutputs();
}

auto Output::GetUnspentOutputs(const NodeID& balanceNode) const noexcept
    -> std::vector<UTXO>
{
    return imp_->GetUnspentOutputs(balanceNode);
}

auto Output::ReserveUTXO(
    const identifier::Nym& spender,
    const Identifier& proposal,
    const Spend policy) noexcept -> std::optional<UTXO>
{
    return imp_->ReserveUTXO(spender, proposal, policy);
}

auto Output::Rollback(
    const eLock& lock,
    const SubchainID& subchain,
    const block::Position& position) noexcept -> bool
{
    return imp_->Rollback(lock, subchain, position);
}

Output::~Output() = default;
}  // namespace opentxs::blockchain::database::wallet

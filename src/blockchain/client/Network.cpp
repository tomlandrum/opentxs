// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "blockchain/client/Network.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "blockchain/client/network/SyncServer.hpp"
#include "internal/blockchain/Params.hpp"
#include "internal/blockchain/client/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/block/Header.hpp"
#include "opentxs/blockchain/block/bitcoin/Block.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/blockchain/sync/Base.hpp"
#include "opentxs/network/blockchain/sync/Data.hpp"
#include "opentxs/network/blockchain/sync/State.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/protobuf/BlockchainTransactionProposal.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedNotification.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedOutput.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/PaymentCode.pb.h"

#define OT_METHOD "opentxs::blockchain::client::implementation::Network::"

namespace opentxs::blockchain::client::implementation
{
constexpr auto proposal_version_ = VersionNumber{1};
constexpr auto notification_version_ = VersionNumber{1};
constexpr auto output_version_ = VersionNumber{1};

struct NullWallet final : public internal::Wallet {
    const api::Core& api_;

    auto ConstructTransaction(const proto::BlockchainTransactionProposal&)
        const noexcept -> std::future<block::pTxid> final
    {
        auto promise = std::promise<block::pTxid>{};
        promise.set_value(api_.Factory().Data());

        return promise.get_future();
    }
    auto GetBalance() const noexcept -> Balance final { return {}; }
    auto GetBalance(const identifier::Nym&) const noexcept -> Balance final
    {
        return {};
    }
    auto GetBalance(const identifier::Nym&, const Identifier&) const noexcept
        -> Balance final
    {
        return {};
    }
    auto GetOutputs(TxoState) const noexcept -> std::vector<UTXO> final
    {
        return {};
    }
    auto GetOutputs(const identifier::Nym&, TxoState) const noexcept
        -> std::vector<UTXO> final
    {
        return {};
    }
    auto GetOutputs(const identifier::Nym&, const Identifier&, TxoState)
        const noexcept -> std::vector<UTXO> final
    {
        return {};
    }
    auto Init() noexcept -> void final {}
    auto Shutdown() noexcept -> std::shared_future<void> final
    {
        auto promise = std::promise<void>{};
        promise.set_value();

        return promise.get_future();
    }

    NullWallet(const api::Core& api)
        : api_(api)
    {
    }
    NullWallet(const NullWallet&) = delete;
    NullWallet(NullWallet&&) = delete;
    auto operator=(const NullWallet&) -> NullWallet& = delete;
    auto operator=(NullWallet&&) -> NullWallet& = delete;

    ~NullWallet() final = default;
};

Network::Network(
    const api::Core& api,
    const api::client::internal::Blockchain& blockchain,
    const Type type,
    const internal::Config& config,
    const std::string& seednode,
    const std::string& syncEndpoint) noexcept
    : Worker(api, std::chrono::seconds(0))
    , shutdown_sender_(api.ZeroMQ(), shutdown_endpoint())
    , database_p_(factory::BlockchainDatabase(
          api,
          blockchain,
          *this,
          blockchain.BlockchainDB(),
          type))
    , config_(config)
    , header_p_(factory::HeaderOracle(api, *database_p_, type))
    , block_p_(factory::BlockOracle(
          api,
          *this,
          *header_p_,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , filter_p_(factory::BlockchainFilterOracle(
          api,
          blockchain,
          config_,
          *this,
          *header_p_,
          *block_p_,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , peer_p_(factory::BlockchainPeerManager(
          api,
          config_,
          *this,
          *header_p_,
          *filter_p_,
          *block_p_,
          *database_p_,
          type,
          database_p_->BlockPolicy(),
          seednode,
          shutdown_sender_.endpoint_))
    , wallet_p_([&]() -> std::unique_ptr<blockchain::client::internal::Wallet> {
        if (config_.disable_wallet_) {

            return std::make_unique<NullWallet>(api);
        } else {
            return factory::BlockchainWallet(
                api,
                blockchain,
                *this,
                *database_p_,
                type,
                shutdown_sender_.endpoint_);
        }
    }())
    , blockchain_(blockchain)
    , chain_(type)
    , database_(*database_p_)
    , filters_(*filter_p_)
    , header_(*header_p_)
    , peer_(*peer_p_)
    , block_(*block_p_)
    , wallet_(*wallet_p_)
    , parent_(blockchain)
    , sync_endpoint_(syncEndpoint)
    , sync_server_([&] {
        if (config_.provide_sync_server_) {
            return std::make_unique<SyncServer>(
                api_,
                database_,
                header_,
                filters_,
                *this,
                chain_,
                filters_.DefaultType(),
                shutdown_sender_.endpoint_,
                sync_endpoint_);
        } else {
            return std::unique_ptr<SyncServer>{};
        }
    }())
    , local_chain_height_(0)
    , remote_chain_height_(
          params::Data::Chains().at(chain_).checkpoint_.height_)
    , waiting_for_headers_(Flag::Factory(false))
    , headers_requested_(Clock::now())
    , headers_received_()
    , work_promises_()
    , state_(State::UpdatingHeaders)
    , init_promise_()
    , init_(init_promise_.get_future())
{
    OT_ASSERT(database_p_);
    OT_ASSERT(filter_p_);
    OT_ASSERT(header_p_);
    OT_ASSERT(peer_p_);
    OT_ASSERT(block_p_);
    OT_ASSERT(wallet_p_);

    database_.SetDefaultFilterType(filters_.DefaultType());
    header_.Init();
    init_executor({api_.Endpoints().InternalBlockchainFilterUpdated(chain_)});
    LogVerbose(config_.print()).Flush();
}

auto Network::AddBlock(
    const std::shared_ptr<const block::bitcoin::Block> pBlock) const noexcept
    -> bool
{
    if (!pBlock) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": invalid ")(DisplayString(chain_))(
            " block")
            .Flush();

        return false;
    }

    const auto& block = *pBlock;

    try {
        const auto bytes = [&] {
            auto output = Space{};

            if (false == block.Serialize(writer(output))) {
                throw std::runtime_error("Serialization error");
            }

            return output;
        }();
        block_.SubmitBlock(reader(bytes));
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to serialize ")(
            DisplayString(chain_))(" block")
            .Flush();

        return false;
    }

    const auto& id = block.ID();

    if (std::future_status::ready !=
        block_.LoadBitcoin(id).wait_for(std::chrono::seconds(10))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": invalid ")(DisplayString(chain_))(
            " block")
            .Flush();

        return false;
    }

    if (false == filters_.ProcessBlock(block)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to index ")(
            DisplayString(chain_))(" block")
            .Flush();

        return false;
    }

    if (false == header_.AddHeader(block.Header().clone())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to process ")(
            DisplayString(chain_))(" header")
            .Flush();

        return false;
    }

    return peer_.BroadcastBlock(block);
}

auto Network::AddPeer(const p2p::Address& address) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.AddPeer(address);
}

auto Network::BroadcastTransaction(
    const block::bitcoin::Transaction& tx) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.BroadcastTransaction(tx);
}

auto Network::Connect() noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.Connect();
}

auto Network::Disconnect() noexcept -> bool
{
    // TODO

    return false;
}

auto Network::FeeRate() const noexcept -> Amount
{
    // TODO the hardcoded default fee rate should be a fallback only
    // if there is no better data available.

    return params::Data::Chains().at(chain_).default_fee_rate_;
}

auto Network::GetConfirmations(const std::string& txid) const noexcept
    -> ChainHeight
{
    // TODO

    return -1;
}

auto Network::GetPeerCount() const noexcept -> std::size_t
{
    if (false == running_.get()) { return false; }

    return peer_.GetPeerCount();
}

auto Network::init() noexcept -> void
{
    local_chain_height_.store(header_.BestChain().first);

    {
        const auto best = database_.CurrentBest();

        OT_ASSERT(best);

        const auto position = best->Position();
        LogVerbose(DisplayString(chain_))(" chain initialized with best hash ")(
            position.second->asHex())(" at height ")(position.first)
            .Flush();
    }

    peer_.init();
    init_promise_.set_value();
    trigger();
}

auto Network::is_synchronized_blocks() const noexcept -> bool
{
    return block_.Tip().first >= this->target();
}

auto Network::is_synchronized_filters() const noexcept -> bool
{
    const auto target = this->target();
    const auto progress = filters_.Tip(filters_.DefaultType()).first;

    return (progress >= target) || config_.use_sync_server_;
}

auto Network::is_synchronized_headers() const noexcept -> bool
{
    const auto target = remote_chain_height_.load();
    const auto progress = local_chain_height_.load();

    return (progress >= target) || config_.use_sync_server_;
}

auto Network::is_synchronized_sync_server() const noexcept -> bool
{
    if (sync_server_) {

        return sync_server_->Tip().first >= this->target();
    } else {

        return false;
    }
}

auto Network::JobReady(const internal::PeerManager::Task type) const noexcept
    -> void
{
    if (peer_p_) { peer_.JobReady(type); }
}

auto Network::Listen(const p2p::Address& address) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.Listen(address);
}

auto Network::pipeline(zmq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    init_.get();
    const auto body = in.Body();

    OT_ASSERT(0 < body.size());

    const auto task = [&] {
        try {

            return body.at(0).as<Task>();
        } catch (...) {

            OT_FAIL;
        }
    }();

    switch (task) {
        case Task::SubmitBlock: {
            process_block(in);
        } break;
        case Task::SubmitBlockHeader: {
            process_header(in);
            [[fallthrough]];
        }
        case Task::StateMachine: {
            do_work();
        } break;
        case Task::FilterUpdate: {
            process_filter_update(in);
        } break;
        case Task::SyncReply:
        case Task::SyncNewBlock: {
            process_sync_data(in);
        } break;
        case Task::Heartbeat: {
            block_.Heartbeat();
            filters_.Heartbeat();
            peer_.Heartbeat();

            if (sync_server_) { sync_server_->Heartbeat(); }

            do_work();
        } break;
        case Task::Shutdown: {
            shutdown(shutdown_promise_);
        } break;
        default: {
            OT_FAIL;
        }
    }
}

auto Network::process_block(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    if (2 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid block").Flush();

        return;
    }

    block_.SubmitBlock(body.at(1).Bytes());
}

auto Network::process_filter_update(network::zeromq::Message& in) noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    OT_ASSERT(2 < body.size());

    const auto height = body.at(2).as<block::Height>();
    const auto target = this->target();

    {
        const auto progress =
            (0 == target) ? double{0}
                          : ((double(height) / double(target)) * double{100});
        auto display = std::stringstream{};
        display << std::setprecision(3) << progress << "%";

        if (false == config_.disable_wallet_) {
            LogDetail(DisplayString(chain_))(" chain sync progress: ")(height)(
                " of ")(target)(" (")(display.str())(")")
                .Flush();
        }
    }

    blockchain_.ReportProgress(chain_, height, target);
}

auto Network::process_header(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    waiting_for_headers_->Off();
    headers_received_ = Clock::now();
    auto promise = int{};
    auto input = std::vector<ReadView>{};

    {
        const auto body = in.Body();

        if (2 > body.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

            return;
        }

        // NOTE can not use std::prev on frame iterators
        auto end = std::next(body.begin(), body.size() - 1u);
        std::transform(
            std::next(body.begin()),
            end,
            std::back_inserter(input),
            [](const auto& frame) { return frame.Bytes(); });
        const auto& promiseFrame = *end;
        promise = promiseFrame.as<int>();
    }

    auto headers = std::vector<std::unique_ptr<block::Header>>{};

    for (const auto& header : input) {
        headers.emplace_back(instantiate_header(header));
    }

    if (false == headers.empty()) { header_.AddHeaders(headers); }

    work_promises_.clear(promise);
}

auto Network::process_sync_data(network::zeromq::Message& in) noexcept -> void
{
    const auto sync = opentxs::network::blockchain::sync::Factory(api_, in);
    const auto& data = sync->asData();

    {
        const auto& state = data.State();

        if (state.Chain() != chain_) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong chain").Flush();

            return;
        }

        remote_chain_height_.store(
            std::max(state.Position().first, remote_chain_height_.load()));
    }

    auto prior = block::BlankHash();
    auto hashes = std::vector<block::pHash>{};
    const auto accepted = header_.ProcessSyncData(prior, hashes, data);

    if (0u == accepted) { return; }

    const auto& blocks = data.Blocks();

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Accepted ")(accepted)(" of ")(
        blocks.size())(" headers")
        .Flush();
    filters_.ProcessSyncData(prior, hashes, data);
}

auto Network::RequestBlock(const block::Hash& block) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestBlock(block);
}

auto Network::RequestBlocks(const std::vector<ReadView>& hashes) const noexcept
    -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestBlocks(hashes);
}

auto Network::SendToAddress(
    const opentxs::identifier::Nym& sender,
    const std::string& address,
    const Amount amount,
    const std::string& memo) const noexcept -> PendingOutgoing
{
    const auto pNym = api_.Wallet().Nym(sender);

    if (!pNym) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid sender ")(sender.str())
            .Flush();

        return {};
    }

    const auto [data, style, chains, supported] =
        blockchain_.DecodeAddress(address);

    if ((0 == chains.count(chain_)) || (!supported)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Address ")(address)(
            " not valid for ")(DisplayString(chain_))
            .Flush();

        return {};
    }

    auto id = api_.Factory().Identifier();
    id->Randomize(32);
    auto proposal = proto::BlockchainTransactionProposal{};
    proposal.set_version(proposal_version_);
    proposal.set_id(id->str());
    proposal.set_initiator(sender.data(), sender.size());
    proposal.set_expires(
        Clock::to_time_t(Clock::now() + std::chrono::hours(1)));
    proposal.set_memo(memo);
    using Style = api::client::blockchain::AddressStyle;
    auto& output = *proposal.add_output();
    output.set_version(output_version_);
    output.set_amount(amount);

    switch (style) {
        case Style::P2WPKH: {
            output.set_segwit(true);
            [[fallthrough]];
        }
        case Style::P2PKH: {
            output.set_pubkeyhash(data->str());
        } break;
        case Style::P2WSH: {
            output.set_segwit(true);
            [[fallthrough]];
        }
        case Style::P2SH: {
            output.set_scripthash(data->str());
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported address type")
                .Flush();

            return {};
        }
    }

    return wallet_.ConstructTransaction(proposal);
}

auto Network::SendToPaymentCode(
    const opentxs::identifier::Nym& sender,
    const std::string& recipient,
    const Amount amount,
    const std::string& memo) const noexcept -> PendingOutgoing
{
    return SendToPaymentCode(
        sender, api_.Factory().PaymentCode(recipient), amount, memo);
}

auto Network::SendToPaymentCode(
    const opentxs::identifier::Nym& nymID,
    const PaymentCode& recipient,
    const Amount amount,
    const std::string& memo) const noexcept -> PendingOutgoing
{
    try {
        const auto pNym = api_.Wallet().Nym(nymID);

        if (!pNym) {
            throw std::runtime_error{std::string{"Invalid nym "} + nymID.str()};
        }

        const auto& nym = *pNym;
        const auto sender = api_.Factory().PaymentCode(nym.PaymentCode());

        if (0 == sender->Version()) {
            throw std::runtime_error{"Invalid sender payment code"};
        }

        if (3 > recipient.Version()) {
            throw std::runtime_error{
                "Sending to version 1 payment codes not yet supported"};
        }

        const auto path = [&] {
            auto out = proto::HDPath{};

            if (false == nym.PaymentCodePath(out)) {
                throw std::runtime_error{
                    "Failed to obtain payment code HD path"};
            }

            return out;
        }();
        const auto reason = api_.Factory().PasswordPrompt(
            std::string{"Sending a transaction to "} + recipient.asBase58());
        const auto& account = blockchain_.PaymentCodeSubaccount(
            nymID, sender, recipient, path, chain_, reason);
        using Subchain = api::client::blockchain::Subchain;
        constexpr auto subchain{Subchain::Outgoing};
        const auto index = account.Reserve(subchain, reason);

        if (false == index.has_value()) {
            throw std::runtime_error{"Failed to allocate next key"};
        }

        const auto pKey = [&] {
            const auto& element =
                account.BalanceElement(subchain, index.value());
            auto out = element.Key();

            if (!out) { throw std::runtime_error{"Failed to instantiate key"}; }

            return out;
        }();
        const auto& key = *pKey;
        const auto proposal = [&] {
            auto out = proto::BlockchainTransactionProposal{};
            out.set_version(proposal_version_);
            out.set_id([&] {
                auto id = api_.Factory().Identifier();
                id->Randomize(32);

                return id->str();
            }());
            out.set_initiator(nymID.data(), nymID.size());
            out.set_expires(
                Clock::to_time_t(Clock::now() + std::chrono::hours(1)));
            out.set_memo(memo);
            auto& txout = *out.add_output();
            txout.set_version(output_version_);
            txout.set_amount(amount);
            txout.set_index(index.value());
            txout.set_paymentcodechannel(account.ID().str());
            const auto pubkey = api_.Factory().Data(key.PublicKey());
            LogVerbose(OT_METHOD)(__FUNCTION__)(": ")(
                " using derived public key ")(pubkey->asHex())(" at index ")(
                index.value())(" for outgoing transaction")
                .Flush();
            txout.set_pubkey(pubkey->str());

            if (account.IsNotified()) {
                // TODO preemptive notifications go here
            } else {
                auto serialize =
                    [&](const PaymentCode& pc) -> proto::PaymentCode {
                    auto proto = proto::PaymentCode{};
                    pc.Serialize(proto);
                    return proto;
                };
                auto& notif = *out.add_notification();
                notif.set_version(notification_version_);
                *notif.mutable_sender() = serialize(sender.get());
                *notif.mutable_path() = path;
                *notif.mutable_recipient() = serialize(recipient);
            }

            return out;
        }();

        return wallet_.ConstructTransaction(proposal);
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Network::shutdown(std::promise<void>& promise) noexcept -> void
{
    init_.get();

    if (running_->Off()) {
        shutdown_sender_.Activate();
        wallet_.Shutdown().get();

        if (sync_server_) { sync_server_->Shutdown().get(); }

        peer_.Shutdown().get();
        filters_.Shutdown();
        block_.Shutdown().get();
        shutdown_sender_.Close();

        try {
            promise.set_value();
        } catch (...) {
        }
    }
}

auto Network::shutdown_endpoint() noexcept -> std::string
{
    return std::string{"inproc://"} + Identifier::Random()->str();
}

auto Network::state_machine() noexcept -> bool
{
    if (false == running_.get()) { return false; }

    LogDebug(OT_METHOD)(__FUNCTION__)(": Starting state machine for ")(
        DisplayString(chain_))
        .Flush();
    state_machine_headers();

    switch (state_.load()) {
        case State::UpdatingHeaders: {
            if (is_synchronized_headers()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " header oracle is synchronized")
                    .Flush();
                using Policy = api::client::blockchain::BlockStorage;

                if (Policy::All == database_.BlockPolicy()) {
                    state_transition_blocks();
                } else {
                    state_transition_filters();
                }
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" header oracle")
                    .Flush();
            }
        } break;
        case State::UpdatingBlocks: {
            if (is_synchronized_blocks()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " block oracle is synchronized")
                    .Flush();
                state_transition_filters();
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" block oracle")
                    .Flush();

                break;
            }
        } break;
        case State::UpdatingFilters: {
            if (is_synchronized_filters()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " filter oracle is synchronized")
                    .Flush();

                if (config_.provide_sync_server_) {
                    state_transition_sync();
                } else {
                    state_transition_normal();
                }
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" filter oracle")
                    .Flush();

                break;
            }
        } break;
        case State::UpdatingSyncData: {
            if (is_synchronized_sync_server()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
                    " sync server is synchronized")
                    .Flush();
                state_transition_normal();
            } else {
                LogDebug(OT_METHOD)(__FUNCTION__)(": updating ")(
                    DisplayString(chain_))(" sync server")
                    .Flush();

                break;
            }
        } break;
        case State::Normal:
        default: {
        }
    }

    LogDebug(OT_METHOD)(__FUNCTION__)(": Completed state machine for ")(
        DisplayString(chain_))
        .Flush();

    return false;
}

auto Network::state_machine_headers() noexcept -> void
{
    constexpr auto limit = std::chrono::seconds{20};
    constexpr auto rateLimit = std::chrono::seconds{1};
    const auto interval = Clock::now() - headers_requested_;
    const auto requestHeaders = [&] {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Requesting ")(DisplayString(
            chain_))(" block headers from all connected peers (instance ")(
            api_.Instance())(")")
            .Flush();
        waiting_for_headers_->On();
        peer_.RequestHeaders();
        headers_requested_ = Clock::now();
    };

    if (waiting_for_headers_.get()) {
        if (interval < limit) { return; }

        LogDetail(OT_METHOD)(__FUNCTION__)(": ")(DisplayString(chain_))(
            " headers not received before timeout (instance ")(api_.Instance())(
            ")")
            .Flush();
        requestHeaders();
    } else if ((false == is_synchronized_headers()) && (interval > rateLimit)) {
        requestHeaders();
    } else if ((Clock::now() - headers_received_) >= limit) {
        requestHeaders();
    }
}

auto Network::state_transition_blocks() noexcept -> void
{
    block_.Init();
    state_.store(State::UpdatingBlocks);
}

auto Network::state_transition_filters() noexcept -> void
{
    filters_.Start();
    state_.store(State::UpdatingFilters);
}

auto Network::state_transition_normal() noexcept -> void
{
    if (false == config_.disable_wallet_) { wallet_.Init(); }

    state_.store(State::Normal);
}

auto Network::state_transition_sync() noexcept -> void
{
    OT_ASSERT(sync_server_);

    sync_server_->Start();
    state_.store(State::UpdatingSyncData);
}

auto Network::Submit(network::zeromq::Message& work) const noexcept -> void
{
    if (false == running_.get()) { return; }

    pipeline_->Push(work);
}

auto Network::Track(network::zeromq::Message& work) const noexcept
    -> std::future<void>
{
    if (false == running_.get()) {
        auto promise = std::promise<void>{};
        promise.set_value();

        return promise.get_future();
    }

    auto [index, future] = work_promises_.get();
    work.AddFrame(index);
    pipeline_->Push(work);

    return std::move(future);
}

auto Network::target() const noexcept -> block::Height
{
    return std::max(local_chain_height_.load(), remote_chain_height_.load());
}

auto Network::UpdateHeight(const block::Height height) const noexcept -> void
{
    if (false == running_.get()) { return; }

    remote_chain_height_.store(std::max(height, remote_chain_height_.load()));
    trigger();
}

auto Network::UpdateLocalHeight(const block::Position position) const noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto& [height, hash] = position;
    LogNormal(DisplayString(chain_))(" block header chain updated to hash ")(
        hash->asHex())(" at height ")(height)
        .Flush();
    local_chain_height_.store(height);
    trigger();
}

Network::~Network() { Shutdown().get(); }
}  // namespace opentxs::blockchain::client::implementation

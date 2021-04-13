// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "api/client/Pair.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <type_traits>

#include "core/StateMachine.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/Factory.hpp"
#include "internal/core/contract/peer/Peer.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/api/client/Pair.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactSection.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/peer/ConnectionInfoType.hpp"
#include "opentxs/core/contract/peer/PeerRequestType.hpp"
#include "opentxs/core/contract/peer/SecretType.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/network/zeromq/socket/Subscribe.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/PairEvent.pb.h"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/PendingBailment.pb.h"
#include "opentxs/protobuf/ZMQEnums.pb.h"
#include "opentxs/protobuf/verify/PeerReply.hpp"
#include "opentxs/protobuf/verify/PeerRequest.hpp"

#define MINIMUM_UNUSED_BAILMENTS 3

#define SHUTDOWN()                                                             \
    {                                                                          \
        if (!running_) { return; }                                             \
                                                                               \
        Sleep(std::chrono::milliseconds(50));                                  \
    }

#define OT_METHOD "opentxs::api::client::implementation::Pair::"

template class opentxs::Pimpl<opentxs::network::zeromq::socket::Publish>;

namespace opentxs::factory
{
auto PairAPI(const Flag& running, const api::client::internal::Manager& client)
    -> api::client::internal::Pair*
{
    return new api::client::implementation::Pair(running, client);
}
}  // namespace opentxs::factory

namespace opentxs::api::client::implementation
{
Pair::Pair(const Flag& running, const api::client::internal::Manager& client)
    : opentxs::api::client::Pair()
    , internal::Pair()
    , Lockable()
    , StateMachine([this]() -> bool {
        return state_.run(
            [this](const auto& id) -> void { state_machine(id); });
    })
    , running_(running)
    , client_(client)
    , state_(decision_lock_, client_)
    , startup_promise_()
    , startup_(startup_promise_.get_future())
    , nym_callback_(zmq::ListenCallback::Factory(
          [this](const auto& in) -> void { callback_nym(in); }))
    , peer_reply_callback_(zmq::ListenCallback::Factory(
          [this](const auto& in) -> void { callback_peer_reply(in); }))
    , peer_request_callback_(zmq::ListenCallback::Factory(
          [this](const auto& in) -> void { callback_peer_request(in); }))
    , pair_event_(client.ZeroMQ().PublishSocket())
    , pending_bailment_(client.ZeroMQ().PublishSocket())
    , nym_subscriber_(client.ZeroMQ().SubscribeSocket(nym_callback_))
    , peer_reply_subscriber_(
          client.ZeroMQ().SubscribeSocket(peer_reply_callback_))
    , peer_request_subscriber_(
          client.ZeroMQ().SubscribeSocket(peer_request_callback_))
{
    // WARNING: do not access client_.Wallet() during construction
    pair_event_->Start(client_.Endpoints().PairEvent());
    pending_bailment_->Start(client_.Endpoints().PendingBailment());
    nym_subscriber_->Start(client_.Endpoints().NymDownload());
    peer_reply_subscriber_->Start(client_.Endpoints().PeerReplyUpdate());
    peer_request_subscriber_->Start(client_.Endpoints().PeerRequestUpdate());
}

Pair::State::State(
    std::mutex& lock,
    const api::client::internal::Manager& client) noexcept
    : lock_(lock)
    , client_(client)
    , state_()
    , issuers_()
{
}

void Pair::State::Add(
    const Lock& lock,
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID,
    const bool trusted) noexcept
{
    OT_ASSERT(CheckLock(lock, lock_));

    issuers_.emplace(issuerNymID);  // copy, then move
    state_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(OTNymID{localNymID}, OTNymID{issuerNymID}),
        std::forward_as_tuple(
            std::make_unique<std::mutex>(),
            client_.Factory().ServerID(),
            client_.Factory().NymID(),
            Status::Error,
            trusted,
            0,
            0,
            std::vector<AccountDetails>{},
            std::vector<OTX::BackgroundTask>{},
            false));
}

void Pair::State::Add(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID,
    const bool trusted) noexcept
{
    Lock lock(lock_);
    Add(lock, OTNymID{localNymID}, OTNymID{issuerNymID}, trusted);
}

auto Pair::State::CheckIssuer(const identifier::Nym& id) const noexcept -> bool
{
    Lock lock(lock_);

    return 0 < issuers_.count(id);
}

auto Pair::State::check_state() const noexcept -> bool
{
    Lock lock(lock_);

    for (auto& [id, details] : state_) {
        auto& [mutex, serverID, serverNymID, status, trusted, offered, registered, accountDetails, pending, needRename] =
            details;

        OT_ASSERT(mutex);

        Lock rowLock(*mutex);

        if (Status::Registered != status) {
            LogTrace(OT_METHOD)(__FUNCTION__)(": Not registered").Flush();

            goto repeat;
        }

        if (needRename) {
            LogTrace(OT_METHOD)(__FUNCTION__)(": Notary name not set").Flush();

            goto repeat;
        }

        const auto accountCount = count_currencies(accountDetails);

        if (accountCount != offered) {
            LogTrace(OT_METHOD)(__FUNCTION__)(
                ": Waiting for account registration, expected: ")(offered)(
                ", have ")(accountCount)
                .Flush();

            goto repeat;
        }

        for (const auto& [unit, account, bailments] : accountDetails) {
            if (bailments < MINIMUM_UNUSED_BAILMENTS) {
                LogTrace(OT_METHOD)(__FUNCTION__)(
                    ": Waiting for bailment instructions for account ")(
                    account)(", expected: ")(MINIMUM_UNUSED_BAILMENTS)(
                    ", have ")(bailments)
                    .Flush();

                goto repeat;
            }
        }
    }

    // No reason to continue executing state machine
    LogTrace(OT_METHOD)(__FUNCTION__)(": Done").Flush();

    return false;

repeat:
    lock.unlock();
    LogTrace(OT_METHOD)(__FUNCTION__)(": Repeating").Flush();
    // Rate limit state machine to reduce unproductive execution while waiting
    // on network activity
    Sleep(std::chrono::milliseconds(50));

    return true;
}

auto Pair::State::count_currencies(
    const std::vector<AccountDetails>& in) noexcept -> std::size_t
{
    auto unique = std::set<OTUnitID>{};
    std::transform(
        std::begin(in),
        std::end(in),
        std::inserter(unique, unique.end()),
        [](const auto& item) -> OTUnitID { return std::get<0>(item); });

    return unique.size();
}

auto Pair::State::count_currencies(const ContactSection& in) noexcept
    -> std::size_t
{
    auto unique = std::set<OTUnitID>{};

    for (const auto& [type, pGroup] : in) {
        OT_ASSERT(pGroup);

        const auto& group = *pGroup;

        for (const auto& [id, pClaim] : group) {
            OT_ASSERT(pClaim);

            const auto& claim = *pClaim;
            unique.emplace(identifier::UnitDefinition::Factory(claim.Value()));
        }
    }

    return unique.size();
}

auto Pair::State::get_account(
    const identifier::UnitDefinition& unit,
    const Identifier& account,
    std::vector<AccountDetails>& details) noexcept -> AccountDetails&
{
    OT_ASSERT(false == unit.empty());
    OT_ASSERT(false == account.empty());

    for (auto& row : details) {
        const auto& [unitID, accountID, bailment] = row;
        const auto match = (unit.str() == unitID->str()) &&
                           (account.str() == accountID->str());

        if (match) { return row; }
    }

    return details.emplace_back(AccountDetails{unit, account, 0});
}

auto Pair::State::GetDetails(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID) noexcept -> StateMap::iterator
{
    Lock lock(lock_);

    return state_.find({localNymID, issuerNymID});
}

auto Pair::State::IssuerList(
    const identifier::Nym& localNymID,
    const bool onlyTrusted) const noexcept -> std::set<OTNymID>
{
    Lock lock(lock_);
    std::set<OTNymID> output{};

    for (auto& [key, value] : state_) {
        auto& pMutex = std::get<0>(value);

        OT_ASSERT(pMutex);

        Lock rowLock(*pMutex);
        const auto& issuerID = std::get<1>(key);
        const auto& trusted = std::get<4>(value);

        if (trusted || (false == onlyTrusted)) { output.emplace(issuerID); }
    }

    return output;
}

auto Pair::State::run(const std::function<void(const IssuerID&)> fn) noexcept
    -> bool
{
    auto list = std::set<IssuerID>{};

    {
        Lock lock(lock_);
        std::transform(
            std::begin(state_),
            std::end(state_),
            std::inserter(list, list.end()),
            [](const auto& in) -> IssuerID { return in.first; });
    }

    std::for_each(std::begin(list), std::end(list), fn);

    return check_state();
}

auto Pair::AddIssuer(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID,
    const std::string& pairingCode) const noexcept -> bool
{
    if (localNymID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid local nym id.").Flush();

        return false;
    }

    if (!client_.Wallet().IsLocalNym(localNymID.str())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid local nym.").Flush();

        return false;
    }

    if (issuerNymID.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid issuer nym id.").Flush();

        return false;
    }

    bool trusted{false};

    {
        auto editor = client_.Wallet().mutable_Issuer(localNymID, issuerNymID);
        auto& issuer = editor.get();
        const bool needPairingCode = issuer.PairingCode().empty();
        const bool havePairingCode = (false == pairingCode.empty());

        if (havePairingCode && needPairingCode) {
            issuer.SetPairingCode(pairingCode);
        }

        trusted = issuer.Paired();
    }

    state_.Add(localNymID, issuerNymID, trusted);
    Trigger();

    return true;
}

void Pair::callback_nym(const zmq::Message& in) noexcept
{
    startup_.get();
    const auto body = in.Body();

    OT_ASSERT(1 < body.size());

    auto nymID = client_.Factory().NymID();
    nymID->Assign(body.at(1).Bytes());
    auto trigger{state_.CheckIssuer(nymID)};

    {
        Lock lock(decision_lock_);

        for (auto& [id, details] : state_) {
            auto& [mutex, serverID, serverNymID, status, trusted, offered, registered, accountDetails, pending, needRename] =
                details;

            OT_ASSERT(mutex);

            Lock rowLock(*mutex);

            if (serverNymID == nymID) { trigger = true; }
        }
    }

    if (trigger) { Trigger(); }
}

void Pair::callback_peer_reply(const zmq::Message& in) noexcept
{
    startup_.get();
    const auto body = in.Body();

    OT_ASSERT(2 <= body.size());

    const auto nymID = client_.Factory().NymID(body.at(0));
    const auto reply = proto::Factory<proto::PeerReply>(body.at(1));
    auto trigger{false};

    if (false == proto::Validate(reply, VERBOSE)) { return; }

    switch (contract::peer::internal::translate(reply.type())) {
        case contract::peer::PeerRequestType::Bailment: {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Received bailment reply.")
                .Flush();
            Lock lock(decision_lock_);
            trigger = process_request_bailment(lock, nymID, reply);
        } break;
        case contract::peer::PeerRequestType::OutBailment: {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Received outbailment reply.")
                .Flush();
            Lock lock(decision_lock_);
            trigger = process_request_outbailment(lock, nymID, reply);
        } break;
        case contract::peer::PeerRequestType::ConnectionInfo: {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Received connection info reply.")
                .Flush();
            Lock lock(decision_lock_);
            trigger = process_connection_info(lock, nymID, reply);
        } break;
        case contract::peer::PeerRequestType::StoreSecret: {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Received store secret reply.")
                .Flush();
            Lock lock(decision_lock_);
            trigger = process_store_secret(lock, nymID, reply);
        } break;
        default: {
        }
    }

    if (trigger) { Trigger(); }
}

void Pair::callback_peer_request(const zmq::Message& in) noexcept
{
    startup_.get();
    const auto body = in.Body();

    OT_ASSERT(2 <= body.size());

    const auto nymID = client_.Factory().NymID(body.at(0));
    const auto request = proto::Factory<proto::PeerRequest>(body.at(1));
    auto trigger{false};

    if (false == proto::Validate(request, VERBOSE)) { return; }

    switch (contract::peer::internal::translate(request.type())) {
        case contract::peer::PeerRequestType::PendingBailment: {
            Lock lock(decision_lock_);
            trigger = process_pending_bailment(lock, nymID, request);
        } break;
        default: {
        }
    }

    if (trigger) { Trigger(); }
}

void Pair::check_accounts(
    const ContactData& issuerClaims,
    Issuer& issuer,
    const identifier::Server& serverID,
    std::size_t& offered,
    std::size_t& registeredAccounts,
    std::vector<Pair::State::AccountDetails>& accountDetails) const noexcept
{
    const auto& localNymID = issuer.LocalNymID();
    const auto& issuerNymID = issuer.IssuerID();
    const auto contractSection =
        issuerClaims.Section(contact::ContactSectionName::Contract);
    const auto haveAccounts = bool(contractSection);

    if (false == haveAccounts) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Issuer does not advertise any contracts.")
            .Flush();
    } else {
        offered = State::count_currencies(*contractSection);
        LogDetail(OT_METHOD)(__FUNCTION__)(": Issuer advertises ")(offered)(
            " contract")((1 == offered) ? "." : "s.")
            .Flush();
    }

    auto uniqueRegistered = std::set<OTUnitID>{};

    if (false == haveAccounts) { return; }

    for (const auto& [type, pGroup] : *contractSection) {
        SHUTDOWN()
        OT_ASSERT(pGroup);

        const auto& group = *pGroup;

        for (const auto& [id, pClaim] : group) {
            SHUTDOWN()
            OT_ASSERT(pClaim);

            const auto& notUsed [[maybe_unused]] = id;
            const auto& claim = *pClaim;
            const auto unitID =
                identifier::UnitDefinition::Factory(claim.Value());

            if (unitID->empty()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": Invalid unit definition")
                    .Flush();

                continue;
            }

            const auto accountList = issuer.AccountList(type, unitID);

            if (0 == accountList.size()) {
                LogDetail(OT_METHOD)(__FUNCTION__)(": Registering ")(unitID)(
                    " account for ")(localNymID)(" on ")(serverID)(".")
                    .Flush();
                const auto& [registered, id] =
                    register_account(localNymID, serverID, unitID);

                if (registered) {
                    LogDetail(OT_METHOD)(__FUNCTION__)(
                        ": Success registering account")
                        .Flush();
                    issuer.AddAccount(type, unitID, id);
                } else {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Failed to register account")
                        .Flush();
                }

                continue;
            } else {
                LogDetail(OT_METHOD)(__FUNCTION__)(": ")(unitID)(
                    " account for ")(localNymID)(" on ")(serverID)(
                    " already exists.")
                    .Flush();
            }

            for (const auto& accountID : accountList) {
                auto& details =
                    State::get_account(unitID, accountID, accountDetails);
                uniqueRegistered.emplace(unitID);
                auto& bailmentCount = std::get<2>(details);
                const auto instructions = issuer.BailmentInstructions(unitID);
                bailmentCount = instructions.size();
                const bool needBailment =
                    (MINIMUM_UNUSED_BAILMENTS > instructions.size());
                const bool nonePending =
                    (false == issuer.BailmentInitiated(unitID));

                if (needBailment && nonePending) {
                    LogDetail(OT_METHOD)(__FUNCTION__)(
                        ": Requesting bailment info for ")(unitID)(".")
                        .Flush();
                    const auto& [sent, requestID] = initiate_bailment(
                        localNymID, serverID, issuerNymID, unitID);

                    if (sent) {
                        issuer.AddRequest(
                            contract::peer::PeerRequestType::Bailment,
                            requestID);
                    }
                }
            }
        }
    }

    registeredAccounts = uniqueRegistered.size();
}

void Pair::check_connection_info(
    Issuer& issuer,
    const identifier::Server& serverID) const noexcept
{
    const auto trusted = issuer.Paired();

    if (false == trusted) { return; }

    const auto btcrpc =
        issuer.ConnectionInfo(contract::peer::ConnectionInfoType::BtcRpc);
    const bool needInfo =
        (btcrpc.empty() &&
         (false == issuer.ConnectionInfoInitiated(
                       contract::peer::ConnectionInfoType::BtcRpc)));

    if (needInfo) {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Sending connection info peer request.")
            .Flush();
        const auto [sent, requestID] = get_connection(
            issuer.LocalNymID(),
            issuer.IssuerID(),
            serverID,
            contract::peer::ConnectionInfoType::BtcRpc);

        if (sent) {
            issuer.AddRequest(
                contract::peer::PeerRequestType::ConnectionInfo, requestID);
        }
    }
}

void Pair::check_rename(
    const Issuer& issuer,
    const identifier::Server& serverID,
    const PasswordPrompt& reason,
    bool& needRename) const noexcept
{
    if (false == issuer.Paired()) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Not trusted").Flush();

        return;
    }

    auto editor = client_.Wallet().mutable_ServerContext(
        issuer.LocalNymID(), serverID, reason);
    auto& context = editor.get();

    if (context.AdminPassword() != issuer.PairingCode()) {
        context.SetAdminPassword(issuer.PairingCode());
    }

    needRename = context.ShouldRename();

    if (needRename) {
        proto::PairEvent event;
        event.set_version(1);
        event.set_type(proto::PAIREVENT_RENAME);
        event.set_issuer(issuer.IssuerID().str());
        const auto published = pair_event_->Send(event);

        if (published) {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Published should rename notification.")
                .Flush();
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error publishing should rename notification.")
                .Flush();
        }
    } else {
        LogTrace(OT_METHOD)(__FUNCTION__)(": No reason to rename").Flush();
    }
}

void Pair::check_store_secret(
    Issuer& issuer,
    const identifier::Server& serverID) const noexcept
{
#if OT_CRYPTO_WITH_BIP32
    if (false == issuer.Paired()) { return; }

    const auto needStoreSecret = (false == issuer.StoreSecretComplete()) &&
                                 (false == issuer.StoreSecretInitiated());

    if (needStoreSecret) {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Sending store secret peer request.")
            .Flush();
        const auto [sent, requestID] =
            store_secret(issuer.LocalNymID(), issuer.IssuerID(), serverID);

        if (sent) {
            issuer.AddRequest(
                contract::peer::PeerRequestType::StoreSecret, requestID);
        }
    }
#endif  // OT_CRYPTO_WITH_BIP32
}

auto Pair::CheckIssuer(
    const identifier::Nym& localNymID,
    const identifier::UnitDefinition& unitDefinitionID) const noexcept -> bool
{
    try {
        const auto contract = client_.Wallet().UnitDefinition(unitDefinitionID);

        return AddIssuer(localNymID, contract->Nym()->ID(), "");
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unit definition contract does not exist.")
            .Flush();

        return false;
    }
}

auto Pair::cleanup() const noexcept -> std::shared_future<void>
{
    peer_request_subscriber_->Close();
    peer_reply_subscriber_->Close();
    nym_subscriber_->Close();

    return StateMachine::Stop();
}

auto Pair::get_connection(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID,
    const identifier::Server& serverID,
    const contract::peer::ConnectionInfoType type) const
    -> std::pair<bool, OTIdentifier>
{
    std::pair<bool, OTIdentifier> output{false, Identifier::Factory()};
    auto& [success, requestID] = output;

    auto setID = [&](const Identifier& in) -> void { output.second = in; };
    auto [taskID, future] = client_.OTX().InitiateRequestConnection(
        localNymID, serverID, issuerNymID, type, setID);

    if (0 == taskID) { return output; }

    const auto result = std::get<0>(future.get());
    success = (otx::LastReplyStatus::MessageSuccess == result);

    return output;
}

void Pair::init() noexcept
{
    Lock lock(decision_lock_);

    for (const auto& nymID : client_.Wallet().LocalNyms()) {
        for (const auto& issuerID : client_.Wallet().IssuerList(nymID)) {
            const auto pIssuer = client_.Wallet().Issuer(nymID, issuerID);

            OT_ASSERT(pIssuer);

            const auto& issuer = *pIssuer;
            state_.Add(lock, nymID, issuerID, issuer.Paired());
        }

        process_peer_replies(lock, nymID);
        process_peer_requests(lock, nymID);
    }

    lock.unlock();
    startup_promise_.set_value();
    Trigger();
}

auto Pair::initiate_bailment(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const identifier::Nym& issuerID,
    const identifier::UnitDefinition& unitID) const
    -> std::pair<bool, OTIdentifier>
{
    auto output = std::pair<bool, OTIdentifier>{false, Identifier::Factory()};
    auto& success = std::get<0>(output);

    try {
        client_.Wallet().UnitDefinition(unitID);
    } catch (...) {
        queue_unit_definition(nymID, serverID, unitID);

        return output;
    }

    auto setID = [&](const Identifier& in) -> void { output.second = in; };
    auto [taskID, future] = client_.OTX().InitiateBailment(
        nymID, serverID, issuerID, unitID, setID);

    if (0 == taskID) { return output; }

    const auto result = std::get<0>(future.get());
    success = (otx::LastReplyStatus::MessageSuccess == result);

    return output;
}

auto Pair::IssuerDetails(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID) const noexcept -> std::string
{
    auto issuer = client_.Wallet().Issuer(localNymID, issuerNymID);

    if (false == bool(issuer)) { return {}; }

    return issuer->toString();
}

auto Pair::need_registration(
    const identifier::Nym& localNymID,
    const identifier::Server& serverID) const -> bool
{
    auto context = client_.Wallet().ServerContext(localNymID, serverID);

    if (context) { return (0 == context->Request()); }

    return true;
}

auto Pair::process_connection_info(
    const Lock& lock,
    const identifier::Nym& nymID,
    const proto::PeerReply& reply) const -> bool
{
    OT_ASSERT(CheckLock(lock, decision_lock_))
    OT_ASSERT(nymID == Identifier::Factory(reply.initiator()))
    OT_ASSERT(
        contract::peer::PeerRequestType::ConnectionInfo ==
        contract::peer::internal::translate(reply.type()))

    const auto requestID = Identifier::Factory(reply.cookie());
    const auto replyID = Identifier::Factory(reply.id());
    const auto issuerNymID = identifier::Nym::Factory(reply.recipient());
    auto editor = client_.Wallet().mutable_Issuer(nymID, issuerNymID);
    auto& issuer = editor.get();
    const auto added = issuer.AddReply(
        contract::peer::PeerRequestType::ConnectionInfo, requestID, replyID);

    if (added) {
        client_.Wallet().PeerRequestComplete(nymID, replyID);

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add reply.").Flush();

        return false;
    }
}

void Pair::process_peer_replies(const Lock& lock, const identifier::Nym& nymID)
    const
{
    OT_ASSERT(CheckLock(lock, decision_lock_));

    auto replies = client_.Wallet().PeerReplyIncoming(nymID);

    for (const auto& it : replies) {
        const auto replyID = Identifier::Factory(it.first);
        const auto reply = client_.Wallet().PeerReply(
            nymID, replyID, StorageBox::INCOMINGPEERREPLY);

        if (false == bool(reply)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load peer reply ")(
                it.first)(".")
                .Flush();

            continue;
        }

        const auto& type = reply->type();

        switch (contract::peer::internal::translate(type)) {
            case contract::peer::PeerRequestType::Bailment: {
                LogDetail(OT_METHOD)(__FUNCTION__)(": Received bailment reply.")
                    .Flush();
                process_request_bailment(lock, nymID, *reply);
            } break;
            case contract::peer::PeerRequestType::OutBailment: {
                LogDetail(OT_METHOD)(__FUNCTION__)(
                    ": Received outbailment reply.")
                    .Flush();
                process_request_outbailment(lock, nymID, *reply);
            } break;
            case contract::peer::PeerRequestType::ConnectionInfo: {
                LogDetail(OT_METHOD)(__FUNCTION__)(
                    ": Received connection info reply.")
                    .Flush();
                process_connection_info(lock, nymID, *reply);
            } break;
            case contract::peer::PeerRequestType::StoreSecret: {
                LogDetail(OT_METHOD)(__FUNCTION__)(
                    ": Received store secret reply.")
                    .Flush();
                process_store_secret(lock, nymID, *reply);
            } break;
            case contract::peer::PeerRequestType::Error:
            case contract::peer::PeerRequestType::PendingBailment:
            case contract::peer::PeerRequestType::VerificationOffer:
            case contract::peer::PeerRequestType::Faucet:
            default: {
                continue;
            }
        }
    }
}

void Pair::process_peer_requests(const Lock& lock, const identifier::Nym& nymID)
    const
{
    OT_ASSERT(CheckLock(lock, decision_lock_));

    const auto requests = client_.Wallet().PeerRequestIncoming(nymID);

    for (const auto& it : requests) {
        const auto requestID = Identifier::Factory(it.first);
        std::time_t time{};
        const auto request = client_.Wallet().PeerRequest(
            nymID, requestID, StorageBox::INCOMINGPEERREQUEST, time);

        if (false == bool(request)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to load peer request ")(it.first)(".")
                .Flush();

            continue;
        }

        const auto& type = request->type();

        switch (contract::peer::internal::translate(type)) {
            case contract::peer::PeerRequestType::PendingBailment: {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Received pending bailment notification.")
                    .Flush();
                process_pending_bailment(lock, nymID, *request);
            } break;
            case contract::peer::PeerRequestType::Error:
            case contract::peer::PeerRequestType::Bailment:
            case contract::peer::PeerRequestType::OutBailment:
            case contract::peer::PeerRequestType::ConnectionInfo:
            case contract::peer::PeerRequestType::StoreSecret:
            case contract::peer::PeerRequestType::VerificationOffer:
            case contract::peer::PeerRequestType::Faucet:
            default: {

                continue;
            }
        }
    }
}

auto Pair::process_pending_bailment(
    const Lock& lock,
    const identifier::Nym& nymID,
    const proto::PeerRequest& request) const -> bool
{
    OT_ASSERT(CheckLock(lock, decision_lock_))
    OT_ASSERT(nymID == Identifier::Factory(request.recipient()))
    OT_ASSERT(
        contract::peer::PeerRequestType::PendingBailment ==
        contract::peer::internal::translate(request.type()))

    const auto requestID = Identifier::Factory(request.id());
    const auto issuerNymID = identifier::Nym::Factory(request.initiator());
    const auto serverID = identifier::Server::Factory(request.server());
    auto editor = client_.Wallet().mutable_Issuer(nymID, issuerNymID);
    auto& issuer = editor.get();
    const auto added = issuer.AddRequest(
        contract::peer::PeerRequestType::PendingBailment, requestID);

    if (added) {
        pending_bailment_->Send(request);
        const OTIdentifier originalRequest =
            Identifier::Factory(request.pendingbailment().requestid());
        if (!originalRequest->empty()) {
            issuer.SetUsed(
                contract::peer::PeerRequestType::Bailment,
                originalRequest,
                true);
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to set request as used on issuer.")
                .Flush();
        }

        auto [taskID, future] = client_.OTX().AcknowledgeNotice(
            nymID, serverID, issuerNymID, requestID, true);

        if (0 == taskID) {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Acknowledgement request already queued.")
                .Flush();

            return false;
        }

        const auto result = future.get();
        const auto status = std::get<0>(result);

        if (otx::LastReplyStatus::MessageSuccess == status) {
            const auto message = std::get<1>(result);
            auto replyID{Identifier::Factory()};
            message->GetIdentifier(replyID);
            issuer.AddReply(
                contract::peer::PeerRequestType::PendingBailment,
                requestID,
                replyID);

            return true;
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add request.").Flush();
    }

    return false;
}

auto Pair::process_request_bailment(
    const Lock& lock,
    const identifier::Nym& nymID,
    const proto::PeerReply& reply) const -> bool
{
    OT_ASSERT(CheckLock(lock, decision_lock_))
    OT_ASSERT(nymID == Identifier::Factory(reply.initiator()))
    OT_ASSERT(
        contract::peer::PeerRequestType::Bailment ==
        contract::peer::internal::translate(reply.type()))

    const auto requestID = Identifier::Factory(reply.cookie());
    const auto replyID = Identifier::Factory(reply.id());
    const auto issuerNymID = identifier::Nym::Factory(reply.recipient());
    auto editor = client_.Wallet().mutable_Issuer(nymID, issuerNymID);
    auto& issuer = editor.get();
    const auto added = issuer.AddReply(
        contract::peer::PeerRequestType::Bailment, requestID, replyID);

    if (added) {
        client_.Wallet().PeerRequestComplete(nymID, replyID);

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add reply.").Flush();

        return false;
    }
}

auto Pair::process_request_outbailment(
    const Lock& lock,
    const identifier::Nym& nymID,
    const proto::PeerReply& reply) const -> bool
{
    OT_ASSERT(CheckLock(lock, decision_lock_))
    OT_ASSERT(nymID == Identifier::Factory(reply.initiator()))
    OT_ASSERT(
        contract::peer::PeerRequestType::OutBailment ==
        contract::peer::internal::translate(reply.type()))

    const auto requestID = Identifier::Factory(reply.cookie());
    const auto replyID = Identifier::Factory(reply.id());
    const auto issuerNymID = identifier::Nym::Factory(reply.recipient());
    auto editor = client_.Wallet().mutable_Issuer(nymID, issuerNymID);
    auto& issuer = editor.get();
    const auto added = issuer.AddReply(
        contract::peer::PeerRequestType::OutBailment, requestID, replyID);

    if (added) {
        client_.Wallet().PeerRequestComplete(nymID, replyID);

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add reply.").Flush();

        return false;
    }
}

auto Pair::process_store_secret(
    const Lock& lock,
    const identifier::Nym& nymID,
    const proto::PeerReply& reply) const -> bool
{
    OT_ASSERT(CheckLock(lock, decision_lock_))
    OT_ASSERT(nymID == Identifier::Factory(reply.initiator()))
    OT_ASSERT(
        contract::peer::PeerRequestType::StoreSecret ==
        contract::peer::internal::translate(reply.type()))

    const auto requestID = Identifier::Factory(reply.cookie());
    const auto replyID = Identifier::Factory(reply.id());
    const auto issuerNymID = identifier::Nym::Factory(reply.recipient());
    auto editor = client_.Wallet().mutable_Issuer(nymID, issuerNymID);
    auto& issuer = editor.get();
    const auto added = issuer.AddReply(
        contract::peer::PeerRequestType::StoreSecret, requestID, replyID);

    if (added) {
        client_.Wallet().PeerRequestComplete(nymID, replyID);
        proto::PairEvent event;
        event.set_version(1);
        event.set_type(proto::PAIREVENT_STORESECRET);
        event.set_issuer(issuerNymID->str());
        const auto published = pair_event_->Send(event);

        if (published) {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Published store secret notification.")
                .Flush();
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error Publishing store secret notification.")
                .Flush();
        }

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add reply.").Flush();

        return false;
    }
}

auto Pair::queue_nym_download(
    const identifier::Nym& localNymID,
    const identifier::Nym& targetNymID) const -> OTX::BackgroundTask
{
    client_.OTX().StartIntroductionServer(localNymID);

    return client_.OTX().FindNym(targetNymID);
}

auto Pair::queue_nym_registration(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const bool setData) const -> OTX::BackgroundTask
{
    return client_.OTX().RegisterNym(nymID, serverID, setData);
}

auto Pair::queue_server_contract(
    const identifier::Nym& nymID,
    const identifier::Server& serverID) const -> OTX::BackgroundTask
{
    client_.OTX().StartIntroductionServer(nymID);

    return client_.OTX().FindServer(serverID);
}

void Pair::queue_unit_definition(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& unitID) const
{
    const auto [taskID, future] =
        client_.OTX().DownloadUnitDefinition(nymID, serverID, unitID);

    if (0 == taskID) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to queue unit definition download")
            .Flush();

        return;
    }

    const auto [result, pReply] = future.get();
    const auto success = (otx::LastReplyStatus::MessageSuccess == result);

    if (success) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Obtained unit definition ")(
            unitID)
            .Flush();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to download unit definition ")(unitID)
            .Flush();
    }
}

auto Pair::register_account(
    const identifier::Nym& nymID,
    const identifier::Server& serverID,
    const identifier::UnitDefinition& unitID) const
    -> std::pair<bool, OTIdentifier>
{
    std::pair<bool, OTIdentifier> output{false, Identifier::Factory()};
    auto& [success, accountID] = output;

    try {
        client_.Wallet().UnitDefinition(unitID);
    } catch (...) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Waiting for unit definition ")(
            unitID)
            .Flush();
        queue_unit_definition(nymID, serverID, unitID);

        return output;
    }

    auto [taskID, future] =
        client_.OTX().RegisterAccount(nymID, serverID, unitID);

    if (0 == taskID) { return output; }

    const auto [result, pReply] = future.get();
    success = (otx::LastReplyStatus::MessageSuccess == result);

    if (success) {
        OT_ASSERT(pReply);

        const auto& reply = *pReply;
        accountID->SetString(reply.m_strAcctID);
    }

    return output;
}

void Pair::state_machine(const IssuerID& id) const
{
    const auto& [localNymID, issuerNymID] = id;
    LogDetail(OT_METHOD)(__FUNCTION__)(": Local nym: ")(localNymID)(
        " Issuer Nym: ")(issuerNymID)
        .Flush();
    auto reason = client_.Factory().PasswordPrompt("Pairing state machine");
    auto it = state_.GetDetails(localNymID, issuerNymID);

    OT_ASSERT(state_.end() != it);

    auto& [mutex, serverID, serverNymID, status, trusted, offered, registeredAccounts, accountDetails, pending, needRename] =
        it->second;

    OT_ASSERT(mutex);

    for (auto i = pending.begin(); i != pending.end();) {
        const auto& [task, future] = *i;
        const auto state = future.wait_for(std::chrono::milliseconds(10));

        if (std::future_status::ready == state) {
            const auto result = future.get();

            if (otx::LastReplyStatus::MessageSuccess == result.first) {
                LogTrace(OT_METHOD)(__FUNCTION__)(": Task ")(task)(
                    " completed successfully.")
                    .Flush();
            } else {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Task ")(task)(" failed.")
                    .Flush();
            }

            i = pending.erase(i);
        } else {
            ++i;
        }
    }

    if (0 < pending.size()) { return; }

    Lock lock(*mutex);
    const auto issuerNym = client_.Wallet().Nym(issuerNymID);

    if (false == bool(issuerNym)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Issuer nym not yet downloaded.")
            .Flush();
        pending.emplace_back(queue_nym_download(localNymID, issuerNymID));
        status = Status::Error;

        return;
    }

    SHUTDOWN()

    const auto& issuerClaims = issuerNym->Claims();
    serverID = issuerClaims.PreferredOTServer();

    if (serverID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Issuer nym does not advertise a server.")
            .Flush();
        // Maybe there's a new version
        pending.emplace_back(queue_nym_download(localNymID, issuerNymID));
        status = Status::Error;

        return;
    }

    SHUTDOWN()

    auto editor = client_.Wallet().mutable_Issuer(localNymID, issuerNymID);
    auto& issuer = editor.get();
    trusted = issuer.Paired();

    SHUTDOWN()

    switch (status) {
        case Status::Error: {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": First pass through state machine.")
                .Flush();
            status = Status::Started;

            [[fallthrough]];
        }
        case Status::Started: {
            if (need_registration(localNymID, serverID)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Local nym not registered on issuer's notary.")
                    .Flush();

                try {
                    const auto contract = client_.Wallet().Server(serverID);

                    SHUTDOWN()

                    pending.emplace_back(
                        queue_nym_registration(localNymID, serverID, trusted));
                } catch (...) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Waiting on server contract.")
                        .Flush();
                    pending.emplace_back(
                        queue_server_contract(localNymID, serverID));

                    return;
                }

                return;
            } else {
                status = Status::Registered;
            }

            [[fallthrough]];
        }
        case Status::Registered: {
            SHUTDOWN()

            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Local nym is registered on issuer's notary.")
                .Flush();

            if (serverNymID->empty()) {
                try {
                    serverNymID =
                        client_.Wallet().Server(serverID)->Nym()->ID();
                } catch (...) {

                    return;
                }
            }

            SHUTDOWN()

            check_rename(issuer, serverID, reason, needRename);

            SHUTDOWN()

            check_store_secret(issuer, serverID);

            SHUTDOWN()

            check_connection_info(issuer, serverID);

            SHUTDOWN()

            check_accounts(
                issuerClaims,
                issuer,
                serverID,
                offered,
                registeredAccounts,
                accountDetails);
            [[fallthrough]];
        }
        default: {
        }
    }
}

#if OT_CRYPTO_WITH_BIP32
auto Pair::store_secret(
    const identifier::Nym& localNymID,
    const identifier::Nym& issuerNymID,
    const identifier::Server& serverID) const -> std::pair<bool, OTIdentifier>
{
    auto reason = client_.Factory().PasswordPrompt(
        "Backing up BIP-39 data to paired node");
    std::pair<bool, OTIdentifier> output{false, Identifier::Factory()};
    auto& [success, requestID] = output;

    auto setID = [&](const Identifier& in) -> void { output.second = in; };
    const auto seedID = client_.Seeds().DefaultSeed();
    auto [taskID, future] = client_.OTX().InitiateStoreSecret(
        localNymID,
        serverID,
        issuerNymID,
        contract::peer::SecretType::Bip39,
        client_.Seeds().Words(seedID, reason),
        client_.Seeds().Passphrase(seedID, reason),
        setID);

    if (0 == taskID) { return output; }

    const auto result = std::get<0>(future.get());
    success = (otx::LastReplyStatus::MessageSuccess == result);

    return output;
}
#endif  // OT_CRYPTO_WITH_BIP32
}  // namespace opentxs::api::client::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                 // IWYU pragma: associated
#include "1_Internal.hpp"               // IWYU pragma: associated
#include "server/MessageProcessor.hpp"  // IWYU pragma: associated

#include <chrono>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/ReplyCallback.hpp"
#include "opentxs/network/zeromq/socket/Dealer.hpp"
#include "opentxs/network/zeromq/socket/Pull.hpp"
#include "opentxs/network/zeromq/socket/Reply.hpp"
#include "opentxs/network/zeromq/socket/Router.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/otx/Reply.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/otx/ServerReplyType.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/OTXPush.pb.h"
#include "opentxs/protobuf/ServerReply.pb.h"
#include "opentxs/protobuf/ServerRequest.pb.h"
#include "opentxs/protobuf/verify/ServerRequest.hpp"
#include "server/Server.hpp"
#include "server/UserCommandProcessor.hpp"

#define OTX_ZAP_DOMAIN "opentxs-otx"

#define OT_METHOD "opentxs::MessageProcessor::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::server
{
MessageProcessor::MessageProcessor(
    Server& server,
    const PasswordPrompt& reason,
    const Flag& running)
    : server_(server)
    , reason_(reason)
    , running_(running)
    , frontend_callback_(zmq::ListenCallback::Factory(
          [=](const zmq::Message& incoming) -> void {
              this->process_frontend(incoming);
          }))
    , frontend_socket_(server.API().ZeroMQ().RouterSocket(
          frontend_callback_,
          zmq::socket::Socket::Direction::Bind))
    , backend_callback_(zmq::ReplyCallback::Factory(
          [=](const zmq::Message& incoming) -> OTZMQMessage {
              return this->process_backend(incoming);
          }))
    , backend_socket_(server.API().ZeroMQ().ReplySocket(
          backend_callback_,
          zmq::socket::Socket::Direction::Bind))
    , internal_callback_(zmq::ListenCallback::Factory(
          [=](const zmq::Message& incoming) -> void {
              this->process_internal(incoming);
          }))
    , internal_socket_(server.API().ZeroMQ().DealerSocket(
          internal_callback_,
          zmq::socket::Socket::Direction::Connect))
    , notification_callback_(zmq::ListenCallback::Factory(
          [=](const zmq::Message& incoming) -> void {
              this->process_notification(incoming);
          }))
    , notification_socket_(server.API().ZeroMQ().PullSocket(
          notification_callback_,
          zmq::socket::Socket::Direction::Bind))
    , thread_()
    , internal_endpoint_(
          std::string("inproc://opentxs/notary/") + Identifier::Random()->str())
    , counter_lock_()
    , drop_incoming_(0)
    , drop_outgoing_(0)
    , active_connections_()
    , connection_map_lock_()
{
    auto bound = backend_socket_->Start(internal_endpoint_);
    bound &= internal_socket_->Start(internal_endpoint_);
    bound &= notification_socket_->Start(
        server_.API().Endpoints().InternalPushNotification());

    OT_ASSERT(bound);
}

void MessageProcessor::associate_connection(
    const identifier::Nym& nymID,
    const Data& connection)
{
    if (nymID.empty()) { return; }
    if (connection.empty()) { return; }

    eLock lock(connection_map_lock_);
    const auto result = active_connections_.emplace(nymID, connection);

    if (std::get<1>(result)) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Nym ")(nymID)(
            " is available via connection ")(connection.asHex())(".")
            .Flush();
    }
}

void MessageProcessor::cleanup()
{
    frontend_socket_->Close();
    notification_socket_->Close();
    internal_socket_->Close();
    backend_socket_->Close();

    if (thread_.joinable()) { thread_.join(); }
}

void MessageProcessor::DropIncoming(const int count) const
{
    Lock lock(counter_lock_);
    drop_incoming_ = count;
}

void MessageProcessor::DropOutgoing(const int count) const
{
    Lock lock(counter_lock_);
    drop_outgoing_ = count;
}

auto MessageProcessor::extract_proto(const zmq::Frame& incoming) const
    -> proto::ServerRequest
{
    return proto::Factory<proto::ServerRequest>(incoming);
}

auto MessageProcessor::get_connection(const network::zeromq::Message& incoming)
    -> OTData
{
    auto output = Data::Factory();
    const auto header = incoming.Header();

    if (0 < header.size()) {
        const auto& frame = header.at(0);
        output = Data::Factory(frame.data(), frame.size());
    }

    return output;
}

void MessageProcessor::init(
    const bool inproc,
    const int port,
    const Secret& privkey)
{
    if (port == 0) { OT_FAIL; }

    auto set = frontend_socket_->SetPrivateKey(privkey);

    OT_ASSERT(set);

    set = frontend_socket_->SetDomain(OTX_ZAP_DOMAIN);

    OT_ASSERT(set);

    std::stringstream endpoint{};

    if (inproc) {
        endpoint << "inproc://opentxs/notary/";
        endpoint << std::to_string(server_.API().Instance());
        endpoint << ":";
    } else {
        endpoint << std::string("tcp://*:");
    }

    endpoint << std::to_string(port);
    const auto bound = frontend_socket_->Start(endpoint.str());

    if (false == bound) {
        throw std::invalid_argument("Cannot connect to endpoint.");
    }

    LogNormal("Bound to endpoint: ")(endpoint.str()).Flush();
}

void MessageProcessor::run()
{
    while (running_) {
        // timeout is the time left until the next cron should execute.
        const auto timeout = server_.ComputeTimeout();

        if (timeout.count() <= 0) {
            // ProcessCron and process_backend must not run simultaneously
            Lock lock(lock_);
            server_.ProcessCron();
        }

        Sleep(std::chrono::milliseconds(50));
    }
}

auto MessageProcessor::process_backend(const zmq::Message& incoming)
    -> OTZMQMessage
{
    // ProcessCron and process_backend must not run simultaneously
    Lock lock(lock_);
    std::string reply{};

    std::string messageString{};
    if (0 < incoming.Body().size()) {
        messageString = *incoming.Body().begin();
    }

    bool error = process_message(messageString, reply);

    if (error) { reply = ""; }

    auto output = server_.API().ZeroMQ().ReplyMessage(incoming);
    output->AddFrame(reply);

    return output;
}

auto MessageProcessor::process_command(
    const proto::ServerRequest& serialized,
    identifier::Nym& nymID) -> bool
{
    const auto allegedNymID = identifier::Nym::Factory(serialized.nym());
    const auto nym = server_.API().Wallet().Nym(allegedNymID);

    if (false == bool(nym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Nym is not yet registered.")
            .Flush();

        return true;
    }

    auto request = otx::Request::Factory(server_.API(), serialized);

    if (request->Validate()) {
        nymID.Assign(request->Initiator());
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid request.").Flush();

        return false;
    }

    // TODO look at the request type and do some stuff

    return true;
}

void MessageProcessor::process_frontend(const zmq::Message& incoming)
{
    Lock lock(counter_lock_);

    if (0 < drop_incoming_) {
        LogNormal(OT_METHOD)(__FUNCTION__)(
            ": Dropping incoming message for testing.")
            .Flush();
        --drop_incoming_;
    } else {
        lock.unlock();
        const auto id = get_connection(incoming);
        const bool isProto{1 < incoming.Body().size()};

        if (isProto) {
            process_proto(id, incoming);
        } else {
            process_legacy(id, incoming);
        }
    }
}

void MessageProcessor::process_internal(const zmq::Message& incoming)
{
    Lock lock(counter_lock_);

    if (0 < drop_outgoing_) {
        LogNormal(OT_METHOD)(__FUNCTION__)(
            ": Dropping outgoing message for testing.")
            .Flush();
        --drop_outgoing_;
    } else {
        lock.unlock();
        OTZMQMessage reply{incoming};
        const auto sent = frontend_socket_->Send(reply);

        if (sent) {
            LogTrace(OT_METHOD)(__FUNCTION__)(": Reply message delivered.")
                .Flush();
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to send reply message.")
                .Flush();
        }
    }
}

void MessageProcessor::process_legacy(
    const Data& id,
    const network::zeromq::Message& incoming)
{
    LogTrace(OT_METHOD)(__FUNCTION__)(": Processing request via ")(id.asHex())
        .Flush();
    OTZMQMessage request{incoming};
    internal_socket_->Send(request);
}

auto MessageProcessor::process_message(
    const std::string& messageString,
    std::string& reply) -> bool
{
    if (messageString.size() < 1) { return true; }

    if (std::numeric_limits<std::uint32_t>::max() < messageString.size()) {
        return true;
    }

    auto armored = Armored::Factory();
    armored->MemSet(
        messageString.data(), static_cast<std::uint32_t>(messageString.size()));
    auto serialized = String::Factory();
    armored->GetString(serialized);
    auto request{server_.API().Factory().Message()};

    if (false == serialized->Exists()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Empty serialized request.")
            .Flush();

        return true;
    }

    if (false == request->LoadContractFromString(serialized)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to deserialized request.")
            .Flush();

        return true;
    }

    auto replymsg{server_.API().Factory().Message()};

    OT_ASSERT(false != bool(replymsg));

    const bool processed =
        server_.CommandProcessor().ProcessUserCommand(*request, *replymsg);

    if (false == processed) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Failed to process user command ")(
            request->m_strCommand)
            .Flush();
        LogVerbose(OT_METHOD)(__FUNCTION__)(String::Factory(*request)).Flush();
    } else {
        LogDetail(OT_METHOD)(__FUNCTION__)(
            ": Successfully processed user command ")(request->m_strCommand)
            .Flush();
    }

    auto serializedReply = String::Factory(*replymsg);

    if (false == serializedReply->Exists()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize reply.")
            .Flush();

        return true;
    }

    auto armoredReply = Armored::Factory(serializedReply);

    if (false == armoredReply->Exists()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to armor reply.").Flush();

        return true;
    }

    reply.assign(armoredReply->Get(), armoredReply->GetLength());

    return false;
}

void MessageProcessor::process_notification(const zmq::Message& incoming)
{
    if (2 != incoming.Body().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message.").Flush();

        return;
    }

    const auto nymID = identifier::Nym::Factory(incoming.Body().at(0));
    const auto connection = query_connection(nymID);

    if (connection->empty()) {
        LogDebug(OT_METHOD)(__FUNCTION__)(
            ": No notification channel available for ")(nymID)(".")
            .Flush();

        return;
    }

    const auto nym = server_.API().Wallet().Nym(server_.GetServerNym().ID());

    OT_ASSERT(nym);

    const auto& payload = incoming.Body().at(1);
    auto message = otx::Reply::Factory(
        server_.API(),
        nym,
        nymID,
        server_.GetServerID(),
        otx::ServerReplyType::Push,
        true,
        0,
        reason_,
        proto::DynamicFactory<proto::OTXPush>(payload));

    OT_ASSERT(message->Validate());

    const auto reply = server_.API().Factory().Data(message->Contract());
    auto pushNotification = zmq::Message::Factory();
    pushNotification->AddFrame(connection);
    pushNotification->AddFrame();
    pushNotification->AddFrame(reply);
    pushNotification->AddFrame();
    const auto sent = frontend_socket_->Send(pushNotification);

    if (sent) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Push notification for ")(nymID)(
            " delivered via ")(connection->asHex())
            .Flush();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to deliver push notifcation for ")(nymID)(" via ")(
            connection->asHex())(".")
            .Flush();
    }
}

void MessageProcessor::process_proto(
    const Data& id,
    const network::zeromq::Message& incoming)
{
    LogTrace(OT_METHOD)(__FUNCTION__)(": Processing request via ")(id.asHex())
        .Flush();
    const auto command = extract_proto(incoming.Body().at(0));

    if (false == proto::Validate(command, VERBOSE)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid otx request.").Flush();

        return;
    }

    auto nymID = identifier::Nym::Factory();
    const auto valid = process_command(command, nymID);

    if (valid) {
        const auto connection = get_connection(incoming);

        if (false == connection->empty()) {
            associate_connection(nymID, connection);
        }

        return;
    }
}

auto MessageProcessor::query_connection(const identifier::Nym& nymID) -> OTData
{
    sLock lock(connection_map_lock_);
    auto it = active_connections_.find(nymID);

    if (active_connections_.end() == it) { return Data::Factory(); }

    return it->second;
}

void MessageProcessor::Start()
{
    thread_ = std::thread(&MessageProcessor::run, this);
}

MessageProcessor::~MessageProcessor() { cleanup(); }
}  // namespace opentxs::server

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "network/ServerConnection.hpp"  // IWYU pragma: associated

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <map>
#include <memory>
#include <thread>

#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/network/ZMQ.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/ServerConnection.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/curve/Client.hpp"
#include "opentxs/network/zeromq/socket/Dealer.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Request.tpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/otx/Reply.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/otx/ServerRequestType.hpp"
#include "opentxs/otx/consensus/Server.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/OTXEnums.pb.h"
#include "opentxs/protobuf/ServerReply.pb.h"
#include "opentxs/protobuf/ServerRequest.pb.h"
#include "opentxs/protobuf/verify/ServerReply.hpp"
#include "opentxs/util/WorkType.hpp"

namespace zmq = opentxs::network::zeromq;

#define OT_METHOD "opentxs::ServerConnection::"

namespace opentxs::network
{
auto ServerConnection::Factory(
    const api::internal::Core& api,
    const api::network::ZMQ& zmq,
    const zeromq::socket::Publish& updates,
    const OTServerContract& contract) -> OTServerConnection
{
    return OTServerConnection(
        new implementation::ServerConnection(api, zmq, updates, contract));
}
}  // namespace opentxs::network

namespace opentxs::network::implementation
{
ServerConnection::ServerConnection(
    const api::internal::Core& api,
    const api::network::ZMQ& zmq,
    const zeromq::socket::Publish& updates,
    const OTServerContract& contract)
    : zmq_(zmq)
    , api_(api)
    , updates_(updates)
    , server_id_(api_.Factory().ServerID(contract->ID()->str()))
    , address_type_(zmq.DefaultAddressType())
    , remote_contract_(contract)
    , thread_()
    , callback_(zeromq::ListenCallback::Factory(
          [=](const zeromq::Message& in) -> void {
              this->process_incoming(in);
          }))
    , registration_socket_(zmq.Context().DealerSocket(
          callback_,
          zmq::socket::Socket::Direction::Connect))
    , socket_(zmq.Context().RequestSocket())
    , notification_socket_(
          zmq.Context().PushSocket(zmq::socket::Socket::Direction::Connect))
    , last_activity_(std::time(nullptr))
    , sockets_ready_(Flag::Factory(false))
    , status_(Flag::Factory(false))
    , use_proxy_(Flag::Factory(false))
    , registration_lock_()
    , registered_for_push_()
{
    thread_ = std::thread(&ServerConnection::activity_timer, this);
    const auto started = notification_socket_->Start(
        api_.Endpoints().InternalProcessPushNotification());

    OT_ASSERT(started);
}

void ServerConnection::activity_timer()
{
    while (zmq_.Running()) {
        const auto limit = zmq_.KeepAlive();
        const auto now = std::chrono::seconds(std::time(nullptr));
        const auto last = std::chrono::seconds(last_activity_.load());
        const auto duration = now - last;

        if (duration > limit) {
            if (limit > std::chrono::seconds(0)) {
                const auto result = socket_->Send(std::string(""));

                if (SendResult::TIMEOUT != result.first) {
                    reset_timer();

                    if (status_->On()) { publish(); };
                }
            } else {
                if (status_->Off()) { publish(); };
            }
        }

        Sleep(std::chrono::seconds(1));
    }
}

auto ServerConnection::async_socket(const Lock& lock) const -> OTZMQDealerSocket
{
    auto output = zmq_.Context().DealerSocket(
        callback_, zmq::socket::Socket::Direction::Connect);
    set_proxy(lock, output);
    set_timeouts(lock, output);
    set_curve(lock, output);
    output->Start(endpoint());

    return output;
}

auto ServerConnection::ChangeAddressType(const core::AddressType type) -> bool
{
    Lock lock(lock_);
    address_type_ = type;
    reset_socket(lock);

    return true;
}

auto ServerConnection::check_for_protobuf(const zeromq::Frame& frame)
    -> std::pair<bool, proto::ServerReply>
{
    std::pair<bool, proto::ServerReply> output{false, {}};
    auto& [valid, serialized] = output;
    serialized = proto::Factory<proto::ServerReply>(frame);
    valid = proto::Validate(serialized, VERBOSE);

    return output;
}

auto ServerConnection::ClearProxy() -> bool
{
    Lock lock(lock_);
    use_proxy_->Off();
    reset_socket(lock);

    return true;
}

auto ServerConnection::EnableProxy() -> bool
{
    Lock lock(lock_);
    use_proxy_->On();
    reset_socket(lock);

    return true;
}

void ServerConnection::disable_push(const identifier::Nym& nymID)
{
    Lock registrationLock(registration_lock_);
    registered_for_push_[nymID] = true;
}

auto ServerConnection::endpoint() const -> std::string
{
    std::uint32_t port{0};
    std::string hostname{""};
    core::AddressType type{};
    const auto have =
        remote_contract_->ConnectInfo(hostname, port, type, address_type_);

    if (false == have) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed retrieving connection info from server contract.")
            .Flush();

        OT_FAIL;
    }

    const auto endpoint = form_endpoint(type, hostname, port);
    LogOutput("Establishing connection to: ")(endpoint).Flush();

    return endpoint;
}

auto ServerConnection::form_endpoint(
    core::AddressType type,
    std::string hostname,
    std::uint32_t port) const -> std::string
{
    std::string output{};

    if (core::AddressType::Inproc == type) {
        output += "inproc://opentxs/notary/";
        output += hostname;
        output += ":";
        output += std::to_string(port);
    } else {
        output += "tcp://";
        output += hostname;
        output += ":";
        output += std::to_string(port);
    }

    return output;
}

auto ServerConnection::get_async(const Lock& lock) -> zeromq::socket::Dealer&
{
    OT_ASSERT(verify_lock(lock))

    if (false == sockets_ready_.get()) {
        registration_socket_ = async_socket(lock);
        socket_ = sync_socket(lock);
        sockets_ready_->On();
    }

    return registration_socket_;
}

auto ServerConnection::get_sync(const Lock& lock) -> zeromq::socket::Request&
{
    OT_ASSERT(verify_lock(lock))

    if (false == sockets_ready_.get()) {
        registration_socket_ = async_socket(lock);
        socket_ = sync_socket(lock);
        sockets_ready_->On();
    }

    return socket_;
}

auto ServerConnection::get_timeout() -> Time
{
    return Clock::now() + zmq_.SendTimeout();
}

void ServerConnection::process_incoming(const proto::ServerReply& in)
{
    try {
        auto message = otx::Reply::Factory(api_, in);

        if (false == message->Validate()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid push notification.")
                .Flush();

            return;
        }

        notification_socket_->Send(api_.Factory().Data(message->Contract()));
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to instantiate reply")
            .Flush();
    }
}

void ServerConnection::process_incoming(const zeromq::Message& in)
{
    if (status_->On()) { publish(); }

    auto message{api_.Factory().Message()};

    OT_ASSERT(false != bool(message));

    if (1 > in.Body().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Received legacy reply on async socket.")
            .Flush();

        return;
    }

    auto& frame = *in.Body().begin();

    if (0 == frame.size()) { return; }

    if (1 < in.Body().size()) {
        const auto [isProto, reply] = check_for_protobuf(frame);

        if (isProto) {
            process_incoming(reply);

            return;
        }

        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Message should be a protobuf but isn't.")
            .Flush();

        return;
    }
}

void ServerConnection::publish() const
{
    const bool state(status_.get());
    auto message = zmq_.Context().TaggedMessage(WorkType::OTXConnectionStatus);
    message->AddFrame(server_id_);
    message->AddFrame(state);
    updates_.Send(message);
}

void ServerConnection::register_for_push(
    const otx::context::Server& context,
    const PasswordPrompt& reason)
{
    if (2 > context.Request()) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Nym is not yet registered")
            .Flush();

        return;
    }

    Lock registrationLock(registration_lock_);
    const auto& nymID = context.Nym()->ID();
    auto& isRegistered = registered_for_push_[nymID];

    if (isRegistered) { return; }

    auto request = otx::Request::Factory(
        api_,
        context.Nym(),
        context.Notary(),
        otx::ServerRequestType::Activate,
        0,
        reason);
    request->SetIncludeNym(true, reason);
    auto message = zmq::Message::Factory();
    message->AddFrame();
    message->AddFrame(request->Contract());
    message->AddFrame();
    Lock socketLock(lock_);
    isRegistered = get_async(socketLock).Send(message);
}

void ServerConnection::reset_socket(const Lock& lock)
{
    OT_ASSERT(verify_lock(lock))

    sockets_ready_->Off();
}

void ServerConnection::reset_timer()
{
    last_activity_.store(std::time(nullptr));
}

auto ServerConnection::Send(
    const otx::context::Server& context,
    const Message& message,
    const PasswordPrompt& reason,
    const Push push) -> NetworkReplyMessage
{
    struct Cleanup {
        const Lock& lock_;
        ServerConnection& connection_;
        SendResult& result_;
        std::shared_ptr<Message>& reply_;
        bool success_{false};

        void SetStatus(const SendResult status)
        {
            if (SendResult::VALID_REPLY == status) { success_ = true; }
        }

        Cleanup(
            const Lock& lock,
            ServerConnection& connection,
            SendResult& result,
            std::shared_ptr<Message>& reply)
            : lock_(lock)
            , connection_(connection)
            , result_(result)
            , reply_(reply)
        {
        }

        ~Cleanup()
        {
            if (false == success_) {
                connection_.reset_socket(lock_);
                reply_.reset();
            }
        }
    };

    if (Push::Enable == push) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Registering for push").Flush();
        register_for_push(context, reason);
    } else {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Skipping push").Flush();
        disable_push(context.Nym()->ID());
    }

    NetworkReplyMessage output{SendResult::Error, nullptr};
    auto& status = output.first;
    auto& reply = output.second;
    reply.reset(api_.Factory().Message().release());

    OT_ASSERT(false != bool(reply));

    auto raw = String::Factory();
    message.SaveContractRaw(raw);
    auto envelope = Armored::Factory(raw);

    if (false == envelope->Exists()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to armor message").Flush();

        return output;
    }

    Lock socketLock(lock_);
    Cleanup cleanup(socketLock, *this, status, reply);
    auto request = api_.ZeroMQ().Message(std::string(envelope->Get()));
    auto sendresult = get_sync(socketLock).Send(request);

    if (status_->On()) { publish(); }

    status = sendresult.first;
    auto in = sendresult.second;
    auto replymessage{api_.Factory().Message()};

    OT_ASSERT(false != bool(replymessage));

    if (SendResult::TIMEOUT == status) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Reply timeout.").Flush();
        cleanup.SetStatus(SendResult::TIMEOUT);

        return output;
    }

    if (1 > in->Body().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Empty reply.").Flush();
        cleanup.SetStatus(SendResult::INVALID_REPLY);

        return output;
    }

    if (1 < in->Body().size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Push notification received as a reply.")
            .Flush();
        cleanup.SetStatus(SendResult::INVALID_REPLY);

        return output;
    }

    auto& frame = *in->Body().begin();

    if (0 == frame.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid reply message.").Flush();
        cleanup.SetStatus(SendResult::INVALID_REPLY);

        return output;
    }

    auto armored = Armored::Factory();
    armored->Set(std::string(frame).c_str());
    auto serialized = String::Factory();
    armored->GetString(serialized);
    const auto loaded = replymessage->LoadContractFromString(serialized);

    if (loaded) {
        reply.reset(replymessage.release());
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Received server reply, but unable to instantiate it as a "
            "Message.")
            .Flush();
        cleanup.SetStatus(SendResult::INVALID_REPLY);
    }

    cleanup.SetStatus(SendResult::VALID_REPLY);

    return output;
}

void ServerConnection::set_curve(
    const Lock& lock,
    zeromq::curve::Client& socket) const
{
    OT_ASSERT(verify_lock(lock));

    const auto set = socket.SetServerPubkey(remote_contract_);

    OT_ASSERT(set);
}

void ServerConnection::set_proxy(
    const Lock& lock,
    zeromq::socket::Dealer& socket) const
{
    OT_ASSERT(verify_lock(lock));

    if (false == use_proxy_.get()) { return; }

    auto proxy = zmq_.SocksProxy();

    if (false == proxy.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Setting proxy to ")(proxy)
            .Flush();
        const auto set = socket.SetSocksProxy(proxy);

        OT_ASSERT(set);
    }
}

void ServerConnection::set_timeouts(
    const Lock& lock,
    zeromq::socket::Socket& socket) const
{
    OT_ASSERT(verify_lock(lock));

    const auto set = socket.SetTimeouts(
        zmq_.Linger(), zmq_.SendTimeout(), zmq_.ReceiveTimeout());

    OT_ASSERT(set);
}

auto ServerConnection::sync_socket(const Lock& lock) const -> OTZMQRequestSocket
{
    auto output = zmq_.Context().RequestSocket();
    set_timeouts(lock, output);
    set_curve(lock, output);
    output->Start(endpoint());

    return output;
}

auto ServerConnection::Status() const -> bool { return status_.get(); }

ServerConnection::~ServerConnection()
{
    if (thread_.joinable()) { thread_.join(); }
}
}  // namespace opentxs::network::implementation

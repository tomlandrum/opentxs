// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/AddressType.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/network/ServerConnection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/socket/Dealer.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Request.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

namespace network
{
class ZMQ;
}  // namespace network
}  // namespace api

namespace network
{
namespace zeromq
{
namespace curve
{
class Client;
}  // namespace curve

namespace socket
{
class Publish;
class Socket;
}  // namespace socket

class Frame;
class Message;
}  // namespace zeromq
}  // namespace network

namespace otx
{
namespace context
{
class Server;
}  // namespace context
}  // namespace otx

namespace proto
{
class ServerReply;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::network::implementation
{
class ServerConnection final
    : virtual public opentxs::network::ServerConnection,
      Lockable
{
public:
    auto ChangeAddressType(const core::AddressType type) -> bool final;
    auto ClearProxy() -> bool final;
    auto EnableProxy() -> bool final;
    auto Send(
        const otx::context::Server& context,
        const Message& message,
        const PasswordPrompt& reason,
        const Push push) -> NetworkReplyMessage final;
    auto Status() const -> bool final;

    ~ServerConnection() final;

private:
    friend opentxs::network::ServerConnection;

    const api::network::ZMQ& zmq_;
    const api::internal::Core& api_;
    const zeromq::socket::Publish& updates_;
    const OTServerID server_id_;
    core::AddressType address_type_{core::AddressType::Error};
    OTServerContract remote_contract_;
    std::thread thread_;
    OTZMQListenCallback callback_;
    OTZMQDealerSocket registration_socket_;
    OTZMQRequestSocket socket_;
    OTZMQPushSocket notification_socket_;
    std::atomic<std::time_t> last_activity_{0};
    OTFlag sockets_ready_;
    OTFlag status_;
    OTFlag use_proxy_;
    mutable std::mutex registration_lock_;
    std::map<OTNymID, bool> registered_for_push_;

    static auto check_for_protobuf(const zeromq::Frame& frame)
        -> std::pair<bool, proto::ServerReply>;

    auto async_socket(const Lock& lock) const -> OTZMQDealerSocket;
    auto clone() const -> ServerConnection* final { return nullptr; }
    auto endpoint() const -> std::string;
    auto form_endpoint(
        core::AddressType type,
        std::string hostname,
        std::uint32_t port) const -> std::string;
    auto get_timeout() -> Time;
    void publish() const;
    void set_curve(const Lock& lock, zeromq::curve::Client& socket) const;
    void set_proxy(const Lock& lock, zeromq::socket::Dealer& socket) const;
    void set_timeouts(const Lock& lock, zeromq::socket::Socket& socket) const;
    auto sync_socket(const Lock& lock) const -> OTZMQRequestSocket;

    void activity_timer();
    void disable_push(const identifier::Nym& nymID);
    auto get_async(const Lock& lock) -> zeromq::socket::Dealer&;
    auto get_sync(const Lock& lock) -> zeromq::socket::Request&;
    void process_incoming(const zeromq::Message& in);
    void process_incoming(const proto::ServerReply& in);
    void register_for_push(
        const otx::context::Server& context,
        const PasswordPrompt& reason);
    void reset_socket(const Lock& lock);
    void reset_timer();

    ServerConnection(
        const api::internal::Core& api,
        const api::network::ZMQ& zmq,
        const zeromq::socket::Publish& updates,
        const OTServerContract& contract);
    ServerConnection() = delete;
    ServerConnection(const ServerConnection&) = delete;
    ServerConnection(ServerConnection&&) = delete;
    auto operator=(const ServerConnection&) -> ServerConnection& = delete;
    auto operator=(ServerConnection&&) -> ServerConnection& = delete;
};
}  // namespace opentxs::network::implementation

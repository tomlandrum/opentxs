// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "api/network/ZMQ.hpp"  // IWYU pragma: associated

#include <atomic>
#include <cstdint>
#include <map>
#include <utility>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/AddressType.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/network/ServerConnection.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"

#define CLIENT_SEND_TIMEOUT_SECONDS 20
#if OT_VALGRIND
#define CLIENT_RECV_TIMEOUT_SECONDS 400
#else
#define CLIENT_RECV_TIMEOUT_SECONDS 40
#endif
#define CLIENT_SOCKET_LINGER_SECONDS 0
#define CLIENT_SEND_TIMEOUT CLIENT_SEND_TIMEOUT_SECONDS
#define CLIENT_RECV_TIMEOUT CLIENT_RECV_TIMEOUT_SECONDS
#define KEEP_ALIVE_SECONDS 30

#define OT_METHOD "opentxs::api::ZMQ::"

template class opentxs::Pimpl<opentxs::network::ServerConnection>;

namespace opentxs
{
auto Factory::ZMQ(const api::internal::Core& api, const Flag& running)
    -> api::network::ZMQ*
{
    return new api::network::implementation::ZMQ(api, running);
}
}  // namespace opentxs

namespace opentxs::api::network::implementation
{
ZMQ::ZMQ(const api::internal::Core& api, const Flag& running)
    : api_(api)
    , running_(running)
    , linger_(std::chrono::seconds(CLIENT_SOCKET_LINGER_SECONDS))
    , receive_timeout_(std::chrono::seconds(CLIENT_RECV_TIMEOUT))
    , send_timeout_(std::chrono::seconds(CLIENT_SEND_TIMEOUT))
    , keep_alive_(std::chrono::seconds(0))
    , lock_()
    , socks_proxy_()
    , server_connections_()
    , status_publisher_(api_.ZeroMQ().PublishSocket())
{
    // WARNING: do not access api_.Wallet() during construction
    status_publisher_->Start(api_.Endpoints().ConnectionStatus());

    Lock lock(lock_);

    init(lock);
}

auto ZMQ::Context() const -> const opentxs::network::zeromq::Context&
{
    return api_.ZeroMQ();
}

auto ZMQ::DefaultAddressType() const -> core::AddressType
{
    bool changed{false};
    const std::int64_t defaultType{
        static_cast<std::int64_t>(core::AddressType::IPV4)};
    std::int64_t configuredType{
        static_cast<std::int64_t>(core::AddressType::Error)};
    api_.Config().CheckSet_long(
        String::Factory("Connection"),
        String::Factory("preferred_address_type"),
        defaultType,
        configuredType,
        changed);

    if (changed) { api_.Config().Save(); }

    return static_cast<core::AddressType>(configuredType);
}

void ZMQ::init(const Lock& lock) const
{
    OT_ASSERT(verify_lock(lock));

    bool notUsed{false};
    std::int64_t linger{0};
    api_.Config().CheckSet_long(
        String::Factory("latency"),
        String::Factory("linger"),
        CLIENT_SOCKET_LINGER_SECONDS,
        linger,
        notUsed);
    linger_.store(std::chrono::seconds(linger));
    std::int64_t send{0};
    api_.Config().CheckSet_long(
        String::Factory("latency"),
        String::Factory("send_timeout"),
        CLIENT_SEND_TIMEOUT,
        send,
        notUsed);
    send_timeout_.store(std::chrono::seconds(send));
    std::int64_t receive{0};
    api_.Config().CheckSet_long(
        String::Factory("latency"),
        String::Factory("recv_timeout"),
        CLIENT_RECV_TIMEOUT,
        receive,
        notUsed);
    receive_timeout_.store(std::chrono::seconds(receive));
    auto socks = String::Factory();
    bool haveSocksConfig{false};
    const bool configChecked = api_.Config().Check_str(
        String::Factory("Connection"),
        String::Factory("socks_proxy"),
        socks,
        haveSocksConfig);
    std::int64_t keepAlive{0};
    api_.Config().CheckSet_long(
        String::Factory("Connection"),
        String::Factory("keep_alive"),
        KEEP_ALIVE_SECONDS,
        keepAlive,
        notUsed);
    keep_alive_.store(std::chrono::seconds(keepAlive));

    if (configChecked && haveSocksConfig && socks->Exists()) {
        socks_proxy_ = socks->Get();
    }

    api_.Config().Save();
}

auto ZMQ::KeepAlive() const -> std::chrono::seconds
{
    return keep_alive_.load();
}

void ZMQ::KeepAlive(const std::chrono::seconds duration) const
{
    keep_alive_.store(duration);
}

auto ZMQ::Linger() const -> std::chrono::seconds { return linger_.load(); }

auto ZMQ::ReceiveTimeout() const -> std::chrono::seconds
{
    return receive_timeout_.load();
}

void ZMQ::RefreshConfig() const
{
    Lock lock(lock_);

    return init(lock);
}

auto ZMQ::Running() const -> const Flag& { return running_; }

auto ZMQ::SendTimeout() const -> std::chrono::seconds
{
    return send_timeout_.load();
}

auto ZMQ::Server(const std::string& id) const noexcept(false)
    -> opentxs::network::ServerConnection&
{
    Lock lock(lock_);
    auto existing = server_connections_.find(id);

    if (server_connections_.end() != existing) { return existing->second; }

    auto contract = api_.Wallet().Server(identifier::Server::Factory(id));
    auto [it, created] = server_connections_.emplace(
        id,
        opentxs::network::ServerConnection::Factory(
            api_, *this, status_publisher_, contract));
    auto& connection = it->second;

    OT_ASSERT(created);

    if (false == socks_proxy_.empty()) { connection->EnableProxy(); }

    return connection;
}

auto ZMQ::SetSocksProxy(const std::string& proxy) const -> bool
{
    bool notUsed{false};
    bool set = api_.Config().Set_str(
        String::Factory("Connection"),
        String::Factory("socks_proxy"),
        String::Factory(proxy),
        notUsed);

    if (false == set) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to set socks proxy.")
            .Flush();

        return false;
    }

    if (false == api_.Config().Save()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to set save config.")
            .Flush();

        return false;
    }

    Lock lock(lock_);
    socks_proxy_ = proxy;

    for (auto& it : server_connections_) {
        opentxs::network::ServerConnection& connection = it.second;

        if (proxy.empty()) {
            set &= connection.ClearProxy();
        } else {
            set &= connection.EnableProxy();
        }
    }

    if (false == set) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to reset connection.")
            .Flush();
    }

    return set;
}

auto ZMQ::SocksProxy(std::string& proxy) const -> bool
{
    Lock lock(lock_);
    proxy = socks_proxy_;

    return (!socks_proxy_.empty());
}

auto ZMQ::SocksProxy() const -> std::string
{
    std::string output{};
    SocksProxy(output);

    return output;
}

auto ZMQ::Status(const std::string& server) const -> ConnectionState
{
    Lock lock(lock_);
    const auto it = server_connections_.find(server);
    const bool haveConnection = it != server_connections_.end();
    lock.unlock();

    if (haveConnection) {
        if (it->second->Status()) {

            return ConnectionState::ACTIVE;
        } else {

            return ConnectionState::STALLED;
        }
    }

    return ConnectionState::NOT_ESTABLISHED;
}

auto ZMQ::verify_lock(const Lock& lock) const -> bool
{
    if (lock.mutex() != &lock_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect mutex.").Flush();

        return false;
    }

    if (false == lock.owns_lock()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Lock not owned.").Flush();

        return false;
    }

    return true;
}

ZMQ::~ZMQ() { server_connections_.clear(); }
}  // namespace opentxs::api::network::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_SERVERCONNECTION_HPP
#define OPENTXS_NETWORK_SERVERCONNECTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/Types.hpp"

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
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq

class ServerConnection;
}  // namespace network

namespace otx
{
namespace context
{
class Server;
}  // namespace context
}  // namespace otx

class PasswordPrompt;

using OTServerConnection = Pimpl<network::ServerConnection>;
}  // namespace opentxs

namespace opentxs
{
namespace network
{
class ServerConnection
{
public:
    enum class Push : bool {
        Enable = true,
        Disable = false,
    };

    OPENTXS_EXPORT static OTServerConnection Factory(
        const api::internal::Core& api,
        const api::network::ZMQ& zmq,
        const zeromq::socket::Publish& updates,
        const OTServerContract& contract);

    OPENTXS_EXPORT virtual bool ChangeAddressType(
        const core::AddressType type) = 0;
    OPENTXS_EXPORT virtual bool ClearProxy() = 0;
    OPENTXS_EXPORT virtual bool EnableProxy() = 0;
    OPENTXS_EXPORT virtual NetworkReplyMessage Send(
        const otx::context::Server& context,
        const Message& message,
        const PasswordPrompt& reason,
        const Push push = Push::Enable) = 0;
    OPENTXS_EXPORT virtual bool Status() const = 0;

    virtual ~ServerConnection() = default;

protected:
    ServerConnection() = default;

private:
    friend OTServerConnection;

    /** WARNING: not implemented */
    virtual ServerConnection* clone() const = 0;

    ServerConnection(const ServerConnection&) = delete;
    ServerConnection(ServerConnection&&) = delete;
    ServerConnection& operator=(const ServerConnection&) = delete;
    ServerConnection& operator=(ServerConnection&&) = delete;
};
}  // namespace network
}  // namespace opentxs
#endif

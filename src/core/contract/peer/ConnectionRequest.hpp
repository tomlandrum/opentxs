// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/peer/ConnectionRequest.cpp"

#pragma once

#include "core/contract/peer/PeerRequest.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/peer/ConnectionRequest.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identifier
{
class Nym;
class Server;
}  // namespace identifier

namespace proto
{
class PeerRequest;
}  // namespace proto

class Factory;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::request::implementation
{
class Connection final : public request::Connection,
                         public peer::implementation::Request
{
public:
    Connection(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& recipientID,
        const core::ConnectionInfoType type,
        const identifier::Server& serverID);
    Connection(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::PeerRequest& serialized);

    ~Connection() final = default;

private:
    friend opentxs::Factory;

    const core::ConnectionInfoType connection_type_;

    auto clone() const noexcept -> Connection* final
    {
        return new Connection(*this);
    }
    auto IDVersion(const Lock& lock) const -> SerializedType final;

    Connection() = delete;
    Connection(const Connection&);
    Connection(Connection&&) = delete;
    auto operator=(const Connection&) -> Connection& = delete;
    auto operator=(Connection&&) -> Connection& = delete;
};
}  // namespace opentxs::contract::peer::request::implementation

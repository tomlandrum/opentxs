// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/peer/NoticeAcknowledgement.cpp"

#pragma once

#include "core/contract/peer/PeerReply.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/peer/NoticeAcknowledgement.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/Types.hpp"

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

class Factory;
class Identifier;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::contract::peer::reply::implementation
{
class Acknowledgement final : public reply::Acknowledgement,
                              public peer::implementation::Reply
{
public:
    Acknowledgement(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& initiator,
        const Identifier& request,
        const identifier::Server& server,
        const PeerRequestType type,
        const bool& ack);
    Acknowledgement(
        const api::internal::Core& api,
        const Nym_p& nym,
        const SerializedType& serialized);

    ~Acknowledgement() final = default;

private:
    friend opentxs::Factory;

    const bool ack_;

    auto clone() const noexcept -> Acknowledgement* final
    {
        return new Acknowledgement(*this);
    }
    auto IDVersion(const Lock& lock) const -> SerializedType final;

    Acknowledgement() = delete;
    Acknowledgement(const Acknowledgement&);
    Acknowledgement(Acknowledgement&&) = delete;
    auto operator=(const Acknowledgement&) -> Acknowledgement& = delete;
    auto operator=(Acknowledgement&&) -> Acknowledgement& = delete;
};
}  // namespace opentxs::contract::peer::reply::implementation

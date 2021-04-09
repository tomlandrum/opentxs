// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEERREQUEST_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEERREQUEST_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/peer/Types.hpp"

namespace opentxs
{
namespace contract
{
namespace peer
{
class Request;
}  // namespace peer
}  // namespace contract

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class PeerRequest;
}  // namespace proto

using OTPeerRequest = SharedPimpl<contract::peer::Request>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
namespace peer
{
class Request : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::PeerRequest;

    OPENTXS_EXPORT virtual SerializedType Contract() const = 0;
    OPENTXS_EXPORT virtual const identifier::Nym& Initiator() const = 0;
    OPENTXS_EXPORT virtual const identifier::Nym& Recipient() const = 0;
    OPENTXS_EXPORT virtual PeerRequestType Type() const = 0;

    OPENTXS_EXPORT ~Request() override = default;

protected:
    Request() noexcept = default;

private:
    friend OTPeerRequest;

#ifndef _WIN32
    OPENTXS_EXPORT Request* clone() const noexcept override = 0;
#endif

    Request(const Request&) = delete;
    Request(Request&&) = delete;
    Request& operator=(const Request&) = delete;
    Request& operator=(Request&&) = delete;
};
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif

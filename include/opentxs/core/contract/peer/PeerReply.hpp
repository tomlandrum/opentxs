// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEERREPLY_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEERREPLY_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/peer/Types.hpp"

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace reply
{
class Acknowledgement;
class Bailment;
class Connection;
class Outbailment;
}  // namespace reply

class Reply;
}  // namespace peer
}  // namespace contract

namespace identifier
{
class Server;
}  // namespace identifier

namespace proto
{
class PeerReply;
}  // namespace proto

using OTPeerReply = SharedPimpl<contract::peer::Reply>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
namespace peer
{
class OPENTXS_EXPORT Reply : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::PeerReply;

    virtual auto asAcknowledgement() const noexcept
        -> const reply::Acknowledgement& = 0;
    virtual auto asBailment() const noexcept -> const reply::Bailment& = 0;
    virtual auto asConnection() const noexcept -> const reply::Connection& = 0;
    virtual auto asOutbailment() const noexcept
        -> const reply::Outbailment& = 0;

    using Signable::Serialize;
    OPENTXS_NO_EXPORT virtual bool Serialize(SerializedType&) const = 0;
    virtual PeerRequestType Type() const = 0;
    virtual const identifier::Server& Server() const = 0;

    virtual ~Reply() override = default;

protected:
    Reply() noexcept = default;

private:
    friend OTPeerReply;

#ifndef _WIN32
    Reply* clone() const noexcept override = 0;
#endif

    Reply(const Reply&) = delete;
    Reply(Reply&&) = delete;
    Reply& operator=(const Reply&) = delete;
    Reply& operator=(Reply&&) = delete;
};
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif

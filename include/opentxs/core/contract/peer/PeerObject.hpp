// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
namespace blind
{
class Purse;
}  // namespace blind

namespace proto
{
class PeerObject;
}  // namespace proto

class PeerObject;

using OTPeerObject = Pimpl<PeerObject>;
}  // namespace opentxs

namespace opentxs
{
class OPENTXS_EXPORT PeerObject
{
public:
    virtual const std::unique_ptr<std::string>& Message() const = 0;
    virtual const Nym_p& Nym() const = 0;
    virtual const std::unique_ptr<std::string>& Payment() const = 0;
#if OT_CASH
    virtual std::shared_ptr<blind::Purse> Purse() const = 0;
#endif
    virtual const OTPeerRequest Request() const = 0;
    virtual const OTPeerReply Reply() const = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(proto::PeerObject&) const = 0;
    virtual contract::peer::PeerObjectType Type() const = 0;
    virtual bool Validate() const = 0;

    virtual std::unique_ptr<std::string>& Message() = 0;
    virtual std::unique_ptr<std::string>& Payment() = 0;
#if OT_CASH
    virtual std::shared_ptr<blind::Purse>& Purse() = 0;
#endif

    virtual ~PeerObject() = default;

protected:
    PeerObject() = default;

private:
    PeerObject(const PeerObject&) = delete;
    PeerObject(PeerObject&&) = delete;
    PeerObject& operator=(const PeerObject&) = delete;
    PeerObject& operator=(PeerObject&&) = delete;
};
}  // namespace opentxs
#endif

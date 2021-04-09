// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
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
class PeerObject
{
public:
    OPENTXS_EXPORT virtual const std::unique_ptr<std::string>& Message()
        const = 0;
    OPENTXS_EXPORT virtual const Nym_p& Nym() const = 0;
    OPENTXS_EXPORT virtual const std::unique_ptr<std::string>& Payment()
        const = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual std::shared_ptr<blind::Purse> Purse() const = 0;
#endif
    OPENTXS_EXPORT virtual const OTPeerRequest Request() const = 0;
    OPENTXS_EXPORT virtual const OTPeerReply Reply() const = 0;
    OPENTXS_EXPORT virtual proto::PeerObject Serialize() const = 0;
    OPENTXS_EXPORT virtual contract::peer::PeerObjectType Type() const = 0;
    OPENTXS_EXPORT virtual bool Validate() const = 0;

    OPENTXS_EXPORT virtual std::unique_ptr<std::string>& Message() = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<std::string>& Payment() = 0;
#if OT_CASH
    OPENTXS_EXPORT virtual std::shared_ptr<blind::Purse>& Purse() = 0;
#endif

    OPENTXS_EXPORT virtual ~PeerObject() = default;

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

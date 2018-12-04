// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEEROBJECT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace opentxs
{
class PeerObject
{
public:
    EXPORT virtual const std::unique_ptr<std::string>& Message() const = 0;
    EXPORT virtual const ConstNym& Nym() const = 0;
    EXPORT virtual const std::unique_ptr<std::string>& Payment() const = 0;
#if OT_CASH
    EXPORT virtual std::shared_ptr<blind::Purse> Purse() const = 0;
#endif
    EXPORT virtual const std::shared_ptr<const PeerRequest> Request() const = 0;
    EXPORT virtual const std::shared_ptr<const PeerReply> Reply() const = 0;
    EXPORT virtual proto::PeerObject Serialize() const = 0;
    EXPORT virtual proto::PeerObjectType Type() const = 0;
    EXPORT virtual bool Validate() const = 0;

    EXPORT virtual std::unique_ptr<std::string>& Message() = 0;
    EXPORT virtual std::unique_ptr<std::string>& Payment() = 0;
#if OT_CASH
    EXPORT virtual std::shared_ptr<blind::Purse>& Purse() = 0;
#endif

    EXPORT virtual ~PeerObject() = default;

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

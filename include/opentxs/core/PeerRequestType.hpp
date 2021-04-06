// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_PEERREQUESTTYPE_HPP
#define OPENTXS_CORE_PEERREQUESTTYPE_HPP

#include "opentxs/core/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace core
{
enum class PeerRequestType : std::uint8_t {
    Error = 0,
    Bailment = 1,
    OutBailment = 2,
    PendingBailment = 3,
    ConnectionInfo = 4,
    StoreSecret = 5,
    VerificationOffer = 6,
    Faucet = 7,
};

}  // namespace core
}  // namespace opentxs
#endif

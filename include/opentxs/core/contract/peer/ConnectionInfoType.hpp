// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_CONNECTIONINFOTYPE_HPP
#define OPENTXS_CORE_CONTRACT_PEER_CONNECTIONINFOTYPE_HPP

#include "opentxs/core/contract/peer/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace contract
{
namespace peer
{
enum class ConnectionInfoType : std::uint8_t {
    Error = 0,
    Bitcoin = 1,
    BtcRpc = 2,
    BitMessage = 3,
    BitMessageRPC = 4,
    SSH = 5,
    CJDNS = 6,
};
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif

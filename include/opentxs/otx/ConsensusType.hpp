// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_CONSENSUS_TYPE_HPP
#define OPENTXS_OTX_CONSENSUS_TYPE_HPP

#include "opentxs/otx/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace otx
{
enum class ConsensusType : std::uint8_t {
    Error = 0,
    Server = 1,
    Client = 2,
    Peer = 3,
};
}  // namespace otx
}  // namespace opentxs
#endif

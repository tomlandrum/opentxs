// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_SECRETTYPE_HPP
#define OPENTXS_CORE_CONTRACT_PEER_SECRETTYPE_HPP

#include "opentxs/core/contract/peer/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace contract
{
namespace peer
{
enum class SecretType : std::uint8_t {
    Error = 0,
    Bip39 = 1,
};
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_TYPES_HPP
#define OPENTXS_CORE_CONTRACT_PEER_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace opentxs
{
namespace contract
{
namespace peer
{
enum class ConnectionInfoType : std::uint8_t;
enum class PeerObjectType : std::uint8_t;
enum class PeerRequestType : std::uint8_t;
enum class SecretType : std::uint8_t;
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif

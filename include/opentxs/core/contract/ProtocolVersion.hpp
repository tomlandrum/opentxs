// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_PROTOCOLVERSION_HPP
#define OPENTXS_CORE_PROTOCOLVERSION_HPP

#include "opentxs/core/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace contract
{
enum class ProtocolVersion : std::uint8_t {
    Error = 0,
    Legacy = 1,
    Notify = 2,
};

}  // namespace contract
}  // namespace opentxs
#endif

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLIND_PURSETYPE_HPP
#define OPENTXS_BLIND_PURSETYPE_HPP

#include "opentxs/crypto/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace blind
{
enum class PurseType : std::uint8_t {
    Error = 0,
    Request = 1,
    Issue = 2,
    Normal = 3,
};
}  // namespace blind
}  // namespace opentxs
#endif

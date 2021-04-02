// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_SOURCE_TYPE_HPP
#define OPENTXS_IDENTITY_SOURCE_TYPE_HPP

#include "opentxs/identity/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace identity
{
enum class SourceType : std::uint8_t {
    Error = 0,
    PubKey = 1,
    Bip47 = 2,
};
}  // namespace identity
}  // namespace opentxs
#endif

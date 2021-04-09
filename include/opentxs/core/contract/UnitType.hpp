// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONTRACT_UNITTYPE_HPP
#define OPENTXS_CONTRACT_UNITTYPE_HPP

#include "opentxs/core/contract/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace contract
{
enum class UnitType : std::uint8_t {
    Error = 0,
    Currency = 1,
    Security = 2,
    Basket = 3,
};

}  // namespace contract
}  // namespace opentxs
#endif

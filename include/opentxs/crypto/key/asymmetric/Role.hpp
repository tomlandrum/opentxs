// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_ASYMMETRIC_ROLE_HPP
#define OPENTXS_CRYPTO_KEY_ASYMMETRIC_ROLE_HPP

#include "opentxs/crypto/key/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace crypto
{
namespace key
{
namespace asymmetric
{

enum class Role : std::uint8_t {
    Error = 0,
    Auth = 1,
    Encrypt = 2,
    Sign = 3,
};
}  // namespace asymmetric
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif

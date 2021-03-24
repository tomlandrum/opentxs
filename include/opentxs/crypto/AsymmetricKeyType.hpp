// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_LIST_ASYMMETRICKEYTYPE_HPP
#define OPENTXS_CRYPTO_LIST_ASYMMETRICKEYTYPE_HPP

#include "opentxs/crypto/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace crypto
{
enum class AsymmetricKeyType : std::uint8_t {
    Error = 0,
    Null = 1,
    Legacy = 2,
    Secp256k1 = 3,
    ED25519 = 4,
};
}  // namespace crypto
}  // namespace opentxs
#endif

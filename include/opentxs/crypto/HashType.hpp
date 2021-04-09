// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_HASHTYPE_HPP
#define OPENTXS_CRYPTO_HASHTYPE_HPP

#include "opentxs/crypto/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace crypto
{
enum class HashType : std::uint8_t {
    Error = 0,
    None = 1,
    Sha256 = 2,
    Sha512 = 3,
    Blake2b160 = 4,
    Blake2b256 = 5,
    Blake2b512 = 6,
    Ripemd160 = 7,
    Sha1 = 8,
    Sha256D = 9,
    Sha256DC = 10,
    Bitcoin = 11,
    SipHash24 = 12,
};

constexpr auto value(const HashType in) noexcept
{
    return static_cast<std::uint8_t>(in);
}

}  // namespace crypto
}  // namespace opentxs
#endif

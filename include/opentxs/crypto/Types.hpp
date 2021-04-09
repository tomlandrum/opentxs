// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_TYPES_HPP
#define OPENTXS_CRYPTO_TYPES_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstddef>
#include <cstdint>

namespace opentxs
{
namespace crypto
{
enum class AsymmetricKeyType : std::uint8_t;
enum class Language : std::uint8_t;
enum class SeedStrength : std::size_t;
enum class SeedStyle : std::uint8_t;
enum class SecretStyle : std::uint8_t;
enum class SignatureRole : std::uint16_t;
enum class SymmetricKeyType : std::uint8_t;
enum class SymmetricMode : std::uint8_t;
}  // namespace crypto

using Bip32Network = std::uint32_t;
using Bip32Depth = std::uint8_t;
using Bip32Fingerprint = std::uint32_t;
using Bip32Index = std::uint32_t;

enum class Bip43Purpose : Bip32Index;
enum class Bip44Type : Bip32Index;
enum class Bip32Child : Bip32Index;
}  // namespace opentxs
#endif

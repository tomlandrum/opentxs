// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_TYPES_HPP
#define OPENTXS_CRYPTO_KEY_TYPES_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstddef>
#include <cstdint>

namespace opentxs
{
namespace crypto
{
namespace key
{
namespace asymmetric
{
enum class Algorithm : std::uint8_t;
enum class Mode : std::uint8_t;
enum class Role : std::uint8_t;
}  // namespace asymmetric

namespace symmetric
{
enum class Source : std::uint8_t;
enum class Algorithm : std::uint8_t;
}  // namespace symmetric
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif

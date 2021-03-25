// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CRYPTO_SYMMETRIC_HPP
#define OPENTXS_API_CRYPTO_SYMMETRIC_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>

#include "opentxs/Proto.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/SymmetricKeyType.hpp"
#include "opentxs/crypto/SymmetricMode.hpp"

namespace opentxs
{
class Secret;
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace crypto
{
class Symmetric
{
public:
    OPENTXS_EXPORT virtual std::size_t IvSize(
        const opentxs::crypto::SymmetricMode mode) const = 0;
    OPENTXS_EXPORT virtual OTSymmetricKey Key(
        const PasswordPrompt& password,
        const opentxs::crypto::SymmetricMode mode =
            opentxs::crypto::SymmetricMode::ChaCha20Poly1305) const = 0;
    OPENTXS_EXPORT virtual OTSymmetricKey Key(
        const proto::SymmetricKey& serialized,
        const opentxs::crypto::SymmetricMode mode) const = 0;
    OPENTXS_EXPORT virtual OTSymmetricKey Key(
        const Secret& seed,
        const std::uint64_t operations = 0,
        const std::uint64_t difficulty = 0,
        const opentxs::crypto::SymmetricMode mode =
            opentxs::crypto::SymmetricMode::ChaCha20Poly1305,
        const opentxs::crypto::SymmetricKeyType type =
            opentxs::crypto::SymmetricKeyType::Argon2) const = 0;

    OPENTXS_EXPORT virtual ~Symmetric() = default;

protected:
    Symmetric() = default;

private:
    Symmetric(const Symmetric&) = delete;
    Symmetric(Symmetric&&) = delete;
    Symmetric& operator=(const Symmetric&) = delete;
    Symmetric& operator=(Symmetric&&) = delete;
};
}  // namespace crypto
}  // namespace api
}  // namespace opentxs
#endif

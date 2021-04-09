// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_LIBRARY_SYMMETRICPROVIDER_HPP
#define OPENTXS_CRYPTO_LIBRARY_SYMMETRICPROVIDER_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>

#include "opentxs/Proto.hpp"
#include "opentxs/crypto/Types.hpp"

namespace opentxs
{
namespace proto
{
class Ciphertext;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
namespace crypto
{
class SymmetricProvider
{
public:
    OPENTXS_EXPORT virtual bool Decrypt(
        const proto::Ciphertext& ciphertext,
        const std::uint8_t* key,
        const std::size_t keySize,
        std::uint8_t* plaintext) const = 0;
    OPENTXS_EXPORT virtual opentxs::crypto::key::symmetric::Algorithm
    DefaultMode() const = 0;
    OPENTXS_EXPORT virtual bool Derive(
        const std::uint8_t* input,
        const std::size_t inputSize,
        const std::uint8_t* salt,
        const std::size_t saltSize,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        const crypto::key::symmetric::Source type,
        std::uint8_t* output,
        std::size_t outputSize) const = 0;
    OPENTXS_EXPORT virtual bool Encrypt(
        const std::uint8_t* input,
        const std::size_t inputSize,
        const std::uint8_t* key,
        const std::size_t keySize,
        proto::Ciphertext& ciphertext) const = 0;
    OPENTXS_EXPORT virtual std::size_t IvSize(
        const opentxs::crypto::key::symmetric::Algorithm mode) const = 0;
    OPENTXS_EXPORT virtual std::size_t KeySize(
        const opentxs::crypto::key::symmetric::Algorithm mode) const = 0;
    OPENTXS_EXPORT virtual std::size_t SaltSize(
        const crypto::key::symmetric::Source type) const = 0;
    OPENTXS_EXPORT virtual std::size_t TagSize(
        const opentxs::crypto::key::symmetric::Algorithm mode) const = 0;

    OPENTXS_EXPORT virtual ~SymmetricProvider() = default;

protected:
    SymmetricProvider() = default;

private:
    SymmetricProvider(const SymmetricProvider&) = delete;
    SymmetricProvider(SymmetricProvider&&) = delete;
    SymmetricProvider& operator=(const SymmetricProvider&) = delete;
    SymmetricProvider& operator=(SymmetricProvider&&) = delete;
};
}  // namespace crypto
}  // namespace opentxs
#endif

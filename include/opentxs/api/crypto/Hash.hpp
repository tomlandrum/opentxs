// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CRYPTO_HASH_HPP
#define OPENTXS_API_CRYPTO_HASH_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Frame;
}  // namespace zeromq
}  // namespace network

class Data;
class Secret;
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace crypto
{
class Hash
{
public:
    OPENTXS_EXPORT virtual bool Digest(
        const opentxs::crypto::HashType hashType,
        const ReadView data,
        const AllocateOutput destination) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Digest(
        const opentxs::crypto::HashType hashType,
        const opentxs::network::zeromq::Frame& data,
        const AllocateOutput destination) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Digest(
        const std::uint32_t type,
        const ReadView data,
        const AllocateOutput encodedDestination) const noexcept = 0;
    OPENTXS_EXPORT virtual bool HMAC(
        const opentxs::crypto::HashType hashType,
        const ReadView key,
        const ReadView& data,
        const AllocateOutput digest) const noexcept = 0;
    OPENTXS_EXPORT virtual void MurmurHash3_32(
        const std::uint32_t& key,
        const Data& data,
        std::uint32_t& output) const noexcept = 0;
    OPENTXS_EXPORT virtual bool PKCS5_PBKDF2_HMAC(
        const Data& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept = 0;
    OPENTXS_EXPORT virtual bool PKCS5_PBKDF2_HMAC(
        const Secret& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept = 0;
    OPENTXS_EXPORT virtual bool PKCS5_PBKDF2_HMAC(
        const std::string& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept = 0;
    OPENTXS_EXPORT virtual auto Scrypt(
        const ReadView input,
        const ReadView salt,
        const std::uint64_t N,
        const std::uint32_t r,
        const std::uint32_t p,
        const std::size_t bytes,
        AllocateOutput writer) const noexcept -> bool = 0;

    OPENTXS_EXPORT virtual ~Hash() = default;

protected:
    Hash() noexcept = default;

private:
    Hash(const Hash&) = delete;
    Hash(Hash&&) = delete;
    Hash& operator=(const Hash&) = delete;
    Hash& operator=(Hash&&) = delete;
};
}  // namespace crypto
}  // namespace api
}  // namespace opentxs
#endif

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/crypto/Hash.cpp"

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/api/crypto/Hash.hpp"

namespace opentxs
{
namespace api
{
namespace crypto
{
class Encode;
}  // namespace crypto
}  // namespace api

namespace crypto
{
class HashingProvider;
class Pbkdf2;
class Ripemd160;
class Scrypt;
}  // namespace crypto

namespace network
{
namespace zeromq
{
class Frame;
}  // namespace zeromq
}  // namespace network

class Data;
class OTPassword;
class Secret;
}  // namespace opentxs

namespace opentxs::api::crypto::implementation
{
class Hash final : public api::crypto::Hash
{
public:
    auto Digest(
        const opentxs::crypto::HashType hashType,
        const ReadView data,
        const AllocateOutput destination) const noexcept -> bool final;
    auto Digest(
        const opentxs::crypto::HashType hashType,
        const opentxs::network::zeromq::Frame& data,
        const AllocateOutput destination) const noexcept -> bool final;
    auto Digest(
        const std::uint32_t type,
        const ReadView data,
        const AllocateOutput destination) const noexcept -> bool final;
    auto HMAC(
        const opentxs::crypto::HashType hashType,
        const ReadView key,
        const ReadView& data,
        const AllocateOutput digest) const noexcept -> bool final;
    void MurmurHash3_32(
        const std::uint32_t& key,
        const Data& data,
        std::uint32_t& output) const noexcept final;
    auto PKCS5_PBKDF2_HMAC(
        const Data& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept -> bool final;
    auto PKCS5_PBKDF2_HMAC(
        const Secret& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept -> bool final;
    auto PKCS5_PBKDF2_HMAC(
        const std::string& input,
        const Data& salt,
        const std::size_t iterations,
        const opentxs::crypto::HashType hashType,
        const std::size_t bytes,
        Data& output) const noexcept -> bool final;
    auto Scrypt(
        const ReadView input,
        const ReadView salt,
        const std::uint64_t N,
        const std::uint32_t r,
        const std::uint32_t p,
        const std::size_t bytes,
        AllocateOutput writer) const noexcept -> bool final;

    Hash(
        const api::crypto::Encode& encode,
        const opentxs::crypto::HashingProvider& sha,
        const opentxs::crypto::HashingProvider& blake,
        const opentxs::crypto::Pbkdf2& pbkdf2,
        const opentxs::crypto::Ripemd160& ripe,
        const opentxs::crypto::Scrypt& scrypt) noexcept;

    ~Hash() final = default;

private:
    const api::crypto::Encode& encode_;
    const opentxs::crypto::HashingProvider& sha_;
    const opentxs::crypto::HashingProvider& blake_;
    const opentxs::crypto::Pbkdf2& pbkdf2_;
    const opentxs::crypto::Ripemd160& ripe_;
    const opentxs::crypto::Scrypt& scrypt_;

    static auto allocate(
        const opentxs::crypto::HashType hashType,
        const AllocateOutput destination) noexcept -> WritableView;

    auto bitcoin_hash_160(
        const void* input,
        const std::size_t size,
        void* output) const noexcept -> bool;
    auto digest(
        const opentxs::crypto::HashType hashType,
        const void* input,
        const std::size_t size,
        void* output) const noexcept -> bool;
    auto HMAC(
        const opentxs::crypto::HashType hashType,
        const std::uint8_t* input,
        const std::size_t inputSize,
        const std::uint8_t* key,
        const std::size_t keySize,
        std::uint8_t* output) const noexcept -> bool;
    auto sha_256_double(const void* input, const std::size_t size, void* output)
        const noexcept -> bool;
    auto sha_256_double_checksum(
        const void* input,
        const std::size_t size,
        void* output) const noexcept -> bool;

    Hash(const Hash&) = delete;
    Hash(Hash&&) = delete;
    auto operator=(const Hash&) -> Hash& = delete;
    auto operator=(Hash&&) -> Hash& = delete;
};
}  // namespace opentxs::api::crypto::implementation

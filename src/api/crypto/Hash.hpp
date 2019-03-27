// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace opentxs::api::crypto::implementation
{
class Hash : public api::crypto::Hash
{
public:
    bool Digest(
        const proto::HashType hashType,
        const OTPassword& data,
        OTPassword& digest) const override;
    bool Digest(const proto::HashType hashType, const Data& data, Data& digest)
        const override;
    bool Digest(
        const proto::HashType hashType,
        const String& data,
        Data& digest) const override;
    bool Digest(
        const proto::HashType hashType,
        const std::string& data,
        Data& digest) const override;
    bool Digest(
        const std::uint32_t type,
        const std::string& data,
        std::string& encodedDigest) const override;
    bool HMAC(
        const proto::HashType hashType,
        const OTPassword& key,
        const Data& data,
        OTPassword& digest) const override;

    ~Hash() = default;

private:
    friend opentxs::Factory;

    const api::crypto::Encode& encode_;
    const opentxs::crypto::HashingProvider& ssl_;
    const opentxs::crypto::HashingProvider& sodium_;
#if OT_CRYPTO_USING_TREZOR || OT_WITH_BLOCKCHAIN
    const opentxs::crypto::Ripemd160& bitcoin_;
#endif

    static bool Allocate(const proto::HashType hashType, OTPassword& input);
    static bool Allocate(const proto::HashType hashType, Data& input);

    bool Digest(
        const proto::HashType hashType,
        const std::uint8_t* input,
        const size_t inputSize,
        std::uint8_t* output) const;
    Hash(
        const api::crypto::Encode& encode,
        const opentxs::crypto::HashingProvider& ssl,
        const opentxs::crypto::HashingProvider& sodium
#if OT_CRYPTO_USING_TREZOR || OT_WITH_BLOCKCHAIN
        ,
        const opentxs::crypto::Ripemd160& bitcoin
#endif
    );
    bool HMAC(
        const proto::HashType hashType,
        const std::uint8_t* input,
        const size_t inputSize,
        const std::uint8_t* key,
        const size_t keySize,
        std::uint8_t* output) const;
    const opentxs::crypto::HashingProvider& SHA2() const;
    const opentxs::crypto::HashingProvider& Sodium() const;

    Hash(const Hash&) = delete;
    Hash(Hash&&) = delete;
    Hash& operator=(const Hash&) = delete;
    Hash& operator=(Hash&&) = delete;
};
}  // namespace opentxs::api::crypto::implementation

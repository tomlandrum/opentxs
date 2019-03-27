// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace opentxs::api::implementation
{
class Crypto : virtual public opentxs::api::Crypto
{
public:
    const crypto::Config& Config() const override;
    const OTCachedKey& DefaultKey() const override;
    Editor<OTCachedKey> mutable_DefaultKey() const override;
    const OTCachedKey& CachedKey(const Identifier& id) const override;
    const OTCachedKey& CachedKey(const OTCachedKey& source) const override;
    const OTCachedKey& LoadDefaultKey(const Armored& serialized) const override;
    void SetTimeout(const std::chrono::seconds& timeout) const override;
    void SetSystemKeyring(const bool useKeyring) const override;

    // Encoding function interface
    const crypto::Encode& Encode() const override;

    // Hash function interface
    const crypto::Hash& Hash() const override;

    // Utility class for misc OpenSSL-provided functions
    const crypto::Util& Util() const override;

    // Asymmetric encryption engines
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
    const opentxs::crypto::AsymmetricProvider& ED25519() const override;
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    const opentxs::crypto::AsymmetricProvider& RSA() const override;
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    const opentxs::crypto::AsymmetricProvider& SECP256K1() const override;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

    // Symmetric encryption engines
    const crypto::Symmetric& Symmetric() const override;

#if OT_CRYPTO_SUPPORTED_ALGO_AES
    const opentxs::crypto::LegacySymmetricProvider& AES() const override;
#endif  // OT_CRYPTO_SUPPORTED_ALGO_AES
#if OT_CRYPTO_WITH_BIP32
    const opentxs::crypto::Bip32& BIP32() const override;
#endif  // OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_WITH_BIP39
    const opentxs::crypto::Bip39& BIP39() const override;
#endif  // OT_CRYPTO_WITH_BIP39

    OTSymmetricKey GetStorageKey(
        const proto::AsymmetricKey& raw) const override;

    ~Crypto();

private:
    friend opentxs::Factory;

    mutable std::mutex cached_key_lock_;
    mutable std::unique_ptr<OTCachedKey> primary_key_;
    mutable std::map<OTIdentifier, std::unique_ptr<OTCachedKey>> cached_keys_;
    std::unique_ptr<api::crypto::Config> config_;
#if OT_WITH_BLOCKCHAIN
    std::unique_ptr<opentxs::crypto::Bitcoin> bitcoin_;
#endif  // OT_WITH_BLOCKCHAIN
#if OT_CRYPTO_USING_TREZOR
    std::unique_ptr<opentxs::crypto::Trezor> trezor_;
#endif  // OT_CRYPTO_USING_TREZOR
    std::unique_ptr<opentxs::crypto::Sodium> sodium_;
#if OT_CRYPTO_USING_OPENSSL
    std::unique_ptr<opentxs::crypto::OpenSSL> ssl_;
#endif  // OT_CRYPTO_USING_OPENSSL
    const api::crypto::Util& util_;
    const opentxs::crypto::EcdsaProvider& secp256k1_helper_;
    const opentxs::crypto::EncodingProvider& base58_;
    const opentxs::crypto::Ripemd160& ripemd160_;
    const opentxs::crypto::Bip32& bip32_;
    const opentxs::crypto::Bip39& bip39_;
#if OT_CRYPTO_USING_LIBSECP256K1
    std::unique_ptr<opentxs::crypto::Secp256k1> secp256k1_;
#endif  // OT_CRYPTO_USING_LIBSECP256K1
    const opentxs::crypto::AsymmetricProvider& secp256k1_provider_;
    std::unique_ptr<crypto::Encode> encode_;
    std::unique_ptr<crypto::Hash> hash_;
    std::unique_ptr<crypto::Symmetric> symmetric_;

    void init_default_key(const Lock& lock) const;

    void Init();
    void Cleanup();

    Crypto(const api::Settings& settings);
    Crypto() = delete;
    Crypto(const Crypto&) = delete;
    Crypto(Crypto&&) = delete;
    Crypto& operator=(const Crypto&) = delete;
    Crypto& operator=(Crypto&&) = delete;
};
}  // namespace opentxs::api::implementation

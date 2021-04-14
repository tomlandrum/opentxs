// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"              // IWYU pragma: associated
#include "1_Internal.hpp"            // IWYU pragma: associated
#include "crypto/key/Secp256k1.hpp"  // IWYU pragma: associated

#include <stdexcept>

#include "internal/api/Api.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"

namespace opentxs::factory
{
using ReturnType = crypto::key::implementation::Secp256k1;

auto Secp256k1Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& input) noexcept
    -> std::unique_ptr<crypto::key::Secp256k1>
{
    try {

        return std::make_unique<ReturnType>(api, ecdsa, input);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Secp256k1Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::key::asymmetric::Role input,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Secp256k1>
{
    try {

        return std::make_unique<ReturnType>(api, ecdsa, input, version, reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Secp256k1Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Data& publicKey,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Secp256k1>
{
    try {
        auto sessionKey = api.Symmetric().Key(reason);

        return std::make_unique<ReturnType>(
            api,
            ecdsa,
            privateKey,
            publicKey,
            role,
            version,
            sessionKey,
            reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Secp256k1Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Secp256k1>
{
#if OT_CRYPTO_WITH_BIP32
    try {
        auto sessionKey = api.Symmetric().Key(reason);

        return std::make_unique<ReturnType>(
            api,
            ecdsa,
            privateKey,
            chainCode,
            publicKey,
            path,
            parent,
            role,
            version,
            sessionKey,
            reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}

auto Secp256k1Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version) noexcept
    -> std::unique_ptr<crypto::key::Secp256k1>
{
#if OT_CRYPTO_WITH_BIP32
    try {
        return std::make_unique<ReturnType>(
            api,
            ecdsa,
            privateKey,
            chainCode,
            publicKey,
            path,
            parent,
            role,
            version);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}
}  // namespace opentxs::factory

namespace opentxs::crypto::key::implementation
{
Secp256k1::Secp256k1(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& serializedKey) noexcept(false)
    : ot_super(api, ecdsa, serializedKey)
{
}

Secp256k1::Secp256k1(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version,
    const PasswordPrompt& reason) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::key::asymmetric::Algorithm::Secp256k1,
          role,
          version,
          reason)
{
}

Secp256k1::Secp256k1(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Data& publicKey,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const opentxs::PasswordPrompt& reason) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::key::asymmetric::Algorithm::Secp256k1,
          privateKey,
          publicKey,
          role,
          version,
          sessionKey,
          reason)
{
}

#if OT_CRYPTO_WITH_BIP32
Secp256k1::Secp256k1(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const opentxs::PasswordPrompt& reason) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::key::asymmetric::Algorithm::Secp256k1,
          privateKey,
          chainCode,
          publicKey,
          path,
          parent,
          role,
          version,
          sessionKey,
          reason)
{
}

Secp256k1::Secp256k1(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const crypto::key::asymmetric::Role role,
    const VersionNumber version) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::key::asymmetric::Algorithm::Secp256k1,
          privateKey,
          chainCode,
          publicKey,
          path,
          parent,
          role,
          version)
{
}
#endif  // OT_CRYPTO_WITH_BIP32

Secp256k1::Secp256k1(const Secp256k1& rhs) noexcept
    : key::Secp256k1()
    , ot_super(rhs)
{
}

Secp256k1::Secp256k1(const Secp256k1& rhs, const ReadView newPublic) noexcept
    : key::Secp256k1()
    , ot_super(rhs, newPublic)
{
}

Secp256k1::Secp256k1(const Secp256k1& rhs, OTSecret&& newSecretKey) noexcept
    : key::Secp256k1()
    , ot_super(rhs, std::move(newSecretKey))
{
}
}  // namespace opentxs::crypto::key::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "crypto/key/Ed25519.hpp"  // IWYU pragma: associated

#include <stdexcept>

#include "internal/api/Api.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/Ed25519.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "util/Sodium.hpp"

namespace opentxs::factory
{
using ReturnType = crypto::key::implementation::Ed25519;

auto Ed25519Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& input) noexcept
    -> std::unique_ptr<crypto::key::Ed25519>
{
    try {

        return std::make_unique<ReturnType>(api, ecdsa, input);
    } catch (const std::exception& e) {
        LogOutput("opentxs::factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Ed25519Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::KeyRole input,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Ed25519>
{
    try {

        return std::make_unique<ReturnType>(api, ecdsa, input, version, reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::factory::")(__FUNCTION__)(
            ": Failed to generate key: ")(e.what())
            .Flush();

        return nullptr;
    }
}

#if OT_CRYPTO_WITH_BIP32
auto Ed25519Key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const proto::KeyRole role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Ed25519>
{
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
}
#endif  // OT_CRYPTO_WITH_BIP32
}  // namespace opentxs::factory

namespace opentxs::crypto::key::implementation
{
Ed25519::Ed25519(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& serializedKey) noexcept(false)
    : ot_super(api, ecdsa, serializedKey)
{
}

Ed25519::Ed25519(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::KeyRole role,
    const VersionNumber version,
    const PasswordPrompt& reason) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::AsymmetricKeyType::ED25519,
          role,
          version,
          reason)
{
}

#if OT_CRYPTO_WITH_BIP32
Ed25519::Ed25519(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const proto::KeyRole role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason) noexcept(false)
    : ot_super(
          api,
          ecdsa,
          crypto::AsymmetricKeyType::ED25519,
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
#endif  // OT_CRYPTO_WITH_BIP32

Ed25519::Ed25519(const Ed25519& rhs) noexcept
    : key::Ed25519()
    , ot_super(rhs)
{
}

Ed25519::Ed25519(const Ed25519& rhs, const ReadView newPublic) noexcept
    : key::Ed25519()
    , ot_super(rhs, newPublic)
{
}

Ed25519::Ed25519(const Ed25519& rhs, OTSecret&& newSecretKey) noexcept
    : key::Ed25519()
    , ot_super(rhs, std::move(newSecretKey))
{
}

auto Ed25519::TransportKey(
    Data& publicKey,
    Secret& privateKey,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (false == HasPublic()) { return false; }
    if (false == HasPrivate()) { return false; }

    return sodium::ToCurveKeypair(
        PrivateKey(reason),
        PublicKey(),
        privateKey.WriteInto(Secret::Mode::Mem),
        publicKey.WriteInto());
}
}  // namespace opentxs::crypto::key::implementation

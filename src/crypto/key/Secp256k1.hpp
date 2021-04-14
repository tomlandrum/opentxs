// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <utility>

#include "crypto/key/EllipticCurve.hpp"
#include "crypto/key/HD.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace crypto
{
namespace key
{
class Symmetric;
}  // namespace key

class EcdsaProvider;
}  // namespace crypto

namespace proto
{
class AsymmetricKey;
class HDPath;
}  // namespace proto

class Data;
class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class Secp256k1 final : virtual public key::Secp256k1, public HD
{
public:
    auto CreateType() const -> NymParameterType final
    {
        return NymParameterType::secp256k1;
    }

    Secp256k1(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::AsymmetricKey& serializedKey) noexcept(false);
    Secp256k1(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const crypto::key::asymmetric::Role role,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    Secp256k1(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const Secret& privateKey,
        const Data& publicKey,
        const crypto::key::asymmetric::Role role,
        const VersionNumber version,
        key::Symmetric& sessionKey,
        const PasswordPrompt& reason) noexcept(false);
#if OT_CRYPTO_WITH_BIP32
    Secp256k1(
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
        const PasswordPrompt& reason) noexcept(false);
    Secp256k1(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const Secret& privateKey,
        const Secret& chainCode,
        const Data& publicKey,
        const proto::HDPath& path,
        const Bip32Fingerprint parent,
        const crypto::key::asymmetric::Role role,
        const VersionNumber version) noexcept(false);
#endif  // OT_CRYPTO_WITH_BIP32
    Secp256k1(const Secp256k1& rhs, const ReadView newPublic) noexcept;
    Secp256k1(const Secp256k1& rhs, OTSecret&& newSecretKey) noexcept;

    ~Secp256k1() final = default;

private:
    using ot_super = HD;

    auto clone() const noexcept -> Secp256k1* final
    {
        return new Secp256k1(*this);
    }
    auto clone_ec() const noexcept -> Secp256k1* final { return clone(); }
    auto get_public() const -> std::shared_ptr<proto::AsymmetricKey> final
    {
        return serialize_public(clone());
    }
    auto replace_public_key(const ReadView newPubkey) const noexcept
        -> std::unique_ptr<EllipticCurve> final
    {
        return std::make_unique<Secp256k1>(*this, newPubkey);
    }
    auto replace_secret_key(OTSecret&& newSecretKey) const noexcept
        -> std::unique_ptr<EllipticCurve> final
    {
        return std::make_unique<Secp256k1>(*this, std::move(newSecretKey));
    }

    Secp256k1() = delete;
    Secp256k1(const Secp256k1&) noexcept;
    Secp256k1(Secp256k1&&) = delete;
    auto operator=(const Secp256k1&) -> Secp256k1& = delete;
    auto operator=(Secp256k1&&) -> Secp256k1& = delete;
};
}  // namespace opentxs::crypto::key::implementation

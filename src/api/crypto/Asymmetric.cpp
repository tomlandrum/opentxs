// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"               // IWYU pragma: associated
#include "1_Internal.hpp"             // IWYU pragma: associated
#include "api/crypto/Asymmetric.hpp"  // IWYU pragma: associated

#include <type_traits>
#include <utility>

#include "crypto/key/Null.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/crypto/Factory.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "internal/crypto/key/Key.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/crypto/Bip32.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
#include "opentxs/crypto/key/Ed25519.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#include "opentxs/crypto/key/HD.hpp"
#if OT_CRYPTO_SUPPORTED_KEY_RSA
#include "opentxs/crypto/key/RSA.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/identity/Types.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "util/Container.hpp"

#define OT_METHOD "opentxs::api::crypto::implementation::Asymmetric::"

namespace opentxs::factory
{
auto AsymmetricAPI(const api::internal::Core& api) noexcept
    -> std::unique_ptr<api::crypto::internal::Asymmetric>
{
    using ReturnType = api::crypto::implementation::Asymmetric;

    return std::make_unique<ReturnType>(api);
}
}  // namespace opentxs::factory

namespace opentxs::api::crypto::implementation
{
const VersionNumber Asymmetric::serialized_path_version_{1};

const Asymmetric::TypeMap Asymmetric::curve_to_key_type_{
    {EcdsaCurve::invalid, opentxs::crypto::key::asymmetric::Algorithm::Error},
    {EcdsaCurve::secp256k1,
     opentxs::crypto::key::asymmetric::Algorithm::Secp256k1},
    {EcdsaCurve::ed25519, opentxs::crypto::key::asymmetric::Algorithm::ED25519},
};

Asymmetric::Asymmetric(const api::internal::Core& api) noexcept
    : api_(api)
{
}

#if OT_CRYPTO_WITH_BIP32
template <typename ReturnType, typename NullType>
auto Asymmetric::instantiate_hd_key(
    const opentxs::crypto::key::asymmetric::Algorithm type,
    const std::string& seedID,
    const opentxs::crypto::Bip32::Key& serialized,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const noexcept -> std::unique_ptr<ReturnType>
{
    const auto& [privkey, ccode, pubkey, path, parent] = serialized;

    switch (type) {
        case opentxs::crypto::key::asymmetric::Algorithm::ED25519:
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        {
            return opentxs::factory::Ed25519Key(
                api_,
                api_.Crypto().ED25519(),
                privkey,
                ccode,
                pubkey,
                serialize_path(seedID, path),
                parent,
                role,
                version,
                reason);
        }
#else
            break;
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
        case opentxs::crypto::key::asymmetric::Algorithm::Secp256k1:
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        {
            return opentxs::factory::Secp256k1Key(
                api_,
                api_.Crypto().SECP256K1(),
                privkey,
                ccode,
                pubkey,
                serialize_path(seedID, path),
                parent,
                role,
                version,
                reason);
        }
#else
            break;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        default: {
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid key type.").Flush();

    return std::make_unique<NullType>();
}
#endif  // OT_CRYPTO_WITH_BIP32

template <typename ReturnType, typename NullType>
auto Asymmetric::instantiate_serialized_key(
    const proto::AsymmetricKey& serialized) const noexcept
    -> std::unique_ptr<ReturnType>

{
    switch (opentxs::crypto::key::internal::translate(serialized.type())) {
        case opentxs::crypto::key::asymmetric::Algorithm::ED25519:
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        {
            return opentxs::factory::Ed25519Key(
                api_, api_.Crypto().ED25519(), serialized);
        }
#else
            break;
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
        case opentxs::crypto::key::asymmetric::Algorithm::Secp256k1:
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        {
            return opentxs::factory::Secp256k1Key(
                api_, api_.Crypto().SECP256K1(), serialized);
        }
#else
            break;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        default: {
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(
        ": Open-Transactions isn't built with support for this key type.")
        .Flush();

    return std::make_unique<NullType>();
}

auto Asymmetric::InstantiateECKey(const proto::AsymmetricKey& serialized) const
    -> Asymmetric::ECKey
{
    using ReturnType = opentxs::crypto::key::EllipticCurve;
    using NullType = opentxs::crypto::key::implementation::NullEC;

    switch (serialized.type()) {
        case proto::AKEYTYPE_ED25519:
        case proto::AKEYTYPE_SECP256K1: {
            return instantiate_serialized_key<ReturnType, NullType>(serialized);
        }
        case (proto::AKEYTYPE_LEGACY): {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong key type (RSA)")
                .Flush();
        } break;
        default: {
        }
    }

    return std::make_unique<NullType>();
}

auto Asymmetric::InstantiateHDKey(const proto::AsymmetricKey& serialized) const
    -> Asymmetric::HDKey
{
    using ReturnType = opentxs::crypto::key::HD;
    using NullType = opentxs::crypto::key::implementation::NullHD;

    switch (serialized.type()) {
        case proto::AKEYTYPE_ED25519:
        case proto::AKEYTYPE_SECP256K1: {
            return instantiate_serialized_key<ReturnType, NullType>(serialized);
        }
        case (proto::AKEYTYPE_LEGACY): {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong key type (RSA)")
                .Flush();
        } break;
        default: {
        }
    }

    return std::make_unique<NullType>();
}

#if OT_CRYPTO_WITH_BIP32
auto Asymmetric::InstantiateKey(
    const opentxs::crypto::key::asymmetric::Algorithm type,
    const std::string& seedID,
    const opentxs::crypto::Bip32::Key& serialized,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const -> Asymmetric::HDKey
{
    using ReturnType = opentxs::crypto::key::HD;
    using BlankType = opentxs::crypto::key::implementation::NullHD;

    return instantiate_hd_key<ReturnType, BlankType>(
        type, seedID, serialized, reason, role, version);
}
#endif  // OT_CRYPTO_WITH_BIP32

auto Asymmetric::InstantiateKey(const proto::AsymmetricKey& serialized) const
    -> Asymmetric::Key
{
    using ReturnType = opentxs::crypto::key::Asymmetric;
    using NullType = opentxs::crypto::key::implementation::Null;

    switch (serialized.type()) {
        case proto::AKEYTYPE_ED25519:
        case proto::AKEYTYPE_SECP256K1: {
            return instantiate_serialized_key<ReturnType, NullType>(serialized);
        }
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        case (proto::AKEYTYPE_LEGACY): {
            return opentxs::factory::RSAKey(
                api_, api_.Crypto().RSA(), serialized);
        }
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
        default: {
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(
        ": Open-Transactions isn't built with support for this key type.")
        .Flush();

    return std::make_unique<NullType>();
}

#if OT_CRYPTO_WITH_BIP32
auto Asymmetric::NewHDKey(
    const std::string& seedID,
    const Secret& seed,
    const EcdsaCurve& curve,
    const opentxs::crypto::Bip32::Path& path,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const -> Asymmetric::HDKey
{
    return InstantiateKey(
        curve_to_key_type_.at(curve),
        seedID,
        api_.Crypto().BIP32().DeriveKey(curve, seed, path),
        reason,
        role,
        version);
}

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
auto Asymmetric::InstantiateSecp256k1Key(
    const ReadView publicKey,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const noexcept -> Secp256k1Key
{
    static const auto blank = api_.Factory().Secret(0);

    return factory::Secp256k1Key(
        api_,
        api_.Crypto().SECP256K1(),
        blank,
        api_.Factory().Data(publicKey),
        role,
        version,
        reason);
}

auto Asymmetric::InstantiateSecp256k1Key(
    const Secret& priv,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const noexcept -> Secp256k1Key
{
    auto pub = api_.Factory().Data();
    const auto& ecdsa = api_.Crypto().SECP256K1();

    if (false == ecdsa.ScalarMultiplyBase(priv.Bytes(), pub->WriteInto())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to calculate public key")
            .Flush();

        return {};
    }

    return factory::Secp256k1Key(api_, ecdsa, priv, pub, role, version, reason);
}

auto Asymmetric::NewSecp256k1Key(
    const std::string& seedID,
    const Secret& seed,
    const opentxs::crypto::Bip32::Path& derive,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const -> Secp256k1Key
{
    const auto serialized =
        api_.Crypto().BIP32().DeriveKey(EcdsaCurve::secp256k1, seed, derive);
    const auto& [privkey, ccode, pubkey, path, parent] = serialized;

    return opentxs::factory::Secp256k1Key(
        api_,
        api_.Crypto().SECP256K1(),
        privkey,
        ccode,
        pubkey,
        serialize_path(seedID, path),
        parent,
        role,
        version,
        reason);
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#endif  // OT_CRYPTO_WITH_BIP32

auto Asymmetric::NewKey(
    const NymParameters& params,
    const PasswordPrompt& reason,
    const opentxs::crypto::key::asymmetric::Role role,
    const VersionNumber version) const -> Asymmetric::Key
{
    switch (params.Algorithm()) {
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
        case (opentxs::crypto::key::asymmetric::Algorithm::ED25519): {
            return opentxs::factory::Ed25519Key(
                api_, api_.Crypto().ED25519(), role, version, reason);
        }
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
        case (opentxs::crypto::key::asymmetric::Algorithm::Secp256k1): {
            return opentxs::factory::Secp256k1Key(
                api_, api_.Crypto().SECP256K1(), role, version, reason);
        }
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_SUPPORTED_KEY_RSA
        case (opentxs::crypto::key::asymmetric::Algorithm::Legacy): {
            return opentxs::factory::RSAKey(
                api_, api_.Crypto().RSA(), role, version, params, reason);
        }
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Open-Transactions isn't built with support for this key "
                "type.")
                .Flush();
        }
    }

    return {};
}

#if OT_CRYPTO_WITH_BIP32
auto Asymmetric::serialize_path(
    const std::string& seedID,
    const opentxs::crypto::Bip32::Path& children) -> proto::HDPath
{
    proto::HDPath output;
    output.set_version(serialized_path_version_);
    output.set_root(seedID);

    for (const auto& index : children) { output.add_child(index); }

    return output;
}
#endif  // OT_CRYPTO_WITH_BIP32
}  // namespace opentxs::api::crypto::implementation

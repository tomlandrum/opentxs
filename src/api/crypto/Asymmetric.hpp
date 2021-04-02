// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/crypto/Symmetric.cpp"

#pragma once

#include <map>
#include <memory>
#include <string>

#include "internal/api/Api.hpp"
#include "internal/api/crypto/Crypto.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/crypto/Bip32.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/identity/KeyRole.hpp"
#include "opentxs/protobuf/HDPath.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace proto
{
class AsymmetricKey;
}  // namespace proto

class NymParameters;
class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::api::crypto::implementation
{
class Asymmetric final : virtual public api::crypto::internal::Asymmetric
{
public:
    auto API() const noexcept -> const api::internal::Core& final
    {
        return api_;
    }
    auto InstantiateECKey(const proto::AsymmetricKey& serialized) const
        -> ECKey final;
    auto InstantiateHDKey(const proto::AsymmetricKey& serialized) const
        -> HDKey final;
#if OT_CRYPTO_WITH_BIP32
    auto InstantiateKey(
        const opentxs::crypto::AsymmetricKeyType type,
        const std::string& seedID,
        const opentxs::crypto::Bip32::Key& serialized,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const -> HDKey final;
#endif  // OT_CRYPTO_WITH_BIP32
    auto InstantiateKey(const proto::AsymmetricKey& serialized) const
        -> Key final;
#if OT_CRYPTO_WITH_BIP32
    auto NewHDKey(
        const std::string& seedID,
        const Secret& seed,
        const EcdsaCurve& curve,
        const opentxs::crypto::Bip32::Path& path,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const -> HDKey final;
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    auto InstantiateSecp256k1Key(
        const ReadView publicKey,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const noexcept -> Secp256k1Key final;
    auto InstantiateSecp256k1Key(
        const Secret& privateKey,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const noexcept -> Secp256k1Key final;
    auto NewSecp256k1Key(
        const std::string& seedID,
        const Secret& seed,
        const opentxs::crypto::Bip32::Path& path,
        const PasswordPrompt& reason,
        const identity::KeyRole role = identity::KeyRole::Sign,
        const VersionNumber version =
            opentxs::crypto::key::Secp256k1::DefaultVersion) const
        -> Secp256k1Key final;
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#endif  // OT_CRYPTO_WITH_BIP32
    auto NewKey(
        const NymParameters& params,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const -> Key final;

    Asymmetric(const api::internal::Core& api) noexcept;

    ~Asymmetric() final = default;

private:
    using TypeMap = std::map<EcdsaCurve, opentxs::crypto::AsymmetricKeyType>;

    static const VersionNumber serialized_path_version_;
    static const TypeMap curve_to_key_type_;

    const api::internal::Core& api_;

#if OT_CRYPTO_WITH_BIP32
    static auto serialize_path(
        const std::string& seedID,
        const opentxs::crypto::Bip32::Path& children) -> proto::HDPath;
#endif  // OT_CRYPTO_WITH_BIP32

#if OT_CRYPTO_WITH_BIP32
    template <typename ReturnType, typename NullType>
    auto instantiate_hd_key(
        const opentxs::crypto::AsymmetricKeyType type,
        const std::string& seedID,
        const opentxs::crypto::Bip32::Key& serialized,
        const PasswordPrompt& reason,
        const identity::KeyRole role,
        const VersionNumber version) const noexcept
        -> std::unique_ptr<ReturnType>;
#endif  // OT_CRYPTO_WITH_BIP32
    template <typename ReturnType, typename NullType>
    auto instantiate_serialized_key(const proto::AsymmetricKey& serialized)
        const noexcept -> std::unique_ptr<ReturnType>;

    Asymmetric() = delete;
    Asymmetric(const Asymmetric&) = delete;
    Asymmetric(Asymmetric&&) = delete;
    auto operator=(const Asymmetric&) -> Asymmetric& = delete;
    auto operator=(Asymmetric&&) -> Asymmetric& = delete;
};
}  // namespace opentxs::api::crypto::implementation

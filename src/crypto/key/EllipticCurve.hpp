// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <string>

#include "crypto/key/Asymmetric.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"

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
class Ciphertext;
class HDPath;
}  // namespace proto

class Data;
class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class EllipticCurve : virtual public key::EllipticCurve, public Asymmetric
{
public:
    operator bool() const noexcept final { return Asymmetric::operator bool(); }

    auto asPublic() const noexcept -> std::unique_ptr<key::Asymmetric> final
    {
        return asPublicEC();
    }
    auto asPublicEC() const noexcept
        -> std::unique_ptr<key::EllipticCurve> final;
    auto CloneEC() const noexcept -> std::unique_ptr<key::EllipticCurve> final
    {
        return std::unique_ptr<key::EllipticCurve>{clone_ec()};
    }
    virtual auto CreateType() const -> NymParameterType = 0;
    auto ECDSA() const noexcept -> const crypto::EcdsaProvider& final
    {
        return ecdsa_;
    }
    auto IncrementPrivate(const Secret& scalar, const PasswordPrompt& reason)
        const noexcept -> std::unique_ptr<key::EllipticCurve> final;
    auto IncrementPublic(const Secret& scalar) const noexcept
        -> std::unique_ptr<key::EllipticCurve> final;
    auto Path() const noexcept -> const std::string override { return {}; }
    auto Path(proto::HDPath&) const noexcept -> bool override { return {}; }
    auto SignDER(
        const ReadView preimage,
        const proto::HashType hash,
        Space& output,
        const PasswordPrompt& reason) const noexcept -> bool final;

    virtual ~EllipticCurve() override = default;

protected:
    const crypto::EcdsaProvider& ecdsa_;

    static auto serialize_public(EllipticCurve* copy)
        -> std::shared_ptr<proto::AsymmetricKey>;

    virtual auto clone_ec() const -> EllipticCurve* = 0;
    virtual auto get_public() const
        -> std::shared_ptr<proto::AsymmetricKey> = 0;

    EllipticCurve(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::AsymmetricKey& serializedKey) noexcept(false);
    EllipticCurve(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const crypto::AsymmetricKeyType keyType,
        const proto::KeyRole role,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    EllipticCurve(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const crypto::AsymmetricKeyType keyType,
        const Secret& privateKey,
        const Data& publicKey,
        const proto::KeyRole role,
        const VersionNumber version,
        key::Symmetric& sessionKey,
        const PasswordPrompt& reason) noexcept(false);
    EllipticCurve(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::AsymmetricKeyType keyType,
        const Secret& privateKey,
        const Data& publicKey,
        const proto::KeyRole role,
        const VersionNumber version) noexcept(false);
    EllipticCurve(const EllipticCurve&) noexcept;
    EllipticCurve(const EllipticCurve& rhs, const ReadView newPublic) noexcept;
    EllipticCurve(const EllipticCurve& rhs, OTSecret&& newSecretKey) noexcept;

private:
    friend crypto::EcdsaProvider;

    static auto extract_key(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::AsymmetricKey& serialized,
        Data& publicKey) -> std::unique_ptr<proto::Ciphertext>;

    virtual auto replace_public_key(const ReadView newPubkey) const noexcept
        -> std::unique_ptr<EllipticCurve> = 0;
    virtual auto replace_secret_key(OTSecret&& newSecretKey) const noexcept
        -> std::unique_ptr<EllipticCurve> = 0;

    EllipticCurve() = delete;
    EllipticCurve(EllipticCurve&&) = delete;
    auto operator=(const EllipticCurve&) -> EllipticCurve& = delete;
    auto operator=(EllipticCurve&&) -> EllipticCurve& = delete;
};
}  // namespace opentxs::crypto::key::implementation

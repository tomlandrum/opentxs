// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "identity/credential/Base.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/identity/Types.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identity
{
namespace internal
{
struct Authority;
}  // namespace internal

class Source;
}  // namespace identity

namespace proto
{
class Credential;
class KeyCredential;
class Signature;
}  // namespace proto

class Data;
class NymParameters;
class OTPassword;
class PasswordPrompt;
class Signature;
}  // namespace opentxs

namespace opentxs::identity::credential::implementation
{
class Key : virtual public credential::internal::Key,
            public credential::implementation::Base
{
public:
    auto GetKeypair(const identity::KeyRole role) const
        -> const crypto::key::Keypair& final
    {
        return GetKeypair(crypto::AsymmetricKeyType::Null, role);
    }
    auto GetKeypair(
        const crypto::AsymmetricKeyType type,
        const identity::KeyRole role) const
        -> const crypto::key::Keypair& final;
    auto GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const opentxs::Signature& theSignature,
        char cKeyType = '0') const -> std::int32_t final;
    auto hasCapability(const NymCapability& capability) const -> bool override;
    using Base::Verify;
    auto Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const identity::KeyRole key) const -> bool final;
    auto Sign(
        const GetPreimage input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        identity::KeyRole key,
        const crypto::HashType hash) const -> bool final;
    auto TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const -> bool final;

    auto SelfSign(
        const PasswordPrompt& reason,
        const std::optional<OTSecret> exportPassword = {},
        const bool onlyPrivate = false) -> bool final;

    ~Key() override = default;

protected:
    const VersionNumber subversion_;
    const OTKeypair signing_key_;
    const OTKeypair authentication_key_;
    const OTKeypair encryption_key_;

    auto serialize(
        const Lock& lock,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const
        -> std::shared_ptr<Base::SerializedType> override;
    auto verify_internally(const Lock& lock) const -> bool override;

    void sign(
        const identity::credential::internal::Primary& master,
        const PasswordPrompt& reason) noexcept(false) override;

    Key(const api::internal::Core& api,
        const identity::internal::Authority& owner,
        const identity::Source& source,
        const NymParameters& nymParameters,
        const VersionNumber version,
        const identity::CredentialRole role,
        const PasswordPrompt& reason,
        const std::string& masterID,
        const bool useProvidedSigningKey = false) noexcept(false);
    Key(const api::internal::Core& api,
        const identity::internal::Authority& owner,
        const identity::Source& source,
        const proto::Credential& serializedCred,
        const std::string& masterID) noexcept(false);

private:
    static const VersionConversionMap credential_subversion_;
    static const VersionConversionMap subversion_to_key_version_;

    static auto deserialize_key(
        const api::internal::Core& api,
        const int index,
        const proto::Credential& credential) -> OTKeypair;
    static auto new_key(
        const api::internal::Core& api,
        const proto::KeyRole role,
        const NymParameters& nymParameters,
        const VersionNumber version,
        const PasswordPrompt& reason,
        const ReadView dh = {}) noexcept(false) -> OTKeypair;
    static auto signing_key(
        const api::internal::Core& api,
        const NymParameters& params,
        const VersionNumber subversion,
        const bool useProvided,
        const PasswordPrompt& reason) noexcept(false) -> OTKeypair;

    auto addKeytoSerializedKeyCredential(
        proto::KeyCredential& credential,
        const bool getPrivate,
        const proto::KeyRole role) const -> bool;
    auto addKeyCredentialtoSerializedCredential(
        std::shared_ptr<Base::SerializedType> credential,
        const bool addPrivate) const -> bool;
    auto VerifySig(
        const Lock& lock,
        const proto::Signature& sig,
        const CredentialModeFlag asPrivate = PRIVATE_VERSION) const -> bool;
    auto VerifySignedBySelf(const Lock& lock) const -> bool;

    Key() = delete;
    Key(const Key&) = delete;
    Key(Key&&) = delete;
    auto operator=(const Key&) -> Key& = delete;
    auto operator=(Key&&) -> Key& = delete;
};
}  // namespace opentxs::identity::credential::implementation

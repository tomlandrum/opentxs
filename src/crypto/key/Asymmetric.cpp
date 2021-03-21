// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"               // IWYU pragma: associated
#include "1_Internal.hpp"             // IWYU pragma: associated
#include "crypto/key/Asymmetric.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "crypto/key/Null.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/OTSignatureMetadata.hpp"
#include "opentxs/crypto/SecretStyle.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/identity/Authority.hpp"
#include "opentxs/identity/credential/Key.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Signature.pb.h"

template class opentxs::Pimpl<opentxs::crypto::key::Asymmetric>;

#define OT_METHOD "opentxs::crypto::key::implementation::Asymmetric::"

namespace opentxs::crypto::key
{
const VersionNumber Asymmetric::DefaultVersion{2};
const VersionNumber Asymmetric::MaxVersion{2};

auto Asymmetric::Factory() noexcept -> OTAsymmetricKey
{
    return OTAsymmetricKey(new implementation::Null);
}
}  // namespace opentxs::crypto::key

namespace opentxs::crypto::key::implementation
{
const std::map<crypto::SignatureRole, VersionNumber> Asymmetric::sig_version_{
    {SignatureRole::PublicCredential, 1},
    {SignatureRole::PrivateCredential, 1},
    {SignatureRole::NymIDSource, 1},
    {SignatureRole::Claim, 1},
    {SignatureRole::ServerContract, 1},
    {SignatureRole::UnitDefinition, 1},
    {SignatureRole::PeerRequest, 1},
    {SignatureRole::PeerReply, 1},
    {SignatureRole::Context, 2},
    {SignatureRole::Account, 2},
    {SignatureRole::ServerRequest, 3},
    {SignatureRole::ServerReply, 3},
};
const Asymmetric::SignatureRoleMap Asymmetric::signaturerole_map_{
    {SignatureRole::PublicCredential, proto::SIGROLE_PUBCREDENTIAL},
    {SignatureRole::PrivateCredential, proto::SIGROLE_PRIVCREDENTIAL},
    {SignatureRole::NymIDSource, proto::SIGROLE_NYMIDSOURCE},
    {SignatureRole::Claim, proto::SIGROLE_CLAIM},
    {SignatureRole::ServerContract, proto::SIGROLE_SERVERCONTRACT},
    {SignatureRole::UnitDefinition, proto::SIGROLE_UNITDEFINITION},
    {SignatureRole::PeerRequest, proto::SIGROLE_PEERREQUEST},
    {SignatureRole::PeerReply, proto::SIGROLE_PEERREPLY},
    {SignatureRole::Context, proto::SIGROLE_CONTEXT},
    {SignatureRole::Account, proto::SIGROLE_ACCOUNT},
    {SignatureRole::ServerRequest, proto::SIGROLE_SERVERREQUEST},
    {SignatureRole::ServerReply, proto::SIGROLE_SERVERREPLY},
};

Asymmetric::Asymmetric(
    const api::internal::Core& api,
    const crypto::AsymmetricProvider& engine,
    const proto::AsymmetricKeyType keyType,
    const proto::KeyRole role,
    const bool hasPublic,
    const bool hasPrivate,
    const VersionNumber version,
    OTData&& pubkey,
    EncryptedExtractor get) noexcept(false)
    : api_(api)
    , provider_(engine)
    , version_(version)
    , type_(keyType)
    , role_(role)
    , has_public_(hasPublic)
    , has_private_(hasPrivate)
    , m_pMetadata(new OTSignatureMetadata(api_))
    , key_(std::move(pubkey))
    , plaintext_key_(api_.Factory().Secret(0))
    , encrypted_key_([&] {
        if (has_private_ && get) {

            return get(const_cast<Data&>(key_.get()), plaintext_key_);
        } else {

            return EncryptedKey{};
        }
    }())
{
    OT_ASSERT(0 < version);
    OT_ASSERT(nullptr != m_pMetadata);
}

Asymmetric::Asymmetric(
    const api::internal::Core& api,
    const crypto::AsymmetricProvider& engine,
    const proto::AsymmetricKeyType keyType,
    const proto::KeyRole role,
    const VersionNumber version,
    EncryptedExtractor getEncrypted) noexcept(false)
    : Asymmetric(
          api,
          engine,
          keyType,
          role,
          true,
          true,
          version,
          api.Factory().Data(),
          getEncrypted)
{
}

Asymmetric::Asymmetric(
    const api::internal::Core& api,
    const crypto::AsymmetricProvider& engine,
    const proto::AsymmetricKey& serialized,
    EncryptedExtractor getEncrypted) noexcept(false)
    : Asymmetric(
          api,
          engine,
          serialized.type(),
          serialized.role(),
          true,
          proto::KEYMODE_PRIVATE == serialized.mode(),
          serialized.version(),
          serialized.has_key() ? api.Factory().Data(serialized.key())
                               : api.Factory().Data(),
          getEncrypted)
{
}

Asymmetric::Asymmetric(const Asymmetric& rhs) noexcept
    : Asymmetric(
          rhs.api_,
          rhs.provider_,
          rhs.type_,
          rhs.role_,
          rhs.has_public_,
          rhs.has_private_,
          rhs.version_,
          OTData{rhs.key_},
          [&](auto&, auto&) -> EncryptedKey {
              if (rhs.encrypted_key_) {

                  return std::make_unique<proto::Ciphertext>(
                      *rhs.encrypted_key_);
              }

              return {};
          })
{
}

Asymmetric::Asymmetric(const Asymmetric& rhs, const ReadView newPublic) noexcept
    : Asymmetric(
          rhs.api_,
          rhs.provider_,
          rhs.type_,
          rhs.role_,
          true,
          false,
          rhs.version_,
          rhs.api_.Factory().Data(newPublic),
          [&](auto&, auto&) -> EncryptedKey { return {}; })
{
}

Asymmetric::Asymmetric(
    const Asymmetric& rhs,
    OTData&& newPublicKey,
    OTSecret&& newSecretKey) noexcept
    : api_(rhs.api_)
    , provider_(rhs.provider_)
    , version_(rhs.version_)
    , type_(rhs.type_)
    , role_(rhs.role_)
    , has_public_(false == newPublicKey->empty())
    , has_private_(false == newSecretKey->empty())
    , m_pMetadata(new OTSignatureMetadata(api_))
    , key_(std::move(newPublicKey))
    , plaintext_key_(std::move(newSecretKey))
    , encrypted_key_(EncryptedKey{})
{
    OT_ASSERT(0 < version_);
    OT_ASSERT(nullptr != m_pMetadata);
}

Asymmetric::operator bool() const noexcept
{
    return has_public_ || has_private_;
}

auto Asymmetric::operator==(const proto::AsymmetricKey& rhs) const noexcept
    -> bool
{
    std::shared_ptr<proto::AsymmetricKey> tempKey = Serialize();
    auto LHData = SerializeKeyToData(*tempKey);
    auto RHData = SerializeKeyToData(rhs);

    return (LHData == RHData);
}

auto Asymmetric::CalculateHash(
    const proto::HashType hashType,
    const PasswordPrompt& reason) const noexcept -> OTData
{
    auto output = api_.Factory().Data();
    const auto hashed = api_.Crypto().Hash().Digest(
        hashType,
        has_private_ ? PrivateKey(reason) : PublicKey(),
        output->WriteInto());

    if (false == hashed) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to calculate hash")
            .Flush();

        return Data::Factory();
    }

    return output;
}

auto Asymmetric::CalculateID(Identifier& output) const noexcept -> bool
{
    if (false == HasPublic()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing public key").Flush();

        return false;
    }

    return output.CalculateDigest(PublicKey());
}

auto Asymmetric::CalculateTag(
    const identity::Authority& nym,
    const proto::AsymmetricKeyType type,
    const PasswordPrompt& reason,
    std::uint32_t& tag,
    Secret& password) const noexcept -> bool
{
    if (false == has_private_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Not a private key.").Flush();

        return false;
    }

    try {
        const auto& cred = nym.GetTagCredential(type);
        const auto& key =
            cred.GetKeypair(type, proto::KEYROLE_ENCRYPT).GetPublicKey();

        if (false == get_tag(key, nym.GetMasterCredID(), reason, tag)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to calculate tag.")
                .Flush();

            return false;
        }

        if (false == get_password(key, reason, password)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to calculate session password.")
                .Flush();

            return false;
        }

        return true;
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid credential").Flush();

        return false;
    }
}

auto Asymmetric::CalculateTag(
    const key::Asymmetric& dhKey,
    const Identifier& credential,
    const PasswordPrompt& reason,
    std::uint32_t& tag) const noexcept -> bool
{
    if (false == has_private_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Not a private key.").Flush();

        return false;
    }

    return get_tag(dhKey, credential, reason, tag);
}

auto Asymmetric::CalculateSessionPassword(
    const key::Asymmetric& dhKey,
    const PasswordPrompt& reason,
    Secret& password) const noexcept -> bool
{
    if (false == has_private_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Not a private key.").Flush();

        return false;
    }

    return get_password(dhKey, reason, password);
}

auto Asymmetric::create_key(
    const api::internal::Core& api,
    const crypto::AsymmetricProvider& provider,
    const NymParameters& options,
    const proto::KeyRole role,
    const AllocateOutput publicKey,
    const AllocateOutput privateKey,
    const Secret& prv,
    const AllocateOutput params,
    const PasswordPrompt& reason) -> std::unique_ptr<proto::Ciphertext>
{
    generate_key(provider, options, role, publicKey, privateKey, params);
    auto pOutput = std::make_unique<proto::Ciphertext>();

    OT_ASSERT(pOutput)

    auto& output = *pOutput;

    if (false == encrypt_key(api, reason, prv.Bytes(), output)) {
        throw std::runtime_error("Failed to encrypt key");
    }

    return pOutput;
}

auto Asymmetric::encrypt_key(
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason,
    const bool attach,
    const ReadView plaintext) noexcept -> std::unique_ptr<proto::Ciphertext>
{
    auto output = std::make_unique<proto::Ciphertext>();

    if (false == bool(output)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct output")
            .Flush();

        return {};
    }

    auto& ciphertext = *output;

    if (encrypt_key(sessionKey, reason, attach, plaintext, ciphertext)) {

        return output;
    } else {

        return {};
    }
}

auto Asymmetric::encrypt_key(
    const api::internal::Core& api,
    const PasswordPrompt& reason,
    const ReadView plaintext,
    proto::Ciphertext& ciphertext) noexcept -> bool
{
    auto sessionKey = api.Symmetric().Key(reason);

    return encrypt_key(sessionKey, reason, true, plaintext, ciphertext);
}

auto Asymmetric::encrypt_key(
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason,
    const bool attach,
    const ReadView plaintext,
    proto::Ciphertext& ciphertext) noexcept -> bool
{
    const auto encrypted =
        sessionKey.Encrypt(plaintext, reason, ciphertext, attach);

    if (false == encrypted) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt key").Flush();

        return false;
    }

    return true;
}

auto Asymmetric::erase_private_data() -> void
{
    plaintext_key_->clear();
    encrypted_key_.reset();
    has_private_ = false;
}

auto Asymmetric::generate_key(
    const crypto::AsymmetricProvider& provider,
    const NymParameters& options,
    const proto::KeyRole role,
    const AllocateOutput publicKey,
    const AllocateOutput privateKey,
    const AllocateOutput params) noexcept(false) -> void
{
    const auto generated =
        provider.RandomKeypair(privateKey, publicKey, role, options, params);

    if (false == generated) {
        throw std::runtime_error("Failed to generate key");
    }
}

auto Asymmetric::get_password(
    const key::Asymmetric& target,
    const PasswordPrompt& reason,
    Secret& password) const noexcept -> bool
{
    return provider_.SharedSecret(
        target, *this, SecretStyle::Default, reason, password);
}

auto Asymmetric::get_private_key(const PasswordPrompt& reason) const
    noexcept(false) -> Secret&
{
    if (0 == plaintext_key_->size()) {
        if (false == bool(encrypted_key_)) {
            throw std::runtime_error{"Missing encrypted private key"};
        }

        const auto& privateKey = *encrypted_key_;
        auto sessionKey = api_.Symmetric().Key(
            privateKey.key(), proto::SMODE_CHACHA20POLY1305);

        if (false == sessionKey.get()) {
            throw std::runtime_error{"Failed to extract session key"};
        }

        auto allocator = plaintext_key_->WriteInto(Secret::Mode::Mem);

        if (false == sessionKey->Decrypt(privateKey, reason, allocator)) {
            throw std::runtime_error{"Failed to decrypt private key"};
        }
    }

    return plaintext_key_;
}

auto Asymmetric::get_tag(
    const key::Asymmetric& target,
    const Identifier& credential,
    const PasswordPrompt& reason,
    std::uint32_t& tag) const noexcept -> bool
{
    auto hashed = api_.Factory().Secret(0);
    auto password = api_.Factory().Secret(0);

    if (false == provider_.SharedSecret(
                     target, *this, SecretStyle::Default, reason, password)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate shared secret")
            .Flush();

        return false;
    }

    if (false == api_.Crypto().Hash().HMAC(
                     proto::HASHTYPE_SHA256,
                     password->Bytes(),
                     credential.Bytes(),
                     hashed->WriteInto(Secret::Mode::Mem))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to hash shared secret")
            .Flush();

        return false;
    }

    OT_ASSERT(hashed->size() >= sizeof(tag));

    return nullptr != std::memcpy(&tag, hashed->data(), sizeof(tag));
}

auto Asymmetric::hasCapability(const NymCapability& capability) const noexcept
    -> bool
{
    switch (capability) {
        case (NymCapability::SIGN_CHILDCRED):
        case (NymCapability::SIGN_MESSAGE):
        case (NymCapability::ENCRYPT_MESSAGE):
        case (NymCapability::AUTHENTICATE_CONNECTION): {

            return true;
        }
        default: {
        }
    }

    return false;
}

auto Asymmetric::NewSignature(
    const Identifier& credentialID,
    const crypto::SignatureRole role,
    const proto::HashType hash) const -> proto::Signature
{
    proto::Signature output{};
    output.set_version(sig_version_.at(role));
    output.set_credentialid(credentialID.str());
    output.set_role(signaturerole_map_.at(role));
    output.set_hashtype((proto::HASHTYPE_ERROR == hash) ? SigHashType() : hash);
    output.clear_signature();

    return output;
}

auto Asymmetric::Path() const noexcept -> const std::string
{
    LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect key type.").Flush();

    return "";
}

auto Asymmetric::Path(proto::HDPath&) const noexcept -> bool
{
    LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect key type.").Flush();

    return false;
}

auto Asymmetric::PrivateKey(const PasswordPrompt& reason) const noexcept
    -> ReadView
{
    try {

        return get_private_key(reason).Bytes();
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Asymmetric::Serialize() const noexcept
    -> std::shared_ptr<proto::AsymmetricKey>
{
    auto pOutput = std::make_shared<proto::AsymmetricKey>();

    OT_ASSERT(pOutput);

    auto& output = *pOutput;
    output.set_version(version_);
    output.set_role(role_);
    output.set_type(static_cast<proto::AsymmetricKeyType>(type_));
    output.set_key(key_->data(), key_->size());

    if (has_private_) {
        output.set_mode(proto::KEYMODE_PRIVATE);

        if (encrypted_key_) {
            *output.mutable_encryptedkey() = *encrypted_key_;
        }
    } else {
        output.set_mode(proto::KEYMODE_PUBLIC);
    }

    return pOutput;
}

auto Asymmetric::SerializeKeyToData(
    const proto::AsymmetricKey& serializedKey) const -> OTData
{
    return api_.Factory().Data(serializedKey);
}

auto Asymmetric::Sign(
    const GetPreimage input,
    const crypto::SignatureRole role,
    proto::Signature& signature,
    const Identifier& credential,
    const PasswordPrompt& reason,
    const proto::HashType hash) const noexcept -> bool
{
    const auto type{(proto::HASHTYPE_ERROR == hash) ? SigHashType() : hash};

    try {
        signature = NewSignature(credential, role, type);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature role.").Flush();

        return false;
    }

    const auto preimage = input();
    auto& output = *signature.mutable_signature();

    return Sign(preimage, type, writer(output), reason);
}

auto Asymmetric::Sign(
    const ReadView preimage,
    const proto::HashType hash,
    const AllocateOutput output,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (false == has_private_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing private key").Flush();

        return false;
    }

    bool success = engine().Sign(api_, preimage, *this, hash, output, reason);

    if (false == success) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign preimage").Flush();
    }

    return success;
}

auto Asymmetric::TransportKey(
    Data& publicKey,
    Secret& privateKey,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (false == HasPrivate()) { return false; }

    return provider_.SeedToCurveKey(
        PrivateKey(reason),
        privateKey.WriteInto(Secret::Mode::Mem),
        publicKey.WriteInto());
}

auto Asymmetric::Verify(const Data& plaintext, const proto::Signature& sig)
    const noexcept -> bool
{
    if (false == HasPublic()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing public key").Flush();

        return false;
    }

    auto signature = Data::Factory();
    signature->Assign(sig.signature().c_str(), sig.signature().size());

    return engine().Verify(plaintext, *this, signature, sig.hashtype());
}

Asymmetric::~Asymmetric()
{
    if (nullptr != m_pMetadata) { delete m_pMetadata; }

    m_pMetadata = nullptr;
}
}  // namespace opentxs::crypto::key::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                 // IWYU pragma: associated
#include "1_Internal.hpp"               // IWYU pragma: associated
#include "identity/credential/Key.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <stdexcept>

#include "core/contract/Signable.hpp"
#include "identity/credential/Base.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/crypto/OTSignatureMetadata.hpp"
#include "opentxs/core/crypto/Signature.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/KeyCredential.pb.h"
#include "opentxs/protobuf/Signature.pb.h"

#define OT_METHOD "opentxs::identity::credential::implementation::Key::"

namespace opentxs::identity::credential::implementation
{
const VersionConversionMap Key::credential_subversion_{
    {1, 1},
    {2, 1},
    {3, 1},
    {4, 1},
    {5, 1},
    {6, 2},
};
const VersionConversionMap Key::subversion_to_key_version_{
    {1, 1},
    {2, 2},
};

Key::Key(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const NymParameters& params,
    const VersionNumber version,
    const identity::CredentialRole role,
    const PasswordPrompt& reason,
    const std::string& masterID,
    const bool useProvided) noexcept(false)
    : credential::implementation::Base(
          api,
          parent,
          source,
          params,
          version,
          role,
          proto::KEYMODE_PRIVATE,
          masterID)
    , subversion_(credential_subversion_.at(version_))
    , signing_key_(signing_key(api_, params, subversion_, useProvided, reason))
    , authentication_key_(new_key(
          api_,
          proto::KEYROLE_AUTH,
          params,
          subversion_to_key_version_.at(subversion_),
          reason))
    , encryption_key_(new_key(
          api_,
          proto::KEYROLE_ENCRYPT,
          params,
          subversion_to_key_version_.at(subversion_),
          reason))
{
    if (0 == version) { throw std::runtime_error("Invalid version"); }
}

Key::Key(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const proto::Credential& serialized,
    const std::string& masterID) noexcept(false)
    : credential::implementation::Base(
          api,
          parent,
          source,
          serialized,
          masterID)
    , subversion_(credential_subversion_.at(version_))
    , signing_key_(deserialize_key(api, proto::KEYROLE_SIGN, serialized))
    , authentication_key_(deserialize_key(api, proto::KEYROLE_AUTH, serialized))
    , encryption_key_(deserialize_key(api, proto::KEYROLE_ENCRYPT, serialized))
{
}

auto Key::addKeyCredentialtoSerializedCredential(
    std::shared_ptr<Base::SerializedType> credential,
    const bool addPrivate) const -> bool
{
    std::unique_ptr<proto::KeyCredential> keyCredential(
        new proto::KeyCredential);

    if (!keyCredential) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate keyCredential protobuf.")
            .Flush();

        return false;
    }

    keyCredential->set_version(subversion_);

    // These must be serialized in this order
    bool auth = addKeytoSerializedKeyCredential(
        *keyCredential, addPrivate, proto::KEYROLE_AUTH);
    bool encrypt = addKeytoSerializedKeyCredential(
        *keyCredential, addPrivate, proto::KEYROLE_ENCRYPT);
    bool sign = addKeytoSerializedKeyCredential(
        *keyCredential, addPrivate, proto::KEYROLE_SIGN);

    if (auth && encrypt && sign) {
        if (addPrivate) {
            keyCredential->set_mode(proto::KEYMODE_PRIVATE);
            credential->set_allocated_privatecredential(
                keyCredential.release());

            return true;
        } else {
            keyCredential->set_mode(proto::KEYMODE_PUBLIC);
            credential->set_allocated_publiccredential(keyCredential.release());

            return true;
        }
    }

    return false;
}

auto Key::addKeytoSerializedKeyCredential(
    proto::KeyCredential& credential,
    const bool getPrivate,
    const proto::KeyRole role) const -> bool
{
    std::shared_ptr<proto::AsymmetricKey> key{nullptr};
    const crypto::key::Keypair* pKey{nullptr};

    switch (role) {
        case proto::KEYROLE_AUTH: {
            pKey = &authentication_key_.get();
        } break;
        case proto::KEYROLE_ENCRYPT: {
            pKey = &encryption_key_.get();
        } break;
        case proto::KEYROLE_SIGN: {
            pKey = &signing_key_.get();
        } break;
        default: {
            return false;
        }
    }

    if (nullptr == pKey) { return false; }

    key = pKey->GetSerialized(getPrivate);

    if (!key) { return false; }

    key->set_role(role);

    auto newKey = credential.add_key();
    *newKey = *key;

    return true;
}

auto Key::deserialize_key(
    const api::internal::Core& api,
    const int index,
    const proto::Credential& credential) -> OTKeypair
{
    const bool hasPrivate =
        (proto::KEYMODE_PRIVATE == credential.mode()) ? true : false;

    const auto publicKey = credential.publiccredential().key(index - 1);

    if (hasPrivate) {
        const auto privateKey = credential.privatecredential().key(index - 1);

        return api.Factory().Keypair(publicKey, privateKey);
    }

    return api.Factory().Keypair(publicKey);
}

auto Key::GetKeypair(
    const crypto::AsymmetricKeyType type,
    const proto::KeyRole role) const -> const crypto::key::Keypair&
{
    const crypto::key::Keypair* output{nullptr};

    switch (role) {
        case proto::KEYROLE_AUTH: {
            output = &authentication_key_.get();
        } break;
        case proto::KEYROLE_ENCRYPT: {
            output = &encryption_key_.get();
        } break;
        case proto::KEYROLE_SIGN: {
            output = &signing_key_.get();
        } break;
        default: {
            throw std::out_of_range("wrong key type");
        }
    }

    OT_ASSERT(nullptr != output);

    if (crypto::AsymmetricKeyType::Null != type) {
        if (type != output->GetPublicKey().keyType()) {
            throw std::out_of_range("wrong key type");
        }
    }

    return *output;
}

// NOTE: You might ask, if we are using theSignature's metadata to narrow down
// the key type, then why are we still passing the key type as a separate
// parameter? Good question. Because often, theSignature will have no metadata
// at all! In that case, normally we would just NOT return any keys, period.
// Because we assume, if a key credential signed it, then it WILL have metadata,
// and if it doesn't have metadata, then a key credential did NOT sign it, and
// therefore we know from the get-go that none of the keys from the key
// credentials will work to verify it, either. That's why, normally, we don't
// return any keys if theSignature has no metadata. BUT...Let's say you know
// this, that the signature has no metadata, yet you also still believe it may
// be signed with one of these keys. Further, while you don't know exactly which
// key it actually is, let's say you DO know by context that it's a signing key,
// or an authentication key, or an encryption key. So you specify that. In which
// case, OT should return all possible matching pubkeys based on that 1-letter
// criteria, instead of its normal behavior, which is to return all possible
// matching pubkeys based on a full match of the metadata.
auto Key::GetPublicKeysBySignature(
    crypto::key::Keypair::Keys& listOutput,
    const opentxs::Signature& theSignature,
    char cKeyType) const
    -> std::int32_t  // 'S' (signing key) or 'E' (encryption key)
                     // or 'A' (authentication key)
{
    // Key type was not specified, because we only want keys that match the
    // metadata on theSignature.
    // And if theSignature has no metadata, then we want to return 0 keys.
    if (('0' == cKeyType) && !theSignature.getMetaData().HasMetadata())
        return 0;

    // By this point, we know that EITHER exact metadata matches must occur, and
    // the signature DOES have metadata, ('0')
    // OR the search is only for 'A', 'E', or 'S' candidates, based on cKeyType,
    // and that the signature's metadata
    // can additionally narrow the search down, if it's present, which in this
    // case it's not guaranteed to be.
    std::int32_t nCount = 0;

    switch (cKeyType) {
        // Specific search only for signatures with metadata.
        // FYI, theSignature.getMetaData().HasMetadata() is true, in this case.
        case '0': {
            // That's why I can just assume theSignature has a key type here:
            switch (theSignature.getMetaData().GetKeyType()) {
                case 'A':
                    nCount = authentication_key_->GetPublicKeyBySignature(
                        listOutput, theSignature);
                    break;  // bInclusive=false by default
                case 'E':
                    nCount = encryption_key_->GetPublicKeyBySignature(
                        listOutput, theSignature);
                    break;  // bInclusive=false by default
                case 'S':
                    nCount = signing_key_->GetPublicKeyBySignature(
                        listOutput, theSignature);
                    break;  // bInclusive=false by default
                default:
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Unexpected keytype value in signature metadata: ")(
                        theSignature.getMetaData().GetKeyType())(" (Failure)!")
                        .Flush();
                    return 0;
            }
            break;
        }
        // Generalized search which specifies key type and returns keys
        // even for signatures with no metadata. (When metadata is present,
        // it's still used to eliminate keys.)
        case 'A':
            nCount = authentication_key_->GetPublicKeyBySignature(
                listOutput, theSignature, true);
            break;  // bInclusive=true
        case 'E':
            nCount = encryption_key_->GetPublicKeyBySignature(
                listOutput, theSignature, true);
            break;  // bInclusive=true
        case 'S':
            nCount = signing_key_->GetPublicKeyBySignature(
                listOutput, theSignature, true);
            break;  // bInclusive=true
        default:
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unexpected value for cKeyType (should be 0, A, E, or S): ")(
                cKeyType)(".")
                .Flush();
            return 0;
    }
    return nCount;
}

auto Key::hasCapability(const NymCapability& capability) const -> bool
{
    switch (capability) {
        case (NymCapability::SIGN_MESSAGE): {
            return signing_key_->CheckCapability(capability);
        }
        case (NymCapability::ENCRYPT_MESSAGE): {
            return encryption_key_->CheckCapability(capability);
        }
        case (NymCapability::AUTHENTICATE_CONNECTION): {
            return authentication_key_->CheckCapability(capability);
        }
        default: {
        }
    }

    return false;
}

auto Key::new_key(
    const api::internal::Core& api,
    const proto::KeyRole role,
    const NymParameters& params,
    const VersionNumber version,
    const PasswordPrompt& reason,
    const ReadView dh) noexcept(false) -> OTKeypair
{
    switch (params.credentialType()) {
        case identity::CredentialType::Legacy: {
            auto revised{params};
#if OT_CRYPTO_SUPPORTED_KEY_RSA
            revised.SetDHParams(dh);
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA

            return api.Factory().Keypair(revised, version, role, reason);
        }
        case identity::CredentialType::HD:
#if OT_CRYPTO_WITH_BIP32
        {
            const auto curve = crypto::AsymmetricProvider::KeyTypeToCurve(
                params.AsymmetricKeyType());

            if (EcdsaCurve::invalid == curve) {
                throw std::runtime_error("Invalid curve type");
            }

            return api.Factory().Keypair(
                params.Seed(),
                params.Nym(),
                params.Credset(),
                params.CredIndex(),
                curve,
                role,
                reason);
        }
#endif  // OT_CRYPTO_WITH_BIP32
        default: {
            throw std::runtime_error("Unsupported credential type");
        }
    }
}

auto Key::SelfSign(
    const PasswordPrompt& reason,
    const std::optional<OTSecret>,
    const bool onlyPrivate) -> bool
{
    Lock lock(lock_);
    auto publicSignature = std::make_shared<proto::Signature>();
    auto privateSignature = std::make_shared<proto::Signature>();
    bool havePublicSig = false;

    if (!onlyPrivate) {
        const auto publicVersion =
            serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);
        auto& signature = *publicVersion->add_signature();
        havePublicSig = Sign(
            [&]() -> std::string { return proto::ToString(*publicVersion); },
            crypto::SignatureRole::PublicCredential,
            signature,
            reason,
            proto::KEYROLE_SIGN,
            proto::HASHTYPE_ERROR);

        OT_ASSERT(havePublicSig);

        if (havePublicSig) {
            publicSignature->CopyFrom(signature);
            signatures_.push_back(publicSignature);
        }
    }

    auto privateVersion = serialize(lock, AS_PRIVATE, WITHOUT_SIGNATURES);
    auto& signature = *privateVersion->add_signature();
    const bool havePrivateSig = Sign(
        [&]() -> std::string { return proto::ToString(*privateVersion); },
        crypto::SignatureRole::PrivateCredential,
        signature,
        reason,
        proto::KEYROLE_SIGN,
        proto::HASHTYPE_ERROR);

    OT_ASSERT(havePrivateSig);

    if (havePrivateSig) {
        privateSignature->CopyFrom(signature);
        signatures_.push_back(privateSignature);
    }

    return ((havePublicSig | onlyPrivate) && havePrivateSig);
}

auto Key::serialize(
    const Lock& lock,
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<Base::SerializedType>
{
    auto serializedCredential = Base::serialize(lock, asPrivate, asSigned);

    addKeyCredentialtoSerializedCredential(serializedCredential, false);

    if (asPrivate) {
        addKeyCredentialtoSerializedCredential(serializedCredential, true);
    }

    return serializedCredential;
}

auto Key::Sign(
    const GetPreimage input,
    const crypto::SignatureRole role,
    proto::Signature& signature,
    const PasswordPrompt& reason,
    proto::KeyRole key,
    const proto::HashType hash) const -> bool
{
    const crypto::key::Keypair* keyToUse{nullptr};

    switch (key) {
        case (proto::KEYROLE_AUTH): {
            keyToUse = &authentication_key_.get();
        } break;
        case (proto::KEYROLE_SIGN): {
            keyToUse = &signing_key_.get();
        } break;
        case (proto::KEYROLE_ERROR):
        case (proto::KEYROLE_ENCRYPT):
        default: {
            LogOutput(": Can not sign with the specified key.").Flush();
            return false;
        }
    }

    if (nullptr != keyToUse) {
        try {
            return keyToUse->GetPrivateKey().Sign(
                input, role, signature, id_, reason, hash);
        } catch (...) {
        }
    }

    return false;
}

auto Key::signing_key(
    const api::internal::Core& api,
    const NymParameters& params,
    const VersionNumber subversion,
    const bool useProvided,
    const PasswordPrompt& reason) noexcept(false) -> OTKeypair
{
    if (useProvided) {
        if (params.Keypair()) {

            return params.Keypair();
        } else {
            throw std::runtime_error("Invalid provided keypair");
        }
    } else {

        return new_key(
            api,
            proto::KEYROLE_SIGN,
            params,
            subversion_to_key_version_.at(subversion),
            reason);
    }
}

auto Key::TransportKey(
    Data& publicKey,
    Secret& privateKey,
    const PasswordPrompt& reason) const -> bool
{
    return authentication_key_->GetTransportKey(publicKey, privateKey, reason);
}

auto Key::Verify(
    const Data& plaintext,
    const proto::Signature& sig,
    const proto::KeyRole key) const -> bool
{
    const crypto::key::Keypair* keyToUse = nullptr;

    switch (key) {
        case (proto::KEYROLE_AUTH):
            keyToUse = &authentication_key_.get();
            break;
        case (proto::KEYROLE_SIGN):
            keyToUse = &signing_key_.get();
            break;
        default:
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Can not verify signatures with the "
                "specified key.")
                .Flush();
            return false;
    }

    OT_ASSERT(nullptr != keyToUse);

    try {
        return keyToUse->GetPublicKey().Verify(plaintext, sig);
    } catch (...) {
        return false;
    }
}

void Key::sign(
    const identity::credential::internal::Primary& master,
    const PasswordPrompt& reason) noexcept(false)
{
    Base::sign(master, reason);

    if (false == SelfSign(reason)) {
        throw std::runtime_error("Failed to obtain self signature");
    }
}

auto Key::verify_internally(const Lock& lock) const -> bool
{
    // Perform common Credential verifications
    if (!Base::verify_internally(lock)) { return false; }

    // All KeyCredentials must sign themselves
    if (!VerifySignedBySelf(lock)) {
        LogNormal(OT_METHOD)(__FUNCTION__)(
            ": Failed verifying key credential: it's not "
            "signed by itself (its own signing key).")
            .Flush();
        return false;
    }

    return true;
}

auto Key::VerifySig(
    const Lock& lock,
    const proto::Signature& sig,
    const CredentialModeFlag asPrivate) const -> bool
{
    std::shared_ptr<Base::SerializedType> serialized;

    if ((proto::KEYMODE_PRIVATE != mode_) && asPrivate) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Can not serialize a public credential as a private credential.")
            .Flush();
        return false;
    }

    if (asPrivate) {
        serialized = serialize(lock, AS_PRIVATE, WITHOUT_SIGNATURES);
    } else {
        serialized = serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);
    }

    auto& signature = *serialized->add_signature();
    signature.CopyFrom(sig);
    signature.clear_signature();
    auto plaintext = api_.Factory().Data(*serialized);

    return Verify(plaintext, sig, proto::KEYROLE_SIGN);
}

auto Key::VerifySignedBySelf(const Lock& lock) const -> bool
{
    auto publicSig = SelfSignature(PUBLIC_VERSION);

    if (!publicSig) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Could not find public self signature.")
            .Flush();

        return false;
    }

    bool goodPublic = VerifySig(lock, *publicSig, PUBLIC_VERSION);

    if (!goodPublic) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Could not verify public self signature.")
            .Flush();

        return false;
    }

    if (Private()) {
        auto privateSig = SelfSignature(PRIVATE_VERSION);

        if (!privateSig) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Could not find private self signature.")
                .Flush();

            return false;
        }

        bool goodPrivate = VerifySig(lock, *privateSig, PRIVATE_VERSION);

        if (!goodPrivate) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Could not verify private self signature.")
                .Flush();

            return false;
        }
    }

    return true;
}
}  // namespace opentxs::identity::credential::implementation

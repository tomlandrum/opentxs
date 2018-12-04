// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_KEYCREDENTIAL_HPP
#define OPENTXS_CORE_CRYPTO_KEYCREDENTIAL_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/crypto/Credential.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <memory>

// A nym contains a list of credential sets.
// The whole purpose of a Nym is to be an identity, which can have
// master credentials.
//
// Each CredentialSet contains list of Credentials. One of the
// Credentials is a MasterCredential, and the rest are ChildCredentials
// signed by the MasterCredential.
//
// A Credential may contain keys, in which case it is a KeyCredential.
//
// Credentials without keys might be an interface to a hardware device
// or other kind of external encryption and authentication system.
//
// Non-key Credentials are not yet implemented.
//
// Each KeyCredential has 3 OTKeypairs: encryption, signing, and authentication.
// Each OTKeypair has 2 crypto::key::Asymmetrics (public and private.)
//
// A MasterCredential must be a KeyCredential, and is only used to sign
// ChildCredentials
//
// ChildCredentials are used for all other actions, and never sign other
// Credentials

namespace opentxs
{
/// KeyCredential
/// A form of Credential that contains 3 key pairs: signing,
/// authentication, and encryption.
/// We won't use KeyCredential directly but only as a common base class for
/// ChildKeyCredential and MasterCredential.
///
class KeyCredential : public Credential
{
private:
    typedef Credential ot_super;
    friend class CredentialSet;
    KeyCredential() = delete;

    bool addKeytoSerializedKeyCredential(
        proto::KeyCredential& credential,
        const bool getPrivate,
        const proto::KeyRole role) const;
    bool addKeyCredentialtoSerializedCredential(
        serializedCredential credential,
        const bool addPrivate) const;
    bool VerifySig(
        const Lock& lock,
        const proto::Signature& sig,
        const CredentialModeFlag asPrivate = PRIVATE_VERSION) const;
    bool VerifySignedBySelf(const Lock& lock) const;

protected:
    serializedCredential serialize(
        const Lock& lock,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const override;
    bool verify_internally(const Lock& lock) const override;

    bool New(const NymParameters& nymParameters) override;
    virtual bool SelfSign(
        const OTPassword* exportPassword = nullptr,
        const OTPasswordData* pPWData = nullptr,
        const bool onlyPrivate = false);

    KeyCredential(
        const api::Core& api,
        CredentialSet& owner,
        const NymParameters& nymParameters);
    KeyCredential(
        const api::Core& api,
        CredentialSet& owner,
        const proto::Credential& serializedCred);

public:
    OTKeypair signing_key_;
    OTKeypair authentication_key_;
    OTKeypair encryption_key_;

    bool ReEncryptKeys(const OTPassword& theExportPassword, bool bImporting);
    EXPORT std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType = '0') const;  // 'S' (signing key) or
                                     // 'E' (encryption key)
                                     // or 'A'
                                     // (authentication key)

    const crypto::key::Keypair& GetKeypair(
        const proto::AsymmetricKeyType type,
        const proto::KeyRole role) const;
    bool hasCapability(const NymCapability& capability) const override;

    using ot_super::Verify;
    bool Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const proto::KeyRole key = proto::KEYROLE_SIGN) const override;
    bool TransportKey(Data& publicKey, OTPassword& privateKey) const override;

    virtual ~KeyCredential() = default;

    template <class C>
    bool SignProto(
        C& serialized,
        proto::Signature& signature,
        proto::KeyRole key = proto::KEYROLE_SIGN,
        const OTPasswordData* pPWData = nullptr) const
    {
        const crypto::key::Keypair* keyToUse{nullptr};

        switch (key) {
            case (proto::KEYROLE_AUTH):
                keyToUse = &authentication_key_.get();
                break;
            case (proto::KEYROLE_SIGN):
                keyToUse = &signing_key_.get();
                break;
            case (proto::KEYROLE_ERROR):
            case (proto::KEYROLE_ENCRYPT):
            default:
                LogOutput(": Can not sign with the "
                          "specified key.")
                    .Flush();
                return false;
        }

        if (nullptr != keyToUse) {
            return keyToUse->SignProto<C>(
                serialized, signature, String::Factory(id_), pPWData);
        }

        return false;
    }

private:
    static OTKeypair deserialize_key(
        const int index,
        const proto::Credential& credential);
#if OT_CRYPTO_SUPPORTED_KEY_HD
    static OTKeypair derive_hd_keypair(
        const api::Crypto& crypto,
        const OTPassword& seed,
        const std::string& fingerprint,
        const std::uint32_t nym,
        const std::uint32_t credset,
        const std::uint32_t credindex,
        const EcdsaCurve& curve,
        const proto::KeyRole role);
#endif
    static OTKeypair new_key(
        const api::Crypto& crypto,
        const proto::KeyRole role,
        const NymParameters& nymParameters);
};
}  // namespace opentxs
#endif

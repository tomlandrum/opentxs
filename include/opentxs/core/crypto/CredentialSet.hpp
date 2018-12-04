// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_CREDENTIALSET_HPP
#define OPENTXS_CORE_CRYPTO_CREDENTIALSET_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/crypto/Credential.hpp"
#include "opentxs/core/crypto/MasterCredential.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/NymIDSource.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <memory>

// A nym contains a list of credential sets.
// The whole purpose of a Nym is to be an identity, which can have master
// credentials.
//
// Each CredentialSet contains list of Credentials. One of the Credentials is a
// MasterCredential, and the rest are ChildCredentials  signed by the
// MasterCredential.
//
// A Credential may contain keys, in which case it is a KeyCredential.
//
// Credentials without keys might be an interface to a hardware device or other
// kind of external encryption and authentication system.
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
typedef std::map<std::string, std::unique_ptr<Credential>> mapOfCredentials;
typedef std::shared_ptr<proto::CredentialSet> SerializedCredentialSet;

/** Contains a MasterCredential, and a list of child credentials signed by that
 master. Each child credential can generate its own Credential contract, even
 the MasterCredential, so a CredentialSet object includes many Credentials.
 (That is, each may be issued at separate times, each may be registered on a
 server at separate times, etc.)

 Each nym has multiple CredentialSets because there may be several master keys,
 each with their own child credentials.

 Two things to verify on a credential set:

 1. If you hash m_pstrSourceForNymID, you should get m_pstrNymID.
 2. m_pstrSourceForNymID should somehow verify m_MasterCredential.GetContents().
 For example, if m_pstrSourceForNymID contains CA DN info, then GetContents
 should contain a verifiable Cert with that same DN info. Another example, if
 m_pstrSourceForNymID contains a public key, then m_MasterCredential.GetContents
 should contain that same public key, or a cert that contains it. Another
 example, if m_pstrSourceForNymID contains a URL, then
 m_MasterCredential.GetContents should contain a public key found at that URL,
 or a public key that, when hashed, matches one of the hashes posted at that
 URL. */
class CredentialSet
{
public:
    EXPORT CredentialSet(const api::Core& api);
    EXPORT CredentialSet(
        const api::Core& api,
        const proto::KeyMode mode,
        const proto::CredentialSet& serializedCredentialSet);
    EXPORT CredentialSet(
        const api::Core& api,
        const NymParameters& nymParameters,
        std::uint32_t version,
        const OTPasswordData* pPWData = nullptr);

    /** The source is the URL/DN/pubkey that hashes to form the NymID. Any
     * credential must verify against its own source. */
    void SetSource(const std::shared_ptr<NymIDSource>& source);
    EXPORT const OTPassword* GetImportPassword() const
    {
        return m_pImportPassword;
    }
    EXPORT void SetImportPassword(const OTPassword* pImportPassword)
    {
        m_pImportPassword = pImportPassword;
    }

    EXPORT OTString MasterAsString() const;

    EXPORT bool Load_Master(
        const String& strNymID,
        const String& strMasterCredID,
        const OTPasswordData* pPWData = nullptr);
    EXPORT bool Load_MasterFromString(
        const String& strInput,
        const String& strNymID,
        const String& strMasterCredID,
        const OTPasswordData* pPWData = nullptr,
        const OTPassword* pImportPassword = nullptr);

    EXPORT bool Path(proto::HDPath& output) const;
    /** Like for when you are exporting a Nym from the wallet. */
    EXPORT bool ReEncryptPrivateCredentials(
        const OTPassword& theExportPassword,
        bool bImporting);
    EXPORT bool LoadChildKeyCredential(const String& strSubID);
    EXPORT bool LoadChildKeyCredential(const proto::Credential& serializedCred);
    EXPORT bool LoadChildKeyCredentialFromString(
        const String& strInput,
        const String& strSubID,
        const OTPassword* pImportPassword = nullptr);
    EXPORT size_t GetChildCredentialCount() const;
    EXPORT const Credential* GetChildCredential(
        const String& strSubID,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const Credential* GetChildCredentialByIndex(
        std::int32_t nIndex) const;
    EXPORT const std::string GetChildCredentialIDByIndex(size_t nIndex) const;
    /** Returns: m_MasterCredential's public credential protobuf. */
    EXPORT const serializedCredential GetSerializedPubCredential() const;
    /** Returns: Master Credential ID */
    EXPORT const std::string GetMasterCredID() const;
    EXPORT const std::string& GetNymID() const;
    EXPORT const NymIDSource& Source() const;
    EXPORT bool hasCapability(const NymCapability& capability) const;

    /** listRevokedIDs should contain a list of std::strings for IDs of
     * already-revoked credentials. That way, SerializeIDs will know whether to
     * mark them as valid while serializing them. bShowRevoked allows us to
     * include/exclude the revoked credentials from the output (filter for
     * valid-only.) bValid=true means we are saving
     * Nym::m_mapCredentials. Whereas bValid=false means we're saving
     * m_mapRevoked. */
    SerializedCredentialSet Serialize(const CredentialIndexModeFlag mode) const;
    EXPORT void SerializeIDs(
        Tag& parent,
        const String::List& listRevokedIDs,
        String::Map* pmapPubInfo = nullptr,
        String::Map* pmapPriInfo = nullptr,
        bool bShowRevoked = false,
        bool bValid = true) const;
    EXPORT bool VerifyInternally() const;
    EXPORT const MasterCredential& GetMasterCredential() const
    {
        return *m_MasterCredential;
    }
    EXPORT std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType = '0') const;  // 'S' (signing key) or 'E' (encryption
                                     // key) or 'A' (authentication key)
    EXPORT const crypto::key::Asymmetric& GetPublicAuthKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Asymmetric& GetPublicEncrKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Asymmetric& GetPublicSignKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Asymmetric& GetPrivateSignKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Asymmetric& GetPrivateEncrKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Asymmetric& GetPrivateAuthKey(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Keypair& GetAuthKeypair(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Keypair& GetEncrKeypair(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT const crypto::key::Keypair& GetSignKeypair(
        proto::AsymmetricKeyType keytype,
        const String::List* plistRevokedIDs = nullptr) const;
    EXPORT void ClearChildCredentials();
    EXPORT bool WriteCredentials() const;

    std::string AddChildKeyCredential(const NymParameters& nymParameters);
    bool GetContactData(std::unique_ptr<proto::ContactData>& contactData) const;
    void RevokeContactCredentials(std::list<std::string>& contactCredentialIDs);
    bool AddContactCredential(const proto::ContactData& contactData);

    bool GetVerificationSet(
        std::unique_ptr<proto::VerificationSet>& verificationSet) const;
    void RevokeVerificationCredentials(
        std::list<std::string>& verificationCredentialIDs);
    bool AddVerificationCredential(
        const proto::VerificationSet& verificationSet);

    bool Sign(
        const MasterCredential& credential,
        proto::Signature& sig,
        const OTPasswordData* pPWData = nullptr) const;

    bool Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const proto::KeyRole key = proto::KEYROLE_SIGN) const;
    bool Verify(const proto::Verification& item) const;
    bool TransportKey(Data& publicKey, OTPassword& privateKey) const;

    template <class C>
    bool SignProto(
        C& serialized,
        proto::Signature& signature,
        const OTPasswordData* pPWData = nullptr,
        proto::KeyRole key = proto::KEYROLE_SIGN) const
    {
        switch (signature.role()) {
            case (proto::SIGROLE_PUBCREDENTIAL): {
                if (m_MasterCredential->hasCapability(
                        NymCapability::SIGN_CHILDCRED)) {
                    return m_MasterCredential->SignProto<C>(
                        serialized, signature, key, pPWData);
                }

                break;
            }
            case (proto::SIGROLE_NYMIDSOURCE): {
                LogOutput(": Credentials to be signed "
                          "with a nym source can not use this method.")
                    .Flush();

                return false;
            }
            case (proto::SIGROLE_PRIVCREDENTIAL): {
                LogOutput(": Private credential can not "
                          "use this method.")
                    .Flush();

                return false;
            }

            // TODO Justus
            case (proto::SIGROLE_ERROR):
            case (proto::SIGROLE_CLAIM):
            case (proto::SIGROLE_SERVERCONTRACT):
            case (proto::SIGROLE_UNITDEFINITION):
            case (proto::SIGROLE_PEERREQUEST):
            case (proto::SIGROLE_PEERREPLY):
            case (proto::SIGROLE_CONTEXT):
            case (proto::SIGROLE_ACCOUNT):

            default: {
                bool haveSignature = false;

                for (auto& it : m_mapCredentials) {
                    auto& credential = it.second;

                    if (nullptr != credential) {
                        if (credential->hasCapability(
                                NymCapability::SIGN_MESSAGE)) {
                            const auto keyCredential =
                                dynamic_cast<const KeyCredential*>(
                                    credential.get());
                            haveSignature = keyCredential->SignProto<C>(
                                serialized, signature, key, pPWData);
                        }

                        if (haveSignature) { return true; }
                    }
                }
            }
        }

        return false;
    }

    EXPORT ~CredentialSet();

private:
    const api::Core& api_;
    std::unique_ptr<MasterCredential> m_MasterCredential;
    mapOfCredentials m_mapCredentials;
    mapOfCredentials m_mapRevokedCredentials;
    std::string m_strNymID;
    std::shared_ptr<NymIDSource> nym_id_source_;
    const OTPassword* m_pImportPassword =
        nullptr;  // Not owned. Just here for convenience.  Sometimes it will be
                  // set, so that when loading something up (and decrypting it)
                  // the password is already available, so the user doesn't have
                  // to type it a million times (such as during import.) So we
                  // use it when it's available. And usually whoever set it,
                  // will immediately set it back to nullptr when he's done.
    std::uint32_t version_{0};
    std::uint32_t index_{0};
    proto::KeyMode mode_{proto::KEYMODE_ERROR};

    bool CreateMasterCredential(const NymParameters& nymParameters);

    CredentialSet() = delete;
};
}  // namespace opentxs
#endif

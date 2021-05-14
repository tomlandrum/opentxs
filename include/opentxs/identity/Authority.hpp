// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_AUTHORITY_HPP
#define OPENTXS_IDENTITY_AUTHORITY_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>

#include "opentxs/Bytes.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/asymmetric/Role.hpp"
#include "opentxs/identity/Nym.hpp"

namespace opentxs
{
namespace identity
{
namespace credential
{
class Key;
}  // namespace credential
}  // namespace identity

namespace proto
{
class Authority;
class ContactData;
class HDPath;
class Signature;
class Verification;
class VerificationSet;
}  // namespace proto

class Secret;
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
class OPENTXS_EXPORT Authority
{
public:
    using AuthorityKeys = Nym::AuthorityKeys;
    using Serialized = proto::Authority;

    virtual VersionNumber ContactCredentialVersion() const = 0;
    virtual AuthorityKeys EncryptionTargets() const noexcept = 0;
    virtual bool GetContactData(proto::ContactData& contactData) const = 0;
    virtual OTIdentifier GetMasterCredID() const = 0;
    virtual const crypto::key::Asymmetric& GetPublicAuthKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Asymmetric& GetPublicEncrKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType = '0') const = 0;
    virtual const crypto::key::Asymmetric& GetPublicSignKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateSignKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateEncrKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateAuthKey(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Keypair& GetAuthKeypair(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Keypair& GetEncrKeypair(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const crypto::key::Keypair& GetSignKeypair(
        crypto::key::asymmetric::Algorithm keytype,
        const String::List* plistRevokedIDs = nullptr) const = 0;
    virtual const credential::Key& GetTagCredential(
        crypto::key::asymmetric::Algorithm keytype) const noexcept(false) = 0;
    virtual bool GetVerificationSet(
        proto::VerificationSet& verificationSet) const = 0;
    virtual bool hasCapability(const NymCapability& capability) const = 0;
    virtual ReadView Params(
        const crypto::key::asymmetric::Algorithm type) const noexcept = 0;
    virtual bool Path(proto::HDPath& output) const = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        Serialized& serialized,
        const CredentialIndexModeFlag mode) const = 0;
    virtual bool Sign(
        const GetPreimage input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        opentxs::crypto::key::asymmetric::Role key =
            opentxs::crypto::key::asymmetric::Role::Sign,
        const crypto::HashType hash = crypto::HashType::Error) const = 0;
    virtual const identity::Source& Source() const = 0;
    virtual bool TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const = 0;
    virtual bool Unlock(
        const crypto::key::Asymmetric& dhKey,
        const std::uint32_t tag,
        const crypto::key::asymmetric::Algorithm type,
        const crypto::key::Symmetric& key,
        PasswordPrompt& reason) const noexcept = 0;
    virtual VersionNumber VerificationCredentialVersion() const = 0;
    virtual bool Verify(
        const Data& plaintext,
        const proto::Signature& sig,
        const opentxs::crypto::key::asymmetric::Role key =
            opentxs::crypto::key::asymmetric::Role::Sign) const = 0;
    virtual bool Verify(const proto::Verification& item) const = 0;
    virtual bool VerifyInternally() const = 0;

    virtual std::string AddChildKeyCredential(
        const NymParameters& nymParameters,
        const PasswordPrompt& reason) = 0;
    virtual bool AddVerificationCredential(
        const proto::VerificationSet& verificationSet,
        const PasswordPrompt& reason) = 0;
    virtual bool AddContactCredential(
        const proto::ContactData& contactData,
        const PasswordPrompt& reason) = 0;
    virtual void RevokeContactCredentials(
        std::list<std::string>& contactCredentialIDs) = 0;
    virtual void RevokeVerificationCredentials(
        std::list<std::string>& verificationCredentialIDs) = 0;

    virtual ~Authority() = default;

protected:
    Authority() noexcept = default;

private:
    Authority(const Authority&) = delete;
    Authority(Authority&&) = delete;
    Authority& operator=(const Authority&) = delete;
    Authority& operator=(Authority&&) = delete;
};
}  // namespace identity
}  // namespace opentxs
#endif

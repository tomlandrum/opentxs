// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_NYM_HPP
#define OPENTXS_IDENTITY_NYM_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <tuple>

#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/HashType.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/iterator/Bidirectional.hpp"

namespace google
{
namespace protobuf
{
class MessageLite;
}  // namespace protobuf
}  // namespace google

namespace opentxs
{
namespace crypto
{
namespace key
{
class Symmetric;
}  // namespace key
}  // namespace crypto

namespace identifier
{
class Nym;
class UnitDefinition;
}  // namespace identifier

namespace identity
{
class Authority;
class Source;
}  // namespace identity

namespace proto
{
class ContactData;
class Nym;
class Signature;
}  // namespace proto

class ContactData;
class NymParameters;
class PasswordPrompt;
class PaymentCode;
class Signature;
class Tag;
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
class OPENTXS_EXPORT Nym
{
public:
    using KeyTypes = std::vector<crypto::key::asymmetric::Algorithm>;
    using AuthorityKeys = std::pair<OTIdentifier, KeyTypes>;
    using NymKeys = std::pair<OTNymID, std::vector<AuthorityKeys>>;
    using Serialized = proto::Nym;
    using key_type = Identifier;
    using value_type = Authority;
    using const_iterator =
        opentxs::iterator::Bidirectional<const Nym, const value_type>;

    static const VersionNumber DefaultVersion;
    static const VersionNumber MaxVersion;

    virtual std::string Alias() const = 0;
    virtual const value_type& at(const key_type& id) const noexcept(false) = 0;
    virtual const value_type& at(const std::size_t& index) const
        noexcept(false) = 0;
    virtual const_iterator begin() const noexcept = 0;
    virtual std::string BestEmail() const = 0;
    virtual std::string BestPhoneNumber() const = 0;
    virtual std::string BestSocialMediaProfile(
        const contact::ContactItemType type) const = 0;
    virtual const_iterator cbegin() const noexcept = 0;
    virtual const_iterator cend() const noexcept = 0;
    virtual const opentxs::ContactData& Claims() const = 0;
    virtual bool CompareID(const Nym& RHS) const = 0;
    virtual bool CompareID(const identifier::Nym& rhs) const = 0;
    virtual VersionNumber ContactCredentialVersion() const = 0;
    virtual VersionNumber ContactDataVersion() const = 0;
    virtual std::set<OTIdentifier> Contracts(
        const contact::ContactItemType currency,
        const bool onlyActive) const = 0;
    virtual std::string EmailAddresses(bool active = true) const = 0;
    virtual NymKeys EncryptionTargets() const noexcept = 0;
    virtual const_iterator end() const noexcept = 0;
    virtual void GetIdentifier(identifier::Nym& theIdentifier) const = 0;
    virtual void GetIdentifier(String& theIdentifier) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateAuthKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateEncrKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;
    virtual const crypto::key::Asymmetric& GetPrivateSignKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;

    virtual const crypto::key::Asymmetric& GetPublicAuthKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;
    virtual const crypto::key::Asymmetric& GetPublicEncrKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;
    // OT uses the signature's metadata to narrow down its search for the
    // correct public key.
    // 'S' (signing key) or
    // 'E' (encryption key) OR
    // 'A' (authentication key)
    virtual std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType = '0') const = 0;
    virtual const crypto::key::Asymmetric& GetPublicSignKey(
        crypto::key::asymmetric::Algorithm keytype =
            crypto::key::asymmetric::Algorithm::Null) const = 0;
    virtual bool HasCapability(const NymCapability& capability) const = 0;
    virtual bool HasPath() const = 0;
    virtual const identifier::Nym& ID() const = 0;

    virtual std::string Name() const = 0;

    virtual bool Path(proto::HDPath& output) const = 0;
    virtual const std::string PathRoot() const = 0;
    virtual int PathChildSize() const = 0;
    virtual std::uint32_t PathChild(int index) const = 0;
    virtual std::string PaymentCode() const = 0;
    virtual bool PaymentCodePath(AllocateOutput destination) const = 0;
    virtual bool PaymentCodePath(proto::HDPath& output) const = 0;
    virtual std::string PhoneNumbers(bool active = true) const = 0;
    virtual std::uint64_t Revision() const = 0;

    virtual bool Serialize(AllocateOutput destination) const = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(Serialized& serialized) const = 0;
    virtual void SerializeNymIDSource(Tag& parent) const = 0;
    virtual bool Sign(
        const google::protobuf::MessageLite& input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        const crypto::HashType hash = crypto::HashType::Error) const = 0;
    virtual std::size_t size() const noexcept = 0;
    virtual std::string SocialMediaProfiles(
        const contact::ContactItemType type,
        bool active = true) const = 0;
    virtual const std::set<contact::ContactItemType> SocialMediaProfileTypes()
        const = 0;
    virtual const identity::Source& Source() const = 0;
    virtual OTSecret TransportKey(Data& pubkey, const PasswordPrompt& reason)
        const = 0;

    virtual bool Unlock(
        const crypto::key::Asymmetric& dhKey,
        const std::uint32_t tag,
        const crypto::key::asymmetric::Algorithm type,
        const crypto::key::Symmetric& key,
        PasswordPrompt& reason) const noexcept = 0;
    virtual bool Verify(
        const google::protobuf::MessageLite& input,
        proto::Signature& signature) const = 0;
    virtual bool VerifyPseudonym() const = 0;

    virtual std::string AddChildKeyCredential(
        const Identifier& strMasterID,
        const NymParameters& nymParameters,
        const PasswordPrompt& reason) = 0;
    virtual bool AddClaim(const Claim& claim, const PasswordPrompt& reason) = 0;
    virtual bool AddContract(
        const identifier::UnitDefinition& instrumentDefinitionID,
        const contact::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active = true) = 0;
    virtual bool AddEmail(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    virtual bool AddPaymentCode(
        const opentxs::PaymentCode& code,
        const contact::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active = true) = 0;
    virtual bool AddPhoneNumber(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    virtual bool AddPreferredOTServer(
        const Identifier& id,
        const PasswordPrompt& reason,
        const bool primary) = 0;
    virtual bool AddSocialMediaProfile(
        const std::string& value,
        const contact::ContactItemType type,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) = 0;
    virtual bool DeleteClaim(
        const Identifier& id,
        const PasswordPrompt& reason) = 0;
    virtual bool SetCommonName(
        const std::string& name,
        const PasswordPrompt& reason) = 0;
    virtual bool SetContactData(
        const proto::ContactData& data,
        const PasswordPrompt& reason) = 0;
    virtual bool SetScope(
        const contact::ContactItemType type,
        const std::string& name,
        const PasswordPrompt& reason,
        const bool primary) = 0;

    virtual ~Nym() = default;

protected:
    Nym() noexcept = default;

private:
    Nym(const Nym&) = delete;
    Nym(Nym&&) = delete;
    Nym& operator=(const Nym&) = delete;
    Nym& operator=(Nym&&) = delete;
};
}  // namespace identity
}  // namespace opentxs
#endif

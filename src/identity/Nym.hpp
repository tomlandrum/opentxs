// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "internal/identity/Identity.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"

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
}  // namespace crypto

namespace identifier
{
class UnitDefinition;
}  // namespace identifier

namespace proto
{
class ContactData;
class HDPath;
class Nym;
class Signature;
class VerificationSet;
}  // namespace proto

class Data;
class Factory;
class OTPassword;
class PasswordPrompt;
class PaymentCode;
class Signature;
class Tag;
}  // namespace opentxs

namespace opentxs::identity::implementation
{
class Nym final : virtual public identity::internal::Nym, Lockable
{
public:
    auto Alias() const -> std::string final;
    auto asPublicNym() const -> const Serialized final;
    auto at(const key_type& id) const noexcept(false) -> const value_type& final
    {
        return *active_.at(id);
    }
    auto at(const std::size_t& index) const noexcept(false)
        -> const value_type& final;
    auto begin() const noexcept -> const_iterator final { return cbegin(); }
    auto BestEmail() const -> std::string final;
    auto BestPhoneNumber() const -> std::string final;
    auto BestSocialMediaProfile(const proto::ContactItemType type) const
        -> std::string final;
    auto cbegin() const noexcept -> const_iterator final
    {
        return const_iterator(this, 0);
    }
    auto cend() const noexcept -> const_iterator final
    {
        return const_iterator(this, size());
    }
    auto Claims() const -> const opentxs::ContactData& final;
    auto CompareID(const identity::Nym& RHS) const -> bool final;
    auto CompareID(const identifier::Nym& rhs) const -> bool final;
    auto ContactCredentialVersion() const -> VersionNumber final;
    auto ContactDataVersion() const -> VersionNumber final
    {
        return contact_credential_to_contact_data_version_.at(
            ContactCredentialVersion());
    }
    auto Contracts(const proto::ContactItemType currency, const bool onlyActive)
        const -> std::set<OTIdentifier> final;
    auto EmailAddresses(bool active) const -> std::string final;
    auto EncryptionTargets() const noexcept -> NymKeys final;
    auto end() const noexcept -> const_iterator final { return cend(); }
    void GetIdentifier(identifier::Nym& theIdentifier) const final;
    void GetIdentifier(String& theIdentifier) const final;
    auto GetPrivateAuthKey(
        crypto::AsymmetricKeyType keytype = crypto::AsymmetricKeyType::Null)
        const -> const crypto::key::Asymmetric& final;
    auto GetPrivateEncrKey(
        crypto::AsymmetricKeyType keytype = crypto::AsymmetricKeyType::Null)
        const -> const crypto::key::Asymmetric& final;
    auto GetPrivateSignKey(
        crypto::AsymmetricKeyType keytype = crypto::AsymmetricKeyType::Null)
        const -> const crypto::key::Asymmetric& final;
    auto GetPublicAuthKey(
        crypto::AsymmetricKeyType keytype = crypto::AsymmetricKeyType::Null)
        const -> const crypto::key::Asymmetric& final;
    auto GetPublicEncrKey(
        crypto::AsymmetricKeyType keytype = crypto::AsymmetricKeyType::Null)
        const -> const crypto::key::Asymmetric& final;
    auto GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const Signature& theSignature,
        char cKeyType) const -> std::int32_t final;
    auto GetPublicSignKey(crypto::AsymmetricKeyType keytype) const
        -> const crypto::key::Asymmetric& final;
    auto HasCapability(const NymCapability& capability) const -> bool final;
    auto ID() const -> const identifier::Nym& final { return id_; }
    auto Name() const -> std::string final;
    auto Path(proto::HDPath& output) const -> bool final;
    auto PaymentCode() const -> std::string final;
    auto PaymentCodePath(proto::HDPath& output) const -> bool final;
    auto PhoneNumbers(bool active) const -> std::string final;
    auto Revision() const -> std::uint64_t final;
    auto SerializeCredentialIndex(const Mode mode) const -> Serialized final;
    void SerializeNymIDSource(Tag& parent) const final;
    auto size() const noexcept -> std::size_t final { return active_.size(); }
    auto SocialMediaProfiles(const proto::ContactItemType type, bool active)
        const -> std::string final;
    auto SocialMediaProfileTypes() const
        -> const std::set<proto::ContactItemType> final;
    auto Source() const -> const identity::Source& final { return source_; }
    auto TransportKey(Data& pubkey, const PasswordPrompt& reason) const
        -> OTSecret final;
    auto Unlock(
        const crypto::key::Asymmetric& dhKey,
        const std::uint32_t tag,
        const crypto::AsymmetricKeyType type,
        const crypto::key::Symmetric& key,
        PasswordPrompt& reason) const noexcept -> bool final;
    auto VerifyPseudonym() const -> bool final;
    auto WriteCredentials() const -> bool final;

    auto AddChildKeyCredential(
        const Identifier& strMasterID,
        const NymParameters& nymParameters,
        const PasswordPrompt& reason) -> std::string final;
    auto AddClaim(const Claim& claim, const PasswordPrompt& reason)
        -> bool final;
    auto AddContract(
        const identifier::UnitDefinition& instrumentDefinitionID,
        const proto::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) -> bool final;
    auto AddEmail(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) -> bool final;
    auto AddPaymentCode(
        const opentxs::PaymentCode& code,
        const proto::ContactItemType currency,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) -> bool final;
    auto AddPreferredOTServer(
        const Identifier& id,
        const PasswordPrompt& reason,
        const bool primary) -> bool final;
    auto AddPhoneNumber(
        const std::string& value,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) -> bool final;
    auto AddSocialMediaProfile(
        const std::string& value,
        const proto::ContactItemType type,
        const PasswordPrompt& reason,
        const bool primary,
        const bool active) -> bool final;
    auto DeleteClaim(const Identifier& id, const PasswordPrompt& reason)
        -> bool final;
    void SetAlias(const std::string& alias) final;
    void SetAliasStartup(const std::string& alias) final { alias_ = alias; }
    auto SetCommonName(const std::string& name, const PasswordPrompt& reason)
        -> bool final;
    auto SetContactData(
        const proto::ContactData& data,
        const PasswordPrompt& reason) -> bool final;
    auto SetScope(
        const proto::ContactItemType type,
        const std::string& name,
        const PasswordPrompt& reason,
        const bool primary) -> bool final;
    auto Sign(
        const ProtobufType& input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        const proto::HashType hash) const -> bool final;
    auto Verify(const ProtobufType& input, proto::Signature& signature) const
        -> bool final;

    ~Nym() final = default;

private:
    using MasterID = OTIdentifier;
    using CredentialMap =
        std::map<MasterID, std::unique_ptr<identity::internal::Authority>>;

    friend opentxs::Factory;

    static const VersionConversionMap akey_to_session_key_version_;
    static const VersionConversionMap
        contact_credential_to_contact_data_version_;

    const api::internal::Core& api_;
    const std::unique_ptr<const identity::Source> source_p_;
    const identity::Source& source_;
    const OTNymID id_;
    const proto::NymMode mode_;
    std::int32_t version_;
    std::uint32_t index_;
    std::string alias_;
    std::atomic<std::uint64_t> revision_;
    mutable std::unique_ptr<opentxs::ContactData> contact_data_;
    CredentialMap active_;
    CredentialMap m_mapRevokedSets;
    // Revoked child credential IDs
    String::List m_listRevokedIDs;

    static auto create_authority(
        const api::internal::Core& api,
        const identity::Nym& parent,
        const identity::Source& source,
        const VersionNumber version,
        const NymParameters& params,
        const PasswordPrompt& reason) noexcept(false) -> CredentialMap;
    static auto load_authorities(
        const api::internal::Core& api,
        const identity::Nym& parent,
        const identity::Source& source,
        const Serialized& serialized) noexcept(false) -> CredentialMap;
    static auto load_revoked(
        const api::internal::Core& api,
        const identity::Nym& parent,
        const identity::Source& source,
        const Serialized& serialized,
        CredentialMap& revoked) noexcept(false) -> String::List;
    static auto normalize(
        const api::internal::Core& api,
        const NymParameters& in,
        const PasswordPrompt& reason) noexcept(false) -> NymParameters;

    template <typename T>
    auto get_private_auth_key(const T& lock, crypto::AsymmetricKeyType keytype)
        const -> const crypto::key::Asymmetric&;
    template <typename T>
    auto get_private_sign_key(const T& lock, crypto::AsymmetricKeyType keytype)
        const -> const crypto::key::Asymmetric&;
    template <typename T>
    auto get_public_sign_key(const T& lock, crypto::AsymmetricKeyType keytype)
        const -> const crypto::key::Asymmetric&;
    auto has_capability(const eLock& lock, const NymCapability& capability)
        const -> bool;
    void init_claims(const eLock& lock) const;
    auto set_contact_data(
        const eLock& lock,
        const proto::ContactData& data,
        const PasswordPrompt& reason) -> bool;
    auto verify_pseudonym(const eLock& lock) const -> bool;

    auto add_contact_credential(
        const eLock& lock,
        const proto::ContactData& data,
        const PasswordPrompt& reason) -> bool;
    auto add_verification_credential(
        const eLock& lock,
        const proto::VerificationSet& data,
        const PasswordPrompt& reason) -> bool;
    void revoke_contact_credentials(const eLock& lock);
    void revoke_verification_credentials(const eLock& lock);
    auto update_nym(
        const eLock& lock,
        const std::int32_t version,
        const PasswordPrompt& reason) -> bool;

    Nym(const api::internal::Core& api,
        NymParameters& nymParameters,
        std::unique_ptr<const identity::Source> source,
        const PasswordPrompt& reason) noexcept(false);
    Nym(const api::internal::Core& api,
        const proto::Nym& serialized,
        const std::string& alias) noexcept(false);
    Nym() = delete;
    Nym(const Nym&) = delete;
    Nym(Nym&&) = delete;
    auto operator=(const Nym&) -> Nym& = delete;
    auto operator=(Nym&&) -> Nym& = delete;
};
}  // namespace opentxs::identity::implementation

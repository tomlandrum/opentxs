// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"      // IWYU pragma: associated
#include "1_Internal.hpp"    // IWYU pragma: associated
#include "identity/Nym.hpp"  // IWYU pragma: associated

#include <atomic>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "internal/identity/Identity.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Factory.hpp"
#if OT_CRYPTO_WITH_BIP32
#include "opentxs/api/HDSeed.hpp"
#endif  // OT_CRYPTO_WITH_BIP32
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/util/Tag.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/Bip44Type.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/identity/Authority.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/protobuf/Authority.pb.h"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/NymIDSource.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/verify/ContactData.hpp"
#include "opentxs/protobuf/verify/Nym.hpp"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"
#include "util/HDIndex.hpp"

#define OT_METHOD "opentxs::identity::implementation::Nym::"

namespace opentxs
{
auto Factory::Nym(
    const api::internal::Core& api,
    const NymParameters& params,
    const proto::ContactItemType type,
    const std::string name,
    const opentxs::PasswordPrompt& reason) -> identity::internal::Nym*
{
    using ReturnType = identity::implementation::Nym;

    if ((identity::CredentialType::Legacy == params.credentialType()) &&
        (proto::SOURCETYPE_BIP47 == params.SourceType())) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid parameters")
            .Flush();

        return nullptr;
    }

    try {
        auto revised = ReturnType::normalize(api, params, reason);
        auto pSource = std::unique_ptr<identity::Source>{
            NymIDSource(api, revised, reason)};

        if (false == bool(pSource)) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(
                ": Failed to generate nym id source")
                .Flush();

            return nullptr;
        }

        if (proto::CITEMTYPE_ERROR != type && !name.empty()) {
            const auto version =
                ReturnType::contact_credential_to_contact_data_version_.at(
                    identity::internal::Authority::NymToContactCredential(
                        identity::Nym::DefaultVersion));
            const auto blank = ContactData{
                api,
                pSource->NymID()->str(),
                version,
                version,
                ContactData::SectionMap{}};
            const auto scope = blank.SetScope(type, name);
            revised.SetContactData(scope.Serialize());
        }

        return new ReturnType(api, revised, std::move(pSource), reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to create nym: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Factory::Nym(
    const api::internal::Core& api,
    const proto::Nym& serialized,
    const std::string& alias) -> identity::internal::Nym*
{
    try {
        return new identity::implementation::Nym(api, serialized, alias);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to instantiate nym: ")(e.what())
            .Flush();

        return nullptr;
    }
}
}  // namespace opentxs

namespace opentxs::identity
{
const VersionNumber Nym::DefaultVersion{6};
const VersionNumber Nym::MaxVersion{6};
}  // namespace opentxs::identity

namespace opentxs::identity::implementation
{
auto session_key_from_iv(
    const api::internal::Core& api,
    const crypto::key::Asymmetric& signingKey,
    const Data& iv,
    const proto::HashType hashType,
    opentxs::PasswordPrompt& reason) -> bool;

const VersionConversionMap Nym::akey_to_session_key_version_{
    {1, 1},
    {2, 1},
};
const VersionConversionMap Nym::contact_credential_to_contact_data_version_{
    {1, 1},
    {2, 2},
    {3, 3},
    {4, 4},
    {5, 5},
    {6, 6},
};

Nym::Nym(
    const api::internal::Core& api,
    NymParameters& params,
    std::unique_ptr<const identity::Source> source,
    const opentxs::PasswordPrompt& reason) noexcept(false)
    : api_(api)
    , source_p_(std::move(source))
    , source_(*source_p_)
    , id_(source_.NymID())
    , mode_(proto::NYM_PRIVATE)
    , version_(DefaultVersion)
    , index_(1)
    , alias_()
    , revision_(0)
    , contact_data_(nullptr)
    , active_(create_authority(api_, *this, source_, version_, params, reason))
    , m_mapRevokedSets()
    , m_listRevokedIDs()
{
    if (false == bool(source_p_)) {
        throw std::runtime_error("Invalid nym id source");
    }
}

Nym::Nym(
    const api::internal::Core& api,
    const proto::Nym& serialized,
    const std::string& alias) noexcept(false)
    : api_(api)
    , source_p_(opentxs::Factory::NymIDSource(api, serialized.source()))
    , source_(*source_p_)
    , id_(source_.NymID())
    , mode_(serialized.mode())
    , version_(serialized.version())
    , index_(serialized.index())
    , alias_(alias)
    , revision_(serialized.revision())
    , contact_data_(nullptr)
    , active_(load_authorities(api_, *this, source_, serialized))
    , m_mapRevokedSets()
    , m_listRevokedIDs(
          load_revoked(api_, *this, source_, serialized, m_mapRevokedSets))
{
    if (false == bool(source_p_)) {
        throw std::runtime_error("Invalid nym id source");
    }
}

auto Nym::add_contact_credential(
    const eLock& lock,
    const proto::ContactData& data,
    const opentxs::PasswordPrompt& reason) -> bool
{
    OT_ASSERT(verify_lock(lock));

    bool added = false;

    for (auto& it : active_) {
        if (nullptr != it.second) {
            if (it.second->hasCapability(NymCapability::SIGN_CHILDCRED)) {
                added = it.second->AddContactCredential(data, reason);

                break;
            }
        }
    }

    return added;
}

auto Nym::add_verification_credential(
    const eLock& lock,
    const proto::VerificationSet& data,
    const opentxs::PasswordPrompt& reason) -> bool
{
    OT_ASSERT(verify_lock(lock));

    bool added = false;

    for (auto& it : active_) {
        if (nullptr != it.second) {
            if (it.second->hasCapability(NymCapability::SIGN_CHILDCRED)) {
                added = it.second->AddVerificationCredential(data, reason);

                break;
            }
        }
    }

    return added;
}

auto Nym::AddChildKeyCredential(
    const Identifier& masterID,
    const NymParameters& nymParameters,
    const opentxs::PasswordPrompt& reason) -> std::string
{
    eLock lock(shared_lock_);

    std::string output;
    auto it = active_.find(masterID);
    const bool noMaster = (it == active_.end());

    if (noMaster) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Master ID not found.").Flush();

        return output;
    }

    if (it->second) {
        output = it->second->AddChildKeyCredential(nymParameters, reason);
    }

    return output;
}

auto Nym::AddClaim(const Claim& claim, const opentxs::PasswordPrompt& reason)
    -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(contact_data_->AddItem(claim)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddContract(
    const identifier::UnitDefinition& instrumentDefinitionID,
    const proto::ContactItemType currency,
    const opentxs::PasswordPrompt& reason,
    const bool primary,
    const bool active) -> bool
{
    const std::string id(instrumentDefinitionID.str());

    if (id.empty()) { return false; }

    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(
        contact_data_->AddContract(id, currency, primary, active)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddEmail(
    const std::string& value,
    const opentxs::PasswordPrompt& reason,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(
        new ContactData(contact_data_->AddEmail(value, primary, active)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddPaymentCode(
    const opentxs::PaymentCode& code,
    const proto::ContactItemType currency,
    const opentxs::PasswordPrompt& reason,
    const bool primary,
    const bool active) -> bool
{
    const auto paymentCode = code.asBase58();

    if (paymentCode.empty()) { return false; }

    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(
        contact_data_->AddPaymentCode(paymentCode, currency, primary, active)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddPhoneNumber(
    const std::string& value,
    const opentxs::PasswordPrompt& reason,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(
        new ContactData(contact_data_->AddPhoneNumber(value, primary, active)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddPreferredOTServer(
    const Identifier& id,
    const opentxs::PasswordPrompt& reason,
    const bool primary) -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_)

    contact_data_.reset(
        new ContactData(contact_data_->AddPreferredOTServer(id, primary)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::AddSocialMediaProfile(
    const std::string& value,
    const proto::ContactItemType type,
    const opentxs::PasswordPrompt& reason,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(
        contact_data_->AddSocialMediaProfile(value, type, primary, active)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::Alias() const -> std::string { return alias_; }

auto Nym::asPublicNym() const -> const Nym::Serialized
{
    return SerializeCredentialIndex(Mode::Full);
}

auto Nym::at(const std::size_t& index) const noexcept(false)
    -> const Nym::value_type&
{
    for (auto i{active_.cbegin()}; i != active_.cend(); ++i) {
        if (static_cast<std::size_t>(std::distance(active_.cbegin(), i)) ==
            index) {
            return *i->second;
        }
    }

    throw std::out_of_range("Invalid authority index");
}

auto Nym::BestEmail() const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->BestEmail();
}

auto Nym::BestPhoneNumber() const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->BestPhoneNumber();
}

auto Nym::BestSocialMediaProfile(const proto::ContactItemType type) const
    -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->BestSocialMediaProfile(type);
}

auto Nym::Claims() const -> const opentxs::ContactData&
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return *contact_data_;
}

auto Nym::CompareID(const identity::Nym& rhs) const -> bool
{
    sLock lock(shared_lock_);

    return rhs.CompareID(id_);
}

auto Nym::CompareID(const identifier::Nym& rhs) const -> bool
{
    sLock lock(shared_lock_);

    return id_ == rhs;
}

auto Nym::ContactCredentialVersion() const -> VersionNumber
{
    // TODO support multiple authorities
    OT_ASSERT(0 < active_.size())

    return active_.cbegin()->second->ContactCredentialVersion();
}

auto Nym::Contracts(
    const proto::ContactItemType currency,
    const bool onlyActive) const -> std::set<OTIdentifier>
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->Contracts(currency, onlyActive);
}

auto Nym::create_authority(
    const api::internal::Core& api,
    const identity::Nym& parent,
    const identity::Source& source,
    const VersionNumber version,
    const NymParameters& params,
    const PasswordPrompt& reason) noexcept(false) -> CredentialMap
{
    auto output = CredentialMap{};
    auto pAuthority = std::unique_ptr<identity::internal::Authority>(
        opentxs::Factory::Authority(
            api, parent, source, params, version, reason));

    if (false == bool(pAuthority)) {
        throw std::runtime_error("Failed to create nym authority");
    }

    auto& authority = *pAuthority;
    auto id{authority.GetMasterCredID()};
    output.emplace(std::move(id), std::move(pAuthority));

    return output;
}

auto Nym::DeleteClaim(
    const Identifier& id,
    const opentxs::PasswordPrompt& reason) -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(contact_data_->Delete(id)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::EmailAddresses(bool active) const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->EmailAddresses(active);
}

auto Nym::EncryptionTargets() const noexcept -> NymKeys
{
    sLock lock(shared_lock_);
    auto output = NymKeys{id_, {}};

    for (const auto& [id, pAuthority] : active_) {
        const auto& authority = *pAuthority;

        if (authority.hasCapability(NymCapability::ENCRYPT_MESSAGE)) {
            output.second.emplace_back(authority.EncryptionTargets());
        }
    }

    return output;
}

void Nym::GetIdentifier(identifier::Nym& theIdentifier) const
{
    sLock lock(shared_lock_);

    theIdentifier.Assign(id_);
}

// sets argument based on internal member
void Nym::GetIdentifier(String& theIdentifier) const
{
    sLock lock(shared_lock_);

    id_->GetString(theIdentifier);
}

template <typename T>
auto Nym::get_private_auth_key(const T& lock, crypto::AsymmetricKeyType keytype)
    const -> const crypto::key::Asymmetric&
{
    OT_ASSERT(!active_.empty());

    OT_ASSERT(verify_lock(lock));
    const identity::Authority* pCredential{nullptr};

    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPrivateAuthKey(
        keytype, &m_listRevokedIDs);  // success
}

auto Nym::GetPrivateAuthKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    return get_private_auth_key(lock, keytype);
}

auto Nym::GetPrivateEncrKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    OT_ASSERT(!active_.empty());

    const identity::Authority* pCredential{nullptr};

    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPrivateEncrKey(
        keytype,
        &m_listRevokedIDs);  // success
}

auto Nym::GetPrivateSignKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    return get_private_sign_key(lock, keytype);
}

template <typename T>
auto Nym::get_private_sign_key(const T& lock, crypto::AsymmetricKeyType keytype)
    const -> const crypto::key::Asymmetric&
{
    OT_ASSERT(!active_.empty());

    OT_ASSERT(verify_lock(lock));

    const identity::Authority* pCredential{nullptr};

    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPrivateSignKey(
        keytype,
        &m_listRevokedIDs);  // success
}

template <typename T>
auto Nym::get_public_sign_key(const T& lock, crypto::AsymmetricKeyType keytype)
    const -> const crypto::key::Asymmetric&
{
    OT_ASSERT(!active_.empty());

    OT_ASSERT(verify_lock(lock));

    const identity::Authority* pCredential{nullptr};

    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPublicSignKey(
        keytype,
        &m_listRevokedIDs);  // success
}

auto Nym::GetPublicAuthKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    OT_ASSERT(!active_.empty());

    const identity::Authority* pCredential{nullptr};

    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPublicAuthKey(
        keytype,
        &m_listRevokedIDs);  // success
}

auto Nym::GetPublicEncrKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    OT_ASSERT(!active_.empty());

    const identity::Authority* pCredential{nullptr};
    for (const auto& it : active_) {
        // Todo: If we have some criteria, such as which master or
        // child credential
        // is currently being employed by the user, we'll use that here to
        // skip
        // through this loop until we find the right one. Until then, I'm
        // just
        // going to return the first one that's valid (not null).

        pCredential = it.second.get();
        if (nullptr != pCredential) break;
    }
    if (nullptr == pCredential) OT_FAIL;

    return pCredential->GetPublicEncrKey(
        keytype,
        &m_listRevokedIDs);  // success
}

// This is being called by:
// Contract::VerifySignature(const Nym& theNym, const Signature&
// theSignature, PasswordPrompt * pPWData=nullptr)
//
// Note: Need to change Contract::VerifySignature so that it checks all of
// these keys when verifying.
//
// OT uses the signature's metadata to narrow down its search for the correct
// public key.
// Return value is the count of public keys found that matched the metadata on
// the signature.
auto Nym::GetPublicKeysBySignature(
    crypto::key::Keypair::Keys& listOutput,
    const Signature& theSignature,
    char cKeyType) const -> std::int32_t
{
    // Unfortunately, theSignature can only narrow the search down (there may be
    // multiple results.)
    std::int32_t nCount = 0;

    sLock lock(shared_lock_);

    for (const auto& it : active_) {
        const identity::Authority* pCredential = it.second.get();
        OT_ASSERT(nullptr != pCredential);

        const std::int32_t nTempCount = pCredential->GetPublicKeysBySignature(
            listOutput, theSignature, cKeyType);
        nCount += nTempCount;
    }

    return nCount;
}

auto Nym::GetPublicSignKey(crypto::AsymmetricKeyType keytype) const
    -> const crypto::key::Asymmetric&
{
    sLock lock(shared_lock_);

    return get_public_sign_key(lock, keytype);
}

auto Nym::has_capability(const eLock& lock, const NymCapability& capability)
    const -> bool
{
    OT_ASSERT(verify_lock(lock));

    for (auto& it : active_) {
        OT_ASSERT(nullptr != it.second);

        if (nullptr != it.second) {
            const identity::Authority& credSet = *it.second;

            if (credSet.hasCapability(capability)) { return true; }
        }
    }

    return false;
}

auto Nym::HasCapability(const NymCapability& capability) const -> bool
{
    eLock lock(shared_lock_);

    return has_capability(lock, capability);
}

void Nym::init_claims(const eLock& lock) const
{
    OT_ASSERT(verify_lock(lock));

    const auto nymID{id_->str()};
    const auto dataVersion = ContactDataVersion();
    contact_data_.reset(new opentxs::ContactData(
        api_, nymID, dataVersion, dataVersion, ContactData::SectionMap()));

    OT_ASSERT(contact_data_);

    std::unique_ptr<proto::ContactData> serialized{nullptr};

    for (auto& it : active_) {
        OT_ASSERT(nullptr != it.second);

        const auto& credSet = *it.second;
        credSet.GetContactData(serialized);

        if (serialized) {
            OT_ASSERT(proto::Validate(
                *serialized, VERBOSE, proto::ClaimType::Normal));

            opentxs::ContactData claimCred(
                api_, nymID, dataVersion, *serialized);
            contact_data_.reset(
                new opentxs::ContactData(*contact_data_ + claimCred));
            serialized.reset();
        }
    }

    OT_ASSERT(contact_data_)
}

auto Nym::load_authorities(
    const api::internal::Core& api,
    const identity::Nym& parent,
    const identity::Source& source,
    const Serialized& serialized) noexcept(false) -> CredentialMap
{
    auto output = CredentialMap{};

    if (false == proto::Validate<proto::Nym>(serialized, VERBOSE)) {
        throw std::runtime_error("Invalid serialized nym");
    }

    const auto mode = (proto::NYM_PRIVATE == serialized.mode())
                          ? proto::KEYMODE_PRIVATE
                          : proto::KEYMODE_PUBLIC;

    for (auto& it : serialized.activecredentials()) {
        auto pCandidate = std::unique_ptr<identity::internal::Authority>{
            opentxs::Factory::Authority(api, parent, source, mode, it)};

        if (false == bool(pCandidate)) {
            throw std::runtime_error("Failed to instantiate authority");
        }

        const auto& candidate = *pCandidate;
        auto id{candidate.GetMasterCredID()};
        output.emplace(std::move(id), std::move(pCandidate));
    }

    return output;
}

auto Nym::load_revoked(
    const api::internal::Core& api,
    const identity::Nym& parent,
    const identity::Source& source,
    const Serialized& serialized,
    CredentialMap& revoked) noexcept(false) -> String::List
{
    auto output = String::List{};

    if (!opentxs::operator==(Serialized::default_instance(), serialized)) {
        const auto mode = (proto::NYM_PRIVATE == serialized.mode())
                              ? proto::KEYMODE_PRIVATE
                              : proto::KEYMODE_PUBLIC;

        for (auto& it : serialized.revokedcredentials()) {
            auto pCandidate = std::unique_ptr<identity::internal::Authority>{
                opentxs::Factory::Authority(api, parent, source, mode, it)};

            if (false == bool(pCandidate)) {
                throw std::runtime_error("Failed to instantiate authority");
            }

            const auto& candidate = *pCandidate;
            auto id{candidate.GetMasterCredID()};
            output.push_back(id->str());
            revoked.emplace(std::move(id), std::move(pCandidate));
        }
    }

    return output;
}

auto Nym::Name() const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    std::string output = contact_data_->Name();

    if (false == output.empty()) { return output; }

    return alias_;
}

auto Nym::normalize(
    const api::internal::Core& api,
    const NymParameters& in,
    const PasswordPrompt& reason) noexcept(false) -> NymParameters
{
    auto output{in};

    if (identity::CredentialType::HD == in.credentialType()) {
#if OT_CRYPTO_WITH_BIP32
        const auto& seeds = api.Seeds();
        output.SetCredset(0);
        auto nymIndex = Bip32Index{0};
        auto fingerprint = in.Seed();
        auto style = in.SeedStyle();
        auto lang = in.SeedLanguage();

        if (fingerprint.empty()) {
            seeds.GetOrCreateDefaultSeed(
                fingerprint, style, lang, nymIndex, in.SeedStrength(), reason);
        }

        auto seed = seeds.Seed(fingerprint, nymIndex, reason);
        const auto defaultIndex = in.UseAutoIndex();

        if (false == defaultIndex) {
            LogDetail(OT_METHOD)(__FUNCTION__)(
                ": Re-creating nym at specified path.")
                .Flush();

            nymIndex = in.Nym();
        }

        static constexpr auto maxIndex =
            std::numeric_limits<std::int32_t>::max() - 1;

        if (nymIndex >= static_cast<Bip32Index>(maxIndex)) {
            throw std::runtime_error(
                "Requested seed has already generated maximum number of nyms");
        }

        const auto newIndex = static_cast<std::int32_t>(nymIndex) + 1;
        seeds.UpdateIndex(fingerprint, newIndex, reason);
        output.SetEntropy(seed);
        output.SetSeed(fingerprint);
        output.SetNym(nymIndex);
#else
        throw std::runtime_error(
            "opentxs compiled without hd credential support");
#endif
    }

    return output;
}

auto Nym::Path(proto::HDPath& output) const -> bool
{
    sLock lock(shared_lock_);

    for (const auto& it : active_) {
        OT_ASSERT(nullptr != it.second);
        const auto& set = *it.second;

        if (set.Path(output)) {
            output.mutable_child()->RemoveLast();
            return true;
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": No credential set contains a path.")
        .Flush();

    return false;
}

auto Nym::PaymentCode() const -> std::string
{
    if (proto::SOURCETYPE_BIP47 != source_.Type()) { return ""; }

    auto serialized = source_.Serialize();

    if (!serialized) { return ""; }

    auto paymentCode = api_.Factory().PaymentCode(serialized->paymentcode());

    return paymentCode->asBase58();
}

auto Nym::PaymentCodePath(proto::HDPath& output) const -> bool
{
    auto base = proto::HDPath{};

    if (false == Path(base)) { return false; }

    if (2 != base.child().size()) { return false; }

    static const auto expected =
        HDIndex{Bip43Purpose::NYM, Bip32Child::HARDENED};

    if (expected != base.child(0)) { return false; }

    output.set_version(base.version());
    output.set_root(base.root());
    output.add_child(HDIndex{Bip43Purpose::PAYCODE, Bip32Child::HARDENED});
    output.add_child(HDIndex{Bip44Type::BITCOIN, Bip32Child::HARDENED});
    output.add_child(base.child(1));

    return true;
}

auto Nym::PhoneNumbers(bool active) const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->PhoneNumbers(active);
}

auto Nym::Revision() const -> std::uint64_t { return revision_.load(); }

void Nym::revoke_contact_credentials(const eLock& lock)
{
    OT_ASSERT(verify_lock(lock));

    std::list<std::string> revokedIDs;

    for (auto& it : active_) {
        if (nullptr != it.second) {
            it.second->RevokeContactCredentials(revokedIDs);
        }
    }

    for (auto& it : revokedIDs) { m_listRevokedIDs.push_back(it); }
}

void Nym::revoke_verification_credentials(const eLock& lock)
{
    OT_ASSERT(verify_lock(lock));

    std::list<std::string> revokedIDs;

    for (auto& it : active_) {
        if (nullptr != it.second) {
            it.second->RevokeVerificationCredentials(revokedIDs);
        }
    }

    for (auto& it : revokedIDs) { m_listRevokedIDs.push_back(it); }
}

auto Nym::SerializeCredentialIndex(const Mode mode) const -> Nym::Serialized
{
    sLock lock(shared_lock_);
    Serialized index;
    index.set_version(version_);
    auto nymID = String::Factory(id_);
    index.set_nymid(nymID->Get());

    if (Mode::Abbreviated == mode) {
        index.set_mode(mode_);

        if (proto::NYM_PRIVATE == mode_) { index.set_index(index_); }
    } else {
        index.set_mode(proto::NYM_PUBLIC);
    }

    index.set_revision(revision_.load());
    *(index.mutable_source()) = *(source_.Serialize());

    for (auto& it : active_) {
        if (nullptr != it.second) {
            auto credset = it.second->Serialize(static_cast<bool>(mode));
            auto pCredSet = index.add_activecredentials();
            *pCredSet = *credset;
            pCredSet = nullptr;
        }
    }

    for (auto& it : m_mapRevokedSets) {
        if (nullptr != it.second) {
            auto credset = it.second->Serialize(static_cast<bool>(mode));
            auto pCredSet = index.add_revokedcredentials();
            *pCredSet = *credset;
            pCredSet = nullptr;
        }
    }

    return index;
}

void Nym::SerializeNymIDSource(Tag& parent) const
{
    // We encode these before storing.
    TagPtr pTag(new Tag("nymIDSource", source_.asString()->Get()));
    const auto description = source_.Description();

    if (description->Exists()) {
        auto ascDescription = Armored::Factory();
        ascDescription->SetString(
            description,
            false);  // bLineBreaks=true by default.

        pTag->add_attribute("Description", ascDescription->Get());
    }

    parent.add_tag(pTag);
}

auto Nym::set_contact_data(
    const eLock& lock,
    const proto::ContactData& data,
    const opentxs::PasswordPrompt& reason) -> bool
{
    OT_ASSERT(verify_lock(lock));

    auto version = proto::NymRequiredVersion(data.version(), version_);

    if ((0 == version) || version > MaxVersion) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Contact data version not supported by this nym.")
            .Flush();

        return false;
    }

    if (false == has_capability(lock, NymCapability::SIGN_CHILDCRED)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": This nym can not be modified.")
            .Flush();

        return false;
    }

    if (false == proto::Validate(data, VERBOSE, proto::ClaimType::Normal)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid contact data.").Flush();

        return false;
    }

    revoke_contact_credentials(lock);

    if (add_contact_credential(lock, data, reason)) {

        return update_nym(lock, version, reason);
    }

    return false;
}

void Nym::SetAlias(const std::string& alias)
{
    eLock lock(shared_lock_);

    alias_ = alias;
    revision_++;
}

auto Nym::SetCommonName(
    const std::string& name,
    const opentxs::PasswordPrompt& reason) -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    contact_data_.reset(new ContactData(contact_data_->SetCommonName(name)));

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::SetContactData(
    const proto::ContactData& data,
    const opentxs::PasswordPrompt& reason) -> bool
{
    eLock lock(shared_lock_);
    contact_data_.reset(
        new ContactData(api_, id_->str(), ContactDataVersion(), data));

    return set_contact_data(lock, data, reason);
}

auto Nym::SetScope(
    const proto::ContactItemType type,
    const std::string& name,
    const opentxs::PasswordPrompt& reason,
    const bool primary) -> bool
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    if (proto::CITEMTYPE_UNKNOWN != contact_data_->Type()) {
        contact_data_.reset(
            new ContactData(contact_data_->SetName(name, primary)));
    } else {
        contact_data_.reset(
            new ContactData(contact_data_->SetScope(type, name)));
    }

    OT_ASSERT(contact_data_);

    return set_contact_data(lock, contact_data_->Serialize(), reason);
}

auto Nym::Sign(
    const ProtobufType& input,
    const crypto::SignatureRole role,
    proto::Signature& signature,
    const opentxs::PasswordPrompt& reason,
    const proto::HashType hash) const -> bool
{
    sLock lock(shared_lock_);

    bool haveSig = false;

    auto preimage = [&input]() -> std::string {
        return proto::ToString(input);
    };

    for (auto& it : active_) {
        if (nullptr != it.second) {
            bool success = it.second->Sign(
                preimage, role, signature, reason, proto::KEYROLE_SIGN, hash);

            if (success) {
                haveSig = true;
                break;
            } else {
                LogOutput(": Credential set ")(it.second->GetMasterCredID())(
                    " could not sign protobuf.")
                    .Flush();
            }
        }

        LogOutput(": Did not find any credential sets capable of signing on "
                  "this nym.")
            .Flush();
    }

    return haveSig;
}

auto Nym::SocialMediaProfiles(const proto::ContactItemType type, bool active)
    const -> std::string
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->SocialMediaProfiles(type, active);
}

auto Nym::SocialMediaProfileTypes() const
    -> const std::set<proto::ContactItemType>
{
    eLock lock(shared_lock_);

    if (false == bool(contact_data_)) { init_claims(lock); }

    OT_ASSERT(contact_data_);

    return contact_data_->SocialMediaProfileTypes();
}

auto Nym::TransportKey(Data& pubkey, const opentxs::PasswordPrompt& reason)
    const -> OTSecret
{
    bool found{false};
    auto privateKey = api_.Factory().Secret(0);
    sLock lock(shared_lock_);

    for (auto& it : active_) {
        OT_ASSERT(nullptr != it.second);

        if (nullptr != it.second) {
            const identity::Authority& credSet = *it.second;
            found = credSet.TransportKey(pubkey, privateKey, reason);

            if (found) { break; }
        }
    }

    OT_ASSERT(found);

    return privateKey;
}

auto Nym::Unlock(
    const crypto::key::Asymmetric& dhKey,
    const std::uint32_t tag,
    const crypto::AsymmetricKeyType type,
    const crypto::key::Symmetric& key,
    PasswordPrompt& reason) const noexcept -> bool
{
    for (const auto& [id, authority] : active_) {
        if (authority->Unlock(dhKey, tag, type, key, reason)) { return true; }
    }

    for (const auto& [id, authority] : m_mapRevokedSets) {
        if (authority->Unlock(dhKey, tag, type, key, reason)) { return true; }
    }

    return false;
}

auto Nym::update_nym(
    const eLock& lock,
    const std::int32_t version,
    const opentxs::PasswordPrompt& reason) -> bool
{
    OT_ASSERT(verify_lock(lock));

    if (verify_pseudonym(lock)) {
        // Upgrade version
        if (version > version_) { version_ = version; }

        ++revision_;

        return true;
    }

    return false;
}

auto Nym::Verify(const ProtobufType& input, proto::Signature& signature) const
    -> bool
{
    const auto copy{signature};
    signature.clear_signature();
    const auto plaintext = api_.Factory().Data(input);

    for (auto& it : active_) {
        if (nullptr != it.second) {
            if (it.second->Verify(plaintext, copy)) { return true; }
        }
    }

    return false;
}

auto Nym::verify_pseudonym(const eLock& lock) const -> bool
{
    // If there are credentials, then we verify the Nym via his credentials.
    if (!active_.empty()) {
        // Verify Nym by his own credentials.
        for (const auto& it : active_) {
            const identity::Authority* pCredential = it.second.get();
            OT_ASSERT(nullptr != pCredential);

            // Verify all Credentials in the Authority, including source
            // verification for the master credential.
            if (!pCredential->VerifyInternally()) {
                LogNormal(OT_METHOD)(__FUNCTION__)(": Credential (")(
                    pCredential->GetMasterCredID())(
                    ") failed its own internal verification.")
                    .Flush();
                return false;
            }
        }
        return true;
    }
    LogOutput(OT_METHOD)(__FUNCTION__)(": No credentials.").Flush();
    return false;
}

auto Nym::VerifyPseudonym() const -> bool
{
    eLock lock(shared_lock_);

    return verify_pseudonym(lock);
}

auto Nym::WriteCredentials() const -> bool
{
    sLock lock(shared_lock_);

    for (auto& it : active_) {
        if (!it.second->WriteCredentials()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to save credentials.")
                .Flush();

            return false;
        }
    }

    return true;
}
}  // namespace opentxs::identity::implementation

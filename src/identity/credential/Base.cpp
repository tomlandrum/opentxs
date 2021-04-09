// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "identity/credential/Base.tpp"  // IWYU pragma: associated

#include <list>
#include <memory>
#include <stdexcept>
#include <string>

#include "core/contract/Signable.hpp"
#include "identity/credential/Base.hpp"
#include "internal/api/Api.hpp"
#include "internal/crypto/key/Key.hpp"
#include "internal/identity/Identity.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/identity/credential/Primary.hpp"
#include "opentxs/protobuf/ChildCredentialParameters.pb.h"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/Signature.pb.h"

#define OT_METHOD "opentxs::identity::credential::implementation::Base::"

namespace opentxs::identity::credential::implementation
{
Base::Base(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const NymParameters& nymParameters,
    const VersionNumber version,
    const identity::CredentialRole role,
    const crypto::key::asymmetric::Mode mode,
    const std::string& masterID) noexcept
    : Signable(api, {}, version, {}, {})
    , parent_(parent)
    , source_(source)
    , nym_id_(source.NymID()->str())
    , master_id_(masterID)
    , type_(nymParameters.credentialType())
    , role_(role)
    , mode_(mode)
{
}

Base::Base(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const proto::Credential& serialized,
    const std::string& masterID) noexcept(false)
    : Signable(
          api,
          {},
          serialized.version(),
          {},
          {},
          api.Factory().Identifier(serialized.id()),
          extract_signatures(serialized))
    , parent_(parent)
    , source_(source)
    , nym_id_(source.NymID()->str())
    , master_id_(masterID)
    , type_(
          opentxs::identity::credential::internal::translate(serialized.type()))
    , role_(
          opentxs::identity::credential::internal::translate(serialized.role()))
    , mode_(opentxs::crypto::key::internal::translate(serialized.mode()))
{
    if (serialized.nymid() != nym_id_) {
        throw std::runtime_error(
            "Attempting to load credential for incorrect nym");
    }
}

void Base::add_master_signature(
    const Lock& lock,
    const identity::credential::internal::Primary& master,
    const PasswordPrompt& reason) noexcept(false)
{
    auto serializedMasterSignature = std::make_shared<proto::Signature>();
    auto serialized = serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);
    auto& signature = *serialized->add_signature();

    bool havePublicSig = master.Sign(
        [&serialized]() -> std::string { return proto::ToString(*serialized); },
        crypto::SignatureRole::PublicCredential,
        signature,
        reason);

    if (false == havePublicSig) {
        throw std::runtime_error("Attempting to obtain master signature");
    }

    serializedMasterSignature->CopyFrom(signature);
    signatures_.push_back(serializedMasterSignature);
}

auto Base::asString(const bool asPrivate) const -> std::string
{
    std::shared_ptr<SerializedType> credenial;
    auto dataCredential = Data::Factory();
    auto stringCredential = String::Factory();
    credenial = Serialized(asPrivate, WITH_SIGNATURES);
    dataCredential = api_.Factory().Data(*credenial);
    auto armoredCredential = api_.Factory().Armored(dataCredential);
    armoredCredential->WriteArmoredString(stringCredential, "Credential");

    return stringCredential->Get();
}

auto Base::extract_signatures(const SerializedType& serialized) -> Signatures
{
    auto output = Signatures{};

    for (auto& it : serialized.signature()) {
        output.push_back(std::make_shared<proto::Signature>(it));
    }

    return output;
}

auto Base::get_master_id(const internal::Primary& master) noexcept
    -> std::string
{
    return master.ID()->str();
}

auto Base::get_master_id(
    const proto::Credential& serialized,
    const internal::Primary& master) noexcept(false) -> std::string
{
    const auto& id = serialized.childdata().masterid();

    if (id != master.ID()->str()) {
        throw std::runtime_error(
            "Attempting to load credential for incorrect authority");
    }

    return id;
}

auto Base::GetID(const Lock& lock) const -> OTIdentifier
{
    OT_ASSERT(verify_write_lock(lock));

    auto idVersion = serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);

    OT_ASSERT(idVersion);

    if (idVersion->has_id()) { idVersion->clear_id(); }

    return api_.Factory().Identifier(*idVersion);
}

void Base::init(
    const identity::credential::internal::Primary& master,
    const PasswordPrompt& reason) noexcept(false)
{
    sign(master, reason);

    if (false == Save()) {
        throw std::runtime_error("Failed to save master credential");
    }
}

/** Perform syntax (non-cryptographic) verifications of a credential */
auto Base::isValid(const Lock& lock) const -> bool
{
    std::shared_ptr<SerializedType> serializedProto;

    return isValid(lock, serializedProto);
}

/** Returns the serialized form to prevent unnecessary serializations */
auto Base::isValid(
    const Lock& lock,
    std::shared_ptr<SerializedType>& credential) const -> bool
{
    SerializationModeFlag serializationMode = AS_PUBLIC;

    if (crypto::key::asymmetric::Mode::Private == mode_) {
        serializationMode = AS_PRIVATE;
    }

    credential = serialize(lock, serializationMode, WITH_SIGNATURES);

    return proto::Validate<proto::Credential>(
        *credential,
        VERBOSE,
        opentxs::crypto::key::internal::translate(mode_),
        opentxs::identity::credential::internal::translate(role_),
        true);  // with signatures
}

auto Base::MasterSignature() const -> Base::Signature
{
    auto masterSignature = Signature{};
    const auto targetRole{proto::SIGROLE_PUBCREDENTIAL};

    for (auto& it : signatures_) {
        if ((it->role() == targetRole) && (it->credentialid() == master_id_)) {

            masterSignature = it;
            break;
        }
    }

    return masterSignature;
}

void Base::ReleaseSignatures(const bool onlyPrivate)
{
    for (auto i = signatures_.begin(); i != signatures_.end();) {
        if (!onlyPrivate ||
            (onlyPrivate && (proto::SIGROLE_PRIVCREDENTIAL == (*i)->role()))) {
            i = signatures_.erase(i);
        } else {
            i++;
        }
    }
}

auto Base::Save() const -> bool
{
    Lock lock(lock_);

    std::shared_ptr<SerializedType> serializedProto;

    if (!isValid(lock, serializedProto)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to save serialized credential. Type (")(value(role_))(
            "), version ")(version_)
            .Flush();

        return false;
    }

    const bool bSaved = api_.Wallet().SaveCredential(*serializedProto);

    if (!bSaved) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error saving credential.")
            .Flush();

        return false;
    }

    return true;
}

auto Base::SelfSignature(CredentialModeFlag version) const -> Base::Signature
{
    const auto targetRole{
        (PRIVATE_VERSION == version) ? proto::SIGROLE_PRIVCREDENTIAL
                                     : proto::SIGROLE_PUBCREDENTIAL};
    const auto self = id_->str();

    for (auto& it : signatures_) {
        if ((it->role() == targetRole) && (it->credentialid() == self)) {

            return it;
        }
    }

    return nullptr;
}

auto Base::serialize(
    const Lock& lock,
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<Base::SerializedType>
{
    auto serializedCredential = std::make_shared<proto::Credential>();
    serializedCredential->set_version(version_);
    serializedCredential->set_type(
        opentxs::identity::credential::internal::translate((type_)));
    serializedCredential->set_role(
        opentxs::identity::credential::internal::translate(role_));

    if (identity::CredentialRole::MasterKey != role_) {
        std::unique_ptr<proto::ChildCredentialParameters> parameters;
        parameters.reset(new proto::ChildCredentialParameters);

        parameters->set_version(1);
        parameters->set_masterid(master_id_);
        serializedCredential->set_allocated_childdata(parameters.release());
    }

    if (asPrivate) {
        if (crypto::key::asymmetric::Mode::Private == mode_) {
            serializedCredential->set_mode(
                opentxs::crypto::key::internal::translate(mode_));
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Can't serialize a public credential as a private "
                "credential.")
                .Flush();
        }
    } else {
        serializedCredential->set_mode(
            opentxs::crypto::key::internal::translate(
                crypto::key::asymmetric::Mode::Public));
    }

    if (asSigned) {
        if (asPrivate) {
            auto privateSig = SelfSignature(PRIVATE_VERSION);

            if (privateSig) {
                *serializedCredential->add_signature() = *privateSig;
            }
        }

        auto publicSig = SelfSignature(PUBLIC_VERSION);

        if (publicSig) { *serializedCredential->add_signature() = *publicSig; }

        auto sourceSig = SourceSignature();

        if (sourceSig) { *serializedCredential->add_signature() = *sourceSig; }
    } else {
        serializedCredential->clear_signature();  // just in case...
    }

    serializedCredential->set_id(id(lock)->str());
    serializedCredential->set_nymid(nym_id_);

    return serializedCredential;
}

auto Base::Serialize() const -> OTData
{
    auto serialized =
        Serialized(Private() ? AS_PRIVATE : AS_PUBLIC, WITH_SIGNATURES);

    return api_.Factory().Data(*serialized);
}

auto Base::Serialized(
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<Base::SerializedType>
{
    Lock lock(lock_);

    return serialize(lock, asPrivate, asSigned);
}

void Base::sign(
    const identity::credential::internal::Primary& master,
    const PasswordPrompt& reason) noexcept(false)
{
    Lock lock(lock_);

    if (identity::CredentialRole::MasterKey != role_) {
        add_master_signature(lock, master, reason);
    }
}

auto Base::SourceSignature() const -> Base::Signature
{
    auto signature = Signature{};

    for (auto& it : signatures_) {
        if ((it->role() == proto::SIGROLE_NYMIDSOURCE) &&
            (it->credentialid() == nym_id_)) {
            signature = std::make_shared<proto::Signature>(*it);

            break;
        }
    }

    return signature;
}

/** Override this method for credentials capable of deriving transport keys */
auto Base::TransportKey(Data&, Secret&, const PasswordPrompt&) const -> bool
{
    OT_ASSERT_MSG(false, "This method was called on the wrong credential.");

    return false;
}

auto Base::validate(const Lock& lock) const -> bool
{
    // Check syntax
    if (!isValid(lock)) { return false; }

    // Check cryptographic requirements
    return verify_internally(lock);
}

auto Base::Validate() const -> bool
{
    Lock lock(lock_);

    return validate(lock);
}

/** Verifies the cryptographic integrity of a credential. Assumes the
 * Authority specified by parent_ is valid. */
auto Base::verify_internally(const Lock& lock) const -> bool
{
    if (!CheckID(lock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Purported ID for this credential does not match its actual "
            "contents.")
            .Flush();

        return false;
    }

    bool GoodMasterSignature = false;

    if (identity::CredentialRole::MasterKey == role_) {
        GoodMasterSignature = true;  // Covered by VerifySignedBySelf()
    } else {
        GoodMasterSignature = verify_master_signature(lock);
    }

    if (!GoodMasterSignature) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": This credential hasn't been signed by its master credential.")
            .Flush();

        return false;
    }

    return true;
}

auto Base::verify_master_signature(const Lock& lock) const -> bool
{
    auto serialized = serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);
    auto masterSig = MasterSignature();

    if (!masterSig) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing master signature.")
            .Flush();

        return false;
    }

    return (parent_.GetMasterCredential().Verify(
        *serialized, role_, parent_.GetMasterCredID(), *masterSig));
}
}  // namespace opentxs::identity::credential::implementation

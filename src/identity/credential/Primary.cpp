// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                     // IWYU pragma: associated
#include "1_Internal.hpp"                   // IWYU pragma: associated
#include "identity/credential/Primary.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>

#include "2_Factory.hpp"
#include "core/contract/Signable.hpp"
#include "identity/credential/Key.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "opentxs/identity/KeyMode.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/identity/SourceProofType.hpp"
#include "opentxs/identity/SourceType.hpp"
#include "opentxs/identity/credential/Base.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "opentxs/protobuf/MasterCredentialParameters.pb.h"
#include "opentxs/protobuf/NymIDSource.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/SourceProof.pb.h"
#include "opentxs/protobuf/verify/Credential.hpp"
#include "util/Container.hpp"

#define OT_METHOD "opentxs::identity::credential::implementation::Primary::"

namespace opentxs
{
using ReturnType = identity::credential::implementation::Primary;

auto Factory::PrimaryCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const NymParameters& parameters,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason)
    -> identity::credential::internal::Primary*
{
    try {

        return new ReturnType(api, parent, source, parameters, version, reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to create credential: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Factory::PrimaryCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const proto::Credential& serialized)
    -> identity::credential::internal::Primary*
{
    try {

        return new ReturnType(api, parent, source, serialized);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to deserialize credential: ")(e.what())
            .Flush();

        return nullptr;
    }
}
}  // namespace opentxs

namespace opentxs::identity::credential::implementation
{
const Primary::SourceProofTypeMap Primary::sourceprooftype_map_ = {
    {identity::SourceProofType::SelfSignature,
     proto::SOURCEPROOFTYPE_SELF_SIGNATURE},
    {identity::SourceProofType::Signature, proto::SOURCEPROOFTYPE_SIGNATURE},
};
const Primary::SourceProofTypeReverseMap Primary::sourceprooftype_reverse_map_{
    reverse_map(Primary::sourceprooftype_map_)};
const VersionConversionMap Primary::credential_to_master_params_{
    {1, 1},
    {2, 1},
    {3, 1},
    {4, 1},
    {5, 1},
    {6, 2},
};

Primary::Primary(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const NymParameters& params,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept(false)
    : credential::implementation::Key(
          api,
          parent,
          source,
          params,
          version,
          identity::CredentialRole::MasterKey,
          reason,
          "",
          identity::SourceType::PubKey == params.SourceType())
    , source_proof_(source_proof(params))
{
    {
        Lock lock(lock_);
        first_time_init(lock);
    }

    init(*this, reason);
}

Primary::Primary(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const proto::Credential& serialized) noexcept(false)
    : credential::implementation::Key(api, parent, source, serialized, "")
    , source_proof_(serialized.masterdata().sourceproof())
{
    Lock lock(lock_);
    init_serialized(lock);
}

auto Primary::hasCapability(const NymCapability& capability) const -> bool
{
    switch (capability) {
        case (NymCapability::SIGN_CHILDCRED): {
            return signing_key_->CheckCapability(capability);
        }
        default: {
        }
    }

    return false;
}

auto Primary::Path(proto::HDPath& output) const -> bool
{
    try {
        const bool found = signing_key_->GetPrivateKey().Path(output);

        if (found) { output.mutable_child()->RemoveLast(); }

        return found;
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No private key.").Flush();

        return false;
    }
}

auto Primary::Path() const -> std::string
{
    return signing_key_->GetPrivateKey().Path();
}

auto Primary::serialize(
    const Lock& lock,
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<identity::credential::Base::SerializedType>
{
    auto output = Key::serialize(lock, asPrivate, asSigned);

    OT_ASSERT(output);

    auto& serialized = *output;
    try {
        serialized.set_role(
            credentialrole_map_.at(identity::CredentialRole::MasterKey));
        auto& masterData = *serialized.mutable_masterdata();
        masterData.set_version(credential_to_master_params_.at(version_));
        *masterData.mutable_source() = *(source_.Serialize());
        *masterData.mutable_sourceproof() = source_proof_;
    } catch (...) {
    }

    return output;
}

void Primary::sign(
    const identity::credential::internal::Primary& master,
    const PasswordPrompt& reason) noexcept(false)
{
    Key::sign(master, reason);

    if (proto::SOURCEPROOFTYPE_SELF_SIGNATURE != source_proof_.type()) {
        auto sig = std::make_shared<proto::Signature>();

        OT_ASSERT(sig);

        if (false == source_.Sign(*this, *sig, reason)) {
            throw std::runtime_error("Failed to obtain source signature");
        }

        signatures_.push_back(sig);
    }
}

auto Primary::source_proof(const NymParameters& params) -> proto::SourceProof
{
    auto output = proto::SourceProof{};
    try {
        output.set_version(1);
        output.set_type(sourceprooftype_map_.at(params.SourceProofType()));
    } catch (...) {
    }

    return output;
}

auto Primary::Verify(
    const proto::Credential& credential,
    const identity::CredentialRole& role,
    const Identifier& masterID,
    const proto::Signature& masterSig) const -> bool
{
    auto isValid{false};
    try {
        isValid = proto::Validate<proto::Credential>(
            credential,
            VERBOSE,
            Base::keymode_map_.at(identity::KeyMode::Public),
            credentialrole_map_.at(role),
            false);
    } catch (...) {
    }

    if (!isValid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid credential syntax.")
            .Flush();
        return false;
    }

    bool sameMaster = (id_ == masterID);

    if (!sameMaster) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Credential does not designate this credential as its master.")
            .Flush();

        return false;
    }

    proto::Credential copy;
    copy.CopyFrom(credential);
    auto& signature = *copy.add_signature();
    signature.CopyFrom(masterSig);
    signature.clear_signature();

    return Verify(api_.Factory().Data(copy), masterSig);
}

auto Primary::verify_against_source(const Lock& lock) const -> bool
{
    auto pSerialized = std::shared_ptr<proto::Credential>{};
    auto hasSourceSignature{true};

    switch (source_.Type()) {
        case identity::SourceType::PubKey: {
            pSerialized = serialize(lock, AS_PUBLIC, WITH_SIGNATURES);
            hasSourceSignature = false;
        } break;
        case identity::SourceType::Bip47: {
            pSerialized = serialize(lock, AS_PUBLIC, WITHOUT_SIGNATURES);
        } break;
        default: {
            return false;
        }
    }

    if (false == bool(pSerialized)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize credentials")
            .Flush();

        return false;
    }

    const auto& serialized = *pSerialized;
    const auto pSig = hasSourceSignature ? SourceSignature() : SelfSignature();

    if (false == bool(pSig)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Master credential not signed by its source.")
            .Flush();

        return false;
    }

    const auto& sig = *pSig;

    return source_.Verify(serialized, sig);
}

auto Primary::verify_internally(const Lock& lock) const -> bool
{
    // Perform common Key Credential verifications
    if (!Key::verify_internally(lock)) { return false; }

    // Check that the source validates this credential
    if (!verify_against_source(lock)) {
        LogNormal(OT_METHOD)(__FUNCTION__)(
            ": Failed verifying master credential against "
            "nym id source.")
            .Flush();

        return false;
    }

    return true;
}
}  // namespace opentxs::identity::credential::implementation

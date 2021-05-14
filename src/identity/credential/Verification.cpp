// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                          // IWYU pragma: associated
#include "1_Internal.hpp"                        // IWYU pragma: associated
#include "identity/credential/Verification.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <string>

#include "2_Factory.hpp"
#include "identity/credential/Base.hpp"
#include "internal/api/Api.hpp"
#include "internal/crypto/key/Key.hpp"
#include "internal/identity/Identity.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/key/asymmetric/Mode.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "opentxs/identity/credential/Verification.hpp"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/Verification.pb.h"
#include "opentxs/protobuf/VerificationGroup.pb.h"
#include "opentxs/protobuf/VerificationIdentity.pb.h"
#include "opentxs/protobuf/VerificationSet.pb.h"

#define OT_METHOD "opentxs::identity::credential::Verification::"

namespace opentxs
{
using ReturnType = identity::credential::implementation::Verification;

auto Factory::VerificationCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const identity::credential::internal::Primary& master,
    const NymParameters& parameters,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason)
    -> identity::credential::internal::Verification*
{
    try {

        return new ReturnType(
            api, parent, source, master, parameters, version, reason);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to create credential: ")(e.what())
            .Flush();

        return nullptr;
    }
}

auto Factory::VerificationCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const identity::credential::internal::Primary& master,
    const proto::Credential& serialized)
    -> identity::credential::internal::Verification*
{
    try {

        return new ReturnType(api, parent, source, master, serialized);
    } catch (const std::exception& e) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Failed to deserialize credential: ")(e.what())
            .Flush();

        return nullptr;
    }
}
}  // namespace opentxs

namespace opentxs::identity::credential
{
// static
auto Verification::SigningForm(const proto::Verification& item)
    -> proto::Verification
{
    proto::Verification signingForm(item);
    signingForm.clear_sig();

    return signingForm;
}

// static
auto Verification::VerificationID(
    const api::internal::Core& api,
    const proto::Verification& item) -> std::string
{
    return api.Factory().Identifier(item)->str();
}
}  // namespace opentxs::identity::credential

namespace opentxs::identity::credential::implementation
{
Verification::Verification(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const internal::Primary& master,
    const NymParameters& params,
    const VersionNumber version,
    const PasswordPrompt& reason) noexcept(false)
    : credential::implementation::Base(
          api,
          parent,
          source,
          params,
          version,
          identity::CredentialRole::Verify,
          crypto::key::asymmetric::Mode::Null,
          get_master_id(master))
    , data_([&](const NymParameters& params) -> const proto::VerificationSet {
        auto proto = proto::VerificationSet{};
        params.GetVerificationSet(proto);
        return proto;
    }(params))
{
    {
        Lock lock(lock_);
        first_time_init(lock);
    }

    init(master, reason);
}

Verification::Verification(
    const api::internal::Core& api,
    const identity::internal::Authority& parent,
    const identity::Source& source,
    const internal::Primary& master,
    const proto::Credential& serialized) noexcept(false)
    : credential::implementation::Base(
          api,
          parent,
          source,
          serialized,
          get_master_id(serialized, master))
    , data_(serialized.verification())
{
    Lock lock(lock_);
    init_serialized(lock);
}

auto Verification::GetVerificationSet(
    proto::VerificationSet& verificationSet) const -> bool
{
    verificationSet = proto::VerificationSet(data_);

    return true;
}

auto Verification::serialize(
    const Lock& lock,
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<Base::SerializedType>
{
    auto serializedCredential = Base::serialize(lock, asPrivate, asSigned);
    serializedCredential->set_mode(opentxs::crypto::key::internal::translate(
        crypto::key::asymmetric::Mode::Null));
    serializedCredential->clear_signature();  // this fixes a bug, but shouldn't

    if (asSigned) {
        auto masterSignature = MasterSignature();

        if (masterSignature) {
            // We do not own this pointer.
            proto::Signature* serializedMasterSignature =
                serializedCredential->add_signature();
            *serializedMasterSignature = *masterSignature;
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to get master signature.")
                .Flush();
        }
    }

    *serializedCredential->mutable_verification() = data_;

    return serializedCredential;
}

auto Verification::verify_internally(const Lock& lock) const -> bool
{
    // Perform common Credential verifications
    if (!Base::verify_internally(lock)) { return false; }

    for (auto& nym : data_.internal().identity()) {
        for (auto& claim : nym.verification()) {
            bool valid = parent_.Verify(claim);

            if (!valid) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Invalid claim verification.")
                    .Flush();

                return false;
            }
        }
    }

    return true;
}
}  // namespace opentxs::identity::credential::implementation

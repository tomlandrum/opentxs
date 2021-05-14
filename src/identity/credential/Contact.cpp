// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                     // IWYU pragma: associated
#include "1_Internal.hpp"                   // IWYU pragma: associated
#include "identity/credential/Contact.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

#include "2_Factory.hpp"
#include "identity/credential/Base.hpp"
#include "internal/api/Api.hpp"
#include "internal/contact/Contact.hpp"
#include "internal/crypto/key/Key.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/key/asymmetric/Mode.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "opentxs/identity/credential/Contact.hpp"
#include "opentxs/protobuf/Claim.pb.h"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/Credential.pb.h"
#include "opentxs/protobuf/Signature.pb.h"

#define OT_METHOD "opentxs::identity::credential::implementation::Contact::"

namespace opentxs
{
using ReturnType = identity::credential::implementation::Contact;

auto Factory::ContactCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const identity::credential::internal::Primary& master,
    const NymParameters& parameters,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason)
    -> identity::credential::internal::Contact*
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

auto Factory::ContactCredential(
    const api::internal::Core& api,
    identity::internal::Authority& parent,
    const identity::Source& source,
    const identity::credential::internal::Primary& master,
    const proto::Credential& serialized)
    -> identity::credential::internal::Contact*
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
auto Contact::ClaimID(
    const api::internal::Core& api,
    const std::string& nymid,
    const std::uint32_t section,
    const proto::ContactItem& item) -> std::string
{
    proto::Claim preimage;
    preimage.set_version(1);
    preimage.set_nymid(nymid);
    preimage.set_section(section);
    preimage.set_type(item.type());
    preimage.set_start(item.start());
    preimage.set_end(item.end());
    preimage.set_value(item.value());
    preimage.set_subtype(item.subtype());

    return String::Factory(ClaimID(api, preimage))->Get();
}

// static
auto Contact::ClaimID(
    const api::internal::Core& api,
    const std::string& nymid,
    const contact::ContactSectionName section,
    const contact::ContactItemType type,
    const std::int64_t start,
    const std::int64_t end,
    const std::string& value,
    const std::string& subtype) -> std::string
{
    proto::Claim preimage;
    preimage.set_version(1);
    preimage.set_nymid(nymid);
    preimage.set_section(contact::internal::translate(section));
    preimage.set_type(contact::internal::translate(type));
    preimage.set_start(start);
    preimage.set_end(end);
    preimage.set_value(value);
    preimage.set_subtype(subtype);

    return ClaimID(api, preimage)->str();
}

// static
auto Contact::ClaimID(
    const api::internal::Core& api,
    const proto::Claim& preimage) -> OTIdentifier
{
    return api.Factory().Identifier(preimage);
}

// static
auto Contact::asClaim(
    const api::internal::Core& api,
    const String& nymid,
    const std::uint32_t section,
    const proto::ContactItem& item) -> Claim
{
    std::set<std::uint32_t> attributes;

    for (auto& attrib : item.attribute()) { attributes.insert(attrib); }

    return Claim{
        ClaimID(api, nymid.Get(), section, item),
        section,
        item.type(),
        item.value(),
        item.start(),
        item.end(),
        attributes};
}
}  // namespace opentxs::identity::credential

namespace opentxs::identity::credential::implementation
{
Contact::Contact(
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
          identity::CredentialRole::Contact,
          crypto::key::asymmetric::Mode::Null,
          get_master_id(master))
    , data_([&](const NymParameters& params) -> const proto::ContactData {
        auto proto = proto::ContactData{};
        params.GetContactData(proto);
        return proto;
    }(params))
{
    {
        Lock lock(lock_);
        first_time_init(lock);
    }

    init(master, reason);
}

Contact::Contact(
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
    , data_(serialized.contactdata())
{
    Lock lock(lock_);
    init_serialized(lock);
}

auto Contact::GetContactData(proto::ContactData& contactData) const -> bool
{
    contactData = proto::ContactData(data_);

    return true;
}

auto Contact::serialize(
    const Lock& lock,
    const SerializationModeFlag asPrivate,
    const SerializationSignatureFlag asSigned) const
    -> std::shared_ptr<Base::SerializedType>
{
    auto serializedCredential = Base::serialize(lock, asPrivate, asSigned);
    serializedCredential->set_mode(
        crypto::key::internal::translate(crypto::key::asymmetric::Mode::Null));
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

    *serializedCredential->mutable_contactdata() = data_;

    return serializedCredential;
}

}  // namespace opentxs::identity::credential::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "internal/contact/Contact.hpp"  // IWYU pragma: associated

#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::contact::internal
{
auto contactitemattribute_map() noexcept -> const ContactItemAttributeMap&
{
    static const auto map = ContactItemAttributeMap{
        {ContactItemAttribute::Error, proto::CITEMATTR_ERROR},
        {ContactItemAttribute::Active, proto::CITEMATTR_ACTIVE},
        {ContactItemAttribute::Primary, proto::CITEMATTR_PRIMARY},
        {ContactItemAttribute::Local, proto::CITEMATTR_LOCAL},
    };

    return map;
}

auto contactsectionname_map() noexcept -> const ContactSectionNameMap&
{
    static const auto map = ContactSectionNameMap{
        {ContactSectionName::Error, proto::CONTACTSECTION_ERROR},
        {ContactSectionName::Scope, proto::CONTACTSECTION_SCOPE},
        {ContactSectionName::Identifier, proto::CONTACTSECTION_IDENTIFIER},
        {ContactSectionName::Address, proto::CONTACTSECTION_ADDRESS},
        {ContactSectionName::Communication,
         proto::CONTACTSECTION_COMMUNICATION},
        {ContactSectionName::Profile, proto::CONTACTSECTION_PROFILE},
        {ContactSectionName::Relationship, proto::CONTACTSECTION_RELATIONSHIP},
        {ContactSectionName::Descriptor, proto::CONTACTSECTION_DESCRIPTOR},
        {ContactSectionName::Event, proto::CONTACTSECTION_EVENT},
        {ContactSectionName::Contract, proto::CONTACTSECTION_CONTRACT},
        {ContactSectionName::Procedure, proto::CONTACTSECTION_PROCEDURE},
    };

    return map;
}

auto translate(const ContactItemAttribute in) noexcept
    -> proto::ContactItemAttribute
{
    try {
        return contactitemattribute_map().at(in);
    } catch (...) {
        return proto::CITEMATTR_ERROR;
    }
}

auto translate(const ContactSectionName in) noexcept
    -> proto::ContactSectionName
{
    try {
        return contactsectionname_map().at(in);
    } catch (...) {
        return proto::CONTACTSECTION_ERROR;
    }
}

auto translate(const proto::ContactItemAttribute in) noexcept
    -> ContactItemAttribute
{
    static const auto map = reverse_arbitrary_map<
        ContactItemAttribute,
        contact::ContactItemAttribute,
        ContactItemAttributeReverseMap>(contactitemattribute_map());

    try {
        return map.at(in);
    } catch (...) {
        return ContactItemAttribute::Error;
    }
}

auto translate(const proto::ContactSectionName in) noexcept
    -> ContactSectionName
{
    static const auto map = reverse_arbitrary_map<
        ContactSectionName,
        contact::ContactSectionName,
        ContactSectionNameReverseMap>(contactsectionname_map());

    try {
        return map.at(in);
    } catch (...) {
        return ContactSectionName::Error;
    }
}
}  // namespace opentxs::contact::internal

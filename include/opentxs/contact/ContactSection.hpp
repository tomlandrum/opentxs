// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONTACT_CONTACTSECTION_HPP
#define OPENTXS_CONTACT_CONTACTSECTION_HPP

// IWYU pragma: no_include "opentxs/contact/ContactSectionName.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <iosfwd>
#include <map>
#include <memory>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace proto
{
class ContactData;
class ContactSection;
}  // namespace proto

class ContactGroup;
class ContactItem;
class Identifier;
}  // namespace opentxs

namespace opentxs
{
class ContactSection
{
public:
    using GroupMap =
        std::map<contact::ContactItemType, std::shared_ptr<ContactGroup>>;

    OPENTXS_EXPORT ContactSection(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber parentVersion,
        const contact::ContactSectionName section,
        const GroupMap& groups);
    OPENTXS_EXPORT ContactSection(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber parentVersion,
        const contact::ContactSectionName section,
        const std::shared_ptr<ContactItem>& item);
    OPENTXS_EXPORT ContactSection(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber parentVersion,
        const proto::ContactSection& serialized);
    OPENTXS_EXPORT ContactSection(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber parentVersion,
        const Space& serialized);
    OPENTXS_EXPORT ContactSection(const ContactSection&) noexcept;
    OPENTXS_EXPORT ContactSection(ContactSection&&) noexcept;

    OPENTXS_EXPORT ContactSection operator+(const ContactSection& rhs) const;

    OPENTXS_EXPORT ContactSection
    AddItem(const std::shared_ptr<ContactItem>& item) const;
    OPENTXS_EXPORT GroupMap::const_iterator begin() const;
    OPENTXS_EXPORT std::shared_ptr<ContactItem> Claim(
        const Identifier& item) const;
    OPENTXS_EXPORT ContactSection Delete(const Identifier& id) const;
    OPENTXS_EXPORT GroupMap::const_iterator end() const;
    OPENTXS_EXPORT std::shared_ptr<ContactGroup> Group(
        const contact::ContactItemType& type) const;
    OPENTXS_EXPORT bool HaveClaim(const Identifier& item) const;
    OPENTXS_EXPORT bool Serialize(
        AllocateOutput destination,
        const bool withIDs = false) const;
    OPENTXS_EXPORT bool SerializeTo(
        proto::ContactData& data,
        const bool withIDs = false) const;
    OPENTXS_EXPORT std::size_t Size() const;
    OPENTXS_EXPORT const contact::ContactSectionName& Type() const;
    OPENTXS_EXPORT VersionNumber Version() const;

    OPENTXS_EXPORT ~ContactSection();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;

    ContactSection() = delete;
    ContactSection& operator=(const ContactSection&) = delete;
    ContactSection& operator=(ContactSection&&) = delete;
};
}  // namespace opentxs
#endif

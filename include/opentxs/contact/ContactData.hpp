// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONTACT_CONTACTDATA_HPP
#define OPENTXS_CONTACT_CONTACTDATA_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/protobuf/ContactData.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

class ContactGroup;
class ContactItem;
class ContactSection;

class ContactData
{
public:
    using SectionMap =
        std::map<contact::ContactSectionName, std::shared_ptr<ContactSection>>;

    OPENTXS_EXPORT static std::string PrintContactData(
        const proto::ContactData& data);

    OPENTXS_EXPORT ContactData(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber targetVersion,
        const SectionMap& sections);
    OPENTXS_EXPORT ContactData(
        const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber targetVersion,
        const proto::ContactData& serialized);
    OPENTXS_EXPORT ContactData(const ContactData&);

    OPENTXS_EXPORT ContactData operator+(const ContactData& rhs) const;

    OPENTXS_EXPORT operator std::string() const;

    OPENTXS_EXPORT ContactData AddContract(
        const std::string& instrumentDefinitionID,
        const proto::ContactItemType currency,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT ContactData AddEmail(
        const std::string& value,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT ContactData AddItem(const Claim& claim) const;
    OPENTXS_EXPORT ContactData
    AddItem(const std::shared_ptr<ContactItem>& item) const;
    OPENTXS_EXPORT ContactData AddPaymentCode(
        const std::string& code,
        const proto::ContactItemType currency,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT ContactData AddPhoneNumber(
        const std::string& value,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT ContactData
    AddPreferredOTServer(const Identifier& id, const bool primary) const;
    OPENTXS_EXPORT ContactData AddSocialMediaProfile(
        const std::string& value,
        const proto::ContactItemType type,
        const bool primary,
        const bool active) const;
    OPENTXS_EXPORT SectionMap::const_iterator begin() const;
    OPENTXS_EXPORT std::string BestEmail() const;
    OPENTXS_EXPORT std::string BestPhoneNumber() const;
    OPENTXS_EXPORT std::string BestSocialMediaProfile(
        const proto::ContactItemType type) const;
    OPENTXS_EXPORT std::shared_ptr<ContactItem> Claim(
        const Identifier& item) const;
    OPENTXS_EXPORT std::set<OTIdentifier> Contracts(
        const proto::ContactItemType currency,
        const bool onlyActive) const;
    OPENTXS_EXPORT ContactData Delete(const Identifier& id) const;
    OPENTXS_EXPORT std::string EmailAddresses(bool active = true) const;
    OPENTXS_EXPORT SectionMap::const_iterator end() const;
    OPENTXS_EXPORT std::shared_ptr<ContactGroup> Group(
        const contact::ContactSectionName& section,
        const proto::ContactItemType& type) const;
    OPENTXS_EXPORT bool HaveClaim(const Identifier& item) const;
    OPENTXS_EXPORT bool HaveClaim(
        const contact::ContactSectionName& section,
        const proto::ContactItemType& type,
        const std::string& value) const;
    OPENTXS_EXPORT std::string Name() const;
    OPENTXS_EXPORT std::string PhoneNumbers(bool active = true) const;
    OPENTXS_EXPORT OTServerID PreferredOTServer() const;
    OPENTXS_EXPORT std::shared_ptr<ContactSection> Section(
        const contact::ContactSectionName& section) const;
    OPENTXS_EXPORT proto::ContactData Serialize(
        const bool withID = false) const;
    OPENTXS_EXPORT ContactData SetCommonName(const std::string& name) const;
    OPENTXS_EXPORT ContactData
    SetName(const std::string& name, const bool primary = true) const;
    OPENTXS_EXPORT ContactData
    SetScope(const proto::ContactItemType type, const std::string& name) const;
    OPENTXS_EXPORT std::string SocialMediaProfiles(
        const proto::ContactItemType type,
        bool active = true) const;
    OPENTXS_EXPORT const std::set<proto::ContactItemType>
    SocialMediaProfileTypes() const;
    OPENTXS_EXPORT proto::ContactItemType Type() const;
    OPENTXS_EXPORT VersionNumber Version() const;

    OPENTXS_EXPORT ~ContactData();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;

    ContactData() = delete;
    ContactData(ContactData&&) = delete;
    ContactData& operator=(const ContactData&) = delete;
    ContactData& operator=(ContactData&&) = delete;
};
}  // namespace opentxs

#endif

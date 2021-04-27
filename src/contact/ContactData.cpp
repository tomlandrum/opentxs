// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                     // IWYU pragma: associated
#include "1_Internal.hpp"                   // IWYU pragma: associated
#include "opentxs/contact/ContactData.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <iterator>
#include <sstream>
#include <tuple>
#include <utility>

#include "internal/contact/Contact.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/contact/ContactSection.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/protobuf/Contact.hpp"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/ContactSection.pb.h"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"

#define OT_METHOD "opentxs::ContactData::"

namespace opentxs
{
static auto check_version(
    const VersionNumber in,
    const VersionNumber targetVersion) -> VersionNumber
{
    // Upgrade version
    if (targetVersion > in) { return targetVersion; }

    return in;
}

static auto extract_sections(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber targetVersion,
    const proto::ContactData& serialized) -> ContactData::SectionMap
{
    ContactData::SectionMap sectionMap{};

    for (const auto& it : serialized.section()) {
        if ((0 != it.version()) && (it.item_size() > 0)) {
            sectionMap[contact::internal::translate(it.name())].reset(
                new ContactSection(
                    api,
                    nym,
                    check_version(serialized.version(), targetVersion),
                    it));
        }
    }

    return sectionMap;
}

struct ContactData::Imp {
    using Scope = std::
        pair<contact::ContactItemType, std::shared_ptr<const ContactGroup>>;

    const api::internal::Core& api_;
    const VersionNumber version_{0};
    const std::string nym_{};
    const SectionMap sections_{};

    Imp(const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber targetVersion,
        const SectionMap& sections)
        : api_(api)
        , version_(check_version(version, targetVersion))
        , nym_(nym)
        , sections_(sections)
    {
        if (0 == version) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Warning: malformed version. "
                "Setting to ")(targetVersion)(".")
                .Flush();
        }
    }

    Imp(const Imp& rhs)
        : api_(rhs.api_)
        , version_(rhs.version_)
        , nym_(rhs.nym_)
        , sections_(rhs.sections_)
    {
    }

    auto scope() const -> Scope
    {
        const auto it = sections_.find(contact::ContactSectionName::Scope);

        if (sections_.end() == it) {
            return {contact::ContactItemType::Unknown, nullptr};
        }

        OT_ASSERT(it->second);

        const auto& section = *it->second;

        if (1 != section.Size()) {
            return {contact::ContactItemType::Error, nullptr};
        }

        return *section.begin();
    }
};

ContactData::ContactData(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber version,
    const VersionNumber targetVersion,
    const SectionMap& sections)
    : imp_(std::make_unique<Imp>(api, nym, version, targetVersion, sections))
{
    OT_ASSERT(imp_);
}

ContactData::ContactData(const ContactData& rhs)
    : imp_(std::make_unique<Imp>(*rhs.imp_))
{
    OT_ASSERT(imp_);
}

ContactData::ContactData(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber targetVersion,
    const proto::ContactData& serialized)
    : ContactData(
          api,
          nym,
          serialized.version(),
          targetVersion,
          extract_sections(api, nym, targetVersion, serialized))
{
}

ContactData::ContactData(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber targetVersion,
    const ReadView& serialized)
    : ContactData(
          api,
          nym,
          targetVersion,
          proto::Factory<proto::ContactData>(serialized))
{
}

auto ContactData::operator+(const ContactData& rhs) const -> ContactData
{
    auto map{imp_->sections_};

    for (auto& it : rhs.imp_->sections_) {
        const auto& rhsID = it.first;
        const auto& rhsSection = it.second;

        OT_ASSERT(rhsSection);

        auto lhs = map.find(rhsID);
        const bool exists = (map.end() != lhs);

        if (exists) {
            auto& section = lhs->second;

            OT_ASSERT(section);

            section.reset(new ContactSection(*section + *rhsSection));

            OT_ASSERT(section);
        } else {
            const auto [it, inserted] = map.emplace(rhsID, rhsSection);

            OT_ASSERT(inserted);

            [[maybe_unused]] const auto& notUsed = it;
        }
    }

    const auto version = std::max(imp_->version_, rhs.imp_->version_);

    return ContactData(imp_->api_, imp_->nym_, version, version, map);
}

ContactData::operator std::string() const
{
    return PrintContactData(Serialize(false));
}

auto ContactData::AddContract(
    const std::string& instrumentDefinitionID,
    const contact::ContactItemType currency,
    const bool primary,
    const bool active) const -> ContactData
{
    bool needPrimary{true};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Contract};
    auto group = Group(section, currency);

    if (group) { needPrimary = group->Primary().empty(); }

    std::set<contact::ContactItemAttribute> attrib{};

    if (active || primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Active);
    }

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(section),
        contact::internal::translate(currency),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        section,
        currency,
        instrumentDefinitionID,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::AddEmail(
    const std::string& value,
    const bool primary,
    const bool active) const -> ContactData
{
    bool needPrimary{true};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Communication};
    const contact::ContactItemType type{contact::ContactItemType::Email};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    std::set<contact::ContactItemAttribute> attrib{};

    if (active || primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Active);
    }

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(section),
        contact::internal::translate(type),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        section,
        type,
        value,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::AddItem(const ClaimTuple& claim) const -> ContactData
{
    auto version = proto::RequiredVersion(
        contact::internal::translate(
            static_cast<contact::ContactSectionName>(std::get<1>(claim))),
        contact::internal::translate(
            static_cast<contact::ContactItemType>(std::get<2>(claim))),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_, imp_->nym_, version, version, claim);

    return AddItem(item);
}

auto ContactData::AddItem(const std::shared_ptr<ContactItem>& item) const
    -> ContactData
{
    OT_ASSERT(item);

    const auto& sectionID = item->Section();
    auto map{imp_->sections_};
    auto it = map.find(sectionID);

    auto version = proto::RequiredVersion(
        contact::internal::translate(sectionID),
        contact::internal::translate(item->Type()),
        imp_->version_);

    if (map.end() == it) {
        auto& section = map[sectionID];
        section.reset(new ContactSection(
            imp_->api_, imp_->nym_, version, version, sectionID, item));

        OT_ASSERT(section);
    } else {
        auto& section = it->second;

        OT_ASSERT(section);

        section.reset(new ContactSection(section->AddItem(item)));

        OT_ASSERT(section);
    }

    return ContactData(imp_->api_, imp_->nym_, version, version, map);
}

auto ContactData::AddPaymentCode(
    const std::string& code,
    const contact::ContactItemType currency,
    const bool primary,
    const bool active) const -> ContactData
{
    bool needPrimary{true};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Procedure};
    auto group = Group(section, currency);

    if (group) { needPrimary = group->Primary().empty(); }

    std::set<contact::ContactItemAttribute> attrib{};

    if (active || primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Active);
    }

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(section),
        contact::internal::translate(currency),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        section,
        currency,
        code,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::AddPhoneNumber(
    const std::string& value,
    const bool primary,
    const bool active) const -> ContactData
{
    bool needPrimary{true};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Communication};
    const contact::ContactItemType type{contact::ContactItemType::Phone};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    std::set<contact::ContactItemAttribute> attrib{};

    if (active || primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Active);
    }

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(section),
        contact::internal::translate(type),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        section,
        type,
        value,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::AddPreferredOTServer(const Identifier& id, const bool primary)
    const -> ContactData
{
    bool needPrimary{true};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Communication};
    const contact::ContactItemType type{contact::ContactItemType::Opentxs};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    std::set<contact::ContactItemAttribute> attrib{
        contact::ContactItemAttribute::Active};

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(section),
        contact::internal::translate(type),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        section,
        type,
        String::Factory(id)->Get(),
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::AddSocialMediaProfile(
    const std::string& value,
    const contact::ContactItemType type,
    const bool primary,
    const bool active) const -> ContactData
{
    auto map = imp_->sections_;
    // Add the item to the profile section.
    auto& section = map[contact::ContactSectionName::Profile];

    bool needPrimary{true};
    if (section) {
        auto group = section->Group(type);

        if (group) { needPrimary = group->Primary().empty(); }
    }

    std::set<contact::ContactItemAttribute> attrib{};

    if (active || primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Active);
    }

    if (primary || needPrimary) {
        attrib.emplace(contact::ContactItemAttribute::Primary);
    }

    auto version = proto::RequiredVersion(
        contact::internal::translate(contact::ContactSectionName::Profile),
        contact::internal::translate(type),
        imp_->version_);

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        version,
        version,
        contact::ContactSectionName::Profile,
        type,
        value,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    if (section) {
        section.reset(new ContactSection(section->AddItem(item)));
    } else {
        section.reset(new ContactSection(
            imp_->api_,
            imp_->nym_,
            version,
            version,
            contact::ContactSectionName::Profile,
            item));
    }

    OT_ASSERT(section);

    // Add the item to the communication section.
    auto commSectionTypes =
        proto::AllowedItemTypes().at(proto::ContactSectionVersion(
            version,
            contact::internal::translate(
                contact::ContactSectionName::Communication)));
    if (commSectionTypes.count(contact::internal::translate(type))) {
        auto& commSection = map[contact::ContactSectionName::Communication];

        if (commSection) {
            auto group = commSection->Group(type);

            if (group) { needPrimary = group->Primary().empty(); }
        }

        attrib.clear();

        if (active || primary || needPrimary) {
            attrib.emplace(contact::ContactItemAttribute::Active);
        }

        if (primary || needPrimary) {
            attrib.emplace(contact::ContactItemAttribute::Primary);
        }

        item = std::make_shared<ContactItem>(
            imp_->api_,
            imp_->nym_,
            version,
            version,
            contact::ContactSectionName::Communication,
            type,
            value,
            attrib,
            NULL_START,
            NULL_END,
            "");

        OT_ASSERT(item);

        if (commSection) {
            commSection.reset(new ContactSection(commSection->AddItem(item)));
        } else {
            commSection.reset(new ContactSection(
                imp_->api_,
                imp_->nym_,
                version,
                version,
                contact::ContactSectionName::Communication,
                item));
        }

        OT_ASSERT(commSection);
    }

    // Add the item to the identifier section.
    auto identifierSectionTypes =
        proto::AllowedItemTypes().at(proto::ContactSectionVersion(
            version,
            contact::internal::translate(
                contact::ContactSectionName::Identifier)));
    if (identifierSectionTypes.count(contact::internal::translate(type))) {
        auto& identifierSection = map[contact::ContactSectionName::Identifier];

        if (identifierSection) {
            auto group = identifierSection->Group(type);

            if (group) { needPrimary = group->Primary().empty(); }
        }

        attrib.clear();

        if (active || primary || needPrimary) {
            attrib.emplace(contact::ContactItemAttribute::Active);
        }

        if (primary || needPrimary) {
            attrib.emplace(contact::ContactItemAttribute::Primary);
        }

        item = std::make_shared<ContactItem>(
            imp_->api_,
            imp_->nym_,
            version,
            version,
            contact::ContactSectionName::Identifier,
            type,
            value,
            attrib,
            NULL_START,
            NULL_END,
            "");

        OT_ASSERT(item);

        if (identifierSection) {
            identifierSection.reset(
                new ContactSection(identifierSection->AddItem(item)));
        } else {
            identifierSection.reset(new ContactSection(
                imp_->api_,
                imp_->nym_,
                version,
                version,
                contact::ContactSectionName::Identifier,
                item));
        }

        OT_ASSERT(identifierSection);
    }

    return ContactData(imp_->api_, imp_->nym_, version, version, map);
}

ContactData::SectionMap::const_iterator ContactData::begin() const
{
    return imp_->sections_.begin();
}

auto ContactData::BestEmail() const -> std::string
{
    std::string bestEmail;

    auto group = Group(
        contact::ContactSectionName::Communication,
        contact::ContactItemType::Email);

    if (group) {
        std::shared_ptr<ContactItem> best = group->Best();

        if (best) { bestEmail = best->Value(); }
    }

    return bestEmail;
}

auto ContactData::BestPhoneNumber() const -> std::string
{
    std::string bestEmail;

    auto group = Group(
        contact::ContactSectionName::Communication,
        contact::ContactItemType::Phone);

    if (group) {
        std::shared_ptr<ContactItem> best = group->Best();

        if (best) { bestEmail = best->Value(); }
    }

    return bestEmail;
}

auto ContactData::BestSocialMediaProfile(
    const contact::ContactItemType type) const -> std::string
{
    std::string bestProfile;

    auto group = Group(contact::ContactSectionName::Profile, type);
    if (group) {
        std::shared_ptr<ContactItem> best = group->Best();

        if (best) { bestProfile = best->Value(); }
    }

    return bestProfile;
}

auto ContactData::Claim(const Identifier& item) const
    -> std::shared_ptr<ContactItem>
{
    for (const auto& it : imp_->sections_) {
        const auto& section = it.second;

        OT_ASSERT(section);

        auto claim = section->Claim(item);

        if (claim) { return claim; }
    }

    return {};
}

auto ContactData::Contracts(
    const contact::ContactItemType currency,
    const bool onlyActive) const -> std::set<OTIdentifier>
{
    std::set<OTIdentifier> output{};
    const contact::ContactSectionName section{
        contact::ContactSectionName::Contract};
    auto group = Group(section, currency);

    if (group) {
        for (const auto& it : *group) {
            const auto& id = it.first;

            OT_ASSERT(it.second);

            const auto& claim = *it.second;

            if ((false == onlyActive) || claim.isActive()) {
                output.insert(id);
            }
        }
    }

    return output;
}

auto ContactData::Delete(const Identifier& id) const -> ContactData
{
    bool deleted{false};
    auto map = imp_->sections_;

    for (auto& it : map) {
        auto& section = it.second;

        OT_ASSERT(section);

        if (section->HaveClaim(id)) {
            section.reset(new ContactSection(section->Delete(id)));

            OT_ASSERT(section);

            deleted = true;

            if (0 == section->Size()) { map.erase(it.first); }

            break;
        }
    }

    if (false == deleted) { return *this; }

    return ContactData(
        imp_->api_, imp_->nym_, imp_->version_, imp_->version_, map);
}

auto ContactData::EmailAddresses(bool active) const -> std::string
{
    std::ostringstream stream;

    auto group = Group(
        contact::ContactSectionName::Communication,
        contact::ContactItemType::Email);
    if (group) {
        for (const auto& it : *group) {
            OT_ASSERT(it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.isActive()) {
                stream << claim.Value() << ',';
            }
        }
    }

    std::string output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

ContactData::SectionMap::const_iterator ContactData::end() const
{
    return imp_->sections_.end();
}

auto ContactData::Group(
    const contact::ContactSectionName& section,
    const contact::ContactItemType& type) const -> std::shared_ptr<ContactGroup>
{
    const auto it = imp_->sections_.find(section);

    if (imp_->sections_.end() == it) { return {}; }

    OT_ASSERT(it->second);

    return it->second->Group(type);
}

auto ContactData::HaveClaim(const Identifier& item) const -> bool
{
    for (const auto& section : imp_->sections_) {
        OT_ASSERT(section.second);

        if (section.second->HaveClaim(item)) { return true; }
    }

    return false;
}

auto ContactData::HaveClaim(
    const contact::ContactSectionName& section,
    const contact::ContactItemType& type,
    const std::string& value) const -> bool
{
    auto group = Group(section, type);

    if (false == bool(group)) { return false; }

    for (const auto& it : *group) {
        OT_ASSERT(it.second);

        const auto& claim = *it.second;

        if (value == claim.Value()) { return true; }
    }

    return false;
}

auto ContactData::Name() const -> std::string
{
    auto group = imp_->scope().second;

    if (false == bool(group)) { return {}; }

    auto claim = group->Best();

    if (false == bool(claim)) { return {}; }

    return claim->Value();
}

auto ContactData::PhoneNumbers(bool active) const -> std::string
{
    std::ostringstream stream;

    auto group = Group(
        contact::ContactSectionName::Communication,
        contact::ContactItemType::Phone);
    if (group) {
        for (const auto& it : *group) {
            OT_ASSERT(it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.isActive()) {
                stream << claim.Value() << ',';
            }
        }
    }

    std::string output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

auto ContactData::PreferredOTServer() const -> OTServerID
{
    auto group = Group(
        contact::ContactSectionName::Communication,
        contact::ContactItemType::Opentxs);

    if (false == bool(group)) { return identifier::Server::Factory(); }

    auto claim = group->Best();

    if (false == bool(claim)) { return identifier::Server::Factory(); }

    return identifier::Server::Factory(claim->Value());
}

auto ContactData::PrintContactData(const proto::ContactData& data)
    -> std::string
{
    std::stringstream output;
    output << "Version " << data.version() << " contact data" << std::endl;
    output << "Sections found: " << data.section().size() << std::endl;

    for (const auto& section : data.section()) {
        output << "- Section: " << proto::TranslateSectionName(section.name())
               << ", version: " << section.version() << " containing "
               << section.item().size() << " item(s)." << std::endl;

        for (const auto& item : section.item()) {
            output << "-- Item type: \""
                   << proto::TranslateItemType(item.type()) << "\", value: \""
                   << item.value() << "\", start: " << item.start()
                   << ", end: " << item.end() << ", version: " << item.version()
                   << std::endl
                   << "--- Attributes: ";

            for (const auto& attribute : item.attribute()) {
                output << proto::TranslateItemAttributes(
                              contact::internal::translate(
                                  static_cast<contact::ContactItemAttribute>(
                                      attribute)))
                       << " ";
            }

            output << std::endl;
        }
    }

    return output.str();
}

auto ContactData::Section(const contact::ContactSectionName& section) const
    -> std::shared_ptr<ContactSection>
{
    const auto it = imp_->sections_.find(section);

    if (imp_->sections_.end() == it) { return {}; }

    return it->second;
}

auto ContactData::SetCommonName(const std::string& name) const -> ContactData
{
    const contact::ContactSectionName section{
        contact::ContactSectionName::Identifier};
    const contact::ContactItemType type{contact::ContactItemType::Commonname};
    std::set<contact::ContactItemAttribute> attrib{
        contact::ContactItemAttribute::Active,
        contact::ContactItemAttribute::Primary};

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        section,
        type,
        name,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::SetName(const std::string& name, const bool primary) const
    -> ContactData
{
    const Imp::Scope& scopeInfo = imp_->scope();

    OT_ASSERT(scopeInfo.second);

    const contact::ContactSectionName section{
        contact::ContactSectionName::Scope};
    const contact::ContactItemType type = scopeInfo.first;

    std::set<contact::ContactItemAttribute> attrib{
        contact::ContactItemAttribute::Active};

    if (primary) { attrib.emplace(contact::ContactItemAttribute::Primary); }

    auto item = std::make_shared<ContactItem>(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        section,
        type,
        name,
        attrib,
        NULL_START,
        NULL_END,
        "");

    OT_ASSERT(item);

    return AddItem(item);
}

auto ContactData::SetScope(
    const contact::ContactItemType type,
    const std::string& name) const -> ContactData
{
    OT_ASSERT(type);

    const contact::ContactSectionName section{
        contact::ContactSectionName::Scope};

    if (contact::ContactItemType::Unknown == imp_->scope().first) {
        auto mapCopy = imp_->sections_;
        mapCopy.erase(section);
        std::set<contact::ContactItemAttribute> attrib{
            contact::ContactItemAttribute::Active,
            contact::ContactItemAttribute::Primary};

        auto version = proto::RequiredVersion(
            contact::internal::translate(section),
            contact::internal::translate(type),
            imp_->version_);

        auto item = std::make_shared<ContactItem>(
            imp_->api_,
            imp_->nym_,
            version,
            version,
            section,
            type,
            name,
            attrib,
            NULL_START,
            NULL_END,
            "");

        OT_ASSERT(item);

        auto newSection = std::make_shared<ContactSection>(
            imp_->api_, imp_->nym_, version, version, section, item);

        OT_ASSERT(newSection);

        mapCopy[section] = newSection;

        return ContactData(imp_->api_, imp_->nym_, version, version, mapCopy);
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Scope already set.").Flush();

        return *this;
    }
}

auto ContactData::Serialize(AllocateOutput destination, const bool withID) const
    -> bool
{
    write(Serialize(withID), destination);
    return true;
}

auto ContactData::Serialize(const bool withID) const -> proto::ContactData
{
    proto::ContactData output;
    output.set_version(imp_->version_);

    for (const auto& it : imp_->sections_) {
        const auto& section = it.second;

        OT_ASSERT(section);

        section->SerializeTo(output, withID);
    }

    return output;
}

auto ContactData::SocialMediaProfiles(
    const contact::ContactItemType type,
    bool active) const -> std::string
{
    std::ostringstream stream;

    auto group = Group(contact::ContactSectionName::Profile, type);
    if (group) {
        for (const auto& it : *group) {
            OT_ASSERT(it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.isActive()) {
                stream << claim.Value() << ',';
            }
        }
    }

    std::string output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

auto ContactData::SocialMediaProfileTypes() const
    -> const std::set<contact::ContactItemType>
{
    try {
        auto profiletypes =
            proto::AllowedItemTypes().at(proto::ContactSectionVersion(
                CONTACT_CONTACT_DATA_VERSION, proto::CONTACTSECTION_PROFILE));

        std::set<contact::ContactItemType> output;
        std::transform(
            profiletypes.begin(),
            profiletypes.end(),
            std::inserter(output, output.end()),
            [](proto::ContactItemType itemtype) -> contact::ContactItemType {
                return contact::internal::translate(itemtype);
            });

        return output;

    } catch (...) {
        return {};
    }
}

auto ContactData::Type() const -> contact::ContactItemType
{
    return imp_->scope().first;
}

auto ContactData::Version() const -> VersionNumber { return imp_->version_; }

ContactData::~ContactData() = default;
}  // namespace opentxs

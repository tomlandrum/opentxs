// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <utility>

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactSection.hpp"  // IWYU pragma: associated
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/ContactSection.pb.h"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"

#define OT_METHOD "opentxs::ContactSection::"

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

static auto create_group(
    const std::string& nym,
    const proto::ContactSectionName section,
    const std::shared_ptr<ContactItem>& item) -> ContactSection::GroupMap
{
    OT_ASSERT(item);

    ContactSection::GroupMap output{};
    const auto& itemType = item->Type();

    output[itemType].reset(new ContactGroup(nym, section, item));

    return output;
}

static auto extract_groups(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber parentVersion,
    const proto::ContactSection& serialized) -> ContactSection::GroupMap
{
    ContactSection::GroupMap groupMap{};
    std::map<proto::ContactItemType, ContactGroup::ItemMap> itemMaps{};
    const auto& section = serialized.name();

    for (const auto& item : serialized.item()) {
        const auto& itemType = item.type();
        auto instantiated = std::make_shared<ContactItem>(
            api,
            nym,
            check_version(serialized.version(), parentVersion),
            section,
            item);

        OT_ASSERT(instantiated);

        const auto& itemID = instantiated->ID();
        auto& itemMap = itemMaps[itemType];
        itemMap.emplace(itemID, instantiated);
    }

    for (const auto& itemMap : itemMaps) {
        const auto& type = itemMap.first;
        const auto& map = itemMap.second;
        auto& group = groupMap[type];
        group.reset(new ContactGroup(nym, section, type, map));
    }

    return groupMap;
}

struct ContactSection::Imp {
    const api::internal::Core& api_;
    const VersionNumber version_;
    const std::string nym_;
    const proto::ContactSectionName section_;
    const GroupMap groups_;

    auto add_scope(const std::shared_ptr<ContactItem>& item) const
        -> ContactSection
    {
        OT_ASSERT(item);

        auto scope = item;

        bool needsPrimary{true};

        const auto& groupID = scope->Type();
        GroupMap groups = groups_;
        const auto& group = groups[groupID];

        if (group) { needsPrimary = (1 > group->Size()); }

        if (needsPrimary && false == scope->isPrimary()) {
            scope.reset(new ContactItem(scope->SetPrimary(true)));
        }

        if (false == scope->isActive()) {
            scope.reset(new ContactItem(scope->SetActive(true)));
        }

        groups[groupID].reset(new ContactGroup(nym_, section_, scope));

        auto version = proto::RequiredVersion(section_, item->Type(), version_);

        return ContactSection(api_, nym_, version, version, section_, groups);
    }

    Imp(const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber parentVersion,
        const proto::ContactSectionName section,
        const GroupMap& groups)
        : api_(api)
        , version_(check_version(version, parentVersion))
        , nym_(nym)
        , section_(section)
        , groups_(groups)
    {
    }

    Imp(const Imp& rhs) noexcept
        : api_(rhs.api_)
        , version_(rhs.version_)
        , nym_(rhs.nym_)
        , section_(rhs.section_)
        , groups_(rhs.groups_)
    {
    }

    Imp(Imp&& rhs) noexcept
        : api_(rhs.api_)
        , version_(rhs.version_)
        , nym_(std::move(const_cast<std::string&>(rhs.nym_)))
        , section_(rhs.section_)
        , groups_(std::move(const_cast<GroupMap&>(rhs.groups_)))
    {
    }
};

ContactSection::ContactSection(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber version,
    const VersionNumber parentVersion,
    const proto::ContactSectionName section,
    const GroupMap& groups)
    : imp_(std::make_unique<
           Imp>(api, nym, version, parentVersion, section, groups))
{
}

ContactSection::ContactSection(const ContactSection& rhs) noexcept
    : imp_(std::make_unique<Imp>(*rhs.imp_))
{
    OT_ASSERT(imp_);
}

ContactSection::ContactSection(ContactSection&& rhs) noexcept
    : imp_(std::move(rhs.imp_))
{
    OT_ASSERT(imp_);
}

ContactSection::ContactSection(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber version,
    const VersionNumber parentVersion,
    const proto::ContactSectionName section,
    const std::shared_ptr<ContactItem>& item)
    : ContactSection(
          api,
          nym,
          version,
          parentVersion,
          section,
          create_group(nym, section, item))
{
    if (0 == version) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Warning: malformed version. "
                                           "Setting to ")(parentVersion)(".")
            .Flush();
    }
}

ContactSection::ContactSection(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber parentVersion,
    const proto::ContactSection& serialized)
    : ContactSection(
          api,
          nym,
          serialized.version(),
          parentVersion,
          serialized.name(),
          extract_groups(api, nym, parentVersion, serialized))
{
}

auto ContactSection::operator+(const ContactSection& rhs) const
    -> ContactSection
{
    auto map{imp_->groups_};

    for (auto& it : rhs.imp_->groups_) {
        auto& rhsID = it.first;
        auto& rhsGroup = it.second;

        OT_ASSERT(rhsGroup);

        auto lhs = map.find(rhsID);
        const bool exists = (map.end() != lhs);

        if (exists) {
            auto& group = lhs->second;

            OT_ASSERT(group);

            group.reset(new ContactGroup(*group + *rhsGroup));

            OT_ASSERT(group);
        } else {
            [[maybe_unused]] const auto [it, inserted] =
                map.emplace(rhsID, rhsGroup);

            OT_ASSERT(inserted);
        }
    }

    const auto version = std::max(imp_->version_, rhs.Version());

    return ContactSection(
        imp_->api_, imp_->nym_, version, version, imp_->section_, map);
}

auto ContactSection::AddItem(const std::shared_ptr<ContactItem>& item) const
    -> ContactSection
{
    OT_ASSERT(item);

    const bool specialCaseScope =
        (proto::CONTACTSECTION_SCOPE == imp_->section_);

    if (specialCaseScope) { return imp_->add_scope(item); }

    const auto& groupID = item->Type();
    const bool groupExists = imp_->groups_.count(groupID);
    auto map = imp_->groups_;

    if (groupExists) {
        auto& existing = map.at(groupID);

        OT_ASSERT(existing);

        existing.reset(new ContactGroup(existing->AddItem(item)));
    } else {
        map[groupID].reset(new ContactGroup(imp_->nym_, imp_->section_, item));
    }

    auto version =
        proto::RequiredVersion(imp_->section_, item->Type(), imp_->version_);

    return ContactSection(
        imp_->api_, imp_->nym_, version, version, imp_->section_, map);
}

auto ContactSection::begin() const -> ContactSection::GroupMap::const_iterator
{
    return imp_->groups_.cbegin();
}

auto ContactSection::Claim(const Identifier& item) const
    -> std::shared_ptr<ContactItem>
{
    for (const auto& group : imp_->groups_) {
        OT_ASSERT(group.second);

        auto claim = group.second->Claim(item);

        if (claim) { return claim; }
    }

    return {};
}

auto ContactSection::Delete(const Identifier& id) const -> ContactSection
{
    bool deleted{false};
    auto map = imp_->groups_;

    for (auto& it : map) {
        auto& group = it.second;

        OT_ASSERT(group);

        if (group->HaveClaim(id)) {
            group.reset(new ContactGroup(group->Delete(id)));
            deleted = true;

            if (0 == group->Size()) { map.erase(it.first); }

            break;
        }
    }

    if (false == deleted) { return *this; }

    return ContactSection(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        imp_->section_,
        map);
}

auto ContactSection::end() const -> ContactSection::GroupMap::const_iterator
{
    return imp_->groups_.cend();
}

auto ContactSection::Group(const proto::ContactItemType& type) const
    -> std::shared_ptr<ContactGroup>
{
    const auto it = imp_->groups_.find(type);

    if (imp_->groups_.end() == it) { return {}; }

    return it->second;
}

auto ContactSection::HaveClaim(const Identifier& item) const -> bool
{
    for (const auto& group : imp_->groups_) {
        OT_ASSERT(group.second);

        if (group.second->HaveClaim(item)) { return true; }
    }

    return false;
}

auto ContactSection::SerializeTo(
    proto::ContactData& section,
    const bool withIDs) const -> bool
{
    bool output = true;
    auto& serialized = *section.add_section();
    serialized.set_version(imp_->version_);
    serialized.set_name(imp_->section_);

    for (const auto& it : imp_->groups_) {
        const auto& group = it.second;

        OT_ASSERT(group);

        output &= group->SerializeTo(serialized, withIDs);
    }

    return output;
}

auto ContactSection::Size() const -> std::size_t
{
    return imp_->groups_.size();
}

auto ContactSection::Type() const -> const proto::ContactSectionName&
{
    return imp_->section_;
}

auto ContactSection::Version() const -> VersionNumber { return imp_->version_; }

ContactSection::~ContactSection() = default;
}  // namespace opentxs

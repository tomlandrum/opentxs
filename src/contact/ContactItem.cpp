// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                     // IWYU pragma: associated
#include "1_Internal.hpp"                   // IWYU pragma: associated
#include "opentxs/contact/ContactItem.hpp"  // IWYU pragma: associated

#include <memory>
#include <tuple>
#include <utility>

#include "internal/contact/Contact.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/identity/credential/Contact.hpp"
#include "opentxs/protobuf/ContactItem.pb.h"

#define OT_METHOD "opentxs::ContactItem::"

namespace opentxs
{
static auto extract_attributes(const proto::ContactItem& serialized)
    -> std::set<contact::ContactItemAttribute>
{
    std::set<contact::ContactItemAttribute> output{};

    for (const auto& attribute : serialized.attribute()) {
        output.emplace(static_cast<contact::ContactItemAttribute>(attribute));
    }

    return output;
}

static auto extract_attributes(const Claim& claim)
    -> std::set<contact::ContactItemAttribute>
{
    std::set<contact::ContactItemAttribute> output{};

    for (const auto& attribute : std::get<6>(claim)) {
        output.emplace(static_cast<contact::ContactItemAttribute>(attribute));
    }

    return output;
}

struct ContactItem::Imp {
    const api::internal::Core& api_;
    const VersionNumber version_;
    const std::string nym_;
    const contact::ContactSectionName section_;
    const contact::ContactItemType type_;
    const std::string value_;
    const std::time_t start_;
    const std::time_t end_;
    const std::set<contact::ContactItemAttribute> attributes_;
    const OTIdentifier id_;
    const std::string subtype_;

    static auto check_version(
        const VersionNumber in,
        const VersionNumber targetVersion) -> VersionNumber
    {
        // Upgrade version
        if (targetVersion > in) { return targetVersion; }

        return in;
    }

    Imp(const api::internal::Core& api,
        const std::string& nym,
        const VersionNumber version,
        const VersionNumber parentVersion,
        const contact::ContactSectionName section,
        const contact::ContactItemType& type,
        const std::string& value,
        const std::set<contact::ContactItemAttribute>& attributes,
        const std::time_t start,
        const std::time_t end,
        const std::string subtype)
        : api_(api)
        , version_(check_version(version, parentVersion))
        , nym_(nym)
        , section_(section)
        , type_(type)
        , value_(value)
        , start_(start)
        , end_(end)
        , attributes_(attributes)
        , id_(Identifier::Factory(
              identity::credential::Contact::
                  ClaimID(api, nym, section, type, start, end, value, subtype)))
        , subtype_(subtype)
    {
        if (0 == version) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Warning: malformed version. "
                "Setting to ")(parentVersion)(".")
                .Flush();
        }
    }

    Imp(const Imp& rhs)
        : api_(rhs.api_)
        , version_(rhs.version_)
        , nym_(rhs.nym_)
        , section_(rhs.section_)
        , type_(rhs.type_)
        , value_(rhs.value_)
        , start_(rhs.start_)
        , end_(rhs.end_)
        , attributes_(rhs.attributes_)
        , id_(rhs.id_)
        , subtype_(rhs.subtype_)
    {
    }

    Imp(Imp&& rhs)
        : api_(rhs.api_)
        , version_(rhs.version_)
        , nym_(std::move(const_cast<std::string&>(rhs.nym_)))
        , section_(rhs.section_)
        , type_(rhs.type_)
        , value_(std::move(const_cast<std::string&>(rhs.value_)))
        , start_(rhs.start_)
        , end_(rhs.end_)
        , attributes_(
              std::move(const_cast<std::set<contact::ContactItemAttribute>&>(
                  rhs.attributes_)))
        , id_(std::move(const_cast<OTIdentifier&>(rhs.id_)))
        , subtype_(std::move(const_cast<std::string&>(rhs.subtype_)))
    {
    }

    auto set_attribute(
        const contact::ContactItemAttribute& attribute,
        const bool value) const -> ContactItem
    {
        auto attributes = attributes_;

        if (value) {
            attributes.emplace(attribute);

            if (contact::ContactItemAttribute::Primary == attribute) {
                attributes.emplace(contact::ContactItemAttribute::Active);
            }
        } else {
            attributes.erase(attribute);
        }

        return ContactItem(
            api_,
            nym_,
            version_,
            version_,
            section_,
            type_,
            value_,
            attributes,
            start_,
            end_,
            subtype_);
    }
};

ContactItem::ContactItem(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber version,
    const VersionNumber parentVersion,
    const contact::ContactSectionName section,
    const contact::ContactItemType& type,
    const std::string& value,
    const std::set<contact::ContactItemAttribute>& attributes,
    const std::time_t start,
    const std::time_t end,
    const std::string subtype)
    : imp_(std::make_unique<Imp>(
          api,
          nym,
          version,
          parentVersion,
          section,
          type,
          value,
          attributes,
          start,
          end,
          subtype))
{
    OT_ASSERT(imp_);
}

ContactItem::ContactItem(const ContactItem& rhs) noexcept
    : imp_(std::make_unique<Imp>(*rhs.imp_))
{
    OT_ASSERT(imp_);
}

ContactItem::ContactItem(ContactItem&& rhs) noexcept
    : imp_(std::move(rhs.imp_))
{
    OT_ASSERT(imp_);
}

ContactItem::ContactItem(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber version,
    const VersionNumber parentVersion,
    const Claim& claim)
    : ContactItem(
          api,
          nym,
          version,
          parentVersion,
          static_cast<contact::ContactSectionName>(std::get<1>(claim)),
          static_cast<contact::ContactItemType>(std::get<2>(claim)),
          std::get<3>(claim),
          extract_attributes(claim),
          std::get<4>(claim),
          std::get<5>(claim),
          "")
{
}

ContactItem::ContactItem(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber parentVersion,
    const contact::ContactSectionName section,
    const proto::ContactItem& data)
    : ContactItem(
          api,
          nym,
          data.version(),
          parentVersion,
          section,
          contact::internal::translate(data.type()),
          data.value(),
          extract_attributes(data),
          data.start(),
          data.end(),
          data.subtype())
{
}

ContactItem::ContactItem(
    const api::internal::Core& api,
    const std::string& nym,
    const VersionNumber parentVersion,
    const contact::ContactSectionName section,
    const Space& bytes)
    : ContactItem(
          api,
          nym,
          parentVersion,
          section,
          proto::Factory<proto::ContactItem>(bytes.data(), bytes.size()))
{
}

auto ContactItem::operator==(const ContactItem& rhs) const -> bool
{
    if (false == (imp_->version_ == rhs.imp_->version_)) { return false; }

    if (false == (imp_->nym_ == rhs.imp_->nym_)) { return false; }

    if (false == (imp_->section_ == rhs.imp_->section_)) { return false; }

    if (false == (imp_->type_ == rhs.imp_->type_)) { return false; }

    if (false == (imp_->value_ == rhs.imp_->value_)) { return false; }

    if (false == (imp_->start_ == rhs.imp_->start_)) { return false; }

    if (false == (imp_->end_ == rhs.imp_->end_)) { return false; }

    if (false == (imp_->attributes_ == rhs.imp_->attributes_)) { return false; }

    if (false == (imp_->id_ == rhs.imp_->id_)) { return false; }

    return true;
}

ContactItem::operator proto::ContactItem() const { return Serialize(true); }

auto ContactItem::End() const -> const std::time_t& { return imp_->end_; }

auto ContactItem::ID() const -> const Identifier& { return imp_->id_; }

auto ContactItem::isActive() const -> bool
{
    return 1 == imp_->attributes_.count(contact::ContactItemAttribute::Active);
}

auto ContactItem::isLocal() const -> bool
{
    return 1 == imp_->attributes_.count(contact::ContactItemAttribute::Local);
}

auto ContactItem::isPrimary() const -> bool
{
    return 1 == imp_->attributes_.count(contact::ContactItemAttribute::Primary);
}

auto ContactItem::Section() const -> const contact::ContactSectionName&
{
    return imp_->section_;
}

auto ContactItem::Serialize(AllocateOutput destination, const bool withID) const
    -> bool
{
    auto item = Serialize(withID);
    auto view = destination(item.ByteSizeLong());
    if (!item.SerializeToArray(view.data(), static_cast<int>(view.size()))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize the contactitem.")
            .Flush();
        return false;
    }
    return true;
}

auto ContactItem::Serialize(const bool withID) const -> proto::ContactItem
{
    proto::ContactItem output{};
    output.set_version(imp_->version_);

    if (withID) { output.set_id(String::Factory(imp_->id_)->Get()); }

    output.set_type(contact::internal::translate(imp_->type_));
    output.set_value(imp_->value_);
    output.set_start(imp_->start_);
    output.set_end(imp_->end_);

    for (const auto& attribute : imp_->attributes_) {
        output.add_attribute(contact::internal::translate(attribute));
    }

    return output;
}

auto ContactItem::SetActive(const bool active) const -> ContactItem
{
    const bool existingValue =
        1 == imp_->attributes_.count(contact::ContactItemAttribute::Active);

    if (existingValue == active) { return *this; }

    return imp_->set_attribute(contact::ContactItemAttribute::Active, active);
}

auto ContactItem::SetEnd(const std::time_t end) const -> ContactItem
{
    if (imp_->end_ == end) { return *this; }

    return ContactItem(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        imp_->section_,
        imp_->type_,
        imp_->value_,
        imp_->attributes_,
        imp_->start_,
        end,
        imp_->subtype_);
}

auto ContactItem::SetLocal(const bool local) const -> ContactItem
{
    const bool existingValue =
        1 == imp_->attributes_.count(contact::ContactItemAttribute::Local);

    if (existingValue == local) { return *this; }

    return imp_->set_attribute(contact::ContactItemAttribute::Local, local);
}

auto ContactItem::SetPrimary(const bool primary) const -> ContactItem
{
    const bool existingValue =
        1 == imp_->attributes_.count(contact::ContactItemAttribute::Primary);

    if (existingValue == primary) { return *this; }

    return imp_->set_attribute(contact::ContactItemAttribute::Primary, primary);
}

auto ContactItem::SetStart(const std::time_t start) const -> ContactItem
{
    if (imp_->start_ == start) { return *this; }

    return ContactItem(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        imp_->section_,
        imp_->type_,
        imp_->value_,
        imp_->attributes_,
        start,
        imp_->end_,
        imp_->subtype_);
}

auto ContactItem::SetValue(const std::string& value) const -> ContactItem
{
    if (imp_->value_ == value) { return *this; }

    return ContactItem(
        imp_->api_,
        imp_->nym_,
        imp_->version_,
        imp_->version_,
        imp_->section_,
        imp_->type_,
        value,
        imp_->attributes_,
        imp_->start_,
        imp_->end_,
        imp_->subtype_);
}

auto ContactItem::Start() const -> const std::time_t& { return imp_->start_; }

auto ContactItem::Subtype() const -> const std::string&
{
    return imp_->subtype_;
}

auto ContactItem::Type() const -> const contact::ContactItemType&
{
    return imp_->type_;
}

auto ContactItem::Value() const -> const std::string& { return imp_->value_; }

auto ContactItem::Version() const -> VersionNumber { return imp_->version_; }

ContactItem::~ContactItem() = default;
}  // namespace opentxs

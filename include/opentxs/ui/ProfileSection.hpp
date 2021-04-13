// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_PROFILESECTION_HPP
#define OPENTXS_UI_PROFILESECTION_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/ui/List.hpp"
#include "opentxs/ui/ListRow.hpp"

#ifdef SWIG
#include <algorithm>
#include <tuple>
#include <vector>

// clang-format off
%template(UIProfileItemType) std::pair<int, std::string>;
%template(UIProfileTypeList) std::vector<std::pair<int, std::string>>;
%extend opentxs::ui::ProfileSection {
    bool AddClaim(
        const int type,
        const std::string& value,
        const bool primary,
        const bool active) const
    {
        return $self->AddClaim(
            static_cast<opentxs::contact::ContactItemType>(type),
            value,
            primary,
            active);
    }
    static std::vector<std::pair<int, std::string>> AllowedItemTypes(
        const int section,
        const std::string& lang)
    {
        const auto types = opentxs::ui::ProfileSection::AllowedItems(
            static_cast<opentxs::contact::ContactSectionName>(section),
            lang);
        std::vector<std::pair<int, std::string>> output;
        std::transform(
            types.begin(),
            types.end(),
            std::inserter(output, output.end()),
            [](std::pair<opentxs::contact::ContactItemType, std::string> type) ->
                std::pair<int, std::string> {
                    return {static_cast<int>(type.first), type.second};} );

        return output;
    }
    std::vector<std::pair<int, std::string>> Items(
        const std::string& lang) const
    {
        const auto types =
            opentxs::ui::ProfileSection::AllowedItems($self->Type(), lang);
        std::vector<std::pair<int, std::string>> output;
        std::transform(
            types.begin(),
            types.end(),
            std::inserter(output, output.end()),
            [](std::pair<opentxs::contact::ContactItemType, std::string> type) ->
                std::pair<int, std::string> {
                    return {static_cast<int>(type.first), type.second};} );

        return output;
    }
    int Type() const
    {
        return static_cast<int>($self->Type());
    }
}
%ignore opentxs::ui::ProfileSection::AddClaim;
%ignore opentxs::ui::ProfileSection::AllowedItems;
%ignore opentxs::ui::ProfileSection::Items;
%ignore opentxs::ui::ProfileSection::Type;
%ignore opentxs::ui::ProfileSection::Update;
%template(OTUIProfileSection) opentxs::SharedPimpl<opentxs::ui::ProfileSection>;
%rename(UIProfileSection) opentxs::ui::ProfileSection;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ProfileSection;
class ProfileSubsection;
}  // namespace ui

using OTUIProfileSection = SharedPimpl<ui::ProfileSection>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ProfileSection : virtual public List, virtual public ListRow
{
public:
    using ItemType = std::pair<contact::ContactItemType, std::string>;
    using ItemTypeList = std::vector<ItemType>;

    OPENTXS_EXPORT static ItemTypeList AllowedItems(
        const contact::ContactSectionName section,
        const std::string& lang) noexcept;

    OPENTXS_EXPORT virtual bool AddClaim(
        const contact::ContactItemType type,
        const std::string& value,
        const bool primary,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Delete(
        const int type,
        const std::string& claimID) const noexcept = 0;
    OPENTXS_EXPORT virtual ItemTypeList Items(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Name(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileSubsection>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileSubsection>
    Next() const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetActive(
        const int type,
        const std::string& claimID,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetPrimary(
        const int type,
        const std::string& claimID,
        const bool primary) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetValue(
        const int type,
        const std::string& claimID,
        const std::string& value) const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactSectionName Type()
        const noexcept = 0;

    OPENTXS_EXPORT ~ProfileSection() override = default;

protected:
    ProfileSection() noexcept = default;

private:
    ProfileSection(const ProfileSection&) = delete;
    ProfileSection(ProfileSection&&) = delete;
    ProfileSection& operator=(const ProfileSection&) = delete;
    ProfileSection& operator=(ProfileSection&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_PROFILE_HPP
#define OPENTXS_UI_PROFILE_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/ui/List.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::Profile {
    bool AddClaim(
        const int section,
        const int type,
        const std::string& value,
        const bool primary,
        const bool active) const noexcept
    {
        return $self->AddClaim(
            static_cast<opentxs::contact::ContactSectionName>(section),
            static_cast<opentxs::contact::ContactItemType>(type),
            value,
            primary,
            active);
    }
    std::vector<std::pair<int, std::string>> AllowedItems(
        const int section,
        const std::string& lang) noexcept
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
    std::vector<std::pair<int, std::string>> AllowedSections(
        const std::string& lang) noexcept
    {
        const auto sections = $self->AllowedSections(lang);
        std::vector<std::pair<int, std::string>> output;
        std::transform(
            sections.begin(),
            sections.end(),
            std::inserter(output, output.end()),
            [](std::pair<opentxs::contact::ContactSectionName, std::string> type) ->
                std::pair<int, std::string> {
                    return {static_cast<int>(type.first), type.second};} );

        return output;
    }
}
%ignore opentxs::ui::Profile::AddClaim;
%ignore opentxs::ui::Profile::AllowedItems;
%ignore opentxs::ui::Profile::AllowedSections;
%rename(UIProfile) opentxs::ui::Profile;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class Profile;
class ProfileSection;
}  // namespace ui
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class Profile : virtual public List
{
public:
    using ItemType = std::pair<contact::ContactItemType, std::string>;
    using ItemTypeList = std::vector<ItemType>;
    using SectionType = std::pair<contact::ContactSectionName, std::string>;
    using SectionTypeList = std::vector<SectionType>;

    OPENTXS_EXPORT virtual bool AddClaim(
        const contact::ContactSectionName section,
        const contact::ContactItemType type,
        const std::string& value,
        const bool primary,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual ItemTypeList AllowedItems(
        const contact::ContactSectionName section,
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual SectionTypeList AllowedSections(
        const std::string& lang) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Delete(
        const int section,
        const int type,
        const std::string& claimID) const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayName() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileSection>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string ID() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ProfileSection>
    Next() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string PaymentCode() const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetActive(
        const int section,
        const int type,
        const std::string& claimID,
        const bool active) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetPrimary(
        const int section,
        const int type,
        const std::string& claimID,
        const bool primary) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SetValue(
        const int section,
        const int type,
        const std::string& claimID,
        const std::string& value) const noexcept = 0;

    OPENTXS_EXPORT ~Profile() override = default;

protected:
    Profile() noexcept = default;

private:
    Profile(const Profile&) = delete;
    Profile(Profile&&) = delete;
    Profile& operator=(const Profile&) = delete;
    Profile& operator=(Profile&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif

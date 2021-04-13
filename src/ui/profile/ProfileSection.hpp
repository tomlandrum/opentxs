// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/ProfileSection.cpp"

#pragma once

#if OT_QT
#include <QHash>
#endif  // OT_QT
#include <set>
#include <string>
#include <utility>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/ui/ProfileSection.hpp"
#include "ui/base/Combined.hpp"
#include "ui/base/List.hpp"
#include "ui/base/RowType.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

class ContactSection;
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using ProfileSectionList = List<
    ProfileSectionExternalInterface,
    ProfileSectionInternalInterface,
    ProfileSectionRowID,
    ProfileSectionRowInterface,
    ProfileSectionRowInternal,
    ProfileSectionRowBlank,
    ProfileSectionSortKey,
    ProfileSectionPrimaryID>;
using ProfileSectionRow =
    RowType<ProfileRowInternal, ProfileInternalInterface, ProfileRowID>;

class ProfileSection final
    : public Combined<ProfileSectionList, ProfileSectionRow, ProfileSortKey>
{
public:
    auto AddClaim(
        const contact::ContactItemType type,
        const std::string& value,
        const bool primary,
        const bool active) const noexcept -> bool final;
    auto Delete(const int type, const std::string& claimID) const noexcept
        -> bool final;
    auto Items(const std::string& lang) const noexcept -> ItemTypeList final;
#if OT_QT
    int FindRow(const ProfileSectionRowID& id) const noexcept final
    {
        return find_row(id);
    }
#endif
    auto Name(const std::string& lang) const noexcept -> std::string final;
    auto NymID() const noexcept -> const identifier::Nym& final
    {
        return primary_id_;
    }
    auto SetActive(
        const int type,
        const std::string& claimID,
        const bool active) const noexcept -> bool final;
    auto SetPrimary(
        const int type,
        const std::string& claimID,
        const bool primary) const noexcept -> bool final;
    auto SetValue(
        const int type,
        const std::string& claimID,
        const std::string& value) const noexcept -> bool final;
    auto Type() const noexcept -> contact::ContactSectionName final
    {
        return row_id_;
    }

    auto reindex(const ProfileSortKey& key, CustomData& custom) noexcept
        -> bool final;

    ProfileSection(
        const ProfileInternalInterface& parent,
        const api::client::internal::Manager& api,
        const ProfileRowID& rowID,
        const ProfileSortKey& key,
        CustomData& custom) noexcept;
    ~ProfileSection() final = default;

private:
    static auto sort_key(const ProfileSectionRowID type) noexcept -> int;
    static auto check_type(const ProfileSectionRowID type) noexcept -> bool;

    auto construct_row(
        const ProfileSectionRowID& id,
        const ProfileSectionSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;

    auto last(const ProfileSectionRowID& id) const noexcept -> bool final
    {
        return ProfileSectionList::last(id);
    }
    auto process_section(const opentxs::ContactSection& section) noexcept
        -> std::set<ProfileSectionRowID>;
    void startup(const opentxs::ContactSection section) noexcept;

    ProfileSection() = delete;
    ProfileSection(const ProfileSection&) = delete;
    ProfileSection(ProfileSection&&) = delete;
    auto operator=(const ProfileSection&) -> ProfileSection& = delete;
    auto operator=(ProfileSection&&) -> ProfileSection& = delete;
};
}  // namespace opentxs::ui::implementation

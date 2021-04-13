// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/ProfileSubsection.cpp"

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
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
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

namespace ui
{
class ProfileSubsection;
}  // namespace ui

class ContactGroup;
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using ProfileSubsectionList = List<
    ProfileSubsectionExternalInterface,
    ProfileSubsectionInternalInterface,
    ProfileSubsectionRowID,
    ProfileSubsectionRowInterface,
    ProfileSubsectionRowInternal,
    ProfileSubsectionRowBlank,
    ProfileSubsectionSortKey,
    ProfileSubsectionPrimaryID>;
using ProfileSubsectionRow = RowType<
    ProfileSectionRowInternal,
    ProfileSectionInternalInterface,
    ProfileSectionRowID>;

class ProfileSubsection final : public Combined<
                                    ProfileSubsectionList,
                                    ProfileSubsectionRow,
                                    ProfileSectionSortKey>
{
public:
    auto AddItem(
        const std::string& value,
        const bool primary,
        const bool active) const noexcept -> bool final;
    auto Delete(const std::string& claimID) const noexcept -> bool final;
    auto Name(const std::string& lang) const noexcept -> std::string final;
    auto NymID() const noexcept -> const identifier::Nym& final
    {
        return primary_id_;
    }
    auto Section() const noexcept -> contact::ContactSectionName final
    {
        return row_id_.first;
    }
    auto SetActive(const std::string& claimID, const bool active) const noexcept
        -> bool final;
    auto SetPrimary(const std::string& claimID, const bool primary)
        const noexcept -> bool final;
    auto SetValue(const std::string& claimID, const std::string& value)
        const noexcept -> bool final;
    auto Type() const noexcept -> contact::ContactItemType final
    {
        return row_id_.second;
    }

    auto reindex(const ProfileSectionSortKey& key, CustomData& custom) noexcept
        -> bool final;

    ProfileSubsection(
        const ProfileSectionInternalInterface& parent,
        const api::client::internal::Manager& api,
        const ProfileSectionRowID& rowID,
        const ProfileSectionSortKey& key,
        CustomData& custom) noexcept;
    ~ProfileSubsection() final = default;

private:
    ProfileSectionSortKey sequence_;

    static auto check_type(const ProfileSubsectionRowID type) -> bool;

    auto construct_row(
        const ProfileSubsectionRowID& id,
        const ProfileSubsectionSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;

    auto last(const ProfileSubsectionRowID& id) const noexcept -> bool final
    {
        return ProfileSubsectionList::last(id);
    }
    auto process_group(const opentxs::ContactGroup& group) noexcept
        -> std::set<ProfileSubsectionRowID>;
    void startup(const opentxs::ContactGroup group) noexcept;

    ProfileSubsection() = delete;
    ProfileSubsection(const ProfileSubsection&) = delete;
    ProfileSubsection(ProfileSubsection&&) = delete;
    auto operator=(const ProfileSubsection&) -> ProfileSubsection& = delete;
    auto operator=(ProfileSubsection&&) -> ProfileSubsection& = delete;
};
}  // namespace opentxs::ui::implementation

template class opentxs::SharedPimpl<opentxs::ui::ProfileSubsection>;

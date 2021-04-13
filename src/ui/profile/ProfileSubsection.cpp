// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                      // IWYU pragma: associated
#include "1_Internal.hpp"                    // IWYU pragma: associated
#include "ui/profile/ProfileSubsection.hpp"  // IWYU pragma: associated

#include <memory>
#include <set>
#include <thread>
#include <type_traits>

#include "internal/contact/Contact.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"
#include "ui/base/Combined.hpp"
#include "ui/base/Widget.hpp"

//#define OT_METHOD "opentxs::ui::implementation::ProfileSubsection::"

namespace opentxs::factory
{
auto ProfileSubsectionWidget(
    const ui::implementation::ProfileSectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ProfileSectionRowID& rowID,
    const ui::implementation::ProfileSectionSortKey& key,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ProfileSectionRowInternal>
{
    using ReturnType = ui::implementation::ProfileSubsection;

    return std::make_shared<ReturnType>(parent, api, rowID, key, custom);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
ProfileSubsection::ProfileSubsection(
    const ProfileSectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ProfileSectionRowID& rowID,
    const ProfileSectionSortKey& key,
    CustomData& custom) noexcept
    : Combined(api, parent.NymID(), parent.WidgetID(), parent, rowID, key)
    , sequence_(-1)
{
    init();
    startup_.reset(new std::thread(
        &ProfileSubsection::startup,
        this,
        extract_custom<opentxs::ContactGroup>(custom)));

    OT_ASSERT(startup_)
}

auto ProfileSubsection::AddItem(
    const std::string& value,
    const bool primary,
    const bool active) const noexcept -> bool
{
    return parent_.AddClaim(row_id_.second, value, primary, active);
}

auto ProfileSubsection::construct_row(
    const ProfileSubsectionRowID& id,
    const ProfileSubsectionSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::ProfileItemWidget(*this, api_, id, index, custom);
}

auto ProfileSubsection::Delete(const std::string& claimID) const noexcept
    -> bool
{
    rLock lock{recursive_lock_};
    auto& claim = lookup(lock, Identifier::Factory(claimID));

    if (false == claim.Valid()) { return false; }

    return claim.Delete();
}

auto ProfileSubsection::Name(const std::string& lang) const noexcept
    -> std::string
{
    return proto::TranslateItemType(
        contact::internal::translate(row_id_.second), lang);
}

auto ProfileSubsection::process_group(
    const opentxs::ContactGroup& group) noexcept
    -> std::set<ProfileSubsectionRowID>
{
    OT_ASSERT(row_id_.second == group.Type())

    std::set<ProfileSubsectionRowID> active{};

    for (const auto& [id, claim] : group) {
        OT_ASSERT(claim)

        CustomData custom{new opentxs::ContactItem(*claim)};
        add_item(id, ++sequence_, custom);
        active.emplace(id);
    }

    return active;
}

auto ProfileSubsection::reindex(
    const ProfileSectionSortKey&,
    CustomData& custom) noexcept -> bool
{
    delete_inactive(
        process_group(extract_custom<opentxs::ContactGroup>(custom)));

    return true;
}

auto ProfileSubsection::SetActive(const std::string& claimID, const bool active)
    const noexcept -> bool
{
    rLock lock{recursive_lock_};
    auto& claim = lookup(lock, Identifier::Factory(claimID));

    if (false == claim.Valid()) { return false; }

    return claim.SetActive(active);
}

auto ProfileSubsection::SetPrimary(
    const std::string& claimID,
    const bool primary) const noexcept -> bool
{
    rLock lock{recursive_lock_};
    auto& claim = lookup(lock, Identifier::Factory(claimID));

    if (false == claim.Valid()) { return false; }

    return claim.SetPrimary(primary);
}

auto ProfileSubsection::SetValue(
    const std::string& claimID,
    const std::string& value) const noexcept -> bool
{
    rLock lock{recursive_lock_};
    auto& claim = lookup(lock, Identifier::Factory(claimID));

    if (false == claim.Valid()) { return false; }

    return claim.SetValue(value);
}

void ProfileSubsection::startup(const opentxs::ContactGroup group) noexcept
{
    process_group(group);
    finish_startup();
}
}  // namespace opentxs::ui::implementation

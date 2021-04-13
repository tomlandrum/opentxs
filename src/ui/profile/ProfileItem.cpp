// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "ui/profile/ProfileItem.hpp"  // IWYU pragma: associated

#include <set>
#include <tuple>

#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "ui/base/Widget.hpp"

namespace opentxs::factory
{
auto ProfileItemWidget(
    const ui::implementation::ProfileSubsectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::ProfileSubsectionRowID& rowID,
    const ui::implementation::ProfileSubsectionSortKey& sortKey,
    ui::implementation::CustomData& custom) noexcept
    -> std::shared_ptr<ui::implementation::ProfileSubsectionRowInternal>
{
    using ReturnType = ui::implementation::ProfileItem;

    return std::make_shared<ReturnType>(parent, api, rowID, sortKey, custom);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
ProfileItem::ProfileItem(
    const ProfileSubsectionInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ProfileSubsectionRowID& rowID,
    const ProfileSubsectionSortKey& sortKey,
    CustomData& custom) noexcept
    : ProfileItemRow(parent, api, rowID, true)
    , item_{new opentxs::ContactItem(
          extract_custom<opentxs::ContactItem>(custom))}
{
}

auto ProfileItem::add_claim(const Claim& claim) const noexcept -> bool
{
    auto reason = api_.Factory().PasswordPrompt(__FUNCTION__);

    auto nym = api_.Wallet().mutable_Nym(parent_.NymID(), reason);

    return nym.AddClaim(claim, reason);
}

auto ProfileItem::as_claim() const noexcept -> Claim
{
    sLock lock(shared_lock_);
    Claim output{};
    auto& [id, section, type, value, start, end, attributes] = output;
    id = "";
    section = contact::internal::translate(parent_.Section());
    type = parent_.Type();
    value = item_->Value();
    start = item_->Start();
    end = item_->End();

    if (item_->isPrimary()) {
        attributes.emplace(contact::internal::translate(
            contact::ContactItemAttribute::Primary));
    }

    if (item_->isPrimary() || item_->isActive()) {
        attributes.emplace(contact::internal::translate(
            contact::ContactItemAttribute::Active));
    }

    return output;
}

auto ProfileItem::Delete() const noexcept -> bool
{
    auto reason = api_.Factory().PasswordPrompt(__FUNCTION__);

    auto nym = api_.Wallet().mutable_Nym(parent_.NymID(), reason);

    return nym.DeleteClaim(row_id_, reason);
}

auto ProfileItem::reindex(
    const ProfileSubsectionSortKey&,
    CustomData& custom) noexcept -> bool
{
    eLock lock(shared_lock_);
    item_.reset(
        new opentxs::ContactItem(extract_custom<opentxs::ContactItem>(custom)));

    OT_ASSERT(item_);

    return true;
}

auto ProfileItem::SetActive(const bool& active) const noexcept -> bool
{
    Claim claim{};
    auto& attributes = std::get<6>(claim);

    if (active) {
        attributes.emplace(contact::internal::translate(
            contact::ContactItemAttribute::Active));
    } else {
        attributes.erase(contact::internal::translate(
            contact::ContactItemAttribute::Active));
        attributes.erase(contact::internal::translate(
            contact::ContactItemAttribute::Primary));
    }

    return add_claim(claim);
}

auto ProfileItem::SetPrimary(const bool& primary) const noexcept -> bool
{
    Claim claim{};
    auto& attributes = std::get<6>(claim);

    if (primary) {
        attributes.emplace(contact::internal::translate(
            contact::ContactItemAttribute::Primary));
        attributes.emplace(contact::internal::translate(
            contact::ContactItemAttribute::Active));
    } else {
        attributes.erase(contact::internal::translate(
            contact::ContactItemAttribute::Primary));
    }

    return add_claim(claim);
}

auto ProfileItem::SetValue(const std::string& newValue) const noexcept -> bool
{
    Claim claim{};
    std::get<3>(claim) = newValue;

    return add_claim(claim);
}
}  // namespace opentxs::ui::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/MessagableListItem.cpp"

#pragma once

#include <string>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "ui/contactlist/ContactListItem.hpp"

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
class MessagableListItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs::ui::implementation
{
class MessagableListItem final : public implementation::ContactListItem
{
public:
    MessagableListItem(
        const ContactListInternalInterface& parent,
        const api::client::internal::Manager& api,
        const ContactListRowID& rowID,
        const ContactListSortKey& key) noexcept;
    ~MessagableListItem() final = default;

private:
    using ot_super = implementation::ContactListItem;

    auto calculate_section(const Lock& lock) const noexcept -> std::string final
    {
        return translate_section(lock);
    }

    MessagableListItem() = delete;
    MessagableListItem(const MessagableListItem&) = delete;
    MessagableListItem(MessagableListItem&&) = delete;
    auto operator=(const MessagableListItem&) -> MessagableListItem& = delete;
    auto operator=(MessagableListItem&&) -> MessagableListItem& = delete;
};
}  // namespace opentxs::ui::implementation

template class opentxs::SharedPimpl<opentxs::ui::MessagableListItem>;

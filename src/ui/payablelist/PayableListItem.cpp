// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "ui/payablelist/PayableListItem.hpp"  // IWYU pragma: associated

#if OT_QT
#include <QObject>
#endif  // OT_QT
#include <memory>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Log.hpp"

namespace opentxs::factory
{
auto PayableListItem(
    const ui::implementation::PayableInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::PayableListRowID& rowID,
    const ui::implementation::PayableListSortKey& key,
    const std::string& paymentcode,
    const contact::ContactItemType& currency) noexcept
    -> std::shared_ptr<ui::implementation::PayableListRowInternal>
{
    using ReturnType = ui::implementation::PayableListItem;

    return std::make_shared<ReturnType>(
        parent, api, rowID, key, paymentcode, currency);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
PayableListItem::PayableListItem(
    const PayableInternalInterface& parent,
    const api::client::internal::Manager& api,
    const PayableListRowID& rowID,
    const PayableListSortKey& key,
    const std::string& paymentcode,
    const contact::ContactItemType& currency) noexcept
    : ot_super(parent, api, rowID, key)
    , payment_code_(paymentcode)
    , currency_(currency)
{
}

auto PayableListItem::PaymentCode() const noexcept -> std::string
{
    Lock lock{lock_};

    return payment_code_;
}

#if OT_QT
QVariant PayableListItem::qt_data(const int column, int role) const noexcept
{
    if (0 == column) { return ContactListItem::qt_data(column, role); }

    switch (role) {
        case Qt::DisplayRole: {

            return PaymentCode().c_str();
        }
        default: {
            return {};
        }
    }
}
#endif

auto PayableListItem::reindex(
    const ContactListSortKey& key,
    CustomData& custom) noexcept -> bool
{
    Lock lock{lock_};

    return reindex(lock, key, custom);
}

auto PayableListItem::reindex(
    const Lock& lock,
    const ContactListSortKey& key,
    CustomData& custom) noexcept -> bool
{
    auto output = ot_super::reindex(lock, key, custom);

    const auto contact = api_.Contacts().Contact(row_id_);

    OT_ASSERT(contact);

    auto paymentCode = contact->PaymentCode(currency_);

    if (payment_code_ != paymentCode) {
        payment_code_ = paymentCode;
        output |= true;
    }

    return output;
}
}  // namespace opentxs::ui::implementation

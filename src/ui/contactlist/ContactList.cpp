// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                    // IWYU pragma: associated
#include "1_Internal.hpp"                  // IWYU pragma: associated
#include "ui/contactlist/ContactList.hpp"  // IWYU pragma: associated

#if OT_QT
#include <QString>
#endif  // OT_QT
#include <future>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#if OT_QT
#include "opentxs/ui/qt/ContactList.hpp"
#endif  // OT_QT
#include "ui/base/List.hpp"

#define OT_METHOD "opentxs::ui::implementation::ContactList::"

namespace opentxs::factory
{
auto ContactListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::ContactList>
{
    using ReturnType = ui::implementation::ContactList;

    return std::make_unique<ReturnType>(api, nymID, cb);
}

#if OT_QT
auto ContactListQtModel(ui::implementation::ContactList& parent) noexcept
    -> std::unique_ptr<ui::ContactListQt>
{
    using ReturnType = ui::ContactListQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(ContactListQt, implementation::ContactList)

QString ContactListQt::addContact(
    const QString& label,
    const QString& paymentCode,
    const QString& nymID) const noexcept
{
    return parent_
        .AddContact(
            label.toStdString(), paymentCode.toStdString(), nymID.toStdString())
        .c_str();
}
}  // namespace opentxs::ui
#endif

namespace opentxs::ui::implementation
{
ContactList::ContactList(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    : ContactListList(
          api,
          nymID,
          cb,
          false
#if OT_QT
          ,
          Roles{
              {ContactListQt::ContactIDRole, "id"},
              {ContactListQt::SectionRole, "section"}},
          1,
          1
#endif
          )
    , Worker(api, {})
    , owner_contact_id_(Widget::api_.Contacts().ContactID(nymID))
    , owner_(factory::ContactListItem(*this, api, owner_contact_id_, "Owner"))
{
    OT_ASSERT(false == owner_contact_id_->empty());
    OT_ASSERT(owner_);

#if OT_QT
    register_child(owner_.get());
#endif  // OT_QT
    init();
    init_executor({api.Endpoints().ContactUpdate()});
    pipeline_->Push(MakeWork(Work::init));
}

ContactList::ParsedArgs::ParsedArgs(
    const api::internal::Core& api,
    const std::string& purportedID,
    const std::string& purportedPaymentCode) noexcept
    : nym_id_(extract_nymid(api, purportedID, purportedPaymentCode))
    , payment_code_(extract_paymentcode(api, purportedID, purportedPaymentCode))
{
}

auto ContactList::ParsedArgs::extract_nymid(
    const api::internal::Core& api,
    const std::string& purportedID,
    const std::string& purportedPaymentCode) noexcept -> OTNymID
{
    auto output = api.Factory().NymID();

    if (false == purportedID.empty()) {
        // Case 1: purportedID is a nym id
        output = api.Factory().NymID(purportedID);

        if (false == output->empty()) { return output; }

        // Case 2: purportedID is a payment code
        output = api.Factory().NymIDFromPaymentCode(purportedID);

        if (false == output->empty()) { return output; }
    }

    if (false == purportedPaymentCode.empty()) {
        // Case 3: purportedPaymentCode is a payment code
        output = api.Factory().NymIDFromPaymentCode(purportedPaymentCode);

        if (false == output->empty()) { return output; }

        // Case 4: purportedPaymentCode is a nym id
        output->SetString(purportedPaymentCode);

        if (false == output->empty()) { return output; }
    }

    // Case 5: not possible to extract a nym id

    return output;
}

auto ContactList::ParsedArgs::extract_paymentcode(
    const api::internal::Core& api,
    const std::string& purportedID,
    const std::string& purportedPaymentCode) noexcept -> OTPaymentCode
{
    if (false == purportedPaymentCode.empty()) {
        // Case 1: purportedPaymentCode is a payment code
        auto output = api.Factory().PaymentCode(purportedPaymentCode);

        if (output->Valid()) { return output; }
    }

    if (false == purportedID.empty()) {
        // Case 2: purportedID is a payment code
        auto output = api.Factory().PaymentCode(purportedID);

        if (output->Valid()) { return output; }
    }

    // Case 3: not possible to extract a payment code

    return api.Factory().PaymentCode(std::string{});
}

auto ContactList::AddContact(
    const std::string& label,
    const std::string& paymentCode,
    const std::string& nymID) const noexcept -> std::string
{
    auto args = ParsedArgs{Widget::api_, nymID, paymentCode};
    const auto contact = Widget::api_.Contacts().NewContact(
        label, args.nym_id_, args.payment_code_);
    const auto& id = contact->ID();
    Widget::api_.OTX().CanMessage(primary_id_, id, true);

    return id.str();
}

auto ContactList::add_item(
    const ContactListRowID& id,
    const ContactListSortKey& index,
    CustomData& custom) noexcept -> void
{
    if (owner_contact_id_ == id) {
        if (owner_->reindex(index, custom)) { row_modified(0, owner_.get()); }

        return;
    }

    ContactListList::add_item(id, index, custom);
}

auto ContactList::construct_row(
    const ContactListRowID& id,
    const ContactListSortKey& index,
    CustomData&) const noexcept -> RowPointer
{
    return factory::ContactListItem(*this, Widget::api_, id, index);
}

#if OT_QT
auto ContactList::find_row(const ContactListRowID& id) const noexcept -> int
{
    if (owner_contact_id_ == id) { return 0; }

    const auto output = ContactListList::find_row(id);

    if (-1 == output) { return output; }

    return output + 1;
}
#endif

auto ContactList::first(const rLock&) const noexcept
    -> SharedPimpl<ContactListRowInterface>
{
    return SharedPimpl<ContactListRowInterface>(owner_);
}

#if OT_QT
auto ContactList::index(int row, int column, const QModelIndex& parent)
    const noexcept -> QModelIndex
{
    const auto* pointer = get_pointer(parent);

    if (nullptr == parent.internalPointer()) {
        if (0 == row) {
            return createIndex(row, column, owner_.get());
        } else {
            return get_index(row, column);
        }
    } else {
        return pointer->qt_index(row, column);
    }
}
#endif

auto ContactList::last(const ContactListRowID& id) const noexcept -> bool
{
    if (owner_contact_id_ == id) {

        return 0 == size();
    } else {

        return ContactListList::last(id);
    }
}

auto ContactList::lookup(const rLock& lock, const ContactListRowID& id)
    const noexcept -> const ContactListRowInternal&
{
    if (owner_contact_id_ == id) {

        return *owner_;
    } else {

        return ContactListList::lookup(lock, id);
    }
}

auto ContactList::pipeline(const Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    if (1 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

        OT_FAIL;
    }

    const auto work = [&] {
        try {

            return body.at(0).as<Work>();
        } catch (...) {

            OT_FAIL;
        }
    }();

    switch (work) {
        case Work::contact: {
            process_contact(in);
        } break;
        case Work::init: {
            startup();
        } break;
        case Work::statemachine: {
            do_work();
        } break;
        case Work::shutdown: {
            running_->Off();
            shutdown(shutdown_promise_);
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unhandled type").Flush();

            OT_FAIL;
        }
    }
}

auto ContactList::process_contact(const Message& in) noexcept -> void
{
    const auto body = in.Body();

    OT_ASSERT(1 < body.size());

    const auto& id = body.at(1);
    auto contactID = Widget::api_.Factory().Identifier();
    contactID->Assign(id.Bytes());

    OT_ASSERT(false == contactID->empty())

    const auto name = Widget::api_.Contacts().ContactName(contactID);
    auto custom = CustomData{};
    add_item(contactID, name, custom);
}

auto ContactList::row_modified(const Lock&, const ContactListRowID& id) noexcept
    -> void
{
    if (id == owner_contact_id_) {
        row_modified(0, owner_.get());
    } else {
        rLock lock{recursive_lock_};
        const auto index = find_index(id);

        if (false == index.has_value()) { return; }

        auto& row = const_cast<ContactListRowInternal&>(
            ContactListList::lookup(lock, id));
        row_modified(index.value() + 1, &row);
    }
}

auto ContactList::startup() noexcept -> void
{
    const auto contacts = Widget::api_.Contacts().ContactList();
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Loading ")(contacts.size())(
        " contacts.")
        .Flush();

    for (const auto& [id, alias] : contacts) {
        auto custom = CustomData{};
        add_item(Widget::api_.Factory().Identifier(id), alias, custom);
    }

    finish_startup();
}

ContactList::~ContactList()
{
    wait_for_startup();
    stop_worker().get();
}
}  // namespace opentxs::ui::implementation

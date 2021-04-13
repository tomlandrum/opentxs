// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                    // IWYU pragma: associated
#include "1_Internal.hpp"                  // IWYU pragma: associated
#include "ui/payablelist/PayableList.hpp"  // IWYU pragma: associated

#include <future>
#include <list>
#include <memory>
#include <string>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#if OT_QT
#include "opentxs/ui/qt/PayableList.hpp"
#endif  // OT_QT
#include "ui/base/List.hpp"

#define OT_METHOD "opentxs::ui::implementation::PayableList::"

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(PayableListQt, implementation::PayableList)
}  // namespace opentxs::ui
#endif

namespace opentxs::factory
{
auto PayableListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const contact::ContactItemType& currency,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::PayableList>
{
    using ReturnType = ui::implementation::PayableList;

    return std::make_unique<ReturnType>(api, nymID, currency, cb);
}

#if OT_QT
auto PayableListQtModel(ui::implementation::PayableList& parent) noexcept
    -> std::unique_ptr<ui::PayableListQt>
{
    using ReturnType = ui::PayableListQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
PayableList::PayableList(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const contact::ContactItemType& currency,
    const SimpleCallback& cb) noexcept
    : PayableListList(
          api,
          nymID,
          cb,
          false
#if OT_QT
          ,
          Roles{
              {PayableListQt::ContactIDRole, "id"},
              {PayableListQt::SectionRole, "section"}},
          2
#endif
          )
    , Worker(api, {})
    , owner_contact_id_(Widget::api_.Factory().Identifier())  // FIXME wtf
    , currency_(currency)
{
    init();
    init_executor(
        {api.Endpoints().ContactUpdate(), api.Endpoints().NymDownload()});
    pipeline_->Push(MakeWork(Work::init));
}

auto PayableList::construct_row(
    const PayableListRowID& id,
    const PayableListSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::PayableListItem(
        *this,
        Widget::api_,
        id,
        index,
        extract_custom<const std::string>(custom, 0),
        currency_);
}

auto PayableList::pipeline(const Message& in) noexcept -> void
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
        case Work::nym: {
            process_nym(in);
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

auto PayableList::process_contact(
    const PayableListRowID& id,
    const PayableListSortKey& key) noexcept -> void
{
    if (owner_contact_id_ == id) { return; }

    const auto contact = Widget::api_.Contacts().Contact(id);

    if (false == bool(contact)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: Contact ")(id)(
            " can not be loaded.")
            .Flush();

        return;
    }

    OT_ASSERT(contact);

    auto paymentCode =
        std::make_unique<std::string>(contact->PaymentCode(currency_));

    OT_ASSERT(paymentCode);

    if (false == paymentCode->empty()) {
        auto custom = CustomData{paymentCode.release()};
        add_item(id, key, custom);
    } else {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Skipping unpayable contact ")(id)
            .Flush();
    }
}

auto PayableList::process_contact(const Message& message) noexcept -> void
{
    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto& id = body.at(1);
    auto contactID = Widget::api_.Factory().Identifier();
    contactID->Assign(id.Bytes());

    OT_ASSERT(false == contactID->empty())

    const auto name = Widget::api_.Contacts().ContactName(contactID);
    process_contact(contactID, name);
}

auto PayableList::process_nym(const Message& message) noexcept -> void
{
    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto& id = body.at(1);
    auto nymID = Widget::api_.Factory().NymID();
    nymID->Assign(id.Bytes());

    OT_ASSERT(false == nymID->empty())

    const auto contactID = Widget::api_.Contacts().ContactID(nymID);
    const auto name = Widget::api_.Contacts().ContactName(contactID);
    process_contact(contactID, name);
}

auto PayableList::startup() noexcept -> void
{
    const auto contacts = Widget::api_.Contacts().ContactList();
    LogDetail(OT_METHOD)(__FUNCTION__)(": Loading ")(contacts.size())(
        " contacts.")
        .Flush();

    for (const auto& [id, alias] : contacts) {
        process_contact(Identifier::Factory(id), alias);
    }

    finish_startup();
}

PayableList::~PayableList()
{
    wait_for_startup();
    stop_worker().get();
}
}  // namespace opentxs::ui::implementation

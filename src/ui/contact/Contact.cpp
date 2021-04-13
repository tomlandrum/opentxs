// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "ui/contact/Contact.hpp"  // IWYU pragma: associated

#if OT_QT
#include <QString>
#endif  // OT_QT
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <utility>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactSection.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#if OT_QT
#include "opentxs/ui/qt/Contact.hpp"
#endif  // OT_QT
#include "ui/base/List.hpp"

#define OT_METHOD "opentxs::ui::implementation::Contact::"

namespace opentxs::factory
{
auto ContactModel(
    const api::client::internal::Manager& api,
    const Identifier& contactID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::Contact>
{
    using ReturnType = ui::implementation::Contact;

    return std::make_unique<ReturnType>(api, contactID, cb);
}

#if OT_QT
auto ContactQtModel(ui::implementation::Contact& parent) noexcept
    -> std::unique_ptr<ui::ContactQt>
{
    using ReturnType = ui::ContactQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(ContactQt, implementation::Contact)

QString ContactQt::displayName() const noexcept
{
    return parent_.DisplayName().c_str();
}
QString ContactQt::contactID() const noexcept
{
    return parent_.ContactID().c_str();
}
QString ContactQt::paymentCode() const noexcept
{
    return parent_.PaymentCode().c_str();
}
}  // namespace opentxs::ui
#endif

namespace opentxs::ui::implementation
{
const std::set<contact::ContactSectionName> Contact::allowed_types_{
    contact::ContactSectionName::Communication,
    contact::ContactSectionName::Profile};

const std::map<contact::ContactSectionName, int> Contact::sort_keys_{
    {contact::ContactSectionName::Communication, 0},
    {contact::ContactSectionName::Profile, 1}};

Contact::Contact(
    const api::client::internal::Manager& api,
    const Identifier& contactID,
    const SimpleCallback& cb) noexcept
    : ContactType(api, contactID, cb)
    , listeners_({
          {api_.Endpoints().ContactUpdate(),
           new MessageProcessor<Contact>(&Contact::process_contact)},
      })
    , name_(api_.Contacts().ContactName(contactID))
    , payment_code_()
{
    // NOTE nym_id_ is actually the contact id
    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&Contact::startup, this));

    OT_ASSERT(startup_)
}

auto Contact::check_type(const contact::ContactSectionName type) noexcept
    -> bool
{
    return 1 == allowed_types_.count(type);
}

auto Contact::construct_row(
    const ContactRowID& id,
    const ContactSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::ContactSectionWidget(*this, api_, id, index, custom);
}

auto Contact::ContactID() const noexcept -> std::string
{
    return primary_id_->str();
}

auto Contact::DisplayName() const noexcept -> std::string
{
    rLock lock{recursive_lock_};

    return name_;
}

auto Contact::PaymentCode() const noexcept -> std::string
{
    rLock lock{recursive_lock_};

    return payment_code_;
}

void Contact::process_contact(const opentxs::Contact& contact) noexcept
{
    {
        rLock lock{recursive_lock_};
        name_ = contact.Label();
        payment_code_ = contact.PaymentCode();
    }

    UpdateNotify();
    auto active = std::set<ContactRowID>{};
    const auto data = contact.Data();

    if (data) {
        for (const auto& section : *data) {
            auto& type = section.first;

            if (check_type(type)) {
                auto custom =
                    CustomData{new opentxs::ContactSection(*section.second)};
                add_item(type, sort_key(type), custom);
                active.emplace(type);
            }
        }
    }

    delete_inactive(active);
}

void Contact::process_contact(const Message& message) noexcept
{
    wait_for_startup();

    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto& id = body.at(1);
    auto contactID = Widget::api_.Factory().Identifier();
    contactID->Assign(id.Bytes());

    OT_ASSERT(false == contactID->empty())

    if (contactID != primary_id_) { return; }

    const auto contact = api_.Contacts().Contact(contactID);

    OT_ASSERT(contact)

    process_contact(*contact);
}

auto Contact::sort_key(const contact::ContactSectionName type) noexcept -> int
{
    return sort_keys_.at(type);
}

void Contact::startup() noexcept
{
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Loading contact ")(primary_id_)
        .Flush();
    const auto contact = api_.Contacts().Contact(primary_id_);

    OT_ASSERT(contact)

    process_contact(*contact);
    finish_startup();
}

Contact::~Contact()
{
    for (auto& it : listeners_) { delete it.second; }
}
}  // namespace opentxs::ui::implementation

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/Contact.cpp"

#pragma once

#if OT_QT
#include <QHash>
#endif  // OT_QT
#include <map>
#include <set>
#include <string>
#include <utility>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "ui/base/List.hpp"
#include "ui/base/Widget.hpp"

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

class Message;
}  // namespace zeromq
}  // namespace network

class Contact;
class Identifier;
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using ContactType = List<
    ContactExternalInterface,
    ContactInternalInterface,
    ContactRowID,
    ContactRowInterface,
    ContactRowInternal,
    ContactRowBlank,
    ContactSortKey,
    ContactPrimaryID>;

class Contact final : public ContactType
{
public:
    auto ContactID() const noexcept -> std::string final;
    auto DisplayName() const noexcept -> std::string final;
#if OT_QT
    int FindRow(const ContactRowID& id) const noexcept final
    {
        return find_row(id);
    }
#endif
    auto PaymentCode() const noexcept -> std::string final;

    Contact(
        const api::client::internal::Manager& api,
        const Identifier& contactID,
        const SimpleCallback& cb) noexcept;
    ~Contact() final;

private:
    static const std::set<contact::ContactSectionName> allowed_types_;
    static const std::map<contact::ContactSectionName, int> sort_keys_;

    const ListenerDefinitions listeners_;
    std::string name_;
    std::string payment_code_;

    static auto sort_key(const contact::ContactSectionName type) noexcept
        -> int;
    static auto check_type(const contact::ContactSectionName type) noexcept
        -> bool;

    auto construct_row(
        const ContactRowID& id,
        const ContactSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;

    auto last(const ContactRowID& id) const noexcept -> bool final
    {
        return ContactType::last(id);
    }
    void update(ContactRowInterface& row, CustomData& custom) const noexcept;

    void process_contact(const opentxs::Contact& contact) noexcept;
    void process_contact(const Message& message) noexcept;
    void startup() noexcept;

    Contact() = delete;
    Contact(const Contact&) = delete;
    Contact(Contact&&) = delete;
    auto operator=(const Contact&) -> Contact& = delete;
    auto operator=(Contact&&) -> Contact& = delete;
};
}  // namespace opentxs::ui::implementation

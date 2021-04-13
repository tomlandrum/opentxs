// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_CONTACTS_HPP
#define OPENTXS_API_CLIENT_CONTACTS_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/identity/Nym.hpp"

namespace opentxs
{
class Contact;
class Identifier;
class PaymentCode;
}  // namespace opentxs

namespace opentxs
{
namespace api
{
namespace client
{
class Contacts
{
public:
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact> Contact(
        const Identifier& id) const = 0;
    /** Returns the contact ID for a nym, if it exists */
    OPENTXS_EXPORT virtual OTIdentifier ContactID(
        const identifier::Nym& nymID) const = 0;
    OPENTXS_EXPORT virtual ObjectList ContactList() const = 0;
    OPENTXS_EXPORT virtual std::string ContactName(
        const Identifier& contactID) const = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact> Merge(
        const Identifier& parent,
        const Identifier& child) const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<Editor<opentxs::Contact>>
    mutable_Contact(const Identifier& id) const = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact> NewContact(
        const std::string& label) const = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact> NewContact(
        const std::string& label,
        const identifier::Nym& nymID,
        const PaymentCode& paymentCode) const = 0;
#if OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact>
    NewContactFromAddress(
        const std::string& address,
        const std::string& label,
        const contact::ContactItemType currency =
            contact::ContactItemType::BTC) const = 0;
#endif  // OT_BLOCKCHAIN
    /** Returns an existing contact ID if it exists, or creates a new one */
    OPENTXS_EXPORT virtual OTIdentifier NymToContact(
        const identifier::Nym& nymID) const = 0;
    /** Returns an existing contact ID if it exists, or creates a new one */
    OPENTXS_EXPORT virtual OTIdentifier PaymentCodeToContact(
        const PaymentCode& code) const = 0;
    OPENTXS_EXPORT virtual OTIdentifier PaymentCodeToContact(
        const std::string& code) const = 0;
    OPENTXS_EXPORT virtual std::shared_ptr<const opentxs::Contact> Update(
        const identity::Nym::Serialized& nym) const = 0;

    virtual ~Contacts() = default;

protected:
    Contacts() = default;

private:
    Contacts(const Contacts&) = delete;
    Contacts(Contacts&&) = delete;
    Contacts& operator=(const Contacts&) = delete;
    Contacts& operator=(Contacts&&) = delete;
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONTACT_CONTACT_HPP
#define OPENTXS_CONTACT_CONTACT_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"
// IWYU pragma: no_include "opentxs/blockchain/BlockchainType.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <atomic>
#include <cstdint>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"

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

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

class ContactData;
class ContactGroup;
class ContactItem;
class Identifier;
class PaymentCode;

namespace proto
{
class Contact;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
class Contact
{
public:
    using AddressStyle = api::client::blockchain::AddressStyle;
    using BlockchainType = blockchain::Type;
    using BlockchainAddress = std::tuple<OTData, AddressStyle, BlockchainType>;

    OPENTXS_EXPORT static std::shared_ptr<ContactItem> Best(
        const ContactGroup& group);
    OPENTXS_EXPORT static std::string ExtractLabel(const identity::Nym& nym);
    OPENTXS_EXPORT static contact::ContactItemType ExtractType(
        const identity::Nym& nym);
    OPENTXS_EXPORT static std::string PaymentCode(
        const ContactData& data,
        const contact::ContactItemType currency);

    OPENTXS_EXPORT Contact(
        const api::client::internal::Manager& api,
        const proto::Contact& serialized);
    OPENTXS_EXPORT Contact(
        const api::client::internal::Manager& api,
        const std::string& label);

    OPENTXS_EXPORT operator proto::Contact() const;
    OPENTXS_EXPORT Contact& operator+=(Contact& rhs);

    OPENTXS_EXPORT std::string BestEmail() const;
    OPENTXS_EXPORT std::string BestPhoneNumber() const;
    OPENTXS_EXPORT std::string BestSocialMediaProfile(
        const contact::ContactItemType type) const;
    OPENTXS_EXPORT std::vector<BlockchainAddress> BlockchainAddresses() const;
    OPENTXS_EXPORT std::shared_ptr<ContactData> Data() const;
    OPENTXS_EXPORT std::string EmailAddresses(bool active = true) const;
    OPENTXS_EXPORT const Identifier& ID() const;
    OPENTXS_EXPORT const std::string& Label() const;
    OPENTXS_EXPORT std::time_t LastUpdated() const;
    OPENTXS_EXPORT std::vector<OTNymID> Nyms(
        const bool includeInactive = false) const;
    OPENTXS_EXPORT std::string PaymentCode(
        const contact::ContactItemType currency =
            contact::ContactItemType::BTC) const;
    OPENTXS_EXPORT std::vector<std::string> PaymentCodes(
        const contact::ContactItemType currency =
            contact::ContactItemType::BTC) const;
    OPENTXS_EXPORT std::string PhoneNumbers(bool active = true) const;
    OPENTXS_EXPORT std::string Print() const;
    OPENTXS_EXPORT std::string SocialMediaProfiles(
        const contact::ContactItemType type,
        bool active = true) const;
    OPENTXS_EXPORT const std::set<contact::ContactItemType>
    SocialMediaProfileTypes() const;
    OPENTXS_EXPORT contact::ContactItemType Type() const;

    OPENTXS_EXPORT bool AddBlockchainAddress(
        const std::string& address,
        const contact::ContactItemType currency =
            contact::ContactItemType::Unknown);
    OPENTXS_EXPORT bool AddBlockchainAddress(
        const api::client::blockchain::AddressStyle& style,
        const blockchain::Type chain,
        const opentxs::Data& bytes);
    OPENTXS_EXPORT bool AddEmail(
        const std::string& value,
        const bool primary,
        const bool active);
    OPENTXS_EXPORT bool AddNym(const Nym_p& nym, const bool primary);
    OPENTXS_EXPORT bool AddNym(
        const identifier::Nym& nymID,
        const bool primary);
    OPENTXS_EXPORT bool AddPaymentCode(
        const opentxs::PaymentCode& code,
        const bool primary,
        const contact::ContactItemType currency = contact::ContactItemType::BTC,
        const bool active = true);
    OPENTXS_EXPORT bool AddPhoneNumber(
        const std::string& value,
        const bool primary,
        const bool active);
    OPENTXS_EXPORT bool AddSocialMediaProfile(
        const std::string& value,
        const contact::ContactItemType type,
        const bool primary,
        const bool active);
    OPENTXS_EXPORT bool RemoveNym(const identifier::Nym& nymID);
    OPENTXS_EXPORT void SetLabel(const std::string& label);
    OPENTXS_EXPORT void Update(const identity::Nym::Serialized& nym);

    OPENTXS_EXPORT ~Contact();

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;

    Contact() = delete;
    Contact(const Contact&) = delete;
    Contact(Contact&&) = delete;
    Contact& operator=(const Contact&) = delete;
    Contact& operator=(Contact&&) = delete;
};
}  // namespace opentxs
#endif

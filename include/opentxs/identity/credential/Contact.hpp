// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_CREDENTIAL_CONTACT_HPP
#define OPENTXS_IDENTITY_CREDENTIAL_CONTACT_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/identity/credential/Base.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api
namespace proto

{
class Claim;
class ContactItem;
}  // namespace proto
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
namespace credential
{
class Contact : virtual public Base
{
public:
    OPENTXS_EXPORT static std::string ClaimID(
        const api::internal::Core& api,
        const std::string& nymid,
        const std::uint32_t section,
        const proto::ContactItem& item);
    OPENTXS_EXPORT static std::string ClaimID(
        const api::internal::Core& api,
        const std::string& nymid,
        const contact::ContactSectionName section,
        const contact::ContactItemType type,
        const std::int64_t start,
        const std::int64_t end,
        const std::string& value,
        const std::string& subtype);
    OPENTXS_EXPORT static OTIdentifier ClaimID(
        const api::internal::Core& api,
        const proto::Claim& preimage);
    OPENTXS_EXPORT static Claim asClaim(
        const api::internal::Core& api,
        const String& nymid,
        const std::uint32_t section,
        const proto::ContactItem& item);

    OPENTXS_EXPORT ~Contact() override = default;

protected:
    Contact() noexcept {}  // TODO Signable

private:
    Contact(const Contact&) = delete;
    Contact(Contact&&) = delete;
    Contact& operator=(const Contact&) = delete;
    Contact& operator=(Contact&&) = delete;
};
}  // namespace credential
}  // namespace identity
}  // namespace opentxs
#endif

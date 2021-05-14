// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_WOT_VERIFICATION_NYM_HPP
#define OPENTXS_IDENTITY_WOT_VERIFICATION_NYM_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include "opentxs/Types.hpp"
#include "opentxs/identity/wot/verification/Item.hpp"
#include "opentxs/iterator/Bidirectional.hpp"

namespace opentxs
{
namespace proto
{
class VerificationIdentity;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
namespace identity
{
namespace wot
{
namespace verification
{
class OPENTXS_EXPORT Nym
{
public:
    using value_type = Item;
    using const_iterator =
        opentxs::iterator::Bidirectional<const Nym, const value_type>;
    using SerializedType = proto::VerificationIdentity;

    static const VersionNumber DefaultVersion;

    OPENTXS_NO_EXPORT virtual operator SerializedType() const noexcept = 0;

    /// Throws std::out_of_range for invalid position
    virtual const value_type& at(const std::size_t position) const
        noexcept(false) = 0;
    virtual const_iterator begin() const noexcept = 0;
    virtual const_iterator cbegin() const noexcept = 0;
    virtual const_iterator cend() const noexcept = 0;
    virtual const_iterator end() const noexcept = 0;
    virtual const identifier::Nym& ID() const noexcept = 0;
    virtual std::size_t size() const noexcept = 0;
    virtual VersionNumber Version() const noexcept = 0;

    virtual bool AddItem(
        const Identifier& claim,
        const identity::Nym& signer,
        const PasswordPrompt& reason,
        const Item::Type value = Item::Type::Confirm,
        const Time start = {},
        const Time end = {},
        const VersionNumber version = Item::DefaultVersion) noexcept = 0;
    virtual bool DeleteItem(const Identifier& item) noexcept = 0;

    virtual ~Nym() = default;

protected:
    Nym() = default;

private:
    Nym(const Nym&) = delete;
    Nym(Nym&&) = delete;
    Nym& operator=(const Nym&) = delete;
    Nym& operator=(Nym&&) = delete;
};
}  // namespace verification
}  // namespace wot
}  // namespace identity
}  // namespace opentxs
#endif

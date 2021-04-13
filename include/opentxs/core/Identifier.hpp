// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_IDENTIFIER_HPP
#define OPENTXS_CORE_IDENTIFIER_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Data.hpp"

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::Identifier>::Pimpl(opentxs::Identifier const &);
%ignore opentxs::Pimpl<opentxs::Identifier>::operator opentxs::Identifier&;
%ignore opentxs::Pimpl<opentxs::Identifier>::operator const opentxs::Identifier &;
%rename(identifierCompareEqual) opentxs::Identifier::operator==(const Identifier& rhs) const;
%rename(identifierCompareNotEqual) opentxs::Identifier::operator!=(const Identifier& rhs) const;
%rename(identifierCompareGreaterThan) opentxs::Identifier::operator>(const Identifier& rhs) const;
%rename(identifierCompareLessThan) opentxs::Identifier::operator<(const Identifier& rhs) const;
%rename(identifierCompareGreaterOrEqual) opentxs::Identifier::operator>=(const Identifier& rhs) const;
%rename(identifierCompareLessOrEqual) opentxs::Identifier::operator<=(const Identifier& rhs) const;
%rename (IdentifierFactory) opentxs::Identifier::Factory;
%template(OTIdentifier) opentxs::Pimpl<opentxs::Identifier>;
// clang-format on
#endif

namespace opentxs
{
namespace proto
{
class HDPath;
}  // namespace proto

class Cheque;
class Contract;
class Identifier;
class Item;

using OTIdentifier = Pimpl<Identifier>;

#ifndef SWIG
OPENTXS_EXPORT bool operator==(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
OPENTXS_EXPORT bool operator!=(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
OPENTXS_EXPORT bool operator<(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
OPENTXS_EXPORT bool operator>(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
OPENTXS_EXPORT bool operator<=(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
OPENTXS_EXPORT bool operator>=(
    const opentxs::Pimpl<opentxs::Identifier>& lhs,
    const opentxs::Identifier& rhs) noexcept;
#endif
}  // namespace opentxs

namespace opentxs
{
/** An Identifier is basically a 256 bit hash value. This class makes it easy to
 * convert IDs back and forth to strings. */
class Identifier : virtual public Data
{
public:
    using ot_super = opentxs::Data;

    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Random();
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory();
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const Identifier& rhs);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const std::string& rhs);
#ifndef SWIG
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const String& rhs);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const identity::Nym& nym);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const Cheque& cheque);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const Item& item);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const Contract& contract);
    OPENTXS_EXPORT static opentxs::Pimpl<opentxs::Identifier> Factory(
        const contact::ContactItemType type,
        const proto::HDPath& path);
#endif
    OPENTXS_EXPORT static bool Validate(const std::string& id);

    using ot_super::operator==;
    OPENTXS_EXPORT virtual bool operator==(
        const Identifier& rhs) const noexcept = 0;
    using ot_super::operator!=;
    OPENTXS_EXPORT virtual bool operator!=(
        const Identifier& rhs) const noexcept = 0;
    using ot_super::operator>;
    OPENTXS_EXPORT virtual bool operator>(
        const Identifier& rhs) const noexcept = 0;
    using ot_super::operator<;
    OPENTXS_EXPORT virtual bool operator<(
        const Identifier& rhs) const noexcept = 0;
    using ot_super::operator<=;
    OPENTXS_EXPORT virtual bool operator<=(
        const Identifier& rhs) const noexcept = 0;
    using ot_super::operator>=;
    OPENTXS_EXPORT virtual bool operator>=(
        const Identifier& rhs) const noexcept = 0;

#ifndef SWIG
    OPENTXS_EXPORT virtual void GetString(String& theStr) const = 0;
#endif
    OPENTXS_EXPORT virtual const ID& Type() const = 0;

    OPENTXS_EXPORT virtual bool CalculateDigest(
        const ReadView bytes,
        const ID type = ID::blake2b) = 0;
    OPENTXS_EXPORT virtual void SetString(const std::string& encoded) = 0;
#ifndef SWIG
    OPENTXS_EXPORT virtual void SetString(const String& encoded) = 0;
#endif
    using ot_super::swap;
    OPENTXS_EXPORT virtual void swap(Identifier& rhs) = 0;

    OPENTXS_EXPORT ~Identifier() override = default;

protected:
    Identifier() = default;

private:
    friend opentxs::Pimpl<opentxs::Identifier>;

#ifndef _WIN32
    Identifier* clone() const override = 0;
#endif
    Identifier(const Identifier&) = delete;
    Identifier(Identifier&&) = delete;
    Identifier& operator=(const Identifier&) = delete;
    Identifier& operator=(Identifier&&) = delete;
};
}  // namespace opentxs

#ifndef SWIG
namespace std
{
template <>
struct less<opentxs::OTIdentifier> {
    OPENTXS_EXPORT bool operator()(
        const opentxs::OTIdentifier& lhs,
        const opentxs::OTIdentifier& rhs) const;
};
}  // namespace std
#endif
#endif

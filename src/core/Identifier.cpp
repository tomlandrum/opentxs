// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "core/Identifier.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <type_traits>
#include <utility>

#include "core/Data.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/HDPath.pb.h"

template class opentxs::Pimpl<opentxs::Identifier>;
template class std::set<opentxs::OTIdentifier>;
template class std::map<opentxs::OTIdentifier, std::set<opentxs::OTIdentifier>>;

namespace std
{
auto less<opentxs::Pimpl<opentxs::Identifier>>::operator()(
    const opentxs::OTIdentifier& lhs,
    const opentxs::OTIdentifier& rhs) const -> bool
{
    return lhs.get() < rhs.get();
}
}  // namespace std

namespace opentxs
{
auto operator==(const OTIdentifier& lhs, const Identifier& rhs) noexcept -> bool
{
    return lhs.get().operator==(rhs);
}

auto operator==(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator==(rhs);
}

auto operator==(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator==(rhs);
}

auto operator==(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator==(rhs);
}

auto operator!=(const OTIdentifier& lhs, const Identifier& rhs) noexcept -> bool
{
    return lhs.get().operator!=(rhs);
}

auto operator!=(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator!=(rhs);
}

auto operator!=(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator!=(rhs);
}

auto operator!=(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator!=(rhs);
}

auto operator<(const OTIdentifier& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<(rhs);
}

auto operator<(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<(rhs);
}

auto operator<(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<(rhs);
}

auto operator<(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<(rhs);
}

auto operator>(const OTIdentifier& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>(rhs);
}

auto operator>(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>(rhs);
}

auto operator>(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>(rhs);
}

auto operator>(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>(rhs);
}

auto operator<=(
    const OTIdentifier& lhs,
    const opentxs::Identifier& rhs) noexcept -> bool
{
    return lhs.get().operator<=(rhs);
}

auto operator<=(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<=(rhs);
}

auto operator<=(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<=(rhs);
}

auto operator<=(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator<=(rhs);
}

auto operator>=(
    const OTIdentifier& lhs,
    const opentxs::Identifier& rhs) noexcept -> bool
{
    return lhs.get().operator>=(rhs);
}

auto operator>=(const OTNymID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>=(rhs);
}

auto operator>=(const OTServerID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>=(rhs);
}

auto operator>=(const OTUnitID& lhs, const opentxs::Identifier& rhs) noexcept
    -> bool
{
    return lhs.get().operator>=(rhs);
}

auto Identifier::Factory() -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier());
}

auto identifier::Nym::Factory() -> OTNymID
{
    return OTNymID(new implementation::Identifier());
}

auto identifier::Server::Factory() -> OTServerID
{
    return OTServerID(new implementation::Identifier());
}

auto identifier::UnitDefinition::Factory() -> OTUnitID
{
    return OTUnitID(new implementation::Identifier());
}

auto Identifier::Factory(const Identifier& rhs) -> OTIdentifier
{
    return OTIdentifier(
#ifndef _WIN32
        rhs.clone()
#else
        dynamic_cast<Identifier*>(rhs.clone())
#endif
    );
}

auto Identifier::Factory(const std::string& rhs) -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier(rhs));
}

auto identifier::Nym::Factory(const std::string& rhs) -> OTNymID
{
    return OTNymID(new implementation::Identifier(rhs));
}

auto identifier::Server::Factory(const std::string& rhs) -> OTServerID
{
    return OTServerID(new implementation::Identifier(rhs));
}

auto identifier::UnitDefinition::Factory(const std::string& rhs) -> OTUnitID
{
    return OTUnitID(new implementation::Identifier(rhs));
}

auto Identifier::Factory(const String& rhs) -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier(rhs));
}

auto identifier::Nym::Factory(const String& rhs) -> OTNymID
{
    return OTNymID(new implementation::Identifier(rhs));
}

auto identifier::Server::Factory(const String& rhs) -> OTServerID
{
    return OTServerID(new implementation::Identifier(rhs));
}

auto identifier::UnitDefinition::Factory(const String& rhs) -> OTUnitID
{
    return OTUnitID(new implementation::Identifier(rhs));
}

auto Identifier::Factory(const identity::Nym& nym) -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier(nym));
}

auto identifier::Nym::Factory(const identity::Nym& rhs) -> OTNymID
{
    return OTNymID(new implementation::Identifier(rhs));
}

auto Identifier::Factory(const Contract& contract) -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier(contract));
}

auto Identifier::Factory(const Cheque& cheque) -> OTIdentifier
{
    return OTIdentifier(
        implementation::Identifier::contract_contents_to_identifier(cheque));
}

auto Identifier::Factory(const Item& item) -> OTIdentifier
{
    return OTIdentifier(
        implementation::Identifier::contract_contents_to_identifier(item));
}

auto Identifier::Factory(
    const contact::ContactItemType type,
    const proto::HDPath& path) -> OTIdentifier
{
    return OTIdentifier(new implementation::Identifier(type, path));
}

auto Identifier::Random() -> OTIdentifier
{
    OTIdentifier output{new implementation::Identifier};
    auto nonce = Data::Factory();
    Context().Crypto().Encode().Nonce(32, nonce);

    OT_ASSERT(32 == nonce->size());

    output->CalculateDigest(nonce->Bytes());

    OT_ASSERT(false == output->empty());

    return output;
}

auto Identifier::Validate(const std::string& input) -> bool
{
    if (input.empty()) { return false; }

    const auto id = Factory(input);

    return (0 < id->size());
}
}  // namespace opentxs

namespace opentxs::implementation
{
Identifier::Identifier()
    : ot_super()
    , type_(DefaultType)
{
}

Identifier::Identifier(const std::string& theStr)
    : ot_super()
    , type_(DefaultType)
{
    SetString(theStr);
}

Identifier::Identifier(const String& theStr)
    : ot_super()
    , type_(DefaultType)
{
    SetString(theStr);
}

Identifier::Identifier(const Contract& theContract)
    : ot_super()  // Get the contract's ID into this identifier.
    , type_(DefaultType)
{
    (const_cast<Contract&>(theContract)).GetIdentifier(*this);
}

Identifier::Identifier(const identity::Nym& theNym)
    : ot_super()  // Get the Nym's ID into this identifier.
    , type_(DefaultType)
{
    (const_cast<identity::Nym&>(theNym)).GetIdentifier(*this);
}

Identifier::Identifier(const Vector& data, const ID type)
    : ot_super(data)
    , type_(type)
{
}

Identifier::Identifier(
    const contact::ContactItemType type,
    const proto::HDPath& path)
    : ot_super()
    , type_(DefaultType)
{
    CalculateDigest(path_to_data(type, path)->Bytes(), DefaultType);
}

auto Identifier::CalculateDigest(const ReadView bytes, const ID type) -> bool
{
    type_ = type;

    return Context().Crypto().Hash().Digest(
        IDToHashType(type_), bytes, WriteInto());
}

auto Identifier::clone() const -> Identifier*
{
    return new Identifier(data_, type_);
}

auto Identifier::contract_contents_to_identifier(const Contract& in)
    -> Identifier*
{
    auto output = std::make_unique<Identifier>();

    OT_ASSERT(output);

    const auto preimage = String::Factory(in);
    output->CalculateDigest(preimage->Bytes(), DefaultType);

    return output.release();
}

// This Identifier is stored in binary form.
// But what if you want a pretty string version of it?
// Just call this function.
void Identifier::GetString(String& id) const
{
    const auto value = to_string();

    if (0 == value.size()) {
        id.Release();
    } else {
        id.Set(value.c_str());
    }
}

auto Identifier::IDToHashType(const ID type) -> crypto::HashType
{
    switch (type) {
        case (ID::sha256): {
            return crypto::HashType::Sha256;
        }
        case (ID::blake2b): {
            return crypto::HashType::Blake2b160;
        }
        default: {
            return crypto::HashType::None;
        }
    }
}

auto Identifier::path_to_data(
    const contact::ContactItemType type,
    const proto::HDPath& path) -> OTData
{
    auto output = Data::Factory(static_cast<const void*>(&type), sizeof(type));
    output += Data::Factory(path.root().c_str(), path.root().size());

    for (const auto& child : path.child()) {
        output += Data::Factory(&child, sizeof(child));
    }

    return output;
}

// SET (binary id) FROM ENCODED STRING
void Identifier::SetString(const String& encoded)
{
    return SetString(std::string(encoded.Get()));
}

void Identifier::SetString(const std::string& encoded)
{
    empty();

    if (MinimumSize > encoded.size()) { return; }

    if ('o' != encoded.at(0)) { return; }
    if ('t' != encoded.at(1)) { return; }

    std::string input(encoded.data() + 2, encoded.size() - 2);
    auto data = Context().Crypto().Encode().IdentifierDecode(input);

    if (!data.empty()) {
        type_ = static_cast<ID>(data[0]);

        switch (type_) {
            case (ID::sha256): {
            } break;
            case (ID::blake2b): {
            } break;
            default: {
                type_ = ID::invalid;

                return;
            }
        }

        Assign(
            (reinterpret_cast<const std::uint8_t*>(data.data()) + 1),
            (data.size() - 1));
    }
}

auto Identifier::str() const -> std::string { return to_string(); }

void Identifier::swap(opentxs::Identifier& rhs)
{
    auto& input = dynamic_cast<Identifier&>(rhs);
    ot_super::swap(std::move(input));
    std::swap(type_, input.type_);
}

auto Identifier::to_string() const noexcept -> std::string
{
    const auto preimage = [&] {
        auto out = Data::Factory();

        if (0 == size()) { return out; }

        out->Assign(&type_, sizeof(type_));
        out->Concatenate(data(), size());

        return out;
    }();
    auto ss = std::stringstream{};

    if (0 < preimage->size()) {
        ss << "ot";
        ss << Context().Crypto().Encode().IdentifierEncode(preimage);
    }

    return ss.str();
}
}  // namespace opentxs::implementation

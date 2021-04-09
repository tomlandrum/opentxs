// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/NymFile.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"
#include "util/Blank.hpp"

namespace opentxs
{
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs
{
template <>
struct make_blank<OTData> {
    static auto value(const api::Core&) -> OTData { return Data::Factory(); }
};
template <>
struct make_blank<OTIdentifier> {
    static auto value(const api::Core&) -> OTIdentifier
    {
        return Identifier::Factory();
    }
};
}  // namespace opentxs

namespace opentxs::internal
{
struct NymFile : virtual public opentxs::NymFile {
    virtual auto LoadSignedNymFile(const PasswordPrompt& reason) -> bool = 0;
    virtual auto SaveSignedNymFile(const PasswordPrompt& reason) -> bool = 0;
};
}  // namespace opentxs::internal

namespace opentxs::core::internal
{
using AddressTypeMap = std::map<AddressType, proto::AddressType>;
using AddressTypeReverseMap = std::map<proto::AddressType, AddressType>;

auto addresstype_map() noexcept -> const AddressTypeMap&;
auto translate(const AddressType in) noexcept -> proto::AddressType;
auto translate(const proto::AddressType in) noexcept -> AddressType;
}  // namespace opentxs::core::internal

namespace opentxs::factory
{
auto Secret(const std::size_t bytes) noexcept
    -> std::unique_ptr<opentxs::Secret>;
auto Secret(const ReadView bytes, const bool mode) noexcept
    -> std::unique_ptr<opentxs::Secret>;
}  // namespace opentxs::factory

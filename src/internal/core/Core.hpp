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
using AddressTypeMap = std::map<core::AddressType, proto::AddressType>;
using AddressTypeReverseMap = std::map<proto::AddressType, core::AddressType>;
using ConnectionInfoTypeMap =
    std::map<core::ConnectionInfoType, proto::ConnectionInfoType>;
using ConnectionInfoTypeReverseMap =
    std::map<proto::ConnectionInfoType, core::ConnectionInfoType>;
using PeerObjectTypeMap = std::map<core::PeerObjectType, proto::PeerObjectType>;
using PeerObjectTypeReverseMap =
    std::map<proto::PeerObjectType, core::PeerObjectType>;
using PeerRequestTypeMap =
    std::map<core::PeerRequestType, proto::PeerRequestType>;
using PeerRequestTypeReverseMap =
    std::map<proto::PeerRequestType, core::PeerRequestType>;
using ProtocolVersionMap =
    std::map<core::ProtocolVersion, proto::ProtocolVersion>;
using ProtocolVersionReverseMap =
    std::map<proto::ProtocolVersion, core::ProtocolVersion>;
using SecretTypeMap = std::map<core::SecretType, proto::SecretType>;
using SecretTypeReverseMap = std::map<proto::SecretType, core::SecretType>;
using UnitTypeMap = std::map<core::UnitType, proto::UnitType>;
using UnitTypeReverseMap = std::map<proto::UnitType, core::UnitType>;

auto addresstype_map() noexcept -> const AddressTypeMap&;
auto connectioninfotype_map() noexcept -> const ConnectionInfoTypeMap&;
auto peerobjecttype_map() noexcept -> const PeerObjectTypeMap&;
auto peerrequesttype_map() noexcept -> const PeerRequestTypeMap&;
auto protocolversion_map() noexcept -> const ProtocolVersionMap&;
auto secrettype_map() noexcept -> const SecretTypeMap&;
auto unittype_map() noexcept -> const UnitTypeMap&;
auto translate(const core::AddressType in) noexcept -> proto::AddressType;
auto translate(const core::ConnectionInfoType in) noexcept
    -> proto::ConnectionInfoType;
auto translate(const core::PeerObjectType in) noexcept -> proto::PeerObjectType;
auto translate(const core::PeerRequestType in) noexcept
    -> proto::PeerRequestType;
auto translate(const core::ProtocolVersion in) noexcept
    -> proto::ProtocolVersion;
auto translate(const core::SecretType in) noexcept -> proto::SecretType;
auto translate(const core::UnitType in) noexcept -> proto::UnitType;
auto translate(const proto::AddressType in) noexcept -> core::AddressType;
auto translate(const proto::ConnectionInfoType in) noexcept
    -> core::ConnectionInfoType;
auto translate(const proto::PeerObjectType in) noexcept -> core::PeerObjectType;
auto translate(const proto::PeerRequestType in) noexcept
    -> core::PeerRequestType;
auto translate(const proto::ProtocolVersion in) noexcept
    -> core::ProtocolVersion;
auto translate(const proto::SecretType in) noexcept -> core::SecretType;
auto translate(const proto::UnitType in) noexcept -> core::UnitType;
}  // namespace opentxs::core::internal

namespace opentxs::factory
{
auto Secret(const std::size_t bytes) noexcept
    -> std::unique_ptr<opentxs::Secret>;
auto Secret(const ReadView bytes, const bool mode) noexcept
    -> std::unique_ptr<opentxs::Secret>;
}  // namespace opentxs::factory

// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "internal/core/Core.hpp"  // IWYU pragma: associated

#include "opentxs/core/AddressType.hpp"
#include "opentxs/core/ProtocolVersion.hpp"
#include "opentxs/core/UnitType.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::core::internal
{
auto addresstype_map() noexcept -> const AddressTypeMap&
{
    static const auto map = AddressTypeMap{
        {AddressType::Error, proto::ADDRESSTYPE_ERROR},
        {AddressType::IPV4, proto::ADDRESSTYPE_IPV4},
        {AddressType::IPV6, proto::ADDRESSTYPE_IPV6},
        {AddressType::Onion, proto::ADDRESSTYPE_ONION},
        {AddressType::EEP, proto::ADDRESSTYPE_EEP},
        {AddressType::Inproc, proto::ADDRESSTYPE_INPROC},
    };

    return map;
}

auto protocolversion_map() noexcept -> const ProtocolVersionMap&
{
    static const auto map = ProtocolVersionMap{
        {ProtocolVersion::Error, proto::PROTOCOLVERSION_ERROR},
        {ProtocolVersion::Legacy, proto::PROTOCOLVERSION_LEGACY},
        {ProtocolVersion::Notify, proto::PROTOCOLVERSION_NOTIFY},
    };

    return map;
}

auto translate(core::AddressType in) noexcept -> proto::AddressType
{
    try {
        return addresstype_map().at(in);
    } catch (...) {
        return proto::ADDRESSTYPE_ERROR;
    }
}

auto translate(core::ProtocolVersion in) noexcept -> proto::ProtocolVersion
{
    try {
        return protocolversion_map().at(in);
    } catch (...) {
        return proto::PROTOCOLVERSION_ERROR;
    }
}

auto translate(core::UnitType in) noexcept -> proto::UnitType
{
    try {
        return unittype_map().at(in);
    } catch (...) {
        return proto::UNITTYPE_ERROR;
    }
}

auto translate(proto::AddressType in) noexcept -> core::AddressType
{
    static const auto map = reverse_arbitrary_map<
        core::AddressType,
        proto::AddressType,
        AddressTypeReverseMap>(addresstype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::AddressType::Error;
    }
}

auto translate(proto::ProtocolVersion in) noexcept -> core::ProtocolVersion
{
    static const auto map = reverse_arbitrary_map<
        core::ProtocolVersion,
        proto::ProtocolVersion,
        ProtocolVersionReverseMap>(protocolversion_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::ProtocolVersion::Error;
    }
}

auto translate(proto::UnitType in) noexcept -> core::UnitType
{
    static const auto map = reverse_arbitrary_map<
        core::UnitType,
        proto::UnitType,
        UnitTypeReverseMap>(unittype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::UnitType::Error;
    }
}

auto unittype_map() noexcept -> const UnitTypeMap&
{
    static const auto map = UnitTypeMap{
        {UnitType::Error, proto::UNITTYPE_ERROR},
        {UnitType::Currency, proto::UNITTYPE_CURRENCY},
        {UnitType::Security, proto::UNITTYPE_SECURITY},
        {UnitType::Basket, proto::UNITTYPE_ERROR},
    };

    return map;
}

}  // namespace opentxs::core::internal

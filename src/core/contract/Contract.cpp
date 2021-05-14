// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "0_stdafx.hpp"                         // IWYU pragma: associated
#include "1_Internal.hpp"                       // IWYU pragma: associated
#include "internal/core/contract/Contract.hpp"  // IWYU pragma: associated

#include "opentxs/core/contract/ProtocolVersion.hpp"
#include "opentxs/core/contract/UnitType.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::contract::peer::blank
{
auto Reply::asAcknowledgement() const noexcept
    -> const peer::reply::Acknowledgement&
{
    static auto const blank = peer::reply::blank::Acknowledgement{api_};
    return blank;
}

auto Reply::asBailment() const noexcept -> const peer::reply::Bailment&
{
    static auto const blank = peer::reply::blank::Bailment{api_};
    return blank;
}

auto Reply::asConnection() const noexcept -> const peer::reply::Connection&
{
    static auto const blank = peer::reply::blank::Connection{api_};
    return blank;
}

auto Reply::asOutbailment() const noexcept -> const peer::reply::Outbailment&
{
    static auto const blank = peer::reply::blank::Outbailment{api_};
    return blank;
}

auto Reply::Serialize(SerializedType& output) const -> bool
{
    output = {};
    return true;
}

auto Request::asBailment() const noexcept -> const peer::request::Bailment&
{
    static auto const blank = peer::request::blank::Bailment{api_};
    return blank;
}

auto Request::asBailmentNotice() const noexcept
    -> const peer::request::BailmentNotice&
{
    static auto const blank = peer::request::blank::BailmentNotice{api_};
    return blank;
}

auto Request::asConnection() const noexcept -> const peer::request::Connection&
{
    static auto const blank = peer::request::blank::Connection{api_};
    return blank;
}

auto Request::asOutbailment() const noexcept
    -> const peer::request::Outbailment&
{
    static auto const blank = peer::request::blank::Outbailment{api_};
    return blank;
}

auto Request::asStoreSecret() const noexcept
    -> const peer::request::StoreSecret&
{
    static auto const blank = peer::request::blank::StoreSecret{api_};
    return blank;
}

auto Request::Serialize(SerializedType& output) const -> bool
{
    output = {};
    return true;
}
}  // namespace opentxs::contract::peer::blank

namespace opentxs::contract::blank
{
auto Server::Serialize(proto::ServerContract& output, bool includeNym) const
    -> bool
{
    output = {};
    return true;
}

auto Unit::Serialize(proto::UnitDefinition& output, bool includeNym) const
    -> bool
{
    output = {};
    return true;
}
}  // namespace opentxs::contract::blank

namespace opentxs::contract::internal
{
auto protocolversion_map() noexcept -> const ProtocolVersionMap&
{
    static const auto map = ProtocolVersionMap{
        {ProtocolVersion::Error, proto::PROTOCOLVERSION_ERROR},
        {ProtocolVersion::Legacy, proto::PROTOCOLVERSION_LEGACY},
        {ProtocolVersion::Notify, proto::PROTOCOLVERSION_NOTIFY},
    };

    return map;
}

auto translate(ProtocolVersion in) noexcept -> proto::ProtocolVersion
{
    try {
        return protocolversion_map().at(in);
    } catch (...) {
        return proto::PROTOCOLVERSION_ERROR;
    }
}

auto translate(UnitType in) noexcept -> proto::UnitType
{
    try {
        return unittype_map().at(in);
    } catch (...) {
        return proto::UNITTYPE_ERROR;
    }
}

auto translate(proto::ProtocolVersion in) noexcept -> ProtocolVersion
{
    static const auto map = reverse_arbitrary_map<
        ProtocolVersion,
        proto::ProtocolVersion,
        ProtocolVersionReverseMap>(protocolversion_map());

    try {
        return map.at(in);
    } catch (...) {
        return ProtocolVersion::Error;
    }
}

auto translate(proto::UnitType in) noexcept -> UnitType
{
    static const auto map =
        reverse_arbitrary_map<UnitType, proto::UnitType, UnitTypeReverseMap>(
            unittype_map());

    try {
        return map.at(in);
    } catch (...) {
        return UnitType::Error;
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

}  // namespace opentxs::contract::internal

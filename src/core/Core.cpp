// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "internal/core/Core.hpp"  // IWYU pragma: associated

#include "opentxs/core/AddressType.hpp"
#include "opentxs/core/ConnectionInfoType.hpp"
#include "opentxs/core/PeerObjectType.hpp"
#include "opentxs/core/PeerRequestType.hpp"
#include "opentxs/core/ProtocolVersion.hpp"
#include "opentxs/core/SecretType.hpp"
#include "opentxs/core/UnitType.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"
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

auto connectioninfotype_map() noexcept -> const ConnectionInfoTypeMap&
{
    static const auto map = ConnectionInfoTypeMap{
        {ConnectionInfoType::Error, proto::CONNECTIONINFO_ERROR},
        {ConnectionInfoType::Bitcoin, proto::CONNECTIONINFO_BITCOIN},
        {ConnectionInfoType::BtcRpc, proto::CONNECTIONINFO_BTCRPC},
        {ConnectionInfoType::BitMessage, proto::CONNECTIONINFO_BITMESSAGE},
        {ConnectionInfoType::BitMessageRPC,
         proto::CONNECTIONINFO_BITMESSAGERPC},
        {ConnectionInfoType::SSH, proto::CONNECTIONINFO_SSH},
        {ConnectionInfoType::CJDNS, proto::CONNECTIONINFO_CJDNS},
    };

    return map;
}

auto peerobjecttype_map() noexcept -> const PeerObjectTypeMap&
{
    static const auto map = PeerObjectTypeMap{
        {PeerObjectType::Error, proto::PEEROBJECT_ERROR},
        {PeerObjectType::Message, proto::PEEROBJECT_MESSAGE},
        {PeerObjectType::Request, proto::PEEROBJECT_REQUEST},
        {PeerObjectType::Response, proto::PEEROBJECT_RESPONSE},
        {PeerObjectType::Payment, proto::PEEROBJECT_PAYMENT},
        {PeerObjectType::Cash, proto::PEEROBJECT_CASH},
    };

    return map;
}

auto peerrequesttype_map() noexcept -> const PeerRequestTypeMap&
{
    static const auto map = PeerRequestTypeMap{
        {PeerRequestType::Error, proto::PEERREQUEST_ERROR},
        {PeerRequestType::Bailment, proto::PEERREQUEST_BAILMENT},
        {PeerRequestType::OutBailment, proto::PEERREQUEST_OUTBAILMENT},
        {PeerRequestType::PendingBailment, proto::PEERREQUEST_PENDINGBAILMENT},
        {PeerRequestType::ConnectionInfo, proto::PEERREQUEST_CONNECTIONINFO},
        {PeerRequestType::StoreSecret, proto::PEERREQUEST_STORESECRET},
        {PeerRequestType::VerificationOffer,
         proto::PEERREQUEST_VERIFICATIONOFFER},
        {PeerRequestType::Faucet, proto::PEERREQUEST_FAUCET},
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

auto secrettype_map() noexcept -> const SecretTypeMap&
{
    static const auto map = SecretTypeMap{
        {SecretType::Error, proto::SECRETTYPE_ERROR},
        {SecretType::Bip39, proto::SECRETTYPE_BIP39},
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

auto translate(core::ConnectionInfoType in) noexcept
    -> proto::ConnectionInfoType
{
    try {
        return connectioninfotype_map().at(in);
    } catch (...) {
        return proto::CONNECTIONINFO_ERROR;
    }
}

auto translate(core::PeerObjectType in) noexcept -> proto::PeerObjectType
{
    try {
        return peerobjecttype_map().at(in);
    } catch (...) {
        return proto::PEEROBJECT_ERROR;
    }
}

auto translate(core::PeerRequestType in) noexcept -> proto::PeerRequestType
{
    try {
        return peerrequesttype_map().at(in);
    } catch (...) {
        return proto::PEERREQUEST_ERROR;
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

auto translate(core::SecretType in) noexcept -> proto::SecretType
{
    try {
        return secrettype_map().at(in);
    } catch (...) {
        return proto::SECRETTYPE_ERROR;
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

auto translate(proto::ConnectionInfoType in) noexcept
    -> core::ConnectionInfoType
{
    static const auto map = reverse_arbitrary_map<
        core::ConnectionInfoType,
        proto::ConnectionInfoType,
        ConnectionInfoTypeReverseMap>(connectioninfotype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::ConnectionInfoType::Error;
    }
}

auto translate(proto::PeerObjectType in) noexcept -> core::PeerObjectType
{
    static const auto map = reverse_arbitrary_map<
        core::PeerObjectType,
        proto::PeerObjectType,
        PeerObjectTypeReverseMap>(peerobjecttype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::PeerObjectType::Error;
    }
}

auto translate(proto::PeerRequestType in) noexcept -> core::PeerRequestType
{
    static const auto map = reverse_arbitrary_map<
        core::PeerRequestType,
        proto::PeerRequestType,
        PeerRequestTypeReverseMap>(peerrequesttype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::PeerRequestType::Error;
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

auto translate(proto::SecretType in) noexcept -> core::SecretType
{
    static const auto map = reverse_arbitrary_map<
        core::SecretType,
        proto::SecretType,
        SecretTypeReverseMap>(secrettype_map());

    try {
        return map.at(in);
    } catch (...) {
        return core::SecretType::Error;
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

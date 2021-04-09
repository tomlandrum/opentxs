// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                          // IWYU pragma: associated
#include "1_Internal.hpp"                        // IWYU pragma: associated
#include "internal/core/contract/peer/Peer.hpp"  // IWYU pragma: associated

#include "opentxs/core/contract/peer/ConnectionInfoType.hpp"
#include "opentxs/core/contract/peer/PeerObjectType.hpp"
#include "opentxs/core/contract/peer/PeerRequestType.hpp"
#include "opentxs/core/contract/peer/SecretType.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::contract::peer::internal
{
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

auto secrettype_map() noexcept -> const SecretTypeMap&
{
    static const auto map = SecretTypeMap{
        {SecretType::Error, proto::SECRETTYPE_ERROR},
        {SecretType::Bip39, proto::SECRETTYPE_BIP39},
    };

    return map;
}

auto translate(ConnectionInfoType in) noexcept -> proto::ConnectionInfoType
{
    try {
        return connectioninfotype_map().at(in);
    } catch (...) {
        return proto::CONNECTIONINFO_ERROR;
    }
}

auto translate(PeerObjectType in) noexcept -> proto::PeerObjectType
{
    try {
        return peerobjecttype_map().at(in);
    } catch (...) {
        return proto::PEEROBJECT_ERROR;
    }
}

auto translate(PeerRequestType in) noexcept -> proto::PeerRequestType
{
    try {
        return peerrequesttype_map().at(in);
    } catch (...) {
        return proto::PEERREQUEST_ERROR;
    }
}

auto translate(SecretType in) noexcept -> proto::SecretType
{
    try {
        return secrettype_map().at(in);
    } catch (...) {
        return proto::SECRETTYPE_ERROR;
    }
}

auto translate(proto::ConnectionInfoType in) noexcept -> ConnectionInfoType
{
    static const auto map = reverse_arbitrary_map<
        ConnectionInfoType,
        proto::ConnectionInfoType,
        ConnectionInfoTypeReverseMap>(connectioninfotype_map());

    try {
        return map.at(in);
    } catch (...) {
        return ConnectionInfoType::Error;
    }
}

auto translate(proto::PeerObjectType in) noexcept -> PeerObjectType
{
    static const auto map = reverse_arbitrary_map<
        PeerObjectType,
        proto::PeerObjectType,
        PeerObjectTypeReverseMap>(peerobjecttype_map());

    try {
        return map.at(in);
    } catch (...) {
        return PeerObjectType::Error;
    }
}

auto translate(proto::PeerRequestType in) noexcept -> PeerRequestType
{
    static const auto map = reverse_arbitrary_map<
        PeerRequestType,
        proto::PeerRequestType,
        PeerRequestTypeReverseMap>(peerrequesttype_map());

    try {
        return map.at(in);
    } catch (...) {
        return PeerRequestType::Error;
    }
}

auto translate(proto::SecretType in) noexcept -> SecretType
{
    static const auto map = reverse_arbitrary_map<
        SecretType,
        proto::SecretType,
        SecretTypeReverseMap>(secrettype_map());

    try {
        return map.at(in);
    } catch (...) {
        return SecretType::Error;
    }
}

}  // namespace opentxs::contract::peer::internal

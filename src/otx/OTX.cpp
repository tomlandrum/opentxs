// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"          // IWYU pragma: associated
#include "1_Internal.hpp"        // IWYU pragma: associated
#include "internal/otx/OTX.hpp"  // IWYU pragma: associated

#include "opentxs/otx/ConsensusType.hpp"
#include "opentxs/otx/ServerRequestType.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"
#include "opentxs/protobuf/OTXEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::otx::internal
{
auto consensustype_map() noexcept -> const ConsensusTypeMap&
{
    static const auto map = ConsensusTypeMap{
        {ConsensusType::Error, proto::CONSENSUSTYPE_ERROR},
        {ConsensusType::Server, proto::CONSENSUSTYPE_SERVER},
        {ConsensusType::Client, proto::CONSENSUSTYPE_CLIENT},
        {ConsensusType::Peer, proto::CONSENSUSTYPE_PEER},
    };

    return map;
}

auto lastreplystatus_map() noexcept -> const LastReplyStatusMap&
{
    static const auto map = LastReplyStatusMap{
        {LastReplyStatus::Invalid, proto::LASTREPLYSTATUS_INVALID},
        {LastReplyStatus::None, proto::LASTREPLYSTATUS_NONE},
        {LastReplyStatus::MessageSuccess,
         proto::LASTREPLYSTATUS_MESSAGESUCCESS},
        {LastReplyStatus::MessageFailed, proto::LASTREPLYSTATUS_MESSAGEFAILED},
        {LastReplyStatus::Unknown, proto::LASTREPLYSTATUS_UNKNOWN},
        {LastReplyStatus::NotSent, proto::LASTREPLYSTATUS_NOTSENT},
    };

    return map;
}

auto serverreplytype_map() noexcept -> const ServerReplyTypeMap&
{
    static const auto map = ServerReplyTypeMap{
        {ServerReplyType::Error, proto::SERVERREPLY_ERROR},
        {ServerReplyType::Activate, proto::SERVERREPLY_ACTIVATE},
        {ServerReplyType::Push, proto::SERVERREPLY_PUSH},
    };

    return map;
}

auto serverrequesttype_map() noexcept -> const ServerRequestTypeMap&
{
    static const auto map = ServerRequestTypeMap{
        {ServerRequestType::Error, proto::SERVERREQUEST_ERROR},
        {ServerRequestType::Activate, proto::SERVERREQUEST_ACTIVATE},
    };

    return map;
}

auto translate(otx::ConsensusType in) noexcept -> proto::ConsensusType
{
    try {
        return consensustype_map().at(in);
    } catch (...) {
        return proto::CONSENSUSTYPE_ERROR;
    }
}

auto translate(otx::LastReplyStatus in) noexcept -> proto::LastReplyStatus
{
    try {
        return lastreplystatus_map().at(in);
    } catch (...) {
        return proto::LASTREPLYSTATUS_INVALID;
    }
}

auto translate(otx::ServerReplyType in) noexcept -> proto::ServerReplyType
{
    try {
        return serverreplytype_map().at(in);
    } catch (...) {
        return proto::SERVERREPLY_ERROR;
    }
}

auto translate(otx::ServerRequestType in) noexcept -> proto::ServerRequestType
{
    try {
        return serverrequesttype_map().at(in);
    } catch (...) {
        return proto::SERVERREQUEST_ERROR;
    }
}

auto translate(proto::ConsensusType in) noexcept -> otx::ConsensusType
{
    static const auto map = reverse_arbitrary_map<
        otx::ConsensusType,
        proto::ConsensusType,
        ConsensusTypeReverseMap>(consensustype_map());

    try {
        return map.at(in);
    } catch (...) {
        return otx::ConsensusType::Error;
    }
}

auto translate(proto::LastReplyStatus in) noexcept -> otx::LastReplyStatus
{
    static const auto map = reverse_arbitrary_map<
        otx::LastReplyStatus,
        proto::LastReplyStatus,
        LastReplyStatusReverseMap>(lastreplystatus_map());

    try {
        return map.at(in);
    } catch (...) {
        return otx::LastReplyStatus::Invalid;
    }
}

auto translate(proto::ServerReplyType in) noexcept -> otx::ServerReplyType
{
    static const auto map = reverse_arbitrary_map<
        otx::ServerReplyType,
        proto::ServerReplyType,
        ServerReplyTypeReverseMap>(serverreplytype_map());

    try {
        return map.at(in);
    } catch (...) {
        return otx::ServerReplyType::Error;
    }
}

auto translate(proto::ServerRequestType in) noexcept -> otx::ServerRequestType
{
    static const auto map = reverse_arbitrary_map<
        otx::ServerRequestType,
        proto::ServerRequestType,
        ServerRequestTypeReverseMap>(serverrequesttype_map());

    try {
        return map.at(in);
    } catch (...) {
        return otx::ServerRequestType::Error;
    }
}
}  // namespace opentxs::otx::internal

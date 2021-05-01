// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/otx/ConsensusType.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"
#include "opentxs/otx/ServerReplyType.hpp"
#include "opentxs/otx/ServerRequestType.hpp"
#include "opentxs/protobuf/ConsensusEnums.pb.h"
#include "opentxs/protobuf/OTXEnums.pb.h"

namespace opentxs::otx::internal
{
using ConsensusTypeMap = std::map<ConsensusType, proto::ConsensusType>;
using ConsensusTypeReverseMap = std::map<proto::ConsensusType, ConsensusType>;
using LastReplyStatusMap = std::map<LastReplyStatus, proto::LastReplyStatus>;
using LastReplyStatusReverseMap =
    std::map<proto::LastReplyStatus, LastReplyStatus>;
using OTXPushTypeMap = std::map<OTXPushType, proto::OTXPushType>;
using OTXPushTypeReverseMap = std::map<proto::OTXPushType, OTXPushType>;
using ServerReplyTypeMap = std::map<ServerReplyType, proto::ServerReplyType>;
using ServerReplyTypeReverseMap =
    std::map<proto::ServerReplyType, ServerReplyType>;
using ServerRequestTypeMap =
    std::map<ServerRequestType, proto::ServerRequestType>;
using ServerRequestTypeReverseMap =
    std::map<proto::ServerRequestType, ServerRequestType>;

auto consensustype_map() noexcept -> const ConsensusTypeMap&;
auto lastreplystatus_map() noexcept -> const LastReplyStatusMap&;
auto otxpushtype_map() noexcept -> const OTXPushTypeMap&;
auto serverreplytype_map() noexcept -> const ServerReplyTypeMap&;
auto serverrequesttype_map() noexcept -> const ServerRequestTypeMap&;
OPENTXS_EXPORT auto translate(ConsensusType in) noexcept
    -> proto::ConsensusType;
OPENTXS_EXPORT auto translate(LastReplyStatus in) noexcept
    -> proto::LastReplyStatus;
OPENTXS_EXPORT auto translate(OTXPushType in) noexcept -> proto::OTXPushType;
OPENTXS_EXPORT auto translate(ServerReplyType in) noexcept
    -> proto::ServerReplyType;
OPENTXS_EXPORT auto translate(ServerRequestType in) noexcept
    -> proto::ServerRequestType;
OPENTXS_EXPORT auto translate(proto::ConsensusType in) noexcept
    -> ConsensusType;
OPENTXS_EXPORT auto translate(proto::LastReplyStatus in) noexcept
    -> LastReplyStatus;
OPENTXS_EXPORT auto translate(proto::OTXPushType in) noexcept -> OTXPushType;
OPENTXS_EXPORT auto translate(proto::ServerReplyType in) noexcept
    -> ServerReplyType;
OPENTXS_EXPORT auto translate(proto::ServerRequestType in) noexcept
    -> ServerRequestType;

}  // namespace opentxs::otx::internal

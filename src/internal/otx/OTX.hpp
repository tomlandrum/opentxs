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
using ServerReplyTypeMap = std::map<ServerReplyType, proto::ServerReplyType>;
using ServerReplyTypeReverseMap =
    std::map<proto::ServerReplyType, ServerReplyType>;
using ServerRequestTypeMap =
    std::map<ServerRequestType, proto::ServerRequestType>;
using ServerRequestTypeReverseMap =
    std::map<proto::ServerRequestType, ServerRequestType>;

auto consensustype_map() noexcept -> const ConsensusTypeMap&;
auto lastreplystatus_map() noexcept -> const LastReplyStatusMap&;
auto serverreplytype_map() noexcept -> const ServerReplyTypeMap&;
auto serverrequesttype_map() noexcept -> const ServerRequestTypeMap&;
auto translate(ConsensusType in) noexcept -> proto::ConsensusType;
auto translate(LastReplyStatus in) noexcept -> proto::LastReplyStatus;
auto translate(ServerReplyType in) noexcept -> proto::ServerReplyType;
auto translate(ServerRequestType in) noexcept -> proto::ServerRequestType;
auto translate(proto::ConsensusType in) noexcept -> ConsensusType;
auto translate(proto::LastReplyStatus in) noexcept -> LastReplyStatus;
auto translate(proto::ServerReplyType in) noexcept -> ServerReplyType;
auto translate(proto::ServerRequestType in) noexcept -> ServerRequestType;

}  // namespace opentxs::otx::internal

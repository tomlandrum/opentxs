// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/core/contract/peer/Types.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"
#include "util/Blank.hpp"

namespace opentxs::contract::peer::internal
{
using ConnectionInfoTypeMap =
    std::map<ConnectionInfoType, proto::ConnectionInfoType>;
using ConnectionInfoTypeReverseMap =
    std::map<proto::ConnectionInfoType, ConnectionInfoType>;
using PeerObjectTypeMap = std::map<PeerObjectType, proto::PeerObjectType>;
using PeerObjectTypeReverseMap =
    std::map<proto::PeerObjectType, PeerObjectType>;
using PeerRequestTypeMap = std::map<PeerRequestType, proto::PeerRequestType>;
using PeerRequestTypeReverseMap =
    std::map<proto::PeerRequestType, PeerRequestType>;
using SecretTypeMap = std::map<SecretType, proto::SecretType>;
using SecretTypeReverseMap = std::map<proto::SecretType, SecretType>;

auto connectioninfotype_map() noexcept -> const ConnectionInfoTypeMap&;
auto peerobjecttype_map() noexcept -> const PeerObjectTypeMap&;
auto peerrequesttype_map() noexcept -> const PeerRequestTypeMap&;
auto secrettype_map() noexcept -> const SecretTypeMap&;
auto translate(const ConnectionInfoType in) noexcept
    -> proto::ConnectionInfoType;
auto translate(const PeerObjectType in) noexcept -> proto::PeerObjectType;
auto translate(const PeerRequestType in) noexcept -> proto::PeerRequestType;
auto translate(const SecretType in) noexcept -> proto::SecretType;
auto translate(const proto::ConnectionInfoType in) noexcept
    -> ConnectionInfoType;
auto translate(const proto::PeerObjectType in) noexcept -> PeerObjectType;
auto translate(const proto::PeerRequestType in) noexcept -> PeerRequestType;
auto translate(const proto::SecretType in) noexcept -> SecretType;
}  // namespace opentxs::contract::peer::internal

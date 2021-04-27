// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/core/contract/peer/ConnectionInfoType.hpp"
// IWYU pragma: no_include "opentxs/core/contract/peer/PeerObjectType.hpp"
// IWYU pragma: no_include "opentxs/core/contract/peer/PeerRequestType.hpp"
// IWYU pragma: no_include "opentxs/core/contract/peer/SecretType.hpp"

#pragma once

#include <map>

#include "opentxs/Bytes.hpp"
#include "opentxs/core/contract/peer/Types.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PairEvent.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"
#include "opentxs/protobuf/ZMQEnums.pb.h"
#include "util/Blank.hpp"

namespace opentxs::contract::peer::internal
{
enum class PairEventType : std::uint8_t {
    Error = 0,
    Rename = 1,
    StoreSecret = 2,
};

struct OPENTXS_EXPORT PairEvent {
    std::uint32_t version_;
    PairEventType type_;
    std::string issuer_;

    PairEvent(const ReadView);

private:
    PairEvent(const std::uint32_t, const PairEventType, const std::string&);
    PairEvent(const proto::PairEvent&);

    PairEvent() = delete;
    PairEvent(const PairEvent&) = delete;
    PairEvent(PairEvent&&) = delete;
    auto operator=(const PairEvent&) -> PairEvent& = delete;
    auto operator=(PairEvent&&) -> PairEvent& = delete;
};

using ConnectionInfoTypeMap =
    std::map<ConnectionInfoType, proto::ConnectionInfoType>;
using ConnectionInfoTypeReverseMap =
    std::map<proto::ConnectionInfoType, ConnectionInfoType>;
using PairEventTypeMap = std::map<PairEventType, proto::PairEventType>;
using PairEventTypeReverseMap = std::map<proto::PairEventType, PairEventType>;
using PeerObjectTypeMap = std::map<PeerObjectType, proto::PeerObjectType>;
using PeerObjectTypeReverseMap =
    std::map<proto::PeerObjectType, PeerObjectType>;
using PeerRequestTypeMap = std::map<PeerRequestType, proto::PeerRequestType>;
using PeerRequestTypeReverseMap =
    std::map<proto::PeerRequestType, PeerRequestType>;
using SecretTypeMap = std::map<SecretType, proto::SecretType>;
using SecretTypeReverseMap = std::map<proto::SecretType, SecretType>;

auto connectioninfotype_map() noexcept -> const ConnectionInfoTypeMap&;
auto paireventtype_map() noexcept -> const PairEventTypeMap&;
auto peerobjecttype_map() noexcept -> const PeerObjectTypeMap&;
auto peerrequesttype_map() noexcept -> const PeerRequestTypeMap&;
auto secrettype_map() noexcept -> const SecretTypeMap&;
OPENTXS_EXPORT auto translate(const ConnectionInfoType in) noexcept
    -> proto::ConnectionInfoType;
OPENTXS_EXPORT auto translate(const PairEventType in) noexcept
    -> proto::PairEventType;
OPENTXS_EXPORT auto translate(const PeerObjectType in) noexcept
    -> proto::PeerObjectType;
OPENTXS_EXPORT auto translate(const PeerRequestType in) noexcept
    -> proto::PeerRequestType;
OPENTXS_EXPORT auto translate(const SecretType in) noexcept
    -> proto::SecretType;
OPENTXS_EXPORT auto translate(const proto::ConnectionInfoType in) noexcept
    -> ConnectionInfoType;
OPENTXS_EXPORT auto translate(const proto::PairEventType in) noexcept
    -> PairEventType;
OPENTXS_EXPORT auto translate(const proto::PeerObjectType in) noexcept
    -> PeerObjectType;
OPENTXS_EXPORT auto translate(const proto::PeerRequestType in) noexcept
    -> PeerRequestType;
OPENTXS_EXPORT auto translate(const proto::SecretType in) noexcept
    -> SecretType;
}  // namespace opentxs::contract::peer::internal

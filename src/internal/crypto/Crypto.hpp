// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/SymmetricKeyType.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs::crypto::internal
{
using AsymmetricKeyTypeMap =
    std::map<crypto::AsymmetricKeyType, proto::AsymmetricKeyType>;
using AsymmetricKeyTypeReverseMap =
    std::map<proto::AsymmetricKeyType, crypto::AsymmetricKeyType>;
using SymmetricKeyTypeMap =
    std::map<crypto::SymmetricKeyType, proto::SymmetricKeyType>;
using SymmetricKeyTypeReverseMap =
    std::map<proto::SymmetricKeyType, crypto::SymmetricKeyType>;
using SymmetricModeMap = std::map<crypto::SymmetricMode, proto::SymmetricMode>;
using SymmetricModeReverseMap =
    std::map<proto::SymmetricMode, crypto::SymmetricMode>;

auto asymmetrickeytype_map() noexcept -> const AsymmetricKeyTypeMap&;
auto translate(crypto::AsymmetricKeyType in) noexcept
    -> proto::AsymmetricKeyType;
auto translate(proto::AsymmetricKeyType in) noexcept
    -> crypto::AsymmetricKeyType;

auto symmetrickeytype_map() noexcept -> const SymmetricKeyTypeMap&;
auto translate(crypto::SymmetricKeyType in) noexcept -> proto::SymmetricKeyType;
auto translate(proto::SymmetricKeyType in) noexcept -> crypto::SymmetricKeyType;

auto symmetricmode_map() noexcept -> const SymmetricModeMap&;
auto translate(crypto::SymmetricMode in) noexcept -> proto::SymmetricMode;
auto translate(proto::SymmetricMode in) noexcept -> crypto::SymmetricMode;

}  // namespace opentxs::crypto::internal

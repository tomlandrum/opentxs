// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/crypto/key/asymmetric/Mode.hpp"
#include "opentxs/crypto/key/asymmetric/Role.hpp"
#include "opentxs/crypto/key/symmetric/Algorithm.hpp"
#include "opentxs/crypto/key/symmetric/Source.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs::crypto::key::internal
{
using AsymmetricAlgorithmMap =
    std::map<asymmetric::Algorithm, proto::AsymmetricKeyType>;
using AsymmetricAlgorithmReverseMap =
    std::map<proto::AsymmetricKeyType, asymmetric::Algorithm>;
using ModeMap = std::map<asymmetric::Mode, proto::KeyMode>;
using ModeReverseMap = std::map<proto::KeyMode, asymmetric::Mode>;
using RoleMap = std::map<asymmetric::Role, proto::KeyRole>;
using RoleReverseMap = std::map<proto::KeyRole, asymmetric::Role>;
using SourceMap = std::map<symmetric::Source, proto::SymmetricKeyType>;
using SourceReverseMap = std::map<proto::SymmetricKeyType, symmetric::Source>;
using SymmetricAlgorithmMap =
    std::map<symmetric::Algorithm, proto::SymmetricMode>;
using SymmetricAlgorithmReverseMap =
    std::map<proto::SymmetricMode, symmetric::Algorithm>;

auto asymmetricalgorithm_map() noexcept -> const AsymmetricAlgorithmMap&;
auto mode_map() noexcept -> const ModeMap&;
auto role_map() noexcept -> const RoleMap&;
auto source_map() noexcept -> const SourceMap&;
auto symmetricalgorithm_map() noexcept -> const SymmetricAlgorithmMap&;
auto translate(asymmetric::Algorithm in) noexcept -> proto::AsymmetricKeyType;
auto translate(asymmetric::Mode in) noexcept -> proto::KeyMode;
auto translate(asymmetric::Role in) noexcept -> proto::KeyRole;
auto translate(symmetric::Source in) noexcept -> proto::SymmetricKeyType;
auto translate(symmetric::Algorithm in) noexcept -> proto::SymmetricMode;
auto translate(proto::KeyMode in) noexcept -> asymmetric::Mode;
auto translate(proto::KeyRole in) noexcept -> asymmetric::Role;
auto translate(proto::AsymmetricKeyType in) noexcept -> asymmetric::Algorithm;
auto translate(proto::SymmetricKeyType in) noexcept -> symmetric::Source;
auto translate(proto::SymmetricMode in) noexcept -> symmetric::Algorithm;

}  // namespace opentxs::crypto::key::internal

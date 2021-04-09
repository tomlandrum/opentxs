// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                 // IWYU pragma: associated
#include "1_Internal.hpp"               // IWYU pragma: associated
#include "internal/crypto/key/Key.hpp"  // IWYU pragma: associated

#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/crypto/key/symmetric/Algorithm.hpp"
#include "opentxs/crypto/key/symmetric/Source.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "util/Container.hpp"

namespace opentxs::crypto::key::internal
{
auto asymmetricalgorithm_map() noexcept -> const AsymmetricAlgorithmMap&
{
    static const auto map = AsymmetricAlgorithmMap{
        {asymmetric::Algorithm::Error, proto::AKEYTYPE_ERROR},
        {asymmetric::Algorithm::Null, proto::AKEYTYPE_NULL},
        {asymmetric::Algorithm::Legacy, proto::AKEYTYPE_LEGACY},
        {asymmetric::Algorithm::Secp256k1, proto::AKEYTYPE_SECP256K1},
        {asymmetric::Algorithm::ED25519, proto::AKEYTYPE_ED25519},
    };

    return map;
}

auto mode_map() noexcept -> const ModeMap&
{
    static const auto map = ModeMap{
        {asymmetric::Mode::Error, proto::KEYMODE_ERROR},
        {asymmetric::Mode::Null, proto::KEYMODE_NULL},
        {asymmetric::Mode::Private, proto::KEYMODE_PRIVATE},
        {asymmetric::Mode::Public, proto::KEYMODE_PUBLIC},
    };

    return map;
}

auto role_map() noexcept -> const RoleMap&
{
    static const auto map = RoleMap{
        {crypto::key::asymmetric::Role::Error, proto::KEYROLE_ERROR},
        {crypto::key::asymmetric::Role::Auth, proto::KEYROLE_AUTH},
        {crypto::key::asymmetric::Role::Encrypt, proto::KEYROLE_ENCRYPT},
        {crypto::key::asymmetric::Role::Sign, proto::KEYROLE_SIGN},
    };

    return map;
}

auto source_map() noexcept -> const SourceMap&
{
    static const auto map = SourceMap{
        {symmetric::Source::Error, proto::SKEYTYPE_ERROR},
        {symmetric::Source::Raw, proto::SKEYTYPE_RAW},
        {symmetric::Source::ECDH, proto::SKEYTYPE_ECDH},
        {symmetric::Source::Argon2, proto::SKEYTYPE_ARGON2},
    };

    return map;
}

auto symmetricalgorithm_map() noexcept -> const SymmetricAlgorithmMap&
{
    static const auto map = SymmetricAlgorithmMap{
        {symmetric::Algorithm::Error, proto::SMODE_ERROR},
        {symmetric::Algorithm::ChaCha20Poly1305, proto::SMODE_CHACHA20POLY1305},
    };

    return map;
}

auto translate(asymmetric::Algorithm in) noexcept -> proto::AsymmetricKeyType
{
    try {
        return asymmetricalgorithm_map().at(in);
    } catch (...) {
        return proto::AKEYTYPE_ERROR;
    }
}

auto translate(const asymmetric::Mode in) noexcept -> proto::KeyMode
{
    try {
        return mode_map().at(in);
    } catch (...) {
        return proto::KEYMODE_ERROR;
    }
}

auto translate(const asymmetric::Role in) noexcept -> proto::KeyRole
{
    try {
        return role_map().at(in);
    } catch (...) {
        return proto::KEYROLE_ERROR;
    }
}

auto translate(symmetric::Source in) noexcept -> proto::SymmetricKeyType
{
    try {
        return source_map().at(in);
    } catch (...) {
        return proto::SKEYTYPE_ERROR;
    }
}

auto translate(symmetric::Algorithm in) noexcept -> proto::SymmetricMode
{
    try {
        return symmetricalgorithm_map().at(in);
    } catch (...) {
        return proto::SMODE_ERROR;
    }
}

auto translate(proto::AsymmetricKeyType in) noexcept -> asymmetric::Algorithm
{
    static const auto map = reverse_arbitrary_map<
        asymmetric::Algorithm,
        proto::AsymmetricKeyType,
        AsymmetricAlgorithmReverseMap>(asymmetricalgorithm_map());

    try {
        return map.at(in);
    } catch (...) {
        return asymmetric::Algorithm::Error;
    }
}

auto translate(const proto::KeyMode in) noexcept -> asymmetric::Mode
{
    static const auto map =
        reverse_arbitrary_map<asymmetric::Mode, proto::KeyMode, ModeReverseMap>(
            mode_map());

    try {
        return map.at(in);
    } catch (...) {
        return asymmetric::Mode::Error;
    }
}

auto translate(const proto::KeyRole in) noexcept
    -> opentxs::crypto::key::asymmetric::Role
{
    static const auto map = reverse_arbitrary_map<
        crypto::key::asymmetric::Role,
        proto::KeyRole,
        RoleReverseMap>(role_map());

    try {
        return map.at(in);
    } catch (...) {
        return opentxs::crypto::key::asymmetric::Role::Error;
    }
}

auto translate(proto::SymmetricKeyType in) noexcept -> symmetric::Source
{
    static const auto map = reverse_arbitrary_map<
        symmetric::Source,
        proto::SymmetricKeyType,
        SourceReverseMap>(source_map());

    try {
        return map.at(in);
    } catch (...) {
        return symmetric::Source::Error;
    }
}

auto translate(proto::SymmetricMode in) noexcept -> symmetric::Algorithm
{
    static const auto map = reverse_arbitrary_map<
        symmetric::Algorithm,
        proto::SymmetricMode,
        SymmetricAlgorithmReverseMap>(symmetricalgorithm_map());

    try {
        return map.at(in);
    } catch (...) {
        return symmetric::Algorithm::Error;
    }
}
}  // namespace opentxs::crypto::key::internal

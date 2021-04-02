// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                // IWYU pragma: associated
#include "1_Internal.hpp"              // IWYU pragma: associated
#include "internal/crypto/Crypto.hpp"  // IWYU pragma: associated

#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/SymmetricMode.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "util/Container.hpp"

namespace opentxs::crypto::internal
{
auto asymmetrickeytype_map() noexcept -> const AsymmetricKeyTypeMap&
{
    static const auto map = AsymmetricKeyTypeMap{
        {AsymmetricKeyType::Error, proto::AKEYTYPE_ERROR},
        {AsymmetricKeyType::Null, proto::AKEYTYPE_NULL},
        {AsymmetricKeyType::Legacy, proto::AKEYTYPE_LEGACY},
        {AsymmetricKeyType::Secp256k1, proto::AKEYTYPE_SECP256K1},
        {AsymmetricKeyType::ED25519, proto::AKEYTYPE_ED25519},
    };

    return map;
}

auto symmetrickeytype_map() noexcept -> const SymmetricKeyTypeMap&
{
    static const auto map = SymmetricKeyTypeMap{
        {SymmetricKeyType::Error, proto::SKEYTYPE_ERROR},
        {SymmetricKeyType::Raw, proto::SKEYTYPE_RAW},
        {SymmetricKeyType::ECDH, proto::SKEYTYPE_ECDH},
        {SymmetricKeyType::Argon2, proto::SKEYTYPE_ARGON2},
    };

    return map;
}

auto symmetricmode_map() noexcept -> const SymmetricModeMap&
{
    static const auto map = SymmetricModeMap{
        {SymmetricMode::Error, proto::SMODE_ERROR},
        {SymmetricMode::ChaCha20Poly1305, proto::SMODE_CHACHA20POLY1305},
    };

    return map;
}

auto translate(crypto::AsymmetricKeyType in) noexcept
    -> proto::AsymmetricKeyType
{
    try {
        return asymmetrickeytype_map().at(in);
    } catch (...) {
        return proto::AKEYTYPE_ERROR;
    }
}

auto translate(crypto::SymmetricKeyType in) noexcept -> proto::SymmetricKeyType
{
    try {
        return symmetrickeytype_map().at(in);
    } catch (...) {
        return proto::SKEYTYPE_ERROR;
    }
}

auto translate(crypto::SymmetricMode in) noexcept -> proto::SymmetricMode
{
    try {
        return symmetricmode_map().at(in);
    } catch (...) {
        return proto::SMODE_ERROR;
    }
}

auto translate(proto::AsymmetricKeyType in) noexcept
    -> crypto::AsymmetricKeyType
{
    static const auto map = reverse_arbitrary_map<
        crypto::AsymmetricKeyType,
        proto::AsymmetricKeyType,
        AsymmetricKeyTypeReverseMap>(asymmetrickeytype_map());

    try {
        return map.at(in);
    } catch (...) {
        return crypto::AsymmetricKeyType::Error;
    }
}

auto translate(proto::SymmetricKeyType in) noexcept -> crypto::SymmetricKeyType
{
    static const auto map = reverse_arbitrary_map<
        crypto::SymmetricKeyType,
        proto::SymmetricKeyType,
        SymmetricKeyTypeReverseMap>(symmetrickeytype_map());

    try {
        return map.at(in);
    } catch (...) {
        return crypto::SymmetricKeyType::Error;
    }
}

auto translate(proto::SymmetricMode in) noexcept -> crypto::SymmetricMode
{
    static const auto map = reverse_arbitrary_map<
        crypto::SymmetricMode,
        proto::SymmetricMode,
        SymmetricModeReverseMap>(symmetricmode_map());

    try {
        return map.at(in);
    } catch (...) {
        return crypto::SymmetricMode::Error;
    }
}
}  // namespace opentxs::crypto::internal

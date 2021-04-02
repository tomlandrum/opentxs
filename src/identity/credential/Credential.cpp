// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "identity/credential/Base.hpp"  // IWYU pragma: associated

#include "internal/identity/credential/Credential.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "opentxs/identity/CredentialType.hpp"
#include "util/Container.hpp"

namespace opentxs::identity::credential::internal
{
auto credentialrole_map() noexcept -> const CredentialRoleMap&
{
    static const auto map = CredentialRoleMap{
        {identity::CredentialRole::Error, proto::CREDROLE_ERROR},
        {identity::CredentialRole::MasterKey, proto::CREDROLE_MASTERKEY},
        {identity::CredentialRole::ChildKey, proto::CREDROLE_CHILDKEY},
        {identity::CredentialRole::Contact, proto::CREDROLE_CONTACT},
        {identity::CredentialRole::Verify, proto::CREDROLE_VERIFY},
    };

    return map;
}

auto credentialtype_map() noexcept -> const CredentialTypeMap&
{
    static const auto map = CredentialTypeMap{
        {identity::CredentialType::Error, proto::CREDTYPE_ERROR},
        {identity::CredentialType::HD, proto::CREDTYPE_HD},
        {identity::CredentialType::Legacy, proto::CREDTYPE_LEGACY},
    };

    return map;
}

auto keymode_map() noexcept -> const KeyModeMap&
{
    static const auto map = KeyModeMap{
        {identity::KeyMode::Error, proto::KEYMODE_ERROR},
        {identity::KeyMode::Null, proto::KEYMODE_NULL},
        {identity::KeyMode::Private, proto::KEYMODE_PRIVATE},
        {identity::KeyMode::Public, proto::KEYMODE_PUBLIC},
    };

    return map;
}

auto keyrole_map() noexcept -> const KeyRoleMap&
{
    static const auto map = KeyRoleMap{
        {identity::KeyRole::Error, proto::KEYROLE_ERROR},
        {identity::KeyRole::Auth, proto::KEYROLE_AUTH},
        {identity::KeyRole::Encrypt, proto::KEYROLE_ENCRYPT},
        {identity::KeyRole::Sign, proto::KEYROLE_SIGN},
    };

    return map;
}

auto translate(const identity::CredentialRole in) noexcept
    -> proto::CredentialRole
{
    try {
        return credentialrole_map().at(in);
    } catch (...) {
        return proto::CREDROLE_ERROR;
    }
}

auto translate(const identity::CredentialType in) noexcept
    -> proto::CredentialType
{
    try {
        return credentialtype_map().at(in);
    } catch (...) {
        return proto::CREDTYPE_ERROR;
    }
}

auto translate(const identity::KeyMode in) noexcept -> proto::KeyMode
{
    try {
        return keymode_map().at(in);
    } catch (...) {
        return proto::KEYMODE_ERROR;
    }
}

auto translate(const identity::KeyRole in) noexcept -> proto::KeyRole
{
    try {
        return keyrole_map().at(in);
    } catch (...) {
        return proto::KEYROLE_ERROR;
    }
}

auto translate(const proto::CredentialRole in) noexcept
    -> identity::CredentialRole
{
    static const auto map = reverse_arbitrary_map<
        identity::CredentialRole,
        proto::CredentialRole,
        CredentialRoleReverseMap>(credentialrole_map());

    try {
        return map.at(in);
    } catch (...) {
        return identity::CredentialRole::Error;
    }
}

auto translate(const proto::CredentialType in) noexcept
    -> identity::CredentialType
{
    static const auto map = reverse_arbitrary_map<
        identity::CredentialType,
        proto::CredentialType,
        CredentialTypeReverseMap>(credentialtype_map());

    try {
        return map.at(in);
    } catch (...) {
        return identity::CredentialType::Error;
    }
}

auto translate(const proto::KeyMode in) noexcept -> identity::KeyMode
{
    static const auto map = reverse_arbitrary_map<
        identity::KeyMode,
        proto::KeyMode,
        KeyModeReverseMap>(keymode_map());

    try {
        return map.at(in);
    } catch (...) {
        return identity::KeyMode::Error;
    }
}

auto translate(const proto::KeyRole in) noexcept -> identity::KeyRole
{
    static const auto map = reverse_arbitrary_map<
        identity::KeyRole,
        proto::KeyRole,
        KeyRoleReverseMap>(keyrole_map());

    try {
        return map.at(in);
    } catch (...) {
        return identity::KeyRole::Error;
    }
}

}  // namespace opentxs::identity::credential::internal

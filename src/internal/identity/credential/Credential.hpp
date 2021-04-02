// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>

#include "opentxs/core/Secret.hpp"  // IWYU pragma: keep
#include "opentxs/identity/Types.hpp"
#include "opentxs/identity/credential/Base.hpp"
#include "opentxs/identity/credential/Contact.hpp"
#include "opentxs/identity/credential/Key.hpp"
#include "opentxs/identity/credential/Primary.hpp"
#include "opentxs/identity/credential/Secondary.hpp"
#include "opentxs/identity/credential/Verification.hpp"

namespace opentxs::identity::credential::internal
{
using CredentialRoleMap =
    std::map<identity::CredentialRole, proto::CredentialRole>;
using CredentialRoleReverseMap =
    std::map<proto::CredentialRole, identity::CredentialRole>;
using CredentialTypeMap =
    std::map<identity::CredentialType, proto::CredentialType>;
using CredentialTypeReverseMap =
    std::map<proto::CredentialType, identity::CredentialType>;
using KeyModeMap = std::map<identity::KeyMode, proto::KeyMode>;
using KeyModeReverseMap = std::map<proto::KeyMode, identity::KeyMode>;
using KeyRoleMap = std::map<identity::KeyRole, proto::KeyRole>;
using KeyRoleReverseMap = std::map<proto::KeyRole, identity::KeyRole>;

auto credentialrole_map() noexcept -> const CredentialRoleMap&;
auto credentialtype_map() noexcept -> const CredentialTypeMap&;
auto keymode_map() noexcept -> const KeyModeMap&;
auto keyrole_map() noexcept -> const KeyRoleMap&;
auto translate(identity::CredentialRole in) noexcept -> proto::CredentialRole;
auto translate(identity::CredentialType in) noexcept -> proto::CredentialType;
auto translate(identity::KeyMode in) noexcept -> proto::KeyMode;
auto translate(identity::KeyRole in) noexcept -> proto::KeyRole;
auto translate(proto::CredentialRole in) noexcept -> identity::CredentialRole;
auto translate(proto::CredentialType in) noexcept -> identity::CredentialType;
auto translate(proto::KeyMode in) noexcept -> identity::KeyMode;
auto translate(proto::KeyRole in) noexcept -> identity::KeyRole;

struct Base : virtual public identity::credential::Base {
    virtual void ReleaseSignatures(const bool onlyPrivate) = 0;

#ifdef _MSC_VER
    Base() {}
#endif  // _MSC_VER
    ~Base() override = default;
};
struct Contact : virtual public Base,
                 virtual public identity::credential::Contact {

#ifdef _MSC_VER
    Contact() {}
#endif  // _MSC_VER
    ~Contact() override = default;
};
struct Key : virtual public Base, virtual public identity::credential::Key {
    virtual auto SelfSign(
        const PasswordPrompt& reason,
        const std::optional<OTSecret> exportPassword = {},
        const bool onlyPrivate = false) -> bool = 0;

#ifdef _MSC_VER
    Key() {}
#endif  // _MSC_VER
    ~Key() override = default;
};
struct Primary : virtual public Key,
                 virtual public identity::credential::Primary {

#ifdef _MSC_VER
    Primary() {}
#endif  // _MSC_VER
    ~Primary() override = default;
};
struct Secondary : virtual public Key,
                   virtual public identity::credential::Secondary {

#ifdef _MSC_VER
    Secondary() {}
#endif  // _MSC_VER
    ~Secondary() override = default;
};
struct Verification : virtual public Base,
                      virtual public identity::credential::Verification {
#ifdef _MSC_VER
    Verification() {}
#endif  // _MSC_VER
    ~Verification() override = default;
};
}  // namespace opentxs::identity::credential::internal

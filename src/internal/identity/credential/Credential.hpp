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
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs::identity::credential::internal
{
using CredentialRoleMap = std::map<CredentialRole, proto::CredentialRole>;
using CredentialRoleReverseMap =
    std::map<proto::CredentialRole, CredentialRole>;
using CredentialTypeMap = std::map<CredentialType, proto::CredentialType>;
using CredentialTypeReverseMap =
    std::map<proto::CredentialType, CredentialType>;

auto credentialrole_map() noexcept -> const CredentialRoleMap&;
auto credentialtype_map() noexcept -> const CredentialTypeMap&;
auto translate(CredentialRole in) noexcept -> proto::CredentialRole;
auto translate(CredentialType in) noexcept -> proto::CredentialType;
auto translate(proto::CredentialRole in) noexcept -> CredentialRole;
auto translate(proto::CredentialType in) noexcept -> CredentialType;

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

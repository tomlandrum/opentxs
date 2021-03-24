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
struct Base : virtual public identity::credential::Base {
    using CredentialRoleMap =
        std::map<identity::CredentialRole, proto::CredentialRole>;
    using CredentialRoleReverseMap =
        std::map<proto::CredentialRole, identity::CredentialRole>;
    using CredentialTypeMap =
        std::map<identity::CredentialType, proto::CredentialType>;
    using CredentialTypeReverseMap =
        std::map<proto::CredentialType, identity::CredentialType>;

    static const CredentialRoleMap credentialrole_map_;
    static const CredentialRoleReverseMap credentialrole_reverse_map_;
    static const CredentialTypeMap credentialtype_map_;
    static const CredentialTypeReverseMap credentialtype_reverse_map_;

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

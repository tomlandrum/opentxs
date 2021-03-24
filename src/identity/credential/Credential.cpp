// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "identity/credential/Base.hpp"  // IWYU pragma: associated

#include "internal/identity/credential/Credential.hpp"
#include "opentxs/identity/CredentialRole.hpp"
#include "util/Container.hpp"

namespace opentxs::identity::credential::internal
{
const Base::CredentialRoleMap Base::credentialrole_map_{
    {identity::CredentialRole::MasterKey, proto::CREDROLE_MASTERKEY},
    {identity::CredentialRole::ChildKey, proto::CREDROLE_CHILDKEY},
    {identity::CredentialRole::Contact, proto::CREDROLE_CONTACT},
    {identity::CredentialRole::Verify, proto::CREDROLE_VERIFY},
};
const Base::CredentialRoleReverseMap Base::credentialrole_reverse_map_{
    reverse_map(credentialrole_map_)};

const Base::CredentialTypeMap Base::credentialtype_map_{
    {identity::CredentialType::HD, proto::CREDTYPE_HD},
    {identity::CredentialType::Legacy, proto::CREDTYPE_LEGACY},
};
const Base::CredentialTypeReverseMap Base::credentialtype_reverse_map_{
    reverse_map(credentialtype_map_)};

}  // namespace opentxs::identity::credential::internal

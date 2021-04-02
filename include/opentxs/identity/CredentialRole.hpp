// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_CREDENTIAL_ROLE_HPP
#define OPENTXS_IDENTITY_CREDENTIAL_ROLE_HPP

#include "opentxs/identity/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace identity
{
enum class CredentialRole : std::uint8_t {
    Error = 0,
    MasterKey = 1,
    ChildKey = 2,
    Contact = 3,
    Verify = 4,
};

constexpr auto value(const CredentialRole in) noexcept
{
    return static_cast<std::uint8_t>(in);
}

}  // namespace identity
}  // namespace opentxs
#endif

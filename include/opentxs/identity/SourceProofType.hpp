// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_SOURCE_PROOF_TYPE_HPP
#define OPENTXS_IDENTITY_SOURCE_PROOF_TYPE_HPP

#include "opentxs/identity/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace identity
{
enum class SourceProofType : std::uint8_t {
    SelfSignature = 0,
    Signature = 1,
};
}  // namespace identity
}  // namespace opentxs
#endif

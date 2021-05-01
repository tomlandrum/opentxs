// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_PUSH_TYPE_HPP
#define OPENTXS_OTX_PUSH_TYPE_HPP

#include "opentxs/otx/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace otx
{
enum class OTXPushType : std::uint8_t {
    Error = 0,
    Nymbox = 1,
    Inbox = 2,
    Outbox = 3,
};
}  // namespace otx
}  // namespace opentxs
#endif

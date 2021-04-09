// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_LAST_REPLY_STATUS_HPP
#define OPENTXS_OTX_LAST_REPLY_STATUS_HPP

#include "opentxs/otx/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace otx
{
enum class LastReplyStatus : std::uint8_t {
    Invalid = 0,
    None = 1,
    MessageSuccess = 2,
    MessageFailed = 3,
    Unknown = 4,
    NotSent = 5,
};

constexpr auto value(const LastReplyStatus in) noexcept
{
    return static_cast<std::uint8_t>(in);
}
}  // namespace otx
}  // namespace opentxs
#endif

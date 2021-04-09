// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_PAYMENTWORKFLOWSTATE_HPP
#define OPENTXS_API_CLIENT_PAYMENTWORKFLOWSTATE_HPP

#include "opentxs/crypto/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace api
{
namespace client
{
enum class PaymentWorkflowState : std::uint8_t {
    Error = 0,
    Unsent = 1,
    Conveyed = 2,
    Cancelled = 3,
    Accepted = 4,
    Completed = 5,
    Expired = 6,
    Initiated = 7,
    Aborted = 8,
    Acknowledged = 9,
    Rejected = 10,
};
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif

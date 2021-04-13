// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONTACT_CONTACT_ITEM_ATTRIBUTE_HPP
#define OPENTXS_CONTACT_CONTACT_ITEM_ATTRIBUTE_HPP

#include "opentxs/contact/Types.hpp"  // IWYU pragma: associated

#include <cstdint>

namespace opentxs
{
namespace contact
{
enum class ContactItemAttribute : std::uint8_t {
    Error = 0,
    Active = 1,
    Primary = 2,
    Local = 3,
};
}  // namespace contact
}  // namespace opentxs
#endif

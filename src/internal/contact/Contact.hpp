// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>

#include "opentxs/contact/Types.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"

namespace opentxs::contact::internal
{
using ContactItemAttributeMap =
    std::map<contact::ContactItemAttribute, proto::ContactItemAttribute>;
using ContactItemAttributeReverseMap =
    std::map<proto::ContactItemAttribute, contact::ContactItemAttribute>;
using ContactSectionNameMap =
    std::map<contact::ContactSectionName, proto::ContactSectionName>;
using ContactSectionNameReverseMap =
    std::map<proto::ContactSectionName, contact::ContactSectionName>;

auto contactitemattribute_map() noexcept -> const ContactItemAttributeMap&;
auto contactsectionname_map() noexcept -> const ContactSectionNameMap&;
auto translate(const contact::ContactItemAttribute in) noexcept
    -> proto::ContactItemAttribute;
auto translate(const contact::ContactSectionName in) noexcept
    -> proto::ContactSectionName;
auto translate(const proto::ContactItemAttribute in) noexcept
    -> contact::ContactItemAttribute;
auto translate(const proto::ContactSectionName in) noexcept
    -> contact::ContactSectionName;
}  // namespace opentxs::contact::internal

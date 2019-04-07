// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/Identifier.hpp"

#include "BalanceTree.hpp"
#include "HDChain.hpp"

namespace opentxs
{
blockchain::implementation::HDChain* Factory::HDChain(
    const api::client::Blockchain& blockchain,
    const blockchain::implementation::BalanceTree& parent,
    const Identifier& accountid)
{
    return new blockchain::implementation::HDChain(
        blockchain, parent, accountid);
}
}  // namespace opentxs

namespace opentxs::blockchain::implementation
{
HDChain::HDChain(
    const api::client::Blockchain& blockchain,
    const BalanceTree& parent,
    const Identifier& accountid)
    : implementation::BalanceNode(blockchain, parent, accountid)
    , implementation::Deterministic(blockchain, parent, accountid)
	, callback_()
{
}

HDIndex HDChain::ExternalCount() const
{
    // TODO
    return 0;
}

HDIndex HDChain::InternalCount() const
{
    // TODO
    return 0;
}

bool HDChain::SetAddressAllocatedCallback(
    AddressAllocatedCallback&& callback) const
{
    callback_ = callback;
    // TODO
    return (callback_) ? true : false;
}
}  // namespace opentxs::blockchain::implementation

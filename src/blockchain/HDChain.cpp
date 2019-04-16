// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/core/identifier/Nym.hpp"

#include "BalanceTree.hpp"
#include "HDChain.hpp"

namespace opentxs
{
blockchain::HDChain* Factory::HDChain(
    const api::client::Blockchain& blockchain,
    const blockchain::BalanceTree& parent,
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
    const blockchain::BalanceTree& parent,
    const Identifier& accountid)
    : implementation::BalanceNode(blockchain, parent, accountid)
    , implementation::Deterministic(blockchain, parent, accountid)
    , callback_()
{
}

HDIndex HDChain::ExternalCount() const
{
    const auto account = blockchain_.Account(
        identifier::Nym::Factory(parent_.NymID()), accountid_);
    if (nullptr != account.get()) { return account->externalindex(); }
    return 0;
}

HDIndex HDChain::InternalCount() const
{
    const auto account = blockchain_.Account(
        identifier::Nym::Factory(parent_.NymID()), accountid_);
    if (nullptr != account.get()) { return account->internalindex(); }
    return 0;
}

bool HDChain::SetAddressAllocatedCallback(
    AddressAllocatedCallback&& callback) const
{
    callback_ = callback;

    return bool(callback_);
}
}  // namespace opentxs::blockchain::implementation

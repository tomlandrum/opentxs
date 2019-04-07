// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/Wallet.hpp"

#include "BalanceList.hpp"
#include "BalanceTree.hpp"

namespace opentxs
{
blockchain::implementation::BalanceList* Factory::BalanceList(
    const api::client::Manager& client,
    const proto::ContactItemType type)
{
    return new blockchain::implementation::BalanceList(client, type);
}
}  // namespace opentxs

namespace opentxs::blockchain::implementation
{
BalanceList::BalanceList(
    const api::client::Manager& client,
    const proto::ContactItemType type)
    : balances_{}
{
    const auto& nymids = client.Wallet().LocalNyms();
    for (const auto& nymid : nymids) {

        const auto& accountids = client.Blockchain().AccountList(nymid, type);
        std::shared_ptr<BalanceTree> balancetree(
            Factory::BalanceTree(client.Blockchain(), nymid, type, accountids));

        balances_.emplace(nymid, balancetree);
    }
}

bool BalanceList::SetNewAccountCallback(AccountAddedCallback&& callback) const
{
	callback_ = callback;
	
	return (callback_) ? true : false;
}
}  // namespace opentxs::blockchain::implementation

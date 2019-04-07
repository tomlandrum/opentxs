// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/Identifier.hpp"

#include "BalanceNode.hpp"

namespace opentxs::blockchain::implementation
{
BalanceNode::BalanceNode(
    const api::client::Blockchain& blockchain,
    const BalanceTree& parent,
	const Identifier& accountid)
    : blockchain_(blockchain)
    , parent_(parent)
	, accountid_(accountid)
{
}

bool BalanceNode::AssociateIncomingTransaction(
    const proto::BlockchainTransaction& transaction,
    const Subchain chain,
    const HDIndex index) const
{
	// TODO
    return false;
}

Data BalanceNode::GetLastBlockHash() const
{
    // TODO: return blockchain_.GetLastBlockHash();
    return {};
}

const BalanceTree& BalanceNode::Parent() const { return parent_; }

bool BalanceNode::SetLastBlockHash(const Data& hash) const
{
    // TODO: return blockchain_.SetLastBlockHash(hash);
    return false;
}

}  // namespace opentxs::blockchain::implementation

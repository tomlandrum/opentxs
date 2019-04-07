// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Proto.hpp"

#include <opentxs/blockchain/BalanceNode.hpp>
#include <opentxs/blockchain/BalanceTree.hpp>

namespace opentxs::blockchain::implementation
{
class BalanceNode : virtual public blockchain::BalanceNode
{
public:
    const BalanceTree& Parent() const override;

    Data GetLastBlockHash() const override;

    bool SetLastBlockHash(const Data& hash) const override;

    bool AssociateIncomingTransaction(
        const proto::BlockchainTransaction& transaction,
        const Subchain chain,
        const HDIndex index) const override;

    virtual ~BalanceNode() = default;

protected:
    BalanceNode(
        const api::client::Blockchain&,
        const BalanceTree&,
        const Identifier&);

private:
    const api::client::Blockchain& blockchain_;
    const BalanceTree& parent_;
    OTIdentifier accountid_;

    BalanceNode(const BalanceNode&) = delete;
    BalanceNode(BalanceNode&&) = delete;
    BalanceNode& operator=(const BalanceNode&) = delete;
    BalanceNode& operator=(BalanceNode&&) = delete;
};
}  // namespace opentxs::blockchain::implementation

// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include <opentxs/blockchain/BalanceList.hpp>
#include <opentxs/blockchain/BalanceTree.hpp>

namespace opentxs::blockchain::implementation
{
class BalanceList : public opentxs::blockchain::BalanceList
{
public:
    std::size_t GetAccountCount() const override;

    const blockchain::BalanceTree& Account(
        const std::size_t index) const override;

    bool SetNewAccountCallback(AccountAddedCallback&& callback) const override;

    ~BalanceList() override = default;

protected:
    friend opentxs::Factory;

    BalanceList(
        const api::client::Manager& client,
        const proto::ContactItemType type);

private:
    std::map<const OTNymID, std::shared_ptr<BalanceTree>> balances_;
    mutable AccountAddedCallback callback_;

    BalanceList() = delete;
    BalanceList(const BalanceList&) = delete;
    BalanceList(BalanceList&&) = delete;
    BalanceList& operator=(const BalanceList&) = delete;
    BalanceList& operator=(BalanceList&&) = delete;
};
}  // namespace opentxs::blockchain::implementation

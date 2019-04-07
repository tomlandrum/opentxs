// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "BalanceNode.hpp"

#include <opentxs/blockchain/Deterministic.hpp>

namespace opentxs::blockchain::implementation
{
class Deterministic : virtual public BalanceNode,
                      virtual public blockchain::Deterministic
{
public:
    HDIndex AllocateNext(const Subchain type) const override;
    Data Xpriv() const override;

    bool SetIndexUpdatedCallback(
        IndexUpdatedCallback&& callback) const override;

    virtual ~Deterministic() = default;

protected:
    mutable IndexUpdatedCallback callback_;

    Deterministic(
        const api::client::Blockchain& blockchain,
        const BalanceTree& parent,
        const Identifier& accountid);

private:
    Deterministic(const Deterministic&) = delete;
    Deterministic(Deterministic&&) = delete;
    Deterministic& operator=(const Deterministic&) = delete;
    Deterministic& operator=(Deterministic&&) = delete;
};
}  // namespace opentxs::blockchain::implementation

// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/Identifier.hpp"

#include "Deterministic.hpp"

namespace opentxs::blockchain::implementation
{
Deterministic::Deterministic(
    const api::client::Blockchain& blockchain,
    const blockchain::BalanceTree& parent,
	const Identifier& accountid)
    : implementation::BalanceNode(blockchain, parent, accountid)
	, callback_()
{
}

HDIndex Deterministic::AllocateNext(const Subchain type) const
{
    // TODO
    return 0;
}

bool Deterministic::SetIndexUpdatedCallback(
    IndexUpdatedCallback&& callback) const
{
    callback_ = callback;

    return bool(callback_);
}

Data Deterministic::Xpriv() const { return {}; }
}  // namespace opentxs::blockchain::implementation

// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include <opentxs/blockchain/HDChain.hpp>

namespace opentxs::blockchain::internal
{
struct BalanceTree {
    virtual bool AddHDChain(
        const opentxs::blockchain::HDChain& hdchain) const = 0;

    // TODO
    //    virtual bool AddImported(
    //        const implementation::Imported& imported) const = 0;
    //    virtual bool AddPaymentCodeChain(
    //        const implementation::PaymentCodeChain&) const = 0;

    virtual ~BalanceTree() = default;
};
}  // namespace opentxs::blockchain::internal

// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "Deterministic.hpp"

#include <opentxs/blockchain/HDChain.hpp>

namespace opentxs::blockchain::implementation
{
class HDChain : virtual public Deterministic, virtual public blockchain::HDChain
{
public:
    HDIndex ExternalCount() const override;
    HDIndex InternalCount() const override;

    bool SetAddressAllocatedCallback(
        AddressAllocatedCallback&& callback) const override;

    ~HDChain() = default;

protected:
    friend opentxs::Factory;

    HDChain(
        const api::client::Blockchain& blockchain,
        const blockchain::BalanceTree& parent,
        const Identifier& accountid);

private:
    mutable AddressAllocatedCallback callback_;

    HDChain(const HDChain&) = delete;
    HDChain(HDChain&&) = delete;
    HDChain& operator=(const HDChain&) = delete;
    HDChain& operator=(HDChain&&) = delete;
};
}  // namespace opentxs::blockchain::implementation
